#include "capture/ScreenEncoder.h"

#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <ui/DisplayInfo.h>

#include <gui/Surface.h>

#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/ICrypto.h>

#include "Configuration.h"
#include "utils/Logger.h"
#include "utils/ConvUtils.h"
#include "utils/MediaCodecUtils.h"

namespace android {

#define ALIGN(x, a) (((x) >> (a)) << (a))

CScreenEncoder::CScreenEncoder() : Thread(false)
{
    AUTO_LOG();

    m_screenshot_base  = 0;
    m_screenshot_src_w = 0;
    m_screenshot_src_h = 0;
    m_screenshot_req_w = 0;
    m_screenshot_req_h = 0;
    m_screenshot_s     = 0;
    m_screenshot_f     = 0;
    m_screenshot_size  = 0;
    memset(m_codec_name, 0, sizeof(m_codec_name));
    m_codec_index      = -1;
}

CScreenEncoder::~CScreenEncoder()
{
    AUTO_LOG();
}

void CScreenEncoder::ConfigReporter(CChannel *reporter)
{
    AUTO_LOG();

    m_reporter = reporter;
}

String8 CScreenEncoder::StartEncoder()
{
    AUTO_LOG();

    String8 ret = CreateMediaCodec();
    if (ret != "OK") {
        DWLOGE("Cannot create media codec !!!");
        return ret;
    }

    status_t err = GetEncoder()->start();
    if (err != NO_ERROR) {
        DWLOGE("Cannot start media codec !!! err(%d)", err);
        return String8("ERR_CODC");
    }

    err = run("DisplayWingsFeedBuffers");
    if (err != NO_ERROR) {
        DWLOGE("Cannot start encode thread !!! err(%d)", err);
        return String8("ERR_CODC");
    }

    return String8("OK");;
}

status_t CScreenEncoder::StopEncoder()
{
    AUTO_LOG();

    // Stop encode thread
    status_t err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop encoder thread !!! err(%d)", err);
        return err;
    }

    err = GetEncoder()->stop();
    if (err != NO_ERROR) {
        DWLOGE("Cannot start media codec !!! err(%d)", err);
    }

    err = GetEncoder()->release();
    if (err != OK) {
        DWLOGE("Cannot release media codec !!! err(%d)", err);
    }

    m_codec = NULL;
    return NO_ERROR;
}

String8 CScreenEncoder::CreateMediaCodec()
{
    AUTO_LOG();

    sp<AMessage> format = new AMessage;

    status_t err = GetAvailableCodec();
    if (err != NO_ERROR) {
        DWLOGE("There is not available codec !!! err(%d)", err);
        return String8("ERR_NO_CODC");
    }

    err = QueryColorFormat();
    if (err != NO_ERROR) {
        DWLOGE("Cannot query color format !!! err(%d)", err);
        return String8("ERR_NO_FORM");
    }

    // Create looper for media codec
    m_looper = new ALooper();
    if (m_looper == NULL) {
        DWLOGE("Cannot create looper for media codec !!!");
        return String8("ERR_CODC");
    }
    m_looper->setName("DisplayWings");
    m_looper->start();

    // Query display information
    DisplayInfo display_info;
    m_display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
    err = SurfaceComposerClient::getDisplayInfo(m_display, &display_info);
    if (err != NO_ERROR) {
        DWLOGE("Cannot query display information !!! err(%d)", err);
        return String8("ERR_CODC");
    }

    m_screenshot_src_w = display_info.w;
    m_screenshot_req_w = ALIGN(display_info.w, 4);
    m_screenshot_src_h = display_info.h;
    m_screenshot_req_h = ALIGN(display_info.h, 4);

    ResConf res_conf = Configuration::getInstance().Get()->res_conf;
    if (res_conf.h > 0 && res_conf.w > 0) {
        uint32_t bigger = res_conf.h >= res_conf.w ? res_conf.h : res_conf.w;
        uint32_t little = res_conf.h <= res_conf.w ? res_conf.h : res_conf.w;

        bigger = ALIGN(bigger, 4);
        little = ALIGN(little, 4);

        if (m_screenshot_src_w > m_screenshot_src_h) {
            m_screenshot_req_w = bigger;
            m_screenshot_req_h = little;
        } else {
            m_screenshot_req_w = little;
            m_screenshot_req_h = bigger;
        }
    } else if (m_screenshot_src_w > m_screenshot_src_h) {
        if (m_screenshot_src_w > 1280) {
            m_screenshot_req_w = 1280;
            double resolution_ratio = (double)m_screenshot_src_h / (double)m_screenshot_src_w;
            m_screenshot_req_h = m_screenshot_req_w * resolution_ratio;
            m_screenshot_req_h = ALIGN(m_screenshot_req_h, 4);
        }
    } else {
        if (m_screenshot_src_h > 1280) {
            m_screenshot_req_h = 1280;
            double resolution_ratio = (double)m_screenshot_src_w / (double)m_screenshot_src_h;
            m_screenshot_req_w = m_screenshot_req_h * resolution_ratio;
            m_screenshot_req_w = ALIGN(m_screenshot_req_w, 4);
        }
    }

    assert(m_codec_index >= 0);
    assert(m_codec_name[0] != 0);

    // Create h246 codec
    m_codec= MediaCodec::CreateByComponentName(m_looper, m_codec_name);
    //AString componentName;
    //m_codec->getName(&componentName);
    //char tmp[512]={0};
    //componentName.setTo(tmp);
    //DWLOGD("Created codec is %s", tmp);
    if (m_codec == NULL) {
        DWLOGE("Cannot create H264 codec by name - %s !!!", m_codec_name);
        return String8("ERR_NO_CODC");
    }

    format->setInt32("width", m_screenshot_req_w);
    format->setInt32("height", m_screenshot_req_h);
    format->setString("mime", MEDIA_CODEC_TYPE);
    format->setInt32("color-format", m_color_format);
    format->setInt32("bitrate", DEFAULT_BIT_RATE);
    format->setFloat("frame-rate", DEFAULT_FPS);
    format->setInt32("i-frame-interval", DEFAULT_I_FRAME_INTERVAL);

    // TODO: codec doesn't support some screen resolution, need to be fixed
    err = m_codec->configure(format, NULL, NULL, MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err == NO_ERROR) {
        DWLOGD("Succeeded confige codec.");
    } else {
        DWLOGE("Cannot config media codec !!! err(%d)", err);
        m_codec->stop();
        m_codec->release();

        DWLOGD("display width: %d", m_screenshot_src_w);
        DWLOGD("display height: %d", m_screenshot_src_h);
        DWLOGD("request width: %d", m_screenshot_req_w);
        DWLOGD("request height: %d", m_screenshot_req_h);
        DWLOGD("mime: %s", MEDIA_CODEC_TYPE);
        DWLOGD("color format: %d", m_color_format);
        DWLOGD("bit rate: %d", DEFAULT_BIT_RATE);
        DWLOGD("frame rate: %d", DEFAULT_FPS);
        DWLOGD("i frame interval: %d", DEFAULT_I_FRAME_INTERVAL);

        return String8("ERR_CONF");
    }

    // Check wthether agent can captuer device screen by specified resolution or not
    ProcessState::self()->startThreadPool();
    ScreenshotClient screenshot_client;
    err = codec::UpdateScreenCapture(m_display,
                                     &screenshot_client,
                                     m_screenshot_req_w,
                                     m_screenshot_req_h,
                                     m_screenshot_src_w,
                                     m_screenshot_src_h);
    if (err == NO_ERROR) {
        return String8("OK");
    } else {
        DWLOGE("Cannot capture device screen by sepcified resoultion !!! err(%d:%s)", err, strerror(err));
        return String8("ERR_CONF");
    }
}


status_t CScreenEncoder::GetAvailableCodec()
{
    status_t ret;
    char media_codec_name[CODEC_NAME_LEN] = {0};

    ret = MediaCodecList::getInstance()->findCodecByType(MEDIA_CODEC_TYPE, true, 0);
    if (ret < 0) {
        DWLOGE("Current platform does not support h264 codec !!! err(%d)", ret);
        memset(m_codec_name, 0, sizeof(m_codec_name));
        m_codec_index = -1;
        return ret;
    }

    codec::GetMediaCodecName(ret, media_codec_name);
    if (strlen(media_codec_name) <= 0) {
        DWLOGE("The codec name is empty !!!");
        goto ERR_LBL;
    }

    DWLOGI("Current codec is %s", media_codec_name);
    if (strlen(media_codec_name) < sizeof(m_codec_name)) {
        memset(m_codec_name, 0, sizeof(m_codec_name));
        strcpy(m_codec_name, media_codec_name);
        m_codec_index = ret;
        return NO_ERROR;
    }

ERR_LBL:
    memset(m_codec_name, 0, sizeof(m_codec_name));
    m_codec_index = -1;
    return UNKNOWN_ERROR;
}

status_t CScreenEncoder::QueryColorFormat()
{
    AUTO_LOG();

    Vector<uint32_t> codec_color_formats;

    assert(m_codec_index >= 0);
    assert(m_codec_name[0] != 0);

    codec_color_formats = codec::GetMediaCodecColorFormats(m_codec_index);
    int color_formats_count = codec_color_formats.size();
    bool found = false;
    for (int loop_index = 0; loop_index < color_formats_count; loop_index++) {
        int color_format_tmp = codec_color_formats.itemAt(loop_index);
        switch (color_format_tmp) {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
            m_color_format = color_format_tmp;
            found = true;
            break;
        default:
            break;
        }

        if (found)
            break;
    }

    if (found) {
        DWLOGD("Current color format is %d", m_color_format);
        return NO_ERROR;
    } else {
        DWLOGE("Cannot find supported color-format !!!");
        return UNKNOWN_ERROR;
    }
}


sp<MediaCodec> CScreenEncoder::GetEncoder()
{
    AUTO_LOG();

    return m_codec;
}

status_t CScreenEncoder::readyToRun()
{
    status_t err;

    AUTO_LOG();

    // Get input buffers
    err = m_codec->getInputBuffers(&m_codec_input_buffers);
    if (err != NO_ERROR) {
        DWLOGE("Cannot get input buffers !!! err(%d)", err);
        return err;
    }

    ProcessState::self()->startThreadPool();
    return NO_ERROR;
}

bool CScreenEncoder::threadLoop()
{
    status_t err;
    size_t index;
    uint8_t screenshot_w_tmp = 0;
    uint8_t screenshot_h_tmp = 0;
    int64_t current_time_us  = 0;
    struct timeval tv;
    static uint64_t frame_index = 0;
    ScreenshotClient screen_shot_client;

    uint32_t hsc_fps = Configuration::getInstance().Get()->fps;
    if (hsc_fps > 0) {
        double time_interval = 1000.0 / hsc_fps;
        struct timeval delay = {0, 0};
        delay.tv_sec  = 0;
        delay.tv_usec = time_interval * 1000;
        select(0, NULL, NULL, NULL, &delay);
    }

    Statistics::getInstance().Stamp(Statistics::CAP_TS, GetCurrentTimeStamp());
    err = codec::UpdateScreenCapture(m_display,
                                     &screen_shot_client,
                                     m_screenshot_req_w,
                                     m_screenshot_req_h,
                                     m_screenshot_src_w,
                                     m_screenshot_src_h);
    if (err != NO_ERROR) {
        Statistics::getInstance().Revert(Statistics::CAP_TS);
        DWLOGE("Cannot update display to get screen shot !!! err(%d:%s)", err, strerror(err));
        if (m_reporter != NULL) {
            String8 err_msg("[ERROR] Cannot capture screen !!!");
            m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
        }
        return false;
    } else {
        m_screenshot_base = screen_shot_client.getPixels();
        screenshot_w_tmp  = screen_shot_client.getWidth();
        screenshot_h_tmp  = screen_shot_client.getHeight();
        m_screenshot_s    = screen_shot_client.getStride();
        m_screenshot_f    = screen_shot_client.getFormat();
        m_screenshot_size = screen_shot_client.getSize();
#ifdef DW_DEBUG
        DumpRawData("screen_shot_raw", m_screenshot_base, m_screenshot_size);
#endif
    }

    if (Configuration::getInstance().Get()->record_frame_index)
        m_frame_index_file.Write(frame_index++);

    Statistics::getInstance().Stamp(Statistics::DIB_TS, GetCurrentTimeStamp());
    // TODO: if the inputbuffer is full-filled, so what is the error code?
    err = m_codec->dequeueInputBuffer(&index, -1ll);
    if (err != NO_ERROR) {
        Statistics::getInstance().Revert(Statistics::DIB_TS);
        DWLOGE("Cannot dequeue input buffers !!! err(%d)", err);
        if (m_reporter != NULL) {
            String8 err_msg("[ERROR] Cannot dequeue input buffers !!!");
            m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
        }
        return false;
    }

    uint8_t *image_data = m_codec_input_buffers.itemAt(index)->data();
    // TODO: need query color-format and set avalible value
    Statistics::getInstance().Stamp(Statistics::CVT_TS, GetCurrentTimeStamp());
    if (m_color_format == OMX_COLOR_FormatYUV420SemiPlanar) {
        ConvertRGB32ToNV12((const uint8_t *)m_screenshot_base,
                           image_data,
                           image_data + m_screenshot_req_w * m_screenshot_req_h,
                           m_screenshot_req_w,
                           m_screenshot_req_h,
                           m_screenshot_s << 2,
                           m_screenshot_req_w,
                           m_screenshot_req_w);
    } else if (m_color_format == OMX_COLOR_FormatYUV420Planar) {
        ConvertRGB32ToI420((const uint8_t *)m_screenshot_base,
                           image_data,
                           image_data + m_screenshot_req_w * m_screenshot_req_h,
                           image_data + m_screenshot_req_w * m_screenshot_req_h * 5 / 4,
                           m_screenshot_req_w,
                           m_screenshot_req_h,
                           m_screenshot_s << 2,
                           m_screenshot_req_w,
                           m_screenshot_req_w >> 1,
                           m_screenshot_req_w >> 1);
    } else {
        DWLOGE("The color-format does not supported yet !!! color-format(%d)", m_color_format);
        if (m_reporter != NULL) {
            String8 err_msg("[ERROR] The color-format does not supported yet !!!");
            m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
        }
        return false;
    }
#ifdef DW_DEBUG
    DumpRawData("screen_shot_yuv", image_data, m_codec_input_buffers.itemAt(index)->size());
#endif

    Statistics::getInstance().Stamp(Statistics::ENC_TS, GetCurrentTimeStamp());
    gettimeofday(&tv, NULL);
    current_time_us = (tv.tv_sec) * 1000 * 1000 + (tv.tv_usec);
    err = m_codec->queueInputBuffer(index,
                                    0,
                                    m_codec_input_buffers.itemAt(index)->size(),
                                    current_time_us,
                                    0);
    if (err != NO_ERROR) {
        DWLOGE("Cannot queue input buffers !!! err(%d)",err);
        if (m_reporter != NULL) {
            String8 err_msg("[ERROR] Cannot queue input buffers !!!");
            m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
        }
        return false;
    }

    return true;
}

}

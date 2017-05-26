#include "capture/ScreenEncoderConsumer.h"

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/foundation/AMessage.h>

#include "capture/Statistics.h"
#include "utils/Logger.h"
#include "utils/ConvUtils.h"

namespace android {

CScreenEncoderConsumer::CScreenEncoderConsumer(sp<MediaCodec> codec) : Thread(false)
{
    AUTO_LOG();

    m_codec = codec;
    // m_codec_output_buffers.Clear();

    // Deque output buffer
    m_deque_index = 0;
    m_deque_offset = 0;
    m_deque_size = 0;
    m_deque_presentationTimeUs = 0;
    m_deque_flags = 0;

    m_reporter = NULL;
}

CScreenEncoderConsumer::~CScreenEncoderConsumer()
{
    AUTO_LOG();
}

status_t CScreenEncoderConsumer::StartConsuming()
{
    AUTO_LOG();

    status_t err = run("DisplayWingsConsumer");
    if (err != NO_ERROR) {
        DWLOGE("Cannot start cosuming thread !!! err(%d)", err);
        return err;
    }

    return NO_ERROR;
}

status_t CScreenEncoderConsumer::StopConsuming()
{
    AUTO_LOG();

    // Stop encode thread
    status_t err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop cosuming thread !!! err(%d)", err);
        return err;
    }

    return NO_ERROR;
}

void CScreenEncoderConsumer::ConfigReporter(CChannel *reporter)
{
    AUTO_LOG();

    m_reporter = reporter;
}

status_t CScreenEncoderConsumer::readyToRun()
{
    AUTO_LOG();

    m_codec_output_buffers.clear();
    status_t err = m_codec->getOutputBuffers(&m_codec_output_buffers);
    if (err != NO_ERROR) {
        DWLOGE("Cannot get output buffers !!! err(%d)", err);
        return err;
    }

    return NO_ERROR;
}

bool CScreenEncoderConsumer::threadLoop()
{
    status_t err = NO_ERROR;
    int wrote_count = 0;
    int es_frame_size = 0;

    err = m_codec->dequeueOutputBuffer(&m_deque_index,
                                       &m_deque_offset,
                                       &m_deque_size,
                                       &m_deque_presentationTimeUs,
                                       &m_deque_flags,
                                       5000ll);
    if (err == NO_ERROR) {
        // NOTE: the following branch condition cannot be meeted, because the encoder always encodes screen
        if ((m_deque_flags & MediaCodec::BUFFER_FLAG_EOS) != 0) {
            // Not expecting EOS from SurfaceFlinger.  Go with it.
            DWLOGE("End of stream");
            return false;
        }

        // TODO: the following code should be abstracted as a reporter class
        if (m_reporter != NULL && m_reporter->IsActive()) {
            es_frame_size = m_codec_output_buffers.itemAt(m_deque_index)->size();
            Statistics::getInstance().Input(m_codec_output_buffers.itemAt(m_deque_index)->data(), es_frame_size);
            Statistics::getInstance().Accumulate(es_frame_size);
            wrote_count = m_reporter->Report(m_codec_output_buffers.itemAt(m_deque_index)->data(),
                                             es_frame_size);
            if (wrote_count != es_frame_size) {
                DWLOGE("Failed to write es data !!! wrote count: %d, es frame size:%d", wrote_count, es_frame_size);
                String8 err_msg("[ERROR] Failed to write es data !!!");
                m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
            }
        } else {
            DWLOGD("Reporter is: %p, or not active", m_reporter);
        }

        err = m_codec->releaseOutputBuffer(m_deque_index);
        if (err != NO_ERROR) {
            DWLOGE("Unable to release output buffer (err=%d)", err);
            if (m_reporter != NULL) {
                String8 err_msg("[ERROR] Unable to release output buffer !!!");
                m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
            }
            return false;
        }
     } else if (err == -EAGAIN) {
     } else if (err == INFO_FORMAT_CHANGED) {
        DWLOGW("Format has changed !!!");
        // TODO: Need encoder to be re-configured?
        sp<AMessage> newFormat;
        m_codec->getOutputFormat(&newFormat);
     } else if (err == INFO_OUTPUT_BUFFERS_CHANGED) {
        // Not expected for an encoder; handle it anyway.
        DWLOGW("Encoder buffers has changed");
        err = m_codec->getOutputBuffers(&m_codec_output_buffers);
        if (err != NO_ERROR) {
            DWLOGE("Unable to get output buffer !!! (err=%d)", err);
            if (m_reporter != NULL) {
                String8 err_msg("[ERROR] Unable to get output buffer !!!");
                m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
            }
            return false;
        }
     } else {
        DWLOGE("Unknown err !!! (err=%d)", err);
        if (m_reporter != NULL) {
            String8 err_msg("[ERROR] Unknown err !!!");
            m_reporter->Report((uint8_t *)err_msg.string(), err_msg.bytes(), CChannel::k_m_moni);
        }
        return false;
     }

    return true;
}

}

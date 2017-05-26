#include "utils/MediaCodecUtils.h"

#include "utils/Logger.h"

namespace android {

namespace codec {

void GetMediaCodecName(size_t index, char *codec_name)
{
    const char *src_codec_name = NULL;

    if (codec_name == NULL) {
        DWLOGE("Invalid paramter !!!");
        return;
    }

#if defined(AOSP21) || defined(AOSP22) || defined(AOSP23)
    sp<MediaCodecInfo> codec_info = MediaCodecList::getInstance()->getCodecInfo(index);
    src_codec_name = codec_info->getCodecName();
#else
    src_codec_name = MediaCodecList::getInstance()->getCodecName(index);
#endif
    strncpy(codec_name, src_codec_name, CODEC_NAME_LEN - 1);
}

Vector<uint32_t> GetMediaCodecColorFormats(size_t index)
{
    char codec_name[CODEC_NAME_LEN] = {0};
    Vector<uint32_t> codec_color_formats;
    uint32_t codec_flags;
    status_t ret = NO_ERROR;

    GetMediaCodecName(index, codec_name);
    if (strlen(codec_name) <= 0) {
        DWLOGE("Cannot get media codec name !!!");
        return codec_color_formats;
    }

#if  defined(AOSP21) || defined(AOSP22) || defined(AOSP23)
    sp<MediaCodecInfo> codec_info = MediaCodecList::getInstance()->getCodecInfo(index);
    // TODO: Pass MIME for getting capabilities
    sp<MediaCodecInfo::Capabilities> codec_cap = codec_info->getCapabilitiesFor("video/avc");

    // TODO: check codec_cap NULL
    codec_cap->getSupportedColorFormats(&codec_color_formats);
#elif defined(AOSP19)
    Vector<MediaCodecList::ProfileLevel> codec_profile_levels;
    ret = MediaCodecList::getInstance()->getCodecCapabilities(index,
                                                              codec_name,
                                                              &codec_profile_levels,
                                                              &codec_color_formats,
                                                              &codec_flags);
#elif defined(AOSP18) || defined(AOSP17)
    Vector<MediaCodecList::ProfileLevel> codec_profile_levels;
    ret = MediaCodecList::getInstance()->getCodecCapabilities(index,
                                                              codec_name,
                                                              &codec_profile_levels,
                                                              &codec_color_formats);
#else
    #error "Does not support current android OS !!!"
#endif

    if (ret != NO_ERROR) {
        DWLOGE("Cannot get color format !!! err(%d)", ret);
    }

    return codec_color_formats;
}


status_t UpdateScreenCapture(const sp<IBinder>& display,
                             ScreenshotClient *screen_shot_client,
                             uint32_t req_width,
                             uint32_t req_height,
                             uint32_t src_width,
                             uint32_t src_height)
{
    if (screen_shot_client == NULL) {
        DWLOGE("Invalid screen shot client !!!");
        return UNKNOWN_ERROR;
    }

#if  defined(AOSP21) || defined(AOSP22) || defined(AOSP23)
    Rect rec(0, 0, src_width, src_height);
    return screen_shot_client->update(display, rec, req_width, req_height, false);
#else
    return screen_shot_client->update(display, req_width, req_height);
#endif
}

} // end of namespace codec

} // end of namespace android

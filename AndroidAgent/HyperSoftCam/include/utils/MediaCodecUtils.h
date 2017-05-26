#ifndef _MEDIA_CODEC_UTILS_H_
#define _MEDIA_CODEC_UTILS_H_

#include <media/stagefright/MediaCodecList.h>
#include <gui/SurfaceComposerClient.h>

#include <stdint.h>
#include <sys/types.h>

#include <binder/IBinder.h>
#include <binder/IMemory.h>

namespace android {

namespace codec {

#define MEDIA_CODEC_TYPE ("video/avc")
#define CODEC_NAME_LEN   (512)

void GetMediaCodecName(size_t index, char *codec_name);

Vector<uint32_t> GetMediaCodecColorFormats(size_t index);

status_t UpdateScreenCapture(const sp<IBinder>& display,
                             ScreenshotClient *screen_shot_client,
                             uint32_t req_width,
                             uint32_t req_height,
                             uint32_t src_width,
                             uint32_t src_height);

} // end of namespace codec

} // end of namespace android

#endif

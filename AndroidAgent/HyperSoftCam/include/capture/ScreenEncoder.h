#ifndef _SCREEN_ENCODER_H_
#define _SCREEN_ENCODER_H_

#include <utils/Thread.h>
#include <utils/String8.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/ABuffer.h>

#include "com/Channel.h"
#include "Statistics.h"
#include "FrameIndexFile.h"

namespace android {

class CScreenEncoder : public Thread {
public:
    CScreenEncoder();
    ~CScreenEncoder();

    String8  StartEncoder();
    status_t StopEncoder();
    void ConfigReporter(CChannel *reporter);

    sp<MediaCodec> GetEncoder();

private:
    // (overrides Thread method)
    virtual status_t readyToRun();
    virtual bool threadLoop();
    // local private funtion
    status_t GetAvailableCodec();
    String8  CreateMediaCodec();
    status_t QueryColorFormat();

private:

    sp<ALooper>            m_looper;
    sp<MediaCodec>         m_codec;
    sp<IBinder>            m_display;
    Vector<sp<ABuffer> >   m_codec_input_buffers;
    const void            *m_screenshot_base;
    uint32_t               m_screenshot_req_w;
    uint32_t               m_screenshot_req_h;
    uint32_t               m_screenshot_src_w;
    uint32_t               m_screenshot_src_h;
    uint32_t               m_screenshot_s;
    uint32_t               m_screenshot_f;
    size_t                 m_screenshot_size;
    uint32_t               m_color_format;
    char                   m_codec_name[512];
    size_t                 m_codec_index;
    CChannel              *m_reporter;
    FrameIndexFile         m_frame_index_file;
};

}

#endif


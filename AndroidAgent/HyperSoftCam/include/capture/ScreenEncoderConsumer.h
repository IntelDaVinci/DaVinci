#ifndef _SCREEN_ENCODER_CONSUMER_H_
#define _SCREEN_ENCODER_CONSUMER_H_

#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/ABuffer.h>

#include "com/Channel.h"

namespace android {

class CScreenEncoderConsumer : public Thread {
public:
    CScreenEncoderConsumer(sp<MediaCodec> codec);
    ~CScreenEncoderConsumer();

    status_t StartConsuming();
    status_t StopConsuming();
    void ConfigReporter(CChannel *reporter);

private:
    // (overrides Thread method)
    virtual status_t readyToRun();
    virtual bool threadLoop();

private:
    sp<MediaCodec>       m_codec;
    Vector<sp<ABuffer> > m_codec_output_buffers;
    // Deque output buffer
    size_t               m_deque_index;
    size_t               m_deque_offset;
    size_t               m_deque_size;
    int64_t              m_deque_presentationTimeUs;
    uint32_t             m_deque_flags;
    CChannel            *m_reporter;
};

}

#endif


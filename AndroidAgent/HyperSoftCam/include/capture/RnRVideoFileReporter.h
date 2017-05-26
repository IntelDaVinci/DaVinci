#ifndef _RNR_VIDEO_FILE_REPORTER_H_
#define _RNR_VIDEO_FILE_REPORTER_H_

#include "com/Channel.h"
#include "Parser.h"

namespace android {

class RnRVideoFileReporter : public CChannel {
public:
    RnRVideoFileReporter(sp<Observer> observer, int fd_);
    ~RnRVideoFileReporter();

    bool IsActive();
    status_t Channelize();
    status_t Activate();
    void Inactivate();
    void ConfigPort(uint16_t port);
    int Report(uint8_t *data, int data_len);
    int Collect(uint8_t *data);
    virtual int GetFd();

private:
    bool m_is_active;
    Parser parser;
    int fd;
};

}

#endif


#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <unistd.h>

#include <utils/Thread.h>
#include <utils/String8.h>

namespace android {

class CChannel {
public:
    CChannel() {};
    virtual ~CChannel() {};

    virtual bool     IsActive()                          = 0;
    virtual status_t Channelize()                        = 0;
    virtual status_t Activate()                          = 0;
    virtual void     Inactivate()                        = 0;
    virtual void     ConfigPort(uint16_t port)           = 0;
    virtual int      Report(uint8_t *data, int data_len) = 0;
    virtual int      Report(uint8_t *data, int data_len, String8 channel_id)
    {
        return 1;
    }
    virtual int      Collect(uint8_t *data)              = 0;
    virtual int      GetFd()                             = 0;

public:
    const static String8 k_m_moni;
    const static String8 k_m_data;
};

}

#endif

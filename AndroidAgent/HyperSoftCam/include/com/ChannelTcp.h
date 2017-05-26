#ifndef _CHANNEL_TCP_H_
#define _CHANNEL_TCP_H_

#include "Channel.h"

#include <unistd.h>
#include <utils/String8.h>
#include <utils/KeyedVector.h>

namespace android {

class CChannelTcp : public CChannel {
public:
    CChannelTcp();
    ~CChannelTcp();

    virtual bool     IsActive();
    virtual status_t Channelize();
    virtual status_t Activate();
    virtual void     Inactivate();
    virtual void     ConfigPort(uint16_t port);
    virtual int      Report(uint8_t *data, int data_len);
    virtual int      Report(uint8_t *data, int data_len, String8 channel_id);
    virtual int      Collect(uint8_t *data);
    virtual int      GetFd();

private:
    status_t ActivateChannel(String8 channel_id);
    void     ResetConnections();

private:
    bool                      m_is_active;
    uint16_t                  m_port;
    int                       m_listenner_sock_fd;
    KeyedVector<String8, int> m_sock_connections;
};

}

#endif

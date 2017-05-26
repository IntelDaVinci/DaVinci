#ifndef _WIN_CHANNEL_H_
#define _WIN_CHANNEL_H_

#include "channel.h"

#include <Windows.h>

class CWinChannelImpl : public CChannelImpl {
public:
    CWinChannelImpl(CHANNEL_TYPE channel_type);
    ~CWinChannelImpl();

    virtual int ConnectTarget(const char *p2p_server_address, int p2p_server_port);
    virtual int SendData(const char *data_buffer, int data_len);
    virtual int ReceiveData(char *data_buffer, int data_len);
    virtual void Disconnect();

private:
    SOCKET       m_scoketClientSocket;
    CHANNEL_TYPE m_channel_type;
};

#endif

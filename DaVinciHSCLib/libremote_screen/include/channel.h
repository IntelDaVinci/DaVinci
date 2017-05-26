#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "mfxcommon.h"

enum NETWORK_CONNETION_STATUS {
    NETWORK_CONNECTION_CONNECTED,
    NETWORK_CONNECTION_STOPPING,
    NETWORK_CONNECTION_ERROR,
    NETWORK_CONNECTION_DISCONNECTED
};

enum CHANNEL_TYPE {
    CHANNEL_DATA,
    CHANNEL_CONTROL,
    CHANNEL_MONITOR
};

class CChannelImpl {
public:
    virtual ~CChannelImpl() {}
    virtual int ConnectTarget(const char *p2p_server_address, int p2p_server_port) = 0;
    virtual int SendData(const char *data_buffer, int data_len) = 0;
    virtual int ReceiveData(char *data_buffer, int data_len) = 0;
    virtual void Disconnect() = 0;
};

class CChannel : public CChannelImpl {
public:
    CChannel(CHANNEL_TYPE channel_type);
    ~CChannel();

    int ConnectTarget(const char *p2p_server_address, int p2p_server_port);
    int SendData(const char *data_buffer, int data_len);
    int ReceiveData(char *data_buffer, int data_len);
    void Disconnect();

    NETWORK_CONNETION_STATUS CurrentConnectionStatus();

private:
    CChannel(const CChannel& obj);
    CChannel& operator=(CChannel& obj);

private:
    NETWORK_CONNETION_STATUS      m_currentConnectionStatus;
    CChannelImpl                 *m_impl;
};

#endif

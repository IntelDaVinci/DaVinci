#include "channel.h"

#include "logger.h"

#if defined(_WIN32) || defined(_WIN64)
#include "win/win_channel_impl.h"
#else
#endif

CChannel::CChannel(CHANNEL_TYPE channel_type)
{
    AUTO_LOG();
    m_currentConnectionStatus = NETWORK_CONNECTION_DISCONNECTED;
#if defined(_WIN32) || defined(_WIN64)
    m_impl = new CWinChannelImpl(channel_type);
#else
#pragma error "Doesn't support current platform."
#endif
}

CChannel::~CChannel()
{
    AUTO_LOG();
    if (m_impl != 0)
        delete m_impl;
}

int CChannel::ConnectTarget(const char *p2p_server_address, int p2p_server_port)
{
    AUTO_LOG();
    int ret = -1;
    ret = m_impl->ConnectTarget(p2p_server_address, p2p_server_port);
    m_currentConnectionStatus = ret == 0 ? NETWORK_CONNECTION_CONNECTED : NETWORK_CONNECTION_ERROR;
    return ret;
}

int CChannel::SendData(const char *data_buffer, int data_len)
{
    AUTO_LOG();
    int ret = -1;
    ret = m_impl->SendData(data_buffer, data_len);

    if (ret <= 0) {
        if (m_currentConnectionStatus == NETWORK_CONNECTION_STOPPING)
            m_currentConnectionStatus = NETWORK_CONNECTION_DISCONNECTED;
        else
            m_currentConnectionStatus = NETWORK_CONNECTION_ERROR;
    }
    return ret;
}

int CChannel::ReceiveData(char *data_buffer, int data_len)
{
    int ret = -1;
    ret = m_impl->ReceiveData(data_buffer, data_len);

    if (ret <= 0) {
        if (m_currentConnectionStatus == NETWORK_CONNECTION_STOPPING)
            m_currentConnectionStatus = NETWORK_CONNECTION_DISCONNECTED;
        else
            m_currentConnectionStatus = NETWORK_CONNECTION_ERROR;
    }
    return ret;
}

void CChannel::Disconnect()
{
    AUTO_LOG();
    m_currentConnectionStatus = NETWORK_CONNECTION_STOPPING;
    m_impl->Disconnect();
    m_currentConnectionStatus = NETWORK_CONNECTION_DISCONNECTED;
}

NETWORK_CONNETION_STATUS CChannel::CurrentConnectionStatus()
{
    return m_currentConnectionStatus;
}

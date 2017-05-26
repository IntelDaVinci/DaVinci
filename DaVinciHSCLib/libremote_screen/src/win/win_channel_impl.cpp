#include "win/win_channel_impl.h"

#include "logger.h"

#include <fstream>

#pragma comment(lib, "ws2_32.lib")

CWinChannelImpl::CWinChannelImpl(CHANNEL_TYPE channel_type)
{
    AUTO_LOG();
    m_scoketClientSocket = INVALID_SOCKET;
    m_channel_type       = channel_type;
}

CWinChannelImpl::~CWinChannelImpl()
{
    AUTO_LOG();

    Disconnect();
}

int CWinChannelImpl::ConnectTarget(const char *p2p_server_address, int p2p_server_port)
{
    WSADATA  Ws;
    int Ret = 0;
    struct sockaddr_in ServerAddr;

    AUTO_LOG();

    if (WSAStartup(MAKEWORD(2,2), &Ws) != 0)
        return -1;

    m_scoketClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_scoketClientSocket == INVALID_SOCKET) {
        LOGE("Cannot create socket!!!");
        WSACleanup();
        return -1;
    }
    char enable_tcp_nodelay = 1;
    Ret = setsockopt(m_scoketClientSocket, IPPROTO_TCP, TCP_NODELAY, &enable_tcp_nodelay, sizeof(char));
    if (Ret != 0) {
        LOGE("Cannot enable TCP_NODELAY !!! err(%d)", WSAGetLastError());
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr(p2p_server_address);
    ServerAddr.sin_port = htons((unsigned short)p2p_server_port);
    memset(ServerAddr.sin_zero, 0x00, sizeof(ServerAddr.sin_zero));

    Ret = connect(m_scoketClientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
    if (Ret == SOCKET_ERROR) {
        LOGE("Cannot connect to remote target(ip:%s, port:%d) !!!", p2p_server_address, p2p_server_port);
        closesocket(m_scoketClientSocket);
        m_scoketClientSocket = INVALID_SOCKET;
        WSACleanup();
        return -1;
    }

    char channel_id[6] = {0};
    if (m_channel_type == CHANNEL_DATA) {
        strcpy_s(channel_id, "DATA\n");
    } else if (m_channel_type == CHANNEL_CONTROL) {
        strcpy_s(channel_id, "CONT\n");
    } else if (m_channel_type == CHANNEL_MONITOR) {
        strcpy_s(channel_id, "MONI\n");
    }
    Ret = SendData(channel_id, strlen(channel_id));
    if (Ret <= 0) {
        LOGE("Cannot send channel id to target device.");
        Disconnect();
        return -1;
    }

    char channel_response[6] = {0};
    Ret = ReceiveData(channel_response, sizeof(channel_response));
    if (Ret <= 0) {
        LOGE("Cannot receive channel id from target device.");
        Disconnect();
        return -1;
    }
    if (strcmp(channel_response, channel_id) != 0) {
        LOGE("Failed setting channel id.");
        Disconnect();
        return -1;
    }

    return 0;
}

int CWinChannelImpl::SendData(const char *data_buffer, int data_len)
{
    int ret = -1;
    ret = send(m_scoketClientSocket, data_buffer, data_len, 0);

    if (ret == SOCKET_ERROR ) {
        LOGE("Socket sent error. Error code:(%d)", WSAGetLastError());
    } else if (ret == 0) {
        LOGI("Socket has been closed.");
    }

    return ret;
}

int CWinChannelImpl::ReceiveData(char *data_buffer, int data_len)
{
    int ret = -1;
    ret = recv(m_scoketClientSocket, data_buffer, data_len, 0);

#if defined(DUMP_STREAM)
    if (ret > 0) {
        std::ofstream of("recv.h264", std::ios::binary | std::ios::app);
        of.write(data_buffer, data_len);
        of.close();
    }
#endif

    if (ret == SOCKET_ERROR ) {
        CLogger::Instance()->Log(CLogger::LOG_ERRO, "Socket received error. Error code:(%d)", WSAGetLastError());
    } else if (ret == 0) {
        CLogger::Instance()->Log(CLogger::LOG_INFO, "Socket has been closed.");
    }

    return ret;
}

void CWinChannelImpl::Disconnect()
{
    AUTO_LOG();
    if (m_scoketClientSocket != INVALID_SOCKET) {
        if (shutdown(m_scoketClientSocket, 2) == SOCKET_ERROR) {
            LOGE("Cannot close TCP connection. Error code:(%d)", WSAGetLastError());
        }
        if (closesocket(m_scoketClientSocket) == SOCKET_ERROR) {
            LOGE("Cannot close socket. Error code:(%d)", WSAGetLastError());
        }
        m_scoketClientSocket = INVALID_SOCKET;
        WSACleanup();
    }
}

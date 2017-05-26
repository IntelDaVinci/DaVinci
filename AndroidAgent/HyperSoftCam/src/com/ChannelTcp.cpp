#include "com/ChannelTcp.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "utils/Logger.h"

namespace android {

CChannelTcp::CChannelTcp()
{
    AUTO_LOG();

    m_port               = 0;
    m_listenner_sock_fd  = -1;
    m_is_active          = false;
}

CChannelTcp::~CChannelTcp()
{
    AUTO_LOG();

    Inactivate();
}

bool CChannelTcp::IsActive()
{
    return m_is_active;
}

void CChannelTcp::ConfigPort(uint16_t port)
{
    AUTO_LOG();

    m_port = port;
}

status_t CChannelTcp::Channelize()
{
    struct sockaddr_in address;
    int                reuse      = 1;
    int                sock_flags = 0;

    AUTO_LOG();

    memset((char *)&address, 0, sizeof(struct sockaddr_in));
    m_listenner_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenner_sock_fd == -1) {
        DWLOGE("Cannot create listen socket !!! err(%d:%s)", errno, strerror(errno));
        return UNKNOWN_ERROR;
    }

    DWLOGD("Socket port is %d", m_port);
    address.sin_family = AF_INET;
    address.sin_port = htons(m_port);
    address.sin_addr.s_addr = INADDR_ANY;
    bzero(&(address.sin_zero), 8);
    setsockopt(m_listenner_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    if (bind(m_listenner_sock_fd, (struct sockaddr *)&address, sizeof(struct sockaddr)) == -1) {
        DWLOGE("Failed to bind socket on port %u !!! err(%d:%s)", m_port, errno, strerror(errno));
        return UNKNOWN_ERROR;
    }

    DWLOGD("Listen on socket %d", m_listenner_sock_fd);

    if (listen(m_listenner_sock_fd, 4) == -1) {
        DWLOGE("Failed to listen on socket !!! err(%d:%s)", errno, strerror(errno));
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}


void CChannelTcp::Inactivate()
{
    AUTO_LOG();

    if (m_listenner_sock_fd > 0) {
        DWLOGD("Close listen socket !!!");
        shutdown(m_listenner_sock_fd, SHUT_RDWR);
        close(m_listenner_sock_fd);
        m_listenner_sock_fd = -1;
    }

    ResetConnections();
    m_sock_connections.clear();

    m_is_active = false;
}

void CChannelTcp::ResetConnections()
{
    AUTO_LOG();
    size_t connections_count = m_sock_connections.size();
    for (size_t i = 0; i < connections_count; i++) {
        shutdown(m_sock_connections.valueAt(i), SHUT_RDWR);
        close(m_sock_connections.valueAt(i));
        m_sock_connections.replaceValueAt(i, -1);
    }
}

status_t CChannelTcp::ActivateChannel(String8 channel_id)
{
    struct sockaddr_in peer;
    socklen_t          size = sizeof(struct sockaddr_in);
    int                flag = 1;
    char               buffer[512] = {0};

    AUTO_LOG();
    DWLOGD("Waiting for connection from client for channel(%s)", channel_id.string());

    if (CChannel::k_m_moni != channel_id && CChannel::k_m_data != channel_id) {
        DWLOGE("Invalid channel ID");
        return UNKNOWN_ERROR;
    }

    int sock_fd = accept(m_listenner_sock_fd, (struct sockaddr *)&peer, &size);
    if (sock_fd == -1) {
        DWLOGE("Cannot accept client connection !!! err(%d:%s)", errno, strerror(errno));
        m_is_active = false;
        return UNKNOWN_ERROR;
    }

    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        DWLOGE("Cannot receive data from client !!! err(%d:%s)", errno, strerror(errno));
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        sock_fd = -1;

        return errno;
    }
    DWLOGD("Received buffer from host - (%s)", buffer);

    if (String8(buffer) == channel_id) {
        bytes = send(sock_fd, String8(buffer).string(), String8(buffer).bytes(), 0);
        if (bytes == String8(buffer).bytes()) {
            setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
            m_sock_connections.add(channel_id, sock_fd);
        } else {
            DWLOGE("Cannot send data to client !!! err(%d:%s)", errno, strerror(errno));
            shutdown(sock_fd, SHUT_RDWR);
            close(sock_fd);
            sock_fd = -1;

            return errno;
        }
    }

    return NO_ERROR;
}

status_t CChannelTcp::Activate()
{
    struct sockaddr_in peer;
    socklen_t          size = sizeof(struct sockaddr_in);
    int                flag = 1;
    char               buffer[512] = {0};
    status_t           ret  = UNKNOWN_ERROR;

    AUTO_LOG();
    DWLOGD("Waiting for connection from client");

    // Close all connections first
    ResetConnections();

    ret = ActivateChannel(CChannel::k_m_moni);
    if (ret != NO_ERROR) {
        DWLOGE("Cannot activate monitor channel");
        return ret;
    }

    ret = ActivateChannel(CChannel::k_m_data);
    if (ret != NO_ERROR) {
        DWLOGE("Cannot activate data channel");
        return ret;
    }

    DWLOGD("Connected with client");
    m_is_active = true;
    return NO_ERROR;
}


int CChannelTcp::Report(uint8_t *data, int data_len, String8 channel_id)
{
    AUTO_LOG();

    int ret = -1;

    int sock_fd = m_sock_connections.valueFor(channel_id);
    if (sock_fd <= 0) {
        DWLOGE("Cannot find the socket of %s", channel_id.string());
        m_is_active = false;
        return ret;
    }

    ret = send(sock_fd, data, data_len, MSG_NOSIGNAL);
    if (ret == -1) {
        DWLOGE("Cannot report data !!! err(%d:%s)", errno, strerror(errno));
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        m_sock_connections.replaceValueFor(channel_id, -1);
        m_is_active = false;
    }

    return ret;
}

int CChannelTcp::Report(uint8_t *data, int data_len)
{
    AUTO_LOG();

    return Report(data, data_len, CChannel::k_m_data);
}

int CChannelTcp::Collect(uint8_t *data)
{
    return 0;
}

int CChannelTcp::GetFd()
{
    struct sockaddr_in peer;
    socklen_t          size = sizeof(struct sockaddr_in);
    int                flag = 1;

    int fd = accept(m_listenner_sock_fd, (struct sockaddr *)&peer, &size);
    if (fd != -1) {
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag));
    }
    return fd;
}

}

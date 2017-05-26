#include "replay/MonkeyBroker.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <errno.h>

#include "utils/Logger.h"

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(MonkeyBroker)

MonkeyBroker::MonkeyBroker()
{
    m_monkey_channel = ENOTSOCK;
    m_initialized    = false;
}

MonkeyBroker::~MonkeyBroker()
{
    if (m_monkey_channel != ENOTSOCK) {
        close(m_monkey_channel);
    }
}

void MonkeyBroker::InitializeEnvironment()
{
    struct sockaddr_in server_address;
    int                reuse      = 1;
    int                sock_flags = 0;
    int                client_skt = socket(AF_INET, SOCK_STREAM, 0);
    int                ret        = -1;

    memset((char *)&server_address, 0, sizeof(struct sockaddr_in));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // TODO: The port should be defined as a constant variable
    server_address.sin_port = htons(12321);
    memset(server_address.sin_zero, 0, 8);

    m_monkey_channel = socket(AF_INET, SOCK_STREAM, 0);
    if (m_monkey_channel < 0) {
        DWLOGE("Cannot create socket. (%s)", strerror(errno));
        m_initialized = false;
        return;
    }

    ret = connect(m_monkey_channel,
                  (struct sockaddr *)&server_address,
                  sizeof(server_address));
    if (ret < 0) {
        DWLOGE("Cannot connect to monkey. (%s)", strerror(errno));
        m_initialized = false;
        close(m_monkey_channel);
        return;
    }

    m_initialized = true;
}

bool MonkeyBroker::ExecuteSyncCommand(String8 command)
{
    int ret = 0;

    if (command.length() <= 0) {
        DWLOGE("Invalid command");
        return false;
    }

    if (!m_initialized) {
        InitializeEnvironment();
    }

    if (!m_initialized) {
        DWLOGE("Cannot initialize monkey environment");
        return false;
    }

    command += String8("\n");
    DWLOGD("Send Buffer: (%s)", command.string());
    ret = send(m_monkey_channel,
               command.string(),
               command.length(),
               0
               );
    if (ret <= 0) {
        DWLOGE("Error to send message! %s\n", strerror(errno));
        close(m_monkey_channel);
        m_monkey_channel = ENOTSOCK;
        return false;
    }

    char recv_buffer[128] = {0};
    ret = recv(m_monkey_channel,
               recv_buffer,
               sizeof(recv_buffer) - 1,
               0
               );
    if (ret <= 0) {
        DWLOGE("Error to receive message! %s\n", strerror(errno));
        close(m_monkey_channel);
        m_monkey_channel = ENOTSOCK;
        return false;
    }
    DWLOGD("Received Buffer: (%s)", recv_buffer);

    if (recv_buffer[0] == 'O' && recv_buffer[1] == 'K') {
        return true;
    } else {
        return false;
    }
}

}

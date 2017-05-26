#include "capture/RnRVideoFileReporter.h"

#include "utils/Logger.h"

namespace android {

RnRVideoFileReporter::RnRVideoFileReporter(sp<Observer> observer, int fd_) : parser(observer), fd(fd_)
{
    AUTO_LOG();

    m_is_active = false;
}

RnRVideoFileReporter::~RnRVideoFileReporter()
{
    AUTO_LOG();
}

bool RnRVideoFileReporter::IsActive()
{
    return m_is_active;
}

void RnRVideoFileReporter::ConfigPort(uint16_t port)
{
    AUTO_LOG();
}

status_t RnRVideoFileReporter::Channelize()
{
    AUTO_LOG();

    return NO_ERROR;
}

void RnRVideoFileReporter::Inactivate()
{
    AUTO_LOG();
    m_is_active = false;
}

status_t RnRVideoFileReporter::Activate()
{
    AUTO_LOG();

    m_is_active = true;
    return NO_ERROR;
}

int RnRVideoFileReporter::Report(uint8_t *data, int data_len)
{
    int ret = -1;

    parser.Input(data, data_len);
    ret = write(fd, data, data_len);
    if (ret == -1) {
        DWLOGE("Cannot report data !!! err(%d:%s)", errno, strerror(errno));
        m_is_active = false;
    }

    return ret;
}

int RnRVideoFileReporter::Collect(uint8_t *data)
{
    AUTO_LOG();
    return 0;
}

int RnRVideoFileReporter::GetFd()
{
    AUTO_LOG();
    return fd;
}

}


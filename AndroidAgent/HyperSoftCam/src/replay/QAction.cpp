#include "replay/QAction.h"

#include "replay/MonkeyBroker.h"
#include "utils/Logger.h"

namespace android {

QAction::QAction()
{
    m_action_time_stamp = 0;
}

QAction::~QAction()
{
    m_action_time_stamp = 0;
    m_command.clear();
}

bool QAction::Execute()
{
    bool ret = false;
    if (m_command.length() > 0) {
        ret = MonkeyBroker::getInstance().ExecuteSyncCommand(m_command);
        if (!ret)
            DWLOGE("Failed executing command (%s)", m_command.string());
    }
    return ret;
}

void QAction::SetActionTimeStamp(uint64_t action_time_stamp)
{
    AUTO_LOG();

    DWLOGD("action time stemp: %llu", action_time_stamp);
    m_action_time_stamp = action_time_stamp;
}

uint64_t QAction::GetActionTimeStamp()
{
    AUTO_LOG();
    DWLOGD("action time stemp: %llu", m_action_time_stamp);
    return m_action_time_stamp;
}

}

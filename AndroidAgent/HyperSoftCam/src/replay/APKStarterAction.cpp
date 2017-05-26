#include "replay/APKStarterAction.h"

#include <stdlib.h>

#include "utils/Logger.h"

namespace android {

APKStarterAction::APKStarterAction()
{
}

APKStarterAction::~APKStarterAction()
{
}

bool APKStarterAction::Execute()
{
    if (m_command.isEmpty()) {
        DWLOGE("There is no command to be executed");
        return false;
    }

    int res = system(m_command.string());
    if (res != 0) {
        DWLOGE("Failed executing installer action(%s): %d", m_command.string(), res);
        return false;
    }

    return true;
}

void APKStarterAction::SetAPKStarterAction(
        APKStarterIndicator action_indicator,
        String8             package_name,
        String8             activity_name
        )
{
    if (action_indicator == STAT_APP_INDICATOR) {
        m_command = String8("am start ") +
                    String8("-W ")        +
                    String8("\"")        +
                    package_name         +
                    String8("/")         +
                    activity_name        +
                    String8("\"");
    } else if (action_indicator == STOP_APP_INDICATOR) {
        m_command = String8("am force-stop ") + package_name;
    }
}

}

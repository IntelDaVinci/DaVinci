#include "replay/InstallerAction.h"

#include <stdlib.h>

#include "utils/Logger.h"
#include "utils/StringUtil.h"

namespace android {

InstallerAction::InstallerAction()
{
}

InstallerAction::~InstallerAction()
{
}

bool InstallerAction::Execute()
{
    if (m_command.isEmpty()) {
        DWLOGE("There is no command to be executed");
        return false;
    }

    int res = system(m_command.string());
    if (res != 0 && StringUtil::startsWith(m_command, "pm install")) {
        DWLOGE("Failed executing installer action(%s): %d",
               m_command.string(),
               res
               );
        return false;
    }

    return true;
}

void InstallerAction::SetInstallerAction(
        InstallActionIndicator action_indicator,
        String8                apk_name
        )
{
    m_command += "pm ";

    if (action_indicator == INSTALL_INDICATOR) {
        m_command += "install -r ";
    } else if (action_indicator == UNINSTALL_INDICATOR) {
        m_command += "uninstall ";
    }

    m_command += apk_name;
}

}

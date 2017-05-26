#include "replay/ShellAction.h"

#include <stdlib.h>

#include "utils/Logger.h"

namespace android {

ShellAction::ShellAction()
{
}

ShellAction::~ShellAction()
{
}

bool ShellAction::Execute()
{
    if (m_command.isEmpty()) {
        DWLOGE("There is no shell command to be executed");
        return false;
    }

    int res = system(m_command.string());
    if (-1 != res && 127 != res) {
        DWLOGE("Cannot execute shell command");
    }

    return true;
}

void ShellAction::SetShellCommand(String8 shell_command_path,
                                  String8 shell_command_parameters)
{
    if (shell_command_path.isEmpty())
        shell_command_path = "/system/bin/sh";

    m_command = shell_command_path + " " + shell_command_parameters;
}

}

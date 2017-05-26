#ifndef _SHELL_ACTION_H_
#define _SHELL_ACTION_H_

#include "QAction.h"

#include <utils/String8.h>

namespace android {

class ShellAction : public QAction {
public:
    ShellAction();
    virtual ~ShellAction();

    virtual bool Execute();

    void SetShellCommand(
            String8 shell_command_path,
            String8 shell_command_parameters
            );
};

}

#endif

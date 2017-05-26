#ifndef _INSTALLER_ACTION_H_
#define _INSTALLER_ACTION_H_

#include "QAction.h"

#include <utils/String8.h>

namespace android {

typedef enum {
    INSTALL_INDICATOR = 0,
    UNINSTALL_INDICATOR
} InstallActionIndicator;

class InstallerAction : public QAction {
public:
    InstallerAction();
    virtual ~InstallerAction();

    virtual bool Execute();

    void SetInstallerAction(
            InstallActionIndicator action_indicator,
            String8 apk_name
            );
};

}

#endif

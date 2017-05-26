#ifndef _APK_STARTER_ACTION_H_
#define _APK_STARTER_ACTION_H_

#include "QAction.h"

#include <utils/String8.h>

namespace android {

typedef enum {
    STAT_APP_INDICATOR = 0,
    STOP_APP_INDICATOR
} APKStarterIndicator;

class APKStarterAction : public QAction {
public:
    APKStarterAction();
    virtual ~APKStarterAction();

    virtual bool Execute();

    void SetAPKStarterAction(APKStarterIndicator action_indicator,
                             String8             package_name,
                             String8             activity_name);
};

}

#endif

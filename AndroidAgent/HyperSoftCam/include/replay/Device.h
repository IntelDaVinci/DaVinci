#ifndef _MONKEY_DEVICE_H_
#define _MONKEY_DEVICE_H_

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <utils/Singleton.h>

#include "replay/QAction.h"

namespace android {

typedef enum {
    ORIENTATION_0 = 0,
    ORIENTATION_90,
    ORIENTATION_180,
    ORIENTATION_270
} OrientationIndicator;

typedef struct {
    int a;
    int b;
} BiUnit;

class Device : public Singleton<Device> {
friend class Singleton<Device>;

protected:
    Device();
    virtual ~Device();

public:
    bool ExecuteAction(sp<QAction> action);
    BiUnit GetResolution();
    OrientationIndicator GetOrientation();

private:
    sp<IBinder>          m_display;
    DisplayInfo          m_display_info;
};

}

#endif

#include "replay/Device.h"

#include "utils/Logger.h"

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(Device)

Device::Device()
{
    memset(&m_display_info, 0, sizeof(m_display_info));
}

Device::~Device()
{
}

bool Device::ExecuteAction(sp<QAction> action)
{
    if (action == NULL)
        return false;

    bool ret = action->Execute();
    return ret;
}

BiUnit Device::GetResolution()
{
    BiUnit   resolution;
    status_t err;

    if (m_display == NULL) {
        m_display = SurfaceComposerClient::getBuiltInDisplay(
                            ISurfaceComposer::eDisplayIdMain
                            );
        err       = SurfaceComposerClient::getDisplayInfo(
                            m_display,
                            &m_display_info
                            );
        if (err != NO_ERROR)
            DWLOGE("Cannot query display information !!! err(%d)", err);
    }

    resolution.a = m_display_info.w;
    resolution.b = m_display_info.h;
    return resolution;
}

OrientationIndicator Device::GetOrientation()
{
    if (m_display != NULL) {
        SurfaceComposerClient::getDisplayInfo(m_display, &m_display_info);
        return (OrientationIndicator)(m_display_info.orientation);
    } else {
        return ORIENTATION_0;
    }
}

}

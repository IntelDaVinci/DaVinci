#include "capture/StreamHandler.h"

#include <sys/stat.h>

#include "capture/DisplayManager.h"
#include "Configuration.h"
#include "utils/Logger.h"

extern bool gExit;

namespace android {

bool StreamHandler::Handle()
{
    AUTO_LOG();

    HSCConf* hsc_conf = Configuration::getInstance().Get();
    if (!hsc_conf->stream_conf.is) {
        if (m_next_handler != NULL) {
            return m_next_handler->Handle();
        } else {
            return false;
        }
    } else {
        sp<CDWManager> dw_manager(new CDWManager(hsc_conf->stream_conf.port));
        status_t err = dw_manager->Start();
        if (err != NO_ERROR) {
            DWLOGE("Cannot start display wings manager !!! err(%d)", err);
            return false;
        }

        while (!gExit) {
            usleep(1000);
        }

        dw_manager->Stop();
        return true;
    }
}

}

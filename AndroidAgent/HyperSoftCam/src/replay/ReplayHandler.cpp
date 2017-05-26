#include "replay/ReplayHandler.h"

#include "Configuration.h"
#include "replay/MonkeyService.h"
#include "replay/Device.h"
#include "replay/QAction.h"
#include "replay/QScript.h"

#include "utils/ConvUtils.h"
#include "utils/Logger.h"

namespace android {

bool ReplayHandler::Handle()
{
    AUTO_LOG();

    HSCConf* hsc_conf = Configuration::getInstance().Get();
    if (!hsc_conf->replay_conf.is) {
        if (m_next_handler != NULL) {
            return m_next_handler->Handle();
        } else {
            return false;
        }
    } else {
        sp<MonkeyService> monkey_service(new MonkeyService(12321));
        monkey_service->Start();

        sp<QScript> qscript(new QScript(hsc_conf->replay_conf.app_qs_name));

        sp<QAction> current_qaction = qscript->WaitNextAction(0);
        while (current_qaction != NULL) {
            uint64_t qaction_start_point = 0;
            uint64_t qaction_done_point  = 0;

            qaction_start_point = GetCurrentTimeStamp();
            if (qaction_start_point <= 0) {
                DWLOGE("Failed in getting timestap of action start point.");
                break;
            }

            if (!Device::getInstance().ExecuteAction(current_qaction))
                break;

            qaction_done_point = GetCurrentTimeStamp();
            if (qaction_done_point <= 0) {
                DWLOGE("Failed in getting timestap of action done point.");
                break;
            }

            current_qaction = qscript->WaitNextAction(qaction_done_point - qaction_start_point);
        }

        monkey_service->Stop();
        return true;
    }
}

}

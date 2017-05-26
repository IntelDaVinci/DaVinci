#include "replay/QScript.h"

#include <stdlib.h>
#include <ui/DisplayInfo.h>

#include "replay/ShellAction.h"
#include "replay/TouchAction.h"
#include "replay/InstallerAction.h"
#include "replay/APKStarterAction.h"

#include "utils/StringUtil.h"
#include "utils/Logger.h"

namespace android {

QScript::QScript(String8 qs_file_name)
{
    AUTO_LOG();
    m_qs_file_name    = qs_file_name;
    m_last_time_stamp = 0;
    ParseQScript();
}

QScript::~QScript()
{
    AUTO_LOG();
}

sp<QAction> QScript::WaitNextAction(uint64_t elapsed_time)
{
    AUTO_LOG();

    if (m_qactions.isEmpty()) {
        DWLOGE("There are no executable actions...");
        return NULL;
    }

    sp<QAction> qaction           = m_qactions[0];
    uint64_t    qaction_timestamp = qaction->GetActionTimeStamp();
    if (qaction_timestamp == 0) {
        m_qactions.removeAt(0);
        return qaction;
    }

    DWLOGD("Current Time Stamp is %llu", m_last_time_stamp);
    DWLOGD("Wait Time Stamp is %llu", qaction_timestamp);
    if (qaction_timestamp > (m_last_time_stamp + elapsed_time))  {
        uint64_t qaction_interval = 0;
        if (qaction_timestamp > (m_last_time_stamp + elapsed_time)) {
            qaction_interval = qaction_timestamp - m_last_time_stamp - elapsed_time;
            struct timeval delay;
            delay.tv_sec  = qaction_interval / 1000000;
            delay.tv_usec = qaction_interval % 1000000;
            select(0, NULL, NULL, NULL, &delay);
        }
    }

    m_last_time_stamp = qaction_timestamp;
    m_qactions.removeAt(0);
    return qaction;
}

bool QScript::GetConfiguration()
{
    AUTO_LOG();

    FILE *qs_file = fopen(m_qs_file_name.string(), "r");
    if (qs_file == NULL) {
        DWLOGE("%s does not exist", m_qs_file_name.string());
        return false;
    }

    char line_buffer[1024] = {0};
    while (fgets(line_buffer, sizeof(line_buffer) - 1, qs_file)) {
        // Remove new line feed
        RemoveCRLF(line_buffer);

        String8         line_string(line_buffer);
        Vector<String8> line_items;

        DWLOGD("Current line: (%s)", line_string.string());

        if (StringUtil::startsWith(line_string, "PackageName=")) {
            m_package_name = GetConfigKeyValue(line_string)[1];
            DWLOGI("Package name is: (%s)", m_package_name.string());
        } else if (StringUtil::startsWith(line_string, "ActivityName=")) {
            m_activity_name = GetConfigKeyValue(line_string)[1];
            DWLOGI("Activity name is: (%s)", m_activity_name.string());
        } else if (StringUtil::startsWith(line_string, "APKName=")) {
            m_apk_name = GetConfigKeyValue(line_string)[1];
            DWLOGI("APK name is: (%s)", m_apk_name.string());
        } else if (line_string == "[Events and Actions]") {
            break;
        }

        memset(line_buffer, 0, sizeof(line_buffer));
    }

    fclose(qs_file);
    return true;
}

bool QScript::GetActions()
{
    AUTO_LOG();

    FILE *qs_file = fopen(m_qs_file_name.string(), "r");
    if (qs_file == NULL) {
        DWLOGE("%s does not exist", m_qs_file_name.string());
        return false;
    }

    char line_buffer[1024] = {0};
    while (fgets(line_buffer, sizeof(line_buffer) - 1, qs_file)) {
        // Remove new line feed
        RemoveCRLF(line_buffer);

        String8 line_string(line_buffer);
        if (line_string == "[Events and Actions]")
            break;
        memset(line_buffer, 0, sizeof(line_buffer));
    }

    memset(line_buffer, 0, sizeof(line_buffer));
    while (fgets(line_buffer, sizeof(line_buffer), qs_file)) {
        // Remove new line feed
        RemoveCRLF(line_buffer);

        String8 opcode_line_string(line_buffer);
        Vector<String8> opcode_items = FormatOPCodeString(opcode_line_string);

        DWLOGD("OPCODE_ITEMS:");
        for (size_t i = 0; i < opcode_items.size(); i++)
            DWLOGD("    %s", opcode_items[i].string());

        if (opcode_items.size() < 3) {
            DWLOGE("[%s] is invalide OPCODE action", opcode_line_string.string());
            continue;
        }

        if (!CheckTouchAction(opcode_items)        &&
            !CheckInstallerAction(opcode_items)    &&
            !CheckAPKStarterAction(opcode_items)   &&
            !CheckShellCommandAction(opcode_items) &&
            !CheckExitCommandAction(opcode_items)) {
            DWLOGE("Does not support [%s]", opcode_line_string.string());
            fclose(qs_file);
            return false;
        }

        memset(line_buffer, 0, sizeof(line_buffer));
    }

    fclose(qs_file);
    return true;
}

void QScript::ParseQScript()
{
    AUTO_LOG();

    if (m_qs_file_name.getPathExtension() != ".qs") {
        DWLOGE("%s is not QScript file", m_qs_file_name.string());
        return;
    }

    if (!GetConfiguration() || !GetActions())
        m_qactions.clear();
}

Vector<String8> QScript::GetConfigKeyValue(String8 config_line_string)
{
    Vector<String8> line_items;
    line_items = StringUtil::split(config_line_string, '=');
    if (line_items.size() == 1) {
        line_items.push(String8(""));
    }

    return line_items;
}

Vector<String8> QScript::FormatOPCodeString(String8 opcode_line_string)
{
    Vector<String8> line_items = StringUtil::split(opcode_line_string, ' ');
    Vector<String8>::iterator item;
    for (item = line_items.begin(); item != line_items.end(); ) {
        if ((*item) == " ") {
            line_items.erase(item);
            continue;
        } else {
            if (StringUtil::endsWith(*item, ":")) {
                Vector<String8> sub_items = StringUtil::split(*item, ':');
                *item = sub_items[0];
            }
            ++item;
        }
    }

    return line_items;
}

bool QScript::CheckTouchAction(Vector<String8> opcode_items)
{
    AUTO_LOG();

    double   ms_time_stamp      = atof(FormatTimeStamp(opcode_items[TIME_STAMP_IDX]));
    uint64_t qaction_time_stamp = (uint64_t)(ms_time_stamp * 1000);

    if (StringUtil::startsWith(opcode_items[OPCODE_IDX], "OPCODE_TOUCHDOWN") ||
        StringUtil::startsWith(opcode_items[OPCODE_IDX], "OPCODE_TOUCHUP")   ||
        StringUtil::startsWith(opcode_items[OPCODE_IDX], "OPCODE_TOUCHMOVE")) {
        int x           = atoi(opcode_items[OPCODE_IDX + 1]);
        int y           = atoi(opcode_items[OPCODE_IDX + 2]);
        int orientation = 0;

        if (StringUtil::endsWith(opcode_items[OPCODE_IDX], "HORIZONTAL")) {
            orientation = 1;
        } else {
            // Orientation is optional in QScript
            if (opcode_items.size() == 3) {
                orientation = 0;
            } else {
                orientation = atoi(opcode_items[OPCODE_IDX + 3]);
            }
        }

        TouchState touch_state;
        m_touch_info.x = x;
        m_touch_info.y = y;
        if (StringUtil::startsWith(opcode_items[OPCODE_IDX], "OPCODE_TOUCHDOWN")) {
            touch_state = TOUCH_DOWN;
            if(opcode_items.size() == OPCODE_IDX + 5) {
                m_touch_info.keyValueStr = opcode_items[OPCODE_IDX + 4];
            }
        } else if (StringUtil::startsWith(opcode_items[OPCODE_IDX], "OPCODE_TOUCHUP")) {
            touch_state = TOUCH_UP;
            m_touch_info.keyValueStr = "";
        } else {
            touch_state = TOUCH_MOVE;
        }

        sp<TouchAction> touch_down_action(new TouchAction());
        touch_down_action->SetTouchInfo(m_package_name, touch_state, orientation, m_touch_info);
        touch_down_action->SetActionTimeStamp(qaction_time_stamp);
        m_qactions.push(touch_down_action);

        return true;
    } else {
        return false;
    }
}

bool QScript::CheckShellCommandAction(Vector<String8> opcode_items)
{
    AUTO_LOG();

    double   ms_time_stamp      = atof(FormatTimeStamp(opcode_items[TIME_STAMP_IDX]));
    uint64_t qaction_time_stamp = (uint64_t)(ms_time_stamp * 1000);

    if (opcode_items[OPCODE_IDX] == "OPCODE_CALL_EXTERNAL_SCRIPT") {
        String8 shell_file_path = opcode_items[OPCODE_IDX + 1];

        sp<ShellAction> shell_action(new ShellAction());
        shell_action->SetShellCommand(String8(""), shell_file_path);
        shell_action->SetActionTimeStamp(qaction_time_stamp);
        m_qactions.push(shell_action);

        return true;
    }

    return false;
}

bool QScript::CheckExitCommandAction(Vector<String8> opcode_items)
{
    AUTO_LOG();

    double   ms_time_stamp      = atof(FormatTimeStamp(opcode_items[TIME_STAMP_IDX]));
    uint64_t qaction_time_stamp = (uint64_t)(ms_time_stamp * 1000);

    if (opcode_items[OPCODE_IDX] == "OPCODE_EXIT") {
        return true;
    }

    return false;
}

bool QScript::CheckInstallerAction(Vector<String8> opcode_items)
{
    AUTO_LOG();

    double   ms_time_stamp      = atof(FormatTimeStamp(opcode_items[TIME_STAMP_IDX]));
    uint64_t qaction_time_stamp = (uint64_t)(ms_time_stamp * 1000);

    DWLOGD("opcode time stamp item: %s", opcode_items[TIME_STAMP_IDX].string());
    DWLOGD("ms_time_stamp %f", ms_time_stamp);
    DWLOGD("qaction_time_stamp %llu", qaction_time_stamp);

    if (opcode_items[OPCODE_IDX] == "OPCODE_INSTALL_APP"  ||
        opcode_items[OPCODE_IDX] == "OPCODE_UNINSTALL_APP") {

        InstallActionIndicator action_indicator;
        String8                qs_file_path = m_qs_file_name.getPathDir();
        String8                apk_path;

        if (opcode_items[OPCODE_IDX] == "OPCODE_INSTALL_APP") {
            action_indicator = INSTALL_INDICATOR;

            // There is two optional parameters for specifying APK name to OPCODE_INSTALL_APP.
            // First indicates whether the APK is intalled in external storage or not.
            // Second indicates the APK name.
            String8 dst_apk_name = m_apk_name;
            if (opcode_items.size() >= OPCODE_IDX + 3)
                dst_apk_name = opcode_items[OPCODE_IDX + 2];

            if (qs_file_path.isEmpty()) {
                apk_path = String8("./") + dst_apk_name;
            } else {
                apk_path = qs_file_path + "/" + dst_apk_name;
            }

            if (access(apk_path.string(), R_OK) == -1) {
                DWLOGE("Cannot find APK &s for OPCODE_INSTALL_APP. (%s)",
                        apk_path.string(),
                        strerror(errno));
                return false;
            }
        } else {
            action_indicator = UNINSTALL_INDICATOR;

            // There is an option parameter for specifying package name to OPCODE_UNINSTALL_APP.
            apk_path = m_package_name;
            if (opcode_items.size() >= OPCODE_IDX + 2)
                apk_path = opcode_items[OPCODE_IDX + 1];
            if (apk_path.isEmpty()) {
                DWLOGE("Cannot find package name for OPCODE_UNINSTALL_APP");
                return false;
            }
        }

        sp<InstallerAction> installer_action(new InstallerAction());
        installer_action->SetInstallerAction(action_indicator, apk_path);
        installer_action->SetActionTimeStamp(qaction_time_stamp);
        m_qactions.push(installer_action);

        return true;
    }

    return false;
}

bool QScript::CheckAPKStarterAction(Vector<String8> opcode_items)
{
    AUTO_LOG();

    double ms_time_stamp        = atof(FormatTimeStamp(opcode_items[TIME_STAMP_IDX]));
    uint64_t qaction_time_stamp = (uint64_t)(ms_time_stamp * 1000);

    if (opcode_items[OPCODE_IDX] == "OPCODE_START_APP" ||
        opcode_items[OPCODE_IDX] == "OPCODE_STOP_APP") {

        String8 dst_package_name = m_package_name;
        String8 dst_activity_name = m_activity_name;
        if (opcode_items.size() >= OPCODE_IDX + 2) {
            String8 para = opcode_items[OPCODE_IDX + 1];
            Vector<android::String8> ret = StringUtil::split(para, '/');
            dst_package_name = ret[0];
            if (ret.size() >= 2)
                dst_activity_name = ret[1];
        }

        if (dst_package_name.isEmpty()) {
            DWLOGE("Cannot find package name for OPCODE_START_APP/OPCODE_STOP_APP.");
            return false;
        }

        APKStarterIndicator action_indicator;
        if (opcode_items[OPCODE_IDX] == "OPCODE_START_APP") {
            if (dst_activity_name.isEmpty()) {
                DWLOGE("Cannot find actitivy name for OPCODE_START_APP.");
                return false;
            }
            action_indicator = STAT_APP_INDICATOR;
        } else {
            action_indicator = STOP_APP_INDICATOR;
        }

        sp<APKStarterAction> apk_starter_action(new APKStarterAction());
        apk_starter_action->SetAPKStarterAction(action_indicator,
                                                dst_package_name,
                                                dst_activity_name);
        apk_starter_action->SetActionTimeStamp(qaction_time_stamp);
        m_qactions.push(apk_starter_action);

        return true;
    }

    return false;
}

void QScript::RemoveCRLF(char *src_string)
{
    if (src_string == NULL)
        return;

    if (strlen(src_string) == 0)
        return;

    while (src_string[strlen(src_string) - 1] == '\n' ||
           src_string[strlen(src_string) - 1] == '\r') {
        src_string[strlen(src_string) - 1] = '\0';
    }
}

String8 QScript::FormatTimeStamp(String8 time_stamp)
{
    String8 ret("");
    if (time_stamp.size() <= 0) {
        return ret;
    }

    Vector<String8> res = StringUtil::split(time_stamp, ',');
    for (size_t i = 0; i < res.size(); i++) {
        if (!res[i].isEmpty())
            ret += res[i];
    }

    return ret;
}

}

#ifndef _QSCRIPT_H_
#define _QSCRIPT_H_

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>

#include "QAction.h"
#include "TouchAction.h"

namespace android {

typedef enum {
    LABLE_IDX = 0,
    TIME_STAMP_IDX,
    OPCODE_IDX
} QActionIndex;

class QScript : public RefBase {
public:
    explicit QScript(String8 qs_file_name);
    virtual ~QScript();

    sp<QAction> WaitNextAction(uint64_t elapsed_time);

private:
    void ParseQScript();
    bool GetConfiguration();
    bool GetActions();
    bool CheckTouchAction(Vector<String8> opcode_items);
    bool CheckShellCommandAction(Vector<String8> opcode_items);
    bool CheckInstallerAction(Vector<String8> opcode_items);
    bool CheckAPKStarterAction(Vector<String8> opcode_items);
    bool CheckExitCommandAction(Vector<String8> opcode_items);
    void RemoveCRLF(char *src_string);
    String8 FormatTimeStamp(String8 time_stamp);

    Vector<String8> GetConfigKeyValue(String8 config_line_string);
    Vector<String8> FormatOPCodeString(String8 opcode_line_string);
private:
    Vector<sp<QAction> > m_qactions;
    String8              m_qs_file_name;
    String8              m_package_name;
    String8              m_activity_name;
    String8              m_apk_name;
    uint64_t             m_last_time_stamp;
    TouchInfo            m_touch_info;
};

}

#endif

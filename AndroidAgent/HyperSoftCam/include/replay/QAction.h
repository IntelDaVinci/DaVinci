#ifndef _QACTION_H_
#define _QACTION_H_

#include <utils/RefBase.h>
#include <utils/String8.h>

namespace android {

class QAction : public RefBase {
public:
    QAction();
    virtual ~QAction();

    virtual bool Execute();

    virtual void SetActionTimeStamp(uint64_t action_time_stamp);
    virtual uint64_t GetActionTimeStamp();

protected:
    String8  m_command;
    uint64_t m_action_time_stamp;
};

}

#endif

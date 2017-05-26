#ifndef _MONKEY_BROKER_H_
#define _MONKEY_BROKER_H_

#include <utils/Singleton.h>
#include <utils/String8.h>

namespace android {

class MonkeyBroker : public Singleton<MonkeyBroker> {
friend class Singleton<MonkeyBroker>;

protected:
    MonkeyBroker();
    ~MonkeyBroker();

public:
    bool ExecuteSyncCommand(String8 command);

private:
    void InitializeEnvironment();

private:
    int  m_monkey_channel;
    bool m_initialized;

};

}

#endif

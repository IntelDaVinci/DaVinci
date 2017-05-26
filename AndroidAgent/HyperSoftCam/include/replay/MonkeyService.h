#ifndef _MONKEY_SERVICE_H_
#define _MONKEY_SERVICE_H_

#include <stdio.h>

#include <utils/RefBase.h>

namespace android {

class MonkeyService : public RefBase {
public:
    explicit MonkeyService(int listen_port);
    virtual ~MonkeyService();

    bool Start();
    bool Stop();

private:
    FILE *m_monkey_stream;
    int   m_listen_port;
};

}

#endif

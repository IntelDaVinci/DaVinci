#ifndef RNR_H
#define RNR_H

#include <windows.h>
#include <stdio.h>

#include "logger.h"
#include <tchar.h>

struct Event
{
    /* event description... */
    timeval time;
    unsigned short type;
    unsigned short code;
    int value;
};

class Monitor
{
public:
    Monitor();
    ~Monitor();

    bool Initialize(const char *ip, int port);
    static unsigned int MSDK_THREAD_CALLCONVENTION RnrThread(void* param);

private:

    SOCKET m_socketRnrSocket;
    MSDKThread *m_pRnrThread;
};

#endif
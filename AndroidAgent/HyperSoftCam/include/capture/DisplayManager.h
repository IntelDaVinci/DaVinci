#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

#include <unistd.h>
#include <utils/Thread.h>
#include <utils/String8.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Statistics.h"
#include "Rnr.h"
#include "com/Channel.h"

namespace android {

class CDWManager : public Thread {
public:
    CDWManager(String8& app, String8& package, String8& activity, String8& trace_in_file, String8& trace_out_file);
    CDWManager(uint16_t port);
    ~CDWManager();

    status_t Start();
    status_t Stop();
    void Remove(const char* name);

private:
    // (overrides Thread method)
    virtual status_t readyToRun();
    virtual bool threadLoop();

private:
    Archive      m_archive;
    bool         m_stop_thread;
    CChannel    *m_channel;
    sp<Monitor>  m_monitor;
    bool         m_rnr_mode;
};

}

#endif

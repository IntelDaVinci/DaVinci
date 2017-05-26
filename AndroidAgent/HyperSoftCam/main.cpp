#include <signal.h>
#include <getopt.h>
#include <utils/String8.h>
#include <utils/Thread.h>

#include "Configuration.h"
#include "capture/Statistics.h"
#include "capture/StreamHandler.h"
#include "capture/RecordHandler.h"
#include "replay/ReplayHandler.h"
#include "utils/Logger.h"
#include "utils/StringUtil.h"

using namespace android;

volatile bool gExit = false;

class SigMgr : public Thread {
public:
    virtual status_t readyToRun()
    {
        sigemptyset(&waitset);
        sigaddset(&waitset, SIGQUIT);
        return NO_ERROR;
    }
private:
    virtual bool threadLoop()
    {
        AUTO_LOG();
        int sig;
        int rc = sigwait(&waitset, &sig);
        if (rc == 0) {
            gExit = true;
            DWLOGD("Sgi number is %d", sig);
        } else {
            DWLOGE("Cannot wait signal: %s", strerror(errno));
        }

        return false;
    }

private:
    sigset_t waitset;
};

static void BlockSigQuit()
{
    AUTO_LOG();

    sigset_t oset;
    sigset_t bset;
    sigemptyset(&bset);
    sigaddset(&bset, SIGQUIT);
    if (pthread_sigmask(SIG_BLOCK, &bset, &oset) != 0)
        DWLOGE("Failed in setting pthread signal mask");
}

int main(int argc, char *argv[])
{
    AUTO_LOG();

    bool ret = Configuration::getInstance().ParseConsoleParameters(argc, argv);
    if (!ret) {
        DWLOGE("Invalide Parameters!!!");
        return -1;
    }

    if (Configuration::getInstance().Get()->stat_mode) {
        DWLOGI("Enable statistics mode.");
        Statistics::getInstance().Enable();
    }

    HSCConf* hsc_conf = Configuration::getInstance().Get();
    if (hsc_conf->replay_conf.is) {
        // Try to create process
        pid_t pid = fork();
        if (pid < 0) {
            DWLOGE("Cannot crate replay process!");
            return -1;
        }

        // Parent process
        if (pid > 0) {
            DWLOGD("Current mode is replay mode. So parent pcoess needs to be exited.");
            printf("REPLAY_CREATER_EXIT\n");
            return 0;
        }

        // Child process
        setsid();
    }

    // Wait quit signal at dedicate thread
    sp<SigMgr> sig_mgr(new SigMgr());
    BlockSigQuit();
    sig_mgr->run("MonSIGQUIT");

    sp<HSCHandler> stream_handler(new StreamHandler());
    sp<HSCHandler> record_handler(new RecordHandler());
    sp<HSCHandler> replay_handler(new ReplayHandler());

    replay_handler->SetNextHandler(NULL);
    record_handler->SetNextHandler(replay_handler);
    stream_handler->SetNextHandler(record_handler);

    ret = stream_handler->Handle();
    if (!ret) {
        DWLOGE("Kidding me... Invalid request!!!");
        return -1;
    }

    return 0;
}

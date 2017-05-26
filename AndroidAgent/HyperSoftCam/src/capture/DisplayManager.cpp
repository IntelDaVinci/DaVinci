#include "capture/DisplayManager.h"

#include "com/ChannelTcp.h"
#include "capture/ScreenEncoder.h"
#include "capture/ScreenEncoderConsumer.h"
#include "capture/RnRVideoFileReporter.h"
#include "utils/Logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/String8.h>

namespace android {

CDWManager::CDWManager(String8& app,
                       String8& package,
                       String8& activity,
                       String8& trace_in_file,
                       String8& trace_out_file
                       ) : m_monitor(new Monitor())
{
    AUTO_LOG();

    m_stop_thread   = false;
    m_rnr_mode      = true;
    m_channel       = NULL;
    m_archive.media = -1;
    m_archive.qs    = -1;
    m_archive.qts   = -1;

    status_t    err;
    String8     path("/data/local/tmp/");
    struct stat s;

    path += app;

    DWLOGD("check %s for existence", path.string());
    if (stat(path.string(), &s) == 0) {
        DWLOGW("try to delete %s", path.string());
        Remove(path.string());
    }

    if (mkdir(path.string(), S_IRWXU | S_IRWXG | S_IRWXO) == 0) {
        m_archive.media = open((path + "/" + app + ".h264").string(),
                                O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        m_channel       = new RnRVideoFileReporter(m_monitor, m_archive.media);
        assert(m_channel != NULL);
        m_archive.qs    = open((path + "/" + app + ".qs").string(),
                                O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        m_archive.qts   = open((path + "/" + app + ".qts").string(),
                                O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        m_archive.trace_in = -1;
        if (trace_in_file.size() > 0)
        {
            m_archive.trace_in = open(trace_in_file.string(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        }
        m_archive.trace_out = -1;
        if (trace_out_file.size() > 0)
        {
            m_archive.trace_out = open(trace_out_file.string(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        }
        m_monitor->Initialize(app, package, activity, m_archive.qs, m_archive.qts, m_archive.trace_in, m_archive.trace_out);
    }
}

CDWManager::CDWManager(uint16_t port)
{
    AUTO_LOG();

    m_stop_thread = false;
    m_rnr_mode    = false;

    m_channel     = new CChannelTcp();
    assert(m_channel != NULL);
    m_channel->ConfigPort(port);
}

CDWManager::~CDWManager()
{
    AUTO_LOG();

    if (m_channel != NULL) {
        delete m_channel;
        m_channel = NULL;
    }
}

status_t CDWManager::Start()
{
    status_t err;

    AUTO_LOG();

    m_stop_thread = false;
    err = run("DisplayWingsManager");
    if (err != NO_ERROR) {
        DWLOGE("Cannot start display wings manager !!! err(%d)", err);
        return err;
    }

    return NO_ERROR;
}

status_t CDWManager::Stop()
{
    status_t err = NO_ERROR;

    AUTO_LOG();

    m_stop_thread = true;
    if (m_channel != NULL) {
        m_channel->Inactivate();
    }

    // Stop encode thread
    err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop display manager !!! err(%d)", err);
        return err;
    }

    if (m_rnr_mode) {
        if (m_monitor != NULL)
            m_monitor->Finalize();

        if (m_archive.media != -1)
            close(m_archive.media);
        if (m_archive.qs != -1)
            close(m_archive.qs);
        if (m_archive.qts != -1)
            close(m_archive.qts);
        if (m_archive.trace_in != -1)
            close(m_archive.trace_in);
        if (m_archive.trace_out != -1)
            close(m_archive.trace_out);
    }

    return err;
}

void CDWManager::Remove(const char* name)
{
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    dir = opendir(name);
    if (dir == NULL) {
        DWLOGE("Error opendir(%s)", name);
        return;
    }
    DWLOGW("Open %s successfully", name);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            snprintf(path, (size_t) PATH_MAX, "%s/%s", name, entry->d_name);
            DWLOGW("Try to remove %s", path);
            if (entry->d_type == DT_DIR) {
                Remove(path);
            } else {
                unlink(path);
            }
            DWLOGW("Removed %s successfully!!", path);
        }
    }
    closedir(dir);
    rmdir(name);
}

status_t CDWManager::readyToRun()
{
    if (m_channel != NULL) {
        m_channel->Channelize();
    }
    return NO_ERROR;
}

bool CDWManager::threadLoop()
{
    sp<CScreenEncoder>         screen_encoder;
    sp<CScreenEncoderConsumer> screen_encoder_consumer;
    status_t                   err;

    while (!m_stop_thread) {
        err = m_channel->Activate();
        if (err != NO_ERROR)
            break;

        if (Statistics::getInstance().IsActive())
            Statistics::getInstance().Start(m_channel->GetFd());
        if (screen_encoder == NULL)
            screen_encoder = new CScreenEncoder();
        String8 ret = screen_encoder->StartEncoder();
        m_channel->Report((uint8_t *)ret.string(), ret.bytes(), CChannel::k_m_moni);
        if (ret != "OK") {
            DWLOGE("Cannot start encoder !!! err(%s)", ret.string());
            break;
        }

        if (screen_encoder_consumer == NULL)
            screen_encoder_consumer = new CScreenEncoderConsumer(screen_encoder->GetEncoder());
        // NOTE: the funtion calling order cannot be changed
        screen_encoder_consumer->ConfigReporter(m_channel);
        screen_encoder_consumer->StartConsuming();

        while (!m_stop_thread && m_channel->IsActive()) {
            usleep(500);
        }

        screen_encoder_consumer->StopConsuming();
        screen_encoder->StopEncoder();

        screen_encoder_consumer = NULL;
        screen_encoder = NULL;
    }

    return false;
}

}

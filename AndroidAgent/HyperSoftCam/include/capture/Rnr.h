#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <utils/Thread.h>
#include <netinet/tcp.h>
#include <ui/DisplayInfo.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <utils/String8.h>
#include <utils/Mutex.h>

#include <string.h>

#include "RnRVideoFileReporter.h"
#include "EvtFrameRecorder.h"
#include "utils/Logger.h"
#include "replay/TraceReplayer.h"

namespace android {

struct label {
    const char *name;
    int value;
};

class Monitor : public Observer, virtual public Thread {
public:
    Monitor();
    virtual ~Monitor();
    bool Initialize(String8& app_, String8& package_, String8& activity_, int qs, int qts, int trace_in, int trace_out);
    bool Finalize();

    virtual void OnNewFrame(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset);
    virtual void OnFrameSize(uint32_t width, uint32_t height);

private:
    struct pollfd *ufds;
    int nfds;
    int client;
    String8 path;
    char **devices;
    sp<EvtFrameRecorder> m_evt_frame_rec;
    Resolution dres;  // display resolution;
    TouchResolution tsres; // touch screen resolution;
    String8 app, package, activity;

    int touch_fd;
    sp<TraceReplayer> m_trace_replayer;

    virtual bool threadLoop();
    int Fetch(const char *dirname, int nfd);
    int Open(const char *device);
    int Close(const char *device);
    int Scan(const char *dirname);
    bool DefineTouchDev(int fd);
    bool DefineKeyDev(int fd);
    bool DefineBtnDev(int fd);
};

}

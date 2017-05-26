#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <utils/String8.h>

#include "capture/Rnr.h"

namespace android {

#define BITS_PER_LONG        (sizeof(long) * 8)
#define NBITS(x)             ((((x)-1) / BITS_PER_LONG) + 1)
#define OFF(x)               ((x) % BITS_PER_LONG)
#define LONG(x)              ((x) / BITS_PER_LONG)
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

Monitor::Monitor() : path("/dev/input"), m_evt_frame_rec(new EvtFrameRecorder())
{
    AUTO_LOG();

    ufds    = NULL;
    nfds    = -1;
    client  = -1;
    devices = NULL;
    touch_fd = -1;
}

Monitor::~Monitor()
{
    AUTO_LOG();

    inotify_rm_watch(ufds[0].fd, IN_DELETE | IN_CREATE);
    int i = 0;
    for (int i = 1; i < nfds; ++i) {
        close(ufds[i].fd);
        free(devices[i]);
        devices[i] = NULL;
    }
    free(devices);
    free(ufds);
}

int Monitor::Open(const char *device)
{
    AUTO_LOG();

    int version;
    int fd;
    struct pollfd *new_ufds;
    char **new_device_names;
    char name[80];
    char location[80];
    char idstr[80];
    struct input_id id;

    fd = open(device, O_RDWR);
    if(fd < 0) {
        DWLOGE("Could not open %s, %s", device, strerror(errno));
        return -1;
    }

    // Check current device whether supports touch event or key event.
    if (!DefineTouchDev(fd)) {
        DWLOGE("%s is not touch device.", device);
         if (!DefineKeyDev(fd)) {
            DWLOGE("%s is not key device.", device);
            return -1;
        }
    }

    // Check current device whether supports BTN_TOUCH event. If yes, we will use
    // BTN_TOUCH DOWN/UP to identify OPCODE_TOUCHDOWN/OPCODE_TOUCHUP
    if (DefineBtnDev(fd)) {
        DWLOGD("Current device %s supports BTN_TOUCH event.", device);
        m_evt_frame_rec->SetBtnEvt(true);
    }
    DWLOGD("%s is touch/key device.", device);

    if(ioctl(fd, EVIOCGVERSION, &version)) {
        DWLOGE("Could not get driver version for %s, %s", device, strerror(errno));
        return -1;
    }
    if(ioctl(fd, EVIOCGID, &id)) {
        DWLOGE("Could not get driver id for %s, %s", device, strerror(errno));
        return -1;
    }
    name[sizeof(name) - 1] = '\0';
    location[sizeof(location) - 1] = '\0';
    idstr[sizeof(idstr) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        DWLOGI("Could not get device name for %s, %s", device, strerror(errno));
        name[0] = '\0';
    }
    if(ioctl(fd, EVIOCGPHYS(sizeof(location) - 1), &location) < 1) {
        DWLOGI("Could not get location for %s, %s", device, strerror(errno));
        location[0] = '\0';
    }
    if(ioctl(fd, EVIOCGUNIQ(sizeof(idstr) - 1), &idstr) < 1) {
        DWLOGI("Could not get idstring for %s, %s", device, strerror(errno));
        idstr[0] = '\0';
    }

    new_ufds = (pollfd*)realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL) {
        DWLOGE("Out of memory");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = (char**)realloc(devices, sizeof(devices[0]) * (nfds + 1));
    if(new_device_names == NULL) {
        DWLOGE("Out of memory");
        return -1;
    }
    devices = new_device_names;

    DWLOGI("Add device %d: %s", nfds, device);
    DWLOGI("  bus:      %04x", id.bustype);
    DWLOGI("  vendor    %04x", id.vendor);
    DWLOGI("  product   %04x", id.product);
    DWLOGI("  version   %04x", id.version);
    DWLOGW("  name:     \"%s\"", name);
    DWLOGI("  location: \"%s\"", location);
    DWLOGI("  id:       \"%s\"", idstr);
    DWLOGI("  version:  %d.%d.%d", version >> 16, (version >> 8) & 0xff, version & 0xff);

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    devices[nfds] = strdup(device);
    nfds++;

    return 0;
}

int Monitor::Close(const char *device)
{
    AUTO_LOG();

    int i;
    for(i = 1; i < nfds; i++) {
        if(strcmp(devices[i], device) == 0) {
            int count = nfds - i - 1;
            DWLOGI("Remove device %d: %s\n", i, device);
            free(devices[i]);
            memmove(devices + i, devices + i + 1, sizeof(devices[0]) * count);
            memmove(ufds + i, ufds + i + 1, sizeof(ufds[0]) * count);
            nfds--;
            return 0;
        }
    }
    DWLOGE("Remote device: %s not found\n", device);
    return -1;
}

int Monitor::Scan(const char *dirname)
{
    AUTO_LOG();

    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        Open(devname);
    }
    closedir(dir);
    return 0;
}

bool Monitor::Initialize(String8& app_, String8& package_, String8& activity_, int qs, int qts, int trace_in, int trace_out)
{
    AUTO_LOG();

    int res;

    app = app_;
    package = package_;
    activity = activity_;

    nfds = 1;
    ufds = (pollfd*)calloc(1, sizeof(ufds[0]));
    ufds[0].fd = inotify_init();
    ufds[0].events = POLLIN;
    res = inotify_add_watch(ufds[0].fd, path.string(), IN_DELETE | IN_CREATE);
    if(res < 0) {
        DWLOGE("Could not add watch for %s, %s\n", path.string(), strerror(errno));
        return false;
    }

    // Get touch device node
    res = Scan(path.string());
    if(res < 0) {
        DWLOGE("Scan dir failed for %s\n", path.string());
        return false;
    }

    // Initialize recorder
    bool ret = m_evt_frame_rec->Initialize(qs, qts, trace_out, tsres);
    if (!ret) {
        DWLOGE("Cannot initialize event and frame recorder");
        return false;
    }

    // Start monitoring touch event
    status_t err = run("MonTouchEvt");
    if (err != NO_ERROR) {
        DWLOGE("Cannot start display wings manager !!! err(%d)", err);
        return false;
    }

    // Start replaying raw trace
    if (trace_in != -1)
    {
        DWLOGI("Replay raw trace on touch device: %d", touch_fd);
        m_trace_replayer = sp<TraceReplayer>(new TraceReplayer(trace_in, touch_fd));
        m_trace_replayer->run("TraceReplayer");
    }

    return true;
}

bool Monitor::Finalize()
{
    AUTO_LOG();

    // Stop monitor thread
    status_t err = NO_ERROR;
    err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop monitor !!! err(%d)", err);
    }

    m_evt_frame_rec->Finalize();
    return true;
}

int Monitor::Fetch(const char *dirname, int nfd)
{
    AUTO_LOG();

    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    res = read(nfd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        if(errno == EINTR)
            return 0;
        DWLOGE("Could not get event, %s", strerror(errno));
        return 1;
    }
    DWLOGW("Got %d bytes of event information", res);

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        DWLOGW("%d: %08x \"%s\"", event->wd, event->mask, event->len ? event->name : "");
        if(event->len) {
            strcpy(filename, event->name);
            if(event->mask & IN_CREATE) {
                Open(devname);
            } else {
                Close(devname);
            }
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return 0;
}

bool Monitor::threadLoop()
{
    struct input_event event;
    int res = poll(ufds, nfds, 50);
    if (res == 0) {
        return true;
    } else if (res == -1) {
        DWLOGE("Cannot poll event from input: %s", strerror(errno));
        return false;
    }

    if(ufds[0].revents & POLLIN) {
        Fetch(path.string(), ufds[0].fd);
    }
    for(int i = 1; i < nfds; i++) {
        if(ufds[i].revents) {
            if(ufds[i].revents & POLLIN) {
                res = read(ufds[i].fd, &event, sizeof(event));
                if(res < (int)sizeof(event)) {
                    DWLOGE("Could not get event\n");
                    return false;
                }
                m_evt_frame_rec->AddEvent(event);
            }
        }
    }
    return true;
}

void Monitor::OnNewFrame(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset)
{
    m_evt_frame_rec->RecordFrameInfo(frame_num, frame_type, frame_offset);
}

void Monitor::OnFrameSize(uint32_t width, uint32_t height)
{
    if (dres.width != width || dres.height != height) {
        dres.width = width; dres.height = height;
        m_evt_frame_rec->InitQsHeader(app, package, activity, width, height);
    }
}

bool Monitor::DefineTouchDev(int fd)
{
    unsigned long ev_bits[NBITS(EV_CNT)] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_ABS, EV_CNT), ev_bits) < 0) {
        DWLOGW("Current device doesn't support ABS event. (%s)", strerror(errno));
        return false;
    }

    if (!test_bit(ABS_MT_POSITION_X, ev_bits)) {
        DWLOGW("Current device doesn't support ABS_MT_POSITION_X event. (%s)", strerror(errno));
        return false;
    }
    if (!test_bit(ABS_MT_POSITION_Y, ev_bits)) {
        DWLOGW("Current device doesn't support ABS_MT_POSITION_X event. (%s)", strerror(errno));
        return false;
    }

    struct input_absinfo abs_x;
    struct input_absinfo abs_y;
    if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x) < 0) {
        DWLOGW("Current device doesn't support ABS_MT_POSITION_X event. (%s)", strerror(errno));
        return false;
    }
    if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y) < 0) {
        DWLOGW("Current device doesn't support ABS_MT_POSITION_Y event. (%s)", strerror(errno));
        return false;
    }

    tsres.x_min = abs_x.minimum;
    tsres.x_max = abs_x.maximum;
    tsres.y_min = abs_y.minimum;
    tsres.y_max = abs_y.maximum;
    touch_fd = fd;
    DWLOGD("Current device is touch device. Its touch resolution is [(%d, %d), (%d, %d)]",
            abs_x.minimum, abs_y.minimum,
            abs_x.maximum, abs_y.maximum);
    return true;
}

bool Monitor::DefineKeyDev(int fd)
{
    unsigned long ev_bits[NBITS(KEY_CNT)] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, KEY_CNT), ev_bits) < 0) {
        DWLOGW("Failed in getting event bit. (%s)", strerror(errno));
        return false;
    }

    if (test_bit(KEY_VOLUMEDOWN, ev_bits)) {
        DWLOGW("Current device support KEY_VOLUMEDOWN event.");
        return true;
    }

    if (test_bit(KEY_VOLUMEUP, ev_bits)) {
        DWLOGW("Current device support KEY_VOLUMEUP event.");
        return true;
    }

    if (test_bit(KEY_POWER, ev_bits)) {
        DWLOGW("Current device support KEY_POWER event.");
        return true;
    }

    return DefineBtnDev(fd);
}

bool Monitor::DefineBtnDev(int fd)
{
    unsigned long ev_bits[NBITS(KEY_CNT)] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, KEY_CNT), ev_bits) < 0) {
        DWLOGW("Failed in getting event bit. (%s)", strerror(errno));
        return false;
    }

    if (test_bit(BTN_TOUCH, ev_bits)) {
        DWLOGW("Current device support BTN_TOUCH event.");
        return true;
    }

    return false;
}

}


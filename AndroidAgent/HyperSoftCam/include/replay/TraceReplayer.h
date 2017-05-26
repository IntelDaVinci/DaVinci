#include <unistd.h>
#include <linux/input.h>
#include <utils/Thread.h>

namespace android {
    class TraceReplayer : public Thread {
    public:
        TraceReplayer(int trace_in, int out_fd);
        ~TraceReplayer();
    private:
        int m_trace_in;
        int m_out_fd;
        struct timeval m_event_start_time;
        uint64_t m_start_ts;
        struct input_event* m_event;

        virtual status_t readyToRun();
        virtual bool threadLoop();
        struct input_event* ReadNextEvent();
    };
}

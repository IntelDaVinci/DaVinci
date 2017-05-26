#include "replay/TraceReplayer.h"
#include "utils/ConvUtils.h"

namespace android {

    TraceReplayer::TraceReplayer(int trace_in, int out_fd) :
        m_trace_in(trace_in), m_out_fd(out_fd), m_start_ts(0)
    {
        memset(&m_event_start_time, 0, sizeof(m_event_start_time));
    }

    TraceReplayer::~TraceReplayer()
    {
    }

    status_t TraceReplayer::readyToRun()
    {
        m_start_ts = GetCurrentTimeStamp();
        return NO_ERROR;
    }

    bool TraceReplayer::threadLoop()
    {
        m_event = ReadNextEvent();
        if (m_event == NULL)
        {
            return false;
        }

        int64_t elapsed = GetCurrentTimeStamp() - m_start_ts;
        int64_t event_elapsed = TimevalToTimeStamp(m_event->time);
        int64_t wait_us = event_elapsed - elapsed;
        if (wait_us > 0)
        {
            struct timeval delay = TimeStampToTimeval(wait_us);
            select(0, NULL, NULL, NULL, &delay);
        }

        write(m_out_fd, m_event, sizeof(struct input_event));

        delete m_event;
        return true;
    }

    struct input_event* TraceReplayer::ReadNextEvent()
    {
        struct input_event* event = new struct input_event;
        int size_read = read(m_trace_in, event, sizeof(struct input_event));
        if (size_read < (int)sizeof(struct input_event))
        {
            return NULL;
        }
        return event;
    }
}

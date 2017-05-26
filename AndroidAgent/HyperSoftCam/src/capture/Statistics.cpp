#include "capture/Statistics.h"

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(Statistics)

Statistics::Statistics() : parser(this),
                           calibration(true),
                           count(0),
                           active(false)
{
    AUTO_LOG();
}

Statistics::Statistics(int fd) : parser(this),
                                 calibration(true),
                                 count(0),
                                 active(false)
{
    AUTO_LOG();

    Start(fd);
}

Statistics::~Statistics()
{
    AUTO_LOG();

    status_t err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop statistic thread !!! err(%d)", err);
    }

    size_t info_size = info.size();
    for (size_t i = 0; i < info_size; i++)
        delete info.valueAt(i);
    info.clear();

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&timutex);
}

void Statistics::Start(int fd)
{
    utc = fd;
    if (active) {
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&timutex, NULL);
        accumulation = 0;
        memset(index, 0, sizeof(uint64_t)*TS_CNT);
        memset(time.min, 0xff, sizeof(uint64_t)*TS_CNT);
        memset(&process.min.cpu, 0xff, sizeof(unsigned long));
        // memset(&process.min.mem, 0xff, sizeof(unsigned long));
        process.min.mem = 1 << 30;
        latency.accumulation = 0;
        latency.max = 0;
        latency.min = 0xffffffffffffffff;
        process.max.cpu = process.avg.cpu = 0;
        process.max.mem = process.avg.mem = 0;
        process.cpu = process.mem = process.cnt = 0;

        start = GetCurrentTimeStamp();

        run();
        last = GetCurrentTimeStamp();
        pid = getpid();
    }
}

void Statistics::Stamp(TimeStamp ts, uint64_t value, uint64_t idx)
{
    uint64_t frame_index = idx ? idx : index[ts];
    if (active) {
        pthread_mutex_lock(&mutex);
        ssize_t info_key_index = info.indexOfKey(frame_index);
        if (info_key_index < 0 && ts == CAP_TS) {
            info.add(frame_index, new uint64_t[TS_CNT]);
            memset(info.valueFor(frame_index), 0, sizeof(uint64_t) * TS_CNT);
        }
        pthread_mutex_unlock(&mutex);

        // If the socket connection is closed or reset by peer, the statistic data will be inconsistent.
        // So we need drop some inconsistent data
        if (info.indexOfKey(frame_index) >= 0) {
            uint64_t *pipeline_period_value = info.valueFor(frame_index);
            if (pipeline_period_value[ts] == 0) {
                pipeline_period_value[ts] = value;
                if (ts == FIN_TS) {
                    uint32_t field = CAP_TS;
                    for (; field < FIN_TS; field++) {
                        if (pipeline_period_value[field+1] < pipeline_period_value[field]) {
                            DWLOGW("\t frame drop! TODO... (correct the statistic data)");
                            break;
                        }
                    }
                    if (field == FIN_TS && frame_index >= CALIBRATION_LEAST_TIMES) {
                        Calculate(frame_index);
                    }
                    pthread_mutex_lock(&mutex);
                    info.removeItem(frame_index);
                    pthread_mutex_unlock(&mutex);
                } else if (ts == ENC_TS) {
                    inputs = frame_index;
                } else if (ts == SND_TS) {
                    outputs = frame_index;
                }
                index[ts] = frame_index + 1;
            }
        } else {
            DWLOGE("Cannot find frame index for %llu", frame_index);
        }

    }
}

void Statistics::Revert(TimeStamp ts) {
    if (active) {
        ssize_t info_key_index = info.indexOfKey(index[ts]-1);
        if (info_key_index >= 0) {
            info.removeItem(info_key_index);
            index[ts]--;
        }
    }
}

void Statistics::Calculate(uint64_t frame_index) {
    uint64_t value;
    ssize_t info_key_index = info.indexOfKey(frame_index);
    if (info_key_index >= 0) {
        uint64_t *pipeline_period_value = info.valueFor(frame_index);
        for (uint32_t field = CAP_TS; field < FIN_TS; field++) {
            if (pipeline_period_value[field+1] > pipeline_period_value[field]) {
                value = pipeline_period_value[field+1] - pipeline_period_value[field];
                if (field == ENC_TS) {
                    value /= (inputs - outputs + 1);
                }
                if (value > time.max[field]) time.max[field] = value;
                if (value < time.min[field]) time.min[field] = value;
                time.avg[field] = (time.avg[field] * (frame_index - CALIBRATION_LEAST_TIMES) + value)
                                  / (frame_index - CALIBRATION_LEAST_TIMES + 1);
            }
        }
        value = pipeline_period_value[FIN_TS] - pipeline_period_value[CAP_TS];
        latency.accumulation += value;
        if (latency.max < value) {
            latency.max = value;
        }
        if (latency.min > value) {
            latency.min = value;
        }
    }
    if ((frame_index & ((1<<PRINT_INTERVAL_POWER)-1)) == 0) {
        Stat(frame_index);
        Show(frame_index);
    }
}

void Statistics::Stat(uint64_t index) {
    const char *format = "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu %*lu %*lu %*lu "
                         "%lu %lu %ld %ld "
                         "%*ld %*ld %*ld %*ld %*lu %*lu "
                         "%ld";

    char buf[256];
    FILE *proc = NULL;
    sprintf(buf,"/proc/%d/stat", pid);
    proc = fopen(buf, "r");
    if (proc) {
        if (5 == fscanf(proc, format, &s.utime, &s.stime, &s.cutime, &s.cstime, &s.rss)) {
            fclose(proc);
            uint64_t current = GetCurrentTimeStamp();
            process.cnt++;
            unsigned long usage = 100*JIFFY_TO_US*(s.utime+s.stime+s.cutime+s.cstime-ustime)/(current-last);
            if (usage > process.max.cpu) process.max.cpu = usage;
            if (usage < process.min.cpu) process.min.cpu = usage;
            if (s.rss > 0 && s.rss > process.max.mem) process.max.mem = s.rss;
            if (s.rss > 0 && s.rss < process.min.mem) process.min.mem = s.rss;
            process.cpu += usage;
            process.mem += s.rss;
            process.avg.cpu = process.cpu/process.cnt;
            ustime = s.utime+s.stime+s.cutime+s.cstime;
            last = current;
            process.cur.cpu = usage;
            process.avg.mem = process.mem/process.cnt;
            process.cur.mem = s.rss;
        }
    }
}

void Statistics::Show(uint64_t i)
{
    uint64_t duration = GetCurrentTimeStamp() - start;
    uint64_t avg = ((latency.accumulation+(1 <<(PRINT_INTERVAL_POWER-1)))>>PRINT_INTERVAL_POWER);
    uint64_t framerate = index[FIN_TS] * 1000000 / duration;
    uint64_t drops = inputs - outputs;
    uint64_t max = drops ? drops * duration / index[CAP_TS] : 0;
    uint64_t min = drops > 1 ? duration / index[CAP_TS] : 0;
    DWLOGW("statistics for %llu times, info size(%d), accumulation(%llu), duration(%llu), bitrate(%llu), frames(%llu), framerate(%llu), drops(%llu), diff(%llu):", i, info.size(), accumulation, duration, accumulation * 1000000 * 8/ duration, index[FIN_TS], framerate, drops, Diff());
    DWLOGW("average latency(%llu(%llu ~ %llu) of last 128 times, max latency(%llu), min latency(%llu):", avg, avg-max, avg-min, latency.max, latency.min);
    latency.accumulation = 0;
    DWLOGW("\ttime.avg: CAP(%llu), DIB(%llu), CVT(%llu), ENC(%llu), SND(%llu), RCV(%llu), DEC(%llu), CLR(%llu), CPY(%llu)", time.avg[CAP_TS], time.avg[DIB_TS], time.avg[CVT_TS], time.avg[ENC_TS], time.avg[SND_TS], time.avg[RCV_TS], time.avg[DEC_TS], time.avg[CLR_TS], time.avg[CPY_TS]);
    DWLOGW("\ttime.max: CAP(%llu), DIB(%llu), CVT(%llu), ENC(%llu), SND(%llu), RCV(%llu), DEC(%llu), CLR(%llu), CPY(%llu)", time.max[CAP_TS], time.max[DIB_TS], time.max[CVT_TS], time.max[ENC_TS], time.max[SND_TS], time.max[RCV_TS], time.max[DEC_TS], time.max[CLR_TS], time.avg[CPY_TS]);
    DWLOGW("\ttime.min: CAP(%llu), DIB(%llu), CVT(%llu), ENC(%llu), SND(%llu), RCV(%llu), DEC(%llu), CLR(%llu), CPY(%llu)", time.min[CAP_TS], time.min[DIB_TS], time.min[CVT_TS], time.min[ENC_TS], time.min[SND_TS], time.min[RCV_TS], time.min[DEC_TS], time.min[CLR_TS], time.avg[CPY_TS]);
    DWLOGW("\tcpu usage(%%) :%lu, avg:%lu, min:%lu, max:%lu", process.cur.cpu, process.avg.cpu, process.min.cpu, process.max.cpu);
    DWLOGW("\tmemory usage(MB) :%lu, avg:%lu, min:%lu, max:%lu", process.cur.mem, process.avg.mem, process.min.mem, process.max.mem);
}

bool Statistics::threadLoop() {
    char buffer[16];
    bzero(buffer, 16);
    if (calibration) {
        Message message = { count, TS_CNT };
        base = GetCurrentTimeStamp();
        if (write(utc, &message, sizeof(Message)) < 0)
            return false;
        calibration = false;
    }
    if (read(utc, buffer, 16) < 0)
        return false;
    Message* m = reinterpret_cast<Message*>(buffer);
    uint64_t current = GetCurrentTimeStamp();
    switch (m->ts) {
    case TS_CNT:
        samples[m->index & ((1<<CALIBRATION_SAMPLS_POWER)-1)] = ((current-base)+1)>>1;
        base = 0;
        count++;
        break;
    case RCV_TS:
    case DEC_TS:
    case CLR_TS:
    case CPY_TS:
        Stamp(m->ts, current - Diff(), m->index);
        break;
    case FIN_TS:
        Stamp(m->ts, current - Diff(), m->index);
        if (base == 0) {
            calibration = true;
        }
        break;
    default:
        break;
    }
    return true;
}

void Statistics::OnNewFrame(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset)
{
    Statistics::getInstance().Stamp(Statistics::SND_TS, GetCurrentTimeStamp());
}

void Statistics::OnFrameSize(uint32_t width, uint32_t height)
{
    AUTO_LOG();
}

}

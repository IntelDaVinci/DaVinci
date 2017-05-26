#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include "utils/Logger.h"
#include "utils/ConvUtils.h"

#include <utils/RefBase.h>
#include <utils/Singleton.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <utils/Thread.h>
#include <netinet/tcp.h>

#include <utils/KeyedVector.h>

#include "Parser.h"

#define PRINT_INTERVAL_POWER 7
#define JIFFY_TO_US 10000
#define CALIBRATION_SAMPLS_POWER 7
#define CALIBRATION_LEAST_TIMES 2048

namespace android {

class Statistics : public Observer,
                   public Thread,
                   public Singleton<Statistics> {
friend class Singleton<Statistics>;
public:
    enum TimeStamp {
        CAP_TS = 0,
        DIB_TS,
        CVT_TS,
        ENC_TS,
        SND_TS,
        RCV_TS,
        DEC_TS,
        CLR_TS,
        CPY_TS,
        FIN_TS,
        TS_CNT,
    };

    struct Status {
        unsigned long cpu;
        long mem;
    };

    void Stamp(TimeStamp ts, uint64_t value, uint64_t idx = 0);

    void Revert(TimeStamp ts);

    void Accumulate(uint32_t size)
    {
        if (active) {
            accumulation += size;
        }
    }

    void Enable()
    {
        Activate();
    }

    void Start(int fd);

    virtual inline void Input(uint8_t* data, uint32_t length)
    {
        if (active) {
            parser.Input(data, length);
        }
    }

    bool IsActive()
    {
        return active;
    }

protected:
    Statistics();
    Statistics(int fd);
    virtual ~Statistics();

private:
    Parser parser;
    KeyedVector<uint64_t /* index */, uint64_t* /* arrry of timestamps for each phase*/> info;
    int16_t pid;
    FILE *proc;
    uint64_t ustime;
    uint64_t last;
    int utc;
    uint64_t base;
    bool calibration;
    uint64_t samples[1<<CALIBRATION_SAMPLS_POWER];
    uint64_t count;
    uint64_t index[TS_CNT];
    pthread_mutex_t mutex, timutex;
    uint64_t accumulation;
    uint64_t start;
    uint64_t inputs;
    uint64_t outputs;
    bool active;

    struct Message {
        uint64_t index;
        TimeStamp ts;
    };

    struct {
        uint64_t avg[TS_CNT];
        uint64_t max[TS_CNT];
        uint64_t min[TS_CNT];
    } time;

    struct {
        uint64_t cpu;
        uint64_t mem;
        uint64_t cnt;
        Status cur;
        Status avg;
        Status max;
        Status min;
    } process;

    struct statStuff {
        unsigned long utime;  // %lu
        unsigned long stime;  // %lu
        long          cutime; // %ld
        long          cstime; // %ld
        long          rss;    // %ld
    } s;

    struct {
        uint64_t max;
        uint64_t min;
        uint64_t accumulation;
    } latency;

    void Calculate(uint64_t index);
    void Stat(uint64_t index);
    inline void Show(uint64_t i);
    virtual bool threadLoop();
    inline uint64_t Diff() { uint64_t total = 0; uint32_t m = 0; for (uint32_t i = 0; i < (1<<CALIBRATION_SAMPLS_POWER); ++i) { if (samples[i]) { total += samples[i]; m++; } } return m ? total/m : 0; }
    inline void Activate() { active = true; }

    virtual void OnNewFrame(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset);
    virtual void OnFrameSize(uint32_t width, uint32_t height);
};

}

#endif

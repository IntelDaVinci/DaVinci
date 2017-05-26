#ifndef STATISTICS_H
#define STATISTICS_H

#include <windows.h>
#include <stdio.h>
#include <map>

#include "h264_stream_parser.h"

#include "logger.h"
#include <tchar.h>

enum TimeStamp
{
    CAP_TS = 0,
    DIB_TS,
    CVT_TS,
    ENC_TS,
    SND_TS,
    RCV_TS,  // timestamp at one h264 frame(I/P) received;
    DEC_TS,  // tiemstamp at one h264 frame(I/P) input to decoder;
    CLR_TS,  // timestamp at one decoded frame output by decoder;
    CPY_TS,  // timestamp at one decoded frame copied to DaVinci;
    FIN_TS,  // timestamp at one frame has been handled;
    TS_CNT,
};

struct Message
{
    unsigned long long index;
    TimeStamp ts;
};

class CStatistics : public CVideoParserObserver
{
public:
    struct Stat {
        mfxU64 m_frame_count;
        Stat() : m_frame_count(0) {}
    } stat;
    CStatistics();
    ~CStatistics();

    void Stamp(TimeStamp ts, mfxU64 index);
    CVideoParser* Parser(TimeStamp ts);
    inline void Input(TimeStamp ts, mfxBitstream* mfxBS, mfxU32 offset, mfxU32 length) { Parser(ts)->Input(mfxBS, offset, length); }
    inline void Input(TimeStamp ts, mfxU8* data, mfxU32 length) { Parser(ts)->Input(data, length); }
    inline mfxU64 Count() { return m_dec_count; }

    static CStatistics& GetSingleObject() { static CStatistics statistics; return statistics; }
    bool Initialize(const char *ip, unsigned short port);
    static unsigned int MSDK_THREAD_CALLCONVENTION UtcThread(void* param);

    virtual void OnNewFrame(CVideoParser* parser, mfxU64 frame_num);
    virtual void OnFrameSize(CVideoParser* parser, mfxU32 width, mfxU32 height);
private:

    std::map<CVideoParser*, struct Stat> parsers;
    SOCKET m_socketStatisticsSocket;
    mfxU64 m_rcv_count, m_dec_count;
    MSDKThread *m_pUtcThread;
};

#endif
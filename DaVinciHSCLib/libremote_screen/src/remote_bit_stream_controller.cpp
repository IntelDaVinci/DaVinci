#ifndef __H264_REMOTE_FRAME_READER_H__
#define __H264_REMOTE_FRAME_READER_H__

#include "mfxcommon.h"
#include "vm/thread_defs.h"
#include "message_queue.h"
#include "channel.h"
#include "statistics.h"

#include <vector>

class CRemoteBitStreamReader {
public :
    CRemoteBitStreamReader();
    virtual ~CRemoteBitStreamReader();

    mfxStatus Start(const char *strServerAddress, unsigned short numServerPort);
    void      Close();
    void      Wait(unsigned int msec);
    void      Wait();
    mfxStatus ReadNextFrame(mfxBitstream *pBS);
    void      SetStatMode(bool statMode) { m_bStatMode = statMode; }

    static unsigned int MSDK_THREAD_CALLCONVENTION ReceiveThreadFunc(void* ctx);

private:
    CRemoteBitStreamReader(const CRemoteBitStreamReader& obj);
    CRemoteBitStreamReader& operator=(CRemoteBitStreamReader& obj);

protected:
    char                     m_strServerAddress[512];
    int                      m_numServerPort;
    CMessageQueue            m_crRingBuffer;
    CChannel                 m_channel;
    MSDKThread              *m_pDeliverThread;
    MSDKEvent                m_semaphore;
    mfxStatus                m_mfxStatus;
    bool                     m_bStopThread;
    bool                     m_bStatMode;
};

#endif

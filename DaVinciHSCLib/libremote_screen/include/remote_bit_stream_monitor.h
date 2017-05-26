#ifndef __H264_REMOTE_BIT_STREAM_MONITOR_H__
#define __H264_REMOTE_BIT_STREAM_MONITOR_H__

#include "mfxcommon.h"
#include "vm/thread_defs.h"
#include "channel.h"

#include <vector>

#include "remote_screen.h"

class CMonitorObserver {
public:
    virtual void ObserveStatus(int status, char *message) = 0;
};

class CRemoteBitStreamMonitor {
public :
    CRemoteBitStreamMonitor();
    virtual ~CRemoteBitStreamMonitor();

    mfxStatus Attach(const char *strServerAddress, unsigned short numServerPort);
    mfxStatus MonitorTarget();
    RS_STATUS CheckTargetStatus();
    void      RegisterObserver(CMonitorObserver *monitor_observer);
    void      Detach();

    static unsigned int MSDK_THREAD_CALLCONVENTION ReceiveThreadFunc(void* ctx);

private:
    CRemoteBitStreamMonitor(const CRemoteBitStreamMonitor& obj);
    CRemoteBitStreamMonitor& operator=(CRemoteBitStreamMonitor& obj);

protected:
    char                            m_strServerAddress[512];
    int                             m_numServerPort;
    CChannel                        m_channel;
    MSDKThread                     *m_pDeliverThread;
    mfxStatus                       m_mfxStatus;
    bool                            m_bStopThread;
    std::vector<CMonitorObserver*>  m_observers;
};

#endif

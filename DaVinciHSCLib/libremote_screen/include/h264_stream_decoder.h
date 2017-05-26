#ifndef _H264_STREAM_DECODER_H_
#define _H264_STREAM_DECODER_H_

#include "mfxcommon.h"
#include "mfxvideo++.h"
#include "mfxstructures.h"
#include "vm/thread_defs.h"
#include "statistics.h"

#include "remote_screen.h"
#include "remote_bit_stream_reader.h"
#include "remote_bit_stream_monitor.h"
#include <fstream>

class CH264StreamDecoder : public CMonitorObserver {
public:
    CH264StreamDecoder();
    ~CH264StreamDecoder();

    mfxStatus Start(const char *p2p_server_address, int p2p_server_port, bool stat_mode, RS_STATUS &rs_status);
    void Stop();
    int GetCurrentRgbaSurface(unsigned char *surface_buffer, int len);
    void ObserveStatus(int status, char *message);

    static unsigned int MSDK_THREAD_CALLCONVENTION DecodeFrameFunc(void* ctx);
    static unsigned int MSDK_THREAD_CALLCONVENTION MonitorDeviceFunc(void* ctx);

private:
    CH264StreamDecoder(const CH264StreamDecoder& obj);
    CH264StreamDecoder& operator=(CH264StreamDecoder& obj);

private:
    mfxStatus CreateCodec();
    mfxStatus InitializeCodec(mfxVideoParam *, mfxVideoParam *);
    mfxStatus DecodeRemoteStreamHeader(mfxBitstream *, mfxVideoParam *);

    void ConfigVPPParams(mfxVideoParam *, mfxVideoParam *);
    mfxStatus QueryRequiredVPPSurfaces(mfxVideoParam *, mfxFrameAllocRequest *);
    mfxStatus QueryRequiredDecSurfaces(mfxVideoParam *, mfxFrameAllocRequest *);
    mfxStatus AllocYUVSurfaceBuffer(int, int, int, mfxFrameInfo *, mfxU8 **, mfxFrameSurface1 ***);
    mfxStatus AllocRGBASurfaceBuffer(int, int, int, mfxFrameInfo *, mfxU8 **, mfxFrameSurface1 ***);

    int GetFreeSurfaceIndex(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize);

private:
    CRemoteBitStreamReader   m_remoteBitStreamReader;
    CRemoteBitStreamMonitor  m_remoteBitStreamMonitor;
    mfxU8                   *m_currentRgbaData;
    MSDKThread              *m_decodeThread;
    MSDKMutex                m_syncSurface;
    MFXVideoSession         *m_mfxSession;
    MFXVideoDECODE          *m_mfxDEC;
    MFXVideoVPP             *m_mfxVPP;
    bool                     m_bStopThread;
    bool                     m_bStatMode;
    mfxU64                   m_uFrames[TS_CNT];
};

#endif

#include "h264_stream_decoder.h"

#include "remote_screen.h"
#include "logger.h"
#include "mfxvideo++.h"
#include <fstream>
#include <assert.h>

#define MSDK_ALIGN16(value) (((value + 15) >> 4) << 4)
#define MSDK_ALIGN32(X)     (((mfxU32)((X)+31)) & (~ (mfxU32)31))

extern _CaptureFrameCallBack     g_on_capture_frame;
extern _GeneralExceptionCallBack g_on_exception;

unsigned int MSDK_THREAD_CALLCONVENTION CH264StreamDecoder::DecodeFrameFunc(void* ctx)
{
    mfxStatus            sts                 = MFX_ERR_UNKNOWN;
    mfxU32               nFrame              = 0;
    mfxBitstream         mfxBS               = {0};

    mfxSyncPoint          syncpDec           = NULL;
    mfxFrameSurface1     *pmfxOutSurface     = NULL;
    mfxFrameSurface1    **pmfxDecSurfaces    = NULL;
    mfxU8                *pDecSurfaceBuffers = NULL;
    mfxVideoParam         mfxDecVideoParams  = {0};
    int                   numDecSurfaces     = 0;
    int                   nIndexDec          = 0;
    
    mfxSyncPoint          syncpVPP           = NULL;
    mfxFrameSurface1    **pmfxVPPSurfaces    = NULL;
    mfxU8                *pVPPSurfaceBuffers = NULL;
    mfxVideoParam         mfxVPPVideoParams  = {0};
    int                   numVPPSurfaces     = 0;
    int                   nIndexVPP          = 0;

    AUTO_LOG();

    CH264StreamDecoder *current_decoder = (CH264StreamDecoder *)ctx;
    sts = current_decoder->CreateCodec();
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot create codec !!!");
        goto DONE_LBL;
    }

    // Set required video parameters for decode
    memset(&mfxDecVideoParams, 0, sizeof(mfxDecVideoParams));
    mfxDecVideoParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxDecVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    
    // Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = 1024 * 1024;
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    if (mfxBS.Data == NULL) {
        sts = MFX_ERR_MEMORY_ALLOC;
        LOGE("Unable to malloc memory for media sdk bit stream buffer !!!");
        goto DONE_LBL;
    }

    sts = current_decoder->DecodeRemoteStreamHeader(&mfxBS, &mfxDecVideoParams);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot decode remote stream header !!!");
        goto DONE_LBL;
    }

    // Initialize VPP parameters
    // - For simplistic memory management, system memory surfaces are used to store the raw frames
    //   (Note that when using HW acceleration D3D surfaces are prefered, for better performance)
    memset(&mfxVPPVideoParams, 0, sizeof(mfxVPPVideoParams));
    current_decoder->ConfigVPPParams(&mfxVPPVideoParams, &mfxDecVideoParams);

    // Query number of required surfaces for VPP
    mfxFrameAllocRequest VPPRequest[2];// [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest)*2);
    sts = current_decoder->QueryRequiredVPPSurfaces(&mfxVPPVideoParams, VPPRequest);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot query number of required surfaces for VPP !!!");
        goto DONE_LBL;
    }

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest DecRequest;
    memset(&DecRequest, 0, sizeof(DecRequest));
    sts = current_decoder->QueryRequiredDecSurfaces(&mfxDecVideoParams, &DecRequest);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot query number of required surfaces for decoder !!!");
        goto DONE_LBL;
    }

    // Allocate surfaces for decoder
    // - Width and height of buffer must be aligned, a multiple of 32 
    // - Frame surface array keeps pointers all surface planes and general frame info
    numDecSurfaces = DecRequest.NumFrameSuggested + VPPRequest[0].NumFrameSuggested;
    sts = current_decoder->AllocYUVSurfaceBuffer(DecRequest.Info.Width,
                                                 DecRequest.Info.Height,
                                                 numDecSurfaces,
                                                 &(mfxDecVideoParams.mfx.FrameInfo),
                                                 &pDecSurfaceBuffers,
                                                 &pmfxDecSurfaces);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot allocate surfaces for decoder !!!");
        goto DONE_LBL;
    }

    numVPPSurfaces = numDecSurfaces * 2;
    sts = current_decoder->AllocRGBASurfaceBuffer(VPPRequest[1].Info.Width,
                                                  VPPRequest[1].Info.Height,
                                                  numVPPSurfaces,
                                                  &(mfxVPPVideoParams.vpp.Out),
                                                  &pVPPSurfaceBuffers,
                                                  &pmfxVPPSurfaces);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot allocate surfaces for decoder !!!");
        goto DONE_LBL;
    }

    // Initialize the Media SDK decoder
    sts = current_decoder->InitializeCodec(&mfxDecVideoParams, &mfxVPPVideoParams);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        sts = MFX_ERR_NONE;
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot initialize the Media SDK decoder !!!");
        goto DONE_LBL;
    }

    // ===============================================================
    // Start decoding the frames from the stream
    //

    //
    // Stage 1: Main decoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts)
    {
        if (current_decoder->m_bStopThread)
            break;

        if (MFX_WRN_DEVICE_BUSY == sts)
            Sleep(1); // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            LOGI("Need more data, so start reading next frame <<<<<");
            while (true)
            {
                sts = current_decoder->m_remoteBitStreamReader.ReadNextFrame(&mfxBS); // Read more data into input bit stream
                if (sts == MFX_ERR_ABORTED || sts == MFX_WRN_VALUE_NOT_CHANGED)
                    goto CLOSE_CODEC_LBL;
                else if (sts == MFX_ERR_NONE)
                    break;
                else if(sts == MFX_ERR_MORE_DATA) {
                    current_decoder->m_remoteBitStreamReader.Wait();
                }
            }
            LOGI("Need more data, so start reading next frame >>>>>");
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
        {
            nIndexDec = current_decoder->GetFreeSurfaceIndex(pmfxDecSurfaces, numDecSurfaces); // Find free frame surface
            if (MFX_ERR_NOT_FOUND == nIndexDec) {
                sts = MFX_ERR_MEMORY_ALLOC;
                LOGE("Cannot find free decoder frame surface");
                goto CLOSE_CODEC_LBL;
            }
        }
        
        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
        sts = current_decoder->m_mfxDEC->DecodeFrameAsync(&mfxBS, pmfxDecSurfaces[nIndexDec], &pmfxOutSurface, &syncpDec);

        // Ignore warnings if output is available, 
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpDec) 
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = current_decoder->m_mfxSession->SyncOperation(syncpDec, 60000); // Synchronize. Wait until decoded frame is ready

        if (MFX_ERR_NONE == sts)
        {
            if (current_decoder->m_bStatMode) {
                CStatistics::GetSingleObject().Stamp(CLR_TS, current_decoder->m_uFrames[CLR_TS]++);
            }
            nIndexVPP = current_decoder->GetFreeSurfaceIndex(pmfxVPPSurfaces, numVPPSurfaces); // Find free frame surface 
            if (MFX_ERR_NOT_FOUND == nIndexVPP) {
                LOGE("Cannot find free VPP frame surface");
                goto CLOSE_CODEC_LBL;
            }

            for (;;)
            {
                // Process a frame asychronously (returns immediately)
                sts = current_decoder->m_mfxVPP->RunFrameVPPAsync(pmfxOutSurface, pmfxVPPSurfaces[nIndexVPP], NULL, &syncpVPP);

                if (MFX_ERR_NONE < sts && !syncpVPP) // repeat the call if warning and no output
                {
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        Sleep(1); // wait if device is busy
                }
                else if (MFX_ERR_NONE < sts && syncpVPP)
                {
                    sts = MFX_ERR_NONE; // ignore warnings if output is available
                    break;
                }
                else 
                    break; // not a warning
            } 

            // VPP needs more data, let decoder decode another frame as input
            if (MFX_ERR_MORE_DATA == sts)
            {
                continue;
            }
            else if (MFX_ERR_MORE_SURFACE == sts)
            {
                // Not relevant for the illustrated workload! Therefore not handled.
                // Relevant for cases when VPP produces more frames at output than consumes at input. E.g. framerate conversion 30 fps -> 60 fps
                break;
            }
            else if (sts == MFX_ERR_NONE)
            {
                sts = current_decoder->m_mfxSession->SyncOperation(syncpVPP, 60000);
                if (sts != MFX_ERR_NONE) {
                    LOGE("VPP error: %d", sts);
                    goto CLOSE_CODEC_LBL;
                }

                LOGI("Frame available");
                current_decoder->m_syncSurface.Lock();
                current_decoder->m_currentRgbaData = pVPPSurfaceBuffers + VPPRequest[1].Info.Width * VPPRequest[1].Info.Height * 4 * nIndexVPP;
                if (g_on_capture_frame != NULL) {
                    LOGI("Start delivering frame......");
                    g_on_capture_frame(pmfxOutSurface->Info.CropW, pmfxOutSurface->Info.CropH);
                    LOGI( "Done delivering frame......");
                }
                current_decoder->m_syncSurface.Unlock();
            }
            else {
                LOGE("VPP error: %d", sts);
                goto CLOSE_CODEC_LBL;
            }

        }
    }

CLOSE_CODEC_LBL:
    current_decoder->m_mfxDEC->Close();
    current_decoder->m_mfxVPP->Close();

DONE_LBL:
    if (mfxBS.Data != NULL)
        delete[] mfxBS.Data;

    if (pDecSurfaceBuffers != NULL) {
        delete[] pDecSurfaceBuffers;
        pDecSurfaceBuffers = NULL;
    }

    if (pmfxDecSurfaces != NULL) {
        for (int i = 0; i < numDecSurfaces; i++) {
            delete pmfxDecSurfaces[i];
            pmfxDecSurfaces[i] = NULL;
        }
        delete[] pmfxDecSurfaces;
        pmfxDecSurfaces = NULL;
    }
    
    if (pVPPSurfaceBuffers != NULL) {
        delete[] pVPPSurfaceBuffers;
        pVPPSurfaceBuffers = NULL;
    }

    if (pmfxVPPSurfaces != NULL) {
        for (int i = 0; i < numVPPSurfaces; i++) {
            delete pmfxVPPSurfaces[i];
            pmfxVPPSurfaces[i] = NULL;
        }
        delete[] pmfxVPPSurfaces;
        pmfxVPPSurfaces = NULL;
    }

    // Trigger exception
    if (sts != MFX_ERR_NONE && sts != MFX_WRN_VALUE_NOT_CHANGED && !current_decoder->m_bStopThread) {
        if (g_on_exception != NULL) {
            g_on_exception(RS_CODEC_ABORTED);
        }
    }

    return sts;
}

CH264StreamDecoder::CH264StreamDecoder()
{
    AUTO_LOG();
    m_currentRgbaData = NULL;
    m_mfxSession      = NULL;
    m_mfxDEC          = NULL;
    m_mfxVPP          = NULL;
    m_decodeThread    = NULL;
    m_bStopThread     = false;
}

CH264StreamDecoder::~CH264StreamDecoder()
{
    AUTO_LOG();
    if (m_mfxVPP != NULL) {
        delete m_mfxVPP;
        m_mfxVPP = NULL;
    }

    if (m_mfxDEC != NULL) {
        delete m_mfxDEC;
        m_mfxDEC = NULL;
    }

    if (m_mfxSession != NULL) {
        delete m_mfxSession;
        m_mfxSession = NULL;
    }

    if (m_decodeThread != NULL) {
        delete m_decodeThread;
        m_decodeThread = NULL;
    }
}

mfxStatus CH264StreamDecoder::Start(const char *p2p_server_address, int p2p_server_port, bool stat_mode, RS_STATUS &rs_status)
{
    AUTO_LOG();
    mfxStatus sts;
    m_bStatMode = stat_mode;

    // Monitor remote target device(e.x. Media Encoder, Screen Capture)
    sts = m_remoteBitStreamMonitor.Attach(p2p_server_address, p2p_server_port);
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot monitor target device status.");
        return sts;
    }

    // Read media stream which is encoded by remote target device
    m_remoteBitStreamReader.SetStatMode(stat_mode);
    sts = m_remoteBitStreamReader.Start(p2p_server_address, (unsigned short)p2p_server_port);
    if (MFX_ERR_NONE != sts) {
        LOGE("Cannot get target device video stream.");
        m_remoteBitStreamMonitor.Detach();
        return sts;
    }

    // The statistic initialization operation must be done before check target status.
    // Because the android HSC agent will start encode stream on succeeded in starting encoder,
    // but check target status only can return after encoder has started.
    if (m_bStatMode) {
        CStatistics::GetSingleObject().Initialize(p2p_server_address, (unsigned short)p2p_server_port);
    }

    // Check device status
    rs_status = m_remoteBitStreamMonitor.CheckTargetStatus();
    if (rs_status != RS_SUCCESS) {
        m_remoteBitStreamMonitor.Detach();
        m_remoteBitStreamReader.Close();
        return MFX_ERR_NOT_INITIALIZED;
    } else {
        m_remoteBitStreamMonitor.RegisterObserver(this);
        m_remoteBitStreamMonitor.MonitorTarget();
    }

    m_bStopThread = false;

    m_decodeThread = new MSDKThread(sts, DecodeFrameFunc, this);
    sts = m_decodeThread->SetPriority(PRIORITY_ABOVE_NORMAL);
    if (sts != MFX_ERR_NONE) {
        LOGW("Cannot set decoder thread priority !!!");
        sts = MFX_ERR_NONE;
    }

    return sts;
}

void CH264StreamDecoder::Stop()
{
    AUTO_LOG();

    // NOTE: Don't reorder the code position.
    // If the device doesn't send anything to the host, but the socket is still alive,
    // so if we don't close remote bit stream reader first, the decode thread will hang.
    // Because the decode thread always waits for the device sent data.

    m_bStopThread = true;

    m_remoteBitStreamMonitor.Detach();
    m_remoteBitStreamReader.Close();

    if (m_decodeThread != NULL) {
        if (m_decodeThread->GetExitCode() == MFX_TASK_WORKING) {
            m_decodeThread->Wait();
        }
    }
}

mfxStatus CH264StreamDecoder::CreateCodec()
{
    AUTO_LOG();
    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW accelaration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxStatus  sts  = MFX_ERR_NONE;
    mfxIMPL    impl = MFX_IMPL_HARDWARE;
    mfxVersion ver  = {0, 1};

    m_mfxSession = new MFXVideoSession();
    assert(m_mfxSession != NULL);
    sts = m_mfxSession->Init(impl, &ver);
    if (sts != MFX_ERR_NONE) {
        LOGW("Current host does not support media codec hardware acceleration !!!");
        impl = MFX_IMPL_SOFTWARE;
        sts = m_mfxSession->Init(impl, &ver);
        if (sts != MFX_ERR_NONE) {
            LOGE("There is no software media codec on current host !!!");
            return sts;
        }
    }

    // Create Media SDK decoder
    m_mfxDEC = new MFXVideoDECODE(*m_mfxSession);
    assert(m_mfxDEC != NULL);

    // Create Media SDK VPP component
    m_mfxVPP = new MFXVideoVPP(*m_mfxSession);
    assert(m_mfxVPP != NULL);

    return MFX_ERR_NONE;
}


mfxStatus CH264StreamDecoder::InitializeCodec(mfxVideoParam *pMfxDecVideoParams, mfxVideoParam *pMfxVPPVideoParams)
{
    AUTO_LOG();
    assert(pMfxDecVideoParams != NULL);
    assert(pMfxVPPVideoParams != NULL);
    assert(m_mfxDEC != NULL);
    assert(m_mfxVPP != NULL);
    
    mfxStatus sts;
    memset(m_uFrames, 0, sizeof(m_uFrames));
    // Initialize the Media SDK decoder
    sts = m_mfxDEC->Init(pMfxDecVideoParams);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        sts = MFX_ERR_NONE;
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot initialize decoder !!! %d", sts);
        return sts;
    }

    // Initialize the VPP
    sts = m_mfxVPP->Init(pMfxVPPVideoParams);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        sts = MFX_ERR_NONE;
    if (sts != MFX_ERR_NONE) {
        LOGE("Cannot initialize VPP !!! %d", sts);
        return sts;
    }

    return sts;
}

mfxStatus CH264StreamDecoder::DecodeRemoteStreamHeader(mfxBitstream *mfxBS, mfxVideoParam *mfxVideoParams)
{
    AUTO_LOG();
    mfxStatus sts = MFX_ERR_UNKNOWN;

    assert(m_mfxDEC != NULL);
    assert(mfxBS != NULL);
    assert(mfxVideoParams != NULL);

    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    while (!m_bStopThread) {
        sts = m_mfxDEC->DecodeHeader(mfxBS, mfxVideoParams);
        if (sts == MFX_WRN_PARTIAL_ACCELERATION)
            sts = MFX_ERR_NONE;

        if (sts == MFX_ERR_NONE)
            break;

        if (sts == MFX_ERR_MORE_DATA) {
            while (!m_bStopThread) {
                sts = m_remoteBitStreamReader.ReadNextFrame(mfxBS);
                if (sts == MFX_ERR_ABORTED)
                    goto DONE_LBL;

                if (sts == MFX_ERR_NONE)
                    break;

                if (sts == MFX_ERR_MORE_DATA)
                    m_remoteBitStreamReader.Wait(500);
            }
        }
    }

DONE_LBL:
    return sts;
}

void CH264StreamDecoder::ConfigVPPParams(mfxVideoParam *pVppParams, mfxVideoParam *pVideoParams)
{
    AUTO_LOG();
    assert(pVppParams != NULL);
    assert(pVideoParams != NULL);

    // Input data
    pVppParams->vpp.In.FourCC         = MFX_FOURCC_NV12;
    pVppParams->vpp.In.ChromaFormat   = MFX_CHROMAFORMAT_YUV420;  
    pVppParams->vpp.In.CropX          = 0;
    pVppParams->vpp.In.CropY          = 0; 
    pVppParams->vpp.In.CropW          = pVideoParams->mfx.FrameInfo.CropW;
    pVppParams->vpp.In.CropH          = pVideoParams->mfx.FrameInfo.CropH;
    pVppParams->vpp.In.PicStruct      = MFX_PICSTRUCT_PROGRESSIVE;
    pVppParams->vpp.In.FrameRateExtN  = 60;
    pVppParams->vpp.In.FrameRateExtD  = 1;

    // width must be a multiple of 16 
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture  
    pVppParams->vpp.In.Width  = MSDK_ALIGN16(pVppParams->vpp.In.CropW);
    pVppParams->vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == pVppParams->vpp.In.PicStruct)?
                                 MSDK_ALIGN16(pVppParams->vpp.In.CropH) : MSDK_ALIGN32(pVppParams->vpp.In.CropH);
    // Output data
    pVppParams->vpp.Out.FourCC        = MFX_FOURCC_RGB4;
    pVppParams->vpp.Out.CropX         = 0;
    pVppParams->vpp.Out.CropY         = 0; 
    pVppParams->vpp.Out.CropW         = pVppParams->vpp.In.CropW;  // Resize to half size resolution
    pVppParams->vpp.Out.CropH         = pVppParams->vpp.In.CropH;
    pVppParams->vpp.Out.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    pVppParams->vpp.Out.FrameRateExtN = 60;
    pVppParams->vpp.Out.FrameRateExtD = 1;
    // width must be a multiple of 16 
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    pVppParams->vpp.Out.Width  = MSDK_ALIGN16(pVppParams->vpp.Out.CropW); 
    pVppParams->vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == pVppParams->vpp.Out.PicStruct)?
                                    MSDK_ALIGN16(pVppParams->vpp.Out.CropH) : MSDK_ALIGN32(pVppParams->vpp.Out.CropH);
    pVppParams->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
}

mfxStatus CH264StreamDecoder::QueryRequiredVPPSurfaces(mfxVideoParam *pVPPParams, mfxFrameAllocRequest *pVPPRequest)
{
    AUTO_LOG();
    assert(m_mfxVPP != NULL);
    assert(pVPPParams != NULL);
    assert(pVPPRequest != NULL);

    mfxStatus sts = MFX_ERR_NONE;
    sts = m_mfxVPP->QueryIOSurf(pVPPParams, pVPPRequest);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        sts = MFX_ERR_NONE;
    return sts;
}

mfxStatus CH264StreamDecoder::QueryRequiredDecSurfaces(mfxVideoParam *pDecVideoParams, mfxFrameAllocRequest *pDecRequest)
{
    AUTO_LOG();
    assert(m_mfxDEC != NULL);
    assert(pDecVideoParams != NULL);
    assert(pDecRequest != NULL);

    mfxStatus sts = MFX_ERR_NONE;
    sts = m_mfxDEC->QueryIOSurf(pDecVideoParams, pDecRequest);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        sts = MFX_ERR_NONE;
    return sts;
}


mfxStatus CH264StreamDecoder::AllocYUVSurfaceBuffer(int w, int h, int numSurfaces, mfxFrameInfo *pFrameInfo, mfxU8 **pSurfaceBuffer, mfxFrameSurface1 ***pmfxSurfaces)
{
    AUTO_LOG();
    mfxStatus sts;
    mfxU16 numDecWidth;
    mfxU16 numDecHeight;
    mfxU8  numDecBitsPerPixel;  // NV12 format is a 12 bits per pixel format
    mfxU32 numDecSurfaceSize;

    assert(pFrameInfo != NULL);

    numDecWidth = (mfxU16)MSDK_ALIGN32(w);
    numDecHeight = (mfxU16)MSDK_ALIGN32(h);
    numDecBitsPerPixel = 12;  // NV12 format is a 12 bits per pixel format
    numDecSurfaceSize = numDecWidth * numDecHeight * numDecBitsPerPixel / 8;
    *pSurfaceBuffer = (mfxU8 *)new mfxU8[numDecSurfaceSize * numSurfaces];
    if (*pSurfaceBuffer == NULL) {
        sts = MFX_ERR_MEMORY_ALLOC;
        return sts;
    }

    *pmfxSurfaces = new mfxFrameSurface1*[numSurfaces];
    if (*pmfxSurfaces == NULL) {
        sts = MFX_ERR_MEMORY_ALLOC;
        delete[] (*pSurfaceBuffer);
        return sts;
    }

    for (int i = 0; i < numSurfaces; i++)
    {
        (*pmfxSurfaces)[i] = new mfxFrameSurface1;
        memset((*pmfxSurfaces)[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&((*pmfxSurfaces)[i]->Info), pFrameInfo, sizeof(mfxFrameInfo));
        (*pmfxSurfaces)[i]->Data.Y = &(*pSurfaceBuffer)[numDecSurfaceSize * i];
        (*pmfxSurfaces)[i]->Data.U = (*pmfxSurfaces)[i]->Data.Y + numDecWidth * numDecHeight;
        (*pmfxSurfaces)[i]->Data.V = (*pmfxSurfaces)[i]->Data.U + 1;
        (*pmfxSurfaces)[i]->Data.Pitch = numDecWidth;
    }

    return MFX_ERR_NONE;
}

mfxStatus CH264StreamDecoder::AllocRGBASurfaceBuffer(int w, int h, int numSurfaces, mfxFrameInfo *pFrameInfo, mfxU8 **pSurfaceBuffer, mfxFrameSurface1 ***pmfxSurfaces)
{
    AUTO_LOG();
    mfxStatus sts;
    mfxU16 numDecWidth;
    mfxU16 numDecHeight;
    mfxU8  numDecBitsPerPixel;  // RGBA
    mfxU32 numDecSurfaceSize;

    assert(pFrameInfo != NULL);

    numDecWidth = (mfxU16)(w);
    numDecHeight = (mfxU16)(h);
    numDecBitsPerPixel = 32;  // RGBA
    numDecSurfaceSize = numDecWidth * numDecHeight * numDecBitsPerPixel / 8;
    *pSurfaceBuffer = (mfxU8 *)new mfxU8[numDecSurfaceSize * numSurfaces];
    if (*pSurfaceBuffer == NULL) {
        sts = MFX_ERR_MEMORY_ALLOC;
        return sts;
    }

    *pmfxSurfaces = new mfxFrameSurface1*[numSurfaces];
    if (*pmfxSurfaces == NULL) {
        sts = MFX_ERR_MEMORY_ALLOC;
        delete[] (*pSurfaceBuffer);
        return sts;
    }

    for (int i = 0; i < numSurfaces; i++)
    {
        (*pmfxSurfaces)[i] = new mfxFrameSurface1;
        memset((*pmfxSurfaces)[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&((*pmfxSurfaces)[i]->Info), pFrameInfo, sizeof(mfxFrameInfo));
        (*pmfxSurfaces)[i]->Data.R = &(*pSurfaceBuffer)[numDecSurfaceSize * i];
        (*pmfxSurfaces)[i]->Data.G = (*pmfxSurfaces)[i]->Data.R + 1;
        (*pmfxSurfaces)[i]->Data.B = (*pmfxSurfaces)[i]->Data.R + 2;
        (*pmfxSurfaces)[i]->Data.A = (*pmfxSurfaces)[i]->Data.R + 3;
        (*pmfxSurfaces)[i]->Data.Pitch = numDecWidth * 4;
    }

    return MFX_ERR_NONE;
}

// Get free raw frame surface
int CH264StreamDecoder::GetFreeSurfaceIndex(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize)
{
    AUTO_LOG();
    if (pSurfacesPool)
        for (mfxU16 i = 0; i < nPoolSize; i++)
            if (0 == pSurfacesPool[i]->Data.Locked)
                return i;
    return MFX_ERR_NOT_FOUND;
}

int CH264StreamDecoder::GetCurrentRgbaSurface(unsigned char *surface_buffer, int len)
{
    AUTO_LOG();
    if (m_bStatMode) {
        CStatistics::GetSingleObject().Stamp(CPY_TS, m_uFrames[CPY_TS]++);
    }

    m_syncSurface.Lock();
    memcpy(surface_buffer, m_currentRgbaData, len);
    m_syncSurface.Unlock();
    if (m_bStatMode) {
        CStatistics::GetSingleObject().Stamp(FIN_TS, m_uFrames[FIN_TS]++);
    }

    return len;
}

void CH264StreamDecoder::ObserveStatus(int status, char *message)
{
    AUTO_LOG();

    LOGI("Target device message:%d-%s", status, message);

    if (status != RS_SUCCESS) {
        LOGE("H264 stream decoder error:[%d]-%s", status, message);
        if (g_on_exception != NULL) {
            g_on_exception(status);
        }
    }
}

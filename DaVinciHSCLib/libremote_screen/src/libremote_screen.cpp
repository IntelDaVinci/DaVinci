// libremote_screen.cpp : Defines the exported functions for the DLL application.
//
#include "remote_screen.h"

#include "logger.h"
#include "h264_stream_decoder.h"
#include "remote_bit_stream_reader.h"
#include "vm/thread_defs.h"

static CH264StreamDecoder    *g_h264_stream_decoder;
_CaptureFrameCallBack         g_on_capture_frame;
_GeneralExceptionCallBack     g_on_exception;
unsigned short                g_log_level = LOG_ERRO;
MSDKMutex                     g_sync_api;

RS_STATUS InitializeRSModule()
{
  AUTO_LOG();
  return RS_SUCCESS;
}

void ConfigLogLevel(LOG_LEVEL log_level)
{
  g_log_level = log_level;
  AUTO_LOG();
}

void UninitializeRSModule()
{
  LOGI("%s <<<<<<<<<<", __FUNCTION__);
  CLogger::DestoryInstance();
}

DllDecl RS_STATUS Start(
  const char                *p2p_server_address,
  int                        p2p_server_port,
  int                        p2p_peer_id,
  bool                       stat_mode,
  _CaptureFrameCallBack      on_capture_frame,
  _GeneralExceptionCallBack  on_exception
  )
{
    AUTO_LOG();
    AutoLocker auto_locker(g_sync_api);
    RS_STATUS  rs_status  = RS_SUCCESS;
    mfxStatus  dec_status = MFX_ERR_NONE;

    if (g_h264_stream_decoder == NULL) {
        g_h264_stream_decoder = new CH264StreamDecoder();
        assert(g_h264_stream_decoder != NULL);
    } else {
        return RS_SUCCESS;
    }

    dec_status = g_h264_stream_decoder->Start(p2p_server_address, (unsigned short)p2p_server_port, stat_mode, rs_status);
    if (MFX_ERR_NONE != dec_status || RS_SUCCESS != rs_status) {
        if (g_h264_stream_decoder != NULL) {
            delete g_h264_stream_decoder;
            g_h264_stream_decoder = NULL;
        }

        if (rs_status != RS_SUCCESS)
            return rs_status;
        else
            return RS_UNCONNECTED_SERVER;
    }

    // TODO: p2p_peer_id
    p2p_peer_id = 0;

    g_on_capture_frame = on_capture_frame;
    g_on_exception = on_exception;

    return RS_SUCCESS;
}

DllDecl RS_STATUS Stop()
{
    AUTO_LOG();
    AutoLocker auto_locker(g_sync_api);

    if (g_h264_stream_decoder == NULL) {
        LOGE("The decoder is invalid !!!");
        return RS_CODEC_ABORTED;
    }

    g_h264_stream_decoder->Stop();
    delete g_h264_stream_decoder;
    g_h264_stream_decoder = NULL;
    return RS_SUCCESS;
}

RS_STATUS GetFrameBuffer(
  unsigned char *frame_buffer_data,
  unsigned int   frame_buffer_data_len
  )
{
    AUTO_LOG();

    if (g_h264_stream_decoder == NULL) {
        LOGE("The decoder is invalid !!!");
        return RS_CODEC_ABORTED;
    }
    g_h264_stream_decoder->GetCurrentRgbaSurface(frame_buffer_data, frame_buffer_data_len);
    return RS_SUCCESS;
}

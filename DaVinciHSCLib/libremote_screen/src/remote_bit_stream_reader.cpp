#include "remote_bit_stream_reader.h"

#include "logger.h"
#include "remote_screen.h"

#include <memory.h>
#include <assert.h>
#include <Windows.h>
#include <iostream>

extern _GeneralExceptionCallBack g_on_exception;

CRemoteBitStreamReader::CRemoteBitStreamReader() : m_channel(CHANNEL_DATA),
                                                   m_semaphore(m_mfxStatus, false, false)
{
    AUTO_LOG();
    memset(m_strServerAddress, 0, sizeof(m_strServerAddress));
    m_numServerPort  = 0;
    m_pDeliverThread = NULL;
    m_bStopThread    = false;
    m_mfxStatus      = MFX_ERR_NONE;
    m_bStatMode      = false;
}

CRemoteBitStreamReader::~CRemoteBitStreamReader()
{
    AUTO_LOG();

    if (m_pDeliverThread != NULL) {
        delete m_pDeliverThread;
        m_pDeliverThread = NULL;
    }
}

mfxStatus CRemoteBitStreamReader::Start(const char *strServerAddress, unsigned short numServerPort)
{
    AUTO_LOG();
    assert(strServerAddress != NULL);

    if (0 != m_channel.ConnectTarget(strServerAddress, numServerPort))
        return MFX_ERR_UNKNOWN;

    mfxStatus sts = MFX_ERR_NONE;
    m_bStopThread = false;
    m_pDeliverThread = new MSDKThread(sts, ReceiveThreadFunc, this);
    if (sts != MFX_ERR_NONE) {
        m_channel.Disconnect();
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

unsigned int MSDK_THREAD_CALLCONVENTION CRemoteBitStreamReader::ReceiveThreadFunc(void* ctx)
{
    AUTO_LOG();
    mfxStatus               sts           = MFX_ERR_NONE;
    CRemoteBitStreamReader *currentReader = (CRemoteBitStreamReader *)ctx;

    int  ret = 0;
    char recv_buffer[65000]; // TODO: the magic number should be defined as a variable

    while (!currentReader->m_bStopThread) {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        ret = currentReader->m_channel.ReceiveData(recv_buffer, sizeof(recv_buffer));
        if (ret <= 0) {
            if (currentReader->m_channel.CurrentConnectionStatus() == NETWORK_CONNECTION_ERROR)
                sts = MFX_ERR_UNKNOWN;
            else
                sts = MFX_ERR_NONE;
            break;
        } else {
            ////////////////////////////////////////
            // For debug and statistic information
            CLogger::Instance()->DumpHex(CLogger::LOG_DEBUG, (char *)recv_buffer, ret);
            if (currentReader->m_bStatMode) {
                CStatistics::GetSingleObject().Input(RCV_TS, (mfxU8*)recv_buffer, ret);
            }
            ////////////////////////////////////////

            currentReader->m_crRingBuffer.AddData((unsigned char *)recv_buffer, ret);
            currentReader->m_semaphore.Signal();
        }
    }

    // Trigger exception
    if (!currentReader->m_bStopThread && sts != MFX_ERR_NONE) {
        if (g_on_exception != NULL) {
            g_on_exception(RS_UNCONNECTED);
        }
    }

    // Notify all wait thread
    currentReader->m_semaphore.Signal();
    return sts;
}

void CRemoteBitStreamReader::Close()
{
    AUTO_LOG();

    m_bStopThread = true;

    if (m_channel.CurrentConnectionStatus() == NETWORK_CONNECTION_CONNECTED)
        m_channel.Disconnect();
    m_crRingBuffer.Clear();

    if (m_pDeliverThread != NULL) {
        if (m_pDeliverThread->GetExitCode() == MFX_TASK_WORKING) {
            m_pDeliverThread->Wait();
        }
    }
}

mfxStatus CRemoteBitStreamReader::ReadNextFrame(mfxBitstream *pBS)
{
    if (pBS == NULL)
        return MFX_ERR_NULL_PTR;

    mfxU32 nBytesRead = 0;

    memmove(pBS->Data, pBS->Data + pBS->DataOffset, pBS->DataLength);
    pBS->DataOffset = 0;
    nBytesRead = m_crRingBuffer.FetchData(pBS->Data + pBS->DataLength, pBS->MaxLength - pBS->DataLength);
    if (nBytesRead == 0) {
        if (m_channel.CurrentConnectionStatus() == NETWORK_CONNECTION_ERROR)
            return MFX_ERR_ABORTED;
        else if (m_channel.CurrentConnectionStatus() == NETWORK_CONNECTION_DISCONNECTED)
            return MFX_WRN_VALUE_NOT_CHANGED;
        else
            return MFX_ERR_MORE_DATA;
    } else {
        if (m_bStatMode) {
            CStatistics::GetSingleObject().Input(DEC_TS, pBS, pBS->DataLength, nBytesRead);
        }
    }

    pBS->DataLength += nBytesRead;
    return MFX_ERR_NONE;
}

void CRemoteBitStreamReader::Wait(unsigned int msec)
{
    m_semaphore.TimedWait(msec);
}

void CRemoteBitStreamReader::Wait()
{
    m_semaphore.Wait();
}

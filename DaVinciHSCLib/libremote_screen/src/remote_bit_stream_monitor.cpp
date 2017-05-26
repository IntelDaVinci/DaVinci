#include "remote_bit_stream_monitor.h"

#include "logger.h"
#include "remote_screen.h"

CRemoteBitStreamMonitor::CRemoteBitStreamMonitor() : m_channel(CHANNEL_MONITOR)
{
    AUTO_LOG();

    m_pDeliverThread = NULL;
    m_bStopThread    = true;
    m_numServerPort  = 0;
    m_mfxStatus      = MFX_ERR_NONE;

    m_observers.clear();
    memset(m_strServerAddress, 0, sizeof(m_strServerAddress));
}

CRemoteBitStreamMonitor::~CRemoteBitStreamMonitor()
{
    AUTO_LOG();

    if (m_pDeliverThread != NULL) {
        delete m_pDeliverThread;
        m_pDeliverThread = NULL;
    }
}

mfxStatus CRemoteBitStreamMonitor::MonitorTarget()
{
    mfxStatus sts = MFX_ERR_NONE;
    m_bStopThread = false;
    m_pDeliverThread = new MSDKThread(sts, ReceiveThreadFunc, this);
    if (sts != MFX_ERR_NONE) {
        m_channel.Disconnect();
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

mfxStatus CRemoteBitStreamMonitor::Attach(const char *strServerAddress, unsigned short numServerPort)
{
    AUTO_LOG();
    assert(strServerAddress != NULL);

    if (0 != m_channel.ConnectTarget(strServerAddress, numServerPort))
        return MFX_ERR_UNKNOWN;

    return MFX_ERR_NONE;
}

RS_STATUS CRemoteBitStreamMonitor::CheckTargetStatus()
{
    char recv_buffer[512];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    int ret = m_channel.ReceiveData(recv_buffer, sizeof(recv_buffer));
    if (ret <= 0)
        return RS_UNCONNECTED;
    recv_buffer[sizeof(recv_buffer) - 1] = '\0';

    std::string str_rcv_buffer(recv_buffer);
    if (str_rcv_buffer.compare(std::string("OK")) == 0)
        return RS_SUCCESS;

    if (str_rcv_buffer.compare("ERR_CONF") == 0)
        return RS_DEVICE_CONFIG_CODEC_ERR;

    if (str_rcv_buffer.compare("ERR_NO_CODC") == 0)
        return RS_DEVICE_NO_AVAIL_CODEC;

    if (str_rcv_buffer.compare("ERR_NO_FORM") == 0)
        return RS_DEVICE_NO_AVAIL_FORMAT;

    if (str_rcv_buffer.compare("ERR_CODC") == 0)
        return RS_DEVICE_CODEC_ERR;
}

void CRemoteBitStreamMonitor::RegisterObserver(CMonitorObserver *monitor_observer)
{
    AUTO_LOG();

    if (monitor_observer != NULL)
        m_observers.push_back(monitor_observer);
}

unsigned int MSDK_THREAD_CALLCONVENTION CRemoteBitStreamMonitor::ReceiveThreadFunc(void* ctx)
{
    AUTO_LOG();
    mfxStatus                sts            = MFX_ERR_NONE;
    CRemoteBitStreamMonitor *currentMonitor = (CRemoteBitStreamMonitor *)ctx;

    int  ret = 0;
    char recv_buffer[512]; // TODO: the magic number should be defined as a variable

    while (!currentMonitor->m_bStopThread) {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        ret = currentMonitor->m_channel.ReceiveData(recv_buffer, sizeof(recv_buffer));
        if (ret <= 0) {
            if (!currentMonitor->m_bStopThread) { // Close monitor by self
                std::vector<CMonitorObserver *> observers = currentMonitor->m_observers;
                for (int index = 0; index < observers.size(); index++) {
                    observers[index]->ObserveStatus(RS_UNCONNECTED, "Cannot monitor remote target device");
                }
            }
            break;
        } else {
            ////////////////////////////////////////
            // For debug
            CLogger::Instance()->DumpHex(CLogger::LOG_DEBUG, (char *)recv_buffer, ret);
            ////////////////////////////////////////

            if (std::string(recv_buffer).find("[ERROR]") == 0) {
                LOGE(recv_buffer);
                std::vector<CMonitorObserver *> observers = currentMonitor->m_observers;
                for (int index = 0; index < observers.size(); index++) {
                    observers[index]->ObserveStatus(RS_DEVICE_CODEC_ERR, recv_buffer);
                }
                break;
            }
        }
    }

    return sts;
}

void CRemoteBitStreamMonitor::Detach()
{
    AUTO_LOG();

    m_bStopThread = true;

    if (m_channel.CurrentConnectionStatus() == NETWORK_CONNECTION_CONNECTED)
        m_channel.Disconnect();

    if (m_pDeliverThread != NULL) {
        if (m_pDeliverThread->GetExitCode() == MFX_TASK_WORKING) {
            m_pDeliverThread->Wait();
        }
    }
}

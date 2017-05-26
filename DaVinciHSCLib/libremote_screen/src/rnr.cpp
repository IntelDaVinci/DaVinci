#include "rnr.h"

Monitor::Monitor()
{
    m_socketRnrSocket = INVALID_SOCKET;
    m_pRnrThread = NULL;
}

Monitor::~Monitor()
{
    if (m_socketRnrSocket != INVALID_SOCKET) {
        closesocket(m_socketRnrSocket);
    }
    if (m_pRnrThread)
    {
        delete m_pRnrThread;
        m_pRnrThread = NULL;
    }

}

bool Monitor::Initialize(const char *ip, int port)
{
    WSADATA  Ws;
    struct sockaddr_in Address;
    mfxStatus sts = MFX_ERR_NONE;
    int flag = 1;
    unsigned long ul = 1;
    fd_set fds;
    timeval tm = { 1, 0 };
    int error = -1;
    int len = sizeof(int);

    AUTO_LOG();

    if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
        return false;

    m_socketRnrSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socketRnrSocket == INVALID_SOCKET) {
        LOGE("Cannot create socket!!!");
        WSACleanup();
        return false;
    }

    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = inet_addr(ip);
    Address.sin_port = htons((unsigned short)port);
    memset(Address.sin_zero, 0x00, 8);
    ioctlsocket(m_socketRnrSocket, FIONBIO, &ul);
    if (connect(m_socketRnrSocket, (struct sockaddr*)&Address, sizeof(Address)) == -1)
    {
        FD_ZERO(&fds);
        FD_SET(m_socketRnrSocket, &fds);
        if (select(m_socketRnrSocket + 1, NULL, &fds, NULL, &tm) > 0)
        {
            int ret = getsockopt(m_socketRnrSocket, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
            if (error == 0)
            {
                if (setsockopt(m_socketRnrSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == 0)
                {
                    ul = 0;
                    ioctlsocket(m_socketRnrSocket, FIONBIO, &ul);
                    _tprintf(_T("**************** 1 new thread for rnr ******************\n"));
                    m_pRnrThread = new MSDKThread(sts, RnrThread, (void*)this);
                    return true;
                }
            }
        }
        LOGE("Cannot connect to remote target(ip:%s, port:%d) for statistic !!!", ip, port);
        closesocket(m_socketRnrSocket);
        m_socketRnrSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    else
    {
        ul = 0;
        ioctlsocket(m_socketRnrSocket, FIONBIO, &ul);
        _tprintf(_T("**************** 2 new thread for rnr ******************\n"));
        m_pRnrThread = new MSDKThread(sts, RnrThread, (void*)this);
        return true;
    }
}

unsigned int MSDK_THREAD_CALLCONVENTION Monitor::RnrThread(void* param)
{
    mfxStatus sts = MFX_ERR_NONE;
    Monitor* Context = reinterpret_cast<Monitor*>(param);
    Event event;
    for (;;)
    {
        mfxI32 n = (mfxU32)recv(Context->m_socketRnrSocket, (char*)&event, sizeof(Event), 0);
        if (n > 0)
        {
            /* event handing */
            _tprintf(_T("**************** [%ld:%ld] event(%u, %u, %d) ******************\n"), event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
        }
    }
    return sts;
}
#include "statistics.h"

CStatistics::CStatistics()
{
    m_socketStatisticsSocket = INVALID_SOCKET;
    m_rcv_count = m_dec_count = 0;
    m_pUtcThread = NULL;
}

CStatistics::~CStatistics()
{
    if (m_socketStatisticsSocket != INVALID_SOCKET) {
        closesocket(m_socketStatisticsSocket);
    }
    if (m_pUtcThread)
    {
        delete m_pUtcThread;
        m_pUtcThread = NULL;
    }

}

bool CStatistics::Initialize(const char *ip, unsigned short port)
{
//    WSADATA  Ws;
    struct sockaddr_in Address;
    mfxStatus sts = MFX_ERR_NONE;
    int flag = 1;
    unsigned long ul = 1;
    fd_set fds;
    timeval tm = { 1, 0 };
    int error = -1;
    int len = sizeof(int);

    AUTO_LOG();

    // should be started up elsewhere.
    // if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
    //    return false;
    /* connect ot camera_less for tcp package deliver delay estimation and send back frame info */
    m_socketStatisticsSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socketStatisticsSocket == INVALID_SOCKET) {
        LOGE("Cannot create socket!!!");
        WSACleanup();
        return false;
    }

    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = inet_addr(ip);
    Address.sin_port = htons(port);
    memset(Address.sin_zero, 0x00, 8);
    ioctlsocket(m_socketStatisticsSocket, FIONBIO, &ul);
    if (connect(m_socketStatisticsSocket, (struct sockaddr*)&Address, sizeof(Address)) == -1)
    {
        FD_ZERO(&fds);
        FD_SET(m_socketStatisticsSocket, &fds);
        if (select(m_socketStatisticsSocket + 1, NULL, &fds, NULL, &tm) > 0)
        {
            getsockopt(m_socketStatisticsSocket, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
            if (error == 0)
            {
                if (setsockopt(m_socketStatisticsSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == 0)
                {
                    ul = 0;
                    ioctlsocket(m_socketStatisticsSocket, FIONBIO, &ul);
                    m_pUtcThread = new MSDKThread(sts, UtcThread, (void*)this);
                    return true;
                }
            }
        }
        LOGE("Cannot connect to remote target(ip:%s, port:%d) for statistic !!!", ip, port);
        closesocket(m_socketStatisticsSocket);
        m_socketStatisticsSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    else
    {
        ul = 0;
        ioctlsocket(m_socketStatisticsSocket, FIONBIO, &ul);
        m_pUtcThread = new MSDKThread(sts, UtcThread, (void*)this);
        return true;
    }
}

CVideoParser* CStatistics::Parser(TimeStamp ts)
{
    std::map<CVideoParser*, struct Stat>::const_iterator it = parsers.begin();
    for (; it != parsers.end(); ++it) {
        if (it->first->Phase() == ts) break;
    }
    if (it != parsers.end()) {
        return it->first;
    } else {
        LOGE("[%s:%s@%d] => new parser for %d\n", __FILE__, __FUNCTION__, __LINE__, ts);
        CVideoParser* parser = new CH264VideoParser(this, ts);
        parsers.insert(std::make_pair(parser, Stat()));
        return parser;
    }
}

void CStatistics::Stamp(TimeStamp ts, mfxU64 index)
{
    Message message = { index, ts };
    LOGE("[%s:%s@%d] => timestamp for frame[%llu][%d]\n", __FILE__, __FUNCTION__, __LINE__, index, ts);
    if (m_socketStatisticsSocket != INVALID_SOCKET)
    {
        send(m_socketStatisticsSocket, (char*)&message, sizeof(Message), 0);
    }
}

void CStatistics::OnNewFrame(CVideoParser* parser, mfxU64 frame_num)
{
    AUTO_LOG();
    LOGE("[%s:%s@%d] => new frame parsed out on [%d]\n", __FILE__, __FUNCTION__, __LINE__, parser->Phase());
    std::map<CVideoParser*, struct Stat>::const_iterator it = parsers.find(parser);
    if (it != parsers.end()) {
        Stamp((TimeStamp)parser->Phase(), parsers[parser].m_frame_count++);
    }
}

void CStatistics::OnFrameSize(CVideoParser* parser, mfxU32 width, mfxU32 height)
{
    AUTO_LOG();
}

unsigned int MSDK_THREAD_CALLCONVENTION CStatistics::UtcThread(void* param)
{
    mfxStatus sts = MFX_ERR_NONE;
    CStatistics* Context = reinterpret_cast<CStatistics*>(param);
    Message message;
    for (;;)
    {
        mfxI32 n = (mfxU32)recv(Context->m_socketStatisticsSocket, (char*)&message, sizeof(Message), 0);
        if (n > 0 && message.ts == TS_CNT)
        {
            n = send(Context->m_socketStatisticsSocket, (char*)&message, sizeof(Message), 0);
        }
        if (n <= 0)
        {
            break;
        }
    }
    return sts;
}
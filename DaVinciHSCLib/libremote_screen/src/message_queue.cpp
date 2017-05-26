#include "message_queue.h"

#include "logger.h"

#include <memory.h>
#include <fstream>

CMessageQueue::CMessageQueue()
{
    AUTO_LOG();
    m_lstStreamBuffer.clear();
}

CMessageQueue::~CMessageQueue()
{
    AUTO_LOG();

    Clear();
}

int CMessageQueue::AddData(unsigned char *data_ptr, int data_len)
{
    if (data_ptr == 0)
        return 0;

    m_Mutex.Lock();
    m_lstStreamBuffer.push_back(new StreamBuffer(data_ptr, data_len));
    m_Mutex.Unlock();

    return data_len;
}

int CMessageQueue::FetchData(unsigned char *data_ptr, int data_len)
{
    if (data_ptr == 0)
        return 0;

    m_Mutex.Lock();
    if (m_lstStreamBuffer.size() <= 0) {
        m_Mutex.Unlock();
        return 0;
    }

    static int consumed = 0;
    StreamBuffer *tmp = *(m_lstStreamBuffer.begin());
    int fetch_count = 0;
    if (tmp->m_numDataLen - consumed <= data_len) {
        fetch_count = tmp->m_numDataLen - consumed;
        memcpy(data_ptr, tmp->m_strData + consumed, fetch_count);
        consumed = 0;
        m_lstStreamBuffer.pop_front();
        delete tmp;

        m_Mutex.Unlock();
    } else {
        memcpy(data_ptr, tmp->m_strData + consumed,  data_len);
        consumed += data_len;

        m_Mutex.Unlock();
        fetch_count = data_len;
    }

#if defined(DUMP_STREAM)
    if (fetch_count > 0) {
        std::ofstream of("fetch.h264", std::ios::binary | std::ios::app);
        of.write((char *)data_ptr, fetch_count);
        of.close();
    }
#endif

    CLogger::Instance()->DumpHex(CLogger::LOG_DEBUG, (char *)data_ptr, fetch_count);

    return fetch_count;
}

void CMessageQueue::Clear()
{
    AUTO_LOG();
    m_Mutex.Lock();
    if (!m_lstStreamBuffer.empty()) {
        int buffer_count = m_lstStreamBuffer.size();
        for (int i = 0; i < buffer_count; i++) {
            StreamBuffer *tmp = *(m_lstStreamBuffer.begin());
            m_lstStreamBuffer.pop_front();
            delete tmp;
        }
    }
    m_lstStreamBuffer.clear();
    m_Mutex.Unlock();
}

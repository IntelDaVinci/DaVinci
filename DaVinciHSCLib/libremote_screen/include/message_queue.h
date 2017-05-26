#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "vm/thread_defs.h"

#include <assert.h>
#include <list>

struct StreamBuffer {
    unsigned char *m_strData;
    int            m_numDataLen;
    StreamBuffer(unsigned char *data_ptr, int data_len) {
        m_strData = new unsigned char[data_len];
        assert(m_strData != NULL);
        memcpy(m_strData, data_ptr, data_len);
        m_numDataLen = data_len;
    };

    ~StreamBuffer() {
        if (m_strData != NULL) {
            delete[] m_strData;
            m_strData = NULL;
        }
    }

private:
    StreamBuffer(const StreamBuffer& obj);
    StreamBuffer& operator=(StreamBuffer& obj);
};

class CMessageQueue {
public:
    CMessageQueue();
    ~CMessageQueue();

    int AddData(unsigned char *data_ptr, int data_len);
    int FetchData(unsigned char *data_ptr, int data_len);
    void Clear();

private:
    MSDKMutex                 m_Mutex;
    std::list<StreamBuffer *> m_lstStreamBuffer;
};

#endif

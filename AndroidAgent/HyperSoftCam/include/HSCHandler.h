#ifndef _HSC_HANDLER_H_
#define _HSC_HANDLER_H_

#include <utils/RefBase.h>

namespace android {

class HSCHandler : public RefBase {
public:
    HSCHandler()
    {
        m_next_handler = NULL;
    }

    virtual ~HSCHandler()
    {
        m_next_handler = NULL;
    }

    void SetNextHandler(sp<HSCHandler> next_handler)
    {
        m_next_handler = next_handler;
    }

    virtual bool Handle()
    {
        return false;
    };

protected:
    sp<HSCHandler> m_next_handler;
};

}

#endif

#ifndef _STREAM_HANDLER_H_
#define _STREAM_HANDLER_H_

#include "HSCHandler.h"

namespace android {

class StreamHandler : public HSCHandler {
public:
    StreamHandler()  {};
    ~StreamHandler() {};

    virtual bool Handle();
};

}

#endif

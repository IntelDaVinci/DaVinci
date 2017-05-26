#ifndef _REPLAY_HANDLER_H_
#define _REPLAY_HANDLER_H_

#include "HSCHandler.h"

namespace android {

class ReplayHandler : public HSCHandler {
public:
    ReplayHandler()  {};
    ~ReplayHandler() {};

    virtual bool Handle();
};

}

#endif

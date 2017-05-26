#ifndef _RECORD_HANDLER_H_
#define _RECORD_HANDLER_H_

#include "HSCHandler.h"

namespace android {

class RecordHandler : public HSCHandler {
public:
    RecordHandler()  {};
    ~RecordHandler() {};

    virtual bool Handle();
};

}

#endif

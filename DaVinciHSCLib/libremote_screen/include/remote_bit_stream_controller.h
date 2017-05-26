#ifndef __REMOTE_BIT_STREAM_CONTROLLER_H__
#define __REMOTE_BIT_STREAM_CONTROLLER_H__

#include "mfxcommon.h"
#include "channel.h"

class CRemoteBitStreamController {
public :
    CRemoteBitStreamController();
    virtual ~CRemoteBitStreamController();

    mfxStatus Start(const char *strServerAddress, unsigned short numServerPort);
    void      Close();

    mfxStatus StartCapture();
    mfxStatus StopCapture();

private:
    CRemoteBitStreamController(const CRemoteBitStreamController& obj);
    CRemoteBitStreamController& operator=(CRemoteBitStreamController& obj);

protected:
    char                     m_strServerAddress[512];
    int                      m_numServerPort;
    CChannel                 m_channel;
    mfxStatus                m_mfxStatus;
};

#endif

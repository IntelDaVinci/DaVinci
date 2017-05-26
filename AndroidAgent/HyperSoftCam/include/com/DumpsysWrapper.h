#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Vector.h>
#include "utils/Logger.h"
#include "utils/ConvUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace android {

class DumpsysWrapper : virtual public RefBase {
public:
    DumpsysWrapper();
    virtual ~DumpsysWrapper();

    void Initialize();
    bool DumpViewHierarchy();
    bool DumpWindowBar();

	char* GetRegion();

	bool InitializeRegion();
	void ReleaseRegion();
private:
	sp<IServiceManager> sm;
    sp<IBinder> m_service;
	int m_fd;
	char* m_region;
	static const int fileSize = 8 * 1024 * 1024; // 1M
};

};


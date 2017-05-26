#include "com/DumpsysWrapper.h"

namespace android {

    DumpsysWrapper::DumpsysWrapper(): m_service(0), m_fd(-1), m_region(0)
    {
        AUTO_LOG();
        Initialize();
    }

    DumpsysWrapper::~DumpsysWrapper()
    {
        AUTO_LOG();
    }

    void DumpsysWrapper::Initialize()
    {
        sm = defaultServiceManager();
        if (sm == NULL) 
        {
            ALOGE("Unable to get default service manager!");
            return;
        }
    }

    bool DumpsysWrapper::DumpViewHierarchy()
    {
        AUTO_LOG();
        m_service = sm->checkService(String16("activity"));
        if (m_service == NULL) 
        {
            ALOGE("Unable to get activity service!");
            return false;
        }

        Vector<String16> args;
        args.add(String16("top"));

        int err = m_service->dump(m_fd, args);

        if (err != 0) 
        {
            ALOGE("Unable to dump activity info!");
            return false;
        }

        return true;
    }

    bool DumpsysWrapper::DumpWindowBar()
    {
        AUTO_LOG();
        m_service = sm->checkService(String16("window"));
        if (m_service == NULL) 
        {
            ALOGE("Unable to get window service!");
            return false;
        }

        Vector<String16> args;
	args.push_back(String16("windows"));

        int err = m_service->dump(m_fd, args);

        if (err != 0) 
        {
            ALOGE("Unable to dump window info!");
            return false;
        }

	return true;
    }

    bool DumpsysWrapper::InitializeRegion()
    {
        m_fd = open("/data/local/tmp/davinci_region", O_CREAT | O_TRUNC | O_RDWR, 0777);
        if( m_fd == -1)
        {
	    DWLOGE("Unable to open file for davinci region!");
	    return false;
	}

	m_region = (char*)mmap(NULL, fileSize,
	        PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
   	if( m_region == MAP_FAILED)
	{
	    DWLOGE("Unable to mmap the memory!");
	    return false;
	}

	return true;
    }

    char* DumpsysWrapper::GetRegion()
    {
	return m_region;
    }

    void DumpsysWrapper::ReleaseRegion()
    {
        if (munmap(m_region, fileSize) == -1)
        {
            DWLOGE("Unable to munmap the region!");
        }

        if(m_fd != -1)
            close(m_fd);

        m_region = NULL;
    }

}

/* ****************************************************************************** *\

 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2012-2014 Intel Corporation. All Rights Reserved.

\* ****************************************************************************** */

#ifndef __THREAD_DEFS_H__
#define __THREAD_DEFS_H__

#include "mfxdefs.h"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

class MSDKMutex
{
public:
    MSDKMutex(void);
    ~MSDKMutex(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);
    int Try(void);

private:
    bool m_bInitialized;
#if defined(_WIN32) || defined(_WIN64)
    CRITICAL_SECTION m_CritSec;
#else
    pthread_mutex_t m_mutex;
#endif
};

class AutoLocker {
public:
    AutoLocker(MSDKMutex& mutex)
    {
        m_pMutex = &mutex;
        m_pMutex->Lock();
    }
    ~AutoLocker(void)
    {
        m_pMutex->Unlock();
    }

private:
    MSDKMutex* m_pMutex;
};

class AutomaticMutex
{
public:
    AutomaticMutex(MSDKMutex& mutex);
    ~AutomaticMutex(void);

private:
    void Lock(void);
    void Unlock(void);

    MSDKMutex* m_pMutex;
    bool m_bLocked;
};

class MSDKSemaphore
{
public:
    MSDKSemaphore(mfxStatus &sts, mfxU32 count = 0);
    ~MSDKSemaphore(void);

    void Post(void);
    void Wait(void);
private:
#if defined(_WIN32) || defined(_WIN64)
    void* m_semaphore;
#else
    mfxU32 m_count;
    pthread_cond_t m_semaphore;
    pthread_mutex_t m_mutex;
#endif

};

class MSDKEvent
{
public:
    MSDKEvent(mfxStatus &sts, bool manual, bool state);
    ~MSDKEvent(void);

    void Signal(void);
    void Reset(void);
    void Wait(void);
    mfxStatus TimedWait(mfxU32 msec);

private:
#if defined(_WIN32) || defined(_WIN64)
    void* m_event;
#else
    bool m_manual;
    bool m_state;
    pthread_cond_t m_event;
    pthread_mutex_t m_mutex;
#endif
};

#if defined(_WIN32) || defined(_WIN64)
#define MSDK_THREAD_CALLCONVENTION __stdcall
#else
#define MSDK_THREAD_CALLCONVENTION
#endif

typedef unsigned int (MSDK_THREAD_CALLCONVENTION * msdk_thread_callback)(void*);

typedef enum {
    PRIORITY_IDLE,
    PRIORITY_LOWEST,
    PRIORITY_BELOW_NORMAL,
    PRIORITY_NORMAL,
    PRIORITY_ABOVE_NORMAL,
    PRIORITY_HIGHEST,
    PRIORITY_TIME_CRITICAL
} ThreadPriority;

class MSDKThread
{
public:
    MSDKThread(mfxStatus &sts, msdk_thread_callback func, void* arg);
    ~MSDKThread(void);

    static long GetPID();

    void Wait(void);
    mfxStatus TimedWait(mfxU32 msec);
    mfxStatus GetExitCode();
    mfxStatus SetPriority(ThreadPriority priority);
    ThreadPriority GetPriority();

protected:
#if defined(_WIN32) || defined(_WIN64)
    void* m_thread;
#else
    friend void* msdk_thread_start(void* arg);

    pthread_t m_thread;
    msdk_thread_callback m_func;
    void* m_arg;
    MSDKEvent* m_event;
#endif
};

mfxStatus msdk_setrlimit_vmem(mfxU64 size);

#endif //__THREAD_DEFS_H__
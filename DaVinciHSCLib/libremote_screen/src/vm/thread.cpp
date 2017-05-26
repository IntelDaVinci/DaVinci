/*********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2014 Intel Corporation. All Rights Reserved.

**********************************************************************************/

#include "mfx_samples_config.h"

#if defined(_WIN32) || defined(_WIN64)

#include "vm/thread_defs.h"

MSDKMutex::MSDKMutex(void)
{
    m_bInitialized = true;
    InitializeCriticalSection(&m_CritSec);
}

MSDKMutex::~MSDKMutex(void)
{
    if (m_bInitialized)
    {
        DeleteCriticalSection(&m_CritSec);
    }
}

mfxStatus MSDKMutex::Lock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_bInitialized)
    {
        EnterCriticalSection(&m_CritSec);
    }
    return sts;
}

mfxStatus MSDKMutex::Unlock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_bInitialized)
    {
        LeaveCriticalSection(&m_CritSec);
    }
    return sts;
}

int MSDKMutex::Try(void)
{
    int res = 0;
    if (m_bInitialized)
    {
        res = TryEnterCriticalSection(&m_CritSec);
    }
    return res;
}

/* ****************************************************************************** */

AutomaticMutex::AutomaticMutex(MSDKMutex& mutex)
{
    m_pMutex = &mutex;
    m_bLocked = false;
    Lock();
};
AutomaticMutex::~AutomaticMutex(void)
{
    Unlock();
}

void AutomaticMutex::Lock(void)
{
    if (!m_bLocked)
    {
        if (!m_pMutex->Try())
        {
            m_pMutex->Lock();
        }
        m_bLocked = true;
    }
}

void AutomaticMutex::Unlock(void)
{
    if (m_bLocked)
    {
        m_pMutex->Unlock();
        m_bLocked = false;
    }
}
/* ****************************************************************************** */

MSDKSemaphore::MSDKSemaphore(mfxStatus &sts, mfxU32 count)
{
    sts = MFX_ERR_NONE;
    m_semaphore = CreateSemaphore(NULL, count, LONG_MAX, 0);
    if (!m_semaphore) sts = MFX_ERR_UNKNOWN;
}

MSDKSemaphore::~MSDKSemaphore(void)
{
    if (m_semaphore) CloseHandle(m_semaphore);
}

void MSDKSemaphore::Post(void)
{
    if (m_semaphore) ReleaseSemaphore(m_semaphore, 1, NULL);
}

void MSDKSemaphore::Wait(void)
{
    if (m_semaphore) WaitForSingleObject(m_semaphore, INFINITE);
}

/* ****************************************************************************** */

MSDKEvent::MSDKEvent(mfxStatus &sts, bool manual, bool state)
{
    sts = MFX_ERR_NONE;
    m_event = CreateEvent(NULL, manual, state, NULL);
    if (!m_event) sts = MFX_ERR_UNKNOWN;
}

MSDKEvent::~MSDKEvent(void)
{
    if (m_event) CloseHandle(m_event);
}

void MSDKEvent::Signal(void)
{
    if (m_event) SetEvent(m_event);
}

void MSDKEvent::Reset(void)
{
    if (m_event) ResetEvent(m_event);
}

void MSDKEvent::Wait(void)
{
    if (m_event) WaitForSingleObject(m_event, MFX_INFINITE);
}

mfxStatus MSDKEvent::TimedWait(mfxU32 msec)
{
    if(MFX_INFINITE == msec) return MFX_ERR_UNSUPPORTED;
    mfxStatus mfx_res = MFX_ERR_NOT_INITIALIZED;
    if (m_event)
    {
        DWORD res = WaitForSingleObject(m_event, msec);
        if(WAIT_OBJECT_0 == res) mfx_res = MFX_ERR_NONE;
        else if (WAIT_TIMEOUT == res) mfx_res = MFX_TASK_WORKING;
        else mfx_res = MFX_ERR_UNKNOWN;
    }
    return mfx_res;
}

long MSDKThread::GetPID()
{
    long pid = -1;
    pid = GetCurrentThreadId();
    return pid;
}

MSDKThread::MSDKThread(mfxStatus &sts, msdk_thread_callback func, void* arg)
{
    sts = MFX_ERR_UNKNOWN;
    m_thread = (void*)_beginthreadex(NULL, 0, func, arg, 0, NULL);
    if (m_thread) sts = MFX_ERR_NONE;
}

MSDKThread::~MSDKThread(void)
{
    if (m_thread) CloseHandle(m_thread);
}

void MSDKThread::Wait(void)
{
    if (m_thread) WaitForSingleObject(m_thread, MFX_INFINITE);
}

mfxStatus MSDKThread::TimedWait(mfxU32 msec)
{
    if(MFX_INFINITE == msec) return MFX_ERR_UNSUPPORTED;
    mfxStatus mfx_res = MFX_ERR_NONE;
    if (m_thread)
    {
        DWORD res = WaitForSingleObject(m_thread, msec);
        if(WAIT_OBJECT_0 == res) mfx_res = MFX_ERR_NONE;
        else if (WAIT_TIMEOUT == res) mfx_res = MFX_TASK_WORKING;
        else mfx_res = MFX_ERR_UNKNOWN;
    }
    return mfx_res;
}

mfxStatus MSDKThread::GetExitCode()
{
    mfxStatus mfx_res = MFX_ERR_NOT_INITIALIZED;
    if (m_thread)
    {
        DWORD code = 0;
        int sts = 0;
        sts = GetExitCodeThread(m_thread, &code);
        if(sts == 0) mfx_res = MFX_ERR_UNKNOWN;
        else if(STILL_ACTIVE == code) mfx_res = MFX_TASK_WORKING;
        else mfx_res = MFX_ERR_NONE;
    }
    return mfx_res;
}

mfxStatus MSDKThread::SetPriority(ThreadPriority priority)
{
    mfxStatus mfx_res = MFX_ERR_NOT_INITIALIZED;
    BOOL      is_ok   = FALSE;
    if (m_thread)
    {
        switch (priority) {
        case PRIORITY_IDLE:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_IDLE);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_LOWEST:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_LOWEST);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_BELOW_NORMAL:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_BELOW_NORMAL);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_NORMAL:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_NORMAL);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_ABOVE_NORMAL:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_ABOVE_NORMAL);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_HIGHEST:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_HIGHEST);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        case PRIORITY_TIME_CRITICAL:
            is_ok = SetThreadPriority(m_thread, THREAD_PRIORITY_TIME_CRITICAL);
            if (is_ok == TRUE)
                mfx_res = MFX_ERR_NONE;
            break;
        default:
            break;
        }
    }

    return mfx_res;
}

ThreadPriority MSDKThread::GetPriority()
{
    int priority = PRIORITY_NORMAL;
    if (m_thread)
    {
        priority = GetThreadPriority(m_thread);
        switch (priority) {
        case THREAD_PRIORITY_TIME_CRITICAL:
            priority = PRIORITY_TIME_CRITICAL;
            break;
        case THREAD_PRIORITY_HIGHEST:
            priority = PRIORITY_HIGHEST;
            break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
            priority = PRIORITY_ABOVE_NORMAL;
            break;
        case THREAD_PRIORITY_NORMAL:
            priority = PRIORITY_NORMAL;
            break;
        case THREAD_PRIORITY_BELOW_NORMAL:
            priority = PRIORITY_BELOW_NORMAL;
            break;
        case THREAD_PRIORITY_LOWEST:
            priority = PRIORITY_LOWEST;
            break;
        case THREAD_PRIORITY_IDLE:
            priority = PRIORITY_IDLE;
            break;
        default:
            break;
        }
    }

    return (ThreadPriority)priority;
}

#endif // #if defined(_WIN32) || defined(_WIN64)

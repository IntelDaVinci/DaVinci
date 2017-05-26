/*
 ----------------------------------------------------------------------------------------

 "Copyright 2014-2015 Intel Corporation.

 The source code, information and material ("Material") contained herein is owned by Intel Corporation
 or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
 or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
 The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
 be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
 in any way without Intel's prior express written permission. No license under any patent, copyright or 
 other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
 by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
 must be express and approved by Intel in writing.

 Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
 embedded in Materials by Intel or Intel's suppliers or licensors in any way."
 -----------------------------------------------------------------------------------------
*/

#if !(defined WIN32)
#include "boost/asio/high_resolution_timer.hpp"
#endif

#include "HighResolutionTimer.hpp"

using namespace boost::asio;

namespace DaVinci
{
    HighResolutionTimer::HighResolutionTimer() : periodic(true), period(1), started(false)
#if (defined WIN32 || defined _WIN32)
        , pTimer(NULL), timerResolution(1)
#endif
    {
    }

    HighResolutionTimer::~HighResolutionTimer()
    {
        Stop();
    }

#if (defined WIN32)
    VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
    {
        if (lpParam != NULL)
        {
            auto ev = (AutoResetEvent *)lpParam;
            ev->Set();
        }
    }
#endif

    DaVinciStatus HighResolutionTimer::Start()
    {
        boost::lock_guard<boost::mutex> lock(instanceMutex);
        if (started)
        {
            return errc::device_or_resource_busy;
        }
        started = true;
#if (defined WIN32)
        TIMECAPS tc;
        if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
        {
            started = false;
            return errc::operation_not_supported;
        }
        timerResolution = 1;
        if (tc.wPeriodMin > timerResolution)
        {
            timerResolution = tc.wPeriodMin;
        }
        timeBeginPeriod(timerResolution);
        timerThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&HighResolutionTimer::TimerEntry, this)));
        SetThreadName(timerThread->get_id(), "Timer");
        if (periodic)
        {
            CreateTimerQueueTimer(&pTimer, NULL, (WAITORTIMERCALLBACK)TimerRoutine, &timerFiredEvent, period, period, WT_EXECUTEINTIMERTHREAD);
        }
        else
        {
            CreateTimerQueueTimer(&pTimer, NULL, (WAITORTIMERCALLBACK)TimerRoutine, &timerFiredEvent, period, 0, WT_EXECUTEINTIMERTHREAD | WT_EXECUTEONLYONCE);
        }
#else
        timer = boost::shared_ptr<boost_hrt>(new boost_hrt(io));
        timer->expires_from_now(boost::chrono::milliseconds(period));
        timer->async_wait(boost::bind(&HighResolutionTimer::TimerEntry, this));
        timerThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&io_service::run, &io)));
#endif
        return DaVinciStatusSuccess;
    }

    DaVinciStatus HighResolutionTimer::Stop()
    {
        boost::lock_guard<boost::mutex> lock(instanceMutex);
        started = false;
#if (defined WIN32)
        if (pTimer != NULL)
        {
            DeleteTimerQueueTimer(NULL, pTimer, NULL);
            pTimer = NULL;
        }
        timeEndPeriod(timerResolution);
        timerFiredEvent.Set(); // allow the timerThread to exit
#endif
        if (timerThread != nullptr)
        {
            if (timerThread->get_id() == boost::this_thread::get_id())
            {
                timerThread->detach();
                timerThread = nullptr;
            }
            else
            {
                timerThread->join();
                timerThread = nullptr;
            }
        }
#if !(defined WIN32)
        timer = nullptr;
#endif
        return DaVinciStatusSuccess;
    }

    bool HighResolutionTimer::IsStart()
    {
        return started;
    }

    void HighResolutionTimer::SetPeriodic(bool p)
    {
        periodic = p;
    }

    void HighResolutionTimer::SetPeriod(unsigned int ms)
    {
        period = ms;
    }

    void HighResolutionTimer::SetTimerHandler(TimerCallbackFunc cb)
    {
        callback = cb;
    }

#if !(defined WIN32)
    void HighResolutionTimer::TimerEntry()
    {
        timer->expires_from_now(boost::chrono::milliseconds(period));
        TimerCallbackFunc cb = callback;
        if (!cb.empty())
        {
            cb(*this);
        }
        {
            boost::lock_guard<boost::mutex> lock(instanceMutex);
            if (!started)
            {
                return;
            }
            if (periodic)
            {
                timer->async_wait(boost::bind(&HighResolutionTimer::TimerEntry, this));
            }
        }
    }
#else
    void HighResolutionTimer::TimerEntry()
    {
        // Hold the reference so that the timer object
        // is not deleted when the timer thread itself
        // stops the timer and setting the timer thread
        // object to nullptr (see HighResolutionTimer::Stop),
        // after which the timer object might be dereferenced
        // without the following statement.
        auto self = shared_from_this();
        while (true)
        {
            timerFiredEvent.WaitOne();
            if (!IsStart())
            {
                break;
            }
            TimerCallbackFunc cb = callback;
            if (!cb.empty())
            {
                cb(*this);
            }
            if (!IsStart())
            {
                break;
            }
        }
    }
#endif

    void HighResolutionTimer::Join(int ms)
    {
        try
        {
            auto theThread = timerThread;
            if (theThread != nullptr)
            {
                if (ms >= 0)
                {
                    theThread->try_join_for(boost::chrono::milliseconds(ms));
                }
                else
                {
                    theThread->join();
                }
            }
        }
        catch (...)
        {
        }
    }
}
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

#include "boost/thread/lock_guard.hpp"

#include "StopWatch.hpp"

namespace bc = boost::chrono;

namespace DaVinci
{
    StopWatch::StopWatch() : stopped(true)
    {
    }

    StopWatch::~StopWatch()
    {
    }

    void StopWatch::Start()
    {
        SpinLockGuard lock(clockLock);
        if (stopped)
        {
            startTime = clock.now();
            stopped = false;
        }
    }

    void StopWatch::Stop()
    {
        SpinLockGuard lock(clockLock);
        stopped = true;
        bc::high_resolution_clock::duration lastElapsed = clock.now() - startTime;
        elapsed += lastElapsed;
    }

    void StopWatch::Restart()
    {
        SpinLockGuard lock(clockLock);
        elapsed = bc::high_resolution_clock::duration::zero();
        startTime = clock.now();
        stopped = false;
    }

    void StopWatch::Reset()
    {
        SpinLockGuard lock(clockLock);
        elapsed = bc::high_resolution_clock::duration::zero();
        stopped = true;
    }

    double StopWatch::ElapsedMilliseconds()
    {
        SpinLockGuard lock(clockLock);
        if (stopped)
        {
            return static_cast<double>(bc::duration_cast<bc::microseconds>(elapsed).count()) / 1000;
        }
        else
        {
            bc::high_resolution_clock::duration lastElapsed = clock.now() - startTime;
            return static_cast<double>(bc::duration_cast<bc::microseconds>(elapsed + lastElapsed).count()) / 1000;
        }
    }

    bool StopWatch::IsStarted()
    {
        SpinLockGuard lock(clockLock);
        return !stopped;
    }
}
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

#ifndef __HIGH_RESOLUTION_TIMER_HPP__
#define __HIGH_RESOLUTION_TIMER_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"

#include "boost/noncopyable.hpp"
#include "boost/function.hpp"
#include "boost/asio.hpp"
#include "boost/asio/high_resolution_timer.hpp"
#include "boost/thread/thread.hpp"
#include "boost/atomic.hpp"

#include "DaVinciStatus.hpp"
#include "StopWatch.hpp"

#if (defined WIN32)
#include <mmsystem.h>
#include "DaVinciCommon.hpp"
#endif

namespace DaVinci
{
    using namespace std;

    /// <summary> A high resolution timer supporting resolution up to 1ms. </summary>
    class HighResolutionTimer : boost::noncopyable, public boost::enable_shared_from_this<HighResolutionTimer>
    {
    public:
        /// <summary> Defines an alias representing the timer callback function. </summary>
        typedef boost::function<void(HighResolutionTimer &)> TimerCallbackFunc;

#if !(defined WIN32)
        /// <summary> Defines an alias representing the boost high resolution timer. </summary>
        typedef boost::asio::basic_waitable_timer<boost::chrono::high_resolution_clock> boost_hrt;
#endif

        /// <summary> Default constructor. 1ms period and periodic. </summary>
        HighResolutionTimer();

        /// <summary> Destructor. </summary>
        ~HighResolutionTimer();

        /// <summary> Starts the timer </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Start();

        /// <summary> Stops the timer </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Stop();

        /// <summary> Query if this timer is started. </summary>
        ///
        /// <returns> true if started, false if not. </returns>
        bool IsStart();

        /// <summary> Sets whether the timer is periodic </summary>
        ///
        /// <param name="p"> true if periodic. </param>
        void SetPeriodic(bool p);

        /// <summary> Sets a period in milliseconds </summary>
        ///
        /// <param name="ms"> The milliseconds. </param>
        void SetPeriod(unsigned int ms);

        /// <summary> Handler, called when the timer event triggered. </summary>
        ///
        /// <param name="cb"> The cb. </param>
        void SetTimerHandler(TimerCallbackFunc cb);

        /// <summary> Wait for timer to stop. </summary>
        ///
        /// <param name="ms"> The milliseconds to wait. </param>
        void Join(int ms = -1);

    private:
        boost::asio::io_service io;
#if (defined WIN32)
        HANDLE pTimer;
        unsigned int timerResolution;
        AutoResetEvent timerFiredEvent;
#else
        boost::shared_ptr<boost_hrt> timer;
#endif
        boost::shared_ptr<boost::thread> timerThread;
        bool periodic;
        unsigned int period;
        TimerCallbackFunc callback;
        boost::atomic<bool> started;
        boost::mutex instanceMutex;

        void TimerEntry();
    };
}

#endif
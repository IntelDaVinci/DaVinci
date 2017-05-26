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

#ifndef __DAVINCI_GRACEFULTHREAD_HPP__
#define __DAVINCI_GRACEFULTHREAD_HPP__

#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include "boost/function.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"


namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// the loop body function type, return true means loop continues, return false means loop should exit
    /// </summary>
    /// <returns></returns>
    typedef boost::function<bool ()> Del;

    /// <summary>
    /// GracefulThread is a kind of thread that could exit willing, which does not require to be Aborted
    /// </summary>
    class GracefulThread : public boost::enable_shared_from_this<GracefulThread>
    {
    private:
        boost::shared_ptr<boost::thread> thread;
        boost::atomic<bool> stop;
        Del threadBody;
        string threadName;

    public:
        /// <summary>
        /// Create a new GracefulThread
        /// </summary>
        /// <param name="kernel">The loop kernel of the GracefulThread, which is a function returns bool value.  If Del() returns true, means the thread continue to another execution of Del(), otherwise the thread will end</param>
        /// <param name="background"> whether this gracefull thread is background thread </param>
        GracefulThread(Del kernel, bool background)
        {
            stop = false;
            threadBody = kernel;
            thread = nullptr;
        }

        ~GracefulThread()
        {
            if (thread != nullptr)
            {
                StopAndJoin();
                thread = nullptr;
            }
        }

        /// <summary>
        /// Start the GracefulThread
        /// </summary>
        void Start()
        {
            stop = false;

            thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&GracefulThread::loop, this)));
            if (thread != nullptr)
                SetThreadName(thread->get_id(), threadName);
        }

        /// <summary>
        /// Return whether the GracefulThread is alive
        /// </summary>
        /// <returns></returns>
        bool IsAlive()
        {
            if (thread != nullptr)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Set the name of GracefulThread
        /// </summary>
        /// <param name="name">The name of this thread</param>
        void SetName(string name)
        {
            threadName = name;
        }

        void loop ()
        {
            while (true)
            {
                if (!threadBody())
                    return;
                if (stop)
                {
                    return;
                }
            }
        }

        /// <summary>
        /// Request this thread to stop.  Note this won't stop the GracefulThread immediately, but it will stop eventurally after Del() exits, no matter what return value of Del() is
        /// </summary>
        void Stop()
        {
            stop = true;
        }
        /// <summary>
        /// Join this thread
        /// </summary>
        void Join()
        {
            thread->join();
        }

        /// <summary>
        /// Stop this thread and join
        /// </summary>
        void StopAndJoin()
        {
            if (IsAlive())
            {
                Stop();
                Join();
                thread = nullptr;
            }
        }
    };
}
#endif

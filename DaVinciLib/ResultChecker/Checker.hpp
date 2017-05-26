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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __CHECKER_HPP__
#define __CHECKER_HPP__

#include <string>

#include "DaVinciCommon.hpp"

namespace DaVinci
{
    using namespace std;

    enum CheckerResult
    {
        NotRun = 0,
        Pass = 1,
        Fail = 2
    };

    /// <summary> A base class for resultchecker. </summary>
    class Checker : public boost::enable_shared_from_this<Checker>
    {
        public:
            explicit Checker();

            virtual ~Checker();

            /// <summary>
            /// Gets the final result
            /// </summary>
            /// <returns></returns>
            virtual CheckerResult GetResult();

            /// <summary>
            /// Sets the final result
            /// </summary>
            /// <param name="result"></param>
            virtual void SetResult(CheckerResult result);

            /// <summary> 
            /// Gets checker name. 
            /// </summary>
            ///
            /// <returns> The checker name. </returns>
            virtual string GetCheckerName();

            /// <summary>
            /// Sets checker name. 
            /// </summary>
            /// <param name="name"></param>
            virtual void SetCheckerName(const string &name);

            /// <summary> 
            /// Gets error message. 
            /// </summary>
            /// <returns> The error message. </returns>
            virtual string GetErrorMessage();

            /// <summary>
            /// Sets error message
            /// </summary>
            /// <param name="error"></param>
            virtual void SetErrorMessage(const string &error);

            /// <summary>
            /// Clear error count
            /// </summary>
            virtual void ClearErrorCount();

            /// <summary>
            /// Increment error count
            /// </summary>
            virtual void IncrementErrorCount();

            /// <summary>
            /// Clear error count
            /// </summary>
            ///virtual void ClearErrorCount();

            /// <summary>
            /// Gets error count
            /// </summary>
            /// <returns> The error count. </returns>
            virtual int GetErrorCount();

            /// <summary>
            /// Add the file to reference image list
            /// </summary>
            virtual void AddToReferenceImageList(string refImageName);

            /// <summary>
            /// Gets the reference image list
            /// </summary>
            /// <returns> The reference image list </returns>
            virtual vector<string> GetReferenceImageList();
        
            /// <summary>
            /// Add the file to unmatched image list
            /// </summary>
            virtual void AddToFailedImageList(string failedImageName);

            /// <summary>
            /// Gets the unmatched image list
            /// </summary>
            /// <returns> The unmatched image list </returns>
            virtual vector<string> GetFailedImageList();

            /// <summary>
            /// Worker thread entry
            /// </summary>
            virtual void WorkerThread();

            /// <summary>
            /// Worker thread loop
            /// </summary>
            virtual void WorkerThreadLoop();

            /// <summary>
            /// Start worker thread
            /// </summary>
            virtual void Start();

            /// <summary>
            /// Kill worker thread
            /// </summary>
            virtual void Stop();

            /// <summary>
            /// Wait for worker thread exit
            /// </summary>
            virtual void Wait();

            /// <summary>
            /// Pause worker thread
            /// </summary>
            virtual void Pause();

            /// <summary>
            /// Resume worker thread
            /// </summary>
            virtual void Resume();

            /// <summary>
            /// Create output dir
            /// </summary>
            virtual void CreateOutputDir();

            /// <summary>
            /// Set the Worker Thread check interval
            /// </summary>
            virtual bool SetPeriod(int interval);

            /// <summary>
            /// Get the Worker Thread check interval
            /// </summary>
            virtual int GetPeriod();

            /// <summary>
            /// Return the checker's running status.
            /// </summary>
            /// <returns></returns>
            virtual bool GetRunningFlag();

            /// <summary>
            /// Set the checker's running status.
            /// </summary>
            /// <returns></returns>
            virtual void SetRunningFlag(bool result);

            /// <summary>
            /// Return the checker's pause status.
            /// </summary>
            /// <returns></returns>
            virtual bool GetPauseFlag();

            /// <summary>
            /// Set the checker's pause status.
            /// </summary>
            /// <returns></returns>
            virtual void SetPauseFlag(bool result);

            /// <summary>
            /// Trigger check if there is a working thread.
            /// </summary>
            /// <returns></returns>
            virtual void TriggerCheck();

            /// <summary> 
            /// Gets output Dir
            /// </summary>
            /// <returns> Output dir. </returns>
            virtual string GetOutputDir();

            /// <summary>
            /// Check if situation need to skip check.
            /// </summary>
            /// <returns>True for skip checking. </returns>
            virtual bool SkipCheck(int opcode);

        protected:
            /// <summary> The latest error message of failure. </summary>
            string checkerName;

            /// <summary> Final result of this checker. </summary>
            CheckerResult finalResult;

            /// <summary> Check Event. </summary>
            boost::shared_ptr<AutoResetEvent> triggerEvent;

            /// <summary> Error count. </summary>
            int errorCount;

            /// <summary> The latest error message of failure. </summary>
            string errorMessage;

            /// <summary> The reference image file list. </summary>
            vector<string> referenceImageList;

            /// <summary> The unmatched image file list. </summary>
            vector<string> failedImageList;

            /// <summary> Worker thread running status. </summary>
            boost::atomic<bool> runningFlag;

            /// <summary> Worker thread pause status. </summary>
            boost::atomic<bool> pauseFlag;

            /// <summary> Worker thread running period. </summary>
            int checkInterval;

            /// <summary> Worker thread handler. </summary>
            boost::shared_ptr<boost::thread> workerThreadHandler;

            /// <summary> Event for confirm worker thread is created. </summary>
            boost::shared_ptr<AutoResetEvent> createThreadEvent;

            /// <summary> Event for confirm worker thread is created. </summary>
            boost::shared_ptr<AutoResetEvent> terminateThreadEvent;

            /// <summary> Current activity name. </summary>
            string activityName;

            /// <summary> Current package name. </summary>
            string packageName;

            /// <summary> outputDir. </summary>
            string outputDir;
    };
}

#endif
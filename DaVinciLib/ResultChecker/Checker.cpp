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

#include "Checker.hpp"
#include "TestReport.hpp"
#include "QScript.hpp"
#include "DeviceManager.hpp"

namespace DaVinci
{
    Checker::Checker() : checkerName("BaseChecker"),
                        finalResult(CheckerResult::NotRun),
                        errorCount(0),
                        runningFlag(false),
                        pauseFlag(false),
                        checkInterval(1000)
    {
        errorMessage = "Not Set Error Message!";
        workerThreadHandler = nullptr;
        packageName = "";
        activityName = "";
        outputDir = "./";

        triggerEvent = boost::shared_ptr<AutoResetEvent>(new AutoResetEvent());
        createThreadEvent = boost::shared_ptr<AutoResetEvent>(new AutoResetEvent());
        terminateThreadEvent = boost::shared_ptr<AutoResetEvent>(new AutoResetEvent());
        
        assert(triggerEvent != nullptr);
        assert(createThreadEvent != nullptr);
        assert(terminateThreadEvent != nullptr);
    }

    Checker::~Checker()
    {
        Stop();
    }

    /// <summary>
    /// Create output dir
    /// </summary>
    void Checker::CreateOutputDir()
    {
        if (TestReport::currentQsLogPath.empty())
            outputDir = "./" + checkerName;
        else
            outputDir = TestReport::currentQsLogPath + "/" + checkerName;

        if (!boost::filesystem::exists(outputDir))
        {
            boost::system::error_code ec;
            boost::filesystem::create_directory(outputDir, ec);
            DaVinciStatus status(ec);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << outputDir;
                throw 1;
            }
        }
    }

    CheckerResult Checker::GetResult()
    {
        return finalResult;
    }

    /// <summary>
    /// Sets the final result
    /// </summary>
    /// <param name="result"></param>
    void Checker::SetResult(CheckerResult result)
    {
        finalResult = result;
    }

    /// <summary> Gets checker name. </summary>
    ///
    /// <returns> The checker name. </returns>
    string Checker::GetCheckerName()
    {
        return checkerName;
    }

    /// <summary>
    /// Sets checker name. </summary>
    /// </summary>
    /// <param name="name"></param>
    void Checker::SetCheckerName(const string &name)
    {
        checkerName = name;
    }

    /// <summary> Gets error message. </summary>
    ///
    /// <returns> The error message. </returns>
    string Checker::GetErrorMessage()
    {
        return errorMessage;
    }

    /// <summary>
    /// Sets error message
    /// </summary>
    /// <param name="error"></param>
    void Checker::SetErrorMessage(const string &error)
    {
        errorMessage = error;
    }

    /// <summary>
    /// Increment error count
    /// </summary>
    void Checker::IncrementErrorCount()
    {
        SetResult(CheckerResult::Fail);
        errorCount ++;
    }

    /// <summary>
    /// Clear error count
    /// </summary>
    void Checker::ClearErrorCount()
    {
        SetResult(CheckerResult::NotRun);
        errorCount = 0;
    }

    /// <summary>
    /// Gets error count
    /// </summary>
    /// <returns> The error count. </returns>
    int Checker::GetErrorCount()
    {
        return errorCount;
    }

    /// <summary>
    /// Add the file to reference image list
    /// </summary>
    void Checker::AddToReferenceImageList(string refImageName)
    {
        referenceImageList.push_back(refImageName);
    }

    /// <summary>
    /// Gets the reference image list
    /// </summary>
    /// <returns> The reference image list </returns>
    vector<string> Checker::GetReferenceImageList()
    {
        return  referenceImageList;
    }

    /// <summary>
    /// Add the file to failed image list
    /// </summary>
    void Checker::AddToFailedImageList(string failedImageName)
    {
        failedImageList.push_back(failedImageName);
    }

    /// <summary>
    /// Gets the unmatched image list
    /// </summary>
    /// <returns> The unmatched image list </returns>
    vector<string> Checker::GetFailedImageList()
    {
        return failedImageList;
    } 

    /// <summary>
    /// Worker thread entry
    /// </summary>
    void Checker::WorkerThread()
    {
        createThreadEvent->Set();

        WorkerThreadLoop();

        terminateThreadEvent->Set();
    }

    /// <summary>
    /// Set the Worker Thread check interval
    /// </summary>
    bool Checker::SetPeriod(int interval)
    {
        if (interval > 0)
        {
            checkInterval = interval;
            return true;
        }
        else
        {
            checkInterval = 1000;
            return false;
        }
    }

    /// <summary>
    /// Get the Worker Thread check interval
    /// </summary>
    int Checker::GetPeriod()
    {
        return checkInterval;
    }

    /// <summary>
    /// Worker thread loop
    /// </summary>
    void Checker::WorkerThreadLoop()
    {
        while (GetRunningFlag())
        {
            triggerEvent->WaitOne(checkInterval); 

            //Todo: Get current opcode or condition.
            if (GetPauseFlag() || (!GetRunningFlag()) || SkipCheck(DaVinci::QSEventAndAction::OPCODE_CLICK)) //The current opcode.
               continue;

            // Add Tasks Here:
        }
    }

    /// <summary>
    /// Check if situation need to skip check.
    /// </summary>
    /// <returns>True for skip checking. </returns>
    bool Checker::SkipCheck(int opcode)
    {
        return true;
    }

    /// <summary>
    /// Start worker thread
    /// </summary>
    /// <returns> True for success. </returns>
    void Checker::Start()
    {
        // Only enable the online checker thread when not use -noagent option and current device available.
        if ((DeviceManager::Instance().GetCurrentTargetDevice() != nullptr) && (DeviceManager::Instance().GetEnableTargetDeviceAgent()))
        {
            SetRunningFlag(true);

            if(workerThreadHandler == nullptr) 
            {
                workerThreadHandler = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Checker::WorkerThread, this)));
                SetThreadName(workerThreadHandler->get_id(), checkerName);
                createThreadEvent->WaitOne(2000);
            }
        }
    }

    /// <summary>
    /// Kill worker thread
    /// </summary>
    void Checker::Stop()
    {
        SetRunningFlag(false);
        TriggerCheck();
        Wait();
    }

    /// <summary>
    /// Wait for worker thread exit
    /// </summary>
    void Checker::Wait()
    {
        terminateThreadEvent->WaitOne(2000);

        if (workerThreadHandler != nullptr)
        {
            workerThreadHandler->join();
            workerThreadHandler = nullptr;
        }
    }

    /// <summary>
    /// Return the checker's running status.
    /// </summary>
    /// <returns>True for running!</returns>
    bool Checker::GetRunningFlag()
    {
        return runningFlag;
    }

    /// <summary>
    /// Set the checker's running status.
    /// </summary>
    /// <returns></returns>
    void Checker::SetRunningFlag(bool result)
    {
        runningFlag = result;
    }

    /// <summary>
    /// Pause worker thread
    /// </summary>
    /// <returns> True for success. </returns>
    void Checker::Pause()
    {
        SetPauseFlag(true);
    }

    /// <summary>
    /// Resume worker thread
    /// </summary>
    void Checker::Resume()
    {
        SetPauseFlag(false);
    }

    /// <summary>
    /// Return the checker's pause status.
    /// </summary>
    /// <returns>True for running!</returns>
    bool Checker::GetPauseFlag()
    {
        return pauseFlag;
    }

    /// <summary>
    /// Set the checker's pause status.
    /// </summary>
    /// <returns></returns>
    void Checker::SetPauseFlag(bool result)
    {
        pauseFlag = result;
    }

    /// <summary>
    /// Trigger check if there is a working thread.
    /// </summary>
    /// <returns></returns>
    void Checker::TriggerCheck()
    {
        triggerEvent->Set();
    }

    /// <summary> 
    /// Gets output Dir
    /// </summary>
    /// <returns> Output dir. </returns>
    string Checker::GetOutputDir()
    {
        CreateOutputDir();
        return outputDir;
    }
}
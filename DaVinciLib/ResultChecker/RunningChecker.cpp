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

#include "RunningChecker.hpp"

namespace DaVinci
{
    RunningChecker::RunningChecker(const boost::weak_ptr<ScriptReplayer> obj ):Checker()
    {
        SetCheckerName("RunningChecker");
        SetPeriod(30000); //30S
        boost::shared_ptr<ScriptReplayer> currentScriptTmp = obj.lock();

        if ((currentScriptTmp == nullptr) || (currentScriptTmp->GetQS() == nullptr))
        {
             DAVINCI_LOG_ERROR << "Failed to Init RunningChecker()!";
             throw 1;
        }

        currentQScript = currentScriptTmp;
        packageName = currentScriptTmp->GetQS()->GetPackageName();
    }

    RunningChecker::~RunningChecker()
    {
    }

    /// <summary>
    /// Worker thread entry
    /// </summary>
    void RunningChecker::WorkerThreadLoop()
    {
        while(GetRunningFlag())
        {
            triggerEvent->WaitOne(checkInterval); 

            {
                int opcode = QSEventAndAction::OPCODE_INSTALL_APP;
                boost::shared_ptr<QSEventAndAction> curEvent = nullptr;
                boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();

                assert(currentQScriptTmp != nullptr);

                curEvent = currentQScriptTmp->GetQS()->EventAndAction(currentQScriptTmp->GetQsIp());
                if (curEvent != nullptr)
                    opcode = curEvent->Opcode();

                if (ScriptReplayer::IsSmokeQS(currentQScriptTmp->GetQSMode()) || (SkipCheck(opcode)) || (GetPauseFlag()) || (packageName.empty()))
                    continue;
            }

            if (IsAppRunning(packageName)) 
            {
                PrintAppTopInfo(packageName);
                SetResult(CheckerResult::Pass);
            }
            else
            {
                if (GetPauseFlag() || (!GetRunningFlag()))
                    continue;

                SetResult(CheckerResult::Fail);
                boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();
                currentQScriptTmp->SetFinished();
                DAVINCI_LOG_INFO << "Detected the current application is NOT running!";
            }
        }
    }

    /// <summary>
    /// Check if app running once
    /// </summary>
    /// <param name="packageName"> package name of running process.</param>
    /// <returns>result</returns>
    bool RunningChecker::IsAppRunning(const string &pkgName)
    {
        boost::shared_ptr<AndroidTargetDevice> androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());

        if ((androidDevice == nullptr) || (pkgName.empty()))
            return false;

        if (androidDevice->FindProcess(pkgName) == -1)
            return false;
        else 
            return true;
    }

    /// <summary>
    /// Check if current QS status need to skip app running check.
    /// </summary>
    /// <returns>True for skip checking. </returns>
    bool RunningChecker::SkipCheck(int opcode)
    {
        switch (opcode)
        {
            case QSEventAndAction::OPCODE_UNINSTALL_APP:
            case QSEventAndAction::OPCODE_INSTALL_APP:
            case QSEventAndAction::OPCODE_START_APP:
            case QSEventAndAction::OPCODE_STOP_APP:
            case QSEventAndAction::OPCODE_PUSH_DATA:
//            case QSEventAndAction::OPCODE_EXIT: //Need do app check when exit.
                return true;
            default:
                return false;
        }
    }

     /// <summary>
    /// Print app's system load
    /// </summary>
    /// <param name="packageName"> package name of running process.</param>
    bool RunningChecker::PrintAppTopInfo(const string &pkgName)
    {
        string appTopInfo = "";

        boost::shared_ptr<AndroidTargetDevice> androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());

        if ((androidDevice == nullptr) || (pkgName.empty()))
            return false;

        appTopInfo = androidDevice->GetPackageTopInfo(pkgName);

        DAVINCI_LOG_INFO <<  appTopInfo;

        return true;
    }
}
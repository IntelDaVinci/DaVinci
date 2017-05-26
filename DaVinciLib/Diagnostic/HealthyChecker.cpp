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

#include "HealthyChecker.hpp"  
#include "TestManager.hpp"

namespace DaVinci
{
    HealthyChecker::HealthyChecker(void) : Checker()
    {  
    }  
  
    HealthyChecker::~HealthyChecker(void)  
    {  
    }  

    bool HealthyChecker::LoadCheckList()
    {
        vector <string> lines;
        string hostCheckListFile = TestManager::Instance().GetDaVinciResourcePath("HostCheckList.txt");
        string deviceCheckListFile = TestManager::Instance().GetDaVinciResourcePath("DeviceCheckList.txt");
        CommandChecker tmpCmd;

        lockOfCheckList.lock();

        ReadAllLines(hostCheckListFile, lines);
        if ((lines.empty()) || (lines.size() < 2))
            DAVINCI_LOG_ERROR << string("Incorrect HostCheckList file : ") << hostCheckListFile;
        else
        {
            hostCheckList.clear();
            for(int index = 1; index < (int)(lines.size()); index++) //The first line is title.
            {
                string line = lines[index];
                vector<string> items;

                boost::trim(line);

                boost::algorithm::split(items, line, boost::algorithm::is_any_of(","));
                if(items.size() == 4)
                {
                    string value;

                    value = items[0];
                    if (value != "")
                        TryParse(value, tmpCmd.number);
                    tmpCmd.type = CommandType::WINCMD;
                    tmpCmd.cmdStr = items[1];
                    boost::trim(tmpCmd.cmdStr);
                    tmpCmd.cmdArgs = items[2];
                    boost::trim(tmpCmd.cmdArgs);
                    tmpCmd.keyword = items[3];
                    boost::trim(tmpCmd.keyword);
                    tmpCmd.result = CommandResult::NOTRUN;
                    hostCheckList.push_back(tmpCmd);
                }
            }
        }

        lines.clear();
        ReadAllLines(deviceCheckListFile, lines);
        if (lines.size() < 2)
            DAVINCI_LOG_ERROR << string("Incorrect DeviceCheckList file : ") << deviceCheckListFile;
        else
        {
            deviceCheckList.clear();
            for(int index = 1; index < (int)(lines.size()); index++) //The first line is title.
            {
                string line = lines[index];
                vector<string> items;

                boost::trim(line);

                boost::algorithm::split(items, line, boost::algorithm::is_any_of(","));
                if(items.size() == 4)
                {
                    string value;

                    value = items[0];
                    if (value != "")
                        TryParse(value, tmpCmd.number);
                    tmpCmd.type = CommandType::ADBCMD;
                    tmpCmd.cmdStr = items[1];
                    boost::trim(tmpCmd.cmdStr);
                    tmpCmd.cmdArgs = items[2];
                    boost::trim(tmpCmd.cmdArgs);
                    tmpCmd.keyword = items[3];
                    boost::trim(tmpCmd.keyword);
                    tmpCmd.result = CommandResult::NOTRUN;
                    deviceCheckList.push_back(tmpCmd);
                }
            }
        }

        lockOfCheckList.unlock();

        return true;
    }

    void HealthyChecker::WorkerThreadLoop()
    {
        DaVinciStatus status = errc::not_supported;
        vector<CommandChecker> tmpList;
        bool isWinCmdTitlePrinted = false;
        bool isAdbCmdTitlePrinted = false;

        lockOfCheckList.lock();

        for (unsigned  int i = 0; i < currentCheckList.size(); i++)
        {
            status = errc::not_supported;
            currentCheckList[i].result = CommandResult::FAIL;
    
            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            try
            {
                if (currentCheckList[i].type == CommandType::WINCMD)
                {
                    string command = boost::process::find_executable_in_path(currentCheckList[i].cmdStr);

                    status = RunShellCommand(command + " " + currentCheckList[i].cmdArgs, outStr, "./", WaitForRunCommand);
                } 
                else if (currentCheckList[i].type == CommandType::ADBCMD)
                {
                    boost::shared_ptr<AndroidTargetDevice> androidDut;

                    if (GetDeviceInfo::GetCurrentDevice(androidDut))
                        status = androidDut->AdbCommand(currentCheckList[i].cmdArgs, outStr, WaitForRunCommand);
                }
            }
            catch (...)
            {
                DAVINCI_LOG_ERROR << "Failed to run command: " << currentCheckList[i].cmdStr;
            }

            if (DaVinciSuccess(status))
            {
                currentCheckList[i].outputStr = outStr->str();
                if ((!currentCheckList[i].outputStr.empty()) && (currentCheckList[i].outputStr.find(currentCheckList[i].keyword) != string::npos))
                    currentCheckList[i].result = CommandResult::PASS;
                else
                    currentCheckList[i].result = CommandResult::FAIL;
            }
        }

        DAVINCI_LOG_INFO << "DiagnosticReportStart" ;

        for (unsigned  int i = 0; i < currentCheckList.size(); i++)
        {
            if (currentCheckList[i].type != CommandType::WINCMD)
                continue;
            if (!isWinCmdTitlePrinted)
            {
                DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
                DAVINCI_LOG_INFO << "Host Diagnostic Report:";
                DAVINCI_LOG_INFO << "Number    Command    Arguments    Keywords    Result ";
                isWinCmdTitlePrinted = true;
            }

            if (currentCheckList[i].result == CommandResult::PASS)
                DAVINCI_LOG_INFO <<  currentCheckList[i].number << "    " << currentCheckList[i].cmdStr << "    " << currentCheckList[i].cmdArgs <<"    " << currentCheckList[i].keyword << "    PASS";
            else
            {
                DAVINCI_LOG_INFO <<  currentCheckList[i].number << "    " << currentCheckList[i].cmdStr << "    " << currentCheckList[i].cmdArgs <<"    " << currentCheckList[i].keyword << "    FAIL";
                IncrementErrorCount();
            }
        }

        for (unsigned  int i = 0; i < currentCheckList.size(); i++)
        {
            if (currentCheckList[i].type != CommandType::ADBCMD)
                continue;

            if (!isAdbCmdTitlePrinted)
            {
                DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
                DAVINCI_LOG_INFO << "Device Diagnostic Report:";
                DAVINCI_LOG_INFO << "Number    Command    Arguments    Keywords    Result ";
                isAdbCmdTitlePrinted = true;
            }

            if (currentCheckList[i].result == CommandResult::PASS)
                DAVINCI_LOG_INFO << currentCheckList[i].number << "    " << currentCheckList[i].cmdStr << "    " << currentCheckList[i].cmdArgs << "    " << currentCheckList[i].keyword << "    PASS";
            else
            {
                DAVINCI_LOG_INFO <<  currentCheckList[i].number << "    " << currentCheckList[i].cmdStr << "    " << currentCheckList[i].cmdArgs << "    " << currentCheckList[i].keyword <<"    FAIL";
                IncrementErrorCount();
            }
        }

        if (isAdbCmdTitlePrinted || isWinCmdTitlePrinted)
            DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;

        DAVINCI_LOG_INFO << "DiagnosticReportEnd" ;

        lockOfCheckList.unlock();
    }

    bool HealthyChecker::StopAllTest()
    {
        Stop();

        lockOfCheckList.lock();
        currentCheckList.clear();
        lockOfCheckList.unlock();

        return true;
    }

    bool HealthyChecker::RunHostCheckList()
    {
        Wait();
        lockOfCheckList.lock();
        currentCheckList.clear();
        currentCheckList.insert(currentCheckList.end(), hostCheckList.begin(), hostCheckList.end());
        lockOfCheckList.unlock();
        Start();
        Wait();

        return true;
    }

    bool HealthyChecker::RunDeviceCheckList()
    {
        Wait();
        lockOfCheckList.lock();
        currentCheckList.clear();
        currentCheckList.insert(currentCheckList.end(), deviceCheckList.begin(), deviceCheckList.end());
        lockOfCheckList.unlock();
        Start();
        Wait();

        return true;
    }
}

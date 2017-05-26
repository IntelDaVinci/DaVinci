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

#include "MPMPowerMeasure.hpp"
#include "TestManager.hpp"
#include "HyperSoftCamCapture.hpp"

#include "unzipper.hpp"

namespace DaVinci
{
    MPMPowerMeasure::MPMPowerMeasure(boost::shared_ptr<QScript> qs,  boost::shared_ptr<TargetDevice> targetDev) :
        PowerMeasure(qs, targetDev),
        mStartActionElapsedTime(0.0),
        mDeviceConnected(true),
        mCollectedReport(false),
        mHWAcc(nullptr),
        mQS(qs),
        HW_LIGHT_TIME(5000.0),
        MPM_PACKAGE_NAME("com.intel.mpm.app"),
        MPM_SESSIONS_PATH("/sdcard/Android/data/com.intel.mpm.app/files/mpm/sessions/")
    {
        assert(targetDev != nullptr);
        mAndroidDev = boost::dynamic_pointer_cast<AndroidTargetDevice>(targetDev);
        assert(mAndroidDev != nullptr);
        mDevName = mAndroidDev->GetDeviceName();

        boost::filesystem::path tmp(mQS->GetQsFileName());
        mQSPath = tmp.parent_path().string();
        mGeneratedQS = tmp.stem().string() + "_OnDeviceRnR" + tmp.extension().string();

        mSrcPWTBegSHPath = TestManager::Instance().GetDaVinciResourcePath("PowerTest/MPM/pwt_beg.sh");
        mSrcPWTEndSHPath = TestManager::Instance().GetDaVinciResourcePath("PowerTest/MPM/pwt_end.sh");
        boost::filesystem::path pwtBegSH(mSrcPWTBegSHPath);
        boost::filesystem::path pwtEndSH(mSrcPWTEndSHPath);
        mDstPWTBegSHPath = pwtBegSH.stem().string() + "_" + tmp.stem().string() + pwtBegSH.extension().string();
        mDstPWTEndSHPath = pwtEndSH.stem().string() + "_" + tmp.stem().string() + pwtEndSH.extension().string();
    }

    DaVinciStatus MPMPowerMeasure::Start()
    {
        // Timing current function run time
        ElapsedTime timing(mStartActionElapsedTime);

        DaVinciStatus ret = HyperSoftCamCapture::InstallAgent(mAndroidDev);
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Cannot install HyperSoftCam agent to target device";
            return ret;
        }

        mHWAcc = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (mHWAcc == nullptr)
        {
            DAVINCI_LOG_ERROR << "Cannot find hardware accessory";
            return DaVinciStatus(errc::no_such_device);
        }
        if (!mHWAcc->ReturnFirmwareName())
        {
            DAVINCI_LOG_ERROR << "Current hardware accessory does not connect to host";
            return DaVinciStatus(errc::not_connected);
        }

        if (!IntializeMPMEnv())
        {
            DAVINCI_LOG_ERROR << "Failed in initializing MPM environment";
            return DaVinciStatus(errc::device_or_resource_busy);
        }

        if (!GenerateOnDevQS())
        {
            DAVINCI_LOG_ERROR << "Failed in generating OnDevice QScript";
            return DaVinciStatus(errc::io_error);
        }

        boost::filesystem::path dstQSPath(mQSPath);
        dstQSPath /= mGeneratedQS;
        boost::filesystem::path dstAPKPath(mQSPath);
        dstAPKPath /= mQS->GetApkName();

        mAndroidDev->AdbPushCommand(mSrcPWTBegSHPath, "/data/local/tmp/" + mDstPWTBegSHPath);
        mAndroidDev->AdbPushCommand(mSrcPWTEndSHPath, "/data/local/tmp/" + mDstPWTEndSHPath);
        mAndroidDev->AdbPushCommand(dstAPKPath.string(), std::string("/data/local/tmp/" + mQS->GetApkName()));
        mAndroidDev->AdbPushCommand(dstQSPath.string(), std::string("/data/local/tmp/" + mGeneratedQS));

        // Kill existed monkey service
        // TODO: I think we need to remove monkey dependency
        mAndroidDev->KillProcess("com.android.commands.monkey");
        ret = mAndroidDev->InstallDaVinciMonkey();
        if (!DaVinciSuccess(ret))
            return ret;

        if (!StartupReplayOnDev())
        {
            DAVINCI_LOG_ERROR << "Cannot startup HyperSoftCam to run QScript on device directly.";
            return DaVinciStatus(errc::no_such_process);
        }

        mHWAcc->StartMeasuringPower();
        mDeviceConnected = false;
        return DaVinciStatus(DaVinciStatusSuccess);
    }

    DaVinciStatus MPMPowerMeasure::Stop()
    {
        int timeToSleep = boost::numeric_cast<int>(HW_LIGHT_TIME + mStartActionElapsedTime);
        ThreadSleep(timeToSleep);
        mHWAcc->StopMeasuringPower();

        // Wait for device is connected
        const int MAX_LOOP_CNT = 4;
        for (int i = 0; i < MAX_LOOP_CNT; i++)
        {
            if (mAndroidDev->IsDevicesAttached())
            {
                i = MAX_LOOP_CNT;
                mDeviceConnected = true;
                break;
            }
            else
            {
                ThreadSleep(1000);
            }
        }

        if (!mDeviceConnected) {
            DAVINCI_LOG_ERROR << "Cannot find target device with serial no: " << mDevName;
            return DaVinciStatus(errc::no_such_device);
        } else {
            mAndroidDev->AdbShellCommand(std::string("rm /data/local/tmp/" + mDstPWTBegSHPath));
            mAndroidDev->AdbShellCommand(std::string("rm /data/local/tmp/" + mDstPWTEndSHPath));
            mAndroidDev->AdbShellCommand(std::string("rm /data/local/tmp/" + mQS->GetApkName()));
            mAndroidDev->AdbShellCommand(std::string("rm /data/local/tmp/" + mGeneratedQS));
            return DaVinciStatus(DaVinciStatusSuccess);
        }
    }

    DaVinciStatus MPMPowerMeasure::CollectResult(void *para)
    {
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;
        
        if (!mDeviceConnected) {
            DAVINCI_LOG_ERROR << "Cannot get power measure data from target device.";
            return DaVinciStatus(errc::no_such_device);
        }

        mAndroidDev->AdbShellCommand("ls " + MPM_SESSIONS_PATH, outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        if (lines.size() == 0)
        {
            DAVINCI_LOG_ERROR << "Cannot find MPM result.";
            return DaVinciStatus(errc::no_such_file_or_directory);
        }

        auto pos = lines[0].find("DaVinciPowerMeasure");
        if (pos == std::string::npos)
        {
            DAVINCI_LOG_ERROR << "Cannot find DaVinci power measure result.";
            return DaVinciStatus(errc::no_such_file_or_directory);
        }

        DaVinciStatus dvcStatus(DaVinciStatusSuccess);
        dvcStatus = mAndroidDev->AdbCommand("pull " + MPM_SESSIONS_PATH + " " + mQSPath);
        if (!DaVinciSuccess(dvcStatus))
        {
            DAVINCI_LOG_ERROR << "Cannot get DaVinci power measure result.";
            return DaVinciStatus(errc::no_such_file_or_directory);
        }

        DAVINCI_LOG_INFO << "Collect power measure data successfully.";
        mCollectedReport = true;
        mRawReportName = lines[0];
        // Remove carriage return
        boost::trim_right_if(mRawReportName, boost::is_any_of("\r\n"));
        return DaVinciStatus(DaVinciStatusSuccess);
    }

    DaVinciStatus MPMPowerMeasure::GenerateReport()
    {
        DaVinciStatus dvcStatus;
        if (!mCollectedReport)
        {
            dvcStatus = CollectResult(nullptr);
            if (!DaVinciSuccess(dvcStatus))
            {
                DAVINCI_LOG_ERROR << "Cannot generate power test report";
                return dvcStatus;
            }
        }

        if (mRawReportName.empty())
        {
            DAVINCI_LOG_ERROR << "Cannot find raw power test report";
            return DaVinciStatus(errc::no_such_file_or_directory);
        }

        boost::filesystem::path rawTestReportPath(mQSPath);
        rawTestReportPath /= boost::filesystem::path(mRawReportName);
        rawTestReportPath /= boost::filesystem::path("session.zip");
        boost::system::error_code ec;
        if (!boost::filesystem::exists(rawTestReportPath, ec))
        {
            DAVINCI_LOG_ERROR << "Does not support current raw power test report. (" << ec.message() << ")";
            return DaVinciStatus(errc::function_not_supported);
        }

        if (!ParseRawReport(rawTestReportPath.string()))
        {
            DAVINCI_LOG_ERROR << "Failed in parsing raw power test report: " << rawTestReportPath.string();
            return DaVinciStatus(errc::illegal_byte_sequence);
        }
        else
        {
            DAVINCI_LOG_INFO << "Succeeded in generating power test report";
            mCaseInfo.insert(std::make_pair("case", "MPM"));
            string reportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
            TestReportSummary testReportSummary;
            return testReportSummary.appendPowerMeasureResultNode(reportSummaryName, mCaseInfo, mBatteryEnergy);
        }
    }

    bool MPMPowerMeasure::IntializeMPMEnv()
    {
        DaVinciStatus dvcStatus(DaVinciStatusSuccess);

        // MPM solution just supports IA devices
        std::string cpuArch = mAndroidDev->GetCpuArch();
        if (cpuArch.find("x86") == string::npos)
        {
            DAVINCI_LOG_ERROR << "MPM solution just supports IA device";
            return false;
        }

        // Install MPM APK
        std::vector<std::string> allInstalledPackages = mAndroidDev->GetAllInstalledPackageNames();
        std::vector<std::string>::iterator it;
        // TODO: How to upgrade MPM?
        it = std::find(allInstalledPackages.begin(), allInstalledPackages.end(), MPM_PACKAGE_NAME);
        if (it == allInstalledPackages.end())
        {
            std::string mpmAPKPath = TestManager::Instance().GetDaVinciResourcePath("PowerTest/MPM/MPM.apk");
            dvcStatus = mAndroidDev->InstallApp(mpmAPKPath);
            if (!DaVinciSuccess(dvcStatus))
            {
                DAVINCI_LOG_ERROR << "Failed in installing MPM apk";
                return false;
            }

            // Since we need run MPM at without GUI, so we need run MPM with GUI and send OPT_IN to accept its
            // use terms firstly to enable MPM service.
            dvcStatus = mAndroidDev->AdbShellCommand("am start com.intel.mpm.app/.gui.MPMClient");
            if (!DaVinciSuccess(dvcStatus))
            {
                DAVINCI_LOG_ERROR << "Failed in implementing pre-requisite action(starting GUI) for installing MPM";
                return false;
            }

            dvcStatus = mAndroidDev->AdbShellCommand("am broadcast -a intel.intent.action.mpm.OPT_IN");
            if (!DaVinciSuccess(dvcStatus))
            {
                DAVINCI_LOG_ERROR << "Failed in implementing pre-requisite action(OPT_IN) for installing MPM";
                return false;
            }
        }

        // Enable DaVinci MPM profile
        const std::string MPM_CONFIG_PATH("/sdcard/Android/data/com.intel.mpm.app/files/mpm/config/profiles.cfg");
        std::string mpmCfgPath = TestManager::Instance().GetDaVinciResourcePath("PowerTest/MPM/DaVinciProfiles.cfg");
        dvcStatus = mAndroidDev->AdbPushCommand(mpmCfgPath, MPM_CONFIG_PATH);
        if (!DaVinciSuccess(dvcStatus))
        {
            DAVINCI_LOG_ERROR << "Cannot push MPM configure profile to device with error("
                              << dvcStatus.value()
                              << ":"
                              << dvcStatus.message()
                              << ")";
            return false;
        }

        // Restart MPM to enable DaVinci MPM profile
        mAndroidDev->AdbShellCommand("am force-stop " + MPM_PACKAGE_NAME);
        if (mAndroidDev->FindProcess(MPM_PACKAGE_NAME) > 0)
        {
            DAVINCI_LOG_ERROR << "Cannot stop MPM package to enable new configure profile";
            return false;
        }

        mAndroidDev->AdbShellCommand("am startservice " + MPM_PACKAGE_NAME + "/.AppInitService");
        if (mAndroidDev->FindProcess(MPM_PACKAGE_NAME) <= 0)
        {
            DAVINCI_LOG_ERROR << "Cannot start MPM package to enable new configure profile";
            return false;
        }

        // Clean MPM cached data
        mAndroidDev->AdbShellCommand("rm -rf " + MPM_SESSIONS_PATH);

        DAVINCI_LOG_INFO << "Succeeded in initializing MPM environment";
        return true;
    }

    bool MPMPowerMeasure::GenerateOnDevQS()
    {
        std::string srcQS = mQS->GetQsFileName();
        boost::filesystem::path srcQSPath(srcQS);
        boost::filesystem::path dstQSPath(mQSPath);
        bool stopRemoveFlag = false;
        double timeStampOffset = 0.0;

        dstQSPath /= mGeneratedQS;
        if (boost::filesystem::exists(dstQSPath))
            boost::filesystem::remove(dstQSPath);
        boost::filesystem::copy(srcQSPath, dstQSPath);
        boost::shared_ptr<QScript> dstQsFile = QScript::Load(dstQSPath.string());
        vector<boost::shared_ptr<QSEventAndAction>> allEvts = dstQsFile->GetAllQSEventsAndActions();
        vector<boost::shared_ptr<QSEventAndAction>>::iterator preEvt = allEvts.begin();

        for (auto it = allEvts.begin(); it != allEvts.end(); preEvt = it, it++)
        {
            boost::shared_ptr<QSEventAndAction> curEvt = (*it);
            if (curEvt->Opcode() != QSEventAndAction::OPCODE_POWER_MEASURE_START &&
                curEvt->Opcode() != QSEventAndAction::OPCODE_POWER_MEASURE_STOP)
            {
                if (!stopRemoveFlag)
                {
                    dstQsFile->RemoveEventAndAction(curEvt->LineNumber());
                }
            }

            if (stopRemoveFlag)
            {
                double evtTimeStamp = curEvt->TimeStamp();
                if (FEquals(timeStampOffset, 0.0))
                {
                    timeStampOffset = evtTimeStamp;
                }
                curEvt->SetTimeStamp(evtTimeStamp - timeStampOffset + HW_LIGHT_TIME);
            }

            if (curEvt->Opcode() == QSEventAndAction::OPCODE_POWER_MEASURE_START)
            {
                stopRemoveFlag = true;
                curEvt->Opcode() = QSEventAndAction::OPCODE_CALL_EXTERNAL_SCRIPT;
                curEvt->SetTimeStamp(timeStampOffset + HW_LIGHT_TIME);
                curEvt->StrOperand(0) = "/data/local/tmp/" + mDstPWTBegSHPath;
            }
            else if (curEvt->Opcode() == QSEventAndAction::OPCODE_POWER_MEASURE_STOP)
            {
                stopRemoveFlag = false;
                curEvt->Opcode() = QSEventAndAction::OPCODE_CALL_EXTERNAL_SCRIPT;
                curEvt->SetTimeStamp((*preEvt)->TimeStamp());
                curEvt->StrOperand(0) = "/data/local/tmp/" + mDstPWTEndSHPath;
            }
        }
        dstQsFile->Flush();
        return true;
    }

    bool MPMPowerMeasure::StartupReplayOnDev()
    {
        // We find that the adb shell will not exit even HyperSoftCam has created a child process successfully
        // and returned. The adb shell will exit until HyperSoftCam child process has exited.
        // So we run adb shell asynchronous and check its output to don't break power test sequence.
        const int ADB_SHELL_TIMEOUT = 25; // (ms)
        const int HSC_TIME_OUT = boost::numeric_cast<const int>(HW_LIGHT_TIME * 2);
        namespace bp = boost::process;
        std::vector<std::string> args; 
        args.push_back(AndroidTargetDevice::GetAdbPath());
        args.push_back("-s");
        args.push_back(mDevName);
        args.push_back("shell");
        args.push_back("/data/local/tmp/camera_less --play /data/local/tmp/" + mGeneratedQS);

        bp::context ctx;
        ctx.work_directory = ".";
        ctx.stdout_behavior = bp::capture_stream();
        bp::child replayOnDev = bp::launch(AndroidTargetDevice::GetAdbPath(), args, ctx);

        // Check HyperSoftCam result
        bp::pistream &replayOnDevOutput = replayOnDev.get_stdout();
        std::string replayOnDevOutStr;
        int i = ADB_SHELL_TIMEOUT;
        while (i < HSC_TIME_OUT)
        {
            // The "REPLAY_CREATER_EXIT" is outputted by HyperSoftCam android client.
            std::getline(replayOnDevOutput, replayOnDevOutStr);
            if (replayOnDevOutStr.find("REPLAY_CREATER_EXIT") != std::string::npos)
                break;
            ThreadSleep(ADB_SHELL_TIMEOUT);
            i += ADB_SHELL_TIMEOUT;
        }

        try
        {
            replayOnDev.terminate();
        }
        catch (boost::system::system_error err)
        {
            DAVINCI_LOG_ERROR << err.code() << ":" << err.what();
        }

        if (i >= HSC_TIME_OUT)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool MPMPowerMeasure::ParseRawReport(std::string rawReportPath)
    {
        const char               *DATA_STATS        = "data_stats_0.txt";
        const char               *DATA_SESSION_INFO = "data_session_info.txt";
        std::vector<std::string>  lines;

        if (!ReadZipEntry(rawReportPath.c_str(), DATA_STATS, lines))
        {
            DAVINCI_LOG_ERROR << "Cannot find file: " << rawReportPath << "::" << DATA_STATS;
            return false;
        }
        for (auto it = lines.begin(); it != lines.end(); ++it)
        {
            std::string line = *it;
            vector<string> colItems;
            boost::algorithm::split(colItems, line, boost::algorithm::is_any_of(","));
            if (colItems.size() < 3)
                continue;
            mBatteryEnergy.insert(std::make_pair(colItems[0], colItems[1] + colItems[2]));
        }
        if (mBatteryEnergy.size() <= 0)
        {
            DAVINCI_LOG_ERROR << "Cannot fine useful information in " << DATA_STATS;
            return false;
        }
        if (mBatteryEnergy.find(std::string("Battery Energy Used")) == mBatteryEnergy.end() ||
            mBatteryEnergy.find(std::string("Elapsed Time")) == mBatteryEnergy.end())
        {
            DAVINCI_LOG_ERROR << "Cannot find power test elapsed time or battery energy used.";
            return false;
        }

        lines.clear();
        if (!ReadZipEntry(rawReportPath.c_str(), DATA_SESSION_INFO, lines))
        {
            DAVINCI_LOG_WARNING << "Cannot find file: " << rawReportPath << "::" << DATA_SESSION_INFO;
        }
        else
        {
            for (auto it = lines.begin(); it != lines.end(); ++it)
            {
                std::string line = *it;
                vector<string> colItems;
                boost::algorithm::split(colItems, line, boost::algorithm::is_any_of("="));
                if (colItems.size() < 2)
                    continue;
                mDeviceInfo.insert(std::make_pair(colItems[0], colItems[1]));
            }
            if (mDeviceInfo.size() <= 0)
            {
                DAVINCI_LOG_WARNING << "Cannot fine useful information in " << DATA_STATS;
            }
        }

        return true;
    }

    bool MPMPowerMeasure::ReadZipEntry(
        const std::string rawReportPath,
        const std::string entryName,
        std::vector<std::string> &entryItems)
    {
        ziputils::unzipper        unZipper;
        std::ostringstream        oss;
        std::vector<std::string>  lines;

        if (!unZipper.open(rawReportPath.c_str()))
        {
            DAVINCI_LOG_ERROR << "Cannot open file: " << rawReportPath;
            return false;
        }

        if (!unZipper.openEntry(entryName.c_str()))
        {
            DAVINCI_LOG_ERROR << "Cannot open file: " << entryName << " in " << rawReportPath;
            return false;
        }
        unZipper >> oss;
        ReadAllLinesFromStr(oss.str(), entryItems);
        return true;
    }
}

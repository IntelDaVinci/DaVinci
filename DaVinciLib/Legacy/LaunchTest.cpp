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

#include "LaunchTest.hpp"
#include "DaVinciCommon.hpp"
#include "DaVinciDefs.hpp"
#include "DeviceManager.hpp"
#include "TimeMeasure.hpp"

using namespace cv;


namespace DaVinci
{
    LaunchTest::LaunchTest(const std::string &iniFilename)
    {
        ini_filename = iniFilename;
    }

    bool LaunchTest::Init()
    {
        SetName("LaunchTest");
        DAVINCI_LOG_INFO<<(std::string("LaunchTest module: ini file name = ") + ini_filename);

        std::vector<std::string> lines;
        if (!DaVinciSuccess(ReadAllLines(ini_filename,lines)))
        {
            return false;
        }

        // must contain: apk name, activity name, reference image, resize ratio
        if (lines.size() < 4)
        {
            DAVINCI_LOG_INFO<<(std::string("Bad ini file ") + ini_filename);
            return false;
        }
        apk_class = lines[0];
        std::string cmd_param = lines[1];
        
        boost::shared_ptr<AndroidTargetDevice> device = boost::dynamic_pointer_cast<AndroidTargetDevice>((DeviceManager::Instance()).GetCurrentTargetDevice());
        if (device != nullptr)
        {
            // tap home              input keyevent 3
            device->AdbShellCommand("input keyevent 3");
            device->StopActivity(apk_class);

            std::string adbCommand = string("am start ") + apk_class + "/" + cmd_param;
            const int WaitForStartActivity = 30000;

            if (DeviceManager::Instance().UsingHyperSoftCam()) {
                adbCommand = std::string("\"") + adbCommand + " && /data/local/tmp/camera_less --rt\"";
                auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
                device->AdbShellCommand(adbCommand, outStr, WaitForStartActivity);
                vector<string> lines;
                ReadAllLinesFromStr(outStr->str(), lines);
                if (lines.size() == 0) {
                    DAVINCI_LOG_ERROR << "Failed in getting device current time stamp";
                    return false;
                }

                try {
                    boost::trim(lines.back());
                    DeviceFrameIndexInfo::devCurTimeStamp = boost::lexical_cast<int64>(lines.back());
                } catch (boost::bad_lexical_cast & e) {
                    DAVINCI_LOG_ERROR << e.what();
                    return false;
                }
            } else {
                device->AdbShellCommand(adbCommand, nullptr, WaitForStartActivity);
            }
        }
        StartGroup();
        return true;
    }

    cv::Mat LaunchTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        return frame;
    }    

    CaptureDevice::Preset LaunchTest::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }
}

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

#include "DeviceManager.hpp"
#include "HWAccessoryController.hpp"
#include "TestInterface.hpp"
#include "TestManager.hpp"
#include "FfrdHorizontalCalibration.hpp"

namespace DaVinci
{
    //Ready
    FfrdHorizontalCalibration::FfrdHorizontalCalibration()
    {

    }

    //Ready
    FfrdHorizontalCalibration::~FfrdHorizontalCalibration()
    {

    }

    //Ready
    bool FfrdHorizontalCalibration::Init()
    {
        SetName("FFRD_Horizontal_Calibration_Test");
        DAVINCI_LOG_INFO << string("*************************************************************");
        DAVINCI_LOG_INFO << string("Command line to calibrate FFRD horizontal!");
        DAVINCI_LOG_INFO << string("*************************************************************");

        //Return if no FFRD is connected!
        curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if (curHWController == nullptr)
        {
            DAVINCI_LOG_ERROR << string("No FFRD connected. Quit test.");
            SetFinished();
            return false;
        }
        //Return if FFRD has old type of servo motors!
        major_version = curHWController->ReturnMajorVersionOfFirmware();
        if (major_version < 2)
        {
            DAVINCI_LOG_INFO << string("The FFRD has old type of servo motors without feedback functions.");
            DAVINCI_LOG_INFO << string("Cannot continue with this horizontal calibration. Quit test.");
            SetFinished();
            return false;
        }
        else
        {
            DAVINCI_LOG_INFO << string("The FFRD has new type of servo motors with feedback functions.");
            DAVINCI_LOG_INFO << string("Can continue with this horizontal calibration.");
        }
        //Check if target device is connected!
        targetDevice = (DeviceManager::Instance()).GetCurrentTargetDevice();
        androidTargetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(targetDevice);
        if (androidTargetDevice == nullptr)
        {
            DAVINCI_LOG_WARNING << string("No android target device connected. Cannot get the horizontal angles form Agent for reference!");
            agent_connection_flag = 0;
        }
        else
        {
            DAVINCI_LOG_INFO << string("Android target device is connected. Can get the horizontal angles form Agent for reference!");
            agent_connection_flag = 1;
        }

        processCounter = 20;
        speedMotorOne = 0;
        speedMotorTwo = 0;
        angleMotorOne = 0;
        angleMotorTwo = 0;
        TestManager::Instance().TiltCenter();
        speedMotorOne = curHWController->GetMotorSpeed(1);
        speedMotorTwo = curHWController->GetMotorSpeed(2);
        while (speedMotorOne != 0 || speedMotorTwo != 0)
        {
            speedMotorOne = curHWController->GetMotorSpeed(1);
            speedMotorTwo = curHWController->GetMotorSpeed(2);
        }
        if (speedMotorOne == 0 && speedMotorTwo == 0)
        {
            angleMotorOne = curHWController->GetMotorAngle(1);
            angleMotorTwo = curHWController->GetMotorAngle(2);
            DAVINCI_LOG_INFO << string("The original angle of servo motor No.1 is: ") << (angleMotorOne - 60);
            DAVINCI_LOG_INFO << string("The original angle of servo motor No.2 is: ") << (angleMotorTwo - 60);

#pragma region Get rotation data from Qagent
            rotationDataMap = androidTargetDevice->GetRotationDataFromQAgent();
            if(!rotationDataMap.empty())    //check empty
            {
                azimuthOriginal = rotationDataMap["azimuth"];
                pitchOriginal = rotationDataMap["pitch"];
                rollOriginal = rotationDataMap["roll"];
                DAVINCI_LOG_INFO << string("The original azimuth angle of target device is: ") << azimuthOriginal;
                DAVINCI_LOG_INFO << string("The original pitch angle of target device is: ") << pitchOriginal;
                DAVINCI_LOG_INFO << string("The original roll angle of target device is: ") << rollOriginal;
            }
            else
            {
                DAVINCI_LOG_WARNING << string("Rotation sensor data unavailable in the target device!");
            }
#pragma endregion
        }

        if ((angleMotorOne - 60 == 90) && (angleMotorTwo - 60 == 90))
        {
            DAVINCI_LOG_INFO << string("The original FFRD is in horizontal state, no need for horizontal calibration!");
            SetFinished();
            return true;
        }
        else
        {
            for (int i = 0; i < processCounter; i++)
            {
                DAVINCI_LOG_INFO << string("*************************************************************");
                DAVINCI_LOG_INFO << string("No. ") << i << string(" round of FFRD horizontal calibration!");
                cyclingTest(i);
                DAVINCI_LOG_INFO << string("*************************************************************");
            }
        }

        StartGroup();

        return true;
    }

    //Ready
    cv::Mat FfrdHorizontalCalibration::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        DAVINCI_LOG_ERROR << string("*************************************************************");
        DAVINCI_LOG_ERROR << string("FFRD Horizontal Calibration Test Result:");
        DAVINCI_LOG_ERROR << string("After ") << processCounter << string(" times of FFRD horizontal calibration:");
        curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if (curHWController == nullptr)
        {
            DAVINCI_LOG_ERROR << string("No FFRD Controller connected, please configure it.");
            SetFinished();
            return frame;
        }
        angleMotorOne = curHWController->GetMotorAngle(1);
        angleMotorTwo = curHWController->GetMotorAngle(2);
        DAVINCI_LOG_ERROR << string("The final angle of servo motor No.1 is: ") << (angleMotorOne - 60);
        DAVINCI_LOG_ERROR << string("The final angle of servo motor No.2 is: ") << (angleMotorTwo - 60);

#pragma region Get rotation data from Qagent
        rotationDataMap = androidTargetDevice->GetRotationDataFromQAgent();
        if(!rotationDataMap.empty())    //check empty
        {
            azimuthTemp = rotationDataMap["azimuth"];
            pitchTemp = rotationDataMap["pitch"];
            rollTemp = rotationDataMap["roll"];
            DAVINCI_LOG_INFO << string("The present azimuth angle of target device is: ") << azimuthTemp;
            DAVINCI_LOG_INFO << string("The present pitch angle of target device is: ") << pitchTemp;
            DAVINCI_LOG_INFO << string("The present roll angle of target device is: ") << rollTemp;
            azimuthChange = azimuthTemp - azimuthOriginal;
            pitchChange = pitchTemp - pitchOriginal;
            rollChange = rollTemp - rollOriginal;
            DAVINCI_LOG_INFO << string("The azimuth angle change of target device is: ") << azimuthChange;
            DAVINCI_LOG_INFO << string("The pitch angle change of target device is: ") << pitchChange;
            DAVINCI_LOG_INFO << string("The roll angle change of target device is: ") << rollChange;
        }
        else
        {
            DAVINCI_LOG_WARNING << string("Rotation sensor data unavailable in the target device!");
        }
#pragma endregion

        DAVINCI_LOG_ERROR << string("FFRD Tilting Performance Test is finished! Exit!");
        DAVINCI_LOG_ERROR << string("*************************************************************");
        SetFinished();
        return frame;
    }

    //Ready
    void FfrdHorizontalCalibration::Destroy()
    {
        DAVINCI_LOG_INFO << string("FFRD Horizontal Calibration Finish.");
    }

    //Ready
    CaptureDevice::Preset FfrdHorizontalCalibration::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }

    void FfrdHorizontalCalibration::cyclingTest(int &countFlag)
    {
        if ((angleMotorOne - 60 == 90) && (angleMotorTwo - 60 == 90))
        {
            DAVINCI_LOG_INFO << string("*************************************************************");
            DAVINCI_LOG_INFO << string("FFRD horizontal calibration test result:");
            DAVINCI_LOG_INFO << string("The FFRD is in horizontal state, no need for more horizontal calibration!");
            DAVINCI_LOG_INFO << string("The final angle of servo motor No.1 is: 90");
            DAVINCI_LOG_INFO << string("The final angle of servo motor No.2 is: 90");

#pragma region Get rotation data from Qagent
            rotationDataMap = androidTargetDevice->GetRotationDataFromQAgent();
            if(!rotationDataMap.empty())    //check empty
            {
                azimuthTemp = rotationDataMap["azimuth"];
                pitchTemp = rotationDataMap["pitch"];
                rollTemp = rotationDataMap["roll"];
                DAVINCI_LOG_INFO << string("The present azimuth angle of target device is: ") << azimuthTemp;
                DAVINCI_LOG_INFO << string("The present pitch angle of target device is: ") << pitchTemp;
                DAVINCI_LOG_INFO << string("The present roll angle of target device is: ") << rollTemp;
                azimuthChange = azimuthTemp - azimuthOriginal;
                pitchChange = pitchTemp - pitchOriginal;
                rollChange = rollTemp - rollOriginal;
                DAVINCI_LOG_INFO << string("The azimuth angle change of target device is: ") << azimuthChange;
                DAVINCI_LOG_INFO << string("The pitch angle change of target device is: ") << pitchChange;
                DAVINCI_LOG_INFO << string("The roll angle change of target device is: ") << rollChange;
            }
            else
            {
                DAVINCI_LOG_WARNING << string("Rotation sensor data unavailable in the target device!");
            }
#pragma endregion

            DAVINCI_LOG_INFO << string("FFRD horizontal calibration test is finished! Exit!");
            DAVINCI_LOG_INFO << string("*************************************************************");
            SetFinished();
        }
        if (angleMotorOne - 60 > 90)
        {
            if (countFlag < 10)
            {
                curHWController->TiltLeft(5);
            }
            else
            {
                curHWController->TiltLeft(1);
            }
            angleMotorOne = curHWController->GetMotorAngle(1);										//for testing!
            DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60); //for testing!
        }
        if (angleMotorOne - 60 < 90)
        {
            if (countFlag < 10)
            {
                curHWController->TiltRight(5);
            }
            else
            {
                curHWController->TiltRight(1);
            }
            angleMotorOne = curHWController->GetMotorAngle(1);										//for testing!
            DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60);	//for testing!
        }
        if (angleMotorTwo - 60 > 90)
        {
            if (countFlag < 10)
            {
                curHWController->TiltUp(5);
            }
            else
            {
                curHWController->TiltUp(1);
            }
            angleMotorTwo = curHWController->GetMotorAngle(2);										//for testing!
            DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60); //for testing!
        }
        if (angleMotorTwo - 60 < 90)
        {
            if (countFlag < 10)
            {
                curHWController->TiltDown(5);
            }
            else
            {
                curHWController->TiltDown(1);
            }
            angleMotorTwo = curHWController->GetMotorAngle(2);										//for testing!
            DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60); //for testing!
        }
    }
}
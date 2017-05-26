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

#include "AKazeObjectRecognize.hpp"
#include "DeviceManager.hpp"
#include "FeatureMatcher.hpp"
#include "HWAccessoryController.hpp"
#include "TestInterface.hpp"
#include "TestManager.hpp"
#include "FfrdCheckTest.hpp"

namespace DaVinci
{
    //Ready
    FfrdCheckTest::FfrdCheckTest()
    {

    }

    //Ready
    FfrdCheckTest::~FfrdCheckTest()
    {

    }

    //Ready except for Lotus image match
    bool FfrdCheckTest::Init()
    {
        SetName("FFRD_Check_Test");
        DAVINCI_LOG_INFO << string("*************************************************************");
        DAVINCI_LOG_INFO << string("Command line autotest to check FFRD!");
        DAVINCI_LOG_INFO << string("*************************************************************");
        
        //Return if no target device is connected!
        boost::shared_ptr<TargetDevice> curTargetDevice = (DeviceManager::Instance()).GetCurrentTargetDevice();
        if (curTargetDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "No target device connected. Quit test.";
            SetFinished();
            return false;
        }
        //Return if agent is not connected!
        if (curTargetDevice->IsAgentConnected() == false)
        {
            DAVINCI_LOG_ERROR << "Agent is not connected. Quit test.";
            SetFinished();
            return false;
        }
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
            DAVINCI_LOG_INFO << string("Cannot continue with this performance test. Quit test.");
            SetFinished();
            return false;
        }
        else
        {
            DAVINCI_LOG_INFO << string("The FFRD has new type of servo motors with feedback functions.");
            DAVINCI_LOG_INFO << string("Can continue with this performance test.");
        }
        
        //Check the resource lotus image exists or not
        string davinciHome = TestManager::Instance().GetDaVinciHome();
        if (davinciHome.empty() == true)
        {
            DAVINCI_LOG_ERROR << string("Error: Cannot get Davinci Home Directory, DaVinci Home Directory must NOT be NULL!") << endl;
            SetFinished();
            return false;
        }
        lotusImagePath = TestManager::Instance().GetDaVinciResourcePath("Resources/lotus.jpg");
        if (boost::filesystem::is_regular_file(lotusImagePath) == false)
        {
            DAVINCI_LOG_ERROR << string("Lotus image file doesn't exist: ") << lotusImagePath;
            SetFinished();
            return false;
        }
        lotusImage = imread(lotusImagePath);
        if (lotusImage.data == nullptr)
        {
            DAVINCI_LOG_ERROR << "Calibration stops as lotus image is corrupted!";
            SetFinished();
            return false;
        }

#pragma region Initialize all parameters 
        startTiltingTime = 0;
        stopTiltingTime = 0;

        firstStageOnceTime = 0;
        secondStageOnceTime = 0;
        thirdStageOnceTime = 0;
        fourthStageOnceTime = 0;
        fifthStageOnceTime = 0;
        sixthStageOnceTime = 0;
        seventhStageOnceTime = 0;
        eighthStageOnceTime = 0;
        ninthStageOnceTime = 0;
        tenthStageOnceTime = 0;
        eleventhStageOnceTime = 0;

        firstStageTime = 0;
        secondStageTime = 0;
        thirdStageTime = 0;
        fourthStageTime = 0;
        fifthStageTime = 0;
        sixthStageTime = 0;
        seventhStageTime = 0;
        eighthStageTime = 0;
        ninthStageTime = 0;
        tenthStageTime = 0;
        eleventhStageTime = 0;

        processCounter = 0;
        stage = 0;

        firstStageNum = 10;
        secondStageNum = 10;
        thirdStageNum = 10;
        fourthStageNum = 10;
        fifthStageNum = 10;
        sixthStageNum = 10;
        seventhStageNum = 10;
        eighthStageNum = 10;
        ninthStageNum = 10;
        tenthStageNum = 10;
        eleventhStageNum = 10;

        firstStagePass = 0;
        secondStagePass = 0;
        thirdStagePass = 0;
        fourthStagePass = 0;
        fifthStagePass = 0;
        sixthStagePass = 0;
        seventhStagePass = 0;
        eighthStagePass = 0;
        ninthStagePass = 0;
        tenthStagePass = 0;
        eleventhStagePass = 0;

        speedMotorOne = 0;
        speedMotorTwo = 0;
        angleMotorOne = 0;
        angleMotorTwo = 0;
#pragma endregion

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
        }
        //TestManager::InitCamera(TestManager::HIGH_RES);

        //Show lotus image
        if (curTargetDevice->ShowLotus() == DaVinciStatusSuccess)
        {
            DAVINCI_LOG_INFO << "Show Lotus image!";
        }
        else
        {
            DAVINCI_LOG_ERROR << "FFRD Check Test stops as Lotus image cannot be shown!";
            SetFinished();
            return false;
        }

        StartGroup();
        return true;
    }

    //Ready
    cv::Mat FfrdCheckTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        if (stage < firstStageNum)
        {
            cyclingTest(stage, processCounter, "First", 5, firstStageOnceTime, firstStageTime, firstStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum)
        {
            cyclingTest(stage, processCounter, "Second", 10, secondStageOnceTime, secondStageTime, secondStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum)
        {
            cyclingTest(stage, processCounter, "Third", 15, thirdStageOnceTime, thirdStageTime, thirdStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum)
        {
            cyclingTest(stage, processCounter, "Fourth", 20, fourthStageOnceTime, fourthStageTime, fourthStagePass, frame);
        }

        else if (stage == firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum)
        {
            cyclingTest(stage, processCounter, "Fifth", 30, fifthStageOnceTime, fifthStageTime, fifthStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum)
        {
            cyclingTest(stage, processCounter, "Sixth", 40, sixthStageOnceTime, sixthStageTime, sixthStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum)
        {
            cyclingTest(stage, processCounter, "Seventh", 50, seventhStageOnceTime, seventhStageTime, seventhStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum + eighthStageNum)
        {
            cyclingTest(stage, processCounter, "Eighth", 60, eighthStageOnceTime, eighthStageTime, eighthStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum + eighthStageNum + ninthStageNum)
        {
            cyclingTest(stage, processCounter, "Ninth", 70, ninthStageOnceTime, ninthStageTime, ninthStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum + eighthStageNum + ninthStageNum + tenthStageNum)
        {
            cyclingTest(stage, processCounter, "Tenth", 80, tenthStageOnceTime, tenthStageTime, tenthStagePass, frame);
        }

        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum + eighthStageNum + ninthStageNum + tenthStageNum + eleventhStageNum)
        {
            cyclingTest(stage, processCounter, "Eleventh", 90, eleventhStageOnceTime, eleventhStageTime, eleventhStagePass, frame);
        }

        else if (stage == firstStageNum + secondStageNum + thirdStageNum + fourthStageNum + fifthStageNum + sixthStageNum + seventhStageNum + eighthStageNum + ninthStageNum + tenthStageNum + eleventhStageNum)
        {
            DAVINCI_LOG_INFO << string("*************************************************************") << endl;
            DAVINCI_LOG_INFO << string("FFRD Tilting Performance Test Results:") << endl;

            DAVINCI_LOG_INFO << string("First Type tilting cycling test - All: ") << firstStageNum << string(", Passed: ") << firstStagePass << string(", Failed: ") << (firstStageNum - firstStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 5-degrees-tilting (8 times * ") << firstStageNum << string(") : ") << firstStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 5-degrees-tilting: ") << (firstStageTime / (8 * firstStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Second Type tilting cycling test - All: ") << secondStageNum << string(", Passed: ") << secondStagePass << string(", Failed: ") << (secondStageNum - secondStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 10-degrees-tilting (8 times * ") << secondStageNum << string(") : ") << secondStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 10-degrees-tilting: ") << (secondStageTime / (8 * secondStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Third Type tilting cycling test - All: ") << thirdStageNum << string(", Passed: ") << thirdStagePass << string(", Failed: ") << (thirdStageNum - thirdStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 15-degrees-tilting (8 times * ") << thirdStageNum << string(") : ") << thirdStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 15-degrees-tilting: ") << (thirdStageTime / (8 * thirdStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Fourth Type tilting cycling test - All: ") << fourthStageNum << string(", Passed: ") << fourthStagePass << string(", Failed: ") << (fourthStageNum - fourthStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 20-degrees-tilting (8 times * ") << fourthStageNum << string(") : ") << fourthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 20-degrees-tilting: ") << (fourthStageTime / (8 * fourthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Fifth Type tilting cycling test - All: ") << fifthStageNum << string(", Passed: ") << fifthStagePass << string(", Failed: ") << (fifthStageNum - fifthStagePass) << std::endl;
            DAVINCI_LOG_INFO << string("Total time duration of 30-degrees-tilting (8 times * ") << fifthStageNum << string(") : ") << fifthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 30-degrees-tilting: ") << (fifthStageTime / (8 * fifthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Sixth Type tilting cycling test - All: ") << sixthStageNum << string(", Passed: ") << sixthStagePass << string(", Failed: ") << (sixthStageNum - sixthStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 40-degrees-tilting (8 times * ") << sixthStageNum << string(") : ") << sixthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 40-degrees-tilting: ") << (sixthStageTime / (8 * sixthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Seventh Type tilting cycling test - All: ") << seventhStageNum << string(", Passed: ") << seventhStagePass << string(", Failed: ") << (seventhStageNum - seventhStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 50-degrees-tilting (8 times * ") << seventhStageNum << string(") : ") << seventhStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 50-degrees-tilting: ") << (seventhStageTime / (8 * seventhStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Eighth Type tilting cycling test - All: ") << eighthStageNum << string(", Passed: ") << eighthStagePass << string(", Failed: ") << (eighthStageNum - eighthStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 60-degrees-tilting (8 times * ") << eighthStageNum << string(") : ") << eighthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 60-degrees-tilting: ") << (eighthStageTime / (8 * eighthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Ninth Type tilting cycling test - All: ") << ninthStageNum << string(", Passed: ") << ninthStagePass << string(", Failed: ") << (ninthStageNum - ninthStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 70-degrees-tilting (8 times * ") << ninthStageNum << string(") : ") << ninthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 70-degrees-tilting: ") << (ninthStageTime / (8 * ninthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Tenth Type tilting cycling test - All: ") << tenthStageNum << string(", Passed: ") << tenthStagePass << string(", Failed: ") << (tenthStageNum - tenthStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 80-degrees-tilting (8 times * ") << tenthStageNum << string(") : ") << tenthStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 80-degrees-tilting: ") << (tenthStageTime / (8 * tenthStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("Eleventh Type tilting cycling test - All: ") << eleventhStageNum << string(", Passed: ") << eleventhStagePass << string(", Failed: ") << (eleventhStageNum - eleventhStagePass) << endl;
            DAVINCI_LOG_INFO << string("Total time duration of 90-degrees-tilting (8 times * ") << eleventhStageNum << string(") : ") << eleventhStageTime << string(" milliseconds.") << endl;
            DAVINCI_LOG_INFO << string("Average time duration of 90-degrees-tilting: ") << (eleventhStageTime / (8 * eleventhStageNum)) << string(" milliseconds.") << endl;

            DAVINCI_LOG_INFO << string("FFRD Tilting Performance Test is finished! Exit!") << endl;
            DAVINCI_LOG_INFO << string("*************************************************************") << endl;
            SetFinished();
        }

        return frame;
    }

    /*
    //Ready
    void FfrdCheckTest::Destroy()
    {
        
    }
    */

    //Ready
    CaptureDevice::Preset FfrdCheckTest::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }

    void FfrdCheckTest::cyclingTest(int &stageFlag, int &countFlag, const string &stageName, int degree, double &onceTimeDuration, double &stageTimeDuration, int &stagePass, const cv::Mat &frame)
    {
        countFlag++;

        switch (countFlag)
        {
        case 1:
            {
                DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
                DAVINCI_LOG_INFO << stageName << string(" Type cycling test, Round:") << stage << endl;
                break;
            }
        case 20:
            {
                DAVINCI_LOG_INFO << string("Tilting Up ") << degree << string(" degrees") << endl;
                curHWController->TiltUp(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 40:
            {
                DAVINCI_LOG_INFO << string("Tilting Down ") << degree << string(" degrees") << endl;
                curHWController->TiltDown(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 60:
            {
                DAVINCI_LOG_INFO << string("Tilting Down ") << degree << string(" degrees") << endl;
                curHWController->TiltDown(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 80:
            {
                DAVINCI_LOG_INFO << string("Tilting Up ") << degree << string(" degrees") << endl;
                curHWController->TiltUp(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 100:
            {
                DAVINCI_LOG_INFO << string("Tilting Left ") << degree << string(" degrees") << endl;
                curHWController->TiltLeft(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 120:
            {
                DAVINCI_LOG_INFO << string("Tilting Right ") << degree << string(" degrees") << endl;
                curHWController->TiltRight(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 140:
            {
                DAVINCI_LOG_INFO << string("Tilting Right ") << degree << string(" degrees") << endl;
                curHWController->TiltRight(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 160:
            {
                DAVINCI_LOG_INFO << string("Tilting Left ") << degree << string(" degrees") << endl;
                curHWController->TiltLeft(degree);
                startTiltingTime = GetCurrentMillisecond();
                speedMotorOne = curHWController->GetMotorSpeed(1);
                speedMotorTwo = curHWController->GetMotorSpeed(2);
                while (speedMotorOne != 0 || speedMotorTwo != 0)
                {
                    speedMotorOne = curHWController->GetMotorSpeed(1);
                    speedMotorTwo = curHWController->GetMotorSpeed(2);
                }
                if (speedMotorOne == 0 && speedMotorTwo == 0)
                {
                    stopTiltingTime = GetCurrentMillisecond();
                    DAVINCI_LOG_INFO << string("Time duration of servo motor is: ") << (stopTiltingTime - startTiltingTime) << string(" milliseconds.") << endl;
                    onceTimeDuration = onceTimeDuration + stopTiltingTime - startTiltingTime;

                    angleMotorOne = curHWController->GetMotorAngle(1);
                    angleMotorTwo = curHWController->GetMotorAngle(2);
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.1 is: ") << (angleMotorOne - 60) << endl;
                    DAVINCI_LOG_INFO << string("The angle of servo motor No.2 is: ") << (angleMotorTwo - 60) << endl;
                }
                break;
            }
        case 200:
            {
                //AKaze for image match
                boost::shared_ptr<AKazeObjectRecognize> pObjRec(new AKazeObjectRecognize());
                FeatureMatcher featMatch(pObjRec) ;
                double unMatchRatio = 0;
                double* pUnmatchRatio = &unMatchRatio;
                Mat H;
                Mat refImage = lotusImage;
                Mat observed = frame;
                featMatch.SetModelImage(refImage, true);
                H = featMatch.Match(observed, pUnmatchRatio);

                if (!H.empty() && *pUnmatchRatio <= 0.45)
                {
                    DAVINCI_LOG_INFO << string("The lotus image is matched! The Servo Motors are working properly!") << endl;
                    stagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Servo Motors are not working properly!") << endl;
                }
                DAVINCI_LOG_INFO << string("Total time duration of ") << degree << string("-degrees-tilting (8 times) : ") << firstStageOnceTime << string(" milliseconds.") << endl;

                stageTimeDuration = stageTimeDuration + onceTimeDuration;
                curHWController->TiltTo(90, 90);
                break;
            }
        case 220:
            {
                countFlag = 0;
                onceTimeDuration = 0;
                stageFlag++;
                break;
            }
        default:
                break;
        }
    }
}
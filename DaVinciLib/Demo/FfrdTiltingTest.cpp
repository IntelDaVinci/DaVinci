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
#include "FfrdTiltingTest.hpp"

namespace DaVinci
{
    //Ready
    FfrdTiltingTest::FfrdTiltingTest()
    {
        InitParameters();
    }

    //Ready
    FfrdTiltingTest::~FfrdTiltingTest()
    {

    }

    void FfrdTiltingTest::InitParameters()
    {
        processCounter = 0;
        stage = 0;
        firstStageNum = 10;        
        secondStageNum = 10;
        thirdStageNum = 10;
        fourthStageNum = 10;
        firstStagePass = 0;
        secondStagePass = 0;
        thirdStagePass = 0;
        fourthStagePass = 0;
    }

    //Ready
    bool FfrdTiltingTest::Init()
    {
        SetName("FFRD_Tilting_Test");
        DAVINCI_LOG_INFO << string("*************************************************************");
        DAVINCI_LOG_INFO << string("Autotest FFRD Tilting!");
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
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if (curHWController == nullptr)
        {
            DAVINCI_LOG_ERROR << string("No FFRD connected. Quit test.");
            SetFinished();
            return false;
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

        InitParameters();

        TestManager::Instance().TiltCenter();
        //TestManager::InitCamera(TestManager::HIGH_RES);

        //Show lotus image
        if (curTargetDevice->ShowLotus() == DaVinciStatusSuccess)
        {
            DAVINCI_LOG_INFO << "Show Lotus image!";
        }
        else
        {
            DAVINCI_LOG_ERROR << "FFRD Tilting Test stops as Lotus image cannot be shown!";
            SetFinished();
            return false;
        }

        StartGroup();
        return true;
    }

    void FfrdTiltingTest::FirstTypeCyclingTest(const cv::Mat &frame)
    {
#pragma region First type cycling test, 5 degrees.
        switch (processCounter)
        {
        case 1:
            {
                DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
                DAVINCI_LOG_INFO << string("First type cycling test, Round:") << stage << endl;                    
                break;
            }
        case 20:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 5 degrees") << endl;
                TestManager::Instance().TiltUp(5);
                break;
            }
        case 40:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 5 degrees") << endl;
                TestManager::Instance().TiltDown(5);
                break;
            }
        case 60:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 5 degrees") << endl;
                TestManager::Instance().TiltDown(5);
                break;
            }
        case 80:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 5 degrees") << endl;
                TestManager::Instance().TiltUp(5);
                break;
            }
        case 100:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 5 degrees") << endl;
                TestManager::Instance().TiltLeft(5);
                break;
            }
        case 120:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 5 degrees") << endl;
                TestManager::Instance().TiltRight(5);
                break;
            }
        case 140:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 5 degrees") << endl;
                TestManager::Instance().TiltRight(5);
                break;
            }
        case 160:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 5 degrees") << endl;
                TestManager::Instance().TiltLeft(5);
                break;
            }
        case 200:
            {
                //AKaze for image match
                boost::shared_ptr<AKazeObjectRecognize>  pObjRec (new AKazeObjectRecognize());
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
                    firstStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Servo Motors are not working properly!") << endl;
                }
                TestManager::Instance().TiltTo(90, 90);
                break;
            }
        case 220:
            {
                processCounter = 0;
                stage++;
                break;
            }
        default:
            break;
        }
#pragma endregion
    }

    void FfrdTiltingTest::SecondTypeCyclingTest(const cv::Mat &frame)
    {
#pragma region Second type cycling test, 10 degrees.
        switch (processCounter)
        {
        case 1:
            {
                DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
                DAVINCI_LOG_INFO << string("Second type cycling test, Round:") << stage << endl;
                break;
            }
        case 20:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 10 degrees") << endl;
                TestManager::Instance().TiltUp(10);
                break;
            }
        case 40:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 10 degrees") << endl;
                TestManager::Instance().TiltDown(10);
                break;
            }
        case 60:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 10 degrees") << endl;
                TestManager::Instance().TiltDown(10);
                break;
            }
        case 80:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 10 degrees") << endl;
                TestManager::Instance().TiltUp(10);
                break;
            }
        case 100:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 10 degrees") << endl;
                TestManager::Instance().TiltLeft(10);
                break;
            }
        case 120:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 10 degrees") << endl;
                TestManager::Instance().TiltRight(10);
                break;
            }
        case 140:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 10 degrees") << endl;
                TestManager::Instance().TiltRight(10);
                break;
            }
        case 160:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 10 degrees") << endl;
                TestManager::Instance().TiltLeft(10);
                break;
            }
        case 200:
            {
                //AKaze for image match
                boost::shared_ptr<AKazeObjectRecognize>  pObjRec (new AKazeObjectRecognize());
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
                    secondStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Servo Motors are not working properly!") << endl;
                }                    
                TestManager::Instance().TiltTo(90, 90);
                break;
            }
        case 220:
            {
                processCounter = 0;
                stage++;
                break;
            }
        default:
            break;
        }
#pragma endregion
    }

    void FfrdTiltingTest::ThirdTypeCyclingTest(const cv::Mat &frame)
    {
#pragma region Third type cycling test, 15 degrees.
        switch (processCounter)
        {
        case 1:
            {
                DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
                DAVINCI_LOG_INFO << string("Third type cycling test, Round:") << stage << endl;
                break;
            }
        case 20:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 15 degrees") << endl;
                TestManager::Instance().TiltUp(15);
                break;
            }
        case 40:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 15 degrees") << endl;
                TestManager::Instance().TiltDown(15);
                break;
            }
        case 60:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 15 degrees") << endl;
                TestManager::Instance().TiltDown(15);
                break;
            }
        case 80:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 15 degrees") << endl;
                TestManager::Instance().TiltUp(15);
                break;
            }
        case 100:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 15 degrees") << endl;
                TestManager::Instance().TiltLeft(15);
                break;
            }
        case 120:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 15 degrees") << endl;
                TestManager::Instance().TiltRight(15);
                break;
            }
        case 140:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 15 degrees") << endl;
                TestManager::Instance().TiltRight(15);
                break;
            }
        case 160:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 15 degrees") << endl;
                TestManager::Instance().TiltLeft(15);
                break;
            }
        case 200:
            {
                //AKaze for image match
                boost::shared_ptr<AKazeObjectRecognize>  pObjRec (new AKazeObjectRecognize());
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
                    thirdStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Servo Motors are not working properly!") << endl;
                }                    
                TestManager::Instance().TiltTo(90, 90);
                break;
            }
        case 220:
            {
                processCounter = 0;
                stage++;
                break;
            }
        default:
            break;
        }
#pragma endregion
    }

    void FfrdTiltingTest::FourthTypeCyclingTest(const cv::Mat &frame)
    {
#pragma region Fourth type cycling test, 5 + 5 + 5 degrees.
        switch (processCounter)
        {
        case 1:
            {
                DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
                DAVINCI_LOG_INFO << string("Fourth type cycling test, Round:") << stage << endl;
                break;
            }
        case 20:
        case 40:
        case 60:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 5 degrees") << endl;
                TestManager::Instance().TiltUp(5);
                break;
            }
        case 80:
        case 100:
        case 120:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 5 degrees") << endl;
                TestManager::Instance().TiltDown(5);
                break;
            }
        case 140:
        case 160:
        case 180:
            {
                DAVINCI_LOG_INFO << string("Tilting Down 5 degrees") << endl;
                TestManager::Instance().TiltDown(5);
                break;
            }
        case 200:
        case 220:
        case 240:
            {
                DAVINCI_LOG_INFO << string("Tilting Up 5 degrees") << endl;
                TestManager::Instance().TiltUp(5);
                break;
            }
        case 260:
        case 280:
        case 300:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 5 degrees") << endl;
                TestManager::Instance().TiltLeft(5);
                break;
            }
        case 320:
        case 340:
        case 360:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 5 degrees") << endl;
                TestManager::Instance().TiltRight(5);
                break;
            }
        case 380:
        case 400:
        case 420:
            {
                DAVINCI_LOG_INFO << string("Tilting Right 5 degrees") << endl;
                TestManager::Instance().TiltRight(5);
                break;
            }
        case 440:
        case 460:
        case 480:
            {
                DAVINCI_LOG_INFO << string("Tilting Left 5 degrees") << endl;
                TestManager::Instance().TiltLeft(5);
                break;
            }
        case 520:
            {
                //AKaze for image match
                boost::shared_ptr<AKazeObjectRecognize>  pObjRec (new AKazeObjectRecognize());
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
                    fourthStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Servo Motors are not working properly!") << endl;
                }                    
                TestManager::Instance().TiltTo(90, 90);
                break;
            }
        case 540:
            {
                processCounter = 0;
                stage++;
                break;
            }
        default:
            break;
        }
#pragma endregion
    }

    //Ready
    cv::Mat FfrdTiltingTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        processCounter++;

#pragma region First type cycling test, 5 degrees.
        if (stage < firstStageNum)
        {
            FirstTypeCyclingTest(frame);
        }
#pragma endregion

#pragma region Second type cycling test, 10 degrees.
        else if (stage < firstStageNum + secondStageNum)
        {
            SecondTypeCyclingTest(frame);
        }
#pragma endregion

#pragma region Third type cycling test, 15 degrees.
        else if (stage < firstStageNum + secondStageNum + thirdStageNum)
        {
            ThirdTypeCyclingTest(frame);
        }
#pragma endregion

#pragma region Fourth type cycling test, 5 + 5 + 5 degrees.
        else if (stage < firstStageNum + secondStageNum + thirdStageNum + fourthStageNum)
        {
            FourthTypeCyclingTest(frame);
        }
#pragma endregion

#pragma region Summary.
        else if (stage == firstStageNum + secondStageNum + thirdStageNum + fourthStageNum)
        {
            DAVINCI_LOG_INFO << string("*************************************************************") << endl;
            DAVINCI_LOG_INFO << string("FFRD Tilting test Results:") << endl;
            DAVINCI_LOG_INFO << string("First type tilting cycling test - All: ") << firstStageNum << string(", Passed: ") << firstStagePass << string(", Failed: ") << (firstStageNum - firstStagePass) << endl;
            DAVINCI_LOG_INFO << string("Second type tilting cycling test - All: ") << secondStageNum << string(", Passed: ") << secondStagePass << string(", Failed: ") << (secondStageNum - secondStagePass) << endl;
            DAVINCI_LOG_INFO << string("Third type tilting cycling test - All: ") << thirdStageNum << string(", Passed: ") << thirdStagePass << string(", Failed: ") << (thirdStageNum - thirdStagePass) << endl;
            DAVINCI_LOG_INFO << string("Fourth type tilting cycling test - All: ") << fourthStageNum << string(", Passed: ") << fourthStagePass << string(", Failed: ") << (fourthStageNum - fourthStagePass) << endl;
            DAVINCI_LOG_INFO << string("FFRD Tilting test is finished! Exit!");
            DAVINCI_LOG_INFO << string("*************************************************************") << endl;
            SetFinished();
        }
#pragma endregion

        return frame;
    }

    /*
    //Ready
    void FfrdTiltingTest::Destroy()
    {
        
    }
    */

    //Ready
    CaptureDevice::Preset FfrdTiltingTest::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }

}
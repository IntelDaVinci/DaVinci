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
#include "FfrdLifterTest.hpp"

namespace DaVinci
{
	//Ready
	FfrdLifterTest::FfrdLifterTest()
	{
        InitParameters();
	}

	//Ready
	FfrdLifterTest::~FfrdLifterTest()
	{

	}

    void FfrdLifterTest::InitParameters()
    {
        processCounter = 0;
		stage = 0;
		firstStageNum = 10;
		secondStageNum = 10;
		firstStagePass = 0;
		secondStagePass = 0;
    }

	//Ready
	bool FfrdLifterTest::Init()
	{
		SetName("FFRD_Lifter_Test");
		DAVINCI_LOG_INFO << string("*************************************************************");
		DAVINCI_LOG_INFO << string("Autotest FFRD Lifter!");
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
		string baseDir = TestManager::Instance().GetDaVinciHome();
		if (baseDir == "")
		{
			DAVINCI_LOG_ERROR << string("Error: DaVinci Directory must NOT be NULL!") << endl;
			return false;
		}
		
		StartGroup();
		return true;
	}

    void FfrdLifterTest::FirstTypeCyclingTest(const cv::Mat &frame)
    {
        processCounter++;

		switch (processCounter)
		{
		case 1:
			{
				DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
				DAVINCI_LOG_INFO << string("First Type cycling test, Round:") << stage << endl;
				break;
			}
		case 20:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 1 cm") << endl;
				TestManager::Instance().OnMoveHolder(1);
				break;
			}
		case 40:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 1 cm") << endl;
				TestManager::Instance().OnMoveHolder(-1);
				break;
			}
		case 60:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 2 cm") << endl;
				TestManager::Instance().OnMoveHolder(2);
				break;
			}
		case 80:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 2 cm") << endl;
				TestManager::Instance().OnMoveHolder(-2);
				break;
			}
		case 100:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 3 cm") << endl;
				TestManager::Instance().OnMoveHolder(3);
				break;
			}
		case 120:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 3 cm") << endl;
				TestManager::Instance().OnMoveHolder(-3);
				break;
			}
		case 140:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 4 cm") << endl;
				TestManager::Instance().OnMoveHolder(4);
				break;
			}
		case 160:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 5 cm") << endl;
				TestManager::Instance().OnMoveHolder(-4);
				break;
			}
		case 180:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 4 cm") << endl;
				TestManager::Instance().OnMoveHolder(5);
				break;
			}
		case 200:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 5 cm") << endl;
				TestManager::Instance().OnMoveHolder(-5);
				break;
			}
		case 240:
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
                    DAVINCI_LOG_INFO << string("The lotus image is matched! The Lifter is working properly!") << endl;
                    firstStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Lifter is not working properly!") << endl;
                }
				break;
			}
		case 260:
			{
				processCounter = 0;
				stage++;
				break;
			}
		default:
			{
				break;
			}
        }
    }

    void FfrdLifterTest::SecondTypeCyclingTest(const cv::Mat &frame)
    {
        processCounter++;
		switch (processCounter)
		{
		case 1:
			{
				DAVINCI_LOG_INFO << string("-------------------------------------------------------------");
				DAVINCI_LOG_INFO << string("Second Type cycling test, Round:") << stage << endl;
				break;
			}
		case 20:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 1 cm") << endl;
				TestManager::Instance().OnMoveHolder(1);
				break;
			}
		case 40:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 2 cm") << endl;
				TestManager::Instance().OnMoveHolder(2);
				break;
			}
		case 60:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 3 cm") << endl;
				TestManager::Instance().OnMoveHolder(3);
				break;
			}
		case 80:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 4 cm") << endl;
				TestManager::Instance().OnMoveHolder(4);
				break;
			}
		case 100:
			{
				DAVINCI_LOG_INFO << string("Lifter Up 5 cm") << endl;
				TestManager::Instance().OnMoveHolder(5);
				break;
			}
		case 120:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 1 cm") << endl;
				TestManager::Instance().OnMoveHolder(-1);
				break;
			}
		case 140:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 2 cm") << endl;
				TestManager::Instance().OnMoveHolder(-2);
				break;
			}
		case 160:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 3 cm") << endl;
				TestManager::Instance().OnMoveHolder(-3);
				break;
			}
		case 180:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 4 cm") << endl;
				TestManager::Instance().OnMoveHolder(-4);
				break;
			}
		case 200:
			{
				DAVINCI_LOG_INFO << string("Lifter Down 5 cm") << endl;
				TestManager::Instance().OnMoveHolder(-5);
				break;
			}
		case 240:
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
                    DAVINCI_LOG_INFO << string("The lotus image is matched! The Lifter is working properly!") << endl;
					secondStagePass++;
                }
                else
                {
                    DAVINCI_LOG_INFO << string("The lotus image is not matched! The Lifter is not working properly!") << endl;
                }
				break;
			}
		case 260:
			{
				processCounter = 0;
				stage++;
				break;
			}
		default:
			{
				break;
			}
		}
    }

	//Ready except for Lotus image match
	cv::Mat FfrdLifterTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
	{
		if (stage < firstStageNum)
		{
            FirstTypeCyclingTest(frame);
		}

		else if (stage < firstStageNum + secondStageNum)
		{
			SecondTypeCyclingTest(frame);
		}

		else if (stage == firstStageNum + secondStageNum)
		{
			DAVINCI_LOG_INFO << string("*************************************************************") << endl;
			DAVINCI_LOG_INFO << string("FFRD Lifter test Results:") << endl;
			DAVINCI_LOG_INFO << string("First Type lifting cycling test - All: ") << firstStageNum << string(", Passed: ") << firstStagePass << string(", Failed: ") << (firstStageNum - firstStagePass) << endl;
			DAVINCI_LOG_INFO << string("Second Type lifting cycling test - All: ") << secondStageNum << string(", Passed: ") << secondStagePass << string(", Failed: ") << (secondStageNum - secondStagePass) << endl;
			DAVINCI_LOG_INFO << string("FFRD Lifter test is finished! Exit!");
			DAVINCI_LOG_INFO << string("*************************************************************") << endl;
			SetFinished();
		}

		return frame;
	}

    /*
	//Ready
	void FfrdLifterTest::Destroy()
	{

	}
    */

	//Ready
	CaptureDevice::Preset FfrdLifterTest::CameraPreset()
	{
		return CaptureDevice::Preset::PresetDontCare;
	}

}
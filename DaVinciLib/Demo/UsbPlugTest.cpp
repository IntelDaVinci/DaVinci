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
#include "UsbPlugTest.hpp"

namespace DaVinci
{
	//Ready
	UsbPlugTest::UsbPlugTest(int direction)
	{
		this->direction = direction;
	}

	//Ready
	UsbPlugTest::~UsbPlugTest()
	{

	}

	//Ready except for Lotus image match
	bool UsbPlugTest::Init()
	{
		SetName("USB_Unplug&Plug_Test");
		DAVINCI_LOG_INFO << string("*************************************************************");
		DAVINCI_LOG_INFO << string("Command line test of USB unplug & plug!");
		DAVINCI_LOG_INFO << string("*************************************************************");
        
        //Return if no FFRD is connected!
		boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
		if (curHWController == nullptr)
		{
			DAVINCI_LOG_ERROR << string("No FFRD connected. Quit test.");
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

		switch (direction)
		{
		case 0:
			{
				DAVINCI_LOG_INFO << string("Plug out USB devices!") << endl;            
				curHWController->UnplugUSB();
				DAVINCI_LOG_INFO << string("USB Unplug Test is finished! Exit!") << endl;
				break;
			}
		case 1:
			{
				DAVINCI_LOG_INFO << string("Plug in USB devices!") << endl;
				curHWController->PlugUSB();
				DAVINCI_LOG_INFO << string("USB Plug Test is finished! Exit!") << endl;
				break;
			}
		default:
			break;
		}
		SetFinished();

		return true;
	}

	//Ready
	cv::Mat UsbPlugTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
	{
		return frame;
	}

    /*
	//Ready
	void UsbPlugTest::Destroy()
	{

	}
    */

	//Ready
	CaptureDevice::Preset UsbPlugTest::CameraPreset()
	{
		return CaptureDevice::Preset::PresetDontCare;
	}

}
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

#ifndef __MAP_POWER_PUSHER_HPP__
#define __MAP_POWER_PUSHER_HPP__

#include <iostream>
#include <fstream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN     // define before #include <windows.h>
#include <windows.h>
#include "boost/filesystem.hpp"
#include "opencv2/opencv.hpp"
#include "AndroidTargetDevice.hpp"
#include "DeviceManager.hpp"
#include "HWAccessoryController.hpp"
#include "TestInterface.hpp"
#include "TestManager.hpp"

namespace DaVinci
{
	class MapPowerPusher : public TestInterface
	{
	public:
		string exe_direction;
		string LogFile;
		//StreamWriter sw;
		string DetailedLogFile;
		//StreamWriter detailed_sw;
		bool map_process_result;
	private:
		// these time parameters are measured by ms.
		//static const int short_press_time = 150;			// time period of short press
		static const int safe_interval = 1000;				// time period after short press to check the screen status
		static const int mapping_interval = 10000;			// interval between each mapping process
		static const int overall_results_interval = 15000;	// interval between mapping process and generating overall results
		int pusher_num_before;								//number of power pusher connected with the host PC before mapping
		int device_num_before;								//number of mobile device connected with the host PC before mapping
		int pusher_num_after;								//number of power pusher connected with the host PC after mapping
		int device_num_after;								//number of mobile device connected with the host PC after mapping
		vector<string> FFRD_before;
		vector<string> FFRD_after;
		vector<string> mobile_device_before;
		vector<string> mobile_device_after;        

	public:
		explicit MapPowerPusher();

		virtual ~MapPowerPusher();
        
        virtual void InitParameters();

		/// <summary>
		/// Initialize function of the mapping test.
		/// </summary>
		/// <returns></returns>
		virtual bool Init();

		/// <summary>
		/// Map Power Pusher function.
		/// </summary>
		/// <param name="COM_num"> COM port number </param>
		/// <returns></returns>
		virtual void MapPusher(int COM_num);

		/// <summary>
		/// Callback function that will be called when every frame processed
		/// </summary>
		/// <param name="frame"> Frame to be processed </param>
		/// <param name="timeStamp"> Timestamp when this frame captured </param>
		/// <returns></returns>
		virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

		/// <summary>
		/// Destroy function of the test case, just print out the exit information.
		/// </summary>
		virtual void Destroy();

		/// <summary>
		/// Return the camera preset this test case want.
		/// </summary>
		/// <returns>Return -1, doesn't care about the camera preset</returns>
		virtual CaptureDevice::Preset CameraPreset();
	private:
		/// <summary>
		/// Judge whether the file exist or not.
		/// </summary>
		/// <param name="fileName"> name of the file </param>
		/// <returns> if file exists, return 1; if file not exists, return 0 </returns>
		virtual int fileExists(const string &fileName);

		/// <summary>
		/// Get Screen Status
		/// </summary>
		/// <param name="deviceName"> name of the mobile device </param>
		/// <returns> screen status, 1 for screen-on, 0 for screen-off, -1 for no information when executing dumpsys display </returns>
		virtual int getScreenOnStatus(const string &deviceName);
	};
}

#endif
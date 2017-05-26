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

#ifndef __FFRD_HORIZONTAL_CALIBRATION_HPP__
#define __FFRD_HORIZONTAL_CALIBRATION_HPP__

#include <iostream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"

namespace DaVinci
{
	class FfrdHorizontalCalibration : public TestInterface
	{
    private:
		int major_version;		    //major version of firmware
        int agent_connection_flag;  //flag to show the target device is connected (1) or not (0)
        boost::shared_ptr<HWAccessoryController> curHWController;
        boost::shared_ptr<TargetDevice> targetDevice;
        boost::shared_ptr<AndroidTargetDevice> androidTargetDevice;
		int processCounter;			//counter used for controlling calibration times
		int speedMotorOne;	        //variable to store the speed of servo motor one
		int speedMotorTwo;	        //variable to store the speed of servo motor two
		int angleMotorOne;	        //variable to store the angle of servo motor one
		int angleMotorTwo;	        //variable to store the angle of servo motor two
        std::map <string, float> rotationDataMap;
        float azimuthOriginal;      //variable to store the original azimuth angle of android target device
        float pitchOriginal;        //variable to store the original pitch angle of android target device
        float rollOriginal;         //variable to store the original roll angle of android target device
        float azimuthTemp;          //variable to store the temporal azimuth angle of android target device
        float pitchTemp;            //variable to store the temporal pitch angle of android target device
        float rollTemp;             //variable to store the temporal roll angle of android target device
        float azimuthChange;        //variable to store the azimuth angle change of android target device
        float pitchChange;          //variable to store the pitch angle change of android target device
        float rollChange;           //variable to store the roll angle change of android target device
	
	public:
		/// <summary>
        /// Constructor
        /// </summary>
        /// <returns></returns>
		explicit FfrdHorizontalCalibration();

		virtual ~FfrdHorizontalCalibration();

		/// <summary>
        /// Initialize function of the horizontal calibration test. 
        /// </summary>
        /// <returns></returns>
        virtual bool Init();

		/// <summary>
        /// Callback function that will be called when every frame processed
        /// </summary>
        /// <param name="frame">Frame to be processed</param>
        /// <param name="timeStamp">Timestamp when this frame captured.</param>
        /// <returns></returns>
        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

        /// <summary>
        /// Destroy function of the test case.
        /// </summary>
        virtual void Destroy();

        /// <summary>
        /// Return the camera preset this test case want.
        /// </summary>
        /// <returns>Return -1, doesn't care about the camera preset</returns>
        virtual CaptureDevice::Preset CameraPreset();
    
    private:
		/// <summary>
		/// API function of cycling test
		/// </summary>
		/// <param name="countFlag">counter used for process controlling</param>
		/// <returns></returns>
        virtual void cyclingTest(int &countFlag);
	};
}

#endif
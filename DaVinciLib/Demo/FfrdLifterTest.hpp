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

#ifndef __FFRD_LIFTER_TEST_HPP__
#define __FFRD_LIFTER_TEST_HPP__

#include <iostream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"

namespace DaVinci
{
	class FfrdLifterTest : public TestInterface
	{
	private:
        string lotusImagePath;  //path name of Lotus image
        cv::Mat lotusImage;     //image object of Lotus image
		int processCounter;		//counter used for process controlling
		int stage;				//stage flag for process controlling
		int firstStageNum;	    //the number of first stage cycling test
		int secondStageNum;	    //the number of second stage cycling test
		int firstStagePass;	    //the number of first stage cycling test which is successfully passed
		int secondStagePass;	//the number of second stage cycling test which is successfully passed

	public:
		/// <summary>
        /// Construction function to init the test.
        /// </summary>
		explicit FfrdLifterTest();

		virtual ~FfrdLifterTest();

        virtual void InitParameters();

		/// <summary>
        /// Initialize function of the test case. 
        /// It will read the ini file and move the arms to default place (90,90), then start the test.
        /// </summary>
        /// <returns></returns>
		virtual bool Init();

        /// <summary>
		/// First type cycling test function.
		/// </summary>
		/// <param name="frame">Frame to be processed</param>
		/// <returns></returns>
		virtual void FirstTypeCyclingTest(const cv::Mat &frame);

        /// <summary>
		/// Second type cycling test function.
		/// </summary>
		/// <param name="frame">Frame to be processed</param>
		/// <returns></returns>
		virtual void SecondTypeCyclingTest(const cv::Mat &frame);

		/// <summary>
        /// Callback function that will be called when every frame processed
        /// </summary>
        /// <param name="frame">Frame to be processed</param>
        /// <param name="timeStamp">Timestamp when this frame captured.</param>
        /// <returns></returns>
		virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

        /*
		/// <summary>
        /// Destroy function of the test case, just to kill the application.
        /// </summary>
        virtual void Destroy();
        */

		/// <summary>
        /// Return the camera preset this test case want.
        /// </summary>
        /// <returns>Return -1, doesn't care about the camera preset</returns>
        virtual CaptureDevice::Preset CameraPreset();
	};
}

#endif
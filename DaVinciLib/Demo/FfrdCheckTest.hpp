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

#ifndef __FFRD_CHECK_TEST_HPP__
#define __FFRD_CHECK_TEST_HPP__

#include <iostream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"

namespace DaVinci
{
	class FfrdCheckTest : public TestInterface
	{
    private:
		int major_version;				//major version of firmware
		string lotusImagePath;          //path name of Lotus image		
		cv::Mat lotusImage;			    //image object of Lotus image

		double startTiltingTime;		//timestamp for start tilting
		double stopTiltingTime;			//timestamp for stop tilting

		double firstStageOnceTime;		//time duration of tilting once in first stage
		double secondStageOnceTime;		//time duration of tilting once in second stage
		double thirdStageOnceTime;		//time duration of tilting once in third stage
		double fourthStageOnceTime;		//time duration of tilting once in fourth stage
		double fifthStageOnceTime;		//time duration of tilting once in fifth stage
		double sixthStageOnceTime;		//time duration of tilting once in sixth stage
		double seventhStageOnceTime;	//time duration of tilting once in seventh stage
		double eighthStageOnceTime;		//time duration of tilting once in eighth stage
		double ninthStageOnceTime;		//time duration of tilting once in ninth stage
		double tenthStageOnceTime;		//time duration of tilting once in tenth stage
		double eleventhStageOnceTime;	//time duration of tilting once in eleventh stage

		double firstStageTime;			//total time duration of tilting in first stage
		double secondStageTime;			//total time duration of tilting in second stage
		double thirdStageTime;			//total time duration of tilting in third stage
		double fourthStageTime;			//total time duration of tilting in fourth stage
		double fifthStageTime;			//total time duration of tilting in fifth stage
		double sixthStageTime;			//total time duration of tilting in sixth stage
		double seventhStageTime;		//total time duration of tilting in seventh stage
		double eighthStageTime;			//total time duration of tilting in eighth stage
		double ninthStageTime;			//total time duration of tilting in ninth stage
		double tenthStageTime;			//total time duration of tilting in tenth stage
		double eleventhStageTime;		//total time duration of tilting in eleventh stage

		boost::shared_ptr<HWAccessoryController> curHWController;

		int processCounter;				//counter used for process controlling
		int stage;						//stage flag for process controlling
		int firstStageNum;			    //the number of first stage cycling test
		int secondStageNum;			    //the number of second stage cycling test
		int thirdStageNum;			    //the number of third stage cycling test
		int fourthStageNum;			    //the number of fourth stage cycling test
		int fifthStageNum;			    //the number of fifth stage cycling test
		int sixthStageNum;			    //the number of sixth stage cycling test
		int seventhStageNum;			//the number of seventh stage cycling test
		int eighthStageNum;			    //the number of eighth stage cycling test
		int ninthStageNum;			    //the number of ninth stage cycling test
		int tenthStageNum;			    //the number of tenth stage cycling test
		int eleventhStageNum;			//the number of eleventh stage cycling test

		int firstStagePass;			    //the number of first stage cycling test which is successfully passed
		int secondStagePass;			//the number of second stage cycling test which is successfully passed
		int thirdStagePass;			    //the number of third stage cycling test which is successfully passed
		int fourthStagePass;			//the number of fourth stage cycling test which is successfully passed
		int fifthStagePass;			    //the number of fifth stage cycling test which is successfully passed
		int sixthStagePass;			    //the number of sixth stage cycling test which is successfully passed
		int seventhStagePass;			//the number of seventh stage cycling test which is successfully passed
		int eighthStagePass;			//the number of eighth stage cycling test which is successfully passed
		int ninthStagePass;			    //the number of ninth stage cycling test which is successfully passed
		int tenthStagePass;			    //the number of tenth stage cycling test which is successfully passed
		int eleventhStagePass;		    //the number of eleventh stage cycling test which is successfully passed

		int speedMotorOne;			    //variable to store the speed of servo motor one
		int speedMotorTwo;			    //variable to store the speed of servo motor two
		int angleMotorOne;			    //variable to store the angle of servo motor one
		int angleMotorTwo;			    //variable to store the angle of servo motor two

	public:
		/// <summary>
		/// Construction function to init the test.
		/// </summary>
        explicit FfrdCheckTest();

		virtual ~FfrdCheckTest();

		/// <summary>
		/// Initialize function of the test case. 
		/// It will read the ini file and move the arms to default place (90,90), then start the test.
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

	private:
		/// <summary>
		/// API function of cycling test
		/// </summary>
		/// <param name="stageFlag">stage flag for process controlling</param>
		/// <param name="countFlag">counter used for process controlling</param>
        /// <param name="stageName">string name of this stage</param>
		/// <param name="degree">tilting degrees in this stage</param>
        /// <param name="onceTimeDuration">time duration of tilting once in this stage</param>
		/// <param name="stageTimeDuration">total time duration of tilting in this stage</param>
        /// <param name="stagePass">the number of this stage cycling test which is successfully passed</param>
        /// <param name="frame">Frame to be processed</param>
		/// <returns></returns>
        virtual void cyclingTest(int &stageFlag, int &countFlag, const string &stageName, int degree, double &onceTimeDuration, double &stageTimeDuration, int &stagePass, const cv::Mat &frame);
	};
}

#endif
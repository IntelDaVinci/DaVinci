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

#ifndef __LAUNCHTEST__HPP__
#define __LAUNCHTEST__HPP__

#include "TestInterface.hpp"
#include <string>
#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/
//using System.Windows.Controls;
//using System.Windows.Documents;

// Open source kit for integrating C++ OpenCV APIs into C#
// Warning: GPL license!


namespace DaVinci
{
    /// <summary>
    /// The test that start an application for lauch time test
    /// Note: measuring the launch time is not in this test, this test only start the application
    /// Measuring the launch time is in TimeMeasure class
    /// </summary>
    class LaunchTest : public TestInterface
    {
    private:
        std::string apk_class;
        std::string ini_filename;

        /// <summary>
        /// Create a launch test based on iniFile
        /// Format of ini file:
        /// first line: package name of the app in form of com.aaa.bbb
        /// second line: default activity which can send intent
        /// third line: the path of the launch image that is compared to
        /// forth line: the scale of the launch image, 300 is a good number, 200 makes matching faster, 400 makes matching more accurate
        /// fifth line: time out in unit of seconds, default number is 120
        /// </summary>
        /// <param name="iniFilename">the file name of ini file</param>
    public:
        LaunchTest(const std::string &iniFilename);

        /// <summary>
        /// Init the test and read parameters from ini file
        /// </summary>
        /// <returns>if init succeeded</returns>
        virtual bool Init() override;

        /// <summary>
        /// Process the frame, this is a dummy function
        /// </summary>
        /// <param name="frame">The frame captured</param>
        /// <param name="timeStamp">the time that the frame is captured</param>
        /// <returns>the processed frame</returns>
        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);        

        /// <summary>
        /// this test does not care about camera preset
        /// </summary>
        /// <returns>-1 means don't care</returns>
        virtual CaptureDevice::Preset CameraPreset() ;
    };
}

#endif	//#ifndef __LAUNCHTEST__

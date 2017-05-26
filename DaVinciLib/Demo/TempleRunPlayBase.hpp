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

#ifndef __TEMPLERUN_PLAY_BASE_HPP__
#define __TEMPLERUN_PLAY_BASE_HPP__

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
#include "DeviceManager.hpp"
#include "TestInterface.hpp"
#include "TestManager.hpp"
#include "ORBObjectRecognize.hpp"
#include "ObjectRecognize.hpp"

namespace DaVinci
{

    using namespace std;

    class TempleRunPlayBase : public TestInterface
    {
    public:
        TempleRunPlayBase();
        TempleRunPlayBase(string ini_filename);
        //~TempleRunPlayBase();

        /// <summary>
        /// class name of apk
        /// </summary>
        string package;
        /// <summary>
        /// application activity name to be tested
        /// </summary>
        string activity;
        /// <summary>
        /// file name of start image
        /// </summary>
        std::string start_fname;
        cv::Mat startImage;
        /// <summary>
        /// file name of resume image
        /// </summary>
        string resume_fname;
        cv::Mat resumeImage;
        /// <summary>
        /// file name of run again image
        /// </summary>
        string run_again_fname;
        cv::Mat replayImage;
        /// <summary>
        /// file name of image: next, back
        /// </summary>
        vector<string> image_fname;
        /// <summary>
        /// ORB object queue
        /// </summary>        
        vector<boost::shared_ptr<FeatureMatcher>> orbImageArray;
        /// <summary>
        /// number of image object in the queue and array of image file name
        /// </summary>
        int image_num;
        /// <summary>
        /// xml file for AI
        /// </summary>
        string resultxml;
        /// <summary>
        /// xml file for DL'S SVMS
        /// </summary>
        string DLSVMxml;
        /// <summary>
        /// ORB object for start image
        /// </summary>
        boost::shared_ptr<FeatureMatcher>  orbPlay;
        /// <summary>
        /// ORB object for resume image
        /// </summary>
        boost::shared_ptr<FeatureMatcher>  orbResume;
        /// <summary>
        /// ORB object for run again image
        /// </summary>
        boost::shared_ptr<FeatureMatcher> orbReplay;

        /// <summary>
        /// flag for playing state
        /// </summary>
        int played_stage;
        /// <summary>
        /// frame count between matching "start" image
        /// </summary>
        int played_frame_count;
        /// <summary>
        /// counter for saving start/run_again image, for debugging usage 
        /// </summary>
        int runCount;
        /// <summary>
        /// timestamp for replaying
        /// </summary>
        long replay_t;
        /// <summary>
        /// timestamp for jump action
        /// </summary>
        long jump_t;
        /// <summary>
        /// timestamp for down action
        /// </summary>
        long down_t;
        /// <summary>
        /// timestamp for turn action
        /// </summary>
        long turn_t;
        /// <summary>
        /// timestamp for tilt action
        /// </summary>
        long tilt_t;

        int check;

        int runCountSavingImages;

        /// <summary>
        /// constants for replay interval
        /// </summary>
        int replayInterval; // tick count


        int matchReplayCount;
        int maxMatchReplayCount;

        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

        cv::Mat getROI(const cv::Mat &image, double left, double right, double top, double bottom, int width, int height);

        virtual CaptureDevice::Preset CameraPreset();

        virtual bool Init();

        virtual void Destroy();

    private:
        boost::shared_ptr<TargetDevice> currentDevice;
    };
}

#endif

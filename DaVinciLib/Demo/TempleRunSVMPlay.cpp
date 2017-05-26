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

#include "TempleRunSVMPlay.hpp"
#include "TempleRunPlayBase.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "DaVinciCommon.hpp"
#include "TestManager.hpp"
#include "TestInterface.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace xercesc;
    using namespace boost::filesystem;
    using namespace boost::posix_time;

    TempleRunSVMPlay::TempleRunSVMPlay()
    {
        winWidth = 64;
        winHeight = 64;
        ROI_LEFT = 0.00;
        ROI_RIGHT = 1;
        ROI_TOP = 0.00;
        ROI_BOTTOM = 1;
        ROI_WIDTH = 64;
        ROI_HEIGHT = 64;

        ACTION_INTERVAL = 500;
        TILT_ACTION_INTERVAL = 380;
    }

    TempleRunSVMPlay::TempleRunSVMPlay(const string &iniFile) : TempleRunPlayBase(iniFile)
    {
        winWidth = 64;
        winHeight = 64;
        ROI_LEFT = 0.00;
        ROI_RIGHT = 1;
        ROI_TOP = 0.00;
        ROI_BOTTOM = 1;
        ROI_WIDTH = 64;
        ROI_HEIGHT = 64;

        ACTION_INTERVAL = 500;
        TILT_ACTION_INTERVAL = 380;   
    }

    TempleRunSVMPlay::~TempleRunSVMPlay()
    {
    }

    bool TempleRunSVMPlay::Init()
    {
        SetName("TempleRunSVMPlay");

        //TestManager::Instance().TiltCenter();

        if (false == TempleRunPlayBase::Init())
        {
            return false;
        }

        // the name should be aligned with machineFileName in TempleRunSVMTrain.cpp        
        boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
        resPath /= "examples\\templerun\\TempleRunSVM.xml";
        string userResultXML = resPath.string();

        if (exists(userResultXML))
        {
            resultxml = userResultXML;
            DAVINCI_LOG_INFO << string("AI XML trained by user: ") << resultxml;
        }
        else
        {
            DAVINCI_LOG_INFO << string("AI XML included in DaVinci: ") << resultxml;
        }

        try
        {
            svm.load(resultxml.c_str());
        }
        catch (Exception e)
        {
            DAVINCI_LOG_INFO << string("Error loading XML file for SVM: ") << e.msg;
            Destroy();
            return false;
        }

        /*
        HOGDescriptor(Size win_size=Size(64, 128), 
        Size block_size=Size(16, 16),
        Size block_stride=Size(8, 8), 
        Size cell_size=Size(8, 8),
        int nbins=9, 
        double win_sigma=DEFAULT_WIN_SIGMA,
        double threshold_L2hys=0.2, 
        bool gamma_correction=true,
        int nlevels=DEFAULT_NLEVELS);
        */

        hog = boost::shared_ptr<HOGDescriptor>(new HOGDescriptor(
            Size(winWidth, winHeight), 
            Size(16, 16), 
            Size(8, 8), 
            Size(8, 8), 
            9));


        map = Mat(2,3,CV_32FC1);
        map.at<float>(0,0) = (float)cos(CV_PI / 2);
        map.at<float>(0,1) = (float)sin(CV_PI / 2);
        map.at<float>(1,0) = -map.at<float>(0,1);
        map.at<float>(1,1) = map.at<float>(0,0);
        map.at<float>(0,2) = 30;
        map.at<float>(1,2) = 40;

        return true;
    }


    cv::Mat TempleRunSVMPlay::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        cv::Mat result = TempleRunPlayBase::ProcessFrame(frame, timeStamp);
        if (played_stage < 2)
            return result;

        long tick = GetTickCount();
        if (tick - replay_t > replayInterval)
        {
            //Notice the construction function width and height is different from the c++
            cv::Mat image = getROI(frame, ROI_LEFT, ROI_RIGHT, ROI_TOP, ROI_BOTTOM, ROI_WIDTH, ROI_HEIGHT);

            map.at<float>(0,2) = (float)(image.cols / 2);
            map.at<float>(1,2) = (float)(image.rows / 2);

            cv::Mat src = Mat(winHeight, winWidth, CV_32FC1); // must be 2^N
            IplImage* cvSrc = cvCreateImage(cvSize(winHeight, winWidth), 8, 3);
            IplImage cvImage = (IplImage)(image);
            CvMat cvMap = (CvMat)(map);
            cvGetQuadrangleSubPix( &cvImage, cvSrc, &cvMap);
            src = ((Mat)cvSrc).clone();
            cvReleaseImage(&cvSrc);

            vector<float> des;
            hog->compute(src, des);

            Mat matDes(1, (int)des.size(), CV_32FC1);
            for (size_t tempi = 0; tempi < des.size(); ++tempi)
            {
                matDes.at<float>(0, (int)tempi) = des[tempi];
            }

            float svmresult = svm.predict(matDes);

            TempleRunSVMPlay::ACTIONS action = TempleRunSVMPlay::ACTIONS::JUMP;
            action = ACTIONS((int)svmresult);
            if (action == TempleRunSVMPlay::ACTIONS::JUMP && boost::lexical_cast<long>(GetTickCount()) - jump_t > ACTION_INTERVAL)
            {
                ThreadSleep(20);
                TestManager::Instance().OnSwipe(SwipeActionUp);
                DAVINCI_LOG_INFO << string("JUMP");
                jump_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::DOWN && boost::lexical_cast<long>(GetTickCount()) - down_t > ACTION_INTERVAL)
            {
                ThreadSleep(30);
                TestManager::Instance().OnSwipe(SwipeActionDown);
                DAVINCI_LOG_INFO << string("DOWN");
                down_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::LEFT && boost::lexical_cast<long>(GetTickCount()) - turn_t > ACTION_INTERVAL)
            {
                ThreadSleep(65);
                TestManager::Instance().OnSwipe(SwipeActionLeft);
                DAVINCI_LOG_INFO << string("LEFT");
                turn_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::RIGHT && boost::lexical_cast<long>(GetTickCount()) - turn_t > ACTION_INTERVAL)
            {
                //ThreadSleep(15);
                TestManager::Instance().OnSwipe(SwipeActionRight);
                DAVINCI_LOG_INFO << string("RIGHT");
                turn_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::TILT_LEFT && boost::lexical_cast<long>(GetTickCount()) - tilt_t > TILT_ACTION_INTERVAL)
            {
                ThreadSleep(0);
                //TestManager::Instance().OnSwipe(
                //TestManager::Instance().TiltLeft(5);
                DAVINCI_LOG_INFO << string("TILT LEFT");
                tilt_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::TILT_RIGHT && boost::lexical_cast<long>(GetTickCount()) - tilt_t > TILT_ACTION_INTERVAL)
            {
                ThreadSleep(0);
                //TestManager::Instance().TiltRight(5);
                DAVINCI_LOG_INFO << string("TILT RIGHT");
                tilt_t = GetTickCount();
            }
            else if (action == TempleRunSVMPlay::ACTIONS::STRAIGHT)
            {
                //printf("STRAIGHT\n");
            }
        }


        //DAVINCI_LOG_DEBUG << "tick count = " << ((cv::getTickCount() - tick1) *1000.0 / cv::getTickFrequency());
        return frame;
    }

}
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

#include "PseudoCameraCapture.hpp"
#include "TestManager.hpp"

namespace DaVinci
{
    boost::mutex PseudoCameraCapture::initMutex;

    cv::Mat PseudoCameraCapture::logo;

    PseudoCameraCapture::PseudoCameraCapture()
    {
        boost::lock_guard<boost::mutex> lock(initMutex);
        if (logo.empty())
        {
            string logoFile = TestManager::Instance().GetDaVinciResourcePath("Resources/DaVinciLogo.jpg");
            logo = cv::imread(logoFile);
            // if we fail to read the image file
            if (logo.empty())
            {
                logo = Mat(720, 1280, CV_8UC3, Scalar(255,255,255));
            }
        }
    }

    PseudoCameraCapture::~PseudoCameraCapture()
    {

    }

    string PseudoCameraCapture::GetCameraName()
    {
        return "PseudoCam";
    }

    cv::Mat PseudoCameraCapture::RetrieveFrame(int *frameIndex)
    {
        frameRetrieveWatch.Restart();
        return logo;
    }

    bool PseudoCameraCapture::IsFrameAvailable()
    {
        if (frameRetrieveWatch.ElapsedMilliseconds() < frameInterval)
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(frameInterval / 5));
        }
        return true;
    }

    DaVinciStatus PseudoCameraCapture::Start()
    {
        frameRetrieveWatch.Start();
        return CaptureDevice::Start();
    }

    DaVinciStatus PseudoCameraCapture::Stop()
    {
        frameRetrieveWatch.Reset();
        return CaptureDevice::Stop();
    }

    DaVinciStatus PseudoCameraCapture::Restart()
    {
        frameRetrieveWatch.Restart();
        return DaVinciStatusSuccess;
    }

    bool PseudoCameraCapture::InitCamera(CaptureDevice::Preset preset)
    {
        return true;
    }


    double PseudoCameraCapture::GetCaptureProperty(int prop)
    {
        return 0.0;
    }

    void PseudoCameraCapture::SetCaptureProperty(int prop, double value, bool shouldCheck)
    {
    }
}
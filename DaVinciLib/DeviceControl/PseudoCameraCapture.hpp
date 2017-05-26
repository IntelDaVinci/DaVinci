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

#ifndef __PSEUDO_CAMERA_CAPTURE_HPP__
#define __PSEUDO_CAMERA_CAPTURE_HPP__

#include "CaptureDevice.hpp"
#include "StopWatch.hpp"

namespace DaVinci
{
    class PseudoCameraCapture : public CaptureDevice
    {
    public:
        PseudoCameraCapture();

        virtual ~PseudoCameraCapture();

        virtual string GetCameraName() override;

        virtual cv::Mat RetrieveFrame(int *frameIndex = NULL) override;

        virtual bool IsFrameAvailable() override;

        virtual DaVinciStatus Restart() override;

        virtual DaVinciStatus Start() override;

        virtual DaVinciStatus Stop() override;

        virtual bool InitCamera(CaptureDevice::Preset preset) override;

        virtual double GetCaptureProperty(int prop) override;

        virtual void SetCaptureProperty(int prop, double value, bool shouldCheck = false) override;

    private:
        static const int frameInterval = 20;
        static boost::mutex initMutex;
        static cv::Mat logo;

        StopWatch frameRetrieveWatch;
    };
}

#endif
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

#ifndef __HYPER_SOFT_CAM_CAPTURE_HPP__
#define __HYPER_SOFT_CAM_CAPTURE_HPP__

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <queue>

#include "boost/thread/mutex.hpp"
#include "boost/process.hpp"

#include "remote_screen.h"
#include "CaptureDevice.hpp"
#include "DaVinciDefs.hpp"
#include "AndroidTargetDevice.hpp"
#include "StopWatch.hpp"

namespace DaVinci
{
    class HyperSoftCamCapture : public CaptureDevice
    {
    public:
        static DaVinciStatus InstallAgent(const boost::shared_ptr<AndroidTargetDevice> &);

    public:
        HyperSoftCamCapture(const string &deviceName);

        virtual ~HyperSoftCamCapture()
        {
        }

        virtual DaVinciStatus Start() override;

        virtual DaVinciStatus Stop() override;

        virtual DaVinciStatus Restart() override;

        virtual cv::Mat RetrieveFrame(int *frameIndex = NULL) override;

        virtual bool IsFrameAvailable() override;

        virtual string GetCameraName() override;

        virtual void SetCaptureProperty(int prop, double value, bool shouldCheck);
        virtual double GetCaptureProperty(int prop);

        virtual bool InitCamera(CaptureDevice::Preset preset) override;

        void SetLocalPort(unsigned short port);
        static const unsigned short localPortBase = 23456;

        // the width and height should be aligned to 16-byte
        static const int highResWidth = 1280;
        static const int aiResWidth = 640;
        static const int highSpeedWidth = 320;

        static const int maxRestartCount = 5;

    private:
        static const unsigned short remotePort = 8888;

        static bool isHscModuleInitialized;
        static boost::mutex hscModuleInitMutex;

        // TODO: Now we assume only one instance is active globally
        // this will be removed after the camera-less source code is integrated into
        // DaVinci project. Then we will use function and bind for callback.
        static boost::shared_ptr<HyperSoftCamCapture> self;
        static void GeneralExceptionCallback(RS_STATUS code);
        static void CaptureFrameCallback(unsigned int width, unsigned int height);

        boost::mutex startStopMutex;
        string targetDeviceName;
        boost::shared_ptr<AndroidTargetDevice> targetDevice;
        unsigned short localPort;
        unsigned int maxFrameQueueCount;
        int devFrameIndex;
        boost::mutex frameQueueMutex;
        queue<std::pair<int, cv::Mat>> frameQueue;
        AutoResetEvent frameReadyEvent;
        boost::shared_ptr<boost::process::process> captureServer;
        string cameralessAgentBase;
        int frameWidth;
        int frameHeight;
        int deviceWidth;
        int deviceHeight;
        int fps;
        RS_STATUS hyperSoftCamStatus;

        boost::atomic<int> restartCount;

        void EnqueueFrame(const cv::Mat &frame);

        DaVinciStatus SetupCameralessAgent();

        void TeardownCameralessAgent();

    };
}

#endif
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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __DAVINCI_TESTINTERFACE_HPP__
#define __DAVINCI_TESTINTERFACE_HPP__

#include <queue>

#include "DaVinciCommon.hpp"
#include "GracefulThread.hpp"
#include "TestGroup.hpp"
#include "opencv2/opencv.hpp"
#include "DeviceControl/CaptureDevice.hpp"

#include <atomic>

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class TestGroup;

    class FrameBuffer
    {
    private:
        queue<Mat> frameQueue;
        queue<double> tsQueue;
        queue<int> frameIndexQueue;
        boost::mutex mtx;
        unsigned int maxFrameQueueCount;
        std::atomic<bool> isStop;
        AutoResetEvent readyEvent;

    public:
        FrameBuffer();
        virtual ~FrameBuffer();
        void Stop();
        bool Write(const Mat &theImage, const double stamp, int frameIndex);
        bool Read(Mat & theImage, double & stamp, int &frameIndex);
    };


    class TestInterface : public boost::enable_shared_from_this<TestInterface>
    {
    public:
        TestInterface();
        virtual ~TestInterface();

        virtual Mat ProcessFrame(const Mat &frame, const double timeStamp)
        {
            return frame;
        }

        void SetName(const string &_name);
        string GetName(void);

        void SetActive(bool isActive);

        bool IsActive(void);

        bool WriteInFrame(const Mat &frame, const double timeStamp, int frameIndex = -1);
        bool ReadInFrame(Mat & frame, double & timeStamp);
        bool WriteOutFrame(const Mat &frame, const double timeStamp, int frameIndex = -1);
        bool ReadOutFrame(Mat & frame, double & timeStamp);
        bool InitThread(void);

        void DestroyThread(void);

        bool AddTest(boost::shared_ptr<TestInterface> test, bool isInit = true);
        void WaitTestFinish(boost::shared_ptr<TestInterface> test);

        bool CallTest(boost::shared_ptr<TestInterface> childTest, bool childIsForeground = false, bool waitChildFinished = true);
        void SetTestAsForeground(boost::shared_ptr<TestInterface> test);
        bool StopTest(boost::shared_ptr<TestInterface> test);

        virtual void ProcessWave(const boost::shared_ptr<vector<short>> &samples)
        {
        }

        virtual bool Init(void)
        {
            // TODO
            return false;
        }

        virtual CaptureDevice::Preset CameraPreset(void)
        {
            // TODO
            return CaptureDevice::Preset::PresetDontCare;
        }

        virtual void Destroy(void)
        {
            // TODO
        }

        bool IsForeground(void);
        void SetForeground(bool fore);

        virtual void SetFinished(void);
        virtual bool IsFinished(void);

        virtual void Start(void);

        void SetGroup(const boost::weak_ptr<TestGroup> theGroup);

        void StartGroup();

        void SetParent(const boost::shared_ptr<TestInterface> test);
        boost::shared_ptr<TestInterface> GetParent();

    protected:
        int curFrameIndex;

    private:
        bool FrameThread();

        enum class TestState
        {
            TestStateInactive = -1,
            TestStateActive,
            TestStateFinish
        };

        std::atomic<TestState> state;
        string testName;
        boost::shared_ptr<FrameBuffer> inFrame;
        boost::shared_ptr<FrameBuffer> outFrame;
        boost::shared_ptr<GracefulThread> threadFrame;

        bool foreground;
        boost::weak_ptr<TestGroup> group;
        boost::weak_ptr<TestInterface> parentTest;
    };
}

#endif

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

#include "TestInterface.hpp"

#include <thread>
#include <chrono>

namespace DaVinci
{
    FrameBuffer::FrameBuffer()
    {
        maxFrameQueueCount = 1;
        isStop = false;
        readyEvent.Reset();
    }

    FrameBuffer::~FrameBuffer()
    {
        Stop();
    }

    void FrameBuffer::Stop()
    {
        if (isStop == false)
        {
            isStop = true;
            {
                boost::lock_guard<boost::mutex> lock(mtx);
                while (frameQueue.empty() == false)
                    frameQueue.pop();
                while (tsQueue.empty() == false)
                    tsQueue.pop();
            }
            readyEvent.Set();
        }
    }

    bool FrameBuffer::Write(const Mat &theImage, const double stamp, int frameIndex)
    {
        if (isStop)
            return false;

        {
            boost::lock_guard<boost::mutex> lock(mtx);
            if (frameQueue.size() == maxFrameQueueCount)
            {
                frameQueue.pop();
                tsQueue.pop();
                frameIndexQueue.pop();
            }
            frameQueue.push(theImage);
            tsQueue.push(stamp);
            frameIndexQueue.push(frameIndex);
        }

        readyEvent.Set();

        return true;
    }

    bool FrameBuffer::Read(Mat & theImage, double & stamp, int & frameIndex)
    {
        bool ret = true;

        if (isStop)
        {
            stamp = 0;
            return false;
        }

        readyEvent.WaitOne();

        boost::lock_guard<boost::mutex> lock(mtx);
        if (frameQueue.empty() == true)
        {
            // image.initEmpty();
            stamp = 0;
            frameIndex = -1;
            ret = false;
        }
        else
        {
            theImage = frameQueue.front();
            frameQueue.pop();
            stamp = tsQueue.front();
            tsQueue.pop();
            frameIndex = frameIndexQueue.front();
            frameIndexQueue.pop();
        }

        return ret;
    }


    TestInterface::TestInterface()
    {
        SetName("AnonymousTest");
        state = TestState::TestStateInactive;
        foreground = false;
        curFrameIndex = -1;
    }

    TestInterface::~TestInterface()
    {
        DestroyThread();
    }

    void TestInterface::SetName(const string &_name)
    {
        testName = _name;
    }

    string TestInterface::GetName()
    {
        return testName;
    }

    void TestInterface::SetActive(bool isActive)
    {
        if (isActive)
            state = TestState::TestStateActive;
        else
        {
            // outFrame->Stop();
            state = TestState::TestStateInactive;
        }
    }

    bool TestInterface::IsActive()
    {
        return (state == TestState::TestStateActive);
    }

    bool TestInterface::WriteInFrame(const Mat &frame, double timeStamp, int frameIndex)
    {
        return inFrame->Write(frame, timeStamp, frameIndex);
    }

    bool TestInterface::ReadInFrame(Mat & frame, double & timeStamp)
    {
        if (!inFrame->Read(frame, timeStamp, curFrameIndex))
        {
            return false;
        }
        return true;
    }

    bool TestInterface::WriteOutFrame(const Mat &frame, double timeStamp, int frameIndex)
    {
        return outFrame->Write(frame, timeStamp, frameIndex);
    }

    bool TestInterface::ReadOutFrame(Mat & frame, double & timeStamp)
    {
        int frameIndex = -1;
        if (!outFrame->Read(frame, timeStamp, frameIndex))
        {
            return false;
        }
        return true;
    }

    bool TestInterface::InitThread(void)
    {
        inFrame = boost::shared_ptr<FrameBuffer>(new FrameBuffer());
        outFrame = boost::shared_ptr<FrameBuffer>(new FrameBuffer());
        threadFrame = boost::shared_ptr<GracefulThread>(new GracefulThread(boost::bind(&TestInterface::FrameThread, this), true));
        if (testName.length() > 0)
            threadFrame->SetName(testName);
        threadFrame->Start();
        return true;
    }

    void TestInterface::DestroyThread()
    {
        if (threadFrame != nullptr)
        {
            threadFrame->Stop();
            if (inFrame != nullptr)
            {
                inFrame->Stop();
            }
            threadFrame->StopAndJoin();
            threadFrame = nullptr;
        }
    }

    bool TestInterface::FrameThread()
    {
        Mat frame;
        double timeStamp = 0;

        if (ReadInFrame(frame, timeStamp) == false)
            return true;

        Mat _out = ProcessFrame(frame, timeStamp);

        WriteOutFrame(_out, timeStamp);

        return true;
    }

    bool TestInterface::IsForeground()
    {
        return foreground;
    }

    void TestInterface::SetForeground(bool fore)
    {
        foreground = fore;
    }

    void TestInterface::SetFinished() 
    { 
        state = TestState::TestStateFinish; 
    }

    bool TestInterface::IsFinished() 
    { 
        return (state == TestState::TestStateFinish); 
    }

    void TestInterface::Start()
    {
        SetActive(true);
    }

    void TestInterface::SetGroup(const boost::weak_ptr<TestGroup> theGroup)
    {
        group = theGroup;
    }

    void TestInterface::StartGroup()
    {
        // only lock returns true, accessing the shared pointer to group is safe.
        if (boost::shared_ptr<TestGroup> npGroup = group.lock())
        {
            npGroup->Start();
        }
    }

    bool TestInterface::AddTest(boost::shared_ptr<TestInterface> test, bool isInit)
    {
        if (test == nullptr || test == shared_from_this())
            return false;

        if (isInit)
        {
            bool isActive = test->IsActive();
            test->SetActive(false);
            if (test->Init() == false)
            {
                test->SetActive(isActive);
                return false;
            }
            test->InitThread();
            test->SetActive(true);
        }

        test->SetGroup(group);
        // only lock returns true, accessing the shared pointer to group is safe.
        if (boost::shared_ptr<TestGroup> npGroup = group.lock())
        {
            npGroup->AddTest(test);
        }

        return true;
    }

    void TestInterface::WaitTestFinish(boost::shared_ptr<TestInterface> test)
    {
        if (test == nullptr)
            return;

        while (!test->IsFinished())
            ThreadSleep(10);
        return;
    }

    /// <summary>
    /// call another test and return after it's finished
    /// </summary>
    /// <param name="currTest"></param>
    /// <param name="childTest"></param>
    /// <param name="childIsForeground"></param>
    bool TestInterface::CallTest(boost::shared_ptr<TestInterface> childTest, bool childIsForeground, bool waitChildFinished)
    {
        if (childTest == nullptr)
            return false;

        SetActive(false);

        // the thread running TestGroup->ProcessFrame may wait for reading output frame from parent test
        // here we need to write a dummy frame and reset the event of waiting frame ready
        if (WriteOutFrame(Mat(), 0) == false)
        {
            SetActive(true);
            return false;
        }

        if (false == AddTest(childTest))
        {
            SetActive(true);
            return false;
        }

        childTest->SetParent(shared_from_this());
        if (childIsForeground == true)
            SetTestAsForeground(childTest);

        if (waitChildFinished)
        {
            WaitTestFinish(childTest);
            StopTest(childTest);
        }

        SetActive(true);

        return true;
    }

    void TestInterface::SetTestAsForeground(boost::shared_ptr<TestInterface> test)
    {
        if (test == nullptr)
            return;

        if (boost::shared_ptr<TestGroup> npGroup = group.lock())
        {
            npGroup->SetTestAsForeground(test);
        }
    }

    bool TestInterface::StopTest(boost::shared_ptr<TestInterface> test)
    {
        if (test == nullptr)
            return false;

        if (boost::shared_ptr<TestGroup> npGroup = group.lock())
        {
            return npGroup->StopTest(test);
        }

        return false;
    }

    void TestInterface::SetParent(const boost::shared_ptr<TestInterface> test)
    {
        parentTest = test;
    }

    boost::shared_ptr<TestInterface> TestInterface::GetParent()
    {
        return parentTest.lock();
    }
}
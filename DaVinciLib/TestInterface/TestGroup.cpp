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

#include "TestGroup.hpp"
#include "DeviceManager.hpp"


namespace DaVinci
{
    TestGroup::TestGroup()
    {
    }


    void TestGroup::AddTest(const boost::shared_ptr<TestInterface> test, bool foreground)
    {
        if (test == nullptr)
            return;

        {
            bool found = false;
            boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
            vector<boost::shared_ptr<TestInterface>>::iterator it = testLane.begin();
            while(it != testLane.end())
            {
                if ((*it) == test)
                {
                    found = true;
                    break;
                }
                it++;
            }

            if (!found)
            {
                testLane.push_back(test);
            }
            else
            {
                DAVINCI_LOG_ERROR << "The test is already in test lane.";
            }
        }

        test->SetGroup(shared_from_this());
        test->SetForeground(foreground);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="test"></param>
    bool TestGroup::StopTest(const boost::shared_ptr<TestInterface> test)
    {
        boost::shared_ptr<TestInterface> testToStop;

        if (test == nullptr)
            return false;

        {
            boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
            vector<boost::shared_ptr<TestInterface>>::iterator it = testLane.begin();
            while(it != testLane.end())
            {
                if ((*it) == test)
                {
                    testToStop = test;
                    testLane.erase(it);
                    break;
                }
                it++;
            }
        }

        if (testToStop == nullptr)
            return false;

        testToStop->SetFinished();
        testToStop->Destroy();
        testToStop->DestroyThread();

        return true;
    }

    bool TestGroup::StopTests()
    {
        vector<boost::shared_ptr<TestInterface>> testLaneToStop;
        {
            boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
            if (testLane.empty())
                return false;
            testLaneToStop = testLane;
            testLane.clear();
        }

        vector<boost::shared_ptr<TestInterface>>::reverse_iterator riter;
        for (riter = testLaneToStop.rbegin(); riter != testLaneToStop.rend(); riter++)
        {
            (*riter)->SetFinished();
            (*riter)->Destroy();
            (*riter)->DestroyThread();
        }

        return true;
    }

    /// <summary>
    /// Init this test group by init each test in this test group
    /// </summary>
    /// <returns>whether the init succeeded</returns>
    bool TestGroup::Init(void)
    {
        bool retVal = true;
        CaptureDevice::Preset preset = CaptureDevice::Preset::PresetDontCare;

        boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
        if (testLane.empty())
        {
            return false;
        }
        vector<boost::shared_ptr<TestInterface>>::iterator it = testLane.begin();
        while(it != testLane.end())
        {
            if ((*it)->CameraPreset() != CaptureDevice::Preset::PresetDontCare)
            {
                if (preset != CaptureDevice::Preset::PresetDontCare && preset != (*it)->CameraPreset())
                {
                    DAVINCI_LOG_ERROR << "Test contains different presets: " 
                        << (int)preset << " != " << (int)(*it)->CameraPreset() << ".";
                    return false;
                }
                else
                {
                    preset = (*it)->CameraPreset();
                }
            }
            ++it;
        }

        if (preset != CaptureDevice::Preset::PresetDontCare)
        {
            boost::shared_ptr<CaptureDevice> cameraCap = DeviceManager::Instance().GetCurrentCaptureDevice();
            if (cameraCap != nullptr)
            {
                cameraCap->InitCamera(preset);
            }
        }

        for (unsigned int i = 0; i < testLane.size(); i++)
        {
            auto it = testLane[i];
            it->SetActive(true);
            retVal = retVal && it->Init();
            if (it->IsFinished() == false)
                it->SetActive(true);
            it->InitThread();
        }

        return retVal;
    }

    /// <summary>
    /// Get captured frame sent from TestManager, send frame to each test in current test group in a specific order
    /// </summary>
    /// <param name="frame"> The frame captured</param>
    /// <param name="timeStamp"> The timestamp of the frame captured</param>
    /// <returns> The processed frame</returns>
    Mat TestGroup::ProcessFrame(const Mat& frame, double timeStamp, int frameIndex)
    {
        int frameCount = 0;
        Mat retVal;
        boost::scoped_array<bool> activeStates;

        {
            boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
            if (testLane.empty())
            {
                DAVINCI_LOG_ERROR << string("Error: none of test is running!");
                // SetFinished();
                return frame;
            }

            activeStates.reset(new bool[testLane.size()]);
            for (unsigned int i=0; i<testLane.size(); i++)
            {
                activeStates[i] = testLane[i]->IsActive();
                if (activeStates[i] == true)
                {
                    testLane[i]->WriteInFrame(frame, timeStamp, frameIndex);
                    frameCount++;
                }
            }

            double _timeStamp = 0;
            if (frameCount > 0)
            {
                for (unsigned int i = 0; i < testLane.size(); i++)
                {
                    // state during getting input and output frame, and the corresponding handling
                    //     true,  true:  ReadoutFrame, frameCount--
                    //     true,  false: frameCount--, but doesn't call ReadOutFrame
                    //     false, false: ignore
                    //     false, true:  ignore
                    if (activeStates[i] == true)
                    {
                        if (testLane[i]->IsActive() == true)
                        {
                            Mat outFrame;
                            bool ret = testLane[i]->ReadOutFrame(outFrame, _timeStamp);

                            if ((ret) && (testLane[i]->IsForeground()))
                            {
                                retVal = outFrame;
                                break;
                            }
                        }
                        frameCount--;
                    }
                }
            }
        }

        if (retVal.empty())
        {
            retVal = frame;
        }

        return retVal;
    }

    /// <summary>
    /// Pass the audio event to the underlying tests.
    /// </summary>
    /// <param name="e"></param>
    void TestGroup::ProcessWave(const boost::shared_ptr<vector<short>> &samples)
    {
        assert(samples != nullptr);
        for (unsigned int i = 0; i < testLane.size(); i++)
        {
            if (testLane[i]->IsActive() == true)
            {
                testLane[i]->ProcessWave(samples);
            }
        }
    }

    /// <summary>
    /// Set each test in this test group to start state
    /// </summary>
    void TestGroup::Start()
    {
        boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
        for (unsigned int i = 0; i < testLane.size(); i++)
        {
            testLane[i]->Start();
        }
    }

    /// <summary>
    /// Check whether all tests in current test group finished.  The test group finishes if any one of the tests finished
    /// TestManager is free to destroy a test group if it is finished.
    /// </summary>
    /// <returns>the finish state of the test group</returns>
    bool TestGroup::IsFinished()
    {
        bool retVal = false;
        {
            boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
            if (testLane.empty())
            {
                return true;
            }

            for (unsigned int i = 0; i < testLane.size(); i++)
            {
                if (testLane[i]->GetParent() == nullptr)
                    retVal = retVal || testLane[i]->IsFinished();
            }
        }

        return retVal;
    }

    /// <summary>
    /// 
    /// </summary>
    void TestGroup::SetFinished()
    {
        boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
        for (unsigned int j = 0; j < testLane.size(); j++)
        {
            testLane[j]->SetFinished();
        }
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="test"></param>
    void TestGroup::SetTestAsForeground(const boost::shared_ptr<TestInterface> test)
    {
        boost::lock_guard<boost::recursive_mutex> lock(mutexTestLane);
        if (testLane.size() > 0 && test != nullptr)
        {
            for (unsigned int i = 0; i < testLane.size(); i++)
            {
                if (testLane[i] == test)
                {
                    testLane[i]->SetForeground(true);
                }
                else if (testLane[i]->IsForeground() == true)
                {
                    testLane[i]->SetForeground(false);
                }
            }
        }
    }

}
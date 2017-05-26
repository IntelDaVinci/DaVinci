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

#include "ScriptEngineBase.hpp"
#include "DeviceManager.hpp"

namespace bc = boost::chrono;

namespace DaVinci
{
    ScriptEngineBase::ScriptEngineBase(const string &script) :
        qsFileName(script), needVideoRecording(true), isMultiLayerMode(false)
    {
    }

    ScriptEngineBase::~ScriptEngineBase()
    {
    }

    bool ScriptEngineBase::Init()
    {
        return true;
    }

    void ScriptEngineBase::Destroy()
    {
        CloseMediaFiles();
    }

    void ScriptEngineBase::CloseMediaFiles()
    {
        {
            boost::lock_guard<boost::mutex> lock(audioMutex);
            if (audioFile != nullptr)
            {
                audioFile = nullptr;
            }
        }

        if (needVideoRecording)
        {
            boost::lock_guard<boost::mutex> lock(videoMutex);
            if (IsActive())
                SetActive(false);

            if (videoWriter.isOpened())
            {
                videoWriter.release();
            }

            if (!qtsFileName.empty())
            {
                ofstream qts;
                qts.open(qtsFileName, ios::out | ios::trunc);
                if (qts.is_open())
                {
                    qts << "Version:1" << endl;
                    for (auto ts = timeStampList.begin(); ts != timeStampList.end(); ts++)
                    {
                        qts << static_cast<int64_t>(*ts) << endl;
                    }
                    qts.close();
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Unable to open QTS file " << qtsFileName << " for writing.";
                }
            }
        }
    }

    boost::shared_ptr<QScript> ScriptEngineBase::GetQS()
    {
        return qs;
    }

    vector<double> ScriptEngineBase::GetTimeStampList()
    {
        return timeStampList;
    }

    Mat ScriptEngineBase::ProcessFrame(const Mat &frame, const double timeStamp)
    {
        if (needVideoRecording)
        {
            boost::lock_guard<boost::mutex> lock(videoMutex);
            if (IsActive())
            {
                Mat currentFrame = frame;
                if(DeviceManager::Instance().UsingDisabledCam())
                {
                    double screenCapStartTime = GetCurrentMillisecond();
                    boost::shared_ptr<TargetDevice> dut = DeviceManager::Instance().GetCurrentTargetDevice();

                    if (dut != nullptr)
                    {
                        currentFrame = dut->GetScreenCapture();
                        double screenCapEndTime = GetCurrentMillisecond();
                        DAVINCI_LOG_DEBUG << "Screencap from " << boost::lexical_cast<string>(screenCapStartTime) << " to " << boost::lexical_cast<string>(screenCapEndTime) << endl;
                        if(currentFrame.empty())
                        {
                            Mat emptyMat = Mat(currentFrame.size(), currentFrame.type());
                            return frame;
                        }
                    }
                    else
                    {
                        if(DeviceManager::Instance().GetEnableTargetDeviceAgent())
                        {
                            DAVINCI_LOG_WARNING << "Failed to get frame due to no target device connected! ";
                        }
                        return frame;
                    }
                }

                if (!videoWriter.isOpened())
                {
                    videoFrameSize.width = currentFrame.cols;
                    videoFrameSize.height = currentFrame.rows;
                    if (!videoWriter.open(videoFileName, CV_FOURCC('M', 'J', 'P', 'G'), 30, videoFrameSize))
                    {
                        DAVINCI_LOG_ERROR << "      ERROR: Cannot create the video file for training or replaying!";
                        SetFinished();
                        return frame;
                    }
                }
                Mat frameResize = currentFrame;
                if (currentFrame.cols != videoFrameSize.width || currentFrame.rows != videoFrameSize.height)
                {
                    frameResize = Mat(videoFrameSize.height, videoFrameSize.width, currentFrame.type());
                    resize(currentFrame, frameResize, videoFrameSize, 0, 0, INTER_CUBIC);
                }
                timeStampList.push_back(qsWatch.ElapsedMilliseconds());
                videoWriter.write(frameResize);
                currentFrameIndex ++;
            }
            else
            {
                if (videoWriter.isOpened())
                {
                    videoWriter.release();
                }
            }
        }
        return frame;
    }

    void ScriptEngineBase::ProcessWave(const boost::shared_ptr<vector<short>> &samples)
    {
        boost::lock_guard<boost::mutex> lock(audioMutex);
        if (audioFile == nullptr)
        {
            audioFile = AudioFile::CreateWriteWavFile(audioFileName);
        }
        if (audioFile != nullptr)
        {
            sf_write_short(audioFile->Handle(), &(*samples)[0], samples->size());
        }
    }
}
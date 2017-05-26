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

#include "DaVinciCommon.hpp"
#include "TestManager.hpp"
#include "DeviceManager.hpp"
#include "AndroidTargetDevice.hpp"
#include "HyperSoftCamCapture.hpp"

#include "opencv2/gpu/gpu.hpp"

#include <chrono>
#include <thread>

namespace DaVinci
{
    bool HyperSoftCamCapture::isHscModuleInitialized = false;

    boost::mutex HyperSoftCamCapture::hscModuleInitMutex;

    boost::shared_ptr<HyperSoftCamCapture> HyperSoftCamCapture::self;

    DaVinciStatus HyperSoftCamCapture::InstallAgent(const boost::shared_ptr<AndroidTargetDevice> &targetDevice)
    {
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "Invalid target device";
            return DaVinciStatus(errc::invalid_argument);
        }

        boost::filesystem::path agentPath(TestManager::Instance().GetDaVinciResourcePath("CameralessAgent/Android"));
        string cpuArch = targetDevice->GetCpuArch();
        if (cpuArch.find("x86") != string::npos)
        {
            agentPath /= "x86";
        }
        else
        {
            agentPath /= "arm";
        }

        string sdkVersion = targetDevice->GetSdkVersion(true);
        DAVINCI_LOG_DEBUG << "SDK Version: " << sdkVersion;
        int apiLevel = 19; // 4.4 by default
        TryParse(sdkVersion, apiLevel);
        agentPath /= AndroidTargetDevice::ApiLevelToVersion(apiLevel);
        agentPath /= "camera_less";
        targetDevice->AdbPushCommand(agentPath.string(), "/data/local/tmp/camera_less");
        targetDevice->AdbShellCommand("chmod 777 /data/local/tmp/camera_less");
        return DaVinciStatus(DaVinciStatusSuccess);
    }

    HyperSoftCamCapture::HyperSoftCamCapture(const string &deviceName)
        : targetDeviceName(deviceName), maxFrameQueueCount(1),
          captureServer(nullptr), localPort(0), restartCount(0), devFrameIndex(-1)
    {
        {
            boost::lock_guard<boost::mutex> lock(hscModuleInitMutex);
            if (!isHscModuleInitialized)
            {
                InitializeRSModule();
                isHscModuleInitialized = true;
            }
        }

        targetDevice = DeviceManager::Instance().GetAndroidTargetDevice(targetDeviceName);
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_FATAL << "Could not start Hyper Soft Camera without the target device connected!";
            throw DaVinciError(DaVinciStatus(errc::no_such_device));
        }

        frameReadyEvent.Reset();

        // TODO: we may get all resolutions supported by target device
        // and set corresponding width/height for high resolution here
        // HyperSoftCam resolution
        frameWidth = -1;
        frameHeight = -1;
        fps = 0;
        currentPreset = CaptureDevice::Preset::HighResolution;

        deviceHeight = targetDevice->GetDeviceHeight();
        deviceWidth = targetDevice->GetDeviceWidth();
        if (deviceWidth <= 0 || deviceHeight <= 0)
        {
            throw DaVinciError(DaVinciStatus(errc::invalid_argument));
        }

        hyperSoftCamStatus = RS_SUCCESS;
    }

    void HyperSoftCamCapture::GeneralExceptionCallback(RS_STATUS code)
    {
        DAVINCI_LOG_ERROR << string("Camera-less OnError status: ") << code;
        assert(self != nullptr);
        self->hyperSoftCamStatus = code;
    }

    void HyperSoftCamCapture::CaptureFrameCallback(unsigned int width, unsigned int height)
    {
        assert(self != nullptr);
        // TODO: can we pass BGRA image directly to the consumer?
        unsigned int bufferSize = width * height * 4;
        cv::Mat frameBuffer(static_cast<int>(height), static_cast<int>(width), CV_8UC4);
        GetFrameBuffer(frameBuffer.data, bufferSize);
        cv::Mat rgbFrameBuffer(frameBuffer.size(), CV_8UC3);

        if (DeviceManager::Instance().GpuAccelerationEnabled())
        {
            cv::gpu::GpuMat gpuFrameBuffer(frameBuffer);
            cv::gpu::GpuMat rgbGpuFrameBuffer(frameBuffer.size(), CV_8UC3);
            cv::gpu::cvtColor(gpuFrameBuffer, rgbGpuFrameBuffer, CV_BGRA2BGR, 3);
            rgbGpuFrameBuffer.download(rgbFrameBuffer);
            // Reset restart count
            self->restartCount = 0;
        }
        else
        {
            cv::cvtColor(frameBuffer, rgbFrameBuffer, CV_BGRA2BGR, 3);
        }
        self->EnqueueFrame(rgbFrameBuffer);
    }

    void HyperSoftCamCapture::SetLocalPort(unsigned short port)
    {
        localPort = port;
    }

    DaVinciStatus HyperSoftCamCapture::Start()
    {
        boost::lock_guard<boost::mutex> lock(startStopMutex);

        unsigned short base = localPortBase;
        if (localPort != 0)
            base = localPort;
        localPort = GetAvailableLocalPort(base);
        if (localPort == 0)
        {
            DAVINCI_LOG_ERROR << "Error: unable to find a free local port for camera-less streaming!";
            return DaVinciStatus(errc::address_not_available);
        }

        self = boost::dynamic_pointer_cast<HyperSoftCamCapture>(shared_from_this());

        DaVinciStatus ret = CaptureDevice::Start();
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Error: unable to start capture device";
            return ret;
        }

        TeardownCameralessAgent();

        ret = SetupCameralessAgent();
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Error: unable to set up HyperSoftCam agent.";
            return ret;
        }

        bool statisticsMode = TestManager::Instance().GetStatisticsMode();
        hyperSoftCamStatus = RS_SUCCESS;
        unsigned int status = ::Start("127.0.0.1", localPort, 0, statisticsMode, CaptureFrameCallback, GeneralExceptionCallback);
        if (status == 0)
        {
            return DaVinciStatusSuccess;
        }
        else
        {
            // should we stop the parent here?
            // the current design depends on the caller to stop
            self = nullptr;
            DAVINCI_LOG_ERROR << string("Error starting Camera-less with error status code ") << status;
            return DaVinciStatus(status);
        }
    }

    DaVinciStatus HyperSoftCamCapture::Stop()
    {
        boost::lock_guard<boost::mutex> lock(startStopMutex);
        ::Stop();
        self = nullptr;
        TeardownCameralessAgent();

        CaptureDevice::SetStopFlag();
        frameReadyEvent.Set();
        CaptureDevice::Join();

        // Reset frame index
        devFrameIndex = -1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus HyperSoftCamCapture::SetupCameralessAgent()
    {
        string extParam = "";

        DaVinciStatus ret = HyperSoftCamCapture::InstallAgent(targetDevice);
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Cannot install HyperSoftCam agent to target device";
            return ret;
        }

        if (TestManager::Instance().GetStatisticsMode()) {
            extParam = " --stat";
        }

        if (TestManager::Instance().NeedRecordFrameIndex()) {
            extParam = " --index";
        }

        if (frameWidth > 0 && frameHeight > 0) {
            extParam = extParam + " --res " + boost::lexical_cast<string>(frameWidth) + "x" + boost::lexical_cast<string>(frameHeight);
        }

        if (fps > 0) {
            extParam = extParam + " --fps " + boost::lexical_cast<string>(fps);
        }

        targetDevice->AdbCommand("forward tcp:" + boost::lexical_cast<string>(localPort) + " tcp:" + boost::lexical_cast<string>(remotePort));
        RunProcessAsync(AndroidTargetDevice::GetAdbPath(), " -s " + targetDeviceName + " shell /data/local/tmp/camera_less --port " + boost::lexical_cast<string>(remotePort) + extParam, captureServer);
        // wait enough time for the agent to start
        ThreadSleep(2000);
        return DaVinciStatus(DaVinciStatusSuccess);
    }

    void HyperSoftCamCapture::TeardownCameralessAgent()
    {
        // Set the timeout to stop camera-less on device side
        int count = 0;
        const int maxTryTimes = 4;
        const int timeInterval = 500;

        targetDevice->KillProcess("camera_less", 3);
        while (count < maxTryTimes)
        {
            count++;
            if (targetDevice->FindProcess("camera_less") < 0)
            {
                break;
            }
            else
            {
                ThreadSleep(timeInterval);
            }
        }

        if (captureServer != nullptr)
        {
            try
            {
                captureServer->terminate();
            }
            catch (...)
            {
                DAVINCI_LOG_DEBUG << "unable to terminate the capture server";
            }
            captureServer = nullptr;
        }
        if (localPort > 0)
        {
            targetDevice->AdbCommand(string("forward --remove tcp:") + boost::lexical_cast<string>(localPort));
        }
    }

    void HyperSoftCamCapture::EnqueueFrame(const cv::Mat &frame)
    {
        {
            boost::lock_guard<boost::mutex> lock(frameQueueMutex);
            if (frameQueue.size() == maxFrameQueueCount)
            {
                frameQueue.pop();
            }
            frameQueue.push(std::make_pair(++devFrameIndex, frame));
        }
        frameReadyEvent.Set();
    }

    cv::Mat HyperSoftCamCapture::RetrieveFrame(int *frameIndex)
    {
        cv::Mat currentFrame;
        {
            boost::lock_guard<boost::mutex> lock(frameQueueMutex);
            if (!frameQueue.empty())
            {
                std::pair<int, cv::Mat> curItem = frameQueue.front();
                currentFrame = curItem.second;
                if (frameIndex != NULL)
                    *frameIndex = curItem.first;
                frameQueue.pop();
            }
        }

        if (!currentFrame.empty())
        {
            int curFrameWidth = currentFrame.cols;
            int curFrameHeight = currentFrame.rows;
            unsigned int lSide = std::max(curFrameWidth, curFrameHeight);
            unsigned int sSide = CalPxLengthOfSide(lSide,
                                                   deviceWidth,
                                                   deviceHeight);
            if (std::min(curFrameWidth, curFrameHeight) != (int)sSide)
            {
                cv::Size sz(static_cast<int>(curFrameWidth >= curFrameHeight ? lSide : sSide),
                            static_cast<int>(curFrameWidth >= curFrameHeight ? sSide : lSide));
                cv::resize(currentFrame, currentFrame, sz);
            }

            if (currentFrame.cols < currentFrame.rows)
            {
                currentFrame = RotateImage270(currentFrame);
            }
        }

        return currentFrame;
    }

    bool HyperSoftCamCapture::IsFrameAvailable()
    {
        frameReadyEvent.WaitOne();
        boost::lock_guard<boost::mutex> lock(frameQueueMutex);
        if (frameQueue.empty())
        {
            return false;
        }
        return true;
    }

    string HyperSoftCamCapture::GetCameraName()
    {
        return "HyperSoftCam - " + targetDeviceName;
    }

    void HyperSoftCamCapture::SetCaptureProperty(int prop, double value, bool shouldCheck)
    {
        if (prop == CV_CAP_PROP_FRAME_WIDTH) {
            frameWidth = (int)value;
        } else if(prop == CV_CAP_PROP_FRAME_HEIGHT) {
            frameHeight = (int)value;
        } else if(prop == CV_CAP_PROP_FPS) {
            fps = (int)value;
        } else {
            DAVINCI_LOG_ERROR << "Invalid property for HyperSoftCam capture";
        } 
    }

    DaVinciStatus HyperSoftCamCapture::Restart()
    {
        boost::lock_guard<boost::mutex> lock(startStopMutex);

        ::Stop();
        // Reset frame index
        devFrameIndex = -1;

        frameReadyEvent.Set();

        TeardownCameralessAgent();

        if (hyperSoftCamStatus == RS_DEVICE_CODEC_ERR)
        {
            return errc::state_not_recoverable;
        }

        if (restartCount < maxRestartCount)
        {
            restartCount++;
        }
        else
        {
            restartCount = 0;
            DAVINCI_LOG_ERROR << "HyperSoftCam may not support current device";
            return errc::not_supported;
        }

        DaVinciStatus ret = SetupCameralessAgent();
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Cannot setup HyperSoftCam agent to restart HyperSoftCam";
            return DaVinciStatus(errc::state_not_recoverable);
        }

        bool statisticsMode = TestManager::Instance().GetStatisticsMode();
        unsigned int status = ::Start("127.0.0.1", localPort, 0, statisticsMode, CaptureFrameCallback, GeneralExceptionCallback);
        DAVINCI_LOG_INFO << "Restarted HyperSoftCam with status code: " << status;
        if (status == RS_SUCCESS)
        {
            hyperSoftCamStatus = RS_SUCCESS;
            return DaVinciStatusSuccess;
        }
        else
        {
            return DaVinciStatus(status);
        }
    }

    double HyperSoftCamCapture::GetCaptureProperty(int prop)
    {
        if (prop == CV_CAP_PROP_FRAME_WIDTH) {
            return frameWidth;
        } else if(prop == CV_CAP_PROP_FRAME_HEIGHT) {
            return frameHeight;
        }  else {
            DAVINCI_LOG_ERROR << "Invalid property for HyperSoftCam capture";
        }
        return -1.0;
    }

    bool HyperSoftCamCapture::InitCamera(CaptureDevice::Preset preset)
    {
        assert(preset >= CaptureDevice::Preset::PresetDontCare &&
            preset < CaptureDevice::Preset::PresetNumber);
        DAVINCI_LOG_DEBUG << "Initialize camera properties for preset: "  << CapturePresetName[(int)preset+1];

        int capResOtherSidePxLen = 0;
        int capResLongerSidePxLen = TestManager::Instance().GetCapResLongerSidePxLen();
        if (capResLongerSidePxLen <= 0)
        {
            if (currentPreset == preset)
            {
                DAVINCI_LOG_DEBUG << "The preset was already initialized.";
                return true;
            }
            currentPreset = preset;

            capResLongerSidePxLen = highSpeedWidth;
            if (preset == Preset::HighResolution)
            {
                capResLongerSidePxLen = highResWidth;
            }
            else if (preset == Preset::AIResolution)
            {
                capResLongerSidePxLen = aiResWidth;
            }
        }
        assert(capResLongerSidePxLen > 0);

        int deviceWidth = targetDevice->GetDeviceWidth();
        int deviceHeight = targetDevice->GetDeviceHeight();
        capResOtherSidePxLen = CalPxLengthOfSide(capResLongerSidePxLen,
                                                 deviceWidth,
                                                 deviceHeight);
        if (capResOtherSidePxLen <= 0)
        {
            DAVINCI_LOG_ERROR << "Failed in calculating HyperSoftCam resolution. "
                                << "Device Width:" << deviceWidth << " "
                                << "Device Height:" << deviceHeight;
            return false;
        }

        if (capResLongerSidePxLen != frameWidth)
        {
            DAVINCI_LOG_INFO << "Set resolution: " << capResLongerSidePxLen << "x" << capResOtherSidePxLen;
            DeviceManager::Instance().StopCaptureMonitor();
            Stop();
            SetCaptureProperty(CV_CAP_PROP_FRAME_WIDTH, capResLongerSidePxLen, true);
            SetCaptureProperty(CV_CAP_PROP_FRAME_HEIGHT, capResOtherSidePxLen, true);
            DaVinciStatus ret = Start();
            DeviceManager::Instance().StartCaptureMonitor();
            if (ret == DaVinciStatus(RS_DEVICE_CONFIG_CODEC_ERR))
            {
                DAVINCI_LOG_WARNING << "Target device doesn't support codec resolution :"
                                    << capResLongerSidePxLen << "x" << capResOtherSidePxLen;
                SetCaptureProperty(CV_CAP_PROP_FRAME_WIDTH, -1, true);
                SetCaptureProperty(CV_CAP_PROP_FRAME_HEIGHT, -1, true);

                if (Start() != DaVinciStatus(RS_SUCCESS))
                {
                    DAVINCI_LOG_ERROR << "Failed to set target device codec resolution :"
                                      << capResLongerSidePxLen << "x" << capResOtherSidePxLen;
                    return false;
                }
            }
            else if (ret == DaVinciStatus(RS_SUCCESS))
            {
                return true;
            }
            else
            {
                DAVINCI_LOG_ERROR << "Failed to set target device codec resolution(" << ret.value() << ").";
                return false;
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << "Same width (" << frameWidth 
                              << "), height(" << frameHeight
                              << "). preset: " << (int)currentPreset;
        }

        return true;
    }
}
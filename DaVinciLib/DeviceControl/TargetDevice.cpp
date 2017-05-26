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

#include "TargetDevice.hpp"

namespace DaVinci
{
    const double ClickInterval    = 150.0;
    const double ClickMaxDistance = 100.0;

    TargetDevice::TargetDevice(const string &name, bool isIpAddress) : deviceName(name),
        identifiedByIp(isIpAddress), screenSource(ScreenSource::CameraIndex0), deviceHeight(0), deviceWidth(0),
        deviceStatus(DeviceConnectionStatus::Offline), multiLayerSupported(false), audioPlayDevice("Disabled"), audioRecordDevice("Disabled"),
        defaultMultiLayerMode(false),currentMultiLayerMode(false), deviceCpuUsage(0), deviceMemTotal(0), deviceMemFree(0),
        deviceDiskTotal(0), deviceDiskFree(0), deviceBatteryLevel(0), hwAccessoryController("Undefined")
    {
    }

    TargetDevice::~TargetDevice()
    {
    }

    string TargetDevice::GetDeviceName()
    {
        return deviceName;
    }

    bool TargetDevice::IsIdentifiedByIp()
    {
        return identifiedByIp;
    }

    string TargetDevice::GetAudioPlayDevice()
    {
        return audioPlayDevice;
    }

    void TargetDevice::SetAudioPlayDevice(const string &device)
    {
        audioPlayDevice = device;
    }

    string TargetDevice::GetAudioRecordDevice()
    {
        return audioRecordDevice;
    }

    void TargetDevice::SetAudioRecordDevice(const string &device)
    {
        audioRecordDevice = device;
    }

    string TargetDevice::GetCalibrationData()
    {
        return calibrationData;
    }

    void TargetDevice::SetCalibrationData(const string &data)
    {
        calibrationData = data;
    }

    string TargetDevice::GetCameraSetting()
    {
        return cameraSetting;
    }

    void TargetDevice::SetCameraSetting(const string &setting)
    {
        cameraSetting = setting;
    }

    int TargetDevice::GetDeviceHeight()
    {
        return deviceHeight;
    }

    void TargetDevice::SetDeviceHeight(int height)
    {
        deviceHeight = height;
    }

    int TargetDevice::GetDeviceWidth()
    {
        return deviceWidth;
    }

    void TargetDevice::SetDeviceWidth(int width)
    {
        deviceWidth = width;
    }

    pair<int, int> TargetDevice::GetAppSize()
    {
        return pair<int, int>(0, 0);
    }

    bool TargetDevice::GetCurrentMultiLayerMode()
    {
        return currentMultiLayerMode;
    }

    void TargetDevice::SetCurrentMultiLayerMode(bool status)
    {
        currentMultiLayerMode = status;
    }

    bool TargetDevice::GetDefaultMultiLayerMode()
    {
        return defaultMultiLayerMode;
    }

    void TargetDevice::SetDefaultMultiLayerMode(bool status)
    {
        // When system initialize or change multilayer mode, so need to sync default & current mode at the same time.
        defaultMultiLayerMode = status;
        currentMultiLayerMode = status;
    }

    DeviceConnectionStatus TargetDevice::GetDeviceStatus()
    {
        return deviceStatus;
    }

    void TargetDevice::SetDeviceStatus(DeviceConnectionStatus status)
    {
        deviceStatus = status;
    }

    ScreenSource TargetDevice::GetScreenSource()
    {
        return screenSource;
    }

    void TargetDevice::SetScreenSource(ScreenSource source)
    {
        screenSource = source;
    }

    string TargetDevice::GetHWAccessoryController()
    {
        return hwAccessoryController;
    }

    void TargetDevice::SetHWAccessoryController(const string &theController)
    {
        hwAccessoryController = theController;
    }

    bool TargetDevice::IsClickAction(int srcX, int srcY, double srcTimeStamp, int dstX, int dstY, double dstTimeStamp)
    {
        if ((dstTimeStamp - srcTimeStamp) <= ClickInterval)
        {
            double xDelta = std::pow(srcX - dstX, 2);
            double yDelta = std::pow(srcY - dstY, 2);
            double touchDistance = std::sqrt(xDelta + yDelta);
            if (touchDistance <= ClickMaxDistance)
            {
                return true;
            }
        }

        return false;
    }
}
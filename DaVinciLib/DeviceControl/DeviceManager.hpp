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

#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>

#include "boost/core/noncopyable.hpp"
#include "boost/thread/recursive_mutex.hpp"

#include "xercesc/dom/DOM.hpp"

#include "TargetDevice.hpp"
#include "PhysicalCameraCapture.hpp"
#include "AndroidTargetDevice.hpp"
#include "StopWatch.hpp"

#include "HWAccessoryController.hpp"

#include "GlobalDeviceStatus.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary> A singleton Manager for devices. </summary>
    class DeviceManager : public SingletonBase<DeviceManager>
    {
    public:
        static const string ConfigRoot;
        static const string ConfigUseGpuAcceleration;
        static const string ConfigAudioRecordDevice;
        static const string ConfigAudioPlayDevice;
        static const string ConfigAndroidDevices;
        static const string ConfigWindowsDevices;
        static const string ConfigChromeDevices;
        static const string ConfigIdentifiedByIp;
        static const string ConfigMultiLayerSupported;
        static const string ConfigHeight;
        static const string ConfigWidth;
        static const string ConfigScreenSource;
        static const string ConfigCalibrateData;
        static const string ConfigCameraSetting;
        static const string ConfigHwAccessoryController;
        static const string ConfigPortOfFFRD;
        static const string ConfigAndroidTargetDevice;
        static const string ConfigDeviceModel;
        static const string ConfigQagentPort;
        static const string ConfigMagentPort;
        static const string ConfigHyperSoftCamPort;

#ifdef _DEBUG
        static const int captureMonitorTimeout = 20000;
#else
        static const int captureMonitorTimeout = 6000;
#endif

        /// <summary> Default constructor. </summary>
        DeviceManager();

        /// <summary> Initialize global devices status </summary>
        ///
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitGlobalDeviceStatus();

        /// <summary>
        /// Detect all the devices per the given configuration.
        /// The function populate all devices configured in the configFile
        /// and the ones discovered in runtime.
        /// </summary>
        ///
        /// <returns> The configuration. </returns>
        DaVinciStatus DetectDevices(String currentTargetDevice="");

        /// <summary> Initialize all the devices on host and target </summary>
        ///
        /// <param name="deviceName"> (Optional) name of the device. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitDevices(const string &deviceName = "");

        /// <summary>
        /// Initialize devices for testing (phone, camera and FFRD)
        /// 
        /// If there is only 1 phone and camera, by default it establish 1x1 mapping;
        /// Otherwise:
        ///     1) In console mode, the function establishes 1xN mapping automatically and generates
        ///     device_config.ini. 2) In window mode, the function only format the device string with
        ///     camera name.
        /// </summary>
        ///
        /// <param name="deviceName"> (Optional) name of the device </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitTestDevices(const string &deviceName = "");

        void ClearPhysicalCameraList();

        /// <summary> Closes the devices. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus CloseDevices();

        void CloseCaptureDevices();
        void CloseTargetDevices();
        void CloseHWAccessoryControllers();

        /// <summary>   Gets current target device. </summary>
        ///
        /// <returns>   The current target device. </returns>
        boost::shared_ptr<TargetDevice> GetCurrentTargetDevice() const
        {
            return currentTargetDevice;
        }

        /// <summary>   Gets current capture device. </summary>
        ///
        /// <returns>   The current capture device. </returns>
        boost::shared_ptr<CaptureDevice> GetCurrentCaptureDevice() const
        {
            return currentCaptureDevice;
        }

        /// <summary>   Gets current ffrd controller device. </summary>
        /// 
        /// <returns>   The current ffrd controller device. </returns>
        boost::shared_ptr<HWAccessoryController> GetCurrentHWAccessoryController() const
        {
            return currentHWAccessoryController;
        }

        /// <summary>   Gets a hardware accessory controller device by COM port. </summary>
        ///
        /// <param name="comPort"> COM port </param>
        ///
        /// <returns>   The hardware accessory controller device attached on specific COM port. </returns>
        boost::shared_ptr<HWAccessoryController> DeviceManager::GetHWAccessoryControllerDevice(const string &comPort);


        /// <summary> Gets target device with the specific name </summary>
        ///
        /// <param name="name"> The name. </param>
        ///
        /// <returns> The target device. </returns>
        boost::shared_ptr<TargetDevice> GetTargetDevice(const string &name);

        /// <summary> Get list of all target devices. </summary>
        ///
        /// <returns> vector list of the target devices. </returns>
        vector<boost::shared_ptr<TargetDevice>>& GetTargetDeviceList();

        /// <summary> Gets android target device. </summary>
        ///
        /// <param name="name"> The name. </param>
        ///
        /// <returns> The android target device. </returns>
        boost::shared_ptr<AndroidTargetDevice> GetAndroidTargetDevice(const string &name)
        {
            return boost::dynamic_pointer_cast<AndroidTargetDevice>(GetTargetDevice(name));
        }

        Orientation GetDeviceDefaultOrientation()
        {
            if (currentTargetDevice == nullptr)
                return Orientation::Unknown;
            else
                return currentTargetDevice->GetDefaultOrientation();
        }

        /// <summary>
        /// Enables the target device agent. By default, the target device agent is enabled. If the
        /// function is called with "false" argument, the device manager would not connect to the target
        /// device's agent. If the current device is already connected, it would be disconnected.
        /// </summary>
        ///
        /// <param name="enable"> true to enable, false to disable. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus EnableTargetDeviceAgent(bool enable);

        /// <summary>
        /// Get the flag's for enables the target device agent. By default, the target device agent is enabled. If the
        /// function is called with "false" argument, the device manager would not connect to the target
        /// device's agent. If the current device is already connected, it would be disconnected.
        /// </summary>
        /// <returns> True for yes. </returns>
        bool GetEnableTargetDeviceAgent();

        /// <summary> Query if this host machine has cuda GPGPU support. </summary>
        ///
        /// <returns> true if cuda support, false if not. </returns>
        bool HasCudaSupport();

        /// <summary> Enables the GPU acceleration. </summary>
        ///
        /// <param name="enable"> true to enable, false to disable. </param>
        void EnableGpuAcceleration(bool enable);

        /// <summary> Determines if GPU acceleration is enabled. </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        bool GpuAccelerationEnabled();

        /// <summary> Queries GPU information. </summary>
        ///
        /// <returns> The GPU information. </returns>
        string QueryGpuInfo();

        /// <summary> Determines if we are using hyper soft camera. </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        bool UsingHyperSoftCam();

        /// <summary> Determines if we are using disabled camera. </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        bool UsingDisabledCam();

        /// <summary> Gets current capture string. </summary>
        ///
        /// <returns> The current capture string. </returns>
        string GetCurrentCaptureStr();

        /// <summary> Gets current capture preset. </summary>
        ///
        /// <returns> The current capture preset. </returns>
        CaptureDevice::Preset GetCurrentCapturePreset();

        bool IsAndroidDevice(const boost::shared_ptr<TargetDevice> &device) const;

        /// <summary> Loads the device configuration. </summary>
        ///
        /// <param name="configFile"> The configuration file. </param>
        ///
        /// <returns> The configuration. </returns>
        DaVinciStatus LoadConfig(const string &configFile);

        /// <summary> Saves the device configuration. </summary>
        ///
        /// <param name="configFile"> The configuration file. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SaveConfig(const string &configFile);

        /// <summary> Update device order. </summary>
        ///
        /// <param name="curDevName"> The name of current device. </param>
        ///
        /// <returns> </returns>
        void UpdateDeviceOrder(const string &curDevName);

        /// <summary> Return current device name. </summary>
        ///
        /// <param name=""> </param>
        ///
        /// <returns> The name of current device. </returns>
        string GetCurrentDeviceName();

        /// <summary>
        /// Initialises the specified target device. If the deviceName is empty,
        /// the only available device will be initialized.
        /// 
        /// The currentTargetDevice will be set when available device is found (regardless
        /// of whether its agent is connected or not), otherwise, not changed.
        /// </summary>
        ///
        /// <param name="deviceName"> (Optional) name of the device to initialize. </param>
        ///
        /// <returns> Failed when there is no available device or cannot connect to the device </returns>
        DaVinciStatus InitTargetDevice(const string &deviceName = "");

        /// <summary>
        /// Initialises the capture device.
        /// 
        /// This function should be called after target devices and HW accessories are initialized since
        /// the capture device could be a softcam from target device or a physical camera controllable by
        /// a USB replug.
        /// 
        /// If the preferredSource is undefined, the function would first get the current capture device
        /// according to the following steps:
        /// (1) Current target device's configuration
        /// (2) First camera if exists
        /// (3) Pseudo camera
        /// 
        /// The function then sets the current capture and starts it. The previous set current capture
        /// would be stopped before the capture started.
        /// </summary>
        ///
        /// <param name="preferredSource"> (Optional) the preferred source. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitCaptureDevice(ScreenSource preferredSource = ScreenSource::Undefined, bool isUpdateScreenSource=true);

        string GetAudioRecordDevice() const;

        string GetAudioPlayDevice() const;

        DaVinciStatus SetAudioRecordDevice(const string &audioDevice);

        DaVinciStatus SetAudioPlayDevice(const string &audioDevice);

        DaVinciStatus SetRecordFromDevice(const string &targetDevice, const string &audioDevice);

        DaVinciStatus SetPlayToDevice(const string &targetDevice, const string &audioDevice);
        /// <summary>
        /// Initialises the capture device with name. it's called by C++ API, which is provided to C# layer.
        /// </summary>
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitCaptureDevice(string name);

        DaVinciStatus SetCaptureDeviceForTargetDevice(string deviceName, string captureName);
        DaVinciStatus SetHWAccessoryControllerForTargetDevice(string deviceName, string comName);

        /// <summary> Initialises all the HW accessories. </summary>
        ///
        /// <param name="comPort"> (Optional) name of the COM port to initialize. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitHWAccessoryControllers(const string &comPort = "");

        vector<string> GetTargetDeviceNames();
        vector<string> GetCaptureDeviceNames();
        vector<string> GetHWAccessoryControllerNames();

        void SetForceFFRDDetect(bool flag);

        int GetNullFrameCount();
        void IncreaseNullFrameCount();
        void ResetNullFrameCount();
        void HandleDiedCapture(bool needRestart = true);

        void StartCaptureMonitor();
        void StopCaptureMonitor();

        /// <summary>
        /// Get the device's connected status. 
        /// </summary>
        ///
        /// <returns> True for connected, False for idle. </returns>
        bool IsDeviceConnectedByOthers(const DeviceCategory category, const string &name);

        /// <summary>
        /// Check and update the device's connected status. 
        /// </summary>
        ///
        /// <returns> True for when the device is not connected by others. </returns>
        bool CheckAndUpdateGlobalStatus(const DeviceCategory category, const string &name, bool connect);

    private:
        static const int maxNullFrameCount = 30;
        boost::atomic<int> nullFrameCount;

        /// <summary> flag indicating if we have checked cuda support, assume checking takes time </summary>
        bool testedCudaSupport;

        /// <summary> flag indicating if we have cuda support </summary>
        bool hasCudaSupport;

        /// <summary> flag indicating if GPU acceleration is enabled </summary>
        bool gpuAccelerationEnabled;

        /// <summary> flag indicating if the target device agent should be enabled. </summary>
        bool enableTargetDeviceAgent;

        /// <summary> The audio device that records user's voice from host machine </summary>
        string audioRecordDevice;

        /// <summary> The audio device that plays device audio on host machine </summary>
        string audioPlayDevice;

        boost::shared_ptr<GlobalDeviceStatus> globalDeviceStatus;

        /// <summary>   The current target device. </summary>
        boost::shared_ptr<TargetDevice> currentTargetDevice;
        /// <summary>   The current capture device. </summary>
        boost::shared_ptr<CaptureDevice> currentCaptureDevice;
        /// <summary>   The current controller. </summary>
        boost::shared_ptr<HWAccessoryController> currentHWAccessoryController;

        boost::recursive_mutex targetDevicesMutex;
        boost::recursive_mutex captureDevicesMutex;
        boost::recursive_mutex hwAccessoriesMutex;
        vector<boost::shared_ptr<TargetDevice>> targetDevices;
        vector<boost::shared_ptr<PhysicalCameraCapture>> physicalCams;
        vector<boost::shared_ptr<HWAccessoryController>> hwAccessoryControllers;

        // data structure to monitor the liveness of capture device
        StopWatch captureLiveWatch;
        boost::shared_ptr<boost::thread> captureMonitor;
        boost::atomic<bool> captureMonitorShouldExit;
        boost::mutex captureMonitorMutex;

        /// <summary> Initialises the host devices (GPU, audio etc.). </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InitHostDevices();

        /// <summary> Scan all the attaching target devices. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ScanTargetDevices(String & currentTargetDeviceName);

        /// <summary> Scan all the attaching physical camera capture devices. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ScanCaptureDevices(int currentPhysCamsIndex = -1);

        /// <summary> Scan all the attaching hardware accessories. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ScanHWAccessories(String currentHWAccessory="");

        bool forceFFRDDetect;

        /// <summary> Finds the only available device </summary>
        ///
        /// <returns> The target device that is not offline, or null if none or more than one available </returns>
        boost::shared_ptr<TargetDevice> FindOnlyAvailableDevice();

        /// <summary> Parse configuration from root XML </summary>
        ///
        /// <param name="configFile"> The configuration file. </param>
        /// <param name="doc">        The document. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseConfigRoot(const string &configFile, const xercesc::DOMDocument *doc);

        /// <summary> Parse common configuration of a target device </summary>
        ///
        /// <param name="device">   The device to initialize </param>
        /// <param name="attrList"> List of attributes of a target device </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseConfigTargetDevice(const boost::shared_ptr<TargetDevice> &device, const xercesc::DOMNodeList *attrList);

        /// <summary> Parse configuration of an android device </summary>
        ///
        /// <param name="configFile">  The configuration file. </param>
        /// <param name="devicesNode"> The devices node. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseConfigAndroid(const string &configFile, const xercesc::DOMNode *devicesNode);

        /// <summary> Parse configuration of a windows device </summary>
        ///
        /// <param name="configFile">  The configuration file. </param>
        /// <param name="devicesNode"> The devices node. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseConfigWindows(const string &configFile, const xercesc::DOMNode *devicesNode);

        /// <summary> Parse configuration of a chrome device </summary>
        ///
        /// <param name="configFile">  The configuration file. </param>
        /// <param name="devicesNode"> The devices node. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseConfigChrome(const string &configFile, const xercesc::DOMNode *devicesNode);

        /// <summary> Appends root elements to device configuration XML. </summary>
        ///
        /// <param name="doc"> The document. </param>
        void AppendRootElements(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &root);

        /// <summary> Appends common configuration of a target device. </summary>
        ///
        /// <param name="doc">           The document. </param>
        /// <param name="deviceElement"> The device element. </param>
        /// <param name="targetDevice">  Target device. </param>
        void AppendTargetDevice(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &deviceElement, const boost::shared_ptr<TargetDevice> &targetDevice);

        /// <summary> Appends android devices to device configuration XML. </summary>
        ///
        /// <param name="doc">             The document. </param>
        /// <param name="androidElements"> The android elements. </param>
        void AppendAndroidDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &androidElements);

        /// <summary> Appends windows devices to device configuration XML. </summary>
        ///
        /// <param name="doc">             The document. </param>
        /// <param name="windowsElements"> The windows elements. </param>
        void AppendWindowsDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &windowsElements);

        /// <summary> Append chrome devices to device configuration XML. </summary>
        ///
        /// <param name="doc">            The document. </param>
        /// <param name="chromeElements"> The chrome elements. </param>
        void AppendChromeDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &chromeElements);

        // data structure to monitor the liveness of capture device
        /// <summary>
        /// The entry point of the frame handler. It calls ProcessFrame and 
        /// also does some book-keeping such as refreshing capture monitor watch.
        /// </summary>
        ///
        /// <param name="captureDevice"> The capture device. </param>
        void ProcessFrameEntry(boost::shared_ptr<CaptureDevice> captureDevice);
        void CaptureMonitorEntry();
    };
}

#endif
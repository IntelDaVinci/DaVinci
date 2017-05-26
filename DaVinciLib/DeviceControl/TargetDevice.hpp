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

#ifndef __TARGET_DEVICE_HPP__
#define __TARGET_DEVICE_HPP__

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"

#include "opencv2/opencv.hpp"

#include "DaVinciStatus.hpp"
#include "DeviceControlCommon.hpp"
#include "boost/atomic.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum class ButtonEventMode
    {
        Up = 0,
        Down = 1,
        DownAndUp = 2,
    };

    enum class Brightness
    {
        Up = 0,
        Down = 1,
    };

    extern const double ClickInterval;   // millisecond
    // TODO: ClickMaxDistance depends on PPI. If device PPI is high, the value of ClickMaxDistance should be enlarged.
    // TODO: But we have not abstracted its pattern, so we defined it as a fixed value now.
    extern const double ClickMaxDistance; // distance of click points

    /// <summary> A target device. </summary>
    class TargetDevice : public boost::enable_shared_from_this<TargetDevice>
    {
        friend class DeviceManager;

    public:
        explicit TargetDevice(const string &name, bool isIpAddress = false);

        virtual ~TargetDevice();

        /// <summary> Gets device name. </summary>
        ///
        /// <returns> The device name. </returns>
        virtual string GetDeviceName();

        /// <summary> Is the device name identified by IP. </summary>
        ///
        /// <returns> true if the device name is identified by ip address (and port) </returns>
        virtual bool IsIdentifiedByIp();

        /// <summary>
        /// Gets the audio device to record audio from the device
        /// </summary>
        /// <returns> The name of the audio device </returns>
        virtual string GetAudioRecordDevice();

        /// <summary>
        /// Sets the audio device to record audio from the device
        /// </summary>
        /// <param name="device"> The name of the audio device </param>
        virtual void SetAudioRecordDevice(const string &device);

        /// <summary>
        /// Gets the audio device to playback the audio from the device
        /// </summary>
        /// <returns> The name of the audio device </returns>
        virtual string GetAudioPlayDevice();

        /// <summary>
        /// Sets the audio device to playback the audio from the device
        /// </summary>
        /// <param name="device"> The name of the audio device </param>
        virtual void SetAudioPlayDevice(const string &device);

        /// <summary>
        /// Check whether the device supports multi-touch.
        /// </summary>
        /// <returns> Yes or No. </returns>
        // virtual bool IsMultiTouchSupported();

        /// <summary>
        /// Check whether the device/test case's current multi-layer mode
        /// </summary>
        /// <returns> Yes or No. </returns>
        virtual bool GetCurrentMultiLayerMode();

        /// <summary>
        /// Set the device/test case's current multi-layer mode
        /// </summary>
        /// <returns> Yes or No. </returns>
        virtual void SetCurrentMultiLayerMode(bool status);

        /// <summary>
        /// Check whether the device/test case's default multi-layer mode
        /// </summary>
        /// <returns> Yes or No. </returns>
        virtual bool GetDefaultMultiLayerMode();

        /// <summary>
        /// Set the device/test case's default multi-layer mode
        /// </summary>
        /// <returns> Yes or No. </returns>
        virtual void SetDefaultMultiLayerMode(bool status);

        /// <summary> Query if this target device supports hyper soft camera. </summary>
        ///
        /// <returns> true if hyper soft camera supported, false if not. </returns>
        virtual bool IsHyperSoftCamSupported() = 0;

        /// <summary>
        /// Gets the camera calibration data when the device is tested under a camera
        /// </summary>
        /// <returns></returns>
        virtual string GetCalibrationData();

        /// <summary>
        /// Sets the camera calibration data when the device is tested under a camera
        /// </summary>
        /// <param name="para"></param>
        virtual void SetCalibrationData(const string &para);

        /// <summary>
        /// Gets the camera parameters when the device is tested under a camera
        /// </summary>
        /// <returns></returns>
        virtual string GetCameraSetting();

        /// <summary>
        /// Sets the camera parameters when the device is tested under a camera
        /// </summary>
        /// <param name="para"></param>
        virtual void SetCameraSetting(const string &para);

        /// <summary>
        /// Gets the device height in pixel
        /// </summary>
        /// <returns></returns>
        virtual int GetDeviceHeight();

        /// <summary>
        /// Sets the device height in pixel
        /// </summary>
        /// <param name="height"></param>
        virtual void SetDeviceHeight(int height);

        /// <summary>
        /// Gets the device width in pixel
        /// </summary>
        /// <returns></returns>
        virtual int GetDeviceWidth();

        /// <summary>
        /// Sets the device width in pixel
        /// </summary>
        /// <param name="width"></param>
        virtual void SetDeviceWidth(int width);

        /// <summary>
        /// Get status bar height
        /// </summary>
        /// <returns></returns>
        virtual vector<int> GetStatusBarHeight() = 0;

        /// <summary>
        /// Get action bar height
        /// </summary>
        virtual int GetActionBarHeight() = 0;

        /// <summary>
        /// Get nagivation bar height
        /// </summary>
        virtual vector<int> GetNavigationBarHeight() = 0;

        /// <summary>
        /// Get CPU usage. 40->40%
        /// </summary>
        virtual int GetCpuUsage(bool isRealTime = false) = 0;

        /// <summary>
        /// Get Free memory. 256 -> 256MB
        /// </summary>
        virtual bool GetMemoryInfo(long &memtotal, long &memfree, bool isRealTime = false) = 0;

        /// <summary>
        /// Get free disk.1024->1024MB
        /// </summary>
        virtual bool GetDiskInfo(long &disktotal, long& diskfree, bool isRealTime = false) = 0;

        /// <summary> Get battery level language </summary>
        ///
        /// <returns>  battery level. </returns>
        virtual bool GetBatteryLevel(long &batteryLevel, bool isRealTime = false) = 0;

        /// <summary> Get display language </summary>
        ///
        /// <returns>  display language. </returns>
        virtual string GetLanguage(bool isRealTime = false) = 0;

        /// <summary>
        /// Get Sensor Data
        /// </summary>
        virtual bool GetSensorData(int type, string &data) = 0;

        /// <summary>
        /// Get app size
        /// </summary>
        virtual pair<int, int> GetAppSize();

        /// <summary>
        /// connect the agent to the device
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus Connect() = 0;

        /// <summary>
        /// disconnect the agent to the device
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus Disconnect() = 0;

        /// <summary>
        /// Device status: connected, offline, ready for connection
        /// </summary>
        /// <returns></returns>
        virtual DeviceConnectionStatus GetDeviceStatus();

        /// <summary>
        /// Sets device status.
        /// 
        /// This private function intends to internal use.
        /// </summary>
        ///
        /// <param name="status"> The status. </param>
        virtual void SetDeviceStatus(DeviceConnectionStatus status);

        /// <summary>
        /// screen source
        /// </summary>
        /// <returns></returns>
        virtual ScreenSource GetScreenSource();

        /// <summary>
        /// set screen source
        /// </summary>
        /// <param name="source"></param>
        virtual void SetScreenSource(ScreenSource source);

        /// <summary>
        /// Gets the controller corresponding to this device, currently it is the com port
        /// ffrd connects to
        /// </summary>
        /// <returns></returns>
        virtual string GetHWAccessoryController();

        /// <summary>
        /// Sets the controller corresponding to this device, currently it is the com port
        /// ffrd connects to
        /// </summary>
        /// <param name="hwAccessoryController"></param>
        virtual void SetHWAccessoryController(const string &hwAccessoryController);

        /// <summary>
        /// This API will send press HOME or Win command to agent server.
        /// The agent server will do press HOME action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressHome(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// Send press power button
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressPower(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// This API will send press BACK command to agent server.
        /// The agent server will do press BACK action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressBack(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// This API will send press MENU command to agent server.
        /// The agent server will do press MENU action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressMenu(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary> Installs the application described by appName. </summary>
        ///
        /// <param name="appName"> Name of the application. It could be a path to the app file. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus InstallApp(const string &appName, const string &ops = "", bool toExtSD = false) = 0;

        /// <summary> Uninstall application. </summary>
        ///
        /// <param name="appName"> Name of the application. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus UninstallApp(const string &appName) = 0;

        /// <summary> Installs the application described by appName. </summary>
        ///
        /// <param name="appName"> Name of the application. It could be a path to the app file. </param>
        ///
        /// <param name="log"> Output. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus InstallAppWithLog(const string &appName, string &log, const string &ops = "", bool toExtSD = false, bool x64 = false) = 0;

        /// <summary> Uninstall application. </summary>
        ///
        /// <param name="appName"> Name of the application. </param>
        ///
        /// <param name="log"> Output. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus UninstallAppWithLog(const string &appName, string &log) = 0;

        /// <summary>
        /// start an activity in a package 
        /// </summary>
        /// <param name="activityFullName">
        /// On Android, it's the package/activity to be started.
        /// On Windows tablet, the name looks like: "Microsoft.BingMaps_8wekyb3d8bbwe!AppexMaps"
        /// </param> 
        /// <returns></returns>
        virtual DaVinciStatus StartActivity(const string &activityFullName, bool syncMode = false) = 0;

        /// <summary>
        /// Force stop an activity or application
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns></returns>
        virtual DaVinciStatus StopActivity(const string &packageName) = 0;

        /// <summary>
        /// Clear application data
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns></returns>
        virtual DaVinciStatus ClearAppData(const string &packageName) = 0;

        virtual string GetLaunchActivityByQAgent(const string &packageName) = 0;

        /// <summary>
        /// Init the system information of the target device
        /// Failed if both output are 0.
        /// Called when device is connected.
        /// </summary>
        virtual void GetSystemInformation() = 0;

        /// <summary>
        /// click on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation"></param>
        /// <returns></returns>
        virtual DaVinciStatus Click(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait, bool tapmode = false) = 0;
        /// <summary>
        /// double click on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation"></param>
        /// <returns></returns>
        // virtual DaVinciStatus DoubleClick(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) = 0;

        /// <summary>
        /// touch down on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchDown(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) = 0;

        /// <summary>
        /// touch up on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchUp(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) = 0;

        /// <summary>
        /// touch move on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchMove(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) = 0;

        /// <summary> Send keyboard event to the device </summary>
        ///
        /// <param name="isKeyDown"> true if key down. </param>
        /// <param name="keyCode">   The key code. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Keyboard(bool isKeyDown, int keyCode) = 0;

        /// <summary> Get the current orientation of the device. </summary>
        ///
        /// <param name="isRealTime">
        /// (Optional) true if we want to get the real-time orientation. Or don't care.
        /// The implemenation can choose to always get the real-time orientation regardless of this flag.
        /// </param>
        ///
        /// <returns> The orientation. </returns>
        virtual Orientation GetDefaultOrientation(bool isRealTime = false) = 0;

        /// <summary> Get the current orientation of the device. </summary>
        ///
        /// <param name="isRealTime">
        /// (Optional) true if we want to get the real-time orientation. Or don't care.
        /// The implemenation can choose to always get the real-time orientation regardless of this flag.
        /// </param>
        ///
        /// <returns> The orientation. </returns>
        virtual Orientation GetCurrentOrientation(bool isRealTime = false) = 0;

        /// <summary> Set the orientation of the device. </summary>
        ///
        /// <param name="orientation"> The device orientation </param>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        virtual DaVinciStatus SetCurrentOrientation(Orientation orientation) = 0;

        /// <summary>
        /// press volume down button or make volue down
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus VolumeDown(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// press volume up button or make volume up
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus VolumeUp(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// wake up the target device
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus WakeUp(ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;

        /// <summary>
        /// Turn down the brightness of the device
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus BrightnessAction(ButtonEventMode mode = ButtonEventMode::DownAndUp, Brightness actionMode = Brightness::Up) = 0;

        /// <summary>
        /// Show the lotus image for calibration
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus ShowLotus() = 0;

        /// <summary>
        /// Hide the lotus image for calibration
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus HideLotus() = 0;

        /// <summary>
        /// Type a string on the device
        /// </summary>
        /// <param name="str">the typed string</param>
        /// <returns></returns>
        virtual DaVinciStatus TypeString(const string &str) = 0;

        /// <summary>
        /// Clear the string
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus ClearInput() = 0;

        /// <summary>
        /// press enter
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressEnter() = 0;

        /// <summary>
        /// press delete
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressDelete() = 0;

        /// <summary>
        /// press search
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressSearch() = 0;

        virtual string GetTopPackageName() = 0;

        virtual string GetTopActivityName() = 0;

        virtual string GetPackageVersion(const string &packageName) = 0;

        virtual string GetPackageTopInfo(const string &packageName) = 0;

        virtual vector<string> GetTopActivityViewHierarchy()  = 0;

        virtual vector<string> GetAllInstalledPackageNames() = 0;

        virtual vector<string> GetSystemPackageNames() = 0;

        virtual bool CheckSurfaceFlingerOutput(string matchStr) = 0;

        virtual string GetHomePackageName() = 0;

        /// <summary>
        /// Mark a series of actions(TouchDown/TouchMove/TouchUp) as swipe action.
        /// </summary>
        virtual void MarkSwipeAction() = 0;

        /// <summary>
        /// This API will send drag commands to agent server assume phone in vertical mode.
        /// The agent server will do drag action from (x1,y1) to (x2,y2) in the phone screen. 
        /// </summary>
        /// <param name="x1">Start position in x-axis</param>
        /// <param name="y1">Start position in y-axis</param>
        /// <param name="x2">End position in x-axis</param>
        /// <param name="y2">End position in y-axis</param>
        /// <returns>Return true if successful</returns>
        virtual DaVinciStatus Drag(int x1, int y1, int x2, int y2, Orientation orientation = Orientation::Portrait, bool tapmode = false) = 0;

        /// <summary> Searches for the process running on Android device. </summary>
        ///
        /// <param name="name"> The name. </param>
        ///
        /// <returns> The found android process. </returns>
        virtual int FindProcess(const string &name) = 0;

        /// <summary> Kill android process with specified signal </summary>
        ///
        /// <param name="name"> The name. </param>
        /// <param name="sig">  The signal. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus KillProcess(const string &name, int sig = 9) = 0;

        /// <summary> Gets CPU arch of the target device </summary>
        ///
        /// <returns> The CPU arch. </returns>
        virtual string GetCpuArch() = 0;

        /// <summary> Gets CPU Info of the target device </summary>
        ///
        /// <returns> The CPU Info. </returns>
        virtual bool GetCpuInfo(string &strProcessorName, unsigned int &dwMaxClockSpeed) = 0;

        /// <summary> Gets operating system version. </summary>
        ///
        /// <returns> The operating system version. </returns>
        virtual string GetOsVersion(bool isRealTime = false) = 0;

        /// <summary> Gets dpi info. </summary>
        ///
        /// <returns> The dpi info. </returns>
        virtual double GetDPI() = 0;

        /// <summary> Get system prop info </summary>
        ///
        /// <param name="propName">   The prop name. </param>
        /// <returns>  system prop info </returns>
        virtual string GetSystemProp(string propName) = 0;

        /// <summary> Gets screen capture. </summary>
        ///
        /// <returns> The screen capture. Returns empty mat if not available. </returns>
        virtual Mat GetScreenCapture(bool needResize = true) = 0;

        virtual bool IsAgentConnected() = 0;

        /// <summary> 
        /// Check current action whether is click action
        /// </summary>
        ///
        /// <param name="touchDownX"> X of touch down point. </param>
        /// <param name="touchDownY"> Y of touch down point. </param>
        /// <param name="touchDownTimeStamp"> Timestamp of touch down point. </param>
        /// <param name="touchUpX"> X of touch up point. </param>
        /// <param name="touchUpY"> Y of touch up point. </param>
        /// <param name="touchUpTimeStamp"> Timestamp of touch up point. </param>
        ///
        /// <returns> Whether current action is click or not. </returns>
        static bool IsClickAction(int touchDownX, int touchDownY, double touchDownTimeStamp,
                                  int touchUpX,   int touchUpY,   double touchUpTimeStamp);


    protected:
        /// <summary> Name of the device. </summary>
        string deviceName;
        /// <summary> The flag to note if the device is identified by name or ip/port </summary>
        bool identifiedByIp;

        /// <summary> The audio record device. </summary>
        string audioRecordDevice;
        /// <summary> The audio play device. </summary>
        string audioPlayDevice;

        /// <summary> true if multi touch supported. </summary>
        boost::atomic<bool> multiTouchSupported;

        /// <summary> true if multi layer supported. </summary>
        boost::atomic<bool> multiLayerSupported;

        /// <summary> Information describing the calibration. </summary>
        string calibrationData;
        /// <summary> The camera setting. </summary>
        string cameraSetting;

        /// <summary> Width of the device. </summary>
        boost::atomic<int> deviceWidth;
        /// <summary> Height of the device. </summary>
        boost::atomic<int> deviceHeight;

        /// <summary> CPU usage </summary>
        boost::atomic<long> deviceCpuUsage;
        /// <summary> Memory total </summary>
        boost::atomic<long> deviceMemTotal;
        /// <summary> Memory free </summary>
        boost::atomic<long> deviceMemFree;
        /// <summary> Disk total </summary>
        boost::atomic<long> deviceDiskTotal;
        /// <summary> Disk free </summary>
        boost::atomic<long> deviceDiskFree;
        /// <summary> Battery level </summary>
        boost::atomic<long> deviceBatteryLevel;

        /// <summary> The device status. </summary>
        DeviceConnectionStatus deviceStatus;

        /// <summary> The screen source. </summary>
        ScreenSource screenSource;
        /// <summary> The HW accessory controller. </summary>
        string hwAccessoryController;

        /// <summary> true if multi layer mode. </summary>
        boost::atomic<bool> defaultMultiLayerMode;

        /// <summary> true if multi layer mode. </summary>
        boost::atomic<bool> currentMultiLayerMode;

    };
}

#endif
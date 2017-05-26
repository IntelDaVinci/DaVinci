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

#ifndef __ANDROID_TARGET_DEVICE_HPP__
#define __ANDROID_TARGET_DEVICE_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include <queue>
#include <string>
#include <iostream>

#include "boost/asio.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"

#include "TargetDevice.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "ViewHierarchyParser.hpp"
#include "AppUtil.hpp"

namespace DaVinci
{
    using namespace std;
    using boost::asio::ip::tcp;

    class DeviceManager;

    class AndroidTargetDevice : public TargetDevice
    {
        friend class DeviceManager;
    public:

        enum AppInfo
        {
            PACKAGE_NAME = 0,
            ACTIVITY_NAME = 1,
            APP_NAME = 2, 
            APP_INFO_SIZE = 3
        };

        enum BarStyle
        {
            Navigationbar = 0,
            StatusBar = 1,
            InputMethod = 2,
            FocusedWindow
        };

        struct WindowBarInfo
        {
            Rect statusbarRect;
            Rect navigationbarRect;
            Rect focusedWindowRect;
            bool inputmethodExist;
            bool isPopUpWindow;

            WindowBarInfo()
            {
                statusbarRect = EmptyRect;
                navigationbarRect = EmptyRect;
                focusedWindowRect = EmptyRect;
                inputmethodExist = false;
                isPopUpWindow = false;
            }
        };

        static const unsigned int Q_COORDINATE_X = 4096;
        static const unsigned int Q_COORDINATE_Y = 4096;

        /// <summary> Gets adb path. </summary>
        ///
        /// <returns> The adb path. </returns>
        static string GetAdbPath();

        /// <summary> Gets aapt path. </summary>
        ///
        /// <returns> The aapt path. </returns>
        static string GetAaptPath();

        /// <summary> Gets QAgent path. </summary>
        ///
        /// <returns> The QAgent path. </returns>
        static string GetQAgentPath();

        /// <summary> Adb command without providing a device </summary>
        ///
        /// <param name="adbArgs"> The adb arguments. </param>
        /// <param name="outStr">  [in,out] (Optional) (Optional) the out string. </param>
        /// <param name="timeout"> (Optional) the timeout. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        static DaVinciStatus AdbCommandNoDevice(const string &adbArgs, const boost::shared_ptr<ostringstream> &outStr = nullptr, int timeout = 0);

        /// <summary> Get device name list with adb devices command. </summary>
        ///
        /// <param name="deviceNames"> [in,out] List of names of the devices. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        static DaVinciStatus AdbDevices(vector<string> &deviceNames);

        /// <summary>
        /// Get package name and launch activity name
        /// </summary>
        /// <param name="apk_location">apk_location</param>
        /// <returns>(package_name, activity_name, icon_name)</returns>
        static vector<string> GetPackageActivity(const string &apkLocation);

        /// <summary> Map API level to version. </summary>
        ///
        /// <param name="sdk"> The API level </param>
        ///
        /// <returns> Android version string. Empty string when api level is not supported. </returns>
        static string ApiLevelToVersion(int apiLevel);

        explicit AndroidTargetDevice(const string &name, bool isIpAddress = false);
        virtual ~AndroidTargetDevice()
        {
        }

        /// <summary> Install DaVinci Monkey </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InstallDaVinciMonkey();

        /// <summary> Get the geometry of specific window. </summary>
        ///
        /// <param name="pt"> The left top point of Status bar </param>
        /// <param name="width"> The width of Status bar </param>
        /// <param name="height"> The height of status bar </param>
        /// <param name="deviceSize"> The device size </param>
        /// <param name="barOrientation"> The bar orientation </param>
        /// <param name="frameWidth"> The frame width </param>
        ///
        /// <returns> Special window rect of final picture in DVC host. </returns>
        static Rect GetWindowRectangle(const Point &pt, int width, int height, const Size &deviceSize, const Orientation &barOrientation, int frameWidth = 1280);

        static std::unordered_map<string, string> GetWindows(vector<string>& lines, string& focusWindow);

        static bool IsWindowChanged(vector<string>& window1Lines, vector<string>& window2Lines);
	
        /// <summary> Get the transformed rectangle, it's more genetic than previous one.</summary>
        ///
        /// <param name="rect"> The rect needs to be transformed. </param>
        /// <param name="resultSz"> The size of the finished image </param>
        /// <param name="originSz"> The size of original image </param>
        /// <param name="orientation"> The current orientation of the device </param>
        /// <returns> Rectangle transformed from rect. </returns>        
        static Rect GetTransformedRectangle(const Rect& rect, const Size& resultSz, const Size& originSz, const Orientation &orientation);

        /// <summary> Connect the agent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Connect() override;

        /// <summary> Disconnects the agent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Disconnect() override;

        /// <summary> Get status bar height </summary>
        ///
        /// <returns> Device status bar height. </returns>
        virtual vector<int> GetStatusBarHeight() override;

        /// <summary> Get action bar height </summary>
        ///
        /// <returns> Device action bar height. </returns>
        virtual int GetActionBarHeight() override;

        /// <summary> Get navigatin bar height </summary>
        ///
        /// <returns> Navigation bar height. </returns>
        virtual vector<int> GetNavigationBarHeight() override;

        /// <summary> Get display language </summary>
        ///
        /// <returns>  display language. </returns>
        virtual string GetLanguage(bool isRealTime = false) override;

        /// <summary>
        /// This API will send press HOME or Win command to agent server.
        /// The agent server will do press HOME action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressHome(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// Send press power button
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressPower(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// This API will send press BACK command to agent server.
        /// The agent server will do press BACK action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressBack(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// This API will send press MENU command to agent server.
        /// The agent server will do press MENU action in the phone.
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressMenu(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// Gets the name of the top package.
        /// </summary>
        /// <returns></returns>
        virtual string GetTopPackageName() override;

        /// <summary>
        /// Gets the name of the top activity.
        /// </summary>
        /// <returns></returns>
        virtual string GetTopActivityName() override;

        /// <summary>
        /// Gets the version of the app
        /// </summary>
        /// <returns></returns>
        virtual string GetPackageVersion(const string &packageName) override;

        /// <summary>
        /// Get package's CPU and Memory usage
        /// </summary>
        /// <param name="packageName">packageName</param>
        /// <returns>(PID PR CPU% S  #THR     VSS     RSS PCY UID      Name)</returns>
        virtual string GetPackageTopInfo(const string &packageName) override;

        /// <summary>
        /// Gets the name of the top activity view hierarchy.
        /// </summary>
        /// <returns></returns>
        virtual vector<string> GetTopActivityViewHierarchy() override;

        virtual vector<string> GetAllInstalledPackageNames() override;

        virtual vector<string> GetSystemPackageNames() override;

        virtual bool CheckSurfaceFlingerOutput(string matchStr) override;

        /// <summary>
        /// Gets the name of the home package.
        /// </summary>
        /// <returns></returns>
        virtual string GetHomePackageName() override;

        /// <summary> Installs the application described by appName. </summary>
        ///
        /// <param name="apkName"> Name of the application. It could be a path to the app file. </param>
        ///
        /// <param name="log"> Output. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus InstallAppWithLog(const string &apkName, string &log, const string &ops = "", bool toExtSD = false, bool x64 = false) override;

        /// <summary> Installs the application described by appName. </summary>
        ///
        /// <param name="apkName"> Name of the application. It could be a path to the app file. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus InstallApp(const string &apkName, const string &ops = "", bool toExtSD = false) override;

        /// <summary> Uninstall application. </summary>
        ///
        /// <param name="appName"> Name of the application. </param>
        ///
        /// <param name="log"> Output. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus UninstallAppWithLog(const string &packageName, string &log) override;

        /// <summary> Uninstall application. </summary>
        ///
        /// <param name="appName"> Name of the application. </param>
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus UninstallApp(const string &packageName) override;

        /// <summary>
        /// start an activity in a package 
        /// </summary>
        /// <param name="activityFullName">
        /// On Android, it's the package/activity to be started.
        /// On Windows tablet, the name looks like: "Microsoft.BingMaps_8wekyb3d8bbwe!AppexMaps"
        /// </param> 
        /// <returns></returns>
        virtual DaVinciStatus StartActivity(const string &activityFullName, bool syncMode = false) override;

        /// <summary>
        /// Force stop an activity or application
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns></returns>
        virtual DaVinciStatus StopActivity(const string &packageName) override;

        /// <summary>
        /// Clear application data
        /// </summary>
        /// <param name="packageName"></param>
        /// <returns></returns>
        virtual DaVinciStatus ClearAppData(const string &packageName) override;

        virtual string GetLaunchActivityByQAgent(const string &packageName) override;

        /// <summary>
        /// click on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation"></param>
        /// <returns></returns>
        virtual DaVinciStatus Click(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait, bool tapmode = true) override;
        /// <summary>
        /// double click on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation"></param>
        /// <returns></returns>
        // In Android No Double click.
        // virtual DaVinciStatus DoubleClick(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) override;

        /// <summary>
        /// touch down on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchDown(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) override;

        /// <summary>
        /// touch up on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchUp(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) override;

        /// <summary>
        /// touch move on normalized coordinate (x,y) with pointer ptr.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="ptr"></param>
        /// <param name="orientation">Portrait by default if the argument is not provided or unknown</param>
        /// <returns></returns>
        virtual DaVinciStatus TouchMove(int x, int y, int ptr = -1, Orientation orientation = Orientation::Portrait) override;

        /// <summary> Send keyboard event to the device </summary>
        ///
        /// <param name="isKeyDown"> true if key down. </param>
        /// <param name="keyCode">   The key code. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus Keyboard(bool isKeyDown, int keyCode) override;

        /// <summary>
        /// Init the system information of the target device
        /// Failed if both output are 0.
        /// Called when device is connected.
        /// </summary>
        virtual void GetSystemInformation() override;

        /// <summary> Get the default orientation of the device. </summary>
        ///
        /// <param name="isRealTime">
        /// (Optional) true if we want to get the real-time orientation. Or don't care.
        /// The implementation can choose to always get the real-time orientation regardless of this flag.
        /// </param>
        ///
        /// <returns> The orientation. </returns>
        virtual Orientation GetDefaultOrientation(bool isRealTime = false) override;

        /// <summary> Get the current orientation of the device. </summary>
        ///
        /// <param name="isRealTime">
        /// (Optional) true if we want to get the real-time orientation. Or don't care.
        /// The implemenation can choose to always get the real-time orientation regardless of this flag.
        /// </param>
        ///
        /// <returns> The orientation. </returns>
        virtual Orientation GetCurrentOrientation(bool isRealTime = false) override;

        /// <summary> Set the orientation of the device. </summary>
        ///
        /// <param name="orientation"> The device orientation </param>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        virtual DaVinciStatus SetCurrentOrientation(Orientation orientation) override;

        /// <summary>
        /// press volume down button or make volue down
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus VolumeDown(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// press volume up button or make volume up
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus VolumeUp(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// Do brightness adjustment action
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus BrightnessAction(ButtonEventMode mode = ButtonEventMode::DownAndUp, Brightness actionMode = Brightness::Up) override;

        /// <summary>
        /// wake up the target device
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus WakeUp(ButtonEventMode mode = ButtonEventMode::DownAndUp) override;

        /// <summary>
        /// Show the lotus image for calibration
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus ShowLotus() override;

        /// <summary>
        /// Hide the lotus image for calibration
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus HideLotus() override;

        /// <summary>
        /// Type a string on the device
        /// </summary>
        /// <param name="str">the typed string</param>i
        /// <returns></returns>
        virtual DaVinciStatus TypeString(const string &str) override;

        /// <summary>
        /// Clear the string
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus ClearInput() override;

        /// <summary>
        /// press enter
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressEnter() override;

        /// <summary>
        /// press delete
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressDelete() override;

        /// <summary>
        /// press search
        /// </summary>
        /// <returns></returns>
        virtual DaVinciStatus PressSearch() override;

        virtual void MarkSwipeAction() override;

        /// <summary>
        /// This API will send drag commands to agent server.
        /// The agent server will do drag action from (x1,y1) to (x2,y2) in the phone screen. 
        /// </summary>
        /// <param name="x1">Start position in x-axis</param>
        /// <param name="y1">Start position in y-axis</param>
        /// <param name="x2">End position in x-axis</param>
        /// <param name="y2">End position in y-axis</param>
        /// <param name="orientation">The orientation used to interpret the axis</param>
        /// <returns></returns>
        virtual DaVinciStatus Drag(int x1, int y1, int x2, int y2, Orientation orientation = Orientation::Portrait, bool tapmode = false) override;

        /// <summary> Searches for the process running on Android device. </summary>
        ///
        /// <param name="name"> The name. </param>
        ///
        /// <returns> The found android process. </returns>
        virtual int FindProcess(const string &name) override;

        /// <summary> Kill android process with specified signal </summary>
        ///
        /// <param name="name"> The name. </param>
        /// <param name="sig">  The signal. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        virtual DaVinciStatus KillProcess(const string &name, int sig = 9) override;

        /// <summary> Gets CPU arch of the target device </summary>
        ///
        /// <returns> The CPU arch. </returns>
        virtual string GetCpuArch() override;

        /// <summary> Gets CPU Info of the target device </summary>
        ///
        /// <returns> The CPU Info. </returns>
        virtual bool GetCpuInfo(string &strProcessorName, unsigned int &dwMaxClockSpeed);

        /// <summary> Gets operating system version. </summary>
        ///
        /// <returns> The operating system version. </returns>
        virtual string GetOsVersion(bool isRealTime = false) override;

        /// <summary> Get system prop info </summary>
        ///
        /// <param name="propName">   The prop name. </param>
        /// <returns>  system prop info </returns>
        virtual string GetSystemProp(string propName) override;

        /// <summary>
        /// Get CPU usage. 40->40%
        /// </summary>
        virtual int GetCpuUsage(bool isRealTime = false) override;

        /// <summary>
        /// Get Free memory. 256 -> 256MB
        /// </summary>
        virtual bool GetMemoryInfo(long &memtotal, long &memfree, bool isRealTime = false) override;

        /// <summary>
        /// Get free disk.1024->1024MB
        /// </summary>
        virtual bool GetDiskInfo(long &disktotal, long& diskfree, bool isRealTime = false) override;

        /// <summary>
        /// Get battery level
        /// </summary>
        virtual bool GetBatteryLevel(long &batteryLevel, bool isRealTime = false) override;

        /// <summary> Gets operating system SDK version. </summary>
        ///
        /// <returns> The operating system SDK version. </returns>
        virtual string GetSdkVersion(bool isRealTime = false);

        /// <summary>
        /// Get Sensor Data
        /// </summary>
        virtual bool GetSensorData(int type, string &data) override;

        /// <summary> Gets device Model. </summary>
        ///
        /// <returns> The device Model. </returns>
        virtual string GetModel(bool isRealTime = false);

        virtual bool IsARCdevice();

        /// <summary> Gets DPI information. </summary>
        ///
        /// <returns> DPI information. </returns>
        virtual double GetDPI();

        virtual bool IsHyperSoftCamSupported() override;

        virtual Mat GetScreenCapture(bool needResize = true) override;

        int GetQAgentPort() const;

        int GetMAgentPort() const;

        int GetHyperSoftCamPort() const;

        /// <summary> Executes the adb command operation. </summary>
        ///
        /// <param name="adbArgs">   The adb arguments. </param>
        /// <param name="outStr"> [in,out] (Optional) the output string. </param>
        /// <param name="timeout">   (Optional) the timeout. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus AdbCommand(const string &adbArgs, const boost::shared_ptr<ostringstream> &outStr = nullptr, int timeout = 0);

        /// <summary> Executes the adb push command operation. </summary>
        ///
        /// <param name="source">  Source file. </param>
        /// <param name="target">  Target file. </param>
        /// <param name="timeout"> (Optional) the timeout. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus AdbPushCommand(const string &source, const string &target, int timeout = 0);

        /// <summary> Executes the adb shell command operation. </summary>
        ///
        /// <param name="command">   The command. </param>
        /// <param name="outStr"> [in,out] (Optional) the output string. </param>
        /// <param name="timeout">   (Optional) the timeout. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus AdbShellCommand(const string &command, const boost::shared_ptr<ostringstream> &outStr = nullptr, int timeout = 0);

        /// <summary>
        /// Click off install confirm button (different package name maps different install confirm dialog)
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="topPackageName"></param>
        /// <returns></returns>
        bool ClickOffInstallConfirmButton(cv::Mat frame, string topPackageName);

        /// <summary>
        /// Main logic of click off install confirm for different device models
        /// </summary>
        /// <param name="apkName"></param>
        /// <returns></returns>
        void StartClickOffInstallConfirm(string apkName);

        /// <summary>
        /// Get data from QAgent daemon, sendStr must end with "\n"
        /// </summary>
        /// <returns>the string received</returns>
        string GetDataFromQAgent(const string &sendStr);

        /// <summary>
        /// Get value from string
        /// </summary>
        /// <param name="input"></param>
        /// <param name="valueNum"> Start from 0, usually value 0 is the command string</param>
        /// <returns></returns>
        string GetValueFromString(String input, int valueNum = 1);

        /// <summary> Query if both QAgent and MAgent are connected. </summary>
        ///
        /// <returns> true if agent connected, false if not. </returns>
        virtual bool IsAgentConnected();

        void AllocateAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList);
        void AllocateHyperSoftCamPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList);

        void InitKeyboardSpecialCharacters();
        void InitInstallationConfirmPackages();
        void InitInstallationConfirmManufacturers();

        void InitIncompatiblePackages();
        void InitSpecialActivity();
        void InitInstallNoReturnModels();

        bool MatchInstallationConfirmManufacturer(string targetStr);

        /// <summary> Check If the port is used by other app </summary>
        ///
        /// <returns> True for yes.</returns>
        bool IsDevicePortBusy(const int port);

        /// <summary> Check if device is connected to PC. </summary>
        ///
        /// <returns> true if device connected, false if not. </returns>
        bool IsDevicesAttached();

        /// <summary> Set the Monkey's resolution of the device. </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>
        bool SetMonkeyResolution();

        /// <summary>
        /// Send to QAgent daemon
        /// </summary>
        /// <returns>whether send succeeds</returns>
        DaVinciStatus SendToQAgent(const string &sendStr);

        /// <summary> Send the message buffer to MAgent </summary>
        ///
        /// <param name="buffer"> [in] The buffer. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SendToMAgent(const vector<unsigned char> &buffer);

        /// <summary>
        /// Get data from MAgent
        /// </summary>
        /// <returns>the string received</returns>
        string GetDataFromMAgent(const string &sendStr);

        /// <summary> GetOrientationFromSurfaceFlinger. </summary>
        /// Use it when QAgent not connected.
        /// <returns> orientation string. </returns>
        string GetOrientationFromSurfaceFlinger();

        /// <summary> Indicate touch move frequency whether it is high or not </summary>
        inline void ReduceTouchMoveFrequency(bool reduce)
        {
            reduceTouchMoveFrequency = reduce;
        }

        /// <summary>
        /// Get package name and launch activity name
        /// </summary>
        /// <param name="lines"> the lines from dump activity inf</param>
        /// <returns>(package_name, activity_name, icon_name)</returns>
        static bool ParsePackageActivityString(const vector<string> &dumpLines, const vector<string> &allLines, vector<string> &list);

        /// <summary> ParseCPUUsageString. </summary>
        /// <returns> true for success</returns>
        bool ParseCpuUsageString(const vector<string> &lines, ULONGLONG &totaljiffies, ULONGLONG  &workjiffies);

        /// <summary> ParsMemoryInfoString. </summary>
        /// <returns> true for success</returns>
        bool ParseMemoryInfoString(const vector<string> &lines, long &memtotal, long& memfree);

        /// <summary> ParsDiskInfoString. </summary>
        /// <returns> true for success</returns>
        bool ParseDiskInfoString(const vector<string> &lines, long &disktotal, long&diskfree);

        /// <summary> ParsDiskInfoString. </summary>
        /// <returns> true for success</returns>
        bool ParseBarHeightFromSurfaceFlingerString(const vector<std::string> &lines, const cv::Size &deviceSize, int &frameLongSide, int &statusBarHeight, int &navigationBarHeight);

        /// <summary> ParseGetCpuInfoString. </summary>
        /// <returns> true for success</returns>
        bool ParseCpuInfoString(const vector<string> &lines, string &strProcessorName, unsigned int &dwMaxClockSpeed);

        /// <summary> ParseTopActivityNameString. </summary>
        /// <returns> true for success</returns>
        bool ParseTopActivityNameString(const vector<string> &lines, string &topActivityName);

        /// <summary> ParsePackageVersionString. </summary>
        /// <returns> true for success</returns>
        bool ParsePackageVersionString(const vector<string> &lines, string &version);

        /// <summary> ParseDPIString. </summary>
        /// <returns> true for success</returns>
        bool ParseDPIString(const vector<string> &lines, string &dpi);

        /// <summary> ParsePackageTopInfoString. </summary>
        /// <returns> true for success</returns>
        bool ParsePackageTopInfoString(const vector<string> &lines, const string &packageName, string &topInfo);

        /// <summary> ParseTopActivityNameString. </summary>
        /// <returns> true for success</returns>
        bool ParseTopPackageNameString(const vector<string> &lines, string &topPackageName);

        /// <summary> ParsePackageNamesString. </summary>
        /// <returns> true for success</returns>
        bool ParsePackageNamesString(const vector<string> &lines, vector<string> &allPackagesName);

        /// <summary> ParsePackageNamesString. </summary>
        /// <returns> true for success</returns>
        bool ParseScreenCaptureString(const vector<char> &inputData, const Orientation &orientation, bool needResize, Mat &screenCapMat);

        /// <summary> ParseOrientationFromSurfaceFlingerString. </summary>
        /// <returns> true for success</returns>
        bool ParseOrientationFromSurfaceFlingerString(const vector<string> &lines, string &orientation);

        /// <summary> GetInformation of navigationbar, statusbar and inputmethod (on device). </summary>
        /// <returns> WindowBarInfo struct</returns>
        AndroidTargetDevice::WindowBarInfo GetWindowBarInfo(std::vector<std::string>& lines);

        std::vector<std::string> GetWindowBarLines();

        /// <summary> Get geometry infomation about specific window.</summary>
        /// <returns> whether found the window.</returns>
        bool GetBarInfoFromWindowService(const vector<std::string> &lines, Point &pt, int &width, int &height, bool &isPopUp, const BarStyle &barStyle);

    private:
        static const unsigned int Q_TOUCH_DOWN = 0x17;
        static const unsigned int Q_TOUCH_HDOWN = 0x18;
        static const unsigned int Q_TOUCH_UP = 0x19;
        static const unsigned int Q_TOUCH_HUP = 0x1a;
        static const unsigned int Q_TOUCH_MOVE = 0x1b;
        static const unsigned int Q_TOUCH_HMOVE = 0x1c;

        static const unsigned int Q_MULTI_DOWN = 0x21;
        static const unsigned int Q_MULTI_HDOWN = 0x22;
        static const unsigned int Q_MULTI_UP = 0x23;
        static const unsigned int Q_MULTI_HUP = 0x24;
        static const unsigned int Q_MULTI_MOVE = 0x25;
        static const unsigned int Q_MULTI_HMOVE = 0x26;

        static const unsigned int Q_SWIPE_START = 0x30;
        static const unsigned int Q_SWIPE_END = 0x3f;
        static const unsigned int Q_SWIPE_HORIZONTAL_START = 0x35;
        static const unsigned int Q_SWIPE_HORIZONTAL_END = 0x36;

        static const unsigned int Q_WAKE = 0x40;
        static const unsigned int Q_SHELL_INPUT_TEXT = 0x41;
        static const unsigned int Q_SHELL_INPUT_TAP = 0x42;
        static const unsigned int Q_SHELL_INPUT_HTAP = 0x43;
        static const unsigned int Q_SHELL_SWIPE_START = 0x44;
        static const unsigned int Q_SHELL_SWIPE_END = 0x45;
        static const unsigned int Q_SHELL_SWIPE_HORIZONTAL_START = 0x46;
        static const unsigned int Q_SHELL_SWIPE_HORIZONTAL_END = 0x47;

        static const unsigned int Q_PRESS_KEY = 0x50;
        static const unsigned int Q_KEYEVENT_DOWN = 0x00;
        static const unsigned int Q_KEYEVENT_UP = 0x01;
        static const unsigned int Q_KEYEVENT_DOWN_AND_UP = 0x02;

        //http://developer.android.com/reference/android/view/KeyEvent.html#KEYCODE_DPAD_CENTER
        static const unsigned int Q_KEYCODE_NOTSUPPORT = 0x00;
        static const unsigned int Q_KEYCODE_HOME = 0x01;
        static const unsigned int Q_KEYCODE_BACK = 0x02;
        static const unsigned int Q_KEYCODE_MENU = 0x03;
        static const unsigned int Q_KEYCODE_SEARCH = 0x04;
        static const unsigned int Q_KEYCODE_DPAD_CENTER = 0x05;
        static const unsigned int Q_KEYCODE_DPAD_UP = 0x06;
        static const unsigned int Q_KEYCODE_DPAD_DOWN = 0x07;
        static const unsigned int Q_KEYCODE_DPAD_LEFT = 0x08;
        static const unsigned int Q_KEYCODE_DPAD_RIGHT = 0x09;
        static const unsigned int Q_KEYCODE_ENTER = 0x11;
        static const unsigned int Q_KEYCODE_DEL = 0x12; 
        static const unsigned int Q_KEYCODE_POWER = 0x13;
        static const unsigned int Q_KEYCODE_VOLUME_DOWN = 0x14;
        static const unsigned int Q_KEYCODE_VOLUME_UP = 0x15;
        static const unsigned int Q_KEYCODE_WHITESPACE = 0x16;
        static const unsigned int Q_KEYCODE_APP_SWITCH = 0x17;
        static const unsigned int Q_KEYCODE_BRIGHTNESS_DOWN = 0x18;
        static const unsigned int Q_KEYCODE_BRIGHTNESS_UP = 0x19;
        static const unsigned int Q_KEYCODE_MEDIA_PLAY_PAUSE = 0x20;

        static const unsigned int Q_KEYCODE_A = 0x21;
        static const unsigned int Q_KEYCODE_B = 0x22;
        static const unsigned int Q_KEYCODE_C = 0x23;
        static const unsigned int Q_KEYCODE_D = 0x24;
        static const unsigned int Q_KEYCODE_E = 0x25;
        static const unsigned int Q_KEYCODE_F = 0x26;
        static const unsigned int Q_KEYCODE_G = 0x27;
        static const unsigned int Q_KEYCODE_H = 0x28;
        static const unsigned int Q_KEYCODE_I = 0x29;
        static const unsigned int Q_KEYCODE_J = 0x2a;
        static const unsigned int Q_KEYCODE_K = 0x2b;
        static const unsigned int Q_KEYCODE_L = 0x2c;
        static const unsigned int Q_KEYCODE_M = 0x2d;
        static const unsigned int Q_KEYCODE_N = 0x2e;
        static const unsigned int Q_KEYCODE_O = 0x2f;
        static const unsigned int Q_KEYCODE_P = 0x30;
        static const unsigned int Q_KEYCODE_Q = 0x31;
        static const unsigned int Q_KEYCODE_R = 0x32;
        static const unsigned int Q_KEYCODE_S = 0x33;
        static const unsigned int Q_KEYCODE_T = 0x34;
        static const unsigned int Q_KEYCODE_U = 0x35;
        static const unsigned int Q_KEYCODE_V = 0x36;
        static const unsigned int Q_KEYCODE_W = 0x37;
        static const unsigned int Q_KEYCODE_X = 0x38;
        static const unsigned int Q_KEYCODE_Y = 0x39;
        static const unsigned int Q_KEYCODE_Z = 0x3a;

        static const unsigned int Q_KEYCODE_0 = 0x51;
        static const unsigned int Q_KEYCODE_1 = 0x52;
        static const unsigned int Q_KEYCODE_2 = 0x53;
        static const unsigned int Q_KEYCODE_3 = 0x54;
        static const unsigned int Q_KEYCODE_4 = 0x55;
        static const unsigned int Q_KEYCODE_5 = 0x56;
        static const unsigned int Q_KEYCODE_6 = 0x57;
        static const unsigned int Q_KEYCODE_7 = 0x58;
        static const unsigned int Q_KEYCODE_8 = 0x59;
        static const unsigned int Q_KEYCODE_9 = 0x5a;

        static const unsigned int Q_KEYCODE_SPACE = 0x62;
        static const unsigned int Q_KEYCODE_GRAVE  = 0x64;
        static const unsigned int Q_KEYCODE_AT = 0x66;
        static const unsigned int Q_KEYCODE_POUND = 0x67;
        static const unsigned int Q_KEYCODE_START = 0x72;
        static const unsigned int Q_KEYCODE_NUMPAD_LEFT_PAREN = 0x73;
        static const unsigned int Q_KEYCODE_NUMPAD_RIGHT_PAREN = 0x74;
        static const unsigned int Q_KEYCODE_NUMPAD_SUBTRACT = 0x75;
        static const unsigned int Q_KEYCODE_PLUS = 0x77;
        static const unsigned int Q_KEYCODE_EQUALS = 0x78;
        static const unsigned int Q_KEYCODE_LEFT_BRACKET = 0x81;
        static const unsigned int Q_KEYCODE_RIGHT_BRACKET = 0x82;
        static const unsigned int Q_KEYCODE_BACKSLASH = 0x84;
        static const unsigned int Q_KEYCODE_SEMICOLON = 0x86;
        static const unsigned int Q_KEYCODE_APOSTROPHE = 0x88;
        static const unsigned int Q_KEYCODE_COMMA = 0x91;
        static const unsigned int Q_KEYCODE_PERIOD = 0x92;
        static const unsigned int Q_KEYCODE_NUMPAD_DIVIDE = 0x94;
        static const unsigned int Q_KEYCODE_FORWARD_DEL = 0x95;

        static const unsigned int Q_SETRESOLUTION = 0xf1;

        static const int QAgentVersion = 29;

        static const int WaitForAgentsConnectionReady = 60000;
        static const int WaitForAutoCheckConnection = 15000;
        static const int WaitForThreadReady = 5000;
        static const int WaitForQAgentStartup = 500;
        static const int WaitForQAgentReceiveThreadExit = 10000;
        static const int WaitForConnectThreadExit = 10000;
        static const int WaitForJavaTermation = 1000;
        static const int WaitForJavaReady = 2000;
        static const int WaitForSocketClose = 1000;
        static const int WaitForSocketConnection = 500;
        static const int WaitForSocketData = 1000;
        static const int WaitForQAgentExecution = 2000;
        static const int WaitForDownEvent = 50;
        static const int WaitForRunCommand = 30000; //30S
        static const int WaitForAdbInstallThread = 500;
        static const int WaitForAdbScreenCap = 30000;
        static const int WaitForStartOrientation = 1000;
        static const int WaitForHideOrientation = 500;
        static const int WaitForInstallConfirmScreenCap = 3000;
        static const int WaitForInstallConfirm = 3000;
        static const int WaitForInstallConfirmMinTimeout = 60000;
        static const int WaitForInstallConfirmMaxTimeout = 120000;
        static const int WaitForInstallApp = 600000;
        static const int WaitForStartActivity = 30000;
        static const int MaxHeartbeatCount = 3;
        static const int LongTouchTime = 300;
        static const int MinTouchMoveInterval = 10;

        static const int TouchDownEvent = 0;
        static const int TouchMoveEvent = 1;
        static const int TouchUpEvent = 2;

        static const ULONGLONG KB2MB = 1024;
        static const ULONGLONG BIT2MB = (1024*1024);
        static const ULONGLONG BIT2GB = (1024*1024*1024);

        Point swipeBegPoint;
        bool isSwipeAction;
        bool reduceTouchMoveFrequency;

        // Align installation package name 
        enum DeviceInstallMode
        {
            INSTALL_LENOVO_0 = 0,
            INSTALL_LENOVO_1 = 1,
            INSTALL_GOOGLE = 2,
            INSTALL_XIAOMI = 3,
            INSTALL_SAMSUNG = 4
        };

        vector<string> installConfirmPackages;
        vector<string> installConfirmManufacturers;
        boost::shared_ptr<boost::thread> installThread;

        vector<string> incompatiblePackages;
        map<string, string> specialPackageActivityName;

        vector<string> installNoReturnModels;

        Orientation defaultOrientation;
        Orientation currentOrientation;
        bool isDisplayLotus;
        boost::posix_time::ptime pressHomeButtonTime[2];

        std::map <char, int> androidSpecialKeycodeList;

        boost::atomic<bool> isAutoReconnect;
        boost::atomic<bool> isMAgentConnected;
        boost::atomic<bool> isQAgentConnected;
        boost::atomic<bool> isFirstTimeConnected;
        boost::atomic<bool> isAutoPerfUpdated;

        boost::atomic<int> heartBeatCount;
        boost::mutex lockOfMAgent;
        boost::mutex lockOfQAgent;
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::socket socketOfMAgent;
        boost::asio::ip::tcp::socket socketOfQAgent;
        unsigned short qagentPort;
        unsigned short magentPort;
        unsigned short hyperSoftCamPort;

        static const unsigned short MAgentDefaultPort = 31500;
        static const unsigned short QAgentDefaultPort = 31600;
        static const unsigned short DeviceQAgentPort = 31700;
        static const unsigned short DeviceMonkeyPort = 12345;

        boost::shared_ptr<boost::thread> connectThreadHandle;
        boost::shared_ptr<boost::thread> QAgentReceiveThreadHandle;

        AutoResetEvent connectThreadReadyEvent;
        AutoResetEvent connectThreadExitEvent;
        AutoResetEvent requestConnectEvent;
        AutoResetEvent completeConnectEvent;

        queue<string> QAgentReceiveQueue;
        boost::mutex lockOfQAgentReceiveQueue;
        AutoResetEvent QAgentReceiveQueueEvent;

        AutoResetEvent QAgentReceiveThreadReadyEvent;
        AutoResetEvent QAgentReceiveThreadExitEvent;

        String Language;
        String osVersion;
        String sdkVersion;
        String model;

        boost::posix_time::ptime  lastQAgentDataTime;

        struct BarHeight
        {
            int portraitStatus;
            int landscapeStatus;
            int portraitNavigation;
            int landscapeNavigation;
        };

        BarHeight barHeight;

        AndroidTargetDevice::WindowBarInfo windowBarInfo;

        /// <summary> Get Agent Ports </summary>
        /// <param name="mkyPort"></param>
        /// <param name="qgtPort"></param>
        DaVinciStatus GetAgentPorts(int &mkyPort, int &qgtPort);

        /// <summary> Stop Running MAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus StopMAgent();

        /// <summary> Start MAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus StartMAgent();

        /// <summary> Install QAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InstallQAgent();

        /// <summary> Uninstall QAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus UninstallQAgent();

        /// <summary> Start QAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus StartQAgent();

        /// <summary> Stop QAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus StopQAgent();

        /// <summary> Connect to QAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ConnectToQAgent();

        /// <summary> Connect to MAgent </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ConnectToMAgent();

        /// <summary> Check Agent Connect Status </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        bool CheckAgentsStatus();

        /// <summary> Makes a 32-bit command sent to magent </summary>
        ///
        /// <param name="command"> The command. </param>
        /// <param name="para1">   The first para. </param>
        /// <param name="para2">   The second para. </param>
        ///
        /// <returns> The command sent to magent </returns>
        unsigned int MakeMAgentCommand(unsigned int command, unsigned int para1, unsigned int para2);

        /// <summary>
        /// Check whether the coordinate is right. 
        /// If not valid coordinate, make them valid.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        void CheckCoordinate(int &x, int &y);

        /// <summary>
        /// Send command to QAgent
        /// </summary>
        /// <param name="cmd"> the command</param>
        /// <returns> whether send succeeds</returns>
        DaVinciStatus SendQAgentCommand(const string &cmd);

        /// <summary>
        /// Add data to QAgent daemon
        /// </summary>
        /// <param name="newStr"> The string for appending</param>
        /// <returns>true for success</returns>
        bool AddToQAgentQueue(string newStr);

        /// <summary>
        /// Receive some execute result from remote for console.
        /// </summary>
        /// <param name="waitTimeout"></param>
        /// <returns></returns>
        string GetFromQAgentQueue(int waitTimeout);

        /// <summary>
        /// Reset QAgent Queue, remove existing items.
        /// </summary>
        /// <returns></returns>
        void ResetQAgentQueue();

        void ConnectThreadEntry(void);

        void QAgentReceiveThreadEntry(void);

        void QAgentAutoMsgHandler(string recvStr);

        /// <summary> get the resolution data of the device. </summary>
        ///
        /// <returns> None </returns>
        void GetDeviceResolution();

        /// <summary> Check if QAgent need upgrade. </summary>
        ///
        /// <returns> true if it need upgrade. </returns>
        bool IsQAgentNeedUpgrade();

        /// <summary> Convert windows keycode to android keycode. </summary>
        ///
        /// <returns> Android keycode. </returns>
        unsigned int GetAndroidKeyCode(int winKeycode);

        /// <summary> Sets q agent port. </summary>
        ///
        /// <param name="p"> The int to process. </param>
        void SetQAgentPort(unsigned short p);

        /// <summary> Sets m agent port. </summary>
        ///
        /// <param name="p"> The int to process. </param>
        void SetMAgentPort(unsigned short p);

        /// <summary> Sets hypersoftcam port. </summary>
        ///
        /// <param name="p"> The int to process. </param>
        void SetHyperSoftCamPort(unsigned short p);

        void AllocateMAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList);
        void AllocateQAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList);

        // Measure status and navigation bar height
        void MeasureBarHeight(int frameWidth = 1280);

        bool GetBarHeightFromSurfaceFlinger(const cv::Size &deviceSize, int frameLongSide, int &statusBarHeight, int &navigationBarHeight);

        /// <summary> Stop those running incompatible packages. </summary>
        ///
        /// <returns> true if there are some. </returns>
        void StopIncompatiblePacakges();

        /// <summary> Check if QAgent receive thread exited. </summary>
        ///
        /// <returns> true if yes. </returns>
        bool WaitQAgentReceiveThreadExit();

        /// <summary> Check if connect thread exited. </summary>
        ///
        /// <returns> true if yes. </returns>
        bool WaitConnectThreadExit();

        /// <summary> Convert package name if the package name include special chars. </summary>
        ///
        /// <returns> true for success. </returns>
        static bool ConvertApkName(string &apkFullName);		


        bool GetInfoFromTargetString(const vector<std::string> &lines, const string &barstyleMatchStr, Point &pt, int &width, int &height);

        Point ConvertToDevicePoint(int x, int y, Orientation orientation);
    };
}

#endif
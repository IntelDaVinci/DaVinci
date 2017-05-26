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

#ifndef __TEST_MANAGER_HPP__
#define __TEST_MANAGER_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"

#include "boost/core/noncopyable.hpp"
#include "boost/thread/mutex.hpp"

#include "DaVinciAPI.h"
#include "TestInterface.hpp"
#include "TestGroup.hpp"
#include "DaVinciStatus.hpp"
#include "TestReport.hpp"
#include "CaptureDevice.hpp"
#include "AndroidTargetDevice.hpp"
#include "StopWatch.hpp"
#include "Diagnostic.hpp"
#include "KeywordUtil.hpp"

#include <vector>
#include <unordered_set>

namespace DaVinci
{
    using namespace std;
    typedef struct TestProjectConfigData
    {
        string baseFolder;
        string name;
        string apkName;
        string packageName;
        string activityName;
        string pushDataSource;
        string pushDataTarget;
    } TestProjectConfigData;

    typedef enum _POWER_TEST_TYPE {
        MPM      = 0,
        HARDWARE = 1,
        OFF      = 0xFF
    } POWER_TEST_TYPE;

    typedef boost::function<DaVinciStatus(const vector<string>, unsigned int &, boost::shared_ptr<TestGroup> &)> CommandHandlerCallback;

    class CommandOptions
    {
    public:
        CommandHandlerCallback handler;
        string description;

        CommandOptions()
        {
        }

        CommandOptions(CommandHandlerCallback _handler, string _description)
        {
            handler = _handler;
            description = _description;
        }
    };

    /// <summary> The singleton test manager </summary>
    class TestManager : public SingletonBase<TestManager>
    {
        friend class DeviceManager;

    public:
        class UserActionListener
        {
        public:
            virtual void OnKeyboard(bool isKeyDown, int keyCode) = 0;
            virtual void OnMouse(MouseButton button, MouseActionType actionType, int x, int y, int actionX, int actionY, int ptr, Orientation o) = 0;
            virtual void OnTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2) = 0;
            virtual void OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed) = 0;
            virtual void OnAppAction(AppLifeCycleAction action, string info1, string info2) = 0;
            virtual void OnSetText(const string &text) = 0;
            virtual void OnOcr() = 0;
            virtual void OnSwipe(SwipeAction action) = 0;
            virtual void OnButton(ButtonAction action, ButtonEventMode mode = ButtonEventMode::DownAndUp) = 0;
            virtual void OnMoveHolder(int distance) = 0;
            virtual void OnPowerButtonPusher(PowerButtonPusherAction action) = 0;
            virtual void OnUsbSwitch(UsbSwitchAction action) = 0;
            virtual void OnEarphonePuller(EarphonePullerAction action) = 0;
            virtual void OnRelayController(RelayControllerAction action) = 0;
            virtual void OnAudioRecord(bool startRecord) = 0;
        };

        static boost::mutex gpuLock;

        TestManager();

        /// <summary>
        /// Initialize DaVinci.
        /// 
        /// This interface should be called at the very beginning of the program.
        /// 
        /// It parses the command line options and initializes DaVinci accordingly. The configuration
        /// file could be given via the command line or the default configuration file Davinci.config
        /// will be used.
        /// 
        /// It records the test to run if the test is given via the option.
        /// 
        /// </summary>
        ///
        /// <param name="theDaVincHome"> The home path of DaVinci package. </param>
        /// <param name="args">       The command line args excluding executable. </param>
        ///
        /// <returns> The error status. </returns>
        DaVinciStatus Init(const string &theDaVincHome, const vector<string> &args);

        /// <summary> Shuts down DaVinci </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Shutdown();

        /// <summary>
        /// Sets up all the libraries used by DaVinci.
        /// </summary>
        ///
        /// <param name="theDaVinciHome"> The DaVinci home </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SetupHostLibs(const string &theDaVinciHome = ".");

        /// <summary> Teardown libraries used by DaVinci </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus TeardownHostLibs();

        /// <summary> Sets up the logging. </summary>
        ///
        /// <param name="theDaVinciHome"> the da vinci home. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SetupLogging(const string &theDaVinciHome = ".");

        /// <summary> Teardown logging. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus TeardownLogging();

        /// <summary> Sets up the XML library. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SetupXmlLib();

        void SetupLocale();

        /// <summary> Teardown XML library. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus TeardownXmlLib();

        /// <summary> Executes the configured test provided from command option </summary>
        ///
        /// <returns> The error status. </returns>
        DaVinciStatus StartCurrentTest();

        DaVinciStatus StartTest(const boost::shared_ptr<TestInterface> &theTest);

        DaVinciStatus StartTest(const boost::shared_ptr<TestGroup> &theTest);

        bool RunTestInit();

        bool CheckTestState();

        DaVinciStatus StopCurrentTest();

        void SetTestStatusEventHandler(TestStatusEventHandler handler);

        TestStatusEventHandler testStatusEventHandler;

        /// <summary> Gets builtin test by name. </summary>
        ///
        /// <param name="testName"> Name of the test. </param>
        ///
        /// <returns> The builtin test by name. </returns>
        boost::shared_ptr<TestGroup> GetBuiltinTestByName(const string &testName);

        /// <summary> Gets the home path of DaVinci package </summary>
        ///
        /// <returns> The home path of DaVinci package </returns>
        string GetDaVinciHome() const;

        /// <summary> Helper function to get the resource path relative to the DaVinci home. </summary>
        ///
        /// <param name="relativePath">   Relative path of the resource file. </param>
        /// <param name="theDaVinciHome"> (Optional) the DaVinci home, if empty, use the setting set previously. </param>
        ///
        /// <returns> The complete path of the resource file. </returns>
        string GetDaVinciResourcePath(const string &relativePath, const string &theDaVinciHome = "") const;

        /// <summary> Set the handler, called when there is message to show on GUI </summary>
        ///
        /// <param name="handler"> The handler. </param>
        void SetMessageEventHandler(MessageEventHandler handler);

        /// <summary> Set the handler, called when there is an image to show on ImageBox </summary>
        ///
        /// <param name="handler"> The handler. </param>
        void SetImageEventHandler(ImageEventHandler handler);

        /// <summary> Set the handler, called when need draw dot on ImageBox </summary>
        ///
        /// <param name="handler"> The handler. </param>
        void SetDrawDotHandler(DrawDotEventHandler drawHandler);

        void SetDrawLinesHandler(DrawLinesEventHandler drawHandler);

        /// <summary> Executes the tilt action. </summary>
        ///
        /// <param name="action">  The action. </param>
        /// <param name="degree1"> The first degree. </param>
        /// <param name="degree2"> The second degree. </param>
        /// <param name="speed1">  The first speed. </param>
        /// <param name="speed2">  The second speed. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnTilt(TiltAction action, int degree1, int degree2, int speed1 = 0, int speed2 = 0);

        /// <summary> Executes the Z-axis rotate action. </summary>
        ///
        /// <param name="zAxisAction"> Indicated the rotate action. </param>
        /// <param name="degree">      Indicated the degree of Z axis rotate action. </param>
        /// <param name="speed">       Indicated the speed of Z axis rotate action. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed);

        /// <summary> Execute the OPCODE action. </summary>
        ///
        /// <param name="action">  The action. </param>
        /// <param name="info1"> The 1st parameter that pass to the OnAppAction. </param>
        /// <param name="info2"> The 2nd parameter that pass to the OnAppAction. 
        /// This one is use for passing Target path for push data action. </param>
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnAppAction(AppLifeCycleAction action, string info1, string info2);

        /// <summary> Executes the move holder action. </summary>
        ///
        /// <param name="distance"> The distance, positive up, negative down. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnMoveHolder(int distance);

        /// <summary> Executes the actions of power button pusher. </summary>
        ///
        /// <param name="action"> The action pattern. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnPowerButtonPusher(PowerButtonPusherAction action);

        /// <summary> Executes the actions of USB switch. </summary>
        ///
        /// <param name="action"> The action pattern. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnUsbSwitch(UsbSwitchAction action);

        /// <summary> Executes the actions of earphone puller. </summary>
        ///
        /// <param name="action"> The action pattern. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnEarphonePuller(EarphonePullerAction action);

        /// <summary> Executes the actions of relay controller. </summary>
        ///
        /// <param name="action"> The action pattern. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnRelayController(RelayControllerAction action);

        /// <summary> Executes the swipe action. </summary>
        ///
        /// <param name="action"> The action. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnSwipe(SwipeAction action);

        /// <summary> Executes the button action action. </summary>
        ///
        /// <param name="action"> The action. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnButtonAction(ButtonAction action, ButtonActionType mode = ButtonActionType::DownAndUp);

        /// <summary> Executes the mouse action action. </summary>
        ///
        /// <param name="imageBoxInfo"> Information describing the image box. </param>
        /// <param name="action">       The action. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnMouseAction(const ImageBoxInfo *imageBoxInfo, const MouseAction *action);

        /// <summary> Executes the set text action. </summary>
        ///
        /// <param name="text"> The text. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnSetText(const string &text);

        /// <summary> Executes the OCR action. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnOcr();

        /// <summary> Executes the audio record action. </summary>
        ///
        /// <param name="startRecord"> true if we start to record, false to stop. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus OnAudioRecord(bool startRecord);

        /// <summary> Write QScript content into QSFile. </summary>
        ///
        /// <param name="scriptFileName"> The script file name. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus WriteQSFile(const string &scriptFileName);

        /// <summary> Edit QScript. </summary>
        ///
        /// <param name="scriptFileName"> The script file name which need to be edit. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus EditQSFile(const string &scriptFileName);

        bool IsConsoleMode() const;

        DaVinciStatus TiltTo(int degree0, int degree1, int speed0 = 0, int speed1 = 0);
        DaVinciStatus TiltUp(int degree, int speed = 0);
        DaVinciStatus TiltDown(int degree, int speed = 0);
        DaVinciStatus TiltLeft(int degree, int speed = 0);
        DaVinciStatus TiltRight(int degree, int speed = 0);
        /// <summary>
        /// Control arm0/arm1 and tilt the plate to center position at (90, 90).
        /// </summary>
        /// <returns></returns>
        DaVinciStatus TiltCenter(int speed = 0);

        DaVinciStatus DragOnScreen(int x1_pos, int y1_pos, int x2_pos, int y2_pos, bool normalized);
        void NormalizeScreenCoordinate(Orientation deviceOrientation, int &x_pos, int &y_pos);
        void TransformImageBoxToDeviceCoordinate(Size deviceSize, Orientation deviceOrientation, int &x_pos, int &y_pos);

        void RegisterUserActionListener(const boost::shared_ptr<UserActionListener> listener);
        void UnregisterUserActionListener(const boost::shared_ptr<UserActionListener> listener);

        TestConfiguration GetTestConfiguration();

        DeviceOrientation GetImageBoxLayout();
        DaVinciStatus SetRotateImageBoxInfo(ImageBoxInfo *imageBoxInfoPassIn);
        void SetImageBoxInfo(ImageBoxInfo *imageBoxInfoPassIn);

        bool IsOfflineCheck() const;
        bool IsCheckFlickering() const;

        boost::shared_ptr<TestGroup> GetTestGroup();

        void ProcessWave(const boost::shared_ptr<vector<short>> &samples);

        string GetConfigureFile();
        DrawDotEventHandler drawDotHandler;
        DrawLinesEventHandler drawLinesHandler;

        bool GetStatisticsMode();

        DrawLinesEventHandler GetDrawLinesEvent();
        DrawDotEventHandler GetDrawDotsEvent();

        TestProjectConfigData tstConfig;
        DaVinciStatus SetTestProjectConfigData(const TestProjectConfig *tstProjConfig);
        TestProjectConfigData GetTestProjectConfigData();

        DaVinciStatus Snapshot(const string &imageName);

        /// <summary> The diagnostic tool. </summary>
        boost::shared_ptr<Diagnostic> systemDiagnostic;

        int GetCapResLongerSidePxLen()
        {
            return capResLongerSidePxLen;
        }

        int GetCapFPS()
        {
            return capFPS;
        }

        bool NeedRecordFrameIndex()
        {
            return recordFrameIndex;
        }

        void SetCapResLongerSidePxLen(int longerSidePxLen)
        {
            capResLongerSidePxLen = longerSidePxLen;
        }

        string GetApksPath()
        {
            return apksPlacedPath;
        }

        bool GetIdRecord()
        {
            return idRecord;
        }

        string GetLogFile()
        {
            return logFile;
        }
        // check the stage before checking group state, to avoid the cost used for lock
        enum class TestStage{
            TestStageBeforeInit = 0, // set just before intializing test.
            TestStageAfterInit = 1,  // default value, or set after initializing test.
        };

        bool FindKeyFromKeywordMap(string key);
        std::map<std::string, std::vector<Keyword>> GetValueFromKeywordMap(string key);

        void SetRecordWithID(int id);

        int GetRecordWithID();

        void InitImageBoxInfo();

        POWER_TEST_TYPE GetPowerTestType()
        {
            return powerTestType;
        }

    private:
        /// <summary>
        /// The keywords read from config.
        /// </summary>
        std::map<std::string, std::map<std::string, std::vector<Keyword>>> configKeywordMap;

        static const int defaultTiltDegree = 90;
        static const int defaultZAxisTiltDegree = 360;

        /// <summary> pattern = 1, The arduino device will Turn on No.1 Relay to plug in No.1 USB. </summary>
        static const int patternPlugInUSB1 = 1;
        /// <summary> pattern = 2, The arduino device will Turn off No.1 Relay to plug out No.1 USB. </summary>
        static const int patternPlugOutUSB1 = 2;
        /// <summary> pattern = 3, The arduino device will Turn on No.2 Relay to plug in No.2 USB. </summary>
        static const int patternPlugInUSB2 = 3;
        /// <summary> pattern = 4, The arduino device will Turn off No.2 Relay to plug out No.2 USB. </summary>
        static const int patternPlugOutUSB2 = 4;

        static const int Q_COORDINATE_X = 4096;
        static const int Q_COORDINATE_Y = 4096;

        static const string defaultDeviceConfig;
        static const string defaultKeywordConfig;
        static string testOutputDir;
        static boost::mutex singletonMutex;

        static void UpdateText(const string &msg);

        void CheckCoordinate(int &x, int &y);

        int capResLongerSidePxLen;
        int capFPS;
        bool consoleMode, statMode;
        bool recordFrameIndex;
        string apksPlacedPath;
        bool idRecord;
        boost::atomic<TestStage> currentTestStage;

        const ImageBoxInfo *imageBoxInfo;
        boost::shared_ptr<ImageBoxInfo> rotateImageBoxInfo;
        ImageEventHandler imageEventHandler;

        MessageEventHandler messageEventHandler;

        string daVinciHome;
        string deviceConfig;
        string currentDevice;
        boost::shared_ptr<AndroidTargetDevice> androidDevice;

        boost::shared_ptr<TestReport> testReport;

        boost::shared_ptr<TestGroup> currentTestGroup;

        boost::shared_ptr<boost::thread> threadInit;
        boost::mutex lockTestInit;

        boost::recursive_mutex lockOfCurTestGroup;

        boost::shared_ptr<TestGroup> tstGroup;

        unordered_set<boost::shared_ptr<UserActionListener>> userActionListeners;
        boost::mutex userActionListenersMutex;

        TestConfiguration testConfiguration;

        int recordWithId;

        /// <summary> The watch to check whether the running test times out. </summary>
        StopWatch testWatch;

        /// <summary> Path of DaVinci.log. </summary>
        string logFile;

        std::unordered_map<string, CommandOptions> supportedCommands;
        bool hasFpsCommand;

        POWER_TEST_TYPE powerTestType;

        /// <summary>
        /// Parse commandline options.
        /// 
        /// The function will set the current
        /// tests and other test states per command line options.
        /// </summary>
        ///
        /// <param name="args"> The arguments. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus ParseCommandlineOptions(const vector<string> &args);

        DaVinciStatus InitBuiltinTests();

        Mat RotateFrame(Mat & frame);

        void RecordUserMouseAction(const MouseAction * action, boost::shared_ptr<TargetDevice> curDevice, int x_pos, int y_pos);

        void DispatchUserMouse(MouseButton button, MouseActionType actionType, int x, int y, int actionX, int actionY, int ptr, Orientation o);
        void DispatchUserMoveHolder(int distance);
        void DispatchUserTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2);
        void DispatchUserZRotate(ZAXISTiltAction zAxisAction, int degree, int speed);
        void DispatchUserPowerButtonPusher(PowerButtonPusherAction action);
        void DispatchUserUsbSwitch(UsbSwitchAction action);
        void DispatchUserEarphonePuller(EarphonePullerAction action);
        void DispatchUserRelayController(RelayControllerAction action);
        void DispatchUserAppAction(AppLifeCycleAction action, string info1, string info2);
        void DispatchUserSwipe(SwipeAction action);
        void DispatchUserButton(ButtonAction action, ButtonEventMode mode);
        void DispatchUserSetText(const string &text);
        void DispatchUserOcr();
        void DispatchUserAudioRecord(bool startRecord);

        void InitTestConfiguration();

        void ProcessFrame(boost::shared_ptr<CaptureDevice> captureDevice);


        void InitCommands();
        void Usage();

        DaVinciStatus HandleLaunchTimeCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleQScriptCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleGenRnRCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleOfflineImageMatchCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleBlankPagesCheckerCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleGenerateLaunchCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleFpsCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleDeviceCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleGpuCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleTimeoutCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleNoAgentCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleCalibrateCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleDevRecoveryCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleLifterCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleTiltingCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleUsbUnplugCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleUsbPlugCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleMapPowerpusherCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleAiCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleDeviceRecordCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleIdRecordCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleStatCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleResolutionCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleNoImageShowCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandlePowerTestCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleCheckFlickerCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleGenAudioFormCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
        DaVinciStatus HandleCheckNoResponseCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group);
    };
}

#endif
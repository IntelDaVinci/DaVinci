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

#include "boost/log/trivial.hpp"
#include "boost/log/utility/setup/from_stream.hpp"
#include "boost/log/utility/setup/from_settings.hpp"
#include "boost/log/utility/setup/common_attributes.hpp"
#include "boost/log/utility/setup/formatter_parser.hpp"
#include "boost/log/utility/setup/filter_parser.hpp"
#include "boost/log/utility/setup/settings_parser.hpp"
#include "boost/log/sinks/sync_frontend.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "DaVinciCommon.hpp"
#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include "FPSMeasure.hpp"
#include "LaunchTest.hpp"
#include "TimeMeasure.hpp"
#include "ScriptReplayer.hpp"
#include "CameraCalibrate.hpp"
#include "DeviceCrashDetectTest.hpp"
#include "FfrdLifterTest.hpp"
#include "FfrdTiltingTest.hpp"
#include "UsbPlugTest.hpp"
#include "MapPowerPusher.hpp"
#include "TempleRunSVMPlay.hpp"
#include "HWAccessoryController.hpp"
#include "MainTest.hpp"
#include "LoggerProxySink.hpp"
#include "PseudoCameraCapture.hpp"
#include "OnDeviceScriptRecorder.hpp"
#include "LoginRecognizer.hpp"
#include "FileCommon.hpp"
#include "KeywordConfigReader.hpp"
#include "LaunchGenerator.hpp"
#include "FlickeringChecker.hpp"
#include "BlankPagesChecker.hpp"
#include "AudioFormGenerator.hpp"
#include "NoResponseChecker.hpp"

using namespace boost::filesystem;

namespace DaVinci
{
    const string TestManager::defaultDeviceConfig = "Davinci.config";
    const string TestManager::defaultKeywordConfig = "DaVinci_Keyword.xml";
    boost::mutex TestManager::gpuLock;
    boost::mutex TestManager::singletonMutex;

    TestManager::TestManager() : consoleMode(false), testStatusEventHandler(nullptr), currentTestStage(TestStage::TestStageAfterInit),
        imageEventHandler(nullptr), messageEventHandler(nullptr), capResLongerSidePxLen(0),
        statMode(false), idRecord(false), capFPS(0), recordFrameIndex(false),
        recordWithId(0), hasFpsCommand(false), powerTestType(POWER_TEST_TYPE::OFF)
    {
        testReport = boost::shared_ptr<TestReport>(new TestReport());
        InitImageBoxInfo();
        InitTestConfiguration();

        systemDiagnostic = boost::shared_ptr<Diagnostic>(new Diagnostic());
    }

    DaVinciStatus TestManager::Init(const string &theDaVincHome, const vector<string> &args)
    {    
        DaVinciStatus status;

        // check the DaVinciHome is valid or not
        boost::filesystem::path daVinciHomePath(theDaVincHome);
        if (boost::filesystem::exists(daVinciHomePath) == false)
        {
            DAVINCI_LOG_ERROR << "Invalid DaVinci Home: " << theDaVincHome;
            return DaVinciStatus(errc::invalid_argument);
        }

        daVinciHome = theDaVincHome;

        SetupLocale();
        status = SetupHostLibs(theDaVincHome);
        if (!DaVinciSuccess(status))
        {
            return status;
        }

        TestManager::Instance().systemDiagnostic->CheckHostInfo();

        status = ParseCommandlineOptions(args);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_DEBUG << "Error parsing command line options!";
            return status;
        }

        // TODO: define language type to make implementation independent (need OCR language configuration)
        // Load keyword from keyword configuration file
        std::string fullKeywordConfig = GetDaVinciResourcePath(defaultKeywordConfig);
        if (fullKeywordConfig != "" && boost::filesystem::is_regular_file(fullKeywordConfig))
        {
            boost::shared_ptr<KeywordConfigReader> configReader = boost::shared_ptr<KeywordConfigReader>(new KeywordConfigReader());
            configKeywordMap = configReader->ReadKeywordConfig(fullKeywordConfig);
        }

        DeviceManager& devManager = DeviceManager::Instance();

        status = devManager.InitGlobalDeviceStatus();
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Failed initialize global device management related resource:" << status.message();
            return status;
        }

        if (!deviceConfig.empty() && boost::filesystem::is_regular_file(deviceConfig))
        {
            status = devManager.LoadConfig(deviceConfig);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Failed to load device configuration " << deviceConfig << ":" << status.message();
                return status;
            }
        }

        if (consoleMode)
        {
            status = devManager.DetectDevices(currentDevice);
        }
        else
        {
            status = devManager.DetectDevices();
        }
        if (!DaVinciSuccess(status))
        {
            return status;
        }

        status = devManager.InitDevices(currentDevice);
        if (!DaVinciSuccess(status) && consoleMode)
        {
            return status;
        }

        if (args.empty())
        {
            // if no command line args are given, we assume it is GUI mode
            // we initialize all the built-in tests that might be called from GUI

            // FIXME: ignore the return value as it's not implemented.
            InitBuiltinTests();
        }

        systemDiagnostic->StartPerfMonitor();

        return status;
    }

    bool TestManager::FindKeyFromKeywordMap(string key)
    {
        return TestManager::Instance().configKeywordMap.find(key) != TestManager::Instance().configKeywordMap.end();
    }

    std::map<std::string, std::vector<Keyword>> TestManager::GetValueFromKeywordMap(string key)
    {
        return TestManager::Instance().configKeywordMap[key];
    }

    string TestManager::GetDaVinciHome() const
    {
        return daVinciHome;
    }

    string TestManager::GetDaVinciResourcePath(const string &relativePath, const string &theDaVinciHome) const
    {
        path resPath(theDaVinciHome.empty() ? GetDaVinciHome() : theDaVinciHome);
        resPath /= relativePath;
        return system_complete(resPath).string();
    }

    void TestManager::UpdateText(const string &msg)
    {
        TestManager &tm = TestManager::Instance();
        if (tm.messageEventHandler != nullptr)
        {
            tm.messageEventHandler(msg.c_str());
        }
    }

    void TestManager::CheckCoordinate(int &x, int &y)
    {
        if (x < 0)
            x = 0;
        else if (x > Q_COORDINATE_X)
            x = Q_COORDINATE_X;

        if (y < 0)
            y = 0;
        else if (y > Q_COORDINATE_Y)
            y = Q_COORDINATE_Y;
    }

    class LoggerProxySinkFactory : public boost::log::sink_factory<char>
    {
    public:
        LoggerProxySinkFactory(LoggerProxySink::LoggerProxySinkCallback cb) : callback(cb)
        {
        }

        boost::shared_ptr<boost::log::sinks::sink> create_sink(settings_section const& settings)
        {
            auto backend = boost::shared_ptr<LoggerProxySink>(new LoggerProxySink());
            backend->SetCallback(callback);
            return boost::shared_ptr<boost::log::sinks::synchronous_sink<LoggerProxySink>>(new boost::log::sinks::synchronous_sink<LoggerProxySink>(backend));
        }

    private:
        LoggerProxySink::LoggerProxySinkCallback callback;
    };

    DaVinciStatus TestManager::SetupLogging(const string &theDaVinciHome)
    {
        try
        {
            boost::log::add_common_attributes();
            boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
            boost::log::register_simple_filter_factory<boost::log::trivial::severity_level, char>("Severity");

            auto guiFactory = boost::shared_ptr<LoggerProxySinkFactory>(new LoggerProxySinkFactory(
                boost::bind(&TestManager::UpdateText, _1)));
            boost::log::register_sink_factory("GUI", guiFactory);
#ifdef _DEBUG
            ifstream logSetting(GetDaVinciResourcePath("LogSettingsDebug.ini", theDaVinciHome));
#else
            ifstream logSetting(GetDaVinciResourcePath("LogSettings.ini", theDaVinciHome));
#endif
            auto settings = boost::log::parse_settings(logSetting);
            if (settings["Sinks"] && settings["Sinks"]["TextFile"]
            && settings["Sinks"]["TextFile"]["FileName"])
            {
                auto val = settings["Sinks"]["TextFile"]["FileName"].get();
                boost::filesystem::path logPath(*val);
                if (!logPath.is_absolute())
                {
                    logPath = boost::filesystem::system_complete(logPath);
                }
                logFile = logPath.string();
            }
            boost::log::init_from_settings(settings);
        }
        catch (...)
        {
            DAVINCI_LOG_INFO << "Unable to load LogSettings.ini, use default logging setting!";
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::TeardownLogging()
    {
        boost::log::core::get()->flush();
        boost::log::core::get()->remove_all_sinks();
        logFile = "";
        return DaVinciStatusSuccess;
    }

    void TestManager::SetupLocale()
    {
        boost::filesystem::path::imbue(std::locale());
    }

    DaVinciStatus TestManager::SetupXmlLib()
    {
        try
        {
            xercesc::XMLPlatformUtils::Initialize();
            return DaVinciStatusSuccess;
        }
        catch (const xercesc::XMLException &)
        {
            return errc::operation_not_supported;
        }
    }

    DaVinciStatus TestManager::TeardownXmlLib()
    {
        xercesc::XMLPlatformUtils::Terminate();
        return DaVinciStatusSuccess;
    }

    void TestManager::Usage()
    {
        boost::log::core::get()->flush();

        DAVINCI_LOG_INFO << "\nRun DaVinci tests/scripts automatically.\n";
        map<string, CommandOptions> orderedCommands(supportedCommands.begin(), supportedCommands.end());
        for (auto it = orderedCommands.begin(); it != orderedCommands.end(); ++it)
        {
            DAVINCI_LOG_INFO << setw(20) << left << it->first << it->second.description;
        }
    }

    DaVinciStatus TestManager::HandleLaunchTimeCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
            return errc::invalid_argument;

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        string iniFile = args[argIndex + 1];
        if (!is_regular_file(iniFile))
        {
            DAVINCI_LOG_ERROR << string("ini file doesn't exist: ") << iniFile ;
            return errc::invalid_argument;
        }
        DeviceManager::Instance().EnableTargetDeviceAgent(false);

        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(boost::shared_ptr<TimeMeasure>(new TimeMeasure(iniFile)), true);
        group->AddTest(boost::shared_ptr<LaunchTest>(new LaunchTest(iniFile)));

        DAVINCI_LOG_INFO << string("Running Launchtime test: ");
        DAVINCI_LOG_INFO << iniFile ;
        testReport->SetReportType(DavinciLogType::LaunchTime);
        testReport->PrepareQScriptReplay(iniFile);

        // Hard code 12FPS(Experience Value) for Launch Time Test. In next step, we will expose it as a configuration.
        capFPS = 12;

        // Record frame index and corresponding time stamp on device side
        recordFrameIndex = true;

        argIndex += 2;

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleQScriptCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
            return errc::invalid_argument;

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        string scriptFile = args[argIndex + 1];

        if (!is_regular_file(scriptFile))
        {
            DAVINCI_LOG_ERROR << string("Script file doesn't exist: ") << scriptFile;
            return errc::invalid_argument;
        }

        boost::shared_ptr<TestInterface> test = nullptr;
        if (boost::iends_with(scriptFile, ".qs"))
        {
            test = boost::shared_ptr<ScriptReplayer>(new ScriptReplayer(scriptFile));
            DAVINCI_LOG_INFO << string("Playing a QS script: ");
            DAVINCI_LOG_INFO << scriptFile;
        }
        else if (boost::iends_with(scriptFile, ".xml"))
        {
            test = boost::shared_ptr<MainTest>(new MainTest(scriptFile));
            DAVINCI_LOG_INFO << string("Run a main test: ");
            DAVINCI_LOG_INFO << scriptFile;
        }
        else
        {
            DAVINCI_LOG_ERROR << scriptFile << string("is not a qscript") ;
            return errc::invalid_argument;
        }

        if (test != nullptr)
        {
            group = boost::shared_ptr<TestGroup>(new TestGroup());
            group->AddTest(test, true);
        }

        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleGenRnRCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
            return errc::invalid_argument;

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        string sequenceFile = args[argIndex + 1];
        if(boost::filesystem::exists(sequenceFile) == false)
        {
            return DaVinciStatus(errc::file_exists);
        }

        if(ScriptHelper::GenActionSequenceToRnR(sequenceFile) == false)
        {
            DAVINCI_LOG_INFO << "Generate RnR from images: FAIL";
            return DaVinciStatus(errc::bad_message);
        }

        argIndex += 2;
        DAVINCI_LOG_INFO << "Generate RnR from images: SUCCESS";
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleOfflineImageMatchCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        DAVINCI_LOG_INFO << string("Offline image match");
        testConfiguration.offlineConcurrentMatch = 1;
        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleBlankPagesCheckerCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> &group)
    {
        DAVINCI_LOG_INFO<< string("Blank pages check for replay video");
        if (argIndex + 1 >= args.size())
        {
            return DaVinciStatus(errc::invalid_argument);
        }
        if(group != nullptr)
        {
            return errc::address_in_use;
        }
        string replayVideoFile = args[argIndex + 1];
        
        if (!is_regular_file(replayVideoFile))
        {
            DAVINCI_LOG_ERROR << string("Replay video file doesn't exist: ") << replayVideoFile;
            return DaVinciStatus(errc::invalid_argument);
        }

        if (boost::iends_with(replayVideoFile, ".avi"))
        {
            DeviceManager::Instance().DetectDevices();
            DeviceManager::Instance().InitTargetDevice(currentDevice);
            boost::shared_ptr<TargetDevice> device = DeviceManager::Instance().GetCurrentTargetDevice();
            if (device != nullptr)
            {
                boost::shared_ptr<BlankPagesChecker> blankFrmsCheck = boost::shared_ptr<BlankPagesChecker>(new BlankPagesChecker());
                blankFrmsCheck->HasVideoBlankFrames(replayVideoFile);
            }
            DeviceManager::Instance().CloseDevices();
        }
        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleGenerateLaunchCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        DAVINCI_LOG_INFO << string("Generate launch time test case");
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        string packageActivity = args[argIndex + 1];
        int seconds = 60;

        if (argIndex + 2 >= args.size())
        {
            argIndex += 2;
        }
        else
        {
            if (!TryParse(args[argIndex + 2], seconds) || seconds <= 0)
            {
                seconds = 60;
                argIndex += 2;
            }
            else
            {
                argIndex += 3;
            }
        }

        boost::shared_ptr<TestInterface> test = boost::shared_ptr<LaunchGenerator>(
            new LaunchGenerator(packageActivity, seconds));
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleFpsCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        hasFpsCommand = true;
        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleDeviceCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        currentDevice = args[argIndex + 1];
        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleCheckFlickerCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (args.size() == 2)
        {
            if (argIndex + 1 >= args.size())
            {
                return errc::invalid_argument;
            }

            string currentVideo = args[argIndex + 1];
            if (boost::iends_with(currentVideo, ".avi") && is_regular_file(currentVideo))
            {
                boost::shared_ptr<FlickeringChecker> flickeringChecker = boost::shared_ptr<FlickeringChecker>(new FlickeringChecker());
                flickeringChecker->VideoQualityEvaluation(currentVideo);
                argIndex += 2;

                //For exit directly without checking other things
                const int DaVinciStatusFinished = 1;
                return DaVinciStatusFinished;
            }
            else
            {
                return errc::file_exists;
            }
        }
        else if (args.size() >= 3)
        {
            DAVINCI_LOG_INFO << string("Check flickering");
            testConfiguration.checkflicker = BoolTrue;
            argIndex += 1;
            return DaVinciStatusSuccess;
        }

        return errc::invalid_argument;
    }

    DaVinciStatus TestManager::HandleGenAudioFormCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }
        if (group != nullptr)
        {
            return errc::address_in_use;
        }
        string audioFile = args[argIndex + 1];
        boost::shared_ptr<AudioFormGenerator> audioFormGen = boost::shared_ptr<AudioFormGenerator>(new AudioFormGenerator());
        audioFormGen->SaveAudioWave(audioFile);
        audioFormGen->CheckQualityResult(audioFile);
        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleCheckNoResponseCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        string replayVideoFileName = args[argIndex + 1];
        if(!boost::filesystem::exists(replayVideoFileName))
        {
            return errc::invalid_argument;
        }

        string qfdFileName;
        string qsFile;

        if(replayVideoFileName.find(".avi") != string::npos)
        {
            qfdFileName = replayVideoFileName.substr(0, replayVideoFileName.find(".avi")) + ".qfd" ;
            qsFile = replayVideoFileName.substr(0, replayVideoFileName.find(".avi")) + ".qs" ;

            if(!boost::filesystem::exists(qfdFileName) || !boost::filesystem::exists(qsFile))
            {
                return errc::invalid_argument;
            }
        }
        else
        {
            return errc::invalid_argument;
        }

              
        boost::shared_ptr<NoResponseChecker> noResponseChecker = boost::shared_ptr<NoResponseChecker>(new NoResponseChecker(replayVideoFileName, qfdFileName, qsFile));
        noResponseChecker->HasNoResponse();
        argIndex += 1;
        
        //For exit directly without checking other things
        const int DaVinciStatusFinished = 1;
        return DaVinciStatusFinished;
    }

    DaVinciStatus TestManager::HandleGpuCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (DeviceManager::Instance().HasCudaSupport())
        {
            DeviceManager::Instance().EnableGpuAcceleration(true);
            DAVINCI_LOG_INFO << DeviceManager::Instance().QueryGpuInfo();
        }
        else
        {
            DAVINCI_LOG_INFO << string("Cpu Acceleration") ;
        }
        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleTimeoutCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        int seconds = 0;
        if (!TryParse(args[argIndex + 1], seconds) || seconds < 0)
        {
            DAVINCI_LOG_ERROR << string("Invalid parameter: ") << args[argIndex + 1] ;
            return errc::invalid_argument;
        }
        else
        {
            DAVINCI_LOG_INFO << string("Set timeout for test: ") << seconds ;
            testConfiguration.timeout = seconds;
        }

        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleNoAgentCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        DeviceManager::Instance().EnableTargetDeviceAgent(false);
        DeviceManager::Instance().SetForceFFRDDetect(true);
        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleCalibrateCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        int calibrateType = 0;
        string calibParam = args[argIndex + 1];
        if (!TryParse(calibParam, calibrateType))
        {
            DAVINCI_LOG_ERROR << string("Wrong parameter for calibration test: ") << calibParam ;
            return errc::invalid_argument;
        }

        if (calibrateType < static_cast<int>(CameraCalibrationType::MinCalibrateType) ||
            calibrateType > static_cast<int>(CameraCalibrationType::MaxCalibrateType) ||
            calibrateType == static_cast<int>(CameraCalibrationType::Unsupported))
        {
            DAVINCI_LOG_ERROR << string("Undefined calibration type");
            return errc::invalid_argument;
        }

        boost::shared_ptr<TestInterface> test = boost::shared_ptr<CameraCalibrate>(new CameraCalibrate(static_cast<CameraCalibrationType>(calibrateType)));

        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        DAVINCI_LOG_INFO << string("Running calibration test: ") << calibParam;
        testReport->SetReportType(DavinciLogType::Calibration);

        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleDevRecoveryCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        DAVINCI_LOG_INFO << string("Start handling devrecovery ...\n");
        boost::shared_ptr<TestInterface> test = boost::shared_ptr<DeviceCrashDetectTest>(new DeviceCrashDetectTest());
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleLifterCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        boost::shared_ptr<TestInterface> test = boost::shared_ptr<FfrdLifterTest>(new FfrdLifterTest());
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        DAVINCI_LOG_INFO << string("Running FFRD Lifter test!\n");

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleTiltingCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        boost::shared_ptr<TestInterface>  test = boost::shared_ptr<FfrdTiltingTest>(new FfrdTiltingTest());
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        DAVINCI_LOG_INFO << string("Running FFRD Tilting test!\n");

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleUsbUnplugCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        DeviceManager::Instance().SetForceFFRDDetect(true);
        boost::shared_ptr<TestInterface> test = boost::shared_ptr<UsbPlugTest>(new UsbPlugTest(UsbPlugTest::UsbUnplug));
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);
        DAVINCI_LOG_INFO << string("Running USB Unplug test!\n");

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleUsbPlugCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        DeviceManager::Instance().SetForceFFRDDetect(true);
        boost::shared_ptr<TestInterface> test = boost::shared_ptr<UsbPlugTest>(new UsbPlugTest(UsbPlugTest::UsbPlug));
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);
        DAVINCI_LOG_INFO << string("Running USB Plug test!\n");

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleMapPowerpusherCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        DeviceManager::Instance().SetForceFFRDDetect(true);
        boost::shared_ptr<TestInterface> test = boost::shared_ptr<MapPowerPusher>(new MapPowerPusher());
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);
        DAVINCI_LOG_INFO << string("Mapping power pusher with mobile device!\n");

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleAiCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        if (argIndex + 1 >= args.size())
        {
            return errc::invalid_argument;
        }

        if (group != nullptr)
        {
            return errc::address_in_use;
        }

        boost::shared_ptr<TestInterface> test;
        bool bInvalidParam = false;
        if (args[argIndex + 1] == "templerun_human_train_SVM")
        {
            test = boost::shared_ptr<TempleRunSVMPlay>(new TempleRunSVMPlay(GetDaVinciResourcePath("examples/templerun/TempleRunHOGSVM.ini")));
        }
        else if (args[argIndex + 1] == "templerun_human_train_SVM_tilt")
        {
            test = boost::shared_ptr<TempleRunSVMPlay>(new TempleRunSVMPlay(GetDaVinciResourcePath("examples/templerun/TempleRunSVM_WithTilt.ini")));
        }
        else
        {
            bInvalidParam = true;
        }

        if (bInvalidParam == false)
        {
            DAVINCI_LOG_INFO << string("Running AI test: ") << args[argIndex + 1] ;
            testReport->SetReportType(DavinciLogType::AI);
        }
        else
        {
            DAVINCI_LOG_ERROR << string("Unsupported parameter for AI test: ") << args[argIndex + 1] ;
            return errc::invalid_argument;
        }


        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        argIndex += 2;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandlePowerTestCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        argIndex += 1;
        if (argIndex < args.size())
        {
            std::string powerTypeStr = args[argIndex];
            boost::to_upper(powerTypeStr);
            if (powerTypeStr == "MPM")
            {
                powerTestType = POWER_TEST_TYPE::MPM;
            }
            else
            {
                return errc::not_supported;
            }

            argIndex += 1;
            return DaVinciStatusSuccess;
        }
        else
        {
            return errc::invalid_argument;
        }
    }

    DaVinciStatus TestManager::HandleDeviceRecordCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        argIndex += 1;
        if (argIndex < args.size())
        {
            apksPlacedPath = args[argIndex];
            if (!boost::filesystem::is_directory(apksPlacedPath))
            {
                DAVINCI_LOG_ERROR << "The folder (" << apksPlacedPath <<") is invalid.";
                return errc::invalid_argument;
            }
            argIndex += 1;
        }
        testConfiguration.suppressImageBox = BoolTrue;
        boost::shared_ptr<TestInterface> test = boost::shared_ptr<OnDeviceScriptRecorder>(new OnDeviceScriptRecorder());
        group = boost::shared_ptr<TestGroup>(new TestGroup());
        group->AddTest(test, true);

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleIdRecordCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        DAVINCI_LOG_INFO << string("Record script with id") << std::endl;
        argIndex += 1;
        idRecord = true;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleStatCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        DAVINCI_LOG_INFO << string("run program to get delay time of each operation") << std::endl;
        argIndex += 1;
        consoleMode = false;
        statMode = true;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleResolutionCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        argIndex += 1;
        if (argIndex == args.size())
            return errc::invalid_argument;

        if ((TryParse(args[argIndex], capResLongerSidePxLen) == false)
            || (capResLongerSidePxLen <= 0))
        {
            DAVINCI_LOG_ERROR << "Invalid Capture resolution:" << args[argIndex];
            return errc::invalid_argument;
        }

        argIndex += 1;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::HandleNoImageShowCommand(const vector<string> args, unsigned int & argIndex, boost::shared_ptr<TestGroup> & group)
    {
        argIndex += 1;
        if (argIndex == args.size())
            return errc::invalid_argument;
        testConfiguration.suppressImageBox = BoolTrue;
        return DaVinciStatusSuccess;
    }

    void TestManager::InitCommands()
    {
        if (supportedCommands.size() > 0)
            return;

        supportedCommands["-l"] = CommandOptions(boost::bind(&TestManager::HandleLaunchTimeCommand, this, _1, _2, _3), "Run a launch-time test script");
        supportedCommands["-p"] = CommandOptions(boost::bind(&TestManager::HandleQScriptCommand, this, _1, _2, _3), "Replay a test script");
        supportedCommands["-g"] = CommandOptions(boost::bind(&TestManager::HandleGenRnRCommand, this, _1, _2, _3), "Generate RnR script");
        supportedCommands["-offline"] = CommandOptions(boost::bind(&TestManager::HandleOfflineImageMatchCommand, this, _1, _2, _3), "Run launch time test under offline mode");
        supportedCommands["-offlineimagematch"] = CommandOptions(boost::bind(&TestManager::HandleOfflineImageMatchCommand, this, _1, _2, _3), "Same as -offline");
        supportedCommands["-checkblankpage"] = CommandOptions(boost::bind(&TestManager::HandleBlankPagesCheckerCommand, this, _1, _2, _3), "Check blank pages in whole replay video");
        supportedCommands["-generate"] = CommandOptions(boost::bind(&TestManager::HandleGenerateLaunchCommand, this, _1, _2, _3), "Generate launch time test case");
        supportedCommands["-fps"] = CommandOptions(boost::bind(&TestManager::HandleFpsCommand, this, _1, _2, _3), "Measure FPS value for the specified test");
        supportedCommands["-device"] = CommandOptions(boost::bind(&TestManager::HandleDeviceCommand, this, _1, _2, _3), "Specify the DaVinci platform to run tests");
        supportedCommands["-gpu"] = CommandOptions(boost::bind(&TestManager::HandleGpuCommand, this, _1, _2, _3), "Run the test on GPU(CUDA) if possible");
        supportedCommands["-timeout"] = CommandOptions(boost::bind(&TestManager::HandleTimeoutCommand, this, _1, _2, _3), "Run tests and stop them at specified timeout (0 is to run infinite time.");
        supportedCommands["-noagent"] = CommandOptions(boost::bind(&TestManager::HandleNoAgentCommand, this, _1, _2, _3), "Run DaVinci without connecting device agent");
        supportedCommands["-c"] = CommandOptions(boost::bind(&TestManager::HandleCalibrateCommand, this, _1, _2, _3), "Run a calibration");
        supportedCommands["-devrecovery"] = CommandOptions(boost::bind(&TestManager::HandleDevRecoveryCommand, this, _1, _2, _3), "Run devrecovery test alone. To reboot/power-on device by HW power button while crash/power-off.");
        supportedCommands["-lifter"] = CommandOptions(boost::bind(&TestManager::HandleLifterCommand, this, _1, _2, _3), "Run automatic test of FFRD Lifter");
        supportedCommands["-tilting"] = CommandOptions(boost::bind(&TestManager::HandleTiltingCommand, this, _1, _2, _3), "Run automatic test of FFRD Tilting");
        supportedCommands["-usbunplug"] = CommandOptions(boost::bind(&TestManager::HandleUsbUnplugCommand, this, _1, _2, _3), "Run USB unplug test");
        supportedCommands["-usbplug"] = CommandOptions(boost::bind(&TestManager::HandleUsbPlugCommand, this, _1, _2, _3), "Run USB plug test");
        supportedCommands["-mappowerpusher"] = CommandOptions(boost::bind(&TestManager::HandleMapPowerpusherCommand, this, _1, _2, _3), "Map power pusher with mobile device");
        supportedCommands["-ai"] = CommandOptions(boost::bind(&TestManager::HandleAiCommand, this, _1, _2, _3), "Run AI test for TempleRun. Parameters: templerun_human_train_SVM/templerun_human_train_SVM_tilt");
        supportedCommands["-r"] = CommandOptions(boost::bind(&TestManager::HandleDeviceRecordCommand, this, _1, _2, _3), "Record a script directly on the device");
        supportedCommands["-i"] = CommandOptions(boost::bind(&TestManager::HandleIdRecordCommand, this, _1, _2, _3), "Record a script with id");
        supportedCommands["-stat"] = CommandOptions(boost::bind(&TestManager::HandleStatCommand, this, _1, _2, _3), "Run in statistics mode, get delay time of each operation");
        supportedCommands["-res"] = CommandOptions(boost::bind(&TestManager::HandleResolutionCommand, this, _1, _2, _3), "Set capture resolution(e.x. 320x240)");
        supportedCommands["-noimageshow"] = CommandOptions(boost::bind(&TestManager::HandleNoImageShowCommand, this, _1, _2, _3), "Run DaVinci without displaying image in imagebox");
        supportedCommands["-power"] = CommandOptions(boost::bind(&TestManager::HandlePowerTestCommand, this, _1, _2, _3), "Run DaVinci with collecting power consumption");
        supportedCommands["-checkflicker"] = CommandOptions(boost::bind(&TestManager::HandleCheckFlickerCommand, this, _1, _2, _3), "Specify the Video to check Flickering");
        supportedCommands["-audiowaveform"] = CommandOptions(boost::bind(&TestManager::HandleGenAudioFormCommand, this, _1, _2, _3), "Generate audio waveform image");
        supportedCommands["-checknoresponse"] = CommandOptions(boost::bind(&TestManager::HandleCheckNoResponseCommand, this, _1, _2, _3), "Specify the Video to check Flickering");
    }

    DaVinciStatus TestManager::ParseCommandlineOptions(const vector<string> &args)
    {
        // TODO: add an option to accept a device configuration file
        // currently, we just hard code the file name and location
        deviceConfig = GetDaVinciResourcePath(defaultDeviceConfig);
        statMode = false;

        if (args.empty())
        {
            return DaVinciStatusSuccess;
        }

        consoleMode = true;

        InitCommands();

        unsigned int currentArgIdx = 0;
        boost::shared_ptr<TestGroup> group = nullptr;
        hasFpsCommand = false;

        while (currentArgIdx < args.size())
        {
            string currentArg = boost::to_lower_copy(args[currentArgIdx]);
            if ((supportedCommands.find(currentArg) == supportedCommands.end())
                || (supportedCommands[currentArg].handler(args, currentArgIdx, group) != DaVinciStatusSuccess))
            {
                Usage();
                return DaVinciStatus(errc::invalid_argument);
            }
        }

        if (group != nullptr)
        {
            boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
            currentTestGroup = group;
            if (hasFpsCommand)
            {
                boost::shared_ptr<FPSMeasure> fps = boost::shared_ptr<FPSMeasure>(new FPSMeasure(""));
                currentTestGroup->AddTest(fps);
            }
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::Shutdown()
    {
        DaVinciStatus status;

        if (IsConsoleMode() == false)
        {
            DAVINCI_LOG_INFO << "Save Configuration..." << deviceConfig;
            if (!deviceConfig.empty())
            {
                status = DeviceManager::Instance().SaveConfig(deviceConfig);
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Cannot save device configuration: " << status.message();
                }
            }
            else
            {
                DAVINCI_LOG_ERROR << "Invalid file name for device configuration.";
                status = errc::address_not_available;
            }
        }

        StopCurrentTest();

        systemDiagnostic->StopPerfMonitor();

        // we only dump error message but always return success on shutdown
        // since it is the last operation anyway.
        DaVinciStatus status2 = DeviceManager::Instance().CloseDevices();
        TeardownHostLibs();

        if (status != DaVinciStatusSuccess)
            return status;
        if (!DaVinciSuccess(status2))
        {
            DAVINCI_LOG_ERROR << "Fail to close devices: " << status.message();
            return status2;
        }

        return DaVinciStatusSuccess;
    }

    void TestManager::SetMessageEventHandler(MessageEventHandler handler)
    {
        messageEventHandler = handler;
    }

    void TestManager::SetImageEventHandler(ImageEventHandler handler)
    {
        imageEventHandler = handler;
    }

    void TestManager::SetDrawDotHandler(DrawDotEventHandler drawHandler)
    {
        drawDotHandler = drawHandler;
    }

    void TestManager::SetDrawLinesHandler(DrawLinesEventHandler drawHandler)
    {
        drawLinesHandler = drawHandler;
    }

    DrawLinesEventHandler TestManager::GetDrawLinesEvent()
    {
        return drawLinesHandler;
    }

    DrawDotEventHandler TestManager::GetDrawDotsEvent()
    {
        return drawDotHandler;
    }

    bool TestManager::RunTestInit()
    {
        // to avoid the test is called in parallel
        boost::lock_guard<boost::mutex> lock(lockTestInit);

        bool result = true;

        currentTestStage = TestStage::TestStageBeforeInit;

        {
            boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);

            if (currentTestGroup == nullptr)
                return result;

            bool retVal = currentTestGroup->Init();

            if (!retVal)
            {
                DAVINCI_LOG_ERROR << "Error in initializing tests.";

                currentTestGroup->SetFinished();
                result = false;
            }
            else
            {
                testWatch.Restart();
            }
        }

        currentTestStage = TestStage::TestStageAfterInit;
        return result;
    }

    // return true if test is finished, otherwise false
    bool TestManager::CheckTestState()
    {
        if (currentTestStage == TestStage::TestStageBeforeInit)
            return false;

        // stop tests
        boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
        if (currentTestGroup == nullptr)
            return true;

        if ((currentTestStage == TestStage::TestStageAfterInit)
            && (testConfiguration.timeout > 0)
            && (testWatch.ElapsedMilliseconds() / 1000 > testConfiguration.timeout))
        {
            currentTestGroup->SetFinished();
        }

        if (currentTestGroup->IsFinished() == false)
            return false;

        StopCurrentTest();
        if (testStatusEventHandler != nullptr)
            testStatusEventHandler(TestStatusStopped);
        return true;
    }

    DaVinciStatus TestManager::StartCurrentTest()
    {
        return StartTest(currentTestGroup);
    }

    DaVinciStatus TestManager::StopCurrentTest()
    {
        if (threadInit != nullptr)
        {
            threadInit->join();
            threadInit = nullptr;
        }
        boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
        if (currentTestGroup != nullptr)
        {
            currentTestGroup->StopTests();
            currentTestGroup = nullptr;
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::StartTest(const boost::shared_ptr<TestGroup> &theTest)
    {
        if (theTest == nullptr)
            return DaVinciStatus(errc::invalid_argument);

        currentTestStage = TestStage::TestStageBeforeInit;
        {
            boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
            currentTestGroup = theTest;
        }

        threadInit = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TestManager::RunTestInit, this)));
        if (threadInit != nullptr)
            SetThreadName(threadInit->get_id(), "Init Test Thread");
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::StartTest(const boost::shared_ptr<TestInterface> &theTest)
    {
        if (theTest == nullptr)
            return DaVinciStatus(errc::invalid_argument);

        boost::shared_ptr<TestGroup> testGroup = boost::shared_ptr<TestGroup>(new TestGroup());
        testGroup->AddTest(theTest);
        return StartTest(testGroup);
    }

    DaVinciStatus TestManager::Snapshot(const string &imageName)
    {
        boost::shared_ptr<CaptureDevice> capDevice = DeviceManager::Instance().GetCurrentCaptureDevice();

        if (capDevice == nullptr)
            return DaVinciStatus(errc::no_such_device);

        bool isPseudoCameraEnabled = (boost::dynamic_pointer_cast<PseudoCameraCapture>(capDevice) != nullptr);
        Mat frame;
        boost::shared_ptr<TargetDevice> device = DeviceManager::Instance().GetCurrentTargetDevice();
        if (isPseudoCameraEnabled)
        {
            // if current capture device is pseudo camera, save screen capture got from target device
            if(device != nullptr)
            {
                frame = device->GetScreenCapture();
            }
        }
        else
        {
            if (!capDevice->IsFrameAvailable())
                return DaVinciStatus(errc::resource_unavailable_try_again);
            frame = capDevice->RetrieveFrame();
        }

        if (frame.empty() == true)
        {
            DAVINCI_LOG_ERROR << string("Error: NULL frame!");
            return DaVinciStatus(errc::no_stream_resources);
        }

        try
        {
            imwrite(imageName, frame);
        }
        catch(...)
        {
            return DaVinciStatus(errc::invalid_argument);
        }

        return DaVinciStatusSuccess;
    }

    boost::shared_ptr<TestGroup> TestManager::GetBuiltinTestByName(const string &testName)
    {
        if (testName != "OnDeviceRnR")
            return nullptr;

        auto test = boost::shared_ptr<OnDeviceScriptRecorder>(new OnDeviceScriptRecorder());
        boost::shared_ptr<TestGroup> testGroup = boost::shared_ptr<TestGroup>(new TestGroup());
        testGroup->AddTest(test);
        return testGroup;
    }

    DaVinciStatus TestManager::InitBuiltinTests()
    {
        // TODO initialize all builtin tests
        return DaVinciStatus(errc::not_supported);
    }

    void TestManager::SetTestStatusEventHandler(TestStatusEventHandler handler)
    {
        testStatusEventHandler = handler;
    }

    DaVinciStatus TestManager::TiltTo(int degree0, int degree1, int speed0, int speed1)
    {
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if(curHWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD Controller connected, please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        else
        {
            if (curHWController->TiltTo(degree0, degree1, speed0, speed1) == false)
            {
                return false;
            }
            // TODO: Need to update tilt degree in status bar in Main Window
        }
        return DaVinciStatusSuccess;
    }
    DaVinciStatus TestManager::TiltUp(int degree, int speed)
    {
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if(curHWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD Controller connected, please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        else
        {
            if (curHWController->TiltUp(degree, speed) == false)
            {
                return false;
            }
            // TODO: Need to update tilt degree in status bar in Main Window
        }
        return DaVinciStatusSuccess;
    }
    DaVinciStatus TestManager::TiltDown(int degree, int speed)
    {
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if(curHWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD Controller connected, please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        else
        {
            if (curHWController->TiltDown(degree, speed) == false)
            {
                return false;
            }
            // TODO: Need to update tilt degree in status bar in Main Window
        }
        return DaVinciStatusSuccess;
    }
    DaVinciStatus TestManager::TiltLeft(int degree, int speed)
    {
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if(curHWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD Controller connnected, please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        else
        {
            if (curHWController->TiltLeft(degree, speed) == false)
            {
                return false;
            }
            // TODO: Need to update tilt degree in status bar in Main Window
        }
        return DaVinciStatusSuccess;
    }
    DaVinciStatus TestManager::TiltRight(int degree, int speed)
    {
        boost::shared_ptr<HWAccessoryController> curHWController = (DeviceManager::Instance()).GetCurrentHWAccessoryController();
        if(curHWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD Controller connected, please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        else
        {
            if (curHWController->TiltRight(degree, speed) == false)
            {
                return false;
            }
            // TODO: Need to update tilt degree in status bar in Main Window
        }
        return DaVinciStatusSuccess;
    }
    DaVinciStatus TestManager::TiltCenter(int speed)
    {
        int degree0 = HWAccessoryController::defaultTiltDegree;
        int degree1 = HWAccessoryController::defaultTiltDegree;

        if (speed == 0)
        {
            if (TiltTo(degree0, degree1) == false)
            {
                return false;
            }
        }
        else
        {
            if (TiltTo(degree0, degree1, speed, speed) == false)
            {
                return false;
            }
        }
        return true;
    }

    DaVinciStatus TestManager::OnTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2)
    {
        DispatchUserTilt(action, degree1, degree2, speed1, speed2);
        if (action == TiltActionUp)
        {
            return TiltUp(degree1, speed1);
        }
        else if (action == TiltActionDown)
        {
            return TiltDown(degree1, speed1);
        }
        else if (action == TiltActionLeft)
        {
            return TiltLeft(degree1, speed1);
        }
        else if (action == TiltActionRight)
        {
            return TiltRight(degree1, speed1);
        }
        else if (action == TiltActionUpLeft)
        {
            double time_start = GetCurrentMillisecond();
            DaVinciStatus result = TiltUp(degree1, speed1);
            double time_temp = GetCurrentMillisecond();
            DAVINCI_LOG_DEBUG << string("        ") << string("Time duration of tilting execution: ") << time_temp - time_start;
            if (result != DaVinciStatusSuccess)
            {
                return result;
            }
            return TiltLeft(degree1, speed1);
        }
        else if (action == TiltActionDownLeft)
        {
            double time_start = GetCurrentMillisecond();
            DaVinciStatus result = TiltDown(degree1, speed1);
            double time_temp = GetCurrentMillisecond();
            DAVINCI_LOG_DEBUG << string("        ") << string("Time duration of tilting execution: ") << time_temp - time_start;

            if (result != DaVinciStatusSuccess)
            {
                return result;
            }
            return TiltLeft(degree1, speed1);
        }
        else if (action == TiltActionUpRight)
        {
            double time_start = GetCurrentMillisecond();
            DaVinciStatus result = TiltUp(degree1, speed1);
            double time_temp = GetCurrentMillisecond();
            DAVINCI_LOG_DEBUG << string("        ") << string("Time duration of tilting execution: ") << time_temp - time_start;

            if (result != DaVinciStatusSuccess)
            {
                return result;
            }
            return TiltRight(degree1, speed1);
        }
        else if (action == TiltActionDownRight)
        {
            double time_start = GetCurrentMillisecond();
            DaVinciStatus result = TiltDown(degree1, speed1);
            double time_temp = GetCurrentMillisecond();
            DAVINCI_LOG_DEBUG << string("        ") << string("Time duration of tilting execution: ") << time_temp - time_start;

            if (result != DaVinciStatusSuccess)
            {
                return result;
            }
            return TiltRight(degree1, speed1);
        }
        else if (action == TiltActionTo)
        {
            return TiltTo(degree1, degree2, speed1, speed2);
        }
        else
        {
            // TiltReset
            return TiltTo(defaultTiltDegree, defaultTiltDegree, speed1, speed2);
        }
    }

    DaVinciStatus TestManager::OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserZRotate(zAxisAction, degree, speed);
        if (zAxisAction == ZAXISTiltActionClockwise)
        {
            return hWController->ZAxisClockwiseTilt(degree, speed);
        }
        else if (zAxisAction == ZAXISTiltActionAnticlockwise)
        {
            return hWController->ZAxisAnticlockwiseTilt(degree, speed);
        }
        else if (zAxisAction == ZAXISTiltActionTo)
        {            
            return hWController->ZAxisTiltTo(degree, speed);
        }
        else if (zAxisAction == ZAXISTiltActionReset)
        {            
            return hWController->ZAxisTiltTo(defaultZAxisTiltDegree, speed);
        }
        else
        {
            return DaVinciStatus(errc::invalid_argument);
        }
    }

    DaVinciStatus TestManager::OnAppAction(AppLifeCycleAction action, string info1, string info2)
    {
        DispatchUserAppAction(action, info1, info2);
        boost::shared_ptr<TargetDevice> curTargetDevice = DeviceManager::Instance().GetCurrentTargetDevice();

        if (curTargetDevice == nullptr)
        {
            return DaVinciStatus(errc::no_such_device);
        }

        boost::shared_ptr<AndroidTargetDevice> curAndroidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(curTargetDevice);
        if (curAndroidDevice == nullptr)
        {
            return DaVinciStatus(errc::no_such_device);
        }
        if (!info1.empty())
        {
            switch (action)
            {
            case AppActionUninstall:
                return curTargetDevice->UninstallApp(info1);
            case AppActionInstall:
                return curTargetDevice->InstallApp(info1);
            case AppActionPushData:
                if (!info2.empty())
                {
                    return curAndroidDevice->AdbPushCommand(info1, info2);
                }
                else
                {
                    return DaVinciStatus(errc::invalid_argument);
                }
            case AppActionClearData:
                return curTargetDevice->ClearAppData(info1);
            case AppActionStart:
                return curTargetDevice->StartActivity(info1);
            case AppActionStop:
                return curTargetDevice->StopActivity(info1);
            default:
                DAVINCI_LOG_ERROR << string("Unknown app action!");
                return DaVinciStatus(errc::invalid_argument);
            }
        }
        else
        {
            return DaVinciStatus(errc::invalid_argument);
        }
    }

    DaVinciStatus TestManager::OnMoveHolder(int distance)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserMoveHolder(distance);
        if (distance < 0)
        {
            return hWController->HolderDown(abs(distance));
        }
        else
        {
            return hWController->HolderUp(abs(distance));
        }
    }

    DaVinciStatus TestManager::OnPowerButtonPusher(PowerButtonPusherAction action)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserPowerButtonPusher(action);
        switch (action)
        {
        case PowerButtonPusherPress:
            return hWController->PressPowerButton();
        case PowerButtonPusherRelease:
            return hWController->ReleasePowerButton();
        case PowerButtonPusherShortPress:
            return hWController->ShortPressPowerButton();
        case PowerButtonPusherLongPress:
            return hWController->LongPressPowerButton();
        default:
            DAVINCI_LOG_ERROR << string("Unknown action for execute Power Button Pusher!");
            return DaVinciStatus(errc::invalid_argument);
        }
    }

    DaVinciStatus TestManager::OnUsbSwitch(UsbSwitchAction action)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserUsbSwitch(action);
        switch (action)
        {
        case UsbOnePlugIn:
            return hWController->PlugUSB(patternPlugInUSB1);
        case UsbOnePullOut:
            return hWController->PlugUSB(patternPlugOutUSB1);
        case UsbTwoPlugIn:
            return hWController->PlugUSB(patternPlugInUSB2);
        case UsbTwoPullOut:
            return hWController->PlugUSB(patternPlugOutUSB2);
        default:
            DAVINCI_LOG_ERROR << string("Unknown action for execute USB Switch!");
            return DaVinciStatus(errc::invalid_argument);
        }
    }

    DaVinciStatus TestManager::OnEarphonePuller(EarphonePullerAction action)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserEarphonePuller(action);
        switch (action)
        {
        case EarphonePullerPlugIn:
            return hWController->PlugInEarphone();
        case EarphonePullerPullOut:
            return hWController->PlugOutEarphone();
        default:
            DAVINCI_LOG_ERROR << string("Unknown action for execute Earphone Puller!");
            return DaVinciStatus(errc::invalid_argument);
        }
    }

    DaVinciStatus TestManager::OnRelayController(RelayControllerAction action)
    {
        boost::shared_ptr<HWAccessoryController> hWController = DeviceManager::Instance().GetCurrentHWAccessoryController();
        if (hWController == nullptr)
        {
            DAVINCI_LOG_INFO << string("No FFRD controller connected. Please configure it.");
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserRelayController(action);
        switch (action)
        {
        case RelayOneConnect:
            return hWController->ConnectRelay(1);
        case RelayOneDisconnect:
            return hWController->DisconnectRelay(1);
        case RelayTwoConnect:
            return hWController->ConnectRelay(2);
        case RelayTwoDisconnect:
            return hWController->DisconnectRelay(2);
        case RelayThreeConnect:
            return hWController->ConnectRelay(3);
        case RelayThreeDisconnect:
            return hWController->DisconnectRelay(3);
        case RelayFourConnect:
            return hWController->ConnectRelay(4);
        case RelayFourDisconnect:
            return hWController->DisconnectRelay(4);
        default:
            DAVINCI_LOG_ERROR << string("Unknown action for execute Relay Controller!");
            return DaVinciStatus(errc::invalid_argument);
        }        
    }

    void TestManager::NormalizeScreenCoordinate(Orientation deviceCurrentOrientation, int &x_pos, int &y_pos)
    {
        int temp = 0;

        if((imageBoxInfo == nullptr)
            || (imageBoxInfo->height == 0)
            || (imageBoxInfo->width == 0))
        {
            DAVINCI_LOG_WARNING << string("ImageBox has not been rendered, (0,0) is sent.");
            x_pos = 0;
            y_pos = 0;
            ThreadSleep(1000);
            return;
        }

        if((int)rotateImageBoxInfo->orientation == (int)deviceCurrentOrientation)
        {
            x_pos = maxWidthHeight * x_pos / imageBoxInfo->width;
            y_pos = maxWidthHeight * y_pos / imageBoxInfo->height;
        }
        else if(rotateImageBoxInfo->orientation == OrientationPortrait && deviceCurrentOrientation == Orientation::Landscape
            ||  rotateImageBoxInfo->orientation == OrientationLandscape && deviceCurrentOrientation == Orientation::ReversePortrait
            ||  rotateImageBoxInfo->orientation == OrientationReversePortrait && deviceCurrentOrientation == Orientation::ReverseLandscape
            ||  rotateImageBoxInfo->orientation == OrientationReverseLandscape && deviceCurrentOrientation == Orientation::Portrait)
        {
            temp = x_pos;
            x_pos = maxWidthHeight * y_pos / imageBoxInfo->height;
            y_pos = maxWidthHeight - maxWidthHeight * temp / imageBoxInfo->width;

        }
        else if(rotateImageBoxInfo->orientation == OrientationLandscape && deviceCurrentOrientation == Orientation::Portrait
            ||  rotateImageBoxInfo->orientation == OrientationReversePortrait && deviceCurrentOrientation == Orientation::Landscape
            ||  rotateImageBoxInfo->orientation == OrientationReverseLandscape && deviceCurrentOrientation == Orientation::ReversePortrait
            ||  rotateImageBoxInfo->orientation == OrientationPortrait && deviceCurrentOrientation == Orientation::ReverseLandscape)
        {
            temp = x_pos;
            x_pos = maxWidthHeight - maxWidthHeight * y_pos / imageBoxInfo->height;
            y_pos = maxWidthHeight * temp / imageBoxInfo->width;
        }
        else
        {
            x_pos = maxWidthHeight - maxWidthHeight * x_pos / imageBoxInfo->width;
            y_pos = maxWidthHeight - maxWidthHeight * y_pos / imageBoxInfo->height;
        }
        // Check boundaries of coordinate
        CheckCoordinate(x_pos, y_pos);
    }

    void TestManager::TransformImageBoxToDeviceCoordinate(Size deviceSize, Orientation deviceCurrentOrientation, int &x_pos, int &y_pos)
    {
        int tmp_x = x_pos, tmp_y = y_pos;
        switch(deviceCurrentOrientation)
        {
        case Orientation::Portrait:
            {
                switch(rotateImageBoxInfo->orientation)
                {
                case Orientation::Portrait:
                    {
                        x_pos = deviceSize.width * tmp_x / imageBoxInfo->width;
                        y_pos = deviceSize.height * tmp_y / imageBoxInfo->height;
                        break;
                    }
                case Orientation::Landscape:
                    {
                        x_pos = deviceSize.width * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        y_pos = deviceSize.height * tmp_x / imageBoxInfo->width;
                        break;
                    }
                case Orientation::ReversePortrait:
                    {
                        x_pos = deviceSize.width * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        y_pos = deviceSize.height * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        break;
                    }
                case Orientation::ReverseLandscape:
                    {
                        x_pos = deviceSize.width * tmp_y / imageBoxInfo->height;
                        y_pos = deviceSize.height * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        break;
                    }
                default:
                    {
                        x_pos = 0;
                        y_pos = 0;
                    }
                }
                break;
            }
        case Orientation::Landscape:
            {
                switch(rotateImageBoxInfo->orientation)
                {
                case Orientation::Portrait:
                    {
                        x_pos = deviceSize.height * tmp_y / imageBoxInfo->height;
                        y_pos = deviceSize.width * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        break;
                    }
                case Orientation::Landscape:
                    {
                        x_pos = deviceSize.height * tmp_x / imageBoxInfo->width;
                        y_pos = deviceSize.width * tmp_y / imageBoxInfo->height; 
                        break;
                    }
                case Orientation::ReversePortrait:
                    {
                        x_pos = deviceSize.height * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        y_pos = deviceSize.width * tmp_x/ imageBoxInfo->width; 
                        break;
                    }
                case Orientation::ReverseLandscape:
                    {
                        x_pos = deviceSize.height * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        y_pos = deviceSize.width * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height; 
                        break;
                    }
                default:
                    {
                        x_pos = 0;
                        y_pos = 0;
                    }
                }
                break;
            }
        case Orientation::ReversePortrait:
            {
                switch(rotateImageBoxInfo->orientation)
                {
                case Orientation::Portrait:
                    {
                        x_pos = deviceSize.width * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        y_pos = deviceSize.height * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        break;
                    }
                case Orientation::Landscape:
                    {
                        x_pos = deviceSize.width * tmp_y / imageBoxInfo->height;
                        y_pos = deviceSize.height * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width; 
                        break;
                    }
                case Orientation::ReversePortrait:
                    {
                        x_pos = deviceSize.width * tmp_x / imageBoxInfo->width;
                        y_pos = deviceSize.height * tmp_y/ imageBoxInfo->height; 
                        break;
                    }
                case Orientation::ReverseLandscape:
                    {
                        x_pos = deviceSize.width * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        y_pos = deviceSize.height * tmp_x / imageBoxInfo->width; 
                        break;
                    }
                default:
                    {
                        x_pos = 0;
                        y_pos = 0;
                    }
                }
                break;
            }
        case Orientation::ReverseLandscape:
            {
                switch(rotateImageBoxInfo->orientation)
                {
                case Orientation::Portrait:
                    {
                        x_pos = deviceSize.height * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height;
                        y_pos = deviceSize.width * tmp_x / imageBoxInfo->width;
                        break;
                    }
                case Orientation::Landscape:
                    {
                        x_pos = deviceSize.height * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width;
                        y_pos = deviceSize.width * (imageBoxInfo->height - tmp_y) / imageBoxInfo->height; 
                        break;
                    }
                case Orientation::ReversePortrait:
                    {
                        x_pos = deviceSize.height * tmp_y / imageBoxInfo->height;
                        y_pos = deviceSize.width * (imageBoxInfo->width - tmp_x) / imageBoxInfo->width; 
                        break;
                    }
                case Orientation::ReverseLandscape:
                    {
                        x_pos = deviceSize.height * tmp_x / imageBoxInfo->width;
                        y_pos = deviceSize.width * tmp_y / imageBoxInfo->height; 
                        break;
                    }
                default:
                    {
                        x_pos = 0;
                        y_pos = 0;
                    }
                }
                break;
            }
        default:
            {
                x_pos = 0;
                y_pos = 0;
            }
        }
    }

    DaVinciStatus TestManager::DragOnScreen(int x1_pos, int y1_pos, int x2_pos, int y2_pos, bool normalized)
    {
        boost::shared_ptr<TargetDevice> curDevice = (DeviceManager::Instance()).GetCurrentTargetDevice();
        if(curDevice == nullptr)
        {
            return DaVinciStatus(errc::no_such_device);
        }
        // Get device orientation
        Orientation curOrientation = curDevice->GetCurrentOrientation();
        // Normalize
        int normalized_x1 = x1_pos, normalized_y1 = y1_pos;
        int normalized_x2 = x2_pos, normalized_y2 = y2_pos;

        if(!normalized)
        {
            NormalizeScreenCoordinate(curOrientation, normalized_x1, normalized_y1);
            NormalizeScreenCoordinate(curOrientation, normalized_x2, normalized_y2);
        }

        if(normalized_x2 >= maxWidthHeight)
        {
            normalized_x2 = maxWidthHeight - 1;
        }
        else if(normalized_x2 <= 0)
        {
            normalized_x2 = 1;
        }
        if(normalized_y2 >= maxWidthHeight)
        {
            normalized_y2 = maxWidthHeight - 1;
        }
        else if(normalized_y2 <= 0)
        {
            normalized_y2 = 1;
        }
        return curDevice->Drag(normalized_x1, normalized_y1, normalized_x2, normalized_y2);
    }

    DaVinciStatus TestManager::OnSwipe(SwipeAction action)
    {
        DispatchUserSwipe(action);
        if(action == SwipeActionUp)
        {
            return DragOnScreen(maxWidthHeight / 2, maxWidthHeight / 2, maxWidthHeight / 2, maxWidthHeight / 4, true);
        }
        else if (action == SwipeActionDown)
        {
            return DragOnScreen(maxWidthHeight / 2, maxWidthHeight / 2, maxWidthHeight / 2, 3 * maxWidthHeight / 4, true);
        }
        else if (action == SwipeActionLeft)
        {
            return DragOnScreen(maxWidthHeight / 2, maxWidthHeight / 2, maxWidthHeight / 4, maxWidthHeight / 2, true);
        }
        else
        {
            return DragOnScreen(maxWidthHeight / 2, maxWidthHeight / 2, 3 * maxWidthHeight / 4, maxWidthHeight / 2, true);
        }
    }

    DaVinciStatus TestManager::OnButtonAction(ButtonAction action, ButtonActionType mode)
    {
        ButtonEventMode eventMode =ButtonEventMode::DownAndUp;

        androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());

        if(androidDevice == nullptr)
        {
            return DaVinciStatus(errc::no_such_device);
        }

        switch (mode)
        {
        case ButtonActionType::Up:
            eventMode = ButtonEventMode::Up;
            break;
        case ButtonActionType::Down:
            eventMode = ButtonEventMode::Down;
            break;
        case ButtonActionType::DownAndUp:
        default:
            eventMode = ButtonEventMode::DownAndUp;
            break;
        }
        DispatchUserButton(action, eventMode);

        switch (action)
        {
        case ButtonActionBack:
            return androidDevice->PressBack(eventMode);
        case ButtonActionMenu:
            return androidDevice->PressMenu(eventMode);
        case ButtonActionHome:
            return androidDevice->PressHome(eventMode);
        case ButtonActionLightUp:
            return androidDevice->WakeUp(eventMode);
        case ButtonActionVolumeUp:
            return androidDevice->VolumeUp(eventMode);
        case ButtonActionVolumeDown:
            return androidDevice->VolumeDown(eventMode);
        case ButtonActionPower:
            return androidDevice->PressPower(eventMode);
        case ButtonActionBrightnessUp:
            return androidDevice->BrightnessAction(eventMode, Brightness::Up);
        case ButtonActionBrightnessDown:
            return androidDevice->BrightnessAction(eventMode, Brightness::Down);
        default:
            return DaVinciStatus(errc::not_supported);
        }
    }

    void TestManager::RecordUserMouseAction(const MouseAction* action, boost::shared_ptr<TargetDevice> curDevice, int x_pos, int y_pos)
    {
        Orientation deviceCurrentOrientation = curDevice->GetCurrentOrientation();
        if(action->button == MouseButtonLeft)
        {
            if(action->actionType == MouseActionTypeDown)
            {
                DispatchUserMouse(action->button, action->actionType, x_pos, y_pos, action->x, action->y, action->ptr, deviceCurrentOrientation);
            }
            else
            {
                DispatchUserMouse(action->button, action->actionType, x_pos, y_pos, action->x, action->y, action->ptr, deviceCurrentOrientation);
            }
        }
        else
        {
            DispatchUserMouse(action->button, action->actionType, x_pos, y_pos, action->x, action->y, action->ptr, deviceCurrentOrientation);
        }
    }

    DaVinciStatus TestManager::OnMouseAction(const ImageBoxInfo *imageBoxInfoPara, const MouseAction *action)
    {
        int x_pos = 0;
        int y_pos = 0;
        imageBoxInfo = imageBoxInfoPara;
        boost::shared_ptr<TargetDevice> curDevice = (DeviceManager::Instance()).GetCurrentTargetDevice();
        if (curDevice == nullptr || imageBoxInfoPara == nullptr || action == nullptr)
        {
            return DaVinciStatus(errc::no_such_device_or_address);
        }
        // Get device orientation
        Orientation deviceCurrentOrientation = curDevice->GetCurrentOrientation();
        // Check normalize
        x_pos = action->x;
        y_pos = action->y;

        NormalizeScreenCoordinate(deviceCurrentOrientation, x_pos, y_pos);
        RecordUserMouseAction(action, curDevice, x_pos, y_pos);

        // Mouse action
        switch (action->button)
        {
        case MouseButtonLeft: 
            { 
                if (action->actionType == MouseActionTypeDown)
                {
                    return curDevice->TouchDown(x_pos, y_pos, action->ptr, deviceCurrentOrientation);
                }
                else if (action->actionType == MouseActionTypeUp)
                {
                    return curDevice->TouchUp(x_pos, y_pos, action->ptr, deviceCurrentOrientation);
                }
                else if (action->actionType == MouseActionTypeMove)
                {
                    return curDevice->TouchMove(x_pos, y_pos, action->ptr, deviceCurrentOrientation);
                }
                else
                    return DaVinciStatus(errc::operation_not_supported);

                /* 
                The code will never check the clicks. Remove them.
                if (action->clicks == 1)
                {
                return curDevice->Click(x_pos, y_pos, action->ptr, deviceCurrentOrientation);
                }
                else if (action->clicks == 2)
                {
                return curDevice->DoubleClick(x_pos, y_pos, action->ptr, deviceCurrentOrientation);
                }
                */
            }
        case MouseButtonRight:
            {
                // TODO: Waiting for WindowsTargetDevice and ChromeTargetDevice implementation
                return DaVinciStatus(errc::operation_not_supported);
            }
        case MouseButtonMiddle:
            {
                // TODO: Waiting for WindowsTargetDevice and ChromeTargetDevice implementation
                return DaVinciStatus(errc::operation_not_supported);
            }
        default:
            return DaVinciStatus(errc::operation_not_supported);
        }
    }

    DaVinciStatus TestManager::OnSetText(const string &text)
    {
        boost::shared_ptr<AndroidTargetDevice> curDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
        if (curDevice == nullptr)
        {
            return DaVinciStatus(errc::no_such_device);
        }
        DispatchUserSetText(text);
        return curDevice->TypeString(text);
    }

    DaVinciStatus TestManager::OnOcr()
    {
        DispatchUserOcr();
        // TODO: issue OCR
        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::OnAudioRecord(bool startRecord)
    {
        DispatchUserAudioRecord(startRecord);
        return DaVinciStatusSuccess;
    }

    void TestManager::ProcessFrame(boost::shared_ptr<CaptureDevice> captureDevice)
    {
        // check test state before grabbing a frame from device
        CheckTestState();

        double timestamp = GetCurrentMillisecond();
        // retrieve frame from capture device
        int frameIndex = -1;
        Mat frame = captureDevice->RetrieveFrame(&frameIndex);
        if (frame.empty() == true)
        {
            DAVINCI_LOG_ERROR << "Error: NULL frame!";
            DeviceManager::Instance().IncreaseNullFrameCount();
            return;
        }
        else if (DeviceManager::Instance().GetNullFrameCount() > 0)
        {
            DeviceManager::Instance().ResetNullFrameCount();
        }

        // process frame if there are tests running
        if (currentTestStage == TestStage::TestStageAfterInit)
        {
            boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
            if ((currentTestGroup != nullptr)
                && (currentTestGroup->IsFinished() == false))
            {
                Mat ret = currentTestGroup->ProcessFrame(frame, timestamp, frameIndex);
                if (ret.data != frame.data)
                    frame = ret;
            }
        }

        // display frame on image box
        if (imageEventHandler != nullptr)
        {
            Mat rotated = RotateFrame(frame);

            bool isDoingTest = false;

            if (currentTestStage == TestStage::TestStageAfterInit)
            {
                boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
                isDoingTest = ((currentTestGroup != nullptr) && (!currentTestGroup->IsFinished()));
            }

            Point center (20, 20);
            Scalar color;

            if (isDoingTest)
                color = Red;
            else
                color = Green;
            circle(rotated, center, 0, color, 10);

#ifdef _DEBUG
            // debug information: millseconds for processing current frame
            double duration = (GetCurrentMillisecond() - timestamp) / 1000.0; // second
            char durationArray[10];
            Point strPos(10, 60);
            sprintf_s(durationArray, "%02.3f", duration);
            string durationStr (durationArray);
            cv::putText(rotated, durationStr, strPos, FONT_HERSHEY_PLAIN, 1.0, White, 2);
#endif
            // call the function in c# layer to display image
            ImageInfo imageInfo;
            imageInfo.iplImage = new IplImage(rotated);
            imageInfo.mat = new cv::Mat(rotated);

            //DAVINCI_LOG_DEBUG << "Render Image" ;
            imageEventHandler(&imageInfo);
        }
    }

    void TestManager::ProcessWave(const boost::shared_ptr<vector<short>> &samples)
    {
        boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
        if (samples != nullptr &&
            currentTestGroup != nullptr &&
            currentTestGroup->IsFinished() == false &&
            currentTestStage == TestStage::TestStageAfterInit)
        {
            currentTestGroup->ProcessWave(samples);
        }
    }

    DaVinciStatus TestManager::SetRotateImageBoxInfo(ImageBoxInfo *imageBoxInfoPassIn)
    {
        if (imageBoxInfoPassIn == nullptr)
            return DaVinciStatus(errc::invalid_argument);
        rotateImageBoxInfo->height = imageBoxInfoPassIn->height;
        rotateImageBoxInfo->width = imageBoxInfoPassIn->width;
        rotateImageBoxInfo->orientation = imageBoxInfoPassIn->orientation;
        return DaVinciStatusSuccess;
    }

    void TestManager::SetImageBoxInfo(ImageBoxInfo *imageBoxInfoPassIn)
    {
        if (imageBoxInfoPassIn == nullptr)
            return;
        imageBoxInfo = imageBoxInfoPassIn;
    }

    DeviceOrientation TestManager::GetImageBoxLayout()
    {
        if (rotateImageBoxInfo != nullptr)
        {
            return rotateImageBoxInfo->orientation;
        }
        else 
        {
            return OrientationPortrait;
        }
    }

    Mat TestManager::RotateFrame(Mat & frame)
    {
        DeviceOrientation imageBoxLayout = GetImageBoxLayout();

        switch (imageBoxLayout)
        {
        case OrientationPortrait:
            return RotateImage90(frame);
        case OrientationLandscape:
            return frame.clone();
        case OrientationReversePortrait:
            return RotateImage270(frame);
        case OrientationReverseLandscape:
            return RotateImage180(frame);
        default:
            return RotateImage90(frame);
        }
    }

    bool TestManager::IsConsoleMode() const
    {
        return consoleMode;
    }

    bool TestManager::IsOfflineCheck()const
    {
        return testConfiguration.offlineConcurrentMatch == 1;
    }

    bool TestManager::IsCheckFlickering()const
    {
        return testConfiguration.checkflicker == BoolTrue;
    }

    void TestManager::RegisterUserActionListener(const boost::shared_ptr<TestManager::UserActionListener> listener)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        userActionListeners.insert(listener);
    }

    void TestManager::UnregisterUserActionListener(const boost::shared_ptr<TestManager::UserActionListener> listener)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        userActionListeners.erase(listener);
    }

    void TestManager::DispatchUserMouse(MouseButton button, MouseActionType actionType, int x, int y, int actionX, int actionY, int ptr, Orientation o)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnMouse(button, actionType, x, y, actionX, actionY, ptr, o);
        }
    }

    void TestManager::DispatchUserMoveHolder(int distance)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnMoveHolder(distance);
        }
    }

    void TestManager::DispatchUserTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnTilt(action, degree1, degree2, speed1, speed2);
        }
    }

    void TestManager::DispatchUserZRotate(ZAXISTiltAction zAxisAction, int degree, int speed)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnZRotate(zAxisAction, degree, speed);
        }
    }

    void TestManager::DispatchUserPowerButtonPusher(PowerButtonPusherAction action)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnPowerButtonPusher(action);
        }
    }

    void TestManager::DispatchUserUsbSwitch(UsbSwitchAction action)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnUsbSwitch(action);
        }
    }

    void TestManager::DispatchUserEarphonePuller(EarphonePullerAction action)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnEarphonePuller(action);
        }
    }

    void TestManager::DispatchUserRelayController(RelayControllerAction action)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnRelayController(action);
        }
    }

    void TestManager::DispatchUserAppAction(AppLifeCycleAction action, string info1, string info2)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnAppAction(action, info1, info2);
        }
    }

    void TestManager::DispatchUserSwipe(SwipeAction action)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnSwipe(action);
        }
    }

    void TestManager::DispatchUserButton(ButtonAction action, ButtonEventMode mode)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnButton(action, mode);
        }
    }

    void TestManager::DispatchUserSetText(const string &text)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnSetText(text);
        }
    }

    void TestManager::DispatchUserOcr()
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnOcr();
        }
    }

    void TestManager::DispatchUserAudioRecord(bool startRecord)
    {
        boost::lock_guard<boost::mutex> lock(userActionListenersMutex);
        for (auto listener : userActionListeners)
        {
            listener->OnAudioRecord(startRecord);
        }
    }

    TestConfiguration TestManager::GetTestConfiguration()
    {
        return testConfiguration;
    }

    void TestManager::InitTestConfiguration()
    {
        memset(&testConfiguration, 0, sizeof(TestConfiguration));
    }

    void TestManager::InitImageBoxInfo()
    {
        imageBoxInfo = nullptr;
        rotateImageBoxInfo = boost::shared_ptr<ImageBoxInfo>(new ImageBoxInfo());
    }

    boost::shared_ptr<TestGroup> TestManager::GetTestGroup()
    {
        boost::lock_guard<boost::recursive_mutex> lock(lockOfCurTestGroup);
        return currentTestGroup;
    }

    DaVinciStatus TestManager::SetupHostLibs(const string &theDaVinciHome)
    {
        DaVinciStatus status;

        TeardownLogging();
        status = SetupLogging(theDaVinciHome);
        if (!DaVinciSuccess(status))
        {
            return status;
        }

        status = SetupXmlLib();
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Cannot initialize XML library!";
            return status;
        }

        status = QScript::InitSpec();
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Cannot initialize Q script spec!";
            return status;
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestManager::TeardownHostLibs()
    {
        DaVinciStatus status;
        status = TeardownXmlLib();
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Did not close XML lib successfully: " << status.message();
        }
        TeardownLogging();
        return DaVinciStatusSuccess;
    }

    string TestManager::GetConfigureFile()
    {
        return defaultDeviceConfig;
    }

    bool TestManager::GetStatisticsMode()
    {
        return statMode;
    }

    void TestManager::SetRecordWithID(int needId)
    {
        recordWithId = needId;
    }

    int TestManager::GetRecordWithID()
    {
        return recordWithId;
    }

    DaVinciStatus TestManager::SetTestProjectConfigData(const TestProjectConfig *projConfig)
    {
        tstConfig.name = "";
        tstConfig.baseFolder = "";
        tstConfig.apkName = "";
        tstConfig.packageName = "";
        tstConfig.activityName = "";
        tstConfig.pushDataSource = "";
        tstConfig.pushDataTarget = "";

        if (projConfig == nullptr)
        {
            return DaVinciStatus(errc::invalid_argument);
        }

        if (projConfig->Name != nullptr)
        {
            tstConfig.name = projConfig->Name;
        }
        if (projConfig->BaseFolder != nullptr)
        {
            tstConfig.baseFolder = projConfig->BaseFolder;
        }
        if (projConfig->Application != nullptr)
        {
            tstConfig.apkName = projConfig->Application;
        }
        if (projConfig->PackageName != nullptr)
        {
            tstConfig.packageName = projConfig->PackageName;
        }
        if (projConfig->ActivityName != nullptr)
        {
            tstConfig.activityName = projConfig->ActivityName;
        }
        if (projConfig->PushDataSource != nullptr)
        {
            tstConfig.pushDataSource = projConfig->PushDataSource;
        }
        if (projConfig->PushDataTarget != nullptr)
        {
            tstConfig.pushDataTarget = projConfig->PushDataTarget;
        }
        return DaVinciStatusSuccess;
    }

    TestProjectConfigData TestManager::GetTestProjectConfigData()
    {
        return tstConfig;
    }
}

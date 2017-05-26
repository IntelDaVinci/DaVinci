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

#include "DeviceManager.hpp"
#include "TargetDevice.hpp"
#include "ScriptReplayer.hpp"
#include "SpinLock.hpp"
#include "ObjectUtil.hpp"
#include "Smoke.hpp"
#include "BiasObject.hpp"
#include "BiasFileReader.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "TestReport.hpp"
#include "AudioManager.hpp"
#include "AudioRecognize.hpp"
#include "VideoCaptureProxy.hpp"

#include "boost/smart_ptr/shared_ptr.hpp"
#include "ResultChecker.hpp"
#include "QReplayInfo.hpp"
#include "OnDeviceScriptRecorder.hpp"
#include "KeywordCheckHandlerForPopupWindow.hpp"

namespace DaVinci
{
    using namespace boost::filesystem;

    ScriptReplayer::ScriptReplayer(const string &script)
        : ScriptEngineBase(script), isRecordedSoftCam(false), isReplaySoftCam(false),
        sourceFrameWidth(0), sourceFrameHeight(0), qsMode(QS_NORMAL), seedIteration(0),
        needCurrentCheck(false), needNextCheck(false), isAppActive(false), endQsIp(0),
        lastCrashCheckTime(0), sensorCheckFlag(false), isBreakPoint(false), isTouchMove(false), needTapMode(false),
        installComplete(false), isEntryScript(true), unpluggedUSBPort(0),
        imgMatcherStartIndex(0), resultChecker(nullptr), needClickId(false), treeId(0),
        breakPointFrameOrientation(Orientation::Unknown), currentTouchMoveIndex(0),
        idXOffset(0), idYOffset(0), scriptReplayMode(ScriptReplayMode::REPLAY_COORDINATE),
        pwt(nullptr)
    {
    }

    ScriptReplayer::~ScriptReplayer()
    {
    }

    void ScriptReplayer::AddFPSTest(const string& scriptName, const string& pkgName)
    {                    
        fpsTest.reset(new FPSMeasure(scriptName));
        fpsTest->SetDutAppName(pkgName);
        fpsTest->Init();                    
        AddTest(fpsTest, false);
    }

    bool ScriptReplayer::IsValidForPowerTest()
    {
        auto allEvts = qs->GetAllQSEventsAndActions();

        bool foundPowerStartEvt = false;
        bool foundPowerStopEvt = false;

        for (auto it = allEvts.begin(); it != allEvts.end(); it++)
        {
            int opcode = (*it)->Opcode();
            if (opcode == QSEventAndAction::OPCODE_POWER_MEASURE_START)
            {
                if (foundPowerStartEvt || foundPowerStopEvt)
                {
                    DAVINCI_LOG_ERROR << "Invalid power test OPCODE pair";
                    return false;
                }
                else
                {
                    foundPowerStartEvt = true;
                }
            }
            else if (opcode == QSEventAndAction::OPCODE_POWER_MEASURE_STOP)
            {
                if (!foundPowerStartEvt || foundPowerStopEvt)
                {
                    DAVINCI_LOG_ERROR << "Invalid power test OPCODE pair";
                    return false;
                }
                else
                {
                    foundPowerStopEvt = true;
                }
            }
        }

        return (foundPowerStartEvt && foundPowerStopEvt);
    }

    bool ScriptReplayer::Init()
    {
        SetName("ScriptReplayer - " + qsFileName);
        if(isEntryScript)
        {
            testReport = boost::shared_ptr<TestReport>(new TestReport());
            testReport->PrepareQScriptReplay(qsFileName);
        }

        DaVinciStatus status;
        DAVINCI_LOG_INFO << "Playing a Q script loaded from " << qsFileName;
        qs = QScript::Load(qsFileName);
        if (qs == nullptr)
        {
            DAVINCI_LOG_ERROR << "Error loading Q script: " <<  qsFileName;
            SetFinished();
            return false;
        }

        string recordDeviceWidth = qs->GetResolutionWidth();
        string recordDeviceHeight = qs->GetResolutionHeight();
        TryParse(recordDeviceWidth, recordDeviceSize.width);
        TryParse(recordDeviceHeight, recordDeviceSize.height);

        testReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());

        videoFileName = qs->GetResourceFullPath(qs->GetScriptName() + "_replay.avi");
        audioFileName = qs->GetResourceFullPath(qs->GetScriptName() + "_replay.wav");
        qtsFileName = qs->GetResourceFullPath(qs->GetScriptName() + "_replay.qts");
        qfdFileName = qs->GetResourceFullPath(qs->GetScriptName() + "_replay.qfd");
        sensorCheckFileName = qs->GetResourceFullPath(qs->GetScriptName() + "_SensorCheck.txt");
        sensorCheckFlag = false;
        sensorCheckData = vector<String>();     //Init the data structure for storage

        SetCaptureType();
        SetSourceFrameResolution();
        hwAcc = DeviceManager::Instance().GetCurrentHWAccessoryController();
        dut = DeviceManager::Instance().GetCurrentTargetDevice();
        // configure start orientation correctly if we have such configuration

        // if dut is nullptr, that is, no target device is connected, script will 
        // continue to run, as sometimes script may utilize FFRD functionality for 
        // testing.
        if ((dut == nullptr) && (DeviceManager::Instance().GetEnableTargetDeviceAgent()))
        {
            DAVINCI_LOG_WARNING << "No target device is connected.";
        }

        // Not used now
        navigationBarHeights = qs->GetNavigationBarHeight();
        needVideoRecording = qs->NeedVideoRecording();
        isMultiLayerMode = qs->IsMultiLayerMode();
        // TODO: reset camera preset if it is different from current capture setting

        DAVINCI_LOG_INFO << "      CameraType = " << DeviceManager::Instance().GetCurrentCaptureStr();
        DAVINCI_LOG_INFO << "      CameraPresetting = " << static_cast<int>(DeviceManager::Instance().GetCurrentCapturePreset());
        DAVINCI_LOG_INFO << "      QS CameraType = " << qs->GetConfiguration(QScript::ConfigCameraType);
        DAVINCI_LOG_INFO << "      Video Recording = " << qs->GetConfiguration(QScript::ConfigVideoRecording);
        DAVINCI_LOG_INFO << "      MultiLayer Mode = " << qs->GetConfiguration(QScript::ConfigMultiLayerMode);
        DAVINCI_LOG_INFO << "      QS Presetting = " << qs->GetConfiguration(QScript::ConfigCameraPresetting);
        DAVINCI_LOG_INFO << "      QS StartOrientation = " << qs->GetConfiguration(QScript::ConfigStartOrientation);
        DAVINCI_LOG_INFO << "      Events & Actions #  = " << qs->NumEventAndActions();
        DAVINCI_LOG_INFO << "      Offline Image Match = " << TestManager::Instance().IsOfflineCheck();

        state.Init();
        scriptReplayMode = ScriptReplayMode::REPLAY_COORDINATE;

        if (dut != nullptr) {
            dut->SetCurrentMultiLayerMode(isMultiLayerMode);

            boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
            assert(androidDut != nullptr);
            std::string recordedVideoType = qs->GetConfiguration(QScript::ConfigRecordedVideoType);
            boost::to_upper(recordedVideoType);
            // If recorded video type of Qscript is not H264, the touch move frequency of Qscript may be high.
            if (recordedVideoType == QScript::ConfigVideoTypeH264) {
                androidDut->ReduceTouchMoveFrequency(false);
            } else {
                androidDut->ReduceTouchMoveFrequency(true);
            }
        }

#pragma region GeneratoryTest
        seedIteration = 0;
        biasFileReader = boost::shared_ptr<BiasFileReader>(new BiasFileReader());
        smokeTest = boost::shared_ptr<Smoke>(new Smoke());
        launcherRecognizer = boost::shared_ptr<LauncherAppRecognizer>(new LauncherAppRecognizer());
        keyboardRecognizer = boost::shared_ptr<KeyboardRecognizer>(new KeyboardRecognizer());
        inputMethodRecognizer =  boost::shared_ptr<InputMethodRecognizer>(new InputMethodRecognizer());
        biasObject = boost::shared_ptr<BiasObject>(new BiasObject());
        expEngine = boost::shared_ptr<ExploratoryEngine>(new ExploratoryEngine(qs->GetPackageName(), qs->GetActivityName(), qs->GetIconName(), dut, qs->GetOSversion()));
        viewHierarchyParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
        CheckSmokeMode();
#pragma endregion GeneratoryTest

        if ((dut != nullptr) && (DeviceManager::Instance().GetEnableTargetDeviceAgent()))
        {
            if (IsSmokeQS(qsMode))
            {
                // Stop any top package except QAgent to make test clean (e.g., screen locked)
                string topPackageName = dut->GetTopPackageName();
                if ((topPackageName != "com.example.qagent") && (topPackageName.find("launcher") == string::npos) && (!AppUtil::Instance().IsLauncherApp(topPackageName)))
                {
                    dut->StopActivity(topPackageName);
                    dut->PressBack();
                    dut->PressHome();
                    ThreadSleep(1000); //Wait for device switch to home page, if don't wait will impact setcurrentorientation.
                }
            }

            Orientation currentOrientation = dut->GetCurrentOrientation(true);
            String orientationString = qs->GetConfiguration(QScript::ConfigStartOrientation);
            if (!orientationString.empty())
            {
                Orientation expectedOrientation = StringToOrientation(orientationString);

                if (expectedOrientation != currentOrientation)
                    DAVINCI_LOG_WARNING << "!!! The device's current orientation: " << OrientationToString(currentOrientation) <<" not align with recording orientation: " << orientationString <<", this may lead to test result incorrect!!!";
            }
        }

        bool hasFpsInstr = false;
        double fpsStartTime = -1.0, fpsStopTime = -1.0;
        string fpsStartImage, fpsStopImage;
        for (int ip = 1; ip <= static_cast<int>(qs->NumEventAndActions()); ip++)
        {
            boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(ip);
            if (qsEvent == nullptr)
            {
                break;
            }
            switch (qsEvent->Opcode())
            {
            case QSEventAndAction::OPCODE_IF_MATCH_IMAGE:
            case QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT:
            case QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH:
            case QSEventAndAction::OPCODE_IMAGE_CHECK:
                {
                    // create SURF objects for OPCODE_IF_MATCH_IMAGE and OPCODE_IF_MATCH_IMAGE_WAIT.
                    string imageName = QScript::ImageObject::Parse(qsEvent->StrOperand(0))->ImageName();
                    if (qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH)
                    {
                        imageName = qsEvent->StrOperand(0);
                    }
                    string imageFullPath = qs->GetResourceFullPath(imageName);
                    if (is_regular_file(imageFullPath))
                    {
                        Mat refImage = imread(imageFullPath);
                        ImageMatchAlgorithm matchAlgo = ImageMatchAlgorithm::Patch;
                        if (qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_MATCH_IMAGE ||
                            qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH)
                        {
                            matchAlgo = OperandToImageMatchAlgorithm(qsEvent->IntOperand(2));
                        }
                        else if (qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT)
                        {
                            matchAlgo = OperandToImageMatchAlgorithm(qsEvent->IntOperand(3));
                        }
                        if (matchAlgo == ImageMatchAlgorithm::Histogram)
                        {
                            if (imageMatchHistogramTable.find(imageName) == imageMatchHistogramTable.end())
                            {
                                imageMatchHistogramTable[imageName] = ComputeHistogram(refImage);
                            }
                        }
                        else if (matchAlgo == ImageMatchAlgorithm::Patch)
                        {
                            if (imageMatchPatchTable.find(imageName) == imageMatchPatchTable.end())
                            {
                                auto matcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(ObjectRecognize::CreateObjectRecognize()));                    
                                matcher->SetModelImage(refImage, false, 1.0f, GetRecordImagePrepType(),matcher->GetDefaultCurrentMatchPara().get());
                                imageMatchPatchTable[imageName] = matcher;
                            }
                        }
                    }
                }
                break;
            case QSEventAndAction::OPCODE_CONCURRENT_IF_MATCH:
                {
                    if(!TestManager::Instance().IsOfflineCheck())
                    {
                        auto imageObject = QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                        string imageFullPath = qs->GetResourceFullPath(imageObject->ImageName());
                        if (!is_regular_file(imageFullPath))
                        {
                            DAVINCI_LOG_ERROR << "Cannot find image: " << imageFullPath;
                            return false;
                        }
                        Mat eventImage = imread(imageFullPath);
                        if (eventImage.empty())
                        {
                            DAVINCI_LOG_WARNING << "Cannot load image (" << imageFullPath << "): " << qsEvent->ToString();
                            break;
                        }

                        DAVINCI_LOG_DEBUG << "Register concurrent events: " << qsEvent->ToString();
                        unsigned int instrCount;
                        if (qsEvent->IntOperand(0) <= 0)
                        {
                            instrCount = 0;
                        }
                        else
                        {
                            instrCount = static_cast<unsigned int>(qsEvent->IntOperand(0));
                        }
                        auto ccEvent = boost::shared_ptr<ConcurrentEvent>(new ConcurrentEvent(imageObject, ip, instrCount, qsEvent->IntOperand(1) == 1));
                        auto matcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
                        matcher->SetModelImage(eventImage, false, 1.0f, GetRecordImagePrepType());
                        ccEvent->featureMatcher = matcher;
                        state.ccEventTable[ip] = ccEvent;
                    }
                    else 
                    {
                        boost::shared_ptr<QScript::ImageObject> imageObject = QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                        OfflineImageCheckEvent offlineImageEvent = OfflineImageCheckEvent(imageObject, qsWatch.ElapsedMilliseconds(), qsEvent->LineNumber());
                        offlineImageEventCheckers.push_back(offlineImageEvent);
                    }
                }
                break;
            case QSEventAndAction::OPCODE_FPS_MEASURE_START:
                fpsStartImage = qsEvent->StrOperand(0);
                if (hasFpsInstr == false)
                {
                    hasFpsInstr = true;
                }
                fpsStartTime = qsEvent->TimeStamp();
                break;
            case QSEventAndAction::OPCODE_FPS_MEASURE_STOP:
                fpsStopImage = qsEvent->StrOperand(0);
                fpsStopTime = qsEvent->TimeStamp();
                break;
            default:
                break;
            }
        }

        ActivateTailingConcurrentEvents();

        if (hasFpsInstr)
        {
            if (!fpsStartImage.empty())
            {
                auto imageObject = QScript::ImageObject::Parse(fpsStartImage);
                string imageFullPath = qs->GetResourceFullPath(imageObject->ImageName());
                Mat eventStartImage = imread(imageFullPath);
                if (eventStartImage.empty())
                {
                    DAVINCI_LOG_WARNING << "Cannot load match start image: " << imageFullPath;
                }
                if (fpsTest == nullptr)
                {
                    AddFPSTest(qs->GetScriptName(), qs->GetPackageName());
                    fpsTest->SetFPSStartImage(eventStartImage, GetRecordImagePrepType(), GetReplayImagePrepType());
                }
            }

            if (!fpsStopImage.empty())
            {
                auto imageObject = QScript::ImageObject::Parse(fpsStopImage);
                string imageFullPath = qs->GetResourceFullPath(imageObject->ImageName());
                Mat eventStopImage = imread(imageFullPath);
                if (eventStopImage.empty())
                {
                    DAVINCI_LOG_WARNING << "Cannot load match stop image: " << imageFullPath;
                }
                if (fpsTest == nullptr)
                {
                    if (fpsStartTime >= .0)
                    {
                        AddFPSTest(qs->GetScriptName(), qs->GetPackageName());
                        if (fpsStartTime == .0)
                            fpsTest->SetStartAtInit(true);
                    }
                }
                fpsTest->SetFPSStopImage(eventStopImage, GetRecordImagePrepType(), GetReplayImagePrepType());
            }

            if (fpsTest == nullptr
                && fpsStartTime >= .0)
            {
                AddFPSTest(qs->GetScriptName(), qs->GetPackageName());
                if (fpsStartTime == .0)
                    fpsTest->SetStartAtInit(true);
            }
        }

        if (IsExploratoryQS(qsMode))
        {
            covCollector = boost::shared_ptr<CoverageCollector>(new CoverageCollector());
        }

        resultChecker = boost::shared_ptr<ResultChecker>(new ResultChecker(boost::dynamic_pointer_cast<ScriptReplayer>(shared_from_this())));

        assert(resultChecker != nullptr);

        if(isBreakPoint == false)
        {
            StartOnlineChecking();
        }

        string presetString = qs->GetConfiguration(QScript::ConfigCameraPresetting);
        CaptureDevice::Preset presetInConfig = CaptureDevice::Preset::PresetDontCare;
        if (boost::algorithm::iequals(presetString, "HIGH_SPEED"))
        {
            presetInConfig = CaptureDevice::Preset::HighSpeed;
        }
        else if (boost::algorithm::iequals(presetString, "HIGH_RES"))
        {
            presetInConfig = CaptureDevice::Preset::HighResolution;
        }
        else if (boost::algorithm::iequals(presetString, "MIDDLE_RES"))
        {
            presetInConfig = CaptureDevice::Preset::AIResolution;
        }
        else
        {
            DAVINCI_LOG_ERROR << "Error camera setting in Q script: " << presetString;
            return false;
        }
        boost::shared_ptr<CaptureDevice> cameraCap = DeviceManager::Instance().GetCurrentCaptureDevice();
        if (cameraCap != nullptr)
        {
            cameraCap->InitCamera(presetInConfig);
        }

        StartReplayTimer();

        return ScriptEngineBase::Init();
    }

#pragma region ConnectScript
    void ScriptReplayer::SetScriptReplayMode(ScriptReplayMode mode)
    {
        this->scriptReplayMode = mode;
    }

    void ScriptReplayer::SetBreakPointFrameSizePointAndOrientation(Size s, vector<Point> p, Orientation o, bool isTouchMove, bool tapMode)
    {
        this->breakPointFrameSize = s;
        this->breakPointFramePoints = p;
        this->breakPointFrameOrientation = o;
        this->isTouchMove = isTouchMove;
        this->needTapMode = tapMode;
        this->currentTouchMoveIndex = 0;
    }

    void ScriptReplayer::StopOnlineChecking()
    {
        resultChecker->StopOnlineChecking();

        TestManager::Instance().systemDiagnostic->StopPerfMonitor();
    }

    void ScriptReplayer::StartOnlineChecking()
    {
        resultChecker->StartOnlineChecking();

        TestManager::Instance().systemDiagnostic->StartPerfMonitor();
    }

    boost::shared_ptr<QScript> ScriptReplayer::GetQs()
    {
        return this->qs;
    }
#pragma endregion ConnectScript

    void ScriptReplayer::Destroy()
    {
        if (sensorCheckFlag == true)
        {
            SaveSensorCheckDataToFile();
        }

        if (resultChecker != nullptr)
            resultChecker->StopOnlineChecking();

        if (launchThread != nullptr)
        {
            launchThread->join();
            launchThread = nullptr;
        }

        if (replayTimer != nullptr) {
            replayTimer->Join();
            replayTimer = nullptr;
        }

        // Generate replay video, audio and qts files
        ScriptEngineBase::Destroy();
        // for compatible with old qts format.
        if (needVideoRecording)
        {
            ofstream qts;
            qts.open(qtsFileName, ios::out | ios::app);
            if (qts.is_open())
            {
                vector<QScript::TraceInfo> & trace = qs->Trace();
                for (auto traceInfo = trace.begin(); traceInfo != trace.end(); traceInfo++)
                {
                    qts << ":" << traceInfo->LineNumber() << ":" << static_cast<int64>(traceInfo->TimeStamp()) << endl;
                }
                qts.close();
            }
        }

        if (offlineImageEventCheckers.size() > 0)
        {
            HandleOfflineImageCheck();
        }

        if (IsSmokeQS(qsMode))
        {
            if(TestManager::Instance().IsCheckFlickering())
            {
                smokeTest->checkVideoFlickering(videoFileName);
            }
            smokeTest->generateAppStageResult();
            smokeTest->generateSmokeCriticalPath();
            SaveGeneratedScriptForSmokeTest();
        }

        if (IsExploratoryQS(qsMode) && this->covCollector != nullptr)
        {
            this->covCollector->SaveOneSeedCoverage();
            this->covCollector->SaveGlobalCoverage();
        }

        if (resultChecker != nullptr)
        {
            QReplayInfo qReplayInfo(timeStampList, qs->Trace());
            qReplayInfo.SaveToFile(qfdFileName);
            resultChecker->StartOfflineChecking();
            resultChecker->FinalReport();
        }

        if (dut != nullptr)
            dut->SetCurrentMultiLayerMode(dut->GetDefaultMultiLayerMode());
    }

    void ScriptReplayer::SaveSensorCheckDataToFile()
    {
        DAVINCI_LOG_DEBUG << string("The sensor check log is stored in: ") << sensorCheckFileName;
        if (boost::filesystem::exists(sensorCheckFileName))
        {
            DAVINCI_LOG_DEBUG << string("The sensor check log file already exists, will delete it!");
            boost::filesystem::remove(sensorCheckFileName);
        }
        DAVINCI_LOG_DEBUG << string("The sensor check log file doesn't exist, will create it!");
        ofstream file_out(sensorCheckFileName, std::ios_base::app | std::ios_base::out);

        for (unsigned int i = 0; i < sensorCheckData.size(); i++)
        {
            file_out << sensorCheckData[i] << endl;
        }
        file_out.flush();
        file_out.close();
        DAVINCI_LOG_INFO <<"     Sensor Check Log Address: ";
        DAVINCI_LOG_INFO << "file://" + sensorCheckFileName;
    }

    void ScriptReplayer::SetCaptureType()
    {
        isRecordedSoftCam = qs->UsedHyperSoftCam();
        isReplaySoftCam = DeviceManager::Instance().UsingHyperSoftCam();
    }

    void ScriptReplayer::SetSourceFrameResolution()
    {
        if(boost::filesystem::exists(qs->GetVideoFileName()))
        {
            VideoCaptureProxy cap;
            if (cap.open(qs->GetVideoFileName()))
            {
                sourceFrameWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
                sourceFrameHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
                cap.release();
            }
        }
    }

    void ScriptReplayer::SetSmokeOutputDir(string smokeOutput)
    {
        this->smokeOutputDir = smokeOutput;
    }

    DaVinciStatus ScriptReplayer::StartReplayTimer()
    {
        replayTimer = boost::shared_ptr<HighResolutionTimer>(new HighResolutionTimer());
        if(isBreakPoint)
            replayTimer->SetTimerHandler(boost::bind(&ScriptReplayer::BreakPointReplayHandler, this, _1));
        else
            replayTimer->SetTimerHandler(boost::bind(&ScriptReplayer::ReplayHandler, this, _1));
        return replayTimer->Start();
    }

    DaVinciStatus ScriptReplayer::StopReplayTimer()
    {
        replayTimer->SetTimerHandler(HighResolutionTimer::TimerCallbackFunc());
        return replayTimer->Stop();
    }

    bool ScriptReplayer::KeyOpcodeForSmoke(int opcode)
    {
        switch (opcode)
        {
        case QSEventAndAction::OPCODE_INSTALL_APP:
        case QSEventAndAction::OPCODE_START_APP:
        case QSEventAndAction::OPCODE_EXPLORATORY_TEST:
        case QSEventAndAction::OPCODE_BACK:
        case QSEventAndAction::OPCODE_UNINSTALL_APP:
            // adding OPCODE_STOP_APP for some app cannot be uninstalled without stopping early
        case QSEventAndAction::OPCODE_STOP_APP:
            // adding OPCODE_CLEAR_DATA to clear app cache
        case QSEventAndAction::OPCODE_CLEAR_DATA:
            return true;
        default:
            return false;
        }
    }

    bool ScriptReplayer::KeyOpcodeForCurrentCheck(int opcode)
    {
        switch (opcode)
        {
        case QSEventAndAction::OPCODE_EXPLORATORY_TEST:
        case QSEventAndAction::OPCODE_UNINSTALL_APP:
            return true;
        default:
            return false;
        }
    }

    void ScriptReplayer::EnableBreakPoint(bool flag)
    {
        isBreakPoint = flag;
    }

    bool ScriptReplayer::IsBreakPoint()
    {
        DAVINCI_LOG_DEBUG << "Set break point and wait ..." << endl;
        breakPointEvent.WaitOne();
        return true;
    }

    void ScriptReplayer::ReleaseBreakPoint()
    {
        DAVINCI_LOG_DEBUG << "Release break point ..." << endl;
        breakPointEvent.Set();
    }

    void ScriptReplayer::WaitBreakPointComplete()
    {
        DAVINCI_LOG_DEBUG << "Wait break point complete ..." << endl;
        completeBreakPointEvent.WaitOne();
    }

    void ScriptReplayer::CompleteBreakPoint()
    {
        DAVINCI_LOG_DEBUG << "Complete break point ..." << endl;
        completeBreakPointEvent.Set();
    }

    void ScriptReplayer::BreakPointReplayHandler(HighResolutionTimer &timer)
    {
        bool isAgentConnected = false;

        if (resultChecker->GetPauseFlag() || (!resultChecker->GetRunningFlag())) //Skip Agent connection check for USB plug out/in test or resultchecker stopped.
            isAgentConnected  = true;
        else if (dut == nullptr ||  dut->IsAgentConnected() || (!DeviceManager::Instance().GetEnableTargetDeviceAgent()))
            isAgentConnected = true;

        // Only for GET/RNR mode, when detect connection lost, try to reconnect once, for test some anti-virus software.
        if (!dut->IsAgentConnected())
        {
            double beforeStart = qsWatch.ElapsedMilliseconds();
            dut->Connect();

            // In this scenario, GET/RNR will finish current script immediately.
            if (!dut->IsAgentConnected())  
                isAgentConnected = false;

            state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeStart);
        }

        if (IsBreakPoint() && !IsFinished() && DaVinciSuccess(state.status) &&
            0 < state.currentQsIp && state.currentQsIp <= qs->NumEventAndActions() && isAgentConnected)
        {
            Mat frame = GetCurrentFrame();

            boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(state.currentQsIp);
            double previousQSEventTimestamp, currentQSEventTimestamp;

            while(qsEvent != nullptr && !IsFinished())
            {
                previousQSEventTimestamp = qsEvent->TimeStamp();
                ExecuteQSInstruction(qsEvent, frame);

                DAVINCI_LOG_INFO << "      Executing Q instruction " << qsEvent->ToString() << endl;

                // After execute QS instruction, currentQSIp is increased
                if(state.currentQsIp == endQsIp + 1)
                {
                    CompleteBreakPoint();
                    break;
                }
                else
                {
                    qsEvent = qs->EventAndAction(state.currentQsIp);
                    currentQSEventTimestamp = qsEvent->TimeStamp();
                    int waitTime = (int)(currentQSEventTimestamp - previousQSEventTimestamp);
                    if(waitTime < 0)
                        waitTime = 0;
                    waitQSEvent.WaitOne(waitTime);
                }
            }
        }
        else
        {
            StopReplayTimer();

            if (!isAgentConnected)
                resultChecker->Agent->SetResult(CheckerResult::Fail);

            if ((state.currentQsIp == 0) || (!DaVinciSuccess(state.status)))
            {
                if (resultChecker->Runtime != nullptr)
                    resultChecker->Runtime->IncrementErrorCount();

                if (!DaVinciSuccess(state.status))
                    DAVINCI_LOG_ERROR << "The script exited unexpectedly at IP: "<< state.currentQsIp << ", status code: " << state.status.message();
                else
                    DAVINCI_LOG_ERROR << "The script exited unexpectedly at IP: "<< state.currentQsIp;
            }

            DAVINCI_LOG_INFO << "Completed executing " << (state.currentQsIp - 1) << " out of " << qs->NumEventAndActions() << " QS instructions.";

            SetFinished();
        }
    }

    void ScriptReplayer::ReplayHandler(HighResolutionTimer &timer)
    {
        bool isAgentConnected = false;

        if (resultChecker->GetPauseFlag() || (!resultChecker->GetRunningFlag())) //Skip Agent connection check for USB plug out/in test or resultchecker stopped.
            isAgentConnected  = true;
        else if (dut == nullptr ||  dut->IsAgentConnected() || (!DeviceManager::Instance().GetEnableTargetDeviceAgent()))
            isAgentConnected = true;

        if (!IsFinished() && DaVinciSuccess(state.status) && !state.exited &&
            0 < state.currentQsIp && state.currentQsIp <= qs->NumEventAndActions() && isAgentConnected)
        {
            Mat frame = GetCurrentFrame();

            if (!qsWatch.IsStarted())
            {
                SetActive(true);
                qsWatch.Start();
            }

            if (frame.empty())
            {
                return;
            }

            boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(state.currentQsIp);
            if (!state.inConcurrentMatch && qsEvent != nullptr && !QScript::IsOpcodeInTouch(qsEvent->Opcode()))
            {
                boost::shared_ptr<FeatureMatcher> matcher = nullptr;
                for (auto ccPair : state.ccEventTable)
                {
                    if (ccPair.second->isActivated && ccPair.second->instrCount > 0)
                    {
                        if (matcher == nullptr)
                        {
                            matcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());

                            matcher->SetModelImage(frame,false,1.0,PreprocessType::Keep,(ccPair.second->featureMatcher)->GetFTParameters().get());
                        }
                        Mat homo = ccPair.second->featureMatcher->Match(*matcher, GetAltRatio(Size(frame.cols, frame.rows)));
                        if (!homo.empty() && ccPair.second->imageObject->AngleMatch(ccPair.second->featureMatcher->GetRotationAngle()))
                        {
                            auto ccQsEvent = qs->EventAndAction(ccPair.second->ccIp);
                            assert(ccQsEvent);
                            DAVINCI_LOG_INFO << "Start handling concurrent matching for " << ccQsEvent->StrOperand(0) << " at line: " << ccQsEvent->LineNumber();
                            // TODO:
                            //DrawDebugPolyline(frame, Array::ConvertAll<PointF, Point>(elem->surf->GetPts(), Point::Round));
                            state.inConcurrentMatch = true;
                            state.ipBeforeConcurrentMatch = state.currentQsIp;
                            state.instrCountLeft = ccPair.second->instrCount;
                            state.imageMatchTable[ccPair.second->imageObject->ImageName()] =
                                boost::shared_ptr<MatchedImageObject>(new MatchedImageObject(
                                ccPair.second->featureMatcher->GetObjectCenter(),
                                qsWatch.ElapsedMilliseconds(),
                                dut != nullptr ? dut->GetCurrentOrientation() : Orientation::Unknown)
                                );
                            if (ccPair.second->isMatchOnce)
                            {
                                ccPair.second->isActivated = false;
                            }
                            state.timeStampOffsetBeforeConcurrentMatch = state.timeStampOffset;
                            state.concurrentMatchStartTime = qsWatch.ElapsedMilliseconds();
                            state.timeStampOffset = ccQsEvent->TimeStamp() - state.concurrentMatchStartTime;
                            state.currentQsIp = ccPair.second->startIp;
                            qsEvent = qs->EventAndAction(state.currentQsIp);
                            break;
                        }
                    }
                }
            }

            double elapsed = qsWatch.ElapsedMilliseconds();

            if ((elapsed - lastCrashCheckTime) > CrashCheckerPeriod) // Trigger frame scan per 10.0S. Save crashdetector's ScreenCapture time.
            {
                if (qsEvent != nullptr)
                    resultChecker->Dialog->ScanFrame(frame, qsEvent->Opcode());

                lastCrashCheckTime = qsWatch.ElapsedMilliseconds();
            }

            while (qsEvent != nullptr && !IsFinished())
            {
                elapsed = qsWatch.ElapsedMilliseconds();
                double oldTimeStampOffset = state.timeStampOffset;
                if (elapsed + state.timeStampOffset < qsEvent->TimeStamp())
                {
                    break;
                }

                qs->SetEventOrActionTimeStampReplay(state.currentQsIp, elapsed);

                if (IsSmokeQS(qsMode))
                {
                    if (KeyOpcodeForSmoke(qsEvent->Opcode()))
                    {
                        if (needNextCheck)
                        {
                            smokeTest->setAppStageResult(GetCurrentFrame());
                            needNextCheck = false;
                        }

                        smokeTest->setCurrentSmokeStage(qsEvent->Opcode());
                        if (KeyOpcodeForCurrentCheck(qsEvent->Opcode()))
                        {
                            needCurrentCheck = true;
                        }
                        else
                        {
                            needNextCheck = true;
                        }
                    }
                }

                // Move out crash checking out of ExecuteQSInstruction
                resultChecker->UpdateCheckpoints(qsEvent->Opcode());
                resultChecker->Dialog->ScanFrame(frame, qsEvent->Opcode());
                lastCrashCheckTime = qsWatch.ElapsedMilliseconds();

                // only dump the execution log when we are not waiting for image matching
                if (!state.imageTimeOutWaiting)
                {
                    DAVINCI_LOG_INFO << "      Executing Q instruction " << qsEvent->ToString()
                        << " at timestamp: " << elapsed << ", late: "
                        << elapsed + oldTimeStampOffset - qsEvent->TimeStamp()
                        ;
                }

                StopWatch exeWatch;
                exeWatch.Start();
                ExecuteQSInstruction(qsEvent, frame);
                exeWatch.Stop();

                if(qsEvent->Opcode() == QSEventAndAction::OPCODE_INSTALL_APP)
                {
                    installComplete = true;
                }

                if (!DaVinciSuccess(state.status))
                {
                    break;
                }

                if (IsSmokeQS(qsMode) && needCurrentCheck)
                {
                    smokeTest->setAppStageResult(GetCurrentFrame());
                    needCurrentCheck = false;
                }

                if (state.inConcurrentMatch)
                {
                    if (--state.instrCountLeft == 0)
                    {
                        DAVINCI_LOG_INFO << "Stop handling and exit concurrent matching";
                        state.inConcurrentMatch = false;
                        state.currentQsIp = state.ipBeforeConcurrentMatch;
                        ThreadSleep(ccMatchLeaveDelay);
                        state.timeStampOffset = state.timeStampOffsetBeforeConcurrentMatch - (qsWatch.ElapsedMilliseconds() - state.concurrentMatchStartTime);
                    }
                }
                // we treat OPCODE_NEXT_SCRIPT as a hard exit as well so that
                // the following opcodes are not executed just like OPCODE_EXIT
                if ((qsEvent->Opcode() == QSEventAndAction::OPCODE_NEXT_SCRIPT) || (state.currentQsIp > qs->NumEventAndActions()))
                {
                    state.exited = true;
                }
                // isSingleStepSet is set to true when one QS instruction completed in ExecuteQSInstruction
                if (state.exited || state.currentQsIp > qs->NumEventAndActions() || !DaVinciSuccess(state.status) || !timer.IsStart())
                {
                    break;
                }
                qsEvent = qs->EventAndAction(state.currentQsIp);

                frame = GetCurrentFrame();
                if (frame.empty())
                {
                    return;
                }
            }
        }
        else
        {
            StopReplayTimer();

            if (!isAgentConnected)
                resultChecker->Agent->SetResult(CheckerResult::Fail);

            if ((state.currentQsIp == 0) || (!DaVinciSuccess(state.status)) || (!state.exited))
            {
                resultChecker->Runtime->IncrementErrorCount();

                if (!DaVinciSuccess(state.status))
                    DAVINCI_LOG_ERROR << "The script exited unexpectedly at IP: "<< state.currentQsIp << ", status code: " << state.status.message();
                else
                    DAVINCI_LOG_ERROR << "The script exited unexpectedly at IP: "<< state.currentQsIp;
            }

            DAVINCI_LOG_INFO << "Completed executing " << (state.currentQsIp - 1) << " out of " << qs->NumEventAndActions() << " QS instructions.";

            SetFinished();
        }
    }

    void ScriptReplayer::CheckSmokeMode()
    {
        int ip = 1;
        while (ip < (int)(qs->NumEventAndActions()))
        {
            boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(ip);
            if (qsEvent != nullptr && qsEvent->Opcode() == QSEventAndAction::OPCODE_EXPLORATORY_TEST)
            {
                string biasFile = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                if (!is_regular_file(biasFile))
                {
                    DAVINCI_LOG_WARNING << "Warning: bias xml does not exist.";
                    return;
                }

                biasObject = biasFileReader->loadDavinciConfig(biasFile);

                if (biasObject->GetTestType() == "SMOKE")
                {
                    qsMode = QS_SMOKE;
                    smokeTest->SetTest(boost::dynamic_pointer_cast<ScriptReplayer>(shared_from_this()));
                    smokeTest->SetCustomizedLogcatPackageNames(biasObject->GetLogcatPackageNames());

                    this->smokeOutputDir = TestReport::currentQsLogPath;
                    if (!boost::filesystem::exists(this->smokeOutputDir))
                        boost::filesystem::create_directory(this->smokeOutputDir);

                    boost::filesystem::path xmlPath(biasFile);

                    boost::filesystem::path xmlDir = xmlPath.parent_path();
                    string smokeDir = xmlDir.string();
                    string apkName = qs->GetApkName();
                    string apkPath = qs->GetResourceFullPath(apkName);

                    string packageName = qs->GetPackageName();
                    smokeTest->setPackageName(packageName);
                    smokeTest->prepareSmokeTest(smokeDir);
                    smokeTest->setApkName(apkName);
                    smokeTest->setApkPath(apkPath);
                    vector<string> appInfoSmoke = AndroidTargetDevice::GetPackageActivity(apkPath);
                    string appname = "";
                    if (appInfoSmoke.size() == AndroidTargetDevice::APP_INFO_SIZE)
                        appname = appInfoSmoke[AndroidTargetDevice::APP_NAME];
                    smokeTest->setAppName(appname);
                    smokeTest->setQscriptFileName(GetQsName());

                    break;
                }
                else if (biasObject->GetTestType() == "EXPLORATORY")
                {
                    qsMode = QS_EXPLORATORY;
                }
            }
            ip++;
        }

        if(qsMode == QS_EXPLORATORY || qsMode == QS_SMOKE)
        {
            path replayfolder(this->smokeOutputDir);
            replayfolder /= "Replay";
            boost::filesystem::create_directory(replayfolder);
            generateSmokeQsript = QScript::New(replayfolder.string() + "/" + GetQsName() + "_replay.qs");   
            generateSmokeQsript->SetConfiguration(QScript::ConfigPackageName, qs->GetPackageName());
            generateSmokeQsript->SetConfiguration(QScript::ConfigActivityName, qs->GetActivityName());
            generateSmokeQsript->SetConfiguration(QScript::ConfigStatusBarHeight, boost::lexical_cast<string>(dut->GetStatusBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetStatusBarHeight()[1]));
        
            
            generateSmokeQsript->SetConfiguration(QScript::ConfigNavigationBarHeight, boost::lexical_cast<string>(dut->GetNavigationBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetNavigationBarHeight()[1]));
        
            generateSmokeQsript->SetConfiguration(QScript::ConfigOSversion, dut->GetOsVersion(true));
            generateSmokeQsript->SetConfiguration(QScript::ConfigDPI, boost::lexical_cast<string>(dut->GetDPI()));
            generateSmokeQsript->SetConfiguration(QScript::ConfigResolution, boost::lexical_cast<string>(dut->GetDeviceWidth()) + "x" + boost::lexical_cast<string>(dut->GetDeviceHeight()));
            
            expEngine->SetGeneratedQScript(generateSmokeQsript);            
            generateSmokeQsript->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(0,
                QSEventAndAction::OPCODE_INSTALL_APP)));
            generateSmokeQsript->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(0,
                QSEventAndAction::OPCODE_START_APP)));

        }


    }

    bool ScriptReplayer::IsSmokeQS(QSMode mode)
    {
        if(mode == QS_SMOKE)
            return true;

        return false;
    }

    bool ScriptReplayer::IsExploratoryQS(QSMode mode)
    {
        if(mode == QS_SMOKE || mode == QS_EXPLORATORY)
            return true;

        return false;
    }

    bool ScriptReplayer::IsRnRQS(QSMode mode)
    {
        if(mode == QS_RNR)
            return true;

        return false;
    }

    bool ScriptReplayer::IsNormalQS(QSMode mode)
    {
        if(mode == QS_NORMAL)
            return true;

        return false;
    }

    Mat ScriptReplayer::ProcessFrame(const Mat &frame, const double timeStamp)
    {
        if (IsActive() == false)
        {
            return frame;
        }

        {
            SpinLockGuard lock(currentFrameLock);
            currentFrame = frame;
        }
        return ScriptEngineBase::ProcessFrame(frame, timeStamp);
    }

    void ScriptReplayer::ProcessWave(const boost::shared_ptr<vector<short>> &samples)
    {
        ScriptEngineBase::ProcessWave(samples);
        {
            boost::lock_guard<boost::mutex> lock(state.waveFileTableLock);
            for (auto waveFilePair : state.waveFileTable)
            {
                if (waveFilePair.second != nullptr)
                {
                    sf_write_short(waveFilePair.second->Handle(), &(*samples)[0], samples->size());
                }
            }
        }
    }

    Mat ScriptReplayer::GetCurrentFrame()
    {
        SpinLockGuard lock(currentFrameLock);
        return currentFrame;
    }

    void ScriptReplayer::AdjustBranchElapse(int qsIp, int nextQsIp)
    {
        boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(qsIp);
        boost::shared_ptr<QSEventAndAction> nextQsEvent = qs->EventAndAction(nextQsIp);
        if (qsEvent != nullptr && nextQsEvent != nullptr)
        {
            boost::shared_ptr<QSEventAndAction> beforeNextQsEvent = qs->EventAndAction(nextQsIp - 1);
            double nextRelativeTs = 0;
            if (beforeNextQsEvent != nullptr)
            {
                nextRelativeTs = nextQsEvent->TimeStamp() - beforeNextQsEvent->TimeStamp();
                if (nextRelativeTs < 0)
                {
                    nextRelativeTs = 0;
                }
            }
            state.timeStampOffset += nextQsEvent->TimeStamp() - qsEvent->TimeStamp() - nextRelativeTs;
        }
    }

    PreprocessType ScriptReplayer::GetRecordImagePrepType()
    {
        if (isRecordedSoftCam && !isReplaySoftCam)
        {
            return PreprocessType::Blur;
        }
        else if (!isRecordedSoftCam && isReplaySoftCam)
        {
            return PreprocessType::Sharpen;
        }
        else
        {
            return PreprocessType::Keep;
        }
    }

    PreprocessType ScriptReplayer::GetReplayImagePrepType()
    {
        if (isRecordedSoftCam && !isReplaySoftCam)
        {
            return PreprocessType::Sharpen;
        }
        else if (!isRecordedSoftCam && isReplaySoftCam)
        {
            return PreprocessType::Blur;
        }
        else
        {
            return PreprocessType::Keep;
        }
    }

    void ScriptReplayer::DrawDebugPolyline(const Mat &frame, const vector<Point2f> &points, Orientation devOrientation)
    {
        DrawLinesEventHandler drawLinesEvent = TestManager::Instance().GetDrawLinesEvent();
        if (drawLinesEvent != nullptr)
        {
            int width = frame.cols;
            int height = frame.rows;
            vector<ActionDotInfo> ptList = vector<ActionDotInfo>(points.size());
            for (int i=0; i < (int)points.size(); i++)
            {
                ptList[i].coorX = points[i].x;
                ptList[i].coorY = points[i].y;
                switch (devOrientation)
                {
                case DaVinci::Orientation::Portrait:
                    ptList[i].orientation = DeviceOrientation::OrientationPortrait;
                    break;
                case DaVinci::Orientation::Landscape:
                    ptList[i].orientation = DeviceOrientation::OrientationLandscape;
                    break;
                case DaVinci::Orientation::ReversePortrait:
                    ptList[i].orientation = DeviceOrientation::OrientationReversePortrait;
                    break;
                case DaVinci::Orientation::ReverseLandscape:
                    ptList[i].orientation = DeviceOrientation::OrientationReverseLandscape;
                    break;
                case DaVinci::Orientation::Unknown:
                    break;
                default:
                    break;
                }
            }

            for (int i=0; i < (int)points.size()-1; i++) 
            { 
                drawLinesEvent(&ptList[i], &ptList[i+1], width, height);
            }
        }
    }

    void ScriptReplayer::DeleteUnmatchedFrame(const Mat &frame, const string &imageName, int qsLineNumber)
    {
        string imageFullPath = qs->GetResourceFullPath(imageName);
        try
        {
            remove(qs->GetResourceFullPath(qs->GetScriptName() + "_UnmatchedFrame_" + boost::lexical_cast<string>(qsLineNumber) + string(".png")));
            remove(qs->GetResourceFullPath(qs->GetScriptName() + "_ReferenceImage_" + boost::lexical_cast<string>(qsLineNumber) + extension(path(imageFullPath))));
        }
        catch (...)
        {
            DAVINCI_LOG_DEBUG << "Cannot remove mismatched frame!";
        }
    }

    void ScriptReplayer::SaveUnmatchedFrame(const Mat &frame, const string &imageName, int qsLineNumber)
    {
        string imageFullPath = qs->GetResourceFullPath(imageName);
        try
        {
            imwrite(qs->GetResourceFullPath(qs->GetScriptName() + "_UnmatchedFrame_" + boost::lexical_cast<string>(qsLineNumber) + string(".png")), frame);
            copy_file(imageFullPath, qs->GetResourceFullPath(qs->GetScriptName() + "_ReferenceImage_" + boost::lexical_cast<string>(qsLineNumber) + extension(path(imageFullPath))), copy_option::overwrite_if_exists);
            // log mismatched frame
            imwrite(TestReport::currentQsLogPath 
                + string("\\") 
                + qs->GetScriptName() + "_UnmatchedFrame_" + boost::lexical_cast<string>(qsLineNumber) 
                + string(".png"), frame);
            copy_file(imageFullPath, 
                TestReport::currentQsLogPath 
                + string("\\") 
                + qs->GetScriptName() + "_ReferenceImage_" + boost::lexical_cast<string>(qsLineNumber) 
                + string(".png"), copy_option::overwrite_if_exists);

            string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";                        
            string resulttype = "type"; //<test-result type="RnR">
            string typevalue = "RNR"; 
            string commonnamevalue = qs->GetScriptName() + ".qs";
            string totalreasult = "FAIL"; 
            string itemname = "unmatchobject";
            string itemresult = "FAIL";
            string itemstarttime = "starttime";
            string itemendtime = "endtime";
            string opcodename = "OPCODE";
            string line = boost::lexical_cast<string>(qsLineNumber);
            string referencename = qs->GetScriptName() + "_ReferenceImage_" + boost::lexical_cast<string>(qsLineNumber) + extension(path(imageFullPath));
            string referenceurl = "./" + referencename;
            string targetname = qs->GetScriptName() + "_UnmatchedFrame_" + boost::lexical_cast<string>(qsLineNumber) + string(".png");
            string targeturl = "./" + targetname;

            testReportSummary->appendCommonNodeUnmatchObject(currentReportSummaryName,
                resulttype, typevalue, commonnamevalue,totalreasult, itemname, itemresult, 
                itemstarttime, itemendtime, opcodename, line, referencename, referenceurl, targetname,
                targeturl);
        }
        catch (...)
        {
            DAVINCI_LOG_DEBUG << "Cannot save mismatched frame";
        }
    }

    double ScriptReplayer::GetAltRatio(const Size &frameSize)
    {
        double ratio = 1.0;
        if (sourceFrameHeight != 0 && sourceFrameWidth != 0)
        {
            ratio = (sourceFrameHeight / sourceFrameWidth) / (static_cast<double>(frameSize.height) / frameSize.width);
        }
        return ratio;
    }

    int ScriptReplayer::HandleImageCheck(const Mat &frame, const boost::shared_ptr<QScript::ImageObject> &imageObject,
        int noLineNumber, int qsLineNumber, int &nextQsIp)
    {
        int noIp = qs->LineNumberToIp(noLineNumber);
        string imageName = imageObject->ImageName();
        string imageFullPath = qs->GetResourceFullPath(imageName);
        int checkResult = 0;

        if (imageMatchPatchTable.find(imageName) != imageMatchPatchTable.end())
        {
            auto matcher = imageMatchPatchTable[imageName];
            double ratio = GetAltRatio(Size(frame.cols, frame.rows));
            double ratioUnmatch = 1.0F;
            auto homo = matcher->Match(frame, 1.0f, ratio, GetReplayImagePrepType(), &ratioUnmatch);
            bool matched = false;
            if (!homo.empty())
            {
                matched = true;
                double userRatio = imageObject->RatioReferenceMatch();
                if (FEquals(userRatio, -1.0))
                    userRatio = 0.9;
                if (userRatio > 0 &&  userRatio < 1.0f
                    && (1.0 - ratioUnmatch) < userRatio)
                {
                    matched = false;
                }
            }
            if (matched)
            {
                DrawDebugPolyline(frame, matcher->GetPts(), Orientation::Portrait);
                checkResult = 0;
                DAVINCI_LOG_INFO << "      Match ratio is " <<  1.0 - ratioUnmatch;
            }
            else
            {                 
                checkResult = 1;
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << "     WARNING: image file " << imageFullPath << " does not exist @line " << qsLineNumber;
            checkResult = 2;
        }


        if (checkResult == 0)
        {
            nextQsIp = 0;
        }
        else
        {
            nextQsIp = noIp;
        }

        return checkResult;
    }

    int ScriptReplayer::HandleIfMatchImage(const Mat &frame, const boost::shared_ptr<QScript::ImageObject> &imageObject,
        int yesLineNumber, int noLineNumber, int qsLineNumber, int &nextQsIp, ImageMatchAlgorithm algorithm)
    {
        int yesIp = qs->LineNumberToIp(yesLineNumber);
        int noIp = qs->LineNumberToIp(noLineNumber);
        string imageName = imageObject->ImageName();
        double matchElapsed = qsWatch.ElapsedMilliseconds();
        string imageFullPath = qs->GetResourceFullPath(imageName);
        state.imageMatchTable.erase(imageName);
        int checkResult = 0;

        switch (algorithm)
        {
        case ImageMatchAlgorithm::Patch:
            if (imageMatchPatchTable.find(imageName) != imageMatchPatchTable.end())
            {
                auto matcher = imageMatchPatchTable[imageName];
                double ratio = GetAltRatio(Size(frame.cols, frame.rows));
                double ratioUnmatch = 1.0F;
                auto homo = matcher->Match(frame, 1.0f, ratio, GetReplayImagePrepType(), &ratioUnmatch);
                bool matched = false;
                if (!homo.empty() && imageObject->AngleMatch(matcher->GetRotationAngle()))
                {
                    matched = true;
                    float userRatio = imageObject->RatioReferenceMatch();
                    if (userRatio > 0 &&  userRatio < 1.0f
                        && (1.0 - ratioUnmatch) < userRatio)
                    {
                        matched = false;
                    }
                }
                if (matched)
                {
                    Point2f matchedCenter = matcher->GetObjectCenter();
                    Orientation o = Orientation::Unknown;
                    if (dut != nullptr)
                    {
                        o = dut->GetCurrentOrientation();
                    }
                    state.imageMatchTable[imageName] = boost::shared_ptr<MatchedImageObject>(new MatchedImageObject(matchedCenter, matchElapsed, o));
                    checkResult = 0;
                    DrawDebugPolyline(frame, matcher->GetPts(), o);
                    DAVINCI_LOG_INFO << "      Match ratio is " <<  1.0 - ratioUnmatch;
                }
                else
                {                 
                    checkResult = 1;
                }
            }
            else
            {
                DAVINCI_LOG_WARNING << "     WARNING: image file " << imageFullPath << " does not exist @line " << qsLineNumber;
                checkResult = 2;
            }
            break;
        case ImageMatchAlgorithm::Histogram:
            {
                // For handle black screen match (bug927)
                if (imageMatchHistogramTable.find(imageName) != imageMatchHistogramTable.end())
                {
                    vector<Mat> refHisto = imageMatchHistogramTable[imageName];
                    vector<Mat> observedHisto = ComputeHistogram(frame);
                    if (CompareHistogram(refHisto, observedHisto) > 0.8)
                    {
                        checkResult = 0;
                    }
                    else
                    {
                        checkResult = 1;
                    }
                }
                else
                {
                    DAVINCI_LOG_WARNING << "     WARNING: image file " << imageFullPath << " does not exist @line " << qsLineNumber;
                    checkResult = 2;
                }
            }
            break;
        default:
            DAVINCI_LOG_WARNING << "     WARNING: Unrecognized match Algorithm for " << imageFullPath << " @line " << qsLineNumber;
            checkResult = 2;
            break;
        }

        if (checkResult == 0)
        {
            nextQsIp = yesIp;
        }
        else
        {
            nextQsIp = noIp;
        }

        return checkResult;
    }

    int ScriptReplayer::HandleIfMatchLayout(const string &targetLayout, const string &sourceLayout,
        int yesLineNumber, int noLineNumber, int qsLineNumber, int &nextQsIp)
    {
        int yesIp = yesLineNumber;
        int noIp = noLineNumber;
        string targetLayoutFullPath = qs->GetResourceFullPath(targetLayout);
        string sourceLayoutFullPath = qs->GetResourceFullPath(sourceLayout);
        if (exists(targetLayoutFullPath) && exists(sourceLayoutFullPath))
        {
            if (IsLayoutMatch(sourceLayoutFullPath, targetLayoutFullPath))
            {
                nextQsIp = yesIp;
                return 0;
            }
            else
            {
                nextQsIp = noIp;
                return 1;
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << "layout file does not exist @line " << qsLineNumber;
            nextQsIp = noIp;
            return 2;
        }
    }

    bool ScriptReplayer::IsLayoutMatch(const string &s, const string &t)
    {
        vector<string> sLines;
        vector<string> tLines;
        ReadAllLines(s, sLines);
        ReadAllLines(t, tLines);
        if (sLines.size() != tLines.size())
        {
            return false;
        }
        else
        {
            for (int i = 0; i < static_cast<int>(sLines.size()); i++)
            {
                if (sLines[i] != tLines[i])
                {
                    return false;
                }
            }
        }

        return true;
    }

    void ScriptReplayer::HandleOfflineImageCheck(){
        unsigned int i = 0;
        while(i < offlineImageEventCheckers.size()){
            string imageFullPath = qs->GetResourceFullPath(offlineImageEventCheckers[i].imageObject->ImageName());
            Mat eventImage = imread(imageFullPath);
            if (eventImage.empty())
            {

                DAVINCI_LOG_WARNING << "Reference image (" << imageFullPath << "): not found @" << offlineImageEventCheckers[i].opcodeLine;
                testReportSummary->appendTxtReportSummary("Reference image (" + imageFullPath + "): not found @" + boost::lexical_cast<string>( offlineImageEventCheckers[i].opcodeLine));
                offlineImageEventCheckers.erase(offlineImageEventCheckers.begin() + i);
            }else{
                boost::shared_ptr<FeatureMatcher> matcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
                matcher->SetModelImage(eventImage, false, 1.0f, GetRecordImagePrepType());
                offlineImageEventCheckers[i].featureMatcher = matcher;
                i++;
            }
        }

        vector<QScript::TraceInfo> & trace = qs->Trace();
        unsigned int imageStartIndex = 0;
        unsigned int traceIndex = 0;
        if(trace.size() > 0)
        {
            for(i = 0; i < trace.size(); i++)
            {
                if(trace[i].LineNumber() == offlineImageEventCheckers[0].opcodeLine){
                    traceIndex = i;
                    break;
                }
            }         
            for(i = 0; i < timeStampList.size(); i++)
            {
                if(timeStampList[i] > trace[traceIndex].TimeStamp())
                {
                    imageStartIndex = i;
                    break;
                }
            }
        }       

        VideoCaptureProxy cap(videoFileName);
        cv::Mat vf;
        cv::Mat lastFrame;
        i = imageStartIndex;
        double maxCaptureFrame = cap.get(CV_CAP_PROP_FRAME_COUNT);

        for( unsigned int startFrame = imageStartIndex; startFrame < maxCaptureFrame; startFrame += 10)
        {
            cap.read(vf);
            bool findMatch = false;
            unsigned int j = 0;
            while(j < offlineImageEventCheckers.size())
            {
                boost::shared_ptr<FeatureMatcher> matcher = offlineImageEventCheckers[j].featureMatcher;
                double ratio = 1.0;
                if(sourceFrameHeight != 0 && sourceFrameWidth !=0){
                    ratio = ((double)sourceFrameHeight / sourceFrameWidth) / (vf.rows / vf.cols);
                }

                Mat homo = matcher->Match(vf, 1.0f, ratio, GetReplayImagePrepType());
                if (!homo.empty() && offlineImageEventCheckers[j].imageObject->AngleMatch(matcher->GetRotationAngle())){
                    findMatch = true;
                    break;
                }else{
                    j++;
                }
            }
            if(findMatch){
                startFrame = (startFrame-11) > 0 ? (startFrame-11): 0;
                i = startFrame;
                cap.set(CV_CAP_PROP_POS_FRAMES, startFrame);
                break;
            }
            cap.set(CV_CAP_PROP_POS_FRAMES, startFrame+10);
        }

        cap.set(CV_CAP_PROP_POS_FRAMES, i);

        while(i < maxCaptureFrame)
        {
            cap.read(vf);
            unsigned int j = 0;
            while(j < offlineImageEventCheckers.size())
            {
                boost::shared_ptr<FeatureMatcher> matcher = offlineImageEventCheckers[j].featureMatcher;
                double ratio = 1.0;
                if(sourceFrameHeight != 0 && sourceFrameWidth !=0){
                    ratio = ((double)sourceFrameHeight / sourceFrameWidth) / (vf.rows / vf.cols);
                }

                Mat homo = matcher->Match(vf, 1.0f, ratio, GetReplayImagePrepType());
                if (!homo.empty() && offlineImageEventCheckers[j].imageObject->AngleMatch(matcher->GetRotationAngle())){
                    if(i < ScriptEngineBase::timeStampList.size()){
                        DAVINCI_LOG_INFO << offlineImageEventCheckers[j].imageObject->ImageName() << " matched at: " <<  boost::lexical_cast<string>(ScriptEngineBase::timeStampList[i]- offlineImageEventCheckers[j].elapsed) + " ms";
                        testReportSummary->appendTxtReportSummary(offlineImageEventCheckers[j].imageObject->ImageName() + " matched at: " + boost::lexical_cast<string>(ScriptEngineBase::timeStampList[i]- offlineImageEventCheckers[j].elapsed) + " ms");
                        string matchedTimeValue = boost::lexical_cast<string>(boost::format("%.3f") % (ScriptEngineBase::timeStampList[i]- offlineImageEventCheckers[j].elapsed)) + " ms";
                        string referenceImage_ = TestReport::currentQsLogPath + string("\\") + offlineImageEventCheckers[j].imageObject->ImageName();
                        string referenceName = offlineImageEventCheckers[j].imageObject->ImageName();
                        string referenceUrl = "./" + referenceName;
                        string imageFullPath = qs->GetResourceFullPath(offlineImageEventCheckers[j].imageObject->ImageName());
                        CopyFile(imageFullPath.c_str(), referenceImage_.c_str(), true);
                        string targetImage = "matched_" + offlineImageEventCheckers[j].imageObject->ImageName();
                        string targetUrl = "./" + targetImage;
                        string targetFullPath = TestReport::currentQsLogPath + "\\" + targetImage;
                        imwrite(boost::filesystem::system_complete(targetFullPath).string(), vf);
                        string beforeImage = targetImage;

                        if(!lastFrame.empty()){
                            beforeImage = "before_"  + offlineImageEventCheckers[j].imageObject->ImageName();
                            string beforeImageFullPath = TestReport::currentQsLogPath + "\\" + beforeImage;
                            imwrite(boost::filesystem::system_complete(beforeImageFullPath).string(), lastFrame);
                        }
                        string beforeUrl = "./" + beforeImage;
                        string startTime = "N/A";
                        string endTime = "N/A";
                        string xmlFile = TestReport::currentQsLogPath + "\\ReportSummary.xml";

                        testReportSummary->appendLaunchTimeResultNode(xmlFile,
                            "type",
                            "RNR",
                            "name",
                            GetQsName(),
                            "N/A",/* totalreasultvalue,*/
                            "N/A", /* totalmessagevalue, */
                            "N/A", /* logcattxt, */
                            "Time measurement", //sub case name
                            "Matched time", // result
                            matchedTimeValue,
                            referenceName,
                            referenceUrl ,/*referenceimageurl,*/
                            targetImage,
                            targetUrl,
                            beforeImage,
                            beforeUrl,
                            startTime, //start time
                            endTime, //end time
                            "N/A", //message
                            "N/A");



                    }

                    offlineImageEventCheckers.erase(offlineImageEventCheckers.begin() + j);
                }else{
                    j++;
                }

            }

            i++;
            cap.set(CV_CAP_PROP_POS_FRAMES, i);
            lastFrame = vf.clone();
        }

        for(unsigned int i = 0; i < offlineImageEventCheckers.size(); i++){
            string imageFullPath = qs->GetResourceFullPath(offlineImageEventCheckers[i].imageObject->ImageName());
            Mat eventImage = imread(imageFullPath);
            DAVINCI_LOG_WARNING << "Reference image (" << imageFullPath << "): not matched " ;
            testReportSummary->appendTxtReportSummary("Reference image (" + imageFullPath + "): not matched ");
        }
    }

    ScriptReplayer::ImageMatchAlgorithm ScriptReplayer::OperandToImageMatchAlgorithm(int operand) const
    {
        switch (operand)
        {
        case 0:
            return ImageMatchAlgorithm::Patch;
        case 1:
            return ImageMatchAlgorithm::Histogram;
        default:
            return ImageMatchAlgorithm::Patch;
        }
    }

    std::unordered_map<string, string> ScriptReplayer::ParseKeyValues(string str)
    {
        std::unordered_map<string, string> keyValueMap;
        string trimStr = boost::trim_copy(str);
        if(trimStr.length() == 0)
            return keyValueMap;

        vector<string> pairs;
        boost::algorithm::split(pairs, trimStr, boost::algorithm::is_any_of("&"));
        for(auto pair : pairs)
        {
            string trimPair = boost::trim_copy(pair);
            if(trimPair.length() == 0)
                continue;

            vector<string> items;
            boost::algorithm::split(items, trimPair, boost::algorithm::is_any_of("="));

            if(keyValueMap.find(items[0]) == keyValueMap.end())
            {
                string key = items[0];
                string value = items[1];
                keyValueMap[key] = value;
            }
        }

        return keyValueMap;
    }

    void ScriptReplayer::ExecuteQSInstruction(const boost::shared_ptr<QSEventAndAction> &qsEvent, const Mat &frame)
    {
        ActionDotInfo dotInfo;

        int nextQsIp = state.currentQsIp + 1;
        boost::shared_ptr<QSEventAndAction> evt = nullptr;
        // TODO: Mat debugLayer = TestManager::GetDebugLayer();
        switch (qsEvent->Opcode())
        {
        case QSEventAndAction::OPCODE_CLICK:
            // To avoid refresh orientation, we call treouch directly
            if (dut != nullptr)
            {
                state.status = dut->Click(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
            }
            break;
        case QSEventAndAction::OPCODE_HOME:
            if (dut != nullptr)
            {
                if (IsSmokeQS(qsMode)) 
                {
                    // For lenovo devices.
                    state.status = dut->PressBack(); 
                }

                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->PressHome(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->PressHome(ButtonEventMode::Up);
                    else
                        state.status = dut->PressHome(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->PressHome(ButtonEventMode::DownAndUp);

                if (IsSmokeQS(qsMode))
                {
                    ThreadSleep(homeWaitMilliseconds);
                    if(dut->GetTopActivityName() == launcherRecognizer->GetLauncherSelectTopActivity())
                    {
                        Mat currentFrame = GetCurrentFrame();
                        if (!currentFrame.empty())
                        {
                            Mat rotatedFrame = RotateFrameUp(currentFrame);
                            launcherRecognizer->SetIconName(qs->GetIconName());
                            expEngine->generateAllRectsAndKeywords(rotatedFrame, Rect(0, 0, rotatedFrame.size().width, rotatedFrame.size().height), ObjectUtil::Instance().GetSystemLanguage());
                            launcherRecognizer->RecognizeLauncherCandidate(rotatedFrame, expEngine->getAllEnProposalRects(), expEngine->getAllZhProposalRects());
                            Rect launcherAppRect = launcherRecognizer->GetLauncherAppRect();
                            Rect alwaysRect = launcherRecognizer->GetAlwaysRect();
                            ClickOnRotatedFrame(rotatedFrame.size(), Point(launcherAppRect.x + launcherAppRect.width / 2, launcherAppRect.y + launcherAppRect.height / 2));
                            ClickOnRotatedFrame(rotatedFrame.size(), Point(alwaysRect.x + alwaysRect.width / 2, alwaysRect.y + alwaysRect.height / 2));
                        }
                    }
                }
            }

            if (IsSmokeQS(qsMode)) 
            {
                ThreadSleep(1000);
                Mat frame = GetCurrentFrame();
                if(!frame.empty())
                {
                    Mat rotatedFrame = RotateFrameUp(frame);
                    SaveImage(rotatedFrame, ConcatPath(TestReport::currentQsLogPath,"home_frame.png"));
                }
            }
            break;
        case QSEventAndAction::OPCODE_BACK:
            // com.sec.android.easyMover will turn off wifi when press back (even no other random actions)
            if (IsSmokeQS(qsMode) && (smokeTest->getSkipStatus() || AppUtil::Instance().IsCloseWifiApp(qs->GetPackageName())))
            {
                break;
            }
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->PressBack(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->PressBack(ButtonEventMode::Up);
                    else
                        state.status = dut->PressBack(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->PressBack(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_MENU:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->PressMenu(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->PressMenu(ButtonEventMode::Up);
                    else
                        state.status = dut->PressMenu(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->PressMenu(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_DRAG:
            if (dut != nullptr)
            {
                state.status = dut->Drag(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), qsEvent->IntOperand(3), qsEvent->GetOrientation());
            }
            break;
        case QSEventAndAction::OPCODE_START_APP:
            if (dut != nullptr)
            {
                double beforeStart = qsWatch.ElapsedMilliseconds();
                bool startInSyncMode = true;

                if (IsSmokeQS(qsMode) && smokeTest->getSkipStatus())
                {
                    break;
                }

                if (IsSmokeQS(qsMode))
                {
                    boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                    if (androidDut != nullptr)
                    {
                        androidDut->AdbCommand("logcat -c");
                    }

                    startInSyncMode = false;
                }

                DAVINCI_LOG_INFO << "Launch app...";

                launchThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&ExploratoryEngine::ClickOffLaunchDialog, (boost::dynamic_pointer_cast<ExploratoryEngine>(expEngine)), boost::ref(dut))));

                SetThreadName(launchThread->get_id(), "Click launch dialog");
                const int WaitForLaunch = 3000;
                ThreadSleep(WaitForLaunch);

                if (qsEvent->NumStrOperands() > 0 && !qsEvent->StrOperand(0).empty())
                {
                    state.status = dut->StartActivity(qsEvent->StrOperand(0), startInSyncMode);
                }
                else if (!qs->GetPackageName().empty() || !qs->GetActivityName().empty())
                {
                    boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                    if (androidDut != nullptr)
                    {
                        if (qs->GetActivityName().empty())
                        {
                            string launchActivity = androidDut->GetLaunchActivityByQAgent(qs->GetPackageName());
                            if(IsSmokeQS(qsMode))
                                smokeTest->setActivityName(launchActivity);
                            state.status = dut->StartActivity(qs->GetPackageName() + "/" + launchActivity, startInSyncMode);
                        }
                        else if(qs->GetActivityName().find("/") != string::npos)
                        {
                            state.status = dut->StartActivity(qs->GetActivityName(), startInSyncMode);
                        }
                        else
                        {
                            state.status = dut->StartActivity(qs->GetPackageName() + "/" + qs->GetActivityName(), startInSyncMode);
                        }
                    }
                    else
                    {
                        state.status = dut->StartActivity(qs->GetActivityName(), startInSyncMode);
                    }
                }
                ThreadSleep(WaitForLaunch);

                if (IsSmokeQS(qsMode) && !qs->GetActivityName().empty())
                {
                    smokeTest->setActivityName(qs->GetActivityName());
                }

                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeStart);
            }
            break;
        case QSEventAndAction::OPCODE_SET_CLICKING_HORIZONTAL:
            if (qsEvent->IntOperand(0) == 0)
            {
                state.clickInHorizontal = false;
            }
            else
            {
                state.clickInHorizontal = true;
            }
            break;
        case QSEventAndAction::OPCODE_TOUCHDOWN:
            {
                // Check whether current a series of actions are swipe action or not
                if (IsSwipeAction(qs, state.currentQsIp))
                    dut->MarkSwipeAction();

                // To avoid refresh orientation, we call touch directly
                double deltaTimeStampOffset = 0;
                if (dut != nullptr)
                {
                    if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                    {
                        double start = qsWatch.ElapsedMilliseconds();
                        string keyValue = qsEvent->StrOperand(0);
                        keyValueMap = ParseKeyValues(keyValue);
                        string idKey = "id", ratioKey = "ratio", layoutKey = "layout";

                        Point idOnDevice = EmptyPoint;
                        Rect idRect;
                        Rect sbRect, nbRect, fwRect;
                        bool hasKeyboardFromWindow;
                        ScriptHelper::ProcessWindowInfo(keyValueMap, sbRect,  nbRect, hasKeyboardFromWindow, fwRect);

                        Point clickPointOnDevice(qsEvent->IntOperand(0) * recordDeviceSize.width / maxWidthHeight, qsEvent->IntOperand(1) * recordDeviceSize.height / maxWidthHeight);
                        clickPointOnDevice = TransformDevicePoint(clickPointOnDevice, qsEvent->GetOrientation(), recordDeviceSize.size(), Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()));

                        vector<string> lines = dut->GetTopActivityViewHierarchy();
                        string treeFile = ConcatPath(TestReport::currentQsLogPath, "replay_" + qs->GetPackageName() + ".qs_" + boost::lexical_cast<string>(++treeId) + ".tree");
                        std::ofstream treeStream(treeFile, std::ios_base::app | std::ios_base::out);
                        for(auto line: lines)
                            treeStream << line << std::flush;
                        treeStream.flush();
                        treeStream.close();

                        boost::shared_ptr<ViewHierarchyTree> replayRoot = viewHierarchyParser->ParseTree(lines);
                        map<string, vector<Rect>> replayTreeControls = viewHierarchyParser->GetTreeControls();
                        viewHierarchyParser->CleanTreeControls();

                        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                        boost::shared_ptr<AndroidTargetDevice> adut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);

                        AndroidTargetDevice::WindowBarInfo windowBarInfo;
                        vector<string> windowLines;
                        if(adut != nullptr)
                        {
                            windowBarInfo = adut->GetWindowBarInfo(windowLines);
                            string windowFileName = "replay_" +  qs->GetPackageName() + ".qs_" + boost::lexical_cast<string>(treeId) + ".window";
                            string windowFilePath = ConcatPath(TestReport::currentQsLogPath, windowFileName);
                            std::ofstream windowStream(windowFilePath, std::ios_base::app | std::ios_base::out);
                            for(auto line: windowLines)
                                windowStream << line << std::flush;
                            windowStream.flush();
                            windowStream.close();
                        }

                        if(keyValueMap.find(idKey) != keyValueMap.end())
                        {
                            if(keyValueMap.find(layoutKey) != keyValueMap.end())
                            {
                                boost::filesystem::path qsPath(qsFileName);
                                qsPath = system_complete(qsPath);
                                boost::filesystem::path qsFolder = qsPath.parent_path();
                                boost::filesystem::path layoutPath = qsFolder / keyValueMap[layoutKey];

                                if(boost::filesystem::exists(layoutPath))
                                {
                                    string layoutFile = layoutPath.string();
                                    vector<string> lines;
                                    ReadAllLines(layoutFile, lines);
                                    boost::shared_ptr<ViewHierarchyTree> recordRoot = viewHierarchyParser->ParseTree(lines);
                                    viewHierarchyParser->CleanTreeControls();
                                    vector<boost::shared_ptr<ViewHierarchyTree>> nodes;
                                    viewHierarchyParser->FindValidNodesWithTreeByPoint(recordRoot, clickPointOnDevice, fwRect, nodes);
                                    vector<int> treeInfos;
                                    boost::shared_ptr<ViewHierarchyTree> recordNode = viewHierarchyParser->FindMatchedIdByTreeInfo(recordRoot, nodes, clickPointOnDevice, treeInfos);
                                    if(recordNode != nullptr)
                                    {
                                        boost::shared_ptr<ViewHierarchyTree> replayNode = viewHierarchyParser->FindMatchedRectByIdAndTree(treeInfos, replayRoot);
                                        if(replayNode == nullptr || !boost::equals(recordNode->id, replayNode->id))
                                        {
                                            DAVINCI_LOG_WARNING << "Cannot find replay node from tree or id is different";
                                            idOnDevice = EmptyPoint;
                                        }
                                        else
                                        {
                                            needClickId = true;
                                            idRect = replayNode->rect;
                                            double xRatio = (clickPointOnDevice.x - recordNode->rect.x) / double(recordNode->rect.width);
                                            double yRatio = (clickPointOnDevice.y - recordNode->rect.y) / double(recordNode->rect.height);
                                            idOnDevice.x = idRect.x + (int)(xRatio * idRect.width);
                                            idOnDevice.y = idRect.y + (int)(yRatio * idRect.height);
                                        }
                                    }
                                    else
                                    {
                                        DAVINCI_LOG_WARNING << "Cannot find record node from tree.";
                                        idOnDevice = EmptyPoint;
                                    }
                                }
                                else
                                {
                                    DAVINCI_LOG_WARNING << "Cannot find record tree.";
                                    idOnDevice = EmptyPoint;
                                }
                            }

                            // Tree match fails
                            if(idOnDevice == EmptyPoint)
                            {
                                // ID exists for match
                                needClickId = true;
                                idRect = viewHierarchyParser->FindMatchedRectById(replayTreeControls, keyValueMap[idKey], windowBarInfo.focusedWindowRect);
                                string ratioStr;
                                if(keyValueMap.find(ratioKey) != keyValueMap.end())
                                    ratioStr = keyValueMap[ratioKey];

                                idRectCenterPoint.x = idRect.x + idRect.width / 2;
                                idRectCenterPoint.y = idRect.y + idRect.height / 2;

                                vector<string> ratioItems;
                                boost::algorithm::split(ratioItems, ratioStr, boost::algorithm::is_any_of(","));
                                if(ratioItems.size() != 2)
                                {
                                    idOnDevice.x = idRectCenterPoint.x;
                                    idOnDevice.y = idRectCenterPoint.y;
                                }
                                else
                                {
                                    string xRatioStr = ratioItems[0];
                                    string yRatioStr = ratioItems[1];
                                    xRatioStr = xRatioStr.substr(1, xRatioStr.length() - 1);
                                    yRatioStr = yRatioStr.substr(0, yRatioStr.length() - 1);
                                    double xRatio = 0.5, yRatio = 0.5;
                                    TryParse(xRatioStr, xRatio);
                                    TryParse(yRatioStr, yRatio);
                                    idOnDevice.x = idRect.x + (int)(xRatio * idRect.width);
                                    idOnDevice.y = idRect.y + (int)(yRatio * idRect.height);
                                }
                            }

                            idXOffset = idOnDevice.x - clickPointOnDevice.x;
                            idYOffset = idOnDevice.y - clickPointOnDevice.y;

                            Orientation currentOrientation = dut->GetCurrentOrientation();
                            if(currentOrientation != qsEvent->GetOrientation())
                            {
                                idOnDevice.x = idRectCenterPoint.x;
                                idOnDevice.y = idRectCenterPoint.y;
                            }

                            double end = qsWatch.ElapsedMilliseconds();
                            deltaTimeStampOffset = (end - start);
                            state.status = TouchOnDevice(dut, Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()), idOnDevice.x, idOnDevice.y, currentOrientation, "down");
                        }
                        else
                        {
                            state.status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
                        }
                    }
                    else
                    {
                        if(breakPointFramePoints.size() > 0)
                        {
                            breakPointFramePoint = breakPointFramePoints[0];
                            breakPointFramePoints.erase(breakPointFramePoints.begin());
                            state.status = TouchDownOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                        }
                    }

                    if (dut->GetCurrentOrientation() == Orientation::Portrait)
                    {
                        dotInfo.orientation = DeviceOrientation::OrientationPortrait;
                    }
                    else if (dut->GetCurrentOrientation() == Orientation::ReversePortrait)
                    {
                        dotInfo.orientation = DeviceOrientation::OrientationReversePortrait;
                    }
                    else if (dut->GetCurrentOrientation() == Orientation::Landscape)
                    {
                        dotInfo.orientation=  DeviceOrientation::OrientationLandscape;
                    }
                    else
                    {
                        dotInfo.orientation = DeviceOrientation::OrientationReverseLandscape;
                    }
                    dotInfo.coorX = static_cast<float>(qsEvent->IntOperand(0));
                    dotInfo.coorY = static_cast<float>(qsEvent->IntOperand(1));
                    DrawDotEventHandler drawDotEvent = TestManager::Instance().GetDrawDotsEvent();
                    if (drawDotEvent != nullptr)
                    {
                        drawDotEvent(&dotInfo);
                    }
                }
                state.timeStampOffset -= deltaTimeStampOffset;
                break;
            }
        case QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly

            // Check whether current a series of actions are swipe action or not
            if (IsSwipeAction(qs, state.currentQsIp))
                dut->MarkSwipeAction();

            if (dut != nullptr)
            {
                if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                {
                    state.status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
                }
                else
                {
                    if(breakPointFramePoints.size() > 0)
                    {
                        breakPointFramePoint = breakPointFramePoints[0];
                        breakPointFramePoints.erase(breakPointFramePoints.begin());
                        state.status = TouchDownOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                    }
                }
                dotInfo.coorX = static_cast<float>(qsEvent->IntOperand(0));
                dotInfo.coorY = static_cast<float>(qsEvent->IntOperand(1));
                //TODO: Check whether the orientation info is correct or not
                dotInfo.orientation = DeviceOrientation::OrientationLandscape;
                DrawDotEventHandler drawDotEvent = TestManager::Instance().GetDrawDotsEvent();
                if (drawDotEvent != nullptr)
                {
                    drawDotEvent(&dotInfo);
                }
            }
            break;
        case QSEventAndAction::OPCODE_TOUCHUP:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                {
                    if(needClickId)
                    {
                        Orientation currentOrientation = dut->GetCurrentOrientation();
                        if(currentOrientation == qsEvent->GetOrientation())
                        {
                            Point clickPointOnDevice(qsEvent->IntOperand(0) * recordDeviceSize.width / maxWidthHeight, qsEvent->IntOperand(1) * recordDeviceSize.height / maxWidthHeight);
                            clickPointOnDevice = TransformDevicePoint(clickPointOnDevice, qsEvent->GetOrientation(), recordDeviceSize.size(), Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()));

                            Point idOnDevice(clickPointOnDevice.x + idXOffset, clickPointOnDevice.y + idYOffset);
                            state.status = TouchOnDevice(dut, Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()), idOnDevice.x, idOnDevice.y, currentOrientation, "up");
                        }
                        else
                        {
                            state.status = TouchOnDevice(dut, Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()), idRectCenterPoint.x, idRectCenterPoint.y, currentOrientation, "up");
                        }
                        needClickId = false;
                        keyValueMap.clear();
                        idXOffset = 0;
                        idYOffset = 0;
                    }
                    else
                    {
                        state.status = dut->TouchUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
                    }
                }
                else
                {
                    if(breakPointFramePoints.size() > 0)
                    {
                        breakPointFramePoint = breakPointFramePoints[0];
                        breakPointFramePoints.erase(breakPointFramePoints.begin());
                        state.status = TouchUpOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                    }
                }

                if (dut->GetCurrentOrientation() == Orientation::Portrait)
                {
                    dotInfo.orientation = DeviceOrientation::OrientationPortrait;
                }
                else if (dut->GetCurrentOrientation() == Orientation::ReversePortrait)
                {
                    dotInfo.orientation = DeviceOrientation::OrientationReversePortrait;
                }
                else if (dut->GetCurrentOrientation() == Orientation::Landscape)
                {
                    dotInfo.orientation = DeviceOrientation::OrientationLandscape;
                }
                else
                {
                    dotInfo.orientation = DeviceOrientation::OrientationReverseLandscape;
                }
                dotInfo.coorX = static_cast<float>(qsEvent->IntOperand(0));
                dotInfo.coorY = static_cast<float>(qsEvent->IntOperand(1));
                DrawDotEventHandler drawDotEvent = TestManager::Instance().GetDrawDotsEvent();
                if (drawDotEvent != nullptr)
                {
                    drawDotEvent(&dotInfo);
                }
            }
            break;
        case QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                {
                    state.status = dut->TouchUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
                }
                else
                {
                    if(breakPointFramePoints.size() > 0)
                    {
                        breakPointFramePoint = breakPointFramePoints[0];
                        breakPointFramePoints.erase(breakPointFramePoints.begin());
                        state.status = TouchUpOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                    }
                }
                dotInfo.coorX = static_cast<float>(qsEvent->IntOperand(0));
                dotInfo.coorY = static_cast<float>(qsEvent->IntOperand(1));
                //TODO: Check whether the orientation info is correct or not
                dotInfo.orientation = DeviceOrientation::OrientationLandscape;
                DrawDotEventHandler drawDotEvent = TestManager::Instance().GetDrawDotsEvent();
                if (drawDotEvent != nullptr)
                {
                    drawDotEvent(&dotInfo);
                }
            }
            break;
        case QSEventAndAction::OPCODE_TOUCHMOVE:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                {
                    if(needClickId)
                    {
                        Orientation currentOrientation = dut->GetCurrentOrientation();
                        if(currentOrientation == qsEvent->GetOrientation())
                        {
                            Point clickPointOnDevice(qsEvent->IntOperand(0) * recordDeviceSize.width / maxWidthHeight, qsEvent->IntOperand(1) * recordDeviceSize.height / maxWidthHeight);
                            clickPointOnDevice = TransformDevicePoint(clickPointOnDevice, qsEvent->GetOrientation(), recordDeviceSize.size(), Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()));

                            Point idOnDevice(clickPointOnDevice.x + idXOffset, clickPointOnDevice.y + idYOffset);
                            state.status = TouchOnDevice(dut, Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()), idOnDevice.x, idOnDevice.y, currentOrientation, "move");
                        }
                        else
                        {
                            state.status = TouchOnDevice(dut, Size(dut->GetDeviceWidth(), dut->GetDeviceHeight()), idRectCenterPoint.x, idRectCenterPoint.y, currentOrientation, "move");
                        }
                    }
                    else
                    {
                        dut->TouchMove(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, qsEvent->GetOrientation());
                    }
                }
                else
                {
                    if(breakPointFramePoints.size() > 0)
                    {
                        breakPointFramePoint = breakPointFramePoints[0];
                        breakPointFramePoints.erase(breakPointFramePoints.begin());
                        state.status = TouchMoveOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                if(scriptReplayMode == ScriptReplayMode::REPLAY_COORDINATE)
                {
                    dut->TouchMove(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, Orientation::Landscape);
                }
                else
                {
                    if(breakPointFramePoints.size() > 0)
                    {
                        breakPointFramePoint = breakPointFramePoints[0];
                        breakPointFramePoints.erase(breakPointFramePoints.begin());
                        state.status = TouchMoveOnFrame(dut, breakPointFrameSize, breakPointFramePoint.x, breakPointFramePoint.y, breakPointFrameOrientation);
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_SWIPE:
            if (dut != nullptr)
            {
                DaVinciStatus status;
                Orientation currentOrientation = dut->GetCurrentOrientation();
                status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, currentOrientation);
                if (DaVinciSuccess(status))
                {
                    ThreadSleep(50);
                    status = dut->TouchUp(qsEvent->IntOperand(2), qsEvent->IntOperand(3), -1, currentOrientation);
                }
                state.status = status;
            }
            break;
        case QSEventAndAction::OPCODE_OCR:
            {
                double beforeCallScript = qsWatch.ElapsedMilliseconds();
                int ocrParam = qsEvent->IntOperand(0);
                Mat ocrImage = GetImageForOCR(frame, ocrParam);
                if (!ocrImage.empty())
                {
                    vector<Rect> rects;
                    vector<string> strings;
                    string text = textRecognize.RecognizeText(ocrImage, ocrParam, true, &rects, &strings);
                    DAVINCI_LOG_INFO << "========== OCR Text Begins ==========";
                    DAVINCI_LOG_INFO << text;
                    DAVINCI_LOG_INFO << "========== OCR Text Ends ==========";    

                    // in case some one has used the above function
                    DAVINCI_LOG_INFO << "========== OCR Text Detail Begins ==========";
                    for (size_t i = 0; i < rects.size(); ++i)
                    {

                        DAVINCI_LOG_INFO <<"("<< rects[i].x << ", " << rects[i].y << "), (" << rects[i].x + rects[i].width \
                            << ", " << rects[i].y+ rects[i].height<<")  " << "\""<<strings[i].substr(0,strings[i].length()-2) <<"\"" ;
                    }
                    DAVINCI_LOG_INFO << text;
                    DAVINCI_LOG_INFO << "========== OCR Text Detail Ends ==========";            
                }
                state.timeStampOffset -= qsWatch.ElapsedMilliseconds() - beforeCallScript;
            }
            break;
        case QSEventAndAction::OPCODE_PREVIEW:
            {
                string referenceImagePath = qs->GetResourceFullPath(qsEvent->StrOperand(1));
                string previewImagePath = qs->GetResourceFullPath(qsEvent->StrOperand(2));
                if (!is_regular_file(referenceImagePath) || !is_regular_file(previewImagePath))
                {
                    DAVINCI_LOG_WARNING << string("Preview image or reference image does not exist.") << endl;
                    nextQsIp = qsEvent->IntOperand(1);
                }
                else
                {
                    Mat referenceImage = imread(referenceImagePath);
                    Mat previewImage = imread(previewImagePath);
                    if (true) // TODO: (!ResultChecker::is_preview_matched(referenceImage, previewImage))
                    {
                        DAVINCI_LOG_INFO << "     PREVIEW UNMATCH: " << qsEvent->StrOperand(0) << ", " << qsEvent->StrOperand(1);
                        nextQsIp = qsEvent->IntOperand(1);
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "     PREVIEW MATCH: " << qsEvent->StrOperand(0) << ", " << qsEvent->StrOperand(1);
                        nextQsIp = qsEvent->IntOperand(0);
                    }
                }
                AdjustBranchElapse(state.currentQsIp, nextQsIp);
            }
            break;
        case QSEventAndAction::OPCODE_TILTUP:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {               
                    hwAcc->TiltUp(qsEvent->IntOperand(0));
                }
                else if (qsEvent->NumIntOperands() == 2)
                {
                    hwAcc->TiltUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTDOWN:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->TiltDown(qsEvent->IntOperand(0));
                }
                else if (qsEvent->NumIntOperands() == 2)
                {
                    hwAcc->TiltDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTLEFT:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->TiltLeft(qsEvent->IntOperand(0));
                }
                else if (qsEvent->NumIntOperands() == 2)
                {
                    hwAcc->TiltLeft(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTRIGHT:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->TiltRight(qsEvent->IntOperand(0));
                }
                else if (qsEvent->NumIntOperands() == 2)
                {
                    hwAcc->TiltRight(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTTO:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 2)
                {
                    hwAcc->TiltTo(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                }
                else if (qsEvent->NumIntOperands() == 4)
                {
                    hwAcc->TiltTo(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), qsEvent->IntOperand(3));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 0)
                {
                    hwAcc->InnerClockwiseContinuousTilt();
                }
                else if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->InnerClockwiseContinuousTilt(qsEvent->IntOperand(0));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 0)
                {
                    hwAcc->OuterClockwiseContinuousTilt();
                }
                else if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->OuterClockwiseContinuousTilt(qsEvent->IntOperand(0));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 0)
                {
                    hwAcc->InnerAnticlockwiseContinuousTilt();
                }
                else if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->InnerAnticlockwiseContinuousTilt(qsEvent->IntOperand(0));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (qsEvent->NumIntOperands() == 0)
                {
                    hwAcc->OuterAnticlockwiseContinuousTilt();
                }
                else if (qsEvent->NumIntOperands() == 1)
                {
                    hwAcc->OuterAnticlockwiseContinuousTilt(qsEvent->IntOperand(0));
                }
                else
                {
                    DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_SENSOR_CHECK:
            {
                sensorCheckFlag = true;

                //Get sensor data from agent
                if ((dut != nullptr) && (dut->IsAgentConnected()))
                {
                    if (qsEvent->NumIntOperands() == 1)
                    {
                        double nowTimeStamp = GetTickCount();
                        string sensorDataFromAgent = "";
                        if (qsEvent->IntOperand(0) == 0)        //Q_SENSOR_CHECKER_STOP
                        {
                            dut->GetSensorData(qsEvent->IntOperand(0), sensorDataFromAgent);
                            sensorCheckData.push_back("TimeStamp: " + boost::lexical_cast<string>(nowTimeStamp) + " # Data from QAgent: " + sensorDataFromAgent);
                        }
                        else if (qsEvent->IntOperand(0) == 1)   //Q_SENSOR_CHECKER_START
                        {
                            dut->GetSensorData(qsEvent->IntOperand(0), sensorDataFromAgent);
                            sensorCheckData.push_back("TimeStamp: " + boost::lexical_cast<string>(nowTimeStamp) + " # Data from QAgent: " + sensorDataFromAgent);
                        }
                        else
                        {
                            dut->GetSensorData(qsEvent->IntOperand(0), sensorDataFromAgent);
                            sensorCheckData.push_back("TimeStamp: " + boost::lexical_cast<string>(nowTimeStamp) + " # Data from QAgent: " + sensorDataFromAgent);
                        }
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                    }
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, failed to get sensor data from android target device!";
                }

                //Get sensor data from sensor module
                if (hwAcc != nullptr)
                {
                    if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_SENSOR_MODULE) == true)
                    {
                        if (qsEvent->NumIntOperands() == 1)
                        {
                            bool result = false;
                            double nowTimeStamp = GetTickCount();
                            string sensorDataFromSensorModule = hwAcc->GetSepcificSensorCheckerTypeDataFromSensorModule(qsEvent->IntOperand(0), result);                            
                            sensorCheckData.push_back("TimeStamp: " + boost::lexical_cast<string>(nowTimeStamp) + " # Data from Sensor Module: " + sensorDataFromSensorModule);
                        }
                        else
                        {
                            DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                        }
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "Sorry, the reference sensor module doesn't exist!";
                    }
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the FFRD doesn't exist!";
                }

                sensorCheckData.push_back("");    //Used for better display!
                break;
            }
        case QSEventAndAction::OPCODE_MATCH_ON:
        case QSEventAndAction::OPCODE_MATCH_OFF:
            break;
        case QSEventAndAction::OPCODE_IF_MATCH_IMAGE:
            {
                auto objImg = QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                int matchResult = HandleIfMatchImage(frame, objImg, qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->LineNumber(),
                    nextQsIp, OperandToImageMatchAlgorithm(qsEvent->IntOperand(2)));

                if (matchResult == 1)
                {
                    SaveUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                }
                else
                {
                    DeleteUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                }
                AdjustBranchElapse(state.currentQsIp, nextQsIp);
                break;
            }
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE:
            if (state.imageMatchTable.find(qsEvent->StrOperand(0)) != state.imageMatchTable.end())
            {
                auto matchedObject = state.imageMatchTable[qsEvent->StrOperand(0)];
                Size offset = NormToFrameOffset(Size(frame.cols, frame.rows), Size(qsEvent->IntOperand(0), qsEvent->IntOperand(1)), matchedObject->orientation);
                state.status = ClickOnFrame(dut, Size(frame.cols, frame.rows), static_cast<int>(matchedObject->center.x) + offset.width, static_cast<int>(matchedObject->center.y) + offset.height, matchedObject->orientation);
            }
            else
            {
                DAVINCI_LOG_WARNING << "     Image " << qsEvent->StrOperand(0) << " has not been matched yet.";
            }
            break;
        case QSEventAndAction::OPCODE_MESSAGE:
            DAVINCI_LOG_INFO << "     MESSAGE: " << qsEvent->StrOperand(0);
            break;
        case QSEventAndAction::OPCODE_ERROR:
            DAVINCI_LOG_INFO << "     ERROR: " << qsEvent->StrOperand(0);
            resultChecker->Runtime->IncrementErrorCount();
            break;
        case QSEventAndAction::OPCODE_GOTO:
            nextQsIp = qs->LineNumberToIp(qsEvent->IntOperand(0));
            AdjustBranchElapse(state.currentQsIp, nextQsIp);
            break;
        case QSEventAndAction::OPCODE_EXIT:
            {
                if (fpsTest != nullptr)
                    fpsTest->PreDestroy();
                state.exited = true;
            }
            break;
        case QSEventAndAction::OPCODE_INSTALL_APP:
            // TODO: use operand first
            if (dut != nullptr)
            {
                DAVINCI_LOG_INFO << "Install app...";
                double beforeInstall = qsWatch.ElapsedMilliseconds();
                int installToSDCard = 0;
                string apkLocation = "";
                string installOps = "";

                if (qsEvent->NumIntOperands() >= 1)
                    installToSDCard = qsEvent->IntOperand(0); // if there is no such parameter, will return 0.

                string installValue = qsEvent->StrOperand(0);
                std::unordered_map<string, string> installValueMap;
                string locationKey = "location", optionKey = "options";

                // for back compatible.
                if (!installValue.empty())
                {
                    if ((installValue.find(locationKey) != string::npos) || (installValue.find(optionKey) != string::npos))
                    {
                        installValueMap = ParseKeyValues(installValue);

                        if(installValueMap.find(locationKey) != installValueMap.end())
                            apkLocation = installValueMap[locationKey];

                        if(installValueMap.find(optionKey) != installValueMap.end())
                            installOps = installValueMap[optionKey];
                    }
                    else
                        apkLocation = installValue;
                }

                if (apkLocation == "")
                    apkLocation = qs->GetResourceFullPath(qs->GetApkName());
                else
                    apkLocation = qs->GetResourceFullPath(apkLocation);

                string log;
                DaVinciStatus status;
                bool toExtSD = false;
                bool x64 = false;

                if (installToSDCard == 1)
                {
                    toExtSD = true;
                }

                if (AppUtil::Instance().InSpecialInstallNames(qs->GetPackageName()))
                {
                    x64 = true;
                }

                status = dut->InstallAppWithLog(apkLocation, log, installOps, toExtSD, x64);
                assert(status == DaVinciStatusSuccess);

                // Generate install log by default
                std::ofstream logStream(smokeOutputDir + "\\apk_install_" + qs->GetPackageName() + ".txt", std::ios_base::app | std::ios_base::out);
                logStream << log << std::flush;
                logStream.flush();
                logStream.close();

                if (log.find("Success") == string::npos)
                {
                    resultChecker->Runtime->IncrementErrorCount();

                    // GET will handle this exception by itself, so skip the following step.
                    // state.status = DaVinciStatus(errc::bad_message);
                }

                // since OPCODE_INSTALL_APP is usually added after script recording and
                // it takes long time to complete, we substract the installation time from
                // the main timeline so that the timing of following events is not impacted.
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeInstall);
            }
            break;
        case QSEventAndAction::OPCODE_UNINSTALL_APP:
            // TODO: use operand first
            if (dut != nullptr)
            {
                if (!qs->GetPackageName().empty() || (qsEvent->StrOperand(0) != ""))
                {
                    // if the apk is installed, then uninstall, otherwise didn't uninstall
                    vector<string> installedPackage = dut->GetAllInstalledPackageNames();
                    if (std::find(installedPackage.begin(), installedPackage.end(), qsEvent->StrOperand(0)) != installedPackage.end() ||
                        std::find(installedPackage.begin(), installedPackage.end(), qs->GetPackageName()) != installedPackage.end())
                    {
                        DAVINCI_LOG_INFO << "Uninstall app...";
                        double beforeUninstall = qsWatch.ElapsedMilliseconds();
                        // pull apk in smart runner
                        //if (IsSmokeQS(qsMode)) // Pull apk for exploratory test
                        //{
                        //    string package = generateSmokeQsript->GetPackageName();
                        //    string version = dut->GetPackageVersion(package);
                        //    string dst = TestReport::currentQsLogPath;
                        //    if(boost::dynamic_pointer_cast<AndroidTargetDevice>(dut))
                        //    {
                        //        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                        //        if(OnDeviceScriptRecorder::PullApk(androidDut, package, version, dst) == DaVinciStatusSuccess)
                        //        {
                        //            generateSmokeQsript->SetConfiguration("APKName", package + "_" + version + ".apk");
                        //        }

                        //    }
                        //    else
                        //    {
                        //        DAVINCI_LOG_WARNING << "Unable to pull apk  " << package << " from device.";
                        //    }
                        //}

                        // Generate uninstall log by default
                        string log;
                        DaVinciStatus status;
                        if (qsEvent->StrOperand(0) == "")
                        {
                            status = dut->UninstallAppWithLog(qs->GetPackageName(), log);
                        }
                        else
                        {
                            status = dut->UninstallAppWithLog(qsEvent->StrOperand(0), log);
                        }
                        assert(status == DaVinciStatusSuccess);

                        std::ofstream logStream(smokeOutputDir + "\\apk_uninstall_" + qs->GetPackageName() + ".txt", std::ios_base::app | std::ios_base::out);
                        logStream << log << std::flush;
                        logStream.flush();
                        logStream.close();

                        // same thing as OPCODE_INSTALL_APP, ignore the time used by app uninstallation.
                        state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeUninstall);
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_PUSH_DATA:
            // TODO: use operand first
            if (boost::dynamic_pointer_cast<AndroidTargetDevice>(dut) != nullptr)
            {
                auto androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                DAVINCI_LOG_INFO << "Push app data...";
                string pushDataSource = qsEvent->StrOperand(0);
                string pushDataTarget = qsEvent->StrOperand(1);
                if (!pushDataSource.empty() && !pushDataTarget.empty())
                {
                    double beforePushData = qsWatch.ElapsedMilliseconds();
                    std::string pathOfPushDataSource = qs->GetResourceFullPath(pushDataSource);
                    state.status = androidDut->AdbPushCommand(pathOfPushDataSource, pushDataTarget);
                    // same thing as OPCODE_INSTALL_APP, ignore the time used by app uninstallation.
                    state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforePushData);
                }
                else
                {
                    string pushData = qs->GetConfiguration(QScript::ConfigPushData);
                    if(!pushData.empty())
                    {
                        vector<string> str;
                        QScript::SplitEventsAndActionsLine(pushData, str);

                        if (str.size() == 2)
                        {
                            std::string pathOfPushDataSource = qs->GetResourceFullPath(str[0]);
                            state.status = androidDut->AdbPushCommand(pathOfPushDataSource, str[1]);
                        }
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING << "Wrong push data location: expecting <source> and <target>.";
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_CALL_SCRIPT:
            {
                double beforeCallScript = qsWatch.ElapsedMilliseconds();
                string calleeFullPath = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                auto callee = boost::shared_ptr<ScriptReplayer>(new ScriptReplayer(calleeFullPath));
                CallTest(callee);
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeCallScript);
            }
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), qsEvent->GetOrientation());
            }
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), Orientation::Landscape);
            }
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHUP:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), qsEvent->GetOrientation());
            }
            break;
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHUP_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), Orientation::Landscape);
            }
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHMOVE:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchMove(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), qsEvent->GetOrientation());
            }
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHMOVE_HORIZONTAL:
            // To avoid refresh orientation, we call touch directly
            if (dut != nullptr)
            {
                state.status = dut->TouchMove(qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->IntOperand(2), Orientation::Landscape);
            }
            break;
        case QSEventAndAction::OPCODE_NEXT_SCRIPT:
            {
                double beforeNextScript = qsWatch.ElapsedMilliseconds();
                string calleeFullPath = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                auto callee = boost::shared_ptr<ScriptReplayer>(new ScriptReplayer(calleeFullPath));
                CallTest(callee);
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeNextScript);
            }
            break;
        case QSEventAndAction::OPCODE_CALL_EXTERNAL_SCRIPT:
            if (!qsEvent->StrOperand(0).empty())
            {
                using namespace boost::process;

                double beforeCallExternalScript = qsWatch.ElapsedMilliseconds();

                if (!qsEvent->StrOperand(0).empty())
                {
                    string logFile = TestManager::Instance().GetLogFile();
                    path scriptOutPath(testReport->currentQsLogPath);
                    path leaf(GetCurrentTimeString() + ".return");
                    scriptOutPath /= leaf;
                    string scriptOutFile = scriptOutPath.string();
                    if (boost::filesystem::exists(scriptOutFile))
                        boost::filesystem::remove(scriptOutFile);
                    string qsWorkDir = qs->GetResourceFullPath(".");
                    environment env = self::get_instance().get_environment();

                    env["DaVinci_Case_Log_File"] = logFile;
                    env["DaVinci_Callee_Script_Output_File"] = scriptOutFile;
                    if (fpsTest != nullptr)
                    {
                        string file = fpsTest->GetDumpsysOutputFile();
                        if (!file.empty())
                            env["DaVinci_FPS_Measure_Dump_File"] = file;
                    }

                    if (DaVinciSuccess(RunShellCommand(qsEvent->StrOperand(0), nullptr, qsWorkDir, 0, env)))
                    {
                        ifstream retStrm(scriptOutFile, ios::binary);
                        string label;
                        getline(retStrm, label);
                        if (!label.empty())
                        {
                            auto qs = GetQs();
                            int line;
                            if (TryParse(label, line))
                            {
                                auto nextIp = qs->LineNumberToIp(line);
                                if (nextIp > 0)
                                {
                                    nextQsIp = nextIp;
                                }
                            }
                        }
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING << "Failed to execute external script/command: " << qsEvent->StrOperand(0);
                    }
                }
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeCallExternalScript);
            }
            break;
        case QSEventAndAction::OPCODE_ELAPSED_TIME:
            DAVINCI_LOG_INFO << "Elapsed Time: " << qsWatch.ElapsedMilliseconds() << "ms";
            break;
            /* TODO: handle FPS measure
            case QSEventAndAction::OPCODE_FPS_MEASURE_START:
            if (qsEvent->operand_string == "")
            {
            if (fpsTest != nullptr)
            {
            fpsTest->Start();
            }
            }
            break;
            case QSEventAndAction::OPCODE_FPS_MEASURE_STOP:
            if (qsEvent->operand_string == "")
            {
            if (fpsTest != nullptr)
            {
            TestManager::StopTest(fpsTest);
            fpsTest.reset();
            }
            }
            break;
            */
        case QSEventAndAction::OPCODE_CONCURRENT_IF_MATCH:
            {
                if(!TestManager::Instance().IsOfflineCheck()){

                    assert(state.ccEventTable.find(state.currentQsIp) != state.ccEventTable.end());
                    auto ccEvent = state.ccEventTable[state.currentQsIp];
                    ccEvent->isActivated = true;
                    AdjustBranchElapse(nextQsIp, nextQsIp + ccEvent->instrCount);
                    nextQsIp += ccEvent->instrCount;
                } 
                else
                {
                    unsigned int instrCount;
                    if (qsEvent->IntOperand(0) <= 0)
                    {
                        instrCount = 0;
                    }
                    else
                    {
                        instrCount = static_cast<unsigned int>(qsEvent->IntOperand(0));
                    }
                    AdjustBranchElapse(nextQsIp, nextQsIp + instrCount);
                    nextQsIp += instrCount;                    
                }
            }
            break;
        case QSEventAndAction::OPCODE_BATCH_PROCESSING:
            {
                double beforeBatchProcessing = qsWatch.ElapsedMilliseconds();
                string iniFullPath = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                path iniDir = path(iniFullPath).parent_path();

                DAVINCI_LOG_DEBUG << "ini file name: " << iniFullPath;

                vector<string> lines;
                DaVinciStatus status = ReadAllLines(iniFullPath, lines);
                if (DaVinciSuccess(status))
                {
                    for (auto line : lines)
                    {
                        if (boost::algorithm::iends_with(line, ".qs"))
                        {
                            path qsPath(line);
                            if (!qsPath.is_complete())
                            {
                                qsPath = iniDir;
                                qsPath /= line;
                            }
                            auto callee = boost::shared_ptr<ScriptReplayer>(new ScriptReplayer(qsPath.string()));
                            callee->SetName("ScriptReplayer - " + line);
                            CallTest(callee);
                            // just in case someone asked me to exit
                            if (IsFinished())
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Error loading the batch ini file.";
                }
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeBatchProcessing);
            }
            break;
        case QSEventAndAction::OPCODE_IF_MATCH_TXT:
        case QSEventAndAction::OPCODE_IF_MATCH_REGEX:
            {
                double tmsBefore = qsWatch.ElapsedMilliseconds();
                int yesIp = qs->LineNumberToIp(qsEvent->IntOperand(1));
                int noIp = qs->LineNumberToIp(qsEvent->IntOperand(2));
                int ocrParam = qsEvent->IntOperand(0);
                Mat ocrImage = GetImageForOCR(frame, ocrParam);
                if (!ocrImage.empty())
                {
                    vector<Rect> textRects;
                    TextRecognize::TextRecognizeResult result = textRecognize.DetectDetail(ocrImage, ocrParam, PageIteratorLevel::RIL_SYMBOL,true);
                    DAVINCI_LOG_DEBUG << "OPCODE_IF_MATCH_TXT/REGEX" << result.GetRecognizedText();
                    if (qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_MATCH_TXT)
                    {
                        string preprocessPattern = qsEvent->StrOperand(0);
                        string metaCharArray[] = {"\\","{","}","[","]","(",")","^","$","*","+","?",".","|"};
                        for(int i = 0 ;i < sizeof(metaCharArray)/sizeof(metaCharArray[0]); i++){
                            if(boost::contains(preprocessPattern, metaCharArray[i]))
                            {
                                boost::replace_all(preprocessPattern, metaCharArray[i], "\\"+metaCharArray[i]);
                            }
                        }
                        textRects = result.GetBoundingRects(preprocessPattern);
                    }
                    else
                    {
                        textRects = result.GetBoundingRects(qsEvent->StrOperand(0));
                    }                    
                    if (textRects.size() > 0)
                    {
                        // project the axis values back to the camera image frame which is always
                        // in horizontal layout.
                        double heightRatio, widthRatio;
                        bool swapHeightWidth = ocrImage.rows > ocrImage.cols;
                        int h = swapHeightWidth ? ocrImage.cols : ocrImage.rows;
                        int w = swapHeightWidth ? ocrImage.rows : ocrImage.cols;
                        heightRatio = static_cast<double>(frame.rows) / h;
                        widthRatio = static_cast<double>(frame.cols) / w;
                        for (int i = 0; i < static_cast<int>(textRects.size()); i++)
                        {
                            Rect rect = textRects[i];
                            if (swapHeightWidth)
                            {
                                rect.x = textRects[i].y;
                                rect.y = h - (textRects[i].x + textRects[i].width);
                                rect.width = textRects[i].height;
                                rect.height = textRects[i].width;
                            }
                            rect.x = static_cast<int>(rect.x * widthRatio);
                            rect.y = static_cast<int>(rect.y * heightRatio);
                            rect.width = static_cast<int>(rect.width * widthRatio);
                            rect.height = static_cast<int>(rect.height * heightRatio);
                            textRects[i] = rect;
                        }
                        auto matchedTextObject = boost::shared_ptr<MatchedTextObject>(
                            new MatchedTextObject(
                            textRects,
                            dut != nullptr ? dut->GetCurrentOrientation() : Orientation::Unknown)
                            );
                        if (qsEvent->Opcode() == QSEventAndAction::OPCODE_IF_MATCH_TXT)
                        {
                            state.textMatchTable[qsEvent->StrOperand(0)] = matchedTextObject;
                        }
                        else
                        {
                            state.regexMatchTable[qsEvent->StrOperand(0)] = matchedTextObject;
                        }
                        for (auto rect : textRects)
                        {
                            vector<Point2f> points;
                            points.push_back(Point(rect.x, rect.y));
                            points.push_back(Point(rect.x + rect.width, rect.y));
                            points.push_back(Point(rect.x + rect.width, rect.y + rect.height));
                            points.push_back(Point(rect.x, rect.y + rect.height));
                            DrawDebugPolyline(frame, points, dut != nullptr ? dut->GetCurrentOrientation() : Orientation::Unknown);
                        }
                        nextQsIp = yesIp;
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "     Cannot match text or regex '" << qsEvent->StrOperand(0) << "' @line " << qsEvent->LineNumber();
                        nextQsIp = noIp;
                    }
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Unable to get image frame for OCR";
                    nextQsIp = noIp;
                }
                AdjustBranchElapse(state.currentQsIp, nextQsIp);
                state.timeStampOffset -= qsWatch.ElapsedMilliseconds() - tmsBefore;
            }
            break;
        case QSEventAndAction::OPCODE_CLICK_MATCHED_TXT:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_REGEX:
            if (dut != nullptr)
            {
                boost::shared_ptr<MatchedTextObject> matchedTextObject;
                if (qsEvent->Opcode() == QSEventAndAction::OPCODE_CLICK_MATCHED_TXT && state.textMatchTable.find(qsEvent->StrOperand(0)) != state.textMatchTable.end())
                {
                    matchedTextObject = state.textMatchTable[qsEvent->StrOperand(0)];
                }
                else if (qsEvent->Opcode() == QSEventAndAction::OPCODE_CLICK_MATCHED_REGEX && state.regexMatchTable.find(qsEvent->StrOperand(0)) != state.regexMatchTable.end())
                {
                    matchedTextObject = state.regexMatchTable[qsEvent->StrOperand(0)];
                }
                if (matchedTextObject != nullptr)
                {
                    if (0 <= qsEvent->IntOperand(0) && qsEvent->IntOperand(0) < static_cast<int>(matchedTextObject->locations.size()))
                    {
                        Rect rect = matchedTextObject->locations[qsEvent->IntOperand(0)];
                        int centerX = rect.x + rect.width / 2;
                        int centerY = rect.y + rect.height / 2;
                        Size normOffset = Size(qsEvent->IntOperand(1), qsEvent->IntOperand(2));
                        Size frameOffset = NormToFrameOffset(Size(frame.cols, frame.rows), normOffset, matchedTextObject->orientation);
                        state.status = ClickOnFrame(dut, Size(frame.cols, frame.rows), centerX + frameOffset.width, centerY + frameOffset.height, matchedTextObject->orientation);
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING << "     " << qsEvent->IntOperand(0) << "th Text " << qsEvent->StrOperand(0) << " was not found.";
                    }
                }
                else
                {
                    DAVINCI_LOG_WARNING << "     Text " << qsEvent->StrOperand(0) << " has not been matched yet.";
                }
            }
            break;
        case QSEventAndAction::OPCODE_AUDIO_RECORD_START:
            {
                if (AudioManager::Instance().RecordFromDeviceDisabled())
                {
                    DAVINCI_LOG_WARNING << "     Warning: audio recording disabled!";
                    break;
                }
                string waveFileName = qsEvent->StrOperand(0);
                boost::lock_guard<boost::mutex> lock(state.waveFileTableLock);
                {
                    if (state.waveFileTable.find(waveFileName) != state.waveFileTable.end())
                    {
                        DAVINCI_LOG_WARNING << "     Warning: wave file " << waveFileName << " is already being recorded.";
                    }
                    else
                    {
                        state.waveFileTable[waveFileName] = AudioFile::CreateWriteWavFile(qs->GetResourceFullPath(waveFileName));
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_AUDIO_RECORD_STOP:
            {
                if (AudioManager::Instance().RecordFromDeviceDisabled())
                {
                    DAVINCI_LOG_WARNING << "     Warning: audio recording disabled!";
                    break;
                }
                string waveFileName = qsEvent->StrOperand(0);
                boost::lock_guard<boost::mutex> lock(state.waveFileTableLock);
                {
                    if (state.waveFileTable.find(waveFileName) != state.waveFileTable.end())
                    {
                        state.waveFileTable.erase(waveFileName);
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING << "     Warning: wave file " << waveFileName << " is not recorded yet.";
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_IF_MATCH_AUDIO:
            {
                int yesIp = qs->LineNumberToIp(qsEvent->IntOperand(0));
                int noIp = qs->LineNumberToIp(qsEvent->IntOperand(1));
                if (AudioManager::Instance().RecordFromDeviceDisabled())
                {
                    DAVINCI_LOG_WARNING << "     Warning: audio recording disabled!";
                    break;
                }
                try
                {
                    string waveFilePath0 = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                    string waveFilePath1 = qs->GetResourceFullPath(qsEvent->StrOperand(1));
                    if (exists(waveFilePath0) && exists(waveFilePath1))
                    {
                        auto audioRecognize = boost::shared_ptr<AudioRecognize>(new AudioRecognize(waveFilePath0));
                        if (audioRecognize->Detect(waveFilePath1))
                        {
                            nextQsIp = yesIp;
                        }
                        else
                        {
                            nextQsIp = noIp;
                        }
                    }
                    else
                    {
                        nextQsIp = noIp;
                    }
                }
                catch (...)
                {
                    nextQsIp = noIp;
                }
                AdjustBranchElapse(state.currentQsIp, nextQsIp);
            }
            break;
        case QSEventAndAction::OPCODE_AUDIO_PLAY:
            AudioManager::Instance().PlayToDevice(qs->GetResourceFullPath(qsEvent->StrOperand(0)));
            break;
            /* TODO: handle AI
            #if !defined(INDE)
            case QSEventAndAction::OPCODE_AI_BUILTIN:
            boost::shared_ptr<XmlDocument> doc = make_shared<XmlDocument>();
            boost::shared_ptr<XmlNodeList> strategy_nodes = nullptr;
            string xmlpath = GetResourceFullPath(qsEvent->operand_string);
            if (File::Exists(xmlpath) == false)
            {
            cout << string("Warning: xml doesn't exist: ") << xmlpath << endl;
            break;
            }
            try
            {
            doc->Load(xmlpath);
            strategy_nodes = doc->SelectNodes("//strategy");
            }
            catch (XmlException e)
            {
            TestManager::UpdateText(string("Error in loading XML: ") + e->Message);
            break;
            }

            if (strategy_nodes->Count != 1)
            {
            TestManager::UpdateText(string("Error in XML strategy node: count = ") + strategy_nodes->Count);
            }
            else
            {
            string strategy = strategy_nodes[0]->Attributes["name"]->Value;
            if (strategy == "LinkLink")
            {
            boost::shared_ptr<AILinkLink> llk = make_shared<AILinkLink>(xmlpath);
            llk->SetName("LianLianKan case");
            double elapsedBeforeCalling = TestManager::GetQSElapsedMilliseconds();
            TestManager::CallTest(shared_from_this(), llk, true);
            timestampOffset -= (TestManager::GetQSElapsedMilliseconds() - elapsedBeforeCalling);
            }
            else if (strategy == "Parkour")
            {
            boost::shared_ptr<XmlNodeList> para_nodes = doc->SelectNodes("//para");
            string aiName = para_nodes[0]->Attributes["name"]->Value;
            if (aiName != "AI_algo")
            {
            cout << string("error in xml file: ") << aiName << endl;
            break;
            }
            aiName = para_nodes[0]->Attributes["value"]->Value;
            if (aiName == "ANN")
            {
            boost::shared_ptr<Thread> annPlayThread = make_shared<Thread>([&] ()
            {
            boost::shared_ptr<ParkourGameAnnPlay> parkour_ann = make_shared<ParkourGameAnnPlay>(xmlpath);
            TestManager::AddTest(parkour_ann);
            });
            annPlayThread->Name = "ANN play Thread";
            annPlayThread->Start();
            }
            else if (aiName == "SVM")
            {
            boost::shared_ptr<Thread> svmPlayThread = make_shared<Thread>([&] ()
            {
            boost::shared_ptr<ParkourGameSVMPlay> parkour_svm = make_shared<ParkourGameSVMPlay>(xmlpath);
            TestManager::AddTest(parkour_svm);
            });
            svmPlayThread->Name = "SVM play Thread";
            svmPlayThread->Start();
            }
            else
            {
            TestManager::UpdateText(string("Error in XML parameter: ") + aiName);
            }
            }
            else if (strategy == "FlappyBird")
            {
            boost::shared_ptr<Thread> flappyThread = make_shared<Thread>([&] ()
            {
            boost::shared_ptr<FlappyBirds> flappyBirds = make_shared<FlappyBirds>(xmlpath);
            flappyBirds->SetName("FlappyBirds case");
            TestManager::AddTest(flappyBirds);
            });
            flappyThread->Name = "Start FlappyBird Thread";
            flappyThread->Priority = ThreadPriority::Highest;
            flappyThread->Start();
            }
            else
            {
            TestManager::UpdateText(string("Error: unsupported XML strategy: ") + strategy);
            }
            }
            break;
            #endif
            */
        case QSEventAndAction::OPCODE_IMAGE_MATCHED_TIME:
            {
                string imagePath = qsEvent->StrOperand(0);
                if (state.imageMatchTable.find(imagePath) != state.imageMatchTable.end())
                {
                    auto matchedObject = state.imageMatchTable[imagePath];
                    DAVINCI_LOG_INFO << "Image " << imagePath << " was matched at " << matchedObject->time << "ms";
                    testReportSummary->appendTxtReportSummary("Image " + imagePath + " was matched at " + boost::lexical_cast<string>( matchedObject->time) + " ms");

                }
                else
                {
                    DAVINCI_LOG_INFO << "Image " << imagePath << " has not been matched yet.";
                    testReportSummary->appendTxtReportSummary("Image " + imagePath + " has not been matched yet.");
                }
            }
            break;
        case QSEventAndAction::OPCODE_FLICK_ON:
        case QSEventAndAction::OPCODE_FLICK_OFF:
            break;
        case QSEventAndAction::OPCODE_IMAGE_CHECK:
            {
                auto objImg = QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                int nextIp;
                if (state.imageTimeOutWaiting)
                {
                    int matchResult = HandleImageCheck(
                        frame, objImg, qsEvent->IntOperand(0), qsEvent->LineNumber(), nextIp);                   
                    assert(matchResult != 2);

                    if (matchResult == 0 || state.imageWaitWatch.ElapsedMilliseconds() > qsEvent->IntOperand(1))
                    {
                        if (state.imageWaitWatch.ElapsedMilliseconds() > qsEvent->IntOperand(1))
                        {
                            SaveUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                        }
                        else
                        {
                            DeleteUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                        }
                        if (nextIp != 0)
                            nextQsIp = nextIp;
                        AdjustBranchElapse(state.currentQsIp, nextQsIp);
                        state.timeStampOffset -= state.imageWaitWatch.ElapsedMilliseconds();
                        state.imageTimeOutWaiting = false;
                    }
                    else
                    {
                        nextQsIp = state.currentQsIp;
                    }
                }
                else
                {
                    state.imageWaitWatch.Reset();
                    state.imageWaitWatch.Start();
                    state.imageTimeOutWaiting = true;
                    nextQsIp = state.currentQsIp;
                }
            }
            break;
        case QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT:
            {
                int nextIp;
                int imgMatcherEndIndex = (int)currentFrameIndex;
                auto objImg = QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                if (imgMatcherStartIndex == 0)
                    imgMatcherStartIndex = currentFrameIndex;

                if (state.imageTimeOutWaiting)
                {
                    int matchResult = HandleIfMatchImage(
                        frame, objImg, qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->LineNumber(), nextIp,
                        OperandToImageMatchAlgorithm(qsEvent->IntOperand(3)));

                    checkedImgIndices.insert(imgMatcherEndIndex);

                    assert(matchResult != 2);
                    if (matchResult == 0 || state.imageWaitWatch.ElapsedMilliseconds() > qsEvent->IntOperand(2))
                    {
                        if (matchResult == 0)
                        {
                            DeleteUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                        }
                        else
                        {
                            // check avi video for missed frames
                            bool offlineMatched = false;
                            VideoCaptureProxy cvAvi(videoFileName);
                            if (cvAvi.isOpened())
                            {
                                for (int i = (int)imgMatcherStartIndex; i < (int)imgMatcherEndIndex; i ++)
                                {
                                    if (checkedImgIndices.find(i) == checkedImgIndices.end())
                                    {
                                        cvAvi.set(CV_CAP_PROP_POS_FRAMES, i);
                                        Mat img;
                                        if (cvAvi.read(img))
                                        {
                                            int matchResult = HandleIfMatchImage(
                                                img, objImg, qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->LineNumber(), nextIp,
                                                OperandToImageMatchAlgorithm(qsEvent->IntOperand(3)));
                                            if (matchResult == 0)
                                            {
                                                DeleteUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                                                offlineMatched = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (!offlineMatched)
                            {
                                SaveUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                            }
                        }

                        imgMatcherStartIndex = 0;
                        checkedImgIndices.clear();
                        nextQsIp = nextIp;
                        AdjustBranchElapse(state.currentQsIp, nextQsIp);
                        state.timeStampOffset -= state.imageWaitWatch.ElapsedMilliseconds();
                        state.imageTimeOutWaiting = false;
                    }
                    else
                    {
                        nextQsIp = state.currentQsIp;
                    }
                }
                else
                {
                    int matchResult = HandleIfMatchImage(
                        frame, objImg, qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->LineNumber(), nextIp,
                        OperandToImageMatchAlgorithm(qsEvent->IntOperand(3)));

                    checkedImgIndices.insert(imgMatcherEndIndex);

                    if (matchResult == 0 || matchResult == 2)
                    {
                        nextQsIp = nextIp;
                        AdjustBranchElapse(state.currentQsIp, nextQsIp);
                        imgMatcherStartIndex = 0;
                        checkedImgIndices.clear();
                    }
                    else
                    {
                        state.imageWaitWatch.Reset();
                        state.imageWaitWatch.Start();
                        state.imageTimeOutWaiting = true;
                        nextQsIp = state.currentQsIp;
                    }
                }
                break;
            }
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
            if (dut != nullptr)
            {
                if (state.imageMatchTable.find(qsEvent->StrOperand(0)) != state.imageMatchTable.end())
                {
                    auto matchedObject = state.imageMatchTable[qsEvent->StrOperand(0)];
                    Size frameSize(frame.cols, frame.rows);
                    Size offset = NormToFrameOffset(
                        frameSize, Size(qsEvent->IntOperand(0), qsEvent->IntOperand(1)), matchedObject->orientation);
                    int frameX = static_cast<int>(matchedObject->center.x) + offset.width;
                    int frameY = static_cast<int>(matchedObject->center.y) + offset.height;
                    switch (qsEvent->Opcode())
                    {
                    case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
                        state.status = ClickOnFrame(dut, frameSize, frameX, frameY, matchedObject->orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
                        state.status = TouchDownOnFrame(dut, frameSize, frameX, frameY, matchedObject->orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY:
                        state.status = TouchUpOnFrame(dut, frameSize, frameX, frameY, matchedObject->orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
                        state.status = TouchMoveOnFrame(dut, frameSize, frameX, frameY, matchedObject->orientation);
                        break;
                    default:
                        assert(false);
                        break;
                    }

                }
                else
                {
                    Orientation orientation = qsEvent->GetOrientation();
                    if (orientation == Orientation::Unknown)
                    {
                        if (state.clickInHorizontal)
                        {
                            orientation = Orientation::Landscape;
                        }
                        else
                        {
                            orientation = Orientation::Portrait;
                        }
                    }
                    switch (qsEvent->Opcode())
                    {
                    case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
                        // To avoid refresh orientation, we call touch directly
                        state.status = dut->Click(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
                        // To avoid refresh orientation, we call touch directly
                        state.status = dut->TouchDown(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY:
                        // To avoid refresh orientation, we call touch directly
                        state.status = dut->TouchUp(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, orientation);
                        break;
                    case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
                        // To avoid refresh orientation, we call touch directly
                        state.status = dut->TouchMove(qsEvent->IntOperand(0), qsEvent->IntOperand(1), -1, orientation);
                        break;
                    default:
                        assert(false);
                        break;
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_CLEAR_DATA:
            if (boost::dynamic_pointer_cast<AndroidTargetDevice>(dut) != nullptr)
            {
                auto androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                DAVINCI_LOG_INFO << "Cleaning app data ...";
                if (qsEvent->StrOperand(0) == "")
                {
                    if (!qs->GetPackageName().empty())
                    {
                        state.status = androidDut->AdbShellCommand(string("pm clear ") + qs->GetPackageName());
                    }
                }
                else
                {
                    state.status = androidDut->AdbShellCommand(string("pm clear ") + qsEvent->StrOperand(0));
                }
            }
            break;

        case QSEventAndAction::OPCODE_KEYBOARD_EVENT:
            {
                if (dut != nullptr)
                {
                    if (qsEvent->IntOperand(1) == 1)
                        dut->Keyboard(true, qsEvent->IntOperand(2));
                    else
                        dut->Keyboard(false, qsEvent->IntOperand(2));
                }
            }
            break;

            /* TODO: handle windows target events
            #if !defined(INDE)
            case QSEventAndAction::OPCODE_MOUSE_EVENT:

            if (TestManager::IsCurWindowsDevice())
            {
            TestManager::getCurrentWindowsTargetDevice()->mouseEvent(qsEvent->operand1, qsEvent->operand2, qsEvent->operand3, qsEvent->operand4);
            }
            else if (TestManager::IsCurChromeDevice())
            {
            TestManager::getCurrentChromeTargetDevice()->mouseEvent(qsEvent->operand1, qsEvent->operand2, qsEvent->operand3, qsEvent->operand4);
            }
            break;

            #endif
            */
        case QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH:
            {
                string targetImage = qsEvent->StrOperand(1);
                string imageFullPath = qs->GetResourceFullPath(targetImage);
                Mat targetFrame = imread(imageFullPath);
                if (!targetFrame.empty())
                {
                    auto objImg =  QScript::ImageObject::Parse(qsEvent->StrOperand(0));
                    int matchResult = HandleIfMatchImage(targetFrame, objImg, qsEvent->IntOperand(0), qsEvent->IntOperand(1),
                        qsEvent->LineNumber(), nextQsIp, OperandToImageMatchAlgorithm(qsEvent->IntOperand(2)));
                    assert(matchResult != 2);
                    if (matchResult == 1)
                    {
                        SaveUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());    
                    }
                    else
                    {
                        DeleteUnmatchedFrame(frame, objImg->ImageName(), qsEvent->LineNumber());
                    }
                    AdjustBranchElapse(state.currentQsIp, nextQsIp);
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Target image " << targetImage << " not found @" << qsEvent->LineNumber();
                }
            }
            break;
        case QSEventAndAction::OPCODE_IF_SOURCE_LAYOUT_MATCH:
            {
                vector<string> layouts;
                boost::algorithm::split(layouts, qsEvent->StrOperand(0), boost::algorithm::is_any_of("$"));
                HandleIfMatchLayout(qsEvent->StrOperand(0), qsEvent->StrOperand(1), qsEvent->IntOperand(0), qsEvent->IntOperand(1), qsEvent->LineNumber(), nextQsIp);
                AdjustBranchElapse(state.currentQsIp, nextQsIp);
            }
            break;
        case QSEventAndAction::OPCODE_CRASH_ON:
            {
                double beforeStart = qsWatch.ElapsedMilliseconds();
                StartOnlineChecking();
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeStart);
            }
            break;
        case QSEventAndAction::OPCODE_CRASH_OFF:
            {
                double beforeStart = qsWatch.ElapsedMilliseconds();
                StopOnlineChecking();
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeStart);
            }
            break;
        case QSEventAndAction::OPCODE_SET_VARIABLE:
            {
                string varName = qsEvent->StrOperand(0);
                state.variableTable[varName] = qsEvent->IntOperand(0);
                DAVINCI_LOG_INFO << "Set " << varName << "=" << qsEvent->IntOperand(0);
            }
            break;
        case QSEventAndAction::OPCODE_LOOP:
            {
                string varName = qsEvent->StrOperand(0);
                if (state.variableTable.find(varName) != state.variableTable.end())
                {
                    int loopCount = state.variableTable[varName];
                    if (--loopCount > 0)
                    {
                        DAVINCI_LOG_INFO << "Loop " << varName << "=" << loopCount;
                        state.variableTable[varName] = loopCount;
                        nextQsIp = qs->LineNumberToIp(qsEvent->IntOperand(0));
                        AdjustBranchElapse(state.currentQsIp, nextQsIp);
                    }
                }
            }
            break;
        case QSEventAndAction::OPCODE_STOP_APP:
            if (dut != nullptr)
            {
                if (IsSmokeQS(qsMode))
                {
                    ClickOffAllowButton();
                }

                DAVINCI_LOG_INFO << "Stop app...";
                if (qsEvent->StrOperand(0) != "")
                {
                    dut->StopActivity(qsEvent->StrOperand(0));
                }
                else
                {
                    dut->StopActivity(qs->GetPackageName());
                }
            }
            break;
        case QSEventAndAction::OPCODE_SET_TEXT:
            if (dut != nullptr)
            {
                double beforeText = qsWatch.ElapsedMilliseconds();
                state.status = dut->TypeString(qsEvent->StrOperand(0));
                state.timeStampOffset -= qsWatch.ElapsedMilliseconds() - beforeText;
            }
            break;
        case QSEventAndAction::OPCODE_EXPLORATORY_TEST:
            {
                double beforeExploratory = qsWatch.ElapsedMilliseconds();

                Mat rotatedFrame = RotateFrameUp(frame);

                if (IsSmokeQS(qsMode) && (smokeTest->getSkipStatus() || AppUtil::Instance().IsCloseWifiApp(qs->GetPackageName()) || AppUtil::Instance().IsModifyFontApp(qs->GetPackageName())))
                {
                    smokeTest->SetWholePage(rotatedFrame);
                    break;
                }

                if (rotatedFrame.empty())
                {
                    break;
                }

                if(AppUtil::Instance().IsInputMethodApp(qs->GetPackageName()))
                {
                    if (IsSmokeQS(qsMode))
                    {
                        smokeTest->SetClickExitButtonToHome(false);
                        smokeTest->SetWholePage(rotatedFrame);
                    }
                }
                else
                {
                    string biasXML = qs->GetResourceFullPath(qsEvent->StrOperand(0));
                    if (!is_regular_file(biasXML))
                    {
                        DAVINCI_LOG_WARNING << string("Warning: bias xml does not exist.") << endl;
                        break;
                    }

                    boost::filesystem::path xmlPath(biasXML);
                    boost::filesystem::path xmlDir = xmlPath.parent_path();
                    boost::filesystem::path outDirPath(TestReport::currentQsLogPath);
                    outDirPath /= boost::filesystem::path("GET");
                    string outDir = outDirPath.string();

                    if (!boost::filesystem::exists(outDir))
                    {
                        boost::system::error_code ec;
                        boost::filesystem::create_directory(outDir, ec);
                        DaVinciStatus status(ec);
                        if (!DaVinciSuccess(status))
                        {
                            DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << outDir;
                        }
                    }

                    // To handle basic smoke mode
                    if (biasObject == nullptr || biasObject->GetSilent())
                    {
                        break;
                    }

                    ObjectUtil::Instance().debugPrefixDir = outDir + string("\\") + qs->GetPackageName() + string("_") + boost::lexical_cast<string>(biasObject->GetSeedValue()) + string("_");

                    if (IsExploratoryQS(qsMode))
                    {
                        this->covCollector->PrepareCoverageCollector(qs->GetPackageName(), biasObject->GetSeedValue(), xmlDir.string());
                    }

                    Point clickPoint = EmptyPoint;
                    cv::Mat cloneFrame = rotatedFrame.clone();
                    bool isClickOtherObjToHome = expEngine->runExploratoryTest(rotatedFrame, covCollector, biasObject, biasObject->GetSeedValue() + seedIteration, cloneFrame, clickPoint);
                    vector<shared_ptr<QSEventAndAction>> actions = expEngine->GetGeneratedQEvents();
                    for(shared_ptr<QSEventAndAction> qe : actions)
                    {
                        qe->TimeStamp() += beforeExploratory;
                        qe->TimeStampReplay() += beforeExploratory;
                        generateSmokeQsript->AppendEventAndAction(qe);
                        generateSmokeQsript->Trace().push_back(QScript::TraceInfo(qe->LineNumber(), qe->TimeStampReplay()));

                    }

                    if (IsSmokeQS(qsMode))
                    {
                        SaveImage(rotatedFrame, ObjectUtil::Instance().debugPrefixDir + std::string("orignal_") + boost::lexical_cast<string>(beforeExploratory)  + std::string(".png"));
                        if (clickPoint != EmptyPoint)
                        {
                            DrawCircle(rotatedFrame, Point(clickPoint.x, clickPoint.y), Red, 10);
                            SaveImage(rotatedFrame, ObjectUtil::Instance().debugPrefixDir + boost::lexical_cast<string>(beforeExploratory) + std::string(".png"));
                        }
                        else
                        {
                            SaveImage(cloneFrame, ObjectUtil::Instance().debugPrefixDir + boost::lexical_cast<string>(beforeExploratory) + std::string(".png"));
                        }
                        smokeTest->SetClickExitButtonToHome(isClickOtherObjToHome);
                        smokeTest->SetSharingIssue(expEngine->GetSharingIssueState());
                        smokeTest->SetLoginResult(expEngine->getLoginResult());
                        smokeTest->SetAllEnKeywords(expEngine->getAllEnKeywords());
                        smokeTest->SetAllZhKeywords(expEngine->getAllZhKeywords());
                        smokeTest->SetWholePage(expEngine->getWholePage());
                    }
                }
                seedIteration = seedIteration + 1;
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeExploratory);
            }
            break;
        case QSEventAndAction::OPCODE_POWER:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->PressPower(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->PressPower(ButtonEventMode::Up);
                    else
                        state.status = dut->PressPower(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->PressPower(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_LIGHT_UP:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->WakeUp(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->WakeUp(ButtonEventMode::Up);
                    else
                        state.status = dut->WakeUp(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->WakeUp(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_VOLUME_UP:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->VolumeUp(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->VolumeUp(ButtonEventMode::Up);
                    else
                        state.status = dut->VolumeUp(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->VolumeUp(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_VOLUME_DOWN:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (qsEvent->IntOperand(0) == 1)
                        state.status = dut->VolumeDown(ButtonEventMode::Down);
                    else if (qsEvent->IntOperand(0) == 0)
                        state.status = dut->VolumeDown(ButtonEventMode::Up);
                    else
                        state.status = dut->VolumeDown(ButtonEventMode::DownAndUp);
                }
                else
                    state.status = dut->VolumeDown(ButtonEventMode::DownAndUp);
            }
            break;
        case QSEventAndAction::OPCODE_BRIGHTNESS:
            if (dut != nullptr)
            {
                if (qsEvent->NumIntOperands() == 1)
                {
                    if (0 == qsEvent->IntOperand(0))
                    {
                        dut->BrightnessAction(ButtonEventMode::DownAndUp, Brightness::Up);
                    }
                    else if (1 == qsEvent->IntOperand(0))
                    {
                        dut->BrightnessAction(ButtonEventMode::DownAndUp, Brightness::Down);
                    }
                }
                else
                    dut->BrightnessAction(ButtonEventMode::DownAndUp, Brightness::Up);
            }
            break;
        case QSEventAndAction::OPCODE_DEVICE_POWER:
            if (hwAcc != nullptr)
            {
                if (0 == qsEvent->IntOperand(0))
                {
                    hwAcc->PressPowerButton();
                }
                else if (1 == qsEvent->IntOperand(0))   //Typo for 1!
                {
                    hwAcc->ReleasePowerButton();
                }
                else if (2 == qsEvent->IntOperand(0))
                {
                    hwAcc->ShortPressPowerButton(qsEvent->IntOperand(1));
                }
                else if (3 == qsEvent->IntOperand(0))
                {
                    hwAcc->LongPressPowerButton(qsEvent->IntOperand(1));
                }
            }
            break;
        case QSEventAndAction::OPCODE_AUDIO_MATCH_ON:
        case QSEventAndAction::OPCODE_AUDIO_MATCH_OFF:
            break;
        case QSEventAndAction::OPCODE_HOLDER_UP:
            if (hwAcc != nullptr)
            {
                hwAcc->HolderUp(qsEvent->IntOperand(0));
            }
            break;
        case QSEventAndAction::OPCODE_HOLDER_DOWN:
            if (hwAcc != nullptr)
            {
                hwAcc->HolderDown(qsEvent->IntOperand(0));
            }
            break;
        case QSEventAndAction::OPCODE_USB_IN:
            if (hwAcc != nullptr)
            {
                if (qsEvent->IntOperand(0) == 1)
                {
                    // plug in No.1 USB.
                    hwAcc->PlugUSB(1);
                    unpluggedUSBPort &= ~(1 << 1); 
                }
                else if (qsEvent->IntOperand(0) == 2)
                {
                    // plug in No.2 USB.
                    hwAcc->PlugUSB(3);
                    unpluggedUSBPort &= ~(1 << 2); 
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the number of USB port is wrong!";
                }

                if (unpluggedUSBPort == 0)
                    resultChecker->ResumeOnlineChecking();
            }
            break;
        case QSEventAndAction::OPCODE_USB_OUT:
            if (hwAcc != nullptr)
            {
                resultChecker->PauseOnlineChecking();

                if (qsEvent->IntOperand(0) == 1)
                {
                    // plug out No.1 USB.
                    hwAcc->PlugUSB(2);
                    unpluggedUSBPort |= 1<<1;
                }
                else if (qsEvent->IntOperand(0) == 2)
                {
                    // plug out No.2 USB.
                    hwAcc->PlugUSB(4);
                    unpluggedUSBPort |= 1<<2;
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the number of USB port is wrong!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_POWER_MEASURE_START:
            {
                if (!IsValidForPowerTest())
                {
                    state.status = DaVinciStatus(errc::not_supported);
                    break;
                }

                double startPowerMeasure = qsWatch.ElapsedMilliseconds();
                if (pwt == nullptr)
                {
                    if (dut == nullptr)
                    {
                        DAVINCI_LOG_ERROR << "Cannot find device!";
                        state.status = DaVinciStatus(errc::no_such_device);
                        break;
                    }

                    resultChecker->PauseOnlineChecking();

                    pwt = PowerMeasure::CreateInstance(qs, dut);
                    if (pwt == nullptr)
                    {
                        DAVINCI_LOG_ERROR << "Cannot create power test!";
                        state.status = DaVinciStatus(errc::no_buffer_space);
                        break;
                    }
                }

                DaVinciStatus ret = pwt->Start();
                if (!DaVinciSuccess(ret))
                {
                    DAVINCI_LOG_ERROR << "Cannot starting measuring power!";
                    state.status = ret;
                    break;
                }

                evt = qs->EventAndAction(nextQsIp);
                while (evt != nullptr)
                {
                    if (evt->Opcode() != QSEventAndAction::OPCODE_POWER_MEASURE_STOP)
                    {
                        nextQsIp += 1;
                        evt = qs->EventAndAction(nextQsIp);
                    }
                    else
                    {
                        break;
                    }
                }
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - startPowerMeasure);
            }
            break;
        case QSEventAndAction::OPCODE_POWER_MEASURE_STOP:
            {
                if (!IsValidForPowerTest())
                {
                    state.status = DaVinciStatus(errc::not_supported);
                    break;
                }

                double stopPowerMeasure = qsWatch.ElapsedMilliseconds();
                if (pwt != nullptr)
                {
                    DaVinciStatus ret = pwt->Stop();
                    if (!DaVinciSuccess(ret))
                    {
                        DAVINCI_LOG_ERROR << "Cannot stop measuring power!";
                        state.status = ret;
                        break;
                    }

                    ret = pwt->GenerateReport();
                    if (!DaVinciSuccess(ret))
                    {
                        DAVINCI_LOG_ERROR << "Cannot collect power test result!";
                        state.status = ret;
                        break;
                    }

                    resultChecker->ResumeOnlineChecking();
                }
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - stopPowerMeasure);
            }
            break;
        case QSEventAndAction::OPCODE_TILTCLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_Z_TILTER) == true)
                {
                    if (qsEvent->NumIntOperands() == 1)
                    {
                        hwAcc->ZAxisClockwiseTilt(qsEvent->IntOperand(0));
                    }
                    else if (qsEvent->NumIntOperands() == 2)
                    {
                        hwAcc->ZAxisClockwiseTilt(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                    }
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the Z-axis tilting platform doesn't exist!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTANTICLOCKWISE:
            if (hwAcc != nullptr)
            {
                if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_Z_TILTER) == true)
                {
                    if (qsEvent->NumIntOperands() == 1)
                    {
                        hwAcc->ZAxisAnticlockwiseTilt(qsEvent->IntOperand(0));
                    }
                    else if (qsEvent->NumIntOperands() == 2)
                    {
                        hwAcc->ZAxisAnticlockwiseTilt(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                    }
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the Z-axis tilting platform doesn't exist!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_TILTZAXISTO:
            if (hwAcc != nullptr)
            {
                if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_Z_TILTER) == true)
                {
                    if (qsEvent->NumIntOperands() == 1)
                    {
                        hwAcc->ZAxisTiltTo(qsEvent->IntOperand(0));
                    }
                    else if (qsEvent->NumIntOperands() == 2)
                    {
                        hwAcc->ZAxisTiltTo(qsEvent->IntOperand(0), qsEvent->IntOperand(1));
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "The number of operand is wrong!";
                    }
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Sorry, the Z-axis tilting platform doesn't exist!";
                }
            }
            break;
        case QSEventAndAction::OPCODE_EARPHONE_IN:
            if (hwAcc != nullptr)
            {
                hwAcc->PlugInEarphone();
            }
            break;
        case QSEventAndAction::OPCODE_EARPHONE_OUT:
            if (hwAcc != nullptr)
            {
                hwAcc->PlugOutEarphone();
            }
            break;
        case QSEventAndAction::OPCODE_RELAY_CONNECT:
            if (hwAcc != nullptr)
            {
                if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_RELAY_CONTROLLER) == true)
                {
                    hwAcc->ConnectRelay(qsEvent->IntOperand(0));
                }
            }
            break;
        case QSEventAndAction::OPCODE_RELAY_DISCONNECT:
            if (hwAcc != nullptr)
            {
                if (hwAcc->GetFFRDHardwareInfo(HardwareInfo::Q_HARDWARE_RELAY_CONTROLLER) == true)
                {
                    hwAcc->DisconnectRelay(qsEvent->IntOperand(0));
                }
            }
            break;
        case QSEventAndAction::OPCODE_BUTTON:
            if (dut != nullptr)
            {
                if (boost::algorithm::to_upper_copy(qsEvent->StrOperand(0)) == "ENTER")
                {
                    state.status = dut->PressEnter();
                }
                else if (boost::algorithm::to_upper_copy(qsEvent->StrOperand(0)) == "SEARCH")
                {
                    state.status = dut->PressSearch();
                }
                else if (boost::algorithm::to_upper_copy(qsEvent->StrOperand(0)) == "CLEAR")
                {
                    state.status = dut->ClearInput();
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Wrong button name.";
                }
            }
            break;
        case QSEventAndAction::OPCODE_RANDOM_ACTION:
            {
                string countPercentageStr = qsEvent->StrOperand(0);
                int actionPro = 0;
                string actionType = "CLICK";
                if (countPercentageStr == "" || countPercentageStr == "")
                {
                    actionPro = 100;
                }
                else
                {
                    boost::trim(countPercentageStr);
                    if (countPercentageStr.find("|") != string::npos)
                    {
                        vector<string> items;
                        boost::algorithm::split(items, countPercentageStr, boost::algorithm::is_any_of("|"));
                        actionType = items[0];
                        string probStr = items[1];
                        if (boost::ends_with(probStr, "%"))
                        {
                            probStr = probStr.substr(0, probStr.length() - 1);
                        }

                        bool isInt = TryParse(probStr, actionPro);
                        if (!isInt || actionPro > 100)
                        {
                            actionPro = 100;
                        }
                    }
                    else
                    {
                        actionType = countPercentageStr;
                        actionPro = 100;
                    }
                }
                expEngine->DispatchRandomAction(frame, actionType, actionPro);
            }
            break;
        case QSEventAndAction::OPCODE_FPS_MEASURE_START:
            if (qsEvent->StrOperand(0) == "")
            {
                if (fpsTest != nullptr)
                {
                    fpsTest->Start();
                }
            }
            break;
        case QSEventAndAction::OPCODE_FPS_MEASURE_STOP:
            if (qsEvent->StrOperand(0) == "")
            {
                if (fpsTest != nullptr)
                {
                    fpsTest->PreDestroy();
                    StopTest(fpsTest);
                }
            }
            break;
        case QSEventAndAction::OPCODE_DUMP_UI_LAYOUT:
            {
                string outputXMLname = qsEvent->StrOperand(0);
                string currentQSfolder = TestReport::currentQsDirName;

                double beforeDumpUI = qsWatch.ElapsedMilliseconds();

                if (outputXMLname == "")
                {
                    DAVINCI_LOG_ERROR << "Missing output xml file name";
                    break;
                }

                // get current device name
                string currentDeviceName = DeviceManager::Instance().GetCurrentTargetDevice()->GetDeviceName();

                // save screen capture before dump UI layout
                boost::filesystem::path outPath(TestReport::currentQsLogPath);
                outPath /= boost::filesystem::path("UI_LAYOUT");
                string outputDir = outPath.string();

                if (!boost::filesystem::exists(outputDir))
                {
                    boost::system::error_code ec;
                    boost::filesystem::create_directory(outputDir, ec);
                    DaVinciStatus status(ec);
                    if (!DaVinciSuccess(status))
                    {
                        DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << outputDir;
                    }
                }

                string imageName = outputDir + string("\\dump_ui_layout_") + boost::lexical_cast<std::string>(beforeDumpUI) + ".png";
                Mat frame = GetCurrentFrame();
                Mat rotatedFrame = RotateFrameUp(frame);
                if (!rotatedFrame.empty())
                    imwrite(imageName, rotatedFrame);

                // if the output file is not an absolute path, we will put the output file under Qscript folder
                if (!boost::contains(outputXMLname, "\\"))
                {
                    outputXMLname = currentQSfolder + "\\" + outputXMLname;
                }

                // disconnect agent
                int count = 0;
                dut->Disconnect();

                while (dut->IsAgentConnected())
                {
                    ThreadSleep(1000);
                    ++ count;
                    if (count > 30)
                    {
                        DAVINCI_LOG_ERROR << "Couldn't disconnect agent!" << qsEvent->ToString();
                        break;
                    }
                }

                // dump uiautomator
                string dumpXML = "/data/local/tmp/window_dump.xml";
                auto outputStr = boost::shared_ptr<ostringstream>(new ostringstream());
                string cmd = " -s " + currentDeviceName + " shell uiautomator dump " + dumpXML;
                DaVinciStatus retValue = AndroidTargetDevice::AdbCommandNoDevice(cmd, outputStr, 60000);
                if (!DaVinciSuccess(retValue)) 
                {
                    DAVINCI_LOG_ERROR << "Cannot execute adb command to dump uiautomator !";
                    break;
                }

                cmd = " -s " + currentDeviceName + " pull " + dumpXML + " " + outputXMLname;
                retValue = AndroidTargetDevice::AdbCommandNoDevice(cmd, outputStr, 60000);
                if (!DaVinciSuccess(retValue)) 
                {
                    DAVINCI_LOG_ERROR << "Cannot execute adb command to pull out xml file !";
                    break;
                }

                // connect agent
                count = 0;
                dut->Connect();

                while (!dut->IsAgentConnected())
                {
                    ThreadSleep(1000);
                    ++ count;
                    if (count > 60)
                    {
                        DAVINCI_LOG_ERROR << "Couldn't connect agent!" << qsEvent->ToString();
                        break;
                    }
                }
                state.timeStampOffset -= (qsWatch.ElapsedMilliseconds() - beforeDumpUI);
            }
            break;
        default:
            DAVINCI_LOG_ERROR << "    Invalid OPCODE: " << qsEvent->ToString();
            break;
        }

        // Do check for the last ip
        if (nextQsIp > static_cast<int>(qs->NumEventAndActions()))
        {
            resultChecker->Dialog->ScanFrame(frame, QSEventAndAction::OPCODE_EXIT);
        }
        state.currentQsIp = nextQsIp;
    }

    ScriptReplayer::MachineState::MachineState()
    {
        Init();
    }

    void ScriptReplayer::MachineState::Init()
    {
        currentQsIp = 1;
        exited = false;
        clickInHorizontal = false;
        timeStampOffset = 0;
        imageTimeOutWaiting = false;
        inConcurrentMatch = false;
        ipBeforeConcurrentMatch = 0;
        instrCountLeft = 0;
        timeStampOffsetBeforeConcurrentMatch = 0;
        concurrentMatchStartTime = 0;
    }

    void ScriptReplayer::SetStartQSIp(unsigned int ip)
    {
        state.currentQsIp = ip;
    }

    void ScriptReplayer::SetEndQSIp(unsigned int ip)
    {
        endQsIp = ip;
    }

    void ScriptReplayer::SetStartTimeStampOffset(double tso)
    {
        state.timeStampOffset = tso;
    }

    void ScriptReplayer::SetActionState()
    {
        state.exited = false;
    }

    void ScriptReplayer::SetEntryScript(bool flag)
    {
        isEntryScript = flag;
    }

    ScriptReplayer::ConcurrentEvent::ConcurrentEvent(const boost::shared_ptr<QScript::ImageObject> &obj, int ip, unsigned int count, bool matchOnce)
        : imageObject(obj), instrCount(count), isMatchOnce(matchOnce), isActivated(false)
    {
        ccIp = ip;
        if (instrCount == 0)
        {
            startIp = 0;
        }
        else
        {
            startIp = ccIp + 1;
        }
    }

    Mat ScriptReplayer::GetImageForOCR(const Mat &frame, int &param)
    {
        if (dut == nullptr)
        {
            param |= TextRecognize::PARAM_CAMERA_IMAGE;
            return frame;
        }

        Orientation currentOrientation = dut->GetCurrentOrientation(true);
        Mat screencap;
        if ((param & TextRecognize::PARAM_CAMERA_IMAGE) == 0)
        {
            if (dut->GetScreenSource() == ScreenSource::HyperSoftCam)
            {
                screencap = frame;
            }
            else
            {
                screencap = dut->GetScreenCapture();
            }
        }
        if (!screencap.empty())
        {
            if (currentOrientation == Orientation::Landscape)
            {
                if (screencap.cols < screencap.rows)
                {
                    screencap = RotateImage270(screencap);
                }
            }
            else
            {
                if (screencap.cols > screencap.rows)
                {
                    screencap = RotateImage90(screencap);
                }
            }
        }
        else
        {
            screencap = frame;
            if (currentOrientation != Orientation::Landscape)
            {
                screencap = RotateImage90(screencap);
            }
            param |= TextRecognize::PARAM_CAMERA_IMAGE;
        }
        return screencap;
    }

    void ScriptReplayer::SetQSMode(QSMode flag)
    {
        this->qsMode = flag;
    }

    QSMode ScriptReplayer::GetQSMode()
    {
        return this->qsMode;
    }

    bool ScriptReplayer::GetCrashDialogDetected()
    {
        if ((resultChecker != nullptr) && (resultChecker->Dialog != nullptr))
        {
            if ((resultChecker->Dialog->GetResult() == CheckerResult::Fail) &&
                (resultChecker->Dialog->GetDialogType() == DialogCheckResult::crashDialog)) 
                return true;
            else
                return false;
        }

        return false;
    }

    bool ScriptReplayer::GetANRDialogDetected()
    {
        if ((resultChecker != nullptr) && (resultChecker->Dialog != nullptr))
        {
            if ((resultChecker->Dialog->GetResult() == CheckerResult::Fail) && 
                (resultChecker->Dialog->GetDialogType() == DialogCheckResult::anrDialog)) 
                return true;
            else
                return false;
        }

        return false;
    }

    cv::Mat ScriptReplayer::GetCrashDialogFrame()
    {
        if ((resultChecker != nullptr) && (resultChecker->Dialog != nullptr))
            return resultChecker->Dialog->GetLastCrashFrame();
        else
            return Mat();
    }

    string ScriptReplayer::GetQsDirectory()
    {
        if (qs != nullptr)
            return qs->GetResourceFullPath("");
        return ".";
    }

    string ScriptReplayer::GetQsName()
    {
        if (qs != nullptr)
            return qs->GetScriptName();
        return "QScript";
    }

    /// <summary>
    /// Now we activate the concurrent matching when the opcode is encountered. For
    /// backward-compatibility, we activate those concurrent events residing at the
    /// end of the script by default. These events are generated by the legacy popup-killer
    /// wizard.
    /// </summary>
    void ScriptReplayer::ActivateTailingConcurrentEvents()
    {
        bool moreActivated;
        do {
            moreActivated = false;
            for (auto pair : state.ccEventTable)
            {
                if (!pair.second->isActivated)
                {
                    int nextIp = pair.first + pair.second->instrCount + 1;
                    // next block resides at the end of the script or
                    // above the tailing (activated) concurrent match blocks
                    if (nextIp > static_cast<int>(qs->NumEventAndActions()) ||
                        (state.ccEventTable.find(nextIp) != state.ccEventTable.end() &&
                        state.ccEventTable[nextIp]->isActivated))
                    {
                        pair.second->isActivated = true;
                        moreActivated = true;
                    }
                }
            }
        } while (moreActivated);
    }

    vector<Mat> ScriptReplayer::ComputeHistogram(const Mat &image)
    {
        vector<Mat> hists;
        float h_ranges[] = { 0, 180 };
        float s_ranges[] = { 0, 256 };
        float v_ranges[] = { 0, 256 };
        const float* ranges[] = { h_ranges, s_ranges, v_ranges };
        int channels[] = { 0, 1, 2 };
        int histSize[] = { 180, 256, 256 };

        Mat hist, hsv;
        cvtColor(image, hsv, COLOR_BGR2HSV);
        for (size_t i = 0; i < 3; i++)
        {
            calcHist(&hsv, 1, channels + i, Mat(), hist, 1, histSize + i, ranges + i, true, false );
            hists.push_back(hist);
        }
        return hists;
    }

    double ScriptReplayer::CompareHistogram(const vector<Mat> &hist1, const vector<Mat> &hist2)
    {
        double diff = 1;
        assert(hist1.size() == hist2.size());
        for (size_t i = 0; i < hist1.size(); i++)
        {
            diff *= compareHist(hist1[i], hist2[i], CV_COMP_CORREL);
        }
        return diff;
    }

    int ScriptReplayer::GetQsIp()
    {
        return state.currentQsIp;
    }

    void ScriptReplayer::SaveGeneratedScriptForSmokeTest()
    {
        if(generateSmokeQsript)
        {        

            boost::filesystem::path qsPath = boost::filesystem::path(generateSmokeQsript->GetQsFileName());
            boost::filesystem::path qsFolder = qsPath.parent_path();
            OnDeviceScriptRecorder::CalibrateIDAction(generateSmokeQsript, qsFolder);

            if(generateSmokeQsript->GetAllQSEventsAndActions().size() > 0){
                double dt = generateSmokeQsript->GetAllQSEventsAndActions()[generateSmokeQsript->GetAllQSEventsAndActions().size()-1]->TimeStamp();
                generateSmokeQsript->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(dt+30000,
                    QSEventAndAction::OPCODE_STOP_APP)));
                generateSmokeQsript->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(dt+33000,
                    QSEventAndAction::OPCODE_UNINSTALL_APP)));
            }

            generateSmokeQsript->Flush();
            if (needVideoRecording)
            {
                path expQts(TestReport::currentQsLogPath);
                expQts /= "Replay";
                expQts /= generateSmokeQsript->GetScriptName() + ".qts";
                copy_file(qtsFileName, expQts, copy_option::overwrite_if_exists);

                string generatedQts = generateSmokeQsript->GetQtsFileName();
                ofstream qts;
                qts.open(generatedQts, ios::out | ios::trunc);
                if (qts.is_open())
                {
                    qts << "Version:1" << endl;
                    for (auto ts = timeStampList.begin(); ts != timeStampList.end(); ts++)
                    {
                        qts << static_cast<int64_t>(*ts) << endl;
                    }
                    vector<QScript::TraceInfo> & trace = generateSmokeQsript->Trace();
                    for (auto traceInfo = trace.begin(); traceInfo != trace.end(); traceInfo++)
                    {
                        qts << ":" << traceInfo->LineNumber() << ":" << static_cast<int64>(traceInfo->TimeStamp()) << endl;
                    }
                    qts.close();                   
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Unable to open QTS file " << generatedQts << " for writing.";
                }
            }

            path xmlPath(TestReport::currentQsLogPath);
            xmlPath /= "Replay";
            xmlPath /= generateSmokeQsript->GetScriptName() + ".xml";
            OnDeviceScriptRecorder::GenerateXMLConfiguration(generateSmokeQsript->GetScriptName(), xmlPath.string());
        }
    }

    bool ScriptReplayer::IsSwipeAction(boost::shared_ptr<QScript> qs, int ip)
    {
        boost::shared_ptr<QSEventAndAction> qsCurEvent = qs->EventAndAction(ip);
        if (qsCurEvent == nullptr)
            return false;

        int nextQsIp = ip + 1;
        boost::shared_ptr<QSEventAndAction> qsNextEvent = qs->EventAndAction(nextQsIp);
        while (qsNextEvent != nullptr)
        {
            if (qsNextEvent->Opcode() == QSEventAndAction::OPCODE_TOUCHUP ||
                qsNextEvent->Opcode() == QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL)
            {
                break;
            }
            qsNextEvent = qs->EventAndAction(++nextQsIp);
        }
        assert(qsNextEvent != nullptr);

        // https://android.googlesource.com/platform/frameworks/base/+/cd92588/cmds/input/src/com/android/commands/input/Input.java
        // line: 201
        //     private void sendSwipe(int inputSource, float x1, float y1, float x2, float y2, int duration) {
        //         if (duration < 0) {
        //             duration = 300;
        //         }
        // But when we checked QS and found that the interval of most swipe actions is between 300~850ms,
        // so we set the default swipe interval is 1000ms
        const int DEFAULT_ANDROID_SWIPE_INTERVAL = 1000; //(MS)
        double touchDownUpInterval = qsNextEvent->TimeStamp() - qsCurEvent->TimeStamp();
        if (touchDownUpInterval > ClickInterval && touchDownUpInterval < DEFAULT_ANDROID_SWIPE_INTERVAL)
        {
            // In case of long touch
            if (qsCurEvent->IntOperand(0) != qsNextEvent->IntOperand(0) ||
                qsCurEvent->IntOperand(1) != qsNextEvent->IntOperand(1))
            {
                if (!TargetDevice::IsClickAction(qsCurEvent->IntOperand(0),
                    qsCurEvent->IntOperand(1),
                    qsCurEvent->TimeStamp(),
                    qsNextEvent->IntOperand(0),
                    qsNextEvent->IntOperand(1),
                    qsNextEvent->TimeStamp()))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void ScriptReplayer::ClickOffAllowButton()
    {
        int maxAllowNumber = 6;

        // For smoke test, check popup window before stop app
        for (int i = 0; i < maxAllowNumber; ++i)
        {
            std::vector<std::string> windowLines;
            bool isPopUpWindow;

            if (dut == nullptr)
                return;

            boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);

            if (androidDut != nullptr)
            {
                Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
                Orientation orientation = dut->GetCurrentOrientation(true);
                AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
                isPopUpWindow = windowBarInfo.isPopUpWindow;

                if (!isPopUpWindow)
                    isPopUpWindow = ObjectUtil::Instance().IsPopUpDialog(deviceSize, orientation, windowBarInfo.statusbarRect, windowBarInfo.focusedWindowRect);

                // check 'allow' button on popup window, click off 'allow' button
                if (isPopUpWindow)
                {
                    Rect allowRect = EmptyRect;
                    string systemLanguage = ObjectUtil::Instance().GetSystemLanguage();
                    boost::shared_ptr<KeywordRecognizer> keywordRecog = expEngine->getKeywordRecog();
                    boost::shared_ptr<KeywordCheckHandlerForPopupWindow> popupWindowHandler = boost::shared_ptr<KeywordCheckHandlerForPopupWindow>(new KeywordCheckHandlerForPopupWindow(keywordRecog));
                     
                    Mat frame = GetCurrentFrame();
                    Mat rotatedFrame = RotateFrameUp(frame);
                    expEngine->generateAllRectsAndKeywords(rotatedFrame, Rect(0, 0, rotatedFrame.size().width, rotatedFrame.size().height), systemLanguage);

                    popupWindowHandler->HandleRequest(systemLanguage, expEngine->getAllEnKeywords(), expEngine->getAllZhKeywords());

                    if (popupWindowHandler != nullptr)
                    {
                        allowRect = popupWindowHandler->GetOkRect(rotatedFrame);
                    }

                    if (allowRect != EmptyRect)
                    {
                        ClickOnRotatedFrame(rotatedFrame.size(), Point(allowRect.x + allowRect.width / 2, allowRect.y + allowRect.height / 2));
                        DAVINCI_LOG_INFO << "Click off 'allow' button before stopping application";
                        ThreadSleep(1000);
                    }
                }
            }
        }
    }
}

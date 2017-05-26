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

#include <boost/core/null_deleter.hpp>
#include "boost/filesystem.hpp"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/algorithm/string.hpp"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "MainTest.hpp"
#include "DaVinciCommon.hpp"
#include "DaVinciDefs.hpp"
#include "DeviceManager.hpp"
#include "SerializationUtil.hpp"
#include "ShapeUtil.hpp"
#include "ScriptHelper.hpp"
#include "QReplayInfo.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;
    using namespace boost;

    MainTest::MainTest(const std::string &xmlFilename)
    {
        xmlFile = xmlFilename;

        InitVariables();
        InitSharedPtrs();
    }

    void MainTest::InitVariables()
    {
        unknownQsName = "unknown.qs"; 
        isEntryScript = false;
        isEntryScriptStarted = false;
        isScheduleStart = false;
        isScheduleComplete = false;
        startRandomAction = false;
        needTapMode = false;
        currentScriptReplayer = nullptr;
        currentScript = nullptr;
        matchIndex = 0;
        randomIndex = 0;
        currentRandomAction = 0;
        processExit = false;
        launchFail = false;
        imageCheckPass = true;
        videoRecording = false;
        audioSaving = false;
        needPhysicalCamera = false;
        isPopUpWindow = false;
        isSystemPopUpWindow = false;
        replayKeyboardPopUp = false;
        globalStretchMode = "";
        currentMatchPattern = "";
        isPatternSwitch = false;
        replayHasVirtualBar = false;
        popupFailure = false;
        entryStartApp = false;
        actionWaitTime = 0;
        frameUnmatchRatioThreshold = 0;
        currentFrameUnmatchRatioThreshold = 0;
        launchWaitTime = 0;
        maxRandomAction = 0;
        randomAfterUnmatchAction = 0;
        staticActionNumber = 0;
        timeout = 0;
        startLoginMessage = "LOGIN_START";
        finishLoginMessage = "LOGIN_END";
        recordDeviceSize = Size(0, 0);
        replayDeviceSize = Size(0, 0);
        recordDPI = 0;
        replayDPI = 0;

        systemFocusWindows.push_back("android");
        systemFocusWindows.push_back("com.android.settings");
        systemFocusWindows.push_back("com.google.android.gms");

        systemPopUpWindows.push_back("com.google.android.packageinstaller");
    }

    void MainTest::CleanUpDebugResources()
    {
        boost::filesystem::path directory(replayFolder);
        vector<boost::filesystem::path> filePaths;

        getAllFiles(directory, filePaths);

        for(auto file: filePaths)
        {
            boost::filesystem::remove(file);
        }
    }

    // copy debug resources to _Logs_ folder
    void MainTest::CopyDebugResources()
    {
        try
        {
            assert(boost::filesystem::exists(recordFolder));
            {
                boost::filesystem::path recordDestPath(TestReport::currentQsLogPath);
                recordDestPath /= boost::filesystem::path("_RECORD_");
                boost::filesystem::create_directory(recordDestPath);

                boost::filesystem::path directory(recordFolder);
                vector<boost::filesystem::path> filePaths;
                getAllFiles(directory, filePaths);
                for(auto file: filePaths)
                {
                    boost::filesystem::path destPath = recordDestPath;
                    destPath /= boost::filesystem::path(file.filename().string());
                    boost::filesystem::copy(file, destPath);
                }
            }

            assert(boost::filesystem::exists(replayFolder));
            {
                boost::filesystem::path replayDestPath(TestReport::currentQsLogPath);
                replayDestPath /= boost::filesystem::path("_REPLAY_");
                boost::filesystem::create_directory(replayDestPath);

                boost::filesystem::path directory(replayFolder);
                vector<boost::filesystem::path> filePaths;
                getAllFiles(directory, filePaths);
                for(auto file: filePaths)
                {
                    boost::filesystem::path destPath = replayDestPath;
                    destPath /= boost::filesystem::path(file.filename().string());
                    boost::filesystem::copy(file, destPath);
                }
            }
        }
        catch(const std::exception&)
        { 
            DAVINCI_LOG_WARNING << "Failed to copy critical path resources.";
        }
    }

    bool MainTest::DoQSPreprocess(bool isBreakPoint)
    {
        boost::filesystem::path xmlPath(xmlFile);
        xmlPath = system_complete(xmlPath);
        xmlFolder = xmlPath.parent_path();


        if (dut != nullptr)
        {
            replayDeviceSize = Size(dut->GetDeviceWidth(), dut->GetDeviceHeight());
            replayDPI = dut->GetDPI();
            replayNavigationBarHeights = dut->GetNavigationBarHeight();
            languageMode = ObjectUtil::Instance().GetSystemLanguage();
        }
        else
        {
            return false;
        }

        eventDispatcher->SetDevice(dut);

        recordFolder = xmlFolder;
        recordFolder /= boost::filesystem::path("_RECORD_");
        if (!boost::filesystem::exists(recordFolder))
        {
            boost::filesystem::create_directory(recordFolder);
        }

        replayFolder = xmlFolder;
        replayFolder /= boost::filesystem::path("_REPLAY_");
        if (!boost::filesystem::exists(replayFolder))
        {
            boost::filesystem::create_directory(replayFolder);
        }
        CleanUpDebugResources();

        biasObject = biasFileReader->loadDavinciConfig(xmlFile);

        if(biasObject == nullptr)
        {
            DAVINCI_LOG_ERROR << "Error: Invalid configuration XML file: " << xmlFile << endl;
            return false;
        }

        eventRandomGenerator =  boost::shared_ptr<SeedRandom>(new SeedRandom(biasObject->GetSeedValue()));
        timeout = biasObject->GetTimeOut() * 1000;
        launchWaitTime = biasObject->GetLaunchWaitTime() * 1000;
        actionWaitTime = biasObject->GetActionWaitTime() * 1000;
        randomAfterUnmatchAction = biasObject->GetRandomAfterUnmatchAction();
        maxRandomAction = biasObject->GetMaxRandomAction();
        frameUnmatchRatioThreshold = biasObject->GetUnmatchRatio(); 
        matchPattern = biasObject->GetMatchPattern();
        videoRecording = biasObject->GetVideoRecording();
        audioSaving = biasObject->GetAudioSaving();

        if(videoRecording)
        {
            boost::filesystem::path videoPath = xmlFolder;             
            videoPath /= xmlPath.stem().string() + "_replay.avi";
            videoFileName = videoPath.string();
        }

        if(audioSaving)
        {
            boost::filesystem::path audioPath = xmlFolder;

            audioPath /= xmlPath.stem().string() + "_replay.wav";
            audioFileName = audioPath.string();
        }

        staticActionNumber = biasObject->GetStaticActionNumber();
        checkers = biasObject->GetCheckers();
        events = biasObject->GetEvents();

        boost::filesystem::path scriptPath = xmlFolder;
        // Preprocess the recorded scripts, the first qs is entry one which requires uninstall, install, and launch
        for (auto scriptPair : biasObject->GetQScripts())
        {
            string scriptName = scriptPair.first;
            string scriptPriority = scriptPair.second;
            scriptPriorities[scriptName] = StringToPriority(scriptPriority);
            scriptPath = xmlFolder;
            scriptPath /= scriptName;
            if (!is_regular_file(scriptPath) || extension(scriptPath) != ".qs")
            {
                DAVINCI_LOG_INFO << scriptPath << " does not exist" << endl;
                return false;
            }

            boost::shared_ptr<ScriptReplayer> scriptTest = boost::shared_ptr<ScriptReplayer>(new ScriptReplayer(scriptPath.string()));

            scriptTest->SetScriptReplayMode(ScriptReplayMode::REPLAY_COORDINATE);
            scriptTest->EnableBreakPoint(isBreakPoint);
            if (isEntryScript)
            {
                scriptTest->SetEntryScript(false);
            }

            if (!scriptTest->Init())
            {
                DAVINCI_LOG_WARNING << "Failed to initialize ScriptReplayer, stop the test.";
                return false;
            }
            // Stop online result checking from script replayer
            scriptTest->StopOnlineChecking();
            scriptReplayers[scriptName] = scriptTest;

            boost::shared_ptr<QScript> qs = scriptTest->GetQs();
            scripts[scriptName] = qs;
            scriptHelper->SetCurrentQS(qs);

            if (!isEntryScript)
            {
                isEntryScript = true;
                entryQSName = scriptName;
                SetCurrentQSIpPair(entryQSName, 1);

                packageName = qs->GetPackageName();

                if(packageName == "")
                {
                    DAVINCI_LOG_WARNING << "Invalid Q scripts with no package name, stop the test.";
                    return false;
                }

                startPackageName = dut->GetTopPackageName();

                boost::filesystem::path reportDestPath(TestReport::currentQsLogPath);
                reportDestPath /= boost::filesystem::path("ReportSummary.xml");
                mainTestReport = boost::shared_ptr<MainTestReport>(new MainTestReport(packageName, reportDestPath.string()));
                mainTestReport->SetRecordFolder(recordFolder);
                mainTestReport->SetReplayFolder(replayFolder);
                mainTestReport->SetXmlFolder(xmlFolder);

                activityName = qs->GetActivityName();
                apkName = qs->GetApkName();

                string xmlFolderPath = xmlFolder.string();
                string apkPath = qs->GetResourceFullPath(apkName);

                vector<string> appInfoSmoke = AndroidTargetDevice::GetPackageActivity(apkPath);
                assert(appInfoSmoke.size() == AndroidTargetDevice::APP_INFO_SIZE);
                appName = appInfoSmoke[AndroidTargetDevice::APP_NAME];

                needTapMode = qs->IsMultiLayerMode();
                recordStatusBarHeights = qs->GetStatusBarHeight();
                recordNavigationBarHeights = qs->GetNavigationBarHeight();

                currentScriptReplayer = scriptTest;
                currentScriptReplayer->SetQSMode(QS_RNR);
                currentScriptReplayer->SetSmokeOutputDir(TestReport::currentQsLogPath);

                currentScript = qs;
                coverageCollector->PrepareCoverageCollector(packageName, biasObject->GetSeedValue(), xmlFolder.string());
                if (activityName.empty())
                {
                    activityName= dut->GetLaunchActivityByQAgent(packageName);
                }
                mainTestReport->PrepareSmokeTestReport(xmlFolderPath, apkName, apkPath, packageName, activityName, appName, qs->GetScriptName());
                exploreEngine = boost::shared_ptr<ExploratoryEngine>(new ExploratoryEngine(packageName, activityName, qs->GetIconName(), dut, qs->GetOSversion()));

            }

            boost::filesystem::path mtimePath = recordFolder;
            mtimePath /= boost::filesystem::path(scriptName + "_mtime");
            boost::filesystem::path serializationPath = recordFolder;
            serializationPath /= boost::filesystem::path(scriptName + "_serialization");

            if (!boost::filesystem::exists(mtimePath)) // The first time to run test of the script, get scriptPreprocessInfos by generate and serialize them
            {
                boost::shared_ptr<struct stat> info = boost::shared_ptr<struct stat>(new struct stat());
                int status = stat(scriptPath.string().c_str(), info.get());
                assert(status != -1);
                DAVINCI_LOG_INFO << "Info Status: " << boost::lexical_cast<string>(status) << " CurrentQSMTime: " << boost::lexical_cast<string>(info->st_mtime);
                scriptPreprocessInfos[scriptName] = scriptHelper->OrganizeQSPreprocessInfoForKeyboard(scriptHelper->GenerateQSPreprocessInfo(recordFolder.string(), matchPattern));
                if(scriptPreprocessInfos[scriptName].size() > 0)
                {
                    SerializationUtil::Instance().Serialize<long>(mtimePath.string(), (long)info->st_mtime); // Store qs size, if mtime was changed, test should be stopped
                    SerializationUtil::Instance().Serialize<std::map<int, ScriptHelper::QSPreprocessInfo>>(serializationPath.string(), scriptPreprocessInfos[scriptName]);
                }
                else
                {
                    DAVINCI_LOG_WARNING << "No serlialized data.";
                }
            }
            else
            {
                boost::shared_ptr<struct stat> info = boost::shared_ptr<struct stat>(new struct stat());
                int status = stat(scriptPath.string().c_str(), info.get());
                assert(status != -1);
                long currentQSMTime = (long)info->st_mtime;
                long preQSMTime = SerializationUtil::Instance().Deserialize<long>(mtimePath.string());
                DAVINCI_LOG_INFO << "Info Status: " << boost::lexical_cast<string>(status) << " CurrentQSMTime: " << boost::lexical_cast<string>(currentQSMTime) << " - PreQSMTime: " << boost::lexical_cast<string>(preQSMTime);
                if (currentQSMTime != preQSMTime) // QS was changed, test should be stopped
                {
                    DAVINCI_LOG_INFO << "Regenerate serialization data for RnR scripts";
                    scriptPreprocessInfos[scriptName] = scriptHelper->OrganizeQSPreprocessInfoForKeyboard(scriptHelper->GenerateQSPreprocessInfo(recordFolder.string(), matchPattern));
                    if(scriptPreprocessInfos[scriptName].size() > 0)
                    {
                        SerializationUtil::Instance().Serialize<long>(mtimePath.string(), (long)info->st_mtime); // Store qs size, if mtime was changed, test should be stopped
                        SerializationUtil::Instance().Serialize<std::map<int, ScriptHelper::QSPreprocessInfo>>(serializationPath.string(), scriptPreprocessInfos[scriptName]);
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING << "No serlialized data.";
                    }
                }
                else if (boost::filesystem::exists(serializationPath)) // Get scriptPreprocessInfos by deserialize
                {
                    scriptPreprocessInfos[scriptName] = SerializationUtil::Instance().Deserialize<std::map<int, ScriptHelper::QSPreprocessInfo>>(serializationPath.string());
                    assert(scriptPreprocessInfos[scriptName].size() != 0);
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Failed to open binary file for deserialize, stop the test.";
                    return false;
                }
            }
        }

        // Correct timeout based on entry script timestamp
        map<int, ScriptHelper::QSPreprocessInfo> entryQSPreprocessInfo = scriptPreprocessInfos[entryQSName];
        if(entryQSPreprocessInfo.size() > 0)
        {
            map<int, ScriptHelper::QSPreprocessInfo>::const_reverse_iterator infoIt = entryQSPreprocessInfo.rbegin();
            int timeoutFromQScript = int(scripts[entryQSName]->EventAndAction(infoIt->first)->TimeStamp() * defaultTimes);
            timeout = max(timeoutFromQScript, timeout);
        }

        for(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>::const_iterator scriptIt = scriptPreprocessInfos.begin(); scriptIt != scriptPreprocessInfos.end(); ++scriptIt) 
        {
            string qsName = scriptIt->first;
            map<int, ScriptHelper::QSPreprocessInfo> qsInfos = scriptIt->second;
            for(map<int, ScriptHelper::QSPreprocessInfo>::const_iterator qsIt = qsInfos.begin(); qsIt != qsInfos.end(); ++qsIt) 
            {
                if(qsIt->second.isTouchAction)
                {
                    if(boost::equals(qsName, entryQSName))
                        recordTouchSequence.push_back(qsIt->first);
                }
                recordIpSequence.push_back(QSIpPair(qsName, qsIt->first, scripts[qsName]->EventAndAction(qsIt->first)->GetOrientation(), false, qsIt->second.isTouchAction));
            }
        }

        eventSequence = GenerateEventSequence();

        // Set resources for report
        mainTestReport->SetUnknownQSName(unknownQsName);
        mainTestReport->SetLanguageMode(languageMode);
        mainTestReport->SetExploratoryEngine(exploreEngine);
        mainTestReport->SetCheckerInfos(checkers);
        mainTestReport->SetScripts(scripts);
        mainTestReport->SetScriptInfos(scriptPreprocessInfos);
        mainTestReport->SetRecordIpSequence(recordIpSequence);

        boost::filesystem::path replayPath(TestReport::currentQsLogPath);        
        replayPath /= "Replay";
        boost::filesystem::create_directory(replayPath);

        string rnrPath = replayPath.string() + "/" + xmlPath.stem().string() + "_replay.qs";     

        generatedGetRnRQScript = QScript::New(rnrPath);    
        generatedGetRnRQScript->SetConfiguration("PackageName", packageName);
        exploreEngine->SetGeneratedQScript(generatedGetRnRQScript);        
        SetBuiltInResolutions();
        return true;
    }

    void MainTest::InitSharedPtrs()
    {
        scriptHelper = boost::shared_ptr<ScriptHelper>(new ScriptHelper());
        shapeMatcher = boost::shared_ptr<ShapeMatcher>(new ShapeMatcher());
        shapeExtractor = boost::shared_ptr<ShapeExtractor>(new ShapeExtractor());

        akazeRecog = boost::shared_ptr<AKazeObjectRecognize>(new AKazeObjectRecognize());
        featMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(akazeRecog));

        pointMatcher = boost::shared_ptr<PointMatcher>(new PointMatcher());
        androidBarRecog = boost::shared_ptr<AndroidBarRecognizer>(new AndroidBarRecognizer());
        crossRecognizer = boost::shared_ptr<CrossRecognizer>(new CrossRecognizer());
        coverageCollector = boost::shared_ptr<CoverageCollector>(new CoverageCollector());
        stopWatch = boost::shared_ptr<StopWatch>(new StopWatch());
        dispatchTimer = boost::shared_ptr<HighResolutionTimer>(new HighResolutionTimer());

        keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        keywordCheckHandler1 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForCrashANR(keywordRecog));
        keywordCheckHandler2 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForCrashANR(keywordRecog, false));
        keywordCheckHandler1->SetSuccessor(keywordCheckHandler2);

        viewHierarchyParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
        sceneTextExtractor = boost::shared_ptr<SceneTextExtractor>(new SceneTextExtractor());

        testReport = boost::shared_ptr<TestReport>(new TestReport());
        biasFileReader = boost::shared_ptr<BiasFileReader>(new BiasFileReader());
        eventDispatcher = boost::shared_ptr<EventDispatcher>(new EventDispatcher());

        keyboardRecog = boost::shared_ptr<KeyboardRecognizer>(new KeyboardRecognizer());
    }

    bool MainTest::Init()
    {
        dut = DeviceManager::Instance().GetCurrentTargetDevice();

        if ((dut == nullptr) && (DeviceManager::Instance().GetEnableTargetDeviceAgent()))
        {
            DAVINCI_LOG_WARNING << "No target device is connected.";
            return false;
        }

        prepareThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MainTest::PrepareForDispatchHandler, this)));
        SetThreadName(prepareThread->get_id(), "Preprocess RnR resources");
        StartDispatchTimer();
        return true;
    }

    cv::Mat MainTest::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        if(!isScheduleStart)
        {
            return frame;
        }
        else
        {
            SpinLockGuard lock(currentFrameLock);
            if(DeviceManager::Instance().UsingDisabledCam())
            {
                double screenCapStartTime = GetCurrentMillisecond();
                currentFrame = dut->GetScreenCapture();
                double screenCapEndTime = GetCurrentMillisecond();
                DAVINCI_LOG_DEBUG << "Screencap from " << boost::lexical_cast<string>(screenCapStartTime) << " to " << boost::lexical_cast<string>(screenCapEndTime) << endl;

                if(currentFrame.empty())
                {
                    DAVINCI_LOG_ERROR << "Cannot grab the screen capture frame; return empty frame." << endl;
                    Mat emptyMat = Mat(currentFrame.size(), currentFrame.type());
                    return emptyMat;
                }
            }
            else
            {
                if(DeviceManager::Instance().UsingHyperSoftCam())
                {
                    currentFrame = frame;
                }
                else
                {
                    if(!needPhysicalCamera)
                    {
                        double screenCapStartTime = GetCurrentMillisecond();
                        currentFrame = dut->GetScreenCapture();
                        double screenCapEndTime = GetCurrentMillisecond();
                        DAVINCI_LOG_DEBUG << "Screencap from " << boost::lexical_cast<string>(screenCapStartTime) << " to " << boost::lexical_cast<string>(screenCapEndTime) << endl;

                        if(currentFrame.empty())
                        {
                            DAVINCI_LOG_ERROR << "Cannot grab the screen capture frame; return empty frame." << endl;
                            Mat emptyMat = Mat(currentFrame.size(), currentFrame.type());
                            return emptyMat;
                        }
                    }
                    else
                    {
                        currentFrame = frame;
                    }
                }
            }

            if(videoRecording)
            {
                if (!videoWriter.isOpened())
                {
                    videoFrameSize.width = currentFrame.cols;
                    videoFrameSize.height = currentFrame.rows;
                    if (!videoWriter.open(videoFileName, CV_FOURCC('M', 'J', 'P', 'G'), 30, videoFrameSize))
                    {
                        DAVINCI_LOG_ERROR << "      ERROR: Cannot create the video file for training or replaying!";
                        SetTestComplete();
                        return frame;
                    }
                }
                assert (currentFrame.cols == videoFrameSize.width && currentFrame.rows == videoFrameSize.height);
                timeStampList.push_back(stopWatch->ElapsedMilliseconds());
                videoWriter.write(currentFrame);
            }

            return frame;
        }
    }

    cv::Mat MainTest::GetCurrentFrame()
    {
        SpinLockGuard lock(currentFrameLock);
        return currentFrame;
    }

    bool MainTest::CheckProcessExit()
    {
        bool isExit = false;

        if(dut != nullptr)
        { 
            DAVINCI_LOG_INFO << "Start package name: " << startPackageName;

            string topPackageName = dut->GetTopPackageName();
            if(boost::empty(topPackageName))
            {
                DAVINCI_LOG_WARNING << "Top package name: " << topPackageName;
            }
            else
            {
                DAVINCI_LOG_INFO << "Top package name: " << topPackageName;

                if(boost::empty(topPackageName))
                    return false;

                if(boost::empty(packageName))
                    return false;

                if(topPackageName != packageName 
                    && topPackageName == startPackageName 
                    && !WaiveProcessExit(allMatchedEvents))
                {
                    isExit = true;
                }
            }
        }

        return isExit;
    }

    bool MainTest::NeedCheckProcessExitForTouchAction(ScheduleEvent event)
    {
        boost::shared_ptr<QScript> script = scripts[event.qsName];
        if(scriptPreprocessInfos[event.qsName][event.startIp].isVirtualNavigation)
            return false;

        return true;
    }

    bool MainTest::NeedCheckProcessExitForNonTouchAction(int opcode)
    {
        if(QScript::IsOpcodeNonTouchCheckProcess(opcode))
            return true;
        else
            return false;
    }

    void MainTest::CleanQScriptResources()
    {
        scriptPreprocessInfos.clear();
        scriptReplayers.clear();
        scriptPriorities.clear();
        scriptPriorities.clear();
        scripts.clear();
        builtInResolutions.clear();
        checkers.clear();
    }

    void MainTest::SetTestComplete()
    {
        isScheduleComplete = true;
        SetFinished();
    }

    void MainTest::Destroy()
    {
        if(isScheduleStart)
        {
            if(prepareThread != nullptr)
            {
                prepareThread->join();
                prepareThread = nullptr;
            }
        }
        StopDispatchTimer();

        for(auto scriptReplayer : scriptReplayers)
        {
            scriptReplayer.second->ReleaseBreakPoint();
            scriptReplayer.second->SetFinished();
            scriptReplayer.second->Destroy();
            scriptReplayer.second->DestroyThread();
        }

        if (videoRecording)
        {
            if (videoWriter.isOpened())
            {
                videoWriter.release();
            }
        }
        if(audioSaving)
        {
            boost::lock_guard<boost::mutex> lock(audioMutex);
            if (audioFile != nullptr)
            {
                audioFile = nullptr;
            }
        }

        if(isScheduleStart)
        {
            coverageCollector->SaveOneSeedCoverage();
            coverageCollector->SaveGlobalCoverage();
            GenerateReport();
        }

        CleanQScriptResources();
    }

    int MainTest::FindRandomAvailableRecordTouchIp(vector<int>& unavailableIndexes)
    {
        int recordTouchIpSize = (int)recordTouchSequence.size();
        int eventChooser = -1;
        while(true)
        {
            eventChooser = eventRandomGenerator->GetRandomWithRange(0, recordTouchIpSize - 1);
            if(!ContainVectorElement(unavailableIndexes, eventChooser))
            {
                unavailableIndexes.push_back(eventChooser);
                break;
            }
        }

        return eventChooser;
    }

    std::unordered_map<int, EventType> MainTest::GenerateEventSequence()
    {
        std::unordered_map<int, EventType> eventSeq;
        vector<int> unavailableIndexes;

        for(auto eventPair : events)
        {
            EventType eventType = eventPair.first;
            EventInfo eventInfo = eventPair.second;
            if(eventInfo.enabled)
            {
                for(int i = 0; i < eventInfo.number; i++)
                {
                    int index = FindRandomAvailableRecordTouchIp(unavailableIndexes);
                    if(index != -1)
                    {
                        eventSeq[recordTouchSequence[index]] = eventType;
                    }
                }
            }
        }

        return eventSeq;
    }

    void MainTest::GenerateReport()
    {
        CopyDebugResources();
        mainTestReport->SetTargetDevice(dut);
        mainTestReport->SetEntryQSName(entryQSName);
        mainTestReport->SetLoginResult(CheckLoginResult());

        mainTestReport->SetCrashFrame(crashFrame);
        mainTestReport->SetProcessExit(processExit);
        mainTestReport->SetLaunchFail(launchFail);
        mainTestReport->SetImageCheckPass(imageCheckPass);
        mainTestReport->SetReplayIpSequence(replayIpSequence);
        mainTestReport->SetXmlFolder(xmlFolder);
        mainTestReport->SetXmlFile(xmlFile);
        mainTestReport->GenerateLogcat();
        mainTestReport->GenerateCriticalPathCoverage();
        mainTestReport->GenerateMainTestReport();
        mainTestReport->SetGetRnRQScriptTimeStamp(timeStampList);
        generatedGetRnRQScript->Flush();
        mainTestReport->SetGeneratedGetRnRQScript(generatedGetRnRQScript);
        mainTestReport->PopulateGetRnRQScript();

        bool isFail = mainTestReport->DetectApplicationFailures();
        mainTestReport->ReportRecordCoverge(isFail);
        mainTestReport->GenerateSmokeTestReport();
        testReport->CopyDaVinciLog();
        testReport->CopyDaVinciVideoAndAudio(xmlFile, packageName);
    }

    CaptureDevice::Preset MainTest::CameraPreset()
    {
        return CaptureDevice::Preset::PresetDontCare;
    }

    DaVinciStatus MainTest::StartDispatchTimer()
    {
        dispatchTimer->SetTimerHandler(boost::bind(&MainTest::DispatchHandler, this, _1));
        return dispatchTimer->Start();
    }

    // Execute single QS block [startIp, endIp, timeStampOffset]
    DaVinciStatus MainTest::ExecuteQSStep(boost::shared_ptr<ScriptReplayer> scriptReplayer, unsigned int startIp, unsigned int endIp, double tso)
    {
        assert (scriptReplayer != nullptr);
        DAVINCI_LOG_INFO << "Execute QS under break point ... " << startIp << " " << endIp << " " << tso << endl;
        for(unsigned int i = startIp; i <= endIp; i++)
        {
            AddEventToGetRnRQScript(scriptReplayer->GetQS()->EventAndAction(i));
        }

        scriptReplayer->SetStartQSIp(startIp);
        scriptReplayer->SetEndQSIp(endIp);
        scriptReplayer->SetStartTimeStampOffset(tso);
        scriptReplayer->ReleaseBreakPoint();
        scriptReplayer->WaitBreakPointComplete();

        SetCurrentQSIpPair(scriptReplayer->GetQsName() + ".qs", startIp);

        if(scripts[currentQSIpPair.qsName]->EventAndAction(currentQSIpPair.startIp)->Opcode() == QSEventAndAction::OPCODE_FPS_MEASURE_START)
        {
            needPhysicalCamera = true;
        }
        else if(scripts[currentQSIpPair.qsName]->EventAndAction(currentQSIpPair.startIp)->Opcode() == QSEventAndAction::OPCODE_FPS_MEASURE_STOP)
        {
            needPhysicalCamera = false;
        }

        WaitForNextAction();

        return DaVinciStatusSuccess;
    }

    void MainTest::SetCurrentQSIpPair(string qsName, int startIp)
    {
        currentQSIpPair.qsName = qsName;
        currentQSIpPair.startIp = startIp;
    }

    void MainTest::WaitForNextAction()
    {
        nextQSIpPair = FindNextQSIp(scriptPreprocessInfos, currentQSIpPair);

        double waitTime = 0.0;
        if(nextQSIpPair.qsName == currentQSIpPair.qsName)
        {
            waitTime = MeasureBreakPointWaitTime(scripts, currentQSIpPair.qsName, currentQSIpPair.startIp, nextQSIpPair.startIp);
        }
        waitTime = UpdateWaitTime(waitTime, actionWaitTime);

        // Support customized single action wait time
        std::unordered_map<string, string> keyValueMap = ParseRecordTouchDown(nextQSIpPair);
        string waitKey = "wait";
        double singleWaitTime = 0.0;
        if(keyValueMap.find(waitKey) != keyValueMap.end())
        {
            singleWaitTime = boost::lexical_cast<double>(keyValueMap[waitKey]) * 1000;
        }

        if(FSmaller(0.0, singleWaitTime))
            waitTime = singleWaitTime;

        breakPointWaitEvent.WaitOne((int)(waitTime));
    }

    DaVinciStatus MainTest::WaitBreakPointForVirtualAction(boost::shared_ptr<ScriptReplayer> scriptReplayer, unsigned int startIp, unsigned int endIp)
    {
        assert (scriptReplayer != nullptr);

        currentQSIpPair.qsName = scriptReplayer->GetQsName() + ".qs";
        currentQSIpPair.startIp = startIp;
        nextQSIpPair = FindNextQSIp(scriptPreprocessInfos, currentQSIpPair);

        double waitTime = defaultVirtualActionWaitTime + 0.0;
        if(nextQSIpPair.qsName != unknownQsName)
        {
            if(nextQSIpPair.qsName == currentQSIpPair.qsName)
            {
                waitTime = MeasureBreakPointWaitTime(scripts, currentQSIpPair.qsName, currentQSIpPair.startIp, nextQSIpPair.startIp);
            }
        }
        waitTime = UpdateWaitTime(waitTime);
        breakPointWaitEvent.WaitOne((int)(waitTime));

        return DaVinciStatusSuccess;
    }

    double MainTest::MeasureBreakPointWaitTime(std::unordered_map<string, boost::shared_ptr<QScript>> scripts, string qsName, int currentIp, int nextIp)
    {
        boost::shared_ptr<QScript> qScript = scripts[qsName];
        boost::shared_ptr<QSEventAndAction> qsEventCurrent = qScript->EventAndAction(currentIp);
        boost::shared_ptr<QSEventAndAction> qsEventNext = qScript->EventAndAction(nextIp);
        int currentTimeStamp = (int)qsEventCurrent->TimeStamp();
        int nextTimeStamp = (int)qsEventNext->TimeStamp();
        return (nextTimeStamp - currentTimeStamp + defaultTimeStampDeltaTime);
    }

    double MainTest::MeasureStartAppWaitTime(std::unordered_map<string, boost::shared_ptr<QScript>> scripts, string qsName, int currentQSIp)
    {
        boost::shared_ptr<QScript> qScript = scripts[qsName];
        auto currentEvent = qScript->EventAndAction(currentQSIp);
        auto nextEvent = qScript->EventAndAction(currentQSIp + 1);
        return (nextEvent->TimeStamp() - currentEvent->TimeStamp());
    }

    QSIpPair MainTest::FindNextQSIp(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> info, QSIpPair currentQSIp)
    {
        QSIpPair nextQSIp(unknownQsName, 0, Orientation::Unknown);

        std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>::iterator nextInfoIt;
        for(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>::iterator infoIt = info.begin(); infoIt != info.end(); ++infoIt) 
        {
            if(infoIt->first == currentQSIp.qsName)
            {
                map<int, ScriptHelper::QSPreprocessInfo> qsIp = infoIt->second;
                map<int, ScriptHelper::QSPreprocessInfo>::iterator indexIt;
                bool indexItFound = false;
                for(map<int, ScriptHelper::QSPreprocessInfo>::iterator qsIt = qsIp.begin(); qsIt != qsIp.end(); ++qsIt) 
                {
                    if(qsIt->first == currentQSIp.startIp)
                    {
                        indexIt = qsIt;
                        indexItFound = true;
                        break;
                    }
                }
                if(indexItFound)
                {                  
                    map<int, ScriptHelper::QSPreprocessInfo>::iterator nextQSIt = (++indexIt);
                    if(nextQSIt == qsIp.end())
                    {
                        nextInfoIt = (++infoIt);
                        if(nextInfoIt != info.end())
                        {
                            // Next QS
                            nextQSIp.qsName = nextInfoIt->first;
                            nextQSIp.startIp = nextInfoIt->second.begin()->first;
                        }
                        return nextQSIp;
                    }
                    else
                    {
                        nextQSIp.qsName = currentQSIp.qsName;
                        nextQSIp.startIp = nextQSIt->first;
                        return nextQSIp;
                    }                   
                }
            }
        }

        return nextQSIp;
    }

    QSResolution MainTest::CheckQSResolution(Size frameSize)
    {
        for(std::unordered_map<QSResolution, pair<int, int>>::iterator it = builtInResolutions.begin(); it != builtInResolutions.end(); ++it) 
        {
            pair<int, int> resolution = it->second;
            if(resolution.first * frameSize.height == resolution.second * frameSize.width)
                return it->first;
        }

        return R_16_9;
    }

    bool MainTest::IsSameResolution(Size recordDeviceSize, Size replayDeviceSize)
    {
        // 16:9 is for height:width
        Size recordFrameSize(recordDeviceSize.height, recordDeviceSize.width);
        Size replayFrameSize(replayDeviceSize.height, replayDeviceSize.width);
        QSResolution recordResolution = CheckQSResolution(recordFrameSize);
        QSResolution replayResolution = CheckQSResolution(replayFrameSize);
        if(recordResolution == replayResolution)
            return true;
        else
            return false;
    }

    bool MainTest::IsSameDPI(Size recordDeviceSize, Size replayDeviceSize, double recordDPI, double replayDPI)
    {
        const int allowDPIDiff = 1;
        const double frameWidth = 1280.0;
        int sourceDPI = int(recordDeviceSize.height * replayDPI / frameWidth + 0.5);
        int targetDPI = int(replayDeviceSize.height * recordDPI / frameWidth + 0.5);
        if(abs(sourceDPI - targetDPI) <= allowDPIDiff)
            return true;
        else
            return false;
    }

    bool MainTest::IsAkazePreferResolution(Size recordFrameSize, Size replayFrameSize)
    {
        QSResolution recordResolution = CheckQSResolution(recordFrameSize);
        QSResolution replayResolution = CheckQSResolution(replayFrameSize);
        if(recordResolution == replayResolution)
        {
            return false;
        }
        else
        {
            if(recordResolution == R_4_3 || replayResolution == R_4_3)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    void MainTest::SetBuiltInResolutions()
    {
        builtInResolutions[R_4_3] = pair<int, int>(4, 3);
        builtInResolutions[R_16_9] = pair<int, int>(16, 9);
        builtInResolutions[R_16_10] = pair<int, int>(16, 10);
    }

    QSPriority MainTest::StringToPriority(string& priority)
    {
        boost::to_upper(priority);
        QSPriority qsPriority = QSPriority::QS_MAX;
        if(priority == "LOW")
        {
            qsPriority = QSPriority::QS_LOW;
        }
        else if(priority == "MEDIUM")
        {
            qsPriority = QSPriority::QS_MEDIUM;
        }
        else if(priority == "HIGH")
        {
            qsPriority = QSPriority::QS_HIGH;
        }

        return qsPriority;
    }

    // Find first uncovered schedule event from multiple proposal events
    ScheduleEvent MainTest::FindFirstUncoveredScheduleEvent(vector<ScheduleEvent> allMatchedEvents, vector<ScheduleEvent> lastMatchedEvents)
    {
        for (auto scheduleEvent: lastMatchedEvents)
        {
            if (!ContainMatchedEvent(allMatchedEvents, scheduleEvent) && scheduleEvent.IsForwardEvent(allMatchedEvents.back()))
            {
                return scheduleEvent;
            }
        }

        return emptyEvent;
    }

    // Get the schedule event by coverage
    ScheduleEvent MainTest::GetScheduleEventByCoverage(vector<ScheduleEvent> lastMatchedEvents)
    {
        // Coverage-guided test scheduler based on event info pair <qsName, startIP, endIP)
        if (lastMatchedEvents.empty())
        {
            return emptyEvent;
        }

        ScheduleEvent scheduleEvent;
        if (allMatchedEvents.empty())
        {
            scheduleEvent = lastMatchedEvents[0];
        }
        else
        {
            scheduleEvent = FindFirstUncoveredScheduleEvent(allMatchedEvents, lastMatchedEvents);
            if (emptyEvent.IsSameEvent(scheduleEvent))
            {
                scheduleEvent = lastMatchedEvents[0];
            }
        }

        return scheduleEvent;
    }

    void MainTest::PrepareForDispatchHandler()
    {
        if (DoQSPreprocess() == false)
        {
            DAVINCI_LOG_INFO << "DoQSPreprocess causes SetFinished.";
            SetTestComplete();
            return;
        }

        isScheduleStart = true;
        stopWatch->Start();
    }

    bool MainTest::IsQsIpPairInMatchedEvent(vector<ScheduleEvent> events, QSIpPair qsIpPair)
    {
        for(auto event: events)
        {
            if(event.qsName == qsIpPair.qsName && event.startIp == qsIpPair.startIp)
                return true;
        }

        return false;
    }

    bool MainTest::IsValidTargetQSIpPairCandidatesByDistance(QSIpPair candidate)
    {
        int startIp = 1;
        if(allMatchedEvents.size() > 0)
            startIp = FindLastIpFromMatchedEvents(candidate);
        else 
            startIp = FindFirstTouchDownIpFromScriptInfos(candidate);

        if(!IsValidCandidateDistance(candidate, startIp))
            return false;
        else
            return true;
    }

    bool MainTest::IsValidTargetQSIpPairCandidatesByWindow(ScriptHelper::QSPreprocessInfo qsInfo, vector<string>& replayWindows, string replayFocusWindow, bool isPopUpWindow, bool isSnipScript)
    {
        if(isSnipScript)
        {
            if(qsInfo.isPopUpWindow == isPopUpWindow)
                return true;
            else
                return false;
        }
        else if(qsInfo.windows.size() == 0)
        {
            return true; // legacy script
        }
        else
        {
            // For system pop up (current focus window), sometimes recording does not capture system popup, however replay does - waive such scenario from match system popup
            if(DetectPopUpTypeByTargetWindows(qsInfo.windows, replayWindows, qsInfo.focusWindow, replayFocusWindow) == PopUpType_Same)
                return true;
            else
                return false;
        }
    }

    bool MainTest::HasNonTouchFirstCandidate(vector<QSIpPair>& candidates, QSIpPair& nonTouchCandidate)
    {
        if(candidates.size() == 0)
            return false;

        // For Get-RnR mode with single script or plain RnR mode with single script
        if(scriptPreprocessInfos.size() == 1) 
        {
            QSIpPair candidate = candidates[0];
            if(!scriptPreprocessInfos[candidate.qsName][candidate.startIp].isTouchAction)
            {
                nonTouchCandidate = candidate;
                return true;
            }
        }
        else
        {
            // For Get-RnR mode with multiple scripts
            if(entryStartApp)
            {
                QSIpPair candidate = candidates[0];
                if(!scriptPreprocessInfos[candidate.qsName][candidate.startIp].isTouchAction) // For script with high priority, non-touch opcodes follow with touch opcodes
                {
                    nonTouchCandidate = candidate;
                    return true;
                }
            }
            else
            {
                for(auto candidate : candidates)
                {
                    if(candidate.qsName == entryQSName) // Execute UNINSTALL/INSTALL/START opcodes from entry script
                    {
                        boost::shared_ptr<QScript> script = scripts[candidate.qsName];
                        int opcode = script->EventAndAction(candidate.startIp)->Opcode();
                        if(QScript::IsOpcodeNonTouchBeforeAppStart(opcode))
                        {
                            if(opcode== QSEventAndAction::OPCODE_START_APP)
                            {
                                entryStartApp = true;
                            }
                            nonTouchCandidate = candidate;
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    vector<QSIpPair> MainTest::FindTargetQSIpPairCandidatesByDistance(vector<QSIpPair>& candidates)
    {
        vector<QSIpPair> validCandidates;

        for(auto pair : candidates)
        {
            if(pair.qsName != currentQSIpPair.qsName)
            {
                validCandidates.push_back(pair);
            }
            else
            {
                if(IsValidTargetQSIpPairCandidatesByDistance(pair))
                {
                    validCandidates.push_back(pair);
                }
                else
                {
                    DAVINCI_LOG_INFO << "Skip invalid IP candidate due to far distance (" << pair.qsName << " , " << boost::lexical_cast<string>(pair.startIp) << ")";
                }
            }
        }

        return validCandidates;
    }

    vector<QSIpPair> MainTest::FindTargetQSIpPairCandidatesByPriority(QSPriority priority, vector<string>& replayTargetWindows, string replayFocusWindow, bool isPopUpWindow, bool fallThrough)
    {
        vector<QSIpPair> targetQSIpPairCandidates;
        int lookForwardNum = 0; // 2 for entry QS and 1 for snippet QS
        bool isSnipScript = false;
        for (std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>::const_iterator infoIt = scriptPreprocessInfos.begin(); infoIt != scriptPreprocessInfos.end(); ++infoIt)
        {
            string qsName = infoIt->first;
            if(scriptPriorities[qsName] != priority)
                continue;

            if(qsName == entryQSName)
            {
                lookForwardNum = 2;
            }
            else
            {
                lookForwardNum = 1;
                isSnipScript = true;
            }
            map<int, ScriptHelper::QSPreprocessInfo> ips = infoIt->second;
            for (map<int, ScriptHelper::QSPreprocessInfo>::const_iterator ipIt = ips.begin(); ipIt != ips.end(); ++ipIt)
            {
                int qsIp = ipIt->first;
                ScriptHelper::QSPreprocessInfo qsInfo = ipIt->second;
                QSIpPair qsIpPair(qsName, qsIp, scripts[qsName]->EventAndAction(qsIp)->GetOrientation());

                if (!IsQsIpPairInMatchedEvent(allMatchedEvents, qsIpPair))
                {
                    if (lookForwardNum > 0)
                    {
                        if(isSnipScript)
                        {
                            lookForwardNum--;
                        }
                        if(fallThrough || !qsInfo.isTouchAction)
                        {
                            targetQSIpPairCandidates.push_back(qsIpPair);
                            break;
                        }
                        else if(qsInfo.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Command 
                            || QScript::IsOpcodeVirtualNavigation(qsInfo.navigationEvent)
                            || IsValidTargetQSIpPairCandidatesByWindow(qsInfo, replayTargetWindows, replayFocusWindow, isPopUpWindow, isSnipScript))
                        {
                            targetQSIpPairCandidates.push_back(qsIpPair);
                            if(!qsInfo.isPopUpWindow && !qsInfo.isAdvertisement) // Allow more candidates for pop up and advertisement
                                lookForwardNum--;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        return targetQSIpPairCandidates;
    }

    vector<QSIpPair> MainTest::FindTargetQSIpPairCandidatesFromAllScripts(vector<string>& replayTargetWindows, string replayFocusWindow, bool isPopUpWindow, bool fallThrough)
    {
        vector<QSIpPair> targetCandidates;
        for(QSPriority priority = QSPriority::QS_HIGH; priority < QSPriority::QS_MAX;)
        {
            vector<QSIpPair> candidates = FindTargetQSIpPairCandidatesByPriority(priority,  replayTargetWindows, replayFocusWindow, isPopUpWindow, fallThrough);
            priority = QSPriority(priority + 1);
            AddVectorToVector(targetCandidates, candidates);
        }

        return targetCandidates;
    }

    bool MainTest::IsContinousNonMatchObjects(int maxNumber)
    {
        int replayActionSize =  (int)replayIpSequence.size();
        int nonMatchNumber = 0;
        bool exceedMaxNumber = false;
        for(int i = replayActionSize - 1; i >= 0; i--)
        {
            QSIpPair pair = replayIpSequence[i];
            if(pair.qsName == unknownQsName)
            {
                nonMatchNumber++;
                if(nonMatchNumber >= maxNumber)
                {
                    exceedMaxNumber = true;
                    break;
                }
            }
            else
            {
                nonMatchNumber = 0;
            }
        }

        return exceedMaxNumber;
    }

    bool MainTest::IsSameWindowPackage(string recordWindow, string replayWindow)
    {
        vector<string> recordItems;
        boost::algorithm::split(recordItems, recordWindow, boost::algorithm::is_any_of("/"));

        vector<string> replayItems;
        boost::algorithm::split(replayItems, replayWindow, boost::algorithm::is_any_of("/"));

        int recordItemSize = (int)recordItems.size();
        int replayItemSize = (int)replayItems.size();

        if(recordItemSize == replayItemSize)
        {
            if(recordItemSize == 2)
            {
                if(recordItems[0] == replayItems[0])
                {
                    return true;
                }
            }
            else if(recordWindow == replayWindow)
            {
                return true;
            }
        }
        else
        {
            string recordWindowPrefix = recordWindow;
            string replayWindowPrefix = replayWindow;
            if(recordItemSize == 2)
            {
                recordWindowPrefix = recordItems[0];
            }
            if(replayItemSize == 2)
            {
                replayWindowPrefix = replayItems[0];
            }
            if(recordWindowPrefix == replayWindowPrefix)
            {
                return true;
            }
        }

        return false;
    }

    QSPopUpType MainTest::DetectPopUpTypeByTargetWindows(vector<string>& recordWindows, vector<string>& replayWindows, string recordFocusWindow, string replayFocusWindow)
    {
        // By pass the empty record focus window or small replay pop up window 
        if(boost::empty(recordFocusWindow) || boost::empty(replayFocusWindow) || boost::equals(replayFocusWindow, "PopupWindow") )
        {
            return QSPopUpType::PopUpType_Same;
        }

        if(!IsSameWindowPackage(recordFocusWindow, replayFocusWindow))
        {
            return QSPopUpType::PopUpType_Replay_Only;
        }

        for(auto replayWindow: replayWindows)
        {
            bool recordExist = false;
            for(auto recordWindow: recordWindows)
            {
                if(IsSameWindowPackage(recordWindow, replayWindow))
                {
                    recordExist = true;
                    break;
                }
            }

            if(!recordExist)
            {
                return QSPopUpType::PopUpType_Replay_Only;
            }
        }

        return QSPopUpType::PopUpType_Same;
    }

    vector<string> MainTest::GetTargetWindows(vector<string>& windowLines, string packageName, string& focusWindow)
    {
        std::unordered_map<string, string> map = AndroidTargetDevice::GetWindows(windowLines, focusWindow);

        vector<string> windows;

        for(auto pair : map)
        {
            if(boost::starts_with(pair.second, packageName))
                windows.push_back(pair.second);
        }

        return windows;
    }

    QSPopUpType MainTest::DetectPopUpType(vector<string>& recordLines, vector<string>& replayLines, string packageName)
    {
        if(boost::empty(packageName))
            return QSPopUpType::PopUpType_Unknown;

        string recordFocusWindow, replayFocusWindow;
        vector<string> recordWindows = GetTargetWindows(recordLines, packageName, recordFocusWindow);
        vector<string> replayWindows = GetTargetWindows(replayLines, packageName, replayFocusWindow);

        return DetectPopUpTypeByTargetWindows(recordWindows, replayWindows, recordFocusWindow, replayFocusWindow);
    }

    void MainTest::DispatchHandler(const HighResolutionTimer &timer)
    {
        if (!IsFinished())
        {
            if (!isScheduleStart || isScheduleComplete)
            {
                return;
            }

            if(stopWatch->ElapsedMilliseconds() >= timeout)
            {
                DAVINCI_LOG_INFO << "Replay timeout - stop test";
                stopWatch->Stop();
                SetTestComplete();
                return;
            }

            if(IsContinousNonMatchObjects())
            {
                DAVINCI_LOG_INFO << "Continous random actions - stop test";
                stopWatch->Stop();
                SetTestComplete();
                return;
            }


            if(IsAllScriptsCompleted(scriptPreprocessInfos, currentQSIpPair))
            {
                DAVINCI_LOG_INFO << "Complete all record actions - stop test";
                stopWatch->Stop();
                SetTestComplete();
                return;
            }

            Mat frame = GetCurrentFrame();

            if (frame.empty())
            {
                return;
            }

            if(needPhysicalCamera)
            {
                vector<string> emptyLines;
                vector<QSIpPair> targetQSIpPairs = FindTargetQSIpPairCandidatesFromAllScripts(emptyLines, "", isPopUpWindow, true);
                vector<QSIpPair> entryQSIpPairs;
                for(auto pair : targetQSIpPairs)
                {
                    if(pair.qsName == entryQSName)
                    {
                        entryQSIpPairs.push_back(pair);
                    }
                }
                assert(entryQSIpPairs.size() > 0);
                QSIpPair qsIpPair = entryQSIpPairs[0];
                ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[entryQSName][qsIpPair.startIp];
                ScheduleEvent scheduleEvent;
                scheduleEvent.qsName = entryQSName;
                scheduleEvent.startIp = qsIpPair.startIp;
                scheduleEvent.endIp = info.endIp;
                scheduleEvent.qsPriority = scriptPriorities[entryQSName];

                allMatchedEvents.push_back(scheduleEvent);

                DAVINCI_LOG_INFO << "Replay directly @" << qsIpPair.qsName << " "  << qsIpPair.startIp << endl;
                DispatchDirectRecordAction(scheduleEvent, frame);
            }
            else
            {
                DAVINCI_LOG_INFO << "Replay by object ... ";

                boost::filesystem::path resourcePath = replayFolder;
                resourcePath /= boost::filesystem::path("replay_reference.png");
                SaveImage(frame, resourcePath.string());

                replayFrameSize = frame.size();
                Mat rotatedFrame = RotateFrameUp(frame);
                resourcePath = replayFolder;
                resourcePath /= boost::filesystem::path("replay_reference_rotated.png");
                SaveImage(rotatedFrame, resourcePath.string());

                assert(dut != nullptr);
                viewHierarchyLines = dut->GetTopActivityViewHierarchy();
                Orientation orientation = dut->GetCurrentOrientation(true);

                androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                assert(androidDut != nullptr);
                windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
                replayKeyboardPopUp = windowBarInfo.inputmethodExist;

                // Workaround for app isssue that focus window rect is empty
                if(windowBarInfo.focusedWindowRect.width == 0 || windowBarInfo.focusedWindowRect.height == 0)
                {
                    windowBarInfo.focusedWindowRect.x = windowBarInfo.statusbarRect.x;
                    windowBarInfo.focusedWindowRect.y = windowBarInfo.statusbarRect.y + windowBarInfo.statusbarRect.height;
                    if (orientation == Orientation::Portrait || orientation == Orientation::ReversePortrait)
                    {
                        windowBarInfo.focusedWindowRect.width = replayDeviceSize.width;
                        windowBarInfo.focusedWindowRect.height = replayDeviceSize.height - windowBarInfo.focusedWindowRect.y - windowBarInfo.navigationbarRect.height;
                    }
                    else
                    {
                        windowBarInfo.focusedWindowRect.width = replayDeviceSize.height;
                        windowBarInfo.focusedWindowRect.height = replayDeviceSize.width - windowBarInfo.focusedWindowRect.y - windowBarInfo.navigationbarRect.height;
                    }
                }

                isPopUpWindow = ObjectUtil::Instance().IsPopUpDialog(replayDeviceSize, orientation, windowBarInfo.statusbarRect, windowBarInfo.focusedWindowRect);
                // Keyboard will make the focus window looks like a pop up
                if(windowBarInfo.inputmethodExist)
                    isPopUpWindow = false;

                isPopUpWindow = isPopUpWindow || windowBarInfo.isPopUpWindow;
                if(isPopUpWindow)
                    DAVINCI_LOG_INFO << "Current window is pop up (" 
                    << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.x) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.y) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height) << ")" << endl;
                else
                    DAVINCI_LOG_INFO << "Current window is not pop up (" 
                    << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.x) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.y) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width) 
                    << "," << boost::lexical_cast<string>(windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height) << ")" << endl;

                Point fwPoint(windowBarInfo.focusedWindowRect.x, windowBarInfo.focusedWindowRect.y);
                Rect replayFrameMainROI = AndroidTargetDevice::GetWindowRectangle(fwPoint, windowBarInfo.focusedWindowRect.width, windowBarInfo.focusedWindowRect.height, replayDeviceSize, orientation, replayFrameSize.width);

                if(orientation == Orientation::Portrait || orientation == Orientation::ReversePortrait)
                    replayFrameMainROIWithBar = AndroidTargetDevice::GetWindowRectangle(fwPoint, windowBarInfo.focusedWindowRect.width, replayDeviceSize.height - windowBarInfo.focusedWindowRect.y, replayDeviceSize, orientation, replayFrameSize.width);
                else
                    replayFrameMainROIWithBar = AndroidTargetDevice::GetWindowRectangle(fwPoint, windowBarInfo.focusedWindowRect.width, replayDeviceSize.width - windowBarInfo.focusedWindowRect.y, replayDeviceSize, orientation, replayFrameSize.width);

                if(windowBarInfo.navigationbarRect != EmptyRect)
                    replayHasVirtualBar = true;

                Size replayMainSize = replayFrameMainROI.size();
                Mat replayMainFrame;

                if(!ContainRect(Rect(EmptyPoint, frame.size()), replayFrameMainROI))
                {
                    DAVINCI_LOG_INFO << "Invalid replay main ROI, please check resolution.";
                    stopWatch->Stop();
                    SetTestComplete();
                    return;
                }

                ObjectUtil::Instance().CopyROIFrame(frame, replayFrameMainROI, replayMainFrame);
                Mat rotatedReplayMainFrame = RotateFrameUp(replayMainFrame);
#ifdef _DEBUG
                SaveImage(rotatedReplayMainFrame, "rotatedReplayMainFrame.png");
#endif

                vector<string> replayTargetWindows = GetTargetWindows(windowLines, packageName, currentReplayFocusWindow);
                if(boost::empty(currentReplayFocusWindow))
                {
                    const int exceptionalWindowLength = 1;
                    if(windowLines.size() == exceptionalWindowLength)
                    {
                        DAVINCI_LOG_WARNING << "System hang with window dump info: " << windowLines[0];
                    }
                }
                else
                {
                    DAVINCI_LOG_INFO << "Current focus window: " << currentReplayFocusWindow;
                }

                DAVINCI_LOG_INFO << "Current QS name: " << currentQSIpPair.qsName << " with IP " << currentQSIpPair.startIp;
                targetQSIpPairs = FindTargetQSIpPairCandidatesFromAllScripts(replayTargetWindows, currentReplayFocusWindow, isPopUpWindow);
#ifdef _DEBUG
                for(auto pair: targetQSIpPairs)
                {
                    DAVINCI_LOG_INFO << "QS candidate name: " << pair.qsName << " with IP " << pair.startIp;
                }
#endif
                targetQSIpPairs = FindTargetQSIpPairCandidatesByDistance(targetQSIpPairs);
#ifdef _DEBUG
                for(auto pair: targetQSIpPairs)
                {
                    DAVINCI_LOG_INFO << "Valid QS candidate name: " << pair.qsName << " with IP " << pair.startIp;
                }
#endif
                isSystemPopUpWindow = MatchSystemPopUpWindow(currentReplayFocusWindow);
                if(isSystemPopUpWindow)
                    targetQSIpPairs.clear();

                QSIpPair nonTouchCandidate;
                if(HasNonTouchFirstCandidate(targetQSIpPairs, nonTouchCandidate))
                {
                    ScheduleEvent scheduleEvent;
                    scheduleEvent.qsName = nonTouchCandidate.qsName;
                    scheduleEvent.startIp = nonTouchCandidate.startIp;
                    scheduleEvent.endIp = nonTouchCandidate.startIp;
                    DispatchNonTouchAction(scheduleEvent, frame, rotatedFrame);
                }
                else
                {
                    vector<Point> points;
                    ScheduleEventMatchType matchType = ScheduleEventMatchType::MatchType_Random;
                    vector<string> idCandidates;
                    bool objectMatched = false;
                    QSIpPair matchedPair;
                    for(auto pair: targetQSIpPairs)
                    {
                        std::unordered_map<string, string> keyValueMap = ParseRecordTouchDown(pair);
                        currentMatchPattern = ScriptHelper::FindAdditionalInfo(keyValueMap, "match");

                        if(boost::empty(currentMatchPattern))
                        {
                            // For script snipt, we should use local match pattern
                            if(pair.qsName != entryQSName)
                            {
                                currentMatchPattern = "";
                            }
                            else
                            {
                                currentMatchPattern = matchPattern;
                            }
                        }
                        else
                        {
                            DAVINCI_LOG_INFO << "Switch to local match pattern from " << matchPattern << " to " << currentMatchPattern;
                        }

                        string unmatchRatioStr = ScriptHelper::FindAdditionalInfo(keyValueMap, "unmatchratio");
                        if(boost::empty(unmatchRatioStr))
                        {
                            currentFrameUnmatchRatioThreshold = frameUnmatchRatioThreshold;
                        }
                        else
                        {
                            currentFrameUnmatchRatioThreshold = boost::lexical_cast<double>(unmatchRatioStr);
                        }

                        string idValue = ScriptHelper::FindAdditionalInfo(keyValueMap, "id");
                        if(!boost::empty(idValue))
                        {
                            if(!ContainVectorElement(idCandidates, pair.qsName))
                            {
                                idCandidates.push_back(pair.qsName);
                                currentMatchPattern = currentMatchPattern + "+id";
                            }
                        }

                        if(scriptPreprocessInfos[pair.qsName][pair.startIp].isAdvertisement)
                        {
                            currentMatchPattern = currentMatchPattern + "+cross";
                        }

                        DAVINCI_LOG_INFO << "Current local match pattern: " << currentMatchPattern;

                        // Use currrent script device information in case snip script is recoreded from target device
                        boost::shared_ptr<QScript> currentScript = scripts[pair.qsName];
                        string currentRecordDeviceWidth = currentScript->GetResolutionWidth();
                        string currentRecordDeviceHeight = currentScript->GetResolutionHeight();
                        TryParse(currentRecordDeviceWidth, recordDeviceSize.width);
                        TryParse(currentRecordDeviceHeight, recordDeviceSize.height);
                        string currentRecordDPIStr = currentScript->GetDPI();
                        TryParse(currentRecordDPIStr, recordDPI);

                        // Set globla stretch mode when device size and DPI is consistent
                        if(UseStretch(currentMatchPattern))
                        {
                            if(IsSameResolution(recordDeviceSize, replayDeviceSize) && IsSameDPI(recordDeviceSize, replayDeviceSize, recordDPI, replayDPI))
                            {
                                globalStretchMode = "direct";
                            }
                        }

                        if(MatchForObjectAction(pair, keyValueMap, frame, rotatedFrame, replayMainFrame, rotatedReplayMainFrame, orientation,  replayFrameMainROI, points, matchType))
                        {
                            matchedPair = pair;
                            objectMatched = true;
                            break;
                        }
                    }

                    if(!objectMatched)
                    {
                        MatchForRandomAction(rotatedReplayMainFrame, points, matchType);
                    }

                    DispatchAction(matchType, points, frame, rotatedFrame, replayMainFrame, rotatedReplayMainFrame, orientation, replayFrameMainROI);

                    if(objectMatched && boost::equals(matchedPair.qsName, entryQSName))
                    {
                        DispatchEvent(matchedPair);
                    }
                }

                lastMatchedEvents.clear();
            }
        }
    }

    void MainTest::SaveEventFrame()
    {
        Mat frame = GetCurrentFrame();
        assert(dut != nullptr);
        Orientation orientation = dut->GetCurrentOrientation(true);

        assert(androidDut != nullptr);
        AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
        Point fwPoint(windowBarInfo.focusedWindowRect.x, windowBarInfo.focusedWindowRect.y);
        Rect replayFrameMainROI = AndroidTargetDevice::GetWindowRectangle(fwPoint, windowBarInfo.focusedWindowRect.width, windowBarInfo.focusedWindowRect.height, replayDeviceSize, orientation, replayFrameSize.width);

        Mat replayMainFrame;

        if(!ContainRect(Rect(EmptyPoint, frame.size()), replayFrameMainROI))
        {
            DAVINCI_LOG_INFO << "Invalid replay main ROI, please check resolution.";
            stopWatch->Stop();
            SetTestComplete();
            return;
        }

        ObjectUtil::Instance().CopyROIFrame(frame, replayFrameMainROI, replayMainFrame);

        replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, orientation, true, false));
        mainTestReport->SaveRandomReplayFrame(frame, randomIndex);
        mainTestReport->SaveRandomMainReplayFrame(replayMainFrame, randomIndex);
        mainTestReport->SaveRandomReplayFrame(frame, randomIndex, true);
        randomIndex++;
    }

    void MainTest::DispatchEvent(QSIpPair pair)
    {
        if(MapContainKey(pair.startIp, eventSequence))
        {
            EventType eventType = eventSequence[pair.startIp];
            switch(eventType)
            {
            case EventType::EventMenuSwitch:
            case EventType::EventAppSwitch:
                {
                    eventDispatcher->SendEvent(eventType);
                    SaveEventFrame();
                    eventDispatcher->RecoverEvent(eventType);
                    SaveEventFrame();
                    break;
                }
            case EventType::EventRelaunch:
                {
                    assert(androidDut != nullptr);
                    std::vector<std::string> windowLines = androidDut->GetWindowBarLines();
                    string focusWindow;
                    AndroidTargetDevice::GetWindows(windowLines, focusWindow);
                    eventDispatcher->SendEvent(eventType);
                    SaveEventFrame();
                    eventDispatcher->SetLaunchableActivity(focusWindow);
                    eventDispatcher->RecoverEvent(eventType);
                    SaveEventFrame();
                    break;
                }
            default:
                {
                    DAVINCI_LOG_WARNING << "Unexpected event type: should not reach here.";
                    break;
                }
            }
        }
    }

    bool MainTest::MatchForObjectAction(QSIpPair pair, std::unordered_map<string, string> keyValueMap, Mat frame, Mat rotatedFrame, Mat replayMainFrame, Mat rotatedReplayMainFrame, Orientation orientation, Rect replayFrameMainROI, vector<Point>& points, ScheduleEventMatchType& matchType)
    {
        if(MatchIdObject(pair, keyValueMap, viewHierarchyLines, orientation, windowBarInfo.focusedWindowRect, replayFrameMainROI))
        {
            matchType = ScheduleEventMatchType::MatchType_ID; 
            return true;
        }

        if(MatchVirtualObject(pair))
        {
            matchType = ScheduleEventMatchType::MatchType_Virtual; 
            return true;
        }

        points.clear();
        if(MatchPopUpObject(rotatedReplayMainFrame, points))
        {
            matchType = ScheduleEventMatchType::MatchType_RecordPopUp;
            return true;
        }

        if(MatchCrossObject(rotatedReplayMainFrame, points))
        {
            matchType = ScheduleEventMatchType::MatchType_RecordCross;
            return true;
        }

        if(MatchTextObject(pair, rotatedReplayMainFrame, rotatedFrame.size(), replayFrameMainROI))
        {
            matchType = ScheduleEventMatchType::MatchType_Text; 
            return true;
        }

        if(MatchShapeObject(pair, replayMainFrame, replayFrameMainROI))
        {
            matchType = ScheduleEventMatchType::MatchType_Shape; 
            return true;
        }

        if(MatchAkazeObject(pair, replayMainFrame, replayFrameMainROI))
        {
            matchType = ScheduleEventMatchType::MatchType_Akaze; 
            return true;
        }

        if(MatchDPIObject(pair, orientation))
        {
            matchType = ScheduleEventMatchType::MatchType_DPI; 
            return true;
        }

        if(MatchStretchObject(pair, keyValueMap, replayFrameMainROI))
        {
            matchType = ScheduleEventMatchType::MatchType_Stretch; 
            return true;
        }

        return false;
    }

    void MainTest::MatchForRandomAction(Mat rotatedReplayMainFrame, vector<Point>& points, ScheduleEventMatchType& matchType)
    {
        if(MatchKeyboardPopUp(replayKeyboardPopUp))
        {
            matchType = ScheduleEventMatchType::MatchType_KeyboardPopUp;
            return;
        }

        if(MatchSystemFocusWindow(currentReplayFocusWindow))
        {
            matchType = ScheduleEventMatchType::MatchType_SystemPopUp;
            return;
        }

        if(MatchPopUpObject(rotatedReplayMainFrame, points, true))
        {
            matchType = ScheduleEventMatchType::MatchType_RandomPopUp;
            return;
        }

        if(MatchCrossObject(rotatedReplayMainFrame, points, true))
        {
            matchType = ScheduleEventMatchType::MatchType_RandomCross;
            return;
        }
    }

    void MainTest::DispatchAction(ScheduleEventMatchType matchType, vector<Point> points, Mat frame, Mat rotatedFrame, Mat replayMainFrame, Mat rotatedReplayMainFrame, Orientation orientation, Rect replayFrameMainROI)
    {
        ScheduleEvent scheduleEvent = GetScheduleEventByCoverage(lastMatchedEvents);

        std::vector<std::string> newWindowLines = androidDut->GetWindowBarLines();
        if(AndroidTargetDevice::IsWindowChanged(windowLines, newWindowLines))
        {
            DAVINCI_LOG_WARNING << "Windows are changed - skip current action.";
            return;
        }

        switch(matchType)
        {
        case ScheduleEventMatchType::MatchType_ID:
            {
                DAVINCI_LOG_INFO << "Matcheded by ID @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchIdRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;

                break;
            }
        case ScheduleEventMatchType::MatchType_Virtual:
            {
                DAVINCI_LOG_INFO << "Matcheded by Virtual @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchVirtualAction(scheduleEvent, frame, replayDeviceSize, windowBarInfo.navigationbarRect, windowBarInfo.statusbarRect, windowBarInfo.focusedWindowRect);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;

                break;
            }
        case ScheduleEventMatchType::MatchType_Stretch:
            {
                DAVINCI_LOG_INFO << "Matcheded by Stretch @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchIdRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;
                break;
            }
        case ScheduleEventMatchType::MatchType_DPI:
            {
                DAVINCI_LOG_INFO << "Matcheded by DPI @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchIdRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;
                break;
            }
        case ScheduleEventMatchType::MatchType_Text:
            {
                DAVINCI_LOG_INFO << "Matcheded by Text @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchObjectRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;

                break;
            }
        case ScheduleEventMatchType::MatchType_Shape:
            {
                DAVINCI_LOG_INFO << "Matcheded by Shape @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchObjectRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;
                break;
            }
        case ScheduleEventMatchType::MatchType_Akaze:
            {
                DAVINCI_LOG_INFO << "Matcheded by Akaze @" << scheduleEvent.qsName << " "  << scheduleEvent.startIp << endl;
                allMatchedEvents.push_back(scheduleEvent);
                MarkEventsBeforeMatchEvent(targetQSIpPairs, scheduleEvent);

                DispatchObjectRecordAction(scheduleEvent, frame);

                for (auto event : lastMatchedEvents)
                {
                    boost::filesystem::path resourcePath = replayFolder;
                    resourcePath /= boost::filesystem::path("matched_" + boost::lexical_cast<string>(matchIndex) + "_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + ".png");
                    Mat matchedFrame = mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
                    SaveImage(matchedFrame, resourcePath.string());
                }

                ++matchIndex;
                break;
            }
        case ScheduleEventMatchType::MatchType_RecordPopUp:
        case ScheduleEventMatchType::MatchType_RandomPopUp:
            {
                DispatchPopUpAction(frame, replayMainFrame, rotatedReplayMainFrame, orientation, points, replayFrameMainROI, scheduleEvent);
                if(popupFailure)
                {
                    DAVINCI_LOG_INFO << "Detect error dialog - stop test";
                    SetTestComplete();
                }
                break;
            }
        case ScheduleEventMatchType::MatchType_RecordCross:
        case ScheduleEventMatchType::MatchType_RandomCross:
            {
                DispatchCrossAction(frame, replayMainFrame, rotatedReplayMainFrame, orientation, points[0], replayFrameMainROI, scheduleEvent);
                break;
            }
        case ScheduleEventMatchType::MatchType_KeyboardPopUp:
        case ScheduleEventMatchType::MatchType_SystemPopUp:
            {
                DispatchWindowCloseAction(frame, replayMainFrame, orientation);
                break;
            }
        case ScheduleEventMatchType::MatchType_Random:
            {
                DispatchRandomAction(frame, rotatedFrame, replayMainFrame, rotatedReplayMainFrame, orientation);
                break;
            }
        default:
            {
                DAVINCI_LOG_INFO << "Should not reach here: unexpected match type";
                break;
            }
        }
    }

    // TODO: enhance the strategy to evaluate whether one sript completes its critical path exploration
    // FIXME: it is difficult to set the unmatch ratio for all cases (the first four pics in beautyPlus1 and the first one pic in beautyPlus2) 
    bool MainTest::IsScriptCompleted(string qsName, map<int, ScriptHelper::QSPreprocessInfo> qsInfos)
    {
        for(map<int, ScriptHelper::QSPreprocessInfo>::const_iterator infoIt = qsInfos.begin(); infoIt != qsInfos.end(); ++infoIt) 
        {
            if(!IsSingleIpCompleted(qsName, infoIt->first))
                return false;
        }

        return true;
    }

    bool MainTest::IsAllScriptsCompleted(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> qsInfos, QSIpPair currentQSIpPair)
    {
        boost::shared_ptr<QSEventAndAction> qsEvent = scripts[currentQSIpPair.qsName]->EventAndAction(currentQSIpPair.startIp);
        if(currentQSIpPair.qsName == entryQSName && qsEvent->Opcode() == QSEventAndAction::OPCODE_EXIT)
            return true;

        for(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>::const_iterator infoIt = qsInfos.begin(); infoIt != qsInfos.end(); ++infoIt) 
        {
            string qsName = infoIt->first;
            map<int, ScriptHelper::QSPreprocessInfo> infos = infoIt->second;
            if(!IsScriptCompleted(qsName, infos))
                return false;
        }

        return true;
    }

    bool MainTest::IsSingleIpCompleted(string qsName, int ip)
    {
        for(auto event: allMatchedEvents)
        {
            if(event.qsName == qsName && event.startIp == ip)
                return true;
        }

        return false;
    }

    bool MainTest::ContainMatchedEvent(vector<ScheduleEvent> events, ScheduleEvent event)
    {
        for (auto e: events)
        {
            if (e.IsSameEvent(event))
            {
                return true;
            }
        }

        return false;
    }

    vector<Point> MainTest::MatchByText(ScriptHelper::QSPreprocessInfo info, Size rotatedFrameSize, Rect replayFrameMainROI, Mat rotatedReplayMainFrame)
    {
        vector<Point> targetPoints;

        if (boost::empty(info.clickedEnText) && boost::empty(info.clickedZhText))
        {
            return targetPoints;
        }

        Orientation orientation = Orientation::Portrait;
        if(dut != nullptr)
        {
            orientation = dut->GetCurrentOrientation();
        }

        Point rotatedPoint = TransformFramePointToRotatedFramePoint(rotatedFrameSize, info.framePoint, orientation);
        Rect textROI;
        int deltaYRatio = 2;
        if(!isPopUpWindow)
        {
            Rect frameRect(EmptyPoint, rotatedReplayMainFrame.size());
            textROI = Rect(0, rotatedPoint.y - rotatedReplayMainFrame.rows / deltaYRatio, rotatedReplayMainFrame.cols, rotatedReplayMainFrame.rows / deltaYRatio);
            textROI &= frameRect;

            if(textROI == EmptyRect)
            {
                textROI = Rect(EmptyPoint, rotatedReplayMainFrame.size());
            }
        }
        else
        {
            textROI = Rect(EmptyPoint, rotatedReplayMainFrame.size());
            DebugSaveImage(rotatedReplayMainFrame, ConcatPath(replayFolder.string(), "debug_texts_original_frame.png"));
        }

        vector<Rect> textRects = TextUtil::Instance().ExtractSceneTexts(rotatedReplayMainFrame, textROI);
        cv::Mat debugFrame = rotatedReplayMainFrame.clone();
        vector<Rect> initialMatchTextRects;
        for (auto textRect : textRects)
        {
            textRect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(textRect, rotatedReplayMainFrame.size(), 2, 2);
            DrawRect(debugFrame, textRect, Yellow);
            initialMatchTextRects.push_back(textRect);
        }

        DebugSaveImage(debugFrame, ConcatPath(replayFolder.string(), "debug_texts_frame.png"));

        int minHammDistance = INT_MAX;
        string language = "en";
        for (size_t index = 0; index < initialMatchTextRects.size(); index++)
        {
            Rect textRect = initialMatchTextRects[index];
            double widthRatio = (double)textRect.width / info.clickedTextRect.width;
            double heightRatio = (double)textRect.height / info.clickedTextRect.height;
            if (widthRatio >= 3 || widthRatio <= 0.3 || heightRatio >= 3 || heightRatio <= 0.3)
            {
                continue;
            }

            Mat clickedTextRoiFrame;
            ObjectUtil::Instance().CopyROIFrame(rotatedReplayMainFrame, textRect, clickedTextRoiFrame);

            DebugSaveImage(clickedTextRoiFrame, ConcatPath(replayFolder.string(), "debug_text_frame_" + boost::lexical_cast<string>(index) + ".png"));

            string recognizedText = "";
            string recordText = "";
            if (!boost::empty(info.clickedEnText))
            {
                recognizedText = TextUtil::Instance().RecognizeEnglishText(clickedTextRoiFrame);
                recognizedText = TextUtil::Instance().PostProcessText(recognizedText);
                if (!TextUtil::Instance().IsValidEnglishText(recognizedText))
                {
                    continue;
                }

                recordText = info.clickedEnText;
                language = "en";
            }
            else if (!boost::empty(info.clickedZhText))
            {
                recognizedText = TextUtil::Instance().RecognizeChineseText(clickedTextRoiFrame);
                recognizedText = TextUtil::Instance().PostProcessText(recognizedText);
                if (!TextUtil::Instance().IsValidChineseText(recognizedText))
                {
                    continue;
                }

                recordText = info.clickedZhText;
                language = "zh";
            }

            int hammDistance;
            vector<string> splittedTexts;
            boost::algorithm::split(splittedTexts, recognizedText, boost::algorithm::is_any_of("|")); // FIXME: some characters are recognized wrongly, l to |

            int textStart = 0, textEnd = 0, textLength = 0;
            for(auto splitText : splittedTexts)
            {
                textLength = (int)splitText.length();
                textEnd = textStart + textLength;
                if (TextUtil::Instance().MatchText(splitText, recordText, hammDistance, language))
                {
                    Rect splittedRect;
                    splittedRect.x = textRect.x + (int)(textRect.width * textStart / (double)(recognizedText.length()));
                    splittedRect.y = textRect.y;
                    splittedRect.width = (int)(textRect.width * textLength / (double)(recognizedText.length()));
                    splittedRect.height = textRect.height;

                    if (hammDistance <= minHammDistance)
                    {
                        targetPoints.clear();
                        minHammDistance = hammDistance;

                        Point rotatedTextPoint = cv::Point(splittedRect.x + splittedRect.width / 2, splittedRect.y + splittedRect.height / 2);
                        Size frameSize;
                        Point targetPoint = TransformRotatedFramePointToFramePoint(rotatedReplayMainFrame.size(), rotatedTextPoint, orientation, frameSize);
                        targetPoint = ObjectUtil::Instance().RelocatePointByOffset(targetPoint, replayFrameMainROI, true);
                        targetPoints.push_back(targetPoint);
                    }
                }
                textStart = textEnd;
            }
        }

        return targetPoints;
    }

    Point MainTest::MatchByAkaze(ScriptHelper::QSPreprocessInfo info, Rect replayFrameMainROI, const cv::Mat &homography)
    {
        Mat sourcePointMat, targetPointMat;
        Point targetPoint = EmptyPoint;
        Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(info.framePoint, info.mainROI, false);

        if (!homography.empty())
        {
            sourcePointMat = (Mat_<double>(3, 1) << pointOnMainROI.x, pointOnMainROI.y, 1);
            targetPointMat = homography * sourcePointMat;
            targetPoint.x = (int)(targetPointMat.at<double>(0, 0) / targetPointMat.at<double>(2, 0));
            targetPoint.y = (int)(targetPointMat.at<double>(1, 0) / targetPointMat.at<double>(2, 0));
            targetPoint = ObjectUtil::Instance().RelocatePointByOffset(targetPoint, replayFrameMainROI, true);
        }

        return targetPoint;
    }

    cv::Mat MainTest::MatchFrameByAkaze(ScriptHelper::QSPreprocessInfo info, cv::Mat replayMainFrame, cv::Rect replayFrameMainROI, double *unmatchRatio)
    {
        cv::Mat recordMainFrame;

        ObjectUtil::Instance().CopyROIFrame(info.frame, info.mainROI, recordMainFrame);

        this->featMatcher->SetModelImage(recordMainFrame);
        return featMatcher->Match(replayMainFrame, unmatchRatio);
    }

    Point MainTest::MatchByShape(ScriptHelper::QSPreprocessInfo info, Mat replayMainFrame, Rect replayFrameMainROI, ScheduleEvent &scheduleEvent)
    {
        Point targetPoint = EmptyPoint;
        Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(info.framePoint, info.mainROI, false);

        // If not click on shape, no need to do shape match
        if (info.clickedShape == nullptr)
        {
            return targetPoint;
        }

        // Initialize scheduleEvent
        scheduleEvent.mainROI = replayFrameMainROI;
        scheduleEvent.isTriangle = false;
        scheduleEvent.trianglePoint1 = EmptyPoint;
        scheduleEvent.trianglePoint2 = EmptyPoint;

        Size replayMainSize = replayMainFrame.size();
        Rect replayFrameRect(EmptyPoint, replayMainSize);
        Point recordReferencePoint(info.mainROI.width / 2, info.mainROI.height / 2);
        Point replayReferencePoint(replayFrameMainROI.width / 2, replayFrameMainROI.height / 2);

        // Get replay shapes
        shapeExtractor->SetFrame(replayMainFrame);
        shapeExtractor->Extract();
        vector<boost::shared_ptr<Shape>> replayShapes = shapeExtractor->GetShapes();
        // Get replay align
        ShapeAlignOrientation replayAlignOrientation = ShapeAlignOrientation::UNKNOWN;
        vector<boost::shared_ptr<Shape>> replayAlignShapes = ShapeUtil::Instance().AlignMaxShapeGroup(replayShapes, replayAlignOrientation);

        // Get record shapes
        vector<boost::shared_ptr<Shape>> recordShapes = info.shapes;
        // Get record align
        ShapeAlignOrientation recordAlignOrientation = info.alignOrientation;
        vector<boost::shared_ptr<Shape>> recordAlignShapes = info.alignShapes;
        int alignShapeIndex = info.clickedAlignShapeIndex;

        // Get replay click point by shapes, include align and triangle
        std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> shapePairs;

        // Customize shapeMatcher
        shapeMatcher->SetSourceReferencePoint(recordReferencePoint);
        shapeMatcher->SetTargetReferencePoint(replayReferencePoint);
        shapeMatcher->SetSourceShapes(recordShapes);
        shapeMatcher->SetTargetShapes(replayShapes);

        // Customize pointMatcher
        pointMatcher->SetTargetFrameRect(replayFrameRect);
        pointMatcher->SetSourceReferencePoint(recordReferencePoint);
        pointMatcher->SetTargetReferencePoint(replayReferencePoint);
        pointMatcher->SetSourceClickedShape(info.clickedShape);
        pointMatcher->SetSourceShapes(recordShapes);
        pointMatcher->SetTargetShapes(replayShapes);

        scheduleEvent.shapes = replayShapes;
        if (shapeMatcher->MatchAlignShapeGroup(recordAlignShapes, replayAlignShapes, recordAlignOrientation, replayAlignOrientation))
        {
            if (alignShapeIndex != -1)
            {
                targetPoint = pointMatcher->MatchPointByOffsetRatio(pointOnMainROI, recordAlignShapes[alignShapeIndex]->boundRect, replayAlignShapes[alignShapeIndex]->boundRect);
            }
            else
            {
                size_t recordAlignShapeSize = recordAlignShapes.size();
                targetPoint = pointMatcher->LocatePointByTriangle(pointOnMainROI, recordAlignShapes[recordAlignShapeSize - 1]->center, recordAlignShapes[0]->center, replayAlignShapes[recordAlignShapeSize - 1]->center, replayAlignShapes[0]->center);

                if (targetPoint != EmptyPoint)
                {
                    scheduleEvent.isTriangle = true;
                    scheduleEvent.trianglePoint1 = recordAlignShapes[recordAlignShapeSize - 1]->center;
                    scheduleEvent.trianglePoint2 = recordAlignShapes[0]->center;
                }
            }
        }
        else
        {
            shapePairs = shapeMatcher->MatchFuzzyShapePairs();
            pointMatcher->SetShapePairs(shapePairs);
            targetPoint = pointMatcher->MatchPoint(pointOnMainROI, scheduleEvent.isTriangle, scheduleEvent.trianglePoint1, scheduleEvent.trianglePoint2);
        }

        if (targetPoint != EmptyPoint)
        {
            targetPoint = ObjectUtil::Instance().RelocatePointByOffset(targetPoint, replayFrameMainROI, true);
        }

        return targetPoint;
    }

    bool MainTest::MatchCross(const Mat& replayFrame, vector<Point> &points)
    {
        points.clear();
        bool crossFound = crossRecognizer->BelongsTo(replayFrame);
        if(crossFound)
        {
            Point point = crossRecognizer->GetCrossCenterPoint();
            points.push_back(point);
        }

        return crossFound;
    }

    bool MainTest::IsValidPoint(Rect roi, Point p)
    {
        // Contain include the points on the boundary
        if(roi.contains(p) && p.x > 0 && p.y > 0)
            return true;
        else
            return false;
    }

    bool MainTest::IsValidRect(Rect r)
    {
        if(r.x < 0)
            return false;

        if(r.y < 0)
            return false;

        if(r.width == 0)
            return false;

        if(r.height == 0)
            return false;

        return true;
    }

    void MainTest::MarkEventsBeforeMatchEvent(vector<QSIpPair> targetQSIpPairs, ScheduleEvent matchedEvent)
    {
        for(auto pair : targetQSIpPairs)
        {
            if(pair.qsName == matchedEvent.qsName && pair.startIp < matchedEvent.startIp)
            {
                allMatchedEvents.push_back(ScheduleEvent(pair.qsName, pair.startIp,  scriptPreprocessInfos[pair.qsName][pair.startIp].endIp));
            }
        }
    }

    vector<QSIpPair> MainTest::FindFirstTargetQSIpPairCandidates(vector<QSIpPair> pairs)
    {
        vector<QSIpPair> idPairs;
        std::unordered_map<string, bool> existingIdMap;
        for(auto pair: pairs)
        {
            string qsName = pair.qsName;
            if(existingIdMap.find(qsName) == existingIdMap.end())
            {
                existingIdMap[qsName] = true;
                idPairs.push_back(pair);
            }
            else
            {
                continue;
            }
        }

        return idPairs;
    }

    Point MainTest::TransformDevicePointToFramePoint(Point devicePoint, Size deviceSize, Orientation orientation, Size frameSize)
    {
        Size orientationDeviceSize;
        switch(orientation)
        {
        case Orientation::Portrait:
        case Orientation::ReversePortrait:
            {
                orientationDeviceSize = Size(deviceSize.width, deviceSize.height);
                break;
            }
        case Orientation::Landscape:
        case Orientation::ReverseLandscape:
            {
                orientationDeviceSize = Size(deviceSize.height, deviceSize.width);
                break;
            }
        default:
            {
                orientationDeviceSize = Size(deviceSize.width, deviceSize.height);
            }
        }

        Size tmpSize;
        Point framePoint = TransformRotatedFramePointToFramePoint(orientationDeviceSize, devicePoint, orientation, tmpSize);

        framePoint.x = framePoint.x * frameSize.width / max(deviceSize.width, deviceSize.height);
        framePoint.y = framePoint.y * frameSize.height / min(deviceSize.width, deviceSize.height);

        return framePoint;
    }

    std::unordered_map<string, string> MainTest::ParseRecordTouchDown(QSIpPair pair)
    {
        std::unordered_map<string, string> keyValueMap;
        if(boost::equals(pair.qsName, unknownQsName))
            return keyValueMap;

        if(!scriptPreprocessInfos[pair.qsName][pair.startIp].isTouchAction)
            return keyValueMap;

        boost::shared_ptr<QScript> script = scripts[pair.qsName];
        boost::shared_ptr<QSEventAndAction> qsEvent = script->EventAndAction(pair.startIp);
        string keyValue = qsEvent->StrOperand(0);
        keyValueMap = ScriptReplayer::ParseKeyValues(keyValue);
        return keyValueMap;
    }

    bool MainTest::MatchIdObject(QSIpPair pair, std::unordered_map<string, string> keyValueMap, vector<string>& lines, Orientation orientation, Rect fw, Rect replayFrameMainROI)
    {
        if(!UseID(currentMatchPattern))
            return false;

        string idKey = "id", ratioKey = "ratio", layoutKey = "layout";
        Point idOnDevice = EmptyPoint;
        Rect idRect;
        boost::shared_ptr<ViewHierarchyTree> replayRoot = viewHierarchyParser->ParseTree(lines);
        map<string, vector<Rect>> replayTreeControls = viewHierarchyParser->GetTreeControls();
        viewHierarchyParser->CleanTreeControls();

        if(keyValueMap.find(layoutKey) != keyValueMap.end())
        {
            boost::filesystem::path layoutPath = xmlFolder / keyValueMap[layoutKey];
            if(boost::filesystem::exists(layoutPath))
            {
                boost::shared_ptr<QSEventAndAction> qsEvent = scripts[pair.qsName]->EventAndAction(pair.startIp);
                Point clickPointOnDevice(qsEvent->IntOperand(0) * recordDeviceSize.width / maxWidthHeight, qsEvent->IntOperand(1) * recordDeviceSize.height / maxWidthHeight);

                Rect sbRect, nbRect, fwRect;
                bool hasKeyboardFromWindow;
                ScriptHelper::ProcessWindowInfo(keyValueMap, sbRect,  nbRect, hasKeyboardFromWindow, fwRect);

                string layoutFile = layoutPath.string();
                vector<string> recordLines;
                ReadAllLines(layoutFile, recordLines);
                boost::shared_ptr<ViewHierarchyTree> recordRoot = viewHierarchyParser->ParseTree(recordLines);
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
                        DAVINCI_LOG_WARNING << "Cannot find replay node or record/replay id is different from tree";
                        idOnDevice = EmptyPoint;
                    }
                    else
                    {
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
            Rect idRect = viewHierarchyParser->FindMatchedRectById(replayTreeControls, keyValueMap[idKey], fw);
            if(idRect == EmptyRect)
            {
                DAVINCI_LOG_WARNING << "Replay does not find ID(" << keyValueMap[idKey] << ") - skip ID match";
                return false;
            }

            if(IsValidRect(idRect))
            {
                string ratioStr = ScriptHelper::FindAdditionalInfo(keyValueMap, ratioKey);

                Point idRectCenterPoint;
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
            else
            {
                DAVINCI_LOG_WARNING << "Invalid Id rect from ID match.";
                return false;
            }
        }

        Point framePoint = TransformDevicePointToFramePoint(idOnDevice, replayDeviceSize, orientation, replayFrameSize);
        if(!replayFrameMainROI.contains(framePoint))
        {
            DAVINCI_LOG_WARNING << "Replay main ROI does not contain frame point - skip ID match";
            return false;
        }

        ScheduleEvent scheduleEvent;
        scheduleEvent.qsName = pair.qsName;
        scheduleEvent.startIp = pair.startIp;
        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[pair.qsName][pair.startIp];
        scheduleEvent.endIp = info.endIp;
        scheduleEvent.qsPriority = scriptPriorities[pair.qsName];

        scheduleEvent.isTouchMove = info.isTouchMove;
        scheduleEvent.objectMatchedPoint = framePoint;

        scheduleEvent.isAdvertisement = info.isAdvertisement;
        lastMatchedEvents.push_back(scheduleEvent);

        string treeFile = ConcatPath(TestReport::currentQsLogPath, "replay_" + scheduleEvent.qsName + "_" + boost::lexical_cast<string>(scheduleEvent.startIp) + ".tree");
        std::ofstream treeStream(treeFile, std::ios_base::app | std::ios_base::out);
        for(auto line: lines)
            treeStream << line << "\r\n" << std::flush;
        treeStream.flush();
        treeStream.close();

        return true;
    }

    // Stretch only supports one unit distance
    bool MainTest::IsValidCandidateDistance(QSIpPair candidate, int startIp, int maxDistance)
    {
        map<int, ScriptHelper::QSPreprocessInfo> infos = scriptPreprocessInfos[candidate.qsName];

        int startIndex = 0, endIndex = INT_MAX, index = 0;
        int nonTouchActionsBetween = 0;
        int popUpActionsBetween = 0;
        bool isStart = false;
        for(auto infoIt : infos) 
        {
            if(infoIt.first == candidate.startIp)
            {
                if(infoIt.second.isPopUpWindow)
                {
                    popUpActionsBetween++;
                }

                endIndex = index;
                break;
            }
            else if(infoIt.first == startIp)
            {
                isStart = true;
                startIndex = index;
            }
            else
            {
                if(isStart)
                {
                    if(!infoIt.second.isTouchAction)
                    {
                        nonTouchActionsBetween++;
                    }
                    else if(infoIt.second.isPopUpWindow)
                    {
                        popUpActionsBetween++;
                    }
                }
            }
            index++;
        }

        if(endIndex - startIndex <= maxDistance + nonTouchActionsBetween + popUpActionsBetween)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    int MainTest::FindFirstTouchDownIpFromScriptInfos(QSIpPair candidate)
    {
        map<int, ScriptHelper::QSPreprocessInfo> infos = scriptPreprocessInfos[candidate.qsName];
        for(auto infoIt : infos) 
        {
            if(infoIt.second.isTouchAction)
                return infoIt.first;
        }

        return -1;
    }

    int MainTest::FindLastIpFromMatchedEvents(QSIpPair candidate)
    {
        int size = (int)allMatchedEvents.size();
        if(size == 0)
            return -1;

        for(int index = size - 1; index >= 0; index--)
        {
            if(allMatchedEvents[index].qsName == candidate.qsName) // Check whether touch action
                return allMatchedEvents[index].startIp;
        }
        return -1;
    }

    bool MainTest::UseDirectStretch(string matchPattern)
    {
        if(boost::equals(matchPattern, "direct"))
            return true;

        return false;
    }

    bool MainTest::UseReverseDirectStretch(string matchPattern)
    {
        if(boost::equals(matchPattern, "reversedirect"))
            return true;

        return false;
    }

    bool MainTest::UseStretch(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("stretch") != -1)
            return true;

        return false;
    }

    bool MainTest::UseID(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("id") != -1)
            return true;

        return false;
    }

    bool MainTest::UsePopUp(string matchPattern)
    {
        if(matchPattern.find("popup") != -1)
            return true;

        return false;
    }

    bool MainTest::UseCross(string matchPattern)
    {
        if(matchPattern.find("cross") != -1)
            return true;

        return false;
    }

    bool MainTest::UseDPI(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("dpi") != -1)
            return true;

        return false;
    }

    bool MainTest::UseText(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("text") != -1)
            return true;

        return false;
    }

    bool MainTest::UseAkaze(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("akaze") != -1)
            return true;

        return false;
    }

    bool MainTest::UseShape(string matchPattern)
    {
        if(boost::equals(matchPattern, "") || matchPattern.find("shape") != -1)
            return true;

        return false;
    }

    bool MainTest::MatchStretchObject(QSIpPair candidate, std::unordered_map<string, string> keyValueMap, Rect replayFrameMainROI)
    {
        if(!UseStretch(currentMatchPattern))
            return false;

        Mat recordFrame = mainTestReport->ReadFrameFromResources(candidate.qsName, candidate.startIp);
        Size recordFrameSize = recordFrame.size();
        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        boost::shared_ptr<QScript> script = scripts[candidate.qsName];
        boost::shared_ptr<QSEventAndAction> qsEvent = script->EventAndAction(candidate.startIp);
        string keyValue = qsEvent->StrOperand(0);
        Orientation recordOrientation = qsEvent->GetOrientation();
        Rect sbRect, nbRect, fwRect;
        bool recordKeyboardPopUp;
        ScriptHelper::ProcessWindowInfo(keyValueMap, sbRect,  nbRect, recordKeyboardPopUp, fwRect);

        if(replayKeyboardPopUp != recordKeyboardPopUp)
            return false;

        Rect recordMainROI = info.mainROI;
        Rect replayMainROI = replayFrameMainROI;
        string localStretchMode = ScriptHelper::FindAdditionalInfo(keyValueMap, "stretch");

        // Correct main roi if record and replay has different behaviors
        // Virtual bar / hard key
        // Different keyboard
        Point targetPoint;
        if(UseDirectStretch(localStretchMode))
        {
            Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(info.framePoint, recordMainROI, false);
            targetPoint = ObjectUtil::Instance().RelocatePointByOffset(pointOnMainROI, replayMainROI, true);
        }
        else if(UseReverseDirectStretch(localStretchMode))
        {
            Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(info.framePoint, recordMainROI, false);
            Point reverseOffset;
            reverseOffset.x = recordMainROI.width - pointOnMainROI.x;
            reverseOffset.y = pointOnMainROI.y;

            if(info.hasVirtualNavigation == replayHasVirtualBar)
            {
                reverseOffset.x = int(reverseOffset.x * replayFrameMainROI.width / (double)recordMainROI.width);
                reverseOffset.y = int(reverseOffset.y * replayFrameMainROI.height / (double)recordMainROI.height);
            }
            else
            {
                Rect recordFrameMainROIWithBar;
                Point fwPoint(fwRect.x, fwRect.y);
                if(recordOrientation == Orientation::Portrait || recordOrientation == Orientation::ReversePortrait)
                    recordFrameMainROIWithBar = AndroidTargetDevice::GetWindowRectangle(fwPoint, fwRect.width, recordDeviceSize.height - fwRect.y, recordDeviceSize, recordOrientation, recordFrameSize.width);
                else
                    recordFrameMainROIWithBar = AndroidTargetDevice::GetWindowRectangle(fwPoint, fwRect.width, recordDeviceSize.width - fwRect.y, recordDeviceSize, recordOrientation, recordFrameSize.width);

                reverseOffset.x = int(reverseOffset.x * replayFrameMainROIWithBar.width / (double)recordFrameMainROIWithBar.width);
                reverseOffset.y = int(reverseOffset.y * replayFrameMainROIWithBar.height / (double)recordFrameMainROIWithBar.height);
            }

            targetPoint.x = replayMainROI.width - reverseOffset.x;
            targetPoint.y = reverseOffset.y;

            targetPoint = ObjectUtil::Instance().RelocatePointByOffset(targetPoint, replayMainROI, true);
        }
        else if(UseDirectStretch(globalStretchMode))
        {
            Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(info.framePoint, recordMainROI, false);
            targetPoint = ObjectUtil::Instance().RelocatePointByOffset(pointOnMainROI, replayMainROI, true);
        }
        else
        {
            targetPoint = pointMatcher->MatchPointByStretch(info.framePoint, recordMainROI, replayMainROI);
        }

        if(targetPoint != EmptyPoint)
        {
            ScheduleEvent scheduleEvent;
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = targetPoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);
            return true;
        }

        return false;
    }

    bool MainTest::MatchPopUpObject(const Mat rotatedReplayFrame, vector<Point> &points, bool isRandom)
    {
        if(!isRandom && !UsePopUp(currentMatchPattern))
        {
            return false;
        }

        vector<Keyword> allKeywords;
        string matchedLanguage;
        if(exploreEngine->MatchPopUpKeyword(rotatedReplayFrame, points, languageMode, matchedLanguage, allKeywords))
        {
            if(!isRandom)
            {
                if(targetQSIpPairs.size() > 0)
                {
                    QSIpPair pair = targetQSIpPairs[0];
                    ScheduleEvent scheduleEvent;
                    scheduleEvent.qsName = pair.qsName;
                    scheduleEvent.startIp = pair.startIp;
                    allMatchedEvents.push_back(scheduleEvent);
                }
            }
            else
            {
                boost::shared_ptr<KeywordCheckHandler> handler;
                popupFailure = false;
                if(boost::equals(matchedLanguage, "en"))
                {
                    vector<Keyword> zhKeywords;
                    handler = keywordCheckHandler1->HandleRequest(matchedLanguage, allKeywords, zhKeywords);
                }
                else
                {
                    vector<Keyword> enKeywords;
                    handler = keywordCheckHandler1->HandleRequest(matchedLanguage, enKeywords, allKeywords);
                }
                if (handler != nullptr)
                {
                    if(IsErrorFailureTypeForFrame(handler->GetCurrentFailureType()))
                    {
                        popupFailure = true;
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool MainTest::MatchCrossObject(const Mat rotatedReplayFrame, vector<Point> &points, bool isRandom)
    {
        if(!isRandom && !UseCross(currentMatchPattern))
            return false;

        if(MatchCross(rotatedReplayFrame, points))
        {
            if(!isRandom && targetQSIpPairs.size() > 0)
            {
                QSIpPair pair = targetQSIpPairs[0];
                ScheduleEvent scheduleEvent;
                scheduleEvent.qsName = pair.qsName;
                scheduleEvent.startIp = pair.startIp;
                allMatchedEvents.push_back(scheduleEvent);
            }
            return true;
        }

        return false;
    }

    bool MainTest::MatchSystemFocusWindow(string replayFocusWindow)
    {
        for(auto sWindow: systemFocusWindows)
        {
            if(boost::starts_with(replayFocusWindow, sWindow))
                return true;
        }

        return false;
    }   

    bool MainTest::MatchSystemPopUpWindow(string replayFocusWindow)
    {
        for(auto sWindow: systemPopUpWindows)
        {
            if(boost::starts_with(replayFocusWindow, sWindow))
                return true;
        }

        return false;
    }

    bool MainTest::MatchKeyboardPopUp(bool keyboardPopUp)
    {
        if(keyboardPopUp)
            return true;
        else
            return false;
    }

    bool MainTest::MatchDPIObject(QSIpPair candidate, Orientation orientation)
    {
        if(!UseDPI(currentMatchPattern))
            return false;

        if(FEquals(recordDPI, 0.0) || FEquals(replayDPI, 0.0))
            return false;

        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        Point targetPointOnDevice = pointMatcher->MatchPointByDPI(info.devicePoint, recordDeviceSize, replayDeviceSize, recordDPI, replayDPI);
        // DPI for text is valid, however sometimes universal DPI may lead to point out of replay device size
        if(!Rect(EmptyPoint, replayDeviceSize).contains(targetPointOnDevice))
            return false;

        Point framePoint = TransformDevicePointToFramePoint(targetPointOnDevice, replayDeviceSize, orientation, replayFrameSize);

        if(framePoint != EmptyPoint)
        {
            ScheduleEvent scheduleEvent;
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = framePoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);
            return true;
        }

        return false;
    }

    bool MainTest::MatchVirtualObject(QSIpPair candidate)
    {
        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        if (info.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Command || info.isVirtualNavigation)
        {
            ScheduleEvent scheduleEvent;
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = EmptyPoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);
            return true;
        }

        return false;
    }

    bool MainTest::MatchTextObject(QSIpPair candidate, cv::Mat rotatedMainFrame, Size rotatedFrameSize, Rect replayFrameMainROI)
    {
        if(!UseText(currentMatchPattern))
            return false;

        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        Mat recordFrame = mainTestReport->ReadFrameFromResources(candidate.qsName, candidate.startIp);
        info.frame = recordFrame;

        Point targetPoint = EmptyPoint;
        if (info.clickedTextRect != EmptyRect)
        {
            vector<Point> targetPoints = MatchByText(info, rotatedFrameSize, replayFrameMainROI, rotatedMainFrame);

            if (!targetPoints.empty())
            {
                double minDistance = DBL_MAX;
                size_t minIndex = 0;
                for (size_t i = 0; i < targetPoints.size(); i++)
                {
                    double distance = ComputeDistance(targetPoints[i], info.framePoint);
                    if (FSmaller(distance, minDistance))
                    {
                        minDistance = distance;
                        minIndex = i;
                    }
                }
                targetPoint = targetPoints[minIndex];
            }
        }

        if (targetPoint != EmptyPoint)
        {
            ScheduleEvent scheduleEvent;
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = targetPoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);

            return true;
        }

        return false;
    }

    bool MainTest::MatchAkazeObject(QSIpPair candidate, cv::Mat replayMainFrame, Rect replayFrameMainROI)
    {
        if(!UseAkaze(currentMatchPattern))
            return false;

        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        Mat recordFrame = mainTestReport->ReadFrameFromResources(candidate.qsName, candidate.startIp);
        info.frame = recordFrame;

        double zeroUnmatchRatio = 0;
        double *pUnmatchRatio = &zeroUnmatchRatio;
        cv::Mat homography = MatchFrameByAkaze(info, replayMainFrame, replayFrameMainROI, pUnmatchRatio);
        double unmatchRatio = *pUnmatchRatio;

#ifdef _DEBUG
        Mat debugRecordFrame, debugReplayFrame;
        ObjectUtil::Instance().CopyROIFrame(info.frame, info.mainROI, debugRecordFrame);
        debugReplayFrame = replayMainFrame.clone();

        SaveImage(debugRecordFrame, ConcatPath(replayFolder.string(),"debug_record_" + boost::lexical_cast<string>(unmatchRatio).substr(0,5) + ".png"));
        SaveImage(debugReplayFrame, ConcatPath(replayFolder.string(), "debug_replay_" + boost::lexical_cast<string>(unmatchRatio).substr(0,5) + ".png"));
#endif
        Point targetPoint = EmptyPoint;
        if (unmatchRatio < currentFrameUnmatchRatioThreshold)
        {
            targetPoint = MatchByAkaze(info, replayFrameMainROI, homography);
        }

        if (targetPoint != EmptyPoint)
        {
            ScheduleEvent scheduleEvent;
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = targetPoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);

            return true;
        }

        return false;
    }

    bool MainTest::MatchShapeObject(QSIpPair candidate, cv::Mat replayFrame, Rect replayFrameMainROI)
    {
        if(!UseShape(currentMatchPattern))
            return false;

        ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[candidate.qsName][candidate.startIp];
        Mat recordFrame = mainTestReport->ReadFrameFromResources(candidate.qsName, candidate.startIp);
        info.frame = recordFrame;

        ScheduleEvent scheduleEvent;
        Point targetPoint = MatchByShape(info, replayFrame, replayFrameMainROI, scheduleEvent);

        if (targetPoint != EmptyPoint)
        {
            scheduleEvent.qsName = candidate.qsName;
            scheduleEvent.startIp = candidate.startIp;
            scheduleEvent.endIp = info.endIp;
            scheduleEvent.qsPriority = scriptPriorities[candidate.qsName];

            scheduleEvent.isTouchMove = info.isTouchMove;
            scheduleEvent.objectMatchedPoint = targetPoint;

            scheduleEvent.isAdvertisement = info.isAdvertisement;
            lastMatchedEvents.push_back(scheduleEvent);

            return true;
        }

        return false;
    }

    void MainTest::DispatchCrossAction(Mat frame, const Mat replayMainFrame, const Mat rotatedReplayMainFrame, Orientation orientation, Point point, Rect replayFrameMainROI, ScheduleEvent event)
    {
        boost::filesystem::path resourcePath;
        if(event.IsSameEvent(emptyEvent))
        {
            replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, orientation, true, false));
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex);
            mainTestReport->SaveRandomMainReplayFrame(replayMainFrame, randomIndex);
        }
        else
        {
            replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation, false, true));
            resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
            SaveImage(frame, resourcePath.string());
        }

        DAVINCI_LOG_INFO << "Dispatch cross action ... " << endl;
        Size frameSize = frame.size();
        Point framePoint = TransformRotatedFramePointToFramePoint(rotatedReplayMainFrame.size(), point, orientation, frameSize);
        framePoint = ObjectUtil::Instance().RelocatePointByOffset(framePoint, replayFrameMainROI, true);
        ClickOnFrame(dut, frame.size(), framePoint.x, framePoint.y, orientation);
        DAVINCI_LOG_INFO << "Click cross @(" << boost::lexical_cast<string>(framePoint.x) << "," << boost::lexical_cast<string>(framePoint.y) << ")" << endl;
        Point norm = FrameToNormCoord(frameSize, point, orientation);
        AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_TOUCHDOWN, norm.x, norm.y, -1, 0, "", orientation)));
        AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_TOUCHUP, norm.x, norm.y, -1, 0, "", orientation)));
        DrawCircle(frame, framePoint, Yellow);
        breakPointWaitEvent.WaitOne(defaultPopUpActionWaitTime);
        breakPointWaitEvent.WaitOne(defaultPopUpSwitchWaitTime);

        if(event.IsSameEvent(emptyEvent))
        {
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex, true);
            randomIndex++;
        }
        else
        {
            resourcePath = replayFolder;
            resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
            SaveImage(frame, resourcePath.string());
        }

        CheckProcessExitForNoneTouchAction(framePoint, frame, orientation);

        WaitForNextAction();
    }

    void MainTest::DispatchBackAction(Mat frame, const Mat replayMainFrame, Orientation orientation)
    {
        replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, orientation, true, false));
        mainTestReport->SaveRandomReplayFrame(frame, randomIndex);
        mainTestReport->SaveRandomMainReplayFrame(replayMainFrame, randomIndex);
        PressBack();            

        breakPointWaitEvent.WaitOne(defaultPopUpSwitchWaitTime);

        mainTestReport->SaveRandomReplayFrame(frame, randomIndex, true);
        randomIndex++;
    }

    // Close unexpected keyboard and app resolve window
    void MainTest::DispatchWindowCloseAction(Mat frame, const Mat replayMainFrame, Orientation orientation)
    {
        DAVINCI_LOG_INFO << "Dispatch window close action ... " << endl;
        DispatchBackAction(frame, replayMainFrame, orientation);

        WaitForNextAction();
    }

    void MainTest::DispatchPopUpAction(Mat frame, const Mat replayMainFrame, const Mat rotatedReplayMainFrame, Orientation orientation, vector<Point> points, Rect replayFrameMainROI, ScheduleEvent event)
    {
        boost::filesystem::path resourcePath;
        if(event.IsSameEvent(emptyEvent))
        {
            replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, orientation, true, false));
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex);
            mainTestReport->SaveRandomMainReplayFrame(replayMainFrame, randomIndex);
        }
        else
        {
            replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation, false, true));
            resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
            SaveImage(frame, resourcePath.string());
        }

        DAVINCI_LOG_INFO << "Dispatch popup action ... " << endl;
        Size frameSize = frame.size();
        for(auto clickPoint : points)
        {
            Point point = TransformRotatedFramePointToFramePoint(rotatedReplayMainFrame.size(), clickPoint, orientation, frameSize);
            point = ObjectUtil::Instance().RelocatePointByOffset(point, replayFrameMainROI, true);
            ClickOnFrame(dut, frame.size(), point.x, point.y, orientation);

            Point norm = FrameToNormCoord(frameSize, point, orientation);
            AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_TOUCHDOWN, norm.x, norm.y, -1, 0, "", orientation)));
            AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_TOUCHUP, norm.x, norm.y, -1, 0, "", orientation)));

            DAVINCI_LOG_INFO << "Click popup keyword @(" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
            DrawCircle(frame, point, Yellow);
            breakPointWaitEvent.WaitOne(defaultPopUpActionWaitTime);

            CheckProcessExitForNoneTouchAction(point, frame, orientation);
        }
        breakPointWaitEvent.WaitOne(defaultPopUpSwitchWaitTime);

        if(event.IsSameEvent(emptyEvent))
        {
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex, true);
            randomIndex++;
        }
        else
        {
            resourcePath = replayFolder;
            resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
            SaveImage(frame, resourcePath.string());
        }

        WaitForNextAction();
    }

    DaVinciStatus MainTest::StopDispatchTimer()
    {
        dispatchTimer->SetTimerHandler(HighResolutionTimer::TimerCallbackFunc());
        return dispatchTimer->Stop();
    }

    void MainTest::DispatchVirtualKeyAction(ScriptHelper::QSPreprocessInfo qsInfo, ScheduleEvent event, Mat frame, Orientation orientation, Size replayDeviceSize, Rect nbRect, Rect sbRect, Rect fwRect)
    {
        boost::filesystem::path resourcePath;
        resourcePath = replayFolder;
        string subInputFileName = qsInfo.keyboardInput;
        boost::algorithm::replace_all(subInputFileName, "?", "");       // file name shouldn't contain ? and >
        boost::algorithm::replace_all(subInputFileName, ">", "");

        resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_keyboard_" + subInputFileName + ".png");
        SaveImage(frame, resourcePath.string());

        resourcePath = replayFolder;
        resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
        SaveImage(frame, resourcePath.string());

        string strInput = qsInfo.keyboardInput;

        if (qsInfo.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Only)      // click on keyboard
        {
            string osVersion = androidDut->GetOsVersion(true);
            Mat rotatedFrame = RotateFrameUp(frame, orientation);

            Rect kbROIOnDevice = ObjectUtil::Instance().GetKeyboardROI(orientation, replayDeviceSize, nbRect, sbRect, fwRect);
            Point kbROIPoint(kbROIOnDevice.x, kbROIOnDevice.y);
            Rect kbROIOnFrame = AndroidTargetDevice::GetWindowRectangle(kbROIPoint, kbROIOnDevice.width, kbROIOnDevice.height, replayDeviceSize, orientation, frame.size().width);
            Rect kbROIOnRotatedFrame = RotateROIUp(kbROIOnFrame, frame.size(), orientation);

            if(keyboardRecog->RecognizeKeyboard(rotatedFrame, osVersion, kbROIOnRotatedFrame))
            {
                boost::to_lower(strInput);
                keyboardRecog->ClickKeyboardForString(rotatedFrame, strInput);
            }
        }
        else
        {
            for (size_t i = 0; i < strInput.size(); ++i)
            {
                if (keyboardRecog->isSwitchButton(strInput[i]) || keyboardRecog->isUnsupportedButton(strInput[i]))      // Switch button or Unsupported button
                {
                    continue;
                }
                else if (keyboardRecog->isEnterButton(strInput[i]))     // Enter button
                {
                    androidDut->PressEnter();
                }
                else if (keyboardRecog->isDeleteButton(strInput[i]))    // Delete button
                {
                    androidDut->PressDelete();
                }
                else
                {
                    dut->TypeString(string(1, strInput[i]));
                }
            }
        }

        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];
        WaitBreakPointForVirtualAction(scriptReplayer, event.startIp, event.endIp);
        replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));

        // Check process exit
        if (CheckProcessExit())
        {
            this->processExit = true;
            crashFrame = GetCurrentFrame();
            DAVINCI_LOG_INFO << "Some action leads to process exit to home";
            SetTestComplete();
            return;
        }
    }

    void MainTest::DispatchVirtualBarAction(ScriptHelper::QSPreprocessInfo qsInfo, ScheduleEvent event, Mat frame, Orientation orientation)
    {
        boost::filesystem::path resourcePath;
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];
        if (qsInfo.navigationEvent == QSEventAndAction::OPCODE_BACK)
        {
            DAVINCI_LOG_INFO << "Press back ..." << endl;
            mainTestReport->SaveNonTouchReplayFrame(frame, QSEventAndAction::OPCODE_BACK, event.qsName, event.startIp);
            PressBack();            
            WaitBreakPointForVirtualAction(scriptReplayer, event.startIp, event.endIp);
            replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));
        }
        else if (qsInfo.navigationEvent == QSEventAndAction::OPCODE_HOME)
        {
            DAVINCI_LOG_INFO << "Press home ..." << endl;
            mainTestReport->SaveNonTouchReplayFrame(frame, QSEventAndAction::OPCODE_HOME, event.qsName, event.startIp);

            PressHome();
            WaitBreakPointForVirtualAction(scriptReplayer, event.startIp, event.endIp);
            replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));
        }
        else
        {
            DAVINCI_LOG_INFO << "Press menu ..." << endl;
            mainTestReport->SaveNonTouchReplayFrame(frame, QSEventAndAction::OPCODE_MENU, event.qsName, event.startIp);

            PressMenu();
            WaitBreakPointForVirtualAction(scriptReplayer, event.startIp, event.endIp);
            replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));
        }
    }

    void MainTest::DispatchNonTouchAction(ScheduleEvent event, Mat frame, Mat rotatedFrame)
    {
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];
        boost::shared_ptr<QScript> currentScript = scripts[event.qsName];
        nextQSIpPair = FindNextQSIp(scriptPreprocessInfos, currentQSIpPair);
        boost::shared_ptr<QSEventAndAction> currentQSEvent = currentScript->EventAndAction(event.startIp);
        boost::shared_ptr<QScript> nextScript = scripts[nextQSIpPair.qsName];
        boost::shared_ptr<QSEventAndAction> nextQSEvent = nextScript->EventAndAction(nextQSIpPair.startIp);

        double waitTime = nextQSEvent->TimeStamp() - 1000;
        if(nextQSEvent->Opcode() == QSEventAndAction::OPCODE_IMAGE_CHECK)
        {
            // Wait enough time for image check
            waitTime = waitTime - nextQSEvent->IntOperand(1);
        }
        else if(nextQSEvent->Opcode() == QSEventAndAction::OPCODE_BACK)
        {
            // Wait enough time for advertisement or network loading
            waitTime = waitTime - 3000;
        }

        waitTime = UpdateWaitTime(waitTime, 0);

        replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, dut->GetCurrentOrientation(), false, false));
        allMatchedEvents.push_back(ScheduleEvent(event.qsName, event.startIp, event.endIp));

        if(currentQSEvent->Opcode() == QSEventAndAction::OPCODE_IMAGE_CHECK 
            || currentQSEvent->Opcode() == QSEventAndAction::OPCODE_IF_MATCH_IMAGE)
        {
            auto objImg = QScript::ImageObject::Parse(nextQSEvent->StrOperand(0));
            int nextIp;
            int matchResult = scriptReplayer->HandleImageCheck(frame, objImg, nextQSEvent->IntOperand(0), nextQSEvent->LineNumber(), nextIp);

            if(matchResult == 0)
            {
                DAVINCI_LOG_INFO << "OPCODE_IMAGE_CHECK @" << nextQSEvent->LineNumber() << ": PASS" << endl;
            }
            else if (matchResult == 1)
            {
                imageCheckPass = false;
                crashFrame = frame;
                DAVINCI_LOG_INFO << "OPCODE_IMAGE_CHECK @" << nextQSEvent->LineNumber() << ": FAIL" << endl;
            }
            else
            {
                imageCheckPass = false;
                crashFrame = frame;
                DAVINCI_LOG_INFO << "OPCODE_IMAGE_CHECK @" << nextQSEvent->LineNumber() << ": IMAGE NOT EXIST (" << nextQSEvent->StrOperand(0) << ")" << endl;
            }

            SetCurrentQSIpPair(event.qsName, event.startIp);
        }
        else if(currentQSEvent->Opcode() == QSEventAndAction::OPCODE_CRASH_ON)
        {
            DAVINCI_LOG_INFO << "Ignore opcode crash on in Get-RnR test.";
            SetCurrentQSIpPair(event.qsName, event.startIp);
        }
        else
        {
            mainTestReport->SaveNonTouchReplayFrame(frame, currentQSEvent->Opcode(), event.qsName, event.startIp);
            ExecuteQSStep(scriptReplayer, event.startIp, event.endIp, waitTime);

            if(currentQSEvent->Opcode() == QSEventAndAction::OPCODE_MESSAGE)
            {
                if ((boost::starts_with(nextQSEvent->StrOperand(0), startLoginMessage)) && (!rotatedFrame.empty()))
                {
                    boost::filesystem::path beforeLoginImage(TestReport::currentQsLogPath + "/beforeLoginImage.png");
                    imwrite(beforeLoginImage.string(), rotatedFrame);
                }
                else if ((boost::starts_with(nextQSEvent->StrOperand(0), finishLoginMessage)) && (!rotatedFrame.empty()))
                {
                    boost::filesystem::path afterLoginImage(TestReport::currentQsLogPath + "/afterLoginImage.png");
                    imwrite(afterLoginImage.string(), rotatedFrame);
                }
            }
            else if(currentQSEvent->Opcode() == QSEventAndAction::OPCODE_START_APP)
            {
                mainTestReport->SaveNonTouchReplayFrame(GetCurrentFrame(), currentQSEvent->Opcode(), event.qsName, event.startIp);  // save the frame after starting app
                vector<string> installedPackage = dut->GetAllInstalledPackageNames();
                if (std::find(installedPackage.begin(), installedPackage.end(), packageName) == installedPackage.end())
                {
                    this->launchFail = true;
                    crashFrame = GetCurrentFrame();
                    DAVINCI_LOG_INFO << "App is not installed, cannot launch.";
                    SetTestComplete();
                    return;
                }

                if (CheckProcessExit())
                {
                    this->launchFail = true;
                    crashFrame = GetCurrentFrame();
                    DAVINCI_LOG_INFO << "App exits instantly after launch.";
                    SetTestComplete();
                    return;
                }
            }
            else if(currentQSEvent->Opcode() == QSEventAndAction::OPCODE_DRAG)
            {
                int x1 = currentQSEvent->IntOperand(0);
                int y1 = currentQSEvent->IntOperand(1);
                int x2 = currentQSEvent->IntOperand(2);
                int y2 = currentQSEvent->IntOperand(3);

                Point p1(x1, y1);
                Point p2(x2, y2);
                vector<Point> points;
                points.push_back(p1);
                points.push_back(p2);

                SwipeDirection direction = ScriptHelper::GetDragDirection(points, currentQSEvent->GetOrientation());

                boost::filesystem::path resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
                SaveImage(frame, resourcePath.string());

                resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
                ExploratoryEngine::DrawPoints(frame, direction, Yellow, 5);
                SaveImage(frame, resourcePath.string());
            }
            else
            {
                // Check process exit
                if (NeedCheckProcessExitForNonTouchAction(currentQSEvent->Opcode()) && CheckProcessExit())
                {
                    this->processExit = true;
                    DAVINCI_LOG_INFO << "Some action leads to process exit to home";
                    crashFrame = GetCurrentFrame();

                    SetTestComplete();
                    return;
                }
            }
        }
    }

    void MainTest::DispatchDirectRecordAction(ScheduleEvent event, Mat frame)
    {
        if(dut == nullptr)
            return;

        Orientation orientation = dut->GetCurrentOrientation();
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];

        boost::shared_ptr<QScript> script = scripts[event.qsName];
        boost::shared_ptr<QSEventAndAction> downQsEvent = script->EventAndAction(event.startIp);

        Size frameSize = frame.size();
        Point framePoint;
        Mat referenceFrame = frame.clone();

        boost::filesystem::path resourcePath;
        for (int blockIndex = 0; blockIndex <= event.endIp - event.startIp; blockIndex++)
        {
            if (blockIndex == 0)
            {
                framePoint = NormToFrameCoord(frameSize, Point2d(downQsEvent->IntOperand(0), downQsEvent->IntOperand(1)), orientation);
                DrawCircle(referenceFrame, framePoint, Yellow);
            }
            else
            {
                boost::shared_ptr<QSEventAndAction> moveUpQsEvent = script->EventAndAction(event.startIp + blockIndex);
                framePoint = NormToFrameCoord(frameSize, Point2d(moveUpQsEvent->IntOperand(0), moveUpQsEvent->IntOperand(1)), orientation);
                DrawCircle(referenceFrame, framePoint, Yellow);
            }
        }

        resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
        SaveImage(frame, resourcePath.string());

        resourcePath = replayFolder;
        resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
        SaveImage(referenceFrame, resourcePath.string());

        scriptReplayer->SetScriptReplayMode(ScriptReplayMode::REPLAY_COORDINATE);
        double waitTime = downQsEvent->TimeStamp() - 1000;
        waitTime = UpdateWaitTime(waitTime, 0);

        // Execute QS block
        ExecuteQSStep(scriptReplayer, event.startIp, event.endIp, waitTime);
        replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));

        CheckProcessExitForTouchAction(event, frame, orientation);
        ResetRandomStatus();
    }

    void MainTest::DispatchIdRecordAction(ScheduleEvent event, Mat frame)
    {
        if(dut == nullptr)
            return;

        Orientation orientation = dut->GetCurrentOrientation();
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];

        boost::filesystem::path resourcePath;
        boost::shared_ptr<QScript> script = scripts[event.qsName];
        boost::shared_ptr<QSEventAndAction> downQsEvent = script->EventAndAction(event.startIp);

        Size frameSize = frame.size();
        vector<Point> framePoints;
        Point framePoint;
        Point blockFistFramePoint;
        Mat referenceFrame = frame.clone();
        Mat recordFrame =  mainTestReport->ReadFrameFromResources(event.qsName, event.startIp);
        Size recordFrameSize = recordFrame.size();

        for (int blockIndex = 0; blockIndex <= event.endIp - event.startIp; blockIndex++)
        {
            if (blockIndex == 0)
            {
                blockFistFramePoint = NormToFrameCoord(frameSize, Point2d(downQsEvent->IntOperand(0), downQsEvent->IntOperand(1)), orientation);
                blockFistFramePoint.x = blockFistFramePoint.x * frameSize.width / recordFrameSize.width;
                blockFistFramePoint.y = blockFistFramePoint.y * frameSize.height / recordFrameSize.height;

                framePoint = event.objectMatchedPoint;
                DrawCircle(referenceFrame, framePoint, Yellow);
            }
            else
            {
                boost::shared_ptr<QSEventAndAction> moveUpQSEvent = script->EventAndAction(event.startIp + blockIndex);
                Point blockMiddleFramePoint = NormToFrameCoord(frameSize, Point2d(moveUpQSEvent->IntOperand(0), moveUpQSEvent->IntOperand(1)), orientation);
                blockMiddleFramePoint.x = blockMiddleFramePoint.x * frameSize.width / recordFrameSize.width;
                blockMiddleFramePoint.y = blockMiddleFramePoint.y * frameSize.height / recordFrameSize.height;

                int xMoveOffset = blockMiddleFramePoint.x - blockFistFramePoint.x;
                int yMoveOffset = blockMiddleFramePoint.y - blockFistFramePoint.y;

                framePoint.x = (int)(event.objectMatchedPoint.x) + xMoveOffset;
                framePoint.y = (int)(event.objectMatchedPoint.y) + yMoveOffset;
                DrawCircle(referenceFrame, framePoint, Yellow);
            }

            framePoints.push_back(framePoint);
        }

        resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
        SaveImage(frame, resourcePath.string());

        resourcePath = replayFolder;
        resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
        SaveImage(referenceFrame, resourcePath.string());

        bool isTouchMove = event.isTouchMove;
        scriptReplayer->SetScriptReplayMode(ScriptReplayMode::REPLAY_OBJECT);
        scriptReplayer->SetBreakPointFrameSizePointAndOrientation(frameSize, framePoints, orientation, isTouchMove, needTapMode);

        double waitTime = downQsEvent->TimeStamp() - 1000;
        waitTime = UpdateWaitTime(waitTime, 0);

        // Execute QS block
        ExecuteQSStep(scriptReplayer, event.startIp, event.endIp, waitTime);
        replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));

        CheckProcessExitForTouchAction(event, frame, orientation);
        ResetRandomStatus();
    }

    void MainTest::DispatchVirtualAction(ScheduleEvent event, Mat frame, Size replayDeviceSize, Rect nbRect, Rect sbRect, Rect fwRect)
    {
        if(dut == nullptr)
            return;

        Orientation orientation = dut->GetCurrentOrientation();
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];

        ScriptHelper::QSPreprocessInfo qsInfo = scriptPreprocessInfos[event.qsName][event.startIp];

        if (qsInfo.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Command)
        {
            DispatchVirtualKeyAction(qsInfo, event, frame, orientation, replayDeviceSize, nbRect, sbRect, fwRect);
        }
        else if (qsInfo.isVirtualNavigation)
        {
            DispatchVirtualBarAction(qsInfo, event, frame, orientation);
        }

        ResetRandomStatus();
    }

    // Execute block opcode [TOUCHDOWN...TOUCHMOVE...TOUCHUP]
    void MainTest::DispatchObjectRecordAction(ScheduleEvent event, Mat frame)
    {
        if(dut == nullptr)
            return;

        Orientation orientation = dut->GetCurrentOrientation();
        boost::shared_ptr<ScriptReplayer> scriptReplayer = scriptReplayers[event.qsName];

        // Used for debug drawing, reference is for original frame, shapes and clickpoint are for scaled frame
        ScriptHelper::QSPreprocessInfo qsInfo = scriptPreprocessInfos[event.qsName][event.startIp];

        boost::filesystem::path resourcePath;
        boost::shared_ptr<QScript> script = scripts[event.qsName];
        boost::shared_ptr<QSEventAndAction> downQsEvent = script->EventAndAction(event.startIp);

        Size frameSize = frame.size();
        vector<Point> framePoints;
        Point framePoint;
        Point blockFistFramePoint;
        Mat referenceFrame = frame.clone();

        for (int blockIndex = 0; blockIndex <= event.endIp - event.startIp; blockIndex++)
        {

            if (blockIndex == 0)
            {
                blockFistFramePoint = NormToFrameCoord(frameSize, Point2d(downQsEvent->IntOperand(0), downQsEvent->IntOperand(1)), orientation);
                framePoint = event.objectMatchedPoint;

                resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference.png");
                SaveImage(frame, resourcePath.string());

                DrawCircle(referenceFrame, framePoint, Yellow);
                resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
                SaveImage(referenceFrame, resourcePath.string());

                if (!event.shapes.empty())
                {
                    Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(framePoint, event.mainROI, false);
                    Mat mainFrame;
                    ObjectUtil::Instance().CopyROIFrame(frame, event.mainROI, mainFrame);

                    DrawShapes(mainFrame, event.shapes, true);
                    DrawCircle(mainFrame, pointOnMainROI, Yellow);
                    if (event.isTriangle)
                    {
                        DrawCircle(mainFrame, event.trianglePoint1, Green);
                        DrawCircle(mainFrame, event.trianglePoint2, Green);
                        line(mainFrame, event.trianglePoint1, pointOnMainROI, Yellow, 2);
                        line(mainFrame, event.trianglePoint2, pointOnMainROI, Yellow, 2);
                    }
                    resourcePath = replayFolder / boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_shapes.png");
                    SaveImage(mainFrame, resourcePath.string());
                }
            }
            else
            {
                boost::shared_ptr<QSEventAndAction> moveUpQSEvent = script->EventAndAction(event.startIp + blockIndex);
                Point blockMiddleFramePoint = NormToFrameCoord(frameSize, Point2d(moveUpQSEvent->IntOperand(0), moveUpQSEvent->IntOperand(1)), orientation);
                int xMoveOffset = blockMiddleFramePoint.x - blockFistFramePoint.x;
                int yMoveOffset = blockMiddleFramePoint.y - blockFistFramePoint.y;

                framePoint.x = (int)(event.objectMatchedPoint.x) + xMoveOffset;
                framePoint.y = (int)(event.objectMatchedPoint.y) + yMoveOffset;

                DrawCircle(referenceFrame, framePoint, Yellow);
                resourcePath = replayFolder;
                resourcePath /= boost::filesystem::path("replay_" + event.qsName + "_" + boost::lexical_cast<string>(event.startIp) + "_reference_clickPoint.png");
                SaveImage(referenceFrame, resourcePath.string());
            }

            framePoints.push_back(framePoint);
        }

        scriptReplayer->SetScriptReplayMode(ScriptReplayMode::REPLAY_OBJECT);
        scriptReplayer->SetBreakPointFrameSizePointAndOrientation(frameSize, framePoints, orientation, event.isTouchMove, needTapMode);

        double waitTime = downQsEvent->TimeStamp() - 1000;
        waitTime = UpdateWaitTime(waitTime, 0);

        // Execute QS block
        ExecuteQSStep(scriptReplayer, event.startIp, event.endIp, waitTime);
        replayIpSequence.push_back(QSIpPair(event.qsName, event.startIp, orientation));

        CheckProcessExitForTouchAction(event, frame, orientation);
        ResetRandomStatus();
    }

    void MainTest::ResetRandomStatus()
    {
        currentRandomAction = 0;
        startRandomAction = false;
    }

    void MainTest::CheckProcessExitForTouchAction(ScheduleEvent event, Mat frame, Orientation frameOrientation)
    {
        vector<Keyword> clickedKeywords;
        // Check process exit
        if (NeedCheckProcessExitForTouchAction(event) && CheckProcessExit())
        {
            Mat rotatedFrame = RotateFrameUp(frame, frameOrientation);
            Point rotatedFramePoint = TransformFramePointToRotatedFramePoint(rotatedFrame.size(), event.objectMatchedPoint, frameOrientation);
            this->processExit = !TextUtil::Instance().IsExpectedProcessExit(rotatedFrame, rotatedFramePoint, languageMode, clickedKeywords);

            if(this->processExit)
            {
                DAVINCI_LOG_INFO << "Some action leads to unexpected process exit to home";
                crashFrame = GetCurrentFrame();

                SetTestComplete();
            }
        }
    }

    void MainTest::CheckProcessExitForNoneTouchAction(Point point, Mat frame, Orientation orientation)
    {
        vector<Keyword> clickedKeywords;
        if (CheckProcessExit())
        {
            Mat rotatedFrame = RotateFrameUp(frame, orientation);
            Point rotatedFramePoint = TransformFramePointToRotatedFramePoint(rotatedFrame.size(), point, orientation);
            this->processExit = !TextUtil::Instance().IsExpectedProcessExit(rotatedFrame, rotatedFramePoint, languageMode, clickedKeywords);

            if(this->processExit)
            {
                DAVINCI_LOG_INFO << "Some action leads to unexpected process exit to home";
                crashFrame = GetCurrentFrame();
            }
            SetTestComplete();
        }
    }

    void MainTest::DispatchRandomAction(cv::Mat frame, cv::Mat rotatedFrame, cv::Mat replayMainFrame, cv::Mat rotatedReplayMainFrame, Orientation orientation)
    {
        // Skip random actions
        if(maxRandomAction == 0)
        {
            ThreadSleep(defaultSkipRandomWaitTime);
            SetTestComplete();
            return;
        }

        // Wait at most three times of one second
        if(!startRandomAction)
        {
            ++currentRandomAction;
            if(currentRandomAction > randomAfterUnmatchAction)
            {
                currentRandomAction = 0;
                startRandomAction = true;
                ThreadSleep(defaultRandomActionWaitTime);
            }
            else
            {
                DAVINCI_LOG_INFO << "Wait for " << boost::lexical_cast<string>(currentRandomAction) << " times of one second before random action.";
            }
        }

        if(startRandomAction)
        {
            replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, orientation, true, false));
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex);
            mainTestReport->SaveRandomMainReplayFrame(replayMainFrame, randomIndex);

            ++currentRandomAction;
            if(currentRandomAction > maxRandomAction)
            {
                DAVINCI_LOG_INFO << "Test completes after " << boost::lexical_cast<string>(maxRandomAction) << " random actions." << endl;
                currentRandomAction = 0;
                startRandomAction = false;
                SetTestComplete();
            }

            DAVINCI_LOG_INFO << "Dispatch random ... " << endl;
            Point clickPoint;
            cv::Mat cloneFrame = rotatedFrame.clone();
            double elapsed = stopWatch->ElapsedMilliseconds(); 
            exploreEngine->runExploratoryTest(rotatedFrame, coverageCollector, biasObject, biasObject->GetSeedValue(), cloneFrame, clickPoint);


            for(boost::shared_ptr<QSEventAndAction> qe : exploreEngine->GetGeneratedQEvents())
            {
                qe->TimeStamp() = elapsed + qe->TimeStamp();
                qe->TimeStampReplay() = elapsed + qe->TimeStamp();
                AddEventToGetRnRQScript(qe, false);
            }

            Size frameSize = frame.size();
            Size rotatedFrameSize = rotatedFrame.size();
            Point point = TransformRotatedFramePointToFramePoint(rotatedFrameSize, clickPoint, orientation, frameSize);
            point = ObjectUtil::Instance().RelocatePointByOffset(point, Rect(EmptyPoint, rotatedFrameSize), true);
            DAVINCI_LOG_INFO << "Click random action @(" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;

            DrawCircle(frame, point, Yellow);
            mainTestReport->SaveRandomReplayFrame(frame, randomIndex, true);

            // If random exploratory test jumps to home or new app (advertisement), press back
            if(dut->GetTopPackageName() != packageName) 
            {
                PressBack();
                ThreadSleep(defaultBackWaitTime);
            }

            if(dut->GetTopPackageName() == startPackageName) 
            {
                DAVINCI_LOG_INFO << "Test completes due to out of test package name." << endl;

                // Corner case for CFBench: random action leads to app crash
                randomIndex++;
                replayIpSequence.push_back(QSIpPair(unknownQsName, randomIndex, dut->GetCurrentOrientation(true), true, false));
                Mat updatedRandomFrame = GetCurrentFrame();
                mainTestReport->SaveRandomReplayFrame(updatedRandomFrame, randomIndex, true);

                SetTestComplete();
            }

            randomIndex++;
        }
    }

    double MainTest::UpdateWaitTime(double time1, double time2)
    {
        if(FSmallerE(time1, time2))
            return time2;
        else
            return time1;
    }

    // The below functions are for coverage purpose
    void MainTest::SetQSPreprocessInfo(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> infos)
    {
        scriptPreprocessInfos = infos;
    }

    vector<ScheduleEvent> MainTest::GetAllMatchedEvents()
    {
        return allMatchedEvents;
    }

    void MainTest::SetAllMatchedEvents(vector<ScheduleEvent> events)
    {
        allMatchedEvents = events;
    }

    bool MainTest::WaiveProcessExit(vector<ScheduleEvent> events)
    {
        int size = (int)(events.size());
        const int waiveSteps = 2;
        if(size <= waiveSteps)
            return false;

        ScheduleEvent lastFirstEvent = events[size - 1];
        ScheduleEvent lastSecondEvent = events[size - 2];

        ScriptHelper::QSPreprocessInfo lastFirstInfo = scriptPreprocessInfos[lastFirstEvent.qsName][lastFirstEvent.startIp];
        ScriptHelper::QSPreprocessInfo lastSecondInfo = scriptPreprocessInfos[lastSecondEvent.qsName][lastSecondEvent.startIp];

        if(lastFirstInfo.isTouchAction && (lastSecondInfo.navigationEvent == QSEventAndAction::OPCODE_BACK || lastSecondInfo.navigationEvent == QSEventAndAction::OPCODE_MENU))
            return true;

        return false;
    }

    string MainTest::CheckLoginResult()
    {
        string loginResult = "SKIP";
        boost::filesystem::path beforeLoginImage(TestReport::currentQsLogPath + "/beforeLoginImage.png");
        boost::filesystem::path afterLoginImage(TestReport::currentQsLogPath + "/afterLoginImage.png");

        if (!boost::filesystem::is_regular_file(beforeLoginImage) || !boost::filesystem::is_regular_file(afterLoginImage))
        {
            return loginResult;
        }
        Mat frame = imread(afterLoginImage.string());
        exploreEngine->generateAllRectsAndKeywords(frame, Rect(0, 0, frame.size().width, frame.size().height), languageMode);

        bool loginFailCenter = this->exploreEngine->JudgeLoginFailByKeyword(languageMode, exploreEngine->getAllEnKeywords(), exploreEngine->getAllZhKeywords());
        bool loginFailFullPage = this->exploreEngine->JudgeLoginFailOnFullPart(beforeLoginImage, afterLoginImage);
        if (loginFailCenter == true || loginFailFullPage == true)
        {
            loginResult = "FAIL";
        }
        else
        {
            loginResult = "PASS";
        }
        return loginResult;
    }

    void MainTest::ProcessWave(const boost::shared_ptr<vector<short>> &samples)
    {
        boost::lock_guard<boost::mutex> lock(audioMutex);
        if(audioSaving)
        {
            if (audioFile == nullptr)
            {
                audioFile = AudioFile::CreateWriteWavFile(audioFileName);
            }
            else
            {
                sf_write_short(audioFile->Handle(), &(*samples)[0], samples->size());
            }
        }

    }

    void MainTest::AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction> qEvent, bool updateTime)
    {

        qEvent->LineNumber() = generatedGetRnRQScript->GetUnusedLineNumber();

        if(updateTime)
        {
            qEvent->TimeStamp() = stopWatch->ElapsedMilliseconds(); 
            qEvent->TimeStampReplay() = stopWatch->ElapsedMilliseconds();
        }        

        generatedGetRnRQScript->AppendEventAndAction(qEvent);
        generatedGetRnRQScript->Trace().push_back(QScript::TraceInfo(qEvent->LineNumber(), qEvent->TimeStampReplay()));
    }

    void MainTest::PressBack()
    {
        dut->PressBack();        
        AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_BACK, "",  dut->GetCurrentOrientation(false))));   
    }

    void MainTest::PressHome()
    {
        dut->PressHome();        
        AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_HOME, "",  dut->GetCurrentOrientation(false))));   
    }

    void MainTest::PressMenu()
    {
        dut->PressMenu();        
        AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_MENU, "",  dut->GetCurrentOrientation(false))));   
    }
}
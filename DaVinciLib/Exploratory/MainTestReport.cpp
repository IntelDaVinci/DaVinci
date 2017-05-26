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

#include "MainTestReport.hpp"
#include "VideoCaptureProxy.hpp"
#include "FlickeringChecker.hpp"
#include "BlankPagesChecker.hpp"
#include "AudioFormGenerator.hpp"
#include "boost/algorithm/string.hpp"
#include "QReplayInfo.hpp"
#include "OnDeviceScriptRecorder.hpp"

using namespace std;
using namespace cv;

namespace DaVinci
{
    MainTestReport::MainTestReport(string pn, string reportPath)
    {
        packageName = pn;
        xmlReportPath = reportPath;

        testReport = boost::shared_ptr<TestReport>(new TestReport());
        smokeTest = boost::shared_ptr<Smoke>(new Smoke());
        xmlReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());

        screenFrozenChecker = boost::shared_ptr<ScreenFrozenChecker>(new ScreenFrozenChecker());

        keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        keywordCheckHandler1 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForCrashANR(keywordRecog));
        keywordCheckHandler2 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForCrashANR(keywordRecog, false));
        keywordCheckHandler3 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForNetworkIssue(keywordRecog));
        keywordCheckHandler4 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForInitializationIssue(keywordRecog));
        keywordCheckHandler5 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForAbnormalFont(keywordRecog));
        keywordCheckHandler6 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForErrorDialog(keywordRecog));
        keywordCheckHandler7 = boost::shared_ptr<KeywordCheckHandler>(new KeywordCheckHandlerForLocationIssue(keywordRecog));
        keywordCheckHandler1->SetSuccessor(keywordCheckHandler2);
        keywordCheckHandler2->SetSuccessor(keywordCheckHandler3);
        keywordCheckHandler3->SetSuccessor(keywordCheckHandler4);
        keywordCheckHandler4->SetSuccessor(keywordCheckHandler5);
        keywordCheckHandler5->SetSuccessor(keywordCheckHandler6);
        keywordCheckHandler6->SetSuccessor(keywordCheckHandler7);

        for(int i = 0; i < 5; i++)
        {
            stageResults[i] = SmokeResult::PASS;
        }

        imageCheckPass = true;
        launchFail = false;
        matchedActions =  0;
        passrate = 0;
        processExit = false;
    }

    FailureType MainTestReport::DetectFrameIssue(Mat targetFrame, Rect& okRect, Mat originalFrame, const Rect navigationBarRect, const Rect statusBarRect)
    {
        okRect = EmptyRect;
        cv::Mat weightedFrame = AddWeighted(targetFrame);
        std::vector<cv::Rect> proposalRects = ObjectUtil::Instance().DetectRectsByContour(weightedFrame, 240, 6, 1, 0.5, 50, 400, 35000);
        keywordRecog->SetProposalRects(proposalRects, true);

        std::vector<Keyword> enTexts = keywordRecog->DetectAllKeywords(targetFrame, "en");
        std::vector<Keyword> zhTexts = keywordRecog->DetectAllKeywords(targetFrame, "zh");

        boost::shared_ptr<KeywordCheckHandler> handler = keywordCheckHandler1->HandleRequest(languageMode, enTexts, zhTexts);
        if (handler != nullptr)
        {
            okRect = handler->GetOkRect(targetFrame);
            return handler->GetCurrentFailureType();
        }

        if (ObjectUtil::Instance().IsPureSingleColor(originalFrame, navigationBarRect, statusBarRect, true))
        {
            return FailureType::WarMsgAbnormalPage;
        }

        return FailureType::WarMsgMax;
    }

    Mat MainTestReport::ReadFrameFromResources(string qsName, int startIp, bool isRecord)
    {
        boost::filesystem::path resourceImage;
        if(isRecord)
        {
            resourceImage = recordFolder;
            resourceImage /= ("record_" + qsName + "_" + boost::lexical_cast<string>(startIp) + "_reference.png");
        }
        else
        {
            resourceImage = replayFolder;
            resourceImage /= ("replay_" + qsName + "_" + boost::lexical_cast<string>(startIp) + "_reference.png");
        }

        Mat resourceFrame = cv::imread(resourceImage.string());
        return resourceFrame;
    }

    int MainTestReport::DetectWarningAbnormalPage(vector<FailurePair> pairs, int condition)
    {
        int pairSize = (int)pairs.size();

        if(pairSize == 0 || condition == 0)
        {
            return -1;
        }

        if(pairSize < condition)
        {
            return -1;
        }

        FailurePair startPair, endPair;
        for(int i = 0; i <= pairSize - condition; i++)
        {
            startPair = pairs[i];
            endPair = pairs[i + condition - 1];
            if(endPair.index - startPair.index == condition - 1)
                return endPair.index;
        }

        return -1;
    }

    FailureType MainTestReport::DetectWarningIssues(vector<FailurePair> pairs, int &failureIndex)
    {
        if(pairs.size() == 0)
            return FailureType::WarMsgMax;

        FailureType currentFailureType = FailureType::WarMsgAbnormalPage;
        vector<FailurePair> currentWarningPairs;

        if(MapContainKey(currentFailureType, checkers) && checkers[currentFailureType].enabled)
        {
            for(auto pair : pairs)
            {
                if(pair.type == currentFailureType)
                    currentWarningPairs.push_back(pair);
            }

            failureIndex = DetectWarningAbnormalPage(currentWarningPairs, checkers[currentFailureType].condition);
            if(failureIndex != -1)
            {
                return currentFailureType;
            }
        }

        // Warning order (abnormal page, network issue, initialization, error dialog and abnormal font)
        currentFailureType = FailureType::WarMsgNetwork;
        currentWarningPairs.clear();

        if(MapContainKey(currentFailureType, checkers) && checkers[currentFailureType].enabled)
        {
            for(auto pair : pairs)
            {
                if(pair.type == currentFailureType)
                    currentWarningPairs.push_back(pair);
            }

            if(currentWarningPairs.size() > 0)
            {
                failureIndex = currentWarningPairs[0].index;
                return currentFailureType;
            }
        }

        currentFailureType = FailureType::WarMsgErrorDialog;
        currentWarningPairs.clear();

        if(MapContainKey(currentFailureType, checkers) && checkers[currentFailureType].enabled)
        {
            for(auto pair : pairs)
            {
                if(pair.type == currentFailureType)
                    currentWarningPairs.push_back(pair);
            }

            if(currentWarningPairs.size() > 0)
            {
                failureIndex = currentWarningPairs[0].index;
                return currentFailureType;
            }
        }

        currentFailureType = FailureType::WarMsgAbnormalFont;
        currentWarningPairs.clear();

        if(MapContainKey(currentFailureType, checkers) && checkers[currentFailureType].enabled)
        {
            for(auto pair : pairs)
            {
                if(pair.type == currentFailureType)
                    currentWarningPairs.push_back(pair);
            }

            if(currentWarningPairs.size() > 0)
            {
                failureIndex = currentWarningPairs[0].index;
                return currentFailureType;
            }
        }

        return FailureType::WarMsgMax;
    }

    void MainTestReport::GenerateNavigationStatusBar(Mat replayFrame, Orientation orientation, Rect& navigationBar, Rect& statusBar)
    {
        int navigationBarHeight = min(navigationBar.height, navigationBar.width);
        int statusBarHeight = min(statusBar.height, statusBar.width);

        if (navigationBarHeight == 0)
            navigationBarHeight = 64;
        if (statusBarHeight == 0)
            statusBarHeight = 33;

        cv::Size frameSize = replayFrame.size();

        switch(orientation)
        {
        case(Orientation::Portrait):
            {
                navigationBar = Rect(frameSize.width - navigationBarHeight, 0, navigationBarHeight, frameSize.height);
                statusBar = Rect(0, 0, statusBarHeight, frameSize.height);
                break;
            }
        case(Orientation::Landscape):
            {
                navigationBar = Rect(0, frameSize.height - navigationBarHeight, frameSize.width, navigationBarHeight);
                statusBar = Rect(0, 0, frameSize.width, statusBarHeight);
                break;
            }
        case(Orientation::ReversePortrait):
            {
                navigationBar = Rect(0, 0, navigationBarHeight, frameSize.height);
                statusBar = Rect(frameSize.width - statusBarHeight, 0, statusBarHeight, frameSize.height);
                break;
            }
        case(Orientation::ReverseLandscape):
            {
                navigationBar = Rect(0, 0, frameSize.width, navigationBarHeight);
                statusBar = Rect(0, frameSize.height - statusBarHeight, frameSize.width, statusBarHeight);
                break;
            }
        }
    }

    FailureType MainTestReport::DetectFrameIssueFromReplayPairs(vector<QSIpPair>& replayIpSequence, Rect& okRect, int& failureIndex)
    {
        DAVINCI_LOG_INFO << "Detect frame issue from replay frame pairs ..." << endl;

        FailureType failureType = FailureType::WarMsgMax;
        failureIndex = -1;
        okRect = EmptyRect;
        vector<FailurePair> warningPairs;

        Mat replayFrame, rotatedReplayFrame;
        Orientation replayFrameOrientation;
        Rect navigationBar = EmptyRect;
        Rect statusBar = EmptyRect;

        if (dut != nullptr)
        {
            auto androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
            if (androidDut != nullptr)
            {
                std::vector<std::string> windowLines;
                AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
                statusBar = windowBarInfo.statusbarRect;
                navigationBar = windowBarInfo.navigationbarRect;
            }
        }

        for(int i = 0; i < (int)replayIpSequence.size(); i++)
        {
            QSIpPair replayPair = replayIpSequence[i];
            if((!scriptPreprocessInfos[replayPair.qsName][replayPair.startIp].isTouchAction) && (!replayPair.random))
                continue;

            replayFrame = ReadFrameFromResources(replayPair.qsName, replayPair.startIp, false);
            if(replayFrame.empty())
                continue;

            // To be used for first and last valid image
            if(replayPair.random || replayPair.touch)
                validFramePairs.push_back(replayPair);

            DAVINCI_LOG_INFO << "Checking frame pair @" +  replayPair.qsName + " #" + boost::lexical_cast<string>(replayPair.startIp) << " ... " << endl;
            replayFrameOrientation = replayPair.orientation;
            GenerateNavigationStatusBar(replayFrame, replayFrameOrientation, navigationBar, statusBar);
            rotatedReplayFrame = RotateFrameUp(replayFrame, replayFrameOrientation);

            failureType = DetectFrameIssue(rotatedReplayFrame, okRect, replayFrame, navigationBar, statusBar);

            if(failureType != FailureType::WarMsgMax)
            {
                if(IsErrorFailureTypeForFrame(failureType))
                {
                    failureIndex = i;
                    DAVINCI_LOG_INFO << "Detecting frame issue @" << replayPair.qsName << " #" << replayPair.startIp << ": " << FailureTypeToString(failureType) << endl;
                    return failureType;
                }
                else
                {
                    warningPairs.push_back(FailurePair(i, replayPair.qsName, replayPair.startIp, failureType));
                }
            }
        }

        FailureType warningType = DetectWarningIssues(warningPairs, failureIndex);
        return warningType;
    }

    vector<string> MainTestReport::GetFirstLastImages(vector<QSIpPair> validFramePairs, QSIpPair& firstPair, QSIpPair& lastPair)
    {
        vector<string> images;
        int pairSize = (int)validFramePairs.size();
        string firstImage, lastImage;
        if(pairSize > 0)
        {
            firstPair = validFramePairs[0];
            firstImage = ConcatPath(replayFolder.string(), "replay_" + firstPair.qsName + "_" + boost::lexical_cast<string>(firstPair.startIp) + "_reference.png");
            images.push_back(firstImage);

            if(pairSize > 1)
            {
                lastPair = validFramePairs[pairSize - 1];
                lastImage = ConcatPath(replayFolder.string(), "replay_" + lastPair.qsName + "_" + boost::lexical_cast<string>(lastPair.startIp) + "_reference.png");
                images.push_back(lastImage);
            }
        }

        return images;
    }

    vector<QSIpPair> MainTestReport::GetMatchedTouchPairs()
    {
        Mat targetFrame;
        vector<QSIpPair> matchedFramePairs;
        for(int i = 0; i < (int)replayIpSequence.size(); i++)
        {
            QSIpPair replayQSIpPair = replayIpSequence[i];
            if(replayQSIpPair.random)
                continue;

            boost::shared_ptr<QSEventAndAction> qsEvent = scripts[replayQSIpPair.qsName]->EventAndAction(replayQSIpPair.startIp);

            // Skip non-touch opcodes (treat drag as touch)
            if(!QScript::IsOpcodeTouch(qsEvent->Opcode()) && qsEvent->Opcode() != QSEventAndAction::OPCODE_DRAG)
                continue;

            boost::filesystem::path resourcePath = replayFolder;
            resourcePath /= boost::filesystem::path("replay_" + replayQSIpPair.qsName + "_" + boost::lexical_cast<string>(replayQSIpPair.startIp) + "_reference.png");
            targetFrame = imread(resourcePath.string());

            if(targetFrame.empty())
                continue;

            matchedFramePairs.push_back(replayQSIpPair);
        }

        return matchedFramePairs;
    }

    bool MainTestReport::ContainQSPairIp(vector<QSIpPair> ips, QSIpPair ip)
    {
        for(auto i: ips)
        {
            if(i.qsName == ip.qsName && i.startIp == ip.startIp)
                return true;
        }

        return false;
    }

    void MainTestReport::GenerateCriticalPathCoverage()
    {
        matchedActions = (int)replayIpSequence.size();
        matchedTouchFramePairs = GetMatchedTouchPairs();

        int recordedActionNumber = (int)recordIpSequence.size();
        if(recordedActionNumber == 0)
        {
            matchedActions = recordedActionNumber;
            passrate = 0.0; // Mark it 0.0% if no record actions
        }
        else
        {
            if(recordedActionNumber <= matchedActions)
            {
                passrate = 100.0; // Some actions are clicked off by pop up/advertisement (should exclude)
                matchedActions = recordedActionNumber;
            }
            else
            {
                passrate = matchedActions / (recordedActionNumber + 0.0) * 100;
            }
        }
        passrate = floor(passrate + 0.5);
        DAVINCI_LOG_INFO << "###PASS RATE: [" << boost::lexical_cast<string>(passrate) << "%][" 
            << boost::lexical_cast<string>(matchedActions) 
            << "/" << boost::lexical_cast<string>(recordedActionNumber) << "]" << endl;
    }

    void MainTestReport::SaveFrameForTouchPairs(int stage, int framePairIndex)
    {
        vector<QSIpPair> saveFramePairs;
        if (matchedTouchFramePairs.size() > 0)
        {
            saveFramePairs = matchedTouchFramePairs;
        }
        else
        {
            for (auto replaySequence: replayIpSequence)             // no matched pairs, save the frame with unknowQsName
            {
                if (replaySequence.qsName == unknownQsName)
                {
                    saveFramePairs.push_back(replaySequence);
                }
            }
        }

        if(saveFramePairs.size() > 0)
        {
            if(framePairIndex >= 0 && framePairIndex < (int)saveFramePairs.size())
            {
                QSIpPair pair = saveFramePairs[framePairIndex];
                Mat launchFrame = ReadFrameFromResources(pair.qsName, pair.startIp, false);
                string stageResource;
                if(stage == SmokeStage::SMOKE_LAUNCH)
                    stageResource = std::string("apk_launch_") + packageName + std::string(".png");
                else if(stage == SmokeStage::SMOKE_RANDOM)
                    stageResource = std::string("apk_random_") + packageName + std::string(".png");
                else
                    stageResource = std::string("apk_back_") + packageName + std::string(".png");

                string imageName = ConcatPath(TestReport::currentQsLogPath, stageResource);
                SaveImage(launchFrame, imageName);
            }
            else
            {
                string imageName = TestReport::currentQsLogPath + std::string("\\apk_random_") + packageName + std::string(".png");
                SaveImage(crashFrame, imageName);
            }
        }
    }

    void MainTestReport::SaveFrameForNonTouchPairs(int stage, QSIpPair pair)
    {
        Mat launchFrame = ReadFrameFromResources(pair.qsName, pair.startIp, false);
        string stageResource;
        if(stage == SmokeStage::SMOKE_LAUNCH)
            stageResource = std::string("apk_launch_") + packageName + std::string(".png");
        else if(stage == SmokeStage::SMOKE_RANDOM)
            stageResource = std::string("apk_random_") + packageName + std::string(".png");
        else
            stageResource = std::string("apk_back_") + packageName + std::string(".png");

        string imageName = ConcatPath(TestReport::currentQsLogPath, stageResource);
        SaveImage(launchFrame, imageName);
    }


    vector<QSIpPair> MainTestReport::DetectValidReplayIpSequence()
    {
        vector<QSIpPair> validIpSequence;
        for(auto pair: replayIpSequence)
        {
            Mat frame = ReadFrameFromResources(pair.qsName, pair.startIp, false);
            if(!frame.empty())
                validIpSequence.push_back(pair);
        }

        return validIpSequence;
    }

    void MainTestReport::SaveFrameForLaunchFail()
    {
        string stageResource = std::string("apk_launch_") + packageName + std::string(".png");
        string imageName = ConcatPath(TestReport::currentQsLogPath, stageResource);
        SaveImage(crashFrame, imageName);
    }

    vector<QSIpPair> MainTestReport::DetectBackActions(vector<QSIpPair> replayPairs)
    {
        vector<QSIpPair> backPairs;
        for(auto pair: replayPairs)
        {
            if(pair.random)
                continue;

            if(scripts[pair.qsName]->EventAndAction(pair.startIp)->Opcode() == QSEventAndAction::OPCODE_BACK 
                || scriptPreprocessInfos[pair.qsName][pair.startIp].navigationEvent == QSEventAndAction::OPCODE_BACK)
                backPairs.push_back(pair);
        }

        return backPairs;
    }

    bool MainTestReport::IsFailureFromBackAction(vector<QSIpPair> backPairs, QSIpPair failurePair)
    {
        for(auto pair : backPairs)
        {
            if(pair.IsSameQSIpPair(failurePair))
                return true;
        }

        return false;
    }

    void MainTestReport::SaveNonTouchReplayFrame(Mat frame, int opcode, string qsFilename, int startIp)
    {
        boost::filesystem::path replayImage;
        replayImage = replayFolder / boost::filesystem::path("replay_"+ qsFilename + "_" + boost::lexical_cast<string>(startIp) + "_reference.png");
        SaveImage(frame, replayImage.string());

        // When recording clicks virtual navigation bar, preprocessing will transform to event navigation, save the frame based on file format (no click point draw)
        if(scriptPreprocessInfos[qsFilename][startIp].isVirtualNavigation)
        {
            replayImage = replayFolder / boost::filesystem::path("replay_"+ qsFilename + "_" + boost::lexical_cast<string>(startIp) + "_reference_clickPoint.png");
            SaveImage(frame, replayImage.string());
        }

#ifdef _DEBUG
        replayImage = replayFolder / boost::filesystem::path("replay_"+ qsFilename + "_" + boost::lexical_cast<string>(startIp) + "_reference_" + QScript::OpcodeToString(opcode) + ".png");
        SaveImage(frame, replayImage.string());
#endif
    }


    void MainTestReport::SaveRandomReplayFrame(Mat frame, int randomIndex, bool isClick)
    {
        boost::filesystem::path replayImage;
        if(isClick)
            replayImage = replayFolder / boost::filesystem::path("replay_" + unknownQsName + "_" + boost::lexical_cast<string>(randomIndex) + "_reference_clickPoint.png");
        else
            replayImage = replayFolder / boost::filesystem::path("replay_" + unknownQsName + "_" + boost::lexical_cast<string>(randomIndex) + "_reference.png");
        SaveImage(frame, replayImage.string());
    }

    void MainTestReport::SaveRandomMainReplayFrame(Mat frame, int randomIndex)
    {
        boost::filesystem::path replayImage;
        replayImage = replayFolder / boost::filesystem::path("replay_" + unknownQsName + "_" + boost::lexical_cast<string>(randomIndex) + "_reference_focus.png");
        SaveImage(frame, replayImage.string());
    }

    void MainTestReport::SaveReferenceImge(int stage)
    {
        boost::filesystem::path resourceImage = replayFolder;
        resourceImage /= ("replay_reference_rotated.png");
        Mat resourceFrame = cv::imread(resourceImage.string());

        string stageResource;
        if(stage == SmokeStage::SMOKE_LAUNCH)
            stageResource = std::string("apk_launch_") + packageName + std::string(".png");
        else if(stage == SmokeStage::SMOKE_RANDOM)
            stageResource = std::string("apk_random_") + packageName + std::string(".png");
        else
            stageResource = std::string("apk_back_") + packageName + std::string(".png");

        string imageName = ConcatPath(TestReport::currentQsLogPath, stageResource);
        SaveImage(resourceFrame, imageName);
    }

    void MainTestReport::GenerateLogcat()
    {
        testReport->GetLogcat(packageName, "apk_logcat_" + packageName + ".txt");
    }

    FailureType MainTestReport::ClickOffErrorDialog(Mat frame, FailureType failureType)
    {
        Orientation orientation = Orientation::Portrait;
        if(dut != nullptr)
            orientation = dut->GetCurrentOrientation(true);
        Mat rotatedFrame = RotateFrameUp(frame, orientation);
        Rect okRect;

        Rect navigationBar = EmptyRect;
        Rect statusBar = EmptyRect;
        GenerateNavigationStatusBar(frame, orientation, navigationBar, statusBar);

        FailureType frameFailureType = DetectFrameIssue(rotatedFrame, okRect, frame, navigationBar, statusBar);
        if(IsErrorFailureTypeForFrame(frameFailureType))
        {
            exploreEngine->dispatchClickOffErrorDialogAction(rotatedFrame, Point(okRect.x + okRect.width / 2, okRect.y + okRect.height / 2));
            return frameFailureType;
        }

        return failureType;
    }

    bool MainTestReport::DetectApplicationFailureOnInstall()
    {
        std::string installLogFile = ConcatPath(TestReport::currentQsLogPath, std::string("apk_install_") + packageName + std::string(".txt"));
        GenerateInstallStageResult(installLogFile);

        if(stageResults[SmokeStage::SMOKE_INSTALL] == SmokeResult::FAIL)
        {
            stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

            failureReason = FailureTypeToString(FailureType::ErrMsgInstallFail);
            DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
            return false;
        }

        return true;
    }

    bool MainTestReport::DetectApplicationFailureOnImageCheck()
    {
        if(!imageCheckPass)
        {
            stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::PASS;
            stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::FAIL;
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

            SaveFrameForTouchPairs(SmokeStage::SMOKE_LAUNCH, 0);
            SaveFrameForTouchPairs(SmokeStage::SMOKE_RANDOM, -1);

            failureReason = FailureTypeToString(FailureType::ErrMsgScriptCheck);
            DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
            return false;
        }

        return true;
    }

    bool MainTestReport::DetectApplicationFailureOnLaunch()
    {
        if(launchFail)
        {
            stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::FAIL;
            stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

            SaveFrameForLaunchFail();

            FailureType currentFailureType = ClickOffErrorDialog(crashFrame, FailureType::ErrMsgLaunchFail);
            failureReason = FailureTypeToString(currentFailureType);

            DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
            DAVINCI_LOG_INFO << "###Failure Type: " << failureReason;
            return false;
        }

        return true;
    }

    bool MainTestReport::DetectApplicationFailureOnProcessExit()
    {
        if(processExit)
        {
            stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::PASS;
            stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::FAIL;
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

            SaveFrameForTouchPairs(SmokeStage::SMOKE_LAUNCH, 0);
            SaveFrameForTouchPairs(SmokeStage::SMOKE_RANDOM, -1);

            FailureType currentFailureType = ClickOffErrorDialog(crashFrame, FailureType::ErrMsgProcessExit);
            failureReason = FailureTypeToString(currentFailureType);

            DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
            DAVINCI_LOG_INFO << "###Failure Type: " << failureReason;
            return false;
        }

        return true;
    }

    bool MainTestReport::DetectAlwaysLoadingFailure(vector<QSIpPair> pairs, QSIpPair& firstPair, QSIpPair& lastPair)
    {
        vector<string> images = GetFirstLastImages(pairs, firstPair, lastPair);

        if(images.size() <= 1)
            return false;

        if(boost::empty(images[0]) || boost::empty(images[1]))
            return false;

        if(boost::equals(images[0], images[1]))
            return false;

        // Always loading has higher priority than other warning types
        return smokeTest->isAppAlwaysLoading(images[0], images[1]);
    }

    bool MainTestReport::DetectApplicationFailureCore()
    {
        // Detect frame issue from replay frame pairs
        Rect okRect;
        int failureIndex;
        FailureType failureType = DetectFrameIssueFromReplayPairs(replayIpSequence, okRect, failureIndex);
        backPairs = DetectBackActions(replayIpSequence);

        if(!IsErrorFailureTypeForFrame(failureType))
        {
            FailureType currentFailureType = FailureType::WarMsgAlwaysLoading;
            if(MapContainKey(currentFailureType, checkers) && checkers[currentFailureType].enabled)
            {
                QSIpPair firstPair, lastPair;
                if(DetectAlwaysLoadingFailure(validFramePairs, firstPair, lastPair))
                {
                    // App hang warning has higher priority than other warnings
                    failureType = currentFailureType; 
                    failureReason = FailureTypeToString(failureType);

                    stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::PASS;
                    stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::WARNING;
                    stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
                    stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

                    SaveFrameForNonTouchPairs(SmokeStage::SMOKE_LAUNCH, firstPair);
                    SaveFrameForNonTouchPairs(SmokeStage::SMOKE_RANDOM, lastPair);

                    DAVINCI_LOG_INFO << "###Final Result: WARNING" << endl;
                    DAVINCI_LOG_INFO << "###Warning Type: " << FailureTypeToString(failureType);
                    return false;
                }
            }
        }

        if(failureType != FailureType::WarMsgMax)
        {
            assert(failureIndex >= 0);
            QSIpPair failurePair = replayIpSequence[failureIndex];

            stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::PASS;
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;
            failureReason = FailureTypeToString(failureType);

            if(matchedTouchFramePairs.size() > 0)
            {
                SaveFrameForTouchPairs(SmokeStage::SMOKE_LAUNCH, 0);
            }
            else
            {
                vector<QSIpPair> validIpSequence = DetectValidReplayIpSequence();
                if(validIpSequence.size() > 0)
                {
                    SaveFrameForNonTouchPairs(SmokeStage::SMOKE_LAUNCH, validIpSequence[0]);
                }
                else
                {
                    SaveReferenceImge(SmokeStage::SMOKE_LAUNCH);
                }
            }

            if(backPairs.size() == 0)
            {
                if(IsErrorFailureTypeForFrame(failureType))
                {
                    stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::FAIL;
                }
                else
                {
                    stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::WARNING;
                }
                stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
                SaveFrameForNonTouchPairs(SmokeStage::SMOKE_RANDOM, failurePair);
            }
            else
            {
                bool isBackFailure = IsFailureFromBackAction(backPairs, failurePair);
                if(isBackFailure)
                {
                    stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::PASS;
                    SaveFrameForTouchPairs(SmokeStage::SMOKE_RANDOM, (int)(matchedTouchFramePairs.size()) - 1);

                    if(IsErrorFailureTypeForFrame(failureType))
                    {
                        stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::FAIL;
                    }
                    else
                    {
                        stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::WARNING;
                    }

                    SaveFrameForNonTouchPairs(SmokeStage::SMOKE_BACK, failurePair);
                }
                else
                {
                    if(IsErrorFailureTypeForFrame(failureType))
                    {
                        stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::FAIL;
                    }
                    else
                    {
                        stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::WARNING;
                    }

                    stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
                    SaveFrameForNonTouchPairs(SmokeStage::SMOKE_RANDOM, failurePair);
                }
            }

            if(IsErrorFailureTypeForFrame(failureType))
            {
                DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
                DAVINCI_LOG_INFO << "###Failure Type: " << FailureTypeToString(failureType);
                Orientation orientation = Orientation::Portrait;
                if(dut != nullptr)
                    orientation = dut->GetCurrentOrientation(true);
                Mat rotatedFrame = RotateFrameUp(crashFrame, orientation);
                exploreEngine->dispatchClickOffErrorDialogAction(rotatedFrame, Point(okRect.x + okRect.width / 2, okRect.y + okRect.height / 2));
            }
            else // if(IsWarningFailureTypeForFrame(failureType))
            {
                DAVINCI_LOG_INFO << "###Final Result: WARNING" << endl;
                DAVINCI_LOG_INFO << "###Warning Type: " << FailureTypeToString(failureType);
            }

            return false;
        }

        return true;
    }

    int MainTestReport::FindSuccessiveBackPairForReport(vector<QSIpPair> validIpSequence, QSIpPair backPair)
    { 
        int size = (int)validIpSequence.size();
        for(int index = 0; index < size; index++)
        {
            if(backPair.IsSameQSIpPair(validIpSequence[index]))
            {
                return index;
            }
        }

        return -1;
    }

    bool MainTestReport::DetectApplicationFailureOnUninstall()
    { 
        std::string uninstallLogFile = ConcatPath(TestReport::currentQsLogPath, std::string("apk_uninstall_") + packageName + std::string(".txt"));
        GenerateUninstallStageResult(uninstallLogFile);

        vector<QSIpPair> validIpSequence = DetectValidReplayIpSequence();
        stageResults[SmokeStage::SMOKE_LAUNCH] = SmokeResult::PASS;
        stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::PASS;
        if(backPairs.size() == 0)
        {
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::SKIP;
        }
        else
        {
            stageResults[SmokeStage::SMOKE_BACK] = SmokeResult::PASS;
            int index = FindSuccessiveBackPairForReport(validIpSequence, backPairs[0]);
            if(index == -1 || index == ((int)validIpSequence.size() - 1))
                SaveReferenceImge(SmokeStage::SMOKE_BACK);
            else
            {
                QSIpPair successivePair = validIpSequence[index + 1];
                SaveFrameForNonTouchPairs(SmokeStage::SMOKE_BACK, successivePair);
            }
        }

        if(matchedTouchFramePairs.size() > 0)
        {
            SaveFrameForTouchPairs(SmokeStage::SMOKE_LAUNCH, 0);
            SaveFrameForTouchPairs(SmokeStage::SMOKE_RANDOM, (int)(matchedTouchFramePairs.size()) - 1);
        }
        else
        {
            if(validIpSequence.size() > 0)
            {
                SaveLaunchFrame(validIpSequence);
                SaveRandomFrame(validIpSequence);
            }
            else
            {
                stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::SKIP;
                SaveReferenceImge(SmokeStage::SMOKE_LAUNCH);
            }
        }

        if(stageResults[SmokeStage::SMOKE_UNINSTALL] == SmokeResult::FAIL)
        {
            if (failureReason.empty())
                failureReason = FailureTypeToString(FailureType::ErrMsgUninstallFail);
            DAVINCI_LOG_INFO << "###Final Result: FAIL" << endl;
            return false;
        }

        return true;
    }

    void MainTestReport::SaveRandomFrame(vector<QSIpPair> validIpSequence)
    {
        bool findRandomImage = false;
        for (auto ipsequence: validIpSequence)      // Check opcode for each valid IP sequence, if opcode is touch event, save it
        {
            boost::shared_ptr<QSEventAndAction> qsEvent = scripts[ipsequence.qsName]->EventAndAction(ipsequence.startIp);

            if(QScript::IsOpcodeTouch(qsEvent->Opcode()) || qsEvent->Opcode() == QSEventAndAction::OPCODE_DRAG)
            {
                findRandomImage = true;
                SaveFrameForNonTouchPairs(SmokeStage::SMOKE_RANDOM, ipsequence);
            }
        }
        if (findRandomImage == false)               // Couldn't find touch event, random stage is SKIP
            stageResults[SmokeStage::SMOKE_RANDOM] = SmokeResult::SKIP;
    }


    void MainTestReport::SaveLaunchFrame(vector<QSIpPair> validIpSequence)
    {
        bool findStartImage = false;
        for (auto ipsequence: validIpSequence)      // Check opcode for each valid IP sequence, if opcode is OPCODE_START_APP, save it
        {
            boost::shared_ptr<QScript> script = scripts[ipsequence.qsName];
            boost::shared_ptr<QSEventAndAction> qsEvent = script->EventAndAction(ipsequence.startIp);
            int opcode = qsEvent->Opcode();
            if (opcode == QSEventAndAction::OPCODE_START_APP)
            {
                findStartImage = true;
                SaveFrameForNonTouchPairs(SmokeStage::SMOKE_LAUNCH, ipsequence);
                break;
            }
        }
        if (findStartImage == false)                // Couldn't find OPCODE_START_APP, save the first image
            SaveFrameForNonTouchPairs(SmokeStage::SMOKE_LAUNCH, validIpSequence[0]);
    }

    void MainTestReport::ReportRecordCoverge(bool isCaseFail)
    {
        if(isCaseFail)
            return;

        if(FEquals(passrate, 100.0))
        {
            DAVINCI_LOG_INFO << "###Final Result: PASS" << endl; 
        }
        else
        {
            DAVINCI_LOG_INFO << "###Final Result: WARNING" << endl;
        }
    }

    bool MainTestReport::DetectApplicationFailures()
    {
        if(!DetectApplicationFailureOnInstall())
            return true;

        if(!DetectApplicationFailureOnImageCheck())
            return true;

        if(!DetectApplicationFailureOnLaunch())
            return true;

        if(!DetectApplicationFailureOnProcessExit())
            return true;

        if(!DetectApplicationFailureCore())
            return true;

        if(!DetectApplicationFailureOnUninstall())
            return true;

        if(!CheckReferenceFrames())
            return true;

        if(!CheckVideoBlankPage())
            return true;

        if(!CheckVideoFlickering())
            return true;

        if (!CheckAudio())
            return true;

        return false;
    }

    void MainTestReport::GenerateInstallStageResult(string installLogFile)
    {
        if (boost::filesystem::is_regular_file(installLogFile))
            stageResults[SmokeStage::SMOKE_INSTALL] = smokeTest->checkInstallLog(installLogFile) ? SmokeResult::PASS : SmokeResult::FAIL;
        else
            stageResults[SmokeStage::SMOKE_INSTALL] = SmokeResult::SKIP;

        if (stageResults[SmokeStage::SMOKE_INSTALL] == SmokeResult::FAIL)
        {
            // reset 'Device Incompatible' and 'version downgrade' as warning
            if (smokeTest->GetFailReason() == FailureTypeToString(WarMsgDeviceIncompatible))
            {
                failureReason = FailureTypeToString(FailureType::WarMsgDeviceIncompatible);
                stageResults[SmokeStage::SMOKE_INSTALL] = SmokeResult::WARNING;
            }
            else if (smokeTest->GetFailReason() == FailureTypeToString(WarMsgDowngrade))
            {
                failureReason = FailureTypeToString(FailureType::WarMsgDowngrade);
                stageResults[SmokeStage::SMOKE_INSTALL] = SmokeResult::WARNING;
            }
            else
            {
                // double check if app is installed using "shell pm list packages"
                vector<string> installedPackage = dut->GetAllInstalledPackageNames();
                if (std::find(installedPackage.begin(), installedPackage.end(), packageName) != installedPackage.end())
                {
                    stageResults[SmokeStage::SMOKE_INSTALL] = SmokeResult::PASS; // if app is installed, keep testing
                }
                else
                {
                    failureReason = FailureTypeToString(FailureType::ErrMsgInstallFail);
                }
            }
        }
    }

    void MainTestReport::GenerateUninstallStageResult(string uninstallLogFile)
    {
        if (boost::filesystem::is_regular_file(uninstallLogFile))
            stageResults[SmokeStage::SMOKE_UNINSTALL] = smokeTest->checkUninstallLog(uninstallLogFile) ? SmokeResult::PASS : SmokeResult::FAIL;
        else
            stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::SKIP;

        if(stageResults[SmokeStage::SMOKE_UNINSTALL] == SmokeResult::FAIL)
        {
            failureReason = FailureTypeToString(FailureType::ErrMsgUninstallFail);
        }
        else
        {
            if(screenFrozenChecker->Check() == Fail)
            {
                stageResults[SmokeStage::SMOKE_UNINSTALL] = SmokeResult::FAIL;
                failureReason = FailureTypeToString(FailureType::ErrMsgScreenFrozen);
            }
        }
    }

    bool IsInAnimation(boost::shared_ptr<QScript> qs, int startIP, boost::shared_ptr<VideoCaptureProxy> refVideo,
        Mat& mask)
    {
        if (!refVideo->isOpened())
            return true;

        // parameters for GoodFeaturesToTrack
        static const int maxFeaturesPerChannel = 5000;
        double qualityLevel(0.01);
        double minDistance(10.0);

        // parameters for calcOpticalFlowPyrLK
        static const int maxLevel = 3;
        Size winSize(31, 31);
        TermCriteria criteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01);

        bool ret = false;
        vector<vector<Point2f>> corners(2);
        Mat prevImg;
        auto evt = qs->EventAndAction(startIP);
        if (evt->NumStrOperands() == 0)
            return true;
        Rect sb, nb, fw;
        bool hasKeyborad;
        ScriptHelper::ProcessWindowInfo(ScriptReplayer::ParseKeyValues(evt->StrOperand(0)),
            sb, nb, hasKeyborad, fw);

        string recordDeviceWidth = qs->GetResolutionWidth();
        string recordDeviceHeight = qs->GetResolutionHeight();
        Size disp;
        bool isParsed = TryParse(recordDeviceWidth, disp.width);
        assert(isParsed);
        isParsed = TryParse(recordDeviceHeight, disp.height);
        assert(isParsed);

        if (disp.width <= 0
            || disp.height <= 0)
            return true;

        Rect rctSb = AndroidTargetDevice::GetTransformedRectangle(sb,
            Size(mask.cols, mask.rows), disp,
            evt->GetOrientation());
        Rect rctNb = AndroidTargetDevice::GetTransformedRectangle(nb,
            Size(mask.cols, mask.rows), disp,
            evt->GetOrientation());
        // ignore status bar and notification bar area
        mask(rctSb).setTo(0);
        mask(rctNb).setTo(0);

        auto vecTms = qs->TimeStamp();
        auto tms = evt->TimeStamp();
        auto totalFrames = (unsigned)refVideo->get(CV_CAP_PROP_FRAME_COUNT);
        for (unsigned i = 0; i < totalFrames; ++ i)
        {
            if (vecTms[i] >= tms)
            {
                for (unsigned j = i; tms - vecTms[j] < 500 && j >= 0; -- j)
                {
                    Mat img, gray;
                    refVideo->set(CV_CAP_PROP_POS_FRAMES, j);
                    if (refVideo->read(img))
                    {                        
                        cvtColor(img, gray, CV_BGR2GRAY);
                        vector<uchar> status;
                        vector<float> err;
                        if (!prevImg.empty() && !corners[0].empty())
                        {
                            calcOpticalFlowPyrLK(prevImg, gray, corners[0], corners[1],
                                status, err, winSize, maxLevel, criteria);
                            for (unsigned k = 0; k < corners[0].size(); ++ k)
                            {
                                if (0 == status[k])
                                {
                                    //doesn't match
                                    continue;
                                }
                                float dx, dy;
                                dx = corners[1][k].x - corners[0][k].x;
                                dy = corners[1][k].y - corners[0][k].y;
                                if ((dx*dx + dy*dy) > ((1.0F * max(img.cols, img.rows) / max(disp.width, disp.height)) 
                                    * (1.0F * min(img.cols, img.rows) / min(disp.width, disp.height))))
                                {
                                    ret = true;
#ifdef _DEBUG
                                    cv::circle(prevImg, corners[0][k], 3, cv::Scalar(255, 0, 0), -1);
                                    imwrite("debug_opticalflow_0.png", prevImg);
                                    cv::circle(gray, corners[1][k], 3, cv::Scalar(255, 0, 0), -1);
                                    imwrite("debug_opticalflow_1.png", gray);
#endif
                                    break;
                                }
                            }
                        }

                        if (ret)
                            break;

                        vector<Point2f> cornersTmp;
                        goodFeaturesToTrack(gray, cornersTmp, maxFeaturesPerChannel,
                            qualityLevel, minDistance);
                        if (cornersTmp.empty())
                        {
                            DAVINCI_LOG_WARNING << "Abnormal frame.";                                    
                        }

                        prevImg = gray;
                        corners[0] = cornersTmp;
                    }
                }
                break;
            }
        }
        return ret;
    }

    bool MainTestReport::CheckVideoBlankPage()
    {
        FailureType currentFailureType = FailureType::WarMsgVideoBlankPage;
        if(!MapContainKey(currentFailureType, checkers) || !checkers[currentFailureType].enabled)
        {
            return true;
        }

        // Find replay video
        string videoFile = xmlFile.stem().string();
        vector<boost::filesystem::path> avifiles;
        testReport->GetAllMediaFiles(xmlFolder, avifiles, videoFile + "_replay");
        if (avifiles.empty())
        {
            DAVINCI_LOG_INFO << "No replay video found to copy.";
            return true;
        }

        boost::shared_ptr<BlankPagesChecker> videoChecker = boost::shared_ptr<BlankPagesChecker>(new BlankPagesChecker());
        if(videoChecker != nullptr)
        {
            vector<boost::filesystem::path>::reverse_iterator end = avifiles.rend();
            for(vector<boost::filesystem::path>::reverse_iterator i = avifiles.rbegin(); i != end; i++)
            {
                string replayVideoFile = (*i).string();
                if(boost::filesystem::is_regular_file(replayVideoFile))
                {
                    bool result = true;
                    DAVINCI_LOG_INFO << "Checking blank frames in replay video: " << replayVideoFile;
                    if (videoChecker->HasVideoBlankFrames(replayVideoFile))
                    {
                        result = false;

                        //Add info to XML for report
                        {
                            string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                            string resulttype = "type"; //<test-result type="RnR">
                            string typevalue = "RNR"; 
                            string commonnamevalue = videoFile + ".xml";
                            string totalreasult = "WARNING"; 
                            string message = TestReport::currentQsLogPath + "/Video_Abnormal_Frames";
                            string itemname = "blankpagevideo";
                            string itemresult = "WARNING";
                            string itemstarttime = "starttime";
                            string itemendtime = "endtime";
                            string timestamp = "timestamp";
                            string infoname = "name";
                            string infourl = "";
                            xmlReportSummary->appendCommonNodeCommoninfo(currentReportSummaryName, resulttype, typevalue, commonnamevalue, totalreasult, 
                                message, itemname, itemresult, itemstarttime, itemendtime, timestamp, infoname, infourl);
                        }
                    }
                    return result;
                }
            }
        }
        return true;
    }

    bool MainTestReport::CheckVideoFlickering()
    {
        FailureType currentFailureType = FailureType::WarMsgVideoFlickering;
        if(!MapContainKey(currentFailureType, checkers) || !checkers[currentFailureType].enabled)
        {
            return true;
        }

        // Check video flicking
        // Find all packageName_replay*.avi videos in current xml folder
        string videoFile = xmlFile.stem().string();
        vector<boost::filesystem::path> avifiles;
        testReport->GetAllMediaFiles(xmlFolder, avifiles, videoFile + "_replay");
        if (avifiles.empty())
        {
            DAVINCI_LOG_INFO << "No replay video found to copy.";
            return true;
        }

        boost::shared_ptr<FlickeringChecker> checkFlickering = boost::shared_ptr<FlickeringChecker>(new FlickeringChecker());
        if (checkFlickering != nullptr)
        {
            vector<boost::filesystem::path>::reverse_iterator end = avifiles.rend();
            for(vector<boost::filesystem::path>::reverse_iterator i = avifiles.rbegin(); i != end; i++)
            {
                string replayVideoFile = (*i).string();
                if(boost::filesystem::is_regular_file(replayVideoFile))
                {
                    bool result = true;
                    DAVINCI_LOG_INFO << "Checking flicking frames in replay video: " << replayVideoFile;
                    if (checkFlickering->VideoQualityEvaluation(replayVideoFile) == DaVinciStatusSuccess)
                    {
                        result = false;

                        //Add info to XML for report
                        {
                            string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                            string resulttype = "type"; //<test-result type="RnR">
                            string typevalue = "RNR"; 
                            string commonnamevalue = videoFile + ".xml";
                            string totalreasult = "WARNING"; 
                            string message = TestReport::currentQsLogPath + "/FlickingVideos";
                            string itemname = "flickeringvideo";
                            string itemresult = "WARNING";
                            string itemstarttime = "starttime";
                            string itemendtime = "endtime";
                            string timestamp = "timestamp";
                            string infoname = "name";
                            string infourl = "";
                            xmlReportSummary->appendCommonNodeCommoninfo(currentReportSummaryName, resulttype, typevalue, commonnamevalue, totalreasult, 
                                message, itemname, itemresult, itemstarttime, itemendtime, timestamp, infoname, infourl);
                        }
                    }
                    return result;
                }
            }
        }
        return true;
    }

    bool MainTestReport::CheckAudio()
    {
        FailureType currentFailureType = FailureType::WarMsgAbnormalAudio;
        if (!MapContainKey(currentFailureType, checkers) || !checkers[currentFailureType].enabled)
        {
            return true;
        }
        // No replay audio file generated in GetRnR test, should play the script one time in order to
        // generate audio.
        string audioFile = xmlFile.stem().string();
        vector<boost::filesystem::path> audioFiles;
        testReport->GetAllMediaFiles(xmlFolder, audioFiles, audioFile);
        if (audioFiles.empty())
        {
            DAVINCI_LOG_INFO << "No replay audio found to copy.";
            return true;
        }

        boost::shared_ptr<AudioFormGenerator> audioFormGen = boost::shared_ptr<AudioFormGenerator>(new AudioFormGenerator());
        if (audioFormGen != nullptr)
        {
            vector<boost::filesystem::path>::reverse_iterator end = audioFiles.rend();
            for(vector<boost::filesystem::path>::reverse_iterator i = audioFiles.rbegin(); i != end; i++)
            {
                string replayAudioFile = (*i).string();
                if (!boost::algorithm::ends_with(replayAudioFile, ".wav"))
                {
                    continue;
                }
                if(boost::filesystem::is_regular_file(replayAudioFile))
                {
                    bool result = true;
                    DAVINCI_LOG_INFO << "Drawing spectrum for audio file: " << replayAudioFile;
                    if (audioFormGen->SaveAudioWave(replayAudioFile) == DaVinciStatusSuccess)
                    {
                        result = false;
                        //Check lot to see whether the audio contain noise samples or just a silent one
                        string checkResult = audioFormGen->CheckQualityResult(replayAudioFile);
                        //Add info to XML for report
                        {
                            string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";
                            string resulttype = "type"; //<test-result type="RnR">
                            string typevalue = "RNR"; 
                            string commonnamevalue = audioFile + ".xml";
                            string totalreasult = "WARNING"; 
                            string message;
                            string itemname = "audio quality";
                            string itemresult;
                            if (checkResult == "Noise")
                            {
                                itemresult = "WARNING";
                                message = "Audio file might have noise! Please double confirm it!";
                            }
                            else if (checkResult == "Silent")
                            {
                                itemresult = "WARNING";
                                message = "Audio file might have NO SOUND! Please double confirm it!";
                            }
                            else
                            {
                                itemresult = "PASS";
                                message = "";
                            }
                            string itemstarttime = "starttime";
                            string itemendtime = "endtime";
                            string timestamp = "timestamp";
                            string infoname = "name";
                            string infourl = replayAudioFile;
                            xmlReportSummary->appendCommonNodeCommoninfo(currentReportSummaryName, resulttype, typevalue, commonnamevalue, totalreasult, 
                                message, itemname, itemresult, itemstarttime, itemendtime, timestamp, infoname, infourl);
                        }
                    }
                    return result;
                }
            }
        }
        return true;
    }

    bool MainTestReport::CheckReferenceFrames()
    {
        FailureType currentFailureType = FailureType::WarMsgReference;
        if(!MapContainKey(currentFailureType, checkers) || !checkers[currentFailureType].enabled)
        {
            return true;
        }
        
        Size recordDeviceSize = Size(0,0);
        Size replayDeviceSize = Size(0,0);
        double recordDPI = 0;
        double replayDPI = 0;
        string currentRecordDeviceWidth = "";
        string currentRecordDeviceHeight = "";
        string currentRecordDPIStr = "";
        bool isBackgroundDynamic = false;
        double DefaultUnmatchedRatio = 0.45;
        double ResolutionImpactRatio = 0.15;
        double DpiImpactRatio = 0.1;
        double BackgroundImpactRatio = 0.1;

        double unmatchRatio = 0.0;

        unmatchRatio = DefaultUnmatchedRatio;

        DAVINCI_LOG_INFO << "Reference check start.";

        boost::filesystem::path currentQsFile;
        auto qsDir = replayFolder.parent_path();
        boost::shared_ptr<QScript> qs;
        boost::shared_ptr<VideoCaptureProxy> refVideo;
        auto dut = DeviceManager::Instance().GetCurrentTargetDevice();
        auto andDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (andDut == nullptr)
        {
            DAVINCI_LOG_WARNING << "Cannot find DUT.";
            return true;
        }

        replayDeviceSize = Size(andDut->GetDeviceWidth(), andDut->GetDeviceHeight());
        replayDPI = andDut->GetDPI();

        for(auto pair : matchedTouchFramePairs)
        {
            auto tmpFile = qsDir / pair.qsName;
            if (tmpFile != currentQsFile)
            {
                currentQsFile = tmpFile;
                qs = QScript::Load(currentQsFile.string());
                if (qs != nullptr)
                {                
                    string videoFile = qs->GetResourceFullPath(qs->GetVideoFileName());
                    refVideo.reset(new VideoCaptureProxy(videoFile));
                }
            }

            if (qs == nullptr)
                continue;

            currentRecordDeviceWidth = qs->GetResolutionWidth();
            currentRecordDeviceHeight = qs->GetResolutionHeight();
            TryParse(currentRecordDeviceWidth, recordDeviceSize.width);
            TryParse(currentRecordDeviceHeight, recordDeviceSize.height);
            currentRecordDPIStr = qs->GetDPI();
            TryParse(currentRecordDPIStr, recordDPI);

            unmatchRatio = DefaultUnmatchedRatio;
            isBackgroundDynamic = false;

            if ((recordDeviceSize.width * recordDeviceSize.height) != (replayDeviceSize.width * replayDeviceSize.height)) 
            {
                unmatchRatio = unmatchRatio + ResolutionImpactRatio;
                DAVINCI_LOG_INFO << "Reference checker: resolution not the same. expected umatch ratio:" << unmatchRatio;
            }

            if (recordDPI != replayDPI)
            {
                unmatchRatio = unmatchRatio + DpiImpactRatio;
                DAVINCI_LOG_INFO << "Reference checker: DPI not the same. expected umatch ratio:" << unmatchRatio;
            }

            Mat recordFrame = ReadFrameFromResources(pair.qsName, pair.startIp);
            Mat replayFrame = ReadFrameFromResources(pair.qsName, pair.startIp, false); 
            Mat mask(recordFrame.size(), CV_8UC1, Scalar(255)); 
            if (IsInAnimation(qs, pair.startIp, refVideo, mask))
            {
                unmatchRatio = unmatchRatio + BackgroundImpactRatio;
                isBackgroundDynamic = true;
                DAVINCI_LOG_INFO << "Reference checker: background is dynamic. expected umatch ratio:" << unmatchRatio;
            }

            if (ImageChecker::IsImageMatched(replayFrame, recordFrame, unmatchRatio))
            {
                xmlReportSummary->addMatchresult(xmlReportPath, to_string(pair.startIp), "match");
                DAVINCI_LOG_INFO << "#Reference image check on @(" << pair.qsName << "," << pair.startIp << "): match" << endl;
            }
            else
            {
                if (isBackgroundDynamic)
                    xmlReportSummary->addMatchresult(xmlReportPath, to_string(pair.startIp), "unmatch (background dynamic)");
                else
                    xmlReportSummary->addMatchresult(xmlReportPath, to_string(pair.startIp), "unmatch");

                DAVINCI_LOG_WARNING << "#Reference image check on @(" << pair.qsName << "," << pair.startIp << "): unmatch" << endl;
            }
        }

        DAVINCI_LOG_INFO << "Reference check completes.";
        return true;
    }

    void MainTestReport::AppendCriticalPath(string qsName, string lineNumber, string sourceImage, string sourceImagePath, string replayImage, string replayImagePath)
    {
        xmlReportSummary->appendPathImage(xmlReportPath,
            qsName,
            "OPCODE", lineNumber, 
            sourceImage, sourceImagePath,
            replayImage, replayImagePath
            );
    }

    void MainTestReport::GenerateMainTestReport()
    {
        string recordImage, replayImage;

        for(auto pair : replayIpSequence)
        {
            if(ContainQSPairIp(matchedTouchFramePairs, pair))
            {
                ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[pair.qsName][pair.startIp];
                if(info.isVirtualKeyboard != KeyboardInputType::KeyboardInput_Command)
                {
                    boost::filesystem::path recordFilePath("_RECORD_");
                    boost::filesystem::path replayFilePath("_REPLAY_");
                    recordImage = "record_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference_clickPoint.png";
                    recordFilePath /= recordImage;

                    replayImage = "replay_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference_clickPoint.png";
                    replayFilePath /= replayImage;

                    AppendCriticalPath(pair.qsName, boost::lexical_cast<string>(pair.startIp), recordImage, recordFilePath.string(), replayImage, replayFilePath.string());
                }
            }
            else
            {
                if(pair.random)
                {
                    boost::filesystem::path replayFilePath("_REPLAY_");

                    replayImage = "replay_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference_clickPoint.png";
                    replayFilePath /= replayImage;
                    AppendCriticalPath(pair.qsName, "0", recordImage, "", replayImage, replayFilePath.string()); // Hide the record image for random replay image
                }
                else
                {
                    boost::shared_ptr<QScript> script = scripts[pair.qsName];
                    boost::shared_ptr<QSEventAndAction> qsEvent = script->EventAndAction(pair.startIp);
                    int opcode = qsEvent->Opcode();
                    if(QScript::IsOpcodeNonTouchShowFrame(opcode))
                    {
                        boost::filesystem::path replayFilePath("_REPLAY_");
                        // Save the frame for OPCODE_BACK 0
                        if(opcode == QSEventAndAction::OPCODE_HOME || opcode == QSEventAndAction::OPCODE_BACK || opcode == QSEventAndAction::OPCODE_MENU)
                        {
                            if ((qsEvent->NumIntOperands() == 0) || (qsEvent->NumIntOperands() == 1 && qsEvent->IntOperand(0) == 0))
                            {
                                replayImage = "replay_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference.png";
                                replayFilePath /= replayImage;
                                AppendCriticalPath(pair.qsName, boost::lexical_cast<string>(pair.startIp), recordImage, "", replayImage, replayFilePath.string());
                            }
                        }
                        else
                        {
                            replayImage = "replay_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference.png";
                            replayFilePath /= replayImage;
                            AppendCriticalPath(pair.qsName, boost::lexical_cast<string>(pair.startIp), recordImage, "", replayImage, replayFilePath.string());
                        }
                    }
                }
            }
        }

        for(auto pair : recordIpSequence)
        {
            ScriptHelper::QSPreprocessInfo info = scriptPreprocessInfos[pair.qsName][pair.startIp];
            if(!ContainQSPairIp(matchedTouchFramePairs, pair) && pair.touch && !info.isAdvertisement)
            {
                boost::filesystem::path recordFilePath("_RECORD_");

                recordImage = "record_" + pair.qsName + "_" + boost::lexical_cast<string>(pair.startIp) + "_reference_clickPoint.png";
                recordFilePath /= recordImage;
                AppendCriticalPath(pair.qsName, boost::lexical_cast<string>(pair.startIp), recordImage, recordFilePath.string(), replayImage, ""); // Hide the record image for random replay image
            }
        }

        xmlReportSummary->addPathCoverage(xmlReportPath,
            entryQSName,
            boost::lexical_cast<string>(passrate) + "%");
    }

    void MainTestReport::PrepareSmokeTestReport(string xmlFolderPath, string apkName, string apkPath, string packageName, string activityName, string appName, string qsFileName)
    {
        smokeTest->SetReportOutputFolder(TestReport::currentQsLogPath);
        smokeTest->prepareSmokeTest(xmlFolderPath);
        smokeTest->setApkName(apkName);
        smokeTest->setApkPath(apkPath);
        smokeTest->setPackageName(packageName);
        smokeTest->setActivityName(activityName);
        smokeTest->setAppName(appName);
        smokeTest->setQscriptFileName(qsFileName);
    }

    void MainTestReport::GenerateSmokeTestReport()
    {
        smokeTest->SetRnRMode(true);
        smokeTest->SetStageResults(stageResults);
        smokeTest->SetFailureReason(failureReason);
        smokeTest->SetLoginResult(loginResult);
        smokeTest->generateAppStageResult();
    }

    void MainTestReport::PopulateGetRnRQScript()
    {

        string rnrScript = TestReport::currentQsLogPath + "/Replay/" + xmlFile.stem().string() + "_replay.qfd";           
        QReplayInfo qfd(generatedQScriptTimeStamp, generatedGetRnRQScript->Trace());
        qfd.SaveToFile(rnrScript);
        string qtsFileName = TestReport::currentQsLogPath + "/Replay/" + xmlFile.stem().string() + "_replay.qts";    
        ofstream qts;
        qts.open(qtsFileName, ios::out | ios::trunc);
        if (qts.is_open())
        {
            qts << "Version:1" << endl;
            for (auto ts = generatedQScriptTimeStamp.begin(); ts != generatedQScriptTimeStamp.end(); ts++)
            {
                qts << static_cast<int64_t>(*ts) << endl;
            }
            vector<QScript::TraceInfo> & trace = generatedGetRnRQScript->Trace();
            for (auto traceInfo = trace.begin(); traceInfo != trace.end(); traceInfo++)
            {
                qts << ":" << traceInfo->LineNumber() << ":" << static_cast<int64>(traceInfo->TimeStamp()) << endl;
            }
            qts.close();
        }
        else
        {
            DAVINCI_LOG_ERROR << "Unable to open QTS file " << qtsFileName << " for writing.";
        }

    }
}

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

#ifndef __MAINTEST__
#define __MAINTEST__

#include "MainTestCommon.hpp"
#include "TestReportSummary.hpp"
#include "ScriptHelper.hpp"
#include "Smoke.hpp"
#include "ImageChecker.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class MainTestReport
    {
    private:
        boost::filesystem::path replayFolder;
        boost::filesystem::path recordFolder;
        boost::filesystem::path xmlFolder;
        boost::filesystem::path xmlFile;

        string entryQSName;
        string packageName;
        string xmlReportPath;
        boost::shared_ptr<TestReportSummary> xmlReportSummary;

        double passrate;
        map<int,int> stageResults;
        string failureReason;

        int matchedActions;

        // unknown name
        string unknownQsName;

        // language mode
        string languageMode;

        // key: qsName, value: QScript
        std::unordered_map<string, boost::shared_ptr<QScript>> scripts;

        // key: qsName, value: preprocess info (record frame, record shapes, QS block info)
        std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> scriptPreprocessInfos;

        // key: failureType, value: enabled/condition
        std::unordered_map<FailureType, CheckerInfo> checkers;

        boost::shared_ptr<ExploratoryEngine> exploreEngine;

        vector<QSIpPair> matchedTouchFramePairs;
        vector<QSIpPair> validFramePairs;
        vector<QSIpPair> nonTouchReplayPairs;
        vector<QSIpPair> recordIpSequence;
        vector<QSIpPair> replayIpSequence;
        vector<QSIpPair> backPairs;

        bool processExit;
        bool launchFail;
        bool imageCheckPass;
        Mat crashFrame;
        string loginResult;

        boost::shared_ptr<TargetDevice> dut;
        boost::shared_ptr<TestReport> testReport;
        boost::shared_ptr<Smoke> smokeTest;

        boost::shared_ptr<KeywordRecognizer> keywordRecog;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler1;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler2;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler3;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler4;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler5;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler6;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler7;
        boost::shared_ptr<ScreenFrozenChecker> screenFrozenChecker;
        boost::shared_ptr<QScript> generatedGetRnRQScript;
        vector<double> generatedQScriptTimeStamp;

    public:
        /// <summary>
        /// Construct function
        /// </summary>
        MainTestReport(string pn, string reportPath);

        void SetXmlFolder(boost::filesystem::path f)
        {
            xmlFolder = f;
        }

        void SetXmlFile(string f)
        {
            string xmlFileStr = f;
            boost::filesystem::path xmlPath(xmlFileStr);
            xmlFile = xmlPath;
        }

        void SetRecordFolder(boost::filesystem::path f)
        {
            recordFolder = f;
        }

        void SetReplayFolder(boost::filesystem::path f)
        {
            replayFolder = f;
        }

        void SetUnknownQSName(string n)
        {
            unknownQsName = n;
        }

        void SetLanguageMode(string l)
        {
            languageMode = l;
        }

        void SetScripts(std::unordered_map<string, boost::shared_ptr<QScript>>& q)
        {
            scripts = q;
        }

        void SetScriptInfos(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>>& i)
        {
            scriptPreprocessInfos = i;
        }

        void SetCheckerInfos(std::unordered_map<FailureType, CheckerInfo>& c)
        {
            checkers = c;
        }

        void SetExploratoryEngine(boost::shared_ptr<ExploratoryEngine> e)
        {
            exploreEngine = e;
        }

        void SetRecordIpSequence(vector<QSIpPair>& s)
        {
            recordIpSequence = s;
        }

        void SetReplayIpSequence(vector<QSIpPair>& s)
        {
            replayIpSequence = s;
        }

        void SetProcessExit(bool p)
        {
            processExit = p;
        }

        void SetLaunchFail(bool l)
        {
            launchFail = l;
        }

        void SetImageCheckPass(bool s)
        {
            imageCheckPass = s;
        }

        void SetEntryQSName(string e)
        {
            entryQSName = e;
        }

        void SetLoginResult(string s)
        {
            loginResult = s;
        }

        void SetTargetDevice(boost::shared_ptr<TargetDevice> t)
        {
            dut = t;
        }

        void SetGeneratedGetRnRQScript(boost::shared_ptr<QScript> script)
        {
            generatedGetRnRQScript = script;
        }

        void SetGetRnRQScriptTimeStamp(vector<double> ts)
        {
            generatedQScriptTimeStamp = ts;    
        }

        void SetCrashFrame(Mat& frame)
        {
            crashFrame = frame;
        }

        double GetPassRate()
        {
            return passrate;
        }

        void SetMatchedTouchPairs(vector<QSIpPair> matchedTouchPairs)
        {
            matchedTouchFramePairs = matchedTouchPairs;
        }

        map<int, int> GetStageResults()
        {
            return stageResults;
        }

        string GetFailureReason()
        {
            return failureReason;
        }

        vector<QSIpPair> DetectBackActions(vector<QSIpPair> replayPairs);

        int FindSuccessiveBackPairForReport(vector<QSIpPair> validIpSequence, QSIpPair backPair);

        int DetectWarningAbnormalPage(vector<FailurePair> pairs, int condition);

        bool IsFailureFromBackAction(vector<QSIpPair> backPairs, QSIpPair failurePair);

        bool DetectAlwaysLoadingFailure(vector<QSIpPair> pairs, QSIpPair& firstPair, QSIpPair& lastPair);

        FailureType DetectWarningIssues(vector<FailurePair> pairs, int& failureIndex);

        FailureType DetectFrameIssue(Mat targetFrame, Rect& okRect, Mat originalFrame, const Rect navigationBarRect, const Rect statusBarRect);

        FailureType DetectFrameIssueFromReplayPairs(vector<QSIpPair>& replayIpSequence, Rect& okRect, int& failureIndex);

        vector<string> GetFirstLastImages(vector<QSIpPair> validFramePairs, QSIpPair& firstPair, QSIpPair& lastPair);

        void GenerateNavigationStatusBar(Mat replayFrame, Orientation orientation, Rect& navigationBar, Rect& statusBar);

        void SaveFrameForTouchPairs(int stage, int framePairIndex);

        void SaveFrameForNonTouchPairs(int stage, QSIpPair pair);

        void SaveReferenceImge(int stage);

        void SaveFrameForLaunchFail();

        bool CheckReferenceFrames();

        bool CheckVideoBlankPage();

        bool CheckVideoFlickering();

        bool CheckAudio();

        vector<QSIpPair> GetMatchedTouchPairs();

        bool DetectApplicationFailures();

        void GenerateLogcat();

        void GenerateCriticalPathCoverage();

        Mat ReadFrameFromResources(string qsName, int startIp, bool isRecord = true);

        void SaveRandomReplayFrame(Mat frame, int randomIndex, bool isClick = false);

        void SaveRandomMainReplayFrame(Mat frame, int randomIndex);

        void SaveNonTouchReplayFrame(Mat frame, int opcode, string qsFilename, int startIp);

        void GenerateInstallStageResult(string installLogFile);

        void GenerateUninstallStageResult(string uninstallLogFile);

        bool DetectApplicationFailureOnInstall();

        bool DetectApplicationFailureOnImageCheck();

        bool DetectApplicationFailureOnLaunch();

        bool DetectApplicationFailureOnProcessExit();

        bool DetectApplicationFailureCore();

        bool DetectApplicationFailureOnUninstall();

        vector<QSIpPair> DetectValidReplayIpSequence();

        void ReportRecordCoverge(bool isCaseFail);

        FailureType ClickOffErrorDialog(Mat frame, FailureType failureType);

        /// <summary>
        /// Append critical path (left record image and right replay image with click point)
        /// </summary>
        void AppendCriticalPath(string qsName, string lineNumber, string sourceImage, string sourceImagePath, string replayImage, string replayImagePath);

        /// <summary>
        /// Generate RnR report with pass rate
        /// </summary>
        void GenerateMainTestReport();

        /// <summary>
        /// Generate Smoke report under RnR
        /// </summary>
        void GenerateSmokeTestReport();

        /// <summary>
        /// Prepare app information for smoke test report
        /// </summary>
        void PrepareSmokeTestReport(string xmlFolderPath, string apkName, string apkPath, string packageName, string activityName, string appName, string qsFileName);

        /// <summary>
        /// Populate GetRnR QScript 
        /// </summary>
        void PopulateGetRnRQScript();


    private:
        bool ContainQSPairIp(vector<QSIpPair> ips, QSIpPair ip);

        void SaveLaunchFrame(vector<QSIpPair> validIpSequence);

        void SaveRandomFrame(vector<QSIpPair> validIpSequence);
    };
}


#endif    //#ifndef __MAINTEST__

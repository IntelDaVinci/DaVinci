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

#ifndef __MAINTEST__HPP__
#define __MAINTEST__HPP__

#include "MainTestCommon.hpp"
#include "HighResolutionTimer.hpp"
#include "SpinLock.hpp"
#include "TestInterface.hpp"
#include "ScriptReplayer.hpp"
#include "ShapeUtil.hpp"
#include "TextUtil.hpp"
#include "ShapeExtractor.hpp"
#include "ShapeMatcher.hpp"
#include "AKazeObjectRecognize.hpp"
#include "ObjectCommon.hpp"
#include "FileCommon.hpp"
#include "ObjectRecognize.hpp"
#include "FeatureMatcher.hpp"
#include "StopWatch.hpp"
#include "PointMatcher.hpp"
#include "AndroidBarRecognizer.hpp"
#include "MainTestReport.hpp"
#include "ViewHierarchyParser.hpp"
#include "SceneTextExtractor.hpp"
#include "TestReport.hpp"
#include "KeyboardRecognizer.hpp"
#include "VideoWriterProxy.hpp"
#include "EventDispatcher.hpp"
#include "QScript.hpp"

namespace DaVinci
{
    class MainTest : public TestInterface
    {
    private:
        string xmlFile;
        boost::filesystem::path xmlFolder;
        boost::filesystem::path recordFolder;
        boost::filesystem::path replayFolder;

        // key: qsName, value: script replayer
        std::unordered_map<string, boost::shared_ptr<ScriptReplayer>> scriptReplayers;

        // key: qsName, value: QS priority
        std::unordered_map<string, QSPriority> scriptPriorities;

        // key: qsName, value: QScript
        std::unordered_map<string, boost::shared_ptr<QScript>> scripts;

        // key: qsName, value: preprocess info (record frame, record shapes, QS block info)
        std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> scriptPreprocessInfos;

        std::unordered_map<QSResolution, pair<int, int>> builtInResolutions;

        std::unordered_map<FailureType, CheckerInfo> checkers;
        std::unordered_map<EventType, EventInfo> events;
        std::unordered_map<int, EventType> eventSequence; // Support entry QS

        boost::shared_ptr<ShapeExtractor> shapeExtractor;
        boost::shared_ptr<ShapeMatcher> shapeMatcher;
        boost::shared_ptr<PointMatcher> pointMatcher;

        boost::shared_ptr<AKazeObjectRecognize> akazeRecog;
        boost::shared_ptr<FeatureMatcher> featMatcher;

        boost::shared_ptr<ScriptHelper> scriptHelper;
        boost::shared_ptr<ScriptReplayer> currentScriptReplayer;
        boost::shared_ptr<QScript> currentScript;
        boost::shared_ptr<AndroidBarRecognizer> androidBarRecog;
        boost::shared_ptr<ExploratoryEngine> exploreEngine;
        boost::shared_ptr<CrossRecognizer> crossRecognizer;
        boost::shared_ptr<CoverageCollector> coverageCollector;
        boost::shared_ptr<BiasObject> biasObject;
        boost::shared_ptr<BiasFileReader> biasFileReader;
        boost::shared_ptr<MainTestReport> mainTestReport;
        boost::shared_ptr<KeywordRecognizer> keywordRecog;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler1;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler2;
        boost::shared_ptr<ViewHierarchyParser> viewHierarchyParser;
        boost::shared_ptr<SceneTextExtractor> sceneTextExtractor;
        boost::shared_ptr<TestReport> testReport;
        boost::shared_ptr<EventDispatcher> eventDispatcher;
        boost::shared_ptr<SeedRandom> eventRandomGenerator;

        string packageName;
        string unknownQsName;
        // Entry script
        string entryQSName;
        bool isEntryScript;
        bool isEntryScriptStarted;
        bool isPopUpWindow;
        bool isSystemPopUpWindow;
        bool replayKeyboardPopUp;

        bool entryStartApp;
        Size recordDeviceSize;
        Size replayDeviceSize;
        vector<int> replayStatusBarHeights;
        vector<int> recordStatusBarHeights;
        vector<int> replayNavigationBarHeights;
        vector<int> recordNavigationBarHeights;

        double recordDPI;
        bool needTapMode;
        Size replayFrameSize;
        double replayDPI;
        string languageMode;

        bool startRandomAction;
        int currentRandomAction;

        // Schedule start
        bool isScheduleStart;
        bool isScheduleComplete;
        bool videoRecording;
        bool audioSaving;
        string videoFileName;
        VideoWriterProxy videoWriter;
        Size videoFrameSize;

        boost::shared_ptr<TargetDevice> dut;
        boost::shared_ptr<AndroidTargetDevice> androidDut;
        boost::shared_ptr<HighResolutionTimer> dispatchTimer;
        boost::shared_ptr<boost::thread> prepareThread;
        boost::shared_ptr<StopWatch> stopWatch;

        int timeout;
        int staticActionNumber;
        string matchPattern;
        string currentMatchPattern;
        string globalStretchMode;
        bool isPatternSwitch;

        int launchWaitTime;
        int actionWaitTime;
        int randomAfterUnmatchAction;
        int maxRandomAction;
        double frameUnmatchRatioThreshold;
        double currentFrameUnmatchRatioThreshold;

        int randomIndex;
        int matchIndex;

        SpinLock currentFrameLock;
        Mat currentFrame;

        ScheduleEvent emptyEvent;
        vector<ScheduleEvent> lastMatchedEvents;
        vector<ScheduleEvent> allMatchedEvents;
        vector<QSIpPair> targetQSIpPairs;

        QSIpPair currentQSIpPair;
        QSIpPair nextQSIpPair;
        vector<int> recordTouchSequence; // for entry qs Name
        vector<QSIpPair> recordIpSequence;
        vector<QSIpPair> replayIpSequence;

        vector<string> systemFocusWindows;
        vector<string> systemPopUpWindows;
        vector<string> viewHierarchyLines;
        vector<string> windowLines;
        string currentReplayFocusWindow;
        AndroidTargetDevice::WindowBarInfo windowBarInfo;
        Rect replayFrameMainROIWithBar;
        bool replayHasVirtualBar;

        AutoResetEvent breakPointWaitEvent;
        AutoResetEvent startAppWaitEvent;

        bool popupFailure;
        bool processExit;
        bool launchFail;
        bool imageCheckPass;

        string startPackageName;
        Mat crashFrame;

        string apkName;
        string appName;
        string activityName;

        string startLoginMessage;
        string finishLoginMessage;

        bool needPhysicalCamera;

        static const int defaultTimes = 3;
        static const int defaultVirtualActionWaitTime = 1000;
        static const int defaultPopUpActionWaitTime = 1000;
        static const int defaultPopUpSwitchWaitTime = 3000;
        static const int defaultBackWaitTime = 500;
        static const int defaultHomePageWaitTime = 5000;
        static const int defaultRandomActionWaitTime = 1000;
        static const int defaultSkipRandomWaitTime = 3000;
        static const int defaultTimeStampDeltaTime = 500;

        boost::shared_ptr<KeyboardRecognizer> keyboardRecog;

        boost::shared_ptr<QScript> generatedGetRnRQScript;
        vector<double> timeStampList;

    public:
        MainTest(const std::string &xmlFilename);

        virtual bool Init() override;
        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);
        virtual void Destroy() ;
        virtual CaptureDevice::Preset CameraPreset() ;

        void SetTestComplete();

        void InitSharedPtrs();
        void InitVariables();
        bool DoQSPreprocess(bool isBreakPoint = true);
        void PrepareForDispatchHandler();

        void SetBuiltInResolutions();

        QSResolution CheckQSResolution(Size frameSize);
        bool IsAkazePreferResolution(Size recordFrameSize, Size replayFrameSize);
        bool IsSameResolution(Size recordDeviceSize, Size replayDeviceSize);
        bool IsSameDPI(Size recordDeviceSize, Size replayDeviceSize, double recordDPI, double replayDPI);

        vector<string> GetTargetWindows(vector<string>& windowLines, string packageName, string& focusWindow);
        QSPopUpType DetectPopUpType(vector<string>& window1Lines, vector<string>& window2Lines, string packageName);
        QSPopUpType DetectPopUpTypeByTargetWindows(vector<string>& recordWindows, vector<string>& replayWindows, string recordFocusWindow, string replayFocusWindow);

        QSPriority StringToPriority(string& priority);

        std::unordered_map<string, string> ParseRecordTouchDown(QSIpPair pair);

        bool MatchForObjectAction(QSIpPair pair, std::unordered_map<string, string> keyValueMap, Mat frame, Mat rotatedFrame, Mat replayMainFrame, Mat rotatedReplayMainFrame, Orientation orientation, Rect replayFrameMainROI, vector<Point>& points, ScheduleEventMatchType& matchType);

        void MatchForRandomAction(Mat rotatedReplayMainFrame, vector<Point>& points, ScheduleEventMatchType& matchType);

        bool MatchIdObject(QSIpPair candidate, std::unordered_map<string, string> keyValueMap, vector<string>& viewHierarchyLines, Orientation orientation, Rect fw, Rect replayFrameMainROI);

        bool MatchVirtualObject(QSIpPair candidate);

        bool MatchTextObject(QSIpPair candidate, cv::Mat rotatedMainFrame, Size rotatedFrameSize, Rect replayFrameMainROI);

        bool MatchShapeObject(QSIpPair candidate, cv::Mat replayFrame, Rect replayFrameMainROI);

        bool MatchAkazeObject(QSIpPair candidate, cv::Mat replayFrame, Rect replayFrameMainROI);

        bool MatchDPIObject(QSIpPair candidate, Orientation orientation);

        bool MatchStretchObject(QSIpPair candidate, std::unordered_map<string, string> keyValueMap, Rect replayFrameMainROI);

        bool MatchSystemFocusWindow(string replayFocusWindow);

        bool MatchSystemPopUpWindow(string replayFocusWindow);

        bool MatchKeyboardPopUp(bool keyboardPopUp);

        bool MatchPopUpObject(const Mat rotatedReplayFrame, vector<Point> &points, bool isRandom = false);

        bool MatchCrossObject(const Mat rotatedReplayFrame, vector<Point> &points, bool isRandom = false);

        Point TransformDevicePointToFramePoint(Point devicePoint, Size deviceSize, Orientation orientation, Size frameSize);

        bool UseDirectStretch(string matchPattern);

        bool UseReverseDirectStretch(string matchPattern);

        bool UseStretch(string matchPattern);

        bool UseDPI(string matchPattern);

        bool UseID(string matchPattern);

        bool UsePopUp(string matchPattern);

        bool UseCross(string matchPattern);

        bool IsPopUpOrCross(string matchPattern);

        bool UseText(string matchPattern);

        bool UseAkaze(string matchPattern);

        bool UseShape(string matchPattern);

        void DispatchAction(ScheduleEventMatchType matchType, vector<Point> points, Mat frame, Mat rotatedFrame, Mat replayMainFrame, Mat rotatedReplayMainFrame, Orientation orientation, Rect replayFrameMainROI);

        void SaveEventFrame();

        void DispatchEvent(QSIpPair pair);

        void DispatchObjectRecordAction(ScheduleEvent event, Mat frame);

        void DispatchDirectRecordAction(ScheduleEvent event, Mat frame);

        void DispatchIdRecordAction(ScheduleEvent event, Mat frame);

        void DispatchVirtualAction(ScheduleEvent event, Mat frame, Size replayDeviceSize, Rect nbRect, Rect sbRect, Rect fwRect);

        void ResetRandomStatus();

        DaVinciStatus ExecuteQSStep(boost::shared_ptr<ScriptReplayer> scriptReplayer, unsigned int startIp, unsigned int endIp, double tso);

        void DispatchRandomAction(cv::Mat frame, cv::Mat rotatedFrame, cv::Mat replayMainFrame, cv::Mat rotatedReplayMainFrame, Orientation orientation);

        void DispatchBackAction(Mat frame, const Mat replayMainFrame, Orientation orientation);

        void DispatchWindowCloseAction(Mat frame, const Mat replayMainFrame, Orientation orientation);

        bool MatchCross(const Mat& replayFrame, vector<Point> &targetPoints);

        Point MatchByShape(ScriptHelper::QSPreprocessInfo info, Mat replayMainFrame, Rect replayFrameMainROI, ScheduleEvent &scheduleEvent);

        vector<Point> MatchByText(ScriptHelper::QSPreprocessInfo info, Size rotatedFrameSize, cv::Rect replayFrameMainROI, Mat rotatedReplayMainFrame);

        Point MatchByAkaze(ScriptHelper::QSPreprocessInfo info, cv::Rect replayFrameMainROI, const cv::Mat &homography);

        cv::Mat MatchFrameByAkaze(ScriptHelper::QSPreprocessInfo info, cv::Mat replayMainFrame, cv::Rect replayFrameMainROI, double *unmatchRatio);

        void DispatchHandler(const HighResolutionTimer &timer);

        DaVinciStatus StartDispatchTimer();

        DaVinciStatus StopDispatchTimer();

        void GenerateReport();

        int FindRandomAvailableRecordTouchIp(vector<int>& unavailableIPs);

        std::unordered_map<int, EventType> GenerateEventSequence();

        void EnqueueFrame(const cv::Mat &frame);

        bool CheckProcessExit();

        void CheckProcessExitForTouchAction(ScheduleEvent event, Mat frame, Orientation frameOrientation);

        void CheckProcessExitForNoneTouchAction(Point point, Mat frame, Orientation orientation);

        bool NeedCheckProcessExitForTouchAction(ScheduleEvent event);

        bool NeedCheckProcessExitForNonTouchAction(int opcode);

        double MeasureBreakPointWaitTime(std::unordered_map<string, boost::shared_ptr<QScript>> scripts, string qsName, int startIp, int endIp);

        double MeasureStartAppWaitTime(std::unordered_map<string, boost::shared_ptr<QScript>> scripts, string qsName, int currentQSIp);

        DaVinciStatus WaitBreakPointForVirtualAction(boost::shared_ptr<ScriptReplayer> scriptReplayer, unsigned int startIp, unsigned int endIp);

        double UpdateWaitTime(double time1, double time2 = 0.0);

        bool IsValidPoint(Rect roi, Point p);

        bool IsValidRect(Rect r);

        bool IsValidCandidateDistance(QSIpPair candidate, int startIp, int maxDistance = 1);
        bool IsValidTargetQSIpPairCandidatesByDistance(QSIpPair candidate);

        int FindFirstTouchDownIpFromScriptInfos(QSIpPair candidate);

        int FindLastIpFromMatchedEvents(QSIpPair candidate);

        void MarkEventsBeforeMatchEvent(vector<QSIpPair> targetQSIpPairs, ScheduleEvent matchedEvent);

        bool IsSingleIpCompleted(string qsName, int ip);

        void WaitForNextAction();

        void SetCurrentQSIpPair(string qsName, int startIp);

        void DispatchPopUpAction(Mat frame, const Mat replayMainFrame, const Mat rotatedReplayMainFrame, Orientation orientation, vector<Point> points, Rect replayFrameMainROI, ScheduleEvent event);

        void DispatchCrossAction(Mat frame, const Mat replayMainFrame, const Mat rotatedReplayMainFrame, Orientation orientation, Point point, Rect replayFrameMainROI, ScheduleEvent event);

        void DispatchVirtualKeyAction(ScriptHelper::QSPreprocessInfo qsInfo, ScheduleEvent event, Mat frame, Orientation orientation, Size replayDeviceSize, Rect nbRect, Rect sbRect, Rect fwRect);

        void DispatchVirtualBarAction(ScriptHelper::QSPreprocessInfo qsInfo, ScheduleEvent event, Mat frame, Orientation orientation);

        void DispatchNonTouchAction(ScheduleEvent event, Mat frame, Mat rotatedFrame);

        ScheduleEvent GetScheduleEventByCoverage(vector<ScheduleEvent> matchedEvents);

        bool IsContinousNonMatchObjects(int maxNumber = 8);

        // Improve code coverage
        void SetQSPreprocessInfo(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> infos);

        std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> GetQSPreprocessInfo()
        {
            return scriptPreprocessInfos;
        }

        vector<ScheduleEvent> GetAllMatchedEvents();

        void SetAllMatchedEvents(vector<ScheduleEvent> events);

        QSIpPair FindNextQSIp(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> info, QSIpPair qsIp);

        vector<QSIpPair> FindFirstTargetQSIpPairCandidates(vector<QSIpPair> pairs);

        void SetTargetDevice(boost::shared_ptr<TargetDevice> t)
        {
            dut = t;
        }

        void SetMainTestReport(boost::shared_ptr<MainTestReport> report)
        {
            mainTestReport = report;
        }

        std::unordered_map<string, boost::shared_ptr<ScriptReplayer>> GetScriptReplayers()
        {
            return scriptReplayers;
        }

        void SetEventRandomGenerator(boost::shared_ptr<SeedRandom> seedRandom)
        {
            eventRandomGenerator = seedRandom;
        }

        void SetEvents(std::unordered_map<EventType, EventInfo> es)
        {
            events = es;
        }

        void SetRecordTouchIps(vector<int> ips)
        {
            recordTouchSequence = ips;
        }

        virtual void ProcessWave(const boost::shared_ptr<vector<short>> &samples) override;

    private:
        string GetResourcePath(string file);

        bool IsValidTargetQSIpPairCandidatesByWindow(ScriptHelper::QSPreprocessInfo info, vector<string>& replayWindows, string replayFocusWindow, bool isPopUpWindow, bool isSnipScript);

        bool IsSameWindowPackage(string recordWindow, string replayWindow);

        vector<QSIpPair> FindTargetQSIpPairCandidatesByPriority(QSPriority priority, vector<string>& replayTargetWindows, string replayFocusWindow, bool isPopUpWindow, bool fallThrough);

        vector<QSIpPair> FindTargetQSIpPairCandidatesFromAllScripts(vector<string>& targetWindows, string replayFocusWindow, bool isPopUpWindow, bool fallThrough = false);

        vector<QSIpPair> FindTargetQSIpPairCandidatesByDistance(vector<QSIpPair>& candidates);

        bool HasNonTouchFirstCandidate(vector<QSIpPair>& candidates, QSIpPair& nonTouchCandidate);

        bool IsQsIpPairInMatchedEvent(vector<ScheduleEvent> events, QSIpPair qsIpPair);

        ScheduleEvent FindFirstUncoveredScheduleEvent(vector<ScheduleEvent> allMatchedEvents, vector<ScheduleEvent> matchedEvents);

        bool IsAllScriptsCompleted(std::unordered_map<string, map<int, ScriptHelper::QSPreprocessInfo>> qsInfos, QSIpPair currentQSIpPair);

        bool IsScriptCompleted(string qsName, map<int, ScriptHelper::QSPreprocessInfo> qsInfos);

        cv::Mat GetCurrentFrame();

        void SaveRandomReplayFrame(Mat frame, int randomIndex);

        bool ContainMatchedEvent(vector<ScheduleEvent> events, ScheduleEvent event);

        void CleanUpDebugResources();

        void CopyDebugResources();

        void CleanQScriptResources();

        string CheckLoginResult();

        bool WaiveProcessExit(vector<ScheduleEvent> events);

        boost::mutex audioMutex;
        boost::shared_ptr<AudioFile> audioFile;

        string audioFileName;

        void PressBack();
        void PressHome();
        void PressMenu();
        void AddEventToGetRnRQScript(boost::shared_ptr<QSEventAndAction> qEvent, bool updateTime=true);
    };
}

#endif	//#ifndef __MAINTEST__HPP__

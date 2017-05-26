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

#ifndef __SCRIPT_REPLAYER_HPP__
#define __SCRIPT_REPLAYER_HPP__

#include <unordered_map>

#include "TextRecognize.hpp"
#include "QScript.hpp"
#include "BiasFileReader.hpp"
#include "BiasObject.hpp"
#include "ExploratoryEngine.hpp"
#include "CoverageCollector.hpp"
#include "LauncherAppRecognizer.hpp"
#include "KeyboardRecognizer.hpp"
#include "InputMethodRecognizer.hpp"
#include "ScriptEngineBase.hpp"
#include "HighResolutionTimer.hpp"
#include "SpinLock.hpp"
#include "FeatureMatcher.hpp"
#include "ShapeMatcher.hpp"
#include "ShapeExtractor.hpp"
#include "CrossRecognizer.hpp"
#include "ShapeUtil.hpp"
#include "ScriptHelper.hpp"
#include "FPSMeasure.hpp"
#include "TestReport.hpp"
#include "AudioFile.hpp"
#include "AppUtil.hpp"
#include "ViewHierarchyParser.hpp"
#include "PowerMeasure.hpp"

namespace DaVinci
{
    class ResultChecker;
    class HWAccessoryController;
    class Smoke;

    enum ScriptReplayMode
    {
        REPLAY_OBJECT = 1,
        REPLAY_COORDINATE = 2
    };

    enum QSMode
    {
        QS_SMOKE = 1,
        QS_EXPLORATORY = 2,
        QS_RNR = 3,
        QS_NORMAL = 4
    };

    class ScriptReplayer : public ScriptEngineBase
    {
    public:
        explicit ScriptReplayer(const string &script);
        virtual ~ScriptReplayer();

        virtual bool Init() override;

        virtual void Destroy() override;

        virtual Mat ProcessFrame(const Mat &frame, const double timeStamp) override;

        virtual void ProcessWave(const boost::shared_ptr<vector<short>> &samples) override;

        /// <summary>
        /// Get crash frame from crash detector
        /// </summary>
        cv::Mat GetCrashDialogFrame();

        /// <summary>
        /// Get QS mode
        /// </summary>
        /// <returns></returns>
        QSMode GetQSMode();

        bool GetCrashDialogDetected();

        bool GetANRDialogDetected();

        string GetQsDirectory();
        string GetQsName();
        int GetQsIp();
        boost::shared_ptr<QScript> GetQs();

        void SetStartQSIp(unsigned int ip);
        void SetEndQSIp(unsigned int ip);
        void SetStartTimeStampOffset(double tso);
        void SetActionState();

        // To avoid generate multiple timestamp log
        void SetEntryScript(bool flag);

        void SetScriptReplayMode(ScriptReplayMode mode = ScriptReplayMode::REPLAY_COORDINATE);
        void SetBreakPointFrameSizePointAndOrientation(Size s, vector<Point> p, Orientation o, bool isTouchMove, bool tapMode);
        void StartOnlineChecking();
        void StopOnlineChecking();
        void EnableBreakPoint(bool flag);
        void ReleaseBreakPoint();
        bool IsBreakPoint();
        void WaitBreakPointComplete();
        void CompleteBreakPoint();

        DaVinciStatus StartReplayTimer();
        DaVinciStatus StopReplayTimer();

        Mat GetCurrentFrame();

        int HandleImageCheck(const Mat &frame, const boost::shared_ptr<QScript::ImageObject> &imageObject, int noLineNumber, int qsLineNumber, int &nextQsIp);

        void SetQSMode(QSMode flag);
        static bool IsSmokeQS(QSMode mode);
        static bool IsExploratoryQS(QSMode mode);
        static bool IsRnRQS(QSMode mode);
        static bool IsNormalQS(QSMode mode);
        static bool IsSwipeAction(boost::shared_ptr<QScript> qs, int ip);

        void SetSmokeOutputDir(string smokeOutput);

        bool NeedTextPreprocess(string matchPattern);

        static std::unordered_map<string, string> ParseKeyValues(string str);

        void SaveSensorCheckDataToFile();

    private:
        enum class ImageMatchAlgorithm
        {
            Patch, // patch-based feature such as AKAZE.
            Histogram
        };

        struct MatchedTextObject
        {
            /// <summary>
            /// All the matched text locations
            /// </summary>
            vector<Rect> locations;

            /// <summary>
            /// The device orientation when the text was matched
            /// </summary>
            Orientation orientation;

            MatchedTextObject(const vector<Rect> &locs, Orientation o) : locations(locs), orientation(o)
            {
            }
        };

        struct MatchedImageObject
        {
            /// <summary>
            /// The matched center
            /// </summary>
            Point2f center;

            /// <summary>
            /// The time in millisecond when the image was matched
            /// </summary>
            double time;

            /// <summary>
            /// The device orientation when the image was matched
            /// </summary>
            Orientation orientation;

            MatchedImageObject(Point2f c, double t, Orientation o) : center(c), time(t), orientation(o)
            {
            }
        };

        struct ConcurrentEvent : public boost::enable_shared_from_this<ConcurrentEvent>
        {
            boost::shared_ptr<QScript::ImageObject> imageObject;
            int ccIp; /// The IP of the OPCODE_CONCURRENT_IF_MATCH
            int startIp; /// The IP of the control block following OPCODE_CONCURRENT_IF_MATCH
            unsigned int instrCount; /// Number of instructions following OPCODE_CONCURRENT_IF_MATCH
            bool isMatchOnce; /// whether we deactivate this event after the first match
            boost::shared_ptr<FeatureMatcher> featureMatcher; /// The cached matcher to speed up matching
            bool isActivated; /// whether we should match the object specified in this event

            ConcurrentEvent(const boost::shared_ptr<QScript::ImageObject> &obj, int ip, unsigned int count, bool matchOnce);
        };

        struct MachineState
        {
            unsigned int currentQsIp;
            /// <summary>
            /// When OPCODE_EXIT or all of the opcodes have finished executing, the variable will be marked true indicating
            /// the script should exit immediately.
            /// </summary>
            bool exited;
            bool clickInHorizontal;
            /// <summary>
            /// An offset added to the absolute timestamp, introduced by branches (IF_MATCH_IMAGE, GOTO etc.)
            /// or OPCODE_INSTALL_APP.
            /// </summary>
            double timeStampOffset;
            /// <summary>
            /// Whether we are waiting for a image match (used by OPCODE_IF_MATCH_IMAGE_WAIT)
            /// </summary>
            bool imageTimeOutWaiting;
            /// <summary> The watch tracking how long we have been waiting for the image </summary>
            StopWatch imageWaitWatch;
            /// <summary> true if we are in concurrent match. </summary>
            bool inConcurrentMatch;
            /// <summary> The ip before we matched a concurrent event. </summary>
            int ipBeforeConcurrentMatch;
            /// <summary> The remaining count of instructions to run in concurrent match block. </summary>
            unsigned int instrCountLeft;
            /// <summary> A match specifying the time stamp offset before concurrent. </summary>
            double timeStampOffsetBeforeConcurrentMatch;
            /// <summary> The concurrent match start time. </summary>
            double concurrentMatchStartTime;

            /// <summary>
            /// A table of matches from OPCODE_IF_MATCH_IMAGE and OPCODE_IF_MATCH_IMAGE_WAIT.
            /// Key(string): image_path; Value(MatchedImageObject): matched image info
            /// </summary>
            std::unordered_map<string, boost::shared_ptr<MatchedImageObject>> imageMatchTable;
            /// <summary>
            /// A table of matches from OPCODE_IF_MATCH_TXT.
            /// Key(string): matched text; Value(MatchedTextObject): all locations of the matched text
            /// </summary>
            std::unordered_map<string, boost::shared_ptr<MatchedTextObject>> textMatchTable;
            /// <summary>
            /// A table of matches from OPCODE_IF_MATCH_REGEX.
            /// Key(string): pattern; Value(MatchedTextObject): all locations of the matched text
            /// </summary>
            std::unordered_map<string, boost::shared_ptr<MatchedTextObject>> regexMatchTable;

            /// <summary> The table of concurrent events with ip as the key </summary>
            std::unordered_map<int, boost::shared_ptr<ConcurrentEvent>> ccEventTable;

            boost::mutex waveFileTableLock;
            /// <summary> The table of wav files </summary>
            std::unordered_map<string, boost::shared_ptr<AudioFile>> waveFileTable;

            /// <summary>
            /// The table holding variables defined by OPCODE_SET_VARIABLE.
            /// Key(string): variable name; Value(object): value of the variable
            /// </summary>
            std::unordered_map<string, int> variableTable;

            /// <summary> The status of the machine. </summary>
            DaVinciStatus status;

            MachineState();

            void Init();
        };

        struct OfflineImageCheckEvent {
            double elapsed;
            boost::shared_ptr<QScript::ImageObject> imageObject;
            int opcodeLine;
            boost::shared_ptr<FeatureMatcher> featureMatcher; 
            OfflineImageCheckEvent(boost::shared_ptr<QScript::ImageObject> obj, double elapsedTime, int line) : imageObject(obj), elapsed(elapsedTime), opcodeLine(line){};

        };

        vector<OfflineImageCheckEvent> offlineImageEventCheckers;

        static const int homeWaitMilliseconds = 3000;

        static const int swipeWaitMilliseconds = 1000;

        /// <summary> The delay after leaving the concurrent match block. </summary>
        static const int ccMatchLeaveDelay = 1000;

        MachineState state;
        unsigned int endQsIp;

        /// <summary>
        /// A table of image objects created from OPCODE_IF_MATCH_IMAGE and OPCODE_IF_MATCH_IMAGE_WAIT.
        /// The purpose is to speed up the image matching during script replaying for those single color images.
        /// Key(string): image_path; Value(Mat): the histogram
        /// </summary>
        std::unordered_map<string, vector<Mat>> imageMatchHistogramTable;
        /// <summary>
        /// A table of patch feature objects created from OPCODE_IF_MATCH_IMAGE and OPCODE_IF_MATCH_IMAGE_WAIT.
        /// The purpose is to speed up the image matching during script replaying.
        /// Key(string): image_path; Value(FeatureMatcher).
        /// </summary>
        std::unordered_map<string, boost::shared_ptr<FeatureMatcher>> imageMatchPatchTable;

        // for offline image_match_wait
        size_t imgMatcherStartIndex;
        unordered_set<int> checkedImgIndices;

        TextRecognize textRecognize;

        bool isRecordedSoftCam;
        bool isReplaySoftCam;

        boost::shared_ptr<TestReport> testReport;

        boost::shared_ptr<TestReportSummary> testReportSummary;

        boost::shared_ptr<PowerMeasure> pwt;

        boost::shared_ptr<TargetDevice> dut;
        boost::shared_ptr<HWAccessoryController> hwAcc;

        boost::shared_ptr<HighResolutionTimer> replayTimer;
        SpinLock currentFrameLock;
        Mat currentFrame;

        boost::shared_ptr<Smoke> smokeTest;
        boost::shared_ptr<BiasFileReader> biasFileReader;
        boost::shared_ptr<ExploratoryEngine> expEngine;
        boost::shared_ptr<BiasObject> biasObject;
        boost::shared_ptr<LauncherAppRecognizer> launcherRecognizer;
        boost::shared_ptr<KeyboardRecognizer> keyboardRecognizer;
        boost::shared_ptr<InputMethodRecognizer> inputMethodRecognizer;
        boost::shared_ptr<boost::thread> launchThread;
        boost::shared_ptr<QScript> generateSmokeQsript;

        int seedIteration;

        QSMode qsMode;
        string smokeOutputDir;
        bool needCurrentCheck;
        bool needNextCheck;
        bool installComplete;
        boost::shared_ptr<CoverageCollector> covCollector;
        ScriptReplayMode scriptReplayMode;
        vector<Point> breakPointFramePoints;
        Point breakPointFramePoint;
        bool isTouchMove;
        bool needTapMode;
        int currentTouchMoveIndex;
        Size breakPointFrameSize;
        Orientation breakPointFrameOrientation;
        AutoResetEvent breakPointEvent;
        AutoResetEvent completeBreakPointEvent;
        AutoResetEvent waitQSEvent;
        static const int launchInputMethodCheckWait = 5000;
        static const int launchInputMethodInputWait = 2000;
        bool isBreakPoint;
        bool isEntryScript;

        boost::shared_ptr<ViewHierarchyParser> viewHierarchyParser;
        std::unordered_map<string, string> keyValueMap;
        bool needClickId;
        Point idRectCenterPoint;
        int idXOffset;
        int idYOffset;
        int treeId;
        Rect recordDeviceSize;

        boost::shared_ptr<ResultChecker> resultChecker;
        bool isAppActive;

        boost::shared_ptr<FPSMeasure> fpsTest;
        void CheckSmokeMode();

        unsigned int unpluggedUSBPort;

        
        string qfdFileName;

        /// <summary>
        /// The resolution (width and height) of the source video, used to scale
        /// match the replay video.
        /// </summary>
        double sourceFrameWidth, sourceFrameHeight;
        static const int CrashCheckerPeriod = 10000; //10S
        double lastCrashCheckTime;

        //Flag used to mark whether there is OPCODE_SENSOR_CHECK in the Qscript, (have: true, don't have: false), defalt value: false
        bool sensorCheckFlag;
        //Data structure for storing the sensor check data
        vector<String> sensorCheckData;        

        void SetCaptureType();
        void SetSourceFrameResolution();

        void ReplayHandler(HighResolutionTimer &timer);

        void BreakPointReplayHandler(HighResolutionTimer &timer);

        bool KeyOpcodeForSmoke(int opcode);

        bool KeyOpcodeForCurrentCheck(int opcode);

        /// <summary>
        /// Execute a QS event at the specified ip on the provided image frame.
        /// </summary>
        /// <param name="qs_event">QS event</param>
        /// <param name="frame">The current image</param>
        void ExecuteQSInstruction(const boost::shared_ptr<QSEventAndAction> &qs_event, const Mat &frame);

        /// <summary>
        /// Gets image for OCR.
        /// Adjust the image orientation per device layout and decide if using camera image or not.
        /// </summary>
        ///
        /// <param name="frame"> The frame from capture device. </param>
        /// <param name="param"> [in,out] The OCR parameter. </param>
        ///
        /// <returns> The image for OCR. </returns>
        Mat GetImageForOCR(const Mat &frame, int &param);

        /// <summary>
        /// Adjust the timestamp for jump target action. The function maintains the "timestampOffset" variable
        /// to "fast-forward" the replay timestamp to when the target action happens. It also preserves the
        /// relative timing of the target action (the timestamp difference between the target action and its
        /// previous action). Therefore, it will wait for the relative time required before executing the
        /// target action. The time wait can alleviate the delay caused by heavy IF_MATCH opcodes and the following
        /// actions do not have to rush to catch the timeline.
        /// </summary>
        /// <param name="qsIp">IP of the jump action</param>
        /// <param name="nextQsIp">IP of the target action</param>
        void AdjustBranchElapse(int qsIp, int nextQsIp);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="imageObject"></param>
        /// <param name="yesLineNumber"></param>
        /// <param name="noLineNumber"></param>
        /// <param name="qsLineNumber"></param>
        /// <param name="nextQsIp"></param>
        /// <param name="matchAlg"></param>
        /// <returns>0 on success, 1 on match fail, 2 when image not found</returns>
        int HandleIfMatchImage(const Mat &frame, const boost::shared_ptr<QScript::ImageObject> &imageObject,
            int yesLineNumber, int noLineNumber, int qsLineNumber, int &nextQsIp, ImageMatchAlgorithm algorithm = ImageMatchAlgorithm::Patch);

        PreprocessType GetRecordImagePrepType();

        PreprocessType GetReplayImagePrepType();

        void DrawDebugPolyline(const Mat &frame, const vector<Point2f> &points, Orientation devOri);

        void SaveUnmatchedFrame(const cv::Mat &frame, const std::string &imageName, int qsLineNumber);

        void DeleteUnmatchedFrame(const cv::Mat &frame, const std::string &imageName, int qsLineNumber);

        ImageMatchAlgorithm OperandToImageMatchAlgorithm(int operand) const;

        bool IsOpcodeInCrashWhiteList(int opcode);

        void ActivateTailingConcurrentEvents();

        double GetAltRatio(const Size &frameSize);

        vector<Mat> ComputeHistogram(const Mat &image);

        double CompareHistogram(const vector<Mat> &hist1, const vector<Mat> &hist2);

        int HandleIfMatchLayout(const string &targetLayout, const string &sourceLayout, int yesLineNumber, int noLineNumber, int qsLineNumber, int &nextQsIp);

        bool IsLayoutMatch(const string &s, const string &t);

        void HandleOfflineImageCheck();

        inline void AddFPSTest(const string& scriptName, const string& pkgName);

        bool IsValidForPowerTest();

        void SaveGeneratedScriptForSmokeTest();

        void ClickOffAllowButton();

    };
}

#endif
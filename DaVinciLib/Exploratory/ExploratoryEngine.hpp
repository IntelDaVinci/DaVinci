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

#ifndef __EXPLORATORYENGINE__
#define __EXPLORATORYENGINE__

#include "ObjectRecognizer.hpp"
#include "DownloadAppRecognizer.hpp"
#include "KeyboardRecognizer.hpp"
#include "LoginRecognizer.hpp"
#include "LauncherAppRecognizer.hpp"
#include "LicenseRecognizer.hpp"
#include "CommonKeywordRecognizer.hpp"
#include "RnRKeywordRecognizer.hpp"
#include "SharingKeywordRecognizer.hpp"
#include "BiasObject.hpp"
#include "UiState.hpp"
#include "CoverageCollector.hpp"
#include "UiStateObject.hpp"
#include "AndroidTargetDevice.hpp"
#include "AppUtil.hpp"
#include "TextRecognize.hpp"
#include "StopWatch.hpp"
#include "FeatureMatcher.hpp"
#include "AKazeObjectRecognize.hpp"
#include "CrossRecognizer.hpp"
#include "SeedRandom.hpp"
#include <regex>

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace boost::posix_time;
    static Point ExploratoryClickPoint = EmptyPoint;

    enum RandomActionType
    {
        CLICK = 0,
        SWIPE = 1,
        MOVE = 2, 
        HOME = 3, 
        BACK = 4, 
        MENU = 5, 
        POWER = 6, 
        WAKE_UP = 7,
        VOLUMN_DOWN = 8, 
        VOLUMN_UP = 9,
        TOTAL = 10
    };

    enum SwipeDirection
    {
        UP = 0,
        DOWN = 1,
        LEFT = 2,
        RIGHT = 3
    };

    /// <summary>
    /// Class ExploratoryEngine
    /// </summary>
    class ExploratoryEngine
    {
        /// <summary>
        /// Exit to home white list
        /// </summary>
    private:
        std::vector<cv::Rect> allEnProposalRects;
        std::vector<cv::Rect> allZhProposalRects;
        std::vector<Keyword> allEnKeywords;
        std::vector<Keyword> allZhKeywords;

        std::vector<Keyword> enLoginFailWhiteList;
        std::vector<Keyword> zhLoginFailWhiteList;

        std::vector<Keyword> clickedKeywords;

        cv::Mat wholePage;

        std::string packageName;

        std::string activityName;

        std::string iconName;

        std::string osVersion;

        boost::shared_ptr<ObjectRecognizer> objRecog;
        boost::shared_ptr<DownloadAppRecognizer> downloadRecog;
        boost::shared_ptr<LauncherAppRecognizer> launcherRecog;
        boost::shared_ptr<KeyboardRecognizer> keyboardRecog;
        boost::shared_ptr<LoginRecognizer> loginRecog;
        boost::shared_ptr<CommonKeywordRecognizer> commonKeywordRecog;
        boost::shared_ptr<SharingKeywordRecognizer> sharingKeywordRecog;
        boost::shared_ptr<RnRKeywordRecognizer> rnrKeywordRecog;
        boost::shared_ptr<KeywordRecognizer> keywordRecog;
        boost::shared_ptr<StopWatch> waitPlayStoreTimeoutWatch;
        boost::shared_ptr<AKazeObjectRecognize> akazeRecog;
        boost::shared_ptr<FeatureMatcher> featMatcher;
        boost::shared_ptr<CrossRecognizer> crossRecognizer;

        boost::shared_ptr<TargetDevice> dut;

        DownloadStage downloadStage;

        // Top activity same as launcher activity
        bool readyToLauncher;

        /// <summary>
        /// Adobe complete
        /// </summary>
        bool completeAdobe;

        /// <summary>
        /// License complete
        /// </summary>
        bool completeLicense;

        /// <summary>
        /// Tutorial complete
        /// </summary>
        bool completeTutorial;

        /// <summary>
        /// Check need login
        /// </summary>
        bool checkNeedLogin;

        /// <summary>
        /// Need login if user name and password is not empty
        /// </summary>
        bool needLogin;

        string loginResult;

        /// <summary>
        /// Complete inputting account
        /// </summary>
        bool completeAccount;

        /// <summary>
        /// Complete inputting password
        /// </summary>
        bool completePassword;

        /// <summary>
        /// Complete clicking login button
        /// </summary>
        bool completeLogin;

        /// <summary>
        /// Complete login result judgement
        /// </summary>
        bool completeLoginResultJudgement;

        /// <summary>
        /// Complete clicking keyword
        /// </summary>
        bool completeKeyword;

        bool completeAds;

        bool checkLoginByAccurateKeyword;

        /// <summary>
        /// Check keyboard page
        /// </summary>
        bool isKeyboard;

        /// <summary>
        /// Check popup window
        /// </summary>
        bool isPopUpWindow;

        Rect focusedWindow;
        Rect focusedWindowOnFrame;

        /// <summary>
        /// Check login page
        /// </summary>
        bool isLoginPage;

        /// <summary>
        /// Check login button
        /// </summary>
        bool isLoginButton;

        bool isPatternOne;

        bool isIncompatibleApp;

        bool isFree;

        /// <summary>
        /// Repeated state
        /// </summary>
        int repeatedState;

        int uiStateHashCode;

        TextRecognize textRecognize;

        int accountNameIndex;

        cv::Mat preFrame;
        cv::Mat currentFrame;

        std::vector<cv::Rect> loginInputRects;

        std::string languageModeForLogin;

        std::string downloadImageDir;

        const static int WaitForPlayStore = 10000;
        const static int WaitForPlayStoreIncremental = 1000;
        const static int WaitForPlayStoreTimeout = 10000;
        const static int clickCrashDialogWaitTime = 500;

        const static int WaitForLoginSuccessfully = 10000;
        const static int WaitForLaunchConfirmTimeout = 10000;
        const static int WaitForLaunchConfirmWait = 2000;
        const static int WaitForKeywordAction = 4000;

        std::string currentLanguageMode;
        bool matchCurrentLanguage;
        bool isExpectedProcessExit;
        bool isSharingIssue;

        boost::shared_ptr<StopWatch> stopWatch;
        vector<boost::shared_ptr<QSEventAndAction> > generatedQEvents;
        boost::shared_ptr<ViewHierarchyParser> viewParser;

        boost::shared_ptr<QScript> generatedQScript;
        int treeId;

        std::vector<std::string> activityQueue;
        std::vector<cv::Mat> frameQueue;
        size_t maxQueueCount;
        bool checkSharingIssue;
        std::map<std::string, std::vector<Keyword>> sharingKeywordList;
        std::map<std::string, std::vector<Keyword>> commonIDList;
        std::map<std::string, std::vector<Keyword>> abnormalPageKeywordList;

        SeedRandom randomGenerator;
        ImageBoxInfo imageBoxInfo;            
    public:
        void Init();

        ExploratoryEngine(const string &pn, const string &an, const string &in, const boost::shared_ptr<TargetDevice> &device, const string version);
        ExploratoryEngine();

        /// <summary>
        /// Init random generator
        /// </summary>
        /// <param name="seed"></param>
        /// <returns></returns>
        SeedRandom initRandomGenerator(int seed = -1);

        /// <summary>
        /// Run exploratory test
        /// </summary>
        /// <param name="rotatedFrame"></param>
        /// <param name="covCollector"></param>
        /// <param name="biasObject"></param>
        /// <param name="seedValue"></param>
        /// <param name="clickPoint"></param>
        /// <returns></returns>
        bool runExploratoryTest(const cv::Mat &rotatedFrame, const boost::shared_ptr<CoverageCollector> &covCollector, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint = ExploratoryClickPoint);

        bool MatchLaunchConfirmManufacturer(string str);

        bool ClickOffLaunchDialog(boost::shared_ptr<TargetDevice> dut);

        bool MatchPopUpKeyword(const Mat &replayFrame, vector<Point> &targetPoints, string initialLanguage, string& matchedLanguage, std::vector<Keyword>& allKeywords);

        static void DrawPoints(cv::Mat &frame, SwipeDirection d, const Scalar scalar = Red, int thickness = 10);

        /// <summary>
        /// Dispatch launcher action
        /// </summary>
        /// <param name="rotatedFrame"></param>
        /// <returns>True for success; False for failure</returns>
        bool dispatchLauncherAction(const cv::Mat &rotatedFrame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        void DispatchRandomAction(cv::Mat frame, string actionType, int actionProb);

        bool dispatchClickOffErrorDialogAction(const cv::Mat &frame, const Point &clickPoint);

        void generateAllRectsAndKeywords(const cv::Mat frame, cv::Rect focusedWindowOnFrame, string languageMode, int longSideRatio = 50);

        void setAccountNameIndex(int value)
        {
            accountNameIndex = value;
        }

        string getLoginResult()
        {
            return loginResult;
        }

        std::vector<cv::Rect> getAllEnProposalRects()
        {
            return allEnProposalRects;
        }

        std::vector<cv::Rect> getAllZhProposalRects()
        {
            return allZhProposalRects;
        }

        std::vector<Keyword> getAllEnKeywords()
        {
            return allEnKeywords;
        }

        std::vector<Keyword> getAllZhKeywords()
        {
            return allZhKeywords;
        }

        cv::Mat getWholePage()
        {
            return wholePage;
        }

        cv::Rect getFocusedWindowOnFrame()
        {
            return focusedWindowOnFrame;
        }

        boost::shared_ptr<KeywordRecognizer> getKeywordRecog()
        {
            return keywordRecog;
        }

        void setCompleteTutorial(bool value)
        {
            completeTutorial = value;
        }

        bool GetSharingIssueState()
        {
            return isSharingIssue;
        }

        bool ExploratoryEngine::JudgeLoginFailByKeyword(std::string language, std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts);

        bool ExploratoryEngine::JudgeLoginFailOnFullPart(boost::filesystem::path path1, boost::filesystem::path path2);

        /// <summary>
        /// Dispatch download action
        /// </summary>
        /// <param name="rotatedFrame"></param>
        /// <param name="biasObject"></param>
        /// <returns>True for success; False for failure</returns>
        bool dispatchDowloadAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, Point &clickPoint = ExploratoryClickPoint);

        vector<boost::shared_ptr<QSEventAndAction> > GetGeneratedQEvents()
        {
            return generatedQEvents;
        }

        void SetGeneratedQScript(const boost::shared_ptr<QScript> qs){
            generatedQScript = qs;
        }

    private:
        /// <summary>
        /// Dispatch general action
        /// </summary>
        /// <param name="covCollector"></param>
        /// <param name="rotatedFrame"></param>
        /// <param name="biasObject"></param>
        /// <param name="seedValue"></param>
        /// <returns></returns>
        bool dispatchGeneralAction(const boost::shared_ptr<CoverageCollector> &covCollector, const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint = ExploratoryClickPoint);

        bool dispatchLicenseAction(const cv::Mat &rotatedFrame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        bool dispatchTutorialAction(const cv::Mat &rotatedFrame, cv::Mat &cloneFrame);

        bool dispatchAccountAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        bool dispatchPasswordAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        /// <summary>
        /// Dispatch login action
        /// </summary>
        /// <param name="rotatedFrame"></param>
        /// <param name="biasObject"></param>
        /// <returns>True for success; False for failure</returns>
        bool dispatchLoginAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        void checkLoginButtonForNoneAccount(const cv::Mat &rotatedFrame);

        void checkLoginPage(const cv::Mat &rotatedFrame, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords);

        std::vector<cv::Rect> checkLoginButton(const cv::Mat &rotatedFrame);

        bool dispatchAdsAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, Point &clickPoint = ExploratoryClickPoint);

        bool dispatchTextAction(const cv::Mat &rotatedFrame, Point &clickPoint = ExploratoryClickPoint);

        bool dispatchKeywordAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint = ExploratoryClickPoint);

        bool generateExploratoryAction(const boost::shared_ptr<CoverageCollector> &covCollector, const cv::Mat &rotatedFrame, cv::Rect focusedWindowOnFrame, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint = ExploratoryClickPoint);

        /// <summary>
        /// Generate single exploratory action
        /// </summary>
        /// <param name="rotatedFrame"></param>
        /// <param name="covCollector"></param>
        /// <param name="biasObject"></param>
        /// <param name="uiState"></param>
        /// <param name="seed"></param>
        /// <returns>true for click app object to home</returns>
        bool dispatchProbableAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<CoverageCollector> &covCollector, const boost::shared_ptr<BiasObject> &biasObject,
            const boost::shared_ptr<UiState> &uiState, int seed, cv::Mat cloneFrame, Point &clickPoint = ExploratoryClickPoint);

        void DoRandomeAction(cv::Mat frame, RandomActionType actionType);

        RandomActionType IntToRandomActionType(int type);
        RandomActionType StringToRandomActionType(string type);

        void RecordAndRunClickQEvent(const Size &frameSize, const Point &point);
        void RecordAndRunClickOnRotatedQEvent(const Size &rotatedFrameSize, const Point &rotatedPoint);
        void RecordAndRunSetTextQEvent(const string &text);
        void RecordAndRunButtonQEvent(const string &button);
        void RecordAndRunBackQEvent();
        void RecordAndRunDragQEvent(int x1, int y1, int x2, int y2, Orientation orientation = Orientation::Portrait, bool tapmode = false);
        void RecordAndRunUIAction(boost::shared_ptr<UiStateObject> stateObj, boost::shared_ptr<UiAction> act);
        void RecordAndRunTouchDownQEvent(const Size &frameSize, int x, int y);
        void RecordAndRunTouchUpQEvent(const Size &frameSize, int x1, int y1);
        void RecordAndRunTouchMoveQEvent(const Size &frameSize, int x, int y);       


        void EnqueueFocusWindowActivity(const std::string &focusWindowActivity);

        void EnqueueFrame(const cv::Mat &frame);

        bool CheckSharingIssueByActivity(std::vector<std::string> activityQueue);

        bool CheckSharingIssueByFrame(std::vector<cv::Mat> frameQueue);

        bool JudgeStrInWhiteList(string idInfo, std::vector<Keyword> keywordList);
        bool JudgeWstrInWhiteList(wstring wstrIdInfo, std::vector<Keyword> keywordList);

        boost::shared_ptr<UiState> FilterRectangleForARC(boost::shared_ptr<UiState> uiState, const cv::Mat rotatedFrame);

        void InitFocusWindowInfo(const cv::Mat &rotatedFrame);

        std::vector<Keyword> RelocateRectInKeywords(std::vector<Keyword> keywords, const cv::Mat rotatedFrame, cv::Rect focusedWindowOnFrame);

        bool isClickBackButton(cv::Point clickPoint, boost::shared_ptr<UiState> uiState);

        void RecordClickedKeyword(Point clickPoint, const cv::Mat frame);
    };
}


#endif	//#ifndef __EXPLORATORYENGINE__

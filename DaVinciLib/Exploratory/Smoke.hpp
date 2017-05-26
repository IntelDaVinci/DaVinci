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

#ifndef __SMOKE__
#define __SMOKE__

#include "FailureType.hpp"
#include "ObjectUtil.hpp"
#include "KeywordCheckHandler.hpp"
#include "KeywordCheckHandlerForCrashANR.hpp"
#include "KeywordCheckHandlerForNetworkIssue.hpp"
#include "KeywordCheckHandlerForInitializationIssue.hpp"
#include "KeywordCheckHandlerForAbnormalFont.hpp"
#include "KeywordCheckHandlerForErrorDialog.hpp"
#include "KeywordCheckHandlerForLocationIssue.hpp"
#include "ScriptReplayer.hpp"
#include "ScreenFrozenChecker.hpp"
#include "BugTriage.hpp"

#include <string>
#include <vector>

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum SmokeResult
    {
        SKIP = -1,  // -1
        PASS,       // 0
        FAIL,       // 1
        WARNING     //2
    };

    enum SmokeStage
    {
        SMOKE_INSTALL = 0,
        SMOKE_LAUNCH = 1,
        SMOKE_RANDOM = 2,
        SMOKE_BACK = 3,
        SMOKE_UNINSTALL = 4
    };

    /// <summary>
    /// Smoke test under GET framework
    /// </summary>
    class Smoke
    {
    private:
        map<int, string> stages;
        // Smoke stages (0 ~ 4: Install ~ Uninstall)
        int currentSmokeStage;

        // Warning keywords for logcat
        std::vector<std::string> fatalExSigsegvKeywords;
        std::vector<std::string> iDebugELinkerKeywords;
        std::vector<std::string> winDeathKeywords;

        // Customized interested package names for logcat
        vector<string> customizedLogcatPackageNames;

        // qscript file name
        std::string qscriptFileName;
        // Smoke report output folder
        std::string reportOutputFolder;
        // Smoke report file
        std::string reportOutput;
        // Save the crash image
        std::string saveImageName;
        // summary report name
        std::string currentReportSummaryName;

        // Apk name
        std::string apkName;
        // Application path
        std::string apkPath;
        // Package name from applicationName
        std::string packageName;
        // Activity name from applicationName
        std::string activityName;
        // App name
        std::string appName;

        std::string topPackageName;

        // Flag to know clicking app obj to home
        bool clickExitButtonToHome;

        bool isSharingIssue;

        string loginResult;

        // Flag to control skip the following opcodes
        bool shouldSkip;

        std::vector<cv::Mat> frameQueue;

        size_t maxFrameQueueCount;

        // Smoke stage result
        map<int,int> stageResults;

        // Smoke test failed reason
        std::string failReason;

        bool isRnRMode;

        boost::shared_ptr<AndroidTargetDevice> targetDevice;
        boost::shared_ptr<ExploratoryEngine> exploreEngine;
        boost::shared_ptr<KeywordRecognizer> keywordRecog;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler1;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler2;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler3;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler4;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler5;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler6;
        boost::shared_ptr<KeywordCheckHandler> keywordCheckHandler7;
        boost::shared_ptr<KeyboardRecognizer> keyboardRecog;
        boost::shared_ptr<ScreenFrozenChecker> screenFrozenChecker;
        boost::weak_ptr<ScriptReplayer> currentTest;
        boost::shared_ptr<BugTriage> bugTriage;

        // For XML Report
        boost::shared_ptr<TestReportSummary> xmlReportSummary;
        boost::shared_ptr<TestReport> testReport;
        map<int,string> stageResultsForXML;

        bool isFirstFrameAfterLaunch;

        string language;

        std::vector<Keyword> allEnKeywords;
        std::vector<Keyword> allZhKeywords;
        cv::Mat wholePage;
        string osVersion;

        std::vector<Keyword> enGoogleFrameworkWhiteList;
        std::vector<Keyword> zhGoogleFrameworkWhiteList;

    public:
        /// <summary>
        /// Construct function
        /// </summary>
        Smoke();

        void SetTest(boost::weak_ptr<ScriptReplayer> test);

        /// <summary>
        /// prepare smoke test
        /// </summary>
        /// <param name="qsDir"></param>
        void prepareSmokeTest(std::string &qsDir);

        /// <summary>
        /// Set smoke stage
        /// </summary>
        /// <param name="stage"></param>
        void setCurrentSmokeStage(int stage);

        void SetCustomizedLogcatPackageNames(vector<string> packageNames);

        void SetReportOutputFolder(string folder);

        void SetStageResults(map<int,int> results);

        void SetFailureReason(std::string failureReason);

        /// <summary>
        /// Set ignore home
        /// </summary>
        /// <param name="flag"></param>
        void SetClickExitButtonToHome(bool flag);

        /// <summary>
        /// Set sharing issue
        /// </summary>
        /// <param name="flag"></param>
        void SetSharingIssue(bool flag);

        void SetPopupBadImage(bool flag);

        void SetLoginResult(string result);

        void SetAllEnKeywords(std::vector<Keyword> enKeywords);

        void SetAllZhKeywords(std::vector<Keyword> zhKeywords);

        void SetWholePage(cv::Mat wholePage);

        void SetOSversion(string osVersion);

        void SetRnRMode(bool isRnR);

        /// <summary>
        /// Get skip status
        /// </summary>
        /// <returns></returns>
        bool getSkipStatus();

        /// <summary>
        /// Set application name
        /// </summary>
        /// <param name="s"></param>
        void setApkName(const std::string &s);

        /// <summary>
        /// Set application path
        /// </summary>
        /// <param name="s"></param>
        void setApkPath(const std::string &s);

        /// <summary>
        /// Set package name
        /// </summary>
        /// <param name="s"></param>
        void setPackageName(const std::string &s);

        /// <summary>
        /// Set activity name
        /// </summary>
        /// <param name="s"></param>
        void setActivityName(const std::string &s);

        /// <summary>
        /// Set app name
        /// </summary>
        /// <param name="s"></param>
        void setAppName(const std::string &s);

        /// <summary>
        /// Set qscript file name
        /// </summary>
        /// <param name="s"></param>
        void setQscriptFileName(const std::string &s);

        /// <summary>
        /// Get logcat
        /// </summary>
        void getLogcat();

        string GetFailReason();

        /// <summary>
        /// Set app stage result
        /// </summary>
        /// <param name="frame"></param>
        void setAppStageResult(const cv::Mat &frame);

        void checkVideoFlickering(string videoFileName);

        /// <summary>
        /// Generate app stage result
        /// </summary>
        void generateAppStageResult();

        void generateSmokeCriticalPath();

        /// <summary>
        /// Check whether the application is installed
        /// <param name="logFile"> install log file</param>
        /// </summary>
        /// <returns> True for installed, False for not installed. </returns>
        bool checkInstallLog(const std::string &logFile);

        /// <summary>
        /// Check whether the application is uninstalled
        /// <param name="logFile"> uninstallation log file</param>
        /// </summary>
        /// <returns> True for uninstall true, False for uninstall fail. </returns>
        bool checkUninstallLog(const std::string &logFile);

        string getApkType();

        bool checkWarningInLogcat(std::string logcatFile);

        bool checkUnsupportedFeature(std::string logcatFile);

        bool checkFrameIssue(std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts, cv::Mat frame, int stageIndex);

        bool checkSharingIssue();

        bool isGoogleFrameworkIssue(std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts);

        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool isSimilarPicture(Mat image1, Mat image2);

        bool isAppAlwaysLoading(string firstImage, string lastImage, double allowRatio = 0.2);

    private:
        void init();

        void initReportOutput();

        void stopCurrentTest(const cv::Mat &frame);

        bool isIDebugOrELinkerWarning(const std::string &line);

        bool isFatalExOrSigsegvWarning(const std::string &line);

        bool isWinDeathWarning(const std::string &line);

        bool isTestedPackage();

        bool isHomePackage();

        void EnqueueFrame(const cv::Mat &frame);

        bool isAppAlwaysHomePage();

        void SetInstallStageResult(const cv::Mat &currentFrame);

        void SetLaunchStageResult(const cv::Mat &currentFrame);

        void SetExploratoryStageResult(cv::Mat &currentFrame, cv::Mat &originalFrame);

        void SetBackStageResult(const cv::Mat &currentFrame, const cv::Mat &originalFrame);

        void SetUninstallStageResult(const cv::Mat &currentFrame);

        void AppendFailReason(string reason);

        std::string GetHoudiniInfo();

        std::string GetMiddlewareInfo();
    };
}


#endif    //#ifndef __SMOKE__

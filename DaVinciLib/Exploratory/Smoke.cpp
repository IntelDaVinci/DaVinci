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
#include "TestManager.hpp"
#include "Smoke.hpp"
#include "LauncherAppRecognizer.hpp"
#include "BaseObjectCategory.hpp"
#include "AndroidTargetDevice.hpp"
#include "QScript.hpp"
#include "FlickeringChecker.hpp"
#include "unzipper.hpp"

#include <fstream>

using namespace std;
using namespace cv;
using namespace ziputils;

namespace DaVinci
{
    void Smoke::init()
    {
        stages = map<int, string>();
        stages[SmokeStage::SMOKE_INSTALL] = "Install";
        stages[SmokeStage::SMOKE_LAUNCH] = "Launch";
        stages[SmokeStage::SMOKE_RANDOM] = "Random";
        stages[SmokeStage::SMOKE_BACK] = "Back";
        stages[SmokeStage::SMOKE_UNINSTALL] = "Uninstall";

        loginResult = "SKIP";

        // Smoke stages (0 ~ 4: Install ~ Uninstall)
        currentSmokeStage = 0;

        // Smoke stage result
        stageResults = map<int, int>();
        for(int i = SmokeStage::SMOKE_INSTALL; i <= SmokeStage::SMOKE_UNINSTALL; i++)
        {
            stageResults[i] = SmokeResult::SKIP;
        }

        iDebugELinkerKeywords.push_back("I/DEBUG");
        iDebugELinkerKeywords.push_back("E/linker");
        fatalExSigsegvKeywords.push_back("FATAL EXCEPTION");
        fatalExSigsegvKeywords.push_back("SIGSEGV");
        winDeathKeywords.push_back("WIN DEATH");

        isFirstFrameAfterLaunch = true;

        topPackageName = "";

        reportOutputFolder = TestReport::currentQsLogPath;
        reportOutput = "Smoke_Test_Report.csv";
        currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";

        // Save the crash image
        saveImageName = "";

        // Apk name
        apkName = "";
        // Apk path
        apkPath = "";
        // Package name from applicationName
        packageName = "";
        // Activity name from applicationName
        activityName = "";

        // Flag to know clicking exit button to home
        clickExitButtonToHome = false;

        // Flag to control skip the following opcodes
        shouldSkip = false;

        frameQueue = std::vector<cv::Mat>();

        maxFrameQueueCount = 3;

        isRnRMode = false;
        isSharingIssue = false;
        
        // Smoke test failed reason
        std::string failReason = "";

        auto device = DeviceManager::Instance().GetCurrentTargetDevice();

        targetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(device);
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_WARNING << std::string("Error: didn't get current target device, the case will be skipped.") << std::endl;
            //Todo: need to terminate current test.
        }
        exploreEngine = boost::shared_ptr<ExploratoryEngine>(new ExploratoryEngine(packageName, activityName, appName, targetDevice, osVersion));
        bugTriage = boost::shared_ptr<BugTriage>(new BugTriage());

        language = ObjectUtil::Instance().GetSystemLanguage();
        xmlReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());
        testReport = boost::shared_ptr<TestReport>(new TestReport());
        screenFrozenChecker = boost::shared_ptr<ScreenFrozenChecker>(new ScreenFrozenChecker());

        keywordRecog = this->exploreEngine->getKeywordRecog();
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
        keyboardRecog = boost::shared_ptr<KeyboardRecognizer>(new KeyboardRecognizer());
    }

    Smoke::Smoke()
    {
        init();
    }

    void Smoke::prepareSmokeTest(std::string &qsDir)
    {
        try
        {
            if (qsDir.empty())
            {
                qsDir = ".";
            }
            this->reportOutput = qsDir + std::string("\\") + this->reportOutput;
            initReportOutput();

            // create a new empty file 'apk_logcat_%packname%.txt'
            std::ofstream logStream(this->reportOutputFolder + std::string("\\apk_logcat_") + packageName + ".txt", std::ios_base::app | std::ios_base::out);
            logStream << "" << std::flush;
            logStream.flush();
            logStream.close();
        }
        catch (...)
        {
            DAVINCI_LOG_WARNING << std::string("prepareSmokeTest failed. Probably create directory failed, check permission please.") << std::endl;
        }
    }

    void Smoke::initReportOutput()
    {
        bool needTitle = true;
        std::string reportTitle = "TestTime,DeviceName,Application,AppName,Package,Activity,";
        std::string reportTitleBuilder;
        reportTitleBuilder.append(reportTitle);
        int stageIndex = 0;
        for (auto stage : this->stages)
        {
            reportTitleBuilder.append(stage.second + std::string(","));
            stageResults[stageIndex++] = SmokeResult::SKIP;
        }
        reportTitleBuilder.append("Result,FailReason,ReportLink,Login,QscriptFileName,ApkType,OnDeviceDownload,TestDescription\n");
        string fileName = this->reportOutput;
        vector<string> lines;
        if (boost::filesystem::is_regular_file(fileName))
        {
            ReadAllLines(fileName, lines);

            if (lines.size() > 0 && lines[0].find(reportTitle) != string::npos)
            {
                needTitle = false;
            }
        }

        if (needTitle)
        {
            std::ofstream csvOutput(fileName, std::ios_base::app | std::ios_base::out);
            csvOutput << "\xEF\xBB\xBF"; // UTF-8 BOM
            csvOutput << reportTitleBuilder << std::flush;
            csvOutput.flush();
            csvOutput.close();
        }
    }

    void Smoke::SetCustomizedLogcatPackageNames(vector<string> packageNames)
    {
        customizedLogcatPackageNames = packageNames;
    }

    void Smoke::SetTest(boost::weak_ptr<ScriptReplayer> test)
    {
        currentTest = test;
    }

    void Smoke::setCurrentSmokeStage(int stage)
    {
        this->currentSmokeStage = stage;
    }

    void Smoke::SetClickExitButtonToHome(bool flag)
    {
        this->clickExitButtonToHome = !flag;
    }

    void Smoke::SetSharingIssue(bool flag)
    {
        this->isSharingIssue = flag;
    }

    void Smoke::SetLoginResult(string result)
    {
        this->loginResult = result;
    }

    void Smoke::SetAllEnKeywords(std::vector<Keyword> enKeywords)
    {
        this->allEnKeywords = enKeywords;
    }

    void Smoke::SetAllZhKeywords(std::vector<Keyword> zhKeywords)
    {
        this->allZhKeywords = zhKeywords;
    }

    void Smoke::SetWholePage(cv::Mat wholePage)
    {
        this->wholePage = wholePage;
    }

    void Smoke::SetOSversion(string osVersion)
    {
        this->osVersion = osVersion;
    }

    bool Smoke::getSkipStatus()
    {
        return this->shouldSkip;
    }

    void Smoke::setApkName(const std::string &s)
    {
        this->apkName = s;
    }

    void Smoke::setApkPath(const std::string &s)
    {
        this->apkPath = s;
    }

    void Smoke::setPackageName(const std::string &s)
    {
        this->packageName = s;
    }

    void Smoke::setActivityName(const std::string &s)
    {
        this->activityName = s;
    }

    void Smoke::setAppName(const std::string &s)
    {
        string appname = s;
        if (boost::contains(appname, ","))
            boost::replace_all(appname, ",", " ");
        this->appName = appname;
    }

    void Smoke::setQscriptFileName(const std::string &s)
    {
        this->qscriptFileName = s + ".qs";
    }

    void Smoke::SetRnRMode(bool isRnR)
    {
        this->isRnRMode = isRnR;
    }

    void Smoke::getLogcat()
    {
        if (!boost::filesystem::exists(this->reportOutputFolder))
        {
            boost::filesystem::create_directory(this->reportOutputFolder);
        }

        if (targetDevice != nullptr)
        {
            string logcatPackageName;
            for(auto pn : customizedLogcatPackageNames)
                logcatPackageName += "-e " + pn + " ";
            logcatPackageName += "-e " + packageName + " ";
            logcatPackageName += "-e houdini";

            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            // Quote wraps the logcat command
            string command = "shell \"logcat -d -v time | grep " + logcatPackageName + "\"";
            DaVinciStatus status = targetDevice->AdbCommand(command, outStr, 300000);

            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_DEBUG << "Get logcat failed: " << status.value();
                return;
            }

            string log = outStr->str();
            std::ofstream logStream(this->reportOutputFolder + std::string("\\apk_logcat_") + packageName + ".txt", std::ios_base::app | std::ios_base::out);
            logStream << log << std::flush;
            logStream.flush();
            logStream.close();
        }
    }

    string Smoke::getApkType()
    {
        DaVinciStatus status;
        string apkType  = "";

        boost::filesystem::path p(this->apkPath);
        boost::filesystem::path apkName = p.filename();

        std::wstring wstrApkName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkName.string());

        bool containNoneASCII = ContainNoneASCIIStr(wstrApkName);
        bool containSpecialChar = ContainSpecialChar(wstrApkName);

        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        if (containNoneASCII || containSpecialChar)
        {
            std::wstring wstrApkLocation = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->apkPath);
            boost::filesystem::path srcPath(wstrApkLocation);
            boost::filesystem::path destPath(TestReport::currentQsDirName + "/test.apk");
            if (boost::filesystem::is_regular_file(srcPath) && boost::filesystem::is_directory(TestReport::currentQsDirName))
            {
                if(boost::filesystem::is_regular_file(destPath))
                    boost::filesystem::remove(destPath); 
                boost::filesystem::copy(srcPath, destPath);
                status = RunProcessSync(targetDevice->GetAaptPath(), " dump badging \"" + destPath.string() + "\"", outStr);
            }
        }
        else
        {
            status = RunProcessSync(targetDevice->GetAaptPath(), " dump badging \"" + this->apkPath + "\"", outStr);
        }

        if (!DaVinciSuccess(status))
        {
            return apkType;
        }

        bool nativeCodeFound = false;
        vector<string> lines;
        ReadAllLinesFromStr(outStr->str(), lines);

        // native-code: 'armeabi' 'x86'
        if (!lines.empty())
        {
            string matchingStr = "native-code:";

            int i;
            string line;
            for (i = 0; i < static_cast<int>(lines.size()); i++)
            {
                line = lines[i];
                boost::to_lower(line);

                // native-code: 'xx'
                if (boost::starts_with(line, matchingStr))
                {
                    nativeCodeFound = true;
                    line = line.substr(matchingStr.length() + 1);
                    if (line.find("arm") != string::npos)
                    {
                        apkType += "arm ";
                    }
                    if (line.find("x86") != string::npos)
                    {
                        apkType += "x86 ";
                    }
                    break;
                }
            }
        }
        // Didn't find "native-code:"
        if (nativeCodeFound == false)
            apkType = "Java";

        if (containNoneASCII)
        {
            boost::filesystem::path tmpPath(TestReport::currentQsDirName + "/test.apk");
            if(boost::filesystem::is_regular_file(tmpPath))
                boost::filesystem::remove(tmpPath);
        }
        return apkType;
    }

    bool Smoke::isIDebugOrELinkerWarning(const std::string &line)
    {
        for (auto keyword : iDebugELinkerKeywords)
        {
            if (boost::starts_with(line, keyword))
            {
                if (keyword == iDebugELinkerKeywords[0])
                {
                    failReason = FailureTypeToString(WarMsgIDebug);
                }
                else
                {
                    failReason = FailureTypeToString(WarMsgElinker);
                }
                return true;
            }
        }

        return false;
    }

    bool Smoke::isFatalExOrSigsegvWarning(const std::string &line)
    {
        for (auto keyword : fatalExSigsegvKeywords)
        {
            if (line.find(keyword) != string::npos)
            {
                if (keyword == fatalExSigsegvKeywords[0])
                {
                    failReason = FailureTypeToString(WarMsgFatalException);
                }
                else
                {
                    failReason = FailureTypeToString(WarMsgSigsegv);
                }
                return true;
            }
        }

        return false;
    }

    bool Smoke::isWinDeathWarning(const std::string &line)
    {
        for (auto keyword : winDeathKeywords)
        {
            if (line.find(keyword) != string::npos && line.find(packageName) != string::npos)
            {
                failReason = FailureTypeToString(WarMsgWinDeath);
                return true;
            }

        }

        return false;
    }

    bool Smoke::checkWarningInLogcat(std::string logcatFile)
    {
        if (boost::filesystem::is_regular_file(logcatFile))
        {
            vector<string> lines;
            ReadAllLines(logcatFile, lines);
            for(auto line : lines)
            {
                if (isIDebugOrELinkerWarning(line) || isFatalExOrSigsegvWarning(line) || isWinDeathWarning(line))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool Smoke::checkUnsupportedFeature(std::string logcatFile)
    {
        if (boost::filesystem::is_regular_file(logcatFile))
        {
            vector<string> lines;
            ReadAllLines(logcatFile, lines);
            for(auto line : lines)
            {
                string upperLine = line;
                boost::to_upper(upperLine);
                if (upperLine.find("D/HOUDINI") != string::npos && upperLine.find("UNSUPPORTED FEATURE") != string::npos)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool Smoke::isTestedPackage()
    {
        return topPackageName == packageName;
    }

    bool Smoke::isHomePackage()
    {
        if (targetDevice == nullptr)
            return false;
        else
            return topPackageName == targetDevice->GetHomePackageName();
    }

    void Smoke::EnqueueFrame(const cv::Mat &frame)
    {
        if (frameQueue.size() == maxFrameQueueCount)
        {
            frameQueue.erase(frameQueue.begin());
        }
        frameQueue.push_back(frame);
    }

    string Smoke::GetFailReason()
    {
        return failReason;
    }

    // Stop test is only called when one stage fails
    // Save uninstall log always and update uninstall result; sometimes, there are two stage fails at most
    void Smoke::stopCurrentTest(const cv::Mat &frame)
    {
        auto currentTestHolder = currentTest.lock();
        assert(currentTestHolder != nullptr);
        currentTestHolder->SetFinished();
        if(targetDevice != nullptr)
        {
            // stop activity
            targetDevice->StopActivity(packageName);
            // clear data
            targetDevice->AdbShellCommand(string("pm clear ") + packageName, nullptr, 30000);

            vector<string> installedPackage = targetDevice->GetAllInstalledPackageNames();
            if (std::find(installedPackage.begin(), installedPackage.end(), packageName) != installedPackage.end())
            {
                // if the apk is installed, then uninstall, otherwise didn't uninstall
                string uninstallLogFile = this->reportOutputFolder + "\\apk_uninstall_" + packageName + ".txt";
                if (!boost::filesystem::exists(uninstallLogFile))
                {
                    string log;
                    DaVinciStatus status = targetDevice->UninstallAppWithLog(packageName, log);
                    std::ofstream logStream(uninstallLogFile, std::ios_base::app | std::ios_base::out);
                    logStream << log << std::flush;
                    logStream.flush();
                    logStream.close();

                    if (!checkUninstallLog(uninstallLogFile))
                    {
                        this->stageResults[SMOKE_UNINSTALL] = SmokeResult::FAIL;
                    }
                    else
                    {
                        this->stageResults[SMOKE_UNINSTALL] = SmokeResult::PASS;
                    }
                }
            }
        }
    }

    void Smoke::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        this->enGoogleFrameworkWhiteList = keywordList["EnGoogleFrameworkWhiteList"];
        this->zhGoogleFrameworkWhiteList = keywordList["ZhGoogleFrameworkWhiteList"];
    }

    bool Smoke::isGoogleFrameworkIssue(std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts)
    {
        bool frameworkIssue = false;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::GOOGLEFRAMEWORKISSUE);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            std::map<std::string, std::vector<Keyword>> keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            this->enGoogleFrameworkWhiteList = keywordList["EnGoogleFrameworkWhiteList"];
            this->zhGoogleFrameworkWhiteList = keywordList["ZhGoogleFrameworkWhiteList"];
        }
        for (int i = 1; i <= 2; i++)
        {
            if (boost::equals(language, "zh"))
            {
                this->keywordRecog->SetWhiteList(zhGoogleFrameworkWhiteList);
                frameworkIssue = this->keywordRecog->RecognizeKeyword(zhTexts);
            }
            else
            {
                this->keywordRecog->SetWhiteList(enGoogleFrameworkWhiteList);
                frameworkIssue = this->keywordRecog->RecognizeKeyword(enTexts);
            }
            if (frameworkIssue)
            {
                break;
            }
            else
            {
                language = (boost::equals(language, "zh") ? "en" : "zh");
            }
        }
        return frameworkIssue;
    }

    bool Smoke::checkSharingIssue()
    {
        if(isSharingIssue)
        {
            this->stageResults[SMOKE_RANDOM] = SmokeResult::WARNING;
            AppendFailReason(FailureTypeToString(WarMsgSharingIssue));
            return true;
        }
        return false;
    }

    void Smoke::AppendFailReason(string reason)
    {
        if (this->failReason.empty())
        {
            this->failReason = reason;
        }
        else if (!boost::contains(this->failReason, reason))
        {
            this->failReason = this->failReason + " ; " + reason;
        }
    }

    bool Smoke::checkFrameIssue(std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts, cv::Mat frame, int stageIndex)
    {
        boost::shared_ptr<KeywordCheckHandler> handler = keywordCheckHandler1->HandleRequest(language, enTexts, zhTexts);
        if (handler != nullptr)
        {
            FailureType failureType = handler->GetCurrentFailureType();
            AppendFailReason(FailureTypeToString(failureType));
            if(failureType == FailureType::ErrMsgCrash || failureType == FailureType::ErrMsgANR || failureType == FailureType::ErrMsgInitialization)
            {
                this->stageResults[stageIndex] = SmokeResult::FAIL;
                if (exploreEngine != nullptr)
                {
                    Rect okRect = handler->GetOkRect(frame);
                    exploreEngine->dispatchClickOffErrorDialogAction(frame, Point(okRect.x + okRect.width / 2, okRect.y + okRect.height / 2));
                    if (isGoogleFrameworkIssue(enTexts, zhTexts))
                        failReason = failReason + "(Google Framework Issue)";
                }
            }
            else
            {
                this->stageResults[stageIndex] = SmokeResult::WARNING;
            }
            return true;
        }
        return false;
    }

    void Smoke::SetInstallStageResult(const cv::Mat &currentFrame)
    {
        std::string installLogFile = this->reportOutputFolder + std::string("\\apk_install_") + packageName + std::string(".txt");
        this->stageResults[SMOKE_INSTALL] = checkInstallLog(installLogFile) ? SmokeResult::PASS : SmokeResult::FAIL;

        if (this->stageResults[SMOKE_INSTALL] == SmokeResult::FAIL)
        {
            // reset 'Device Incompatible' and 'version downgrade' as warning
            if (this->failReason == FailureTypeToString(WarMsgDeviceIncompatible) || this->failReason == FailureTypeToString(WarMsgDowngrade))
            {
                this->stageResults[SMOKE_INSTALL] = SmokeResult::WARNING;
                stopCurrentTest(currentFrame);
                return;
            }
            else
            {
                // double check if app is installed using "shell pm list packages"
                vector<string> installedPackage = targetDevice->GetAllInstalledPackageNames();
                if (std::find(installedPackage.begin(), installedPackage.end(), packageName) != installedPackage.end())
                {
                    this->stageResults[SMOKE_INSTALL] = SmokeResult::PASS;
                }
                else
                {
                    if (failReason.empty())
                        failReason = FailureTypeToString(ErrMsgInstallFail);
                    stopCurrentTest(currentFrame); // log contain 'Failure', couldn't find the app using "shell pm list packages", stop testing
                    return;
                }
            }
        }
    }

    void Smoke::SetLaunchStageResult(const cv::Mat &currentFrame)
    {
        if (this->stageResults[SMOKE_INSTALL] == SmokeResult::FAIL || this->stageResults[SMOKE_INSTALL] == SmokeResult::WARNING)
        {
            return;
        }
        if (activityName.empty()) // if activity name is empty, SKIP start app
        {
            this->shouldSkip = true;
            AppendFailReason(FailureTypeToString(WarMsgActivityEmpty));
            return;
        }

        saveImageName = this->reportOutputFolder + std::string("\\apk_launch_") + packageName + std::string(".png");
        imwrite(saveImageName, currentFrame);

        getLogcat(); // get logcat before checking, otherwise if isCrashDialog == true, there is no logcat.txt

        cv::Mat weightedFrame = AddWeighted(currentFrame);

        this->exploreEngine->generateAllRectsAndKeywords(currentFrame, Rect(0, 0, currentFrame.size().width, currentFrame.size().height), this->language);    // use system language to check frame issue

        if (checkFrameIssue(this->exploreEngine->getAllEnKeywords(), this->exploreEngine->getAllZhKeywords(), weightedFrame, 1))
        {
            if (this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL)
                stopCurrentTest(currentFrame);
        }
        else
        {
            string anotherLanguage = (boost::equals(language, "en") ? "zh" : "en");         // use another language to check frame issue
            this->exploreEngine->generateAllRectsAndKeywords(currentFrame, Rect(0, 0, currentFrame.size().width, currentFrame.size().height), anotherLanguage);

            if (checkFrameIssue(this->exploreEngine->getAllEnKeywords(), this->exploreEngine->getAllZhKeywords(), weightedFrame, 1))
            {
                if (this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL)
                    stopCurrentTest(currentFrame);
            }
            else if (AppUtil::Instance().HasShowKeyboardIssueApp(packageName) && keyboardRecog->RecognizeKeyboard(currentFrame, this->osVersion))
            {
                this->stageResults[SMOKE_LAUNCH] = SmokeResult::FAIL;
                AppendFailReason(FailureTypeToString(ErrMsgShowKeyboardAfterLaunch));
                stopCurrentTest(currentFrame);
            }
            else
            {
                if(AppUtil::Instance().IsInputMethodApp(packageName))
                {
                    // Skip OPCODE_START_APP for input method apps
                    this->stageResults[SMOKE_LAUNCH] = SmokeResult::PASS;
                }
                else if (AppUtil::Instance().IsLauncherApp(packageName))
                {
                    // Skip OPCODE_START_APP for launcher apps
                    this->stageResults[SMOKE_LAUNCH] = SmokeResult::PASS;
                }
                else if (topPackageName.empty())
                {
                    DAVINCI_LOG_INFO << "TOP package is empty";
                    this->stageResults[SMOKE_LAUNCH] = SmokeResult::FAIL;
                    AppendFailReason(FailureTypeToString(ErrMsgLaunchFail));
                    stopCurrentTest(currentFrame);
                    return;
                }
                else if (isTestedPackage())
                {
                    this->stageResults[SMOKE_LAUNCH] = SmokeResult::PASS;
                }
                else if (!isHomePackage())
                {
                    // Skip OPCODE_START_APP for non-tested valid package
                    this->stageResults[SMOKE_LAUNCH] = SmokeResult::PASS;
                    if (AppUtil::Instance().NeedSkipApp(topPackageName))
                    {
                        this->shouldSkip = true;
                    }
                }
                else // Stay at home
                {
                    if (clickExitButtonToHome)
                    {
                        // Skip OPCODE_START_APP for explicit click to home
                        this->stageResults[SMOKE_LAUNCH] = SmokeResult::PASS;
                        shouldSkip = true;
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "TOP package is home";
                        this->stageResults[SMOKE_LAUNCH] = SmokeResult::FAIL;
                        AppendFailReason(FailureTypeToString(ErrMsgLaunchFail));
                        stopCurrentTest(currentFrame);
                        return;
                    }
                }
            }
        }
    }

    void Smoke::SetExploratoryStageResult(cv::Mat &currentFrame, cv::Mat &originalFrame)
    {
        cv::Mat firstFrame;
        cv::Mat secondFrame;

        cv::Mat afterClickingFrame = currentFrame;      // afterClickingFrame is the real current frame, after clicking action
        currentFrame = this->wholePage;                 // current frame is handled by Exploratory Engine, before clicking action
        if (this->stageResults[0] == SmokeResult::FAIL || this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL || this->stageResults[SMOKE_LAUNCH] == SmokeResult::WARNING || this->shouldSkip == true)
        {
            return;
        }

        if (isFirstFrameAfterLaunch)
        {
            imwrite(ConcatPath(TestReport::currentQsLogPath, "smoke_first_frame.png"), currentFrame);
            isFirstFrameAfterLaunch = false;
        }

        if (frameQueue.size() == 3)
        {
            firstFrame = frameQueue[0];
            secondFrame = frameQueue[1];
        }

        if (this->stageResults[SMOKE_RANDOM] != SmokeResult::WARNING)
        {
            saveImageName = this->reportOutputFolder + std::string("\\apk_random_") + packageName + std::string(".png");
            imwrite(saveImageName, currentFrame);
        }

        if (checkFrameIssue(allEnKeywords, allZhKeywords, AddWeighted(currentFrame), 2))
        {
            if (this->stageResults[SMOKE_RANDOM] == SmokeResult::FAIL)
            {
                saveImageName = this->reportOutputFolder + std::string("\\apk_random_") + packageName + std::string(".png");
                imwrite(saveImageName, currentFrame);
                stopCurrentTest(currentFrame);
            }
        }
        else if (ObjectUtil::Instance().IsBadImage(originalFrame, firstFrame, secondFrame))
        {
            this->stageResults[SMOKE_RANDOM] = SmokeResult::WARNING;
            AppendFailReason(FailureTypeToString(WarMsgAbnormalPage));
        }
        else
        {
            if(AppUtil::Instance().IsInputMethodApp(packageName))
            {
                // Skip OPCODE_EXPLORATORY_TEST for input method apps
                this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
            }
            else if (AppUtil::Instance().IsLauncherApp(packageName))
            {
                // Skip OPCODE_EXPLORATORY_TEST for launcher apps
                this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
            }
            else if (isTestedPackage() || topPackageName.empty())
            {
                if(checkSharingIssue())
                {
                    return;
                }
                if (this->stageResults[SMOKE_RANDOM] == SmokeResult::SKIP)  // didn't set any result of SMOKE_RANDOM stage
                    this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
            }
            else if (!isHomePackage())
            {
                if(checkSharingIssue())
                {
                    return;
                }
                // Skip OPCODE_EXPLORATORY_TEST for non-tested valid package
                if (this->stageResults[SMOKE_RANDOM] == SmokeResult::SKIP)
                    this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
                if (AppUtil::Instance().NeedSkipApp(topPackageName))
                {
                    this->shouldSkip = true;
                }
            }
            else // Stay at home
            {
                if (clickExitButtonToHome)
                {
                    // Skip OPCODE_EXPLORATORY_TEST for explicit click to home
                    if (this->stageResults[SMOKE_RANDOM] == SmokeResult::SKIP)
                        this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
                    shouldSkip = true;
                }
                else
                {
                    if (!activityName.empty())
                    {
                        bool isARC = false;
                        bool isExpectProcessExitForARC = false;

                        // For ARC++ device, if click Ads and launch Chrome outside of container, top pacakge name is HOME package
                        // Need to double check the images before setting the result to Process exit to home
                        if (targetDevice != nullptr)
                            isARC = targetDevice->IsARCdevice();

                        if (isARC)
                        {
                            boost::filesystem::path homeFramePath = ConcatPath(TestReport::currentQsLogPath, "home_frame.png");

                            if (!boost::filesystem::is_regular_file(homeFramePath))
                            {
                                DAVINCI_LOG_WARNING << std::string("smoke test home frame not exist") << std::endl;
                            }
                            else
                            {
                                Mat homeFrame = imread(homeFramePath.string());
                                if (!isSimilarPicture(afterClickingFrame, homeFrame))
                                {
                                    isExpectProcessExitForARC = true;
                                    targetDevice->StartActivity(packageName + "/" + activityName); 
                                    if (this->stageResults[SMOKE_RANDOM] == SmokeResult::SKIP)
                                        this->stageResults[SMOKE_RANDOM] = SmokeResult::PASS;
                                }
                            }
                        }
                        
                        if ((isARC == false) || (isARC == true && isExpectProcessExitForARC == false))
                        {
                            this->stageResults[SMOKE_RANDOM] = SmokeResult::FAIL;
                            AppendFailReason(FailureTypeToString(ErrMsgProcessExit));
                            imwrite(saveImageName, afterClickingFrame);
                            stopCurrentTest(currentFrame);
                            return;
                        }
                    }
                    else
                    {
                        this->stageResults[SMOKE_RANDOM] = SmokeResult::FAIL;
                        AppendFailReason(FailureTypeToString(ErrMsgLaunchFail));
                        saveImageName = this->reportOutputFolder + std::string("\\apk_random_") + packageName + std::string(".png");
                        imwrite(saveImageName, currentFrame);
                        stopCurrentTest(currentFrame);
                        return;
                    }
                }
            }
        }
    }

    void Smoke::SetBackStageResult(const cv::Mat &currentFrame, const cv::Mat &originalFrame)
    {
        // FIXME: in OPCODE_EXPLORATORY_TEST stage, if click a button and exit to Home, will SKIP OPCODE_BACK stage now
        // if user confused about this, need to remove (|| this.shouldSkip == true) in the following condition
        if (this->stageResults[0] == SmokeResult::FAIL || this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL || this->stageResults[SMOKE_RANDOM] == SmokeResult::FAIL || this->shouldSkip == true)
        {
            return;
        }

        saveImageName = this->reportOutputFolder + std::string("\\apk_back_") + packageName + std::string(".png");
        imwrite(saveImageName, currentFrame);

        cv::Mat weightedFrame = AddWeighted(currentFrame);

        this->exploreEngine->generateAllRectsAndKeywords(currentFrame, Rect(0, 0, currentFrame.size().width, currentFrame.size().height), this->language); // use system language to check frame issue

        if (checkFrameIssue(this->exploreEngine->getAllEnKeywords(), this->exploreEngine->getAllZhKeywords(), weightedFrame, 3))
        {
            if (this->stageResults[SMOKE_BACK] == SmokeResult::FAIL)
                stopCurrentTest(currentFrame);
        }
        else
        {
            string anotherLanguage = (boost::equals(language, "en") ? "zh" : "en");         // use another language to check frame issue
            this->exploreEngine->generateAllRectsAndKeywords(currentFrame, Rect(0, 0, currentFrame.size().width, currentFrame.size().height), anotherLanguage);

            if (checkFrameIssue(this->exploreEngine->getAllEnKeywords(), this->exploreEngine->getAllZhKeywords(), weightedFrame, 3))
            {
                if (this->stageResults[SMOKE_BACK] == SmokeResult::FAIL)
                    stopCurrentTest(currentFrame);
            }
            else
            {
                ObjectUtil::Instance().ReadCurrentBarInfo(currentFrame.size().width);
                cv::Rect nBar = ObjectUtil::Instance().GetNavigationBarRect();
                cv::Rect sBar = ObjectUtil::Instance().GetStatusBarRect();

                if (isTestedPackage() && ObjectUtil::Instance().IsPureSingleColor(originalFrame, nBar, sBar))
                {
                    this->stageResults[SMOKE_BACK] = SmokeResult::WARNING;
                    AppendFailReason(FailureTypeToString(WarMsgAbnormalPage));
                }
                else
                {
                    this->stageResults[SMOKE_BACK] = SmokeResult::PASS;
                }
            }
        }
    }

    void Smoke::SetUninstallStageResult(const cv::Mat &currentFrame)
    {
        if (this->stageResults[0] == SmokeResult::FAIL || this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL || this->stageResults[SMOKE_RANDOM] == SmokeResult::FAIL || this->stageResults[SMOKE_BACK] == SmokeResult::FAIL)
        {
            return;
        }

        // use 'pm list' to check application uninstall or not
        vector<string> installedPackage = targetDevice->GetAllInstalledPackageNames();
        if (std::find(installedPackage.begin(), installedPackage.end(), packageName) == installedPackage.end())
        {
            // application is already uninstalled
            this->stageResults[SMOKE_UNINSTALL] = SmokeResult::PASS;
        }
        else
        {
            std::string uninstallLogFile = this->reportOutputFolder + std::string("\\apk_uninstall_") + packageName + std::string(".txt");
            if (!checkUninstallLog(uninstallLogFile))
            {
                this->stageResults[SMOKE_UNINSTALL] = SmokeResult::FAIL;
                AppendFailReason(FailureTypeToString(ErrMsgUninstallFail));
                stopCurrentTest(currentFrame);
                return;
            }
            else
            {
                this->stageResults[SMOKE_UNINSTALL] = SmokeResult::PASS;
            }
        }
    }


    void Smoke::setAppStageResult(const cv::Mat &frame)
    {
        auto currentTestHolder = currentTest.lock();
        assert(currentTestHolder != nullptr);

        if (targetDevice != nullptr)
            topPackageName = targetDevice->GetTopPackageName();

        cv::Mat currentFrame = RotateFrameUp(frame);
        cv::Mat originalFrame = frame.clone();

        if (!originalFrame.empty())
        {
            EnqueueFrame(originalFrame);
        }

        if (!boost::filesystem::exists(this->reportOutputFolder))
        {
            boost::filesystem::create_directory(this->reportOutputFolder);
        }

        if (this->currentSmokeStage == QSEventAndAction::OPCODE_INSTALL_APP)
        {
            SetInstallStageResult(currentFrame);
        }
        else if (this->currentSmokeStage == QSEventAndAction::OPCODE_START_APP)
        {
            SetLaunchStageResult(currentFrame);
        }
        else if (this->currentSmokeStage == QSEventAndAction::OPCODE_EXPLORATORY_TEST)
        {
            SetExploratoryStageResult(currentFrame, originalFrame);
        }
        else if (this->currentSmokeStage == QSEventAndAction::OPCODE_BACK)
        {
            SetBackStageResult(currentFrame, originalFrame);
        }
        else if (this->currentSmokeStage == QSEventAndAction::OPCODE_UNINSTALL_APP)
        {
            SetUninstallStageResult(currentFrame);
        }
    }

    void Smoke::SetReportOutputFolder(string folder)
    {
        this->reportOutputFolder = folder;
    }

    void Smoke::SetStageResults(map<int,int> results)
    {
        this->stageResults = results;
    }

    void Smoke::SetFailureReason(std::string failureReason)
    {
        this->failReason = failureReason;
    }

    std::string Smoke::GetMiddlewareInfo()
    {
        std::string retMiddlewareInfo = "";
        try
        {
            std::string engApkName = to_iso_string(second_clock::local_time()) + "_middleware.apk";
            std::unordered_map<string, bool> dicTriageResult;
            boost::filesystem::path p(apkPath);
            boost::filesystem::path apkName = p.filename();
            std::wstring wstrApkName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkName.string());
            bool containNoneASCII = ContainNoneASCIIStr(wstrApkName);
            bool containSpecialChar = ContainSpecialChar(wstrApkName);

            if (containNoneASCII || containSpecialChar)
            {
                std::wstring wstrApkLocation = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->apkPath);
                boost::filesystem::path srcPath(wstrApkLocation);
                boost::filesystem::path destPath = ConcatPath(TestReport::currentQsLogPath, engApkName);
                if (boost::filesystem::is_regular_file(srcPath) && boost::filesystem::is_directory(TestReport::currentQsLogPath))
                {
                    if(boost::filesystem::is_regular_file(destPath))
                        boost::filesystem::remove(destPath); 
                    boost::filesystem::copy(srcPath, destPath);
                    dicTriageResult = bugTriage->TriageMiddlewareInfo(destPath.string());
                }
            }
            else
            {
                dicTriageResult = bugTriage->TriageMiddlewareInfo(apkPath);
            }

            if (containNoneASCII || containSpecialChar)
            {
                boost::filesystem::path tmpPath = ConcatPath(TestReport::currentQsLogPath, engApkName);
                if(boost::filesystem::is_regular_file(tmpPath))
                    boost::filesystem::remove(tmpPath);
            }

            for (auto pair : dicTriageResult)
            {
                if (pair.second)
                    retMiddlewareInfo = retMiddlewareInfo + pair.first + ";";
            }

            if (!retMiddlewareInfo.empty())
            {
                retMiddlewareInfo = "(" + retMiddlewareInfo.substr(0, retMiddlewareInfo.size()-1) + ")";
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("Smoke: GetMiddlewareInfo - ") << e.what() << std::endl;
            return retMiddlewareInfo;
        }
        return retMiddlewareInfo;
    }

    std::string Smoke::GetHoudiniInfo()
    {
        std::string retHoudiniInfo = "";
        try
        {
            std::string engApkName = to_iso_string(second_clock::local_time()) + "_houdini.apk";
            std::unordered_map<string, bool> dicTriageResult;
            boost::filesystem::path p(apkPath);
            boost::filesystem::path apkName = p.filename();
            std::wstring wstrApkName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkName.string());
            bool containNoneASCII = ContainNoneASCIIStr(wstrApkName);
            bool containSpecialChar = ContainSpecialChar(wstrApkName);
            string missingX86LibName = "";

            if (containNoneASCII || containSpecialChar)
            {
                std::wstring wstrApkLocation = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->apkPath);
                boost::filesystem::path srcPath(wstrApkLocation);
                boost::filesystem::path destPath = ConcatPath(TestReport::currentQsLogPath, engApkName);
                if (boost::filesystem::is_regular_file(srcPath) && boost::filesystem::is_directory(TestReport::currentQsLogPath))
                {
                    if(boost::filesystem::is_regular_file(destPath))
                        boost::filesystem::remove(destPath); 
                    boost::filesystem::copy(srcPath, destPath);
                    dicTriageResult = bugTriage->TriageHoudiniBug(destPath.string(), missingX86LibName);
                }
            }
            else
            {
                dicTriageResult = bugTriage->TriageHoudiniBug(apkPath, missingX86LibName);
            }

            if (containNoneASCII || containSpecialChar)
            {
                boost::filesystem::path tmpPath = ConcatPath(TestReport::currentQsLogPath, engApkName);
                if(boost::filesystem::is_regular_file(tmpPath))
                    boost::filesystem::remove(tmpPath);
            }

            if (dicTriageResult["BangBangProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgBangBangProtection) + std::string(")");
            }
            if (dicTriageResult["WangQinShield"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgWangQinShield) + std::string(")");
            }
            if (dicTriageResult["iJiaMi"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgiJiaMi) + std::string(")");
            }
            if (dicTriageResult["360Protection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsg360Protection) + std::string(")");
            }
            if (dicTriageResult["360ProtectionV2"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsg360ProtectionV2) + std::string(")");
            }
            if (dicTriageResult["BangBangProtectionPaid"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgBangBangProtectionPaid) + std::string(")");
            }
            if (dicTriageResult["NaGaProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgNaGaProtection) + std::string(")");
            }
            if (dicTriageResult["CitrixXenMobile"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgCitrixXenMobile) + std::string(")");
            }
            if (dicTriageResult["DexMaker"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgDexMaker) + std::string(")");
            }
            if (dicTriageResult["CMCCBillingSDK"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgCMCCBillingSDK) + std::string(")");
            }
            if (dicTriageResult["Payegis"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgPayegis) + std::string(")");
            }
            if (dicTriageResult["QQProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgQQProtection) + std::string(")");
            }
            if (dicTriageResult["BaiduProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgBaiduProtection) + std::string(")");
            }
            if (dicTriageResult["VKey"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgVKey) + std::string(")");
            }
            if (dicTriageResult["Wellbia"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgWellbia) + std::string(")");
            }
            if (dicTriageResult["AliProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgAliProtection) + std::string(")");
            }
            if (dicTriageResult["HyperTechProtection"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgHyperTechProtection) + std::string(")");
            }
            if (dicTriageResult["MissX86"])
            {
                retHoudiniInfo = std::string("(") + FailureTypeToString(ErrMsgX86LibMissing) + " " + missingX86LibName + std::string(")");
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("Smoke: GetHoudiniInfo - ") << e.what() << std::endl;
            return retHoudiniInfo;
        }
        return retHoudiniInfo;
    }

    void Smoke::checkVideoFlickering(string videoFileName)
    {
        if((videoFileName).empty() || !boost::filesystem::is_regular_file(videoFileName))
        {
            DAVINCI_LOG_WARNING << "Could not find normal replay video ...";
            return;
        }

        boost::filesystem::path filePath(videoFileName);
        filePath = system_complete(filePath);
        boost::filesystem::path folder = filePath.parent_path();
        boost::filesystem::path videoPath = folder;

        // Check video flicking
        // Find all packageName_replay*.avi videos in current folder
        string videoFile = filePath.stem().string();
        vector<boost::filesystem::path> avifiles;
        testReport->GetAllMediaFiles(folder, avifiles, videoFile);
        if (avifiles.empty())
        {
            DAVINCI_LOG_INFO << "No replay video found to copy.";
            return;
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
                    DAVINCI_LOG_INFO << "Checking flicking frames in replay video: " << replayVideoFile;
                    if (checkFlickering->VideoQualityEvaluation(replayVideoFile) == DaVinciStatusSuccess)
                    {
                        //Add info to XML for report
                        {
                            string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                            string resulttype = "type"; //<test-result type="RnR">
                            string typevalue = "RNR"; 
                            string commonnamevalue = this->qscriptFileName;
                            string totalreasult = "WARNING";
                            if(this->stageResults[SMOKE_INSTALL] == SmokeResult::FAIL ||
                                this->stageResults[SMOKE_LAUNCH] == SmokeResult::FAIL ||
                                this->stageResults[SMOKE_RANDOM] == SmokeResult::FAIL ||
                                this->stageResults[SMOKE_BACK] == SmokeResult::FAIL ||
                                this->stageResults[SMOKE_UNINSTALL] == SmokeResult::FAIL)
                            {
                                totalreasult = "FAIL";
                            }
                            else
                            {
                                //Add result into SMOKE_RANDOM, marked as WarMsgVideoFlickering
                                this->stageResults[SMOKE_RANDOM] = SmokeResult::WARNING;
                                AppendFailReason(FailureTypeToString(WarMsgVideoFlickering));
                            }
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
                    return;
                }
            }
        }
        return;
    }

    void Smoke::generateAppStageResult()
    {
        std::string totalresult = "N/A";
        std::string currentDeviceName = "";
        std::string htmlLinkPath = this->reportOutputFolder;
        if (!boost::filesystem::is_regular_file(this->reportOutput))
        {
            DAVINCI_LOG_WARNING << std::string("Warning: couldn't find ") << this->reportOutput << std::endl;
            return;
        }
        if (DeviceManager::Instance().GetCurrentTargetDevice() != nullptr)
        {
            currentDeviceName = DeviceManager::Instance().GetCurrentTargetDevice()->GetDeviceName();
        }

        if((boost::filesystem::path(htmlLinkPath)).is_absolute() == false)
        {
            htmlLinkPath = std::string(".//") + std::string("\\_Logs\\") + TestReport::currentQsLogName;
        }

        std::string apkNameCSV = this->apkName;
        if (boost::contains(apkNameCSV, "\\") || boost::contains(apkNameCSV, "/"))    // a full path which contains apk name
        {
            boost::filesystem::path apkNamePath(apkNameCSV);
            apkNameCSV = apkNamePath.filename().string();
        }
        if (boost::contains(apkNameCSV, ","))
            boost::replace_all(apkNameCSV, ",", " ");
        std::ofstream logOutput(this->reportOutput, std::ios_base::app | std::ios_base::out);
        logOutput << (std::string("\t") + TestReport::currentQsLogName + std::string(",") + currentDeviceName + std::string(",") + apkNameCSV + std::string(",") + this->appName + std::string(",") + this->packageName + std::string(",") + this->activityName + std::string(",")) << std::flush;;

        bool finalResult = true;
        bool haveWarning = false;
        bool allTestResultIsNA = true;

        // if Random stage is PASS, need to check if Application always loading
        if (stageResults[2] == SmokeResult::PASS)
        {
            string firstImage = ConcatPath(TestReport::currentQsLogPath, "smoke_first_frame.png");
            string lastImage = this->reportOutputFolder + "/apk_random_" + packageName + ".png";

            if (AppUtil::Instance().HasLoadingIssueApp(packageName) && isAppAlwaysLoading(firstImage, lastImage))
            {
                stageResults[2] = SmokeResult::WARNING;
                AppendFailReason(FailureTypeToString(WarMsgAlwaysLoading));
            }
            if ((!AppUtil::Instance().IsInputMethodApp(packageName)) && (!AppUtil::Instance().IsLauncherApp(packageName)) && (isAppAlwaysHomePage()))
            {
                stageResults[2] = SmokeResult::WARNING;
                AppendFailReason(FailureTypeToString(WarMsgAlwaysHomePage));
            }
        }

        for (int i = 0; i < (int)this->stageResults.size(); i++)
        {
            // check screen frozen if final result is PASS
            if ((i == SMOKE_UNINSTALL) && (this->stageResults[i] == SmokeResult::PASS) && (finalResult == true))
            {
                if(screenFrozenChecker->Check() == Fail)
                {
                    this->stageResults[SMOKE_UNINSTALL] = SmokeResult::FAIL;
                    AppendFailReason(FailureTypeToString(ErrMsgScreenFrozen));
                }
            }

            if (this->stageResults[i] == SmokeResult::FAIL)
            {
                logOutput << "FAIL," << std::flush;
                stageResultsForXML[i] = "FAIL";
                finalResult = false;
                haveWarning = false;
                allTestResultIsNA = false;
            }
            else if (this->stageResults[i] == SmokeResult::PASS)
            {
                logOutput << "PASS," << std::flush;
                stageResultsForXML[i] = "PASS";
                allTestResultIsNA = false;
            }
            else if (this->stageResults[i] == SmokeResult::WARNING)
            {
                logOutput << "WARNING," << std::flush;
                stageResultsForXML[i] = "WARNING";
                finalResult = false;
                haveWarning = true;
                allTestResultIsNA = false;
            }
            else if (this->stageResults[i] == SmokeResult::SKIP)
            {
                logOutput << "SKIP," << std::flush;
                stageResultsForXML[i] = "SKIP";
            }
        }

        int appStatus = -1;

        if (!finalResult)
        {
            appStatus = 0;
            if (haveWarning)
            {
                logOutput << "WARNING," << std::flush;
                totalresult = "WARNING";
            }
            else
            {
                logOutput << "FAIL," << std::flush;
                totalresult = "FAIL";
            }
        }
        else // PASS or SKIP
        {
            if (allTestResultIsNA || activityName.empty())
            {
                logOutput << "SKIP," << std::flush;
                totalresult = "SKIP";
                appStatus = -1;
            }
            else
            {
                logOutput << "PASS," << std::flush;
                totalresult = "PASS";
                failReason = "";
                appStatus = 1;
            }
        }

        // update fail reason (related with houdini)
        if (appStatus == 0)
        {
            failReason = failReason + GetHoudiniInfo() + GetMiddlewareInfo();
        }

        logOutput << failReason + std::string(",") << std::flush;
        logOutput << "=HYPERLINK(\"" + htmlLinkPath + "\\ReportSummary.xml\")," << std::flush;
        logOutput << this->loginResult + std::string(",") << this->qscriptFileName + std::string(",") << getApkType() + ",No,Smoke Test\n" << std::flush;
        logOutput.flush();
        logOutput.close();

        if(xmlReportSummary != nullptr)
        {
            string installtxt = std::string("./apk_install_") + packageName + std::string(".txt");
            string launchpng = std::string("./apk_launch_") + packageName + std::string(".png");
            string randompng = std::string("./apk_random_") + packageName + std::string(".png");
            string backpng = std::string("./apk_back_") + packageName + std::string(".png");
            string uninstalltxt = std::string("./apk_uninstall_") + packageName + std::string(".txt");
            string logcattxt = std::string("./apk_logcat_") + packageName + std::string(".txt"); 
            std::wstring wAppName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(this->appName);
            string appname = WstrToStrAnsi(wAppName);
            string apkname = this->apkName;
            string package = this->packageName;
            string activity = this->activityName;

            for (int i = 0; i < (int)this->stageResultsForXML.size(); i++)
            {

                if (i == 0) //Install
                {
                    xmlReportSummary->appendSmokeResultNode(currentReportSummaryName,
                        "type",
                        "RNR",
                        "name",
                        appname,
                        totalresult,
                        failReason,
                        logcattxt,
                        apkname,
                        package,
                        activity,
                        "Install",
                        stageResultsForXML[i], //result
                        "N/A", //start time
                        "N/A", //end time
                        stageResultsForXML[i], //message
                        installtxt); //link
                }
                else if (i == 1) //Launch
                {
                    xmlReportSummary->appendSmokeResultNode(currentReportSummaryName,
                        "type",
                        "RNR",
                        "name",
                        appname,
                        totalresult,
                        failReason,
                        logcattxt,
                        apkname,
                        package,
                        activity,
                        "Launch",
                        stageResultsForXML[i], //result
                        "N/A", //start time
                        "N/A", //end time
                        stageResultsForXML[i], //message
                        launchpng); //link
                }
                else if (i == 2) //Random
                {
                    xmlReportSummary->appendSmokeResultNode(currentReportSummaryName,
                        "type",
                        "RNR",
                        "name",
                        appname,
                        totalresult,
                        failReason,
                        logcattxt,
                        apkname,
                        package,
                        activity,
                        "Random",
                        stageResultsForXML[i], //result
                        "N/A", //start time
                        "N/A", //end time
                        stageResultsForXML[i], //message
                        randompng); //link
                }
                else if (i == 3) //Back
                {
                    xmlReportSummary->appendSmokeResultNode(currentReportSummaryName,
                        "type",
                        "RNR",
                        "name",
                        appname,
                        totalresult,
                        failReason,
                        logcattxt,
                        apkname,
                        package,
                        activity,
                        "Back",
                        stageResultsForXML[i], //result
                        "N/A", //start time
                        "N/A", //end time
                        stageResultsForXML[i], //message
                        backpng); //link
                }
                else if (i == 4) //Uninstall
                {
                    xmlReportSummary->appendSmokeResultNode(currentReportSummaryName,
                        "type",
                        "RNR",
                        "name",
                        appname,
                        totalresult,
                        failReason,
                        logcattxt,
                        apkname,
                        package,
                        activity,
                        "Uninstall",
                        stageResultsForXML[i], //result
                        "N/A", //start time
                        "N/A", //end time
                        stageResultsForXML[i], //message
                        uninstalltxt); //link
                }

            }
        }

        // reset failed reason
        failReason = "";
        shouldSkip = false;
        clickExitButtonToHome = false;
        isFirstFrameAfterLaunch = true;
    }

    void Smoke::generateSmokeCriticalPath()
    {
        boost::filesystem::path outDirPath(TestReport::currentQsLogPath);
        outDirPath /= boost::filesystem::path("GET");
        string outDir = outDirPath.string();

        if (!boost::filesystem::exists(outDir))
            return;

        vector<boost::filesystem::path> allPictures;
        getFiles(outDirPath, allPictures);

        std::map<double, string> validPictures;
        for (size_t index = 0; index < allPictures.size(); ++ index)
        {
            string fileName = allPictures[index].filename().string();

            if (boost::ends_with(fileName, ".png") && !boost::contains(fileName, "_orignal_"))
            {
                vector<string> subFileNames;
                string prefixFileName = fileName.substr(0, fileName.size() - 4);
                boost::algorithm::split(subFileNames, prefixFileName, boost::algorithm::is_any_of("_"));

                if (subFileNames.size() >= 3)
                {
                    try
                    {
                        double timeStamp = boost::lexical_cast<double>(subFileNames.back());
                        validPictures[ceil(timeStamp)] = std::string("./GET/") + fileName;
                    }
                    catch(...)
                    {
                        continue;
                    }
                }
            }
        }

        int i = 1;
        for (auto item : validPictures)
        {
            xmlReportSummary->appendSmokeImage(currentReportSummaryName, qscriptFileName, boost::lexical_cast<string>(i), boost::lexical_cast<string>(item.first), item.second, item.second);
            ++ i;
        }

    }

    bool Smoke::checkInstallLog(const std::string &logFile)
    {
        bool flag = false;
        vector<string> lines;
        if (boost::filesystem::is_regular_file(logFile))
        {
            ReadAllLines(logFile, lines);
            if (lines.size() == 0)
            {
                DAVINCI_LOG_WARNING << "Failed: The installation log is empty";
            }

            for(auto line : lines)
            {
                if(line != "")
                {
                    boost::trim(line);
                    if (boost::equals(line, "Success"))
                    {
                        flag = true;
                        break;
                    }
                    if (boost::starts_with(line,"Failure"))
                    {
                        if (boost::contains(line, "[INSTALL_FAILED_VERSION_DOWNGRADE]"))
                        {
                            failReason = FailureTypeToString(WarMsgDowngrade);
                            DAVINCI_LOG_WARNING << "Installation version downgrade.";
                            break;
                        }
                        if (boost::contains(line, "[INSTALL_FAILED_INSUFFICIENT_STORAGE]"))
                        {
                            failReason = FailureTypeToString(ErrMsgStorage);
                            DAVINCI_LOG_WARNING << "Insufficient storage for application installation.";
                        }
                        if (boost::contains(line, "[INSTALL_FAILED_MEDIA_UNAVAILABLE]"))
                        {
                            failReason = FailureTypeToString(WarMsgDeviceIncompatible);
                            DAVINCI_LOG_WARNING << "Media unavailable.";
                        }
                        flag = false;
                        break;
                    }
                    if (boost::equals(line, "- waiting for device -"))
                    {
                        DAVINCI_LOG_WARNING << "Failed to connect device during installation stage";
                    }
                }
            }

            if (flag == false && failReason.empty())
            {
                failReason = "Install Failed";
            }
            return flag;
        }
        else
        {
            DAVINCI_LOG_WARNING << std::string("Warning: couldn't find apk_install.txt") << std::endl;
            failReason = "Install Failed";
            return flag;
        }
    }

    bool Smoke::checkUninstallLog(const std::string &logFile)
    {
        vector<string> lines;
        if (boost::filesystem::is_regular_file(logFile))
        {
            ReadAllLines(logFile, lines);
            std::string last_line;
            for(auto line : lines)
            {
                boost::trim(line);
                if (line != "")
                {
                    last_line = line;
                }
            }
            // log contains 'success'
            if (boost::equals(last_line, "Success"))
            {
                return true;
            }
        }
        // double check if the app is a system app
        if(targetDevice != nullptr)
        {
            vector<string> systemPackage = targetDevice->GetSystemPackageNames();
            if (std::find(systemPackage.begin(), systemPackage.end(), packageName) != systemPackage.end())
            {
                // if app is a system app, couldn't uninstall it, DaVinci result should be 'PASS'
                return true;
            }
        }
        return false;
    }

    bool Smoke::isAppAlwaysHomePage()
    {
        if(isRnRMode)
            return false;

        bool flag = false;
        try
        {
            boost::filesystem::path smokeFirstFramePath = ConcatPath(TestReport::currentQsLogPath, "smoke_first_frame.png");
            boost::filesystem::path lastFramePath(this->reportOutputFolder + "/apk_random_" + packageName + ".png");
            boost::filesystem::path homeFramePath = ConcatPath(TestReport::currentQsLogPath, "home_frame.png");

            if (!boost::filesystem::is_regular_file(smokeFirstFramePath) || !boost::filesystem::is_regular_file(homeFramePath) || !boost::filesystem::is_regular_file(lastFramePath))
            {
                DAVINCI_LOG_WARNING << std::string("smoke test first frame not exist or home frame not exist or last frame not exist") << std::endl;
                return flag;
            }
            Mat firstFrame = imread(smokeFirstFramePath.string());
            Mat homeFrame = imread(homeFramePath.string());
            Mat lastFrame = imread(lastFramePath.string());

            // double check the first frame with home frame and last frame with home frame
            if (isSimilarPicture(firstFrame, homeFrame) && isSimilarPicture(lastFrame, homeFrame))
            {
                flag = true;
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("Smoke: isAppAlwaysHomePage - ") << e.what() << std::endl;
            return false;
        }
        return flag;
    }

    bool Smoke::isSimilarPicture(Mat image1, Mat image2)
    {
        bool flag = false;
        try
        {
            if ((image1.size().width != image2.size().width) || (image1.size().height != image2.size().height))
            {
                return false;
            }

            // check the diff between two images
            cv::Mat diffFrame;
            vector<Mat> diffChannels;

            cv::absdiff(image1, image2, diffFrame);
            split(diffFrame, diffChannels);
            int zeroCount = countNonZero(diffChannels[0]);

            Size imageSize = image1.size();
            if (zeroCount < imageSize.height * imageSize.width * 0.001)
            {
                flag = true;
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("Smoke: isSimilarPicture - ") << e.what() << std::endl;
            return false;
        }
        return flag;
    }

    // TODO: use akaze
    bool Smoke::isAppAlwaysLoading(string firstImage, string lastImage, double allowRatio)
    {
        bool flag = false;

        try
        {
            boost::filesystem::path firstFramePath(firstImage);
            boost::filesystem::path lastFramePath(lastImage);
            if (!boost::filesystem::is_regular_file(firstFramePath) || !boost::filesystem::is_regular_file(lastFramePath))
            {
                DAVINCI_LOG_WARNING << std::string("First frame or last frame does not exist") << std::endl;
                return flag;
            }

            Mat image1 = imread(firstImage);
            Mat image2 = imread(lastImage);

            if ((image1.size().width != image2.size().width) || (image1.size().height != image2.size().height))
            {
                return false;
            }

            // check the diff between first frame and last frame of random stage
            cv::Mat diffFrame;
            vector<Mat> diffChannels;

            cv::absdiff(image1, image2, diffFrame);
            split(diffFrame, diffChannels);
            int zeroCount = countNonZero(diffChannels[0]);

            Size imageSize = image1.size();
            if (zeroCount < imageSize.height * imageSize.width * allowRatio)
            {
                flag = true;
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("Smoke: isAppAlwaysLoading - ") << e.what() << std::endl;
            return false;
        }

        return flag;
    }
}

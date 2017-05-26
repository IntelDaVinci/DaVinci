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

#include "ExploratoryEngine.hpp"
#include "BaseObjectCategory.hpp"
#include "UiAction.hpp"
#include "UiStateSwipe.hpp"
#include "UiStateHome.hpp"
#include "UiStateMenu.hpp"
#include "UiStateBack.hpp"
#include "UiRectangle.hpp"
#include "DeviceManager.hpp"
#include "ObjectCommon.hpp"
#include "DaVinciCommon.hpp"
#include "TestManager.hpp"
#include "TextUtil.hpp"
#include <ctime>
#include "UiStateClickable.hpp"
#include "UiStateGeneral.hpp"
#include "ScriptRecorder.hpp"

namespace DaVinci
{
    void ExploratoryEngine::Init()
    {
        objRecog = boost::shared_ptr<ObjectRecognizer>(new ObjectRecognizer()) ;
        downloadRecog = boost::shared_ptr<DownloadAppRecognizer>(new DownloadAppRecognizer());
        keyboardRecog = boost::shared_ptr<KeyboardRecognizer>(new KeyboardRecognizer());
        loginRecog = boost::shared_ptr<LoginRecognizer>(new LoginRecognizer());
        launcherRecog = boost::shared_ptr<LauncherAppRecognizer>(new LauncherAppRecognizer());
        waitPlayStoreTimeoutWatch = boost::shared_ptr<StopWatch>(new StopWatch());
        commonKeywordRecog = boost::shared_ptr<CommonKeywordRecognizer>(new CommonKeywordRecognizer());
        sharingKeywordRecog = boost::shared_ptr<SharingKeywordRecognizer> (new SharingKeywordRecognizer());
        rnrKeywordRecog = boost::shared_ptr<RnRKeywordRecognizer>(new RnRKeywordRecognizer());
        crossRecognizer = boost::shared_ptr<CrossRecognizer>(new CrossRecognizer());

        keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        akazeRecog = boost::shared_ptr<AKazeObjectRecognize>(new AKazeObjectRecognize());
        featMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(akazeRecog));

        boost::filesystem::path outDirPath(TestReport::currentQsLogPath);
        outDirPath /= boost::filesystem::path("DOWNLOAD");
        downloadImageDir = outDirPath.string();

        downloadStage = DownloadStage::NOTBEGIN;
        readyToLauncher = false;
        completeAdobe = false;
        completeLicense = false;
        completeTutorial = false;
        checkNeedLogin = false;
        needLogin = false;
        loginResult = "SKIP";
        completeAccount = false;
        completePassword = false;
        completeLogin = false;
        completeLoginResultJudgement = false;
        completeKeyword = false;
        completeAds = false;
        checkLoginByAccurateKeyword = false;

        isKeyboard = false;
        isPopUpWindow = false;
        focusedWindow = EmptyRect;
        focusedWindowOnFrame = EmptyRect;

        isLoginPage = false;
        isLoginButton = false;
        isPatternOne = true;
        isIncompatibleApp = false;
        isFree = true;

        repeatedState = 0;
        uiStateHashCode = 0;
        accountNameIndex = 0;

        languageModeForLogin = ObjectUtil::Instance().GetSystemLanguage();
        currentLanguageMode = "";
        matchCurrentLanguage = false;
        isExpectedProcessExit = false;
        isSharingIssue = false;
        stopWatch = boost::shared_ptr<StopWatch>(new StopWatch());
        viewParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
        maxQueueCount = 3;
        checkSharingIssue = false;
        this->randomGenerator = initRandomGenerator();
        clickedKeywords.clear();
    }

    ExploratoryEngine::ExploratoryEngine(const string &pn, const string &an, const string &in, const boost::shared_ptr<TargetDevice> &device, const string version) :
        packageName(pn), activityName(an), iconName(in), dut(device), osVersion(version),treeId(0)
    {
        imageBoxInfo.width = 0;
        imageBoxInfo.height = 0;
        imageBoxInfo.orientation = DeviceOrientation::OrientationPortrait;
        Init();
    }

    ExploratoryEngine::ExploratoryEngine()
    {
        packageName = "";
        activityName = "";
        iconName = "";
        dut = DeviceManager::Instance().GetCurrentTargetDevice();
        treeId = 0;
        imageBoxInfo.width = 0;
        imageBoxInfo.height = 0;
        imageBoxInfo.orientation = DeviceOrientation::OrientationPortrait;
        Init();
    }

    void ExploratoryEngine::EnqueueFocusWindowActivity(const std::string &focusWindowActivity)
    {
        if (activityQueue.size() == maxQueueCount)
        {
            activityQueue.erase(activityQueue.begin());
        }
        activityQueue.push_back(focusWindowActivity);
    }

    void ExploratoryEngine::EnqueueFrame(const cv::Mat &frame)
    {
        if (frameQueue.size() == maxQueueCount)
        {
            frameQueue.erase(frameQueue.begin());
        }
        frameQueue.push_back(frame);
    }

    boost::shared_ptr<UiState> ExploratoryEngine::FilterRectangleForARC(boost::shared_ptr<UiState> uiState, const cv::Mat rotatedFrame)
    {
        double ratio = 0.1;
        boost::shared_ptr<UiState> newState = boost::shared_ptr<UiState>(new UiState());
        std::vector<boost::shared_ptr<UiStateObject>> clickableObjects = uiState->GetClickableObjects();

        for (auto object : clickableObjects)
        {
            UiRectangle r = object->GetRect();
            UiPoint p = r.GetCenter();

            if ((static_cast<double>(p.GetY()) / static_cast<double>(rotatedFrame.size().height)) > ratio)      // filter the top buttons for ARC++ device
            {
                newState->AddUiStateObject(object);
            }
        }

        return newState;
    }

    bool ExploratoryEngine::dispatchGeneralAction(const boost::shared_ptr<CoverageCollector> &covCollector, const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint)
    {
        cv::Mat originalFrame = rotatedFrame.clone();
        bool retValue = false;

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut == nullptr)
        {
            return false;
        }

        bool isARC = androidDut->IsARCdevice();
        Size rotatedFrameSize = rotatedFrame.size();
        Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
        std::vector<std::string> windowLines;
        AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
        vector<string> lines = dut->GetTopActivityViewHierarchy();

        const int minArea = 500; // icon size >= 25 * 25
        int maxArea = static_cast<int>(rotatedFrameSize.width * rotatedFrameSize.height * 0.25);

        boost::shared_ptr<UiState> uiState = boost::shared_ptr<UiState>(new UiState());
        
        Mat rotatedMainFrame; 
        vector<ObjectLabelNode> objectNodes;
        ObjectUtil::Instance().CopyROIFrame(rotatedFrame, this->focusedWindowOnFrame, rotatedMainFrame);

        DebugSaveImage(rotatedMainFrame,"rotatedMainFrame.png");
        objRecog->DetectObjectLabels(rotatedMainFrame, objectNodes);
        vector<ObjectLabelNode> confidentLabelObjects = objRecog->SelectObjectLables(objectNodes, 0.6, 10);
        
        if (confidentLabelObjects.size() > 0)
        {
              uiState = objRecog->LabelObjectToUiObject(confidentLabelObjects, rotatedFrame, this->focusedWindowOnFrame);
              
              if (isARC)  // for ARC++ device, need to filter top buttons
                uiState = FilterRectangleForARC(uiState, rotatedFrame);
              
              retValue = dispatchProbableAction(rotatedFrame, covCollector, biasObject, uiState, seedValue, cloneFrame, clickPoint);

              // If click 'back' button OR click ads button on ARC++ device, isExpectedProcessExit is true
              if ((dut->GetTopPackageName() == dut->GetHomePackageName()) && (isExpectedProcessExit == false))
                  isExpectedProcessExit = isClickBackButton(clickPoint, uiState);
        }
        else
        {

            uiState = objRecog->recognizeObjects(rotatedFrame, lines, ObjectType::OBJECT_IMAGE, minArea, maxArea, deviceSize, windowBarInfo.focusedWindowRect, this->focusedWindowOnFrame);
            
            if (isARC)  // for ARC++ device, need to filter top buttons
                uiState = FilterRectangleForARC(uiState, rotatedFrame);
            
            retValue = dispatchProbableAction(rotatedFrame, covCollector, biasObject, uiState, seedValue, cloneFrame, clickPoint);
        }
        return retValue;
    }

    bool ExploratoryEngine::isClickBackButton(cv::Point clickPoint, boost::shared_ptr<UiState> uiState)
    {
        bool retValue =  false;

        std::vector<boost::shared_ptr<UiStateObject>> clickableObjects = uiState->GetClickableObjects();

        for (size_t i = 0; i < clickableObjects.size(); ++i)
        {
            boost::shared_ptr<UiStateObject> clickObject = clickableObjects[i];
            UiRectangle r = clickObject->GetRect();
            Rect rect(Point(r.GetLeftUpper().GetX(), r.GetLeftUpper().GetY()), Point(r.GetRightBottom().GetX(), r.GetRightBottom().GetY()));
            string label = clickObject->GetText();

            if (rect.contains(clickPoint))
            {
                if (label == ObjectLabelToString(ObjectLabel::OBJECT_BACK))
                {
                    retValue = true;
                    break;
                }
            }
        }
        return retValue;   
    }

    bool ExploratoryEngine::MatchLaunchConfirmManufacturer(string str)
    {
        if(boost::equals(str, "ASUS"))
            return true;

        return false;
    }

    bool ExploratoryEngine::ClickOffLaunchDialog(boost::shared_ptr<TargetDevice> dut)
    {
        if(dut == nullptr)
        {
            DAVINCI_LOG_WARNING<< "Clicking launch dialog meets device nullptr";
            return false;
        }

        String manufacturer = dut->GetSystemProp("ro.product.manufacturer");
        boost::to_upper(manufacturer);

        if (manufacturer.empty() || (!MatchLaunchConfirmManufacturer(manufacturer)))
            return false;

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if(androidDut == nullptr)
        {
            DAVINCI_LOG_WARNING<< "Clicking launch dialog meets android device nullptr";
            return false;
        }

        int timeout = WaitForLaunchConfirmTimeout;
        boost::shared_ptr<StopWatch> launchConfirmWatch = boost::shared_ptr<StopWatch>(new StopWatch());
        launchConfirmWatch->Start();
        while (true)
        {
            if(launchConfirmWatch->ElapsedMilliseconds() > timeout)
            {
                launchConfirmWatch->Stop();
                break;
            }

            std::vector<std::string> windowLines;
            AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
            string focusWindow;
            AndroidTargetDevice::GetWindows(windowLines, focusWindow);
            if(focusWindow != "android")
            {
                DAVINCI_LOG_DEBUG << "No need to click off dialog during app launch";
                ThreadSleep(WaitForLaunchConfirmWait);
                continue;
            }

            cv::Mat frame = dut->GetScreenCapture();
            Orientation orientation = dut->GetCurrentOrientation(true);
            Size replayDeviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
            Point fwPoint(windowBarInfo.focusedWindowRect.x, windowBarInfo.focusedWindowRect.y);

            if(!ObjectUtil::Instance().IsPopUpDialog(replayDeviceSize,orientation, windowBarInfo.statusbarRect, windowBarInfo.focusedWindowRect))
            {
                ThreadSleep(WaitForLaunchConfirmWait);
                continue;
            }

            Rect replayFrameMainROI = AndroidTargetDevice::GetWindowRectangle(fwPoint, windowBarInfo.focusedWindowRect.width, windowBarInfo.focusedWindowRect.height, replayDeviceSize, orientation, frame.size().width);
            Mat replayFrameROI;
            ObjectUtil::Instance().CopyROIFrame(frame, replayFrameMainROI, replayFrameROI);

            string initialLanguage = ObjectUtil::Instance().GetSystemLanguage();
            vector<Point> points;
            Mat rotatedFrameROI = RotateFrameUp(replayFrameROI);
            vector<Keyword> allKeywords;
            string matchedLanguage;
            MatchPopUpKeyword(replayFrameROI, points, initialLanguage, matchedLanguage, allKeywords);

            if(points.size() == 0)
            {
                ThreadSleep(WaitForLaunchConfirmWait);
                continue;
            }

            Size frameSize = frame.size();
#ifdef _DEBUG
            Mat debugFrame = frame.clone();
#endif
            for(auto clickPoint : points)
            {
                Point point = TransformRotatedFramePointToFramePoint(rotatedFrameROI.size(), clickPoint, orientation, frameSize);
                point = ObjectUtil::Instance().RelocatePointByOffset(point, replayFrameMainROI, true);
                ClickOnFrame(dut, frame.size(), point.x, point.y, orientation);
                DAVINCI_LOG_INFO << "Click launch keyword @(" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
#ifdef _DEBUG
                DrawCircle(debugFrame, clickPoint);
#endif
            }

#ifdef _DEBUG
            SaveImage(frame, "launchDialog.png");
            SaveImage(debugFrame, "launchDialogClicks.png");
#endif
        }
        return true;
    }

    bool ExploratoryEngine::MatchPopUpKeyword(const Mat &replayFrame, vector<Point> &targetPoints, string initialLanguage, string& matchedLanguage, std::vector<Keyword>& allKeywords)
    {
        targetPoints.clear();
        matchedLanguage = initialLanguage;
        vector<Rect> proposalRects;

        for (int i = 1; i <= 2; i++)
        {
            if(matchedLanguage == "en")
                proposalRects = ObjectUtil::Instance().DetectRectsByContour(replayFrame, 240, 6, 1, 0.5, 20, 300, 95000);
            else
                proposalRects = ObjectUtil::Instance().DetectRectsByContour(replayFrame, 240, 5, 1, 0.5, 20, 300, 95000);

            this->keywordRecog->SetProposalRects(proposalRects, true);
            allKeywords = this->keywordRecog->DetectAllKeywords(replayFrame, matchedLanguage);

            bool isRecognized = this->rnrKeywordRecog->RecognizeRnRKeyword(allKeywords, matchedLanguage);
            if (isRecognized)
            {
                std::vector<Keyword> checkBoxes = this->rnrKeywordRecog->GetCheckBoxes();
                cv::Rect checkBoxLocation;
                if (!checkBoxes.empty())
                {
                    checkBoxLocation = checkBoxes[0].location;
                    Point point = Point(checkBoxLocation.x + checkBoxLocation.width / 2, checkBoxLocation.y + checkBoxLocation.height / 2);
                    targetPoints.push_back(point);
                }

                std::vector<Keyword> keywords = this->rnrKeywordRecog->GetKeywords();
                if (!keywords.empty())
                {
                    for(auto keyword : keywords)
                    {
                        cv::Rect keywordLocation = keyword.location;
                        if(keywordLocation.y > checkBoxLocation.y + checkBoxLocation.height)
                        {
                            Point point = Point(keywordLocation.x + keywordLocation.width / 2, keywordLocation.y + keywordLocation.height / 2);
                            targetPoints.push_back(point);
                            break;
                        }
                    }
                }

                break;
            }
            else
            {
                matchedLanguage = (boost::equals(matchedLanguage, "zh") ? "en" : "zh");
            }
        }

        if(targetPoints.size() > 0)
            return true;
        else 
            return false;
    }

    // Similar to home launcher selection
    bool ExploratoryEngine::dispatchLauncherAction(const cv::Mat &rotatedFrame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        launcherRecog->SetIconName(iconName);
        if(launcherRecog->RecognizeLauncherCandidate(rotatedFrame, allEnProposalRects, allZhProposalRects))
        {
            Rect launchAppRect = launcherRecog->GetLauncherAppRect();
            Point rotatedPoint = Point(launchAppRect.x + launchAppRect.width / 2, launchAppRect.y + launchAppRect.height / 2);
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), rotatedPoint);

            Rect alwaysRect = launcherRecog->GetAlwaysRect();
            rotatedPoint = Point(alwaysRect.x + alwaysRect.width / 2, alwaysRect.y + alwaysRect.height / 2);
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), rotatedPoint);
            clickPoint = rotatedPoint;

            return true;
        }

        return false;
    }

    bool ExploratoryEngine::dispatchLicenseAction(const cv::Mat &rotatedFrame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        ObjectAttributes objAttribute = ObjectAttributes(rotatedFrame);
        LicenseRecognizer licenseRecog = LicenseRecognizer();

        if (licenseRecog.BelongsTo(objAttribute, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords))
        {
#ifdef _DEBUG
            cv::Mat frameCopy = rotatedFrame.clone();
            DebugSaveImage(frameCopy,"license.png");
#endif
            Rect agreeRect = licenseRecog.GetAgreeRect();
            Rect checkboxRect = licenseRecog.GetCheckBoxRect();
            Point rotatedPoint = Point(agreeRect.x + agreeRect.width / 2, agreeRect.y + agreeRect.height / 2);

            // first click agree button
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), rotatedPoint);
            clickPoint = rotatedPoint;
#ifdef _DEBUG
            DrawCircle(frameCopy, Point(rotatedPoint.x, rotatedPoint.y), Red, 10);
#endif
            // then select checkbox, click agree button
            if (checkboxRect != EmptyRect)
            {
                Point rotatedPoint2 = Point(checkboxRect.x + checkboxRect.width / 2, checkboxRect.y + checkboxRect.height / 2);

                ThreadSleep(1000);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), rotatedPoint2);
#ifdef _DEBUG
                DrawCircle(frameCopy, Point(rotatedPoint2.x, rotatedPoint2.y), Red, 10);
#endif
                ThreadSleep(1000);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), rotatedPoint2);
                clickPoint = rotatedPoint2;
            }
#ifdef _DEBUG
            DebugSaveImage(frameCopy,"licenseClick.png");
#endif
            this->completeLicense = true;
            return true;
        }
        return false;
    }

    bool ExploratoryEngine::dispatchTutorialAction(const cv::Mat &rotatedFrame, cv::Mat &cloneFrame)
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            if (isPopUpWindow)
                return false;

            for (int i = 1; i <= 5; i++)
            {
                // drag from 3/4 y-axis
                RecordAndRunDragQEvent(3000, 3060, 512, 3060, androidDut->GetCurrentOrientation(true), true);
                ThreadSleep(1000);
                if (dut->CheckSurfaceFlingerOutput("InputMethod"))      // if popup keyboard, stop tutorial action
                    break;
            }

            DrawPoints(cloneFrame, SwipeDirection::LEFT);

            this->completeTutorial = true;
            return true;
        }

        return false;
    }

    bool ExploratoryEngine::dispatchAccountAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        bool result = false;
        std::vector<cv::Rect> newLoginRects;

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            if ((this->isKeyboard) || (androidDut->IsARCdevice()))      // ARC++ device didn't popup keyboard
            {
                bool loginPageForInput = true;

                for (int i = 1; i <= 2; i++)
                {
                    std::vector<Keyword> loginPageKeywords;
                    this->loginRecog->SetLanguageMode(this->languageModeForLogin);

                    if (boost::equals(this->languageModeForLogin, "zh"))
                    {
                        loginPageKeywords = allZhKeywords;
                    }
                    else
                    {
                        loginPageKeywords = allEnKeywords;
                    }

                    loginPageForInput = this->loginRecog->RecognizeLoginPage(rotatedFrame, loginPageKeywords);
                    if (loginPageForInput)
                    {
                        break;
                    }
                    else
                    {
                        this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
                    }
                }

                if (loginPageForInput)
                {
                    DAVINCI_LOG_INFO << "Use new account rectangle, and start to input.";
                    newLoginRects = this->loginRecog->GetLoginInputRects(); // Login rect recognized the second time (with keyboard)
                }
                else // For water-marks (clicking keyword makes account keyword disappear): reuse the old account rects
                {
                    DAVINCI_LOG_INFO << "Use old account rectangle, and start to input.";
                    newLoginRects = this->loginInputRects;
                }

                // Input account
                // Use 3 / 4 instead of 1 / 2
                // Handle the case that account rectangle has been inputted, we need to make sure where we click is on the right of the inputted string
                cv::Point accountPoint;
                if (!AppUtil::Instance().NeedClearAccountInputApp(packageName))
                {
                    accountPoint = cv::Point(newLoginRects[0].x + newLoginRects[0].width * 3 / 4, newLoginRects[0].y + newLoginRects[0].height / 2);
                }
                else
                {
                    accountPoint = cv::Point(newLoginRects[0].x + newLoginRects[0].width, newLoginRects[0].y + newLoginRects[0].height / 2);
                }
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), accountPoint);
                clickPoint = accountPoint;

                ThreadSleep(500);
                if(AppUtil::Instance().NeedClearAccountInputApp(packageName))
                {
                    androidDut->ClearInput();
                    ThreadSleep(500);
                }
                std::map<int, std::vector<string>> accountInfor = biasObject->GetAccountInfo();
                RecordAndRunSetTextQEvent(accountInfor[this->accountNameIndex].front());
                
                ThreadSleep(1000);

                // For some apps with prompt box when inputting account and it will overlap the password rect
                // Click to make the prompt box disappear (no side effect)
                cv::Point passwordPoint = cv::Point(newLoginRects[1].x + newLoginRects[1].width / 2, newLoginRects[1].y + newLoginRects[1].height / 2);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), passwordPoint);
                clickPoint = passwordPoint;
                ThreadSleep(2000);

                if (this->isKeyboard)
                {
                    androidDut->PressBack(); // Fold keyboard to handle the case that password rect is overlapped by keyboard
                    ThreadSleep(2000);      // sleep 2 seconds after pressing back, otherwise dispatchPasswordAction() will get in-between frame
                }
                this->completeAccount = true;

                result = true;
            }
            else
            {
                DAVINCI_LOG_INFO << "Fail to recognize the correct keyboard page for inputting.";
                result = false;
            }
        }
        else
        {
            DAVINCI_LOG_INFO << "ExploratoryEngine::dispatchAccountAction androidDut is null";
            result = false;
        }
        return result;
    }

    bool ExploratoryEngine::dispatchPasswordAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        bool result = false;
        if (this->loginInputRects.size() < 2)       // if size < 2, get loginInputRects[1] will throw exception
            return result;

        cv::Rect passwordRect = this->loginInputRects[1];
        int y = 0;

        for (int i = 1; i <= 2; i++)
        {
            std::vector<Keyword> passwordKeywords;
            this->loginRecog->SetLanguageMode(this->languageModeForLogin);

            if (boost::equals(this->languageModeForLogin, "zh"))
            {
                passwordKeywords = allZhKeywords;
            }
            else
            {
                passwordKeywords = allEnKeywords;
            }

            y = this->loginRecog->RecognizeNewPassword(rotatedFrame, passwordKeywords, this->loginInputRects[0].br().y);
            if (y != 0)
            {
                passwordRect.y = y;
                break;
            }
            else
            {
                this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
            }
        }

        if (passwordRect != EmptyRect)
        {
            cv::Point passwordPoint = cv::Point(passwordRect.x + passwordRect.width / 2, passwordRect.y + passwordRect.height / 2);

            if (this->isPatternOne)
            {
                DAVINCI_LOG_INFO << "Start to input password.";

                // Input password
                ClickOnRotatedFrame(rotatedFrame.size(), passwordPoint);
                clickPoint = passwordPoint;

                boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                if (androidDut != nullptr)
                {
                    ThreadSleep(500);
                    std::map<int, std::vector<string>> accountInfor = biasObject->GetAccountInfo();
                    RecordAndRunSetTextQEvent(accountInfor[accountNameIndex].back());
                    //androidDut->TypeString(accountInfor[accountNameIndex].back());
                    int newIndex = this->accountNameIndex + 1;
                    this->setAccountNameIndex((newIndex % biasObject->GetAccountInfo().size()));
                    ThreadSleep(1000);

                    if (androidDut->CheckSurfaceFlingerOutput("InputMethod"))      // Fold keyboard to handle the case that login button is overlapped by keyboard
                        androidDut->PressBack(); 
                    this->completePassword = true;
                }
            }
            else
            {
                DAVINCI_LOG_INFO << "Start to click \"next\" button.";
                this->isLoginPage = false;

                // Click "next" button
                ClickOnRotatedFrame(rotatedFrame.size(), passwordPoint);
                clickPoint = passwordPoint;

                ThreadSleep(10000);
            }

            result = true;
        }

        return result;
    }

    void ExploratoryEngine::checkLoginButtonForNoneAccount(const cv::Mat &rotatedFrame)
    {
        this->languageModeForLogin = ObjectUtil::Instance().GetSystemLanguage();

        for (int i = 1; i <= 2; i++) // For now, we only recognize login button under two language modes: simplified chinese and not simplified chinese
        {
            std::vector<Keyword> loginButtonKeywords;
            this->loginRecog->SetLanguageMode(this->languageModeForLogin);

            if (boost::equals(this->languageModeForLogin, "zh"))
            {
                loginButtonKeywords = allZhKeywords;
            }
            else
            {
                loginButtonKeywords = allEnKeywords;
            }

            // the 4th parameter is true, will macth accurate keyword
            this->isLoginButton = this->loginRecog->RecognizeLoginButton(rotatedFrame, loginButtonKeywords, false, true);
            if (this->isLoginButton)
            {
                this->loginResult = "Warning (Didn't prepare account for this login app)";
                break;
            }
            else
            {
                this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
            }
        }
    }

    void ExploratoryEngine::checkLoginPage(const cv::Mat &rotatedFrame, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        this->languageModeForLogin = ObjectUtil::Instance().GetSystemLanguage();

        for (int i = 1; i <= 2; i++) // For now, we only recognize login page under two language modes: simplified chinese and not simplified chinese
        {
            std::vector<Keyword> loginPageKeywords;
            this->loginRecog->SetLanguageMode(this->languageModeForLogin);

            loginPageKeywords = (boost::equals(this->languageModeForLogin, "zh") ? allZhKeywords : allEnKeywords);

            this->isLoginPage = this->loginRecog->RecognizeLoginPage(rotatedFrame, loginPageKeywords);
            if (this->isLoginPage)
            {
                break;
            }
            else
            {
                this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
            }
        }
    }

    std::vector<cv::Rect> ExploratoryEngine::checkLoginButton(const cv::Mat &rotatedFrame)
    {
        // Keep login buttons from last recognition; it is empty at first time
        std::vector<cv::Rect> &loginButtonRects = this->loginRecog->GetLoginButtonRects();

        // Handle false positive of login button recognition
        if (this->isLoginButton && !ObjectUtil::Instance().IsImageChanged(this->preFrame, this->currentFrame))
        {
            DAVINCI_LOG_INFO << "Choose another login button for login app.";
        }
        else
        {
            for (int i = 1; i <= 2; i++) // For now, we only recognize login button under two language modes: simplified chinese and not simplified chinese
            {
                std::vector<Keyword> loginButtonKeywords;
                this->loginRecog->SetLanguageMode(this->languageModeForLogin);

                loginButtonKeywords = (boost::equals(this->languageModeForLogin, "zh") ? allZhKeywords: allEnKeywords);

                this->isLoginButton = this->loginRecog->RecognizeLoginButton(rotatedFrame, loginButtonKeywords);
                if (this->isLoginButton)
                {
                    break;
                }
                else
                {
                    this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
                }
            }

            if (this->isLoginButton)
            {
                DAVINCI_LOG_INFO << "Succeed in recognizing login button for " << this->languageModeForLogin << ".";
                loginButtonRects = this->loginRecog->GetLoginButtonRects();
            }
            else
            {
                loginButtonRects.clear();
            }
        }
        return loginButtonRects;
    }

    bool ExploratoryEngine::dispatchLoginAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        bool result = false;

        if (!this->checkNeedLogin && biasObject->GetAccountInfo().size() > 0)
        {
            this->needLogin = true;
            this->checkNeedLogin = true;
        }

        // If no account info, double check whether there is login button or not
        if (!checkLoginByAccurateKeyword && !this->needLogin)
        {
            checkLoginButtonForNoneAccount(rotatedFrame);
            checkLoginByAccurateKeyword = true;
        }

        // Has account info
        if (this->needLogin)
        {
            this->languageModeForLogin = ObjectUtil::Instance().GetSystemLanguage();

            SaveImage(rotatedFrame, "logining.png");

            if (!this->isLoginPage) // The first time to recognize login page
            {
                DAVINCI_LOG_INFO << "Start to recognize login page for login app.";

                checkLoginPage(rotatedFrame, allEnKeywords, allZhKeywords);

                if (this->isLoginPage)
                {
                    DAVINCI_LOG_INFO << "Succeed in recognizing login page for " << this->languageModeForLogin << ".";

                    // Keep some useful variables the first time recognized, will be used later
                    this->isPatternOne = this->loginRecog->IsPatternOne();
                    this->loginInputRects = this->loginRecog->GetLoginInputRects();

                    // Use 3 / 4 instead of 1 / 2
                    // Handle the case that accunt rectangle has been inputted, we need to make sure where we click is on the right of the inputted string
                    cv::Point accountRectPoint;
                    accountRectPoint = (AppUtil::Instance().NeedClearAccountInputApp(packageName) ? cv::Point(this->loginInputRects[0].x + this->loginInputRects[0].width, this->loginInputRects[0].y + this->loginInputRects[0].height / 2) : cv::Point(this->loginInputRects[0].x + this->loginInputRects[0].width * 3 / 4, this->loginInputRects[0].y + this->loginInputRects[0].height / 2));
                    RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), accountRectPoint);
                    clickPoint = accountRectPoint;
                    ThreadSleep(2000);

                    result = true;
                }
                else
                {
                    DAVINCI_LOG_INFO << "Start to recognize login button for login app.";

                    std::vector<cv::Rect> loginButtonRects = checkLoginButton(rotatedFrame);

                    if (!loginButtonRects.empty())
                    {
                        cv::Rect loginButton = loginButtonRects[0];
                        cv::Point loginButtonPoint = cv::Point(loginButton.x + loginButton.width / 2, loginButton.y + loginButton.height / 2);
                        RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), loginButtonPoint);
                        clickPoint = loginButtonPoint;
                        ThreadSleep(3000);

                        // Remove the clicked login button for false positive one
                        loginButtonRects.erase(loginButtonRects.begin());
                        result = true;
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "Cannot recognize login button for login app.";
                        result = false;
                    }
                }
            }
            else if (!this->completeAccount) // Login page is recognized, but not input account
            {
                result = this->dispatchAccountAction(rotatedFrame, biasObject, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint);
            }
            else if (!this->completePassword) // Login page is recognized, but not input password
            {
                result = this->dispatchPasswordAction(rotatedFrame, biasObject, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint);
            }
            else if (!this->completeLogin) // Account and password are inputted, but not click login button to login
            {
                SaveImage(rotatedFrame, TestManager::Instance().GetDaVinciResourcePath("beforeLogin.png"));

                for (int i = 1; i <= 2; i++) // For now, we only recognize login button under two language modes: simplified chinese and not simplified chinese
                {
                    std::vector<Keyword> loginButtonKeywords;
                    this->loginRecog->SetLanguageMode(this->languageModeForLogin);

                    loginButtonKeywords = (boost::equals(this->languageModeForLogin, "zh") ? allZhKeywords: allEnKeywords);

                    this->isLoginButton = this->loginRecog->RecognizeLoginButton(rotatedFrame, loginButtonKeywords, true);
                    if (this->isLoginButton)
                    {
                        break;
                    }
                    else
                    {
                        this->languageModeForLogin = (boost::equals(this->languageModeForLogin, "zh") ? "en" : "zh");
                    }
                }

                if (isLoginButton)
                {
                    DAVINCI_LOG_INFO << "Click login button to login.";

                    std::vector<cv::Rect> loginButtonRects = this->loginRecog->GetLoginButtonRects();
                    cv::Rect loginButton = loginButtonRects[0];
                    cv::Point loginButtonPoint = cv::Point(loginButton.x + loginButton.width / 2, loginButton.y + loginButton.height / 2);
                     
                    RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), loginButtonPoint);
                    clickPoint = loginButtonPoint;
                    ThreadSleep(WaitForLoginSuccessfully);

                    this->completeLogin = true;
                    result = true;

                    if(AppUtil::Instance().NeedSwipeAfterLogin(packageName))
                    {
                        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                        if (androidDut != nullptr)
                        {
                            for (int i = 1; i <= 5; i++)
                            {
                                
                                RecordAndRunDragQEvent(3000, 2048, 1024, 2048, androidDut->GetCurrentOrientation(), true);
                                ThreadSleep(1000);
                            }
                        }
                    }
                }
                else
                {
                    // Send enter command
                    boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
                    if (androidDut != nullptr)
                    {
                        DAVINCI_LOG_INFO << "Press enter to login.";
                         // The first enter to eliminate mouse cursor
                        RecordAndRunButtonQEvent("ENTER");
                        
                        ThreadSleep(500);
                        // The sencond enter to login
                        RecordAndRunButtonQEvent("ENTER");
                        ThreadSleep(500);

                        ThreadSleep(WaitForLoginSuccessfully);      // sleep 10 seconds waiting for login successfully, otherwise the picture is not changed, DaVinci will set login fail
                        this->completeLogin = true;
                        result = true;

                        if(AppUtil::Instance().NeedSwipeAfterLogin(packageName))
                        {
                            for (int i = 1; i <= 5; i++)
                            {
                                RecordAndRunDragQEvent(3000, 2048, 1024, 2048, androidDut->GetCurrentOrientation(), true);
                                ThreadSleep(1000);
                            }
                        }
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "Cannot complete logining.";
                        result = false;
                    }
                }
            }
            else if (!this->completeLoginResultJudgement) // Login button is clicked, but not judge login result
            {
                SaveImage(rotatedFrame, TestManager::Instance().GetDaVinciResourcePath("afterLogin.png"));

                bool loginFailByKeyword = this->JudgeLoginFailByKeyword(languageModeForLogin, allEnKeywords, allZhKeywords);
                bool loginFailFullPage = this->JudgeLoginFailOnFullPart(TestManager::Instance().GetDaVinciResourcePath("beforeLogin.png"), TestManager::Instance().GetDaVinciResourcePath("afterLogin.png"));
                if (loginFailByKeyword == true || loginFailFullPage == true)
                {
                    this->loginResult = "FAIL";
                }
                else
                {
                    this->loginResult = "PASS";
                }

                this->completeLoginResultJudgement = true;
                result = true;
            }
        }
        else
        {
            result = false;
        }

        return result;
    }

    bool ExploratoryEngine::JudgeLoginFailByKeyword(std::string language, std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts)
    {
        bool loginFail = false;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::LOGINFAIL);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            std::map<std::string, std::vector<Keyword>> keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            this->enLoginFailWhiteList = keywordList["EnLoginFailWhiteList"];
            this->zhLoginFailWhiteList = keywordList["ZhLoginFailWhiteList"];
        }

        for (int i = 1; i <= 2; i++)
        {
            if (boost::equals(language, "zh"))
            {
                this->keywordRecog->SetWhiteList(zhLoginFailWhiteList);
                loginFail = this->keywordRecog->RecognizeKeyword(zhTexts);
            }
            else
            {
                this->keywordRecog->SetWhiteList(enLoginFailWhiteList);
                loginFail = this->keywordRecog->RecognizeKeyword(enTexts);
            }
            if (loginFail)
            {
                break;
            }
            else
            {
                language = (boost::equals(language, "zh") ? "en" : "zh");
            }
        }
        return loginFail;
    }

    bool ExploratoryEngine::JudgeLoginFailOnFullPart(boost::filesystem::path path1, boost::filesystem::path path2)
    {
        bool isFailed = true;
        try
        {
            if (!boost::filesystem::is_regular_file(path1) || !boost::filesystem::is_regular_file(path2))
            {
                DAVINCI_LOG_WARNING << std::string("picture before login or after login not exist") << std::endl;
                return isFailed;
            }

            Mat image1 = imread(path1.string());
            Mat image2 = imread(path2.string());

            const double maxUnmatchRatio = 0.5;
            double unMatchRatio = 0;
            double* pUnmatchRatio = &unMatchRatio;

            this->featMatcher->SetModelImage(image1);
            featMatcher->Match(image2, pUnmatchRatio);

            if (*pUnmatchRatio > maxUnmatchRatio)
                isFailed = false;
            else
                isFailed = true;
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("ExploratoryEngine:: JudgeLoginFailOnFullPart - ") << e.what() << std::endl;
            return true;
        }
        return isFailed;
    }

    bool ExploratoryEngine::dispatchDowloadAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, Point &clickPoint)
    {
        bool result = true;

        if (this->downloadStage == DownloadStage::STOP)
        {
            return false;
        }

        if (!boost::filesystem::exists(downloadImageDir))
        {
            boost::system::error_code ec;
            boost::filesystem::create_directory(downloadImageDir, ec);
            DaVinciStatus status(ec);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << downloadImageDir;
            }
        }

        string currentTimeStamp = to_iso_string(second_clock::local_time());
        boost::filesystem::path saveDirPath(downloadImageDir);
        saveDirPath /= boost::filesystem::path("downloading" + currentTimeStamp + ".png");
        SaveImage(rotatedFrame, saveDirPath.string());

        // For ARC++ device, there is a popup window and need to click 'continue' button
        if (this->isPopUpWindow && this->downloadRecog->RecognizeRandomKeyword(rotatedFrame))
        {
            DAVINCI_LOG_INFO << "Find popup message box" << endl;
            cv::Rect randomRect = this->downloadRecog->GetRandomRect();
            cv::Point point = cv::Point(randomRect.x + randomRect.width / 2, randomRect.y + randomRect.height / 2);
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
            clickPoint = point;
            DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
            boost::filesystem::path saveDirPath(downloadImageDir);
            saveDirPath /= boost::filesystem::path("download_popup_" + currentTimeStamp + ".png");
            SaveImage(rotatedFrame, saveDirPath.string());
            ThreadSleep(1000);
        }

        if (this->downloadStage == DownloadStage::NOTBEGIN)
        {
            DAVINCI_LOG_INFO << "Step 1 - start to launch the app from Play Store" << endl;
            boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
            boost::filesystem::path saveDirPath(downloadImageDir);
            if (androidDut != nullptr)
            {
                androidDut->AdbCommand("shell am start -a android.intent.action.VIEW -d market://details?id=" + biasObject->GetApp());
                ThreadSleep(WaitForPlayStore);

                string topActivity = dut->GetTopActivityName();
                DAVINCI_LOG_INFO << "Top activity: " << topActivity << endl;
                if(topActivity == "com.android.internal.app.ResolverActivity")
                {
                    this->downloadStage = DownloadStage::CHOOSE;
                }
                else
                {
                    string topPackageName = dut->GetTopPackageName();
                    waitPlayStoreTimeoutWatch->Start();
                    while(topPackageName != "com.android.vending")
                    {
                        if(waitPlayStoreTimeoutWatch->ElapsedMilliseconds() > WaitForPlayStoreTimeout)
                        {
                            waitPlayStoreTimeoutWatch->Stop();
                            DAVINCI_LOG_WARNING << "Cannot start Play Store (timeout for Play Store top package)";
                            saveDirPath /= boost::filesystem::path("download_fail_start_timeout_" + currentTimeStamp + ".png");
                            SaveImage(rotatedFrame, saveDirPath.string());
                            result = false;
                            break;
                        }

                        ThreadSleep(WaitForPlayStoreIncremental);
                        topPackageName = dut->GetTopPackageName();
                    }
                    waitPlayStoreTimeoutWatch->Stop();
                    this->downloadStage = DownloadStage::PRICE;
                }
            }
            else
            {
                DAVINCI_LOG_WARNING << "Cannot start Play Store (androidDut is null)" << endl;
                saveDirPath /= boost::filesystem::path("download_fail_start_null_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                result = false;
            }
        }
        else if (this->downloadStage == DownloadStage::CHOOSE)
        {
            DAVINCI_LOG_INFO << "Step 2 - start to choose Play Store launch way" << endl;
            Rect playRect = EmptyRect, okRect = EmptyRect;
            boost::filesystem::path saveDirPath(downloadImageDir);
            if (this->downloadRecog->RecognizeChoosePage(rotatedFrame, playRect, okRect))
            {
                cv::Point point = cv::Point(playRect.x + playRect.width / 2, playRect.y + playRect.height / 2);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                
                clickPoint = point;
                DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
                ThreadSleep(1000);

                if(okRect != EmptyRect)
                {
                    point = cv::Point(okRect.x + okRect.width / 2, okRect.y + okRect.height / 2);
                    RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                    clickPoint = point;
                    DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
                    ThreadSleep(1000);
                }

                saveDirPath /= boost::filesystem::path("download_success_choose_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());

                this->downloadStage = DownloadStage::PRICE;
            }
            else
            {
                DAVINCI_LOG_WARNING << "Cannot choose Play Store." << endl;
                saveDirPath /= boost::filesystem::path("download_fail_choose_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                result = false;
            }
        }
        else if (this->downloadStage == DownloadStage::PRICE)
        {
            boost::filesystem::path saveDirPath(downloadImageDir);

            if (this->downloadRecog->RecognizeIncompatible(rotatedFrame))
            {
                this->isIncompatibleApp = true;
                boost::filesystem::path txtPath = saveDirPath / boost::filesystem::path("incompatible.txt");
                std::ofstream ofs(txtPath.string(), ios::out);
                ofs << "true";
                ofs.close();

                return false;
            }

            DAVINCI_LOG_INFO << "Step 3 - start to click install for free app or price for paid app" << endl;

            if (this->downloadRecog->RecognizeDownloadPage(rotatedFrame, this->downloadStage))
            {
                std::string recognizedPriceStr = this->downloadRecog->GetRecognizedPriceStr();
                if (boost::starts_with(recognizedPriceStr, "un")) // Uninstall, which means app has already been installed
                {
                    this->downloadStage = DownloadStage::STOP;
                    result = false;
                }
                else
                {
                    isFree = this->downloadRecog->IsFree();
                    double recognizedPrice = this->downloadRecog->GetRecognizedPrice();
                    double limitedPrice = biasObject->GetPrice();
                    if ((!isFree && FSmallerE(recognizedPrice, limitedPrice)) || isFree)
                    {
                        cv::Rect priceRect = this->downloadRecog->GetPriceRect();
                        cv::Point point = cv::Point(priceRect.x + priceRect.width / 2, priceRect.y + priceRect.height / 2);
                        RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                        clickPoint = point;
                        DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
                        ThreadSleep(1000);

                        saveDirPath /= boost::filesystem::path("download_success_install_" + currentTimeStamp + ".png");
                        SaveImage(rotatedFrame, saveDirPath.string());
                        // Go into stage accept
                        this->downloadStage = DownloadStage::ACCEPT;
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "App price " << recognizedPrice << " exceeds limited price " << limitedPrice << endl;
                        saveDirPath /= boost::filesystem::path("download_fail_install_paid_" + currentTimeStamp + ".png");
                        SaveImage(rotatedFrame, saveDirPath.string());
                        this->downloadStage = DownloadStage::STOP;
                        result = false;
                    }
                }
            }
            else
            {
                DAVINCI_LOG_WARNING << "Cannot click install for free app or price for paid app" << endl;
                result = false;
                saveDirPath /= boost::filesystem::path("download_fail_install_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
            }
        }
        else if (this->downloadStage == DownloadStage::ACCEPT)
        {
            DAVINCI_LOG_INFO << "Step 4 - start to click accept button" << endl;
            boost::filesystem::path saveDirPath(downloadImageDir);
            if (this->downloadRecog->RecognizeDownloadPage(rotatedFrame, this->downloadStage))
            {
                cv::Rect acceptRect = this->downloadRecog->GetPriceRect();
                cv::Point point = cv::Point(acceptRect.x + acceptRect.width / 2, acceptRect.y + acceptRect.height / 2);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                clickPoint = point;
                DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
                ThreadSleep(1000);

                saveDirPath /= boost::filesystem::path("download_success_accept_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                this->downloadStage = (isFree ? DownloadStage::COMPLETE : DownloadStage::BUY);
            }
            else
            {
                DAVINCI_LOG_WARNING << "Cannot click accept button" << endl;
                saveDirPath /= boost::filesystem::path("download_fail_accept_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                result = false;
            }
        }
        else if (this->downloadStage == DownloadStage::BUY)
        {
            DAVINCI_LOG_INFO << "Step 5 - start to click buy button" << endl;
            boost::filesystem::path saveDirPath(downloadImageDir);
            if (this->downloadRecog->RecognizeDownloadPage(rotatedFrame, this->downloadStage))
            {
                cv::Rect buyRect = this->downloadRecog->GetPriceRect();
                cv::Point point = cv::Point(buyRect.x + buyRect.width / 2, buyRect.y + buyRect.height / 2);
                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                clickPoint = point;
                DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
                ThreadSleep(1000);

                saveDirPath /= boost::filesystem::path("download_success_buy_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                this->downloadStage = DownloadStage::COMPLETE;
            }
            else
            {
                DAVINCI_LOG_WARNING << "Cannot click buy button" << endl;
                saveDirPath /= boost::filesystem::path("download_fail_buy_" + currentTimeStamp + ".png");
                SaveImage(rotatedFrame, saveDirPath.string());
                result = false;
            }
        }

        if ((!result && this->downloadStage != DownloadStage::STOP && this->downloadRecog->RecognizeRandomKeyword(rotatedFrame))
            || (this->downloadStage == DownloadStage::COMPLETE && this->downloadRecog->RecognizeRandomKeyword(rotatedFrame)))
        {
            DAVINCI_LOG_INFO << "Step 6 - start to click random button" << endl;
            cv::Rect randomRect = this->downloadRecog->GetRandomRect();
            cv::Point point = cv::Point(randomRect.x + randomRect.width / 2, randomRect.y + randomRect.height / 2);
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
            clickPoint = point;
            DAVINCI_LOG_INFO << "Click point (" << boost::lexical_cast<string>(point.x) << "," << boost::lexical_cast<string>(point.y) << ")" << endl;
            boost::filesystem::path saveDirPath(downloadImageDir);
            saveDirPath /= boost::filesystem::path("download_random_" + currentTimeStamp + ".png");
            SaveImage(rotatedFrame, saveDirPath.string());
            ThreadSleep(1000);
        }

        return result;
    }

    bool ExploratoryEngine::dispatchKeywordAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords, Point &clickPoint)
    {
        bool result = true;
        std::string languageModeForKeyword = ObjectUtil::Instance().GetSystemLanguage();

        int eventChooser = this->randomGenerator.GetRandomWithRange(1, 100);

        if (eventChooser >= ceil(biasObject->GetDispatchCommonKeywordPossibility()*100))
        {
            DAVINCI_LOG_INFO << "Needn't click common keyword.";
            return false;
        }

        // Mark keyboard button
        // If has keyboard, "next" and "done" may be keyboard button, not keywords
        cv::Rect conflictKeyword = EmptyRect;

        if (this->isKeyboard)
        {
            keyboardRecog->RecognizeKeyboard(rotatedFrame, osVersion);
            std::unordered_map<char, cv::Rect> keyboardMap = this->keyboardRecog->GetKeyboardMapDictionary();
            conflictKeyword = keyboardMap['`'];
        }

        // Keep keywords from the last round, but empty at the first time
        std::vector<Keyword> &keywords = this->commonKeywordRecog->GetKeywords();

        // If click keyword in the last round and the frame not change, then the keyword is false positive
        double ratio = 0.06;        // the default value is 0.025 which is too small, always click the same point
        if (this->completeKeyword && !isPopUpWindow && !ObjectUtil::Instance().IsImageChanged(this->preFrame, this->currentFrame, ratio))
        {
            DAVINCI_LOG_INFO << "Choose another keyword.";
        }
        else // If the first time to recognize keyword or keyword recognized in the last round is true positive, then recognize afresh
        {
            bool isRecognized = false;
            for (int i = 1; i <= 2; i++)
            {
                std::vector<Keyword> allKeywords;
                if (boost::equals(languageModeForKeyword, "zh"))
                {
                    allKeywords = allZhKeywords;
                }
                else
                {
                    allKeywords = allEnKeywords;
                }

                // sort the rectangles by area before recognize common keyword
                std::sort(allKeywords.begin(), allKeywords.end(), SortKeywordsByKeywordAreaAsc());

                isRecognized = this->commonKeywordRecog->RecognizeCommonKeyword(allKeywords, languageModeForKeyword);
                if (isRecognized)
                {
                    keywords = this->commonKeywordRecog->GetKeywords();
                    break;
                }
                else
                {
                    languageModeForKeyword = (boost::equals(languageModeForKeyword, "zh") ? "en" : "zh");
                }
            }

            if (!isRecognized)
            {
                keywords.clear();
            }
        }

        if (!keywords.empty())
        {
            cv::Rect keywordLocation = keywords[0].location;
            if (!ContainRect(conflictKeyword, keywordLocation)) // If not keyboard button, then click keyword and delete this clicked one
            {
                DAVINCI_LOG_INFO << "[info] Click keyword: " << std::endl;
                DAVINCI_LOG_INFO << WstrToStr(keywords[0].text) << std::endl;

                cv::Point point = cv::Point(keywordLocation.x + keywordLocation.width / 2, keywordLocation.y + keywordLocation.height / 2);

                std::vector<Keyword> sharingKeywords;
                sharingKeywords.push_back(keywords[0]);
                bool isSharingIssueRecognized = this->sharingKeywordRecog->RecognizeKeyword(sharingKeywords, languageModeForKeyword);
                string focusWindowBeforeSharing, focusWindowAfterSharing;
                if(isSharingIssueRecognized)
                {
                    this->checkSharingIssue = true;
                }
                else
                {
                    this->checkSharingIssue = false;
                }

                RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
                ThreadSleep(WaitForKeywordAction);

                clickPoint = point;
            }
            else
            {
                DAVINCI_LOG_INFO << "The keyword is actually the keyboard button \"next\" or \"done\", not click and go to the next round.";
            }

            // If the keyword is true positive, the next round will recognize keywords afresh; if the keyword is false positive, will click the next keyword
            // So delete the first one, has no side effect
            keywords.erase(keywords.begin());

            this->completeKeyword= true;
        }
        else
        {
            DAVINCI_LOG_INFO << "There is no keyword to click.";
            result = false;
        }

        return result;
    }

    bool ExploratoryEngine::dispatchAdsAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<BiasObject> &biasObject, Point &clickPoint)
    {
        int eventChooser = this->randomGenerator.GetRandomWithRange(1, 100);
        if (eventChooser >= ceil(biasObject->GetDispatchAdsPossibility()*100))
        {
            DAVINCI_LOG_INFO << "Needn't click off the Ads.";
            return false;
        }

        double ratio = 0.1;
        if (completeAds && !ObjectUtil::Instance().IsImageChanged(this->preFrame, this->currentFrame, ratio))   // if already dispatch ads action but image didn't change, try other actions
            return false;

        bool crossFound = crossRecognizer->BelongsTo(rotatedFrame);
        if(crossFound)
        {
            Point point = crossRecognizer->GetCrossCenterPoint();
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), point);
            clickPoint = point;
            DAVINCI_LOG_INFO << "Click off the Ads.";

            ThreadSleep(4000);
            completeAds = true;
            return true;
        }
        return false;
    }

    bool ExploratoryEngine::dispatchTextAction(const cv::Mat &rotatedFrame, Point &clickPoint)
    {
        if (isKeyboard)
        {
            boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
            if (androidDut != nullptr)
            {
                RecordAndRunSetTextQEvent(std::string("Hello, Shanghai"));                
                ThreadSleep(1000);
                RecordAndRunBackQEvent();        // Hide the keyboard
                ThreadSleep(2000);
            }

            keyboardRecog->RecognizeKeyboard(rotatedFrame, osVersion);
            std::unordered_map<char, Rect> keyboardMap = keyboardRecog->GetKeyboardMapDictionary();
            Rect doneRect = keyboardMap['`'];
            Point donePoint = Point(doneRect.x + doneRect.width / 2, doneRect.y + doneRect.height / 2);
            RecordAndRunClickOnRotatedQEvent(rotatedFrame.size(), donePoint);
            clickPoint = donePoint;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ExploratoryEngine::dispatchClickOffErrorDialogAction(const cv::Mat &frame, const Point &clickPoint)
    {
        ClickOnRotatedFrame(frame.size(), clickPoint, true);
        ThreadSleep(clickCrashDialogWaitTime);
        ClickOnRotatedFrame(frame.size(), clickPoint, true);
        ThreadSleep(clickCrashDialogWaitTime);
        ClickOnRotatedFrame(frame.size(), clickPoint, true);
        return true;
    }

    void ExploratoryEngine::generateAllRectsAndKeywords(const cv::Mat frame, cv::Rect focusedWindowOnFrame, string languageMode, int longSideRatio)
    {
        // add weight
        cv::Mat focusedFrame;
        if (focusedWindowOnFrame != EmptyRect)
        {
            ObjectUtil::Instance().CopyROIFrame(frame, focusedWindowOnFrame, focusedFrame);
        }
        else
        {
            focusedWindowOnFrame = Rect(0, 0, frame.size().width, frame.size().height);
            focusedFrame = frame.clone();
        }

        cv::Mat weightedFrame = AddWeighted(focusedFrame);

        if (languageMode == "en")
        {
            // detect all rects on the image using en mode parameter
            std::vector<cv::Rect> enRectOnFrame = ObjectUtil::Instance().DetectRectsByContour(focusedFrame, 240, 6, 1, 0.5, longSideRatio, 300, 95000);
            std::vector<cv::Rect> enRectOnWeightedFrame = ObjectUtil::Instance().DetectRectsByContour(weightedFrame, 240, 6, 1, 0.5, longSideRatio, 300, 95000);
			std::vector<cv::Rect> enRectOnHighCannyFrame = ObjectUtil::Instance().DetectRectsByContour(focusedFrame, 580, 6, 1, 0.5, longSideRatio, 300, 95000);

            this->allEnProposalRects = MergeVectors(enRectOnFrame, MergeVectors(enRectOnHighCannyFrame, enRectOnWeightedFrame));
            this->allEnProposalRects = ObjectUtil::Instance().RemoveSimilarRects(this->allEnProposalRects, 10);

            // recognize all the keywords using en mode
            this->keywordRecog->SetProposalRects(allEnProposalRects, true);
            this->allEnKeywords = this->keywordRecog->DetectAllKeywords(focusedFrame, "en");

            // Relocate rectangles
            this->allEnProposalRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), this->allEnProposalRects, focusedWindowOnFrame);
            this->allEnKeywords = RelocateRectInKeywords(this->allEnKeywords, frame, focusedWindowOnFrame);

            this->allZhProposalRects.clear();
            this->allZhKeywords.clear();
        }
        else
        {
            // detect all rects on the image using zh mode parameter
            std::vector<cv::Rect> zhRectOnFrame = ObjectUtil::Instance().DetectRectsByContour(focusedFrame, 240, 5, 1, 0.5, longSideRatio, 300, 95000);
            std::vector<cv::Rect> zhRectOnWeightedFrame = ObjectUtil::Instance().DetectRectsByContour(weightedFrame, 240, 5, 1, 0.5, longSideRatio, 300, 95000);
			std::vector<cv::Rect> zhRectOnHighCannyFrame = ObjectUtil::Instance().DetectRectsByContour(focusedFrame, 580, 5, 1, 0.5, longSideRatio, 300, 95000);

            this->allZhProposalRects = MergeVectors(zhRectOnFrame, MergeVectors(zhRectOnHighCannyFrame, zhRectOnWeightedFrame));
            this->allZhProposalRects = ObjectUtil::Instance().RemoveSimilarRects(this->allZhProposalRects, 10);

            // recognize all the keywords using zh mode
            this->keywordRecog->SetProposalRects(allZhProposalRects, true);
            this->allZhKeywords = this->keywordRecog->DetectAllKeywords(focusedFrame, "zh");

            // Relocate rectangles
            this->allZhProposalRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), this->allZhProposalRects, focusedWindowOnFrame);
            this->allZhKeywords = RelocateRectInKeywords(this->allZhKeywords, frame, focusedWindowOnFrame);

            this->allEnProposalRects.clear();
            this->allEnKeywords.clear();
        }

#ifdef _DEBUG
        SaveImage(focusedFrame, "focusedFrame.png");

        cv::Mat debugFrame = frame.clone();
        int i = 0;
        for(auto rect: allEnProposalRects)
        {
            Mat small;
            debugFrame(rect).copyTo(small);
            SaveImage(small, "contour_en_" + boost::lexical_cast<string>(i++) + ".png");
        }

        DrawRects(debugFrame, allEnProposalRects, Red, 2);
        SaveImage(debugFrame, "debug_all_en_proposal_rects.png");

        i = 0;
        for(auto rect: allZhProposalRects)
        {
            Mat small;
            debugFrame(rect).copyTo(small);
            SaveImage(small, "contour_zh_" + boost::lexical_cast<string>(i++) + ".png");
        }

        debugFrame = frame.clone();
        DrawRects(debugFrame, allZhProposalRects, Red, 2);
        SaveImage(debugFrame, "debug_all_zh_proposal_rects.png");
#endif
    }

    std::vector<Keyword> ExploratoryEngine::RelocateRectInKeywords(std::vector<Keyword> keywords, const cv::Mat rotatedFrame, cv::Rect focusedWindowOnFrame)
    {
        std::vector<Keyword> retKeywords;

        for (Keyword k : keywords)
        {
            k.location = ObjectUtil::Instance().RelocateRectangle(rotatedFrame.size(), k.location, focusedWindowOnFrame);
            retKeywords.push_back(k);
        }

        return retKeywords;
    }

    bool ExploratoryEngine::CheckSharingIssueByFrame(std::vector<cv::Mat> frameQueue)
    {
        bool ret = false;
        double ratio = 0.1; 
        if (frameQueue.size() <= 1)
        {
            ret = false;
        }
        else if (frameQueue.size() == 2)
        {
            if (!ObjectUtil::Instance().IsImageChanged(frameQueue[0], frameQueue[1], ratio))
                ret = true;
        }
        else if (frameQueue.size() == 3)
        {
            if ((!ObjectUtil::Instance().IsImageChanged(frameQueue[2], frameQueue[1], ratio)) || (!ObjectUtil::Instance().IsImageChanged(frameQueue[2], frameQueue[0], ratio)))
                ret = true;
        }
        return ret;
    }

    bool ExploratoryEngine::CheckSharingIssueByActivity(std::vector<std::string> activityQueue)
    {
        bool ret = false;
        if (activityQueue.size() <= 1)
        {
            ret = false;
        }
        else if (activityQueue.size() == 2)
        {
            if (activityQueue[0] == activityQueue[1])
                ret = true;
        }
        else if (activityQueue.size() == 3)     // compare current activity with the last activity and the activity before last activity
        {
            if ((activityQueue[2] == activityQueue[1]) || (activityQueue[2] == activityQueue[0]))
                ret = true;
        }
        return ret;
    }

    bool ExploratoryEngine::generateExploratoryAction(const boost::shared_ptr<CoverageCollector> &covCollector, const cv::Mat &rotatedFrame, cv::Rect focusedWindowOnFrame, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint)
    {
        this->wholePage = rotatedFrame;

        if (!this->currentFrame.empty())
        {
            this->preFrame = this->currentFrame.clone();
        }
        this->currentFrame = rotatedFrame.clone();

        if (currentLanguageMode == "")  // first time, using system language
        {
            currentLanguageMode = ObjectUtil::Instance().GetSystemLanguage();
        }
        else
        {
            if (matchCurrentLanguage == false)   // couldn't determine language, switch
            {
                currentLanguageMode = (boost::equals(currentLanguageMode, "en") ? "zh" : "en");
            }
            else
            {
                DAVINCI_LOG_INFO << "Find out current language " << currentLanguageMode;
            }
        }

        generateAllRectsAndKeywords(rotatedFrame, this->focusedWindowOnFrame, currentLanguageMode);  // get all keywords and rects from a frame

        EnqueueFrame(rotatedFrame);
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            isKeyboard = dut->CheckSurfaceFlingerOutput("InputMethod");

            if (this->checkSharingIssue)            // if need to check sharing issue, check activity queue
            {
                if (CheckSharingIssueByActivity(activityQueue) && CheckSharingIssueByFrame(frameQueue))
                {
                    isSharingIssue = true;
                }
                this->checkSharingIssue = false;    // reset the flag of check sharing issue to false
            }

            if (biasObject->GetDownload()) 
            {
                if (!this->isIncompatibleApp && dispatchDowloadAction(rotatedFrame, biasObject, clickPoint))
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                if(dut->GetTopActivityName() == launcherRecog->GetLauncherSelectTopActivity())
                {
                    readyToLauncher = true;
                    return false;
                }

                if (dispatchLoginAction(rotatedFrame, biasObject, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint))
                {
                    matchCurrentLanguage = true;
                }
                else if (readyToLauncher && dispatchLauncherAction(rotatedFrame, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint))
                {
                    matchCurrentLanguage = true;
                    readyToLauncher = false;
                }
                else if (!this->completeLicense && dispatchLicenseAction(rotatedFrame, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint))
                {
                    matchCurrentLanguage = true;
                }
                else if (dispatchKeywordAction(rotatedFrame, biasObject, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords, clickPoint))
                {
                    matchCurrentLanguage = true;
                }
                else if ((!this->isKeyboard) && (!this->completeTutorial) && dispatchTutorialAction(rotatedFrame, cloneFrame))
                {
                }
                else if (dispatchTextAction(rotatedFrame, clickPoint))
                {
                }
                else
                {
                    dispatchGeneralAction(covCollector, rotatedFrame, biasObject, seedValue,  cloneFrame, clickPoint);
                }

                // Record the text which DaVinci clicked, for the checker of 'process exit to home' 
                RecordClickedKeyword(clickPoint, rotatedFrame);

                if((dut->GetTopPackageName() == dut->GetHomePackageName()) && (isExpectedProcessExit == false))     // exit to home, if click Home or Back button isExpectedProcessExit is ture, no need to call function again, the function will return false
                {
                    isExpectedProcessExit = TextUtil::Instance().IsExpectedProcessExit(rotatedFrame, clickPoint, currentLanguageMode, clickedKeywords);
                }

                if (isExpectedProcessExit)
                {
                    dut->StartActivity(packageName + "/" + activityName);   // restart app
                    ThreadSleep(30000);
                    isExpectedProcessExit = false;                          // reset isExpectedProcessExit to false
                    clickedKeywords.clear();
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << "No available device found for exploratory testing." << endl;
            return false;
        }
    }

    void ExploratoryEngine::RecordClickedKeyword(Point clickPoint, const cv::Mat frame)
    {
        Mat roiFrame;
        int rectHeight = 150, rectWidth = 150;
        int x = clickPoint.x - rectWidth / 2;
        int y = clickPoint.y - rectHeight / 2;

        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        
        Rect roiRect(x, y, rectWidth, rectHeight);
        ObjectUtil::Instance().CopyROIFrame(frame, roiRect, roiFrame);

        generateAllRectsAndKeywords(roiFrame, Rect(0, 0, roiFrame.size().width, roiFrame.size().height), "zh");
        for (int i = 0; i < allZhKeywords.size(); ++i)
            clickedKeywords.push_back(allZhKeywords[i]);

        generateAllRectsAndKeywords(roiFrame, Rect(0, 0, roiFrame.size().width, roiFrame.size().height), "en");
        for (int i = 0; i < allEnKeywords.size(); ++i)
            clickedKeywords.push_back(allEnKeywords[i]);
    }

    bool ExploratoryEngine::JudgeStrInWhiteList(string idInfo, std::vector<Keyword> keywordList)
    {
        boost::to_lower(idInfo);
        if (!idInfo.empty())
        {
            for (auto expectedKeyword : keywordList)
            {
                std::tr1::wsmatch wsmatch;
                std::tr1::wregex wregex(expectedKeyword.text.c_str(), std::tr1::regex_constants::icase);
                std::wstring wstrIdInfo = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(idInfo);
                if (std::tr1::regex_search(wstrIdInfo, wsmatch, wregex))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ExploratoryEngine::JudgeWstrInWhiteList(wstring wstrIdInfo, std::vector<Keyword> keywordList)
    {
        if (!wstrIdInfo.empty())
        {
            for (auto expectedKeyword : keywordList)
            {
                std::tr1::wsmatch wsmatch;
                std::tr1::wregex wregex(expectedKeyword.text.c_str(), std::tr1::regex_constants::icase);
                if (std::tr1::regex_search(wstrIdInfo, wsmatch, wregex))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool ExploratoryEngine::dispatchProbableAction(const cv::Mat &rotatedFrame, const boost::shared_ptr<CoverageCollector> &covCollector, const boost::shared_ptr<BiasObject> &biasObject, 
        const boost::shared_ptr<UiState> &uiState, int seed, cv::Mat cloneFrame, Point &clickPoint)
    {
        assert(covCollector != nullptr);

        boost::shared_ptr<UiAction> act;
        boost::shared_ptr<UiStateObject> stateObj;
        std::vector<boost::shared_ptr<UiStateObject>> clickableObjects = uiState->GetClickableObjects();
        std::vector<boost::shared_ptr<UiStateObject>> editableObjects = uiState->GetEditTextFields();

        double clickPos = biasObject->GetClickPossibility();
        double swipePos = biasObject->GetSwipePossibility();
        double textPos = biasObject->GetTextPossibility();

        sharingKeywordList = TestManager::Instance().GetValueFromKeywordMap(KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::SHAREISSUE));
        commonIDList = TestManager::Instance().GetValueFromKeywordMap(KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::COMMONIDINFO));
        std::vector<Keyword> commonIDInfoWhiteList = commonIDList["IDWhiteList"];
        std::vector<Keyword> sharingIssueIDInfoWhiteList = sharingKeywordList["IDInfoWhiteList"];
        
        std::vector<boost::shared_ptr<UiStateObject>> commonObjects;
        for (auto object : clickableObjects)
        {
            string idInfo = object->GetIDInfo();
            if (JudgeStrInWhiteList(idInfo, commonIDInfoWhiteList))
                commonObjects.push_back(object);
        }

        if (clickableObjects.empty() && clickPos > 0.0)
        {
            swipePos += clickPos;
            clickPos = 0.0;
        }
        if (editableObjects.empty() && textPos > 0.0)
        {
            swipePos += textPos;
            textPos = 0.0;
        }

        int eventChooser = this->randomGenerator.GetRandomWithRange(1, 100);
        size_t bIndex = 0;
        if (eventChooser <= ceil(clickPos * 100))
        {
            int clickEventChoose = 0;
            clickEventChoose = this->randomGenerator.GetRandomWithRange(1, 100);
            if (clickEventChoose <= ceil(biasObject->GetClickButton() * 100))
            {
                
                std::vector<boost::shared_ptr<UiStateObject>> objects;
                if (commonObjects.size() > 0 && eventChooser <= ceil(clickPos * 100 / 2))   // if common objects is not empty, 50% possibility to click common objects
                {
                    objects = commonObjects;
                }
                else
                {
                    objects= clickableObjects;
                }
                bIndex = this->randomGenerator.GetRandomWithRange(0, (int)(objects.size())-1);
                
                stateObj = objects[bIndex];
                UiRectangle clickRect = stateObj->GetRect();
                act = boost::shared_ptr<UiAction>(new UiAction(stateObj->GetText(), clickRect, rotatedFrame, dut));
                UiPoint p = clickRect.GetCenter();
                clickPoint = Point(p.GetX(), p.GetY());

                string clickIdInfo = stateObj->GetIDInfo();
                if (JudgeStrInWhiteList(clickIdInfo, sharingIssueIDInfoWhiteList))  // id info in sharing issue white list
                    this->checkSharingIssue = true;     // set check sharing issue as true

                covCollector->WriteCoverageLine(true, uiState, stateObj, "Click", "Button");
                covCollector->WriteCoverageLine(false, uiState, stateObj, "Click", "Button");
                covCollector->UpdateDictionary(uiState, stateObj);
            }
            else if (clickEventChoose <= ceil((biasObject->GetClickButton() + biasObject->GetClickHome()) * 100))
            {
                // home
                stateObj = boost::shared_ptr<UiStateObject>(new UiStateHome());
                act = boost::shared_ptr<UiAction>(new UiAction(std::string(""), UiRectangle(), cv::Mat(), dut));
                covCollector->WriteCoverageLine(true, uiState, nullptr, "Click", "Home");
                covCollector->WriteCoverageLine(false, uiState, nullptr, "Click", "Home");
                isExpectedProcessExit = true;
            }
            else if (clickEventChoose <= ceil((biasObject->GetClickButton() + biasObject->GetClickHome() + biasObject->GetClickMenu()) * 100))
            {
                // menu
                stateObj = boost::shared_ptr<UiStateObject>(new UiStateMenu());
                act = boost::shared_ptr<UiAction>(new UiAction(std::string(""), UiRectangle(), cv::Mat(), dut));

                covCollector->WriteCoverageLine(true, uiState, nullptr, "Click", "Menu");
                covCollector->WriteCoverageLine(false, uiState, nullptr, "Click", "Menu");
            }
            else
            {
                // back
                stateObj = boost::shared_ptr<UiStateObject>(new UiStateBack());
                act = boost::shared_ptr<UiAction>(new UiAction(std::string(""), UiRectangle(), cv::Mat(), dut));
                covCollector->WriteCoverageLine(true, uiState, nullptr, "Click", "Back");
                covCollector->WriteCoverageLine(false, uiState, nullptr, "Click", "Back");
                isExpectedProcessExit = true;
            }
        }
        else if (eventChooser <= ceil((clickPos + swipePos) * 100))
        {
            // swipe
            stateObj = boost::shared_ptr<UiStateObject>(new UiStateSwipe());
            std::string dirStr = "";
            int directionChooser = this->randomGenerator.GetRandomWithRange(1, 100);
            if (directionChooser <= 25)
            {
                dirStr = "Up";
                DrawPoints(cloneFrame, SwipeDirection::UP);
            }
            else if (directionChooser <= 50)
            {
                dirStr = "Down";
                DrawPoints(cloneFrame, SwipeDirection::DOWN);
            }
            else if (directionChooser <= 75)
            {
                dirStr = "Left";
                DrawPoints(cloneFrame, SwipeDirection::LEFT);
            }
            else
            {
                dirStr = "Right";
                DrawPoints(cloneFrame, SwipeDirection::RIGHT);
            }
            act = boost::shared_ptr<UiAction>(new UiAction(dirStr, UiRectangle(), cv::Mat(), dut));
            covCollector->WriteCoverageLine(true, uiState, nullptr, "Swipe", dirStr);
            covCollector->WriteCoverageLine(false, uiState, nullptr, "Swipe", dirStr);
        }
        else
        {
            // TODO: add drag actions
        }

        if (stateObj != nullptr)
        {
            RecordAndRunUIAction(stateObj, act);            
        }
        ThreadSleep(500);

        return true;
    }

    SeedRandom ExploratoryEngine::initRandomGenerator(int seed)
    {
        if (seed == -1)
        {
            return SeedRandom();
        }
        else
        {
            return SeedRandom(seed);
        }
    }

    void ExploratoryEngine::InitFocusWindowInfo(const cv::Mat &rotatedFrame)
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
            std::vector<std::string> windowLines;
            Orientation orientation = dut->GetCurrentOrientation(true);
            AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);

            string focusWindowActivity;
            AndroidTargetDevice::GetWindows(windowLines, focusWindowActivity);
            EnqueueFocusWindowActivity(focusWindowActivity);

            this->focusedWindow = windowBarInfo.focusedWindowRect;

            this->isPopUpWindow = windowBarInfo.isPopUpWindow;
            if (!this->isPopUpWindow)
                this->isPopUpWindow = ObjectUtil::Instance().IsPopUpDialog(deviceSize, orientation, windowBarInfo.statusbarRect, windowBarInfo.focusedWindowRect);

            if(windowBarInfo.inputmethodExist)      // Keyboard will make the focus window looks like a pop up
                this->isPopUpWindow = false;

            Size rotatedFrameSize = rotatedFrame.size();
            double widthRatio = 1.0;
            double heightRatio = 1.0;
            if (rotatedFrameSize.width > rotatedFrameSize.height)
            {
                widthRatio = rotatedFrameSize.width / (double)(max(deviceSize.width, deviceSize.height));
                heightRatio = rotatedFrameSize.height / (double)(min(deviceSize.width, deviceSize.height));
            }
            else
            {
                widthRatio = rotatedFrameSize.width / (double)(min(deviceSize.width, deviceSize.height));
                heightRatio = rotatedFrameSize.height / (double)(max(deviceSize.width, deviceSize.height));
            }
            Point tl = this->focusedWindow.tl();
            Point br = this->focusedWindow.br();
            Point tlOnRotatedFrame, brOnRotatedFrame;
            tlOnRotatedFrame.x = (int)(widthRatio * tl.x);
            tlOnRotatedFrame.y = (int)(heightRatio * tl.y);
            brOnRotatedFrame.x = (int)(widthRatio * br.x);
            brOnRotatedFrame.y = (int)(heightRatio * br.y);
            this->focusedWindowOnFrame = Rect(tlOnRotatedFrame.x, tlOnRotatedFrame.y, brOnRotatedFrame.x - tlOnRotatedFrame.x, brOnRotatedFrame.y - tlOnRotatedFrame.y);
        }
        return;
    }

    bool ExploratoryEngine::runExploratoryTest(const cv::Mat &rotatedFrame, const boost::shared_ptr<CoverageCollector> &covCollector, const boost::shared_ptr<BiasObject> &biasObject, int seedValue, cv::Mat &cloneFrame, Point &clickPoint)
    {
        this->randomGenerator = initRandomGenerator(seedValue);
        stopWatch->Reset();

        bool clickAppObjectToHome = false;
        if (biasObject->GetObjectMode())
        {
            // Sometimes, it will trigger other applications, like browser;
            // If the current package is not tested package, press back;
            // It is a default behavior, not controlled by bias xml

            if (seedValue != -1)
            {
                generatedQEvents.clear();
                InitFocusWindowInfo(rotatedFrame);
                clickAppObjectToHome = generateExploratoryAction(covCollector, rotatedFrame, this->focusedWindowOnFrame, biasObject, seedValue, cloneFrame, clickPoint);
            }
            else
            {
                assert(false);
            }
        }
        else
        {
            assert(false);
        }

        return clickAppObjectToHome;
    }

    void ExploratoryEngine::DoRandomeAction(cv::Mat frame, RandomActionType actionType)
    {
        SeedRandom randomGenerator = initRandomGenerator();
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut == nullptr)
        {
            return;
        }

        Size size = frame.size();
        switch (actionType)
        {
        case RandomActionType::CLICK:
            {
                int x = randomGenerator.GetRandomWithRange(ObjectUtil::Instance().navigationBarHeight + 1, size.width - ObjectUtil::Instance().navigationBarHeight - 1);
                int y = randomGenerator.GetRandomWithRange(ObjectUtil::Instance().navigationBarHeight + 1, size.height - ObjectUtil::Instance().navigationBarHeight - 1);
                RecordAndRunClickQEvent(size, Point(x, y));
            }
            break;
        case RandomActionType::SWIPE:
            {
                int swipeOrientation = randomGenerator.GetRandomWithRange(1, 4);
                if (swipeOrientation == 1)
                {                    
                    RecordAndRunDragQEvent(2048, 2048, 1024, 2048, androidDut->GetCurrentOrientation());
                }
                else if (swipeOrientation == 2)
                {
                    RecordAndRunDragQEvent(2048, 2048, 3072, 2048, androidDut->GetCurrentOrientation());
                }
                else if (swipeOrientation == 3)
                {
                    RecordAndRunDragQEvent(2048, 2048, 2048, 1024, androidDut->GetCurrentOrientation());
                }
                else
                {
                    RecordAndRunDragQEvent(2048, 2048, 2048, 3072, androidDut->GetCurrentOrientation());
                }
            }
            break;
        case RandomActionType::MOVE:
            {
                int x1 = randomGenerator.GetRandomWithRange(ObjectUtil::Instance().navigationBarHeight + 1, size.width - ObjectUtil::Instance().navigationBarHeight - 1);
                int y1 = randomGenerator.GetRandomWithRange(ObjectUtil::Instance().navigationBarHeight + 1, size.height - ObjectUtil::Instance().navigationBarHeight - 1);
                int moveOffset = 50;
                RecordAndRunTouchDownQEvent(frame.size(), x1 - moveOffset, y1 - moveOffset);
                RecordAndRunTouchMoveQEvent(frame.size(), x1, y1);
                RecordAndRunTouchMoveQEvent(frame.size(), x1 + moveOffset, y1 + 50 + moveOffset);
                RecordAndRunTouchUpQEvent(frame.size(), x1 + moveOffset, y1 + 50 + moveOffset);
            }
            break;
        case RandomActionType::HOME:
            {
                RecordAndRunButtonQEvent("HOME");
            }
            break;
        case RandomActionType::BACK:
            {
                RecordAndRunButtonQEvent("BACK");                
            }
            break;
        case RandomActionType::MENU:
            {
                RecordAndRunButtonQEvent("MENU");
            }
            break;
        case RandomActionType::POWER:
            {
                RecordAndRunButtonQEvent("POWER");
            }
            break;
        case RandomActionType::WAKE_UP:
            {
                RecordAndRunButtonQEvent("WAKE_UP");
            }
            break;
        case RandomActionType::VOLUMN_DOWN:
            {
                RecordAndRunButtonQEvent("VOLUMN_DOWN");
            }
            break;
        case RandomActionType::VOLUMN_UP:
            {
                RecordAndRunButtonQEvent("VOLUMN_UP");
            }
            break;
        default:
            break;
        }
    }

    RandomActionType ExploratoryEngine::IntToRandomActionType(int type)
    {
        switch(type)
        {
        case RandomActionType::CLICK:
            return RandomActionType::CLICK;
        case RandomActionType::SWIPE:
            return RandomActionType::SWIPE;
        case RandomActionType::MOVE:
            return RandomActionType::MOVE;
        case RandomActionType::HOME:
            return RandomActionType::HOME;
        case RandomActionType::BACK:
            return RandomActionType::BACK;
        case RandomActionType::MENU:
            return RandomActionType::MENU;
        case RandomActionType::POWER:
            return RandomActionType::POWER; 
        case RandomActionType::WAKE_UP:
            return RandomActionType::WAKE_UP; 
        case RandomActionType::VOLUMN_DOWN:
            return RandomActionType::VOLUMN_DOWN; 
        case RandomActionType::VOLUMN_UP:
            return RandomActionType::VOLUMN_UP; 
        default:
            return RandomActionType::CLICK;
        }
    }

    RandomActionType ExploratoryEngine::StringToRandomActionType(string type)
    {
        if(type == "CLICK")
            return RandomActionType::CLICK;
        else if(type == "SWIPE")
            return RandomActionType::SWIPE;
        else if(type == "MOVE")
            return RandomActionType::MOVE;
        else if(type == "HOME")
            return RandomActionType::HOME;
        else if(type == "BACK")
            return RandomActionType::BACK;
        else if(type == "MENU")
            return RandomActionType::MENU;
        else if(type == "POWER")
            return RandomActionType::POWER; 
        else if(type == "WAKE_UP")
            return RandomActionType::WAKE_UP; 
        else if(type == "VOLUMN_DOWN")
            return RandomActionType::VOLUMN_DOWN; 
        else if(type == "VOLUMN_UP")
            return RandomActionType::VOLUMN_UP; 
        else
            return RandomActionType::CLICK;
    }

    void ExploratoryEngine::DispatchRandomAction(cv::Mat frame, string actionType, int actionProb)
    {
        RandomActionType randomActionType = StringToRandomActionType(actionType);
        if (actionProb == 100)
        {
            DoRandomeAction(frame, randomActionType);
        }
        else
        {
            SeedRandom randomGenerator = initRandomGenerator();
            int randomNum = randomGenerator.GetRandomWithRange(1, 100);
            if (randomNum <= actionProb)
            {
                DoRandomeAction(frame, randomActionType);
            }
            else
            {
                int randomType = randomGenerator.GetRandomWithRange(0, RandomActionType::TOTAL - 1);
                DoRandomeAction(frame, IntToRandomActionType(randomType));
            }
        }
    }

    void ExploratoryEngine::DrawPoints(cv::Mat &frame, SwipeDirection d, const Scalar scalar, int thickness)
    {
        // draw points according to the swipe 
        vector<Point> drawPoints;
        int totalIndex = 21;
        int startIndex = 0;
        int endIndex = 0;

        int widthInterval = (int)(frame.size().width / totalIndex);
        int pointHeight = (int)(frame.size().height / 2);

        int heightInterval = (int)(frame.size().height / totalIndex);
        int pointWidth = (int)(frame.size().width / 2);

        switch (d)
        {
        case SwipeDirection::UP:
            {
                startIndex = 4;
                endIndex = 11;
            }
            break;
        case SwipeDirection::DOWN:
            {
                startIndex = 11;
                endIndex = 18;
            }
            break;
        case SwipeDirection::LEFT:
            {
                startIndex = 4;
                endIndex = 11;
            }
            break;
        case SwipeDirection::RIGHT:
            {
                startIndex = 11;
                endIndex = 18;
            }
            break;
        default:
            break;
        }

        if ((d == SwipeDirection::LEFT) || (d == SwipeDirection::RIGHT))
        {
            for (int i = startIndex; i <= endIndex; i++)
            {
                drawPoints.push_back(Point(i * widthInterval, pointHeight));
            }
            DrawCircles(frame, drawPoints, scalar, thickness);
        }

        if ((d == SwipeDirection::UP) || (d == SwipeDirection::DOWN))
        {
            for (int i = startIndex; i <= endIndex; i++)
            {
                drawPoints.push_back(Point(pointWidth, i * heightInterval));
            }
            DrawCircles(frame, drawPoints, scalar, thickness);
        }
    }

    void ExploratoryEngine::RecordAndRunClickOnRotatedQEvent(const Size &rotatedFrameSize, const Point &rotatedPoint)
    {   

        ClickOnRotatedFrame(rotatedFrameSize, rotatedPoint);
        DAVINCI_LOG_INFO << "Click onrotated @(" << boost::lexical_cast<string>(rotatedPoint.x) << "," << boost::lexical_cast<string>(rotatedPoint.y) << ")" << endl;
                
        
        
        Orientation orientation = dut->GetCurrentOrientation();
        Size frameSize;
        Point newPoint = TransformRotatedFramePointToFramePoint(rotatedFrameSize, rotatedPoint, dut->GetCurrentOrientation(false), frameSize);

        Point normPoint = FrameToNormCoord(frameSize, newPoint, orientation);        
        string info = "";
        //skip unit test
        if(generatedQScript)
        {
            int x = rotatedPoint.x;
            int y = rotatedPoint.y;
            if(imageBoxInfo.width == 0 || imageBoxInfo.height == 0)
            {
                imageBoxInfo.orientation = DeviceOrientation::OrientationPortrait;    
                imageBoxInfo.width = frameSize.width;
                imageBoxInfo.height = frameSize.height;
            }            
            
            TestManager::Instance().SetImageBoxInfo(&imageBoxInfo);            
            ScriptRecorder::RecordAdditionalInfo(x, y, info, generatedQScript->GetQsFileName(), packageName, dut, treeId);          
        }
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_TOUCHDOWN, normPoint.x, normPoint.y, -1, 0, info, orientation)));

        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_TOUCHUP, normPoint.x, normPoint.y, -1, 0, info, orientation)));
        


        
    }

    void ExploratoryEngine::RecordAndRunClickQEvent(const Size &frameSize, const Point &point)
    {
        
        Orientation orientation = dut->GetCurrentOrientation();
        ClickOnFrame(dut, frameSize, point.x, point.y, orientation);
        
        Point newPoint = FrameToNormCoord(frameSize, Point(point.x, point.y), orientation);
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_TOUCHDOWN, newPoint.x, newPoint.y, -1, 0, "", orientation)));

        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_TOUCHUP, newPoint.x, newPoint.y, -1, 0, "", orientation)));
    }

    void ExploratoryEngine::RecordAndRunSetTextQEvent(const string &text)
    {   
        DAVINCI_LOG_INFO << "Set text @(" << text << ")" << endl;
        dut->TypeString(text);
        string textSet = generatedQScript->ProcessQuoteSpaces(text);
        
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_SET_TEXT, textSet)));
    }

    void ExploratoryEngine::RecordAndRunDragQEvent(int x1, int y1, int x2, int y2, Orientation orientation, bool tapmode)
    {   
        DAVINCI_LOG_INFO << "Drag onrotated @(" << boost::lexical_cast<string>(x1) << "," << boost::lexical_cast<string>(y1) << "," << boost::lexical_cast<string>(x2) << "," << boost::lexical_cast<string>(y2)<< ")" << endl;
        dut->Drag(x1,y1,x2,y2,orientation,tapmode);
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_DRAG, x1,y1,x2,y2,"", orientation))); 
    }


    void ExploratoryEngine::RecordAndRunButtonQEvent(const string &button)
    {   
        DAVINCI_LOG_INFO << "Press Button @(" << button << ")" << endl;
        
        if (boost::algorithm::to_upper_copy(button) == "ENTER")
        {
            dut->PressEnter();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_BUTTON, button)));
        }
        else if (boost::algorithm::to_upper_copy(button) == "SEARCH")
        {
            dut->PressSearch();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_BUTTON, button)));
        }
        else if (boost::algorithm::to_upper_copy(button) == "CLEAR")
        {
            dut->ClearInput();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_BUTTON, button)));
        }   
        else if(boost::algorithm::to_upper_copy(button) == "POWER")
        {
            
            dut->PressPower();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_POWER)));
        }
        else if(boost::algorithm::to_upper_copy(button) == "WAKE_UP")
        {
            dut->WakeUp();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_LIGHT_UP)));
        }
        else if(boost::algorithm::to_upper_copy(button) == "VOLUMN_DOWN")
        {
            dut->VolumeDown();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_VOLUME_DOWN, button)));
        }
        else if(boost::algorithm::to_upper_copy(button) == "VOLUMN_UP")
        {
            dut->VolumeUp();
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_VOLUME_UP, button)));
        }       
   
        
    }

    void ExploratoryEngine::RecordAndRunBackQEvent()
    {   
        DAVINCI_LOG_INFO << "Press Back @(" << ")" << endl;
        dut->PressBack();
       
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_BACK)));
        
    }

    void ExploratoryEngine::RecordAndRunUIAction(boost::shared_ptr<UiStateObject> stateObj, boost::shared_ptr<UiAction> act)
    {

        if(boost::dynamic_pointer_cast<UiStateBack>(stateObj)) 
        {
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_BACK)));
        }
        else if (boost::dynamic_pointer_cast<UiStateHome>(stateObj))
        {
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_HOME)));
        }
        else if (boost::dynamic_pointer_cast<UiStateMenu >(stateObj))
        {
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_MENU)));
        }
        else if(boost::dynamic_pointer_cast<UiStateSwipe>(stateObj))
        {
            string dir = act->GetInputString();            
            // copy from UiAction
            if (dir == "Left")
            {
                generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_DRAG, 2048,2048,1024,2048,"", dut->GetCurrentOrientation())));                 
            }
            else if (dir== "Right")
            {
                generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_DRAG, 2048,2048,3072,2048,"", dut->GetCurrentOrientation())));           
                
            }
            else if (dir == "Up")
            {
                generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_DRAG, 2048,2048,2048,1024,"", dut->GetCurrentOrientation()))); 
                
            }
            else
            {
                generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                    QSEventAndAction::OPCODE_DRAG, 2048,2048,2048,3072,"", dut->GetCurrentOrientation())));                
            }
        }
        else if(boost::dynamic_pointer_cast<UiStateClickable>(stateObj) || boost::dynamic_pointer_cast<UiStateGeneral >(stateObj))
        {
            UiRectangle rect = act->GetClickPosition();
            UiPoint p = rect.GetCenter();
            Orientation orientation = dut->GetCurrentOrientation();
            Size frameSize;            
            Point newPoint = TransformRotatedFramePointToFramePoint(act->Getframe().size(), Point(p.GetX(), p.GetY()), dut->GetCurrentOrientation(false), frameSize);            
            Point normPoint = FrameToNormCoord(frameSize, newPoint, orientation);
            string info = "";
            int x = p.GetX();
            int y = p.GetY();
            if(imageBoxInfo.width == 0 || imageBoxInfo.height == 0)
            {
                imageBoxInfo.orientation = DeviceOrientation::OrientationPortrait;    
                imageBoxInfo.width = act->Getframe().cols;
                imageBoxInfo.height = act->Getframe().rows;
            }            
            
            TestManager::Instance().SetImageBoxInfo(&imageBoxInfo);            
            ScriptRecorder::RecordAdditionalInfo(x, y, info, generatedQScript->GetQsFileName(), packageName, dut, treeId);          
            
            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHDOWN, normPoint.x, normPoint.y, -1, 0, info, orientation)));

            generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHUP, normPoint.x, normPoint.y, -1, 0, info, orientation)));
        }
        stateObj->takeAction(act);
    }

    void ExploratoryEngine::RecordAndRunTouchDownQEvent(const Size &frameSize, int x, int y)
    {
        Orientation orientation = dut->GetCurrentOrientation();

        TouchDownOnFrame(dut, frameSize, x,y , orientation);
            

        
        string info = "";         

        //if(TestManager::Instance().GetRecordWithID())
        //    ScriptRecorder::RecordAdditionalInfo(x, y, info, generatedQScript->GetQsFileName(), packageName, dut, treeId);
        Point newPoint = FrameToNormCoord(frameSize, Point(x, y), orientation);    
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHDOWN, newPoint.x, newPoint.y, -1, 0, info, orientation)));

    }
    void ExploratoryEngine::RecordAndRunTouchUpQEvent(const Size &frameSize, int x, int y)
    {
        Orientation orientation = dut->GetCurrentOrientation();
        TouchMoveOnFrame(dut, frameSize, x,y , orientation);
        Point newPoint = FrameToNormCoord(frameSize, Point(x, y), orientation);
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHUP, newPoint.x, newPoint.y, -1, 0, "", orientation)));
    
    }
    void ExploratoryEngine::RecordAndRunTouchMoveQEvent(const Size &frameSize, int x, int y)
    {
        Orientation orientation = dut->GetCurrentOrientation();

        TouchUpOnFrame(dut, frameSize, x,y , orientation);
        Point newPoint = FrameToNormCoord(frameSize, Point(x, y), orientation);
        generatedQEvents.push_back(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(stopWatch->ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHMOVE, newPoint.x, newPoint.y, -1, 0, "", orientation)));
    }
    

    //void ExploratoryEngine::RecordAdditionalInfo(int& x, int& y, string& info)
    //{
    //    info = "";

    //    if(dut == nullptr)
    //        return;

    //    //double start = stopWatch->ElapsedMilliseconds();
    //    Orientation deviceCurrentOrientation = dut->GetCurrentOrientation();
    //    Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
    //    
    //    boost::filesystem::path qsPath = boost::filesystem::path(generatedQScript->GetQsFileName());
    //    boost::filesystem::path qsFolder = qsPath.parent_path();
    //    TestManager::Instance().TransformImageBoxToDeviceCoordinate(deviceSize, deviceCurrentOrientation, x, y);
    //    vector<string> lines = dut->GetTopActivityViewHierarchy();
    //    string treeFileName = "record_" + generatedQScript->GetPackageName()+ ".qs_" + boost::lexical_cast<string>(++treeId) + ".tree";

    //    string treeFilePath = ConcatPath(qsFolder.string(), treeFileName);
    //    std::ofstream treeStream(treeFilePath, std::ios_base::app | std::ios_base::out);
    //    for(auto line: lines)
    //        treeStream << line << "\r\n" << std::flush;
    //    treeStream.flush();
    //    treeStream.close();

    //    viewParser->ParseTree(lines);
    //    Rect rect;
    //    int groupIndex, itemIndex;

    //    boost::shared_ptr<AndroidTargetDevice> adut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
    //    AndroidTargetDevice::WindowBarInfo windowBarInfo;
    //    vector<string> windowLines;
    //    if(adut != nullptr)
    //    {
    //        windowBarInfo = adut->GetWindowBarInfo(windowLines);
    //        string windowFileName = "record_" + generatedQScript->GetPackageName() + ".qs_" + boost::lexical_cast<string>(treeId) + ".window";
    //        string windowFilePath = ConcatPath(qsFolder.string(), windowFileName);
    //        std::ofstream windowStream(windowFilePath, std::ios_base::app | std::ios_base::out);
    //        for(auto line: windowLines)
    //            windowStream << line << "\r\n" << std::flush;
    //        windowStream.flush();
    //        windowStream.close();
    //    }

    //    string id = viewParser->FindMatchedIdByPoint(Point(x, y), windowBarInfo.focusedWindowRect, rect, groupIndex, itemIndex);
    //    viewParser->CleanTreeControls();

    //    if(id != "")
    //    {
    //        info += "id=" + id;
    //        info += "(" + boost::lexical_cast<string>(groupIndex) + "-" + boost::lexical_cast<string>(itemIndex) + ")&";
    //        double ratioX = double(x - rect.x) / rect.width;
    //        double ratioY = double(y - rect.y) / rect.height;
    //        std::ostringstream ss;
    //        ss << std::fixed << std::setprecision(4);
    //        ss << ratioX;
    //        ss << ",";
    //        ss << ratioY;
    //        std::string ratioStr = ss.str();
    //        info += "ratio=(" + ratioStr + ")&";
    //        info += "layout=" + treeFileName + "&";

    //        std::ostringstream barss;
    //        barss << "sb=(";
    //        barss << windowBarInfo.statusbarRect.x;
    //        barss << ",";
    //        barss << windowBarInfo.statusbarRect.y;
    //        barss << ",";
    //        barss << (windowBarInfo.statusbarRect.x + windowBarInfo.statusbarRect.width);
    //        barss << ",";
    //        barss << (windowBarInfo.statusbarRect.y + windowBarInfo.statusbarRect.height);
    //        barss << ")&nb=(";
    //        barss << windowBarInfo.navigationbarRect.x;
    //        barss << ",";
    //        barss << windowBarInfo.navigationbarRect.y;
    //        barss << ",";
    //        barss << (windowBarInfo.navigationbarRect.x + windowBarInfo.navigationbarRect.width);
    //        barss << ",";
    //        barss << (windowBarInfo.navigationbarRect.y + windowBarInfo.navigationbarRect.height);
    //        barss << ")&kb=";
    //        if(windowBarInfo.inputmethodExist)
    //            barss << "1&fw=(";
    //        else
    //            barss << "0&fw=(";
    //        barss << windowBarInfo.focusedWindowRect.x;
    //        barss << ",";
    //        barss << windowBarInfo.focusedWindowRect.y;
    //        barss << ",";
    //        barss << (windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width);
    //        barss << ",";
    //        barss << (windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height);
    //        if(windowBarInfo.isPopUpWindow)
    //            barss << ")&popup=1";
    //        else
    //            barss << ")&popup=0";

    //        std::string windowStr = barss.str();
    //        info += windowStr;
    //    }
    //    else
    //    {
    //        info += "layout=" + treeFileName + "&";
    //        std::ostringstream ss;
    //        ss << "sb=(";
    //        ss << windowBarInfo.statusbarRect.x;
    //        ss << ",";
    //        ss << windowBarInfo.statusbarRect.y;
    //        ss << ",";
    //        ss << (windowBarInfo.statusbarRect.x + windowBarInfo.statusbarRect.width);
    //        ss << ",";
    //        ss << (windowBarInfo.statusbarRect.y + windowBarInfo.statusbarRect.height);
    //        ss << ")&nb=(";
    //        ss << windowBarInfo.navigationbarRect.x;
    //        ss << ",";
    //        ss << windowBarInfo.navigationbarRect.y;
    //        ss << ",";
    //        ss << (windowBarInfo.navigationbarRect.x + windowBarInfo.navigationbarRect.width);
    //        ss << ",";
    //        ss << (windowBarInfo.navigationbarRect.y + windowBarInfo.navigationbarRect.height);
    //        ss << ")&kb=";
    //        if(windowBarInfo.inputmethodExist)
    //            ss << "1&fw=(";
    //        else
    //            ss << "0&fw=(";
    //        ss << windowBarInfo.focusedWindowRect.x;
    //        ss << ",";
    //        ss << windowBarInfo.focusedWindowRect.y;
    //        ss << ",";
    //        ss << (windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width);
    //        ss << ",";
    //        ss << (windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height);
    //        if(windowBarInfo.isPopUpWindow)
    //            ss << ")&popup=1";
    //        else
    //            ss << ")&popup=0";

    //        std::string windowStr = ss.str();
    //        info += windowStr;
    //    }

    //    //double end = stopWatch->ElapsedMilliseconds();
    //    //delay = int(end - start);
    //}
}

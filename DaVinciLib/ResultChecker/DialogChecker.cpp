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

#include "DialogChecker.hpp"
#include "TextUtil.hpp"

namespace DaVinci
{
    DialogChecker::DialogChecker(const boost::weak_ptr<ScriptReplayer> obj):Checker(), okButton(0,0)
    {
        SetCheckerName("DialogChecker");

        curframeResult = DialogCheckResult::moreCheckRequired;
        SetDialogType(DialogCheckResult::noIssueFound);

        boost::shared_ptr<ScriptReplayer> currentScriptTmp = obj.lock();

        if (currentScriptTmp == nullptr)
        {
             DAVINCI_LOG_ERROR << "Failed to Init DialogChecker()!";
             throw 1;
        }

        outputDir = TestReport::currentQsLogPath;
        currentQScript = currentScriptTmp;

        closeStatus = DialogCloseStatus::notFound;
        displayLanguage = Language::ENGLISH;
        osVersion = 4.4;
        manufactory = "";
        SetPeriod(DialogCheckerPeriod); // 12S

        IsCurFrameFromScreenCap = false;
        if (DeviceManager::Instance().UsingHyperSoftCam())
            IsHyperSoftCamMode = true;
        else
            IsHyperSoftCamMode = false;

        currentDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (currentDevice != nullptr)
        {
            String tmpLang = currentDevice->GetLanguage();
            if (tmpLang.find("zh") != string::npos)
                displayLanguage =  Language::SIMPLIFIED_CHINESE;
            else
                displayLanguage =  Language::ENGLISH;

            String tmpOsver = currentDevice->GetOsVersion();
            TryParse(tmpOsver, osVersion);
            manufactory = currentDevice->GetSystemProp("ro.product.manufacturer");

            DAVINCI_LOG_INFO << "The device's Manufactory: " + manufactory + ", Language: " + tmpLang + ", OSVerison: " + tmpOsver ;
        }
        else
            DAVINCI_LOG_ERROR << "Failed to GetCurrentTargetDevice for DialogChecker()!";

        InitBlackList();
        InitWhiteList();
    }

    DialogChecker::~DialogChecker()
    {
    }

    /// <summary>
    /// Worker thread loop
    /// </summary>
    void DialogChecker::WorkerThreadLoop()
    {
        cv::Mat frame;
        cv::Mat dialogBoxImage;
        std::string dialogBoxText = "";
#ifdef _DEBUG
        double startTime = 0, startCheckTime = 0, captureImgTime = 0, quickBoxDetectTime = 0, accuBoxDetectTime = 0, ocrDetectTime = 0, finalTime = 0;
#endif

        // Thread::CurrentThread->IsBackground = true;
        while (GetRunningFlag())
        {
            triggerEvent->WaitOne(checkInterval); 

            if (GetPauseFlag() || (!GetRunningFlag())) // Check current running status per each key point.
               continue;

            IsCurFrameFromScreenCap = false;

#ifdef _DEBUG
            startTime = GetCurrentMillisecond();
#endif

            // For fix bug984, when turn off crash detection, close it immediately.
            if (frameQueue.empty() || (DeviceManager::Instance().UsingDisabledCam()))
            {
                cv::Mat tmpFrame = GetClearScreenImage();
                EnqueueFrame(tmpFrame);
            }

#ifdef _DEBUG
            captureImgTime = GetCurrentMillisecond();
#endif

            // Task1: Check if there crash dialog in frame.
            while (frameQueue.size() > 0)
            {
#ifdef _DEBUG
                startCheckTime = GetCurrentMillisecond();
#endif
                curframeResult = DialogCheckResult::moreCheckRequired;

                frame = DequeueFrame();

                // Quick detect box
                if (QuickDetectBox(frame))
                    curframeResult = DialogCheckResult::moreCheckRequired;
                else
                    curframeResult = DialogCheckResult::noIssueFound;

#ifdef _DEBUG
                quickBoxDetectTime = GetCurrentMillisecond();
#endif

                // OCR, Check if included in a blacklist
                if (curframeResult == DialogCheckResult::moreCheckRequired)
                {
                    if ((!IsCurFrameFromScreenCap) && (!IsHyperSoftCamMode))
                    {
                        frame = GetClearScreenImage();

                        if (frame.empty())
                            continue;
                    }

                    if (GetDialogBoxImage(frame, dialogBoxImage))
                    {
#ifdef _DEBUG
                        accuBoxDetectTime = GetCurrentMillisecond();
#endif
                        dialogBoxText = GetDialogBoxText(dialogBoxImage);

                        string saveImageName;
                        saveImageName = GetOutputDir() + std::string("/") + std::string("DialogBoxImage") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string(".png");
                        imwrite(saveImageName, dialogBoxImage);

                        // Check if the string include blacklist words
                        if (IsInCrashBlackList(dialogBoxText))
                        {
                            curframeResult = DialogCheckResult::crashDialog;
                            SetDialogType(DialogCheckResult::crashDialog);
                            SetResult(CheckerResult::Fail);
                        } 
                        else if(IsInANRBlackList(dialogBoxText))
                        {
                            curframeResult = DialogCheckResult::anrDialog;
                            SetDialogType(DialogCheckResult::anrDialog);
                            SetResult(CheckerResult::Fail);
                        } 
                        else
                        {
                            curframeResult = DialogCheckResult::noIssueFound;
                        }
#ifdef _DEBUG
                        ocrDetectTime = GetCurrentMillisecond();
#endif
                    }
                    else
                    {
                        curframeResult = DialogCheckResult::noIssueFound;
                    }
                }

                if (GetPauseFlag() || (!GetRunningFlag())) // Check current running status per each key point.
                    continue;

                if ((curframeResult == DialogCheckResult::crashDialog) || (curframeResult == DialogCheckResult::anrDialog))
                {
                    {
                        boost::lock_guard<boost::mutex> lock(lockCrashFrame);
                        lastCrashFrame = frame.clone();
                    }

                    // When OK button line is gray, sometimes the OK button's pos incorrect.
                    if (okButton.y < frame.rows / 2)
                        okButton.y = frame.rows - okButton.y;

                    // make sure crash dialog has been closed.
                    if ((okButton.x != 0) || (okButton.y != 0))
                    {
                        CloseDialog(frame, okButton.x, okButton.y, currentDevice->GetCurrentOrientation(true));
                    }

                    //Trigger double confirm if crash dialog closed, only do once.
                    if (closeStatus == DialogCloseStatus::notFound)
                        TriggerCheck();

                    closeStatus = DialogCloseStatus::closed;

                    string saveImageName;
                    if (curframeResult == DialogCheckResult::anrDialog)
                    {
                        saveImageName = GetOutputDir() + std::string("/") + std::string("ANRImage_") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string(".png");
                        DAVINCI_LOG_INFO << (std::string("ANRDialog Detected: ") + saveImageName);
                    }
                    else
                    {
                        saveImageName = GetOutputDir() + std::string("/") + std::string("CrashImage_") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string(".png");
                        DAVINCI_LOG_INFO << (std::string("CrashDialog Detected: ") + saveImageName);
                    }

                    imwrite(saveImageName, frame);
                    AddToFailedImageList(saveImageName);

#ifdef _DEBUG
                    finalTime = GetCurrentMillisecond();
                    DAVINCI_LOG_DEBUG << ("------------------- CrashDialog Detection Performance -----------------");
                    DAVINCI_LOG_DEBUG <<     std::string(" Total      :") 
                        << (finalTime - startCheckTime) << "ms";
                    assert (captureImgTime > startTime);
                    DAVINCI_LOG_DEBUG << std::string(" ScreenCap  :") << (captureImgTime - startTime) << std::string(" ms");
                    assert  (quickBoxDetectTime > startCheckTime);
                    DAVINCI_LOG_DEBUG << std::string(" QuickBox   :") << (quickBoxDetectTime - captureImgTime) << std::string(" ms");
                    assert  (accuBoxDetectTime > quickBoxDetectTime);
                    DAVINCI_LOG_DEBUG << std::string(" AccurateBox:") << (accuBoxDetectTime - quickBoxDetectTime) << std::string(" ms");
                    assert (ocrDetectTime > accuBoxDetectTime);
                    DAVINCI_LOG_DEBUG << std::string(" OCR        :") << (ocrDetectTime - accuBoxDetectTime) << std::string(" ms");
#endif
                }
                else
                {
                    if (closeStatus != DialogCloseStatus::notFound)
                    {
                        closeStatus = DialogCloseStatus::confirmedClosed;

                        boost::shared_ptr<ScriptReplayer> currentScriptTmp = currentQScript.lock();
                        if (!ScriptReplayer::IsSmokeQS(currentScriptTmp->GetQSMode()))
                            currentScriptTmp->SetFinished();
                    }
                }
            }
        }
    }

    bool DialogChecker::ScanFrame(const cv::Mat &frame, int opcode)
    {
        if ((!GetRunningFlag()) || (GetPauseFlag()) || (SkipCheck(opcode)))
            return false;

        Mat rotatedFrame;
        rotatedFrame = RotateFrameUp(frame, currentDevice->GetCurrentOrientation(true)); 

#ifdef _DEBUG
        //string saveImageName;
        //saveImageName = ConcatPath(GetOutputDir(), boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("OriginalImage") + std::string(".png"));
        //SaveImage(frame, saveImageName);

        //saveImageName = ConcatPath(GetOutputDir(), boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("RotatedImage") + std::string(".png"));
        //SaveImage(rotatedFrame, saveImageName);
#endif

        EnqueueFrame(rotatedFrame);
        TriggerCheck();

        return true;
    }

    void DialogChecker::EnqueueFrame(const cv::Mat &frame)
    {
        if (!frame.empty()) // Handle some null frames. null???
        {
            boost::lock_guard<boost::mutex> lock(lockCrashFrame);
            if (frameQueue.size() > maxFrameNumber)
                frameQueue.pop();

            frameQueue.push(frame);
        }
    }

    cv::Mat DialogChecker::DequeueFrame()
    {
        cv::Mat frame;

        boost::lock_guard<boost::mutex> lock(lockCrashFrame);
        if (frameQueue.size() > 0)
        {
            frame = frameQueue.front();
            frameQueue.pop();
        }

        return frame;
    }

    Point DialogChecker::GetROIOKButtonPos(const cv::Mat &crashImg)
    {
        Point center(0, 0);
        int c1 = 0, c2 = 0, horizonUp = 0, horizonDown = 0;
        const double thresh = 100;
        const unsigned char maxGray = 255;
        bool flag = false;

        cv::Mat grayImg; 
        cvtColor(crashImg, grayImg, CV_BGR2GRAY);

        threshold(grayImg, grayImg, thresh, maxGray, CV_THRESH_BINARY);

        //* Skip this step for Android L support.   
        if (osVersion < 5.0)
        {
            for (int y = 0; y < grayImg.rows; ++y)
            {
                for (int x = 0; x < grayImg.cols; ++x)
                {
                    if (grayImg.at<unsigned char>(y, x) == maxGray)
                        c1++;
                    else
                        c2++;
                }
            }

            //            if (c1 > c2)
            //                cvNot(&grayImg, &grayImg);
        }

        for (int y = grayImg.rows - 1; y >= 0; --y)
        {
            int count = 0;
            for (int x = 0; x < grayImg.cols; ++x)
            {
                if (grayImg.at<unsigned char>(y, x) == maxGray)
                    count++;
            }
            if (flag == false && count > 0)
            {
                horizonDown = y;
                flag = true;
            }
            else if (flag == true && count == 0)
            {
                horizonUp = y;
                if ((osVersion < 5.0) || ((osVersion >= 5.0) && (abs(horizonUp - horizonDown) > 20)))
                    break;
            }
        }
        if (osVersion < 5.0)
        {
            center.y = (horizonUp + horizonDown) / 2;
            center.x = grayImg.cols * 7 / 8;
        }
        else
        {
            center.y = (horizonUp + horizonDown) * 3 / 4;
            center.x = grayImg.cols * 15 / 16;
        }

        // For close Xiaomi UI's crash dialog.
        if (!manufactory.empty() && (manufactory.find("Xiaomi") != string::npos))
        {
            if (((center.y < (grayImg.rows * 3 / 4)) && ((grayImg.rows * 3 / 4) - center.y) < 20) || ((abs(grayImg.rows - center.y)) > 100))
            {
                if (grayImg.rows > 200)
                    center.y = grayImg.rows - 40;
            }
        }

        return center;
    }

    double DialogChecker::GetHistFeature(const cv::Mat &image)
    {
        double accuracy = 0.0;
        Mat hsvImage;

        if ((image.size().width < 5) || (image.size().height < 5)) // If two lines too close, just return maximum value.
        {
            accuracy = 1.0;
            return accuracy;
        }

        cvtColor(image, hsvImage, CV_BGR2HSV);
        vector<Mat> hsvChannels;
        split(hsvImage, hsvChannels);

        const int histSize = 256;
        float range[] = {0, histSize};
        const float* histRange = {range};

        Mat hHist;
        calcHist(&hsvChannels[0], 1, 0, Mat(), hHist, 1, &histSize, &histRange, true, false);

        accuracy = hHist.at<float>(0) / (image.rows * image.cols);

        //For none-screencap image: check the value=30's ratio, only need match 90% * areaOverlayRate;
        double  accuracy1 = hHist.at<float>(30) / (image.rows * image.cols);
        accuracy1 = accuracy1 / 0.9; 

        if (accuracy < accuracy1)
            accuracy = accuracy1;

        return accuracy;
    }

    bool DialogChecker::QuickDetectBox(const cv::Mat &srcImg)
    {
        return QuickDetectBox(srcImg, Point(0, srcImg.rows / 4), srcImg.cols, srcImg.rows / 2);
    }

    bool DialogChecker::QuickDetectBox(const cv::Mat &srcImg, Point p, int width, int height)
    {
        const int maxGray = 255;
        const double overlayRate = 0.6;
        double thresh = 170;
        int iterationCount = 10;
        const int elementRows = 1;
        const int elementCols = 5;
        cv::Mat tempImage;

        // Camera mode captured images.
        if ((!IsCurFrameFromScreenCap) && (!IsHyperSoftCamMode))
            thresh = 120;

        if (osVersion >= 5.0)
        {
            iterationCount = 5;

            if (!IsHyperSoftCamMode)
                thresh = 80;
        }

        cvtColor(srcImg, tempImage, CV_BGR2GRAY);

        cv::Rect rectROI(p.x , p.y, width, height);

        Mat grayImg(tempImage, rectROI);
        Mat element = getStructuringElement(MORPH_RECT, Size(elementCols, elementRows), Point(elementCols/2, elementRows/2));
        dilate(grayImg, grayImg, element, Point(-1,-1), iterationCount);
        threshold(grayImg, grayImg, thresh, maxGray, CV_THRESH_BINARY);
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(grayImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        for (unsigned int i=0; i<contours.size(); i++)
        {
            double area = contourArea(contours[i], false);
            Rect r = boundingRect(contours[i]);

            if ((osVersion >= 5.0) || (!manufactory.empty() && (manufactory.find("Xiaomi") != string::npos)))
            {
                if ((r.x < rectROI.width / 2)
                    && ((r.x + r.width) > rectROI.width / 2)
                    && (area / (r.width * r.height) > overlayRate))

                {
                    DAVINCI_LOG_DEBUG << ("Detect a box by quick BoxDetector!");
                    return true;
                }
            }
            else
            {
                if ((r.width > 2 * r.height)
                    && (r.x < rectROI.width / 2)
                    && ((r.x + r.width) > rectROI.width / 2)
                    && (abs((double)r.y - rectROI.height / 2) < rectROI.height / 3)
                    && (area / (r.width * r.height) > overlayRate))
                {
                    DAVINCI_LOG_DEBUG << ("Detect a box by quick BoxDetector!");
                    return true;
                }
            }
        }

        return false;
    }

    bool DialogChecker::GetDialogBoxImage(const cv::Mat &srcImg, cv::Mat &crashImg)
    {
        return GetDialogBoxImage(srcImg, Point(0, srcImg.rows / 4), srcImg.cols, srcImg.rows / 2, crashImg);
    }

    bool DialogChecker::GetDialogBoxImage(const cv::Mat &srcImg, Point p, int width, int height, cv::Mat &crashImg)
    {
        const double maxNum = 1000;
        const double lineOverlayRate = 0.3;
        double areaOverlayRate = 0.93;
        int thresh = 5;
        int minLineLen = 100;

        int reference = 0;
        int startreference = 0;
        int endreference = 0;
        int start = 0;
        int end = 0;
        double accuracy = 1.0;

        int index = 0;
        double minNum = maxNum;
        bool existCrashDialog = false;
        cv::Mat tempImg;

        // Cameraless mode captured images.
        if ((!IsCurFrameFromScreenCap) && (IsHyperSoftCamMode))
        {
            thresh = 3;
            areaOverlayRate = 0.80;
        }

        //For Chinese characters support.
        if (displayLanguage != Language::ENGLISH)
        {
            areaOverlayRate = 0.80;
            minLineLen = 200;
        }

        cvtColor(srcImg, tempImg, CV_BGR2GRAY);
#ifdef _DEBUG
        string saveImageName;

        saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("GetDialogBoxImage-gray") + std::string(".png");
        imwrite(saveImageName, tempImg);
#endif

        cv::Rect rect(p.x, p.y, width, height);
        cv::Mat grayImg (tempImg, rect);

        cv::Mat edgeGray = Mat::zeros(grayImg.size(), CV_8UC1);
        cv::Mat edgeBinary = Mat::zeros(grayImg.size(), CV_8UC1);
        cv::Mat templat = Mat::zeros(edgeBinary.size(), CV_8UC1);

        // same with cvLaplace(src,dest,1). see imgproc/src/deriv.cpp
        Laplacian(grayImg, edgeGray, edgeGray.depth(), 1, 1, 0, BORDER_REPLICATE);
        threshold(edgeGray, edgeBinary, thresh, 255, CV_THRESH_BINARY);
#ifdef _DEBUG
        saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("GetDialogBoxImage-edgeGray") + std::string(".png");
        imwrite(saveImageName, edgeGray);

        saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("GetDialogBoxImage-edgeBinary") + std::string(".png");
        imwrite(saveImageName, edgeBinary);
#endif

        std::vector<Vec4i> lines;
        HoughLinesP(edgeBinary, lines, 1.0, CV_PI / 180.0, 20, 10.0, 1.0);
        std::vector<LineSegment> lList = Vec4iLinesToLineSegments(lines);
        vector<LineSegment> horizontalLines;

        for (unsigned int i=0; i<lList.size(); i++)
        {
            if (lList[i].p1.y == lList[i].p2.y)
            {
                horizontalLines.push_back(lList[i]);
                line(templat, lList[i].p1, lList[i].p2, Scalar(255), 1, 8, 0);
                if ((abs(lList[i].p1.y - edgeBinary.rows / 2) < minNum)
                    && (lList[i].p1.x < edgeBinary.cols / 2)
                    && (lList[i].p2.x > edgeBinary.cols / 2)
                    && (abs(lList[i].p1.x - lList[i].p2.x) > minLineLen))
                {
                    index = (int)(horizontalLines.size()) - 1;
                    minNum = abs(lList[i].p1.y - edgeBinary.rows / 2);
                }
            }
        }

        if (fabs(minNum - maxNum) < 1.0e-8)
            existCrashDialog = false;
        else
        {
            unsigned char grayValue = 255;
            LineSegment keyLine = horizontalLines[index];

            vector<LineSegment> targetLines;

            for (int y = 0; y < edgeBinary.rows; y++)
            {
                int count = 0;
                for (int x = keyLine.p1.x; x < keyLine.p2.x; ++x)
                {
                    unsigned char elemValue = templat.at<unsigned char>(y, x);
                    if (elemValue == grayValue)
                        count++;
                }
                if ((double)count / (keyLine.p2.x - keyLine.p1.x) > lineOverlayRate)
                {
                    LineSegment t(Point(keyLine.p1.x, y), Point(keyLine.p2.x, y));
                    targetLines.push_back(t);
                    if (y == keyLine.p1.y)
                        reference = (int)(targetLines.size()) - 1;
                }
            }

            start = reference;
            end = reference;
            startreference = reference;
            endreference = reference;

            if (osVersion < 5.0)
            {
                if (startreference > 0)
                    startreference = startreference - 1;

                endreference = endreference + 1;
            }

            accuracy = 1.0;
            for (int i = startreference; i >= 0 && (accuracy > areaOverlayRate); --i)
            {
                Rect re(targetLines[i].p1.x, 
                    srcImg.rows / 2 - edgeBinary.rows / 2 + targetLines[i].p1.y,
                    keyLine.p2.x - keyLine.p1.x,
                    keyLine.p2.y - targetLines[i].p2.y);
                Mat box (srcImg, re);
                accuracy = GetHistFeature(box);
                if (accuracy - areaOverlayRate > 0)
                    start = i;
            }

            accuracy = 1.0;
            for (unsigned int i = endreference; i < targetLines.size() && (accuracy > areaOverlayRate); ++i)
            {
                Rect re(keyLine.p1.x, 
                    srcImg.rows / 2 - edgeBinary.rows / 2 + keyLine.p1.y,
                    keyLine.p2.x - keyLine.p1.x,
                    targetLines[i].p2.y - keyLine.p2.y);
                Mat box (srcImg, re);
                accuracy = GetHistFeature(box);
                if (accuracy - areaOverlayRate > 0)
                    end = i;
            }

            if (start == end)
                existCrashDialog = false;
            else
            {
                Rect crashRect(targetLines[start].p1.x,
                    srcImg.rows / 2 - edgeBinary.rows / 2 + targetLines[start].p1.y,
                    targetLines[end].p2.x - targetLines[end].p1.x, 
                    targetLines[end].p2.y - targetLines[start].p2.y);

                Mat tempImage = srcImg.clone();
                crashImg = tempImage(crashRect);

                Point tempPoint = GetROIOKButtonPos(crashImg);
                okButton.x = crashRect.x + tempPoint.x;
                okButton.y = crashRect.y + tempPoint.y;

#ifdef _DEBUG
                saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(GetCurrentMillisecond()) + std::string("GetDialogBoxImage-crashImg") + std::string(".png");
                imwrite(saveImageName, crashImg);
#endif

                existCrashDialog = true;
            }
        }

        return existCrashDialog;
    }

    std::string DialogChecker::GetDialogBoxText(cv::Mat &roi)
    {
        std::string result = "";

        if (roi.empty() == false)
        {
            if (displayLanguage == Language::SIMPLIFIED_CHINESE)
            {
                string text = TextUtil::Instance().RecognizeChineseText(roi);
                result = WstrToStr(StrToWstr(text.c_str())); //Convert to UTF8 string.
            }
            else
                result = TextUtil::Instance().RecognizeEnglishText(roi);

            if (!result.empty())
                DAVINCI_LOG_DEBUG << (std::string("Get DialogBox Text: ") + result);
            else
                DAVINCI_LOG_DEBUG << ("Didn't Get Any Valid Charactors.");
        }

        return result;
    }

    void DialogChecker::InitBlackList()
    {
        crashKeywordBlackList.push_back("Unfortunately");
        crashKeywordBlackList.push_back("has");
        crashKeywordBlackList.push_back("stopped");
        crashKeywordBlackList.push_back("crash");
        crashKeywordBlackList.push_back("exception");

        if (displayLanguage == Language::SIMPLIFIED_CHINESE)
        {
            crashKeywordBlackList.push_back("很");
            crashKeywordBlackList.push_back("抱");
            crashKeywordBlackList.push_back("歉");
            crashKeywordBlackList.push_back("巳");
            crashKeywordBlackList.push_back("已");
            crashKeywordBlackList.push_back("停");
            crashKeywordBlackList.push_back("止");
            crashKeywordBlackList.push_back("运");
            crashKeywordBlackList.push_back("行");
        }

        // For Android No Response dialog
        anrKeywordBlackList.push_back("isn't");
        anrKeywordBlackList.push_back("responding");
        anrKeywordBlackList.push_back("want");
        anrKeywordBlackList.push_back("close");
        if (displayLanguage == Language::SIMPLIFIED_CHINESE)
        {
            anrKeywordBlackList.push_back("无");
            anrKeywordBlackList.push_back("响");
            anrKeywordBlackList.push_back("应"); 
            anrKeywordBlackList.push_back("将");
            anrKeywordBlackList.push_back("其");
            anrKeywordBlackList.push_back("关");
            anrKeywordBlackList.push_back("闭");
        }
    }

    // For fix bug2828 false positive of crash dialog in xiaomi device.
    void DialogChecker::InitWhiteList()
    {
        crashKeywordWhiteList.push_back("tablet");
        crashKeywordWhiteList.push_back("information");
        crashKeywordWhiteList.push_back("write");
        crashKeywordWhiteList.push_back("personal");
        crashKeywordWhiteList.push_back("data");
        crashKeywordWhiteList.push_back("apps");
        crashKeywordWhiteList.push_back("using");
        crashKeywordWhiteList.push_back("agree");
        crashKeywordWhiteList.push_back("SD");

        if (displayLanguage == Language::SIMPLIFIED_CHINESE)
        {
            crashKeywordWhiteList.push_back("手");
            crashKeywordWhiteList.push_back("机");
            crashKeywordWhiteList.push_back("日");
            crashKeywordWhiteList.push_back("志");
            crashKeywordWhiteList.push_back("信");
            crashKeywordWhiteList.push_back("息");
            crashKeywordWhiteList.push_back("个");
            crashKeywordWhiteList.push_back("人");
            crashKeywordWhiteList.push_back("写");
            crashKeywordWhiteList.push_back("入");
            crashKeywordWhiteList.push_back("允");
            crashKeywordWhiteList.push_back("许");
        }

        anrKeywordWhiteList.push_back("tablet");
        anrKeywordWhiteList.push_back("information");
        anrKeywordWhiteList.push_back("write");
        anrKeywordWhiteList.push_back("personal");
        anrKeywordWhiteList.push_back("data");
        anrKeywordWhiteList.push_back("apps");
        anrKeywordWhiteList.push_back("using");
        anrKeywordWhiteList.push_back("agree");
        anrKeywordWhiteList.push_back("SD");

        if (displayLanguage == Language::SIMPLIFIED_CHINESE)
        {
            anrKeywordWhiteList.push_back("手");
            anrKeywordWhiteList.push_back("机");
            anrKeywordWhiteList.push_back("日");
            anrKeywordWhiteList.push_back("志");
            anrKeywordWhiteList.push_back("信");
            anrKeywordWhiteList.push_back("息");
            anrKeywordWhiteList.push_back("个");
            anrKeywordWhiteList.push_back("人");
            anrKeywordWhiteList.push_back("写");
            anrKeywordWhiteList.push_back("入");
            anrKeywordWhiteList.push_back("允");
            anrKeywordWhiteList.push_back("许");
        }
    }

    bool DialogChecker::IsInCrashBlackList(const std::string &tmpStr)
    {
        int blackListMatchedCounter = 0;
        int whitelistmatchedCounter = 0;

        for (unsigned int i = 0; i < crashKeywordBlackList.size(); i++)
        {
            if (tmpStr.find(crashKeywordBlackList[i]) != string::npos)
                blackListMatchedCounter++;
        }

        if (blackListMatchedCounter > CrashMinBlackMatchedCount)
        {
            DAVINCI_LOG_INFO << "IsInCrashBlackList Detected: " << blackListMatchedCounter << " chars in blacklist from TextBox.";

            if (!manufactory.empty() && (manufactory.find("Xiaomi") != string::npos))
            {
                for (unsigned int i = 0; i < crashKeywordWhiteList.size(); i++)
                {
                    if (tmpStr.find(crashKeywordWhiteList[i]) != string::npos)
                        whitelistmatchedCounter++;
                }
            }

            if (whitelistmatchedCounter > CrashMaxWhiteMatchedCount) // when there are > 4 chars in white list, then assume it is not a crash dialog box.
            {
                DAVINCI_LOG_DEBUG << "IsInCrashBlackList Detected: " << whitelistmatchedCounter <<" chars in whitelist from TextBox, it is not Crash Dialog.";
                return false;
            }
            else
                return true;
        }
        else
            DAVINCI_LOG_DEBUG << ("IsInCrashBlackList didn't detecte > 1 chars in blacklist from TextBox.");

        return false;
    }

    bool DialogChecker::IsInANRBlackList(const std::string &tmpStr)
    {
        int blackListMatchedCounter = 0;
        int whitelistmatchedCounter = 0;

        for (unsigned int i = 0; i < anrKeywordBlackList.size(); i++)
        {
            if (tmpStr.find(anrKeywordBlackList[i]) != string::npos)
                blackListMatchedCounter++;
        }

        if (blackListMatchedCounter > ANRMinBlackMatchedCount)
        {
            DAVINCI_LOG_INFO << "IsInANRBlackList Detected: " << blackListMatchedCounter << " chars in blacklist from TextBox.";

            if (!manufactory.empty() && (manufactory.find("Xiaomi") != string::npos))
            {
                for (unsigned int i = 0; i < anrKeywordWhiteList.size(); i++)
                {
                    if (tmpStr.find(anrKeywordWhiteList[i]) != string::npos)
                        whitelistmatchedCounter++;
                }
            }

            if (whitelistmatchedCounter > ANRMaxWhiteMatchedCount) // when there are > 4 chars in white list, then assume it is not a ANR dialog box.
            {
                DAVINCI_LOG_DEBUG << "IsInANRBlackList Detected: " << whitelistmatchedCounter <<" chars in whitelist from TextBox, it is not ANR Dialog.";
                return false;
            }
            else
                return true;
        }
        else
            DAVINCI_LOG_DEBUG << ("IsInANRBlackList didn't detecte > 1 chars in blacklist from TextBox.");

        return false;
    }

    cv::Mat DialogChecker::GetClearScreenImage()
    {
        cv::Mat frame;

        frame = currentDevice->GetScreenCapture();
        frame = RotateFrameUp(frame, currentDevice->GetCurrentOrientation(true));

        IsCurFrameFromScreenCap = true;

        return frame;
    }

    cv::Mat DialogChecker::GetLastCrashFrame()
    {
        cv::Mat tmpFrame;

        boost::lock_guard<boost::mutex> lock(lockCrashFrame);
        if (!lastCrashFrame.empty())
            tmpFrame = lastCrashFrame.clone();

        return tmpFrame;
    }

    /// <summary>
    /// Get dialog type CrashDialog or ANRDialog
    /// </summary>
    /// <return>
    /// Return dialog type CrashDialog or ANRDialog
    /// </return>
    DialogCheckResult DialogChecker::GetDialogType()
    {
        return dialogType;
    }

    /// <summary>
    /// Set dialog type CrashDialog or ANRDialog
    /// </summary>
    /// <return>
    /// </return>
    void DialogChecker::SetDialogType(DialogCheckResult type)
    {
        dialogType = type;
    }

    void DialogChecker::CloseDialog(cv::Mat &crashFrame, int X, int Y, Orientation orientation)
    {
        int clickPX = X;
        int clickPY = Y;

        if ((clickPX != 0) || (clickPY != 0))
        {
            // When OK button line is gray, sometimes the OK button's pos incorrect.
            if (clickPY < crashFrame.rows / 2)
                clickPY = crashFrame.rows - clickPY;

#ifdef _DEBUG
            circle(crashFrame, cv::Point(clickPX, clickPY), 2, Red, 10);
#endif

            switch (orientation)
            {
            case Orientation::ReverseLandscape:
                {
                    if (crashFrame.cols < crashFrame.rows)
                        DAVINCI_LOG_DEBUG << ("CloseCrashDialog ReverseLandscape mode frame width/height incorrect.");
                    else
                    {
                        if (crashFrame.cols > clickPX)
                            clickPX = crashFrame.cols - clickPX;
                        else
                            DAVINCI_LOG_DEBUG << std::string("OK button X incorrect: ") << clickPX;

                        if (crashFrame.rows > clickPY)
                            clickPY = crashFrame.rows - clickPY;
                        else
                            DAVINCI_LOG_DEBUG << std::string("OK button Y incorrect: ") << clickPY;
                    }
                }
                break;

            case Orientation::ReversePortrait:
                {
                    if (crashFrame.cols > crashFrame.rows)
                        DAVINCI_LOG_DEBUG << ("CloseCrashDialog Portrait mode frame width/height incorrect.");
                    else
                    {
                        int temp = clickPX;

                        if (crashFrame.rows > clickPY)
                            clickPX = crashFrame.rows - clickPY;
                        else
                            DAVINCI_LOG_DEBUG << std::string("OK button X incorrect: ") << clickPX;

                        clickPY = temp;
                    }
                }
                break;

            case Orientation::Portrait:
                {
                    if (crashFrame.cols > crashFrame.rows)
                        DAVINCI_LOG_DEBUG << ("CloseCrashDialog Portrait mode frame width/height incorrect.");
                    else
                    {
                        int temp = clickPX;
                        clickPX = clickPY;

                        if (crashFrame.cols > temp)
                            clickPY = crashFrame.cols - temp;
                    }
                }
                break;

            case Orientation::Landscape:
            case Orientation::Unknown:
            default:
                break;
            }

            //When app crash, agent will lose connection, reconnect will take ~2S
            Size frameSize;
            if (crashFrame.cols < crashFrame.rows)
            {
                frameSize.width = crashFrame.rows;
                frameSize.height = crashFrame.cols;
            }
            else
            {
                frameSize.width = crashFrame.cols;
                frameSize.height = crashFrame.rows;
            }

            ClickOnFrame(currentDevice, frameSize, clickPX, clickPY, orientation, true);
            ThreadSleep(500);
            ClickOnFrame(currentDevice, frameSize, clickPX, clickPY, orientation, true);
            ThreadSleep(500);
            ClickOnFrame(currentDevice, frameSize, clickPX, clickPY, orientation, true);
        }
    }

    bool DialogChecker::SkipCheck(int opcode)
    {
        switch (opcode)
        {
        case QSEventAndAction::OPCODE_CLICK:
        case QSEventAndAction::OPCODE_TOUCHDOWN:
        case QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL:
        case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN:
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN_HORIZONTAL:
        case QSEventAndAction::OPCODE_SET_TEXT:
        case QSEventAndAction::OPCODE_SET_VARIABLE:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_REGEX:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_TXT:

        case QSEventAndAction::OPCODE_UNINSTALL_APP:
        case QSEventAndAction::OPCODE_INSTALL_APP:
        case QSEventAndAction::OPCODE_SWIPE:
        case QSEventAndAction::OPCODE_START_APP:
        case QSEventAndAction::OPCODE_STOP_APP:
        case QSEventAndAction::OPCODE_EXIT:
        case QSEventAndAction::OPCODE_CALL_SCRIPT:
        case QSEventAndAction::OPCODE_BATCH_PROCESSING:
        case QSEventAndAction::OPCODE_CALL_EXTERNAL_SCRIPT:
        case QSEventAndAction::OPCODE_EXPLORATORY_TEST:

        case QSEventAndAction::OPCODE_HOME:
        case QSEventAndAction::OPCODE_BACK:
        case QSEventAndAction::OPCODE_MENU:
        case QSEventAndAction::OPCODE_POWER:
        case QSEventAndAction::OPCODE_BUTTON:
            return false;
        default:
            return true;
        }
    }
}

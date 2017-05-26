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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __DIALOG_CHECKER_HPP__
#define __DIALOG_CHECKER_HPP__

#include "Checker.hpp"
#include "ScriptReplayer.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum class DialogCheckResult
    {
        invalid = 0,
        noIssueFound = 1,
        moreCheckRequired = 2,
        crashDialog = 3,
        anrDialog = 4
    };

    enum class DialogCloseStatus
    {
        invalid = 0,
        notFound = 1,
        closed = 2,
        confirmedClosed = 3,
    };

    /// <summary> A base class for resultchecker. </summary>
    class DialogChecker : public Checker
    {
        public:
            DialogChecker();
            
            explicit DialogChecker(const boost::weak_ptr<ScriptReplayer> obj);

            virtual ~DialogChecker();

            /// <summary>
            /// Worker thread loop
            /// </summary>
            virtual void WorkerThreadLoop() override;

            /// public them for unit test
            /// <summary>
            /// Is current frame from screen cap.
            /// </summary>
            bool IsCurFrameFromScreenCap; //public it for unit test

            /// <summary>
            /// Is current hypersoftcam mode.
            /// </summary>
            bool IsHyperSoftCamMode; //public it for unit test

            /// <summary>
            /// OS Version
            /// </summary>
            double osVersion;
            /// <summary>
            /// Manufactory
            /// </summary>
            string manufactory;
            /// <summary>
            /// language
            /// </summary>
            Language displayLanguage;

            /// <summary>
            /// Check if situation need to skip check.
            /// </summary>
            /// <returns>True for skip checking. </returns>
            virtual bool SkipCheck(int opcode) override;

            /// <summary>
            /// Detect if current frame has crashed.
            /// </summary>
            /// <param name="frame"></param>
            bool ScanFrame(const cv::Mat &frame, int opcode = DaVinci::QSEventAndAction::OPCODE_EXIT);

            /// <summary>
            /// Quick detect if there is a dialog box from the given image
            /// </summary>
            /// <param name="srcImg"></param>
            /// <return>
            /// Return true if exist dialog, false if no dialog is found
            /// </return>
            bool QuickDetectBox(const cv::Mat &srcImg);

            /// <summary>
            /// Quick detect if there is a dialog box from the given image
            /// </summary>
            /// <param name="srcImg"></param>
            /// <param name="p">start point of ROI</param>
            /// <param name="width">width of ROI</param>
            /// <param name="height">heigth of ROI</param>
            /// <return>
            /// Return true if exist dialog, false if no dialog is found
            /// </return>
            bool QuickDetectBox(const cv::Mat &srcImg, Point p, int width, int height);

            /// <summary>
            /// Further locate dialog box from a ScreenCap image
            /// </summary>
            /// <param name="srcImg"></param>
            /// <param name="crashImg">crash dialog</param>>
            /// <return>
            /// Return true if locate crash dialog, false if no crash dialog is found
            /// </return>
            bool GetDialogBoxImage(const cv::Mat &srcImg, cv::Mat &crashImg);

            /// <summary>
            /// Further locate dialog box from a ScreenCap image
            /// </summary>
            /// <param name="srcImg"></param>
            /// <param name="p">start point of ROI</param>
            /// <param name="width">width of ROI</param>
            /// <param name="height">heigth of ROI</param>
            /// <param name="crashImg">crash dialog</param>>
            /// <return>
            /// Return true if locate crash dialog, false if no crash dialog is found
            /// </return>
            bool GetDialogBoxImage(const cv::Mat &srcImg, Point p, int width, int height, cv::Mat &crashImg);

            /// <summary>
            /// Get crash dialog text.
            /// </summary>
            /// <param name="roi"></param>
            /// <returns></returns>
            std::string GetDialogBoxText(cv::Mat &roi);

            /// <summary>
            /// Get last Crashed Frame
            /// </summary>
            /// <returns>lastCrashFrame</returns>
            cv::Mat GetLastCrashFrame();

            /// <summary>
            /// Check if the string including some keyworks in crash dialog blacklist.
            /// </summary>
            /// <param name="tmpStr"></param>
            /// <returns></returns>
            bool IsInCrashBlackList(const std::string &tmpStr);

            /// <summary>
            /// Check if the string including some keyworks in ANR dialog blacklist.
            /// </summary>
            /// <param name="tmpStr"></param>
            /// <returns></returns>
            bool IsInANRBlackList(const std::string &tmpStr);

            /// <summary>
            /// Close crash dialog 
            /// </summary>
            /// <param name="crashFrame"></param>
            /// <param name="X"></param>
            /// <param name="Y"></param>
            /// <param name="orientation"></param>
            void CloseDialog(cv::Mat &crashFrame, int X, int Y, Orientation orientation);

            /// <summary>
            /// Get dialog type CrashDialog or ANRDialog
            /// </summary>
            /// <return>
            /// Return dialog type CrashDialog or ANRDialog
            /// </return>
            DialogCheckResult GetDialogType();

            /// <summary>
            /// Set dialog type CrashDialog or ANRDialog
            /// </summary>
            /// <return>
            /// </return>
            void SetDialogType(DialogCheckResult type);

        private:
            static const int DialogCheckerPeriod = 12000;
            static const int CrashMinBlackMatchedCount = 1;
            static const int ANRMinBlackMatchedCount = 1;
            static const int CrashMaxWhiteMatchedCount = 4;
            static const int ANRMaxWhiteMatchedCount = 4;

            boost::weak_ptr<ScriptReplayer> currentQScript;
            boost::shared_ptr<TargetDevice> currentDevice;

            queue<Mat> frameQueue;
            unsigned int maxFrameNumber;
            Mat lastCrashFrame;

            boost::mutex lockCrashFrame;

            DialogCheckResult curframeResult;
            DialogCheckResult dialogType;
            DialogCloseStatus closeStatus;

            std::vector<std::string> crashKeywordBlackList;
            std::vector<std::string> anrKeywordBlackList;

            std::vector<std::string> crashKeywordWhiteList;
            std::vector<std::string> anrKeywordWhiteList;

            Point okButton;

            void InitBlackList();
            void InitWhiteList();
            cv::Mat GetClearScreenImage();

            void TriggerScan();

            void EnqueueFrame(const cv::Mat &frame);

            cv::Mat DequeueFrame();

            /// <summary>
            /// Get OK button Position in crash dialog
            /// </summary>
            /// <param name="crashImg"></param>
            /// <return>
            /// coordinate of OK button in crash dialog
            /// </return>
            Point GetROIOKButtonPos(const cv::Mat &crashImg);

            /// <summary>
            /// Get histogram feature from a given image
            /// </summary>
            /// <return>
            /// Return similarity between a given image and carsh dialog
            /// </return>
            double GetHistFeature(const cv::Mat &image);
    };
}

#endif
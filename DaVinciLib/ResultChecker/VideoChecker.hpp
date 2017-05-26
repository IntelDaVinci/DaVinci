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

#ifndef __VIDEO_CHECKER_HPP__
#define __VIDEO_CHECKER_HPP__

#include "Checker.hpp"
#include "boost/filesystem.hpp"
#include "FeatureMatcher.hpp"
#include "AKazeObjectRecognize.hpp"
#include "ScriptReplayer.hpp"
#include "QScript/VideoCaptureProxy.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum class VideoMatchType
    {
        /// <summary>
        /// IMAGE MATCH
        /// </summary>
        IMAGE,
        /// <summary>
        /// COUNT MATCH
        /// </summary>
        COUNT,
        /// <summary>
        /// UNKNOWN MATCH
        /// </summary>
        UNKNOWN_MATCH_TYPE
    };

    enum class VideoMatchResult
    {
        /// <summary>
        /// IMAGE MATCH SUCCESS
        /// </summary>
        IMAGE_MATCH_SUCCESS,
        /// <summary>
        /// IMAGE MATCH FAIL
        /// </summary>
        IMAGE_MATCH_FAIL,
        /// <summary>
        /// COUNT MATCH SUCCESS
        /// </summary>
        COUNT_MATCH_SUCCESS,
        /// <summary>
        /// COUNT MATCH FAIL
        /// </summary>
        COUNT_MATCH_FAIL
    };

    enum class FrameTypeInVideo
    {
        /// <summary>
        /// Normal Frames
        /// </summary>
        Normal,
        /// <summary>
        /// Duplicated Static Frames, needn't replay this frame in quick replay mode.
        /// </summary>
        Static,
        /// <summary>
        /// Frame with event
        /// </summary>
        Event,
        /// <summary>
        /// Frame with warning
        /// </summary>
        Warning,
        /// <summary>
        /// Frame with error
        /// </summary>
        Error
    };

    /// <summary> A base class for resultchecker. </summary>
    class VideoChecker : public Checker
    {
        public:
            explicit VideoChecker(const boost::weak_ptr<ScriptReplayer> obj);

            virtual ~VideoChecker();

            /// <summary>
            /// Do offline check
            /// </summary>
            void DoOfflineChecking();

            /// <summary>
            /// Check if current video files include flickings.
            /// </summary>
            /// <param name="videoFilename"></param>
            /// <param name="threshold"></param>
            /// <returns>True for yes.</returns>
            bool IsVideoFlicking(const std::string &videoFilename, int threshold = 30);

            /// <summary>
            /// Get flicking count.
            /// </summary>
            /// <returns>flicking count.</returns>
            int GetFlicks();

            /// <summary>
            /// Get preview mismatch count.
            /// </summary>
            /// <returns>preview mismatch count.</returns>
            int GetPreviewMismatches();

            /// <summary>
            /// Get video mismatch count.
            /// </summary>
            /// <returns>mismatch count.</returns>
            int GetVideoMismatches();

            void CheckStaticFrames();

            /// <summary>
            /// Get object mismatch count.
            /// </summary>
            /// <returns>mismatch count.</returns>
            /// int GetObjectMismatches();
    private:
            boost::weak_ptr<ScriptReplayer> currentQScript;
            string replayVideoFilename;
            string replayQtsFilename;
            string videoFilename;
            string qtsFilename;
            string replayWaveFileName;
            string qAudioFilename;
            string currentQSDir;
            string replayDetailFileName;
            ObjectRecognizer objRecog;
            string COUNT_MATCH_STR;
            vector<QScript::TraceInfo> qtsTrace;
            vector<QScript::TraceInfo> replayQtsTrace;

            VideoCaptureProxy replayCapture;
            VideoCaptureProxy recordCapture;
            std::vector<double> replayTimeStampList;
            std::vector<double> recordTimeStampList;
            double replayFrameCount;
            double recordFrameCount;
            int recordQsEventNum;
            int replayQsEventNum;
            double altRatio;
            cv::Mat currentReplayFrame;
            cv::Mat currentRecordFrame;
            double replayStartTimestamp;
            double recordStartTimestamp;

            int videoMismatches, videoFlicks, previewMismatches, objectMismatches, objectCounts;

            /// <summary>
            /// Calculate the average num
            /// </summary>
            /// <param name="list"></param>
            /// <param name="start"></param>
            /// <param name="end"></param>
            /// <returns>average value</returns>
            int CalcAverage(std::vector<int> &list, int start, int end);

            /// <summary>
            /// Evaluate whether the flicking frame has enough intervals
            /// </summary>
            /// <param name="list"></param>
            /// <param name="i"></param>
            /// <param name="interval"></param> 
            /// <returns>true for enough</returns>
            bool IsEnoughInterval(std::vector<double> &list, int i, int interval);

            /// <summary>
            /// Detect video flicking between two time points.
            /// </summary>
            /// <param name="sourceCapture"></param>
            /// <param name="start"></param>
            /// <param name="end"></param>
            /// <param name="threshold"></param>
            /// <returns></returns>
            bool DetectVideoFlicking(const std::string &videoFilename, double start, double end, int threshold = 30);

            bool IsCurrentScriptNeedVideoCheck();

            bool InitVideoMatchResource();

            bool ReleaseVideoMatchResource();

            bool IsVideoMatched();
                                        //SURFObjectRecognize::PreprocessType sourcePrepType = SURFObjectRecognize::PreprocessType::Keep, 
                                        //SURFObjectRecognize::PreprocessType targetPrepType = SURFObjectRecognize::PreprocessType::Keep);

            bool IsImageMatched(const cv::Mat &currentFrame, const cv::Mat &referenceFrame, 
                              int size, double altRatio, int lineNumber);
                              //SURFObjectRecognize::PreprocessType sourcePrepType = SURFObjectRecognize::PreprocessType::Keep, 
                              //SURFObjectRecognize::PreprocessType targetPrepType = SURFObjectRecognize::PreprocessType::Keep)

            bool IsObjectMatched();

            bool IsStaticMatched(const cv::Mat &currentFrame, const cv::Mat &referenceFrame, double threshold=0.02);
    };
}

#endif
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

#ifndef __BLANK_FRAMES_CHECK_HPP__
#define __BLANK_FRAMES_CHECK_HPP__

#include "boost/filesystem.hpp"
#include "FeatureMatcher.hpp"
#include "VideoWriterProxy.hpp"
#include "QScript/VideoCaptureProxy.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "QScript.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class BlankPagesChecker
    {
    public:
        BlankPagesChecker();
        virtual ~BlankPagesChecker();

        /// Check if there are blank frames in video.
        /// </summary>
        /// <param name="videoFilename"></param>
        /// <returns>True for yes.</returns>
        bool HasVideoBlankFrames(const string &videoFile);

    private:
        bool result;
        int barHeight;
        double frameCount;
        string saveVideoFolder;
        int maxThreads;
        int startCheckIndex;
        vector<Rect> naviBarRects;
        vector<Rect> statusBarRects;
        vector<int> saveFrmIndexVec;
        int frmsPerSubVideo;
        int frameVectorSize;
        float intervalTime;
        string qtsFile;
        vector<double> timeStamps;
        bool flagHasBlankFrames;
        boost::mutex lockUpdateStartIndex;
        boost::mutex lockUpdateResult;
        boost::mutex lockReadFrm;
        VideoCaptureProxy replayCapture;
        string logFolder;


        void CalculateBarsRect(Size devSize, Orientation ori, Rect &statusBarRect, Rect &naviBarRect);
        void MappingBarsRect(cv::Mat &frame, vector<Rect> &nBarRect, vector<Rect> &sBarRect);
        void SaveVideoClips(const string &videoFile, int startIndex, int endIndex, Size frmSize);
        void CheckFrmsInSubVideos(const string &videoFile, int startIndex);
        vector<double> GetTimeStamp();
        string LocateTimeStampFile(const string &vidName);
        double FrameIndex2TimeStamp(int index);
        int TimeStamp2FrameIndex(double timeStamp);
        cv::Mat ResizeFrame(cv::Mat orgFrame, int ratio);
    };
}

#endif
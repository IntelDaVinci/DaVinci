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

#ifndef __TIMEMEASURE__
#define __TIMEMEASURE__

#include "TestInterface.hpp"
#include "FeatureMatcher.hpp"
#include "VideoWriterProxy.hpp"
#include "opencv/cv.h"
#include <string>
#include <vector>
#include <cmath>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <DaVinciCommon.hpp>

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/
//using System.Windows.Controls;
//using System.Windows.Documents;

// Open source kit for integrating C++ OpenCV APIs into C#
// Warning: GPL license!


namespace DaVinci
{
    class DeviceFrameIndexInfo
    {
    public:
        DeviceFrameIndexInfo() {};
        ~DeviceFrameIndexInfo() {};

        bool ParseFrameIndexInfo();
        int64 CalculateElapsedTime(int frameIndex);
        //bool RecordDevCurTimeStamp();
        static int64 devCurTimeStamp;

    private:
        std::map<int, int64> frameIndexInfo;
        boost::shared_ptr<AndroidTargetDevice> dut;
    };

    class TimeMeasure : public TestInterface
    {
    private:
        cv::Mat refMat;
        int timeout;
        bool hit;
        boost::shared_ptr<FeatureMatcher>  pMatcher;
        const double defaultUnmatchRatioThreshold;
        double unmatchRatioThreshold;
        std::string apk_class;
        std::string ini_filename;
        double startTime;
        double lastElapsed;
        double lastHitElapsed;
        cv::Mat lastResult;

        std::string tmpVideoFile;
        struct FrameInfo{
            double elapsed;
            int frameIndex;
            int deviceFrameIndex;
            bool isHit;
            double unmatchedRatio;
            FrameInfo(int idx,bool hitted,double ratio, int devIdx) : frameIndex(idx),
                                                                      isHit(hitted),
                                                                      unmatchedRatio(ratio),
                                                                      deviceFrameIndex(devIdx)
            {
            }
        };

        std::vector<boost::shared_ptr<FrameInfo> > frameInfos;
        void saveResultFrames(int hitdFrame);
        VideoWriterProxy videoWriter;
        Size videoFrameSize;
        bool isOffline;
        int matchFirstFrame();
        std::string tempFilePath;
        boost::shared_ptr<TestReportSummary> testReportSummary;
        string referenceImage;
        Rect targetRoi;
        Size refSize;
        const double defaultRoiUnmatchRatioThreshold;
        double roiUnmatchedRatioThreshold;
        bool saveVideo;
        DeviceFrameIndexInfo devFrameIndexInfo;
    public:
        TimeMeasure(const std::string &iniFilename);
        virtual bool Init() override;

        boost::shared_ptr<TestReport> testReport;

        virtual CaptureDevice::Preset CameraPreset() override;

        virtual void Destroy() override;

        virtual void Start() override;

        double GetElapsed(double timeStamp);

        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

        /// <summary>
        /// Draw the model image and observed image, the matched features and homography projection.
        /// </summary>
        /// <param name="modelRecognize">The model recognize</param>
        /// <param name="observedImage">The observed image</param>
        /// <param name="isHit">The hit</param>
        /// <param name="originalImage">The original image</param>
        /// <param name="scale">The scale</param>
        /// <param name="unmatchedRatio"> The unmatched ratio</param>
        /// <returns>The model image and observed image, the matched features and homography projection.</returns>
        cv::Mat DetectObject( boost::shared_ptr<FeatureMatcher> pFeatMather, const cv::Mat &observedImage, bool &isHit, float scale, double &unmatchedRatio);

        void PopulateReport(string referenceimagename, string targetimagename,  string beforeimagename, string launchtimevalue);
    };
}


#endif	//#ifndef __TIMEMEASURE__

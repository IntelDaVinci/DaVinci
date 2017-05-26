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

#ifndef __FPS_MEASURE_HPP__
#define __FPS_MEASURE_HPP__

#include "TestInterface.hpp"
#include "ObjectRecognize.hpp"

#include <atomic>

namespace DaVinci
{
    class FeatureMatcher;
    class HighResolutionTimer;

    // passed to matcher thread
    struct FrameInfo
    {
        Mat image;
        double timestamp;
    };

    class FPSMeasure : public TestInterface
    {
    public:
        explicit FPSMeasure(const string& fileName) :
        corners(vector<vector<cv::Point2f>>(2)),
            _debug(false), motionMinDistance(1.0F),
            isStarted(false), isTraceStop(false),
            caseName(fileName), enableCam(false),
            dispWidth(0), dispHeight(0),
            droppedFramesCount(0), isStartedAtInit(false)
        {
        }

        virtual ~FPSMeasure();

        /// <summary>
        /// initialize all variables needed by FPSMeasure
        /// </summary>
        /// <returns></returns>
        virtual bool Init() override;
        virtual void Start() override;
        virtual Mat ProcessFrame(const Mat &frame, const double timeStamp) override;

        void PreDestroy();
        /// <summary>
        /// Set image as a reference to match observed images.
        /// </summary>
        /// <param name="image">The mat reference as the reference image to start FPS measure session.</param>
        /// <param name="refPrepType">The preprocess method for the reference image.</param>
        /// <param name="obsPrepType">The preprocess method for the observed images.</param>
        void SetFPSStartImage(const Mat &image, PreprocessType refPrepType = PreprocessType::Keep, PreprocessType obsPrepType=PreprocessType::Keep);
        /// <summary>
        /// Set image as a reference to match observed images.
        /// </summary>
        /// <param name="image">The mat reference as the reference image to stop FPS measure session.</param>
        /// <param name="refPrepType">The preprocess method for the reference image.</param>
        /// <param name="obsPrepType">The preprocess method for the observed images.</param>
        void SetFPSStopImage(const Mat &image, PreprocessType refPrepType = PreprocessType::Keep, PreprocessType obsPrepType=PreprocessType::Keep);
        /// <summary>
        /// Because record video is normalized to 1280 pixels by width and optical flow algorithm can get motion accuracy below one pixel, we need original image size to get correct minimum motion distance.
        /// </summary>
        /// <param name="width">The display width in landscape state, unit is pixel.</param>
        /// <param name="height">The display height in landscape state, unit is pixel.</param>
        void SetDisplaySize(int width, int height);

        void GetDisplaySize(int & width, int & height);

        // inline methods
        double GetCameraAvgFPS()
        {
            return avgFPS;
        }
        bool GetStarted()
        {
            return isStarted;
        }
        void SetStarted(bool val)
        {
            isStarted = val;
        }

        void SetStopped(bool val)
        {
            isTraceStop = val;
        }

        void SetDutAppName(const string& val)
        {
            dutAppName = val;
        }
        void SetEnableCam(bool val)
        {
            enableCam = val;
        }
        /// <summary>
        /// if true, poll for SurfaceView every 1 second, could increase system overhead
        /// </summary>
        /// <returns></returns>
        void SetStartAtInit(bool val)
        {
            isStartedAtInit = val;
        }

        /// <summary>
        /// caller can calculate theory FPS with this argument
        /// </summary>
        /// <returns>the dropped framse count during FPS measure session</returns>
        int GetDroppedFramesCount()
        {
            return droppedFramesCount;
        }
        /// <summary>
        /// get normalized dump file contains all the SurfaceFlinger handled frames during FPS measure session 
        /// </summary>
        /// <returns>Absolute file path of normailezed dump file</returns>
        string GetDumpsysOutputFile();


    private:
        // used by camera FPS report also
        double maxFPS, minFPS, avgFPS;
        // used by trace FPS report also
        double maxTraceFPS, minTraceFPS, avgTraceFPS, jankRatio;
        // used for FPS report 
        string startTime, stopTime;
        // used for FPS report
        string caseName;

        // for configurable min motion distance
        int dispWidth;
        int dispHeight;
        double motionMinDistance;

        bool _debug;

        bool enableCam;

        std::atomic<bool> isStarted;
        std::atomic<bool> isTraceStop;

        long long totalFrameCnt;
        long long cameraFrameCnt;
        long long traceFrameCount;
        long long movingFrameCnt;

        string dutAppName;

        Mat matchStartImage;
        Mat matchStopImage;
        boost::shared_ptr<FeatureMatcher> matcherStart;
        boost::shared_ptr<FeatureMatcher> matcherStop;

        // previous image and its and current feature points
        vector<vector<cv::Point2f>> corners;
        Mat prevGray;

        // intermediate file stream to save video raw data
        boost::shared_ptr<ofstream> rawVideoFileStrm;
        string rawVideoFileName;

        // matcher thread needs these
        PreprocessType refPreprocessType, obsPreprocessType;
        queue<cv::Mat> matcherFrameQueue;
        boost::mutex matcherFrameQueueLock;
        boost::shared_ptr<GracefulThread> matcherThread;
        bool MatcherFunc();

        // store thread needs these
        queue<FrameInfo> storeFrameQueue;
        boost::mutex storeFrameQueueLock;
        boost::condition_variable storeFrameQueueEvt;
        boost::shared_ptr<GracefulThread> storeThread;
        bool StoreFunc();

        // internal methods
        void InitTrace();
        void StartTrace();
        void StopTrace();
        void AnalyseFrame(const Mat& frame); 
        void DestroyInternal();

        //For xml report
        boost::shared_ptr<TestReportSummary> testReportSummary;

        // for dumpsys
        boost::shared_ptr<HighResolutionTimer> dumpTimer;
        void StartDumpsys();
        void StopDumpsys();
        void DumpTimerHandler(HighResolutionTimer& timer);
        string sfDumpCmd;
        string dumpFileName;

        // for better result checker 
        int droppedFramesCount;
        double firstStoreTime;

        std::atomic<bool> isStartedAtInit;
    };
}

#endif

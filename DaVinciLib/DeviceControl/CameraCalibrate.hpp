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

#ifndef __CAMERA_CALIRATE_HPP__
#define __CAMERA_CALIRATE_HPP__

#include "TestInterface.hpp"
#include "PhysicalCameraCapture.hpp"
#include "FeatureMatcher.hpp"

namespace DaVinci
{
    /// <summary> Types of camera calibration. </summary>
    enum class CameraCalibrationType
    {
        MinCalibrateType        = -1,
        ResetCalibrate          = -1,
        HighSpeedCalibrate      = 0,
        HighResolutionCalibrate = 1,
        Unsupported             = 2,
        FocusCalibrate          = 3,
        DistanceCalibrate       = 4,
        CameraParamCalibrate    = 5,
        OverallCalibrate        = 6,
        MaxCalibrateType        = 6
    };

    class CameraCalibrate : public TestInterface
    {
    private:
        CameraCalibrationType calibType;
        boost::atomic<int> calibStage;
        boost::atomic<bool> calibFailed;
        int hit;
        bool firstFrame;
        double firstTimeStamp;
        double timeout;
        Mat homography;
        Size lotusSize;
        CaptureDevice::Preset calibPreset;
        int startCount;
        static const int maxStartCount = 30;

        boost::shared_ptr<TargetDevice> targetDevice;
        boost::shared_ptr<PhysicalCameraCapture> cameraCap;
        boost::shared_ptr<FeatureMatcher> featureMatcher;

        double lastTimeStamp;

        static const int intervalSetCamera = 1000; // 1000 millisecond
        int focus, maxFocus, maxFocusEntropy;
        double maxContrast, maxEntropy;
        bool settingFocus;

        int outBoundCount;
        bool pushing;
        double lastPushTime;

        bool isStartCalibParams;
        int paramMax, paramMin, paramStep, currentValue;
        bool done;
        double maxEntropyValue, oldEntropyValue;
        int oldValue;
        int bestGainValue, bestBrightnessValue;
        static const int calibDoneStage = 8;

        void ComputeCalibratePreset();

    public:
        CameraCalibrate(CameraCalibrationType type);

        /// <summary>
        /// Init the test and read parameters from ini file
        /// </summary>
        /// <returns>if init succeeded</returns>
        virtual bool Init() override;

        void InitCommon();
        void InitFocusCalibration();
        void InitCameraParamCalibration();

        /// <summary>
        /// Process the frame, this is a dummy function
        /// </summary>
        /// <param name="frame">The frame captured</param>
        /// <param name="timeStamp">the time that the frame is captured</param>
        /// <returns>the processed frame</returns>
        virtual cv::Mat ProcessFrame(const cv::Mat & frame, const double timeStamp);

        cv::Mat CalibratePerspective(const cv::Mat & frame, const double timeStamp);

        double CalculateContrast(const Mat & image);
        Mat CalibrateFocus(const Mat & frame, const double timestamp);

        bool inBound(int width, int height, double x, double y);
        bool inSmallBound(int width, int height, double x, double y);
        cv::Mat CalibrateDistance(const cv::Mat &frame, const double timeStamp);

        cv::Mat GetROIForEntropy(cv::Mat &image);
        bool SetCaptureProperty(int prop, int value);
        double GetFrequencyOfBin(const Mat channel, const int histSize);
        vector<Mat> CalculateHistogram(const cv::Mat &image, int binSize);
        double ComputeShannonEntropy(const cv::Mat &image);

        void CalibrateByEntropy(const cv::Mat &frame, int prop, const std::string &name, int &bestValue);
        cv::Mat CalibrateCameraParameters(const cv::Mat &frame, double timeStamp);
        cv::Mat CalibrateOverall(const cv::Mat &frame, double timeStamp);

        /// <summary>
        /// Kill the app being tested
        /// </summary>
        virtual void Destroy() ;

        /// <summary>
        /// this test does not care about camera preset
        /// </summary>
        /// <returns>-1 means don't care</returns>
        virtual CaptureDevice::Preset CameraPreset() ;

        void SetCalibrateStage(int calibStageValue);

        int GetCalibrateStage();

        void SetCalibrateResult(bool calibResult);

        void SetCalibratePushing(bool pushingValue);

        void SetCalibrateLastPushingTime(double timeValue);

        void SetCalibrateHitValue(int hitValue);

    };
}

#endif
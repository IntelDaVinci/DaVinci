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

#include "CameraCalibrate.hpp"
#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include "ObjectRecognize.hpp"

namespace DaVinci
{
    string cameraCalibrationName[] =
    {
        "High Speed Calibration",
        "High Resolution Calibration",
        "Unsupported Calibration",
        "Focus Calibration",
        "Distance Calibration",
        "Camera Parameters Calibration",
        "Overall Calibration",
    };


    CameraCalibrate::CameraCalibrate(CameraCalibrationType type)
    {
        calibType = type;

        InitCommon();
    }

    bool CameraCalibrate::Init()
    {
        SetName("CameraCalibrate");

        InitCommon();

        if ((calibType < CameraCalibrationType::MinCalibrateType)
            || (calibType > CameraCalibrationType::MaxCalibrateType)
            || (calibType == CameraCalibrationType::Unsupported))
        {
            DAVINCI_LOG_ERROR << "Undefined calibration type: " << static_cast<int>(calibType);
            SetFinished();
            return false;
        }

        ComputeCalibratePreset();
        if (calibType == CameraCalibrationType::ResetCalibrate)
        {
            DAVINCI_LOG_INFO << "Reset calibration!";
            if (cameraCap != nullptr)
                cameraCap->ResetCalibrateHomography();
            SetFinished();
            return true;
        }

        // return if agent is not connected
        targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "Calibration stops as no device is connected!";
            SetFinished();
            return false;
        }

        // return if capture device is not camera
        if (cameraCap == nullptr)
        {
            DAVINCI_LOG_ERROR << "Calibration stops as no physical camera!";
            SetFinished();
            return false;
        }

        // return if calibration type is set distance and no ffrd is connected
        if (calibType == CameraCalibrationType::DistanceCalibrate)
        {
            boost::shared_ptr<HWAccessoryController> controller
                = DeviceManager::Instance().GetCurrentHWAccessoryController();
            if (controller == nullptr)
            {
                DAVINCI_LOG_ERROR << "Calibration stops as no FFRD connected!";
                SetFinished();
                return false;
            }
        }

        // return if calibration type is focus, and camera model is high speed camera
        if ((cameraCap->IsHighSpeedCamera() == true)
            && ((calibType == CameraCalibrationType::FocusCalibrate)))
        {
            DAVINCI_LOG_ERROR << "Calibration stops as high speed camera doesn't support this calibration!";
            SetFinished();
            return false;
        }

        // check the resource image exists or not
        string davinciHome = TestManager::Instance().GetDaVinciHome();
        if (davinciHome.empty() == true)
        {
            DAVINCI_LOG_ERROR << "Cannot get Davinci Home Directory!";
            SetFinished();
            return false;
        }
        string lotusImagePath = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("Resources", "lotus.jpg"));
        if (boost::filesystem::is_regular_file(lotusImagePath) == false)
        {
            DAVINCI_LOG_ERROR << string("lotus image file doesn't exist: ") << lotusImagePath;
            SetFinished();
            return false;
        }
        Mat lotusImage = imread(lotusImagePath);
        if (lotusImage.data == nullptr)
        {
            DAVINCI_LOG_ERROR << "Calibration stops as lotus image is corrupted!";
            SetFinished();
            return false;
        }

        DAVINCI_LOG_INFO << "Calibrating..." 
            << cameraCalibrationName[(int)calibType];

        int width = targetDevice->GetDeviceWidth();
        int height = targetDevice->GetDeviceHeight();
        if (width < height)
        {
            int temp = width;
            width = height;
            height = temp;
        }
        DAVINCI_LOG_INFO << "device screen resolution = " << width << "x" << height;
        if ((width == 0) || (height == 0))
        {
            width = 1280;
            height = 720;
            DAVINCI_LOG_INFO << "Failed to get the device's resolution, use default value ("
                << width << "x" << height << ")!";
        }

        Mat lotus;

        resize(lotusImage, lotus,
            Size(
            PhysicalCameraCapture::defaultCalibrationWidth[(int)calibPreset],
            PhysicalCameraCapture::defaultCalibrationWidth[(int)calibPreset] * height / width
            ), 0, 0, INTER_CUBIC);

        lotusSize = lotus.size();
        DAVINCI_LOG_DEBUG << "lotus size=" <<lotusSize;

        boost::shared_ptr<ObjectRecognize> lotusRecognize = ObjectRecognize::CreateObjectRecognize();
        featureMatcher = boost::shared_ptr<FeatureMatcher> (new FeatureMatcher(lotusRecognize));
        featureMatcher->SetModelImage(lotus, false);
        if ((calibType == CameraCalibrationType::HighResolutionCalibrate)
            || (calibType == CameraCalibrationType::HighSpeedCalibrate)
            || (calibType == CameraCalibrationType::DistanceCalibrate))
            cameraCap->SetCalibrateHomography(Mat(), calibPreset, 0, 0);

        if (targetDevice->IsAgentConnected())
        {
            targetDevice->ShowLotus();
        }
        else
        {
            DAVINCI_LOG_ERROR << "Calibration stops as agent is not connected!";
            SetFinished();
            return false;
        }

        // Sleep 2 seconds to let Lotus app completely show up. Since Lotus app
        // is vertical, when it is started from horizontal mode, Android would
        // show a rotating animation on start. This confuses SURFObjectRecognize and
        // generates a distorted screen.
        ThreadSleep(2000);

        switch(calibType)
        {
        case CameraCalibrationType::FocusCalibrate:
            InitFocusCalibration();
            break;
        case CameraCalibrationType::CameraParamCalibrate:
            InitCameraParamCalibration();
            break;
        case CameraCalibrationType::OverallCalibrate:
            InitFocusCalibration();
            InitCameraParamCalibration();
            break;
        default:
            // HighResolutionCalibrate, HighSpeedCalibrate, DistanceCalibrate
            break;
        }

        DAVINCI_LOG_INFO << "Initialize done.";

        return true;
    }

    void CameraCalibrate::InitCommon()
    {
        calibStage = -1;
        calibFailed = false;
        firstFrame = true;
        firstTimeStamp = GetCurrentMillisecond();
        timeout = 60000;
        startCount = 0;
        lastTimeStamp = firstTimeStamp;

        hit = -1;

        focus = 0;
        maxFocus = focus;
        maxFocusEntropy = focus;
        maxEntropy = 0.0;
        maxContrast = 0;
        settingFocus = false;

        outBoundCount = 0;
        pushing = false;
        lastPushTime = firstTimeStamp;

        isStartCalibParams = false;
        bestGainValue = 0;
        bestBrightnessValue = 0;
        done = false;
        maxEntropyValue = -DBL_MAX;
        paramMax = 0;
        paramMin = 0;
        paramStep = 0;
        oldValue = 0;
        oldEntropyValue = 0;
        currentValue = 0;

        calibPreset = CaptureDevice::Preset::HighSpeed;
    }

    void CameraCalibrate::InitFocusCalibration()
    {
        focus = (int)cameraCap->GetCapturePropertyMin(CV_CAP_PROP_FOCUS);

        maxFocus = focus;
        maxFocusEntropy = focus;
    }

    void CameraCalibrate::InitCameraParamCalibration()
    {
        DAVINCI_LOG_INFO << "Please put device in the middle of camera view.";
        // generate a new config file with default config
        cameraCap->CopyCameraConfig();
        bestGainValue = (int)cameraCap->GetCaptureProperty(CV_CAP_PROP_GAIN);
        bestBrightnessValue = (int)cameraCap->GetCaptureProperty(CV_CAP_PROP_BRIGHTNESS);
    }

    cv::Mat CameraCalibrate::ProcessFrame(const cv::Mat & frame, const double timeStamp)
    {
        if (calibStage == -1)
        {
            if (startCount < maxStartCount)
            {
                startCount++;

                if (featureMatcher != nullptr)
                {
                    Mat homography = featureMatcher->Match(frame);
                    if (homography.empty())
                    {
                        return frame;
                    }
                }
            }
            else
            {
                if (targetDevice == nullptr)
                {
                    DAVINCI_LOG_ERROR << "Calibration stops as no device is connected!";
                    SetFinished();
                }
                else if (targetDevice->IsAgentConnected() == false)
                {
                    DAVINCI_LOG_ERROR << "Calibration stops as agent is not connected!";
                    SetFinished();
                }
            }

            calibStage++;
            DAVINCI_LOG_INFO << "Calibration started.";
            return frame;
        }

        if (firstFrame)
        {
            firstFrame = false;
            firstTimeStamp = timeStamp;
            lastTimeStamp = timeStamp;
        }

        switch (calibType)
        {
        case CameraCalibrationType::HighResolutionCalibrate:
        case CameraCalibrationType::HighSpeedCalibrate:
            switch (calibStage)
            {
            case 0:
                return CalibratePerspective(frame, timeStamp);
            case 1:
                if (calibFailed == false)
                {
                    DAVINCI_LOG_INFO << "Calibration done.";
                }
                else
                {
                    DAVINCI_LOG_INFO << "Calibration failed.";
                }
                calibStage++;
                SetFinished();
                break;
            }
            break;
        case CameraCalibrationType::FocusCalibrate:
            switch (calibStage)
            {
            case 0:
                return CalibrateFocus(frame, timeStamp);
            case 1:
                DAVINCI_LOG_INFO << "Calibration done.";
                calibStage++;
                SetFinished();
                break;
            }
            break;
        case CameraCalibrationType::DistanceCalibrate:
            switch (calibStage)
            {
            case 0:
                return CalibrateDistance(frame, timeStamp);
            case 1:
                if (calibFailed == false)
                {
                    DAVINCI_LOG_INFO << "Calibration done.";
                }
                else
                {
                    DAVINCI_LOG_INFO << "Calibration failed.";
                }
                calibStage++;
                SetFinished();
                break;
            }
            break;
        case CameraCalibrationType::CameraParamCalibrate:
            return CalibrateCameraParameters(frame, timeStamp);
        case CameraCalibrationType::OverallCalibrate:
            return CalibrateOverall(frame, timeStamp);
        default:
            DAVINCI_LOG_ERROR << "Unsupported calibration: " << (int)calibType;
            SetFinished();
            break;
        }

        return frame;
    }

    void CameraCalibrate::Destroy()
    {
        if (targetDevice != nullptr && targetDevice->IsAgentConnected() == true)
        {
            targetDevice->HideLotus();
        }
    }

    cv::Mat CameraCalibrate::CalibratePerspective(const cv::Mat &frame, const double timeStamp)
    {
        if (hit >= 3)
        {
            Mat result = Mat::zeros(lotusSize, frame.type());
            warpPerspective(frame, result, homography, lotusSize, INTER_LINEAR | WARP_INVERSE_MAP);
            calibStage++;

            if (cameraCap != nullptr)
            {
                cameraCap->SetCalibrateHomography(homography, calibPreset, lotusSize.width, lotusSize.height);

                if (calibPreset == CaptureDevice::Preset::HighResolution)
                {
                    Mat homographyOthers;
                    int width = 0, height = 0;

                    if (true == cameraCap->CalculateCalibrateHomography(homography,
                        CaptureDevice::Preset::HighResolution,
                        CaptureDevice::Preset::AIResolution,
                        homographyOthers, width, height))
                    {
                        cameraCap->SetCalibrateHomography(homographyOthers, CaptureDevice::Preset::AIResolution, width, height);
                        homographyOthers.release();
                    }
                    if (true == cameraCap->CalculateCalibrateHomography(homography,
                        CaptureDevice::Preset::HighResolution,
                        CaptureDevice::Preset::HighSpeed,
                        homographyOthers, width, height))
                    {
                        cameraCap->SetCalibrateHomography(homographyOthers, CaptureDevice::Preset::HighSpeed, width, height);
                        homographyOthers.release();
                    }
                }

                return result;
            }
        }

        if (timeStamp - firstTimeStamp > timeout)
        {
            calibFailed = true;
            calibStage++;
            imwrite("calibrate-timeout.png", frame);
            DAVINCI_LOG_INFO << "Perspective calibration timeout.";

            return frame;
        }

        homography = featureMatcher->Match(frame);

        if (homography.empty() == false)
        {
            Mat result = Mat::zeros(lotusSize, frame.type());
            warpPerspective(frame, result, homography, lotusSize, INTER_LINEAR | WARP_INVERSE_MAP);
            hit++;

            string imageFileName = "calibrate_result-" + boost::lexical_cast<string>(hit) + ".png";
            imwrite(imageFileName, frame);
            DAVINCI_LOG_INFO << "Perspective calibration done. [" << hit << "]";

            return result;
        }

        return frame;
    }

    bool CameraCalibrate::SetCaptureProperty(int prop, int value)
    {
        bool success = false;
        const int tryCount = 30;

        if (cameraCap == nullptr)
        {
            DAVINCI_LOG_ERROR << "Camera is closed! Cannot set capture property!";
            return false;
        }

        for (int i = 0; i < tryCount; i++)
        {
            cameraCap->SetCaptureProperty(prop, value);
            int currentVal = static_cast<int>(cameraCap->GetCaptureProperty(prop));
            if (currentVal != value)
            {
                DAVINCI_LOG_DEBUG << std::string("Property (") 
                    << prop << std::string(") is not ready after being set. value=") 
                    << value << std::string(", value in camera= ") << currentVal;
            }
            else
            {
                success = true;
                break;
            }
        }

        return success;
    }

    double CameraCalibrate::CalculateContrast(const Mat & image)
    {
        cv::Mat gray;
        cvtColor(image, gray, CV_BGR2GRAY);

        double total = 0;
        for (int i = gray.cols / 2 - 10; i < gray.cols / 2 + 10; i++)
        {
            for (int j = gray.rows / 2 - 10; j < gray.rows / 2 + 10; j++)
            {
                for (int k = -1; k <= 1; k++)
                {
                    for (int l = -1; l <= 1; l++)
                    {
                        total += abs(gray.at<uchar>(j + k, i + l) - gray.at<uchar>(j, i));
                    }
                }
            }
        }
        return total;
    }

    Mat CameraCalibrate::CalibrateFocus(const Mat & frame, const double timestamp)
    {
        cv::Mat image = frame;
        if (!settingFocus)
        {
            SetCaptureProperty(CV_CAP_PROP_FOCUS, focus);
            lastTimeStamp = timestamp;
            settingFocus = true;
        }
        else if ((int)(timestamp - lastTimeStamp) > intervalSetCamera)
        {
            double contrast = CalculateContrast(image);
            cv::Mat imageClone = GetROIForEntropy(image);
            double entropy = ComputeShannonEntropy(imageClone);
            DAVINCI_LOG_INFO << "Focus distance=" << focus
                << ", contrast=" << contrast
                <<", entropy=" << entropy;
            if (contrast > maxContrast)
            {
                maxFocus = focus;
                maxContrast = contrast;
            }

            if (entropy > maxEntropy)
            {
                maxFocusEntropy = focus;
                maxEntropy = entropy;
            }

            settingFocus = false;

            if (focus >= static_cast<int>(cameraCap->GetCapturePropertyMax(CV_CAP_PROP_FOCUS)))
            {
                DAVINCI_LOG_INFO << std::string("Focus distance = ") 
                    << maxFocus << std::string(", ") << maxFocusEntropy;
                if (maxFocus > maxFocusEntropy)
                {
                    focus = maxFocusEntropy;
                }
                else
                {
                    focus = maxFocus;
                }

                // save focus value to config file for camera
                cameraCap->SaveCameraConfig(
                    PhysicalCameraCapture::focusIndexInCamCfg,
                    focus);
                SetCaptureProperty(CV_CAP_PROP_FOCUS, focus);
                calibStage++;
            }
            else
            {
                focus += static_cast<int>(cameraCap->GetCapturePropertyStep(CV_CAP_PROP_FOCUS));
            }
        }
        return image;
    }


    bool CameraCalibrate::inBound(int width, int height, double x, double y)
    {
        if (FSmaller(x, 0.0) || FSmaller(y, 0.0)
            || FSmaller((double)width, x) || FSmaller((double)height, y))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool CameraCalibrate::inSmallBound(int width, int height, double x, double y)
    {
        if (FSmaller(x, width * 0.1) || FSmaller(y, height * 0.1)
            || FSmaller(width * 0.9, x) || FSmaller(height * 0.9, y))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    cv::Mat CameraCalibrate::CalibrateDistance(const cv::Mat &frame, const double timeStamp)
    {
        if (pushing && (FSmaller(GetCurrentMillisecond() - lastPushTime, 2000.0)))
            return frame;
        else
            pushing = false;

        if (hit >= 3)
        {
            calibStage++;
            return frame;
        }

        if (timeStamp - firstTimeStamp > timeout)
        {
            calibFailed = true;
            calibStage++;

            DAVINCI_LOG_INFO << "Camera distance calibration timeout.";

            return frame;
        }

        homography = featureMatcher->Match(frame);

        if (homography.empty() == false)
        {
            if (outBoundCount < 30)
            {
                vector<Point2f> pts = featureMatcher->GetPts();
                if (!(inBound(frame.cols, frame.rows, pts[0].x, pts[0].y) 
                    && inBound(frame.cols, frame.rows, pts[1].x, pts[1].y) 
                    && inBound(frame.cols, frame.rows, pts[2].x, pts[2].y) 
                    && inBound(frame.cols, frame.rows, pts[3].x, pts[3].y)))
                {
                    DAVINCI_LOG_DEBUG << "phone screen out of bound, consider raise pusher";
                    boost::shared_ptr<HWAccessoryController> controller
                        = DeviceManager::Instance().GetCurrentHWAccessoryController();
                    if (controller != nullptr)
                    {
                        controller->HolderUp(5);
                        DAVINCI_LOG_DEBUG << "Holder up.";
                    }
                    outBoundCount++;
                    pushing = true;
                    lastPushTime = GetCurrentMillisecond();
                }
                if (inSmallBound(frame.cols, frame.rows, pts[0].x, pts[0].y) 
                    && inSmallBound(frame.cols, frame.rows, pts[1].x, pts[1].y) 
                    && inSmallBound(frame.cols, frame.rows, pts[2].x, pts[2].y) 
                    && inSmallBound(frame.cols, frame.rows, pts[3].x, pts[3].y))
                {
                    DAVINCI_LOG_DEBUG << "phone screen too small, consider drop pusher";
                    boost::shared_ptr<HWAccessoryController> controller
                        = DeviceManager::Instance().GetCurrentHWAccessoryController();
                    if ( controller != nullptr)
                    {
                        controller->HolderDown(5);
                        DAVINCI_LOG_DEBUG << "Holder down.";
                    }
                    outBoundCount++;
                    pushing = true;
                    lastPushTime = GetCurrentMillisecond();
                }
            }

            hit++;

            DAVINCI_LOG_INFO << "Camera distance calibration done. [" << hit << "]";
        }

        return frame;
    }

    vector<Mat> CameraCalibrate::CalculateHistogram(const cv::Mat &image, const int binSize)
    {
        vector<Mat> rgb;
        split(image, rgb);

        vector<Mat> histVector;
        int channels[] = {0};
        float range[] = {0, (float)binSize - 1};
        const float * ranges[] = { range };

        vector<Mat>::iterator iter = rgb.begin();
        while (iter != rgb.end())
        {
            Mat hist;
            calcHist(& (*iter), 1, channels, Mat(), hist, 1, &binSize, ranges);
            histVector.push_back(hist);
            iter++;
        }
        return histVector;
    }

    double CameraCalibrate::GetFrequencyOfBin(const Mat channel, const int histSize)
    {
        double freq = 0.0;
        for (int i = 1; i < histSize; i++)
        {
            freq += abs(channel.at<int>(i));
        }
        return freq;
    }

    double CameraCalibrate::ComputeShannonEntropy(const cv::Mat &image)
    {
        double entropy = 0.0;
        const int histSize = 256;
        vector<Mat> hist = CalculateHistogram(image, histSize);
        vector<Mat>::iterator iter = hist.begin();
        while (iter != hist.end())
        {
            double freq = GetFrequencyOfBin(*iter, histSize);
            for (int i = 1; i < histSize; i++)
            {
                double hc = abs((*iter).at<int>(i));
                if (FEquals(hc, 0.0) == false)
                {
                    entropy += -(hc / freq) * log10(hc / freq);
                }
            }
            iter++;
        }
        return entropy;
    }

    cv::Mat CameraCalibrate::GetROIForEntropy(cv::Mat &image)
    {
        int width = 128, height = 128;
        int x = image.cols / 2 - width / 2;
        int y = image.rows / 2 - height / 2;

        cv::Rect rectROI(x, y, width, height);
        rectangle(image, rectROI, Red);

        Mat imageClone(image, rectROI);

        return imageClone;
    }

    void CameraCalibrate::CalibrateByEntropy(const cv::Mat &frame, int prop, const std::string &name, int &bestValue)
    {
        if (isStartCalibParams == false)
        {
            done = false;

            paramMax = static_cast<int>(cameraCap->GetCapturePropertyMax(prop));
            paramMin = static_cast<int>(cameraCap->GetCapturePropertyMin(prop));
            paramStep = static_cast<int>(cameraCap->GetCapturePropertyStep(prop));

            oldValue = static_cast<int>(cameraCap->GetCaptureProperty(prop));
            oldEntropyValue = ComputeShannonEntropy(frame);
            // the same min and max value means, the property value cannot be changed 
            if (paramMax == paramMin)
            {
                done = true;
                bestValue = oldValue;
                maxEntropyValue = oldEntropyValue;
            }
            else
            {
                if (cameraCap->IsHighSpeedCamera())
                {
                    paramStep *= 10;
                }
                maxEntropyValue = -DBL_MAX;
                currentValue = paramMin;
                bestValue = currentValue;
                if (done == false)
                {
                    if (SetCaptureProperty(prop, currentValue) == false)
                    {
                        DAVINCI_LOG_ERROR << "Failed to set " << name << " property to " << currentValue << ".";
                        done = true;
                    }
                }
                isStartCalibParams = true;
            }

            DAVINCI_LOG_INFO << name << " property to be calibrated: minimum = " << paramMin << ", maximum = " << paramMax << ", step = " << paramStep;
        }
        else if (done == false)
        {
            if (SetCaptureProperty(prop, currentValue) == false)
            {
                DAVINCI_LOG_ERROR << "Failed to set " << name << " property to " << currentValue << ".";
                done = true;
            }
            else
            {
                double entropyValue = ComputeShannonEntropy(frame);
                DAVINCI_LOG_INFO << name << "=" << currentValue << ", entropy=" << entropyValue;

                if (FSmaller(maxEntropyValue, entropyValue))
                {
                    DAVINCI_LOG_DEBUG << "Best entropy value: " << entropyValue << ", " << name << ": " << currentValue;
                    maxEntropyValue = entropyValue;
                    bestValue = currentValue;
                }
                else
                {
                    double ratio = (maxEntropyValue - entropyValue) * 10000.0 / maxEntropyValue;
                    const double minRatio = 1000.0; // 10%
                    if (FSmaller(minRatio, ratio))
                    {
                        done = true;
                    }
                }

                if (currentValue >= paramMax || done == true)
                {
                    if (done == false)
                    {
                        done = true;
                    }

                    double errorRatio = 0.0;
                    const double minErrorRatio = 20.0; //0.02%
                    if (FEquals(oldEntropyValue, 0.0))
                    {
                        errorRatio = 0.0;
                    }
                    else
                    {
                        errorRatio = abs((maxEntropyValue - oldEntropyValue) * 10000.0 / oldEntropyValue);
                    }

                    if (FSmaller(oldEntropyValue, maxEntropyValue) || FSmaller(errorRatio, minErrorRatio))
                    {
                        currentValue = bestValue;
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << "Max entropy: " << maxEntropyValue << "(value=" << bestValue
                            << "), older=" << oldEntropyValue << "(value=" << oldValue << ")";
                        DAVINCI_LOG_INFO << "Error ratio = " << errorRatio;

                        if (bestValue != oldValue)
                        {
                            calibStage = 2;
                            currentValue = oldValue;
                            bestValue = oldValue;
                            return;
                        }
                    }
                }
                else
                {
                    currentValue += paramStep;
                    if (currentValue > paramMax)
                    {
                        done = true;
                    }
                }
            }
        }

        if (done)
        {
            DAVINCI_LOG_INFO << "Best " << name <<"=" << bestValue << ", entropy=" << maxEntropyValue;
            calibStage++;
            isStartCalibParams = false;
        }
    }

    cv::Mat CameraCalibrate::CalibrateCameraParameters(const cv::Mat &frame, double timeStamp)
    {
        if ((int)(timeStamp - lastTimeStamp) > intervalSetCamera)
        {
            cv::Mat image = frame;

            if (calibStage < 2)
            {
                image = GetROIForEntropy(image);
            }

            switch (calibStage)
            {
            case 0:
                CalibrateByEntropy(image, CV_CAP_PROP_GAIN, PhysicalCameraCapture::camCfgString[PhysicalCameraCapture::gainIndexInCamCfg], bestGainValue);
                break;
            case 1:
                if (cameraCap->IsHighSpeedCamera())
                {
                    DAVINCI_LOG_INFO << "Brightness in high speed camera is same as gain.";
                    calibStage++;
                }
                else
                {
                    CalibrateByEntropy(image, CV_CAP_PROP_BRIGHTNESS, 
                        PhysicalCameraCapture::camCfgString[PhysicalCameraCapture::brightnessIndexInCamCfg],
                        bestBrightnessValue);
                }
                break;
            case 2:
                /*
                calibrateByEntropy(image, CAP_PROP.CV_CAP_PROP_SATURATION, TestManager.camCfgString[2]);
                break;
                case 3:
                calibrateByEntropy(image, CAP_PROP.CV_CAP_PROP_CONTRAST, TestManager.camCfgString[1]);
                break;
                case 4:
                calibrateByEntropy(image, CAP_PROP.CV_CAP_PROP_EXPOSURE, TestManager.camCfgString[8]);
                break;
                case 5:
                calibrateByEntropy(image, CAP_PROP.CV_CAP_PROP_WHITE_BALANCE_BLUE_U, TestManager.camCfgString[0]);
                break;
                case 6:
                */
                if (calibFailed == false)
                {
                    DAVINCI_LOG_INFO << "Camera Parameter Calibration done.";
                    // image->Save(TestManager::getDavinciLogFilename(TestManager::DavinciLogType::Calibration, "frame-calib-camera-pass.png"));
                }
                else
                {
                    DAVINCI_LOG_INFO << "Calibration failed.";
                    // image->Save(TestManager::getDavinciLogFilename(TestManager::DavinciLogType::Calibration, "frame-calib-camera-fail.png"));
                }

                cameraCap->SaveCameraConfig(PhysicalCameraCapture::gainIndexInCamCfg, bestGainValue);
                cameraCap->SaveCameraConfig(PhysicalCameraCapture::brightnessIndexInCamCfg, bestBrightnessValue);

                calibStage++;
                SetFinished();
                break;
            }

            lastTimeStamp = timeStamp;
        }

        return frame;
    }

    cv::Mat CameraCalibrate::CalibrateOverall(const cv::Mat &frame, double timeStamp)
    {
        switch (calibStage)
        {
        case 0:
        case 1:
            return CalibrateCameraParameters(frame, timeStamp);
        case 2:
            if (calibFailed == true)
            {
                DAVINCI_LOG_INFO << "Calibration failed.";
                // frame->Save(TestManager::getDavinciLogFilename(TestManager::DavinciLogType::Calibration, "frame-calib-camera-fail.png"));
                calibStage = calibDoneStage;
            }
            else
            {
                // frame->Save(TestManager::getDavinciLogFilename(TestManager::DavinciLogType::Calibration, "frame-calib-camera-pass.png"));

                // TODO: save best gain and brightness value
                cameraCap->SaveCameraConfig(PhysicalCameraCapture::gainIndexInCamCfg, bestGainValue);
                cameraCap->SaveCameraConfig(PhysicalCameraCapture::brightnessIndexInCamCfg, bestBrightnessValue);

                calibStage++;
            }
            break;
        case 3:
            if (cameraCap->IsHighSpeedCamera())
            {
                DAVINCI_LOG_INFO << "Skip focus calibration as it's not supported by high speed camera!";
                calibStage++;
                break;
            }
            else
            {
                return CalibrateFocus(frame, timeStamp);
            }
        case 4:
            if (calibFailed == true)
            {
                calibStage = calibDoneStage;
            }
            else
            {
                firstTimeStamp = timeStamp;
                calibStage++;
                cameraCap->SetCalibrateHomography(Mat(), CaptureDevice::Preset::HighResolution, 0, 0);
            }
            break;
        case 5:
            return CalibratePerspective(frame, timeStamp);
        case 6:
            if (calibFailed == true)
            {
                calibStage = calibDoneStage;
            }
            else
            {
                InitFocusCalibration();
                calibStage++;
            }
            break;
        case 7:
            if (cameraCap->IsHighSpeedCamera())
            {
                DAVINCI_LOG_INFO << "Skip focus calibration as it's not supported by high speed camera!";
                calibStage++;
                break;
            }
            else
            {
                return CalibrateFocus(frame, timeStamp);
            }
        case calibDoneStage:
            if (calibFailed == true)
            {
                DAVINCI_LOG_INFO << "Calibration failed.";
            }
            else
            {
                DAVINCI_LOG_INFO << "Overall Calibration Done.";
            }
            calibStage++;
            SetFinished();
            break;
        }
        return frame;
    }

    void CameraCalibrate::ComputeCalibratePreset()
    {
        cameraCap = boost::dynamic_pointer_cast<PhysicalCameraCapture>(
            DeviceManager::Instance().GetCurrentCaptureDevice());
        if (cameraCap == nullptr || cameraCap->IsHighSpeedCamera() == false)
        {
            if (calibType == CameraCalibrationType::HighResolutionCalibrate
                || calibType == CameraCalibrationType::OverallCalibrate
                || calibType == CameraCalibrationType::FocusCalibrate
                || calibType == CameraCalibrationType::CameraParamCalibrate)
            {
                calibPreset = CaptureDevice::Preset::HighResolution;
            }
            else if (calibType == CameraCalibrationType::HighSpeedCalibrate
                || calibType == CameraCalibrationType::DistanceCalibrate)
            {
                calibPreset = CaptureDevice::Preset::HighSpeed;
            }
        }
    }

    CaptureDevice::Preset CameraCalibrate::CameraPreset()
    {
        if (calibType == CameraCalibrationType::ResetCalibrate)
        {
            return CaptureDevice::Preset::PresetDontCare;
        }
        else 
        {
            ComputeCalibratePreset();
            return calibPreset;
        }
    }

    void CameraCalibrate::SetCalibrateStage(int calibStageValue)
    {
        calibStage = calibStageValue;
    }

    int CameraCalibrate::GetCalibrateStage()
    {
        return calibStage;
    }

    void CameraCalibrate::SetCalibrateResult(bool calibResult)
    {
        calibFailed = calibResult;  // true for calibFailed
    }

    void CameraCalibrate::SetCalibratePushing(bool pushingValue)
    {
        pushing = pushingValue;
    }

    void CameraCalibrate::SetCalibrateLastPushingTime(double timeValue)
    {
        lastPushTime = timeValue;
    }

    void CameraCalibrate::SetCalibrateHitValue(int hitValue)
    {
        hit = hitValue;
    }
}
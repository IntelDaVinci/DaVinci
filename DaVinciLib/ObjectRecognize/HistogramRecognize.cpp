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

#include "opencv/cv.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "HistogramRecognize.hpp"

using namespace std;
using namespace cv;

namespace DaVinci
{
    HistogramRecognize::HistogramRecognize()
    {
    }

    HistogramRecognize::HistogramRecognize(const cv::Mat &s)
    {
        this->modelImage = s;
    }

    double HistogramRecognize::calVValue(const cv::Mat &image)
    {
        cv::Mat hsvImage;
        cvtColor(image, hsvImage, CV_BGR2HSV);

        cv::Mat hChannel[3];
        split(hsvImage, hChannel);

        float range[] = {0, 256};

        int histSize = 256;
        const float* histRange = { range };

        bool uniform = true; bool accumulate = false;

        Mat hist;

        calcHist( &hChannel[2], 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );

        double sum = 0.0;
        for (int i = 0; i < 256; i++)
        {
            sum += hist.at<float>(i);
        }

        return sum / 256.0;
    }

    double HistogramRecognize::calHValue(const cv::Mat &image, int key, int interval)
    {
        cv::Mat hsvImage;
        cvtColor(image, hsvImage, CV_BGR2HSV);

        cv::Mat hChannel[3];
        split(hsvImage, hChannel);

        float range[] = {0, 256};

        int histSize = 256;
        const float* histRange = { range };

        bool uniform = true; bool accumulate = false;

        Mat hist;
        calcHist(&hChannel[0], 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );

        double sum = 0.0;
        for(int i = key - interval; i < key + interval; i++)
        {
            sum += hist.at<float>(i);
        }

        double maxPercentage = sum / (image.rows * image.cols);

        return maxPercentage;
    }

    int HistogramRecognize::calMaxHValue(const cv::Mat &image)
    {
        cv::Mat hsvImage;
        cvtColor(image, hsvImage, CV_BGR2HSV);

        cv::Mat hChannel[3];
        split(hsvImage, hChannel);

        float range[] = {0, 256};

        int histSize = 256;
        const float* histRange = { range };

        bool uniform = true; bool accumulate = false;

        Mat hist;

        calcHist( &hChannel[0], 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );
        double maxVal=0;
        minMaxLoc(hist, 0, &maxVal, 0, 0);

        double max = 0.0;
        int maxIndex = -1;
        for (int i = 0; i < 255; i++)
        {
            double binVal = hist.at<float>(i);
            if (FSmallerE(max, binVal))
            {
                max = binVal;
                maxIndex = i;
            }
        }

        return maxIndex;
    }

    double HistogramRecognize::GetSimilarity(const cv::Mat &observedImage, const cv::Mat &modelImage)
    {
        float h_ranges[] = { 0, 180 };
        float s_ranges[] = { 0, 256 };
        float v_ranges[] = { 0, 256 };
        const float* ranges[] = { h_ranges, s_ranges, v_ranges };
        int channels[] = { 0, 1, 2 };
        int histSize[] = { 180, 256, 256 };

        vector<Mat> ohists;
        Mat hist, hsv;
        cvtColor(observedImage, hsv, COLOR_BGR2HSV);
        for (size_t i = 0; i < 3; i++)
        {
            calcHist(&hsv, 1, channels + i, Mat(), hist, 1, histSize + i, ranges + i, true, false );
            ohists.push_back(hist);
        }

        vector<Mat> mhists;
        cvtColor(modelImage, hsv, COLOR_BGR2HSV);
        for (size_t i = 0; i < 3; i++)
        {
            calcHist(&hsv, 1, channels + i, Mat(), hist, 1, histSize + i, ranges + i, true, false );
            mhists.push_back(hist);
        }

        double diff = 1;
        assert(ohists.size() == mhists.size());
        for (size_t i = 0; i < ohists.size(); i++)
        {
            diff *= compareHist(ohists[i], mhists[i], CV_COMP_CORREL);
        }
        return diff;
    }

    /// <summary>
    /// Detects the specified observed image.
    /// </summary>
    /// <param name="observedImage">The observed image.</param>
    /// <param name="threshold">The threshold.</param>
    /// <returns></returns>
    bool HistogramRecognize::Detect(const cv::Mat &observedImage, float threshold)
    {
        double similarity = GetSimilarity(observedImage, this->modelImage);

        bool matched = false;

        if (similarity > threshold)
        {
            matched = true;
        }

        return matched;
    }

    bool HistogramRecognize::findMatch(const cv::Mat &refImage, const cv::Mat &obsImage, Rect obsROI, Rect &matched, int wstep, int hstep, float threshold)
    {
        Rect rect;
        Rect roi;
        Size size = refImage.size();
        int width = size.width, height = size.height;
        double similarity = 0;

        if (obsROI.width < width || obsROI.height < height)
        {
            matched = obsROI;
            return false;
        }

        for (int i = obsROI.x; i < obsROI.width - width + 1; i += wstep)
        {
            for (int j = obsROI.y; j < obsROI.height - height + 1; j += hstep)
            {
                roi = Rect(i, j, width, height);
                Mat roiImage = obsImage(roi);

                similarity = GetSimilarity(refImage, roiImage);

                if (similarity > threshold)
                {
                    matched = roi;
                    return true;
                }
            }
        }

        return false;
    }
}

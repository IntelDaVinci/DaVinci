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

#ifndef __HISTOGRAMRECOGNIZE__
#define __HISTOGRAMRECOGNIZE__

#include "opencv2/opencv.hpp"
#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    using namespace std;
using namespace cv;
    /// <summary>
    /// Class for histogram matching
    /// </summary>
    class HistogramRecognize
    {
    private:
        cv::Mat modelImage;

        /// <summary>
        /// Default construct
        /// </summary>
    public:
        HistogramRecognize();

        /// <summary>
        /// Construct for histogram recognize
        /// </summary>
        /// <param name="s"></param>
        HistogramRecognize(const cv::Mat &s);

        /// <summary>
        /// Calculate V value
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        double calVValue(const cv::Mat &image);

        /// <summary>
        /// Calculate min and max value
        /// </summary>
        /// <param name="image"></param>
        /// <param name="key"></param>
        /// <param name="interval"></param>
        double calHValue(const cv::Mat &image, int key = 35, int interval = 3);

        /// <summary>
        /// Calculate max value
        /// </summary>
        /// <param name="image"></param>
        int calMaxHValue(const cv::Mat &image);

        /// <summary>
        /// Detect the image similarity
        /// </summary>
        /// <param name="observedImage"></param>
        /// <param name="threshold"></param>
        /// <returns></returns>
        bool Detect(const cv::Mat &observedImage, float threshold = 0.8F);

        /// <summary>
        /// Find matched image
        /// </summary>
        /// <param name="refImage"></param>
        /// <param name="obsImage"></param>
        /// <param name="obsROI"></param>
        /// <param name="matched"></param>
        /// <param name="wstep"></param>
        /// <param name="hstep"></param>
        /// <param name="threshold"></param>
        /// <returns></returns>
        bool findMatch(const cv::Mat &refImage, const cv::Mat &obsImage, Rect obsROI, Rect &matched, int wstep = 10, int hstep = 10, float threshold = 0.8F);

        /// <summary>
        /// Equalize the histogram
        /// </summary>
        /// <param name="observedImage"></param>
        /// <returns></returns>
        cv::Mat Equalize(const cv::Mat &observedImage);

        double GetSimilarity(const cv::Mat &observedImage, const cv::Mat &sourceImage);
    };
}


#endif	//#ifndef __HISTOGRAMRECOGNIZE__

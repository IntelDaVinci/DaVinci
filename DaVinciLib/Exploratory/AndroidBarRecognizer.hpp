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

#ifndef __ANDROIDBARRECOGNIZER__
#define __ANDROIDBARRECOGNIZER__

#include "boost/smart_ptr/shared_ptr.hpp"
#include "opencv2/opencv.hpp"

#include "TargetDevice.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum NavigationBarLocation
    {
        BarLocationBottom,
        BarLocationRight,
        BarLocationTop,
        BarLocationLeft,
        BarLocationUnknown
    };

    /// <summary>
    /// Class SmallObjectRecognizer
    /// </summary>
    class AndroidBarRecognizer
    {
    private:
        int deltaBarHeight;
        cv::Rect statusBarRect;
        cv::Rect navigationBarRect;
        NavigationBarLocation navigationBarLocation;
        std::vector<cv::Rect> threeBarButtons;

        /// <summary>
        /// Construct
        /// </summary>
    public:
        AndroidBarRecognizer();

        cv::Rect GetNavigationBarRect();

        NavigationBarLocation GetNavigationBarLocation();

        // X 50, Y 50, distance 75
        int FindButtonFromPoint(Point p, double allowDistance = 75);

        bool RecognizeNavigationBar(const cv::Mat &frame, int barHeight = 0);

        bool RecognizeNavigationBarWithBarInfo(const cv::Mat &frame, Rect barRect);

        std::vector<cv::Rect> RecognizeThreeBarButtons(const cv::Mat &barFrame);

        std::vector<cv::Rect> FindThreeBarButtons(const cv::Mat &frame, int lowRectArea, int highRectArea, int lowRectInterval, int highRectInterval);

        /// <summary>
        /// Recognize setting button location
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="settingButton"></param>
        /// <returns></returns>
        bool RecognizeSettingButton(const cv::Mat &frame, Rect &settingButton);

        /// <summary>
        /// Recognize bar rect location
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="settingRect"></param>
        /// <param name="statusBarRect"></param>
        /// <param name="actionBarRect"></param>
        /// <returns></returns>
        bool RecognizeBarRectangle(const cv::Mat &frame, Rect &settingRect, Rect &statusBarRect, Rect &actionBarRect);

        int DetectBottomBarBoundary(const cv::Mat &frame, double minLength, int step = 3);

        size_t FindCenteredRect(const std::vector<cv::Rect> &rects, int frameWidth, int delta = 2);
    };
}


#endif	//#ifndef __ANDROIDBARRECOGNIZER__

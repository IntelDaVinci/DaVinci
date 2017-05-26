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

#ifndef __OBJECTUTIL__
#define __OBJECTUTIL__


#include <vector>
#include <map>

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/process.hpp"
#include "boost/thread/thread.hpp"
#include "boost/chrono/chrono.hpp"
#include "boost/functional/hash/hash.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "DaVinciStatus.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum NavigationBarLocation;

    class AndroidBarRecognizer;

    /// <summary>
    /// Class ObjectUtil
    /// </summary>
    class ObjectUtil : public SingletonBase<ObjectUtil>
    {
        SIGLETON_CHILD_CLASS(ObjectUtil);
    private:
        /// <summary>
        /// Navigation Bar Rect
        /// </summary>
        Rect navigationBarRect;

        /// <summary>
        /// Status Bar Rect
        /// </summary>
        Rect statusBarRect;

        static const int maxNavigationBarHeight = 100;
        static const int maxStatusBarHeight = 100;

    public:
        /// <summary>
        /// Status Bar Height
        /// </summary>
        int statusBarHeight;

        /// <summary>
        /// Action Bar Height
        /// </summary>
        int actionBarHeight;

        /// <summary>
        /// Navigation Bar Height
        /// </summary>
        int navigationBarHeight;

        /// <summary>
        /// Device Height
        /// </summary>
        int deviceHeight;

        /// <summary>
        /// Device Width
        /// </summary>
        int deviceWidth;

        string debugPrefixDir;

        boost::shared_ptr<TargetDevice> dut;

    public:
        cv::Rect CutNavigationBarROI(const cv::Size &frameSize, const cv::Rect &barRect, NavigationBarLocation barLocation);

        cv::Rect CutStatusBarROI(const cv::Size &frameSize, int barHeight, Orientation orientation);

        cv::Rect GetNavigationBarRect()
        {
            return this->navigationBarRect;
        }

        cv::Rect GetStatusBarRect()
        {
            return this->statusBarRect;
        }

        cv::Rect GetKeyboardROI(Orientation o,  const cv::Size &deviceSize, const cv::Rect &nb, const cv::Rect &sb, const cv::Rect &fw);

        cv::Rect GetRemainROI(Orientation o,  const cv::Size &deviceSize, const cv::Rect &nb, const cv::Rect &sb, const cv::Rect &fw);

        bool IsPopUpDialog(const Size deviceSize, const Orientation orientation, const cv::Rect &sb, const cv::Rect &fw);

        string GetSystemLanguage();

        /// <summary>
        /// Scale current size to a new size and keep original scale
        /// Pattern one: current size doesn't keep scale, then scale it to original scale size
        /// Pattern two: current size keeps scale, then scale it to reference size
        /// If bar is at bottom or top, then new height known, compute new width; if bar is on right or left, then new width known, compute new height
        /// </summary>
        /// <param name="currentSize"></param>
        /// <param name="originalScaledSize"></param>
        /// <param name="referenceSize"></param>
        /// <param name="barLocation"></param>
        /// <returns></returns>
        cv::Size ScaleFrameSizeWithCroppingNavigationBar(const cv::Size &currentSize, const cv::Size &originalScaledSize, const cv::Size &referenceSize, NavigationBarLocation barLocation);

        /// <summary>
        /// Count non-zero point after canny
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        int CountCannyNonZeroPoint(const cv::Mat &image);

        /// <summary>
        /// Check single color, e.g., black, blue ...
        /// </summary>
        /// <param name="frame"></param>
        /// <returns></returns>
        bool IsPureSingleColor(const cv::Mat &frame, const Rect navigationBarRect, const Rect statusBarRect, bool removeActionBar = true);

        /// <summary>
        /// Check whether it is bad image
        /// </summary>
        /// <param name="currentFrame"></param>
        /// <param name="firstFrame"></param>
        /// <param name="secondFrame"></param>
        /// <returns></returns>
        bool IsBadImage(const cv::Mat &currentFrame, const cv::Mat &firstFrame, const cv::Mat &secondFrame);

        /// <summary>
        /// Hash rectangle by different hash types
        /// </summary>
        /// <param name="rects">rects</param>
        /// <param name="hashType">hash type</param>
        /// <param name="approximateKeyInterval">approximate key interval, if 0, then key must be precise; if not, then approximate key is okay</param>
        /// <returns>multimap</returns>
        std::map<int, std::vector<cv::Rect>> HashRectangles(const std::vector<cv::Rect> &rects, UType hashType, int approximateKeyInterval);

        /// <summary>
        /// Hash rectangle by different hash types with interval([low, high]) one after another
        /// </summary>
        /// <param name="rects">rects</param>
        /// <param name="hashType">hash type</param>
        /// <param name="approximateKeyInterval">approximate key interval, if 0, then key must be precise; if not, then approximate key is okay</param>
        /// <param name="lowRectInterval">low rect interval</param>
        /// <param name="highRectInterval">high rect interval</param>
        /// <param name="isHorizontalInterval">true for horizontal interval; false for vertical interval</param>
        /// <returns>multimap</returns>
        std::multimap<int, std::vector<cv::Rect>> HashRectanglesWithRectInterval(std::vector<cv::Rect> &rects, UType hashType, int approximateKeyInterval, int lowRectInterval, int highRectInterval, bool isHorizontalInterval = true);

        /// <summary>
        /// Merge rectangles to a big one
        /// </summary>
        /// <param name="rectList"></param>
        /// <returns></returns>
        Rect MergeRectangles(std::vector<Rect> &rectList);

        /// <summary>
        /// Relocate rectangles based on target rectangle
        /// </summary>
        /// <param name="size"></param>
        /// <param name="rectList"></param>
        /// <param name="targetRect"></param>
        /// <returns></returns>
        std::vector<Rect> RelocateRectangleList(Size size, std::vector<Rect> &rectList, Rect targetRect);

        /// <summary>
        /// Relocate rectangle based on target rectangle
        /// </summary>
        /// <param name="size"></param>
        /// <param name="rect"></param>
        /// <param name="targetRect"></param>
        /// <returns></returns>
        Rect RelocateRectangle(Size size, Rect rect, Rect targetRect);

        Point RelocatePointByRatio(const Point &sourcePoint, Size sourceSize, Size targetSize);

        Point RelocatePointByOffset(const Point &sourcePoint, Rect rect, bool isSmallToBig);

        /// <summary>
        /// Rotate rectangle anticlockwise along with frame
        /// </summary>
        /// <param name="rect"></param>
        /// <param name="frameSize">frame size before rotating</param>
        /// <param name="rotateAngle"></param>
        /// <returns></returns>
        cv::Rect RotateRectangleAnticlockwise(const cv::Rect &rect, cv::Size frameSize, int rotateAngle);

        /// <summary>
        /// Detect objects by CCL
        /// </summary>
        /// <param name="image"></param>
        /// <param name="threshold"></param>
        /// <returns></returns>
        std::vector<Rect> DetectCCLRectangles(const cv::Mat &image, int threshold = 3);

        /// <summary>
        /// Collect valid rectangle by area and ROI
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="size"></param>
        /// <param name="minArea"></param>
        /// <param name="maxArea"></param>
        /// <param name="ltX"></param>
        /// <param name="ltY"></param>
        /// <param name="rdX"></param>
        /// <param name="rdY"></param>
        /// <param name="edgeDistance"></param>
        /// <param name="isContour"></param>
        /// <returns></returns>
        std::vector<Rect> CollectValidRectanglesByAreaAndROI(std::vector<Rect> &rectList, Size size, int minArea, int maxArea, int ltX, int ltY, int rdX, int rdY, int edgeDistance, bool isContour = false);

        /// <summary>
        /// Get threshold automatically by Da-Jin algorithm(only for images which has two kinds of color information )
        /// </summary>
        /// <param name="grayImg"></param>
        /// <returns></returns>
        int DetectBinaryThreshold(const cv::Mat &grayImg);

        Rect ConstructRectangleFromTwoLines(Size size, LineSegment line1, LineSegment line2, bool horizontalLine);

        bool IsValidRectangle(Rect rect);

        void CopyROIFrame(Mat frame, Rect roi, Mat& roiFrame);

        /// <summary>
        /// Split big rectangle to small rectangles by dimension (3 * 3)
        /// </summary>
        /// <param name="rect"></param>
        /// <param name="dimension"></param>
        /// <returns></returns>
        std::vector<std::vector<Rect>> SplitRectangleByDimension(Rect rect, int dimension = 3);

        /// <summary>
        /// Split big rectangle to small rectangles by number
        /// </summary>
        /// <param name="rect"></param>
        /// <param name="horizontal"></param>
        /// <param name="number"></param>
        /// <returns></returns>
        std::vector<Rect> SplitRectangleByNumber(Rect rect, bool horizontal = true, int number = 2);

        /// <summary>
        /// Check targetRect is contained by any rect in the rectangle list
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="targetRect"></param>
        /// <param name="outRect"></param>
        /// <returns></returns>
        bool IsContainedRectangle(std::vector<Rect> &rectList, Rect &targetRect, Rect &outRect);

        std::vector<cv::Rect> DetectRectsByContour(const cv::Mat &frame, int cannyThreshold, int xKernelSize, int yKernelSize, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea, int retrievalMode = CV_RETR_LIST);

        /// <summary>
        /// Detect real rectangles by contour
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="lowSideRatio"></param>
        /// <param name="highSideRatio"></param>
        /// <param name="lowRectArea"></param>
        /// <param name="highRectArea"></param>
        /// <param name="cannyThreshold"></param>
        /// <param name="ksize"></param>
        /// <param name="retrievalMode"></param>
        /// <returns></returns>
        std::vector<cv::Rect> DetectRealRectsByContour(const cv::Mat &frame, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea, int cannyThreshold = 50, cv::Size ksize = cv::Size(3, 3), int retrievalMode = CV_RETR_LIST);

        /// <summary>
        /// Get rectangles within target rectangles which meet the interval condition with source rectangle
        /// </summary>
        /// <param name="sourceRect">source rectangle</param>
        /// <param name="targetRects">target rectangles</param>
        /// <param name="rectInterval">interval between two rectangles</param>
        /// <param name="isUpward">true for looking for rectangle upwards</param>
        /// <returns>rectangles</returns>
        std::vector<cv::Rect> GetRectsWithInterval(const cv::Rect &sourceRect, std::vector<cv::Rect> &targetRects, int rectInterval, bool isUpward);

        /// <summary>
        /// Binarize frame with low and high binary threshold: if auto detected binary threshold is less than 100, use detected binary threshold; if >= 100 && < 180, use low threshold; otherwise, use high threshold
        /// 170 and 225 are fine-tuned default value
        /// </summary>
        /// <param name="bgrImage">bgr frame</param>
        /// <param name="lowThreshold">low binary threshold</param>
        /// <param name="highThreshold">high binary threshold</param>
        /// <returns>Binarized frame</returns>
        cv::Mat BinarizeWithChangedThreshold(const cv::Mat bgrImage, int lowThreshold = 170, int highThreshold = 225);

        /// <summary>
        /// Check wheter image is changed or not
        /// </summary>
        /// <param name="preImage">previous image</param>
        /// <param name="currentImage">current image</param>
        /// <param name="ratio">larger then the ratio, then changed</param>
        /// <returns>true for changed</returns>
        bool IsImageChanged(const cv::Mat &preImage, const cv::Mat &currentImage, double ratio = 0.025);

        bool IsRectangleValidWithROI(Rect rect, Size size, int ltX, int ltY, int rdX, int rdY, int edgeDistance, bool isContour);

        bool IsRectangleValidWithArea(Rect rect, int minArea, int maxArea);

        vector<Rect> RemoveOuterRects(vector<Rect> rects);

        vector<Rect> RemoveContainedRects(const vector<Rect>& rects);

        vector<Rect> RemoveSimilarRects(const vector<Rect>& rects, int interval);

        vector<Rect> RectsContainPoint(vector<Rect> rects, Point point);

        bool IsImageSolidColored(const cv::Mat &image);

        bool IsImageSolidColoredHis(const cv::Mat& image);

        bool IsGrayImageSolid(const cv::Mat& grayImage, double threshold= 0.9, int delta = 5);

        cv::Rect ConstructRectangleFromCenterPoint(const cv::Point &centerPoint, const cv::Size &frameSize, int rectWidth, int rectHeight);

        cv::Rect ConstructTextRectangle(const cv::Point &centerPoint, const cv::Size &frameSize, int rectWidth, int rectHeight);

        cv::Rect ScaleRectangleWithLeftTopPoint(const cv::Rect &rect, const cv::Size &frameSize, int ltPointXOffset, int ltPointYOffset, bool isExtended = true);

        std::vector<Rect> FilterRects(const std::vector<cv::Rect> &rects, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea);

        void ReadCurrentBarInfo(int frameWidth = 1280);

        NavigationBarLocation FindBarLocation(Size frameSize, Rect nb);
    };
}


#endif	//#ifndef __OBJECTUTIL__

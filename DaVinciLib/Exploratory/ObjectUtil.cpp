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

#include "DaVinciCommon.hpp"
#include "TestManager.hpp"
#include "DeviceManager.hpp"
#include "AndroidTargetDevice.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "AndroidBarRecognizer.hpp"
#include "ObjectCommon.hpp"
#include "KeywordRecognizer.hpp"

using namespace cv;

namespace DaVinci
{
    ObjectUtil::ObjectUtil()
    {
        statusBarHeight = 50;
        actionBarHeight = 112;
        navigationBarHeight = 96;
        deviceHeight = 1920;
        deviceWidth = 1280;
        debugPrefixDir = ".\\";
        navigationBarRect = EmptyRect;
        statusBarRect = EmptyRect;
    }

    void ObjectUtil::ReadCurrentBarInfo(int frameWidth)
    {
        dut = DeviceManager::Instance().GetCurrentTargetDevice();

        if (dut != nullptr)
        {
            boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
            if (androidDut != nullptr)
            {
                Orientation orientation = androidDut->GetCurrentOrientation(true);
                DAVINCI_LOG_INFO << "ReadCurrentBarInfo:orientation is " << (int)orientation;
                Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());
                std::vector<std::string> windowLines;
                AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDut->GetWindowBarInfo(windowLines);
                // Transform bar rect from device to frame
                Point nbPoint(windowBarInfo.navigationbarRect.x, windowBarInfo.navigationbarRect.y);
                this->navigationBarRect = AndroidTargetDevice::GetWindowRectangle(
                    nbPoint, 
                    windowBarInfo.navigationbarRect.width, 
                    windowBarInfo.navigationbarRect.height, 
                    deviceSize, 
                    orientation,
                    frameWidth);
                Point sbPoint(windowBarInfo.statusbarRect.x, windowBarInfo.statusbarRect.y);
                this->statusBarRect = AndroidTargetDevice::GetWindowRectangle(
                    sbPoint, 
                    windowBarInfo.statusbarRect.width, 
                    windowBarInfo.statusbarRect.height, 
                    deviceSize, 
                    orientation,
                    frameWidth);

                if (min(this->navigationBarRect.height, this->navigationBarRect.width) > maxNavigationBarHeight)
                {
                    DAVINCI_LOG_WARNING << "ReadCurrentBarInfo Invalid navigation bar: x: " << navigationBarRect.x << " y: " << navigationBarRect.y << " width: " << navigationBarRect.width << " height: " << navigationBarRect.height;
                    this->navigationBarRect = EmptyRect;
                }
                if (min(this->statusBarRect.height, this->statusBarRect.width) > maxStatusBarHeight)
                {
                    DAVINCI_LOG_WARNING << "ReadCurrentBarInfo Invalid status bar: x: " << statusBarRect.x << " y: " << statusBarRect.y << " width: " << statusBarRect.width << " height: " << statusBarRect.height;
                    this->statusBarRect = EmptyRect;
                }

                this->navigationBarHeight = this->navigationBarRect.height;
                this->statusBarHeight = this->statusBarRect.height;
            }
        }
    }

    cv::Rect ObjectUtil::CutNavigationBarROI(const cv::Size &frameSize, const cv::Rect &barRect, NavigationBarLocation barLocation)
    {
        cv::Rect roi(EmptyPoint, frameSize);

        switch(barLocation)
        {
        case NavigationBarLocation::BarLocationBottom:
            roi = cv::Rect(0, 0, frameSize.width, frameSize.height - barRect.height);
            break;
        case NavigationBarLocation::BarLocationRight:
            roi = cv::Rect(0, 0, frameSize.width - barRect.width, frameSize.height);
            break;
        case NavigationBarLocation::BarLocationTop:
            roi = cv::Rect(0, barRect.height, frameSize.width, frameSize.height - barRect.height);
            break;
        case NavigationBarLocation::BarLocationLeft:
            roi = cv::Rect(barRect.width, 0, frameSize.width - barRect.width, frameSize.height);
            break;
        default:
            break;
        }

        return roi;
    }

    cv::Rect ObjectUtil::CutStatusBarROI(const cv::Size &frameSize, int barHeight, Orientation orientation)
    {
        cv::Rect roi(EmptyPoint, frameSize);

        switch(orientation)
        {
        case Orientation::Portrait:
            roi = cv::Rect(barHeight, 0, frameSize.width - barHeight, frameSize.height);
            break;
        case Orientation::Landscape:
            roi = cv::Rect(0, barHeight, frameSize.width, frameSize.height - barHeight);
            break;
        case Orientation::ReversePortrait:
            roi = cv::Rect(0, 0, frameSize.width - barHeight, frameSize.height);
            break;
        case Orientation::ReverseLandscape:
            roi = cv::Rect(0, 0, frameSize.width, frameSize.height - barHeight);
            break;
        default:
            break;
        }

        return roi;
    }

    cv::Rect ObjectUtil::GetKeyboardROI(Orientation o, const cv::Size &deviceSize, const cv::Rect &nb, const cv::Rect &sb, const cv::Rect &fw)
    {
        Rect kbRect = EmptyRect;
        switch(o)
        {
        case Orientation::Portrait:
        case Orientation::ReversePortrait:
            {
                kbRect = Rect(fw.x, fw.y + fw.height, fw.width, deviceSize.height - nb.height - fw.height - fw.y);
                break;
            }
        case Orientation::Landscape:
        case Orientation::ReverseLandscape:
            {
                if(nb.x > deviceSize.width  / 2)
                    kbRect = Rect(fw.x, fw.y + fw.height, fw.width, deviceSize.width - fw.height - fw.y);
                else
                    kbRect = Rect(fw.x, fw.y + fw.height, fw.width, deviceSize.width - nb.height - fw.height - fw.y);
                break;
            }
        default:
            {
                kbRect = Rect(fw.x, fw.y + fw.height, fw.width, deviceSize.height - nb.height - fw.height - fw.y);
                break;
            }
        }

        if(kbRect.height == 0)
            kbRect = EmptyRect;

        return kbRect;
    }

    bool ObjectUtil::IsPopUpDialog(const Size deviceSize, const Orientation orientation, const cv::Rect &sb, const cv::Rect &fw)
    {
        Point centerPoint(deviceSize.width / 2, deviceSize.height / 2);
        if(orientation == Orientation::Landscape || orientation == Orientation::ReverseLandscape)
        {
            centerPoint = Point(deviceSize.height / 2, deviceSize.width / 2);
        }

        // Pop up dialogs are sometimes very small icons (e.g., for tutorial page)
        // We assume pop up dialogs should contain the device center point
        if(!fw.contains(centerPoint))
            return false;

        // Workaround for some apps (e.g., com.qianwang.qianbao) on Zenfone2
        // There is a layout bar between statusbar and focus window, which treats as pop up previously
        const int allowHeightRatio = 2;

        // FIXME: fw.x > 0 is also pop up
        if(fw.y > sb.y + sb.height * allowHeightRatio || fw.x > 0)
            return true;
        else
            return false;
    }

    string ObjectUtil::GetSystemLanguage()
    {
        auto currentDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (currentDevice == nullptr)
        {
            DAVINCI_LOG_INFO << "ObjectUtil::GetSystemLanguage() GetCurrentTargetDevice return null.";
            return "";
        }

        return currentDevice->GetLanguage();
    }

    cv::Size ObjectUtil::ScaleFrameSizeWithCroppingNavigationBar(const cv::Size &currentSize, const cv::Size &originalScaledSize, const cv::Size &referenceSize, NavigationBarLocation barLocation)
    {
        cv::Size scaledSize = originalScaledSize;
        if (barLocation == NavigationBarLocation::BarLocationBottom || barLocation == NavigationBarLocation::BarLocationTop)
        {
            if (referenceSize.width == 0 && referenceSize.height == 0)
            {
                scaledSize = cv::Size(currentSize.height * originalScaledSize.width / originalScaledSize.height, currentSize.height);
            }
            else
            {
                scaledSize = cv::Size(referenceSize.height * originalScaledSize.width / originalScaledSize.height, referenceSize.height);
            }
        }
        else if (barLocation == NavigationBarLocation::BarLocationRight || barLocation == NavigationBarLocation::BarLocationLeft)
        {
            if (referenceSize.width == 0 && referenceSize.height == 0)
            {
                scaledSize = cv::Size(currentSize.width, currentSize.width * originalScaledSize.height / originalScaledSize.width);
            }
            else
            {
                scaledSize = cv::Size(referenceSize.width, referenceSize.width * originalScaledSize.height / originalScaledSize.width);
            }
        }

        return scaledSize;
    }

    NavigationBarLocation ObjectUtil::FindBarLocation(Size frameSize, Rect nb)
    {
        if(nb.width == 0 || nb.height == 0)
            return NavigationBarLocation::BarLocationUnknown;

        if(nb.x > frameSize.width / 2)
            return NavigationBarLocation::BarLocationRight;

        if(nb.y > frameSize.height / 2)
            return NavigationBarLocation::BarLocationBottom;

        if(nb.x + nb.width < frameSize.width / 2)
            return NavigationBarLocation::BarLocationLeft;

        if(nb.y + nb.height < frameSize.height / 2)
            return NavigationBarLocation::BarLocationTop;

        return NavigationBarLocation::BarLocationUnknown;
    }

    int ObjectUtil::CountCannyNonZeroPoint(const cv::Mat &image)
    {
        int size = 0;

        cv::Mat grayImage;
        if(image.channels() == 3)
            cvtColor(image, grayImage, CV_BGR2GRAY);
        else
            grayImage = image;

        cv::Mat cannyImage;
        cv::Canny(grayImage, cannyImage, 20, 10);

#ifdef _DEBUG

        imwrite("canny.png", cannyImage);
#endif
        size = countNonZero(cannyImage);

        return size;
    }

    bool ObjectUtil::IsPureSingleColor(const cv::Mat &frame, const Rect navigationBarRect, const Rect statusBarRect, bool removeActionBar)
    {
        Size frameSize = frame.size();

        Point p_top_left(1, 1);
        Point p_top_right(frameSize.width - 1, 1);
        Point p_bottom_left(1, frameSize.height - 1);
        Point p_bottom_right(frameSize.width - 1, frameSize.height - 1);

        bool flag = false;
        Rect whole_page = Rect(0, 0, frameSize.width, frameSize.height);
        Rect remove_navigation = whole_page;

        int nRatio = 2;           // consider some buttons above navigation bar

        if (navigationBarRect.contains(p_top_left) && navigationBarRect.contains(p_top_right))
        {
            remove_navigation = Rect(0, navigationBarRect.height * nRatio, whole_page.width, whole_page.height - navigationBarRect.height * nRatio);
        }
        else if (navigationBarRect.contains(p_top_left) && navigationBarRect.contains(p_bottom_left))
        {
            remove_navigation = Rect(navigationBarRect.width * nRatio, 0, whole_page.width - navigationBarRect.width * nRatio, whole_page.height);
        }
        else if (navigationBarRect.contains(p_bottom_left) && navigationBarRect.contains(p_bottom_right))
        {
            remove_navigation = Rect(0, 0, whole_page.width, whole_page.height - navigationBarRect.height * nRatio);
        }
        else if (navigationBarRect.contains(p_bottom_right) && navigationBarRect.contains(p_top_right))
        {
            remove_navigation = Rect(0, 0, whole_page.width - navigationBarRect.width * nRatio, whole_page.height);
        }

        Rect remove_status_navigation = remove_navigation;

        int ratio = 4;          // consider action bar, suppose action_bar_height = 3 * status_bar_height
        if (!removeActionBar)
            ratio = 1;          // needn't to remove action bar

        if (statusBarRect.contains(p_top_left) && statusBarRect.contains(p_top_right))
        {
            remove_status_navigation = Rect(remove_navigation.x, remove_navigation.y + statusBarRect.height * ratio, remove_navigation.width, remove_navigation.height - statusBarRect.height * ratio);
        }
        else if (statusBarRect.contains(p_top_left) && statusBarRect.contains(p_bottom_left))
        {
            remove_status_navigation = Rect(remove_navigation.x + statusBarRect.width * ratio, remove_navigation.y, remove_navigation.width - statusBarRect.width * ratio, remove_navigation.height);
        }
        else if (statusBarRect.contains(p_bottom_left) && statusBarRect.contains(p_bottom_right))
        {
            remove_status_navigation = Rect(remove_navigation.x, remove_navigation.y, remove_navigation.width, remove_navigation.height - statusBarRect.height * ratio);
        }
        else if (statusBarRect.contains(p_bottom_right) && statusBarRect.contains(p_top_right))
        {
            remove_status_navigation = Rect(remove_navigation.x, remove_navigation.y, remove_navigation.width - statusBarRect.width * ratio, remove_navigation.height);
        }

        if ((remove_status_navigation.x < 0) || ((remove_status_navigation.x + remove_status_navigation.width) > frameSize.width) || (remove_status_navigation.y < 0) || ((remove_status_navigation.y + remove_status_navigation.height) > frameSize.height) || (remove_status_navigation.width < 0) || (remove_status_navigation.height < 0))
        {
            DAVINCI_LOG_WARNING << "ObjectUtil::IsPureSingleColor - Invalid rectangle";
            return flag;
        }

        cv::Mat centerRoiFrame = frame(remove_status_navigation);

#ifdef _DEBUG
        imwrite("center.png", centerRoiFrame);
#endif

        if(!centerRoiFrame.empty() && CountCannyNonZeroPoint(centerRoiFrame) == 0)
        {
            flag = true;
        }

        return flag;
    }

    bool ObjectUtil::IsBadImage(const cv::Mat &currentFrame, const cv::Mat &firstFrame,  const cv::Mat &secondFrame)
    {
        bool currentBadImage = false;
        bool firstBadImage = false;
        bool secondBadImage = false;

        ReadCurrentBarInfo(currentFrame.size().width);

        if (!currentFrame.empty())
        {
            currentBadImage = IsPureSingleColor(currentFrame, this->navigationBarRect , this->statusBarRect);
        }
        if (!firstFrame.empty())
        {
            firstBadImage = IsPureSingleColor(firstFrame, this->navigationBarRect , this->statusBarRect);
        }
        if (!secondFrame.empty())
        {
            secondBadImage = IsPureSingleColor(secondFrame, this->navigationBarRect , this->statusBarRect);
        }

        if (currentBadImage && firstBadImage && secondBadImage)
        {
            DAVINCI_LOG_INFO << "Find bad image, navigation bar rect: " << this->navigationBarRect.x << " " << this->navigationBarRect.y << " " << this->navigationBarRect.width << " " << this->navigationBarRect.height;
            DAVINCI_LOG_INFO << "Find bad image, status bar rect: " << this->statusBarRect.x << " " << this->statusBarRect.y << " " << this->statusBarRect.width << " " << this->statusBarRect.height;
            return true;
        }
        else
        {
            DAVINCI_LOG_INFO << "Not bad image, navigation bar rect: " << this->navigationBarRect.x << " " << this->navigationBarRect.y << " " << this->navigationBarRect.width << " " << this->navigationBarRect.height;
            DAVINCI_LOG_INFO << "Not bad image, status bar rect: " << this->statusBarRect.x << " " << this->statusBarRect.y << " " << this->statusBarRect.width << " " << this->statusBarRect.height;
            return false;
        }
    }

    bool ObjectUtil::IsValidRectangle(Rect rect)
    {
        if (rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void ObjectUtil::CopyROIFrame(Mat frame, Rect roi, Mat& roiFrame)
    {
        Size frameSize = frame.size();
        if(!ContainRect(Rect(EmptyPoint, frameSize), roi))
        {
            DAVINCI_LOG_WARNING << "Invalid ROI for frameSize (" 
                << boost::lexical_cast<string>(frameSize.width) 
                << "," << boost::lexical_cast<string>(frameSize.height) 
                << ") with ROI (" << boost::lexical_cast<string>(roi.x) 
                << "," << boost::lexical_cast<string>(roi.y) 
                << "," << boost::lexical_cast<string>(roi.width) 
                << "," << boost::lexical_cast<string>(roi.height) 
                << ")";
            roiFrame = frame.clone();
        }
        else
        {
            frame(roi).copyTo(roiFrame);
        }
    }

    Rect ObjectUtil::ConstructRectangleFromTwoLines(Size size, LineSegment line1, LineSegment line2, bool horizontalLine)
    {
        Point p1 = line1.p1;
        Point p2 = line1.p2;
        Point p3 = line2.p1;
        Point p4 = line2.p2;

        if (horizontalLine)
        {
            int minX = std::min(p1.x, std::min(p2.x, std::min(p3.x, p4.x)));

            int minY = std::min(p1.y, p3.y);

            int width = std::max(abs(p1.x - p2.x), std::max(abs(p1.x - p3.x), std::max(abs(p1.x - p4.x), std::max(abs(p2.x - p3.x), std::max(abs(p2.x - p4.x), abs(p3.x - p4.x))))));

            int height = abs(p3.y - p1.y);

            if (minX + width > size.width)
            {
                width = size.width - minX;
            }

            if (minY + height > size.height)
            {
                height = size.height - minY;
            }

            return Rect(minX, minY, width, height);
        }
        else
        {
            int minX = std::min(p1.x, p3.x);

            int minY = std::min(p1.y, std::min(p2.y, std::min(p3.y, p4.y)));

            int width = abs(p3.x - p1.x);

            int height = std::max(abs(p1.y - p2.y), std::max(abs(p1.y - p3.y), std::max(abs(p1.y - p4.y), std::max(abs(p2.y - p3.y), std::max(abs(p2.y - p4.y), abs(p3.y - p4.y))))));

            if (minX + width > size.width)
            {
                width = size.width - minX;
            }

            if (minY + height > size.height)
            {
                height = size.height - minY;
            }

            return Rect(minX, minY, width, height);
        }
    }


    bool ObjectUtil::IsRectangleValidWithArea(Rect rect, int minArea, int maxArea)
    {
        // Filter objects too small (minArea), too big (maxArea), and too flat (10 ratio)
        if (rect.width < rect.height * 10 && rect.height < rect.width * 10 && rect.width * rect.height > minArea && rect.width * rect.height < maxArea)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ObjectUtil::IsRectangleValidWithROI(Rect rect, Size size, int ltX, int ltY, int rdX, int rdY, int edgeDistance, bool isContour)
    {
        // Note that ROI is either from Portrait mode nor Landscape mode

        // Exclude the objects existing in status and navigation bar
        if (ltX == 0 && ltY == 0 && rdX == 0 && rdY == 0)
        {
            // status bar | app | nagivation bar
            if (rect.y > ObjectUtil::statusBarHeight && rect.y + rect.height < size.height - ObjectUtil::navigationBarHeight)
            {
                if (!isContour)
                {
                    if (rect.x > edgeDistance && rect.x + rect.width < size.width - edgeDistance)
                    {
                        return true;
                    }
                }
                else
                {
                    return true;
                }
            }
        }
        else
        {
            if (rect.y > ltY && rect.y < rdY && rect.x > ltX && rect.x < rdX)
            {
                return true;
            }
        }

        return false;
    }

    std::vector<Rect> ObjectUtil::CollectValidRectanglesByAreaAndROI(std::vector<Rect> &rectList, Size size, int minArea, int maxArea, int ltX, int ltY, int rdX, int rdY, int edgeDistance, bool isContour)
    {
        std::vector<Rect> validRectList = std::vector<Rect>();
        for (auto rect : rectList)
        {
            if (IsRectangleValidWithArea(rect, minArea, maxArea) && IsRectangleValidWithROI(rect, size, ltX, ltY, rdX, rdY, edgeDistance, isContour))
            {
                validRectList.push_back(rect);
            }
        }
        return validRectList;
    }

    std::map<int, std::vector<cv::Rect>> ObjectUtil::HashRectangles(const std::vector<cv::Rect> &rects, UType hashType, int approximateKeyInterval)
    {
        std::map<int, std::vector<cv::Rect>> map;

        for (auto rect : rects)
        {
            int key = -1;
            switch (hashType)
            {
            case UType::UTYPE_X:
                key = rect.x;
                break;
            case UType::UTYPE_Y:
                key = rect.y;
                break;
            case UType::UTYPE_W:
                key = rect.width;
                break;
            case UType::UTYPE_H:
                key = rect.height;
                break;
            case UType::UTYPE_CX:
                key = rect.x + rect.width / 2;
                break;
            case UType::UTYPE_CY:
                key = rect.y + rect.height / 2;
            default:
                break;
            }

            if (map.empty())
            {
                std::vector<cv::Rect> valueRects;
                valueRects.push_back(rect);
                map[key] = valueRects;
            }
            else
            {
                bool isFound = false;
                int realKey = key;

                if (approximateKeyInterval == 0)
                {
                    if (map.find(key) != map.end())
                    {
                        isFound = true;
                        realKey = key;
                    }
                }
                else
                {
                    for (int i = key - approximateKeyInterval; i <= key + approximateKeyInterval; i++)
                    {
                        if (map.find(i) != map.end())
                        {
                            isFound = true;
                            realKey = i;
                            break;
                        }
                    }
                }

                if (isFound)
                {
                    std::vector<cv::Rect> rowRects = map[realKey];
                    rowRects.push_back(rect);
                    map[realKey] = rowRects;
                }
                else
                {
                    std::vector<cv::Rect> rowRects;
                    rowRects.push_back(rect);
                    map[realKey] = rowRects;
                }
            }
        }

        return map;
    }

    std::multimap<int, std::vector<cv::Rect>> ObjectUtil::HashRectanglesWithRectInterval(std::vector<cv::Rect> &rects, UType hashType, int approximateKeyInterval, int lowRectInterval, int highRectInterval, bool isHorizontalInterval)
    {
        std::multimap<int, std::vector<cv::Rect>> map;

        if (isHorizontalInterval)
        {
            SortRects(rects, UTYPE_X, true);
        }
        else
        {
            SortRects(rects, UTYPE_Y, true);
        }

        for (auto rect : rects)
        {
            int key = -1;
            switch (hashType)
            {
            case UType::UTYPE_X:
                key = rect.x;
                break;
            case UType::UTYPE_Y:
                key = rect.y;
                break;
            case UType::UTYPE_W:
                key = rect.width;
                break;
            case UType::UTYPE_H:
                key = rect.height;
                break;
            case UType::UTYPE_CX:
                key = rect.x + rect.width / 2;
                break;
            case UType::UTYPE_CY:
                key = rect.y + rect.height / 2;
            default:
                break;
            }

            if (map.empty())
            {
                std::vector<cv::Rect> valueRects;
                valueRects.push_back(rect);
                map.insert(std::make_pair(key, valueRects));
            }
            else
            {
                bool isFound = false;

                if (approximateKeyInterval == 0)
                {
                    std::multimap<int, std::vector<cv::Rect>>::iterator it = map.find(key);
                    int keyCount = (int)map.count(key);
                    for (int i = 0; i < keyCount; i++, it++)
                    {
                        if (it != map.end())
                        {
                            cv::Rect lastRect = it->second.back(); // compare interval with the last one
                            int interval = (isHorizontalInterval ? rect.x - lastRect.x - lastRect.width : rect.y - lastRect.y - lastRect.height);
                            if (interval >= lowRectInterval && interval <= highRectInterval)
                            {
                                isFound = true;
                                it->second.push_back(rect);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    std::multimap<int, std::vector<cv::Rect>>::iterator low = map.lower_bound(key - approximateKeyInterval);
                    std::multimap<int, std::vector<cv::Rect>>::iterator high = map.upper_bound(key + approximateKeyInterval);
                    for (std::multimap<int, std::vector<cv::Rect>>::iterator it = low; it != high; it++)
                    {
                        cv::Rect lastRect = it->second.back(); // compare interval with the last one
                        int interval = (isHorizontalInterval ? rect.x - lastRect.x - lastRect.width : rect.y - lastRect.y - lastRect.height);
                        if (interval >= lowRectInterval && interval <= highRectInterval)
                        {
                            isFound = true;
                            it->second.push_back(rect);
                            break;
                        }
                    }
                }

                if (!isFound)
                {
                    std::vector<cv::Rect> valueRects;
                    valueRects.push_back(rect);
                    map.insert(std::make_pair(key, valueRects));
                }
            }
        }

        return map;
    }

    Rect ObjectUtil::MergeRectangles(std::vector<Rect> &rectList)
    {
        Point leftTop = Point(INT_MAX, INT_MAX);
        Point rightDown = Point(INT_MIN, INT_MIN);

        for (auto rect : rectList)
        {
            if (rect.x <= leftTop.x)
            {
                leftTop.x = rect.x;
            }

            if (rect.y <= leftTop.y)
            {
                leftTop.y = rect.y;
            }

            if (rect.x + rect.width >= rightDown.x)
            {
                rightDown.x = rect.x + rect.width;
            }

            if (rect.y + rect.height >= rightDown.y)
            {
                rightDown.y = rect.y + rect.height;
            }
        }

        return Rect(leftTop.x, leftTop.y, rightDown.x - leftTop.x, rightDown.y - leftTop.y);
    }

    std::vector<Rect> ObjectUtil::RelocateRectangleList(Size size, std::vector<Rect> &rectList, Rect targetRect)
    {
        std::vector<Rect> relocatedRectList;
        for (auto rect : rectList)
        {
            relocatedRectList.push_back(RelocateRectangle(size, rect, targetRect));
        }

        return relocatedRectList;
    }

    Rect ObjectUtil::RelocateRectangle(Size size, Rect rect, Rect targetRect)
    {
        int x = rect.x + targetRect.x;
        int y = rect.y + targetRect.y;
        int width = rect.width;
        int height = rect.height;

        if (x + width > size.width)
        {
            width = size.width - x;
        }

        if (y + height > size.height)
        {
            height = size.height - y;
        }

        return Rect(x, y, width, height);
    }

    Point ObjectUtil::RelocatePointByRatio(const Point &sourcePoint, Size sourceSize, Size targetSize)
    {
        Point relocatedPoint;

        double widthRatio = (double)targetSize.width / sourceSize.width;
        double heightRatio = (double)targetSize.height / sourceSize.height;

        relocatedPoint.x = int(sourcePoint.x * widthRatio);
        relocatedPoint.y = int(sourcePoint.y * heightRatio);

        return relocatedPoint;
    }

    Point ObjectUtil::RelocatePointByOffset(const Point &sourcePoint, Rect rect, bool isSmallToBig)
    {
        Point relocatedPoint;

        if(isSmallToBig)
        {
            relocatedPoint.x = sourcePoint.x + rect.x;
            relocatedPoint.y = sourcePoint.y + rect.y;
        }
        else
        {
            relocatedPoint.x = sourcePoint.x - rect.x;
            relocatedPoint.y = sourcePoint.y - rect.y;
        }

        return relocatedPoint;
    }

    cv::Rect ObjectUtil::RotateRectangleAnticlockwise(const cv::Rect &rect, cv::Size frameSize, int rotateAngle)
    {
        cv::Rect rotatedRect;
        switch(rotateAngle)
        {
        case 90:
            rotatedRect = cv::Rect(rect.y, frameSize.width - rect.br().x, rect.height, rect.width);
            break;
        case 180:
            rotatedRect = cv::Rect(frameSize.width - rect.br().x, frameSize.height - rect.br().y, rect.width, rect.height);
            break;
        case 270:
            rotatedRect = cv::Rect(frameSize.height - rect.br().y, rect.x, rect.height, rect.width);
            break;
        default:
            rotatedRect = rect;
            break;
        }

        return rotatedRect;
    }

    //Otsu's method http://en.wikipedia.org/wiki/Otsu%27s_method
    int ObjectUtil::DetectBinaryThreshold(const cv::Mat &_src)
    {
        Size size = _src.size();
        int step = (int) _src.step;
        if( _src.isContinuous() )
        {
            size.width *= size.height;
            size.height = 1;
            step = size.width;
        }
        const int N = 256;
        int i, j, h[N] = {0};
        for( i = 0; i < size.height; i++ )
        {
            const uchar* src = _src.ptr() + step*i;
            j = 0;
            for( ; j < size.width; j++ )
                h[src[j]]++;
        }

        double mu = 0, scale = 1./(size.width*size.height);
        for( i = 0; i < N; i++ )
            mu += i*(double)h[i];

        mu *= scale;
        double mu1 = 0, q1 = 0;
        double max_sigma = 0, max_val = 0;

        for( i = 0; i < N; i++ )
        {
            double p_i, q2, mu2, sigma;

            p_i = h[i]*scale;
            mu1 *= q1;
            q1 += p_i;
            q2 = 1. - q1;

            if( std::min(q1,q2) < FLT_EPSILON || std::max(q1,q2) > 1. - FLT_EPSILON )
                continue;

            mu1 = (mu1 + i*p_i)/q1;
            mu2 = (mu - q1*mu1)/q2;
            sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
            if( sigma > max_sigma )
            {
                max_sigma = sigma;
                max_val = i;
            }
        }

        return (int)max_val;
    }

    std::vector<cv::Rect> ObjectUtil::DetectRectsByContour(const cv::Mat &frame, int cannyThreshold, int xKernelSize, int yKernelSize, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea, int retrievalMode)
    {
        std::vector<cv::Rect> rects;
        if(frame.empty())
            return rects;

        cv::Mat grayFrame = BgrToGray(frame);
        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, cannyThreshold, cannyThreshold / 2);
        DebugSaveImage(cannyFrame, "debug_canny.png");

        cv::Mat kernel = cv::getStructuringElement(MORPH_RECT, cv::Size(xKernelSize, yKernelSize));
        cv::Mat dilationFrame;
        cv::morphologyEx(cannyFrame, dilationFrame, MORPH_DILATE, kernel, cv::Point(-1, -1), 2);
        DebugSaveImage(dilationFrame, "debug_dilation.png");

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(dilationFrame, contours, retrievalMode, CV_CHAIN_APPROX_NONE);
#ifdef _DEBUG
        cv::Mat debugDrawFrame = frame.clone();
#endif
        for (auto contour : contours)
        {
            cv::Rect rect = cv::boundingRect(contour);

            double ratio = (double)rect.width/ rect.height;
            if (ratio >= lowSideRatio && ratio <= highSideRatio && rect.area() >= lowRectArea && rect.area() <= highRectArea)
            {
#ifdef _DEBUG
                DrawRect(debugDrawFrame, rect, Red, 1);
                SaveImage(debugDrawFrame, "debug_all_rects_before.png");
#endif
                rects.push_back(rect);
            }
        }

        // Contours by CV_RETR_LIST always have lots of repeated elements, need removing
        SortRects(rects, UType::UTYPE_CC, true);
        RemoveRepeatedElementFromVector(rects);

#ifdef _DEBUG
        cv::Mat debugFrame = frame.clone();
        DrawRects(debugFrame, rects, Red, 1);
        SaveImage(debugFrame, "debug_all_rects.png");
#endif

        return rects;
    }

    std::vector<cv::Rect> ObjectUtil::DetectRealRectsByContour(const cv::Mat &frame, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea, int cannyThreshold, cv::Size ksize, int retrievalMode)
    {
        std::vector<cv::Rect> rects;

        cv::Mat grayFrame = BgrToGray(frame);
        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, cannyThreshold, cannyThreshold / 2);

        cv::Mat kernel = cv::getStructuringElement(MORPH_RECT, ksize);
        cv::Mat closeFrame;
        cv::morphologyEx(cannyFrame, closeFrame, MORPH_CLOSE, kernel);
        DebugSaveImage(closeFrame, "debug_close.png");

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(closeFrame, contours, retrievalMode, CV_CHAIN_APPROX_NONE);
        for (auto contour : contours)
        {
            cv::Rect boundingRect = cv::boundingRect(contour);
            double sideRatio = (double)boundingRect.width / boundingRect.height;

            int boundingRectArea = boundingRect.area();
            double realArea = cv::contourArea(contour) + contour.size();
            double areaRatio = realArea / boundingRectArea;

            if (sideRatio >= lowSideRatio && sideRatio <= highSideRatio && boundingRectArea >= lowRectArea && boundingRectArea <= highRectArea && areaRatio >= 0.9) // A real rectangle is as similar size to its bounding rectangle
            {
                rects.push_back(boundingRect);
            }
        }

        // Contours always have lots of repeated elements, need removing
        SortRects(rects, UType::UTYPE_CC, true);
        RemoveRepeatedElementFromVector(rects);

#ifdef _DEBUG
        cv::Mat debugFrame = frame.clone();
        DrawRects(debugFrame, rects, Red, 1);
        SaveImage(debugFrame, "debug_all_rects.png");
#endif

        return rects;
    }

    std::vector<cv::Rect> ObjectUtil::GetRectsWithInterval(const cv::Rect &sourceRect, std::vector<cv::Rect> &targetRects, int rectInterval, bool isUpward)
    {
        std::vector<cv::Rect> rects;

        if (targetRects.size() > 1)
        {
            if (isUpward)
            {
                SortRects(targetRects, UType::UTYPE_Y, false);
            }
            else
            {
                SortRects(targetRects, UType::UTYPE_Y, true);
            }
        }

        for (auto rect : targetRects)
        {
            int interval = 0;
            if (isUpward)
            {
                interval = sourceRect.y - rect.y - rect.height;
            }
            else
            {
                interval = rect.y - sourceRect.y - sourceRect.height;
            }

            if (interval > 0 && interval <= rectInterval)
            {
                rects.push_back(rect);
            }
        }

        return rects;
    }

    cv::Mat ObjectUtil::BinarizeWithChangedThreshold(const cv::Mat bgrImage, int lowThreshold, int highThreshold)
    {
        cv::Mat binaryImage;
        int binaryThreshold = 0;

        cv::Mat grayImage = BgrToGray(bgrImage);

        int detectedThreshold = ObjectUtil::Instance().DetectBinaryThreshold(grayImage);
        if (detectedThreshold <= 100)
        {
            binaryThreshold = detectedThreshold;
        }
        else if (detectedThreshold < 180)
        {
            binaryThreshold = lowThreshold;
        }
        else
        {
            binaryThreshold = highThreshold;
        }

        cv::threshold(grayImage, binaryImage, binaryThreshold, 255, CV_THRESH_BINARY);

        return binaryImage;
    }

    bool ObjectUtil::IsImageChanged(const cv::Mat &preImage, const cv::Mat &currentImage, double ratio)
    {
        bool isChanged = false;

        if (preImage.size() != currentImage.size())
        {
            isChanged = true;
        }
        else
        {
            cv::Mat diffImage;
            cv::absdiff(currentImage, preImage, diffImage);
            SaveImage(diffImage, "diff.png");

            cv::Mat grayImage = BgrToGray(diffImage);

            cv::Mat binaryImage;
            cv::threshold(grayImage, binaryImage, 10, 255, CV_THRESH_BINARY);
            DebugSaveImage(binaryImage, "diff_binary.png");

            int nonZeroCount = cv::countNonZero(binaryImage); // Count different points
            double nonZeroRatio = (double)nonZeroCount / (binaryImage.cols * binaryImage.rows);

            DAVINCI_LOG_INFO << "[info] Non zero points ratio is " << nonZeroRatio;
            if (nonZeroRatio >= ratio)
            {
                DAVINCI_LOG_INFO << ", so image changed." << std::endl;
                isChanged = true;
            }
            else
            {
                DAVINCI_LOG_INFO << ", so image not changed." << std::endl;
            }
        }

        return isChanged;
    }

    std::vector<std::vector<Rect>> ObjectUtil::SplitRectangleByDimension(Rect rect, int dimension)
    {
        std::vector<std::vector<Rect>> arrays = std::vector<std::vector<Rect>>(dimension);
        for (int i = 0; i < dimension; i++)
        {
            arrays[i] = std::vector<Rect>(dimension);
        }
        int xStep = static_cast<int>(rect.width) / dimension;
        int yStep = static_cast<int>(rect.height) / dimension;
        int x, y;

        for (int i = 0; i < dimension - 1; i++)
        {
            x = rect.x + xStep * i;
            for (int j = 0; j < dimension - 1; j++)
            {
                y = rect.y + yStep * j;
                arrays[i][j] = Rect(x, y, xStep, yStep);
            }
        }

        int leftWidthStart = rect.x + xStep * (dimension - 1);
        int leftHeightStart = rect.y + yStep * (dimension - 1);
        int leftWidth = rect.width - xStep * (dimension - 1);
        int leftHeight = rect.height - yStep * (dimension - 1);

        // right-most column
        for (int j = 0; j < dimension - 1; j++)
        {
            y = rect.y + yStep * j;
            arrays[dimension - 1][j] = Rect(leftWidthStart, y, leftWidth, yStep);
        }

        // down-most column
        for (int i = 0; i < dimension - 1; i++)
        {
            x = rect.x + xStep * i;
            arrays[i][dimension - 1] = Rect(x, leftHeightStart, xStep, leftHeight);
        }

        // right down cell
        arrays[dimension - 1][dimension - 1] = Rect(leftWidthStart, leftHeightStart, leftWidth, leftHeight);

        return arrays;
    }

    std::vector<Rect> ObjectUtil::SplitRectangleByNumber(Rect rect, bool horizontal, int number)
    {
        std::vector<Rect> arrays = std::vector<Rect>(number);

        int xStep = static_cast<int>(rect.width) / number;
        int yStep = static_cast<int>(rect.height) / number;
        int x, y;

        if (horizontal)
        {
            y = rect.y;
            for (int i = 0; i < number - 1; i++)
            {
                x = rect.x + xStep * i;
                arrays[i] = Rect(x, y, xStep, rect.height);
            }
            int leftWidthStart = rect.x + xStep * (number - 1);
            int leftWidth = rect.width - xStep * (number - 1);
            arrays[number - 1] = Rect(leftWidthStart, y, leftWidth, rect.height);
        }
        else
        {
            x = rect.x;
            for (int i = 0; i < number - 1; i++)
            {
                y = rect.y + yStep * i;
                arrays[i] = Rect(x, y, rect.width, yStep);
            }

            int leftHeightStart = rect.y + yStep * (number - 1);
            int leftHeight = rect.height - yStep * (number - 1);
            arrays[number - 1] = Rect(x, leftHeightStart, rect.width, leftHeight);
        }

        return arrays;
    }

    bool ObjectUtil::IsContainedRectangle(std::vector<Rect> &rectList, Rect &targetRect, Rect &outRect)
    {
        outRect = EmptyRect;
        for (auto rect : rectList)
        {
            if (ContainRect(rect, targetRect))
            {
                outRect = rect;
                break;
            }
        }

        if (outRect == EmptyRect)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    vector<Rect> ObjectUtil::RemoveOuterRects(vector<Rect> rects)
    {
        vector<Rect> returnRects;
        int rectSize = (int)rects.size();
        bool hasContainedRect;

        for(int i = 0;i < rectSize; ++i)
        {
            Rect iRect = rects[i];
            hasContainedRect = false;
            for (auto rect : rects)
            {
                if (iRect.x < rect.x && iRect.x + iRect.width > rect.x + rect.width && iRect.y < rect.y && iRect.y + iRect.height > rect.y + rect.height)
                {
                    hasContainedRect = true;
                }
            }

            if (hasContainedRect == false)
                returnRects.push_back(iRect);
        }
        return returnRects;
    }

    vector<Rect> ObjectUtil::RemoveSimilarRects(const vector<Rect>& rects, int interval)
    {
        vector<Rect> returnRects;
        int rectSize = (int)rects.size();

        for(int i = 0; i <= rectSize - 1; ++i)
        {
            Rect iRect = rects[i];
            bool hasSimilarRect = false;

            for (int j = i + 1; j < rectSize; ++j)
            {
                Rect jRect = rects[j];

                // iRect is similar as jRect
                if (abs(iRect.x -jRect.x) <= interval && abs(iRect.y -jRect.y) <= interval && abs(iRect.width -jRect.width) <= interval && abs(iRect.height -jRect.height) <= interval)
                {
                    hasSimilarRect = true;
                }
            }
            if (hasSimilarRect == false)
            {
                returnRects.push_back(iRect);
            }
        }
        return returnRects;
    }


#pragma region  For Get_RnR
    vector<Rect> ObjectUtil::RemoveContainedRects(const vector<Rect>& rects)
    {
        vector<Rect> returnRects;
        int rectSize = (int)rects.size();

        for(int i = 0;i < rectSize; ++i)
        {
            Rect iRect = rects[i];
            Rect outRect;
            if(!IsContainedRectangle(returnRects, iRect, outRect))
            {
                vector<Rect> returnRectsCopy = returnRects;
                for(auto returnRect : returnRectsCopy)
                {
                    if(ContainRect(iRect, returnRect))
                        RemoveElementFromVector(returnRects, returnRect);
                }
                returnRects.push_back(iRect);
            }
        }

        return returnRects;
    }

    vector<Rect> ObjectUtil::RectsContainPoint(vector<Rect> rects, Point point)
    {
        vector<Rect> returnRects;
        for(auto rect : rects)
        {
            if(rect.contains(point))
                returnRects.push_back(rect);
        }

        return returnRects;
    }

#pragma endregion

    bool ObjectUtil::IsImageSolidColoredHis(const cv::Mat& image)
    {
        if(image.channels()==3)
        {
            vector<Mat> bgrPlanes;
            split( image, bgrPlanes );
            for(int i=0;i<3;++i)
            {
                if(IsGrayImageSolid(bgrPlanes[i]) == false)
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            return IsGrayImageSolid(image);
        }
    }

    bool ObjectUtil::IsGrayImageSolid(const cv::Mat& grayImage, double threshold, int delta)
    {
        assert(1==grayImage.channels());
        /// Establish the number of bins
        int histSize = 256;
        assert(delta<histSize);

        /// Set the ranges ( for B,G,R) )
        float range[] = { 0, 256 } ;
        const float* histRange = { range };

        bool uniform = true; bool accumulate = false;

        Mat hist;

        /// Compute the histograms:
        calcHist( &grayImage, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );
        hist/=(grayImage.rows*grayImage.cols);
        double ratio=0;
        int i;
        for(i=0;i<delta;++i)
        {
            ratio+=hist.at<float>(i,0);
        }
        while(i<histSize)
        {
            if(ratio>threshold)
            {
                return true;
            }
            ratio = ratio + hist.at<float>(i,0) - hist.at<float>(i-delta,0);
            ++i;
        }

        return false;
    }

    bool ObjectUtil::IsImageSolidColored(const cv::Mat &image)
    {
        bool isSolidColored = false;
        cv::Mat dstImage = AddWeighted(image);
        DebugSaveImage(dstImage, "grad.png");

        cv::Mat grayImage = BgrToGray(dstImage);
        cv::Mat binaryImage;
        cv::threshold(grayImage, binaryImage, 10, 255, CV_THRESH_BINARY);

        int nonZeroCount = cv::countNonZero(binaryImage);
        double nonZeroRatio = (double)nonZeroCount / (binaryImage.cols * binaryImage.rows);
        if (nonZeroRatio < 0.05)
        {
            isSolidColored = true;
        }

        return isSolidColored;
    }

    cv::Rect ObjectUtil::ConstructRectangleFromCenterPoint(const cv::Point &centerPoint, const cv::Size &frameSize, int rectWidth, int rectHeight)
    {
        cv::Rect frameRect(EmptyPoint, frameSize);
        cv::Rect rect(centerPoint.x - rectWidth / 2, centerPoint.y - rectHeight / 2, rectWidth, rectHeight);
        rect &= frameRect;
        return rect;
    }

    cv::Rect ObjectUtil::ConstructTextRectangle(const cv::Point &centerPoint, const cv::Size &frameSize, int rectWidth, int rectHeight)
    {
        cv::Rect frameRect(EmptyPoint, frameSize);
        cv::Rect rect(0, centerPoint.y - rectHeight / 2, frameSize.width, rectHeight);
        rect &= frameRect;
        return rect;
    }

    cv::Rect ObjectUtil::ScaleRectangleWithLeftTopPoint(const cv::Rect &rect, const cv::Size &frameSize, int ltPointXOffset, int ltPointYOffset, bool isExtended)
    {
        cv::Rect frameRect(EmptyPoint, frameSize);
        cv::Rect scaledRect;

        if (isExtended)
        {
            scaledRect = cv::Rect(rect.x - ltPointXOffset, rect.y - ltPointYOffset, rect.width + 2 * ltPointXOffset, rect.height + 2 * ltPointYOffset);
        }
        else
        {
            scaledRect = cv::Rect(rect.x + ltPointXOffset, rect.y + ltPointYOffset, rect.width - 2 * ltPointXOffset, rect.height - 2 * ltPointYOffset);
        }
        scaledRect &= frameRect;

        return scaledRect;
    }

    std::vector<Rect> ObjectUtil::FilterRects(const std::vector<cv::Rect> &rects, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea)
    {
        std::vector<Rect> filteredRects;

        for (auto rect : rects)
        {
            double ratio = (double)rect.width/ rect.height;
            if (ratio >= lowSideRatio && ratio <= highSideRatio && rect.area() >= lowRectArea && rect.area() <= highRectArea)
            {
                filteredRects.push_back(rect);
            }
        }

        return filteredRects;
    }
}

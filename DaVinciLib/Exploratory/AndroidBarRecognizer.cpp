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

#include <vector>

#include "AndroidTargetDevice.hpp"
#include "AndroidBarRecognizer.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"

using namespace std;
using namespace cv;

namespace DaVinci
{
    AndroidBarRecognizer::AndroidBarRecognizer()
    {
        deltaBarHeight = 20;
        navigationBarRect = EmptyRect;
        threeBarButtons = vector<Rect>(3);
        navigationBarLocation = NavigationBarLocation::BarLocationUnknown;
    }

    cv::Rect AndroidBarRecognizer::GetNavigationBarRect()
    {
        return navigationBarRect;
    }

    NavigationBarLocation AndroidBarRecognizer::GetNavigationBarLocation()
    {
        return navigationBarLocation;
    }

    int AndroidBarRecognizer::FindButtonFromPoint(Point p, double allowDistance)
    {
        if(!navigationBarRect.contains(p))
            return -1;

        for(int index = 0; index < (int)threeBarButtons.size(); index++)
        {
            if(threeBarButtons[index].contains(p))
            {
                return index;
            }
            else
            {
                Rect rect = threeBarButtons[index];
                Point rectCenter(rect.x + rect.width / 2, rect.y + rect.height);
                double distance = ComputeDistance(rectCenter, p);
                if(FSmallerE(distance, allowDistance))
                    return index;
            }
        }

        return -1;
    }

    std::vector<cv::Rect> AndroidBarRecognizer::RecognizeThreeBarButtons(const cv::Mat &barFrame)
    {
        std::vector<cv::Rect> buttons = this->FindThreeBarButtons(barFrame, 250, 3000, 90, 300);

        if (buttons.empty())
        {
            cv::Mat weightedBarFrame = AddWeighted(barFrame);
            DebugSaveImage(weightedBarFrame, "bar.png");

            buttons = this->FindThreeBarButtons(weightedBarFrame, 50, 150, 120, 300);
        }

        return buttons;
    }

    bool AndroidBarRecognizer::RecognizeNavigationBar(const cv::Mat &frame, int barHeight)
    {
        bool isRecognized = false;

        this->navigationBarRect = EmptyRect;
        this->navigationBarLocation = NavigationBarLocation::BarLocationUnknown;

        // Rotate source frame clockwise to recognize bar at the bottom, until recognize the bar, then rotate bar rect anticlockwise
        // Clockwise 0: BarLocationBottom
        // Clockwise 90: BarLocationRight
        // Clockwise 180: BarLocationTop
        // Clockwise 270: BarLocationLeft
        for (int i = 0; i < 4; i++)
        {
            int rotateAngle = i * 90;
            cv::Mat rotateFrame = RotateFrameClockwise(frame, rotateAngle);
            DebugSaveImage(rotateFrame, "rotate.png");

            cv::Rect barRect = EmptyRect;
            cv::Mat barFrame;
            if (barHeight > 0)
            {
                barRect = cv::Rect(0, rotateFrame.rows - barHeight, rotateFrame.cols, barHeight);
                ObjectUtil::Instance().CopyROIFrame(rotateFrame, barRect, barFrame);
            }
            else
            {
                cv::Rect bigBarRect = cv::Rect(0, rotateFrame.rows * 9 / 10, rotateFrame.cols, rotateFrame.rows / 10);
                cv::Mat bigBarFrame;
                ObjectUtil::Instance().CopyROIFrame(rotateFrame, bigBarRect, bigBarFrame);
                DebugSaveImage(bigBarFrame, "bar.png");

                int bottomBarBoundaryY = this->DetectBottomBarBoundary(bigBarFrame, bigBarFrame.cols * 0.8);
                if (bottomBarBoundaryY != 0)
                {
                    barRect = cv::Rect(0, bottomBarBoundaryY, bigBarFrame.cols, bigBarFrame.rows - bottomBarBoundaryY);
                    barRect = ObjectUtil::Instance().RelocateRectangle(rotateFrame.size(), barRect, bigBarRect);
                    ObjectUtil::Instance().CopyROIFrame(rotateFrame, barRect, barFrame);
                    DebugSaveImage(barFrame, "bar.png");

                    if (!ObjectUtil::Instance().IsImageSolidColoredHis(barFrame)) // If not solid colored, use old big bar frame
                    {
                        barRect = bigBarRect;
                        barFrame = bigBarFrame;
                    }
                }
                else
                {
                    barRect = bigBarRect;
                    barFrame = bigBarFrame;
                }
            }

            this->threeBarButtons = RecognizeThreeBarButtons(barFrame);

            if (!this->threeBarButtons.empty())
            {
                isRecognized = true;

                this->navigationBarLocation = NavigationBarLocation(i);
                this->navigationBarRect = ObjectUtil::Instance().RotateRectangleAnticlockwise(barRect, rotateFrame.size(), rotateAngle);
                this->threeBarButtons = ObjectUtil::Instance().RelocateRectangleList(rotateFrame.size(), this->threeBarButtons, barRect);
                for (int i = 0; i < 3; i++)
                {
                    this->threeBarButtons[i] = ObjectUtil::Instance().RotateRectangleAnticlockwise(this->threeBarButtons[i], rotateFrame.size(), rotateAngle);
                }
#ifdef _DEBUG
                cv::Mat debugFrame = frame.clone();
                DrawRects(debugFrame, this->threeBarButtons, Red, 2);
                DrawRect(debugFrame, this->navigationBarRect, Blue, 2);
                SaveImage(debugFrame, "bar.png");
#endif
                break;
            }
        }

        return isRecognized;
    }

    bool AndroidBarRecognizer::RecognizeNavigationBarWithBarInfo(const cv::Mat &frame, Rect barRect)
    {
        bool isRecognized = false;
        this->navigationBarRect = barRect;

        // Rotate source frame clockwise to recognize bar at the bottom, until recognize the bar, then rotate bar rect anticlockwise
        // Clockwise 0: BarLocationBottom
        // Clockwise 90: BarLocationRight
        // Clockwise 180: BarLocationTop
        // Clockwise 270: BarLocationLeft
        Mat barFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, barRect, barFrame);

        for (int i = 0; i < 4; i++)
        {
            int rotateAngle = i * 90;
            cv::Mat rotateFrame = RotateFrameClockwise(barFrame, rotateAngle);
            DebugSaveImage(rotateFrame, "rotate.png");

            this->threeBarButtons = RecognizeThreeBarButtons(rotateFrame);

            if (!this->threeBarButtons.empty())
            {
                isRecognized = true;
                for (int i = 0; i < 3; i++)
                {
                    this->threeBarButtons[i] = ObjectUtil::Instance().RotateRectangleAnticlockwise(this->threeBarButtons[i], rotateFrame.size(), rotateAngle);
                }

                this->threeBarButtons = ObjectUtil::Instance().RelocateRectangleList(frame.size(), this->threeBarButtons, barRect);
#ifdef _DEBUG
                cv::Mat debugFrame = frame.clone();
                DrawRects(debugFrame, this->threeBarButtons, Red, 2);
                SaveImage(debugFrame, "bar.png");
#endif
                break;
            }
        }

        return isRecognized;
    }

    std::vector<cv::Rect> AndroidBarRecognizer::FindThreeBarButtons(const cv::Mat &frame, int lowRectArea, int highRectArea, int lowRectInterval, int highRectInterval)
    {
        std::vector<cv::Rect> barButtons;

        cv::Mat grayFrame = BgrToGray(frame);
        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, 200, 100);
        DebugSaveImage(cannyFrame, "canny.png");

        std::vector<cv::Rect> barButtonRects;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(cannyFrame, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
        for (auto contour : contours)
        {
            cv::Rect rect = boundingRect(contour);
            if (rect.area() >= lowRectArea && rect.area() <= highRectArea && rect.x > 1 && rect.y > 1)
            {
                barButtonRects.push_back(rect);
            }
        }
#ifdef _DEBUG
        cv::Mat debugFrame = frame.clone();
        DrawRects(debugFrame, barButtonRects, Red, 1);
        SaveImage(debugFrame, "bar.png");
#endif

        if (barButtonRects.size() >= 3)
        {
            std::multimap<int, std::vector<cv::Rect>> barMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(barButtonRects, UType::UTYPE_CY, 20, 
                lowRectInterval, highRectInterval);
            for (std::multimap<int, std::vector<cv::Rect>>::reverse_iterator it = barMap.rbegin(); it != barMap.rend(); it++)
            {
                std::vector<cv::Rect> rects = it->second;

                if (rects.size() >= 3)
                {
                    size_t centerRectIndex = this->FindCenteredRect(rects, frame.cols);
                    if (centerRectIndex > 0 && centerRectIndex < rects.size() - 1)
                    {
                        int preIndex = (int)centerRectIndex - 1;
                        int nextIndex = (int)centerRectIndex + 1;

                        int interval1 = rects[centerRectIndex].x - rects[preIndex].br().x;
                        int interval2 = rects[nextIndex].x - rects[centerRectIndex].br().x;
                        int interval3 = rects[preIndex].x;
                        int interval4 = frame.cols - rects[nextIndex].br().x;

                        if (abs(interval1 - interval2) <= 10 && abs(interval3 - interval4) <= 10)
                        {
                            barButtons.push_back(rects[preIndex]);
                            barButtons.push_back(rects[centerRectIndex]);
                            barButtons.push_back(rects[nextIndex]);
                            break;
                        }
                    }
                }
            }
        }

        return barButtons;
    }

    bool AndroidBarRecognizer::RecognizeSettingButton(const cv::Mat &frame, Rect &settingButton)
    {
        bool found = false;
        settingButton = EmptyRect;
        std::vector<cv::Rect> settingButtonRects;;

        cv::Rect roi = cv::Rect(frame.cols * 5 / 6, 0, frame.cols / 6, frame.rows / 7);
        cv::Mat settingFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, settingFrame);

        cv::Mat grayFrame = BgrToGray(settingFrame);
        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, 50, 20);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(cannyFrame, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
        for (auto contour : contours)
        {
            cv::Rect rect = cv::boundingRect(contour);
            if (rect.area() >= 25 && rect.area() <= 250)
            {
                settingButtonRects.push_back(rect);
            }
        }
        DebugDrawRects(settingFrame, settingButtonRects, Red, 1);
        DebugSaveImage(settingFrame, "setting.png");

        std::map<int, std::vector<cv::Rect>> wMap = ObjectUtil::Instance().HashRectangles(settingButtonRects, UType::UTYPE_W, 0);
        for (std::map<int, std::vector<cv::Rect>>::iterator it = wMap.begin(); it != wMap.end(); it++)
        {
            std::vector<cv::Rect> wRects = it->second;
            if (wRects.size() >= 3)
            {
                std::multimap<int, std::vector<cv::Rect>> settingMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(wRects, UType::UTYPE_CX, 0, 1, 10, false);
                for (std::multimap<int, std::vector<cv::Rect>>::reverse_iterator rit = settingMap.rbegin(); rit != settingMap.rend(); rit++)
                {
                    std::vector<cv::Rect> rects = rit->second;
                    DebugDrawRects(settingFrame, rects, Green, 1);
                    DebugSaveImage(settingFrame, "setting.png");

                    if (rects.size() == 3)
                    {
                        int diff1 = rects[2].y - rects[1].y - rects[1].height;
                        int diff2 = rects[1].y - rects[0].y - rects[0].height;
                        if (abs(diff1 - diff2) <= 3)
                        {
                            std::vector<cv::Rect> rolocatdRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), rects, roi);
                            settingButton = ObjectUtil::Instance().MergeRectangles(rolocatdRects);
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (found)
            {
                break;
            }
        }

        return found;
    }

    bool AndroidBarRecognizer::RecognizeBarRectangle(const cv::Mat &frame, Rect &settingRect, Rect &statusBarRect, Rect &actionBarRect)
    {
        settingRect = EmptyRect;
        statusBarRect = EmptyRect;
        actionBarRect = EmptyRect;

        Size frameSize = frame.size();
        bool settingFound = RecognizeSettingButton(frame, settingRect);

        Rect roi = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight + ObjectUtil::Instance().actionBarHeight + 2 * deltaBarHeight);

        cv::Mat roiFrame = frame(roi);

        cv::Mat grayImg;
        cvtColor(roiFrame, grayImg, CV_BGR2GRAY);

        cv::Mat edgeBinary;
        cv::Canny(grayImg, edgeBinary, 100, 20);

        std::vector<Vec4i> lines;
        HoughLinesP(edgeBinary, lines, 1.0, CV_PI / 180.0, 20, 100, 1.0);
        std::vector<LineSegment> horizontalLines = GetHorizontalLineSegments(Vec4iLinesToLineSegments(lines));

        SortLineSegments(horizontalLines, UTYPE_Y, true);

        if (settingFound)
        {
            if (settingRect.y > ObjectUtil::Instance().statusBarHeight)
            {
                statusBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight);
                actionBarRect = Rect(0, ObjectUtil::Instance().statusBarHeight, frameSize.width, ObjectUtil::Instance().actionBarHeight);
            }
            else
            {
                actionBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().actionBarHeight);
            }
        }
        else
        {
            if (horizontalLines.size() >= 1)
            {
                LineSegment line1 = horizontalLines[0];

                if (abs(line1.p1.y - ObjectUtil::Instance().statusBarHeight) < deltaBarHeight)
                {
                    statusBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight);
                }
                else if (abs(line1.p1.y - ObjectUtil::Instance().actionBarHeight) < deltaBarHeight)
                {
                    actionBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().actionBarHeight);
                }
                else //if (abs(line1.p1.y - (ObjectUtil::Instance().statusBarHeight + ObjectUtil::Instance().actionBarHeight)) < deltaBarHeight)
                {
                    statusBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight);
                    actionBarRect = Rect(0, ObjectUtil::Instance().statusBarHeight, frameSize.width, ObjectUtil::Instance().actionBarHeight);
                }

                if (horizontalLines.size() >= 2)
                {
                    LineSegment line2 = horizontalLines[1];
                    if (abs(line2.p1.y - ObjectUtil::Instance().statusBarHeight) < deltaBarHeight)
                    {
                        statusBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight);
                    }
                    else if (abs(line2.p1.y - ObjectUtil::Instance().actionBarHeight) < deltaBarHeight)
                    {
                        actionBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().actionBarHeight);
                    }
                    else // if (abs(line2.p1.y - (ObjectUtil::Instance().statusBarHeight + ObjectUtil::Instance().actionBarHeight)) < deltaBarHeight)
                    {
                        statusBarRect = Rect(0, 0, frameSize.width, ObjectUtil::Instance().statusBarHeight);
                        actionBarRect = Rect(0, ObjectUtil::Instance().statusBarHeight, frameSize.width, ObjectUtil::Instance().actionBarHeight);
                    }
                }
            }
        }

        if (actionBarRect != EmptyRect || statusBarRect != EmptyRect)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    int AndroidBarRecognizer::DetectBottomBarBoundary(const cv::Mat &frame, double minLength, int step)
    {
        int boundaryY = 0;

        cv::Mat gradYFrame;
        cv::Sobel(frame, gradYFrame, CV_32F, 0, 1, 3);

        cv::Mat absGradYFrame;
        cv::convertScaleAbs(gradYFrame, absGradYFrame);
        DebugSaveImage(absGradYFrame, "grad.png");

        cv::Mat grayFrame = BgrToGray(absGradYFrame);
        cv::Mat binaryFrame;
        cv::threshold(grayFrame, binaryFrame, 10, 255, CV_THRESH_BINARY|CV_THRESH_OTSU);
        DebugSaveImage(binaryFrame, "binary.png");

        std::vector<int> rowNonZeroNums;
        for (int i = binaryFrame.rows - 1; i >= -1; i--)
        {
            if (rowNonZeroNums.size() == static_cast<size_t>(step))
            {
                int rowSum = 0;
                for (auto num : rowNonZeroNums)
                {
                    rowSum += num;
                }
                if (FSmaller(minLength, double(rowSum)))
                {
                    boundaryY = i;
                    break;
                }
                else if (i >= 0)
                {
                    rowNonZeroNums.erase(rowNonZeroNums.begin());
                    int sum = cv::countNonZero(binaryFrame.row(i));
                    rowNonZeroNums.push_back(sum);
                }
            }
            else
            {
                int sum = cv::countNonZero(binaryFrame.row(i));
                rowNonZeroNums.push_back(sum);
            }
        }

#ifdef _DEBUG
        if (boundaryY != 0)
        {
            LineSegment line(cv::Point(0, boundaryY), cv::Point(frame.cols - 1, boundaryY));
            cv::Mat debugFrame = frame.clone();
            DrawLine(debugFrame, line, Red, 1);
            SaveImage(debugFrame, "line.png");
        }
#endif

        return boundaryY;
    }

    size_t AndroidBarRecognizer::FindCenteredRect(const std::vector<cv::Rect> &rects, int frameWidth, int delta)
    {
        size_t index = 0;
        for (size_t i = 0; i < rects.size(); i++)
        {
            int centerY = rects[i].x + rects[i].width / 2;
            if (abs(centerY - frameWidth / 2) <= delta)
            {
                index = i;
                break;
            }
        }

        return index;
    }
}

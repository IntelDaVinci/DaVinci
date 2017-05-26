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

#include <algorithm>
#include "boost/filesystem.hpp"
#include "CrossRecognizer.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "TextUtil.hpp"

namespace DaVinci
{
    CrossRecognizer::CrossRecognizer()
    {
        this->objectCategoryName = "Cross";
        this->crossRect = EmptyRect;
        this->crossCenterPoint = EmptyPoint;
    }

    bool CrossRecognizer::RecognizeCross(const cv::Mat &frame)
    {
        bool result = false;
        this->crossRect = EmptyRect;
        this->crossCenterPoint = EmptyPoint;

        for (int i = 0; i < 4; i++) // four corners
        {
            cv::Rect cornerRect;
            cv::Mat cornerFrame = this->CropFrameSpecifiedCorner(frame, CornerPosition(i), cornerRect);
            if (cornerFrame.empty())
            {
                return false;
            }

            cv::Mat hsvFrame;
            cvtColor(cornerFrame, hsvFrame, CV_BGR2HSV);
            std::vector<cv::Mat> hsvChannels;
            split(hsvFrame, hsvChannels);
            cv::Mat vChannel = hsvChannels[2];
            DebugSaveImage(vChannel, "corner.png");

            this->crossRect = this->DetectCrossByEr(vChannel, CornerPosition(i));
            if (this->crossRect != EmptyRect)
            {
                this->crossRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), this->crossRect, cornerRect);
                this->crossCenterPoint = cv::Point(this->crossRect.x + this->crossRect.width / 2, this->crossRect.y + this->crossRect.height / 2);
                result = true;

                break;
            }

            // second, if not found by CCL, use approx points
            /*this->crossRect = this->DetectCrossByApproxPoints(cornerFrame, CornerPosition(i));
            if (this->crossRect != EmptyRect)
            {
            this->crossRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), this->crossRect, cornerRect);
            this->crossCenterPoint = cv::Point(this->crossRect.x + this->crossRect.width / 2, this->crossRect.y + this->crossRect.height / 2);
            result = true;
            break;
            }*/
        }

#ifdef _DEBUG
        if (result)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->crossRect, Red, 2);
            SaveImage(debugFrame, "debug_cross.png");
        }
#endif

        return result;
    }

    bool CrossRecognizer::RecognizeCross(const cv::Mat &frame, const cv::Point &point)
    {
        bool result = false;
        CornerPosition cornerPosition = CornerPosition::RIGHTTOP;
        this->crossRect = EmptyRect;
        this->crossCenterPoint = EmptyPoint;

        if (point.x <= frame.cols / 2)
        {
            if (point.y <= frame.rows / 2)
            {
                cornerPosition = CornerPosition::LEFTTOP;
            }
            else
            {
                cornerPosition = CornerPosition::LEFTBOTTOM;
            }
        }
        else
        {
            if (point.y <= frame.rows / 2)
            {
                cornerPosition = CornerPosition::RIGHTTOP;
            }
            else
            {
                cornerPosition = CornerPosition::RIGHTBOTTOM;
            }
        }

        cv::Rect frameRect(EmptyPoint, frame.size());
        cv::Rect crossROIRect(point.x - 50, point.y - 50, 100, 100);
        crossROIRect &= frameRect;

        cv::Mat crossFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, crossROIRect, crossFrame);
        DebugSaveImage(crossFrame, "cross.png");

        cv::Mat hsvFrame;
        cv::cvtColor(crossFrame, hsvFrame, CV_BGR2HSV);

        std::vector<cv::Mat> hsvChannels;
        cv::split(hsvFrame, hsvChannels);
        cv::Mat vChannel = hsvChannels[2];
        DebugSaveImage(vChannel , "cross.png");

        this->crossRect = this->DetectCrossByEr(vChannel, cornerPosition);

        if (this->crossRect != EmptyRect)
        {
            this->crossRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), this->crossRect, crossROIRect);
            this->crossCenterPoint = cv::Point(this->crossRect.x + this->crossRect.width / 2, this->crossRect.y + this->crossRect.height / 2);
            result = true;
        }

        return result;
    }

    cv::Rect CrossRecognizer::DetectCrossByEr(const cv::Mat &frame, CornerPosition cornerPosition)
    {
        cv::Rect targetRect = EmptyRect;

        MSER er(1,5);
        vector<vector<Point2i>> ers;
        er(frame, ers);
        //may be can be used later for debug
#ifdef _DEBUG
        Mat drawImage = frame;
        if (1 == drawImage.channels())
        {
            cvtColor(drawImage, drawImage, CV_GRAY2BGR);
        }
        vector<Rect> rects;
        for (size_t i=0; i<ers.size();++i)
        {
            Rect rect = boundingRect(ers[i]);
            rects.push_back(rect);
            rectangle(drawImage, rect, Scalar(51,249,40), 2);         
        }
        imwrite("mser.png",drawImage);
#endif

        int erSize = (int)ers.size();
        for (int i = 0; i < erSize; ++i)
        {
            cv::Rect rect = this->CheckSymAndBilinear(frame,ers[i]);
            if (rect != EmptyRect)
            {
                if(!IsText(rect, frame, cornerPosition))
                {
                    targetRect = rect;
                    break;
                }
            }
        }
        return targetRect;
    }

    cv::Rect CrossRecognizer::DetectCrossByApproxPoints(const cv::Mat &frame, CornerPosition cornerPosition)
    {
        cv::Rect targetRect = EmptyRect;

        cv::Mat cloneFrame = frame.clone();
        cv::Mat blurredFrame = MedianBlur(cloneFrame);
        cv::Mat dilatedFrame = Dilation(blurredFrame);
        cv::Mat erosionFrame = Erosion(dilatedFrame);

        cv::Mat grayFrame;
        cv::cvtColor(erosionFrame, grayFrame, CV_BGR2GRAY);

        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, 200, 100);
        DebugSaveImage(cannyFrame, "canny.png");

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(cannyFrame, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

        for (size_t i = 0; i < contours.size(); i++)
        {
            std::vector<cv::Point> approxPoints;
            cv::approxPolyDP(contours[i], approxPoints, 5, true);
            if (approxPoints.size() < 4)
            {
                continue;
            }

            cv::Rect rect = cv::boundingRect(approxPoints);
            if (rect.width >= 2 * rect.height || rect.height >= 2 * rect.width)
            {
                continue;
            }

#ifdef _DEBUG
            for (auto point : approxPoints)
            {
                cv::circle(cloneFrame, point, 1, Blue);
                SaveImage(cloneFrame, "approx.png");
            }
            DrawRect(cloneFrame, rect, Red, 1);
            SaveImage(cloneFrame, "approx.png");
#endif
            std::vector<cv::Point> uniqueApproxPoints = this->GetUniqueApproxPoints(approxPoints);
            std::vector<cv::Point> internalApproxPoints = this->GetInternalApproxPoints(uniqueApproxPoints, rect);
            size_t internalPointsCount = internalApproxPoints.size();

#ifdef _DEBUG
            for (auto point : uniqueApproxPoints)
            {
                cv::circle(cloneFrame, point, 1, Green);
                SaveImage(cloneFrame, "approx.png");
            }

            for (auto point : internalApproxPoints)
            {
                cv::circle(cloneFrame, point, 1, White);
                SaveImage(cloneFrame, "approx.png");
            }
#endif
            // FIXME: normal cross only has 4 internal points
            if (internalPointsCount == 4)
            {
                if (this->IsRhombus(internalApproxPoints) && !this->IsText(rect, frame, cornerPosition))
                {
                    targetRect = rect;
                    break;
                }
            }
        }

        return targetRect;
    }

    cv::Mat CrossRecognizer::CropFrameSpecifiedCorner(const cv::Mat &frame, CornerPosition cornerPosition, cv::Rect &cornerRect)
    {
        int cornerWidth = frame.size().width / 2;
        int cornerHeight = frame.size().height / 2;
        cv::Mat cornerFrame;

        switch(cornerPosition)
        {
        case CornerPosition::RIGHTTOP:
            {
                cornerRect = cv::Rect(cornerWidth, 0, cornerWidth, cornerHeight);
                ObjectUtil::Instance().CopyROIFrame(frame, cornerRect, cornerFrame);
                break;
            }
        case CornerPosition::LEFTTOP:
            {
                cornerRect = cv::Rect(0, 0, cornerWidth, cornerHeight);
                ObjectUtil::Instance().CopyROIFrame(frame, cornerRect, cornerFrame);
                break;
            }
        case CornerPosition::LEFTBOTTOM:
            {
                cornerRect = cv::Rect(0, cornerHeight, cornerWidth, cornerHeight);
                ObjectUtil::Instance().CopyROIFrame(frame, cornerRect, cornerFrame);
                break;
            }
        case CornerPosition::RIGHTBOTTOM:
            {
                cornerRect = cv::Rect(cornerWidth, cornerHeight, cornerWidth, cornerHeight);
                ObjectUtil::Instance().CopyROIFrame(frame, cornerRect, cornerFrame);
                break;
            }
        default:
            {
                cornerFrame = frame;
                break;
            }
        }

        return cornerFrame;
    }

    cv::Rect CrossRecognizer::CheckSymAndBilinear(const cv::Mat &frame, vector<Point2i>& pts, float symThreshold)
    {
        cv::Mat cloneFrame = frame.clone();

        int totalCnt = (int)pts.size();;
        if (totalCnt < 40 || totalCnt > 5000)
        {
            return EmptyRect;
        }

        cv::Rect rect = boundingRect(pts);
        DebugDrawRect(cloneFrame, rect, Red, 2);
        DebugSaveImage(cloneFrame, "rect.png");

        if (rect.height <= 0
            || rect.width <= 0
            || rect.height > 4 * rect.width 
            || rect.width > 4 * rect.height)
        {
            return EmptyRect;
        }

        // Calculate the angle between rect right top point and x axis
        double targetAngle = atan2(rect.height, rect.width) * 180 / CV_PI;
        bool **normalPts = new bool*[rect.height];
        bool **flipVertiPts = new bool*[rect.height];
        bool **flipHonriPts = new bool*[rect.height];

        vector<Point2i> firstQuadrantNormalizedPts;
        for (int i = 0; i < rect.height; ++i)
        {
            flipVertiPts[i] = new bool[rect.width];
            normalPts[i] = new bool[rect.width];
            flipHonriPts[i] = new bool[rect.width];
        }

        for (int i = 0; i < rect.height; ++i)
        {
            for (int j = 0;j < rect.width; ++j)
            {
                flipVertiPts[i][j] = false;
                normalPts[i][j] = false;
                flipHonriPts[i][j] = false;
            }
        }

        int size = (int)pts.size();
        for (int i = 0; i < size; ++i)
        {
            int x = pts[i].x - rect.x;
            int y = pts[i].y - rect.y;
            int flipVy = rect.y + rect.height- 1 -pts[i].y;
            int filpHx = rect.x + rect.width- 1- pts[i].x;
            if (x >= 0 && y>= 0 && x < rect.width && y < rect.height)
            {
                if (x > rect.width / 2 && y < rect.height / 2)
                {
                    firstQuadrantNormalizedPts.push_back(Point2i(x - rect.width /2 , y));
                }
                normalPts[y][x] = true;
            }
            else
            {
                normalPts[y][x] = false;
            }

            //flip V
            if (x >= 0 && flipVy >= 0 && x < rect.width && flipVy < rect.height)
            {
                flipVertiPts[flipVy][x] = true;
            }
            else
            {
                flipVertiPts[flipVy][x] = false;
            }

            //filp H
            if (filpHx >= 0 && y >= 0 && filpHx < rect.width && y < rect.height)
            {
                flipHonriPts[y][filpHx] = true;
            }
            else
            {
                flipHonriPts[y][filpHx] = false;
            }
        }

        cv::Mat binaryImage = cv::Mat::zeros(rect.width, rect.height, CV_8UC1);
        for (int i = 0; i < rect.width; ++i)
        {
            for (int j = 0; j < rect.height; ++j)
            {
                if (normalPts[j][i] == true)
                {
                    binaryImage.at<uchar>(i,j) = 255;
                }
            }
        }
        DebugSaveImage(binaryImage, "test.jpg");
        int centerRowIndex = binaryImage.rows/2;
        Mat centerRow = binaryImage.row(centerRowIndex);
        int centerNonZero = countNonZero(centerRow);
        float centerPointThresh = 0.7;
        if((float)centerNonZero/centerRow.cols > centerPointThresh)
        {
            rect = EmptyRect;
        }
        else
        {
            //check whether symmetric if filp 
            int cntV = 0;
            int cntH = 0;
            for (int i = 0; i < rect.height; ++i)
            {
                for (int j = 0;j < rect.width; ++j)
                {
                    // symmetric by vertical
                    if ((normalPts[i][j] && flipVertiPts[i][j]))
                    {
                        ++cntV;
                    }
                    // symmetric by horizontal
                    if ((normalPts[i][j] && flipHonriPts[i][j]))
                    {
                        ++cntH;
                    }
                }
            }

            if((cntV > totalCnt * symThreshold && cntH > totalCnt * symThreshold)||((totalCnt-cntV)<20 && (totalCnt-cntH)<20))
            {


                if (firstQuadrantNormalizedPts.size() == 0 || !this->CheckBilinear(firstQuadrantNormalizedPts, targetAngle))
                {
                    rect = EmptyRect;
                }
            }
            else
            {
                rect = EmptyRect;
            }
        }
        // release
        for (int i = 0; i < rect.height; ++i)
        {
            delete[] flipVertiPts[i];
            delete[] normalPts[i];
            delete[] flipHonriPts[i];
        }
        delete[] flipVertiPts;
        delete[] normalPts;
        delete[] flipHonriPts;

        return rect;
    }

    bool CrossRecognizer::CheckBilinear(const vector<Point2i> &pts, double targetAngle, float areaThreshold, float angleThreshold)
    {
        cv::RotatedRect rect = minAreaRect(pts);
        float ratio = (float)pts.size() / rect.size.area();
        bool bNotHollow = false;
        if (ratio > areaThreshold)
        {
            bNotHollow = true;
        }

        bool bAngleSatif = false;
        //check angle (MinAreaRect [-90, 0))
        double angle = -rect.angle;
        if (abs(angle - targetAngle) < 20 && angle > 20 && angle < 70)
        {
            bAngleSatif = true;
        }

        bool bDistanceSatif = false;
        double dist = sqrt(rect.center.x * rect.center.x + rect.center.y * rect.center.y);
        double distRatio = dist / rect.size.width * 2;
        if (distRatio < 1.6 && distRatio > 0.5) // 1.5
        {
            bDistanceSatif = true;
        }

        return bNotHollow && bAngleSatif && bDistanceSatif;
    }

    std::vector<cv::Point> CrossRecognizer::GetUniqueApproxPoints(std::vector<cv::Point> approxPoints)
    {
        std::vector<cv::Point> uniqueApproxPoints;

        if (approxPoints.empty())
        {
            return uniqueApproxPoints;
        }

        uniqueApproxPoints.push_back(approxPoints[0]);

        for (auto point : approxPoints)
        {
            size_t count = 0;
            for (auto uniquePoint : uniqueApproxPoints)
            {
                if (!this->AreAdjacentPoints(point, uniquePoint, AdjacentType::XYAdjacent, 2)) // 3
                {
                    count++;
                }
            }
            if (count == uniqueApproxPoints.size())
            {
                uniqueApproxPoints.push_back(point);
            }
        }

        return uniqueApproxPoints;
    }

    std::vector<cv::Point> CrossRecognizer::GetInternalApproxPoints(std::vector<cv::Point> approxPoints, const cv::Rect &boundingRectangle)
    {
        std::vector<cv::Point> internalApproxPoints;
        int interval = static_cast<int>(boundingRectangle.area() * 0.002 + 0.5);

        std::vector<cv::Point> cornerPoints;
        cornerPoints.push_back(cv::Point(boundingRectangle.x, boundingRectangle.y));
        cornerPoints.push_back(cv::Point(boundingRectangle.x, boundingRectangle.y + boundingRectangle.height));
        cornerPoints.push_back(cv::Point(boundingRectangle.x + boundingRectangle.width, boundingRectangle.y + boundingRectangle.height));
        cornerPoints.push_back(cv::Point(boundingRectangle.x + boundingRectangle.width, boundingRectangle.y));

        for (auto approxPoint : approxPoints)
        {
            size_t count = 0;
            for (auto cornerPoint : cornerPoints)
            {
                if (!this->AreAdjacentPoints(approxPoint, cornerPoint, AdjacentType::XAdjacent, interval) 
                    && !this->AreAdjacentPoints(approxPoint, cornerPoint, AdjacentType::YAdjacent, interval))
                {
                    count++;
                }
            }
            if (count == cornerPoints.size())
            {
                internalApproxPoints.push_back(approxPoint);
            }
        }

        return internalApproxPoints;
    }

    bool CrossRecognizer::AreAdjacentPoints(const cv::Point &point1, const cv::Point &point2, AdjacentType adjacentType, int interval)
    {
        bool areAdjacent = false;

        switch(adjacentType)
        {
        case AdjacentType::XYAdjacent:
            {
                if (abs(point1.x - point2.x) <= interval && abs(point1.y - point2.y) <= interval)
                {
                    areAdjacent = true;
                }
                break;
            }
        case AdjacentType::XAdjacent:
            {
                if (abs(point1.x - point2.x) <= interval)
                {
                    areAdjacent = true;
                }
                break;
            }
        case AdjacentType::YAdjacent:
            {
                if (abs(point1.y - point2.y) <= interval)
                {
                    areAdjacent = true;
                }
                break;
            }
        default:
            {
                areAdjacent = false;
                break;
            }
        }

        return areAdjacent;
    }

    // rhombus like this:
    //   *
    // *   *
    //   *
    bool CrossRecognizer::IsRhombus(std::vector<cv::Point> cornerPoints)
    {
        bool fourSidesEquilong = true;
        bool oppositeSidesParallel = false;
        bool twoSidesAngleInRange = true;
        bool diagonalPerpendicular = false;

        std::vector<cv::Point> lrPoints;
        std::vector<cv::Point> tbPoints;

        std::vector<double> sideLengths;
        std::vector<cv::Point> fourSideVectors;

        cv::Point firstHalfMinPoint;
        cv::Point firstHalfMaxPoint;
        cv::Point secondHalfMinPoint;
        cv::Point secondHalfMaxPoint;
        cv::Point minPoint;
        cv::Point maxPoint;

        // group left and right points
        if (cornerPoints[0].x < cornerPoints[1].x)
        {
            firstHalfMinPoint = cornerPoints[0];
            firstHalfMaxPoint = cornerPoints[1];
        }
        else
        {
            firstHalfMinPoint = cornerPoints[1];
            firstHalfMaxPoint = cornerPoints[0];
        }
        if (cornerPoints[2].x < cornerPoints[3].x)
        {
            secondHalfMinPoint = cornerPoints[2];
            secondHalfMaxPoint = cornerPoints[3];
        }
        else
        {
            secondHalfMinPoint = cornerPoints[3];
            secondHalfMaxPoint = cornerPoints[2];
        }
        minPoint = (firstHalfMinPoint.x < secondHalfMinPoint.x ? firstHalfMinPoint : secondHalfMinPoint);
        maxPoint = (firstHalfMaxPoint.x > secondHalfMaxPoint.x ? firstHalfMaxPoint : secondHalfMaxPoint);
        lrPoints.push_back(minPoint);
        lrPoints.push_back(maxPoint);
        if (!AreEqual(minPoint.y, maxPoint.y, 0.00001, 0.1))
        {
            return false;
        }

        // group top and bottom points
        if (cornerPoints[0].y < cornerPoints[1].y)
        {
            firstHalfMinPoint = cornerPoints[0];
            firstHalfMaxPoint = cornerPoints[1];
        }
        else
        {
            firstHalfMinPoint = cornerPoints[1];
            firstHalfMaxPoint = cornerPoints[0];
        }
        if (cornerPoints[2].y < cornerPoints[3].y)
        {
            secondHalfMinPoint = cornerPoints[2];
            secondHalfMaxPoint = cornerPoints[3];
        }
        else
        {
            secondHalfMinPoint = cornerPoints[3];
            secondHalfMaxPoint = cornerPoints[2];
        }
        minPoint = (firstHalfMinPoint.y < secondHalfMinPoint.y ? firstHalfMinPoint : secondHalfMinPoint);
        maxPoint = (firstHalfMaxPoint.y > secondHalfMaxPoint.y ? firstHalfMaxPoint : secondHalfMaxPoint);
        tbPoints.push_back(minPoint);
        tbPoints.push_back(maxPoint);
        if (!AreEqual(minPoint.x, maxPoint.x, 0.00001, 0.1))
        {
            return false;
        }

        // construct four side vectors and calculate side length of four sides
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                cv::Point sideVector = cv::Point(tbPoints[i].x - lrPoints[j].x, tbPoints[i].y - lrPoints[j].y);
                if (!AreEqual(sideVector.x, 0, 2, 0.1) && !AreEqual(sideVector.y, 0, 2, 0.1))
                {
                    fourSideVectors.push_back(sideVector);
                    sideLengths.push_back(sqrt(sideVector.x * sideVector.x + sideVector.y * sideVector.y));
                }
                else
                {
                    return false;
                }
            }
        }

        // check whether four sides are equilong
        for (size_t i = 1; i < 4; i++)
        {
            if (!AreEqual(sideLengths[i], sideLengths[0], 0.00001, 0.3))
            {
                fourSidesEquilong = false;
                break;
            }
        }

        // check whether opposite sides are parallel
        //FIXME: 0.5 may be too large
        if (AreEqual((double)fourSideVectors[0].x / fourSideVectors[3].x, (double)fourSideVectors[0].y / fourSideVectors[3].y, 0.00001, 0.5)
            && AreEqual((double)fourSideVectors[1].x / fourSideVectors[2].x, (double)fourSideVectors[1].y / fourSideVectors[2].y, 0.00001, 0.5))
        {
            oppositeSidesParallel = true;
        }

        // check whether angle between two sides is in range(40, 140)
        for (size_t i = 1; i <= 4; i++)
        {
            int currentIndex = i % 4;
            int previousIndex = (i - 1) % 4;
            int nextIndex = (i + 1) % 4;
            double angle = ComputeAngleByCosineLaw(cornerPoints[currentIndex], cornerPoints[previousIndex], cornerPoints[nextIndex]);
            if (angle <= 40 || angle >= 140)
            {
                twoSidesAngleInRange = false;
                break;
            }
        }

        // check whether two diagonals are perpendicular
        LineSegmentD horizontalLine(lrPoints[0], lrPoints[1]);
        LineSegmentD verticalLine(tbPoints[0], tbPoints[1]);
        cv::Point2d intersectPoint;
        if (ComputeIntersectPoint(horizontalLine, verticalLine, intersectPoint))
        {
            double angle = ComputeAngleByCosineLaw(intersectPoint, lrPoints[0], tbPoints[0]);
            if (AreEqual(angle, 90, 0.00001, 0.5)) //FIXME: 0.5 may be too large
            {
                diagonalPerpendicular = true;
            }
        }

        return fourSidesEquilong && oppositeSidesParallel && twoSidesAngleInRange && diagonalPerpendicular;
    }

    bool CrossRecognizer::IsText(const cv::Rect &rect, const cv::Mat &frame, CornerPosition cornerPosition)
    {
        bool isText = false;

        if ((cornerPosition == CornerPosition::RIGHTTOP && rect.x >= frame.size().width * 2 / 3 && rect.y <= frame.size().height / 3) 
            || (cornerPosition == CornerPosition::LEFTTOP && rect.x <= frame.size().width / 3 && rect.y <= frame.size().height / 3))
        {
            return isText;
        }

        cv::Rect frameRect(EmptyPoint, frame.size());
        cv::Mat textFrame;

        cv::Rect leftRect(rect.x - rect.width * 3 / 2, rect.y - rect.height / 4, rect.width * 3 / 2, rect.height * 3 / 2);
        cv::Rect rightRect(rect.x + rect.width, rect.y, rect.width, rect.height);
        cv::Rect topRect(rect.x, rect.y - rect.height, rect.width, rect.height);
        cv::Rect bottomRect(rect.x, rect.br().y, rect.width, rect.height);

        std::vector<cv::Rect> positionRects;
        positionRects.push_back(leftRect);
        positionRects.push_back(rightRect);
        positionRects.push_back(topRect);
        positionRects.push_back(bottomRect);

        for (auto rect : positionRects)
        {
            rect &= frameRect;
            if (rect == EmptyRect)
            {
                continue;
            }
            ObjectUtil::Instance().CopyROIFrame(frame, rect, textFrame);
            DebugSaveImage(textFrame, "text.png");

            TextRecognize::TextRecognizeResult ocrResult = TextUtil::Instance().AnalyzeEnglishOCR(textFrame);
            string result = ocrResult.GetRecognizedText();
            std::vector<cv::Rect> textRects = ocrResult.GetComponentRects();
            std::vector<float> textConfidences = ocrResult.GetComponentConfidences();

            int meanConfidence = (int)(mean(textConfidences)[0]);
            for (size_t i =0;i<textConfidences.size();++i)
            {
                if (meanConfidence > 65 &&  (result[i] !='(' && result[i]!=')')) // Confidence threshold, because ocr may recognize uncorrectly
                {
                    isText = true;
                    DebugDrawRects(textFrame, textRects, Red, 2);
                    DebugSaveImage(textFrame, "text.png");
                    break;
                }
            }
            if (isText)
            {
                break;
            }
        }

        return isText;
    }

    cv::Rect CrossRecognizer::GetCrossRect()
    {
        return this->crossRect;
    }

    cv::Point CrossRecognizer::GetCrossCenterPoint()
    {
        return this->crossCenterPoint;
    }

    bool CrossRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        cv::Mat image = attributes.image;
        return this->RecognizeCross(image);
    }

    std::string CrossRecognizer::GetCategoryName()
    {
        return this->objectCategoryName;
    }
}
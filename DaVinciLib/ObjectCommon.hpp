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

#ifndef __OBJECT_COMMON_HPP__
#define __OBJECT_COMMON_HPP__

#include "DaVinciCommon.hpp"
#include "Shape.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    inline void DrawLine(cv::Mat &mat, const LineSegment lineS, const Scalar scalar = Red, int thickness = 5)
    {
        line(mat, lineS.p1, lineS.p2, scalar, thickness);
    }

    inline void DrawLines(cv::Mat &mat, const std::vector<LineSegment> lines, const Scalar scalar = Red, int thickness = 5)
    {
        if (!lines.empty())
        {
            for (auto line : lines)
            {
                DrawLine(mat, line, scalar, thickness);
            }
        }
    }

    inline void DrawRect(cv::Mat &mat, const Rect rect, const Scalar scalar = Red, int thickness = 5)
    {
        rectangle(mat, rect, scalar, thickness);
    }

    inline void DrawRects(cv::Mat &mat, const std::vector<cv::Rect> &rects, const Scalar scalar = Red, int thickness = 5)
    {
        for (auto rect : rects)
        {
            rectangle(mat, rect, scalar, thickness);
        }
    }

    inline void DrawCircle(cv::Mat &mat, const Point center, const Scalar scalar = Red, int thickness = 5)
    {
        circle(mat, center, 5, scalar, thickness);
    }

    inline void DrawCircles(cv::Mat &mat, const std::vector<cv::Point> centers, const Scalar scalar = Red, int thickness = 5)
    {
        for (auto center : centers)
        {
            circle(mat, center, 5, scalar, thickness);
        }
    }

    inline void DrawContour(cv::Mat &mat, const vector<Point> contour, const Scalar scalar = Red, int thickness = 5)
    {
        vector<vector<Point>> contours;
        contours.push_back(contour);
        drawContours(mat, contours, 0, scalar, thickness);
    }

    inline void DrawShapes(cv::Mat &mat, const vector<boost::shared_ptr<Shape>> shapes, bool isBoundingRect = false, const Scalar scalar = Red, int thickness = 5)
    {
        for (auto shape : shapes)
        {
            if (isBoundingRect)
            {
                DrawRect(mat, shape->boundRect, scalar, thickness);
            }
            else
            {
                DrawContour(mat, shape->contours, scalar, thickness);
            }
        }
    }

    inline void SaveImage(const cv::Mat &mat, const string &fileName)
    {
        if (!mat.empty())
            imwrite(fileName, mat);
    }

    inline cv::Mat BgrToGray(const cv::Mat &bgrImage)
    {
        cv::Mat grayImage;
        if(bgrImage.channels() == 3)
        {
            cvtColor(bgrImage, grayImage, CV_BGR2GRAY);
        }
        else
        {
            SaveImage(bgrImage, "bad_bgr.png");
            grayImage = Mat::zeros(bgrImage.size(), CV_8UC1);
        }
        return grayImage;
    }

    inline cv::Mat MedianBlur(const cv::Mat &bgrImage, int kernelSize = 3)
    {
        cv::Mat result;
        int doubleKernelSize = (kernelSize << 1);
        int median = doubleKernelSize / 2 +  doubleKernelSize % 2;
        medianBlur(bgrImage, result, median);   
        return result;
    }

    inline cv::Mat Erosion(const cv::Mat &src, int erosionType = 0, int erosionSize = 1, int iterations = 1)
    {
        int type = MORPH_RECT;
        if(erosionType == 0)
        { 
            type = MORPH_RECT; 
        }
        else if(erosionType == 1)
        { 
            type = MORPH_CROSS; 
        }
        else if(erosionType == 2) 
        { 
            type = MORPH_ELLIPSE; 
        }
        cv::Mat target = Mat::zeros(src.size(), src.type());
        cv::Mat element = getStructuringElement(type, Size(2 * erosionSize + 1, 2 * erosionSize + 1), Point(erosionSize, erosionSize));

        erode(src, target, element, cv::Point(-1, -1), iterations);
        return target;
    }

    inline cv::Mat Dilation(const cv::Mat &src, int dilationType = 0, int dilationSize = 1, int iterations = 1)
    {
        int type = MORPH_RECT;
        if(dilationType == 0)
        { 
            type = MORPH_RECT; 
        }
        else if(dilationType == 1)
        { 
            type = MORPH_CROSS; 
        }
        else if(dilationType == 2) 
        { 
            type = MORPH_ELLIPSE; 
        }
        cv::Mat target = Mat::zeros(src.size(), src.type());
        cv::Mat element = getStructuringElement(type, Size(2 * dilationSize + 1, 2 * dilationSize + 1), Point(dilationSize, dilationSize));

        dilate(src, target, element, cv::Point(-1, -1), iterations);
        return target;
    }

    inline cv::Mat AddWeighted(const cv::Mat &frame)
    {
        cv::Mat gradXFrame;
        cv::Mat gradYFrame;
        cv::Mat absGradXFrame;
        cv::Mat absGradYFrame;
        cv::Mat weightedFrame;

        cv::Sobel(frame, gradXFrame, CV_32F, 1, 0, 3);
        cv::convertScaleAbs(gradXFrame, absGradXFrame);

        cv::Sobel(frame, gradYFrame, CV_32F, 0, 1, 3);
        cv::convertScaleAbs(gradYFrame, absGradYFrame);

        cv::addWeighted(absGradXFrame, 0.5, absGradYFrame, 0.5, 0, weightedFrame);

        return weightedFrame;
    }

    inline double ComputeDistance(const Point &p1, const Point &p2)
    {
        int x = p2.x - p1.x;
        int y = p2.y - p1.y;

        double distance = sqrt(pow(x, 2) + pow(y, 2));
        return distance;
    }

    inline cv::Point2d TransformBetweenFrameAndFourthQuadrantPoint(const cv::Point2d &framePoint)
    {
        cv::Point2d quadrantPoint = framePoint;
        if (framePoint.y != 0)
        {
            quadrantPoint.y = -quadrantPoint.y;
        }

        return quadrantPoint;
    }

    /// <summary>
    /// Compute angle between two lines by cosine law ([0, 180])
    /// </summary>
    /// <param name="intersectionPoint">intersection point of two lines</param>
    /// <param name="point1">another point of a line</param>
    /// <param name="point2">another point of another line</param>
    /// <returns>angle</returns>
    inline double ComputeAngleByCosineLaw(const cv::Point &intersectionPoint, const cv::Point &point1, const cv::Point &point2)
    {
        double angle = 0;
        double radian = 0;

        int dx1 = point1.x - intersectionPoint.x;
        int dy1 = point1.y - intersectionPoint.y;
        int dx2 = point2.x - intersectionPoint.x;
        int dy2 = point2.y - intersectionPoint.y;
        int numerator = dx1 * dx2 + dy1 * dy2;
        double denominator = sqrt(dx1 * dx1 + dy1 * dy1) * sqrt(dx2 * dx2 + dy2 * dy2);

        if (denominator == 0)
        {
            angle = 0;
        }
        else
        {
            double cos = (double)numerator / denominator;

            if (cos >= 1)
            {
                angle = 0;
            }
            else if (cos <= -1)
            {
                angle = CV_PI;
            }
            else
            {
                radian = acos(cos);
                angle = radian * 180 / CV_PI;
            }
        }
        return angle;
    }

    inline double ComputeAngle(const Point &p1, const Point &p2)
    {
        cv::Point newP1 = TransformBetweenFrameAndFourthQuadrantPoint(p1);
        cv::Point newP2 = TransformBetweenFrameAndFourthQuadrantPoint(p2);

        int x = newP2.x - newP1.x;
        int y = newP2.y - newP1.y;

        double distance = ComputeDistance(newP1, newP2);

        double cos = x / distance;
        double radian = acos(cos);

        double angle = 180 * radian / CV_PI;

        if (y < 0)
        {
            angle = -angle;
        }

        return angle;
    }

    /// <summary>
    /// Compute angle between line and horizontal line ([0, 180])
    /// These points are for frame
    /// </summary>
    /// <param name="pivotPoint">pivot point, which is on the horizontal line</param>
    /// <param name="point">another point</param>
    /// <returns>angle</returns>
    inline double ComputeAngleForFrame(const Point &pivotPoint, const Point &point)
    {
        // Transform to quadrant point before compute
        cv::Point newPivotPoint = TransformBetweenFrameAndFourthQuadrantPoint(pivotPoint);
        cv::Point newPoint = TransformBetweenFrameAndFourthQuadrantPoint(point);

        cv::Point horizontalPoint(newPivotPoint.x + 10, newPivotPoint.y);
        double angle = ComputeAngleByCosineLaw(newPivotPoint, horizontalPoint, newPoint);

        if (point.y > pivotPoint.y)
        {
            angle = 180 - angle;
        }

        return angle;
    }

    /// <summary>
    /// Compute a line with formula y = kx + b for the fourth quadrant
    /// </summary>
    /// <param name="point">point for frame</param>
    /// <param name="angle">angle, tan(angle) is slope</param>
    /// <returns>line</returns>
    inline LineSegmentD ComputeLineSegmentForQuadrant(const Point &point, const double angle)
    {
        // Transform to quadrant point before compute
        cv::Point newPoint = TransformBetweenFrameAndFourthQuadrantPoint(point);
        double b = newPoint.y - tan(angle * CV_PI / 180 ) * newPoint.x;
        return LineSegmentD(cv::Point2d(newPoint.x, newPoint.y), cv::Point2d(0, b));
    }

    inline bool ComputeIntersectPoint(const LineSegmentD &l1, const LineSegmentD &l2, Point2d &p)
    {
        double a1, b1, c1, a2, b2, c2;
        a1 = l1.p2.y - l1.p1.y;
        b1 = l1.p1.x - l1.p2.x;
        c1 = l1.p2.x * l1.p1.y - l1.p1.x * l1.p2.y;
        a2 = l2.p2.y - l2.p1.y;
        b2 = l2.p1.x - l2.p2.x;
        c2 = l2.p2.x * l2.p1.y - l2.p1.x * l2.p2.y;
        if (FEquals(a1 * b2, b1 * a2)) 
        {
            if (FEquals((a1 + b1) * c2, (a2 + b2) * c1 )) 
            {
                return false; // same line
            } 
            else 
            {
                return false; // parallel
            }
        } 
        else 
        {
            p.x = (b2 * c1 - b1 * c2) / (a2 * b1 - a1 * b2);
            p.y = (a1 * c2 - a2 * c1) / (a2 * b1 - a1 * b2);
            return true;
        }
    }

    inline bool ComputeIntersectPointForFrame(const LineSegmentD &l1, const LineSegmentD &l2, Point2d &p)
    {
        bool isIntersected = ComputeIntersectPoint(l1, l2, p); // Quadrant point after compute
        p = TransformBetweenFrameAndFourthQuadrantPoint(p); // Transform to frame point
        return isIntersected;
    }
	inline void DebugSaveImage(const cv::Mat &mat, const string &fileName)
	{
#ifdef _DEBUG
		SaveImage(mat, fileName);
#endif
	}
	inline void DebugDrawRect(cv::Mat &mat, const Rect rect, const Scalar scalar = Red, int thickness = 5)
	{
#ifdef _DEBUG
		DrawRect(mat, rect, scalar, thickness);
#endif
	}
	inline void DebugDrawRects(cv::Mat &mat, const std::vector<cv::Rect> &rects, const Scalar scalar = Red, int thickness = 5)
	{
#ifdef _DEBUG
		if (!rects.empty())
		{
			for (auto rect : rects)
			{
				DebugDrawRect(mat, rect, scalar, thickness);
			}
		}
#endif
	}

#ifdef _DEBUG
    inline void DebugDrawLine(cv::Mat &mat, const LineSegment lineS, const Scalar scalar = Red, int thickness = 5)
    {
        DrawLine(mat, lineS, scalar, thickness);
    }
#endif
#ifdef _DEBUG
    inline void DebugDrawLines(cv::Mat &mat, const std::vector<LineSegment> lines, const Scalar scalar = Red, int thickness = 5)
    {
        if (!lines.empty())
        {
            for (auto line : lines)
            {
                DebugDrawLine(mat, line, scalar, thickness);
            }
        }
    }
#endif
}

#endif
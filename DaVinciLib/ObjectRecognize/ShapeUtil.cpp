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

#include "ShapeUtil.hpp"

namespace DaVinci
{
    ShapeUtil::ShapeUtil()
    {
    }

    boost::shared_ptr<Shape> ShapeUtil::FindShapeByRect(vector<boost::shared_ptr<Shape>> &shapes, Rect rect)
    {
        for(auto s: shapes)
        {
            if(s->boundRect == rect)
                return s;
        }

        return nullptr;
    }

    bool ShapeUtil::IsValidAlignShapes(vector<Rect> &rects, int minArea)
    {
        for (auto rect: rects)
        {
            if (rect.area() < minArea)
            {
                return false;
            }
        }

        return true;
    }

    vector<boost::shared_ptr<Shape>> ShapeUtil::AlignMaxShapeGroup(vector<boost::shared_ptr<Shape>> &shapes, ShapeAlignOrientation &orientation)
    {
        orientation = ShapeAlignOrientation::UNKNOWN;

        vector<Rect> shapeRects;
        for (auto shape : shapes)
        {
            shapeRects.push_back(shape->boundRect);
        }

        // Align the same row rects
        std::multimap<int, std::vector<cv::Rect>> alignMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(shapeRects, UType::UTYPE_CY, 20, 20, 100);
        int maxRowAlighRectsNumber = INT_MIN;
        int currentAlignRectsNumber = 0;
        vector<Rect> maxRowAlignRects;
        for (std::multimap<int, std::vector<cv::Rect>>::iterator it = alignMap.begin(); it != alignMap.end(); it++)
        {
            std::vector<cv::Rect> rects = it->second;
            currentAlignRectsNumber = (int)rects.size();
            if(currentAlignRectsNumber > maxRowAlighRectsNumber && IsValidAlignShapes(rects, minHighShapeAlignArea))
            {
                maxRowAlighRectsNumber = currentAlignRectsNumber;
                maxRowAlignRects = rects;
            }
        }
        if (maxRowAlighRectsNumber < minShapeAlignSize)
        {
            for (std::multimap<int, std::vector<cv::Rect>>::iterator it = alignMap.begin(); it != alignMap.end(); it++)
            {
                std::vector<cv::Rect> rects = it->second;
                currentAlignRectsNumber = (int)rects.size();
                if(currentAlignRectsNumber > maxRowAlighRectsNumber && IsValidAlignShapes(rects, minLowShapeAlignArea))
                {
                    maxRowAlighRectsNumber = currentAlignRectsNumber;
                    maxRowAlignRects = rects;
                }
            }
        }

        if (maxRowAlighRectsNumber >= minShapeAlignSize)
        {
            int minArea = INT_MAX, maxArea = INT_MIN;
            int minHeight = INT_MAX, maxHeight = INT_MIN;
            for (int index = 0; index < maxRowAlighRectsNumber; index++)
            {
                Rect rect = maxRowAlignRects[index];
                int rectArea = rect.area();

                if (rectArea <= minArea)
                {
                    minArea = rectArea;
                }

                if (rectArea > maxArea)
                {
                    maxArea= rectArea;
                }

                int height = rect.height;

                if (height <= minHeight)
                {
                    minHeight = height;
                }

                if (height > maxHeight)
                {
                    maxHeight= height;
                }
            }

            if (maxArea > 2 * minArea || abs(maxHeight - minHeight) > distanceThreshold)
            {
                maxRowAlighRectsNumber = 0;
                maxRowAlignRects.clear();
            }
        }

        // Align the same column rects
        alignMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(shapeRects, UType::UTYPE_CX, 20, 20, 100, false);
        int maxColumnAlighRectsNumber = INT_MIN;
        currentAlignRectsNumber = 0;
        vector<Rect> maxColumnAlignRects;
        for (std::multimap<int, std::vector<cv::Rect>>::iterator it = alignMap.begin(); it != alignMap.end(); it++)
        {
            std::vector<cv::Rect> rects = it->second;
            currentAlignRectsNumber = (int)rects.size();
            if(currentAlignRectsNumber > maxColumnAlighRectsNumber && IsValidAlignShapes(rects, minHighShapeAlignArea))
            {
                maxColumnAlighRectsNumber = currentAlignRectsNumber;
                maxColumnAlignRects = rects;
            }
        }
        if (maxColumnAlighRectsNumber < minShapeAlignSize)
        {
            for (std::multimap<int, std::vector<cv::Rect>>::iterator it = alignMap.begin(); it != alignMap.end(); it++)
            {
                std::vector<cv::Rect> rects = it->second;
                currentAlignRectsNumber = (int)rects.size();
                if(currentAlignRectsNumber > maxColumnAlighRectsNumber && IsValidAlignShapes(rects, minLowShapeAlignArea))
                {
                    maxColumnAlighRectsNumber = currentAlignRectsNumber;
                    maxColumnAlignRects = rects;
                }
            }
        }

        if (maxColumnAlighRectsNumber >= minShapeAlignSize)
        {
            int minArea = INT_MAX, maxArea = INT_MIN;
            int minWidth = INT_MAX, maxWidth = INT_MIN;
            for (int index = 0; index < maxColumnAlighRectsNumber; index++)
            {
                Rect rect = maxColumnAlignRects[index];
                int rectArea = rect.area();

                if (rectArea <= minArea)
                {
                    minArea = rectArea;
                }

                if (rectArea > maxArea)
                {
                    maxArea= rectArea;
                }

                int height = rect.width;

                if (height <= minWidth)
                {
                    minWidth = height;
                }

                if (height > maxWidth)
                {
                    maxWidth= height;
                }
            }

            if (maxArea > 2 * minArea || abs(maxWidth - minWidth) > distanceThreshold)
            {
                maxColumnAlighRectsNumber = 0;
                maxColumnAlignRects.clear();
            }
        }

        vector<Rect> maxAlignRects;
        vector<boost::shared_ptr<Shape>> maxAlignShapes;
        if (maxColumnAlighRectsNumber >= minShapeAlignSize)
        {
            if (maxRowAlighRectsNumber >= minShapeAlignSize)
            {
                if (maxRowAlighRectsNumber >= maxColumnAlighRectsNumber)
                {
                    maxAlignRects = maxRowAlignRects;
                    orientation = YALIGN;
                }
                else
                {
                    maxAlignRects = maxColumnAlignRects;
                    orientation = XALIGN;
                }
            }
            else
            {
                maxAlignRects = maxColumnAlignRects;
                orientation = XALIGN;
            }
        }
        else
        {
            if (maxRowAlighRectsNumber >= minShapeAlignSize)  
            {
                maxAlignRects = maxRowAlignRects;
                orientation = YALIGN;
            }
        }

        for (auto rect: maxAlignRects)
        {
            maxAlignShapes.push_back(FindShapeByRect(shapes, rect));
        }

        return maxAlignShapes;
    }

    vector<boost::shared_ptr<Shape>> ShapeUtil::ShapesContainPoint(vector<boost::shared_ptr<Shape>> shapes, Point point)
    {
        vector<boost::shared_ptr<Shape>> returnShapes;
        for(auto shape : shapes)
        {
            if(shape->boundRect.contains(point))
                returnShapes.push_back(shape);
        }
        return returnShapes;
    }

    boost::shared_ptr<Shape> ShapeUtil::GetClosestShapeContainedPoint(const std::vector<boost::shared_ptr<Shape>> &shapes, const cv::Point &point)
    {
        boost::shared_ptr<Shape> targetShape = nullptr;
        double minDistance = DBL_MAX;
        double currentDistance = 0.0;

        for (auto shape : shapes)
        {
            if (shape->boundRect.contains(point))
            {
                currentDistance = ComputeDistance(point, shape->center);
                if (FSmallerE(currentDistance, minDistance))
                {
                    minDistance = currentDistance;
                    targetShape = shape;
                }
            }
        }

        return targetShape;
    }
}
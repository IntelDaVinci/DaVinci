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
#include "PointMatcher.hpp"
#include "ObjectUtil.hpp"

namespace DaVinci
{
    PointMatcher::PointMatcher()
    {
    }

    void PointMatcher::SetTargetFrameRect(cv::Rect targetFrameRect)
    {
        this->targetFrameRect = targetFrameRect;
    }

    void PointMatcher::SetSourceReferencePoint(cv::Point sourceReferencePoint)
    {
        this->sourceReferencePoint = sourceReferencePoint;
    }

    void PointMatcher::SetTargetReferencePoint(cv::Point targetReferencePoint)
    {
        this->targetReferencePoint = targetReferencePoint;
    }

    void PointMatcher::SetSourceClickedShape(boost::shared_ptr<Shape> sourceClickedShape)
    {
        this->sourceClickedShape = sourceClickedShape;
    }

    void PointMatcher::SetSourceShapes(std::vector<boost::shared_ptr<Shape>> sourceShapes)
    {
        this->sourceShapes = sourceShapes;
    }

    void PointMatcher::SetTargetShapes(std::vector<boost::shared_ptr<Shape>> targetShapes)
    {
        this->targetShapes = targetShapes;
    }

    void PointMatcher::SetShapePairs(std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> shapePairs)
    {
        this->shapePairs = shapePairs;
    }

    cv::Point PointMatcher::MatchPoint(const cv::Point &sourcePoint, bool &isTriangle, cv::Point &targetTrianglePoint1, cv::Point &targetTrianglePoint2)
    {
        cv::Point targetPoint = EmptyPoint;

        if (this->sourceClickedShape == nullptr)
        {
            // Triangular match by FUZZYRECT
            targetPoint = this->MatchPointByTriangle(sourcePoint, isTriangle, targetTrianglePoint1, targetTrianglePoint2);
        }
        else
        {
            if (MapContainKey(this->sourceClickedShape, this->shapePairs))
            {
                boost::shared_ptr<PairShape> targetShape = this->shapePairs[this->sourceClickedShape];
                targetPoint = this->MatchPointByOffsetRatio(sourcePoint, this->sourceClickedShape->boundRect, targetShape->boundRect);
                isTriangle = false;
                targetTrianglePoint1 = EmptyPoint;
                targetTrianglePoint2 = EmptyPoint;
            }
            else
            {
                // Triangular match by FUZZYRECT
                targetPoint = this->MatchPointByTriangle(sourcePoint, isTriangle, targetTrianglePoint1, targetTrianglePoint2);
            }
        }

        return targetPoint;
    }

    cv::Point PointMatcher::MatchPointByTriangle(const cv::Point &sourcePoint, bool &isTriangle, cv::Point &targetTrianglePoint1, cv::Point &targetTrianglePoint2)
    {
        cv::Point targetPoint = EmptyPoint;

        double sourceDistance = ComputeDistance(sourcePoint, this->sourceReferencePoint);
        double targetDistance = 0.0;
        double deltaDistance = 0.0;

        double sourceAngle = ComputeAngle(this->sourceReferencePoint, sourcePoint);
        double targetAngle = 0.0;
        double deltaAngle = 0.0;

        double totalEvaluation = 0.0;
        double minEvaluation = DBL_MAX;

        std::vector<boost::shared_ptr<Shape>> sourceShapes;
        std::transform(
            this->shapePairs.begin(), 
            this->shapePairs.end(), 
            std::back_inserter(sourceShapes), 
            [](const std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>>::value_type &pair){return pair.first;});

        size_t shapePairsSize = this->shapePairs.size();
        if (shapePairsSize >= 2)
        {
            for (size_t i = 0; i < shapePairsSize - 1; i++)
            {
                for (size_t j = i + 1; j < shapePairsSize; j++)
                {
                    boost::shared_ptr<Shape> sourceShape1 = sourceShapes[i];
                    boost::shared_ptr<Shape> sourceShape2 = sourceShapes[j];
                    boost::shared_ptr<PairShape> targetShape1 = this->shapePairs[sourceShape1];
                    boost::shared_ptr<PairShape> targetShape2 = this->shapePairs[sourceShape2];

                    cv::Point tempPoint = this->LocatePointByTriangleWithRatio(sourcePoint, sourceShape1->center, sourceShape2->center, targetShape1->center, targetShape2->center);
                    if (tempPoint != EmptyPoint && this->targetFrameRect.contains(tempPoint))
                    {
                        targetDistance = ComputeDistance(tempPoint, this->targetReferencePoint);
                        deltaDistance = abs(targetDistance - sourceDistance);

                        targetAngle = ComputeAngle(this->targetReferencePoint, tempPoint);
                        deltaAngle = abs(targetAngle - sourceAngle);

                        totalEvaluation = deltaDistance + deltaAngle;
                        if (FSmallerE(totalEvaluation, minEvaluation))
                        {
                            minEvaluation = totalEvaluation;
                            targetPoint = tempPoint;
                            targetTrianglePoint1 = targetShape1->center;
                            targetTrianglePoint2 = targetShape2->center;
                        }
                    }
                }
            }
        }

        if (targetPoint == EmptyPoint)
        {
            isTriangle = false;
            targetTrianglePoint1 = EmptyPoint;
            targetTrianglePoint2 = EmptyPoint;
        }
        else
        {
            isTriangle = true;
        }

        return targetPoint;
    }

    cv::Point PointMatcher::LocatePointByTriangle(const cv::Point &sourcePoint, const cv::Point &sourcePoint1, const cv::Point &sourcePoint2, const cv::Point &targetPoint1, const cv::Point &targetPoint2)
    {
        cv::Point targetPoint = EmptyPoint;

        double sourceAngle1 = ComputeAngleForFrame(sourcePoint, sourcePoint1);
        double sourceAngle2 = ComputeAngleForFrame(sourcePoint, sourcePoint2);

        LineSegmentD line1 = ComputeLineSegmentForQuadrant(targetPoint1, sourceAngle1);
        LineSegmentD line2 = ComputeLineSegmentForQuadrant(targetPoint2, sourceAngle2);

        cv::Point2d intersectPoint;
        if (ComputeIntersectPointForFrame(line1, line2, intersectPoint))
        {
            targetPoint = cv::Point(static_cast<int>(ceil(intersectPoint.x)), static_cast<int>(ceil(intersectPoint.y)));
        }

        return targetPoint;
    }

    cv::Point PointMatcher::LocatePointByTriangleWithRatio(const cv::Point &sourcePoint, const cv::Point &sourcePoint1, const cv::Point &sourcePoint2, const cv::Point &targetPoint1, const cv::Point &targetPoint2)
    {
        cv::Point targetPoint = EmptyPoint;

        int sourceDeltaX = sourcePoint2.x - sourcePoint1.x;
        int sourceDeltaY = sourcePoint2.y - sourcePoint1.y;
        int targetDeltaX = targetPoint2.x - targetPoint1.x;
        int targetDeltaY = targetPoint2.y - targetPoint1.y;

        double deltaXRatio = 1.0;
        if (sourceDeltaX != 0 && targetDeltaX != 0)
        {
            deltaXRatio = (double)targetDeltaX / sourceDeltaX;
        }

        double deltaYRatio = 1.0;
        if (sourceDeltaY != 0 && targetDeltaY != 0)
        {
            deltaYRatio = (double)targetDeltaY / sourceDeltaY;
        }

        cv::Point newTargetPoint1(int(targetPoint1.x / deltaXRatio), int(targetPoint1.y / deltaYRatio));
        cv::Point newTargetPoint2(int(targetPoint2.x / deltaXRatio), int(targetPoint2.y / deltaYRatio));

        targetPoint = this->LocatePointByTriangle(sourcePoint, sourcePoint1, sourcePoint2, newTargetPoint1, newTargetPoint2);
        if (targetPoint != EmptyPoint)
        {
            targetPoint.x = int(targetPoint.x * deltaXRatio);
            targetPoint.y = int(targetPoint.y * deltaYRatio);
        }

        return targetPoint;
    }

    cv::Point PointMatcher::MatchPointByOffsetRatio(const cv::Point &sourcePoint, const cv::Rect &sourceShapeRect, const cv::Rect &targetShapeRect)
    {
        cv::Point targetPoint = EmptyPoint;

        int deltaX = sourcePoint.x - sourceShapeRect.x;
        int deltaY = sourcePoint.y - sourceShapeRect.y;

        targetPoint.x = targetShapeRect.x + int(deltaX * targetShapeRect.width / double(sourceShapeRect.width));
        targetPoint.y = targetShapeRect.y + int(deltaY * targetShapeRect.height / double(sourceShapeRect.height));

        return targetPoint;
    }

    cv::Point PointMatcher::MatchPointByStretch(const cv::Point &sourcePoint, const cv::Rect &sourceMainROI, const cv::Rect &targetMainROI)
    {
        Point pointOnMainROI = ObjectUtil::Instance().RelocatePointByOffset(sourcePoint, sourceMainROI, false);
        cv::Point targetPoint;

        targetPoint.x = int(pointOnMainROI.x * targetMainROI.width / (double)sourceMainROI.width);
        targetPoint.y = int(pointOnMainROI.y * targetMainROI.height / (double)sourceMainROI.height);

        targetPoint = ObjectUtil::Instance().RelocatePointByOffset(targetPoint, targetMainROI, true);
        return targetPoint;
    }

    cv::Point PointMatcher::MatchPointByDPI(const cv::Point &sourcePointOnDevice, const Size &sourceDeviceSize, const Size &targetDeviceSize, double sourceDPI, double targetDPI)
    {
        cv::Point targetPointOnDevice;

        double normalizedTargetDPI = targetDPI * sourceDeviceSize.height / targetDeviceSize.height;
        targetPointOnDevice.x = (int)(sourcePointOnDevice.x * normalizedTargetDPI / sourceDPI);
        targetPointOnDevice.y = (int)(sourcePointOnDevice.y * normalizedTargetDPI / sourceDPI);

        return targetPointOnDevice;
    }
}
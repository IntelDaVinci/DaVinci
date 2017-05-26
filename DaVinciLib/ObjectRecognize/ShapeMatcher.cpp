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
#include "ObjectCommon.hpp"
#include "ShapeMatcher.hpp"

#include <opencv2/shape/shape_distance.hpp>

#include <bitset>

namespace DaVinci
{
    ShapeMatcher::ShapeMatcher(): shapeMatchLevel(ShapeMatchLevel::FINE_MATCH)
    {
        this->SetShapeMatchThreshold(shapeMatchLevel);

        this->shapeContext = createShapeContextDistanceExtractor();
    }

    void ShapeMatcher::SetSourceReferencePoint(cv::Point sourceReferencePoint)
    {
        this->sourceReferencePoint = sourceReferencePoint;
    }

    void ShapeMatcher::SetTargetReferencePoint(cv::Point targetReferencePoint)
    {
        this->targetReferencePoint = targetReferencePoint;
    }

    void ShapeMatcher::SetSourceShapes(std::vector<boost::shared_ptr<Shape>> sourceShapes)
    {
        this->sourceShapes = sourceShapes;
    }

    void ShapeMatcher::SetTargetShapes(std::vector<boost::shared_ptr<Shape>> targetShapes)
    {
        this->targetShapes = targetShapes;
    }

    void ShapeMatcher::SetShapeMatchThreshold(ShapeMatchLevel shapeMatchLevel)
    {
        switch(shapeMatchLevel)
        {
        case ShapeMatchLevel::FINE_MATCH:
            {
                this->shapeMatchNumberThreshold = 0.5;
                this->shapeMatchAreaThreshold = 0.25;
                this->shapeMatchAngleThreshold = 0.05;
                this->shapeMatchDistanceThreshold = 0.05;
                this->shapeMatchAspectRatioThreshold = 0.25;
                this->shapeMatchXYRatioThreshold = 0.5;
                this->histogramSimilarity = 0.2;

                this->shapeDistanceAbsoluteThreshold = 30.0;
                this->shapeAngleAbsoluteThreshold = 15.0;

                this->alignShapeXYOffsetThreshold = 125;
                this->alignShapeGroupLengthRatioThreshold = 0.5;
            }
            break;
        case ShapeMatchLevel::MIDDLE_MATCH:
            {
                this->shapeMatchNumberThreshold = 0.5;
                this->shapeMatchAreaThreshold = 0.25;
                this->shapeMatchAngleThreshold = 0.15;
                this->shapeMatchDistanceThreshold = 0.15;
                this->shapeMatchAspectRatioThreshold = 0.25;
                this->shapeMatchXYRatioThreshold = 0.5;
                this->histogramSimilarity = 0.2;

                this->shapeDistanceAbsoluteThreshold = 30.0;
                this->shapeAngleAbsoluteThreshold = 15.0;

                this->alignShapeXYOffsetThreshold = 125;
                this->alignShapeGroupLengthRatioThreshold = 0.5;
            }
            break;
        case ShapeMatchLevel::COARSE_MATCH:
            {
                this->shapeMatchNumberThreshold = 0.5;
                this->shapeMatchAreaThreshold = 0.25;
                this->shapeMatchAngleThreshold = 0.25;
                this->shapeMatchDistanceThreshold = 0.25;
                this->shapeMatchAspectRatioThreshold = 0.25;
                this->shapeMatchXYRatioThreshold = 0.5;
                this->histogramSimilarity = 0.2;

                this->shapeDistanceAbsoluteThreshold = 30.0;
                this->shapeAngleAbsoluteThreshold = 15.0;

                this->alignShapeXYOffsetThreshold = 125;
                this->alignShapeGroupLengthRatioThreshold = 0.5;
            }
            break;
        default:
            {
                this->shapeMatchNumberThreshold = 0.5;
                this->shapeMatchAreaThreshold = 0.25;
                this->shapeMatchAngleThreshold = 0.05;
                this->shapeMatchDistanceThreshold = 0.05;
                this->shapeMatchAspectRatioThreshold = 0.25;
                this->shapeMatchXYRatioThreshold = 0.5;
                this->histogramSimilarity = 0.2;

                this->shapeDistanceAbsoluteThreshold = 30.0;
                this->shapeAngleAbsoluteThreshold = 15.0;

                this->alignShapeXYOffsetThreshold = 125;
                this->alignShapeGroupLengthRatioThreshold = 0.5;
            }
            break;
        }
    }

    std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> ShapeMatcher::MatchFuzzyShapePairs()
    {
        std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> shapePairs;

        for (size_t i = 0; i < this->sourceShapes.size(); i++)
        {
            boost::shared_ptr<Shape> sourceShape = this->sourceShapes[i];

            for (size_t j = 0; j < this->targetShapes.size(); j++)
            {
                boost::shared_ptr<Shape> targetShape = this->targetShapes[j];
                boost::shared_ptr<PairShape> targetPairShape = nullptr;
                bool isShapeOutMatched = this->MatchSingleShape(sourceShape, targetShape, targetPairShape, ShapeMatchType::FUZZYRECT);

                if (isShapeOutMatched)
                {
                    if (!MapContainKey(sourceShape, shapePairs) || (MapContainKey(sourceShape, shapePairs) && this->IsCloserPairShape(targetPairShape, shapePairs[sourceShape])))
                    {
                        shapePairs[sourceShape] = targetPairShape;
                    }
                }
            }
        }

        return shapePairs;
    }

    bool ShapeMatcher::MatchSingleShape(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape, boost::shared_ptr<PairShape> &targetPairShape, ShapeMatchType shapeMatchType)
    {
        bool isMatched = false;

        if (sourceShape == nullptr || targetShape == nullptr)
        {
            return isMatched;
        }

        if (shapeMatchType == ShapeMatchType::NUMBER)
        {
            isMatched = true;
        }
        else if (shapeMatchType == ShapeMatchType::AREA)
        {
            double ratio = this->MeasureAreaRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchAreaThreshold);

            if (isMatched)
            {
                targetPairShape->areaRatio = ratio;
            }
        }
        else if (shapeMatchType == ShapeMatchType::DISTANCE)
        {
            double ratio = this->MeasureDistanceRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchDistanceThreshold);

            if (isMatched)
            {
                targetPairShape->distanceRatio = ratio;
            }
        }
        else if (shapeMatchType == ShapeMatchType::ANGLE)
        {
            double ratio = this->MeasureAngleRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchAngleThreshold);

            if (isMatched)
            {
                targetPairShape->angleRatio = ratio;
            }
        }
        else if (shapeMatchType == ShapeMatchType::ASPECTRATIO)
        {
            double ratio = this->MeasureAspectRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchAspectRatioThreshold);

            if (isMatched)
            {
                targetPairShape->aspectRatio = ratio;
            }
        }
        else if (shapeMatchType == ShapeMatchType::XLOCATION)
        {
            isMatched = true;
            /*double ratio = this->MeasureXLocationRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchXYRatioThreshold);*/
        }
        else if (shapeMatchType == ShapeMatchType::YLOCATION)
        {
            isMatched = true;
            /*double ratio = this->MeasureYLocationRatio(sourceShape, targetShape);
            isMatched = FSmallerE(ratio, this->shapeMatchXYRatioThreshold);*/
        }
        else if (shapeMatchType == ShapeMatchType::CONTEXT)
        {
            isMatched = true;
        }
        else
        {
            targetPairShape = boost::shared_ptr<PairShape>(new PairShape());
            targetPairShape->center = targetShape->center;
            targetPairShape->contours = targetShape->contours;
            targetPairShape->boundRect = targetShape->boundRect;

            bitset<ShapeMatchTypeNumber> bits(shapeMatchType);
            for (int i = 0; i < ShapeMatchTypeNumber; i++)
            {
                if (bits[i] == 1)
                {
                    isMatched = this->MatchSingleShape(sourceShape, targetShape, targetPairShape, this->IntToShapeMatchType(1 << i));
                    if (!isMatched)
                    {
                        break;
                    }
                }
            }
        }

        return isMatched;
    }

    bool ShapeMatcher::MatchAlignShapeGroup(const std::vector<boost::shared_ptr<Shape>> &sourceAlignShapes, const std::vector<boost::shared_ptr<Shape>> &targetAlignShapes, ShapeAlignOrientation sourceAlignOrientation, ShapeAlignOrientation targetAlignOrientation)
    {
        bool isMatched = false;

        size_t sourceSize = sourceAlignShapes.size();
        size_t targetSize = targetAlignShapes.size();

        if (sourceAlignOrientation == targetAlignOrientation && sourceSize == targetSize && sourceSize != 0)
        {
            // FIXME: sorted
            if (sourceAlignOrientation == ShapeAlignOrientation::XALIGN)
            {
                if (abs(sourceAlignShapes[0]->boundRect.x - targetAlignShapes[0]->boundRect.x) <= this->alignShapeXYOffsetThreshold
                    && abs(sourceAlignShapes[0]->boundRect.y - targetAlignShapes[0]->boundRect.y) <= this->alignShapeXYOffsetThreshold)
                {
                    int sourceAlignDistance = sourceAlignShapes[sourceSize - 1]->boundRect.y - sourceAlignShapes[0]->boundRect.y;
                    int targetAlignDistance = targetAlignShapes[targetSize - 1]->boundRect.y - targetAlignShapes[0]->boundRect.y;
                    int deltaDistance = abs(sourceAlignDistance - targetAlignDistance);
                    double deltaRatio = (double)deltaDistance / sourceAlignDistance;
                    if (!FSmallerE(this->alignShapeGroupLengthRatioThreshold, deltaRatio))
                    {
                        isMatched = true;
                    }
                }
            }
            else if (sourceAlignOrientation == ShapeAlignOrientation::YALIGN)
            {
                if (abs(sourceAlignShapes[0]->boundRect.x - targetAlignShapes[0]->boundRect.x) <= this->alignShapeXYOffsetThreshold
                    && abs(sourceAlignShapes[0]->boundRect.y - targetAlignShapes[0]->boundRect.y) <= this->alignShapeXYOffsetThreshold)
                {
                    int sourceAlignDistance = sourceAlignShapes[sourceSize - 1]->boundRect.x - sourceAlignShapes[0]->boundRect.x;
                    int targetAlignDistance = targetAlignShapes[targetSize - 1]->boundRect.x - targetAlignShapes[0]->boundRect.x;
                    int deltaDistance = abs(sourceAlignDistance - targetAlignDistance);
                    double deltaRatio = (double)deltaDistance / sourceAlignDistance;
                    if (!FSmallerE(this->alignShapeGroupLengthRatioThreshold, deltaRatio))
                    {
                        isMatched = true;
                    }
                }
            }
        }

        return isMatched;
    }

    double ShapeMatcher::MeasureAreaRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double areaDelta = abs(sourceShape->area - targetShape->area);
        double areaRatio = areaDelta / (sourceShape->area + targetShape->area);

        return areaRatio;
    }

    double ShapeMatcher::MeasureDistanceRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double sourceDistance = ComputeDistance(sourceShape->center, this->sourceReferencePoint);
        double targetDistance = ComputeDistance(targetShape->center, this->targetReferencePoint);

        if (FEquals(sourceDistance, 0.0))
        {
            if (FSmaller(targetDistance, this->shapeDistanceAbsoluteThreshold))
            {
                return 0.0;
            }
            else
            {
                return 1.0;
            }
        }

        double distanceDelta = abs(sourceDistance - targetDistance);
        double distanceRatio = distanceDelta / (sourceDistance + targetDistance);

        return distanceRatio;
    }

    double ShapeMatcher::MeasureAngleRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double sourceAngle = ComputeAngle(this->sourceReferencePoint, sourceShape->center);
        double targetAngle = ComputeAngle(this->targetReferencePoint, targetShape->center);

        if (FEquals(sourceAngle, 0.0))
        {
            if (FSmaller(abs(targetAngle), this->shapeAngleAbsoluteThreshold))
            {
                return 0.0;
            }
            else
            {
                return 1.0;
            }
        }
        if (FEquals(targetAngle, 0.0))
        {
            if (FSmaller(abs(sourceAngle), this->shapeAngleAbsoluteThreshold))
            {
                return 0.0;
            }
            else
            {
                return 1.0;
            }
        }

        if (FEquals(sourceAngle, 180.0) && FSmaller(targetAngle, 0.0))
        {
            sourceAngle = -sourceAngle;
        }
        if (FEquals(targetAngle, 180.0) && FSmaller(sourceAngle, 0.0))
        {
            targetAngle = -targetAngle;
        }

        double angleDelta = abs(sourceAngle - targetAngle);
        double angleRatio = angleDelta / abs(sourceAngle + targetAngle);

        return angleRatio;
    }

    double ShapeMatcher::MeasureAspectRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double sAspectRatio = (double)sourceShape->boundRect.width / sourceShape->boundRect.height;
        double tAspectRatio = (double)targetShape->boundRect.width / targetShape->boundRect.height;

        double aspectDelta = abs(sAspectRatio - tAspectRatio);
        double aspectRatio = aspectDelta / (sAspectRatio + tAspectRatio);

        return aspectRatio;
    }

    double ShapeMatcher::MeasureXLocationRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double xLocationDelta = abs(sourceShape->boundRect.x - targetShape->boundRect.x);
        double xLocationRatio = xLocationDelta / sourceShape->boundRect.x;
        return xLocationRatio;
    }

    double ShapeMatcher::MeasureYLocationRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape)
    {
        double yLocationDelta = abs(sourceShape->boundRect.y - targetShape->boundRect.y);
        double yLocationRatio = yLocationDelta / sourceShape->boundRect.y;
        return yLocationRatio;
    }

    ShapeMatchType ShapeMatcher::IntToShapeMatchType(int shapeMatchType)
    {
        ShapeMatchType matchType;

        if (shapeMatchType == (1 << 0))
        {
            matchType = ShapeMatchType::NUMBER;
        }
        else if (shapeMatchType == (1 << 1))
        {
            matchType = ShapeMatchType::AREA;
        }
        else if (shapeMatchType == (1 << 2))
        {
            matchType = ShapeMatchType::DISTANCE;
        }
        else if (shapeMatchType == (1 << 3))
        {
            matchType = ShapeMatchType::ANGLE;
        }
        else if (shapeMatchType == (1 << 4))
        {
            matchType = ShapeMatchType::ASPECTRATIO;
        }
        else if (shapeMatchType == (1 << 5))
        {
            matchType = ShapeMatchType::XLOCATION;
        }
        else if (shapeMatchType == (1 << 6))
        {
            matchType = ShapeMatchType::YLOCATION;
        }
        else
        {
            matchType = ShapeMatchType::CONTEXT;
        }

        return matchType;
    }

    bool ShapeMatcher::IsCloserPairShape(const boost::shared_ptr<PairShape> &pairShape1, const boost::shared_ptr<PairShape> &pairShape2)
    {
        bool isCloser = false;

        if (FSmaller(pairShape1->angleRatio, pairShape2->angleRatio))
        {
            isCloser = true;
        }

        return isCloser;
    }
}
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

#ifndef __POINTMATCHER__
#define __POINTMATCHER__

#include "Shape.hpp"
#include "ShapeMatcher.hpp"

#include "boost/smart_ptr/shared_ptr.hpp"

namespace DaVinci
{
    using namespace cv;
    using namespace std;

    class PointMatcher
    {
    private:
        cv::Rect targetFrameRect;

        cv::Point sourceReferencePoint;
        cv::Point targetReferencePoint;

        boost::shared_ptr<Shape> sourceClickedShape;

        std::vector<boost::shared_ptr<Shape>> sourceShapes;
        std::vector<boost::shared_ptr<Shape>> targetShapes;

        std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> shapePairs;

    public:
        PointMatcher();

        void SetTargetFrameRect(cv::Rect targetFrameRect);

        void SetSourceReferencePoint(cv::Point sourceReferencePoint);

        void SetTargetReferencePoint(cv::Point targetReferencePoint);

        void SetSourceClickedShape(boost::shared_ptr<Shape> sourceClickedShape);

        void SetSourceShapes(std::vector<boost::shared_ptr<Shape>> sourceShapes);

        void SetTargetShapes(std::vector<boost::shared_ptr<Shape>> targetShapes);

        void SetShapePairs(std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> shapePairs);

        cv::Point MatchPoint(const cv::Point &sourcePoint, bool &isTriangle, cv::Point &targetTrianglePoint1, cv::Point &targetTrianglePoint2);

        cv::Point MatchPointByTriangle(const cv::Point &sourcePoint, bool &isTriangle, cv::Point &targetTrianglePoint1, cv::Point &targetTrianglePoint2);

        cv::Point LocatePointByTriangle(const cv::Point &sourcePoint, const cv::Point &sourcePoint1, const cv::Point &sourcePoint2, const cv::Point &targetPoint1, const cv::Point &targetPoint2);

        cv::Point LocatePointByTriangleWithRatio(const cv::Point &sourcePoint, const cv::Point &sourcePoint1, const cv::Point &sourcePoint2, const cv::Point &targetPoint1, const cv::Point &targetPoint2);

        cv::Point MatchPointByOffsetRatio(const cv::Point &sourcePoint, const cv::Rect &sourceShapeRect, const cv::Rect &targetShapeRect);

        cv::Point MatchPointByStretch(const cv::Point &sourcePoint, const cv::Rect &sourceMainROI, const cv::Rect &targetMainROI);

        cv::Point MatchPointByDPI(const cv::Point &sourcePoint, const Size &sourceDeviceSize, const Size &targetDeviceSize, double sourceDPI, double targetDPI);
    };
}

#endif
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

#ifndef __SHAPEMATCHER__
#define __SHAPEMATCHER__

#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "Shape.hpp"
#include "ShapeExtractor.hpp"
#include "ShapeUtil.hpp"

#include "boost/smart_ptr/shared_ptr.hpp"

#include "opencv2/shape/shape.hpp"
#include "opencv2/shape/shape_distance.hpp"

#include <vector>

namespace DaVinci
{
    using namespace cv;
    using namespace std;

    enum ShapeMatchType
    {
        NUMBER = 1 << 0,
        AREA = 1 << 1,
        DISTANCE = 1 << 2,
        ANGLE = 1 << 3,
        ASPECTRATIO = 1 << 4,
        XLOCATION = 1 << 5,
        YLOCATION = 1 << 6,
        CONTEXT = 1 << 7,
        FUZZYPOINT = DISTANCE | ANGLE | XLOCATION | YLOCATION,
        FUZZYTEXT = AREA | ASPECTRATIO,
        FUZZYRECT = AREA | DISTANCE | ANGLE | ASPECTRATIO | XLOCATION | YLOCATION, 
        FULL = NUMBER | AREA | DISTANCE | ANGLE | ASPECTRATIO | XLOCATION | YLOCATION | CONTEXT
    };

    enum ShapeMatchLevel
    {
        COARSE_MATCH,
        MIDDLE_MATCH,
        FINE_MATCH
    };

    struct PairShape
    {
        cv::Point center;
        std::vector<cv::Point> contours;
        cv::Rect boundRect;
        double areaRatio;
        double distanceRatio;
        double angleRatio;
        double aspectRatio;
    };

    /// <summary>
    /// Class for ShapeMatcher
    /// </summary>
    class ShapeMatcher
    {
    private:
        cv::Point sourceReferencePoint;
        cv::Point targetReferencePoint;

        std::vector<boost::shared_ptr<Shape>> sourceShapes;
        std::vector<boost::shared_ptr<Shape>> targetShapes;

        ShapeMatchLevel shapeMatchLevel;

        double shapeMatchNumberThreshold;
        double shapeMatchAreaThreshold;
        double shapeMatchAngleThreshold;
        double shapeMatchDistanceThreshold;
        double shapeMatchAspectRatioThreshold;
        double shapeMatchXYRatioThreshold;
        double histogramSimilarity;

        double shapeDistanceAbsoluteThreshold;
        double shapeAngleAbsoluteThreshold;

        int alignShapeXYOffsetThreshold;
        double alignShapeGroupLengthRatioThreshold;

        boost::shared_ptr<ShapeExtractor> shapeExtractor;
        cv::Ptr<cv::ShapeContextDistanceExtractor> shapeContext;

    public:
        static const int ShapeMatchTypeNumber = 8; // Update IntToShapeMatchType
        static const int SourceShapeSize = 10; // Source shape size
        static const int ShapeDistancThreshold = 30; // Allow MSER shape extraction threshold

    public:
        ShapeMatcher();

        void SetSourceReferencePoint(cv::Point sourceReferencePoint);

        void SetTargetReferencePoint(cv::Point targetReferencePoint);

        void SetSourceShapes(std::vector<boost::shared_ptr<Shape>> sourceShapes);

        void SetTargetShapes(std::vector<boost::shared_ptr<Shape>> targetShapes);

        void SetShapeMatchThreshold(ShapeMatchLevel shapeMatchLevel);

        std::unordered_map<boost::shared_ptr<Shape>, boost::shared_ptr<PairShape>> MatchFuzzyShapePairs();

        bool MatchSingleShape(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape, boost::shared_ptr<PairShape> &targetPairShape, ShapeMatchType shapeMatchType);

        bool MatchAlignShapeGroup(const std::vector<boost::shared_ptr<Shape>> &sourceAlignShapes, const std::vector<boost::shared_ptr<Shape>> &targetAlignShapes, ShapeAlignOrientation sourceAlignOrientation, ShapeAlignOrientation targetAlignOrientation);

        double MeasureAreaRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        double MeasureDistanceRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        double MeasureAngleRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        double MeasureAspectRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        double MeasureXLocationRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        double MeasureYLocationRatio(const boost::shared_ptr<Shape> &sourceShape, const boost::shared_ptr<Shape> &targetShape);

        ShapeMatchType IntToShapeMatchType(int shapeMatchType);

        /// <summary>
        /// pairShape1 is closer than pairShape2 to source shape
        /// </summary>
        /// <param name="pairShape1"></param>
        /// <param name="pairShape2"></param>
        /// <returns></returns>
        bool IsCloserPairShape(const boost::shared_ptr<PairShape> &pairShape1, const boost::shared_ptr<PairShape> &pairShape2);
    };
}


#endif	//#ifndef __SHAPEMATCHER__

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

#ifndef __SHAPEUTIL__
#define __SHAPEUTIL__ 

#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "CommonKeywordRecognizer.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "Shape.hpp"

namespace DaVinci
{
    class ShapeUtil : public SingletonBase<ShapeUtil>
    {
        SIGLETON_CHILD_CLASS(ShapeUtil);
    public:
        // Minimum shape align size (at least three shapes for one group)
        static const int minShapeAlignSize = 3;
        // Shape distance threshold 
        static const int distanceThreshold = 120;
        // Minimum shape align area (at least 10000 points)
        static const int minHighShapeAlignArea = 10000;
        static const int minLowShapeAlignArea = 2000;

    public:
        vector<boost::shared_ptr<Shape>> AlignMaxShapeGroup(vector<boost::shared_ptr<Shape>> &shapes, ShapeAlignOrientation &orientation);

        vector<boost::shared_ptr<Shape>> ShapesContainPoint(vector<boost::shared_ptr<Shape>> shapes, Point point);

        boost::shared_ptr<Shape> GetClosestShapeContainedPoint(const std::vector<boost::shared_ptr<Shape>> &shapes, const cv::Point &point);

    private:
        bool IsValidAlignShapes(vector<Rect> &shapes, int minArea);
        boost::shared_ptr<Shape> FindShapeByRect(vector<boost::shared_ptr<Shape>> &shapes, Rect rect);
    };
}

#endif
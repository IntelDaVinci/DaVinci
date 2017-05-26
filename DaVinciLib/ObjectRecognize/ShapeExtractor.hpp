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

#ifndef __SHAPEEXTRACTOR__
#define __SHAPEEXTRACTOR__

#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <unordered_map>

#include "opencv2/core/core.hpp"
#include "DaVinciCommon.hpp"
#include "Shape.hpp"

#include "boost/geometry/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "AndroidBarRecognizer.hpp"
#include "SceneTextExtractor.hpp"

namespace DaVinci
{
    using namespace cv;
    using namespace std;

    typedef boost::geometry::model::point<float, 2, boost::geometry::cs::cartesian> gPoint;
    typedef std::pair<gPoint, size_t> gValue;

    enum ShapeExtractionLevel
    {
        COARSE_EXTRACT,
        MIDDLE_EXTRACT,
        FINE_EXTRACT,
        SMALL_EXTRACT
    };

    /// <summary>
    /// Class for ShapeExtractor
    /// </summary>
    class ShapeExtractor
    {
    public:
        ShapeExtractor();

        /// <summary>
        /// Extract shapes
        /// </summary>
        /// <param name="frame"></param>
        /// <returns></returns>
        void Extract(ShapeExtractionLevel level = ShapeExtractionLevel::MIDDLE_EXTRACT);

        vector<Point> FindNearestKPoints(vector<Point> &points, Point point, size_t nearK = 2);
        vector<int> FindNearestKLocations(vector<Point> &points, Point point, size_t nearK = 2);

        vector<boost::shared_ptr<Shape>> FindNearestKShapes(Point point, size_t nearK = 2);

        vector<gValue> FindNearestKPointsInternal(const vector<Point>& points, Point point, size_t nearK = 2);

        vector<boost::shared_ptr<Shape>> GetShapes();

        void SetShapes(const vector<boost::shared_ptr<Shape>>& shapes);

        void SetFrame(const cv::Mat& frame);

        void SetMinArea(int minArea);

        void SetMaxArea(int maxArea);

        void SetEdgeOffset(int edgeOffet);

        void SetNavigationBarRect(Rect navigationBarRect);

        bool ContourContainPoint(const vector<Point>& points, Point point, int interval = 3);

        boost::shared_ptr<Shape> TransformContourToShape(const vector<Point>& contour);

        boost::shared_ptr<Shape> ContourToShape(const vector<Point>& contour);

        boost::shared_ptr<Shape> BoundingRectToShape(Rect boundRect);

        bool IsBoundRectCloseToTwoFrameEdges(Size frameSize, Rect boundRect);
    private:
        cv::Mat frame;
        int minArea;
        int maxArea;
        int edgeOffset;
        vector<boost::shared_ptr<Shape>> shapes;
        Rect navigationBarRect;
        boost::shared_ptr<SceneTextExtractor> sceneTextExtractor;
    };
}
#endif	//#ifndef __SHAPEEXTRACTOR__

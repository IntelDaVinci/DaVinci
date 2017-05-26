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

#include "ShapeExtractor.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    ShapeExtractor::ShapeExtractor() : minArea(2000), edgeOffset(5), maxArea(INT32_MAX)
    {
        sceneTextExtractor = boost::shared_ptr<SceneTextExtractor>(new SceneTextExtractor());
    }

    void ShapeExtractor::SetFrame(const cv::Mat& frame)
    {
        this->frame = frame;
    }

    vector<boost::shared_ptr<Shape>> ShapeExtractor::GetShapes()
    {
        return shapes;
    }

    void ShapeExtractor::SetShapes(const vector<boost::shared_ptr<Shape>>& shapes)
    {
        this->shapes = shapes;
    }

    void ShapeExtractor::SetMinArea(int minArea)
    {
        this->minArea = minArea;
    }

    void ShapeExtractor::SetEdgeOffset(int edgeOffset)
    {
        this->edgeOffset = edgeOffset;
    }

    void ShapeExtractor::SetMaxArea(int maxArea)
    {
        this->maxArea = maxArea;
    }

    void ShapeExtractor::SetNavigationBarRect(Rect navigationBarRect)
    {
        this->navigationBarRect = navigationBarRect;
    }

    bool ShapeExtractor::ContourContainPoint(const vector<Point>& points, Point targePoint, int interval)
    {
        bool isContained = false;

        for(auto point : points)
        {
            if (abs(targePoint.x - point.x) <= interval && abs(targePoint.y - point.y) <= interval)
            {
                isContained = true;
                break;
            }
        }

        return isContained;
    }

    boost::shared_ptr<Shape> ShapeExtractor::BoundingRectToShape(Rect rect)
    {
        Point center = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
        if(navigationBarRect.contains(center))
            return nullptr;

        if(rect.height < 40 || rect.width < 40)
            return nullptr;

        boost::shared_ptr<Shape> shape = boost::shared_ptr<Shape>(new Shape());
        shape->area = rect.area();
        shape->boundRect = rect;
        shape->center = center;
        vector<Point> contour; // reverse clockwise
        contour.push_back(Point(rect.x, rect.y));
        contour.push_back(Point(rect.x, rect.y + rect.height));
        contour.push_back(Point(rect.x + rect.width, rect.y + rect.height));
        contour.push_back(Point(rect.x + rect.width, rect.y));
        shape->contours = contour;
        shape->type = ShapeType::MSERShape;
        return shape;
    }

    boost::shared_ptr<Shape> ShapeExtractor::ContourToShape(const vector<Point>& contour)
    {
        double area = contourArea(contour);
        if(area < minArea)
            return nullptr;

        Rect rect = boundingRect(contour);
        Size frameSize = frame.size();
        if(rect.width > frameSize.width * 3 / 4 
            || rect.height > frameSize.height * 3 / 4 
            || rect.width > rect.height * 5 
            || rect.height > rect.width * 5)
            return nullptr;

        Point center = Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
        if(navigationBarRect.contains(center))
            return nullptr;

        boost::shared_ptr<Shape> shape = boost::shared_ptr<Shape>(new Shape());
        shape->area = area;
        shape->boundRect = rect;
        shape->center = center;
        shape->contours = contour;
        shape->type = ShapeType::ContourShape;
        return shape;
    }

    boost::shared_ptr<Shape> ShapeExtractor::TransformContourToShape(const vector<Point>& contour)
    {
        double interval = 0.05;
        vector<Point> approx;
        approxPolyDP(contour, approx, 5, true);
        boost::shared_ptr<Shape> contourShape = nullptr;

        if(isContourConvex(approx))
        {
            contourShape = ContourToShape(approx);
        }
        else
        {
            vector<Point> validPoints;
            for(auto point : approx)
            {
                if (!ContourContainPoint(validPoints, point))
                {
                    validPoints.push_back(point);
                }
            }

            if (validPoints.size() == 3) // triangle
            {
                contourShape = ContourToShape(approx);
            }
            else if (validPoints.size() >= 4)
            {
                double realArea = contourArea(contour);
                Rect rect = boundingRect(contour);
                int radius = rect.width / 2;
                double area = CV_PI * pow(radius, 2);

                if (FSmallerE(abs(1 - (double)rect.width / rect.height), interval) && FSmallerE(abs(1 - area / realArea), interval)) // circle
                {
                    contourShape = ContourToShape(approx);
                }
                else
                {
                    vector<double> angleList;
                    double averageAngle = 0;
                    int validPointSize = (int) validPoints.size();
                    int approxPointSize = (int) approx.size();
                    double standardAngle = 180 * (validPointSize - 2) / validPointSize;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
                    for (int i = 1; i <= validPointSize; i++)
                    {
                        int currentIndex = i % approxPointSize;
                        int previousIndex = (i - 1) % approxPointSize;
                        int nextIndex = (i + 1) % approxPointSize;

                        angleList.push_back(ComputeAngleByCosineLaw(approx[currentIndex], approx[previousIndex], approx[nextIndex]));
                    }

                    for (auto angle : angleList)
                    {
                        averageAngle += angle;
                    }
                    averageAngle /= angleList.size();

                    if (abs(1 - averageAngle / standardAngle) <= interval)
                    {
                        if (validPointSize == 4) // rectangle
                        {
                            contourShape = ContourToShape(approx);
                        }
                        else // polygon
                        {
                            contourShape = ContourToShape(approx);
                        }
                    }
                    else
                    {
                        // DAVINCI_LOG_WARNING << "Not supported now." << endl;
                    }
                }

            } // end of valid point size 4

        } // end of convex contour

        return contourShape;
    }

    bool ShapeExtractor::IsBoundRectCloseToTwoFrameEdges(Size frameSize, Rect boundRect)
    {
        int leftOffset = boundRect.x;
        int rightOffset = frameSize.width - (boundRect.x + boundRect.width);
        int topOffset = boundRect.y;
        int downOffset = frameSize.height - (boundRect.y + boundRect.height);

        if((leftOffset <= edgeOffset && topOffset <= edgeOffset)
            || (topOffset <= edgeOffset && rightOffset <= edgeOffset)
            || (rightOffset <= edgeOffset && downOffset <= edgeOffset)
            || (downOffset <= edgeOffset && leftOffset <= edgeOffset)
            )
            return true;
        else
            return false;
    }

    void ShapeExtractor::Extract(ShapeExtractionLevel level)
    {
        cv::Mat grayFrame = BgrToGray(frame);
        boost::shared_ptr<Shape> shape;
        vector<boost::shared_ptr<Shape>> mserShapes;

        MSER mser;
        int maxRegionPoint = 60000;
        double regionToBoundRectRatio = 1.2;
        if(level == ShapeExtractionLevel::COARSE_EXTRACT)
        {
            maxRegionPoint = 100000;
            maxArea = int(maxRegionPoint * regionToBoundRectRatio);
            mser = MSER(5, 5, maxRegionPoint, 0.25);
        }
        else if(level == ShapeExtractionLevel::MIDDLE_EXTRACT)
        {
            maxRegionPoint = 60000;
            maxArea = int(maxRegionPoint * regionToBoundRectRatio);
            mser = MSER(5, 5, maxRegionPoint, 0.25);
        }
        else if(level == ShapeExtractionLevel::FINE_EXTRACT)
        {
            maxRegionPoint = 14400;
            maxArea = int(maxRegionPoint * regionToBoundRectRatio);
            mser = MSER(5, 5, maxRegionPoint, 0.25);
        }
        else if (level == ShapeExtractionLevel::SMALL_EXTRACT)
        {
            maxRegionPoint = 60000;
            minArea = 500;
            maxArea = int(maxRegionPoint * regionToBoundRectRatio);
            mser = MSER(5, 5, maxRegionPoint, 0.25);
        }

        vector<vector<Point2i>> mserContours;
        mser(grayFrame, mserContours);

        Mat debugFrame = frame.clone();
        Size frameSize = frame.size();

        if(level == ShapeExtractionLevel::SMALL_EXTRACT)
        {
            for(auto mserContour : mserContours)
            {
                Rect mserBoundRect = boundingRect(mserContour);
                if(IsBoundRectCloseToTwoFrameEdges(frameSize, mserBoundRect))
                    continue;
                int boundRectArea = mserBoundRect.area();
                if(boundRectArea < minArea || boundRectArea > maxArea)
                    continue;

                Point center = Point(mserBoundRect.x + mserBoundRect.width / 2, mserBoundRect.y + mserBoundRect.height / 2);
                boost::shared_ptr<Shape> shape = boost::shared_ptr<Shape>(new Shape());
                shape->area = mserBoundRect.area();
                shape->boundRect = mserBoundRect;
                shape->center = center;
                shape->contours = mserContour;
                shape->type = ShapeType::MSERShape;
                mserShapes.push_back(shape);

                DrawContour(debugFrame, mserContour, Red, 1);
            }
            SaveImage(debugFrame, "mser_shapes.png");
        }
        else
        {
            vector<Rect> shapeRects;

            for(auto mserContour : mserContours)
            {
                Rect mserBoundRect = boundingRect(mserContour);
                if(IsBoundRectCloseToTwoFrameEdges(frameSize, mserBoundRect))
                    continue;
                int boundRectArea = mserBoundRect.area();
                if(boundRectArea < minArea || boundRectArea > maxArea)
                    continue;

                DrawContour(debugFrame, mserContour, Red, 1);
                shapeRects.push_back(mserBoundRect);
            }
            SaveImage(debugFrame, "mser_shapes.png");

            shapeRects = ObjectUtil::Instance().RemoveContainedRects(shapeRects);

            for(auto shapeRect : shapeRects)
            {
                shape = BoundingRectToShape(shapeRect);
                if(shape != nullptr)
                    mserShapes.push_back(shape);
            }
        }

        shapes.clear();
        AddVectorToVector(shapes, mserShapes);
    }

    vector<gValue> ShapeExtractor::FindNearestKPointsInternal(const vector<Point>& points, Point clickPoint, size_t nearK)
    {
        vector<Point> nearPoints;
        boost::geometry::index::rtree<gValue, boost::geometry::index::quadratic<16> > rtree;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        for(size_t i = 0; i < points.size(); i++)
        {
            gPoint point = gPoint(static_cast<float>(points[i].x), static_cast<float>(points[i].y));
            rtree.insert(std::make_pair(point, i));
        }

        std::vector<gValue> returnedValues;
        gPoint targetGPoint = gPoint(static_cast<float>(clickPoint.x), static_cast<float>(clickPoint.y));

        rtree.query(boost::geometry::index::nearest(targetGPoint, (unsigned int)nearK), std::back_inserter(returnedValues));

        return returnedValues;
    }

    vector<int> ShapeExtractor::FindNearestKLocations(vector<Point> &points, Point point, size_t nearK)
    {
        vector<int> nearLocations;
        if(points.size() <= nearK)
        {
            for(int i = 0; i < (int)points.size(); i++)
                nearLocations.push_back(i);
        }
        else
        {
            std::vector<gValue> returnedValues = FindNearestKPointsInternal(points, point, nearK);

            gValue value;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
            for (size_t i = 0; i < nearK; i++) 
            {
                value = returnedValues[i];
                nearLocations.push_back((int)(value.second));
            }
        }

        return nearLocations;
    }

    vector<Point> ShapeExtractor::FindNearestKPoints(vector<Point> &points, Point point, size_t nearK)
    {
        vector<int> nearLocations = FindNearestKLocations(points, point, nearK);
        vector<Point> nearPoints;
        for(auto index : nearLocations)
            nearPoints.push_back(points[index]);

        return nearPoints;
    }

    /// <summary>
    /// Finds the nearest k outside shapes by distance 
    /// </summary>
    /// <param name="shapeCenter">The shape center.</param>
    /// <param name="nearK">The near k.</param>
    /// <returns></returns>
    vector<boost::shared_ptr<Shape>> ShapeExtractor::FindNearestKShapes(Point point, size_t nearK)
    {
        vector<boost::shared_ptr<Shape>> nearShapes;
        if(shapes.size() <= nearK)
        {
            AddVectorToVector(nearShapes, shapes);
        }
        else
        {
            vector<boost::shared_ptr<Shape>> allShapes;
            vector<Point> points;

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
            for(auto shape : shapes)
            {
                allShapes.push_back(shape);
                points.push_back(Point(shape->boundRect.x + shape->boundRect.width / 2, shape->boundRect.y + shape->boundRect.height / 2));
            }

            std::vector<gValue> returnedValues = FindNearestKPointsInternal(points, point, nearK);

            gValue value;
#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
            for (size_t i = 0; i < nearK; i++) 
            {
                value = returnedValues[i];
                //float x = value.first.get<0>();
                //float y = value.first.get<1>();
                nearShapes.push_back(allShapes[value.second]);
            }
        }

        return nearShapes;
    }
}
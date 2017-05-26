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

#ifndef __CROSSRECOGNIZER__
#define __CROSSRECOGNIZER__

#include "BaseObjectCategory.hpp"
namespace DaVinci
{
    enum CornerPosition
    {
        RIGHTTOP,
        LEFTTOP,
        LEFTBOTTOM,
        RIGHTBOTTOM,
        CORNERUNKNOWN
    };

    enum AdjacentType
    {
        XYAdjacent,
        XAdjacent,
        YAdjacent
    };

    // Cross recognizer
    class CrossRecognizer : public BaseObjectCategory
    {
    private:
        std::string objectCategoryName;
        cv::Rect crossRect;
        cv::Point crossCenterPoint;

    public:
        CrossRecognizer();

        bool RecognizeCross(const cv::Mat &frame);

        bool RecognizeCross(const cv::Mat &frame, const cv::Point &point);

        cv::Rect DetectCrossByEr(const cv::Mat &frame, CornerPosition cornerPosition);

        cv::Rect DetectCrossByApproxPoints(const cv::Mat &frame, CornerPosition cornerPosition);

        /// <summary>
        /// Crop a specified corner of frame
        /// </summary>
        /// <param name="frame">frame</param>
        /// <param name="cornerPosition">corner position, such as right top, left top......</param>
        /// <param name="cornerRect">corner rectangle</param>
        /// <returns>corner of frame</returns>
        cv::Mat CropFrameSpecifiedCorner(const cv::Mat &frame, CornerPosition cornerPosition, cv::Rect &cornerRect);

        cv::Rect CheckSymAndBilinear(const cv::Mat &frame, vector<Point2i>& pts, float symThreshold = 0.65); // 0.6 0.75

        bool CheckBilinear(const vector<Point2i> &pts, double targetAngle, float areaThreshold = 0.75, float angleThreshold = 15);

        /// <summary>
        /// Remove adjacent approx points
        /// </summary>
        /// <param name="approxPoints">approx points</param>
        /// <returns>approx points</returns>
        std::vector<cv::Point> GetUniqueApproxPoints(std::vector<cv::Point> approxPoints);

        /// <summary>
        /// Get approx points which are in the bounding rectangle, that is not on or near the sides of rectangle
        /// </summary>
        /// <param name="approxPoints">approx points</param>
        /// <param name="boundingRectangle">bounding rectangle</param>
        /// <returns>approx points</returns>
        std::vector<cv::Point> GetInternalApproxPoints(std::vector<cv::Point> approxPoints, const cv::Rect &boundingRectangle);

        bool AreAdjacentPoints(const cv::Point &point1, const cv::Point &point2, AdjacentType adjacentType, int interval = 3);

        /// <summary>
        /// Check whether four corner points construct a rhombus
        /// </summary>
        /// <param name="cornerPoints">corner points</param>
        /// <returns>true for yes</returns>
        bool IsRhombus(std::vector<cv::Point> cornerPoints);

        bool IsText(const cv::Rect &rect, const cv::Mat &frame, CornerPosition cornerPosition);

        cv::Rect GetCrossRect();

        cv::Point GetCrossCenterPoint();

        /// <summary>
        /// Check wheteher the object attributes satisfies the category
        /// </summary>
        /// <param name="attributes"></param>
        /// <returns></returns>
        virtual bool BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects = std::vector<cv::Rect>(), std::vector<cv::Rect> allZhProposalRects = std::vector<cv::Rect>(), std::vector<Keyword> allEnKeywords = std::vector<Keyword>(), std::vector<Keyword> allZhKeywords = std::vector<Keyword>()) override;

        /// <summary>
        /// Get object category name
        /// </summary>
        /// <returns></returns>
        virtual std::string GetCategoryName() override;
    };
}


#endif
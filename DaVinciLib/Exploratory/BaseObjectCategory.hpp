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

#ifndef __BASEOBJECTCATEGORY__
#define __BASEOBJECTCATEGORY__

#include <vector>

#include "opencv2/opencv.hpp"
#include "KeywordUtil.hpp"

namespace DaVinci
{
    using namespace cv;

    /// <summary>
    /// Object attributes
    /// </summary>
    class ObjectAttributes
    {
        /// <summary>
        /// Image
        /// </summary>
    public:
        cv::Mat image;

        /// <summary>
        /// Construct
        /// </summary>
        ObjectAttributes(const cv::Mat &image);

    };

    /// <summary>
    /// Class BaseObjectCategory
    /// </summary>
    class BaseObjectCategory
    {
        /// <summary>
        /// Check whether object attributs belong to an object
        /// </summary>
        /// <param name="attributes"></param>
        /// <returns></returns>
    public:
        virtual bool BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects = std::vector<cv::Rect>(), std::vector<cv::Rect> allZhProposalRects = std::vector<cv::Rect>(), std::vector<Keyword> allEnKeywords = std::vector<Keyword>(), std::vector<Keyword> allZhKeywords = std::vector<Keyword>()) = 0;

        /// <summary>
        /// Get object category name
        /// </summary>
        /// <returns></returns>
        virtual std::string GetCategoryName();

    };
}


#endif	//#ifndef __BASEOBJECTCATEGORY__

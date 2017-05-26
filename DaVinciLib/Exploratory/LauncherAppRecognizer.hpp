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

#ifndef __LAUNCHERAPPRECOGNIZER__
#define __LAUNCHERAPPRECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "KeywordRecognizer.hpp"

namespace DaVinci
{
    /// <summary>
    /// Launcher app recognizer
    /// </summary>
    class LauncherAppRecognizer : public BaseObjectCategory
    {
    private:
        std::string objectCategory;

        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        std::string iconName;
        std::string launcherSelectTopActivity;

        std::vector<Keyword> enWhiteList;
        std::vector<Keyword> enIconNameWhiteList;
        std::vector<Keyword> zhWhiteList;

        cv::Rect launcherAppRect;
        cv::Rect alwaysRect;
        cv::Rect homeRect;

        /// <summary>
        /// Download app page
        /// </summary>
    public:
        LauncherAppRecognizer();

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

        /// <summary>
        /// Get launcher rectangle
        /// </summary>
        /// <returns></returns>
        cv::Rect GetLauncherAppRect();

        cv::Rect GetAlwaysRect();

        std::string GetLauncherSelectTopActivity();

        /// <summary>
        /// Icon Name
        /// </summary>
        /// <param name="iName"></param>
        void SetIconName(const std::string &iconName);

        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool RecognizeLauncherCandidate(const cv::Mat &frame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects);

        cv::Rect LocateHomeLauncherRect(const std::vector<Keyword> &allKeywords, const cv::Rect &alwaysRect, int yInterval = 30, int heightThreshold = 20);
    };
}


#endif	//#ifndef __LAUNCHERAPPRECOGNIZER__

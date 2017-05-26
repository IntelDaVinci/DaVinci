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

#ifndef __LICENSERECOGNIZER__
#define __LICENSERECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "KeywordRecognizer.hpp"

namespace DaVinci
{
    class LicenseRecognizer : public BaseObjectCategory
    {
    private:
        std::string objectCategory;

        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        std::vector<Keyword> enPositiveWhiteList;
        std::vector<Keyword> enCheckBoxWhiteList;
        std::vector<Keyword> zhPositiveWhiteList;
        std::vector<Keyword> zhCheckBoxWhiteList;

        std::vector<std::string> allLanguages;

        cv::Rect agreeRect;
        cv::Rect checkBoxRect;

    public:
        LicenseRecognizer();

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
        /// Get OK rectangle
        /// </summary>
        /// <returns></returns>
        cv::Rect GetAgreeRect();

        /// <summary>
        /// Get checkbox rectangle
        /// </summary>
        /// <returns></returns>
        cv::Rect GetCheckBoxRect();

        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool RecognizeLicense(const cv::Mat &image, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords);

        bool RecognizeCheckBox(const cv::Mat &image, const cv::Rect &agreeRect, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords);

        std::string GetParentCategoryName()
        {
            return BaseObjectCategory::GetCategoryName();
        }

        bool GetParentBelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects = std::vector<cv::Rect>(), std::vector<cv::Rect> allZhProposalRects = std::vector<cv::Rect>(), std::vector<Keyword> allEnKeywords = std::vector<Keyword>(), std::vector<Keyword> allZhKeywords = std::vector<Keyword>())
        {
            return BaseObjectCategory::BelongsTo(attributes, allEnProposalRects, allZhProposalRects, allEnKeywords, allZhKeywords);
        }
    };
}

#endif
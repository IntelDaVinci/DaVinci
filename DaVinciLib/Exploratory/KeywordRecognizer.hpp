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

#ifndef __KEYWORDRECOGNIZER__
#define __KEYWORDRECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "KeywordUtil.hpp"
#include "boost/asio.hpp"
#include "TextRecognize.hpp"

namespace DaVinci
{
    // Split big image to small images by proposal rects, then OCR small iamges to recognize keywords
    class KeywordRecognizer
    {
    private:
        PageIteratorLevel pageIteratorLevel;

        /// <summary>
        /// Whether binarize images before OCR
        /// </summary>
        bool needBinarization;

        /// <summary>
        /// Whether filter keywords which contain keywords of black list
        /// </summary>
        bool checkBlackList;

        /// <summary>
        /// Whether check negative prior keywords, if they exists, we should choose negative keywords preferentially
        /// </summary>
        bool isNegativePrior;

        std::vector<cv::Rect> proposalRects;

        std::vector<Keyword> whiteList;
        std::vector<Keyword> blackList;
        std::vector<Keyword> negativePriorList;

        std::vector<Keyword> keywords;

    public:
        KeywordRecognizer();

        std::vector<Keyword> GetKeywords();

        void SetPageIteratorLevel(PageIteratorLevel pageIteratorLevel);

        void SetNeedBinarization(bool needBinarization);

        void SetCheckBlackList(bool checkBlackList);

        void SetIsNegativePrior(bool isNegativePrior);

        void SetProposalRects(std::vector<cv::Rect> &proposalRects, bool isFiltered = false);

        void SetWhiteList(const std::vector<Keyword> &whiteList);

        void SetBlackList(const std::vector<Keyword> &blackList);

        void SetNegativePriorList(const std::vector<Keyword> &negativePriorList);

        bool RecognizeKeyword(const std::vector<Keyword> &allKeywords);

        std::vector<Keyword> DetectAllKeywords(const cv::Mat &frame, const std::string &languageMode);

        std::vector<Keyword> DetectKeywordsByOCR(const cv::Mat &frame, const std::string &languageMode);

        /// <summary>
        /// Match keywords by specified keyword
        /// </summary>
        /// <param name="expectedKeywords">expected keywords, only have value, priority and keyword class (positive or negative)</param>
        /// <param name="allKeywords">all keywords which need matching, only have value and location</param>
        /// <returns>matched keywords, have value, priority, location and keyword class</returns>
        std::vector<Keyword> MatchKeywordByRegex(const std::vector<Keyword> &expectedKeywords, const std::vector<Keyword> &allKeywords);

        void FilterProposalRects(std::vector<cv::Rect> &proposalRects);

        void SortKeywordsByKeywordClass(std::vector<Keyword> &keywords, bool isAsc);
    };
}

#endif
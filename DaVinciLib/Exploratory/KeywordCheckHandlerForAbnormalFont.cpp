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

#include "DaVinciCommon.hpp"
#include "KeywordCheckHandlerForAbnormalFont.hpp"

namespace DaVinci
{
    KeywordCheckHandlerForAbnormalFont::KeywordCheckHandlerForAbnormalFont(boost::shared_ptr<KeywordRecognizer> keywordRecog)
    {
        this->keywordRecog = keywordRecog;
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(true);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        this->needSemantics = false;
        this->needNegative = false;
        this->maxLineInterval = 60;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::ABNORMALFONT);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForAbnormalFont::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert (keywordList.size() == 2);

        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
    }

    bool KeywordCheckHandlerForAbnormalFont::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        if(language == "zh")
        {
            return false;
        }
        else
        {
            // FIXME: detect abnormal fonts only for English lanuage
            bool matchKeyword = HandleCommonRequestImpl(language, enTexts, zhTexts);

            // FIXME: workaround for OCR recognition accuracy - some images are recognized as abnormal font
            bool matchWidth = false;
            int abnormalFontWidth = 100;
            std::vector<Keyword> keywords = this->keywordRecog->MatchKeywordByRegex(enObjectWhiteList, enTexts);
            for(auto keyword: keywords)
            {
                if(keyword.location.width < abnormalFontWidth)
                {
                    matchWidth = true;
                    break;
                }
            }
            return matchKeyword && matchWidth;
        }
    }

    FailureType KeywordCheckHandlerForAbnormalFont::GetCurrentFailureType()
    {
        return FailureType::WarMsgAbnormalFont;
    }

    cv::Rect KeywordCheckHandlerForAbnormalFont::GetOkRect(const cv::Mat &frame)
    {
        return EmptyRect;
    }
}
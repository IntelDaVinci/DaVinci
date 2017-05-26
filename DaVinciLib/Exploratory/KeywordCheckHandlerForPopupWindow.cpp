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
#include "KeywordCheckHandlerForPopupWindow.hpp"

namespace DaVinci
{
    KeywordCheckHandlerForPopupWindow::KeywordCheckHandlerForPopupWindow(boost::shared_ptr<KeywordRecognizer> keywordRecog)
    {
        this->keywordRecog = keywordRecog;
        this->needNegative = false;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::POPUPWINDOW);

        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForPopupWindow::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
    }

    bool KeywordCheckHandlerForPopupWindow::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        bool isRecognized = HandleCommonRequestImpl(language, enTexts, zhTexts);
        this->windowLanguage = language;
        this->enWindowKeyword = enTexts;
        this->zhWindowKeyword = zhTexts;
        return isRecognized;
    }

    FailureType KeywordCheckHandlerForPopupWindow::GetCurrentFailureType()
    {
        return FailureType::WarMsgMax;
    }

    cv::Rect KeywordCheckHandlerForPopupWindow::GetOkRect(const cv::Mat &frame)
    {
        cv::Rect okRect = EmptyRect;
        std::vector<Keyword> okKeywords;

        if (boost::equals(this->windowLanguage, "zh"))
        {
            okKeywords = this->keywordRecog->MatchKeywordByRegex(zhObjectWhiteList, zhWindowKeyword);
        }
        else
        {
            okKeywords = this->keywordRecog->MatchKeywordByRegex(enObjectWhiteList, enWindowKeyword);
        }

         
        if (!okKeywords.empty())
        {
            okRect = okKeywords.back().location;
        }

        return okRect;
    }
}
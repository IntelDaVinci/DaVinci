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

#include "KeywordCheckHandlerForCrashANR.hpp"

using namespace cv;

namespace DaVinci
{
    KeywordCheckHandlerForCrashANR::KeywordCheckHandlerForCrashANR(boost::shared_ptr<KeywordRecognizer> keywordRecog, bool isCrash)
    {
        this->crashLanguage  = "en";
        this->isForCrash = isCrash;

        this->keywordRecog = keywordRecog;
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(true);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        this->needSemantics = false;
        this->needNegative = true;
        this->maxLineInterval = 120;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory;
        if(this->isForCrash)
            keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::CRASHDIALOG);
        else
            keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::ANR);

        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForCrashANR::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 6);

        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
        this->enBehaviorWhiteList = keywordList["EnBehaviorWhiteList"];
        this->enOKWhiteList = keywordList["EnOKWhiteList"];
        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
        this->zhBehaviorWhiteList = keywordList["ZhBehaviorWhiteList"];
        this->zhOKWhiteList = keywordList["ZhOKWhiteList"];
    }

    bool KeywordCheckHandlerForCrashANR::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        this->enCrashKeywords = enTexts;
        this->zhCrashKeywords = zhTexts;
        bool isRecognized = HandleCommonRequestImpl(language, enTexts, zhTexts);
        this->crashLanguage = language;
        return isRecognized;
    }

    FailureType KeywordCheckHandlerForCrashANR::GetCurrentFailureType()
    {
        if(this->isForCrash)
            return FailureType::ErrMsgCrash;
        else
            return FailureType::ErrMsgANR;
    }

    Rect KeywordCheckHandlerForCrashANR::GetOkRect(const cv::Mat &frame)
    {
        cv::Rect okRect = EmptyRect;
        std::vector<Keyword> okWhiteList;
        std::vector<Keyword> crashKeywords;

        if (boost::equals(this->crashLanguage, "zh"))
        {
            okWhiteList = this->zhOKWhiteList;
            crashKeywords = this->zhCrashKeywords;
        }
        else
        {
            okWhiteList = this->enOKWhiteList;
            crashKeywords = this->enCrashKeywords;
        }

        std::vector<Keyword> okKeywords = this->keywordRecog->MatchKeywordByRegex(okWhiteList, crashKeywords);
        if (!okKeywords.empty())
        {
            okRect = okKeywords.back().location;
        }
        else
        {
            this->keywordRecog->SetNeedBinarization(false);
            crashKeywords = this->keywordRecog->DetectAllKeywords(frame, this->crashLanguage);
            okKeywords = this->keywordRecog->MatchKeywordByRegex(okWhiteList, crashKeywords);
            if (!okKeywords.empty())
            {
                okRect = okKeywords.back().location;
            }
            this->keywordRecog->SetNeedBinarization(true); // Set back
        }

        return okRect;
    }
}

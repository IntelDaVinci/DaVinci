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
#include "KeywordCheckHandlerForErrorDialog.hpp"

namespace DaVinci
{
    KeywordCheckHandlerForErrorDialog::KeywordCheckHandlerForErrorDialog(boost::shared_ptr<KeywordRecognizer> keywordRecog)
    {
        this->keywordRecog = keywordRecog;
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(true);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        this->needSemantics = false;
        this->needNegative = true;
        this->maxLineInterval = 60;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::ERRORDIALOG);

        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForErrorDialog::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert (keywordList.size() == 6);

        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
        this->enBehaviorWhiteList = keywordList["EnBehaviorWhiteList"];
        this->enBlackList = keywordList["EnBlackList"];
        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
        this->zhBehaviorWhiteList = keywordList["ZhBehaviorWhiteList"];
        this->zhBlackList = keywordList["ZhBlackList"];
    }

    bool KeywordCheckHandlerForErrorDialog::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        bool inBlackList = false;

        std::vector<Keyword> blackList;
        std::vector<Keyword> texts;

        // FIXME: two supported languages
        for (int i = 1; i <= 2; i++)
        {
            if (boost::equals(language, "zh"))
            {
                blackList = this->zhBlackList;
                texts = zhTexts;
            }
            else
            {
                blackList = this->enBlackList;
                texts = enTexts;
            }

            this->keywordRecog->SetWhiteList(blackList);
            inBlackList = this->keywordRecog->RecognizeKeyword(texts);

            if (inBlackList)
            {
                return false;       // return false if find keyword in blacklist
            }
            language = (boost::equals(language, "zh") ? "en" : "zh");
        }

        return HandleCommonRequestImpl(language, enTexts, zhTexts);
    }

    FailureType KeywordCheckHandlerForErrorDialog::GetCurrentFailureType()
    {
        return FailureType::WarMsgErrorDialog;
    }

    cv::Rect KeywordCheckHandlerForErrorDialog::GetOkRect(const cv::Mat &frame)
    {
        return EmptyRect;
    }
}
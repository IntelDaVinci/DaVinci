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
#include "KeywordCheckHandlerForProcessExit.hpp"

namespace DaVinci
{
    KeywordCheckHandlerForProcessExit::KeywordCheckHandlerForProcessExit(boost::shared_ptr<KeywordRecognizer> keywordRecog)
    {
        this->keywordRecog = keywordRecog;
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(false);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);
        this->clickPoint = EmptyPoint;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::PROCESSEXIT);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForProcessExit::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert (keywordList.size() == 6);

        enIndepedentObjectWhiteList = keywordList["EnIndependentWhiteList"];
        enDepedentObjectWhiteList = keywordList["EnDependentWhiteList"];
        enOKObjectWhiteList = keywordList["EnOKWhiteList"];
        zhIndepedentObjectWhiteList = keywordList["ZhIndependentWhiteList"];
        zhDepedentObjectWhiteList = keywordList["ZhDependentWhiteList"];
        zhOKObjectWhiteList = keywordList["ZhOKWhiteList"];
    }

    void KeywordCheckHandlerForProcessExit::SetClickPoint(Point p)
    {
        this->clickPoint = p;
    }

    bool KeywordCheckHandlerForProcessExit::ContainClickPointByKeywords(std::vector<Keyword> &keywordList)
    {
        double allowDistance = 100 * sqrt(2);
        for(auto keyword : keywordList)
        {
            Point tl = keyword.location.tl();
            Point br = keyword.location.br();
            Point center((tl.x + br.x) / 2, (tl.y + br.y) / 2);
            double distanceToClickPoint = ComputeDistance(center, this->clickPoint);
            if(FSmallerE(distanceToClickPoint, allowDistance))
                return true;
        }

        return false;
    }

    bool KeywordCheckHandlerForProcessExit::CheckClickedKeywords(std::vector<Keyword> clickedKeywords)
    {
        bool isRecognized = false;

        // use zh language to check
        this->keywordRecog->SetWhiteList(zhIndepedentObjectWhiteList);
        isRecognized = this->keywordRecog->RecognizeKeyword(clickedKeywords);

        if (!isRecognized)
        {
            // use en language to check
            this->keywordRecog->SetWhiteList(enIndepedentObjectWhiteList);
            isRecognized = this->keywordRecog->RecognizeKeyword(clickedKeywords);
        }
        return isRecognized;
    }

    bool KeywordCheckHandlerForProcessExit::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        std::vector<Keyword> texts;
        bool isIndependentRecognized = false, isDependentRecognized = false;
        bool isRecognized = false;
        std::vector<Keyword> keywords, okKeywords;

        // FIXME: two supported languages
        for (int i = 1; i <= 2; i++)
        {
            if (boost::equals(language, "zh"))
            {
                texts = zhTexts;
                this->keywordRecog->SetWhiteList(zhIndepedentObjectWhiteList);
            }
            else
            {
                texts = enTexts;
                this->keywordRecog->SetWhiteList(enIndepedentObjectWhiteList);
            }

            isIndependentRecognized = this->keywordRecog->RecognizeKeyword(texts);
            if (isIndependentRecognized)
            {
                keywords = this->keywordRecog->GetKeywords();
                if(ContainClickPointByKeywords(keywords))
                {
                    isRecognized = true;
                    break;
                }
            }

            if (boost::equals(language, "zh"))
            {
                this->keywordRecog->SetWhiteList(zhOKObjectWhiteList);
            }
            else
            {
                this->keywordRecog->SetWhiteList(enOKObjectWhiteList);
            }
            this->keywordRecog->RecognizeKeyword(texts);
            okKeywords = this->keywordRecog->GetKeywords();


            if (boost::equals(language, "zh"))
            {
                this->keywordRecog->SetWhiteList(zhDepedentObjectWhiteList);
            }
            else
            {
                this->keywordRecog->SetWhiteList(enDepedentObjectWhiteList);
            }
            isDependentRecognized = this->keywordRecog->RecognizeKeyword(texts);
            keywords = this->keywordRecog->GetKeywords();

            if(ContainClickPointByKeywords(okKeywords) && isDependentRecognized)
            {
                isRecognized = true;
                break;
            }

            language = (boost::equals(language, "zh") ? "en" : "zh");
        }

        return isRecognized;
    }

    FailureType KeywordCheckHandlerForProcessExit::GetCurrentFailureType()
    {
        return FailureType::WarMsgMax;
    }

    cv::Rect KeywordCheckHandlerForProcessExit::GetOkRect(const cv::Mat &frame)
    {
        return EmptyRect;
    }
}
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

#include "RnRKeywordRecognizer.hpp"

namespace DaVinci
{
    RnRKeywordRecognizer::RnRKeywordRecognizer()
    {
        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(true);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(true);

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::RNR);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    std::vector<Keyword> &RnRKeywordRecognizer::GetKeywords()
    {
        return this->keywords;
    }

    std::vector<Keyword> &RnRKeywordRecognizer::GetCheckBoxes()
    {
        return this->checkBoxes;
    }

    void RnRKeywordRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 6);

        this->enWhiteList = keywordList["EnWhiteList"];
        this->enNegativePriorList = keywordList["EnNegativePriorList"];
        this->zhWhiteList = keywordList["ZhWhiteList"];
        this->zhNegativePriorList = keywordList["ZhNegativePriorList"];
        this->enCheckBoxList = keywordList["EnCheckBoxList"];
        this->zhCheckBoxList = keywordList["ZhCheckBoxList"];
    }

    bool RnRKeywordRecognizer::RecognizeRnRKeyword(std::vector<Keyword> allKeywords, const std::string &languageMode)
    {
        bool isRecognized = false, isCheckBox = false;

        if (boost::equals(languageMode, "zh"))
        {
            this->keywordRecog->SetWhiteList(this->zhWhiteList);
            this->keywordRecog->SetNegativePriorList(this->zhNegativePriorList);
        }
        else
        {
            this->keywordRecog->SetWhiteList(this->enWhiteList);
            this->keywordRecog->SetNegativePriorList(this->enNegativePriorList);
        }

        isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);

        if (isRecognized)
        {
            this->keywords = this->keywordRecog->GetKeywords();
            if (boost::equals(languageMode, "zh"))
            {
                this->keywordRecog->SetWhiteList(this->zhCheckBoxList);
            }
            else
            {
                this->keywordRecog->SetWhiteList(this->enCheckBoxList);
            }
            isCheckBox = this->keywordRecog->RecognizeKeyword(allKeywords);
            if(isCheckBox)
            {
                this->checkBoxes = this->keywordRecog->GetKeywords();
                for(auto checkBox : this->checkBoxes)
                {
                    RemoveElementFromVector(this->keywords, checkBox);
                }
            }
        }

        return isRecognized;
    }
}
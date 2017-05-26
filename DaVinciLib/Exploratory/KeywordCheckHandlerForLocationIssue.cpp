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
#include "KeywordCheckHandlerForLocationIssue.hpp"

namespace DaVinci
{
    KeywordCheckHandlerForLocationIssue::KeywordCheckHandlerForLocationIssue(boost::shared_ptr<KeywordRecognizer> keywordRecog)
    {
        this->keywordRecog = keywordRecog;
        this->needNegative = true;

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::LOCATIONISSUE);

        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            this->InitializeKeywordList(keywordList);
        }
    }

    void KeywordCheckHandlerForLocationIssue::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
        this->enBehaviorWhiteList = keywordList["EnBehaviorWhiteList"];
        this->enBlackList = keywordList["EnBlackList"];
        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
        this->zhBehaviorWhiteList = keywordList["ZhBehaviorWhiteList"];
        this->zhBlackList = keywordList["ZhBlackList"];
    }

    bool KeywordCheckHandlerForLocationIssue::HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
    {
        bool isRecognized = false;

        bool hasObject = false;
        bool hasBehavior = false;
        bool inBlackList = false;

        std::vector<Keyword> objectWhiteList;
        std::vector<Keyword> behaviorWhiteList;
        std::vector<Keyword> blackList;

        std::vector<Keyword> texts;

        std::vector<Keyword> objectKeywords;
        std::vector<Keyword> behaviorKeywords;

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

        for (int i = 1; i <= 2; i++)
        {
            if (boost::equals(language, "zh"))
            {
                objectWhiteList = this->zhObjectWhiteList;
                behaviorWhiteList = this->zhBehaviorWhiteList;
                texts = zhTexts;
            }
            else
            {
                objectWhiteList = this->enObjectWhiteList;
                behaviorWhiteList = this->enBehaviorWhiteList;
                texts = enTexts;
            }

            this->keywordRecog->SetWhiteList(objectWhiteList);
            hasObject = this->keywordRecog->RecognizeKeyword(texts);
            if (!needNegative)
            {
                isRecognized = hasObject;
            }
            else
            {
                if (hasObject)
                {
                    objectKeywords = this->keywordRecog->GetKeywords();
                }
                else // If has no object, then continue to save time
                {
                    language = (boost::equals(language, "zh") ? "en" : "zh");   // has no object, change another language
                    continue;
                }

                this->keywordRecog->SetWhiteList(behaviorWhiteList);
                hasBehavior = this->keywordRecog->RecognizeKeyword(texts);
                if (hasBehavior)
                {
                    behaviorKeywords = this->keywordRecog->GetKeywords();
                }

                // couldn't find the location 'Shanghai', return true
                if (hasObject && !hasBehavior)
                {
                    isRecognized = true;
                }
                else if (hasObject && hasBehavior)      // find the location 'Shanghai', but is far away from the object keyword, need to return true
                {
                    int matchNum = 0;
                    for (auto objectKeyword : objectKeywords)
                    {
                        int objectCenterY  = objectKeyword.location.y + objectKeyword.location.height / 2;
                        for (auto behaviorKeyword : behaviorKeywords)
                        {
                            int behaviorCenterY  = behaviorKeyword.location.y + behaviorKeyword.location.height / 2;

                            int lineInterval = abs(objectCenterY - behaviorCenterY);
                            if (lineInterval <= behaviorKeyword.location.height * 2)
                            {
                                matchNum ++;
                            }
                        }
                    }
                    if (matchNum == 0)
                        isRecognized = true;
                }
            }

            if (isRecognized)
            {
                break;
            }
            else
            {
                language = (boost::equals(language, "zh") ? "en" : "zh");
            }
        }

        return isRecognized;
    }

    FailureType KeywordCheckHandlerForLocationIssue::GetCurrentFailureType()
    {
        return FailureType::WarMsgLocationIssue;
    }

    cv::Rect KeywordCheckHandlerForLocationIssue::GetOkRect(const cv::Mat &frame)
    {
        return EmptyRect;
    }
}
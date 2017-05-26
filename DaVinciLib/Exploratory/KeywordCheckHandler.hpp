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

#ifndef __KEYWORDCHECKHANDLER__
#define __KEYWORDCHECKHANDLER__

#include <vector>
#include "FailureType.hpp"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include "KeywordRecognizer.hpp"

namespace DaVinci
{
    class KeywordCheckHandler : public boost::enable_shared_from_this<KeywordCheckHandler>
    {
    protected:
        boost::shared_ptr<KeywordCheckHandler> m_successor;
        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        std::vector<Keyword> enObjectWhiteList;
        std::vector<Keyword> enBehaviorWhiteList;

        std::vector<Keyword> zhObjectWhiteList;
        std::vector<Keyword> zhBehaviorWhiteList;

        bool needSemantics;
        bool needNegative;

        Point clickPoint;

        int maxLineInterval;
        static const int sameLineInterval = 15;

    public:
        virtual void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList) = 0;

        virtual bool HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts) = 0;

        virtual FailureType GetCurrentFailureType() = 0;

        virtual Rect GetOkRect(const cv::Mat &frame) = 0;

        void SetSuccessor(boost::shared_ptr<KeywordCheckHandler> successor)
        {
            m_successor = successor;
        }

        bool IsKeywordPairRecognized(std::vector<Keyword> objectKeywords, std::vector<Keyword> behaviorKeywords)
        {
            bool result = false;

            for (auto objectKeyword : objectKeywords)
            {
                int objectCenterY  = objectKeyword.location.y + objectKeyword.location.height / 2;

                for (auto behaviorKeyword : behaviorKeywords)
                {
                    int behaviorCenterY  = behaviorKeyword.location.y + behaviorKeyword.location.height / 2;

                    int lineInterval = abs(objectCenterY - behaviorCenterY);
                    if (lineInterval <= maxLineInterval) // Allow the max line interval between positive and negative keyword
                    {
                        if (needSemantics && lineInterval <= sameLineInterval) // Same line
                        {
                            if (behaviorKeyword.priority == Priority::LOW) // Low keyword like "try again" no need to compare word interval
                            {
                                result = true;
                                break;
                            }

                            int wordInterval;
                            if (objectKeyword.location.br().x > behaviorKeyword.location.br().x)
                            {
                                wordInterval = objectKeyword.location.x - behaviorKeyword.location.br().x;
                            }
                            else
                            {
                                wordInterval = behaviorKeyword.location.x - objectKeyword.location.br().x;
                            }

                            int allowWordInterval = (objectKeyword.location.height + behaviorKeyword.location.height) / 2;
                            if (wordInterval < allowWordInterval)
                            {
                                result = true;
                                break;
                            }
                        }
                        else // Cross line
                        {
                            if (GetCurrentFailureType() == FailureType::WarMsgNetwork)
                            {
                                if (behaviorKeyword.priority == Priority::LOW) // Low keyword like "try again" no need to compare rect.br(), behavior keyword is placed in the middle
                                {
                                    result = true;
                                    break;
                                }

                                cv::Rect upRowRect = EmptyRect;
                                cv::Rect downRowRect = EmptyRect;
                                if (objectCenterY < behaviorCenterY)
                                {
                                    upRowRect = objectKeyword.location;
                                    downRowRect = behaviorKeyword.location;
                                }
                                else
                                {
                                    upRowRect = behaviorKeyword.location;
                                    downRowRect = objectKeyword.location;
                                }

                                if (upRowRect.br().x > downRowRect.br().x)
                                {
                                    result = true;
                                    break;
                                }
                            }
                            else
                            {
                                result = true;
                                break;
                            }
                        }
                    }
                }

                if (result)
                {
                    break;
                }
            }

            return result;
        }

        boost::shared_ptr<KeywordCheckHandler> HandleRequest(std::string language, std::vector<Keyword> enTexts, std::vector<Keyword> zhTexts)
        {
            bool requestReturn = HandleRequestImpl(language, enTexts, zhTexts);
            if (requestReturn)
            {
                return shared_from_this();
            }
            else
            {
                if (m_successor != nullptr)
                {
                    return m_successor->HandleRequest(language, enTexts, zhTexts);
                }
                else
                {
                    return nullptr;
                }
            }
        }

        bool HandleCommonRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts)
        {
            assert(this->keywordRecog != nullptr);

            bool isRecognized = false;

            bool hasObject = false;
            bool hasBehavior = false;

            std::vector<Keyword> objectWhiteList;
            std::vector<Keyword> behaviorWhiteList;

            std::vector<Keyword> texts;

            std::vector<Keyword> objectKeywords;
            std::vector<Keyword> behaviorKeywords;

            // FIXME: two supported languages
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

                    if (hasObject && hasBehavior)
                    {
                        isRecognized = IsKeywordPairRecognized(objectKeywords, behaviorKeywords);
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
    };
}


#endif	//#ifndef __KEYWORDCHECKHANDLER__

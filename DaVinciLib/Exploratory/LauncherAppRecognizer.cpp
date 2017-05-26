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
#include "LauncherAppRecognizer.hpp"
#include "boost/algorithm/string.hpp"
#include "ObjectUtil.hpp"
#include "TextUtil.hpp"
#include "ObjectCommon.hpp"

namespace DaVinci
{
    LauncherAppRecognizer::LauncherAppRecognizer()
    {
        this->objectCategory = "Launcher App Page";

        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(false);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        std::map<std::string, std::vector<Keyword>> keywordList;
        std::string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::LAUNCHERAPP);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }

        this->launcherSelectTopActivity = "com.android.internal.app.ResolverActivity";
    }

    bool LauncherAppRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        cv::Mat image = attributes.image;
        return this->RecognizeLauncherCandidate(image, allEnProposalRects, allZhProposalRects);
    }

    std::string LauncherAppRecognizer::GetCategoryName()
    {
        return this->objectCategory;
    }

    cv::Rect LauncherAppRecognizer::GetLauncherAppRect()
    {
        return this->launcherAppRect;
    }

    cv::Rect LauncherAppRecognizer::GetAlwaysRect()
    {
        return this->alwaysRect;
    }

    std::string LauncherAppRecognizer::GetLauncherSelectTopActivity()
    {
        return this->launcherSelectTopActivity;
    }

    void LauncherAppRecognizer::SetIconName(const std::string &iconName)
    {
        this->iconName = iconName;
    }

    void LauncherAppRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 2);

        this->enWhiteList = keywordList["EnWhiteList"];
        this->zhWhiteList = keywordList["ZhWhiteList"];
    }

    bool LauncherAppRecognizer::RecognizeLauncherCandidate(const cv::Mat &frame, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects)
    {
        bool isRecognized = false;
        this->launcherAppRect = EmptyRect;
        this->alwaysRect = EmptyRect;

        std::string languageMode = ObjectUtil::Instance().GetSystemLanguage();

        std::vector<Rect> allProposalRects;
        std::vector<Keyword> allKeywords;
        std::vector<Keyword> whiteList;
        for (int i = 1; i <= 2; i++)
        {
            std::vector<Keyword> iconNameWhiteList;
            std::string iconNameStr = "";
            // Set icon name
            std::vector<std::string> items;
            boost::algorithm::split(items, this->iconName, boost::algorithm::is_any_of(" "));

            if (boost::equals(languageMode, "zh"))
            {
                allProposalRects = ObjectUtil::Instance().FilterRects(allZhProposalRects, 1.5, 10, 500, 50000);
                this->keywordRecog->SetProposalRects(allProposalRects, true);
                allKeywords = this->keywordRecog->DetectAllKeywords(frame, "zh");
                whiteList = this->zhWhiteList;

                if (!items.empty() && !boost::equals(items[0], ""))
                {
                    iconNameStr = items[0];
                }
            }
            else
            {
                allProposalRects = ObjectUtil::Instance().FilterRects(allEnProposalRects, 1.5, 10, 500, 50000);
                this->keywordRecog->SetProposalRects(allProposalRects, true);
                allKeywords = this->keywordRecog->DetectAllKeywords(frame, "en");
                whiteList = this->enWhiteList;

                if (!items.empty() && !boost::equals(items[0], ""))
                {
                    iconNameStr = "(\\b" + items[0] + "\\b)";
                }
            }
            std::sort(allKeywords.begin(), allKeywords.end(), SortKeywordsByKeywordLocationYDesc());

            // Find always button
            std::vector<Keyword> alwaysKeywords = this->keywordRecog->MatchKeywordByRegex(whiteList, allKeywords);
            if (!alwaysKeywords.empty())
            {
                this->alwaysRect = alwaysKeywords[0].location;
            }

            if (this->alwaysRect != EmptyRect)
            {
                if (!boost::equals(iconNameStr, ""))
                {
                    std::wstring wIconName = StrToWstrAnsi(iconNameStr);
                    Keyword iconNameKeyword = {wIconName, Priority::NONE, EmptyRect, KeywordClass::POSITIVE};
                    iconNameWhiteList.push_back(iconNameKeyword);

                    std::vector<Keyword> launcherKeywords = this->keywordRecog->MatchKeywordByRegex(iconNameWhiteList, allKeywords);
                    for (auto keyword : launcherKeywords)
                    {
                        if (keyword.location.br().y < this->alwaysRect.y)
                        {
                            this->launcherAppRect = keyword.location;
                            break;
                        }
                    }
                }

                if (this->launcherAppRect == EmptyRect)
                {
                    this->launcherAppRect = LocateHomeLauncherRect(allKeywords, this->alwaysRect);
                }
            }

            if (this->alwaysRect != EmptyRect && this->launcherAppRect != EmptyRect)
            {
                isRecognized = true;
                break;
            }
            else
            {
                languageMode = (boost::equals(languageMode, "zh") ? "en" : "zh");
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->launcherAppRect, Red, 2);
            DrawRect(debugFrame, this->alwaysRect, Blue, 2);
            SaveImage(debugFrame, "debug_launcher.png");
        }
#endif

        return isRecognized;
    }

    cv::Rect LauncherAppRecognizer::LocateHomeLauncherRect(const std::vector<Keyword> &allKeywords, const cv::Rect &alwaysRect, int yInterval, int heightThreshold)
    {
        cv::Rect rect = EmptyRect;

        for (auto keyword : allKeywords)
        {
            if (keyword.location.y >= alwaysRect.br().y)
            {
                continue;
            }

            if (keyword.location.y + yInterval <= alwaysRect.y && keyword.location.height >= heightThreshold)
            {
                rect = keyword.location;
                break;
            }
        }

        return rect;
    }
}

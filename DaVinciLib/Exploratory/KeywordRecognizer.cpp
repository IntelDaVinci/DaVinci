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

#include "KeywordRecognizer.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "TextUtil.hpp"

#include <regex>

namespace DaVinci
{
    KeywordRecognizer::KeywordRecognizer()
    {
        this->pageIteratorLevel = PageIteratorLevel::RIL_TEXTLINE;
        this->needBinarization = true;
        this->checkBlackList = false;
        this->isNegativePrior = false;
    }

    std::vector<Keyword> KeywordRecognizer::GetKeywords()
    {
        return this->keywords;
    }

    void KeywordRecognizer::SetPageIteratorLevel(PageIteratorLevel pageIteratorLevel)
    {
        this->pageIteratorLevel = pageIteratorLevel;
    }

    void KeywordRecognizer::SetNeedBinarization(bool needBinarization)
    {
        this->needBinarization = needBinarization;
    }

    void KeywordRecognizer::SetCheckBlackList(bool checkBlackList)
    {
        this->checkBlackList = checkBlackList;
    }

    void KeywordRecognizer::SetIsNegativePrior(bool isNegativePrior)
    {
        this->isNegativePrior = isNegativePrior;
    }

    void KeywordRecognizer::SetProposalRects(std::vector<cv::Rect> &proposalRects, bool isFiltered)
    {
        if (isFiltered)
        {
            this->FilterProposalRects(proposalRects);
        }
        this->proposalRects = proposalRects;
    }

    void KeywordRecognizer::SetWhiteList(const std::vector<Keyword> &whiteList)
    {
        this->whiteList = whiteList;
    }

    void KeywordRecognizer::SetBlackList(const std::vector<Keyword> &blackList)
    {
        this->blackList = blackList;
    }

    void KeywordRecognizer::SetNegativePriorList(const std::vector<Keyword> &negativePriorList)
    {
        this->negativePriorList = negativePriorList;
    }

    bool KeywordRecognizer::RecognizeKeyword(const std::vector<Keyword> &allKeywords)
    {
        bool isRecognized = false;
        this->keywords.clear();

        // Match white list keywords
        this->keywords = this->MatchKeywordByRegex(this->whiteList, allKeywords);
        size_t keywordsSize = this->keywords.size();

        if (keywordsSize > 0)
        {
            isRecognized = true;

            if (keywordsSize > 1)
            {
                if (this->isNegativePrior)
                {
                    std::vector<Keyword> negativePriorKeywords = this->MatchKeywordByRegex(this->negativePriorList, this->keywords);
                    if (!negativePriorKeywords.empty()) // If keywords of negative prior list appear, make negative keyword first
                    {
                        this->SortKeywordsByKeywordClass(this->keywords, true);
                    }
                    else
                    {
                        this->SortKeywordsByKeywordClass(this->keywords, false);
                    }
                }
            }

            if (this->checkBlackList)
            {
                std::vector<Keyword> blackListKeywords = this->MatchKeywordByRegex(this->blackList, this->keywords);
                for (auto blackListKeyword : blackListKeywords) // Delete keywords of black list
                {
                    RemoveElementFromVector(this->keywords, blackListKeyword);
                }

                if (this->keywords.empty())
                {
                    isRecognized = false;
                }
            }
        }

        return isRecognized;
    }


    std::vector<Keyword> KeywordRecognizer::DetectAllKeywords(const cv::Mat &frame, const std::string &languageMode)
    {
        std::vector<Keyword> keywords;

        if (frame.empty())          // if frame is empty, copyTo() function will throw exception
            return keywords;

#ifdef _DEBUG
        cv::Mat cloneFrame = frame.clone();
        DrawRects(cloneFrame, this->proposalRects, Blue, 2);
        SaveImage(cloneFrame, "debug_keywords.png");
#endif

        for (auto proposalRect : this->proposalRects)
        {
            cv::Mat rectFrame, detectedFrame, resizedFrame;
            ObjectUtil::Instance().CopyROIFrame(frame, proposalRect, rectFrame);

            DebugSaveImage(rectFrame, "rect.png");

            if (this->needBinarization)
            {
                detectedFrame = ObjectUtil::Instance().BinarizeWithChangedThreshold(rectFrame);
                DebugSaveImage(detectedFrame, "binary.png");
            }
            else
            {
                detectedFrame = rectFrame;
            }

            std::vector<Keyword> detectedKeywords = this->DetectKeywordsByOCR(detectedFrame, languageMode);

			if(rectFrame.rows < 30)
			{
				int ratio = (int)ceil(30.0 / rectFrame.rows);
				resize(rectFrame, resizedFrame, Size(rectFrame.cols * ratio, rectFrame.rows * ratio));
				DebugSaveImage(resizedFrame, "rect_resized.png");
				std::vector<Keyword> resizedFramKeywords = this->DetectKeywordsByOCR(resizedFrame, languageMode);
				for (auto keyword : resizedFramKeywords)
				{
					keyword.location.x/=ratio;
					keyword.location.y=ratio;
					keyword.location.width/=ratio;
					keyword.location.height/=ratio;
					detectedKeywords.push_back(keyword);
				}
			}

            if (detectedKeywords.empty())
            {
                continue;
            }

            for (auto keyword : detectedKeywords)
            {
                keyword.location = ObjectUtil::Instance().RelocateRectangle(frame.size(), keyword.location, proposalRect);
                keywords.push_back(keyword);
            }
        }

        return keywords;
    }

    std::vector<Keyword> KeywordRecognizer::DetectKeywordsByOCR(const cv::Mat &frame, const std::string &languageMode)
    {
        std::vector<Keyword> keywords;

        TextRecognize::TextRecognizeResult textResult;
        if (boost::equals(languageMode, "zh"))
        {
            textResult = TextUtil::Instance().AnalyzeChineseOCR(frame, this->pageIteratorLevel);
        }
        else
        {
            textResult = TextUtil::Instance().AnalyzeEnglishOCR(frame, this->pageIteratorLevel);
        }

        std::vector<std::string> texts = textResult.GetComponentTexts();
        std::vector<cv::Rect> locations = textResult.GetComponentRects();

        for (size_t i = 0; i < texts.size(); i++)
        {
            std::wstring wstr = StrToWstr(texts[i].c_str());
            boost::trim(wstr);
            Keyword keyword = {wstr, Priority::NONE, locations[i], KeywordClass::POSITIVE};
            keywords.push_back(keyword);
        }

        return keywords;
    }

    std::vector<Keyword> KeywordRecognizer::MatchKeywordByRegex(const std::vector<Keyword> &expectedKeywords, const std::vector<Keyword> &allKeywords)
    {
        std::vector<Keyword> matchedKeywords;

        for (auto keyword : allKeywords)
        {
            for (auto expectedKeyword : expectedKeywords)
            {
                std::tr1::wsmatch wsmatch;
                std::tr1::wregex wregex(expectedKeyword.text.c_str(), std::tr1::regex_constants::icase);

                if (std::tr1::regex_search(keyword.text, wsmatch, wregex))
                {
                    Keyword matchedKeyword = {keyword.text, expectedKeyword.priority, keyword.location, expectedKeyword.keywordClass};
                    matchedKeywords.push_back(matchedKeyword);
                    break;
                }
            }
        }

        return matchedKeywords;
    }

    void KeywordRecognizer::FilterProposalRects(std::vector<cv::Rect> &proposalRects)
    {
        std::vector<cv::Rect> leftAlignedRects;
        std::vector<cv::Rect> leftAlignedTextRects;

        // Hash the same row rects together
        std::multimap<int, std::vector<cv::Rect>> cyMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(proposalRects, UType::UTYPE_CY, 10, -1, 10);
        for (std::multimap<int, std::vector<cv::Rect>>::iterator it = cyMap.begin(); it != cyMap.end(); it++)
        {
            std::vector<cv::Rect> rects = it->second;
            leftAlignedRects.push_back(rects[0]);
        }

        // Hash the same height rects together
        std::map<int, std::vector<cv::Rect>> heightMap = ObjectUtil::Instance().HashRectangles(leftAlignedRects, UType::UTYPE_H, 5);
        for (std::map<int, std::vector<cv::Rect>>::iterator it = heightMap.begin(); it != heightMap.end(); it++)
        {
            std::vector<cv::Rect> heightRects = it->second;
            if (heightRects.size() >= 4)
            {
                // Hash the same column rects together
                std::multimap<int, std::vector<cv::Rect>> xMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(heightRects, UType::UTYPE_X, 15, 0, 25, false);
                for (std::multimap<int, std::vector<cv::Rect>>::iterator itt = xMap.begin(); itt != xMap.end(); itt++)
                {
                    std::vector<cv::Rect> rects = itt->second;
                    if (rects.size() >= 4) // More then 4 rows left aligned rects may be background texts which should be filtered
                    {
                        AddVectorToVector(leftAlignedTextRects, rects);
                    }
                }
            }
        }

        // Filter
        for (auto textRect : leftAlignedTextRects)
        {
            for (std::multimap<int, std::vector<cv::Rect>>::iterator it = cyMap.begin(); it != cyMap.end(); it++)
            {
                std::vector<cv::Rect> rects = it->second;
                if (ContainVectorElement(rects, textRect))
                {
                    for (auto rect : rects)
                    {
                        RemoveElementFromVector(proposalRects, rect);
                    }
                    break;
                }
            }
        }
    }

    void KeywordRecognizer::SortKeywordsByKeywordClass(std::vector<Keyword> &keywords, bool isAsc)
    {
        if (isAsc)
        {
            std::sort(keywords.begin(), keywords.end(), SortKeywordsByKeywordClassAsc());
        }
        else
        {
            std::sort(keywords.begin(), keywords.end(), SortKeywordsByKeywordClassDesc());
        }
    }
}
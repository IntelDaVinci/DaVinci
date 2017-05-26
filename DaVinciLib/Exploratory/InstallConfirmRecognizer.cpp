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

#include "boost/algorithm/string.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectUtil.hpp"
#include "TextUtil.hpp"
#include "ObjectCommon.hpp"
#include "InstallConfirmRecognizer.hpp"

using namespace boost;
using namespace cv;
using namespace std;

namespace DaVinci
{
    InstallConfirmRecognizer::InstallConfirmRecognizer()
    {
        this->objectCategory = "Install Confirm Page";

        this->language = ObjectUtil::Instance().GetSystemLanguage();

        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(true);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::INSTALLCONFIRM);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    void InstallConfirmRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 4);

        this->enObjectWhiteList = keywordList["EnObjectWhiteList"];
        this->zhObjectWhiteList = keywordList["ZhObjectWhiteList"];
        this->enPriorList = keywordList["EnPriorList"];
        this->zhPriorList = keywordList["ZhPriorList"];
    }

    bool InstallConfirmRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        cv::Mat image = attributes.image;
        return this->RecognizeInstallConfirm(image);
    }

    std::string InstallConfirmRecognizer::GetCategoryName()
    {
        return objectCategory;
    }

    cv::Rect InstallConfirmRecognizer::GetOKRect()
    {
        return this->okRect;
    }

    void InstallConfirmRecognizer::SetLanguage(string language)
    {
        this->language = language;
    }

    bool InstallConfirmRecognizer::RecognizeInstallConfirm(const Mat &frame)
    {
        bool isRecognized = false;

        int halfHeight = frame.rows / 2;
        cv::Rect roi(0, halfHeight, frame.cols, halfHeight);
        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        for (int i = 1; i <= 2; i++)
        {
            std::vector<cv::Rect> proposalRects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 240, 6, 1, 1, 50, 400, 35000);
            keywordRecog->SetProposalRects(proposalRects);
            std::vector<Keyword> allKeywords = keywordRecog->DetectAllKeywords(roiFrame, this->language);

            if(boost::equals(this->language, "zh"))
                this->keywordRecog->SetWhiteList(this->zhObjectWhiteList);
            else
                this->keywordRecog->SetWhiteList(this->enObjectWhiteList);

            isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);
            if (isRecognized)
            {  
                std::vector<Keyword> matchedKeywords = this->keywordRecog->GetKeywords();
                std::sort(matchedKeywords.begin(), matchedKeywords.end(), SortKeywordsByKeywordLocationYAsc());
                if (!matchedKeywords.empty())
                {
                    std::vector<Keyword> matchedPriorKeywords;
                    if(boost::equals(this->language, "zh"))
                        matchedPriorKeywords = this->keywordRecog->MatchKeywordByRegex(this->zhPriorList, matchedKeywords);
                    else
                        matchedPriorKeywords = this->keywordRecog->MatchKeywordByRegex(this->enPriorList, matchedKeywords);

                    if(!matchedPriorKeywords.empty())
                    {
                        std::sort(matchedPriorKeywords.begin(), matchedPriorKeywords.end(), SortKeywordsByKeywordLocationXAsc());
                        okRect = matchedPriorKeywords[matchedPriorKeywords.size()-1].location;
                    }
                    else
                    {
                        okRect = matchedKeywords[0].location;
                    }
                    okRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), okRect, roi);

#ifdef _DEBUG
                    cv::Mat debugFrame = frame.clone();
                    DrawRect(debugFrame, okRect, Red, 1);
                    SaveImage(debugFrame, "debug_install_confirm.png");
#endif
                }
                break;
            }
            else
            {
                this->language = (boost::equals(this->language, "zh") ? "en" : "zh");
            }
        }

        return isRecognized;
    }
}
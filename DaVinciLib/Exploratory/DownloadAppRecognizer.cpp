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

#include <boost/algorithm/string.hpp>

#include "DaVinciCommon.hpp"
#include "DownloadAppRecognizer.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "TestManager.hpp"
#include "TextUtil.hpp"
namespace DaVinci
{
    DownloadAppRecognizer::DownloadAppRecognizer()
    {
        this->objectCategory = "Play Store Page";
        this->isFree = true;

        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(false);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        std::map<std::string, std::vector<Keyword>> keywordList;
        string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::DOWNLOAD);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    bool DownloadAppRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        cv::Mat image = attributes.image;
        return RecognizeAppCandidate(image);
    }

    std::string DownloadAppRecognizer::GetCategoryName()
    {
        return this->objectCategory;
    }

    cv::Rect DownloadAppRecognizer::GetSearchBarRect()
    {
        return this->searchBarRect;
    }

    void DownloadAppRecognizer::SetSearchBarRect(const cv::Rect &searchBarRect)
    {
        this->searchBarRect = searchBarRect;
    }

    cv::Rect DownloadAppRecognizer::GetSearchButtonRect()
    {
        return this->searchButtonRect;
    }

    cv::Rect DownloadAppRecognizer::GetAppRect()
    {
        return this->appRect;
    }

    cv::Rect DownloadAppRecognizer::GetPriceRect()
    {
        return this->priceRect;
    }

    cv::Rect DownloadAppRecognizer::GetRandomRect()
    {
        return this->randomRect;
    }

    std::string DownloadAppRecognizer::GetRecognizedPriceStr()
    {
        return this->recognizedPriceStr;
    }

    double DownloadAppRecognizer::GetRecognizedPrice()
    {
        return this->recognizedPrice;
    }

    bool DownloadAppRecognizer::IsFree()
    {
        return this->isFree;
    }

    void DownloadAppRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 10);

        // TODO: use macro to define the white list name, support multiple language
        this->freeAppCandidateWhiteList = keywordList["EnFreeAppCandidateWhiteList"];
        this->paidAppCandidateWhiteList = keywordList["EnPaidAppCandidateWhiteList"];
        this->playCandidateWhiteList = keywordList["EnPlayCandidateWhiteList"];
        this->alwaysWhiteList = keywordList["EnAlwaysWhiteList"];
        this->incompatibleWhiteList = keywordList["EnIncompatibleWhiteList"];
        this->priceWhiteList = keywordList["EnPriceWhiteList"];
        this->acceptWhiteList = keywordList["EnAcceptWhiteList"];
        this->buyWhiteList = keywordList["EnBuyWhiteList"];
        this->completeWhiteList = keywordList["EnCompleteWhiteList"];
        this->randomWhiteList = keywordList["EnRandomWhiteList"];
    }

    bool DownloadAppRecognizer::RecognizeGooglePlaySearchBar(const cv::Mat &frame)
    {
        bool isRecognized = false;
        this->searchBarRect = EmptyRect;
        this->searchButtonRect = EmptyRect;

        cv::Rect roi(0, 0, frame.cols, frame.rows / 6);
        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        std::vector<cv::Rect> searchBarRects = ObjectUtil::Instance().DetectRealRectsByContour(roiFrame, DBL_MIN, DBL_MAX, 30000, 100000);
        if (!searchBarRects.empty())
        {
            searchBarRects = ObjectUtil::Instance().RemoveOuterRects(searchBarRects);
            searchBarRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), searchBarRects, roi);
            SortRects(searchBarRects, UType::UTYPE_Y, true);

            for (auto searchBarRect : searchBarRects)
            {
                cv::Rect barButtonROI(searchBarRect.x + searchBarRect.width / 2, searchBarRect.y, searchBarRect.width / 2, searchBarRect.height);
                cv::Mat barButtonFrame;
                ObjectUtil::Instance().CopyROIFrame(frame, barButtonROI, barButtonFrame);

                std::vector<cv::Rect> proposalRects = ObjectUtil::Instance().DetectRectsByContour(barButtonFrame, 50, 4, 1, 0.3, 3, 200, 1000);
                if (!proposalRects.empty())
                {
                    this->searchBarRect = searchBarRect;
                    this->searchButtonRect = proposalRects[0];
                    this->searchButtonRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), this->searchButtonRect, barButtonROI);
                    break;
                }
            }
        }

        if (this->searchBarRect != EmptyRect)
        {
            isRecognized = true;

#ifdef _DEBUG
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->searchBarRect, Red, 1);
            DrawRect(debugFrame, this->searchButtonRect, Green, 1);
            SaveImage(debugFrame, "debug_search_bar.png");
#endif

        }

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeAppCandidate(const cv::Mat &frame)
    {
        bool isRecognized = false;
        this->appRect = EmptyRect;

        cv::Rect roi(0, this->searchBarRect.br().y, frame.cols, frame.rows - this->searchBarRect.br().y);
        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        std::vector<cv::Rect> appRects = ObjectUtil::Instance().DetectRealRectsByContour(roiFrame, DBL_MIN, DBL_MAX, 50000, 120000);
        if (!appRects.empty())
        {
            appRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), appRects, roi);
            appRects = ObjectUtil::Instance().RemoveContainedRects(appRects);

            std::multimap<int, std::vector<cv::Rect>> appMap = ObjectUtil::Instance().HashRectanglesWithRectInterval(appRects, UType::UTYPE_CY, 2, 5, 15);
            if (!appMap.empty())
            {
                std::vector<cv::Rect> orderedAppRects;
                for (std::multimap<int, std::vector<cv::Rect>>::iterator it = appMap.begin(); it != appMap.end(); it++)
                {
                    AddVectorToVector(orderedAppRects, it->second);
                }

                if (this->isFree)
                {
                    this->keywordRecog->SetWhiteList(this->freeAppCandidateWhiteList);
                }
                else
                {
                    this->keywordRecog->SetWhiteList(this->paidAppCandidateWhiteList);
                }

                // Three kinds of apps: free, paid and purchased, purchased is always the first one
                for (size_t i = 0; i < orderedAppRects.size(); i++)
                {
                    cv::Rect roi(orderedAppRects[i].x + orderedAppRects[i].width / 2, orderedAppRects[i].y + orderedAppRects[i].height * 2 / 3, 
                        orderedAppRects[i].width / 2, orderedAppRects[i].height / 3);
                    cv::Mat roiFrame;
                    ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);
                    DebugSaveImage(roiFrame, "debug_price.png");

                    std::vector<cv::Rect> proposalPriceRects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 50, 6, 1, 1, 5, 500, 5000);
                    this->keywordRecog->SetProposalRects(proposalPriceRects, false);
                    std::vector<Keyword> allKeywords = this->keywordRecog->DetectAllKeywords(roiFrame, "en");

                    if (this->keywordRecog->RecognizeKeyword(allKeywords))
                    {
                        this->appRect = orderedAppRects[i];
                        break;
                    }
                    else if (i == 0) // If the first one is neither free nor paid, then is purchased
                    {
                        if (this->isFree)
                        {
                            this->keywordRecog->SetWhiteList(this->paidAppCandidateWhiteList);
                        }
                        else
                        {
                            this->keywordRecog->SetWhiteList(this->freeAppCandidateWhiteList);
                        }
                        if (!this->keywordRecog->RecognizeKeyword(allKeywords))
                        {
                            this->appRect = orderedAppRects[i];
                            break;
                        }
                        else
                        {
                            // Set keywords back
                            if (this->isFree)
                            {
                                this->keywordRecog->SetWhiteList(this->freeAppCandidateWhiteList);
                            }
                            else
                            {
                                this->keywordRecog->SetWhiteList(this->paidAppCandidateWhiteList);
                            }
                        }
                    }
                }

                if (this->appRect == EmptyRect)
                {
                    this->appRect = orderedAppRects[0];
                }
            }
        }

        if (this->appRect != EmptyRect)
        {
            isRecognized = true;

#ifdef _DEBUG
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->appRect, Red, 1);
            SaveImage(debugFrame, "debug_download_app.png");
#endif
        }

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeChoosePage(const cv::Mat &frame, cv::Rect &playRect, cv::Rect &alwaysRect)
    {
        bool isRecognized = false;

        std::vector<cv::Rect> playRects;
        playRects = ObjectUtil::Instance().DetectRectsByContour(frame, 50, 6, 1, 1, 10, 300, 10000);

        if (!playRects.empty())
        {
            this->keywordRecog->SetProposalRects(playRects, false);
            std::vector<Keyword> allKeywords = this->keywordRecog->DetectAllKeywords(frame, "en");

            this->keywordRecog->SetWhiteList(this->alwaysWhiteList);

            if (this->keywordRecog->RecognizeKeyword(allKeywords))
            {
                std::vector<Keyword> alwaysKeywords = this->keywordRecog->GetKeywords();
                if (!alwaysKeywords.empty())
                {
                    alwaysRect = alwaysKeywords[0].location;
                }
            }

            this->keywordRecog->SetWhiteList(this->playCandidateWhiteList);

            if (this->keywordRecog->RecognizeKeyword(allKeywords))
            {
                std::vector<Keyword> playStoreKeywords = this->keywordRecog->GetKeywords();
                std::vector<cv::Rect> playStoreRects;
                for (auto rect : playStoreKeywords)
                {
                    if (alwaysRect == EmptyRect || rect.location.br().y < alwaysRect.br().y)
                    {
                        playStoreRects.push_back(rect.location);
                    }
                }

                if (!playStoreRects.empty())
                {
                    if (playStoreRects.size() > 1)
                    {
                        SortRects(playStoreRects, UType::UTYPE_Y, false);
                    }
                    playRect = playStoreRects[0];
                }
            }
        }

        if (playRect != EmptyRect)
        {
            isRecognized = true;

#ifdef _DEBUG
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, playRect, Red, 2);
            if(alwaysRect != EmptyRect)
            {
                DrawRect(debugFrame, alwaysRect, Red, 2);
            }
            SaveImage(debugFrame, "debug_play_always.png");
#endif
        }

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeDownloadPage(const cv::Mat &frame, DownloadStage downloadStage)
    {
        bool isRecognized = false;
        this->priceRect = EmptyRect;
        this->recognizedPrice = 0.0;
        this->recognizedPriceStr = "";

        // Paid is price-"ACCEPT"-"BUY", free is "INSTALL"-"ACCEPT"
        std::vector<cv::Rect> priceRects;
        std::vector<cv::Rect> proposalPriceRects;
        priceRects = ObjectUtil::Instance().DetectRealRectsByContour(frame, 2, 10, 2000, 100000);

        for (auto rect : priceRects)
        {
            // Shrink to make OCR more precise
            rect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(rect, frame.size(), 5, 5, false);
            proposalPriceRects.push_back(rect);
        }

        if (!proposalPriceRects.empty())
        {
            this->keywordRecog->SetProposalRects(proposalPriceRects, false);
            std::vector<Keyword> allKeywords = this->keywordRecog->DetectAllKeywords(frame, "en");

            switch(downloadStage)
            {
            case DownloadStage::PRICE:
                this->keywordRecog->SetWhiteList(this->priceWhiteList);
                break;
            case DownloadStage::ACCEPT:
                this->keywordRecog->SetWhiteList(this->acceptWhiteList);
                break;
            case DownloadStage::BUY:
                this->keywordRecog->SetWhiteList(this->buyWhiteList);
                break;
            default:
                this->keywordRecog->SetWhiteList(this->priceWhiteList);
                break;
            }

            isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);
            if (isRecognized)
            {
                std::vector<Keyword> priceKeywords = this->keywordRecog->GetKeywords();
                if (!priceKeywords.empty())
                {
                    this->priceRect = priceKeywords[0].location;
                    std::wstring wstr = priceKeywords[0].text;
                    this->recognizedPriceStr = WstrToStr(wstr);

                    if (downloadStage == DownloadStage::PRICE)
                    {
                        std::string recognizedPriceSubStr = this->recognizedPriceStr.substr(1);
                        if (!TryParse(recognizedPriceSubStr, this->recognizedPrice)) // Only digit can parse
                        {
                            this->isFree = true;
                            boost::to_lower(this->recognizedPriceStr);
                            DAVINCI_LOG_INFO << "Download free applicaton: " << this->recognizedPriceStr << endl;
                        }
                        else
                        {
                            this->isFree = false;
                            DAVINCI_LOG_INFO << "Download paid applicaton: " << this->recognizedPriceStr << endl;
                        }
                    }
                }
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->priceRect, Red, 2);
            SaveImage(debugFrame, "debug_price.png");
        }
#endif

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeDownloadCompletePage(const cv::Mat &frame, const cv::Rect &priceButtonLocation)
    {
        bool isRecognized = false;

        cv::Rect frameRect(EmptyPoint, frame.size());
        cv::Rect roi(0, priceButtonLocation.y - priceButtonLocation.height, frame.cols, 3 * priceButtonLocation.height);
        roi &= frameRect;

        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);
        std::vector<cv::Rect> proposalRects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 50, 5, 1, 3 ,10, 1000, 5000);

        this->keywordRecog->SetProposalRects(proposalRects, false);
        this->keywordRecog->SetWhiteList(this->completeWhiteList);

        std::vector<Keyword> allKeywords = this->keywordRecog->DetectAllKeywords(roiFrame, "en");
        isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeRandomKeyword(const cv::Mat &frame)
    {
        bool isRecognized = false;
        this->randomRect = EmptyRect;

        std::vector<cv::Rect> proposalRects = ObjectUtil::Instance().DetectRectsByContour(frame, 50, 5, 1, 1, 10, 200, 10000);

        this->keywordRecog->SetProposalRects(proposalRects, false);
        this->keywordRecog->SetWhiteList(this->randomWhiteList);

        std::vector<Keyword> allKeywords = this->keywordRecog->DetectAllKeywords(frame, "en");
        isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);

        if (isRecognized)
        {
            std::vector<Keyword> randomKeywords = this->keywordRecog->GetKeywords();

            if (!randomKeywords.empty())
            {
                std::sort(randomKeywords.begin(), randomKeywords.end(), SortKeywordsByKeywordPriorityDesc());
                if(randomKeywords[0].priority != Priority::LOW)
                {
                    this->randomRect = randomKeywords[0].location;
                }
                else
                {
                    std::sort(randomKeywords.begin(), randomKeywords.end(), SortKeywordsByKeywordLocationYAsc());
                    this->randomRect = randomKeywords[randomKeywords.size() - 1].location;
                }
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, this->randomRect, Red, 2);
            SaveImage(debugFrame, "debug_random.png");
        }
#endif

        return isRecognized;
    }

    bool DownloadAppRecognizer::RecognizeIncompatible(const cv::Mat &frame)
    {
        bool isRecognized = false;

        cv::Rect roi(0, frame.rows / 3, frame.cols, frame.rows / 3);
        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);
        std::vector<cv::Rect> rects = TextUtil::Instance().ExtractRSTRects(frame);

        this->keywordRecog->SetProposalRects(rects, false);
        this->keywordRecog->SetWhiteList(this->incompatibleWhiteList);

        std::vector<Keyword> allKeywords;

        const int minHeight = 32;
        for(size_t i = 0;i<rects.size();++i)
        {
            Mat roi = frame(rects[i]);
            if (roi.rows < minHeight)
            {
                Mat resizeImg;
                float scale = (float)minHeight/roi.rows;
                resize(roi,resizeImg,Size(),scale,scale);
                vector<Keyword> roiKeyWord =  this->keywordRecog->DetectKeywordsByOCR(resizeImg,"en");
                for (size_t j = 0;j < roiKeyWord.size();++j)
                {
                    roiKeyWord[j].location.x = int((float)roiKeyWord[j].location.x /scale) + rects[i].x;
                    roiKeyWord[j].location.y = int((float)roiKeyWord[j].location.y /scale) + rects[i].y;
                    roiKeyWord[j].location.width = int((float)roiKeyWord[j].location.width /scale);
                    roiKeyWord[j].location.height = int((float)roiKeyWord[j].location.height /scale);
                }
                allKeywords.insert(allKeywords.end(),roiKeyWord.begin(),roiKeyWord.end());
            }
            else
            {
                 vector<Keyword> roiKeyWord =  this->keywordRecog->DetectKeywordsByOCR(roi,"en");
                  allKeywords.insert(allKeywords.end(),roiKeyWord.begin(),roiKeyWord.end());
            }
          
        }
        isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);

        return isRecognized;
    }
}

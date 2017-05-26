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
#include "LicenseRecognizer.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "TestManager.hpp"

using namespace boost;
using namespace cv;
using namespace std;

namespace DaVinci
{
    LicenseRecognizer::LicenseRecognizer()
    {
        this->objectCategory = "License Page";

        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetNeedBinarization(false);
        this->keywordRecog->SetCheckBlackList(false);
        this->keywordRecog->SetIsNegativePrior(false);

        std::map<std::string, std::vector<Keyword>> keywordList;
        std::string keywordCategory = KeywordUtil::Instance().KeywordCategoryToString(KeywordCategory::LICENSE);
        if (TestManager::Instance().FindKeyFromKeywordMap(keywordCategory))
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap(keywordCategory);
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }

        this->allLanguages.push_back("en");
        this->allLanguages.push_back("zh");

        this->agreeRect = EmptyRect;
        this->checkBoxRect = EmptyRect;
    }

    bool LicenseRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        this->agreeRect = EmptyRect;
        this->checkBoxRect = EmptyRect;

        cv::Mat image = attributes.image;

        bool isRecognized = RecognizeLicense(image, allEnKeywords, allZhKeywords);

        if (isRecognized)
        {
            // Recognize checkbox
            RecognizeCheckBox(image, this->agreeRect, allEnKeywords, allZhKeywords);
        }

        return isRecognized;
    }

    std::string LicenseRecognizer::GetCategoryName()
    {
        return this->objectCategory;
    }

    cv::Rect LicenseRecognizer::GetAgreeRect()
    {
        return this->agreeRect;
    }

    cv::Rect LicenseRecognizer::GetCheckBoxRect()
    {
        return this->checkBoxRect;
    }

    void LicenseRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 4);

        this->enPositiveWhiteList = keywordList["EnPositiveWhiteList"];
        this->enCheckBoxWhiteList = keywordList["EnCheckBoxWhiteList"];
        this->zhPositiveWhiteList = keywordList["ZhPositiveWhiteList"];
        this->zhCheckBoxWhiteList = keywordList["ZhCheckBoxWhiteList"];
    }

    bool LicenseRecognizer::RecognizeLicense(const cv::Mat &image, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        bool isRecognized = false;
        this->agreeRect = EmptyRect;

        std::vector<Keyword> allKeywords;
        std::vector<Keyword> positiveWhiteList;
        for (auto language : this->allLanguages)
        {
            if (boost::equals(language, "zh"))
            {
                allKeywords = KeywordUtil::Instance().FilterKeywords(allZhKeywords, 1, 10, 200, 10000);
                positiveWhiteList = this->zhPositiveWhiteList;
            }
            else
            {
                allKeywords = KeywordUtil::Instance().FilterKeywords(allEnKeywords, 1, 10, 200, 10000);
                positiveWhiteList = this->enPositiveWhiteList;
            }
            std::sort(allKeywords.begin(), allKeywords.end(), SortKeywordsByKeywordLocationYDesc());

            this->keywordRecog->SetWhiteList(positiveWhiteList);
            if (this->keywordRecog->RecognizeKeyword(allKeywords))
            {
                std::vector<Keyword> agreeKeywords = this->keywordRecog->GetKeywords();
                if (!agreeKeywords.empty())
                {
                    this->agreeRect = agreeKeywords[0].location;
                    isRecognized = true;
                    break;
                }
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            cv::Mat debugFrame = image.clone();
            DrawRect(debugFrame, this->agreeRect, Red, 2);
            SaveImage(debugFrame, "debug_license_agree.png");
        }
#endif

        return isRecognized;
    }

    bool LicenseRecognizer::RecognizeCheckBox(const cv::Mat &image, const cv::Rect &agreeRect, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        bool isRecognized = false;
        this->checkBoxRect = EmptyRect;

        std::vector<Keyword> allKeywords;
        std::vector<Keyword> checkBoxWhiteList;

        cv::Rect imageRect(EmptyPoint, image.size());

        // only focus on the rectangle above agree Rect
        cv::Rect checkBoxROI(0, 0, image.cols, agreeRect.y);
        cv::Mat checkBoxFrame;
        ObjectUtil::Instance().CopyROIFrame(image, checkBoxROI, checkBoxFrame);

        std::vector<Rect> checkBoxRects = ObjectUtil::Instance().DetectRealRectsByContour(checkBoxFrame, 0.7, 1.5, 300, 1000);
        checkBoxRects = ObjectUtil::Instance().RelocateRectangleList(image.size(), checkBoxRects, checkBoxROI);

        for (auto rect : checkBoxRects)
        {
            // Construct ROI
            cv::Rect extendedRect(rect.x - rect.width, rect.y - 2 * rect.height, image.cols / 2, 4 * rect.height);
            extendedRect &= imageRect;

            std::vector<Keyword> allKeywords;
            for (auto language : this->allLanguages)
            {
                if (boost::equals(language, "zh"))
                {
                    allKeywords = allZhKeywords;
                    checkBoxWhiteList = this->zhCheckBoxWhiteList;
                }
                else
                {
                    allKeywords = allEnKeywords;
                    checkBoxWhiteList = this->enCheckBoxWhiteList;
                }

                this->keywordRecog->SetWhiteList(checkBoxWhiteList);
                if (this->keywordRecog->RecognizeKeyword(allKeywords))
                {
                    std::vector<Keyword> checkBoxKeywords = this->keywordRecog->GetKeywords();
                    for (auto keyword : checkBoxKeywords)
                    {
                        if (IntersectWith(extendedRect, keyword.location))
                        {
                            this->checkBoxRect = rect;
                            isRecognized = true;
                            break;
                        }
                    }
                }

                if (isRecognized)
                {
                    break;
                }
            }

            if (isRecognized)
            {
                break;
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            cv::Mat debugFrame = image.clone();
            DrawRect(debugFrame, this->checkBoxRect, Red, 2);
            SaveImage(debugFrame, "debug_license_check_box.png");
        }
#endif

        return isRecognized;
    }
}

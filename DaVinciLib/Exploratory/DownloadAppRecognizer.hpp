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

#ifndef __DOWNLOADAPPRECOGNIZER__
#define __DOWNLOADAPPRECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "KeywordRecognizer.hpp"

namespace DaVinci
{
    enum DownloadStage
    {
        NOTBEGIN,
        CHOOSE,
        PRICE, // or INSTALL
        ACCEPT,
        BUY,
        COMPLETE, // Complete download steps
        STOP // Should stop download, for the cases app has been installed or price exceeds limited price
    };

    /// <summary>
    /// Download app recognizer
    /// </summary>
    class DownloadAppRecognizer : public BaseObjectCategory
    {
    private:
        std::string objectCategory;

        cv::Rect searchBarRect;
        cv::Rect searchButtonRect;
        cv::Rect appRect;
        cv::Rect priceRect;
        cv::Rect randomRect;
        std::string recognizedPriceStr;
        double recognizedPrice;
        bool isFree;

        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        std::vector<Keyword> freeAppCandidateWhiteList;
        std::vector<Keyword> paidAppCandidateWhiteList;
        std::vector<Keyword> playCandidateWhiteList;
        std::vector<Keyword> alwaysWhiteList;
        std::vector<Keyword> incompatibleWhiteList;
        std::vector<Keyword> priceWhiteList;
        std::vector<Keyword> acceptWhiteList;
        std::vector<Keyword> buyWhiteList;
        std::vector<Keyword> completeWhiteList;

        std::vector<Keyword> randomWhiteList;

    public:
        /// <summary>
        /// Download app page
        /// </summary>
        DownloadAppRecognizer();

        /// <summary>
        /// Check wheteher the object attributes satisfies the category
        /// </summary>
        /// <param name="attributes"></param>
        /// <returns></returns>
        virtual bool BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects = std::vector<cv::Rect>(), std::vector<cv::Rect> allZhProposalRects = std::vector<cv::Rect>(), std::vector<Keyword> allEnKeywords = std::vector<Keyword>(), std::vector<Keyword> allZhKeywords = std::vector<Keyword>()) override;

        /// <summary>
        /// Get object category name
        /// </summary>
        /// <returns></returns>
        virtual std::string GetCategoryName() override;

        cv::Rect GetSearchBarRect();

        void SetSearchBarRect(const cv::Rect &searchBarRect);

        cv::Rect GetSearchButtonRect();

        /// <summary>
        /// Get app rectangle
        /// </summary>
        /// <returns></returns>
        cv::Rect GetAppRect();

        /// <summary>
        /// Get price rectangle
        /// </summary>
        /// <returns></returns>
        cv::Rect GetPriceRect();

        cv::Rect GetRandomRect();

        /// <summary>
        /// Get recognized price str
        /// </summary>
        /// <returns></returns>
        std::string GetRecognizedPriceStr();

        /// <summary>
        /// Get recognized price
        /// </summary>
        /// <returns></returns>
        double GetRecognizedPrice();

        bool IsFree();

        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool RecognizeGooglePlaySearchBar(const cv::Mat &frame);

        /// <summary>
        /// Recognize app candidate
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="minArea"></param>
        /// <param name="interval"></param>
        /// <returns></returns>
        bool RecognizeAppCandidate(const cv::Mat &frame);

        /// <summary>
        /// Recognize choose page
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="minArea"></param>
        /// <param name="interval"></param>
        /// <returns></returns>
        bool RecognizeChoosePage(const cv::Mat &frame, cv::Rect &playRect, cv::Rect &alwaysRect);

        /// <summary>
        /// Recognize download page, including INSTALL-ACCEPT or price-ACCEPT-BUY
        /// </summary>
        /// <param name="frame"></param>
        /// <param name="downloadStage"></param>
        /// <returns></returns>
        bool RecognizeDownloadPage(const cv::Mat &frame, DownloadStage downloadStage);

        bool RecognizeDownloadCompletePage(const cv::Mat &frame, const cv::Rect &priceButtonLocation);

        bool RecognizeRandomKeyword(const cv::Mat &frame);

        bool RecognizeIncompatible(const cv::Mat &frame);
    };
}


#endif	//#ifndef __DOWNLOADAPPRECOGNIZER__

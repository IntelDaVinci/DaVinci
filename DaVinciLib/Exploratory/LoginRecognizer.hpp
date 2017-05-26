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

#ifndef __LOGINRECOGNIZER__
#define __LOGINRECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "boost/algorithm/string.hpp"
#include "KeywordRecognizer.hpp"
#include <vector>

namespace DaVinci
{
    class LoginRecognizer : public BaseObjectCategory
    {
    private:
        std::string categoryName;
        std::string languageMode;
        std::vector<std::string> allLanguages;

        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        // login page pattern one: account rectangle and password rectangle
        // login page pattern two: account rectangle and next button
        bool isPatternOne;

        std::vector<Keyword> enAccountWhiteList;
        std::vector<Keyword> enPasswordWhiteList;
        std::vector<Keyword> enNextWhiteList;
        std::vector<Keyword> enLoginButtonForLoginPageWhiteList;
        std::vector<Keyword> enLoginButtonWhiteList;
        std::vector<Keyword> enApkWhiteList;
        std::vector<Keyword> enBlackList;
        std::vector<Keyword> enLoginButtonAccurateKeyword;

        std::vector<Keyword> zhAccountWhiteList;
        std::vector<Keyword> zhPasswordWhiteList;
        std::vector<Keyword> zhNextWhiteList;
        std::vector<Keyword> zhLoginButtonForLoginPageWhiteList;
        std::vector<Keyword> zhLoginButtonWhiteList;
        std::vector<Keyword> zhApkWhiteList;
        std::vector<Keyword> zhBlackList;
        std::vector<Keyword> zhLoginButtonAccurateKeyword;

        std::vector<std::vector<Keyword>> keywordList;

        std::vector<cv::Rect> loginAccountRects;
        std::vector<cv::Rect> loginPasswordRects;
        std::vector<cv::Rect> loginLabelRects;
        std::vector<cv::Rect> loginInputRects;
        std::vector<cv::Rect> loginButtonRects;

    public:
        // Construct funtion
        LoginRecognizer();

        virtual bool BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects = std::vector<cv::Rect>(), std::vector<cv::Rect> allZhProposalRects = std::vector<cv::Rect>(), std::vector<Keyword> allEnKeywords = std::vector<Keyword>(), std::vector<Keyword> allZhKeywords = std::vector<Keyword>()) override;

        // Getter
        virtual std::string GetCategoryName() override;

        bool IsPatternOne();

        std::vector<cv::Rect> GetLoginInputRects();

        std::vector<cv::Rect> &GetLoginButtonRects();

        // Setter
        void SetLanguageMode(std::string languageMode);

        void InitializeAllLanguages();

        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool RecognizeLoginButton(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, bool isForLoginPage = false, bool isAccurateCheck = false);

        bool RecognizeLoginPage(const cv::Mat &frame, const std::vector<Keyword> &allKeywords);

        int RecognizeNewPassword(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, int oldAccountBoundary);

        void DetectLoginKeywordsByContour(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, bool isAccountKnown = false);

        std::vector<cv::Rect> LocateInputFields(const cv::Mat &frame, const std::vector<cv::Rect> &labelRects);

        std::vector<cv::Rect> LocateLabelFields(const cv::Mat &frame, std::vector<cv::Rect> &accountKeywordRects, std::vector<cv::Rect> &passwordKeywordRects);

        cv::Rect LocateKeywordPrecisely(Keyword keyword);

        int DetectInputFieldWidth(const cv::Mat &frame, const std::vector<cv::Rect> &rects);
    };
}

#endif
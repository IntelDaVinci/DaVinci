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
#include "LoginRecognizer.hpp"
#include "ObjectCommon.hpp"
#include "ObjectUtil.hpp"
#include "TestManager.hpp"

#include "unzipper.hpp"
#include "zip.h"
#include <regex>
#include <locale>
#include <codecvt>
#include <fstream>

using namespace ziputils;

namespace DaVinci
{
    LoginRecognizer::LoginRecognizer()
    {
        this->categoryName = "Login";
        this->isPatternOne = true;
        this->InitializeAllLanguages();

        this->keywordRecog = boost::shared_ptr<KeywordRecognizer>(new KeywordRecognizer());
        this->keywordRecog->SetPageIteratorLevel(PageIteratorLevel::RIL_TEXTLINE);
        this->keywordRecog->SetIsNegativePrior(false);
        this->keywordRecog->SetCheckBlackList(true);

        std::map<std::string, std::vector<Keyword>> keywordList;
        if (TestManager::Instance().FindKeyFromKeywordMap("SmartLogin") )
        {
            keywordList = TestManager::Instance().GetValueFromKeywordMap("SmartLogin");
            assert(!keywordList.empty());
            this->InitializeKeywordList(keywordList);
        }
    }

    bool LoginRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        return true;
    }

    std::string LoginRecognizer::GetCategoryName()
    {
        return this->categoryName;
    }

    bool LoginRecognizer::IsPatternOne()
    {
        return this->isPatternOne;
    }

    std::vector<cv::Rect> LoginRecognizer::GetLoginInputRects()
    {
        return this->loginInputRects;
    }

    std::vector<cv::Rect> &LoginRecognizer::GetLoginButtonRects()
    {
        return this->loginButtonRects;
    }

    void LoginRecognizer::SetLanguageMode(std::string languageMode)
    {
        this->languageMode = languageMode;
    }

    void LoginRecognizer::InitializeAllLanguages()
    {
        this->allLanguages.push_back("en");
        this->allLanguages.push_back("zh");
    }

    void LoginRecognizer::InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList)
    {
        assert(keywordList.size() == 16);

        this->enAccountWhiteList = keywordList["EnAccountWhiteList"];
        this->enPasswordWhiteList = keywordList["EnPasswordWhiteList"];
        this->enNextWhiteList = keywordList["EnNextWhiteList"];
        this->enLoginButtonForLoginPageWhiteList = keywordList["EnLoginButtonForLoginPageWhiteList"];
        this->enLoginButtonWhiteList = keywordList["EnLoginButtonWhiteList"];
        this->enApkWhiteList = keywordList["EnApkWhiteList"];
        this->enBlackList = keywordList["EnBlackList"];
        this->enLoginButtonAccurateKeyword = keywordList["EnLoginButtonAccurateKeyword"];

        this->zhAccountWhiteList = keywordList["ZhAccountWhiteList"];
        this->zhPasswordWhiteList = keywordList["ZhPasswordWhiteList"];
        this->zhNextWhiteList = keywordList["ZhNextWhiteList"];
        this->zhLoginButtonForLoginPageWhiteList = keywordList["ZhLoginButtonForLoginPageWhiteList"];
        this->zhLoginButtonWhiteList = keywordList["ZhLoginButtonWhiteList"];
        this->zhApkWhiteList = keywordList["ZhApkWhiteList"];
        this->zhBlackList = keywordList["ZhBlackList"];
        this->zhLoginButtonAccurateKeyword = keywordList["ZhLoginButtonAccurateKeyword"];
    }

    bool LoginRecognizer::RecognizeLoginButton(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, bool isForLoginPage, bool isAccurateCheck)
    {
        bool isRecognized = false;
        this->loginButtonRects.clear();

        if (boost::equals(this->languageMode, "zh"))
        {
            if (isForLoginPage)
            {
                this->keywordRecog->SetWhiteList(this->zhLoginButtonForLoginPageWhiteList);
            }
            else if (isAccurateCheck)
            {
                this->keywordRecog->SetWhiteList(this->zhLoginButtonAccurateKeyword);
            }
            else
            {
                this->keywordRecog->SetWhiteList(this->zhLoginButtonWhiteList);
            }
            this->keywordRecog->SetBlackList(this->zhBlackList);
        }
        else
        {
            if (isForLoginPage)
            {
                this->keywordRecog->SetWhiteList(this->enLoginButtonForLoginPageWhiteList);
            }
            else if (isAccurateCheck)
            {
                this->keywordRecog->SetWhiteList(this->enLoginButtonAccurateKeyword);
            }
            else
            {
                this->keywordRecog->SetWhiteList(this->enLoginButtonWhiteList);
            }
            this->keywordRecog->SetBlackList(this->enBlackList);
        }

        isRecognized = this->keywordRecog->RecognizeKeyword(allKeywords);
        if (isRecognized)
        {
            std::vector<Keyword> buttonKeywords = this->keywordRecog->GetKeywords();
            for (size_t i = 0; i < buttonKeywords.size(); i++)
            {
                if (!boost::equals(this->languageMode, "zh") && buttonKeywords[i].priority == Priority::LOW)
                {
                    buttonKeywords[i].location = this->LocateKeywordPrecisely(buttonKeywords[i]);
                }
            }

            if (isForLoginPage)
            {
                std::vector<cv::Rect> rects = ObjectUtil::Instance().DetectRealRectsByContour(frame, DBL_MIN, DBL_MAX, 2500, 125000);
                for (auto keyword : buttonKeywords)
                {
                    for (auto rect : rects)
                    {
                        if (ContainRect(rect, keyword.location))
                        {
                            this->loginButtonRects.push_back(keyword.location);
                        }
                    }
                }

                if (this->loginButtonRects.empty())
                {
                    isRecognized = false;
                }
            }
            else
            {
                // Sort login button desc by priority
                if (buttonKeywords.size() > 1)
                {
                    std::sort(buttonKeywords.begin(), buttonKeywords.end(), SortKeywordsByKeywordPriorityDesc());
                }

                for (auto keyword : buttonKeywords)
                {
                    this->loginButtonRects.push_back(keyword.location);
                }
            }
        }

#ifdef _DEBUG
        if (isRecognized)
        {
            cv::Mat cloneFrame = frame.clone();
            DrawRect(cloneFrame, this->loginButtonRects.front(), Red, 2);
            SaveImage(cloneFrame, "login_button_result.png");
        }
#endif

        return isRecognized;
    }

    bool LoginRecognizer::RecognizeLoginPage(const cv::Mat &frame, const std::vector<Keyword> &allKeywords)
    {
        bool isRecognized = false;
        cv::Mat cloneLoginFrame = frame.clone();

#ifdef _DEBUG
        cv::Mat cloneFrame = frame.clone();
#endif

        // Label/Keyword: |___keyword__|
        this->DetectLoginKeywordsByContour(frame, allKeywords);

#ifdef _DEBUG
        DrawRects(cloneFrame, this->loginAccountRects, Blue, 1);
        DrawRects(cloneFrame, this->loginPasswordRects, Blue, 1);
        SaveImage(cloneFrame, "login_keywords.png");
#endif

        this->loginLabelRects = this->LocateLabelFields(frame, this->loginAccountRects, this->loginPasswordRects);
        if (!this->loginLabelRects.empty())
        {
            this->loginInputRects = this->LocateInputFields(cloneLoginFrame, this->loginLabelRects);
            if (this->loginInputRects.empty())
            {
                this->loginInputRects = this->loginLabelRects;
            }
            isRecognized = true;

#ifdef _DEBUG
            DrawRects(cloneFrame, this->loginLabelRects, Green, 2);
            DrawRects(cloneFrame, this->loginInputRects, Red, 2);
            SaveImage(cloneFrame, "login_rect_result.png");
#endif
        }

        return isRecognized;
    }

    int LoginRecognizer::RecognizeNewPassword(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, int oldAccountBoundary)
    {
        cv::Rect newPasswordRect = EmptyRect;
        std::vector<Keyword> passwordKeywords;

        for (auto keyword : allKeywords)
        {
            if (keyword.location.y > oldAccountBoundary)
            {
                passwordKeywords.push_back(keyword);
            }
        }

        this->DetectLoginKeywordsByContour(frame, passwordKeywords, true);

        if (!this->loginPasswordRects.empty())
        {
            if (this->loginPasswordRects.size() > 1)
            {
                SortRects(this->loginPasswordRects, UType::UTYPE_CC, true);
            }
            newPasswordRect = this->loginPasswordRects[0];

#ifdef _DEBUG
            cv::Mat cloneFrame = frame.clone();
            DrawRect(cloneFrame, newPasswordRect, Red, 2);
            SaveImage(cloneFrame, "login_password.png");
#endif
        }

        return newPasswordRect.y;
    }

    void LoginRecognizer::DetectLoginKeywordsByContour(const cv::Mat &frame, const std::vector<Keyword> &allKeywords, bool isAccountKnown)
    {
        this->loginAccountRects.clear();
        this->loginPasswordRects.clear();

        std::vector<Keyword> accountWhiteList;
        std::vector<Keyword> passwordWhiteList;
        std::vector<Keyword> nextWhiteList;
        std::vector<Keyword> blackList;

        if (boost::equals(this->languageMode, "zh"))
        {
            accountWhiteList = this->zhAccountWhiteList;
            passwordWhiteList = this->zhPasswordWhiteList;
            nextWhiteList = this->zhNextWhiteList;
            blackList = this->zhBlackList;
        }
        else
        {
            accountWhiteList = this->enAccountWhiteList;
            passwordWhiteList = this->enPasswordWhiteList;
            nextWhiteList = this->enNextWhiteList;
            blackList = this->enBlackList;
        }

        this->keywordRecog->SetBlackList(blackList);
        if (!isAccountKnown)
        {
            this->keywordRecog->SetWhiteList(accountWhiteList);
            if (this->keywordRecog->RecognizeKeyword(allKeywords))
            {
                std::vector<Keyword> accountKeywords = this->keywordRecog->GetKeywords();
                for (auto keyword : accountKeywords)
                {
                    this->loginAccountRects.push_back(keyword.location);
                }
            }
        }

        this->keywordRecog->SetWhiteList(passwordWhiteList);
        if (this->keywordRecog->RecognizeKeyword(allKeywords)) // Pattern one: account rectangle and password rectangle
        {
            this->isPatternOne = true;
            std::vector<Keyword> passwordKeywords = this->keywordRecog->GetKeywords();
            for (auto keyword : passwordKeywords)
            {
                this->loginPasswordRects.push_back(keyword.location);
            }
        }
        else
        {
            this->keywordRecog->SetWhiteList(nextWhiteList);
            if (this->keywordRecog->RecognizeKeyword(allKeywords)) // Pattern two: account rectangle and next button
            {
                this->isPatternOne = false;
                std::vector<Keyword> passwordKeywords = this->keywordRecog->GetKeywords();
                for (auto keyword : passwordKeywords)
                {
                    this->loginPasswordRects.push_back(keyword.location);
                }
            }
        }
    }

    std::vector<cv::Rect> LoginRecognizer::LocateInputFields(const cv::Mat &frame, const std::vector<cv::Rect> &labelRects)
    {
        std::vector<cv::Rect> inputFields;
        bool isAccount = false;
        bool isPassword = false;
        cv::Rect frameRect(EmptyPoint, frame.size());

#ifdef _DEBUG
        cv::Mat cloneFrame = frame.clone();
#endif

        std::vector<cv::Rect> suspectedInputRects = ObjectUtil::Instance().DetectRealRectsByContour(frame, DBL_MIN, DBL_MAX, 12000, 125000);

        suspectedInputRects = ObjectUtil::Instance().RemoveOuterRects(suspectedInputRects);

        // From top to bottom, the first is account
        SortRects(suspectedInputRects, UType::UTYPE_Y, true);

        for (auto rect : suspectedInputRects)
        {
            if (!inputFields.empty() && (rect & inputFields.front()) != EmptyRect)
            {
                continue;
            }

            bool isDetected = false;

#ifdef _DEBUG
            DrawRect(cloneFrame, rect, Red, 1);
            SaveImage(cloneFrame, "login_input.png");
#endif

            if (!isAccount && ContainRect(rect, labelRects[0]))
            {
                inputFields.push_back(rect);
                isAccount = true;
                isDetected = true;
            }
            if (!isPassword && ContainRect(rect, labelRects[1]))
            {
                inputFields.push_back(rect);
                isPassword = true;
                isDetected = true;
            }
            if (isAccount && isPassword)
            {
                break;
            }
            if (isDetected)
            {
                continue;
            }

            cv::Rect extendedRect(rect.x - 20, rect.y - 20, rect.width + 40, rect.height + 40);
            extendedRect &= frameRect;

#ifdef _DEBUG
            DrawRect(cloneFrame, extendedRect, Blue, 1);
            SaveImage(cloneFrame, "login_input.png");
#endif

            if (!isAccount && (extendedRect & labelRects[0]) != EmptyRect)
            {
                inputFields.push_back(rect);
                isAccount = true;
            }
            else if (!isPassword && (extendedRect & labelRects[1]) != EmptyRect)
            {
                inputFields.push_back(rect);
                isPassword = true;
            }
            if (isAccount && isPassword)
            {
                break;
            }
        }

        if (inputFields.size() == 2)
        {
            // Account and password are in the same big rectangle
            if (inputFields[0] == inputFields[1]) // No uncorrect recognition
            {
                inputFields = ObjectUtil::Instance().SplitRectangleByNumber(inputFields[0], false);
            }
            else if ((double)inputFields[0].height / inputFields[1].height >= 1.5 && inputFields[0].height / labelRects[0].height >= 4) // Password is false positive
            {
                inputFields = ObjectUtil::Instance().SplitRectangleByNumber(inputFields[0], false);
            }
            else if ((double)inputFields[1].height / inputFields[0].height >= 1.5 && inputFields[1].height / labelRects[1].height >= 4) // Account is false positive
            {
                inputFields = ObjectUtil::Instance().SplitRectangleByNumber(inputFields[1], false);
            }
        }
        else // If input rect not found, just extend rect width
        {
            inputFields.clear();
            int width = this->DetectInputFieldWidth(frame, labelRects);
            if (width > 0)
            {
                for (auto labelRect : labelRects)
                {
                    cv::Rect rect(labelRect.x, labelRect.y, width, labelRect.height);
                    rect &= frameRect;
                    inputFields.push_back(rect);
                }
            }
            else
            {
                for (auto labelRect : labelRects)
                {
                    cv::Rect rect(labelRect.x, labelRect.y, labelRect.width * 4, labelRect.height);
                    rect &= frameRect;
                    inputFields.push_back(rect);
                }
            }
        }

        return inputFields;
    }

    std::vector<cv::Rect> LoginRecognizer::LocateLabelFields(const cv::Mat &frame, std::vector<cv::Rect> &accountKeywordRects, std::vector<cv::Rect> &passwordKeywordRects)
    {
        std::vector<cv::Rect> labelFields;
        std::vector<cv::Rect> suspectedRects;
        cv::Rect accountKeywordRect = EmptyRect;
        cv::Rect passwordKeywordRect = EmptyRect;

        if (!passwordKeywordRects.empty())
        {
            // Choose left top one when there are several password rectangles (usually "password" and "forget password") 
            if (passwordKeywordRects.size() > 1)
            {
                SortRects(passwordKeywordRects, UType::UTYPE_Y, true);
            }
            passwordKeywordRect = passwordKeywordRects.front();

            if (!accountKeywordRects.empty())
            {
                suspectedRects = accountKeywordRects;
            }
            else
            {
                suspectedRects = ObjectUtil::Instance().DetectRealRectsByContour(frame, DBL_MIN, DBL_MAX, 12000, 125000);
            }

            if (passwordKeywordRect != EmptyRect)
            {
                // Select all the possible account rectangles by height interval
                std::vector<cv::Rect> rects = ObjectUtil::Instance().GetRectsWithInterval(passwordKeywordRect, suspectedRects, 150, true);
                for (auto rect : rects)
                {
                    // Pattern one: account rectangle and password rectangle
                    // Pattern two: account rectangle and next button
                    if ((this->isPatternOne && abs(passwordKeywordRect.x - rect.x) <= 150 && abs(passwordKeywordRect.height - rect.height) <= 80)
                        || (!this->isPatternOne && abs(passwordKeywordRect.height - rect.height) <= 80))
                    {
                        accountKeywordRect = rect;
                        break;
                    }
                }
            }
        }

        if (accountKeywordRect != EmptyRect && passwordKeywordRect != EmptyRect)
        {
            labelFields.push_back(accountKeywordRect);
            labelFields.push_back(passwordKeywordRect);
        }

        return labelFields;
    }

    cv::Rect LoginRecognizer::LocateKeywordPrecisely(Keyword keyword)
    {
        cv::Rect rect = keyword.location;
        std::string textStr = WstrToStr(keyword.text);
        boost::trim(textStr);
        boost::to_lower(textStr);

        int offset = (int)textStr.rfind("log");
        offset *= (rect.width / (int)textStr.length());
        if (offset > 0)
        {
            rect.x += offset;
            rect.width -= offset;
        }
        else if (offset < 0)
        {
            offset = (int)textStr.rfind("sign");
            offset *= (rect.width / (int)textStr.length());
            if (offset > 0)
            {
                rect.x += offset;
                rect.width -= offset;
            }
        }

        return rect;
    }

    int LoginRecognizer::DetectInputFieldWidth(const cv::Mat &frame, const std::vector<cv::Rect> &rects)
    {
        int x = 0;
        int width = 0;

        cv::Mat grayFrame = BgrToGray(frame);
        cv::Mat cannyFrame;
        cv::Canny(grayFrame, cannyFrame, 50, 20);

        std::vector<Vec4i> allLines;
        cv::HoughLinesP(cannyFrame, allLines, 1.0, CV_PI / 180.0, 20, frame.cols * 0.3);
        std::vector<LineSegment> hLines = GetHorizontalLineSegments(Vec4iLinesToLineSegments(allLines));
        SortLineSegments(hLines, UType::UTYPE_Y, false);

#ifdef _DEBUG
        cv::Mat cloneFrame = frame.clone();
        DrawLines(cloneFrame, hLines, Red, 1);
        SaveImage(cloneFrame, "line.png");
#endif

        // Find a line near the given rects
        for (auto rect : rects)
        {
            for (auto line : hLines)
            {
                int downInterval = line.p1.y - rect.y - rect.height;
                int upInterval = rect.y - line.p1.y;
                if ((downInterval >= 0 && downInterval <= 45)
                    || (upInterval >= 0 && upInterval <= 45))
                {
                    x = (line.p2.x > line.p1.x ? line.p2.x : line.p1.x);
                    width = x - rect.x;
                    break;
                }
            }

            if (width > 0)
            {
                break;
            }
        }

        return width;
    }
}
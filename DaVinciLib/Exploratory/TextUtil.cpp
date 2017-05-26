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

#include "ObjectUtil.hpp"
#include "TextDictionary.hpp"
#include "TextUtil.hpp"

using namespace cv;

namespace DaVinci
{
    boost::mutex TextUtil::textRecognizeLock;

    TextUtil::TextUtil(): validCharacterRatio(0.5), maxRectRatio(10.0)
    {
        enTextRecognize = boost::shared_ptr<TextRecognize>(new TextRecognize(AccuracyModel::DEFAULT_MODE, "", Language::ENGLISH));
        zhTextRecognize = boost::shared_ptr<TextRecognize>(new TextRecognize(AccuracyModel::DEFAULT_MODE, "", Language::SIMPLIFIED_CHINESE));

        exploratoryEngine = boost::shared_ptr<ExploratoryEngine>(new ExploratoryEngine());
        processExitHandler = boost::shared_ptr<KeywordCheckHandlerForProcessExit>(new KeywordCheckHandlerForProcessExit(exploratoryEngine->getKeywordRecog()));
        sceneTextExtractor = boost::shared_ptr<SceneTextExtractor>(new SceneTextExtractor());
    }

    TextRecognize::TextRecognizeResult TextUtil::AnalyzeEnglishOCR(const cv::Mat &bgrImage, PageIteratorLevel level)
    {
        boost::lock_guard<boost::mutex> lock(textRecognizeLock);
        TextRecognize::TextRecognizeResult result = enTextRecognize->DetectDetail(bgrImage, 0, level);
        return result;
    }

    TextRecognize::TextRecognizeResult TextUtil::AnalyzeChineseOCR(const cv::Mat &bgrImage, PageIteratorLevel level)
    {
        boost::lock_guard<boost::mutex> lock(textRecognizeLock);
        TextRecognize::TextRecognizeResult result = zhTextRecognize->DetectDetail(bgrImage, 0, level);
        return result;
    }

    std::string TextUtil::RecognizeEnglishText(const cv::Mat &bgrImage, PageIteratorLevel level)
    {
        if(bgrImage.empty())
            return "";
        else
        {
            return AnalyzeEnglishOCR(bgrImage, level).GetRecognizedText();
        }
    }

    std::string TextUtil::RecognizeChineseText(const cv::Mat &bgrImage, PageIteratorLevel level)
    {
        if(bgrImage.empty())
            return "";
        else
        {
            return AnalyzeChineseOCR(bgrImage, level).GetRecognizedText();
        }
    }

    vector<Rect> TextUtil::ExtractRSTRects(const cv::Mat &frame, string language)
    {
        if("en" == language)
        {
            return enTextRecognize->GetSceneTextRect(frame);
        }
        else
        {
            return zhTextRecognize->GetSceneTextRect(frame);
        }
    }

    vector<Rect> TextUtil::ExtractSceneTexts(const cv::Mat &frame, const cv::Rect &roi)
    {
        cv::Mat sROIFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, sROIFrame);
        DebugSaveImage(sROIFrame, "scene_text_source.png");

        vector<Rect> rstTextRects = sceneTextExtractor->ExtractByRST(sROIFrame);
#ifdef _DEBUG
        Mat debugRoiFrame = sROIFrame.clone();
        for(auto rect : rstTextRects)
        {
            DrawRect(debugRoiFrame, rect);
        }
        DebugSaveImage(debugRoiFrame, "scene_text_roi_draw_before.png");
#endif

        vector<Rect> validRects;
        const int minWidth = 15, minHeight = 15;
        for(auto rect: rstTextRects)
        {
            if(rect.x == 0 || rect.y == 0 || rect.width <= minWidth || rect.height <= minHeight)
                continue;

            validRects.push_back(rect);
        }

#ifdef _DEBUG
        debugRoiFrame = sROIFrame.clone();
        for(auto rect : validRects)
        {
            DrawRect(debugRoiFrame, rect);
        }
        DebugSaveImage(debugRoiFrame, "scene_text_roi_draw_after.png");
#endif

        vector<Rect> textRects;
        for(auto rect : validRects)
        {
            Rect rectOnFrame = ObjectUtil::Instance().RelocateRectangle(frame.size(), rect, roi);
            double ratio = rect.width / (double)(rect.height);
            if(!FSmallerE(ratio, maxRectRatio))
            {
                cv::Mat sceneTextFrame;
                ObjectUtil::Instance().CopyROIFrame(frame, rectOnFrame, sceneTextFrame);
                vector<Rect> contourTextRects = ObjectUtil::Instance().DetectRectsByContour(sceneTextFrame, 50, 10, 1, 3, 20, 1000, 95000);
                contourTextRects = ObjectUtil::Instance().RelocateRectangleList(frame.size(), contourTextRects, rectOnFrame);
                AddVectorToVector(textRects, contourTextRects);
            }
            else
            {
                textRects.push_back(rectOnFrame);
            }
        }

#ifdef _DEBUG
        Mat debugFrame = frame.clone();
        for(auto rect : textRects)
        {
            DrawRect(debugFrame, rect);
        }
        DebugSaveImage(debugFrame, "scene_text_draw.png");
#endif

        return textRects;
    }

    cv::Rect TextUtil::ExtractSceneTextShapes(const cv::Mat &frame, const cv::Rect &roi, const cv::Point &clickPoint)
    {
        vector<Rect> textRects = ExtractSceneTexts(frame, roi);
        cv::Rect sceneTextRect = EmptyRect;

        if (!textRects.empty())
        {
            SortRects(textRects, UTYPE_AREA, false);

            if(clickPoint == EmptyPoint)
            {
                sceneTextRect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(textRects[0], frame.size(), 2, 2);
                return sceneTextRect;
            }


#ifdef _DEBUG
            Mat debugFrame = frame.clone();
            for (auto rect : textRects)
            {
                DrawRect(debugFrame, rect);
            }
            DrawCircle(debugFrame, clickPoint);
            DebugSaveImage(debugFrame, "scene_text_all.png");
#endif

            for (auto rect : textRects)
            {
                if (rect.contains(clickPoint))
                {
                    sceneTextRect = rect;
                    break;
                }

                rect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(rect, frame.size(), extendedTextWidth, extendedTextWidth);

                if (rect.contains(clickPoint))
                {
                    sceneTextRect = rect;
                    break;
                }
            }
        }

        return sceneTextRect;
    }

    /*cv::Rect TextUtil::ExtractContourTextShapes(const cv::Mat &frame, const cv::Rect &roi, const cv::Point &clickPoint, int xKernelSize)
    {
    cv::Rect contourTextRect = EmptyRect;

    cv::Mat sROIFrame;
    frame(roi).copyTo(sROIFrame);

    std::vector<cv::Rect> textRects = ObjectUtil::Instance().DetectRectsByContour(sROIFrame, 240, xKernelSize, 1, 0.5, 50, 200, 35000); // 6 is the upper limit of record frame

    if (!textRects.empty())
    {
    SortRects(textRects, UTYPE_AREA, true); // Sort from small to big, because too big is not good for scenetext

    if (clickPoint == EmptyPoint)
    {
    contourTextRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), textRects[0], roi);
    contourTextRect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(contourTextRect, frame.size(), 0, 5);
    }
    else
    {
    double minDistance = DBL_MAX;
    double currentDistance = DBL_MAX;
    cv::Rect closestRect = EmptyRect;
    for (auto rect : textRects)
    {
    rect = ObjectUtil::Instance().RelocateRectangle(frame.size(), rect, roi);
    currentDistance = ComputeDistance(cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2), clickPoint);
    if (FSmallerE(currentDistance, minDistance))
    {
    minDistance = currentDistance;
    closestRect = rect;
    }
    }

    if (FSmallerE(minDistance, 100.0))
    {
    contourTextRect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(closestRect, frame.size(), 0, 5);
    }
    }
    }

    return contourTextRect;
    }
    */

    cv::Rect TextUtil::ExtractTextShapes(const cv::Mat &frame, const cv::Point &clickPoint, int textWidth, int textHeight)
    {
        cv::Rect textROI = ObjectUtil::Instance().ConstructTextRectangle(clickPoint, frame.size(), textWidth, textHeight);

#ifdef _DEBUG
        if (textROI != EmptyRect)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, textROI, Red, 2);
            SaveImage(debugFrame, "debug_record_text_roi.png");
        }
#endif

        cv::Rect textRect = this->ExtractSceneTextShapes(frame, textROI, clickPoint);

#ifdef _DEBUG
        if (textRect != EmptyRect)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, textRect, Red, 2);
            SaveImage(debugFrame, "debug_record_text_rst.png");
        }
#endif

        return textRect;
    }

    cv::Rect TextUtil::ExtractTextShapesNearButton(const cv::Mat &frame, const cv::Point &clickPoint, int textHeight)
    {
        cv::Rect textRect = EmptyRect;

        // Check whether click on a button
        cv::Rect buttonRect = EmptyRect;

        cv::Rect roi = ObjectUtil::Instance().ConstructRectangleFromCenterPoint(clickPoint, frame.size(), 300, 100);
        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        std::vector<cv::Rect> buttonRects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 100, 1, 1, 0.3, 3, 300, 3000);
        SortRects(buttonRects, UType::UTYPE_AREA, false);

        for (auto rect : buttonRects)
        {
            rect = ObjectUtil::Instance().RelocateRectangle(frame.size(), rect, roi);
            if (rect.contains(clickPoint))
            {
                buttonRect = rect;
                break;
            }
        }

        if (buttonRect == EmptyRect)
        {
            return textRect;
        }

        cv::Rect frameRect(EmptyPoint, frame.size());
        roi = cv::Rect(0, clickPoint.y - textHeight / 2, frame.cols, textHeight);
        roi &= frameRect;

        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        std::vector<cv::Rect> textRexts;
        std::vector<cv::Rect> rects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 240, 6, 1, 0.5, 50, 1000, 10000); // 6 is the upper limit of record frame
        for (auto rect : rects)
        {
            rect = ObjectUtil::Instance().RelocateRectangle(frame.size(), rect, roi);
            rect = ObjectUtil::Instance().ScaleRectangleWithLeftTopPoint(rect, frame.size(), 0, 5);
            rect = this->ExtractSceneTextShapes(frame, rect);
            if (rect != EmptyRect && !IntersectWith(rect, buttonRect))
            {
                textRexts.push_back(rect);
            }
        }

        if (textRexts.size() == 1)
        {
            textRect = textRexts[0];
        }
        else if (textRexts.size() > 1)
        {
            // Choose the highest one
            int maxHeight = INT_MIN;
            size_t maxIndex = 0;
            for (size_t i = 0; i < textRexts.size(); i++)
            {
                if (textRexts[i].height > maxHeight)
                {
                    maxHeight = textRexts[i].height;
                    maxIndex = i;
                }
            }
            textRect = textRexts[maxIndex];
        }

#ifdef _DEBUG
        if (buttonRect != EmptyRect && textRect != EmptyRect)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, buttonRect, Red, 2);
            DrawRect(debugFrame, textRect, Green, 2);
            SaveImage(debugFrame, "debug_record_text_button_pair.png");
        }
#endif

        return textRect;
    }

    cv::Rect TextUtil::ExtractButtonRectByTextButtonPair(const cv::Mat &frame, const cv::Rect &sourceTextRect, const cv::Point &sourceButtonPoint, const cv::Rect &targetTextRect)
    {
        cv::Rect targetButtonRect = EmptyRect;
        cv::Rect frameRect(EmptyPoint, frame.size());

        cv::Rect roi;
        if (sourceButtonPoint.x >= sourceTextRect.br().x) // Button is on the right of text
        {
            roi = cv::Rect(targetTextRect.br().x, targetTextRect.y - 15, frame.cols - targetTextRect.br().x, targetTextRect.height + 30);
            roi &= frameRect;
        }
        else // if (sourceButtonPoint.x <= sourceTextRect.x) // Button is on the left of text
        {
            roi = cv::Rect(0, targetTextRect.y - 10, targetTextRect.x, targetTextRect.height + 20);
            roi &= frameRect;
        }

        cv::Mat roiFrame;
        ObjectUtil::Instance().CopyROIFrame(frame, roi, roiFrame);

        std::vector<cv::Rect> buttonRects = ObjectUtil::Instance().DetectRectsByContour(roiFrame, 100, 1, 1, 0.2, 5, 300, 3000);

        if (buttonRects.size() > 0 && buttonRects.size() < 5)
        {
            SortRects(buttonRects, UType::UTYPE_AREA, false);
            targetButtonRect = buttonRects[0];
            targetButtonRect = ObjectUtil::Instance().RelocateRectangle(frame.size(), targetButtonRect, roi);
        }

#ifdef _DEBUG
        if (targetButtonRect != EmptyRect)
        {
            cv::Mat debugFrame = frame.clone();
            DrawRect(debugFrame, targetButtonRect, Red, 2);
            SaveImage(debugFrame, "debug_button_rect.png");
        }
#endif

        return targetButtonRect;
    }

    std::string TextUtil::PostProcessText(std::string text)
    {
        // TODO: enhance robust scene text for big and small text localization
        std::string validLine = boost::trim_copy(text);
        vector<string> lines;
        boost::algorithm::split(lines, text, boost::algorithm::is_any_of("\n"));
        vector<string> validLines;
        for(auto line: lines)
        {
            std::string trimmedLine = boost::trim_copy(line);
            if(!boost::empty(trimmedLine))
                validLines.push_back(trimmedLine);
        }

        if(validLines.size() > 0)
            validLine = validLines[0];

        std::wstring wstr = StrToWstr(validLine.c_str());

        std::string str = WstrToStr(wstr);
        std::string trimmedStr = boost::trim_copy(str);

        return trimmedStr;
    }

    int TextUtil::MeasureHammDistance(std::string text, std::string expectText)
    {
        int hammDistance = 0;

        int len1 = (int)text.size();
        int len2 = (int)expectText.size();
        int maxLen = max(len1, len2);

        if (len1 == 0 || len2 == 0)
        {
            return hammDistance;
        }

        std::string lcs = this->GetLCS(text, expectText);
        hammDistance = maxLen - (int)lcs.length();

        return hammDistance;
    }

    std::string TextUtil::GetLCS(std::string text, std::string expectText)
    {
        std::string substring = "";

        int len1 = (int)expectText.size();
        int len2 = (int)text.size();

        if (len1 == 0 || len2 == 0)
        {
            return substring;
        }

        // Initialize c
        int **c = new int*[len1];
        for (int i = 0; i < len1; i++)
        {
            c[i] = new int[len2];
        }

        for (int i = 0; i < len1; i++)
        {
            for (int j = 0; j < len2; j++)
            {
                c[i][j] = 0;
            }
        }

        int maxLen = 0;
        int substringEndPosition = 0;
        for (int i = 0; i < len1; i++)
        {
            for (int j = len2 - 1; j >= 0; j--)
            {
                if (expectText[i] == text[j])
                {
                    if (i == 0 || j == 0)
                    {
                        c[i][j] = 1;
                    }
                    else
                    {
                        c[i][j] = c[i - 1][j - 1] + 1;
                    }
                }

                if (c[i][j] > maxLen)
                {
                    maxLen = c[i][j];
                    substringEndPosition = j;
                }
            }
        }
        substring = text.substr(substringEndPosition - maxLen + 1, maxLen);

        // Release c
        for (int i = 0; i < len1; i++)
        {
            delete [] c[i];
        }
        delete [] c;

        return substring;
    }

    bool TextUtil::MatchText(std::string text, std::string expectText, int &hammDistance, string language, bool spaceIgnore, int allowHammDistance)
    {
        bool isMatched = false;

        std::string copyExpectText = expectText;
        boost::to_lower(copyExpectText);
        if (spaceIgnore)
        {
            boost::algorithm::erase_all(copyExpectText, " ");
        }
        int expectTextSize = (int)copyExpectText.size();
        if(expectTextSize == 0)
            return isMatched;

        std::vector<std::string> fuzzyTexts = TextDictionary::Instance().LookUpDictionary(text);
        fuzzyTexts.push_back(text);

        for (auto fuzzyText : fuzzyTexts)
        {
            std::string copyText = fuzzyText;
            boost::to_lower(copyText);
            if (spaceIgnore)
            {
                boost::algorithm::erase_all(copyText, " ");
            }
            int textSize = (int)copyText.size();
            if(textSize == 0)
                continue;

            int hammDistanceThreshold = allowHammDistance;
            if (hammDistanceThreshold == -1)
            {
                int maxLen = max(textSize, expectTextSize);
                if(language == "en")
                    hammDistanceThreshold = maxLen / 2;
                else
                {
                    hammDistanceThreshold = maxLen - 2; // FIXME: Chinese recognition may have issues, lower the threshold condition
                    if(hammDistanceThreshold == 0 && expectTextSize == 2) // Hexin: 学 - 股民学校
                        hammDistanceThreshold = 2;
                }
            }

            hammDistance = this->MeasureHammDistance(copyText, copyExpectText);
            if(hammDistance <= hammDistanceThreshold)
            {
                isMatched = true;
                break;
            }
        }

        return isMatched;
    }

    bool TextUtil::IsExpectedProcessExit(const cv::Mat &frame, const cv::Point &clickPoint, const string language, vector<Keyword> clickedKeywords)
    {
        bool ret = false;
        Mat roiFrame;
        Size frameSize = frame.size();
        int allowTopHeight = 300, allowBottomHeight = 100;
        int y = clickPoint.y - allowTopHeight;
        if(y < 0)
            y = 0;
        int height = allowTopHeight + allowBottomHeight;
        if(y + height >= frameSize.height)
            height = frameSize.height - y;

        Rect roiRect(0, y, frameSize.width, height);
        ObjectUtil::Instance().CopyROIFrame(frame, roiRect, roiFrame);
        exploratoryEngine->generateAllRectsAndKeywords(roiFrame, Rect(0, 0, roiFrame.size().width, roiFrame.size().height), language);
        processExitHandler->SetClickPoint(Point(clickPoint.x, clickPoint.y - y));

        ret = processExitHandler->HandleRequest(language, exploratoryEngine->getAllEnKeywords(), exploratoryEngine->getAllZhKeywords());

        if ((ret == false) && (clickedKeywords.size() > 0))
            ret = processExitHandler->CheckClickedKeywords(clickedKeywords);

        return ret;
    }

    bool TextUtil::IsValidTextLength(int validLength, int originLength)
    {
        if(validLength < validCharacterLength)
            return false;

        double validRatio = validLength / (double)(originLength);
        if(FSmallerE(validRatio, validCharacterRatio))
        {
            return false;
        }

        return true;
    }

    bool TextUtil::IsValidEnglishText(const std::string &text)
    {
        bool isValid = true;
        std::string cloneText = text;

        // "<" or "(" is handled specially
        if (cloneText.length() == 1 && (boost::equals(cloneText, "(") || boost::equals(cloneText, "<")))
        {
            return isValid;
        }

        for (std::string::iterator it = cloneText.begin(); it != cloneText.end();)
        {
            char ch = *it;
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == ' ')
            {
                it++;
                continue;
            }

            cloneText.erase(it);
        }

        if (!IsValidTextLength((int)cloneText.length(), (int)text.length()))
        {
            isValid = false;
        }
        else if (cloneText.length() == 2)   // Like "E1" is not a valid text
        {
            char ch1 = cloneText.at(0);
            char ch2 = cloneText.at(1);
            if ((isalpha(ch1) && isdigit(ch2)) || (isalpha(ch2) && isdigit(ch1)))
            {
                isValid = false;
            }
        }

        return isValid;
    }

    bool TextUtil::IsValidChineseText(const std::string &text)
    {
        bool isValid = true;
        std::string cloneText = text;

        for (std::string::iterator it = cloneText.begin(); it != cloneText.end();)
        {
            char firstC = *it;
            if ((firstC & 0x80))
            {
                std::string::iterator secondIt = it + 1;
                if(secondIt != cloneText.end())
                {
                    char secondC = *(it + 1);
                    if ((secondC & 0x80))
                    {
                        it += 2;
                        continue;
                    }
                }
            }

            cloneText.erase(it);
        }

        if (!IsValidTextLength((int)cloneText.length(), (int)text.length()))
        {
            isValid = false;
        }

        return isValid;
    }
}

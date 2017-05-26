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
#include "KeyboardRecognizer.hpp"
#include "ObjectUtil.hpp"
#include "TextUtil.hpp"
#include "ObjectCommon.hpp"
#include "boost/algorithm/string.hpp"
#include "AndroidBarRecognizer.hpp"
#include "KeyboardRec.hpp"
#include <regex>

namespace DaVinci
{
    KeyboardRecognizer::KeyboardRecognizer()
    {
        objectCategory = "Keyboard";

        haveFiveRow = false;
        recognizeNavigationBar = false;

        rectangleWidth = 0;
        rectangleHeight = 0;
        interval = 0;
        imageWidth = 0;
        // the Y-axis of each row
        rowYcoordinate.push_back(0);
        rowYcoordinate.push_back(0);
        rowYcoordinate.push_back(0);
        rowYcoordinate.push_back(0);
        rowYcoordinate.push_back(0);

        row0 = std::vector<Rect>();
        row1 = std::vector<Rect>();
        row2 = std::vector<Rect>();
        row3 = std::vector<Rect>();
        row4 = std::vector<Rect>();
        rowAll = std::vector<Rect>();

        sepicalKeys.push_back('#');     // switch to number
        sepicalKeys.push_back(' ');     // space button
        sepicalKeys.push_back('>');     // switch to the upper letter
        sepicalKeys.push_back('`');     // 'Done' or 'Next' button
        sepicalKeys.push_back('$');     // switch to english or chinese
        sepicalKeys.push_back('.');     // "." button
        sepicalKeys.push_back('@');     // "@" button
        sepicalKeys.push_back('_');     // "_" button
        sepicalKeys.push_back('~');     // "delete" button

        initDicLetter();
        currentKeyboardType = KeyboardType::NONE_KEYBOARD;

        ocrWhiteList.push_back("(^ASD(F|R|1)G.*?)");

        digitWhiteList.push_back("(^0$)");
        digitWhiteList.push_back("(^1$)");
        digitWhiteList.push_back("(^2$)|(^2ABC$)|(^ABC$)");
        digitWhiteList.push_back("(^3$)|(^3DEF$)|(^DEF$)");
        digitWhiteList.push_back("(^4$)|(^4GHI$)|(^GHI$)");
        digitWhiteList.push_back("(^5$)|(^5JKL$)|(^JKL$)");
        digitWhiteList.push_back("(^6$)|(^6MNO$)|(^MNO$)");
        digitWhiteList.push_back("(^7$)|(^7PQRS$)|(^PQRS$)");
        digitWhiteList.push_back("(^8$)|(^8TUV$)|(^TUV$)");
        digitWhiteList.push_back("(^9$)|(^9WXYZ$)|(^WXYZ$)");

        strMatchDone = "(^DONE$)|(^NEXT$)|(^GO$)|(^SEARCH$)";
        wstrMatchDone = L"(完成)|(搜索)|(搜素)|(腰素)|(确认)";

        strMatchNumber = "(^[7|?]123$)|(^#$)|(^123$)|(^ABC$)";
        wstrMatchNumber = L"(符)";

        strMatchChinese = "(EN$)";
        wstrMatchChinese = L"(中)|(英拼)|(^拼)|(英)";

        strMatchAt = "(^@$)";
        strMatchUnderLine = "(^_$)|(^-$)";
        strMatchDelete = "(^x$)|(^X$)";
    }

    bool KeyboardRecognizer::isSwitchButton(char c)
    {
        bool ret = false;
        if (c == '#' || c == '>' || c == '$')
            ret = true;
        return ret;
    }

    bool KeyboardRecognizer::isUnsupportedButton(char c)
    {
        bool ret = false;
        if (c == '?')
            ret = true;
        return ret;
    }

    bool KeyboardRecognizer::isEnterButton(char c)
    {
        bool ret = false;
        if (c == '`')
            ret = true;
        return ret;
    }
        
    bool KeyboardRecognizer::isDeleteButton(char c)
    {
        bool ret = false;
        if (c == '~')
            ret = true;
        return ret;
    }

    void KeyboardRecognizer::initDicLetter()
    {
        dicLetter.clear();

        // Init Dictionary for all the letter
        for (int num = 0; num < 26; ++num)
        {
            Rect rect = EmptyRect;
            dicLetter.insert(make_pair(static_cast<char>(97 + num), rect));
        }

        // Init Dictionary for all digit
        for (int num = 0; num < 10; ++num)
        {
            Rect rect = EmptyRect;
            dicLetter.insert(make_pair(static_cast<char>(48 + num), rect));
        }

        for (char c : sepicalKeys)
            dicLetter.insert(make_pair(c, EmptyRect));
    }

    void KeyboardRecognizer::reset()
    {
        initDicLetter();

        rowYcoordinate[0] = 0;
        rowYcoordinate[1] = 0;
        rowYcoordinate[2] = 0;
        rowYcoordinate[3] = 0;
        rowYcoordinate[4] = 0;

        row0.clear();
        row1.clear();
        row2.clear();
        row3.clear();
        row4.clear();
        rowAll.clear();

        allRowAxis.clear();

        haveFiveRow = false;
        currentKeyboardType = KeyboardType::NONE_KEYBOARD;
        rectangleWidth = 0;
        rectangleHeight = 0;
        interval = 0;
    }

    void KeyboardRecognizer::setRecognizeNavigationBar(bool flag)
    {
        recognizeNavigationBar = flag;
    }

    void KeyboardRecognizer::SetOSversion(string s)
    {
        osVersion = s;
    }

    vector<Rect> KeyboardRecognizer:: EnlargeRect( vector<Rect> &rowAll)
    {

        vector<Rect> rectListLarge;
        Rect rectLarge; for(auto rect:rowAll)
        {

            rectLarge.x = rect.x - (int)rect.width;
            rectLarge.y = rect.y - (int)(0.5*rect.height);
            rectLarge.width = rect.width *2;
            rectLarge.height = rect.height *2;
            rectListLarge.push_back(rectLarge);
        }
        return rectListLarge;
    }

    bool KeyboardRecognizer::BelongsTo(const ObjectAttributes &attributes, std::vector<cv::Rect> allEnProposalRects, std::vector<cv::Rect> allZhProposalRects, std::vector<Keyword> allEnKeywords, std::vector<Keyword> allZhKeywords)
    {
        return RecognizeKeyboard(attributes.image, this->osVersion);
    }

    bool KeyboardRecognizer::RecognizeKeyboard(const cv::Mat &image, string osVersion, cv::Rect keyboardROI)
    {
        Size imageSize = image.size();
        try
        {
            int imageArea = imageSize.width * imageSize.height;
            int minArea = imageArea / 10000;    // for Android L keyboard
            int maxArea = imageArea / 10;
            imageWidth = imageSize.width;

            cv::Mat keyboardFrame;

            // if ROI is empty, focus on the lower part
            if (keyboardROI == EmptyRect)
            {
                cv::Rect navigationBar = EmptyRect;

                if (recognizeNavigationBar == true)
                {
                    boost::shared_ptr<AndroidBarRecognizer> barRecog = boost::shared_ptr<AndroidBarRecognizer>(new AndroidBarRecognizer());
                    barRecog->RecognizeNavigationBar(image, 0);
                    navigationBar = barRecog->GetNavigationBarRect();
                }
                if (navigationBar != EmptyRect && navigationBar.height < imageSize.height / 4)
                {
                    keyboardROI.x = 0;
                    keyboardROI.y = imageSize.height/2;
                    keyboardROI.width = imageSize.width;
                    keyboardROI.height = imageSize.height/2 - navigationBar.height;
                }
                else
                {
                    keyboardROI.x = 0;
                    keyboardROI.y = imageSize.height/2;
                    keyboardROI.width = imageSize.width;
                    keyboardROI.height = imageSize.height/2;
                }
            }
            else
            {
                keyboardROI.x = 0;
                keyboardROI.width = imageSize.width;
            }
            ObjectUtil::Instance().CopyROIFrame(image, keyboardROI, keyboardFrame);

            // detect rectangles, use xKernelSize 2
            std::vector<Rect> rectList = ObjectUtil::Instance().DetectRectsByContour(keyboardFrame, 50, 2, 1, 0.2, 10, minArea, maxArea, CV_RETR_EXTERNAL);
            rectList = ObjectUtil::Instance().RelocateRectangleList(image.size(), rectList, keyboardROI);

            double dOSversion = 5.0;
            if (!osVersion.empty())
                dOSversion = boost::lexical_cast<double>(osVersion);

            // KK keyboard
            if (dOSversion < 5.0)
            {
                if (isAlphabetKeyboard(image, rectList))
                {
                    MapEachKey(image);
                    #ifdef _DEBUG
                    DrawDebugPicture(image);
                    #endif

                    if (verifyEachKey(image))
                        return true;
                    else
                        return false;
                }

                // detect rectangles, use xKernelSize 1
                rectList = ObjectUtil::Instance().DetectRectsByContour(keyboardFrame, 50, 1, 1, 0.2, 10, minArea, maxArea, CV_RETR_EXTERNAL);
                rectList = ObjectUtil::Instance().RelocateRectangleList(image.size(), rectList, keyboardROI);

                if (isDigitKeyboard(image, rectList))
                {
                    #ifdef _DEBUG
                    DrawDebugPicture(image);
                    #endif

                    if (verifyEachKey(image))
                        return true;
                    else
                        return false;
                }
            }
            else
            {
                if(isAlphabetKeyboardNoBondary(image, rectList))
                {
                    #ifdef _DEBUG
                    DrawDebugPicture(image);
                    #endif
                    return true;
                }
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("KeyboardRecognizer: RecognizeKeyboard - ") << e.what() << std::endl;
            return false;
        }
        return false;
    }

    bool KeyboardRecognizer::isDigitKeyboard(const cv::Mat &image, std::vector<Rect> rectList)
    {
        reset();
        // if recognize less than 9 rectangle, return false
        if (rectList.size() < minRectNumForDigit || rectList.size() > maxRectNumForDigit)
        {
            return false;
        }

        // dicY restore all the rectangle Y-axis
        std::unordered_map<int, int> dicY;
        std::unordered_map<int, int> dicWidth;
        std::unordered_map<int, int> dicHeight;

        for (auto rect : rectList)
        {
            if (dicWidth.find(rect.width) == dicWidth.end())
            {
                dicWidth.insert(make_pair(rect.width, 1));
            }
            else
            {
                ++dicWidth[rect.width];
            }

            if (dicHeight.find(rect.height) == dicHeight.end())
            {
                dicHeight.insert(make_pair(rect.height, 1));
            }
            else
            {
                ++dicHeight[rect.height];
            }
        }

        vector<pair<int, int>> dicWidthPairs(dicWidth.begin(), dicWidth.end());
        sort(dicWidthPairs.begin(), dicWidthPairs.end(), SortPairByIntValueSmaller);

        // get rectangle width
        rectangleWidth = dicWidthPairs[0].first;

        vector<pair<int, int>> dicHeightPairs(dicHeight.begin(), dicHeight.end());
        sort(dicHeightPairs.begin(), dicHeightPairs.end(), SortPairByIntValueSmaller);

        // get rectangle height
        for (auto eachHeigh : dicHeightPairs)
        {
            if (eachHeigh.first > minHeight)
            {
                rectangleHeight = eachHeigh.first;
                break;
            }
        }

        for (auto rect : rectList)
        {
            if (rect.height > 2 * rectangleHeight || rect.width > 2 * rectangleWidth || rect.height < rectangleHeight / 2 || rect.width < rectangleWidth / 2)
            {
                continue;
            }
            if (dicY.find(rect.y) == dicY.end())
            {
                dicY.insert(make_pair(rect.y, 1));
            }
            else
            {
                ++dicY[rect.y];
            }
        }

        // keyboard should have more than 4 row
        if (dicY.size() < 4)
        {
            return false;
        }

        vector<pair<int, int>> dicYPairs(dicY.begin(), dicY.end());
        sort(dicYPairs.begin(), dicYPairs.end(), SortPairByIntValueSmaller);

        // get 4 row y coordinate, the most frequency Y coordinate
        int numberY = 0;
        for (auto item : dicYPairs)
        {
            if (allRowContainSimilarYAxis(item.first) == true)
            {
                continue;
            }
            rowYcoordinate[numberY] = item.first;
            allRowAxis.push_back(item.first);
            numberY++;
            if (numberY >= 4)
            {
                break;
            }
        }
        std::sort(rowYcoordinate.begin(), rowYcoordinate.end());

        // should have 4 row, [1] ~ [4]
        if (rowYcoordinate[1] == 0)
        {
            return false;
        }

        // filter some rectangle which are not around the four row y coordinate
        for (auto rect : rectList)
        {
            if (rect.height > 2 * rectangleHeight || rect.width > 2 * rectangleWidth || rect.height < rectangleHeight / 2 || rect.width < rectangleWidth / 2)
            {
                continue;
            }
            if (allRowContainSamePoint(rect, 5) == true)
            {
                continue;
            }
            if (abs(rect.y - rowYcoordinate[1]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row1) == false))
            {
                row1.push_back(rect);
                rowAll.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[2]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row2) == false))
            {
                row2.push_back(rect);
                rowAll.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[3]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row3) == false))
            {
                row3.push_back(rect);
                rowAll.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[4]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row4) == false))
            {
                row4.push_back(rect);
                rowAll.push_back(rect);
            }
        }

        std::unordered_map<int, int> dicX = std::unordered_map<int, int>();
        for (Rect rect : rectList)
        {
            if (abs(rect.width - rectangleWidth) <= 15 && abs(rect.height - rectangleHeight) <= 15)
            {
                if (dicX.find(rect.x) == dicX.end())
                {
                    dicX.insert(make_pair(rect.x, 1));
                }
                else
                {
                    ++dicX[rect.x];
                }
            }
        }

        // keyboard should have more than 3 columns
        if (dicX.size() < 3)
        {
            return false;
        }

        vector<pair<int, int>> dicXPairs(dicX.begin(), dicX.end());
        sort(dicXPairs.begin(), dicXPairs.end(), SortPairByIntKeySmaller);

        // get 3 column X coordinate, the most frequency X coordinate
        vector<int> rowXcoordinate;
        int numberX = 0;
        for (auto item : dicXPairs)
        {
            if (allRowContainSimilarXAxis(item.first) == true)
            {
                continue;
            }
            rowXcoordinate.push_back(item.first);
            allRowAxis.push_back(item.first);
            numberX++;
            if (numberX >= 3)
            {
                break;
            }
        }
        std::sort(rowXcoordinate.begin(), rowXcoordinate.end());

        // keyboard should have more than 3 columns
        if (rowXcoordinate.size() < 3)
        {
            return false;
        }

        // using OCR to judge digit keyboard
        int number = 0;
        for (Rect r : rectList)
        {
            if (r.height > 2 * rectangleHeight || r.width > 2 * rectangleWidth || r.height < rectangleHeight / 2 || r.width < rectangleWidth / 2)
            {
                continue;
            }

            cv::Mat imageForOCR = image(r);

            cv::Mat grayFrame = BgrToGray(imageForOCR);
            MSER mser(5, 5, 14400, 0.15);
            vector<vector<Point2i>> mserContours;
            mser(grayFrame, mserContours);

#ifdef _DEBUG
            Mat debugFrame = imageForOCR.clone();
#endif
            vector<Rect> shapeRects;
            for(auto mserContour : mserContours)
            {
                Rect mserBoundRect = boundingRect(mserContour);
                shapeRects.push_back(mserBoundRect);
#ifdef _DEBUG
                rectangle(debugFrame, mserBoundRect, Red, 2);
                imwrite("keyboard_mser_rects.png", debugFrame);
#endif
            }

            for (Rect sharpRect : shapeRects)
            {
                Mat sharpRectForOCR;
                ObjectUtil::Instance().CopyROIFrame(imageForOCR, sharpRect, sharpRectForOCR);

#ifdef _DEBUG
                imwrite("keyboard_sharpRect_OCR.png", sharpRectForOCR);
#endif
                string text = TextUtil::Instance().RecognizeEnglishText(sharpRectForOCR);
                if (text == "")
                    continue;

                if (inDigitWhiteList(text))
                {
                    number ++;
                }

                boost::to_upper(text);
                boost::trim(text);
                boost::algorithm::erase_all(text, " ");

                if (dicLetter['0'] == EmptyRect && text == "0" && r.height < 3 * rectangleHeight)
                {
                    dicLetter['0'] = r;
                }
                if (dicLetter['`'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchDone, wstrMatchDone))
                {
                    dicLetter['`'] = r;
                }
                if (dicLetter['#'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchNumber, wstrMatchNumber))
                {
                    dicLetter['#'] = r;
                }
                if (dicLetter['$'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchChinese, wstrMatchChinese) && r.width < 3 * rectangleWidth)
                {
                    dicLetter['$'] = r;
                }

                // recognize Chinese
                text = TextUtil::Instance().RecognizeChineseText(sharpRectForOCR);
                if (text == "")
                    continue;
                boost::trim(text);
                boost::algorithm::erase_all(text, " ");

                if (dicLetter['`'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchDone, wstrMatchDone))
                {
                    dicLetter['`'] = r;
                }
                if (dicLetter['#'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchNumber, wstrMatchNumber))
                {
                    dicLetter['#'] = r;
                }
                if (dicLetter['$'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchChinese, wstrMatchChinese)  && r.width < 3 * rectangleWidth)
                {
                    dicLetter['$'] = r;
                }
            }
        }

        SortRects(row2, UTYPE_X, true);
        if (dicLetter['`'] == EmptyRect && row2.size() > 1)
        {
            dicLetter['`'] = row2[row2.size() - 1];
        }

        if (number < matchDigitListThreshold)
        {
            return false;
        }

        dicLetter['1'] = ConstructRectangle(rowXcoordinate[0], 1);
        dicLetter['2'] = ConstructRectangle(rowXcoordinate[1], 1);
        dicLetter['3'] = ConstructRectangle(rowXcoordinate[2], 1);
        dicLetter['4'] = ConstructRectangle(rowXcoordinate[0], 2);
        dicLetter['5'] = ConstructRectangle(rowXcoordinate[1], 2);
        dicLetter['6'] = ConstructRectangle(rowXcoordinate[2], 2);
        dicLetter['7'] = ConstructRectangle(rowXcoordinate[0], 3);
        dicLetter['8'] = ConstructRectangle(rowXcoordinate[1], 3);
        dicLetter['9'] = ConstructRectangle(rowXcoordinate[2], 3);
        if (dicLetter['0'] == EmptyRect)
            dicLetter['0'] = ConstructRectangle(rowXcoordinate[1], 4);
        dicLetter['~'] = Rect(dicLetter['3'].x + rectangleWidth, dicLetter['3'].y, image.size().width - dicLetter['3'].x - rectangleWidth, rectangleHeight);

        // resize for AndroidL digit keyboard
        interval = dicLetter['2'].x - dicLetter['1'].x - rectangleWidth;
        if (interval > 3 * rectangleWidth)
        {
            int intervalY = rowYcoordinate[2] - rowYcoordinate[1] - rectangleHeight;
            for (auto item : dicLetter)
            {
                if (item.second != EmptyRect)
                {
                    Rect r = EmptyRect;
                    r.x = item.second.x - (int) interval/2;
                    r.y = item.second.y - (int) intervalY/2;
                    r.width = rectangleWidth + (int)(interval * 0.8);
                    r.height = rectangleHeight + (int)(intervalY * 0.8);

                    dicLetter[item.first] = r;
                }
            }
        }

        dicLetter['a'] = dicLetter['b'] = dicLetter['c'] = dicLetter['2'];
        dicLetter['d'] = dicLetter['e'] = dicLetter['f'] = dicLetter['3'];
        dicLetter['g'] = dicLetter['h'] = dicLetter['i'] = dicLetter['4'];
        dicLetter['j'] = dicLetter['k'] = dicLetter['l'] = dicLetter['5'];
        dicLetter['m'] = dicLetter['n'] = dicLetter['o'] = dicLetter['6'];
        dicLetter['p'] = dicLetter['q'] = dicLetter['r'] = dicLetter['s'] = dicLetter['7'];
        dicLetter['t'] = dicLetter['u'] = dicLetter['v'] = dicLetter['8'];
        dicLetter['w'] = dicLetter['x'] = dicLetter['y'] = dicLetter['z'] = dicLetter['9'];

        currentKeyboardType = KeyboardType::CHINESE_DIGIT_FOUR_ROW;
        return true;
    }

    // merge rectangle list according to Y Axis
    std::vector<Rect> KeyboardRecognizer::mergeRectangleWithSimailarYAxis(std::vector<Rect> rectList, int interval)
    {
        std::set<int> dicY = std::set<int>();
        std::vector<Rect> afterMerge = std::vector<Rect>();
        bool flag = false;

        for (auto rect : rectList)
        {
            flag = false;
            for (auto y : dicY)
            {
                if (abs(y - rect.y) < interval)   // for Android L keyboard, the y coordinate is a little different even for one row, need to merge them
                {
                    rect.y = y;
                    flag = true;
                    break;
                }
            }
            if (flag == false)
            {
                dicY.insert(rect.y);
            }
            afterMerge.push_back(rect);
        }
        return afterMerge;
    }

    bool KeyboardRecognizer::isAlphabetKeyboard(const cv::Mat &image, std::vector<Rect> rectList)
    {
        reset();
        // if recognize less than 20 rectangle, return false
        if (rectList.size() < minRectNum || rectList.size() > maxRectNum)
        {
            return false;
        }

        // merge rectangle list according to Y Axis
        rectList = mergeRectangleWithSimailarYAxis(rectList, 20);

        // dicY restore all the rectangle Y-axis
        std::unordered_map<int, int> dicY;
        std::unordered_map<int, int> dicWidth;
        std::unordered_map<int, int> dicHeight;

        for (auto rect : rectList)
        {
            if (dicWidth.find(rect.width) == dicWidth.end())
            {
                dicWidth.insert(make_pair(rect.width, 1));
            }
            else
            {
                ++dicWidth[rect.width];
            }

            if (dicHeight.find(rect.height) == dicHeight.end())
            {
                dicHeight.insert(make_pair(rect.height, 1));
            }
            else
            {
                ++dicHeight[rect.height];
            }
        }

        vector<pair<int, int>> dicWidthPairs(dicWidth.begin(), dicWidth.end());
        sort(dicWidthPairs.begin(), dicWidthPairs.end(), SortPairByIntValueSmaller);

        // get rectangle width
        if (dicWidthPairs.empty())
            return false;
        rectangleWidth = dicWidthPairs[0].first;

        vector<pair<int, int>> dicHeightPairs(dicHeight.begin(), dicHeight.end());
        sort(dicHeightPairs.begin(), dicHeightPairs.end(), SortPairByIntValueSmaller);

        // get rectangle height
        if (dicHeightPairs.empty())
            return false;
        rectangleHeight = dicHeightPairs[0].first;

        for (auto rect : rectList)
        {
            // filter invalid rectangles, both width and height are invalid
            if ((rect.width > (int)(1.5 * rectangleWidth) || rect.width < (int)(rectangleWidth / 1.5)) && ((rect.height > (int)(1.5 * rectangleHeight)) || (rect.height < (int)(rectangleHeight / 1.5))))
                continue;

            if (dicY.find(rect.y) == dicY.end())
            {
                dicY.insert(make_pair(rect.y, 1));
            }
            else
            {
                ++dicY[rect.y];
            }
        }

        // keyboard should have more than 4 row
        if (dicY.size() < 4)
        {
            return false;
        }

        vector<pair<int, int>> dicYPairs(dicY.begin(), dicY.end());
        sort(dicYPairs.begin(), dicYPairs.end(), SortPairByIntValueSmaller);

        // judge have 5 rows (one row for digit) or not
        if (dicYPairs[3].second > 7 && dicYPairs.size() >= 5)
        {
            haveFiveRow = true;
        }
        else
        {
            haveFiveRow = false;
        }

        // each row should have more than 5 rectangle
        int number = 0;
        for (auto item : dicYPairs)
        {
            if (number >= 4)
            {
                if (haveFiveRow)
                {
                    rowYcoordinate[number] = item.first;
                }
                break;
            }
            if ((size_t)item.second < minRectNumOfEachRow)
            {
                return false;
            }
            rowYcoordinate[number] = item.first;
            ++ number;
        }

        std::sort(rowYcoordinate.begin(), rowYcoordinate.end());

        // four row keyboard: the first row contains digit and letter
        if (haveFiveRow == false)
        {
            rowYcoordinate[0] = rowYcoordinate[1];
        }

        // filter some rectangle which are not around the four row y coordinate
        for (auto rect : rectList)
        {
            if (allRowContainSamePoint(rect, 5) == true)
            {
                continue;
            }
            if ((abs(rect.y - rowYcoordinate[0]) < maxYfilter) && (rowContainsSimilarXAxis(rect.x, row0) == false))
            {
                row0.push_back(rect);
            }
            if (abs(rect.y - rowYcoordinate[1]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row1) == false))
            {
                row1.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[2]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row2) == false))
            {
                row2.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[3]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row3) == false))
            {
                row3.push_back(rect);
            }
            else if (abs(rect.y - rowYcoordinate[4]) < maxYfilter && (rowContainsSimilarXAxis(rect.x, row4) == false))
            {
                row4.push_back(rect);
            }
        }

        // row0, row1, row2 should contain more than 5 rectangle
        if (row0.size() < minRectNumOfEachRow || row1.size() < minRectNumOfEachRow || row2.size() < minRectNumOfEachRow || row3.size() < minRectNumOfEachRow || row4.size() == 0)
        {
            return false;
        }

        if (haveFiveRow == true && row0.size() < 9)
        {
            haveFiveRow = false;
            row0.clear();
            row0 = row1;
        }

        // using OCR to double confirm
        int upperCaseNumber = 0;
        int lowerCaseNumber = 0;
        int digitalNumber = 0;
        for (Rect r : row2)
        {
            rowAll.push_back(r);
        }
        for (Rect r : row0)
        {
            rowAll.push_back(r);
        }
        for (Rect r : row1)
        {
            rowAll.push_back(r);
        }
        for (Rect r : row3)
        {
            rowAll.push_back(r);
        }
        for (Rect r : row4)
        {
            rowAll.push_back(r);
        }
        if (MatchLetterCount(image, rowAll, upperCaseNumber, lowerCaseNumber, digitalNumber) < matchLetterThreshold)
        {
            return false;
        }

        // sort rectangle by X
        SortRects(row0, UTYPE_X, true);
        SortRects(row1, UTYPE_X, true);
        SortRects(row2, UTYPE_X, true);
        SortRects(row3, UTYPE_X, true);
        SortRects(row4, UTYPE_X, true);

        // Find out interval between two rectangle using Row[0]
        interval = ((row0[(int)row0.size()-1].x - row0[0].x) / ((int)row0.size() - 1)) - rectangleWidth;

        if (haveFiveRow == true && interval > rectangleWidth)
        {
            haveFiveRow = false;
            row0.clear();
            row0 = row1;
        }

        if ((lowerCaseNumber >= upperCaseNumber) && (lowerCaseNumber >= digitalNumber))
        {
            if (haveFiveRow == true)
                currentKeyboardType = KeyboardType::ENGLISH_FIVE_ROW_LOWER;
            else
                currentKeyboardType = KeyboardType::ENGLISH_FOUR_ROW_LOWER;
        }
        else if ((upperCaseNumber >= lowerCaseNumber) && (upperCaseNumber >= digitalNumber))
        {
            if (haveFiveRow == true)
                currentKeyboardType = KeyboardType::ENGLISH_FIVE_ROW_UPPER;
            else
                currentKeyboardType = KeyboardType::ENGLISH_FOUR_ROW_UPPER;
        }
        else
        {
            currentKeyboardType = KeyboardType::ENGLISH_DIGIT_FOUT_ROW;
        }

        return true;
    }

    bool KeyboardRecognizer::isAlphabetKeyboardNoBondary(const cv::Mat &image, std::vector<Rect> rectList)
    {
        reset();
        if (rectList.size() < minRectNum)
        {
            return false;
        }

        int YErr = (int)(0.017 *image.rows);
        vector<Rect> rectListAlph = mergeRectangleWithSimailarYAxis(rectList, YErr);

        bool isSymbol = false;

        KeyboardRec keyboard;
        Mat imageNoBoundary = image.clone();
        vector<vector<Rect>> AllRow;
        AllRow = keyboard.getAllRow(imageNoBoundary, rectListAlph,isSymbol);

        row0 = AllRow[0];
        row1 = AllRow[1];
        row2 = AllRow[2];
        row3 = AllRow[3];

        rowAll.insert(rowAll.begin(),row3.begin(),row3.end());
        rowAll.insert(rowAll.begin(),row0.begin(),row0.end());
        rowAll.insert(rowAll.begin(),row1.begin(),row1.end());
        rowAll.insert(rowAll.begin(),row2.begin(),row2.end());

        int upperCaseNumber = 0;
        int lowerCaseNumber = 0;
        int digitalNumber = 0;
        int upperCaseNum[4];
        int lowerCaseNum[4];
        int digitalNum[4];
        int cntInvalid = 0;

        for(int i = 0; i < 4; i++)
        {
            AllRow[i] = EnlargeRect(AllRow[i]);
            if (MatchLetterCount(image, AllRow[i], upperCaseNum[i], lowerCaseNum[i], digitalNum[i]) < 4)
            {
                cntInvalid++;
            }
            upperCaseNumber = upperCaseNumber + upperCaseNum[i];
            lowerCaseNumber = lowerCaseNumber + lowerCaseNum[i];
            digitalNumber = digitalNumber +digitalNum[i];
        }
        if (cntInvalid == 4)
        {
            return false;
        }

        vector<Rect> rectListLarge = EnlargeRect(rowAll);

        if ((lowerCaseNumber >= upperCaseNumber) && (lowerCaseNumber >= digitalNumber))
        {
            currentKeyboardType = KeyboardType::ENGLISH_FOUR_ROW_LOWER;
        }
        else if ((upperCaseNumber >= lowerCaseNumber) && (upperCaseNumber >= digitalNumber))
        {
            currentKeyboardType = KeyboardType::ENGLISH_FOUR_ROW_UPPER;
        }
        else
        {
            currentKeyboardType = KeyboardType::ENGLISH_DIGIT_FOUT_ROW;
            isSymbol = true;
        }

        if(isSymbol)
        {
            YErr = (int)(0.04 *image.rows);
            vector<Rect> rectListForSymbol= mergeRectangleWithSimailarYAxis(rectList, YErr);;
            AllRow = keyboard.getAllRow(imageNoBoundary, rectListForSymbol,isSymbol);
            row0 = AllRow[0];
            row1 = AllRow[1];
            row2 = AllRow[2];
            row3 = AllRow[3];
            dicLetter = keyboard.RecForTwentieSix(imageNoBoundary,row1, row2, row3,rectListForSymbol);
        }
        else
        {
            dicLetter = keyboard.RecForTwentieSix(imageNoBoundary,row1, row2, row3,rectListAlph);
        }
        return true;
    }

    bool KeyboardRecognizer::allRowContainSimilarYAxis(int y)
    {
        if (allRowAxis.empty())
        {
            return false;
        }
        for (auto item : allRowAxis)
        {
            if (abs(y - item) <= rectangleHeight)
            {
                return true;
            }
        }
        return false;
    }

    bool KeyboardRecognizer::allRowContainSimilarXAxis(int x)
    {
        if (allRowAxis.empty())
        {
            return false;
        }
        for (auto item : allRowAxis)
        {
            if (abs(x - item) <= rectangleWidth)
            {
                return true;
            }
        }
        return false;
    }

    bool KeyboardRecognizer::inDigitWhiteList(string text)
    {
        for (auto str : digitWhiteList)
        {
            std::tr1::cmatch res;
            std::tr1::regex mRegex(str);
            std::tr1::regex_search(text.c_str(), res, mRegex);

            if (res.size() != 0)
            {
                return true;
            }
        }
        return false;
    }

    int KeyboardRecognizer::MatchLetterCount(const cv::Mat image, std::vector<Rect> allRow, int &upperCaseNum, int &lowerCaseNum, int &digitalNum)
    {
        int matchCount = 0;
        upperCaseNum = 0;
        lowerCaseNum = 0;
        digitalNum = 0;
        for (Rect rectForOCR : allRow)
        {
            string text = "";
            if (0 <= rectForOCR.x && 0 <= rectForOCR.width && rectForOCR.x + rectForOCR.width <= image.cols && 0 <= rectForOCR.y && 0 <= rectForOCR.height && rectForOCR.y + rectForOCR.height <= image.rows)
            {
                Mat imageForOCR = image(rectForOCR);
                text = TextUtil::Instance().RecognizeEnglishText(imageForOCR);
                boost::trim(text);
                boost::algorithm::erase_all(text, " ");

                if (isLetterUpperCase(text) == true)
                {
                    ++matchCount;
                    ++upperCaseNum;
                    if (text == "S" || text == "K" || text == "W" || text == "Z" || text == "U" || text == "P" || text == "J" || text == "X" || text == "Z" || text == "C" || text == "V")       // 's' will be recognized as 'S'
                        ++lowerCaseNum;
                }
                else if(isLetterLowerCase(text) == true)
                {
                    ++matchCount;
                    ++lowerCaseNum;
                }
                else if(isDigit(text) == true)
                {
                    ++matchCount;
                    ++digitalNum;
                }
            }
            // no need to OCR for all the rectangles
            if (matchCount > matchLetterThreshold)
            {
                return matchCount;
            }
        }
        return matchCount;
    }

    bool KeyboardRecognizer::isLetterUpperCase(string text)
    {
        if (text == "")
            return false;

        string str = "(^[A-Z]$)";
        std::tr1::cmatch res;
        std::tr1::regex mRegex(str);
        std::tr1::regex_search(text.c_str(), res, mRegex);

        if (res.size() != 0)
        {
            return true;
        }
        return false;
    }

    bool KeyboardRecognizer::isLetterLowerCase(string text)
    {
        if (text == "")
            return false;

        string str = "(^[a-z]$)";
        std::tr1::cmatch res;
        std::tr1::regex mRegex(str);
        std::tr1::regex_search(text.c_str(), res, mRegex);

        if (res.size() != 0)
        {
            return true;
        }
        return false;
    }

    bool KeyboardRecognizer::isDigit(string text)
    {
        if (text == "")
            return false;

        string str = "(^[0-9]$)";
        std::tr1::cmatch res;
        std::tr1::regex mRegex(str);
        std::tr1::regex_search(text.c_str(), res, mRegex);

        if (res.size() != 0)
        {
            return true;
        }
        return false;
    }

    bool KeyboardRecognizer::rowContainsSimilarXAxis(int x, std::vector<Rect> &lstRect)
    {
        if (lstRect.empty())
        {
            return false;
        }
        for (auto item : lstRect)
        {
            if (abs(x - item.x) <= rectangleWidth)
            {
                return true;
            }
        }
        return false;
    }

    bool KeyboardRecognizer::allRowContainSamePoint(Rect rectangle, int rowInterval)
    {
        if (rowAll.empty())
        {
            return false;
        }

        for (auto rect : rowAll)
        {
            if (abs(rectangle.x - rect.x) <= rowInterval && abs(rectangle.y - rect.y) <= rowInterval)
            {
                return true;
            }
        }
        return false;
    }

    void KeyboardRecognizer::MapFirstRow(const cv::Mat &image)
    {
        // Deal with first row
        bool flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8, flag9, flag0;
        flag1 = flag2 = flag3 = flag4 = flag5 = flag6 = flag7 = flag8 = flag9 = flag0 = false;
        for (auto rect : row0)
        {
            if (rect.x / (rectangleWidth + interval) == 0)
            {
                dicLetter['1'] = ResizeRectangle(rect, 0);
                flag1 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 1)
            {
                dicLetter['2'] = ResizeRectangle(rect, 0);
                flag2 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 2)
            {
                dicLetter['3'] = ResizeRectangle(rect, 0);
                flag3 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 3)
            {
                dicLetter['4'] = ResizeRectangle(rect, 0);
                flag4 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 4)
            {
                dicLetter['5'] = ResizeRectangle(rect, 0);
                flag5 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 5)
            {
                dicLetter['6'] = ResizeRectangle(rect, 0);
                flag6 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 6)
            {
                dicLetter['7'] = ResizeRectangle(rect, 0);
                flag7 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 7)
            {
                dicLetter['8'] = ResizeRectangle(rect, 0);
                flag8 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 8)
            {
                dicLetter['9'] = ResizeRectangle(rect, 0);
                flag9 = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 9)
            {
                dicLetter['0'] = ResizeRectangle(rect, 0);
                flag0 = true;
            }
        }

        if (flag1 == false)
        {
            int x = interval;
            dicLetter['1'] = ConstructRectangle(x, 0);
        }
        if (flag2 == false)
        {
            int x = dicLetter['1'].x + rectangleWidth + interval;
            dicLetter['2'] = ConstructRectangle(x, 0);
        }
        if (flag3 == false)
        {
            int x = dicLetter['2'].x + rectangleWidth + interval;
            dicLetter['3'] = ConstructRectangle(x, 0);
        }
        if (flag4 == false)
        {
            int x = dicLetter['3'].x + rectangleWidth + interval;
            dicLetter['4'] = ConstructRectangle(x, 0);
        }
        if (flag5 == false)
        {
            int x = dicLetter['4'].x + rectangleWidth + interval;
            dicLetter['5'] = ConstructRectangle(x, 0);
        }
        if (flag6 == false)
        {
            int x = dicLetter['5'].x + rectangleWidth + interval;
            dicLetter['6'] = ConstructRectangle(x, 0);
        }
        if (flag7 == false)
        {
            int x = dicLetter['6'].x + rectangleWidth + interval;
            dicLetter['7'] = ConstructRectangle(x, 0);
        }
        if (flag8 == false)
        {
            int x = dicLetter['7'].x + rectangleWidth + interval;
            dicLetter['8'] = ConstructRectangle(x, 0);
        }
        if (flag9 == false)
        {
            int x = dicLetter['8'].x + rectangleWidth + interval;
            dicLetter['9'] = ConstructRectangle(x, 0);
        }
        if (flag0 == false)
        {
            int x = dicLetter['9'].x + rectangleWidth + interval;
            dicLetter['0'] = ConstructRectangle(x, 0);
        }
    }
    
    void KeyboardRecognizer::MapSecondRow(const cv::Mat &image)
    {
        // Deal with second row
        bool flagQ, flagW, flagE, flagR, flagT, flagY, flagU, flagI, flagO, flagP;
        flagQ = flagW = flagE = flagR = flagT = flagY = flagU = flagI = flagO = flagP = false;
        for (auto rect : row1)
        {
            if ((rect.x / (rectangleWidth + interval) == 0) && (flagQ == false))        // if flagQ is true, needn't to reset 'Q' rectangle again
            {
                dicLetter['q'] = ResizeRectangle(rect, 1);
                flagQ = true;
            }
            else if ((rect.x / (rectangleWidth + interval) == 1) && (flagW == false))   // if flagW is true, needn't to reset 'W' rectangle again
            {
                dicLetter['w'] = ResizeRectangle(rect, 1);
                flagW = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 2)
            {
                dicLetter['e'] = ResizeRectangle(rect, 1);
                flagE = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 3)
            {
                dicLetter['r'] = ResizeRectangle(rect, 1);
                flagR = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 4)
            {
                dicLetter['t'] = ResizeRectangle(rect, 1);
                flagT = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 5)
            {
                dicLetter['y'] = ResizeRectangle(rect, 1);
                flagY = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 6)
            {
                dicLetter['u'] = ResizeRectangle(rect, 1);
                flagU = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 7)
            {
                dicLetter['i'] = ResizeRectangle(rect, 1);
                flagI = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 8)
            {
                dicLetter['o'] = ResizeRectangle(rect, 1);
                flagO = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 9)
            {
                dicLetter['p'] = ResizeRectangle(rect, 1);
                flagP = true;
            }
        }

        if (flagQ == false)
        {
            int x = interval;
            dicLetter['q'] = ConstructRectangle(x, 1);
        }
        if (flagW == false)
        {
            int x = dicLetter['q'].x + rectangleWidth + interval;
            dicLetter['w'] = ConstructRectangle(x, 1);
        }
        if (flagE == false)
        {
            int x = dicLetter['w'].x + rectangleWidth + interval;
            dicLetter['e'] = ConstructRectangle(x, 1);
        }
        if (flagR == false)
        {
            int x = dicLetter['e'].x + rectangleWidth + interval;
            dicLetter['r'] = ConstructRectangle(x, 1);
        }
        if (flagT == false)
        {
            int x = dicLetter['r'].x + rectangleWidth + interval;
            dicLetter['t'] = ConstructRectangle(x, 1);
        }
        if (flagY == false)
        {
            int x = dicLetter['t'].x + rectangleWidth + interval;
            dicLetter['y'] = ConstructRectangle(x, 1);
        }
        if (flagU == false)
        {
            int x = dicLetter['y'].x + rectangleWidth + interval;
            dicLetter['u'] = ConstructRectangle(x, 1);
        }
        if (flagI == false)
        {
            int x = dicLetter['u'].x + rectangleWidth + interval;
            dicLetter['i'] = ConstructRectangle(x, 1);
        }
        if (flagO == false)
        {
            int x = dicLetter['i'].x + rectangleWidth + interval;
            dicLetter['o'] = ConstructRectangle(x, 1);
        }
        if (flagP == false)
        {
            int x = dicLetter['o'].x + rectangleWidth + interval;
            dicLetter['p'] = ConstructRectangle(x, 1);
        }
    }

    void KeyboardRecognizer::MapThirdRow(const cv::Mat &image)
    {
        // deal with the third row
        bool flagA, flagS, flagD, flagF, flagG, flagH, flagJ, flagK, flagL;
        flagA = flagS = flagD = flagF = flagG = flagH = flagJ = flagK = flagL = false;

        for (auto rect : row2)
        {
            if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 0)
            {
                dicLetter['a'] = ResizeRectangle(rect, 2);
                flagA = true;
            }
            else if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 1)
            {
                dicLetter['s'] = ResizeRectangle(rect, 2);
                flagS = true;
            }
            else if ((rect.x - (int)(interval / 2))  / (rectangleWidth + interval) == 2)
            {
                dicLetter['d'] = ResizeRectangle(rect, 2);
                flagD = true;
            }
            else if ((rect.x - (int)(interval / 2))  / (rectangleWidth + interval) == 3)
            {
                dicLetter['f'] = ResizeRectangle(rect, 2);
                flagF = true;
            }
            else if ((rect.x - (int)(interval / 2))  / (rectangleWidth + interval) == 4)
            {
                dicLetter['g'] = ResizeRectangle(rect, 2);
                flagG = true;
            }
            else if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 5)
            {
                dicLetter['h'] = ResizeRectangle(rect, 2);
                flagH = true;
            }
            else if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 6)
            {
                dicLetter['j'] = ResizeRectangle(rect, 2);
                flagJ = true;
            }
            else if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 7)
            {
                dicLetter['k'] = ResizeRectangle(rect, 2);
                flagK = true;
            }
            else if ((rect.x - (int)(interval / 2)) / (rectangleWidth + interval) == 8)
            {
                dicLetter['l'] = ResizeRectangle(rect, 2);
                flagL = true;
            }
        }

        if (flagA == false)
        {
            int x = rectangleWidth / 2;
            dicLetter['a'] = ConstructRectangle(x, 2);
        }
        if (flagS == false)
        {
            int x = dicLetter['a'].x + rectangleWidth + interval;
            dicLetter['s'] = ConstructRectangle(x, 2);
        }
        if (flagD == false)
        {
            int x = dicLetter['s'].x + rectangleWidth + interval;
            dicLetter['d'] = ConstructRectangle(x, 2);
        }
        if (flagF == false)
        {
            int x = dicLetter['d'].x + rectangleWidth + interval;
            dicLetter['f'] = ConstructRectangle(x, 2);
        }
        if (flagG == false)
        {
            int x = dicLetter['f'].x + rectangleWidth + interval;
            dicLetter['g'] = ConstructRectangle(x, 2);
        }
        if (flagH == false)
        {
            int x = dicLetter['g'].x + rectangleWidth + interval;
            dicLetter['h'] = ConstructRectangle(x, 2);
        }
        if (flagJ == false)
        {
            int x = dicLetter['h'].x + rectangleWidth + interval;
            dicLetter['j'] = ConstructRectangle(x, 2);
        }
        if (flagK == false)
        {
            int x = dicLetter['j'].x + rectangleWidth + interval;
            dicLetter['k'] = ConstructRectangle(x, 2);
        }
        if (flagL == false)
        {
            int x = dicLetter['k'].x + rectangleWidth + interval;
            dicLetter['l'] = ConstructRectangle(x, 2);
        }
    }

    void KeyboardRecognizer::MapFourthRow(const cv::Mat &image)
    {
        // deal with the fourth row
        bool flagZ, flagX, flagC, flagV, flagB, flagN, flagM, flagUpper;
        flagZ = flagX = flagC = flagV = flagB = flagN = flagM = flagUpper = false;

        for (auto rect : row3)
        {
            if ((rect.x / (rectangleWidth + interval) == 0) && (rect.width < 5 * rectangleWidth))
            {
                dicLetter['>'] = rect;
                flagUpper = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 1)
            {
                dicLetter['z'] = ResizeRectangle(rect, 3);
                flagZ = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 2)
            {
                dicLetter['x'] = ResizeRectangle(rect, 3);
                flagX = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 3)
            {
                dicLetter['c'] = ResizeRectangle(rect, 3);
                flagC = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 4)
            {
                dicLetter['v'] = ResizeRectangle(rect, 3);
                flagV = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 5)
            {
                dicLetter['b'] = ResizeRectangle(rect, 3);
                flagB = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 6)
            {
                dicLetter['n'] = ResizeRectangle(rect, 3);
                flagN = true;
            }
            else if (rect.x / (rectangleWidth + interval) == 7)
            {
                dicLetter['m'] = ResizeRectangle(rect, 3);
                flagM = true;
            }
        }
        if (flagUpper == false)
        {
            int x = interval;
            dicLetter['>'] = ConstructRectangle(x, 3);
        }
        if (flagZ == false)
        {
            int x = interval + rectangleWidth;
            dicLetter['z'] = ConstructRectangle(x, 3);
        }
        if (flagX == false)
        {
            int x = dicLetter['z'].x + rectangleWidth + interval;
            dicLetter['x'] = ConstructRectangle(x, 3);
        }
        if (flagC == false)
        {
            int x = dicLetter['x'].x + rectangleWidth + interval;
            dicLetter['c'] = ConstructRectangle(x, 3);
        }
        if (flagV == false)
        {
            int x = dicLetter['c'].x + rectangleWidth + interval;
            dicLetter['v'] = ConstructRectangle(x, 3);
        }
        if (flagB == false)
        {
            int x = dicLetter['v'].x + rectangleWidth + interval;
            dicLetter['b'] = ConstructRectangle(x, 3);
        }
        if (flagN == false)
        {
            int x = dicLetter['b'].x + rectangleWidth + interval;
            dicLetter['n'] = ConstructRectangle(x, 3);
        }
        if (flagM == false)
        {
            int x = dicLetter['n'].x + rectangleWidth + interval;
            dicLetter['m'] = ConstructRectangle(x, 3);
        }
    }


    bool KeyboardRecognizer::MapEachKey(const cv::Mat &image)
    {
        try
        {
            MapFirstRow(image);
            MapSecondRow(image);
            MapThirdRow(image);
            MapFourthRow(image);

            // Find space button
            bool flagSpace = false;
            for (auto rect : row4)
            {
                if ((rect.x < imageWidth / 2) && ((rect.x + rect.width) > imageWidth / 2))
                {
                    dicLetter[' '] = rect;
                    flagSpace = true;
                    break;
                }
            }
            if (flagSpace == false)
            {
                int x = dicLetter['c'].x;
                dicLetter[' '] = ConstructSpaceRectangle(x, 4);
            }

            // Find dot button
            bool flagDot = false;
            if (row4.size() > 2)
            {
                dicLetter['.'] = row4[row4.size() - 2];
                flagDot = true;
            }

            // Find other special key
            vector<Rect> candidateRects;

            if (row3.size() > 1)
            {
                candidateRects.push_back(row3[row3.size()-1]);
                candidateRects.push_back(row3[row3.size()-2]);
            }
            if (row2.size() > 1)
            {
                candidateRects.push_back(row2[row2.size()-1]);
                candidateRects.push_back(row2[0]);              // maybe "@" button
            }
            if (row1.size() > 1)
            {
                candidateRects.push_back(row1[row1.size()-1]);
            }
            for (Rect r : row4)
            {
                candidateRects.push_back(r);
            }

            MapSepcialKey(candidateRects, image); // find 'switch to number' button, 'Done' button, 'swith to Chinese' button

            bool flagNumber = false;
            bool flagDone = false;
            bool flagDelete = false;

            if (dicLetter['#'] != EmptyRect)
            {
                flagNumber = true;
            }
            if (dicLetter['`'] != EmptyRect)
            {
                flagDone = true;
            }
            if (dicLetter['~'] != EmptyRect)
            {
                flagDelete = true;
            }

            if (flagNumber == false) // the first rect of row4
            {
                for (auto rect : row4)
                {
                    if (rect.x / (rectangleWidth + interval) == 0)
                    {
                        dicLetter['#'] = rect;
                        flagNumber = true;
                    }

                }
            }
            if (flagNumber == false)
            {
                int x = interval;
                dicLetter['#'] = ConstructRectangle(x, 4);
                flagNumber = true;
            }

            if (flagDone == false) // the biggest button of last column
            {
                if (row4.size() > 0 )
                {
                    dicLetter['`'] = row4[row4.size()-1];

                    if (row3[row3.size()-1].width > dicLetter['`'].width)
                    {
                        dicLetter['`'] = row3[row3.size()-1];
                    }
                    if (row2[row2.size()-1].width > dicLetter['`'].width)
                    {
                        dicLetter['`'] = row2[row2.size()-1];
                    }
                    if (row1[row1.size()-1].width > dicLetter['`'].width)
                    {
                        dicLetter['`'] = row1[row1.size()-1];
                    }
                    flagDone = true;
                }
            }
            if (flagDone == false) 
            {
                Rect rect = EmptyRect;
                rect.x = dicLetter['l'].x + rectangleWidth;
                rect.y = rowYcoordinate[2];
                rect.width = (int)(rectangleWidth * 1.3);
                rect.height = rectangleHeight;
                dicLetter['`'] = rect;
                flagDone = true;
            }

            if (flagDelete == false)
            {
                // if 'done' button belongs to the fouth row, 'delete' button is in the third row, else in the first row
                if ((abs(dicLetter['`'].y - rowYcoordinate[4]) < maxYfilter) && (row3.size() > 1))
                {
                    if (abs(row3[row3.size()-1].x - dicLetter['l'].x) < maxYfilter)
                        dicLetter['~'] = row3[row3.size()-1];
                    else
                        dicLetter['~'] = ConstructRectangle(dicLetter['l'].x + (int)(rectangleWidth * 0.4), 3);
                    flagDelete = true;
                }
                else
                {
                    dicLetter['~'] = ConstructRectangle(dicLetter['`'].x + dicLetter['`'].width - rectangleWidth, 1);
                    flagDelete = true;
                }
            }

            if ((dicLetter['_'] == EmptyRect) && (row4.size() == 7))
            {
                dicLetter['_'] = row4[2];
            }
        }
        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << std::string("KeyboardRecognizer: MapEachKey - ") << e.what() << std::endl;
            return false;
        }
        return true;
    }


    void KeyboardRecognizer::MapSepcialKey(vector<Rect> rectList, const cv::Mat &image)
    {
        for (Rect r : rectList)
        {
            if (r.x >= 0 && r.width >= 0 && r.x + r.width <= image.size().width && r.y >= 0 && r.height >=0 && r.y + r.height <= image.size().height)
            {
                Mat imageForOCR;
                ObjectUtil::Instance().CopyROIFrame(image, r, imageForOCR);

                cv::Mat grayFrame = BgrToGray(imageForOCR);
                MSER mser(5, 5, 10000, 0.15);
                vector<vector<Point2i>> mserContours;
                mser(grayFrame, mserContours);
#ifdef _DEBUG
                Mat debugFrame = imageForOCR.clone();
#endif
                vector<Rect> shapeRects;
                for(auto mserContour : mserContours)
                {
                    Rect mserBoundRect = boundingRect(mserContour);
                    shapeRects.push_back(mserBoundRect);
#ifdef _DEBUG
                    rectangle(debugFrame, mserBoundRect, Red, 2);
                    imwrite("keyboard_mser_rects.png", debugFrame);
#endif
                }

                for (Rect sharpRect : shapeRects)
                {
                    Mat sharpRectForOCR;
                    ObjectUtil::Instance().CopyROIFrame(imageForOCR, sharpRect, sharpRectForOCR);

#ifdef _DEBUG
                    imwrite("keyboard_sharpRect_OCR.png", sharpRectForOCR);
#endif
                    // recognize English
                    string text =  TextUtil::Instance().RecognizeEnglishText(sharpRectForOCR);
                    if (text == "")
                        continue;
                    boost::trim(text);
                    boost::to_upper(text);
                    boost::algorithm::erase_all(text, " ");

                    if (dicLetter['`'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchDone, wstrMatchDone))
                    {
                        dicLetter['`'] = r;
                    }
                    if (dicLetter['#'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchNumber, wstrMatchNumber))
                    {
                        dicLetter['#'] = r;
                    }
                    if (dicLetter['$'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchChinese, wstrMatchChinese))
                    {
                        dicLetter['$'] = r;
                    }
                    if (dicLetter['@'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchAt, std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes("")))
                    {
                        dicLetter['@'] = r;
                    }
                    if (dicLetter['_'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchUnderLine, std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes("")))
                    {
                        dicLetter['_'] = r;
                    }
                    if (dicLetter['~'] == EmptyRect && matchKeyword(text, Language::ENGLISH, strMatchDelete, std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes("")))
                    {
                        dicLetter['~'] = r;
                    }

                    // recognize Chinese
                    text = TextUtil::Instance().RecognizeChineseText(sharpRectForOCR);
                    boost::trim(text);
                    boost::algorithm::erase_all(text, " ");

                    if (dicLetter['`'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchDone, wstrMatchDone))
                    {
                        dicLetter['`'] = r;
                    }
                    if (dicLetter['#'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchNumber, wstrMatchNumber))
                    {
                        dicLetter['#'] = r;
                    }
                    if (dicLetter['$'] == EmptyRect && matchKeyword(text, Language::SIMPLIFIED_CHINESE, strMatchChinese, wstrMatchChinese))
                    {
                        dicLetter['$'] = r;
                    }
                }
            }
        }
    }

    bool KeyboardRecognizer::matchKeyword(string text, Language language, string strMatch, wstring wstrMatch)
    {
        boost::algorithm::erase_all(text, "\n");
        bool ret = false;
        if (language == Language::ENGLISH)
        {
            std::tr1::cmatch res;
            std::tr1::regex mRegex(strMatch);
            std::tr1::regex_search(text.c_str(), res, mRegex);

            if (res.size() != 0)
            {
                ret = true;
            }
        }
        else
        {
            wstring wtext = StrToWstr(text.c_str());
            std::tr1::wsmatch wswatch;
            std::tr1::wregex wRegex(wstrMatch.c_str());
            std::tr1::regex_search(wtext, wswatch, wRegex);

            if (wswatch.size() != 0)
            {
                ret = true;
            }
        }
        return ret;
    }

    Rect KeyboardRecognizer::ConstructSpaceRectangle(int x, int rowNumber)
    {
        int rowY = rowYcoordinate[rowNumber];

        Rect rect = EmptyRect;
        rect.x = x;
        rect.y = rowY;

        rect.width = rectangleWidth * 5 + interval * 4;
        rect.height = rectangleHeight;

        return rect;
    }

    Rect KeyboardRecognizer::ConstructRectangle(int x, int rowNumber)
    {
        int rowY = rowYcoordinate[rowNumber];
        Rect rect = EmptyRect;

        if (interval < rectangleWidth)
        {
            rect.x = x;
            rect.y = rowY;

            rect.width = rectangleWidth;
            rect.height = rectangleHeight;
        }
        else
        {
            int intervalY = rowYcoordinate[2] - rowYcoordinate[1] - rectangleHeight;
            rect.x = x;
            rect.y = rowY - (int)intervalY/2;

            rect.width = rectangleWidth + (int)(interval * 0.8);
            rect.height = rectangleHeight + (int)(intervalY * 0.8);
        }

        return rect;
    }

    Rect KeyboardRecognizer::ResizeRectangle(Rect rect, int rowNumber)
    {
        if (interval < rectangleWidth)
        {
            int rowY = rowYcoordinate[rowNumber];
            rect.y = rowY;

            rect.width = rectangleWidth;
            rect.height = rectangleHeight;
        }
        else
        {
            // enlarge Android L keyboard
            int intervalY = rowYcoordinate[2] - rowYcoordinate[1] - rectangleHeight;

            rect.y = rowYcoordinate[rowNumber] - (int)intervalY/2;
            rect.x = rect.x - (int)interval/2;
            rect.width = rectangleWidth + (int)(interval * 0.8);
            rect.height = rect.height + (int)(intervalY * 0.8);
        }
        return rect;
    }

    bool KeyboardRecognizer::verifyEachKey(const cv::Mat &image)
    {
        for(auto item : dicLetter)
        {
            Rect r = item.second;
            if ( (r != EmptyRect) && ((r.x + r.width > image.size().width) || (r.y + r.height > image.size().height) || r.x < 0 || r.y < 0) )
            {
                return false;
            }
        }
        // double check the width of dicLetter['p']
        if (dicLetter['p'] != EmptyRect && dicLetter['p'].width < image.size().width / 20)
        {
            return false;
        }
        // double check the location of dicLetter['p']
        if (currentKeyboardType == KeyboardType::ENGLISH_FIVE_ROW_LOWER || currentKeyboardType == KeyboardType::ENGLISH_FIVE_ROW_UPPER || currentKeyboardType == KeyboardType::ENGLISH_FOUR_ROW_LOWER || currentKeyboardType == KeyboardType::ENGLISH_FOUR_ROW_UPPER)
        {
            if (dicLetter['p'] != EmptyRect && (image.size().width - dicLetter['p'].x) > 3 * dicLetter['p'].width )
            {
                return false;
            }
        }
        return true;
    }

    std::string KeyboardRecognizer::GetCategoryName()
    {
        return objectCategory;
    }

    std::unordered_map<char, Rect> KeyboardRecognizer::GetKeyboardMapDictionary()
    {
        return dicLetter;
    }

    KeyboardType KeyboardRecognizer::GetKeyboardType()
    {
        return currentKeyboardType;
    }

    char KeyboardRecognizer::CorrectCharByKeyboardType(char c)
    {
        if (currentKeyboardType == KeyboardType::ENGLISH_FIVE_ROW_UPPER || currentKeyboardType == KeyboardType::ENGLISH_FOUR_ROW_UPPER)
        {
            if (c <= 'z' && c >= 'a')
                return c - 'a' + 'A';
            else
                return c;
        }
        else if (currentKeyboardType == KeyboardType::ENGLISH_DIGIT_FOUT_ROW)
        {
            switch (c)
            {
                case 'q':
                    return '1';
                case 'w':
                    return '2';
                case 'e':
                    return '3';
                case 'r':
                    return '4';
                case 't':
                    return '5';
                case 'y':
                    return '6';
                case 'u':
                    return '7';
                case 'i':
                    return '8';
                case 'o':
                    return '9';
                case 'p':
                    return '0';
                case 'a':
                    return '@';
                default:
                    return c;
            }
        }
        else if (currentKeyboardType == KeyboardType::CHINESE_DIGIT_FOUR_ROW)
        {
            switch (c)
            {
                case 'a': case 'b': case 'c':
                    return '2';
                case 'd': case 'e': case 'f':
                    return '3';
                case 'g': case 'h': case 'i':
                    return '4';
                case 'j': case 'k': case 'l':
                    return '5';
                case 'm': case 'n': case 'o':
                    return '6';
                case 'p': case 'q': case 'r': case 's':
                    return '7';
                case 't': case 'u': case 'v':
                    return '8';
                case 'w': case 'x': case 'y': case 'z':
                    return '9';
                default:
                    return c;
            }
        }
        return c;
    }

    char KeyboardRecognizer::FindCharByPoint(Point p)
    {
        vector<char> retChars;
        for(auto pair : dicLetter)
        {
            if(pair.second.contains(p))
            {
                retChars.push_back(CorrectCharByKeyboardType(pair.first));
            }
        }

        if (retChars.size() == 1)
        {
            return retChars[0];
        }
        else if (retChars.size() > 1)   // for Android L keyboard, '1' and 'q' are the same rectangle, should return 'q'
        {
            return retChars[0] > retChars[1] ? retChars[0] : retChars[1];
        }
        else
        {
            return '?'; // return questionmark if not found from point p
        }
    }

    void KeyboardRecognizer::ClickKeyboardForString(Mat rotatedFrame, string input)
    {
        for(auto c: input)
        {
            if (dicLetter.find(c) != dicLetter.end())
            {
                Rect charRect = dicLetter[c];
                Point charPoint = Point(charRect.x + charRect.width / 2, charRect.y + charRect.height / 2);
                ClickOnRotatedFrame(rotatedFrame.size(), charPoint);
            }
            else
            {
                DAVINCI_LOG_WARNING << "KeyboardRecognizer::ClickKeyboardForString -- Couldn't find " << c << " in the dictionary!";
            }
        }
    }

    void KeyboardRecognizer::DrawDebugPicture(const cv::Mat &image)
    {
        // draw debug frame
        cv::Mat debugFrame = image.clone();
        if (currentKeyboardType == KeyboardType::ENGLISH_FIVE_ROW_UPPER || currentKeyboardType == KeyboardType::ENGLISH_FIVE_ROW_LOWER || currentKeyboardType == KeyboardType::ENGLISH_FOUR_ROW_UPPER || currentKeyboardType == KeyboardType::ENGLISH_FOUR_ROW_LOWER)
        {
            for (std::unordered_map<char, Rect>::iterator it = dicLetter.begin(); it != dicLetter.end(); ++it)
            {
                if (it->first <= '9' && it->first >= '0')
                {
                    if (haveFiveRow == false)
                        continue;
                    else
                        rectangle(debugFrame, it->second, Yellow, 2);
                }

                else if(it->first >= 'a' && it->first <= 'm')
                    rectangle(debugFrame, it->second, Red, 2);
                else if (it->first >= 'n' && it->first <= 'z')
                    rectangle(debugFrame, it->second, Blue, 2);
                else if (it->first == '`')
                    rectangle(debugFrame, it->second, White, 2);
                else if (it->first == '#')
                    rectangle(debugFrame, it->second, Yellow, 2);
                else if(it->first == '$')
                    rectangle(debugFrame, it->second, Black, 5);
                else if(it->first == '.')
                    rectangle(debugFrame, it->second, Red, 1);
                else if(it->first == '~')
                    rectangle(debugFrame, it->second, Green, 1);
                else if(it->first == '_')
                    rectangle(debugFrame, it->second, Yellow, 1);
                else if(it->first == '@')
                    rectangle(debugFrame, it->second, White, 1);
                else
                    rectangle(debugFrame, it->second, Green, 2);
            }
        }
        else
        {
            for (std::unordered_map<char, Rect>::iterator it = dicLetter.begin(); it != dicLetter.end(); ++it)
            {
                char c = CorrectCharByKeyboardType(it->first);
                if (c <= '9' && c >= '7')
                    rectangle(debugFrame, it->second, Yellow, 2);
                else if (c <= '6' && c >= '4')
                    rectangle(debugFrame, it->second, Red, 2);
                else if (c <= '3' && c >= '0')
                    rectangle(debugFrame, it->second, Green, 2);
                else if (c == '`')
                    rectangle(debugFrame, it->second, White, 2);
                else if (c == '#')
                    rectangle(debugFrame, it->second, Blue, 2);
                else if(c == '$')
                    rectangle(debugFrame, it->second, White, 5);
                else if(it->first == '~')
                    rectangle(debugFrame, it->second, Green, 1);
                else if(it->first == 'a')
                    rectangle(debugFrame, it->second, Black, 1);
            }
        }

        DebugSaveImage(debugFrame, "keyboard.png");
    }

}
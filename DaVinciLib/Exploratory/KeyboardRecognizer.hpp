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

#ifndef __KEYBOARDRECOGNIZER__
#define __KEYBOARDRECOGNIZER__

#include "BaseObjectCategory.hpp"
#include "TextRecognize.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace DaVinci
{
    enum KeyboardType
    {
        NONE_KEYBOARD,                  // not a keyboard
        ENGLISH_FOUR_ROW_UPPER,         // English alphabet keyboard with four rows (upper case)
        ENGLISH_FOUR_ROW_LOWER,         // English alphabet keyboard with four rows (lower case)
        ENGLISH_FIVE_ROW_UPPER,         // English alphabet keyboard with five rows (upper case)
        ENGLISH_FIVE_ROW_LOWER,         // English alphabet keyboard with five rows (lower case)
        ENGLISH_DIGIT_FOUT_ROW,         // English digital keyboard with four rows
        CHINESE_DIGIT_FOUR_ROW          // Chinese keyboard with four rows (3x3)
    };

    /// <summary>
    /// Keyboard recognizer class
    /// </summary>
    class KeyboardRecognizer : public BaseObjectCategory
    {
    private:
        std::string objectCategory;

        static const size_t minRectNum = 20;
        static const size_t maxRectNum = 80;
        static const size_t minRectNumForDigit = 9;
        static const size_t maxRectNumForDigit = 60;
        static const size_t minRectNumOfEachRow = 5;
        static const int maxYfilter = 20;
        static const int matchLetterThreshold = 10;
        static const int matchDigitListThreshold = 7;
        static const int minHeight = 20;

        int rectangleWidth;
        int rectangleHeight;
        int interval;
        int imageWidth;

        bool haveFiveRow;

        std::vector<int> rowYcoordinate; // the Y-axis of each row
        std::vector<int> allRowAxis;

        std::vector<Rect> row0;
        std::vector<Rect> row1;
        std::vector<Rect> row2;
        std::vector<Rect> row3;
        std::vector<Rect> row4;

        std::vector<Rect> rowAll;

        std::unordered_map<char, Rect> dicLetter;

        KeyboardType currentKeyboardType;

        bool recognizeNavigationBar;

        std::vector<char> sepicalKeys;

    public:
        KeyboardRecognizer();

        /// <summary>
        /// Recognize keyboard
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        bool RecognizeKeyboard(const cv::Mat &image, string osVersion, cv::Rect keyboardROI = EmptyRect);

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

        /// <summary>
        /// Get keyboard map dictionary
        /// </summary>
        /// <returns></returns>
        std::unordered_map<char, Rect> GetKeyboardMapDictionary();

        KeyboardType GetKeyboardType();

        char FindCharByPoint(Point p);

        void ClickKeyboardForString(Mat rotatedFrame, string input);

        void setRecognizeNavigationBar(bool flag);

        void SetOSversion(string s);
        vector<Rect> EnlargeRect( vector<Rect> &rowAll);

        void DrawDebugPicture(const cv::Mat &image);

        bool isSwitchButton(char c);

        bool isEnterButton(char c);

        bool isDeleteButton(char c);

        bool isUnsupportedButton(char c);

    private:
        string osVersion;

        void initDicLetter();

        void reset();

        std::vector<Rect> mergeRectangleWithSimailarYAxis(std::vector<Rect> rectList, int interval);

        bool rowContainsSimilarXAxis(int x, std::vector<Rect> &lstRect);

        bool allRowContainSamePoint(Rect rect, int rowInterval);

        // Contruct space rectangle, since the width is bigger than rectangle width
        Rect ConstructSpaceRectangle(int x, int rowNumber);

        // Construct rectangle according to the x, set the Y is each row's Y
        // Set the width is the rectangle width, set the height is the rectangle height
        Rect ConstructRectangle(int x, int rowNumber);

        // Resize rectangle, set the Y is each row's Y, set the width is the rectangle width, set the height is the rectangle height
        Rect ResizeRectangle(Rect rect, int rowNumber);

        // check how many rectangles match the letter
        int MatchLetterCount(const cv::Mat image, std::vector<Rect> allRow, int &upperCaseNum, int &lowerCaseNum, int &digitalNum);

        bool isLetterUpperCase(string text);
        bool isLetterLowerCase(string text);
        bool isDigit(string text);

        bool allRowContainSimilarYAxis(int y);
        bool allRowContainSimilarXAxis(int x);

        std::vector<std::string> ocrWhiteList;
        std::vector<std::string> digitWhiteList;

        string strMatchDone;
        wstring wstrMatchDone;
        string strMatchNumber;
        wstring wstrMatchNumber;
        string strMatchChinese;
        wstring wstrMatchChinese;
        string strMatchAt;
        string strMatchUnderLine;
        string strMatchDelete;

        bool isAlphabetKeyboard(const cv::Mat &image, std::vector<Rect> rectList);
        bool isDigitKeyboard(const cv::Mat &image, std::vector<Rect> rectList);
        bool isAlphabetKeyboardNoBondary(const cv::Mat &image, std::vector<Rect> rectList);

        bool inDigitWhiteList(string text);

        bool verifyEachKey(const cv::Mat &image);
        bool MapEachKey(const cv::Mat &image);
        void MapFirstRow(const cv::Mat &image);
        void MapSecondRow(const cv::Mat &image);
        void MapThirdRow(const cv::Mat &image);
        void MapFourthRow(const cv::Mat &image);

        void MapSepcialKey(vector<Rect> rectList, const cv::Mat &image);

        bool matchKeyword(string text, Language language, string strMatch, wstring wstrMatch);
        char CorrectCharByKeyboardType(char c);
    };
}


#endif	//#ifndef __KEYBOARDRECOGNIZER__

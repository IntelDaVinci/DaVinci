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
// -----------------------------------------------------------------------------------------

#ifndef __TEXTUTIL__
#define __TEXTUTIL__


#include <vector>
#include <map>
#include <regex>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "DaVinciStatus.hpp"
#include "ObjectCommon.hpp"
#include "SceneTextExtractor.hpp"
#include "TextRecognize.hpp"
#include "ExploratoryEngine.hpp"
#include "KeywordCheckHandlerForProcessExit.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    /// <summary>
    /// Class TextUtil
    /// </summary>
    class TextUtil : public SingletonBase<TextUtil>
    {
        SIGLETON_CHILD_CLASS(TextUtil);
    private:
        static boost::mutex textRecognizeLock;

        boost::shared_ptr<TextRecognize> enTextRecognize;
        boost::shared_ptr<TextRecognize> zhTextRecognize;

        boost::shared_ptr<ExploratoryEngine> exploratoryEngine;
        boost::shared_ptr<KeywordCheckHandlerForProcessExit> processExitHandler;
        boost::shared_ptr<SceneTextExtractor> sceneTextExtractor;

        double validCharacterRatio;
        const static int validCharacterLength = 2;
        const static int extendedTextWidth = 20;
        double maxRectRatio;
    public:
        /// <summary>
        /// Recognize english text
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        std::string RecognizeEnglishText(const cv::Mat &image, PageIteratorLevel level = PageIteratorLevel::RIL_SYMBOL);

        /// <summary>
        /// Recognize chinese text
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        std::string RecognizeChineseText(const cv::Mat &image, PageIteratorLevel level = PageIteratorLevel::RIL_SYMBOL);

        /// <summary>
        /// OCR chinese text
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        TextRecognize::TextRecognizeResult AnalyzeChineseOCR(const cv::Mat &image, PageIteratorLevel level = PageIteratorLevel::RIL_SYMBOL);

        /// <summary>
        /// OCR english text
        /// </summary>
        /// <param name="image"></param>
        /// <returns></returns>
        TextRecognize::TextRecognizeResult AnalyzeEnglishOCR(const cv::Mat &image, PageIteratorLevel level = PageIteratorLevel::RIL_SYMBOL);

        cv::Rect ExtractSceneTextShapes(const cv::Mat &frame, const cv::Rect &roi, const cv::Point &clickPoint = EmptyPoint);

        vector<Rect> ExtractSceneTexts(const cv::Mat &frame, const cv::Rect &roi);

        vector<Rect> ExtractRSTRects(const cv::Mat &frame,string language = "en");

        //cv::Rect ExtractContourTextShapes(const cv::Mat &frame, const cv::Rect &roi, const cv::Point &clickPoint = EmptyPoint, int xKernelSize = 6);

        cv::Rect ExtractTextShapes(const cv::Mat &frame, const cv::Point &clickPoint, int textWidth = 500, int textHeight = 200);

        /// <summary>
        /// Handle text pattern for recording script: text          button (on/off)
        /// Need to extract text rect
        /// </summary>
        /// <param name="image">frame</param>
        /// <param name="clickPoint">recording clickPoint</param>
        /// <param name="textHeight">text height</param>
        /// <returns></returns>
        cv::Rect ExtractTextShapesNearButton(const cv::Mat &frame, const cv::Point &clickPoint, int textHeight = 50);

        /// <summary>
        /// Handle text pattern for replaying script: text          button (on/off)
        /// Need to click on/off button
        /// </summary>
        /// <param name="frame">frame</param>
        /// <param name="sourceTextRect">recording text rect</param>
        /// <param name="sourceButtonPoint">recording button point</param>
        /// <param name="targetTextRect">replaying text rect</param>
        /// <returns></returns>
        cv::Rect ExtractButtonRectByTextButtonPair(const cv::Mat &frame, const cv::Rect &sourceTextRect, const cv::Point &sourceButtonPoint, const cv::Rect &targetTextRect);

        ////// <summary>
        /// Trim the text around with spaces
        /// </summary>
        std::string PostProcessText(std::string text);

        // Allow hammin distance (2 for Chinese)
        bool MatchText(std::string text, std::string expectText, int &hammDistance, string language = "en", bool spaceIgnore = true, int allowHammDistance = -1);

        int MeasureHammDistance(std::string text, std::string expectText);

        std::string GetLCS(std::string text, std::string expectText);

        bool IsExpectedProcessExit(const cv::Mat &frame, const cv::Point &clickPoint, const string language, vector<Keyword> clickedKeywords);

        /// <summary>
        /// Check valid English text
        /// </summary>
        /// <param name="text"></param>
        /// <returns></returns>
        bool IsValidEnglishText(const std::string &text);

        /// <summary>
        /// Check valid Chinese text
        /// </summary>
        /// <param name="text"></param>
        /// <returns></returns>
        bool IsValidChineseText(const std::string &text);

        bool IsValidTextLength(int validLength, int originLength);
    };
}


#endif	//#ifndef __TEXTUTIL__

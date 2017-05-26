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

#ifndef __DAVINCI_TEXTRECOGNIZE_HPP__ 
#define __DAVINCI_TEXTRECOGNIZE_HPP__

#include "opencv/cv.h"
#include "OCRTesseract.hpp"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/shared_array.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/lock_guard.hpp"
#include "SceneTextExtractor.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum class PageIteratorLevel {
        RIL_BLOCK,     // Block of text/image/separator line.
        RIL_PARA,      // Paragraph within a block.
        RIL_TEXTLINE,  // Line within a paragraph.
        RIL_WORD,      // Word within a textline.
        RIL_SYMBOL     // Symbol/character within a word.
    };

    enum class AccuracyModel
    {
        FAST_MODE , 
        BETTER_ACCURACY_MODE,
        BEST_ACCURACY_MODE,
        DEFAULT_MODE,
    };

    enum Language
    {
        ENGLISH = 1, 
        SIMPLIFIED_CHINESE = 1 << 1,
        JAPANESE = 1 << 2
    };

    /// <summary>
    /// Recognize texts from an image with Tesseract OCR library.
    /// </summary>
    class TextRecognize
    {
    public:   
        class TextRecognizeResult
        {
        public:
            TextRecognizeResult(const string& textP,  const vector<Rect>& componentRectsP,const vector<string>& componentTextsP,
                const vector<float>& componentConfidencesP, 
                Size imageSizeP,int paramP,PageIteratorLevel levelP);

            TextRecognizeResult();

            vector<Rect> GetBoundingRects(const string& pattern,float confidenceThresholdP = 0);

            //bool IsEmpty();

            string GetRecognizedText() const;

            std::vector<cv::Rect> GetComponentRects();

            std::vector<std::string> GetComponentTexts();

            std::vector<float> GetComponentConfidences();

        private:
            PageIteratorLevel level;

            vector<Rect> componentRects;

            vector<string> componentTexts;

            vector<float> componentConfidences;

            float confidenceThreshold;

            string recognizedText;

            boost::shared_array<int> charactorMap; 
        };

        /// <summary>
        /// 2-bit: image scale factor
        /// 0: do not scale, 1: scale 2X, 2: scale 3X, 3: scale 4X
        /// </summary>
        const static int PARAM_SCALE_2 = 1;
        /// <summary>
        /// See doc of PARAM_SCALE_2
        /// </summary>
        const static int PARAM_SCALE_3 = 2;
        /// <summary>
        /// See doc of PARAM_SCALE_2
        /// </summary>
        const static int PARAM_SCALE_4 = 3;
        /// <summary>
        /// 2-bit: upper or lower region of the image to recognize
        /// 0: entire image, 1: upper 1/2, 2: lower 1/2, 3: undefined
        /// </summary>
        const static int PARAM_ROI_UPPER = 4;
        /// <summary>
        /// See doc of PARAM_ROI_UPPER
        /// </summary>
        const static int PARAM_ROI_LOWER = 8;
        /// <summary>
        /// 2-bit: left or right region of the image to recognize
        /// 0: entire image, 1: left 1/2, 2: right 1/2, 3: undefined
        /// </summary>
        const static int PARAM_ROI_LEFT = 16;
        /// <summary>
        /// See doc of PARAM_ROI_LEFT
        /// </summary>
        const static int PARAM_ROI_RIGHT = 32;
        /// <summary>
        /// 2-bit: OCR mode: default, fast, better and best of quality
        /// 0: default, 1: fast, 2: better, 3: best
        /// </summary>
        const static int PARAM_OCR_MODE_DEFAULT = (int)(AccuracyModel::DEFAULT_MODE) << 6;
        /// <summary>
        /// See PARAM_OCR_MODE_DEFAULT
        /// </summary>
        const static int PARAM_OCR_MODE_BEST_ACCURACY = (int)(AccuracyModel::BEST_ACCURACY_MODE )<< 6;
        /// <summary>
        /// See PARAM_OCR_MODE_DEFAULT
        /// </summary>
        const static int PARAM_OCR_MODE_BETTER_ACCURACY = (int)(AccuracyModel::BETTER_ACCURACY_MODE) << 6;
        /// <summary>
        /// See PARAM_OCR_MODE_DEFAULT
        /// </summary>
        const static int PARAM_OCR_MODE_FAST =(int)(AccuracyModel:: BETTER_ACCURACY_MODE )<< 6;
        /// <summary>
        /// 1-bit: indicate the image is captured from a camera and may contain noise
        /// </summary>
        const static int PARAM_CAMERA_IMAGE = 256;

        /// <summary>
        /// Expert edition of constructor that accepts tuning of all the parameters
        /// </summary>
        /// <param name="connectThreshold">Threshold to decide two pixels are connected</param>
        TextRecognize(AccuracyModel accuracyModel = AccuracyModel::DEFAULT_MODE, string whitelistP="", int languageMode = 1, int connectThresholdP=10);

        TextRecognizeResult  DetectDetail(const Mat& image, int param=0, PageIteratorLevel pl = PageIteratorLevel::RIL_SYMBOL,bool useSceneText=false);

        string RecognizeText(const Mat& image, int param=0,bool useSceneText=false, vector<Rect>* rects =NULL, vector<string>* strings = NULL);

        string LanguageIntToStr(int language);

   /*     void SetWhiteList(string str);*/

        void SetVariable(char*key, char* value);

        void GetVariable(char* key, string& str);

        vector<Rect> GetSceneTextRect(const Mat& src);
    private:
        //void SetDefaultParams();
        string langMode;
        RobustSceneText rst;
        Ptr<OCRTesseract> tessObjs;
        const static int PARAM_SCALE_MASK = 3; 
        const static int PARAM_ROI_UPPER_LOWER_MASK = 12;
        const static int PARAM_ROI_LEFT_RIGHT_MASK = 48;   
        const static int PARAM_OCR_MODE_MASK = 3 << 6;
        vector<string> languages;
        static  boost::mutex tessObjsLock;
        Mat Preprocess(const Mat& image, int param,bool needGray=true);     
        //int GetOCRMode(int param);
        int connectThreshold ;
      //  vector<Rect> MergeIntersectRects(const vector<Rect>& rects);
      //  bool RectsInsectWithoutAngle(const Rect& r1, const Rect& r2);

        //struct TopLeftFunctor
        //{
        //    bool operator()(const Rect& rectL,const Rect& rectR)
        //    {
        //        if(rectL.y<rectR.y||(rectL.y==rectR.y&&rectL.x<rectR.x))
        //            return true;
        //        else
        //            return false;
        //    }
        //};

    };
}
#endif
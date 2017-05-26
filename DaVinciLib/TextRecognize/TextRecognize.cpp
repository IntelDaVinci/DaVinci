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

#include "TestManager.hpp"
#include "TextRecognize.hpp"
#include <regex>
#include <limits>
#include <bitset>

namespace DaVinci
{
    boost::mutex TextRecognize::tessObjsLock;
    TextRecognize::TextRecognizeResult::TextRecognizeResult()
    {
        level = PageIteratorLevel::RIL_TEXTLINE;
        confidenceThreshold=0.0;
    }

    string TextRecognize::TextRecognizeResult::GetRecognizedText() const
    {
        return recognizedText;
    }

    string TextRecognize::LanguageIntToStr(int language)
    {
        string lang;
        bitset<3> b(language);

        for(int i = 1; i <= 3; i++)
        {
            if(b[3-i] == 1)
            {
                lang += languages[3-i] + "+";
            }
        }

        if(boost::equals(lang, "+"))
            lang = "eng";
        else
        {
            lang = lang.substr(0, lang.length() - 1);
        }

        return lang;
    }

    TextRecognize::TextRecognizeResult::TextRecognizeResult(const string& RecognizedTextP,
        const vector<Rect>& componentRectsP,
        const vector<string>& componentTextsP,
        const vector<float>& componentConfidencesP, 
        Size imageSizeP, int paramP, PageIteratorLevel levelP)
    {
        level = levelP;
        confidenceThreshold=0.0;
        recognizedText =RecognizedTextP;
        componentTexts=componentTextsP;
        componentRects=componentRectsP;
        componentConfidences=componentConfidencesP;
        int componentSize = (int)(componentTextsP.size());
        // if we have scaled the image for small font, revert the axis back to the unscaled image.
        if ((paramP & PARAM_SCALE_MASK) != 0)
        {
            int scale = (paramP & PARAM_SCALE_MASK) + 1;
            for (int i = 0; i < componentSize; i++)
            {
                componentRects[i].x/=scale;
                componentRects[i].y=scale;
                componentRects[i].width/=scale;
                componentRects[i].height/=scale;
            }
        }
        // correct the position of charactors if the image is clipped
        if ((paramP & PARAM_ROI_UPPER_LOWER_MASK) == PARAM_ROI_LOWER)
        {
            for (int i = 0; i < componentSize; i++)
            {
                componentRects[i].y += imageSizeP.height / 2;
            }
        }
        if ((paramP & PARAM_ROI_LEFT_RIGHT_MASK) == PARAM_ROI_RIGHT)
        {
            for (int i = 0; i < componentSize; i++)
            {
                componentRects[i].x += imageSizeP.height / 2;
            }
        }

        int textSize = static_cast<int>(recognizedText.size());
        int componentTextSize = static_cast<int>(componentTexts.size());
        charactorMap =boost::shared_array<int>(new int[textSize]);

        int j = 0; // index in charactors
        for (int i = 0; i < textSize; i++)
        {
            char c = recognizedText[i];

            if(c == ' ' || c == 10) // ' ' || ''
            {
                charactorMap.get()[i] = -1;
                continue;
            }

            // charactor always has 1 char
            while (j < componentTextSize && (componentTexts[j][0] == ' ' || componentTexts[j].length() == 0))
            {
                j++;
            }
            if (j == componentTextSize || c != componentTexts[j][0])
            {
                charactorMap.get()[i] = -1;
            }
            else
            {
                charactorMap.get()[i] = j++;
            }
        }
    }

    vector<Rect> TextRecognize::TextRecognizeResult::GetBoundingRects(const string& pattern, float confidenceThresholdP)
    {
        vector<Rect> rects;

        string preprocessPattern = pattern;
        std::regex rx(preprocessPattern);

        if(level == PageIteratorLevel::RIL_SYMBOL)
        { 
            const std::sregex_token_iterator end;
            for (std::sregex_token_iterator iter(recognizedText.begin(), recognizedText.end(), rx); iter != end; ++iter)
            {
                string str = iter->str();
                int length = static_cast<int>(iter->length());
                int strIndex = (int)(iter->first - recognizedText.begin());
                int left =  INT_MAX;
                int right = INT_MIN;
                int top =  INT_MAX;
                int bottom = INT_MIN;

                int index = 0;
                for (int i = 0; i < length; i++)
                {
                    index = i + strIndex;

                    int charactorIndex = charactorMap.get()[index];
                    if (charactorIndex != -1)
                    {
                        Rect rect = componentRects[charactorIndex];
                        if(rect.x < left)
                        {
                            left = rect.x;
                        }

                        if(rect.y < top)
                        {
                            top = rect.y;
                        }

                        if(rect.y + rect.height > bottom)
                        {
                            bottom = rect.y + rect.height;
                        }

                        if(rect.x + rect.width > right)
                        {
                            right = rect.x + rect.width;
                        }
                    }
                }
                if (left <= right && top <= bottom)
                {
                    rects.push_back(Rect(left, top, right - left, bottom - top));
                }
            }
            return rects;
        }
        else
        {
            int compoSize = (int)(componentTexts.size());
            for(int i=0;i<compoSize;++i)
            {
                std::tr1::cmatch  matches;
                bool res = std::tr1::regex_search(componentTexts[i].c_str(),matches,rx);
                if(res&&componentConfidences[i]>confidenceThresholdP)
                {
                    rects.push_back(componentRects[i]);
                }
            }
            return rects;
        }
    }

    //bool TextRecognize::TextRecognizeResult::IsEmpty()
    //{
    //    if(componentRects.size()>0)
    //        return false;
    //    else
    //        return true;
    //}

    std::vector<cv::Rect> TextRecognize::TextRecognizeResult::GetComponentRects()
    {
        return this->componentRects;
    }

    std::vector<std::string> TextRecognize::TextRecognizeResult::GetComponentTexts()
    {
        return this->componentTexts;
    }

    std::vector<float> TextRecognize::TextRecognizeResult::GetComponentConfidences()
    {
        return this->componentConfidences;
    }

    //psmode not exposed
    //Note: Best_Accuracy_Mode only supports English
    TextRecognize::TextRecognize(AccuracyModel accuracyModel, string whitelistP, int languageMode, int connectThresholdP)
    {
        connectThreshold = connectThresholdP;
        languages.push_back("eng");
        languages.push_back("chi_sim");
        languages.push_back("jap");

        langMode = LanguageIntToStr(languageMode);
        string dataPath = TestManager::Instance().GetDaVinciResourcePath("TesseractData");

        if(whitelistP=="")
        {
            tessObjs= OCRTesseractImpl::create(dataPath.c_str(),langMode.c_str(),NULL,(int)accuracyModel);
        }
        else
        {
            tessObjs= OCRTesseractImpl::create(dataPath.c_str(),langMode.c_str(),whitelistP.c_str(),(int)accuracyModel);
        }
        //SetDefaultParams();
    }

    Mat TextRecognize::Preprocess(const Mat& cImage, int param,bool needGray)
    {
        Mat image;
        if(cImage.channels()==3&&needGray==true)
            cvtColor(cImage,image,CV_BGR2GRAY);
        else
        {
            image=cImage.clone();
        }
        bool needSmooth = (param & PARAM_CAMERA_IMAGE) != 0;
        // selective gaussian smoothing to remove noise but preserve edges
        if (needSmooth)
        {
            Mat result=Mat(image.rows,image.cols,image.type());
            cv::bilateralFilter( image, result, 0, 30, 7, cv::BORDER_REPLICATE );
            return result;
        }
        else
        {
            return image;
        }
    }

    //int TextRecognize::GetOCRMode(int param)
    //{
    //    return (param & PARAM_OCR_MODE_MASK) >> 6;
    //}

    TextRecognize::TextRecognizeResult TextRecognize::DetectDetail(const Mat& image, int param, PageIteratorLevel pl, bool useSceneText)
    {
        TextRecognizeResult result;
        Mat outImage = Preprocess(image, param,false);
        Mat gray;
        if(outImage.channels()==3)
            cvtColor(outImage,gray,CV_BGR2GRAY);
        else
            gray = outImage;
        if(useSceneText)
        {
            vector<Rect> rects=GetSceneTextRect(outImage);
            // vector<Rect>rects=MergeIntersectRects(sceneRects);
            int rectSize = (int)(rects.size());
            vector<Rect> componentRects;
            vector<string> componentTexts;
            vector<float> componentConfidences;
            string outputText;
            for(int i=0;i<rectSize;++i)
            {
                vector<Rect> componentRectsInner;
                vector<string> componentTextsInner;
                vector<float> componentConfidencesInner;
                string outputTextInner;
                boost::lock_guard<boost::mutex> lock(tessObjsLock);
                int padWidth = (int)min(15.0,rects[i].width*0.2);
                int padHeight = (int)min(10.0,rects[i].height*0.25);
                int left= max(rects[i].x-padWidth,0);
                int right = min(rects[i].x+rects[i].width+padWidth-1,outImage.cols-1);
                int up = max(rects[i].y-padHeight,0);
                int down =min(rects[i].y+rects[i].height+padHeight-1,outImage.rows-1);
                Rect rect(Point(left,up),Point(right,down));
                Mat roi = gray(rect);
                tessObjs->run(roi,outputTextInner,&componentRectsInner,&componentTextsInner,&componentConfidencesInner,(int)pl);
                int innerRectSize = (int)(componentRectsInner.size());
                for(int j=0;j<innerRectSize;++j)
                {
                    componentRectsInner[j].x+=rects[i].x-padWidth;
                    componentRectsInner[j].y+=rects[i].y-padHeight;
                }
                outputText+=outputTextInner;
                componentRects.insert(componentRects.end(),componentRectsInner.begin(),componentRectsInner.end());
                componentTexts.insert(componentTexts.end(),componentTextsInner.begin(),componentTextsInner.end());
                componentConfidences.insert(componentConfidences.end(),componentConfidencesInner.begin(),componentConfidencesInner.end());

            }
            result =TextRecognizeResult(outputText, componentRects,componentTexts,componentConfidences,image.size(), param,pl);

        }
        else
        {
            try
            {
                boost::lock_guard<boost::mutex> lock(tessObjsLock);
                string outputText;
                vector<Rect> componentRects;
                vector<string> componentTexts;
                vector<float> componentConfidences;
                tessObjs->run(gray,outputText,&componentRects,&componentTexts,&componentConfidences,(int)pl);
                result =TextRecognizeResult(outputText, componentRects,componentTexts,componentConfidences,image.size(), param,pl);
            }
            catch (Exception exp)
            {
                result = TextRecognizeResult();
            }
        }
        return result;
    }


    /// <summary>
    /// Detect text from an image
    /// </summary>
    /// <param name="image">The input image to detect text from.</param>
    /// <param name="param">See PARAM_XXX</param>
    /// <param name="languageMode">Language mode</param>
    /// <returns>The recognized details or null if something wrong.</returns>
    string TextRecognize::RecognizeText(const Mat& image, int param,bool useSceneText,vector<Rect> *pRects, vector<string> *pRstStrings )
    {
        Mat outImage = Preprocess(image, param,false);
        string result;
        Mat gray;
        if(outImage.channels()==3)
            cvtColor(outImage,gray,CV_BGR2GRAY);
        else
            gray = outImage;
        try
        {     
            if(useSceneText)
            {   
                vector<Rect> rects=GetSceneTextRect(outImage);
                if (pRects!= NULL)
                {
                    *pRects = rects;
                }
#ifdef _DEBUG
                Mat drawImageSce = outImage.clone();
                int sceneSize = (int)(rects.size());
                for(int i=0;i<sceneSize;++i)
                {
                    rectangle(drawImageSce,rects[i],Scalar( 0,0 , 255 ));
                }
                imwrite("sceneOCRwholetest.jpg",drawImageSce);
#endif
                // vector<Rect>rects=MergeIntersectRects(sceneRects);
                int rectSize = (int)(rects.size());
#ifdef _DEBUG
                Mat drawImage = outImage.clone();
                for(int i=0;i<rectSize;++i)
                {
                    rectangle(drawImage,rects[i],Scalar( 0,0 ,255 ),5);
                }
                imwrite("sceneOCRtest.jpg",drawImage);
#endif
                for(int i=0;i<rectSize;++i)
                {
                    boost::lock_guard<boost::mutex> lock(tessObjsLock);
                    int padWidth = (int)min(15.0,rects[i].width*0.05);
                    int padHeight = (int)min(10.0,rects[i].height*0.02);
                    int left= max(rects[i].x-padWidth,0);
                    int right = min(rects[i].x+rects[i].width+padWidth-1,outImage.cols-1);
                    int up = max(rects[i].y-padHeight,0);
                    int down =min(rects[i].y+rects[i].height+padHeight-1,outImage.rows-1);
                    Rect rect(Point(left,up),Point(right,down));
                    Mat roi = gray(rect);
                    string temp;
                    tessObjs->run(roi,temp);
                    if(pRstStrings !=NULL)
                    {
                        pRstStrings->push_back(temp);
                    }
                    if(result=="")
                    {
                        result+=temp;
                    }
                    else
                    {
                        result+=" "+temp;
                    }
                }
            }
            else
            {
                boost::lock_guard<boost::mutex> lock(tessObjsLock);
                tessObjs->run(gray,result); 
            }
        }
        catch (Exception exp)
        {
            // Console.WriteLine("Exception raised: " + exp.Message);
            result="";
        }

        return result;
    }

    void TextRecognize::SetVariable(char*key, char* value)
    {
        boost::lock_guard<boost::mutex> lock(tessObjsLock);
        tessObjs->setVariable(key, value);
    }

    void TextRecognize::GetVariable(char* key, string& str)
    {
        tessObjs->getVariable(key,str);
    }

    //void TextRecognize::SetWhiteList(string whitelist)
    //{
    //    if(whitelist.length()>0)
    //        SetVariable("tessedit_char_whitelist", (char*)whitelist.c_str());
    //    else
    //        SetVariable("tessedit_char_whitelist", NULL);//reset whitelist
    //}

    //vector<Rect> TextRecognize::MergeIntersectRects(const vector<Rect>& rects)
    //{	
    //    int size = rects.size();
    //    bool* isErased= new bool[size];
    //    memset(isErased,false,size*sizeof(bool));
    //    vector<Rect> temp = rects;
    //    for(int i=0;i<size;++i)
    //    {
    //        for(int j=i+1;j<size;++j)
    //        {
    //            bool bIntersect = RectsInsectWithoutAngle(temp[i],temp[j]);
    //            if(bIntersect)
    //            {
    //                isErased[i]=true;
    //                int left = min(temp[i].x,temp[j].x);
    //                int top = min(temp[i].y,temp[j].y);
    //                int right = max(temp[i].x+temp[i].width,temp[j].x+temp[j].width);
    //                int bottom = max(temp[i].y+temp[i].height,temp[j].y+temp[j].height);
    //                temp[j]=Rect(Point(left,top),Point(right,bottom));
    //                break;
    //            }
    //        }
    //    }
    //    vector<Rect> res;
    //    for(int i=0;i<size;++i)
    //    {
    //        if(isErased[i]==false)
    //        {
    //            res.push_back(temp[i]);
    //        }
    //    }
    //    delete[] isErased;
    //    sort(res.begin(),res.end(),TopLeftFunctor());
    //    return res;
    //}

    vector<Rect> TextRecognize::GetSceneTextRect(const Mat& src)
    {
        vector<Rect> textRects;
        rst.DetectText(src,textRects);
        return textRects;
    }

    //bool TextRecognize::RectsInsectWithoutAngle(const Rect& r1, const Rect& r2)
    //{
    //    return ! ( r2.x > (r1.x+r1.width)
    //        || (r2.x+r2.width)< r1.x
    //        || (r2.y > r1.y+r1.height)
    //        || (r2.y+r2.height)<r1.y
    //        );
    //}

    //void TextRecognize::SetDefaultParams()
    //{
    //    if("chi_sim"==langMode||"jap"==langMode)
    //    {
    //        SetVariable("edges_max_children_per_outline","40");
    //    }
    //    else
    //    {
    //        SetVariable("edges_max_children_per_outline","10");
    //    }

    //}
}
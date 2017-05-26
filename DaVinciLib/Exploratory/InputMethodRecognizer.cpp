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

#include <vector>

#include "InputMethodRecognizer.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"

using namespace std;
using namespace cv;

namespace DaVinci
{
    InputMethodRecognizer::InputMethodRecognizer()
    {
        inputList.push_back(L"拼音");
        okList.push_back(L"确定");
    }

    Rect InputMethodRecognizer::GetInputRect()
    {
        return inputRect;
    }

    Rect InputMethodRecognizer::GetOkRect()
    {
        return okRect;
    }

    bool InputMethodRecognizer::MatchInput(wstring targetStr)
    {
        for(auto input: inputList)
        {
            if(boost::contains(input, targetStr))
            {
                return true;
            }
        }

        return false;
    }

    bool InputMethodRecognizer::MatchOk(wstring targetStr)
    {
        for(auto ok: okList)
        {
            if(boost::contains(ok, targetStr))
            {
                return true;
            }
        }

        return false;
    }

    bool InputMethodRecognizer::RecognizeInitialInput(const cv::Mat &frame)
    {
        Size frameSize = frame.size();
        int kernalSize = 3; // Default Chinese Input
        std::vector<cv::Rect> suspectedRects = ObjectUtil::Instance().DetectRectsByContour(frame, 50, kernalSize, kernalSize, 1.5, 10, 500, 50000);
        boost::shared_ptr<SceneTextExtractor> sceneTextExtractor = boost::shared_ptr<SceneTextExtractor>(new SceneTextExtractor());
        sceneTextExtractor->Extract(frame);
        vector<Rect> sceneTextRects = sceneTextExtractor->GetRecognizedRects();
        AddVectorToVector(suspectedRects, sceneTextRects);
        for (auto sRect : suspectedRects)
        {
            if(sRect.width > 150)
                continue;

            cv::Mat rectFrame;
            ObjectUtil::Instance().CopyROIFrame(frame, sRect, rectFrame);
            DebugSaveImage(rectFrame, "rect.png");

            string text = TextUtil::Instance().RecognizeChineseText(rectFrame);
            boost::trim(text);
            wstring wtext = StrToWstr(text.c_str());
            if(boost::equals(wtext, ""))
                continue;

            if(MatchInput(wtext))
            {
                inputRect = sRect;
            }
            else if(MatchOk(wtext))
            {
                okRect = sRect;
            }

            if(inputRect != EmptyRect && okRect != EmptyRect)
                break;
        }

        if(inputRect != EmptyRect && okRect != EmptyRect)
            return true;
        else
            return false;
    }

}

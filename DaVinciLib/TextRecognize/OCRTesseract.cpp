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

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>
#endif
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv/highgui.h"
#include "OCRTesseract.hpp"
#include <iostream>
#include <DaVinciStatus.hpp>

namespace DaVinci
{
    using namespace std;

    //Default constructor
    OCRTesseractImpl::OCRTesseractImpl(const char* datapath, const char* language, const char* char_whitelist, int oemode, int psmode)
    {
#ifdef HAVE_TESSERACT
        const char *lang = "eng";
        if (language != NULL)
            lang = language;
        if (tess.Init(datapath, lang, tesseract::OcrEngineMode(oemode)))
        {
            DAVINCI_LOG_ERROR << "OCRTesseract: Could not initialize tesseract.";
            throw errc::io_error;
        }
        if(char_whitelist != NULL)
            tess.SetVariable("tessedit_char_whitelist", char_whitelist);
        tess.SetVariable("save_best_choices", "T");
#else
        DAVINCI_LOG_WARNING << "OCRTesseract("<<oemode<<psmode<<"): Tesseract not found." << endl;
        if (datapath != NULL)
            DAVINCI_LOG_INFO << "            " << datapath << endl;
        if (language != NULL)
            DAVINCI_LOG_INFO << "            " << language << endl;
        if (char_whitelist != NULL)
            DAVINCI_LOG_INFO << "            " << char_whitelist << endl;
#endif
    }

    OCRTesseractImpl::~OCRTesseractImpl()
    {
#ifdef HAVE_TESSERACT
        tess.End();
#endif
    }

    void OCRTesseractImpl::setVariable(const char* vName, const char* vValue)
    {
        tess.SetVariable(vName, vValue);
    }

    void OCRTesseractImpl::getVariable(const char* vName, string& vValue)
    {
        STRING str;
        tess.GetVariableAsString(vName,&str);
        const char* pc = str.string();
        vValue = (string)pc;
    }

    void OCRTesseractImpl::run(Mat& image, string& output, vector<Rect>* component_rects,
        vector<string>* component_texts, vector<float>* component_confidences,
        int component_level)
    {

        assert( (image.type() == CV_8UC1) || (image.type() == CV_8UC1) );

#ifdef HAVE_TESSERACT

        if (component_texts != 0)
            component_texts->clear();
        if (component_rects != 0)
            component_rects->clear();
        if (component_confidences != 0)
            component_confidences->clear();
        tess.SetImage((uchar*)image.data, image.size().width, image.size().height, image.channels(), (int)(image.step1()));
        if (tess.Recognize(NULL) != 0)
        {
            DAVINCI_LOG_ERROR<<"Recognized error";
        }
        char* temp = tess.GetUTF8Text();
        output = string(temp);
        delete[] temp;

        if ( (component_rects != NULL) || (component_texts != NULL) || (component_confidences != NULL) )
        {
            tesseract::ResultIterator* ri = tess.GetIterator();
            tesseract::PageIteratorLevel level= (tesseract::PageIteratorLevel)(component_level);//changed
         
            if (ri != 0) {
                do {
                    const char* word = ri->GetUTF8Text(level);//changed
                    if (word == NULL)
                    {
                        continue;
                    }
                    float conf = ri->Confidence(level);
                    int x1, y1, x2, y2;

                    ri->BoundingBox(level, &x1, &y1, &x2, &y2);

                    if (component_texts != 0)
                        component_texts->push_back(string(word));
                    if (component_rects != 0)
                        component_rects->push_back(Rect(x1, y1, x2-x1, y2-y1));
                    if (component_confidences != 0)
                        component_confidences->push_back(conf);
                    delete[] word;
                } while (ri->Next(level));
            }
            delete ri;
        }
        tess.Clear();
#else
        DAVINCI_LOG_WARNING << "OCRTesseract(" << component_level << image.type() <<"): Tesseract not found." << endl;
        output.clear();
        if(component_rects)
            component_rects->clear();
        if(component_texts)
            component_texts->clear();
        if(component_confidences)
            component_confidences->clear();
#endif
    }


    Ptr<OCRTesseract> OCRTesseract::create(const char* datapath, const char* language,
        const char* char_whitelist, int oem, int psmode)
    {
        Ptr<OCRTesseractImpl> pOCR(new OCRTesseractImpl(datapath,language,char_whitelist,oem,psmode) );
        return  pOCR;
    }

}


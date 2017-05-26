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

#include "SceneTextExtractor.hpp"
#include "opencv2/objdetect/erfilter.hpp"
#include "boost/filesystem.hpp"
#include "boost/core/null_deleter.hpp"

using namespace boost;
namespace DaVinci
{
    SceneTextExtractor::SceneTextExtractor()
    {
        erFilter1 = nullptr;
        erFilter1 = nullptr;
        erfilter1File = nullptr;
        erfilter2File = nullptr;
        egGroupFilename = TestManager::Instance().GetDaVinciResourcePath("trained_classifier_erGrouping.xml");
        LoadERFilterClassifier();
        rstParam.lang = RSTLANGUAGE::CHIENG;
        rstParam.recoverLostChar = false;

        rst = boost::shared_ptr<RobustSceneText>(new RobustSceneText(rstParam));
    }

    SceneTextExtractor::~SceneTextExtractor()
    {
        erFilter1.release();
        erFilter2.release();
    }

    void SceneTextExtractor::LoadERFilterClassifier()
    {
        erfilter1File=loadClassifierNM1(TestManager::Instance().GetDaVinciResourcePath("trained_classifierNM1.xml"));
        erfilter2File=loadClassifierNM2(TestManager::Instance().GetDaVinciResourcePath("trained_classifierNM2.xml"));
    }

    vector<Rect> SceneTextExtractor::Extract(const Mat& frame)
    {
        // Extract channels to be processed individually
        this->frame = frame;
        float higharea = 0.01f;
        if(frame.rows*frame.cols<20000)
        {
            higharea=0.9f;
        }
        erFilter1 = createERFilterNM1(erfilter1File, 16, 0.00003f, higharea, 0.0001f, true, 0.0001f);
        erFilter2 = createERFilterNM2(erfilter2File, 0.000001);
        vector<Mat> channels;
        vector<Mat> channels0;
        Mat hsv;
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        vector<Mat> channelsHSV;
        split(hsv, channelsHSV);
        channels.push_back(channelsHSV[2]);
        computeNMChannels(frame, channels0,ERFILTER_NM_IHSGrad);
        channels.push_back(channels0[3]);
        vector<Mat> channelsBGR;
        split(frame, channelsBGR);
        channels.push_back(channelsBGR[1]);
        int cn = (int)channels.size();
        // Append negative channels to detect ER- (bright regions over dark background)
        for (int c = 0; c < cn-1; c++)
        {
            channels.push_back(255-channels[c]);
        }

        vector<vector<ERStat> > regions(channels.size());
        // Apply the default cascade classifier to each independent channel (could be done in parallel)
        DAVINCI_LOG_DEBUG<< "Extracting Class Specific Extremal Regions from " << (int)channels.size() << " channels ...";

#ifdef HAVE_OPENMP
#pragma omp parallel for
#endif
        for (int c = 0; c<(int)channels.size(); c++)
        {
            erFilter1->run(channels[c], regions[c]); // computation intensive
            erFilter2->run(channels[c], regions[c]);
        }

        // Detect character groups
        DAVINCI_LOG_DEBUG << "Grouping extracted ERs ... ";
        // Default group parameters
        erGrouping(channels, regions, egGroupFilename, 0, textRects);

        regions.clear();
        return this->textRects;
    }

    vector<Rect> SceneTextExtractor::ExtractByRST(const Mat& frame)
    {
        vector<Rect> rects;
        rst->DetectText(frame, rects);

        return rects;
    }

    vector<Rect> SceneTextExtractor::GetRecognizedRects()
    {
        return this->textRects;
    }
}
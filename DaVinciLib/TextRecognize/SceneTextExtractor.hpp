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

#ifndef __SCENE_TEXT_EXTRACTOR_HPP__ 
#define __SCENE_TEXT_EXTRACTOR_HPP__

#include "opencv/cv.h"
#include "opencv2/ml/ml.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/objdetect/erfilter.hpp"
#include "TestManager.hpp"
#include "RobustSceneText.hpp"

#include "boost/smart_ptr/shared_ptr.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    /// <summary>
    /// Extract text from scene images
    /// </summary>
    class SceneTextExtractor
    {
    public:
        SceneTextExtractor();
        vector<Rect> Extract(const Mat& frame);
        vector<Rect> ExtractByRST(const Mat& frame);

        vector<Rect> GetRecognizedRects();
        void LoadERFilterClassifier();
        ~SceneTextExtractor();
    private:
        Mat frame;
        vector<Rect> textRects;
        Ptr<ERFilter> erFilter1;
        Ptr<ERFilter> erFilter2;
        Ptr<ERFilter::Callback> erfilter1File;
        Ptr<ERFilter::Callback> erfilter2File;
        string egGroupFilename;

        boost::shared_ptr<RobustSceneText> rst;
        RSTParameter rstParam;
    };
}
#endif
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

#ifndef __DAVINCI_BUTTON_DETECTION_HPP__
#define  __DAVINCI_BUTTON_DETECTION_HPP__
#include "opencv/cv.h"
#include "ObjectRecognizeCaffe.h"
#include "boost/thread/thread.hpp"
namespace DaVinci
{
    using namespace cv;
    using namespace std;
    class ButtonDetection
    {
    public:
        ButtonDetection(const string& netFile, const string& trainedFile,const string& meanfile);
        ~ButtonDetection();
        /// <summary>
        /// Detect this clickable objects in an iamge
        /// </summary>
        /// <param name="src">The source image</param>
        /// <param name="result">to keep the detection result</param>
         /// <param name="bUseGPU">whether use GPU</param>
         /// <param name="topk">how many buttons will be selected, -1 is all </param>
        /// <param name="aspectRatioT">Auxiliary filter conditions,if the rectangle's aspectratio exceeds this value, it will be filter out </param>
        /// <param name="minEdgeT">Auxiliary filter conditions,if the rectangle's minimal edge is samller than this value, it will be filter out </param>
        /// <param name="maxAreaT">Auxiliary filter conditions,if the rectangle's area exceeds this value, it will be filter out </param>
        /// <param name="mserMinArea">same as MSER </param>
        /// <param name="mserMaxArea">same as MSER</param>
        /// <param name="mserMaxVariation">same as MSER</param>
        /// <param name="mserMinDiversity">same as MSER </param>
        /// <returns></returns>
        void Detect(const Mat&src, vector<Rect>& result,bool bUseGPU = false, int topk=20,int aspectRatioT=10,int minEdgeT=10, int maxAreaT=100000, int mserDelta=5, int mserMinArea=500, int mserMaxArea=50000,
            float mserMaxVariation=0.25, float mserMinDiversity=0.2);
        class RectData
        {
        public:
            RectData();
           Rect* pRect;
           float score;
        };
    private:
        static void RemoveOverlapRect(vector<Rect>& rects);
        static double InterUnio(const Rect& rect1,const Rect& rect2 );
        caffe::CaffeNet br;
         static boost::mutex caffeGPUMutex;
    };
    bool compareRectDataRect(const ButtonDetection::RectData& r1, const ButtonDetection::RectData& r2);
}
#endif
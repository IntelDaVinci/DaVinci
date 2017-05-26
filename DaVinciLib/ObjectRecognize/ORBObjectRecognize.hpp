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

#ifndef __DAVINCI_ORBOBJECTRECOGNIZE_HPP__
#define __DAVINCI_ORBOBJECTRECOGNIZE_HPP__
#include "ObjectRecognize.hpp"

namespace DaVinci
{

    class ORBObjectRecognize:public ObjectRecognize
    {
    public:
        // private const int nFeatures = 500;

        ORBObjectRecognize(int nFeatures=500, float scaleFactor=1.2f, int nLevels=8, int edgeThreshold=
            31, int firstLevel=0, int WTA_K=2, int scoreType=0, int patchSize=31);

        /// <summary>
        /// Create an object recognition with scale.
        /// You can optionally pass a smaller version of the object image, the scale means how smaller your image would be.  If your image is half the original size, the scale would be 0.5
        /// Object recognition would return a homography (see Detect() below).  Introducing 'scale' would make sure homography is always based on original images.
        /// </summary>
        /// <param name="objectImage">The image of the object, which would be recognized in each frame</param>
        /// <param name="scale">the scale of the objectImage, compares to the original image</param>
        /// <param name="small">Whether to force a 'small' object recognition.  Small object have different algorithm to increase hit rate.
        ///   Note even if 'small' is false, the small algorithm will still be used if the object image is very small</param>
        /// <param name="prepType">Pre Image Processing Type.</param>
        //SURFObjectRecognize(const cv::Mat& objectImage, float scale=1.0, bool small=false,PreprocessType prepType=Keep);

        virtual ~ORBObjectRecognize();

    private:
        /// <summary>
        /// Get Key points and descriptors
        /// </summary>
        /// <param name="objectImage"></param>
        /// <param name="isSmall"></param>
        /// <param name="keyPoints"></param>
        /// <param name="descriptors"></param>
        virtual void GetKpAndDescs(cv::InputArray grayImage,  bool isSmall, CV_OUT std::vector<cv::KeyPoint>& keypoints, CV_OUT cv::OutputArray  descriptors,FTParameters* otherImageParameters=NULL);
        int nFeatures;
        float scaleFactor;
        int nLevels;
        int edgeThreshold;
        int firstLevel;
        int WTA_K;
        int scoreType;
        int patchSize;

    };
}
#endif
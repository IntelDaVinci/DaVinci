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

#include "ORBObjectRecognize.hpp"
#include "opencv2/core/gpumat.hpp"
#include <vector>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include <opencv2/gpu/gpu.hpp>

using namespace cv;
using namespace gpu;
namespace DaVinci
{

	ORBObjectRecognize::ORBObjectRecognize(int nFeaturesP, float scaleFactorP, int nLevelsP, int edgeThresholdP,int firstLevelP, int WTA_KP, int scoreTypeP, int patchSizeP):
		nFeatures(nFeaturesP),scaleFactor(scaleFactorP),nLevels(nLevelsP),edgeThreshold(edgeThresholdP),firstLevel(firstLevelP),
		WTA_K(WTA_KP),scoreType(scoreTypeP),patchSize(patchSizeP),ObjectRecognize()
	{
	}
	ORBObjectRecognize::~ORBObjectRecognize()
	{
	}

	//SURFObjectRecognize::SURFObjectRecognize(const Mat& objectImage, float scale, bool small,PreprocessType prepType):ObjectRecognize(objectImage,scale,small,prepType)
	//{
	//	//PreprocessAndGetKpDescs(objectImage,ObjectRecognize::isSmall,prepType,keyPoints,descriptors,scale);
	//}


	void ORBObjectRecognize::GetKpAndDescs(InputArray img, bool isSmall, CV_OUT vector<KeyPoint>& keypoints, CV_OUT OutputArray  descriptors,FTParameters* otherImageParameters)
	{
        Mat grayImage;
        if(img.channels()==3)
        {
            cvtColor(img,grayImage,CV_BGR2GRAY);
        }
        else
        {
            grayImage = img.getMat().clone();
        }

        if(otherImageParameters!=NULL)
        {
            otherImageParameters->width=grayImage.cols;
            otherImageParameters->height=grayImage.rows;
        }
        else
        {
            this->ftp->width=grayImage.cols;
            this->ftp->height=grayImage.rows;
        }
        bool useCPU = true;// we only support several gpu due to memory issue
        // For GPU mode, OpenCV requires the image is big enough. The detailed size
        // is implementation-specific. Here we assume both width and height should be
        // at least 200 and area>100000.
        if(gpu::getCudaEnabledDeviceCount()>0 && grayImage.rows>200&&grayImage.cols>200&&grayImage.cols*grayImage.rows >= 100000)
        {
            try
            {
                useCPU= false;
                boost::lock_guard<boost::mutex> lock(TestManager::gpuLock);
                ORB_GPU orb(	 nFeatures,scaleFactor,nLevels,edgeThreshold,firstLevel,WTA_K,scoreType, patchSize); 
                GpuMat keypointsGPU;
                GpuMat descriptorsGPU;
                GpuMat grayImageGPU(grayImage);
                orb(grayImageGPU, GpuMat(), keypointsGPU, descriptorsGPU); 
                if(keypointsGPU.empty())
                    return;
                orb.downloadKeyPoints(keypointsGPU,keypoints);
                descriptorsGPU.download(descriptors.getMatRef());
            }
            catch(...)
            {
                useCPU = true;
            }
        }
        if(useCPU)
        {
            ORB orb(nFeatures,scaleFactor,nLevels,edgeThreshold,firstLevel,WTA_K,scoreType, patchSize); 
            orb.detect( grayImage, keypoints );
            orb.compute(grayImage,keypoints,descriptors.getMatRef());//must getMat ref
        }

	}

}
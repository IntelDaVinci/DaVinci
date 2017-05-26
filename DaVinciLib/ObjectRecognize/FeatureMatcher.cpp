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

#include "FeatureMatcher.hpp"
#include "DaVinciDefs.hpp"
#include "boost/lexical_cast.hpp"
#include "AKazeObjectRecognize.hpp"
using namespace cv;

namespace DaVinci
{
    FeatureMatcher::FeatureMatcher(boost::shared_ptr<ObjectRecognize> pObjectRec)
    {
        pmObjectRec=pObjectRec;
        pmObjectRec->unmatchRatioType = UnmatchRatioType::UnmatchOverWholeFeature; 
    }

    FeatureMatcher::FeatureMatcher()
    {
        boost::shared_ptr<AKazeObjectRecognize>  pObjRec (new AKazeObjectRecognize());
        pmObjectRec=pObjRec;
        pmObjectRec->unmatchRatioType = UnmatchRatioType::UnmatchOverWholeFeature; 
    }

    void FeatureMatcher::SetModelImage(cv::InputArray modelImage,bool isSmall,float scale ,PreprocessType prepType,FTParameters* otherImageParameters)
    {
        assert(pmObjectRec!=NULL);
        if(modelImage.empty())
        {
            DAVINCI_LOG_DEBUG<<"Empty Model Image Input";
            return;
        }
        pmObjectRec->Init(modelImage,isSmall,scale,prepType,otherImageParameters);
    }

    FeatureMatcher::~FeatureMatcher()
    {
    }

    cv::Mat FeatureMatcher::Match(const FeatureMatcher& featureMather, double altRatio, double* pUnmatchRatio,vector<cv::Point2f>* pPointArray)
    {

        return pmObjectRec->Match(featureMather.GetKeyPoints(),featureMather.GetDescriptors(),featureMather.GetSize(),altRatio,pUnmatchRatio,pPointArray);
    }

    Mat FeatureMatcher::Match(InputArray observedImage, float scale, double altRatio, PreprocessType observedPrepType, double* pUnmatchRatio,vector<Point2f>* pPointArray)const
    {
        if(observedImage.empty())
        {
            if(pUnmatchRatio)
            {
                *pUnmatchRatio=1.0;//in case caller use this value to determine match 
            }
            DAVINCI_LOG_DEBUG<<"Empty Observed Image Input";
            return Mat();
        }
        return pmObjectRec->Match( observedImage, scale, altRatio, observedPrepType, pUnmatchRatio, pPointArray);
    }

    Mat FeatureMatcher:: Match(InputArray observedImage,double* pUnmatchRatio)const 
    {
        if(observedImage.empty())
        {
            if(pUnmatchRatio)
            {
                *pUnmatchRatio=1.0;//in case caller use this value to determine match
            }
            DAVINCI_LOG_DEBUG<<"Empty Observed Image Input";
            return Mat();
        }
        return pmObjectRec->Match( observedImage,pUnmatchRatio);
    }

    bool FeatureMatcher::IsSmall()const
    {
        return pmObjectRec->isSmall;
    }

    Size FeatureMatcher::GetImgSize()const
    {
        return pmObjectRec->imageSize;
    }

    std::vector<cv::Point2f> FeatureMatcher::GetPts()const
    {
        return pmObjectRec->pts;
    }

    //void FeatureMatcher::DumpKeyPointsNumber()const
    //{
    //    DAVINCI_LOG_DEBUG<<("object contains " + boost::lexical_cast<string>((int)( pmObjectRec->keyPoints.size()) )+ " feature points");
    //}

    Rect FeatureMatcher::GetBoundingRect()const
    {
        return boundingRect(pmObjectRec->pts);
    }

    double FeatureMatcher::GetRotationAngle()const
    {
        Mat H = pmObjectRec->homography;
        Mat warpInv;
        warpInv=  (Mat_<double>(3,3) << 1.0, 0.0, 0.0, 0.0, 1.0, -0.0, -H.at<double>(2,0), -H.at<double>(2,1), -H.at<double>(2,2));
        Mat m = H*warpInv;
        double scalex = sqrt(m.at<double>(0, 0) * m.at<double>(0, 0) + m.at<double>(0, 1) * m.at<double>(0, 1));
        double cos = m.at<double>(0, 0) / scalex;
        double sin = m.at<double>(0, 1) / scalex;
        double angle = acos(cos) * 180 / CV_PI;
        if (sin < 0)
        {
            angle = 360 - angle;
        }
        return angle;
    }

    vector<KeyPoint> FeatureMatcher::GetKeyPoints()const
    {
        return pmObjectRec->keyPoints;
    }

    Point2f FeatureMatcher::GetObjectCenter()const
    {
        return pmObjectRec->pts[4];
    }

    cv::Mat FeatureMatcher::GetDescriptors()const
    {
        return pmObjectRec->descriptors;
    }

    int  FeatureMatcher::GetSize() const
    {
        return pmObjectRec->size;
    }  

    boost::shared_ptr<FTParameters> FeatureMatcher::GetFTParameters() const
    {
        return pmObjectRec->ftp;
    }

    boost::shared_ptr<FTParameters> FeatureMatcher::GetDefaultCurrentMatchPara() 
    {
        return pmObjectRec->GetDefaultCurrentMatchPara();
    }

    void FeatureMatcher::SetUnmatchRatioType(UnmatchRatioType type)
    {
        pmObjectRec->SetUnmatchRatioType(type);
    }
}


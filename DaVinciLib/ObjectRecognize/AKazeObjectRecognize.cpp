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
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "AKazeObjectRecognize.hpp"
using namespace cv;

namespace DaVinci
{
    AKazeObjectRecognize::AKazeObjectRecognize(int descriptoTypeP,int descriptorSizeP,int descriptorChannelsP, float thresholdP,
        int octavesP,int sublevelsP,int diffusivityP):
    descriptoType(descriptoTypeP),descriptorSize(descriptorSizeP),descriptorChannels(descriptorChannelsP),threshold(thresholdP),octaves(octavesP),
        sublevels(sublevelsP),diffusivity(diffusivityP),fillSize(100),ObjectRecognize()
    {
    }
    AKazeObjectRecognize::~AKazeObjectRecognize()
    {
    }

    void AKazeObjectRecognize::GetKpAndDescs(InputArray img, bool isSmall, CV_OUT vector<KeyPoint>& keypoints, CV_OUT OutputArray  descriptors,FTParameters* param)
    {
        Mat grayImage;
        float scale=1.0;
        float threshold = this->threshold;

        if(param!=NULL)
        {
            threshold = param->p1;
            scale = param->p2;
        }
        else
        {
            this->ftp->p1=threshold;
        }

        if(img.channels()==3)
        {
            cvtColor(img,grayImage,CV_BGR2GRAY);
        }
        else
        {
            grayImage = img.getMat().clone();
        }


        if(grayImage.rows<fillSize||grayImage.cols<fillSize)//for small Image
        {
            grayImage = FillImage(grayImage,Size(fillSize,fillSize));
        }

        //for big image
        if(param==NULL)
        {
            int size = (grayImage.cols+grayImage.rows)/2;
            if(size>800)
                scale=0.7f;
            if(grayImage.cols*scale>1024)
            {
                scale=1024.0f/grayImage.cols;
            }
            this->ftp->p2= scale;
        }

        if(abs(scale-1.0)>0.00001)
        {
            resize(grayImage,grayImage,Size(),scale,scale,CV_INTER_CUBIC);//need check;
        }
        if(param==NULL)
        {
            this->ftp->width=grayImage.cols;
            this->ftp->height=grayImage.rows;
        }
        else
        {
            param->width=grayImage.cols;
            param->height=grayImage.rows;
        }
        AKAZE akaze( descriptoType, descriptorSize, descriptorChannels,threshold,octaves, sublevels, diffusivity);
        try{
            akaze(grayImage,noArray(),keypoints,descriptors);
            if(param==NULL&&keypoints.size()<20&&abs(threshold-0.001)<0.00001)//auto lower down threshold and reCalcu
            {
                this->ftp->p1=threshold=0.00001f;
                AKAZE akaze( descriptoType, descriptorSize, descriptorChannels,threshold,octaves, sublevels, diffusivity);
                akaze(grayImage,noArray(),keypoints,descriptors);
            }
        }
        catch(Exception e)
        {
            DAVINCI_LOG_ERROR<<e.msg;
        }
        if(abs(scale-1.0)>0.00001)
        {
            int size = (int)(keypoints.size());
            for(int i=0;i<size;++i)
            {
                keypoints[i].pt.x/=scale;
                keypoints[i].pt.y/=scale;
            }
        }
    }

    DistanceType AKazeObjectRecognize::SetDistanceType()
    {
        if(descriptoType==DESCRIPTOR_MLDB||descriptoType==DESCRIPTOR_MLDB_UPRIGHT)
            return DistanceType::Hamming2DT;
        else
            return DistanceType::L2DT;
    }
    //assume uchar type 
    Mat AKazeObjectRecognize::FillImage(const Mat& srcImage,Size size)
    {
        if(size.width<srcImage.cols)
        {
            size.width=srcImage.cols;
        }
        if(size.height<srcImage.rows)
        {
            size.height= srcImage.rows;
        }
        int widthGap = (size.width-srcImage.cols)/2;
        int heightGap = (size.height-srcImage.rows)/2;
        Mat result;
        if(widthGap>0)
        {
            Mat LeftImage(srcImage.rows,widthGap,srcImage.type());
            Mat RightImage(srcImage.rows,widthGap,srcImage.type());
            for(int i=0;i<srcImage.rows;++i)
            {
                const uchar* srcPtr = srcImage.ptr<uchar>(i);
                uchar* leftPtr = LeftImage.ptr<uchar>(i);
                uchar* rightPtr= LeftImage.ptr<uchar>(i);

                for(int j=0;j<widthGap;++j)
                {
                    for(int c=0;c<srcImage.channels();++c)
                    {
                        *(leftPtr+j*srcImage.channels()+c)=*(srcPtr+c);
                        *(rightPtr+j*srcImage.channels()+c)=*(srcPtr+(srcImage.cols-1)*srcImage.channels()+c);
                    }
                }
            }       
            /*  for(int i=0;i<srcImage.rows;++i)
            {
            const uchar* srcPtr = srcImage.ptr<uchar>(i);
            uchar* desPtr = LeftImage.ptr<uchar>(i);
            for(int j=0;j<widthGap;++j)
            {

            for(int c=0;c<srcImage.channels;++c)
            {
            *(desPtr+j*srcImage.channels+c)=*(srcPtr+(srcImage.cols-1)*srcImage.channels+c);
            }
            }
            }*/
            Mat tempH;
            hconcat(LeftImage,srcImage,tempH);
            hconcat(tempH,RightImage,result);
        }
        else
        {
            result = srcImage.clone();
        }
        if(heightGap>0)
        {
            Mat bottomImage(heightGap,result.cols,srcImage.type());
            Mat topImage(heightGap,result.cols,srcImage.type());
            const uchar* resultBottom= result.ptr<uchar>(result.rows-1);
            const uchar* resultTop=result.ptr<uchar>(0);
            for(int i=0;i<bottomImage.rows;++i)
            {

                uchar* bottomPtr = bottomImage.ptr<uchar>(i);
                uchar* topPtr = bottomImage.ptr<uchar>(i);
                for(int j=0;j<bottomImage.cols;++j)
                    for(int c=0;c<srcImage.channels();++c)
                    {
                        *(bottomPtr+j*srcImage.channels()+c)=*(resultBottom+j*srcImage.channels()+c);
                        *(topPtr+j*srcImage.channels()+c)=*(resultTop+j*srcImage.channels()+c);
                    }

            }
            Mat tempV;
            vconcat(topImage,result,tempV);
            vconcat(tempV,bottomImage,result);   
        }
        return result;
    }

}
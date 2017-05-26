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

#include "ButtonDetection.hpp"
//#include "opencv2/highgui/highgui.hpp"

namespace DaVinci
{

    double ButtonDetection:: InterUnio(const Rect& rect1,const Rect& rect2 )
    {
        int bi[4];
        bi[0] = max(rect1.x,rect2.x);
        bi[1] = max(rect1.y, rect2.y);
        bi[2] = min(rect1.x+rect1.width, rect2.x+rect2.width);
        bi[3] = min(rect1.y+rect1.height, rect2.y+rect2.height);

        double iw = bi[2] - bi[0] + 1;
        double ih = bi[3] - bi[1] + 1;
        double ov = 0;
        if (iw>0 && ih>0){
            double maxArea= max(rect1.area(),rect2.area());
            ov = iw*ih/maxArea;
        }
        return ov;
    }

    void ButtonDetection::RemoveOverlapRect(vector<Rect>& rect)
    {
        if(rect.size()<=0)
            return;
        const double unioThreshold = 0.5;
        vector<bool> flag(rect.size());
        for(size_t i = 0;i<flag.size();++i)
        {
            flag[i] = false;
        }
        for(size_t i=0;i<rect.size()-1;++i)
        {
            if(flag[i]==true)
                continue;
            for(size_t j=i+1;j<rect.size();++j)
            {
                if(InterUnio(rect[i],rect[j])>unioThreshold)
                {
                    flag[j] = true;
                }
            }
        }
        int index = 0;
        for(size_t i =0;i<rect.size();++i)
        {
            if(flag[i]==true)
            {
                continue;
            }
            else
            {
                rect[index] = rect[i];
                ++index;
            }
        }
        rect.resize(index);
    }

    ButtonDetection::RectData::RectData()
    {
        pRect = NULL;
        score = 0.0;
    }

    boost::mutex ButtonDetection::caffeGPUMutex;

    void ButtonDetection::Detect(const Mat&src, vector<Rect>& result,bool useGPU,int topk,int aspectRatioT,int minEdgeT, int maxAreaT, int delta, int minArea, int maxArea,
        float maxVariation, float minDiversity)
    {
        assert(3==src.channels());
        result.clear();
        Mat gray;
        cvtColor(src,gray,CV_BGR2GRAY);
        vector<vector<Point> > regions;
        MSER mser(delta,minArea,maxArea,maxVariation,minDiversity);
        mser(gray, regions);
        vector<Rect> proposals;
        proposals.resize(regions.size());
       /* Mat drawImage = src.clone();
        vector<Rect> rects;
        for (size_t i = 0; i<regions.size();++i)
        {
            Rect rect = boundingRect(regions[i]);
            rects.push_back(rect);
            rectangle(drawImage,rect,Scalar(51,249,40));
        }
        cv::imwrite("DebugButtonMser.png",drawImage);*/
        int proposalsRealSize = 0;

        vector<Mat> data(regions.size());
        for(size_t i=0;i<regions.size();++i)
        {
            Rect rect= boundingRect(regions[i]);
            if((rect.width/rect.height>aspectRatioT) ||(rect.width/rect.height>aspectRatioT) || rect.width < minEdgeT ||rect.height<minEdgeT || rect.area()>maxAreaT )
                continue;
            int xPad = (int)std::max(5.0,rect.width*0.05);
            int yPad = (int)std::max(5.0,rect.height*0.05);
            int xmin = std::max(0,rect.x-xPad);
            int ymin = std::max(0,rect.y-yPad);
            int xmax = std::min(gray.cols,rect.x+rect.width+xPad);
            int ymax = std::min(gray.rows,rect.y+rect.height+yPad);
            proposals[proposalsRealSize]=Rect(xmin,ymin,xmax-xmin,ymax-ymin);
            Mat temp = src(proposals[proposalsRealSize]);
            resize(temp,data[proposalsRealSize],Size(32,32));
            ++proposalsRealSize;
        }
        if(proposalsRealSize==0)
            return;
        proposals.resize(proposalsRealSize);
        data.resize(proposalsRealSize);
        vector<vector<float>> predictResult;
        //Since Caffe shared only one GPU Status
        boost::lock_guard<boost::mutex> lock(caffeGPUMutex);
        br.SetMode(useGPU);
        br.Predict(data,predictResult);
        vector<RectData> sortData(proposals.size());
        int index = 0;
        for(size_t i =0;i<proposals.size();++i)
        {

            if(predictResult[i][1]-predictResult[i][0]>0.01)
            {
                sortData[index].pRect = &proposals[i];
                sortData[index].score = predictResult[i][1]-predictResult[i][0]+1.0f;
                result.push_back(*(sortData[index].pRect));
                ++index;
            }
        }
        sortData.resize(index);
        if(topk==-1||result.size()<(size_t)topk)
        {
            RemoveOverlapRect(result);
            return;
        }
        sort(sortData.begin(),sortData.end(),compareRectDataRect);

        result.clear();
        for(size_t i=0;i<(size_t)topk*2 && i<sortData.size();++i)
        {
            result.push_back(*sortData[i].pRect);
        }

        RemoveOverlapRect(result);
        if(result.size()>(size_t)topk)
        {
            result.resize(topk);
        }
    }

    ButtonDetection::ButtonDetection(const string& netFile, const string& trainedFile, const string& meanfile):br(netFile.c_str(),trainedFile.c_str(),meanfile.c_str(), 100)
    {

    }

    ButtonDetection::~ButtonDetection()
    {
    }

    bool compareRectDataRect(const ButtonDetection::RectData& r1, const ButtonDetection::RectData& r2)
    {
        if(r1.pRect->area()>r2.pRect->area())
        {
            return true;
        }
        else
        {
            return false;
        }

    }
}
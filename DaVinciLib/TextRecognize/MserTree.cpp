///*
// ----------------------------------------------------------------------------------------
//
// "Copyright 2014-2015 Intel Corporation.
//
// The source code, information and material ("Material") contained herein is owned by Intel Corporation
// or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
// or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
// The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
// be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
// in any way without Intel's prior express written permission. No license under any patent, copyright or 
// other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
// by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
// must be express and approved by Intel in writing.
//
// Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
// embedded in Materials by Intel or Intel's suppliers or licensors in any way."
// -----------------------------------------------------------------------------------------


#include "MSER.hpp"
#include "opencv\highgui.h"
#include <memory>
#include "MserTree.hpp"
namespace DaVinci
{
    using namespace std;
    using namespace cv;
    MSERTree::MSERTree( int delta_, int minArea_, int maxArea_,
        double maxVariation_, double minDiversity_) : delta( delta_ ), minArea( minArea_ ), maxArea( maxArea_),
        maxVariation( maxVariation_ ), minDiversity( minDiversity_)
    {
        blackStorage= cv::Ptr<CvMemStorage>(cvCreateMemStorage(0));
        blackContours = NULL;

        whiteStorage= cv::Ptr<CvMemStorage>(cvCreateMemStorage(0));
        whiteContours = NULL;
    }

    //MSERTree::MSERTree(CvMSERParams param)
    //{
    //    delta = param.delta;
    //    minArea =param.minArea ;
    //    maxArea = param.maxArea;
    //    maxVariation = param.maxVariation;
    //    minDiversity = param.minDiversity;

    //    blackStorage= cv::Ptr<CvMemStorage>(cvCreateMemStorage(0));
    //    blackContours = NULL;

    //    whiteStorage= cv::Ptr<CvMemStorage>(cvCreateMemStorage(0));
    //    whiteContours = NULL;
    //}

    //void MSERTree::operator() ( const Mat& img, vector<vector<Point>>& pts, vector<Rect>& rects)
    //{
    //    vector<CvContour*> blackMsers, whiteMsers;
    //    (*this)( img,blackMsers, whiteMsers );
    //    pts.resize( blackMsers.size() + whiteMsers.size() );
    //    rects.resize( blackMsers.size() + whiteMsers.size() );
    //    for ( size_t i = 0; i < blackMsers.size(); ++i)
    //    {
    //        for( int j = 0; j < blackMsers[i]->total; ++j )
    //        {
    //            CvPoint *pt = CV_GET_SEQ_ELEM(CvPoint, blackMsers[i], j);
    //            pts[i].push_back(*pt);
    //        }
    //    }

    //    for ( size_t i = 0; i < whiteMsers.size(); ++i)
    //    {
    //        for( int j = 0; j < whiteMsers[i]->total; ++j )
    //        {
    //            CvPoint *pt = CV_GET_SEQ_ELEM(CvPoint, whiteMsers[i], j);
    //            pts[i+blackMsers.size()].push_back(*pt);
    //        }
    //    }

    //    for ( size_t i = 0; i < pts.size(); ++i )
    //    {
    //        Rect rect = boundingRect(pts[i]);
    //        rects[i] = rect;
    //    }
    //}

    void MSERTree::operator()(const Mat& img, vector<CvContour*>& blackMsers, vector<CvContour*>& whiteMsers)
    {
        Mat gray;
        if ( 3 == img.channels() )
        {
            cvtColor( img, gray, CV_BGR2GRAY);
        }
        else 
        {
            gray = img.clone();
        }

        CvMSERParams param = DaVinci::cvMSERParams( delta, minArea, maxArea, (float)maxVariation, (float)minDiversity);
        //MSER- 
        IplImage grayIpl = (IplImage)gray;
        DaVinci::cvExtractMSER(&grayIpl, NULL, &blackContours, blackStorage, param,-1); 
        //MSER+
        gray = 255-gray;
        grayIpl = (IplImage)gray;
        cvExtractMSER(&grayIpl, NULL, &whiteContours, whiteStorage, param,1); 

        for (int i = 0; i < blackContours->total; ++i) 
        {
            CvSeq *seq = *CV_GET_SEQ_ELEM(CvSeq *, blackContours, i);
            blackMsers.push_back((CvContour*)seq);

        }

        for (int i = 0; i < whiteContours->total; ++i) 
        {
            CvSeq *seq = *CV_GET_SEQ_ELEM(CvSeq *, whiteContours, i);
            whiteMsers.push_back((CvContour*)seq);
        }

        //set the rects
        for ( size_t i = 0; i < blackMsers.size(); ++i)
        {
            vector<Point> pts;
            for( int j = 0; j < blackMsers[i]->total; ++j )
            {

                CvPoint *pt = CV_GET_SEQ_ELEM(CvPoint, blackMsers[i], j);
                pts.push_back(*pt);
            }

            Rect rect = boundingRect(pts);
            ((CvContour*)(blackMsers[i]))->rect = rect;
        }

        for ( size_t i = 0; i < whiteMsers.size(); ++i)
        {
            vector<Point> pts;
            for( int j = 0; j < whiteMsers[i]->total; ++j )
            {

                CvPoint *pt = CV_GET_SEQ_ELEM(CvPoint, whiteMsers[i], j);
                pts.push_back(*pt);
            }

            Rect rect = boundingRect(pts);
            ((CvContour*)(whiteMsers[i]))->rect = rect;
        }

    }


}
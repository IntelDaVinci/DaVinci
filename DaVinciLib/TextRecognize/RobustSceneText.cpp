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

#include "RobustSceneText.hpp"
#include <queue>
#include <stack>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv/cv.h"
#include "opencv2/ml/ml.hpp"
#include <unordered_map>
#include "Binarization.hpp"


using namespace cv;
using namespace std;
using namespace caffe;
#ifdef DaVinciRST
#include "TestManager.hpp"
#endif

#ifdef SAVEIMAGE
static Mat srcColorImage = Mat();
#endif
namespace DaVinci
{
#ifdef DaVinciRST
    using boost::shared_ptr;
    boost::mutex RobustSceneText::classifierLock;
#endif

    static bool  GuoHallThinning(const Mat1b& img, Mat& skeleton);
    static void GetGradientMagnitude(Mat& grayImage, Mat& gradientMag);
#ifdef _DEBUG
    //draw function, only for debug
    //static void DrawERRecursiveHelper(const Mat& image, const string& saveName, const vector<CvContour*>& subroot);
    //draw function, only for debug
    static void DrawERHelper(const Mat& img, const string& saveName, const vector<CvContour*>& ers);
#endif


    void RobustSceneText::DetectText(const Mat& image, vector<Rect>& textRects, vector<vector<CvContour*>>& textlines)
    {
        Mat gray;
        if (1 == image.channels())
        {
            gray = image;
        }
        else
        {
            cvtColor(image, gray, CV_BGR2GRAY);
        }
        tempParam.maxArea = (int)(param.scale * param.scale * param.maxArea * image.rows * image.cols / (1280 * 720));
        tempParam.maxArea = max(2000, param.maxArea);
        tempParam.minArea = (int)(param.scale * param.scale * param.minArea * image.rows * image.cols / (1280 * 720));
        tempParam.minArea = max(5, param.minArea);

        /* ------- use opencv mser function ------ */
        //#ifdef _DEBUG		
        //		MSER mser(tempParam.delta, tempParam.minArea, tempParam.maxArea, tempParam.maxVariation, tempParam.minDiversity);
        //		vector<vector<Point> > pts;
        //		mser(gray, pts);
        //		Mat drawImage = image.clone();
        //		for (size_t j = 0; j < pts.size(); ++j)
        //		{
        //			Rect rect = boundingRect(pts[j]);
        //			rectangle(drawImage, rect, Scalar(51, 249, 40), 2);
        //			imwrite("debugMser.png", drawImage);
        //		}
        //#endif

        /* ------- use our own mser function ------ */
        vector<CvContour*> charCandidates, realChars;
        if (abs(tempParam.scale - 1.0) < 0.00001)
        {
            DetectCharacter(image, charCandidates, realChars);
        }
        else
        {
            Mat resizeImage;
            resize(gray, resizeImage, Size(), tempParam.scale, tempParam.scale);
            DetectCharacter(resizeImage, charCandidates, realChars);
        }
        vector<MSERFeature> realCharFeatures;
        ExtractFeatures(image, realChars, realCharFeatures);
        vector<vector<int>> clusterLabels;
        textRects = SingleLinkageCluster(realCharFeatures, realChars, textlines, clusterLabels);

        if (tempParam.recoverLostChar)
        {
            RecoverLostCharacter(image, charCandidates, realCharFeatures, clusterLabels, textRects, textlines);
        }

        PostProcess(textRects, image.rows, image.cols);
        RemoveUselessRect(textRects, textlines);

        SortRects(textRects, textlines);

        /*---use Beyes method to filtering non-text lines---*/
        //FilterTextLineByBeyesian(image,charCandidates,textRects,textlines);
    }

    void RobustSceneText::DetectText(const Mat& image, vector<Rect>&textRects)
    {
#ifdef SAVEIMAGE
        srcColorImage = image;
#endif
        vector<vector<CvContour*>> textlines;
        DetectText(image, textRects, textlines);
    }



    void RobustSceneText::DetectTextWithDetail(const Mat& img, vector<Rect>& textRects, vector<Mat>& binary_line)
    {
#ifdef SAVEIMAGE
        srcColorImage = image;
#endif
        textRects.clear();
        binary_line.clear();
        vector<vector<CvContour*>> textlines;
        DetectText(img, textRects, textlines);

        /*-----text regions binarization for text recognition----*/
        /*-----choose one of the three following methods for binarization----*/
        Binarization binary_obj; 
        binary_obj.ImgBinarization(img, textlines, binary_line, tempParam.binaryType);
        //binary_obj.ImgBinarization(img, textlines, binary_line, KMEANS_COLOR_SSP);
        //binary_obj.ImgBinarization(img, textlines, binary_line, OTSU);

    }


    void RobustSceneText::RecoverLostCharacter(const Mat& image, const vector<CvContour*>& blackDisconnectRegion, const vector<MSERFeature>& blackFeautures,
        const vector<vector<int>>& blackClusterLabels, vector<Rect>& blackRects, vector<vector<CvContour*>>&blackClusters)
    {
        //	MSERFeatureFunc featureFunctor;
        float padScale = 2.2f;
        double unionThreshold = 0.8;
        //process black
        vector<Rect> enlargeBlackRects(blackRects.size());
        for (size_t i = 0; i < blackRects.size(); ++i)
        {
            int padwidth = int(blackRects[i].height * padScale);
            enlargeBlackRects[i].x = blackRects[i].x - padwidth;
            enlargeBlackRects[i].y = blackRects[i].y;
            enlargeBlackRects[i].width = blackRects[i].width + 2 * padwidth;
            enlargeBlackRects[i].height = blackRects[i].height;
        }
        for (size_t i = 0; i<blackDisconnectRegion.size(); ++i)
        {
            shared_ptr<MSERFeature> feat;
            if (blackDisconnectRegion[i]->reserved[2] > tempParam.charHighProb || blackDisconnectRegion[i]->reserved[2] < tempParam.charLowProb)
            {
                continue;
            }
            //search which enlargerect will contain this mser
            for (size_t j = 0; j<enlargeBlackRects.size(); ++j)
            {
                if (blackDisconnectRegion[i]->reserved[2] > tempParam.charHighProb)
                {
                    break;
                }
                //int x = blackDisconnectRegion[i]->rect.x;
                //int y = blackDisconnectRegion[i]->rect.y;
                double unionValue = TextInterUnio(enlargeBlackRects[j], blackDisconnectRegion[i]->rect);
                if (unionValue > unionThreshold)
                {
                    //extract erfeature
                    if (feat.get() == NULL)
                    {
                        vector<CvContour*> regions;
                        regions.push_back(blackDisconnectRegion[i]);
                        vector<MSERFeature> mserFeatures;
                        ExtractFeatures(image, regions, mserFeatures);
                        feat = shared_ptr<MSERFeature>(new MSERFeature(mserFeatures[0]));
                    }
                    int clustersSize = (int)(blackClusterLabels[j].size());
                    for (int k = 0; k < clustersSize; ++k)
                    {
                        int featureIndex = blackClusterLabels[j][k];
                        if (MSERFeatureFuncRecover()(blackFeautures[featureIndex], *feat))
                        {
                            blackDisconnectRegion[i]->reserved[2] = tempParam.charHighProb + 1;
                            blackClusters[j].push_back(blackDisconnectRegion[i]);
                            int xTL = std::min(blackRects[j].x, blackDisconnectRegion[i]->rect.x);
                            int yTL = std::min(blackRects[j].y, blackDisconnectRegion[i]->rect.y);
                            int xBR = std::max(blackRects[j].x + blackRects[j].width,
                                blackDisconnectRegion[i]->rect.x + blackDisconnectRegion[i]->rect.width);
                            int yBR = std::max(blackRects[j].y + blackRects[j].height,
                                blackDisconnectRegion[i]->rect.y + blackDisconnectRegion[i]->rect.height);
                            blackRects[j] = Rect(Point(xTL, yTL), Point(xBR, yBR));
                            break;
                        }
                    }//end for k  
                }
            }
        }
    }

    void RobustSceneText::DetectCharacter(const Mat& src, vector<CvContour*>& charCandidates, vector<CvContour*>& realChars)
    {
        realChars.clear();
        ProposeTextCandidate(src, charCandidates);
        Mat image;
        if (src.channels() == 3)
        {
            cvtColor(src, image, CV_BGR2GRAY);
        }
        else
        {
            image = src;
        }
        vector<Rect> rects;
        rects.reserve(charCandidates.size());

        for (size_t i = 0; i < charCandidates.size(); ++i)
        {
            rects.push_back(charCandidates[i]->rect);
        }

        vector<float> probOfChar = EliminateFalseCharacter(image, rects);

        for (size_t i = 0; i < rects.size(); ++i)
        {
            charCandidates[i]->reserved[2] = int(probOfChar[i] * 100);
            if ((charCandidates[i]->reserved[2] < tempParam.charHighProb) || (PreFilterMSER(rects[i], tempParam.preFilterMSERMaxHeight, tempParam.preFilterMSERMinHeight, tempParam.preFilterMSERMaxWidth, tempParam.preFilterMSERMinWidth, tempParam.preFilterMSERMaxArea)))
                continue;
            realChars.push_back(charCandidates[i]);
        }

#ifdef _DEBUG
        DrawERHelper(image, "DebugWholeCharAfterClassfier.jpg", realChars);
#endif
    }

    void RobustSceneText::ProposeTextCandidate(const Mat& src, vector<CvContour*>& charCandidates)
    {
        charCandidates.clear();
        Mat gray;
        if (src.channels() == 3)
        {
            cvtColor(src, gray, CV_BGR2GRAY);
        }
        else
        {
            gray = src.clone();
        }
        vector<CvContour*> blackMser, whiteMser;
        vector<CvContour*>whiteCharCandidates, blackCharCandidates;
        msertree = MSERTree(tempParam.delta, tempParam.minArea, tempParam.maxArea, tempParam.maxVariation, tempParam.minDiversity);
        if (tempParam.useGradientChannel)
        {
            Mat gradientMagnitude = Mat_<float>(gray.size());
            GetGradientMagnitude(gray, gradientMagnitude);
            gradientMagnitude.convertTo(gradientMagnitude, CV_8UC1);
            msertree(gradientMagnitude, whiteMser, blackMser);
            PruneTree(blackMser, charCandidates, true);
            PruneTree(whiteMser, whiteCharCandidates, false);
            charCandidates.insert(charCandidates.end(), whiteCharCandidates.begin(), whiteCharCandidates.end());
            RemoveUselessChars(charCandidates);
        }
        else
        {
            // if(src.channels()==1)
            // {
            msertree(gray, whiteMser, blackMser);
            PruneTree(blackMser,charCandidates,true);
            PruneTree(whiteMser,whiteCharCandidates,false);
            charCandidates.insert(charCandidates.end(),whiteCharCandidates.begin(),whiteCharCandidates.end());
            // }
            /*  else
            {

            vector<Mat> bgrs;
            split(src, bgrs);
            for(int i=0;i<3;++i)
            { 
            Mat c = bgrs[i];
            vector<CvContour*> blackMser, whiteMser;
            vector<CvContour*>blackCharCandidates,whiteCharCandidates;
            msertree(c, whiteMser, blackMser);
            PruneTree(blackMser,blackCharCandidates,true);
            PruneTree(whiteMser,whiteCharCandidates,false);
            charCandidates.insert(charCandidates.end(),whiteCharCandidates.begin(),whiteCharCandidates.end());
            charCandidates.insert(charCandidates.end(),blackCharCandidates.begin(),blackCharCandidates.end());
            }*/
            //   }
            RemoveUselessChars(charCandidates);

        }
        // if(src.channels()==1)
        // {
        //   else
        //  {
        //whiteMser.clear();
        //blackMser.clear();
        //whiteCharCandidates.clear();
        //msertree(gray, whiteMser, blackMser);
        //PruneTree(blackMser,blackCharCandidates,true);
        //PruneTree(whiteMser,whiteCharCandidates,false);
        //charCandidates.insert(charCandidates.end(),whiteCharCandidates.begin(),whiteCharCandidates.end());
        //charCandidates.insert(charCandidates.end(),blackCharCandidates.begin(),blackCharCandidates.end());
        //  }
        // }
        /*  else
        {

        vector<Mat> bgrs;
        split(src, bgrs);
        for(int i=0;i<3;++i)
        {
        Mat c = bgrs[i];
        vector<CvContour*> blackMser, whiteMser;
        vector<CvContour*>blackCharCandidates,whiteCharCandidates;
        msertree(c, whiteMser, blackMser);
        PruneTree(blackMser,blackCharCandidates,true);
        PruneTree(whiteMser,whiteCharCandidates,false);
        charCandidates.insert(charCandidates.end(),whiteCharCandidates.begin(),whiteCharCandidates.end());
        charCandidates.insert(charCandidates.end(),blackCharCandidates.begin(),blackCharCandidates.end());
        }*/
        //   }
        // RemoveUselessChars(charCandidates);


#ifdef _DEBUG
        DrawERHelper(src, "DebugWholeDisconnect.jpg", charCandidates);
#endif
    }

    Rect RobustSceneText::PadRect(const Rect& rect, int rows, int cols)
    {
        const double minXPad = 3.;
        const double minYPad = 3.;
        const int maxXPad = 10;
        const int maxYPad = 10;
        const double widthPadRatio = 0.025;
        const double heightPadRatio = 0.05;
        int xPad = (int)std::max(minXPad, rect.width * widthPadRatio);
        int yPad = (int)max(minYPad, rect.height * heightPadRatio);
        xPad = (int)min(maxXPad, xPad);
        yPad = (int)min(maxYPad, yPad);
        int xmin = (int)max(0, rect.x - xPad);
        int ymin = (int)max(0, rect.y - yPad);
        int xmax = (int)min(cols, rect.x + rect.width + xPad);
        int ymax = (int)min(rows, rect.y + rect.height + yPad);
        return Rect(xmin, ymin, xmax - xmin, ymax - ymin);
    }

    void RobustSceneText::PostProcess(vector<Rect>& textRects, int rows, int cols)
    {

        if (abs(tempParam.scale - 1.0) < 0.00001)
        {
            for (size_t i = 0; i < textRects.size(); ++i)
            {
                textRects[i] = PadRect(textRects[i], rows, cols);
            }
            return;
        }
        for (size_t i = 0; i < textRects.size(); ++i)
        {
            textRects[i].x = int(textRects[i].x / tempParam.scale);
            textRects[i].y = int(textRects[i].y / tempParam.scale);
            textRects[i].width = int(textRects[i].width / tempParam.scale);
            textRects[i].height = int(textRects[i].height / tempParam.scale);
            textRects[i] = PadRect(textRects[i], rows, cols);
        }
    }

    RSTParameter::RSTParameter(RSTLANGUAGE language)
    {
        lang = language;
        clusterUseColor = false;
        clusterUseStroke = true;//for gradient image, we have problems
        binaryType = BinarizationType::KMEANS_COLOR;
        scale = 1.0;
        recoverLostChar = false;
        preFilterMSERMaxHeight = 200;
        preFilterMSERMinHeight = 7;
        preFilterMSERMaxWidth = 100;
        preFilterMSERMinWidth = 3;
        preFilterMSERMaxArea = 10000;
        charHighProb = 50;
        charLowProb = 25;
        useGradientChannel = false;
#ifdef FORMEDICA

        minArea = 10;
        maxArea = 10000;
        maxVariation = 0.5;
        minDiversity = 0.1;
        charHighProb = 50;
        delta = 5;
        postFilterTextLineAspectRatio = 3.0;
        charLowProb = 25;
        // clusterUseColor = false;
        clusterUseStroke = true;
        useGradientChannel = false;

#else
        minArea = 10;
        maxArea = 3500;
        maxVariation = 0.5;
        minDiversity = 0.1;
        delta = 1;
        postFilterTextLineAspectRatio = 5.0;

#endif

    }

#pragma region SINGLE_LINKAGE
    void RobustSceneText::ExtractPoints(const vector<CvContour*>& regions, vector<MSERFeature>&  feats)
    {
        feats.resize(regions.size());
        for (size_t i = 0; i < regions.size(); ++i)
        {
            CvContour* seq = regions[i];
            feats[i].rect = seq->rect;
            feats[i].pPts = shared_ptr<vector<Point>>(new vector<Point>());
            for (int j = 0; j < seq->total; ++j)
            {
                CvPoint *pos = CV_GET_SEQ_ELEM(CvPoint, seq, j);
                feats[i].pPts->push_back(*pos);
            }
        }
    }

    //need double check
    void RobustSceneText::ExtractFeatures(const Mat& src, const vector<CvContour*>& regions, vector<MSERFeature>& feats)
    {
        feats.clear();
        ExtractPoints(regions, feats);

        for (size_t i = 0; i < regions.size(); ++i)
        {
            MSERFeature feat;
            feat.rect = feats[i].rect;
            const vector<Point>& pts = *(feats[i].pPts);
            int borderPad = 5;
            Mat img = Mat::zeros(feat.rect.height + 2 * borderPad, feat.rect.width + 2 * borderPad, CV_8UC1);

            for (size_t j = 0; j < pts.size(); ++j)
            {
                int r = pts[j].y - feat.rect.y + borderPad;
                int c = pts[j].x - feat.rect.x + borderPad;
                uchar* data = img.ptr<uchar>(r);
                data[c] = 255;
            }
            Scalar strokeMean = 1, storkeStd = 1;
            if (tempParam.clusterUseStroke)
            {
                Mat dMap;
                distanceTransform(img, dMap, CV_DIST_L1, 3);
                Mat skeleton = Mat::zeros(img.size(), CV_8UC1);
                Mat imgguo = img.clone();
                GuoHallThinning(imgguo, skeleton);
                Mat mask;
                skeleton.copyTo(mask);
                meanStdDev(dMap, strokeMean, storkeStd, mask);
            }
            feat.strokeMean = strokeMean[0];
            feat.strokeStd = storkeStd[0];
            Scalar colorMean = 1, colorStd = 1;
            if (tempParam.clusterUseColor)
            {
                Mat mask = img(Rect(borderPad, borderPad, feat.rect.width, feat.rect.height));
                meanStdDev(src(feat.rect), colorMean, colorStd, mask);//get one feature
                feat.colorMean = colorMean;
            }
            feats[i].colorMean = feat.colorMean;
            feats[i].strokeMean = feat.strokeMean;
            feats[i].strokeStd = feat.strokeStd;
        }


    }

    vector<Rect> RobustSceneText::SingleLinkageCluster(vector<MSERFeature>& MSERFeatures, vector<CvContour*>&texts, vector<vector<CvContour*>>& clusters, vector<vector<int>>& clusterLabels)
    {
        vector<int> labels;
        clusters.clear();
        clusterLabels.clear();
        if (MSERFeatures.size() <= 0)
        {
            vector<Rect> empty;
            return empty;
        }
        vector<MSERFeature> &temp = MSERFeatures;
        int n = partition(temp, labels, MSERFeatureFunc());

        vector<Rect> rects(n);
        clusters.resize(n);
        clusterLabels.resize(n);
        vector<bool> bInit(n);
        for (int i = 0; i < n; ++i)
        {
            bInit[i] = false;
        }
        for (int i = 0; i < (int)(temp.size()); ++i)
        {
            int label = labels[i];
            clusters[label].push_back(texts[i]);
            clusterLabels[label].push_back(i);
            if (false == bInit[label])
            {
                rects[label] = temp[i].rect;

                bInit[label] = true;
            }
            else
            {
                int xTL = std::min(rects[label].x, temp[i].rect.x);
                int yTL = std::min(rects[label].y, temp[i].rect.y);
                int xBR = std::max(rects[label].x + rects[label].width,
                    temp[i].rect.x + temp[i].rect.width);
                int yBR = std::max(rects[label].y + rects[label].height,
                    temp[i].rect.y + temp[i].rect.height);
                rects[label] = Rect(Point(xTL, yTL), Point(xBR, yBR));
            }
        }


        return rects;
    }

    bool RobustSceneText::MSERFeatureFuncRecover::operator()(const MSERFeature& li, const MSERFeature& lj)
    {

        double spatialDistance = 0;
        if (li.rect.x < lj.rect.x)
        {
            spatialDistance = double(abs(lj.rect.x - li.rect.x - li.rect.width)) / max(li.rect.height, lj.rect.height);
        }
        else
        {
            spatialDistance = double(abs(li.rect.x - lj.rect.x - lj.rect.width)) / max(li.rect.height, lj.rect.height);
        }

        double widthDiff = (double)(abs(li.rect.width - lj.rect.width)) / max(li.rect.width, lj.rect.width);

        if ((li.rect.width > 3 * li.rect.height) || lj.rect.width > 3 * lj.rect.height)//MSER will detect some regions with several chars
            widthDiff = 0;
        double heightDiff = (double)(abs(li.rect.height - lj.rect.height)) / max(li.rect.height, lj.rect.height);
        double topAlignments = (double)(atan2(abs(li.rect.y - lj.rect.y), abs(li.rect.x + li.rect.width / 2 - lj.rect.x - lj.rect.width / 2)));
        double bottomeAlignments = (double)(atan2(abs(li.rect.y + li.rect.height - lj.rect.y - lj.rect.height),
            abs(li.rect.x + li.rect.width / 2 - lj.rect.x - lj.rect.width / 2)));
        double colorDifference = sqrt((li.colorMean[0] - lj.colorMean[0]) * (li.colorMean[0] - lj.colorMean[0])
            + (li.colorMean[1] - lj.colorMean[1]) * (li.colorMean[1] - lj.colorMean[1])
            + (li.colorMean[2] - lj.colorMean[2]) * (li.colorMean[2] - lj.colorMean[2]));
        colorDifference /= 255.0;
        double strokeWidthDiff = (double)(abs(li.strokeMean - lj.strokeMean)) / max(li.strokeMean, lj.strokeMean);
        /*  double distance = spatialDistance*6.6395 + widthDiff*(-0.756415) +heightDiff*(1.520538) + topAlignments*(4.4788746)+\
        bottomeAlignments*4.850933 + colorDifference*0.04525285 + strokeWidthDiff*0.6514 - 9.207139;*/

        double colorDifferenceParamHandTune = 0.5;
        double spatialDistanceParamHandTune = 0.5;//trick one by wenhua
        double widthDiffParamHandTune = .5;//trick one by wenhua
        double heightDiffParamHandTune = 0.5;//trick one by wenhua
        double topAlignmentsHandTune = 0.5;
        double bottomeAlignmentsHandTune = 0.5;
        double distance = spatialDistance * 0.85 * spatialDistanceParamHandTune + widthDiff * (12.82) * widthDiffParamHandTune + heightDiff * (7.68) *heightDiffParamHandTune + topAlignments*(6.4)*topAlignmentsHandTune + \
            bottomeAlignments*38.41*bottomeAlignmentsHandTune + colorDifference*(40.82)*colorDifferenceParamHandTune + strokeWidthDiff*32.02 - 21.52;
        if (distance < 0)
            return true;
        else
            return false;
    }
    bool RobustSceneText::MSERFeatureFunc::operator()(const MSERFeature& li, const MSERFeature& lj)
    {
        const double maxStrokeRatio = 2.0;
        const double maxHeighRatio = 2.3;
        const double maxWidthRatio = 6.0;
        //using std::abs;
        double liHeight = li.rect.height;
        double ljHeight = lj.rect.height;
        double liWidth = li.rect.width;
        double ljWidth = lj.rect.width;

        double widthRatio = (double)liWidth / ljWidth ;
        if(widthRatio < 1.0)
            widthRatio = 1.0/widthRatio;
        double heightRatio = (double)liHeight / ljHeight;
        if(heightRatio < 1.0)
            heightRatio = 1.0/heightRatio;
        //exclude i, j
        const int ijWidth = 5;
        
        if (liWidth > ijWidth && ljWidth> ijWidth)
        {
            if (widthRatio > maxWidthRatio )
                return false;
        }

        if (heightRatio > maxHeighRatio)
            return false;

        double strokeRatio = (double)li.strokeMean / lj.strokeMean;
        if(strokeRatio < 1.0)
        {
            strokeRatio = 1.0/strokeRatio;
        }
        if(strokeRatio > maxStrokeRatio)
        {
            return false;
        }

        //double widthGap =(double) abs(li.rect.x + li.rect.width/2 - lj.rect.x -lj.rect.width/2);
        //double widthGapRatio = widthGap / max(li.rect.width, lj.rect.width);

        //if(widthGapRatio > 3.5)
        //    return false;


        double bottomHeight = min(li.rect.y + li.rect.height, lj.rect.y + lj.rect.height);//different line, not tricky
        double topHeight = max(li.rect.y, lj.rect.y);
        if (topHeight > bottomHeight)
            return false;
        //#ifdef FORMEDICA
        //		double bottomDifRatio = (float)abs(li.rect.y + li.rect.height - lj.rect.y - lj.rect.height) / min(li.rect.height, lj.rect.height);
        //		if (bottomDifRatio > 0.3)
        //			return false;
        //#endif
        /*double heightD = abs(lj.rect.y + lj.rect.height / 2 - li.rect.y - li.rect.height / 2);
        double heightRatio = heightD/(liHeight + ljHeight);
        if(heightRatio>1)
        return false;*/

        /* double angle = atan2(double(lj.rect.y + lj.rect.height/2 - li.rect.y - li.rect.height / 2),
        double(lj.rect.x + lj.rect.width / 2  -   li.rect.x - li.rect.width / 2 ));
        if(angle > 40 || angle < 40)
        {
        return false;
        }
        */

        double gapDiff = 0;
        double gapDiffThreshold = 2;
        //#ifdef FORMEDICA
        //		gapDiffThreshold = 2;
        //#endif
        if (li.rect.x < lj.rect.x)
        {
            gapDiff = double(abs(lj.rect.x - li.rect.x - li.rect.width)) / max(li.rect.height, lj.rect.height);
        }
        else
        {
            gapDiff = double(abs(li.rect.x - lj.rect.x - lj.rect.width)) / max(li.rect.height, lj.rect.height);
        }

        if (gapDiff > gapDiffThreshold)
            return false;

        double spatialDistance = 0;
        if (li.rect.x < lj.rect.x)
        {
            spatialDistance = double(abs(lj.rect.x - li.rect.x - li.rect.width)) / max(li.rect.height, lj.rect.height);
        }
        else
        {
            spatialDistance = double(abs(li.rect.x - lj.rect.x - lj.rect.width)) / max(li.rect.height, lj.rect.height);
        }

        double widthDiff = (double)(abs(li.rect.width - lj.rect.width)) / max(li.rect.width, lj.rect.width);

        if ((li.rect.width > 3 * li.rect.height) || lj.rect.width > 3 * lj.rect.height)//MSER will detect some regions with several chars
            widthDiff = 0;
        double heightDiff = (double)(abs(li.rect.height - lj.rect.height)) / max(li.rect.height, lj.rect.height);
        double topAlignments = (double)(atan2(abs(li.rect.y - lj.rect.y), abs(li.rect.x + li.rect.width / 2 - lj.rect.x - lj.rect.width / 2)));
        double bottomeAlignments = (double)(atan2(abs(li.rect.y + li.rect.height - lj.rect.y - lj.rect.height),
            abs(li.rect.x + li.rect.width / 2 - lj.rect.x - lj.rect.width / 2)));
        double colorDifference = sqrt((li.colorMean[0] - lj.colorMean[0]) * (li.colorMean[0] - lj.colorMean[0])
            + (li.colorMean[1] - lj.colorMean[1]) * (li.colorMean[1] - lj.colorMean[1])
            + (li.colorMean[2] - lj.colorMean[2]) * (li.colorMean[2] - lj.colorMean[2]));
        colorDifference /= 255.0;
        double strokeWidthDiff = (double)(abs(li.strokeMean - lj.strokeMean)) / max(li.strokeMean, lj.strokeMean);
        /*  double distance = spatialDistance*6.6395 + widthDiff*(-0.756415) +heightDiff*(1.520538) + topAlignments*(4.4788746)+\
        bottomeAlignments*4.850933 + colorDifference*0.04525285 + strokeWidthDiff*0.6514 - 9.207139;*/

        double colorDifferenceParamHandTune = 1;
        double spatialDistanceParamHandTune = 0.8;//trick one by wenhua
        double widthDiffParamHandTune = 1;//trick one by wenhua
        double heightDiffParamHandTune = 0.8;//trick one by wenhua
        double topAlignmentsHandTune = 0.8;
        double bottomeAlignmentsHandTune = 0.8;
        double distance = spatialDistance * 0.85 * spatialDistanceParamHandTune + widthDiff * (12.82) * widthDiffParamHandTune + heightDiff * (7.68) *heightDiffParamHandTune + topAlignments*(6.4)*topAlignmentsHandTune + \
            bottomeAlignments*38.41*bottomeAlignmentsHandTune + colorDifference*(40.82)*colorDifferenceParamHandTune + strokeWidthDiff*32.02 - 21.52;
        /* double distance = spatialDistance * 3.78 * spatialDistanceParamHandTune + widthDiff * (9.84) * widthDiffParamHandTune + heightDiff * (2.12) *heightDiffParamHandTune + topAlignments*(1.25)*topAlignmentsHandTune +\
        bottomeAlignments*21.71*bottomeAlignmentsHandTune + colorDifference*(1.22)*colorDifferenceParamHandTune + strokeWidthDiff*21.57 - 21.52;*/
        //		Rect unionRect = li.rect | lj.rect;
        //
        //		int unionArea = unionRect.area();
        //		double areaRatio = (double)(li.rect.area() + lj.rect.area()) / unionArea;
        //#ifdef FORMEDICA
        //		if (distance < 0 && areaRatio > 0.66)
        //#else
        //		if (distance < 0)
        //#endif


        if (distance < 0)
            return true;
        else
            return 0;
    }
#pragma region SINGLE_LINKAGE

#pragma region MSER_TREE_PRUN
    void RobustSceneText::CalcChildNum(vector<CvContour*>&  msers)
    {
        for (size_t i = 0; i < msers.size(); ++i)
        {
            int cSize = 0;
            CvSeq* c = msers[i]->v_next;
            while (c != NULL)
            {
                ++cSize;
                c = c->h_next;
            }
            (msers[i])->reserved[1] = cSize;
        }
    }

    void RobustSceneText::RegVar(vector<CvContour*>& msers, bool isBlack)
    {
        int sign = isBlack ? 1 : -1;//black is postive,white is negtive
        for (size_t i = 0; i<msers.size(); ++i)
        {
            int* var = &(msers[i]->reserved[0]);
            float aspectRatio = (float(msers[i]->rect.height)) / (msers[i]->rect.width);
            *var += 1;//make sure it's postive
            if (aspectRatio > 1.2)
            {
                *var += int((aspectRatio - 1.2) * 100) * sign;
            }
            else if (aspectRatio < 0.3)
            {
                *var += int((0.3 - aspectRatio) * 3500) * sign;
            }
        }
    }

    void RobustSceneText::GetRootNode(const vector<CvContour*>& msers, vector<CvContour*>& roots)
    {
        for (size_t i = 0; i < msers.size(); ++i)
        {
            if (msers[i]->v_prev == NULL)
            {
                roots.push_back(msers[i]);
            }
        }
    }

    void RobustSceneText::PruneTree(vector<CvContour*>&  msers, vector<CvContour*>& charCandidates, bool isblack)
    {
        charCandidates.clear();
        RegVar(msers, isblack);
        CalcChildNum(msers);
        vector<CvContour*> roots;
        GetRootNode(msers, roots);
        for (size_t i = 0; i < roots.size(); ++i)
        {
            CvContour* temp = LinearReduction(roots[i]);

            vector<CvContour*> c = TreeAccumulation(temp);
            charCandidates.insert(charCandidates.end(), c.begin(), c.end());
        }
    }

    CvContour* RobustSceneText::LinearReduction(CvContour* root)
    {
        switch (root->reserved[1])
        {
        case 0:
            {
                return root;
                break;
            }

        case 1:
            {
                CvContour* c = LinearReduction((CvContour*)(root->v_next));
                if (std::abs(c->reserved[0]) < std::abs(root->reserved[0]))
                {
                    return c;
                }
                else
                {
                    //link c's children to root
                    CvSeq* cc = c->v_next;
                    root->v_next = cc;
                    while (cc != NULL)
                    {
                        cc->v_prev = (CvSeq*)root;
                        cc = cc->h_next;
                    }
                    return root;
                }
                break;
            }

        default:
            {
                CvSeq* c = root->v_next;
                vector<CvContour*> children;
                while (c != NULL)
                {

                    CvContour* temp = LinearReduction((CvContour*)c);
                    children.push_back(temp);
                    temp->v_prev = (CvSeq*)root;// reset parents;
                    c = c->h_next;
                }

                root->v_next = (CvSeq*)(children[0]);
                for (size_t i = 0; i < children.size() - 1; ++i)//reset prev and next
                {
                    children[i]->h_next = (CvSeq*)(children[i + 1]);
                    children[i + 1]->h_prev = (CvSeq*)(children[i]);
                }

                return  root;
                break;
            }
        }
    }

    vector<CvContour*> RobustSceneText::TreeAccumulation(CvContour* root)
    {
        //need to recalcu childNum due to linear reduction
        vector<CvContour*> vTemp;
        vTemp.push_back(root);
        CalcChildNum(vTemp);
        assert(root->reserved[1] != 1);
        vector<CvContour*> result;
        if (root->reserved[1] >= 2)
        {
            CvContour* c = (CvContour*)(root->v_next);
            while (c != NULL)
            {
                vector<CvContour*> temp;
                temp = TreeAccumulation(c);
                result.insert(result.end(), temp.begin(), temp.end());
                c = (CvContour*)c->h_next;
            }
            for (size_t i = 0; i < result.size(); ++i)
            {
                if (std::abs(result[i]->reserved[0]) < std::abs(root->reserved[0]))
                {
                    return result;
                }
            }
            result.clear();
            result.push_back(root);
            return result;

        }
        else
        {
            result.push_back(root);
            return result;
        }
    }
#pragma region MSER_TREE_PRUN
    RSTParameter& RobustSceneText::GetParams()
    {
        return param;
    }

    RobustSceneText::RobustSceneText(const RSTParameter& para)
    {
        tempParam = param = para;
        if (tempParam.useGradientChannel)
        {
            tempParam.clusterUseStroke = false;
        }
#ifdef DaVinciRST
        string netFile = TestManager::Instance().GetDaVinciResourcePath("RstData/character.prototxt");
        string meanFile = TestManager::Instance().GetDaVinciResourcePath("RstData/mean.binaryproto");
        string trainFile = "";
        if (para.lang == RSTLANGUAGE::ENG)//
        {
            trainFile = TestManager::Instance().GetDaVinciResourcePath("RstData/characterDataEng");
        }
        else if (para.lang == RSTLANGUAGE::CHI)
        {
            trainFile = TestManager::Instance().GetDaVinciResourcePath("RstData/characterDataChiEng");
        }
        else
        {
            trainFile = TestManager::Instance().GetDaVinciResourcePath("RstData/characterDataChiEng");
        }
#else
        string netFile = "..\\..\\TextRecognition\\caffe\\data\\cifar_test.prototxt";

        string meanFile = "..\\..\\TextRecognition\\caffe\\data\\mean.binaryproto";

        string trainFile;
        if (para.lang == RSTLANGUAGE::ENG)
        {
            trainFile = "..\\..\\TextRecognition\\caffe\\data\\characterDataEng";
        }
        else  if (para.lang == RSTLANGUAGE::CHI)
        {
            trainFile = "..\\..\\TextRecognition\\caffe\\data\\characterDataChi";
        }
        else
        {
            trainFile = "..\\..\\TextRecognition\\caffe\\data\\characterDataChiEng";
        }
#endif
        characterClassifier = shared_ptr<CaffeNet>(new CaffeNet(netFile.c_str(), trainFile.c_str(), meanFile.c_str(), 50));
        characterClassifier->SetMode(false);
    }

    void RobustSceneText::RemoveUselessChars(vector<CvContour*>& charCandidates)
    {
        vector<bool> isErased(charCandidates.size(), false);

        for (size_t i = 0; i < charCandidates.size(); ++i)
            for (size_t j = i + 1; j < charCandidates.size(); ++j)
            {
                if (isErased[i] || isErased[j])
                    continue;
                size_t minIndex = Rect(charCandidates[i]->rect).area()< Rect(charCandidates[j]->rect).area() ? i : j;
                size_t maxIndex = (minIndex == i) ? j : i;
                CvRect& maxAreaRect = charCandidates[maxIndex]->rect;
                Rect interRect = (Rect(charCandidates[i]->rect) & Rect(charCandidates[j]->rect));
                float minRatio = (float)interRect.area() / ((Rect)maxAreaRect).area();
                if (minRatio>0.9)
                {
                    Rect unionRect = (Rect(charCandidates[i]->rect) | Rect(charCandidates[j]->rect));

                    float maxRatio = (float)unionRect.area() / ((Rect)charCandidates[maxIndex]->rect).area();
                    if (maxRatio < 1.2)
                    {
                        //charCandidates[maxIndex]->rect = unionRect;
                        // isErased[minIndex] = true;

                        if (abs(charCandidates[i]->reserved[0]) < abs(charCandidates[j]->reserved[0]))
                        {
                            isErased[j] = true;
                        }
                        else
                        {
                            isErased[i] = true;
                        }
                    }
                }
            }

            vector<CvContour*> notErasedCharCandidates;
            for (size_t i = 0; i < isErased.size(); ++i)
            {
                if (isErased[i])
                {
                    continue;
                }
                notErasedCharCandidates.push_back(charCandidates[i]);
            }
            charCandidates = notErasedCharCandidates;
    }

    void RobustSceneText::SortRects(vector<Rect>& rects, vector<vector<CvContour*>>& textlines)
    {
        const int gapBufferY = 5;// due to some issues, even y coordinates of words in same line have tiny change
        std::vector<int> indices(rects.size());
        int n = 0;
        std::generate(std::begin(indices), std::end(indices), [&]{ return n++; });
        std::sort(std::begin(indices),
            std::end(indices),
            [&](int i1, int i2) { return (rects[i1].y <= rects[i2].y - gapBufferY) ||
            ((rects[i1].y < rects[i2].y + gapBufferY) && (rects[i1].x < rects[i2].x)); });// top left

        vector<Rect> rectsTemp(rects.size());
        vector<vector<CvContour*>> textlinesTemp(textlines.size());
        for (size_t i = 0; i < indices.size(); ++i)
        {
            rectsTemp[i] = rects[indices[i]];
            textlinesTemp[i] = textlines[indices[i]];
        }

        rects = rectsTemp;
        textlines = textlinesTemp;
    }

    //void RobustSceneText::FilterTextLineByBeyesian(const Mat& srcColorImage, const vector<CvContour*>& charCandidates,vector<Rect>& textRects, vector<vector<CvContour*>>& textlines)
    //{
    //    vector<bool> isErased(textRects.size(),false);
    //    vector<vector<float>> charProbInTextLines(textRects.size());
    //

    //    for (size_t i = 0; i<textRects.size(); ++i)
    //    {
    //        for (size_t j = 0; j< charCandidates.size();++j)
    //        {
    //            Rect unionRect = textRects[i] | ((Rect)charCandidates[j]->rect);
    //            if(unionRect == textRects[i])
    //            {
    //                charProbInTextLines[i].push_back((float)charCandidates[j]->reserved[2]/100);
    //            }
    //        }
    //    }
    //      
    //   // static int index = 1;
    //    for(size_t i=0; i<textRects.size(); ++i)
    //    {
    //        int num = charProbInTextLines[i].size();
    //            
    //        if(num>1)
    //        {
    //            float pos = 1.0;
    //            float neg = 1.0;
    //            for ( size_t j =0;j<num;++j)
    //            {
    //                pos *=charProbInTextLines[i][j];
    //                neg *=(1.0-charProbInTextLines[i][j]);
    //            }
    //            if (neg > pos)
    //            {
    //                isErased[i] = true;
    //            }
    //        }
    //        else
    //        {
    //            if(charProbInTextLines[i][0]< 0.95)
    //            {
    //                isErased[i] = true;
    //            }
    //        }
    //        vector<Rect> notErasedTextRects;
    //        vector<vector<CvContour*>> notErasedTextlines;       
    //      for (size_t i = 0; i< isErased.size(); ++i)
    //      {
    //          if (isErased[i])
    //          {
    //              continue;
    //          }              
    //          notErasedTextRects.push_back(textRects[i]);
    //          notErasedTextlines.push_back(textlines[i]);
    //      }
    //      textRects = notErasedTextRects;
    //    textlines =  notErasedTextlines;

    //   
    //        
    //        string imageName = to_string(1) + "//" + to_string(index)+ ".png";
    //        ++index;
    //        imwrite(imageName, srcColorImage(textRects[i]));
    //        
    //    }

    //}


    //currently just remove the contained rects
    void RobustSceneText::RemoveUselessRect(vector<Rect>&textRects, vector<vector<CvContour*>>& textClusters)
    {
        vector<bool> isErased(textRects.size(), false);
        //for(size_t i=0;i<textClusters.size();++i)
        //{
        //    if(textClusters[i].size()>2)
        //        continue;
        //    if(textClusters[i].size()==1)
        //    {
        //        if((textClusters[i][0]->reserved[2] < 90) && (textClusters[i][0]->rect.width < 2*textClusters[i][0]->rect.height))
        //        {
        //            isErased[i] == true;
        //        }
        //    }
        //    else if(textClusters[i].size()==2)
        //    {
        //        if((textClusters[i][0]->reserved[2] < 90) &&(textClusters[i][1]->reserved[2] < 90) &&( textRects[i].width < 3*textRects[i].height))
        //        {
        //            isErased[i] == true;
        //        }
        //    }
        //}
        for (size_t i = 0; i < textRects.size(); ++i)
        {
            float aspectRatio = (float)textRects[i].height / textRects[i].width;
            if (aspectRatio > tempParam.postFilterTextLineAspectRatio)
            {
                isErased[i] = true;
            }
        }
        for (int i = 0; i < (int)(textRects.size()); ++i)
            for (int j = i + 1; j < (int)(textRects.size()); ++j)
            {

                int minIndex = textRects[i].area()< textRects[j].area() ? i : j;
                Rect& minAreaRect = textRects[minIndex];
                Rect interRect = (textRects[i] & textRects[j]);
                float minRatio = (float)interRect.area() / minAreaRect.area();
                if (minRatio>0.8)
                {
                    Rect unionRect = (textRects[i] | textRects[j]);
                    Rect& maxAreaRect = textRects[i].area() < textRects[j].area() ? textRects[j] : textRects[i];
                    float maxRatio = (float)unionRect.area() / maxAreaRect.area();
                    if (maxRatio < 1.2)
                    {
                        maxAreaRect = unionRect;
                        isErased[minIndex] = true;
                    }
                }
            }

            vector<Rect> notErasedTextRects;
            vector<vector<CvContour*>> notErasedTextClusters;
            for (size_t i = 0; i < isErased.size(); ++i)
            {
                if (isErased[i])
                {
                    continue;
                }

                notErasedTextClusters.push_back(textClusters[i]);

                notErasedTextRects.push_back(textRects[i]);
            }
            textRects = notErasedTextRects;
            textClusters = notErasedTextClusters;
    }

    vector<float> RobustSceneText::EliminateFalseCharacter(const Mat& gray, const vector<Rect>& rects, Size resizeScale)
    {
        vector<Mat> rois;
        rois.reserve(rects.size());
        for (size_t i = 0; i < rects.size(); ++i)
        {
            Mat roi = gray(rects[i]);
            Mat resizeImage;
            resize(roi, resizeImage, resizeScale);
            /*  Mat colorImage;
            cvtColor(resizeImage, colorImage, CV_GRAY2BGR);*/
            rois.push_back(resizeImage);
        }
        vector<float> results(rects.size());
        std::vector<std::vector<float>> predictResults;

#ifdef DaVinciRST
        boost::lock_guard<boost::mutex> lock(classifierLock);
#else
        characterClassifier->SetMode(true);
#endif DaVinciRST
        characterClassifier->Predict(rois, predictResults);

#ifdef SAVEIMAGE        
        static int posNum = 1;
        static int negNum = 1;
#endif

        for (size_t i = 0; i<rects.size(); ++i)
        {
            results[i] = predictResults[i][1];
            if (predictResults[i][1]>predictResults[i][0])
            {
#ifdef SAVEIMAGE
                string name = "POS\\gradientCPos_" + to_string(posNum) + ".png";
                ++posNum;
                imwrite(name, srcColorImage(rects[i]));
#endif
            }
            else
            {
#ifdef SAVEIMAGE
                string name = "Neg\\gradientCNeg_" + to_string(negNum) + ".png";
                ++negNum;
                imwrite(name, srcColorImage(rects[i]));
#endif

            }
        }
        return results;
    }

    double RobustSceneText::TextInterUnio(const Rect& rect1, const Rect& rect2)
    {
        int bi[4];
        bi[0] = max(rect1.x, rect2.x);
        bi[1] = max(rect1.y, rect2.y);
        bi[2] = min(rect1.x + rect1.width, rect2.x + rect2.width);
        bi[3] = min(rect1.y + rect1.height, rect2.y + rect2.height);
        double iw = bi[2] - bi[0];
        double ih = bi[3] - bi[1];
        double ov = 0;
        if (iw > 0 && ih > 0)
        {
            double area = rect2.area();
            ov = (double)iw * ih / area;
        }
        return ov;
    }

    //vector<Mat > RobustSceneText::ConvertRGBTextToBinary(const Mat& img, const vector<Rect>& textRects, const vector<vector<Point2i>>& pts)
    //{
    //    if(3 == img.channels())
    //    {
    //        vector<Mat> res;
    //        Mat Lab;
    //        cvtColor(img, Lab, CV_BGR2Lab);
    //        //hash color
    //        for (size_t i = 0; i < pts.size(); ++i)
    //        {
    //            std::unordered_map<unsigned int, bool> htmap;
    //            for (size_t j = 0;j < pts[i].size(); ++j)
    //            {
    //                const Point2i& pt = pts[i][j]; 
    //                unsigned int L = Lab.at<cv::Vec3b>(pt.y, pt.x)[0] / 2;
    //                unsigned int a = (Lab.at<cv::Vec3b>(pt.y, pt.x)[1] + 128) / 5; 
    //                unsigned int b = (Lab.at<cv::Vec3b>(pt.y, pt.x)[2] + 128) / 5; 
    //                unsigned int hashVal = (L<<16) + (a<<8) + b;
    //                htmap[hashVal] = true;
    //            }

    //            int roiWidth = textRects[i].width;
    //            int roiHeight =  textRects[i].height;

    //            Mat binaryImage(roiHeight, roiWidth, CV_8UC1);
    //            for (int r = textRects[i].y; r < textRects[i].y + textRects[i].height; ++r)
    //                for (int c = textRects[i].x;c < textRects[i].x + textRects[i].width; ++c)
    //                {
    //                    unsigned int L = Lab.at<cv::Vec3b>(r, c)[0] / 2;
    //                    unsigned int a = (Lab.at<cv::Vec3b>(r, c)[1] + 128) / 5; 
    //                    unsigned int b = (Lab.at<cv::Vec3b>(r, c)[2]+128) / 5; 
    //                    unsigned int hashVal = (L<<16) + (a<<8) + b;
    //                    int x = c - textRects[i].x;
    //                    int y = r - textRects[i].y;
    //                    if (htmap.find(hashVal) != htmap.end())
    //                    {
    //                        binaryImage.at<uchar>(y, x) = 255;
    //                    }
    //                    else
    //                    {
    //                        binaryImage.at<uchar>(y, x) = 0;
    //                    }
    //                }
    //                res.push_back(binaryImage);
    //        }
    //        return res;
    //    }
    //    else//for gray image
    //    {
    //        vector<Mat> res;
    //        //hash color
    //        for (size_t i = 0; i < pts.size(); ++i)
    //        {
    //            std::unordered_map<uchar, bool> htmap;
    //            for (size_t j = 0;j < pts[i].size(); ++j)
    //            {
    //                const Point2i& pt = pts[i][j]; 
    //                uchar G = img.at<uchar>(pt.y, pt.x) / 5;
    //                htmap[G] = true;
    //            }

    //            int roiWidth = textRects[i].width;
    //            int roiHeight =  textRects[i].height;

    //            Mat binaryImage(roiHeight,roiWidth,CV_8UC1);
    //            for (int r= textRects[i].y; r< textRects[i].y + textRects[i].height; ++r)
    //                for (int c = textRects[i].x; c < textRects[i].x + textRects[i].width; ++c)
    //                {
    //                    uchar G = img.at<uchar>(r,c) / 5;

    //                    int x = c - textRects[i].x;
    //                    int y = r - textRects[i].y;
    //                    if (htmap.find(G) != htmap.end())
    //                    {

    //                        binaryImage.at<uchar>(y,x) = 255;
    //                    }
    //                    else
    //                    {
    //                        binaryImage.at<uchar>(y,x) = 0;
    //                    }
    //                }
    //                res.push_back(binaryImage);
    //        }
    //        return res;
    //    }
    //}

    bool RobustSceneText::PreFilterMSER(const Rect& rect, int maxHeight, int minHeight, int maxWidth, int minWidth, int maxArea)
    {
        //double ratio = (double)rect.height

        if (rect.height > maxHeight || rect.height < minHeight || rect.width > maxWidth || rect.width < minWidth || rect.area() > maxArea)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    static void GetGradientMagnitude(Mat& grayImg, Mat& gradientMagnitude)
    {
        Mat C = Mat_<float>(grayImg);

        Mat kernel = (Mat_<float>(1, 3) << -1, 0, 1);
        Mat grad_x;
        filter2D(C, grad_x, -1, kernel, Point(-1, -1), 0, BORDER_DEFAULT);

        Mat kernel2 = (Mat_<float>(3, 1) << -1, 0, 1);
        Mat grad_y;
        filter2D(C, grad_y, -1, kernel2, Point(-1, -1), 0, BORDER_DEFAULT);

        magnitude(grad_x, grad_y, gradientMagnitude);
    }

    static bool GuoHallThinning(const Mat1b & img, Mat& skeleton)
    {
        uchar* img_ptr = img.data;
        uchar* skel_ptr = skeleton.data;

        for (int row = 0; row < img.rows; ++row)
        {
            for (int col = 0; col < img.cols; ++col)
            {
                if (*img_ptr)
                {
                    int key = row * img.cols + col;
                    if ((col > 0 && *img_ptr != img.data[key - 1]) ||
                        (col < img.cols - 1 && *img_ptr != img.data[key + 1]) ||
                        (row > 0 && *img_ptr != img.data[key - img.cols]) ||
                        (row < img.rows - 1 && *img_ptr != img.data[key + img.cols]))
                    {
                        *skel_ptr = 255;
                    }
                    else
                    {
                        *skel_ptr = 128;
                    }
                }
                img_ptr++;
                skel_ptr++;
            }
        }

        int max_iters = 10000;
        int niters = 0;
        bool changed = true;

        /// list of keys to set to 0 at the end of the iteration
        deque<int> cols_to_set;
        deque<int> rows_to_set;

        while (changed && niters < max_iters)
        {
            changed = false;
            for (unsigned short iter = 0; iter < 2; ++iter)
            {
                uchar *skeleton_ptr = skeleton.data;
                rows_to_set.clear();
                cols_to_set.clear();
                // for each point in skeleton, check if it needs to be changed
                for (int row = 0; row < skeleton.rows; ++row)
                {
                    for (int col = 0; col < skeleton.cols; ++col)
                    {
                        if (*skeleton_ptr++ == 255)
                        {
                            bool p2, p3, p4, p5, p6, p7, p8, p9;
                            p2 = (skeleton.data[(row - 1) * skeleton.cols + col]) > 0;
                            p3 = (skeleton.data[(row - 1) * skeleton.cols + col + 1]) > 0;
                            p4 = (skeleton.data[row     * skeleton.cols + col + 1]) > 0;
                            p5 = (skeleton.data[(row + 1) * skeleton.cols + col + 1]) > 0;
                            p6 = (skeleton.data[(row + 1) * skeleton.cols + col]) > 0;
                            p7 = (skeleton.data[(row + 1) * skeleton.cols + col - 1]) > 0;
                            p8 = (skeleton.data[row     * skeleton.cols + col - 1]) > 0;
                            p9 = (skeleton.data[(row - 1) * skeleton.cols + col - 1]) > 0;

                            int C = (!p2 & (p3 | p4)) + (!p4 & (p5 | p6)) +
                                (!p6 & (p7 | p8)) + (!p8 & (p9 | p2));
                            int N1 = (p9 | p2) + (p3 | p4) + (p5 | p6) + (p7 | p8);
                            int N2 = (p2 | p3) + (p4 | p5) + (p6 | p7) + (p8 | p9);
                            int N = N1 < N2 ? N1 : N2;
                            int m = iter == 0 ? ((p6 | p7 | !p9) & p8) : ((p2 | p3 | !p5) & p4);

                            if ((C == 1) && (N >= 2) && (N <= 3) && (m == 0))
                            {
                                cols_to_set.push_back(col);
                                rows_to_set.push_back(row);
                            }
                        }
                    }
                }

                // set all points in rows_to_set (of skel)
                unsigned int rows_to_set_size = (unsigned int)rows_to_set.size();
                for (unsigned int pt_idx = 0; pt_idx < rows_to_set_size; ++pt_idx)
                {
                    if (!changed)
                        changed = (skeleton.data[rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx]]) > 0;

                    int key = rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx];
                    skeleton.data[key] = 0;
                    if (cols_to_set[pt_idx] > 0 && skeleton.data[key - 1] == 128) // left
                        skeleton.data[key - 1] = 255;
                    if (cols_to_set[pt_idx] < skeleton.cols - 1 && skeleton.data[key + 1] == 128) // right
                        skeleton.data[key + 1] = 255;
                    if (rows_to_set[pt_idx] > 0 && skeleton.data[key - skeleton.cols] == 128) // up
                        skeleton.data[key - skeleton.cols] = 255;
                    if (rows_to_set[pt_idx] < skeleton.rows - 1 && skeleton.data[key + skeleton.cols] == 128) // down
                        skeleton.data[key + skeleton.cols] = 255;
                }

                if ((niters++) >= max_iters) // we have done!
                    break;
            }
        }
        skeleton = (skeleton != 0);
        return true;
    }

#ifdef _DEBUG
    static void DrawERHelper(const Mat& src, const string& saveName, const vector<CvContour*>& ers)
    {
        Mat img3c;
        if (3 == src.channels())
        {
            img3c = src.clone();
        }
        else
        {
            cvtColor(src, img3c, CV_GRAY2BGR);
        }
        Mat drawImage = img3c;
        for (size_t i = 0; i < ers.size(); ++i)
        {
            rectangle(drawImage, ers[i]->rect, Scalar(51, 249, 40), 2);
        }
        imwrite(saveName, drawImage);
    }

    /* static void DrawERRecursiveHelper(const Mat& image, const string& saveName, const vector<CvContour*>& subroot)
    {
    Mat drawImage = image.clone();
    stack<CvContour*> stackER;
    vector<CvContour*> allER;
    for (size_t i = 0; i < subroot.size(); ++i)
    {
    stackER.push(subroot[i]);
    while (stackER.size() != 0)
    {
    CvContour *top = stackER.top();
    stackER.pop();
    allER.push_back(top);
    for (CvContour* c = (CvContour*) top->v_next; c != NULL; c= (CvContour*)c->h_next)
    {
    stackER.push(c);
    }
    }
    }
    for (size_t i = 0; i < allER.size(); ++i)
    {
    rectangle(drawImage, allER[i]->rect, Scalar(51,249,40), 2);
    }
    imwrite(saveName, drawImage);
    }*/
#endif
}

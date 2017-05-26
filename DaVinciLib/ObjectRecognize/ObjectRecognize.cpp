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

#include "ObjectRecognize.hpp"
#include <vector>
#include "opencv2/core/gpumat.hpp"
#include "opencv2/core/types_c.h"
#include "opencv2/gpu/gpu.hpp"
#include "ObjectRecognize.hpp"
#include "AKazeObjectRecognize.hpp"
#include "ORBObjectRecognize.hpp"
#include "TestManager.hpp"


using  std::vector;
using namespace cv;
using namespace cv::gpu;

namespace DaVinci
{
    int ObjectRecognize::kpThreshold = 50;
    double ObjectRecognize::aspectRatioThreshold = 8;
    int ObjectRecognize::areaThreshold = 15000;
    FTParameters::FTParameters(int wP, int hP, float p1P, float p2P)
    {    
        width=wP;
        height=hP;
        p1=p1P;
        p2=p2P;
    }
    boost::shared_ptr<ObjectRecognize> ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm algo)
    {
        switch (algo)
        {
        case PatchFeatureAlgorithm::Auto:
        case PatchFeatureAlgorithm::Akaze:
            return boost::shared_ptr<ObjectRecognize>(new AKazeObjectRecognize());
            break;
        case PatchFeatureAlgorithm::Orb:
            return boost::shared_ptr<ObjectRecognize>(new ORBObjectRecognize());
            break;
        default:
            return nullptr;
        }
    }

    ObjectRecognize::ObjectRecognize()
    {
        homography = Mat();
        objectSize = 0;
        keyPoints = vector<KeyPoint>();
        descriptors = Mat();
        size = 0;
        isSmall = false;
        // Rect ROIRect;
        pts = vector<Point2f>();
        //scalar_zero=Scalar();;
        minX = 0, minY = 0, maxX = 0, maxY = 0;
        nonZeroCountThreshold = 0;
        imageSize = Size(0,0);
        matchType = BruteForceM;
        distanceType = DistanceType::L2DT;
        ftp = boost::shared_ptr<FTParameters>(new FTParameters(0, 0, 0.0f, 0.0f));
        unmatchRatioType = UnmatchRatioType::UnmatchOverWholeFeature;
    }

    bool ObjectRecognize::IsSmallCriterion(int width, int height)
    {
        return width * height < areaThreshold;
    }

    ObjectRecognize::~ObjectRecognize()
    {
    }

    int ObjectRecognize::NonZeroThresholdCriterion(bool isSmall)
    {
        int tempNonZeroCountThreshold = 0;
        vector<Point2f> pts;
        KeyPoint::convert(keyPoints,pts);
        Rect rect =  boundingRect(pts);
        double ratio = (double)rect.width / rect.height;
        //if ratio is too large,possibly the model image is button,lower down the threshold
        //if the keypoints spread over a small region,take it as a small image
        int kpSize = (int)keyPoints.size();
        if ( kpSize > kpThreshold && !isSmall && rect.area() > areaThreshold && (ratio < aspectRatioThreshold || ratio > (1.0 / aspectRatioThreshold)))
            tempNonZeroCountThreshold = 10;
        else
            tempNonZeroCountThreshold = 3;
        return tempNonZeroCountThreshold;
    }

    double ObjectRecognize::SetUniquenessThreshold(bool isSmall)
    {
        if (isSmall)
            return 0.75;
        else
            return 0.7;
    }
    MatchType ObjectRecognize::SetMatchType()
    {
        return BruteForceM;
    }

    DistanceType ObjectRecognize::SetDistanceType()
    {
        return DistanceType::HammingDT;
    }

    void ObjectRecognize::MatchDescriptors(const Mat& modelDescriptors, const Mat& observedDescriptors,int k, CV_OUT vector< vector<DMatch> >& matches)
    {
        if (matchType == FlannM)
        {
            FlannBasedMatcher matcher;
            matcher.knnMatch( modelDescriptors, observedDescriptors, matches, k );
        }
        else
        {
            //comment GPU match code for 2.3 release, will revert it and root cause GPU issue later

            /*   if((DeviceManager::Instance().HasCudaSupport())&&modelDescriptors.rows*observedDescriptors.rows>1000000)
            {
            boost::lock_guard<boost::mutex> lock(TestManager::gpuLock);

            const GpuMat _gpuModelDescriptors(modelDescriptors);
            const GpuMat _observedDescriptors(observedDescriptors);
            BFMatcher_GPU matcher=BFMatcher_GPU((int)distanceType);
            matcher.knnMatch(_gpuModelDescriptors, _observedDescriptors, matches, k);
            }*/
            /* else
            {*/
            BFMatcher matcher = BFMatcher((int)distanceType);
            matcher.knnMatch(modelDescriptors, observedDescriptors, matches, k);
            /* }*/
        }
    }


    void ObjectRecognize::PreprocessAndGetKpDescs(InputArray iObjectImage, bool smallTemp, PreprocessType prepType, vector<KeyPoint>& _keyPoints, OutputArray _descriptors, float scale, FTParameters* otherImageParameters)
    {

        Mat interImage;
        Mat objectImage = iObjectImage.getMat();

        if (smallTemp || this->IsSmallCriterion(objectImage.cols, objectImage.rows))
        {
            isSmall = true;
        }
        else
        {
            isSmall = false;
        }

        switch (prepType)
        {
        case PreprocessType::Blur:
            GaussianBlur(objectImage, interImage, Size(3,3), 0, 0);
            break;
        case PreprocessType::Sharpen:
            interImage = LogSharpen(objectImage);
            break;
        default:
            interImage = objectImage.clone();
            break;
        }
        Mat objectGray;
        if (3 == interImage.channels())
            cvtColor(interImage, objectGray, CV_BGR2GRAY);
        else
            objectGray=interImage.clone();
        assert(1 == objectGray.channels());

        this->GetKpAndDescs(objectGray, isSmall, _keyPoints, _descriptors, otherImageParameters);
        if (NULL == otherImageParameters)
        {
            objectSize = this->ftp->width > this->ftp->height ? this->ftp->width : this->ftp->height;
        }
        else
        {
            objectSize = otherImageParameters->width > otherImageParameters->height ? otherImageParameters->width : otherImageParameters->height;
        }
        if (abs(scale-1.0) > 0.001)
        {
            int keySize = (int)(_keyPoints.size());
            for (int i = 0; i < keySize; ++i)
            {
                _keyPoints[i].pt.x /= scale;
                _keyPoints[i].pt.y /= scale;
            }
        }
    }

    Mat ObjectRecognize::Match(InputArray observedImage, double* pUnmatchRatio)
    {
        return Match(observedImage, 1.0, 1.0, PreprocessType::Keep, pUnmatchRatio, NULL);
    }

    Mat ObjectRecognize::Match(InputArray iObservedImage, float scale, double altRatio, PreprocessType observedPrepType, double* pUnmatchRatio, vector<Point2f>* pPointArray)
    {
        vector<KeyPoint>  observedKeyPoints;
        Mat observedDescriptors;
        Mat observedImage = iObservedImage.getMat();
        FTParameters otherParam(ftp->width, ftp->height, ftp->p1, ftp->p2);
        PreprocessAndGetKpDescs(iObservedImage, isSmall, observedPrepType, observedKeyPoints, observedDescriptors, scale, &otherParam);
        ProcessScaleForObservedImage(observedKeyPoints, scale);
        int observedSize = (observedImage.rows + observedImage.cols) / 2;
        return Match(observedKeyPoints, observedDescriptors, observedSize, altRatio, pUnmatchRatio, pPointArray);
    }

    bool ObjectRecognize:: Init(InputArray iObjectImage,bool smallTemp,float scale, PreprocessType prepType,FTParameters* otherImageParameters)
    {
        matchType=this->SetMatchType();//virtual function
        distanceType = this->SetDistanceType();
        Mat objectImage = iObjectImage.getMat();
        isSmall = smallTemp;
        PreprocessAndGetKpDescs(iObjectImage, isSmall, prepType, keyPoints, descriptors, scale, otherImageParameters);
        if (otherImageParameters != NULL)
        {
            ftp=boost::shared_ptr<FTParameters>(new FTParameters(*otherImageParameters));
        }
        imageSize =  Size((int)(objectImage.cols / scale), (int)(objectImage.rows / scale));//why not use cols directly
        size = (objectImage.cols + objectImage.cols ) / 2;
        return true;
    }


    /// <summary>
    /// Try to match this object in given image with 1:1 scale
    /// </summary>
    /// <param name="observedKeyPoints">observed image key points </param>
    /// <param name="observedDescriptors">observed image key points  descriptors</param>
    /// <param name="observedSize">observed image size</param>
    /// <param name="pointArray">returns a point array which contains a set of points recognized in observed image</param>
    /// <param name="unmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
    /// <param name="altRatio">Alternative aspectRatio of the image that this observed image could be recognized, as scale if X dimension</param>
    /// <returns>the homography matrix of the detection.</returns>
    Mat ObjectRecognize::Match(const vector<KeyPoint>& observedKeyPoints, const Mat& observedDescriptors, int observedSize, double altRatio, double* pUnmatchRatio, vector<Point2f>* pPointArray)
    {
        if (descriptors.dims==0 || keyPoints.size()==0 || observedDescriptors.dims==0 || observedKeyPoints.size() == 0)
        {
            //DAVINCI_LOG_WARNING<<"Warning in FeatureMatcher.Match: invalid parameters";
            if(pUnmatchRatio!=NULL)
            {
                *pUnmatchRatio = 1.0;
            }
            return Mat();
        }

        nonZeroCountThreshold = this->NonZeroThresholdCriterion(isSmall);
        homography = Mat();
        pts = vector<Point2f>();
        int k = 2;
        double uniquenessThreshold = this->SetUniquenessThreshold(isSmall);
        if (pPointArray!=NULL)
            (*pPointArray).clear();
        if (pUnmatchRatio!=NULL)
            *pUnmatchRatio = 1.0;

#ifdef _DEBUG
        int64 tick = 0;
        tick = getTickCount();
#endif

        Mat indices =Mat(descriptors.rows, k, CV_32SC1);
        vector< vector<DMatch> > matches;
        this->MatchDescriptors(descriptors, observedDescriptors, k,matches);
        for (int i=0; i < indices.rows; ++i)
            for (int j=0; j < k; ++j)
            {
                int matchSize = (int)(matches[i].size());
                if (matchSize > j)
                    indices.at<int>(i,j) = matches[i][j].trainIdx;
                else
                {
                    DMatch dmatch;
                    dmatch.distance = 1000;
                    matches[i].push_back(dmatch);
                }
            }
            Mat mask = Mat::ones((int)(matches.size()), 1, CV_8UC1);
            VoteForUniqueness(matches,uniquenessThreshold, mask);
            int nonZeroCount = countNonZero(mask);
            int observedDescriptorsCount = observedDescriptors.rows;
            Mat indices2 = Mat::zeros(observedDescriptorsCount, k , CV_32SC1);
            Mat mask2 = Mat::zeros(indices2.rows, 1, CV_8UC1);

            //update  TestManager.UpdateDebugMessage("non zero count = " + nonZeroCount + ", threshold = " + nonZeroCountThreshold);
            if (nonZeroCount >= nonZeroCountThreshold)
            {
                bool matched = DictateSizeAndOrientation(observedKeyPoints, keyPoints, indices, mask,
                    min(size, observedSize) / 20.0, altRatio);
                if (matched)
                {
                    //mask2.SetValue(0);
                    Mat blank_red = Mat::zeros(imageSize, CV_8UC3);
                    Mat blank_green = Mat::zeros(imageSize, CV_8UC3);
                    Mat blank = Mat::zeros(imageSize, CV_8UC3);


                    for (int i = 0; i < indices.rows; i++)
                    {
                        if (mask.at<uchar>(i, 0) == 0)
                            circle(blank_green, keyPoints[i].pt, 2, Green, size/20);
                    }
                    for (int i = 0; i < indices.rows; i++)
                    {
                        if (mask.at<uchar>(i, 0) != 0)
                        {
                            circle(blank_red, keyPoints[i].pt, 2, Red, size/20);
                            indices2.at<int>(indices.at<int>(i,0), 0) = i;
                            mask2.at<uchar>(indices.at<int>(i,0), 0) = 255;
                        }
                    }

                    if (pUnmatchRatio != NULL)
                    {
                        int dialateSize = 20;
                        if (dialateSize > observedSize / 5.0)
                        {
                            dialateSize = (int)(observedSize / 5.0);
                        }
                        Mat blank_green_dilate, blank_red_dilate;
                        dilate(blank_green, blank_green_dilate, Mat(), Point(-1, -1), dialateSize);
                        dilate(blank_red, blank_red_dilate, Mat(), Point(-1, -1), dialateSize);

                        int greenCount = 0, redCount = 0;
                        uchar* blank_green_data = blank_green_dilate.data;
                        uchar* blank_red_data = blank_red_dilate.data;
                        uchar* blank_data = blank.data;

                        int width = blank_green_dilate.cols;
                        int height = blank_green_dilate.rows;
                        /*		Vec3b redVec;
                        Vec3b blankVec;
                        Vec3b greenVec;*/
                        for (int c = 0; c < width; ++c)
                            for (int r = 0; r < height; ++r)
                            {
                                uchar* redVal = &blank_red_data[r*blank_red_dilate.step + c*3];
                                uchar* blankVal = &blank_data[r*blank.step + c*3];
                                uchar* greenVal = &blank_green_data[r*blank_red_dilate.step + c*3];
                                if (redVal[2] != 0)
                                {
                                    ++redCount;
                                    blankVal[2] = redVal[2];
                                }
                                else
                                {

                                    if(greenVal[1] != 0)
                                    {
                                        ++greenCount;
                                    }
                                    blankVal[1]= greenVal[1];
                                }
                            }

                            if(unmatchRatioType == UnmatchRatioType::UnmatchOverWholeFeature)
                            {
                                *pUnmatchRatio = greenCount * 1.0 / (greenCount + redCount);
                            }
                            else
                            {
                                *pUnmatchRatio = greenCount*1.0 / (imageSize.area());
                            }

#ifdef _DEBUG
                            DAVINCI_LOG_DEBUG << "Unmatch ratio: " << *pUnmatchRatio;
                            imwrite("result_green.png", blank_green_dilate);
                            imwrite("result_red.png", blank_red_dilate);
                            imwrite("result.png", blank);
#endif
                    }
                    // TestManager.UpdateDebugMessage("Feature Matcher hit.");
                    if (!isSmall)
                        homography = GetHomographyMatrixFromMatchedFeatures(keyPoints, observedKeyPoints, indices2, mask2, 2);
                    else
                    {
                        // in small object recognition, Feature2DToolbox is not
                        if (maxX == minX) maxX = minX + 1;
                        if (maxY == minY) maxY = minY + 1;
                        homography = Mat(3, 3, CV_64F);
                        //	homography =  (Mat_<double>(3,3) << 0.0, -1.0, 0.0, -1.0, 5.0f, -1.0, 0.0, -1.0f, 0.0f);
                        homography.at<double>(0, 0) = (double)(maxX - minX) / imageSize.width;
                        homography.at<double>(0, 1) = 0;
                        homography.at<double>(0, 2) = minX;

                        homography.at<double>(1, 0) = 0;
                        homography.at<double>(1, 1) = (double)(maxY - minY) / imageSize.height;
                        homography.at<double>(1, 2) = minY;

                        homography.at<double>(2, 0) = 0;
                        homography.at<double>(2, 1) = 0;
                        homography.at<double>(2, 2) = 1;
                    }

                    float left = 0.0f;
                    float right = (float)imageSize.width;
                    float top = 0.0f;
                    float bottom = (float) imageSize.height;
                    pts.push_back(Point2f(left, bottom));
                    pts.push_back(Point2f(right, bottom));
                    pts.push_back(Point2f(right, top));
                    pts.push_back(Point2f(left, top));
                    pts.push_back(Point2f((left + right) / 2 , (top + bottom) / 2));
                    pts.push_back(Point2f(right, top));
                    pts.push_back(Point2f(right, bottom));
                    pts.push_back(Point2f((left + right)/2, (top + bottom) / 2));
                    pts.push_back(Point2f(left, bottom));
                    pts.push_back(Point2f(left, top));

                    if (homography.rows==3 && homography.cols==3)
                    {
                        ProjectPoints(pts, homography);
                    }

                    if (pPointArray!=NULL)
                    {
                        (*pPointArray).reserve(nonZeroCount);
                        int idx = 0;
                        int observedSize = (int)(observedKeyPoints.size());
                        for (int i = 0; i < observedSize; i++)
                        {
                            if (mask2.at<uchar>(i, 0) == 0) continue;
                            (*pPointArray).push_back(observedKeyPoints[i].pt);//bug
                            idx++;
                        }
                    }
                }
            }

#ifdef _DEBUG

            //int64 tock = getTickCount();
            //double mTime =(double)((tock-tick)*1000/getTickFrequency());
            //TestManager.UpdateDebugMessage("detect took " + (tock - tick) / (double)(TimeSpan.TicksPerSecond) + " seconds");

# endif

            if ((homography.rows==0 || homography.cols==0) && pUnmatchRatio!=NULL) 
            {
                *pUnmatchRatio = 1.0; // make two pictures totally unmatch
            }
            return homography;
    }


    void ObjectRecognize::ProcessScaleForObservedImage(vector<KeyPoint>& observedKeyPoints,  float scale)
    {
        if (scale <= 0 || abs(scale-1.0)<0.00001)
        {
            return;
        }
        int observedSize = (int)(observedKeyPoints.size());
        for (int i = 0; i < observedSize; ++i)
        {
            observedKeyPoints[i].pt.x /= scale;
            observedKeyPoints[i].pt.y /= scale;
        }
    }

    bool ObjectRecognize::DictateSizeAndOrientation(const vector<KeyPoint>& modelKeyPoints, const vector<KeyPoint>& observedKeyPoints,
        const Mat& indices, Mat& mask, double distance_threshold, double alt_ratio)
    {
        assert(mask.type()==CV_8UC1 && indices.type()==CV_32SC1);
        if (isSmall)
        {
            int i=0, j=0;
            int* nearNeighbourCount = new int[observedKeyPoints.size()]();//改成shared,确认下是否都为0，todo
            memset(nearNeighbourCount,0,observedKeyPoints.size()*sizeof(int));
            for (i = 0; i < mask.rows; i++)
            {
                if (mask.at<uchar>(i, 0) == 0) continue;
                for (j = i+1; j < mask.rows; j++)
                {
                    if (mask.at<uchar>(j, 0) == 0) continue;
                    if (j == i) continue;
                    Point2f observed_i=modelKeyPoints[indices.at<int>(i,0)].pt;
                    Point2f observed_j = modelKeyPoints[indices.at<int>(j, 0)].pt;
                    double observed_x_diff = observed_i.x - observed_j.x;
                    double observed_y_diff = observed_i.x - observed_j.x;
                    double observed_distance = sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                    if (observed_distance < objectSize * 2)
                    {
                        ++nearNeighbourCount[i];
                        ++nearNeighbourCount[j];
                    }
                }
            }
            int nonZeroCount = countNonZero(mask);
            int maxCount = 0;
            int max_i = -1;
            //String str = "";
            for (i = 0; i < mask.rows; i++)
            {
                if (mask.at<uchar>(i, 0) == 0) continue;
                //str = str + nearNeighbourCount[i] + " ";
                if (nearNeighbourCount[i] <= nonZeroCount / 2) 
                    mask.at<uchar>(i, 0) = 0;
                else if (nearNeighbourCount[i] > maxCount)
                {
                    maxCount = nearNeighbourCount[i];
                    max_i = i;
                }
            }

            if (maxCount > 0)
            {
                i = max_i;
                minX = maxX = (int)(modelKeyPoints[indices.at< int>(i,0)].pt.x);
                minY = maxY =(int)( modelKeyPoints[indices.at< int>(i,0)].pt.y);
                for (j = 0; j < mask.rows; j++)
                {
                    if (mask.at<uchar>(j, 0) == 0) continue;
                    if (j == i) continue;
                    Point2f observed_i = modelKeyPoints[indices.at<int>(i,0)].pt;
                    Point2f observed_j = modelKeyPoints[indices.at<int>(j,0)].pt;
                    double observed_x_diff = observed_i.x - observed_j.x;
                    double observed_y_diff = observed_i.y - observed_j.y;
                    double observed_distance = sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                    if (observed_distance < objectSize * 2)
                    {
                        if (minX > (int)observed_j.x) minX = (int)observed_j.x;
                        if (maxX < (int)observed_j.x) maxX = (int)observed_j.x;
                        if (minY > (int)observed_j.y) minY = (int)observed_j.y;
                        if (maxY < (int)observed_j.y) maxY = (int)observed_j.y;
                    }
                }
            }
            //TestManager.UpdateDebugMessage(str);
            delete[] nearNeighbourCount;
            nearNeighbourCount=NULL;
            return maxCount != 0;
        }
        else
        {
            const int degreeMapLength = 90+1;
            const int cedaMapLength=180+1;
            int degreeMap[degreeMapLength]={};
            int cedaMap[cedaMapLength]={};
            if (abs(alt_ratio - 1.0) < 1e-3)
                return DictateSizeAndOrientationInner(modelKeyPoints, observedKeyPoints, indices, mask, distance_threshold, degreeMap,degreeMapLength, cedaMap,cedaMapLength, 1.0);
            else
            {
                return DictateSizeAndOrientationInner(modelKeyPoints, observedKeyPoints, indices, mask, distance_threshold, degreeMap,degreeMapLength, cedaMap,cedaMapLength, 1.0)
                    || DictateSizeAndOrientationInner(modelKeyPoints, observedKeyPoints, indices,  mask, distance_threshold, degreeMap, degreeMapLength,cedaMap,cedaMapLength, alt_ratio)
                    || DictateSizeAndOrientationInner(modelKeyPoints, observedKeyPoints, indices,  mask, distance_threshold, degreeMap, degreeMapLength,cedaMap,cedaMapLength, 1.0 / alt_ratio);
            }
        }
    }

    bool ObjectRecognize::DictateSizeAndOrientationInner(const vector<KeyPoint>& modelArray, const vector<KeyPoint>& observedArray,
        const Mat& indices, Mat& mask, double distance_threshold, int* degreeMap,int degreeMapLength ,int* cedaMap, int cedaMapLength,double alt_ratio)
    {
        assert(mask.type()==CV_8UC1&& indices.type()==CV_32SC1);
        vector<KeyPoint> observedArrayNew;
        if (abs(alt_ratio - 1.0) < 1e-3)
        {
            observedArrayNew = observedArray;
        }
        else
        {
            observedArrayNew = observedArray;
            int observedSize = (int)(observedArray.size());
            for (int i = 0; i < observedSize; i++)
                observedArrayNew[i].pt.x = (float)(observedArrayNew[i].pt.x * alt_ratio + 0.5);
        }


        int degree_max, degree_begin, degree_end, ceda_max;
        bool degree_spike, ceda_spike;

        ProcessPtPairsScale(modelArray, observedArrayNew, indices,mask, degreeMap, distance_threshold);

        degree_spike = Detect_spike(degreeMap, degreeMapLength,  degree_max, degree_begin,  degree_end);
        //TestManager.UpdateDebugMessage("degree_spike (" + alt_ratio + "): " + degree_spike);
        PostProcessPtPairsScale(modelArray, observedArrayNew, indices, mask, degreeMap, distance_threshold,
            degree_begin, degree_end);
        // cedaMap = new int[180 + 1];
        ProcessPtPairsOrientation(modelArray, observedArrayNew, indices, mask, cedaMap, distance_threshold);
        ceda_spike = Detect_spike(cedaMap, cedaMapLength,  ceda_max,  degree_begin, degree_end);
        // TestManager.UpdateDebugMessage("ceda_spike (" + alt_ratio + "): " + ceda_spike);
        PostProcessPtPairsOrientation(modelArray, observedArrayNew, indices, mask,  cedaMap, distance_threshold,
            degree_begin, degree_end);
#ifdef _DEBUG
        DAVINCI_LOG_DEBUG << "degree_spike=" << degree_spike << ", ceda_spike=" << ceda_spike;
#endif
        return degree_spike && ceda_spike;
    }

    void ObjectRecognize::ProcessPtPairsScale(const vector<KeyPoint>& modelArray, const vector<KeyPoint>& observedArray,
        const Mat& indices, Mat& mask,  int* degreeMap, double distance_threshold)
    {
        assert(mask.type()==CV_8UC1&& indices.type()==CV_32SC1);
        int i, j;
        for (i = 0; i < mask.rows - 1; i++)
        {
            if (mask.at<uchar>(i, 0) == 0) continue;
            // for (j = i + 1; j < mask.rows && j < i + PAIR_FACTOR; j++)
            for (j = i + 1; j < mask.rows; j++)
            {
                if (mask.at<uchar>(j, 0) == 0) continue;
                Point2f model_i = modelArray[indices.at<int>(i, 0)].pt;
                Point2f model_j = modelArray[indices.at<int>(j, 0)].pt;
                Point2f observed_i = observedArray[i].pt;
                Point2f observed_j = observedArray[j].pt;
                double model_x_diff = model_i.x - model_j.x;
                double model_y_diff = model_i.y - model_j.y;
                double observed_x_diff = observed_i.x - observed_j.x;
                double observed_y_diff = observed_i.y - observed_j.y;
                double model_distance = sqrt(model_x_diff * model_x_diff + model_y_diff * model_y_diff);
                double observed_distance = sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                if (model_distance < distance_threshold || observed_distance < distance_threshold) continue;
                double degree = atan2(observed_distance, model_distance);
                int idegree = (int)(degree / 3.14159 * 180);
                degreeMap[idegree]++;
            }
        }
    }

    //the current mechanism is to check the 10 bins around the max bin, if the numbers in this 10 bins exceeds 0.7*all,return true;  
    bool ObjectRecognize::Detect_spike(int* arr, int arr_len, int& max_p,  int& begin, int& end)
    {
        int gapThreshold = 10;
        int i_max = 0;
        int i;
        //int run_length = 0;
        int total = arr[0];
        max_p = begin = end = 0;
        for (i = 1; i < arr_len; i++)
        {
            if (arr[i] > arr[i_max]) i_max = i;
            total += arr[i];
        }
        // TestManager.UpdateDebugMessage("Detect spike: total = " + total + ", arr[i_max] = " + arr[i_max]);
        if (total < 3) return false;
        if (arr[i_max] == 0)
        {
            return false;
        }
        begin = max(i_max-gapThreshold,0);
        end=min(i_max+gapThreshold,arr_len-1);
        int rightTotal = 0;
        for(int i=begin;i<=end;++i)
        {
            rightTotal+=arr[i];
        }
        double rightRatio = (double)rightTotal/(total);
        max_p = i_max;
        if(rightRatio>0.7||rightTotal>1000) return true;
        return false;
    }

    void ObjectRecognize::PostProcessPtPairsScale(const vector<KeyPoint>& modelArray, const vector<KeyPoint>& observedArray,
        const Mat& indices, Mat& mask, int* degreeMap, double distance_threshold,
        int degree_begin, int degree_end)
    {
        assert(mask.type()==CV_8UC1&& indices.type()==CV_32SC1);
        int i, j;
        Mat maskClone=Mat::zeros(mask.rows,mask.cols,mask.type());
        for (i = 0; i < mask.rows - 1; i++)
        {
            if (mask.at<uchar>(i, 0) == 0) continue;
            for (j = i + 1; j < mask.rows; j++)
                // for (j = i + 1; j < mask.rows && j < i + PAIR_FACTOR; j++)
            {
                if (mask.at<uchar>(j, 0) == 0) continue;

                Point2f model_i = modelArray[indices.at<int>(i, 0)].pt;
                Point2f model_j = modelArray[indices.at<int>(j, 0)].pt;
                Point2f observed_i = observedArray[i].pt;
                Point2f observed_j = observedArray[j].pt;
                double model_x_diff = model_i.x - model_j.x;
                double model_y_diff = model_i.y - model_j.y;
                double observed_x_diff = observed_i.x - observed_j.x;
                double observed_y_diff = observed_i.y - observed_j.y;
                double model_distance = sqrt(model_x_diff * model_x_diff + model_y_diff * model_y_diff);
                double observed_distance = sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                if (model_distance < distance_threshold || observed_distance < distance_threshold) continue;
                double degree = atan2(observed_distance, model_distance);
                int idegree = (int)(degree / 3.14159 * 180);
                if (idegree >= degree_begin && idegree <= degree_end)
                {
                    maskClone.at<uchar>(i, 0) = 255;
                    maskClone.at<uchar>(j, 0) = 255;
                }
            }
        }
        for (i = 0; i < mask.rows; i++)
            for (j = 0; j < mask.cols; j++)
            {
                mask.at<uchar>(i, j) = maskClone.at<uchar>(i, j);
            }     
    }

    void ObjectRecognize::ProcessPtPairsOrientation(const vector<KeyPoint>& modelArray, const vector<KeyPoint>& observedArray,
        const Mat& indices, Mat& mask, int* degreeMap, double distance_threshold)
    {
        assert(mask.type()==CV_8UC1&& indices.type()==CV_32SC1);
        int i, j;
        for (i = 0; i < mask.rows - 1; i++)
        {
            if (mask.at<uchar>(i, 0) == 0) continue;
            for (j = i + 1; j < mask.rows; j++)
                //  for (j = i + 1; j < mask.rows && j < i + PAIR_FACTOR; j++)
            {
                if (mask.at<uchar>(j, 0) == 0) continue;

                Point2f model_i = modelArray[indices.at<int>(i, 0)].pt;
                Point2f model_j = modelArray[indices.at<int>(j, 0)].pt;
                Point2f observed_i = observedArray[i].pt;
                Point2f observed_j = observedArray[j].pt;
                double model_x_diff = model_i.x - model_j.x;
                double model_y_diff = model_i.y - model_j.y;
                double observed_x_diff = observed_i.x - observed_j.x;
                double observed_y_diff = observed_i.y - observed_j.y;
                double model_distance = sqrt(model_x_diff * model_x_diff + model_y_diff * model_y_diff);
                double observed_distance =sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                if (model_distance < distance_threshold || observed_distance < distance_threshold) continue;
                double acos_value = (model_x_diff * observed_x_diff + model_y_diff * observed_y_diff)
                    / (model_distance * observed_distance);
                if (acos_value > 1) acos_value = 1.0;
                else if (acos_value < -1) acos_value = -1.0;
                double ceda = acos(acos_value);
                int iceda = (int)(ceda / 3.14159 * 180);
                degreeMap[iceda]++;
            }
        }
    }


    void  ObjectRecognize::PostProcessPtPairsOrientation(const vector<KeyPoint> modelArray, const vector<KeyPoint> observedArray,
        const Mat& indices, Mat& mask, int* degreeMap, double distance_threshold,
        int degree_begin, int degree_end)
    {
        assert(mask.type()==CV_8UC1&& indices.type()==CV_32SC1);
        int i, j;
        Mat maskClone=Mat::zeros(mask.rows,mask.cols,mask.type());

        for (i = 0; i < mask.rows - 1; i++)
        {
            if (mask.at<uchar>(i, 0) == 0) continue;
            // for (j = i + 1; j < mask.rows && j < i + PAIR_FACTOR; j++)
            for (j = i + 1; j < mask.rows; j++)
            {
                if (mask.at<uchar>(j, 0) == 0) continue;

                Point2f model_i = modelArray[indices.at<int>(i, 0)].pt;
                Point2f model_j = modelArray[indices.at<int>(j, 0)].pt;
                Point2f observed_i = observedArray[i].pt;
                Point2f observed_j = observedArray[j].pt;
                double model_x_diff = model_i.x - model_j.x;
                double model_y_diff = model_i.y - model_j.y;
                double observed_x_diff = observed_i.x - observed_j.x;
                double observed_y_diff = observed_i.y - observed_j.y;
                double model_distance = sqrt(model_x_diff * model_x_diff + model_y_diff * model_y_diff);
                double observed_distance = sqrt(observed_x_diff * observed_x_diff + observed_y_diff * observed_y_diff);
                if (model_distance < distance_threshold || observed_distance < distance_threshold) continue;
                double ceda = acos((model_x_diff * observed_x_diff + model_y_diff * observed_y_diff)
                    / (model_distance * observed_distance));
                int iceda = (int)(ceda / 3.14159 * 180);
                if (iceda >= degree_begin && iceda <= degree_end)
                {
                    maskClone.at<uchar>(i, 0) = 255;
                    maskClone.at<uchar>(j, 0) = 255;
                }
            }
        }
        for (i = 0; i < mask.rows; i++)
            for (j = 0; j < mask.cols; j++)
            {
                mask.at<uchar>(i, j) = maskClone.at<uchar>(i, j);
            }

    }

    bool  ObjectRecognize::CvGetHomographyMatrixFromMatchedFeatures(const vector<cv::KeyPoint>& model, const vector<cv::KeyPoint>& observed, const Mat& indices,const Mat& mask, double randsacThreshold, Mat& homography)
    {
        Mat maskClone;
        if (mask.rows==0 || mask.cols==0)
        {

            maskClone = Mat::ones(indices.rows, 1, CV_8UC1);
        }
        else
        {
            maskClone = mask.clone();
        }

        if (countNonZero(maskClone) < 4)
            return false;
        vector<Point2f> srcPtVec;
        vector<Point2f> dstPtVec;
        for (int i = 0; i < maskClone.rows; ++i)
        {
            if (maskClone.at<uchar>(i))
            {
                int modelIdx = indices.at<int>(i,0);
                srcPtVec.push_back(model[modelIdx].pt);
                dstPtVec.push_back(observed[i].pt);
            }
        }
        std::vector<uchar> ransacMask;
        cv::Mat result = cv::findHomography(cv::Mat(srcPtVec), cv::Mat(dstPtVec), cv::RANSAC, randsacThreshold, ransacMask);
        if (result.empty())
        {
            return false;
        }
        homography = result;
        int idx = 0;
        for (int i = 0; i < maskClone.rows; i++)
        {
            uchar* val = maskClone.ptr<uchar>(i);
            if (*val)
                *val = ransacMask[idx++];
        }
        return true;

    }

    Mat ObjectRecognize::GetHomographyMatrixFromMatchedFeatures( const vector<KeyPoint>& model, const vector<KeyPoint>& observed, const Mat& matchIndices, const Mat&mask, double ransacReprojThreshold)
    {
        Mat homography = Mat();
        bool found =CvGetHomographyMatrixFromMatchedFeatures(model, observed, matchIndices, mask, ransacReprojThreshold, homography);
        if (found)
        {
            return homography;
        }
        else
        {
            return Mat();
        }
    }

    boost::shared_ptr<FTParameters> ObjectRecognize::GetDefaultCurrentMatchPara()
    {
        boost::shared_ptr<FTParameters> para = boost::shared_ptr<FTParameters>(new FTParameters(1,1,0.00001f,1.0));
        return para;
    }

    void ObjectRecognize::SetUnmatchRatioType(UnmatchRatioType type)
    {
        unmatchRatioType = type;
    }
}

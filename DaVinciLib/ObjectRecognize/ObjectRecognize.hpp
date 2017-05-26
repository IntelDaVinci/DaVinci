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

#ifndef __DAVINCI_OBJECTRECOGNIZE_HPP__
#define __DAVINCI_OBJECTRECOGNIZE_HPP__
//#include "./opencv/include/opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/features2d/features2d.hpp"
#include <iostream>
#include "opencv2/core/core.hpp"
#include <vector>
#include "DaVinciCommon.hpp"
#include "TestManager.hpp"
#include "DeviceManager.hpp"

namespace DaVinci
{
    /// <summary> Values that represent patch-based feature detection algorithm. </summary>
    enum class PatchFeatureAlgorithm
    {
        /// <summary> Auto select the algorithm to use </summary>
        Auto,
        /// <summary> AKAZE </summary>
        Akaze,
        /// <summary> ORB </summary>
        Orb
    };
    enum class UnmatchRatioType
    {
        UnmatchOverWholeFeature=0,
        UnmatchOverWholeImage=1
    };
    //different 
    //for AKAZE:p1-threshold,p2-scaleType
    struct FTParameters
    {  
        int width;//just for out; maybe not corresponding to the right image 
        int height;//just for out.
        /// <summary>P1  </summary>
        float p1;
        /// <summary>P2  </summary>
        float p2;
        FTParameters(int,int,float,float);
    };

    /// <summary>
    /// The type of image preprocessing before applying FeatureMatcher
    /// </summary>
    enum class PreprocessType
    {
        ///<summary>Keep the original image without preprocessing</summary>
        Keep,
        ///<summary>Blur the image</summary>
        Blur,
        ///<summary>Sharpen the image</summary>
        Sharpen
    };//end of enum

    enum  class DistanceType
    {
        /// <summary>
        /// 
        /// </summary>
        InfDT = 1,
        /// <summary>
        /// Manhattan distance (city block distance)
        /// </summary>
        L1DT = 2,
        /// <summary>
        /// Squared Euclidean distance
        /// </summary>
        L2DT = 4, 
        /// <summary>
        /// Euclidean distance
        /// </summary>
        L2SqrDT = 5,
        /// <summary>
        /// Hamming distance functor - counts the bit differences between two strings - useful for the Brief descriptor, 
        /// bit count of A exclusive XOR'ed with B. 
        /// </summary>
        HammingDT = 6,
        /// <summary>
        /// Hamming distance functor - counts the bit differences between two strings - useful for the Brief descriptor, 
        /// bit count of A exclusive XOR'ed with B. 
        /// </summary>
        Hamming2DT = 7, //TODO: update the documentation
        /*
        TypeMask = 7, 
        Relative = 8, 
        MinMax = 32 */
    };

    enum  MatchType
    {
        BruteForceM=0,
        FlannM = 1,
    };

     /// <summary>the inner base class for matching, but you should call FeatureMatcher if you want to match two images</summary>
    class ObjectRecognize
    {
    public:
        /// <summary> Creates object recognize. </summary>
        /// <param name="algo"> (Optional) The algorithm to detect feature points and descriptors </param>
        /// <returns> The new object recognize. </returns>
        static boost::shared_ptr<ObjectRecognize> CreateObjectRecognize(PatchFeatureAlgorithm algo = PatchFeatureAlgorithm::Auto);

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
        cv::Mat Match(const cv::vector<cv::KeyPoint>& observedKeyPoints, const cv::Mat& observedDescriptors, int observedSize, double altRatio=1.0, double* pUnmatchRatio=NULL, vector<cv::Point2f>* pPointArray=NULL );

        /// <summary>
        /// Detect this object in given image with given scale
        /// </summary>
        /// <param name="observedImage">The image in which we want to detect object</param>
        /// <param name="scale">The scale of the observedImage, compares to the original image.  0.5 means half size of original image</param>
        /// <param name="pointArray">returns a point array which contains a set of points recognized in observed image</param>
        /// <param name="unmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
        /// <param name="altRatio">Alternative aspectRatio of the image that this observed image could be recognized, as scale if X dimension</param>
        /// <param name="observedPrepType">Pre Image Processing Type.</param>
        /// <returns>the homography matrix of the detection.</returns>
        cv::Mat Match(cv::InputArray observedImage, float scale=1.0, double altRatio = 1.0, PreprocessType observedPrepType =PreprocessType::Keep, double*  pUnmatchRatio=NULL, vector<cv::Point2f>* pPointArray=NULL);

        /// <summary>
        /// Detect this object in given image with given scale
        /// </summary>
        /// <param name="observedImage">The image in which we want to detect object</param>
        /// <param name="unmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
        /// <returns>the homography matrix of the detection.</returns>
        cv::Mat Match(cv::InputArray observedImage, double* unmatchRatio);
         /// <summary>
        /// Set UnmatchRatio  calculation type
        /// </summary>
        /// <param name="type">the enum index of type </param>
        void SetUnmatchRatioType(UnmatchRatioType type);
        /// <summary>
        /// Constructor
        /// </summary>
        /// <returns></returns>
        ObjectRecognize();

        /// <summary>
        /// Destructor
        /// </summary>
        /// <returns></returns>
        virtual ~ObjectRecognize();

        friend class FeatureMatcher;

    protected:

        /// <summary>
        /// Get keyPoint and descriptors of an image
        /// </summary>
        /// <param name="objectImage"></param>
        /// <param name="small"></param>
        /// <param name="prepType"></param>
        /// <param name="_keyPoints"></param>
        /// <param name="_descriptors"></param>
        virtual void PreprocessAndGetKpDescs(cv::InputArray objectImage, bool isSmall,PreprocessType prepType,  std::vector<cv::KeyPoint>& _keyPoints,cv::OutputArray _descriptors,float scale=1.0,FTParameters* otherImageParameters=NULL);

        /// <summary>
        /// The Criterion of isSmall
        /// </summary>
        /// <param name="width">the width of source image</param>
        /// <param name="height">the height of source image</param>
        /// <returns></returns>
        virtual bool IsSmallCriterion(int width, int height);

        /// <summary>
        /// nonZero Threshold Criterion, it controls the keyPoints' threshold  to calculate H
        /// </summary>
        /// <param name="isSmall">isSmall flag of the source</param>
        /// <returns></returns>
        virtual int NonZeroThresholdCriterion(bool isSmall);

        /// <summary>
        /// Get keypoints and descriptors
        /// </summary>
        /// <param name="grayImage">the input gray image</param>
        /// <param name="isSmall">the index whether the image is small, is it's small, the threshold</param>
        /// <param name="keyPoints"></param>
        /// <param name="descriptors"></param>
        ///<param name="pram"></param>
        virtual void GetKpAndDescs(cv::InputArray grayImage,  bool isSmall, CV_OUT std::vector<cv::KeyPoint>& keypoints, CV_OUT cv::OutputArray descriptors,FTParameters* pram=NULL)=0;

        /// <summary>
        ///  Match two sets of descripotrs
        /// </summary>
        /// <param name="modelDescriptors">the descriptors of modle image</param>
        /// <param name="observedDescriptors">the descriptors of observed image</param>
        /// <param name="matches">the match result</param>
        /// <param name="k">how many proposed matches for one descriptor</param>
        virtual void MatchDescriptors(const cv::Mat& modelDescriptors, const cv::Mat& observedDescriptors,int k, CV_OUT std::vector<std::vector<cv::DMatch> >& matches);

        /// <summary>
        ///  Preporcess the image like sharepen or blur,and converte the image to gray
        /// </summary>
        /// <param name="inputImage">the input image</param>
        /// <param name="outputImage">the output image</param>
        /// <param name="prepType">the type of preprocess</param>
        void PreprocessAndGray(cv::InputArray inputImage,cv::OutputArray outputImage,PreprocessType prepType);

        /// <summary>
        ///  Get the current match parameter,including threshold,scale value
        /// </summary>
        /// <param name="prepType">the type of preprocess</param>
        /// <returns>the current match parameter</returns>
        virtual boost::shared_ptr<FTParameters> GetDefaultCurrentMatchPara();

        bool isSmall;

        int size;

        std::vector<cv::KeyPoint> keyPoints;

        cv::Mat descriptors;

        boost::shared_ptr<FTParameters> ftp;

    private:
        UnmatchRatioType unmatchRatioType;

        cv::Mat homography;

        int objectSize;

        cv::Size imageSize;

        int nonZeroCountThreshold;

        std::vector<cv::Point2f> pts;

        MatchType matchType;

        DistanceType distanceType;

        int minX, minY, maxX, maxY;
        
        static int  kpThreshold;
        static double aspectRatioThreshold;
        static int areaThreshold;

        void ProcessScaleForObservedImage(std::vector<cv::KeyPoint>& observedKeyPoints,  float scale);

        bool DictateSizeAndOrientation(const std::vector<cv::KeyPoint>& modelKeyPoints, const std::vector<cv::KeyPoint>& observedKeyPoints,
            const cv::Mat& indices, cv::Mat& mask, double distance_threshold, double alt_ratio);

        bool virtual Init(cv::InputArray objectImage,bool isSmall = false, float scale=1.0,PreprocessType prepType=PreprocessType::Keep,FTParameters* otherImageParameters=NULL);

        static void ProcessPtPairsScale(const std::vector<cv::KeyPoint>& modelArray, const std::vector<cv::KeyPoint>& observedArray,
            const cv::Mat& indices, cv::Mat& mask,  int* degreeMap, double distance_threshold);

        bool DictateSizeAndOrientationInner(const std::vector<cv::KeyPoint>& modelArray, const std::vector<cv::KeyPoint>& observedArray,
            const cv::Mat& indices, cv::Mat& mask, double distance_threshold, int* degreeMap,int degreeMapLength ,int* cedaMap, int cedaMapLength,double alt_ratio);

        static bool Detect_spike(int* arr, int arr_len, int& max_p,  int& begin, int& end);

        void PostProcessPtPairsScale(const std::vector<cv::KeyPoint>& modelArray, const std::vector<cv::KeyPoint>& observedArray,
            const cv::Mat& indices, cv::Mat& mask, int* degreeMap, double distance_threshold,int degree_begin, int degree_end);

        static void PostProcessPtPairsOrientation(const std::vector<cv::KeyPoint> modelArray, const std::vector<cv::KeyPoint> observedArray,
            const cv::Mat& indices, cv::Mat& mask, int* degreeMap, double distance_threshold,int degree_begin, int degree_end);

        static void ProcessPtPairsOrientation(const std::vector<cv::KeyPoint>& modelArray, const std::vector<cv::KeyPoint>& observedArray,
            const cv::Mat& indices, cv::Mat& mask, int* degreeMap, double distance_threshold);

        cv::Mat GetHomographyMatrixFromMatchedFeatures( const std::vector<cv::KeyPoint>& model, const std::vector<cv::KeyPoint>& observed, const cv::Mat& matchIndices, const cv::Mat&mask, double ransacReprojThreshold);

        virtual DistanceType  SetDistanceType();

        virtual MatchType SetMatchType();

        virtual double SetUniquenessThreshold(bool isSmall);

        bool  CvGetHomographyMatrixFromMatchedFeatures(const std::vector<cv::KeyPoint>& model, const std::vector<cv::KeyPoint>& observed, const cv::Mat& indices,const cv::Mat& mask, double randsacThreshold, cv::Mat& homography);
    };//end of class

}//end of namesapce 
#endif

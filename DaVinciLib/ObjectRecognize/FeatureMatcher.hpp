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

#ifndef __DAVINCI_FEATUREMATCHER_HPP__
#define __DAVINCI_FEATUREMATCHER_HPP__
#include "ObjectRecognize.hpp"
#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    /// <summary> the interface to match two images,it can utilize diffirent features to match </summary>
    class FeatureMatcher
    {
    public:
        /// <summary>Constructor. </summary>
        /// <param name="pObjectRec">the concreate feature type, currently support AKazeObjectRecognize,KazeObjectRecognize,ORBObjectRecognize,FreakObjectRecognize </param>
        /// <returns></returns>
        FeatureMatcher(boost::shared_ptr<ObjectRecognize> pObjectRec);

        /// <summary>Default Constructor. </summary>
        /// <returns></returns>
        FeatureMatcher();

        /// <summary>
        /// Bind the model image for this feature matcher
        /// </summary>
        /// <param name="modelImage">The image to be bined</param>
        /// <param name="isSmall">isSmall flag of the source</param>
        /// <param name="scale">The scale of this image, compares to the original image.  0.5 means half size of original image</param>
        /// <param name="prepType">Pre Image Processing Type.</param>
        /// <param name="otherImageParameters">if this paramters is setted, then will adopt the parameters to extract keypoints and descriptors</param>
        /// <returns>the homography matrix of the detection.</returns>
        void SetModelImage(cv::InputArray modelImage, bool isSmall=false, float scale=1.0, PreprocessType prepType=PreprocessType::Keep, FTParameters* otherImageParameters=NULL); 

        /// <summary>
        /// Detect this object in given image
        /// </summary>
        /// <param name="observedImage">The image in which we want to detect object</param>
        /// <param name="scale">The scale of the observedImage, compares to the original image.  0.5 means half size of original image</param>
        /// <param name="altRatio">Alternative aspectRatio of the image that this observed image could be recognized, as scale if X dimension</param>
        /// <param name="observedPrepType">Pre Image Processing Type.</param>
        /// <param name="unmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
        /// <param name="pointArray">returns a point array which contains a set of points recognized in observed image</param>
        /// <returns>the homography matrix of the detection.</returns>
        cv::Mat Match(cv::InputArray observedImage, float scale = 1.0, double altRatio = 1.0, PreprocessType observedPrepType=PreprocessType::Keep, double* unmatchRatio=NULL, std::vector<cv::Point2f>* pointArray=NULL)const;

        /// <summary>
        /// Detect this object in given image
        /// </summary>
        /// <param name="observedImage">The image in which we want to detect object</param>
        /// <param name="unmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
        /// <returns>the homography matrix of the detection.</returns>
        cv::Mat Match(cv::InputArray observedImage,double* unmatchRatio)const;

        /// <summary>
        /// Detect for two featureMatcher, before call this function, maker sure the two adopt the same feature paramter like threshold, scale type
        /// </summary>
        /// <param name="featureMather">The observedFeatureMather in which we want to detect object</param>
        /// <param name="altRatio">Alternative aspectRatio of the image that this observed image could be recognized, as scale if X dimension</param>
        /// <param name="pUnmatchRatio">returns a value between 0 and 1 denotes how many areas had not been matched, a big number indicates that the image are not fully matched</param>
        /// <param name="pPointArray">returns a point array which contains a set of points recognized in observed image</param>
        /// <returns>the homography matrix of the detection.</returns>
        cv::Mat Match(const FeatureMatcher& featureMather, double altRatio=1.0,double* pUnmatchRatio=NULL,vector<cv::Point2f>* pPointArray=NULL);

        /// <summary>
        /// Get the bounding rectangle around the matched points
        /// </summary>
        /// <returns>The bounding rectangle</returns>
        cv::Rect GetBoundingRect()const;

        /// <summary>
        /// Get the center around the matched points
        /// </summary>
        /// <returns>the center point</returns>
        cv::Point2f GetObjectCenter()const;

        /// <summary>
        /// Whether this object is small or not
        /// </summary>
        /// <returns>if this object is small</returns>
        bool IsSmall()const;

        /// <summary>
        /// get the image size
        /// </summary>
        /// <returns>the imageSize</returns>
        cv::Size GetImgSize()const;

        /// <summary>
        /// Get object outline in form of PointF array, refer to below, from 0~8,in c++, ROI is the whole image
        /// new PointF(ROIRect.Left, ROIRect.Bottom),
        /// new PointF(ROIRect.Right, ROIRect.Bottom),
        /// new PointF(ROIRect.Right, ROIRect.Top),
        /// new PointF(ROIRect.Left, ROIRect.Top),
        /// new PointF((ROIRect.Left + ROIRect.Right) / 2, (ROIRect.Top + ROIRect.Bottom) / 2),
        /// new PointF(ROIRect.Right, ROIRect.Top),
        /// new PointF(ROIRect.Right, ROIRect.Bottom),
        /// new PointF((ROIRect.Left + ROIRect.Right) / 2, (ROIRect.Top + ROIRect.Bottom) / 2),
        /// new PointF(ROIRect.Left, ROIRect.Bottom),
        /// new PointF(ROIRect.Left, ROIRect.Top),
        /// </summary>
        /// <returns> the object outline array</returns>
        std::vector<cv::Point2f> GetPts()const;

        ///// <summary>
        ///// Dump key points number of this object recognize
        ///// </summary>
        //void DumpKeyPointsNumber()const;

        /// <summary>
        /// Compute the rotation angle (0 - 360, counter-clockwise) of the reference image against
        /// the observed image.
        /// </summary>
        /// <returns>Rotation angle</returns>
        double GetRotationAngle()const;

        /// <summary>
        /// Get the key points of the model image
        /// </summary>
        /// <returns>keypoints</returns>
        std::vector<cv::KeyPoint> GetKeyPoints()const;

        /// <summary>
        /// Get the descriptors of the model image
        /// </summary>
        /// <returns>descriptors</returns>
        cv::Mat GetDescriptors()const;

        /// <summary>
        /// get the size,normally it's (width+height)/2
        /// </summary>
        /// <returns>the size</returns>
        int GetSize() const;

        /// <summary>
        /// get featuremather parameters,including feature threshold,scale value
        /// </summary>
        /// <returns>the shared pointer to paramter</returns>
        boost::shared_ptr<FTParameters> GetFTParameters() const;

        /// <summary>
        /// get the default featuremather parameters,normally scale value=1.0 etc.
        /// </summary>
        /// <returns>the shared pointer to paramter</returns>
        boost::shared_ptr<FTParameters> GetDefaultCurrentMatchPara();

        /// <summary>
        /// Set UnmatchRatio  calculation type
        /// </summary>
        /// <param name="type">the enum index of type </param>
        void SetUnmatchRatioType(UnmatchRatioType type);

        /// <summary>Destructor</summary>
        /// <returns></returns>
        ~FeatureMatcher();

    private:
        boost::shared_ptr<ObjectRecognize> pmObjectRec;
    };
}
#endif
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

#include "ImageChecker.hpp"

namespace DaVinci
{
    /// <summary>
    /// Check whether the observed source image matches the target image
    /// </summary>
    /// <param name="source"></param>
    /// <param name="reference"></param>
    /// <param name="size = 300"></param>
    /// <param name="altRatio">The alternative ratio for scaled image match</param>
    /// <returns>true for matched</returns>
    bool ImageChecker::IsImageMatched(cv::Mat source, cv::Mat reference, int size, double altRatio)
    {
        FeatureMatcher featMatch;
        double unmatchRatio = 0.0;
        const double MinUnmatchRatio = 0.5;
        bool matched = false;
        
        cv::Mat srcResizedImage; 
        cv::Mat refResizedImage; 

        if ((source.empty()) || (reference.empty()))
            return false;

        Size srcSize = source.size();
        Size refSize = reference.size();

        int srcResizeWidth = size;
        int srcResizeHeight = srcResizeWidth * srcSize.height / srcSize.width;

        int refResizeWidth = size;
        int refResizeHeight = refResizeWidth * refSize.height / refSize.width;

        resize(source, srcResizedImage, Size(srcResizeWidth, srcResizeHeight), 0, 0, CV_INTER_CUBIC);
        resize(reference, refResizedImage, Size(refResizeWidth, refResizeHeight), 0, 0, CV_INTER_CUBIC);

        /*
        string saveImageName;
        saveImageName = std::string("./") + boost::lexical_cast<std::string>(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())) + std::string("_CurrentFrame") + std::string(".png");
        DebugSaveImage(source, saveImageName);

        saveImageName = std::string("./")  + boost::lexical_cast<std::string>(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())) + std::string("_ReferenceFrame") + std::string(".png");
        DebugSaveImage(reference, saveImageName);
        */

        featMatch.SetModelImage(refResizedImage);

        if (featMatch.GetKeyPoints().size() > MinKeyPointsCount)
        {
            Mat H = featMatch.Match(srcResizedImage, &unmatchRatio);

            if (unmatchRatio < MinUnmatchRatio)
                matched = true;
        }
        else
            matched = true;
/*
        if (matched)
        {
            //Todo:
            //source.DrawPolyline(Array.ConvertAll<PointF, Point>(target_recognizer.GetPts(), Point.Round), true, new Bgr(Color.Red), 5);
        }
*/        
        return matched;
    }

    /// <summary>
    /// Check whether the observed source image matches the target image
    /// </summary>
    /// <param name="source"></param>
    /// <param name="reference"></param>
    /// <returns>true for matched</returns>
    bool ImageChecker::IsImageMatched(cv::Mat source, cv::Mat reference, double UnmatchRatio)
    {
        FeatureMatcher featForReference;
        FeatureMatcher featForSource;
        double actualUnmatchRatio = 0.0;
        double expectUnmatchRatio = 0.5;
        double minUnmatchRatio = 0.2;
        double maxUnmatchRatio = 1.0;
        bool matched = false;

        if ((source.empty()) || (reference.empty()))
            return false;

        featForReference.SetModelImage(reference);

        if (UnmatchRatio < minUnmatchRatio)
            UnmatchRatio = minUnmatchRatio;

        if (UnmatchRatio > maxUnmatchRatio)
            UnmatchRatio = maxUnmatchRatio;

        expectUnmatchRatio = UnmatchRatio;

        if (featForReference.GetKeyPoints().size() > MinKeyPointsCount)
        {
            Mat H = featForReference.Match(source, &actualUnmatchRatio);

            if (actualUnmatchRatio < expectUnmatchRatio)
                matched = true;
        }
        else
        {
            featForSource.SetModelImage(source);

            if (featForSource.GetKeyPoints().size() > MinKeyPointsCount)
            {
                DAVINCI_LOG_INFO << "IsImageMatched failed due to reference's key points < 50 while source' key points > 50!";
                matched = false;
            }
            else
            {
                DAVINCI_LOG_INFO << "IsImageMatched success when found that both reference and source's key points < 50.";
                matched = true;
            }
        }
/*
        if (matched)
        {
            //Todo:
            //source.DrawPolyline(Array.ConvertAll<PointF, Point>(target_recognizer.GetPts(), Point.Round), true, new Bgr(Color.Red), 5);
        }
*/        
        return matched;
    }

    /// <summary>
    /// Check whether the preview image matches the source image
    /// </summary>
    /// <param name="source"></param>
    /// <param name="reference"></param>
    /// <returns>true for matched</returns>
    bool ImageChecker::IsPreviewMatched(cv::Mat source, cv::Mat reference)
    {
        FeatureMatcher featMatch;
        bool matched = false;
        cv::Mat srcResizedImage; 
        cv::Mat refResizedImage; 

        if ((source.empty()) || (reference.empty()))
            return false;

        Size srcSize = source.size();
        Size refSize = reference.size();

        if (srcSize.width * refSize.height == refSize.width * srcSize.height)
        {
            int refResizeWidth = srcSize.width;
            int refResizeHeight = refResizeWidth * refSize.height / refSize.width;

            int srcResizeWidth = refResizeWidth;
            int srcResizeHeight = refResizeHeight;

            resize(source, srcResizedImage, Size(srcResizeWidth, srcResizeHeight), 0, 0, CV_INTER_CUBIC);
            resize(reference, refResizedImage, Size(refResizeWidth, refResizeHeight), 0, 0, CV_INTER_CUBIC);

            featMatch.SetModelImage(refResizedImage,false);
            Mat H = featMatch.Match(srcResizedImage);

            if (!H.empty())
            {
                matched = true;
            }
        }
        else
            matched = IsImageMatched(source, reference, 200);

/*Todo:
        if (matched)
        {
            source.DrawPolyline(Array.ConvertAll<PointF, Point>(target_recognizer.GetPts(), Point.Round), true, new Bgr(Color.Red), 5);
        }
*/
       
        return matched;
    }
}
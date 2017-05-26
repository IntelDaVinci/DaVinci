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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __IMAGE_CHECKER_HPP__
#define __IMAGE_CHECKER_HPP__

#include "checker.hpp"
#include "boost/filesystem.hpp"
#include "FeatureMatcher.hpp"
#include "AKazeObjectRecognize.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    /// <summary> A base class for resultchecker. </summary>
    class ImageChecker : public Checker
    {
        public:
            explicit ImageChecker();

            virtual ~ImageChecker();

            /// <summary>
            /// Check whether the observed source image matches the target image
            /// </summary>
            /// <param name="source"></param>
            /// <param name="reference"></param>
            /// <param name="size = 300"></param>
            /// <param name="altRatio">The alternative ratio for scaled image match</param>
            /// <returns>true for matched</returns>
            static bool IsImageMatched(cv::Mat source, cv::Mat reference, int size, double altRatio = 1.0);

            /// <summary>
            /// Check whether the observed source image matches the target image
            /// </summary>
            /// <param name="source"></param>
            /// <param name="reference"></param>
            /// <returns>true for matched</returns>
            static bool IsImageMatched(cv::Mat source, cv::Mat reference, double UnmatchRatio = 0.5);

            /// <summary>
            /// Check whether the preview image matches the source image
            /// </summary>
            /// <param name="source"></param>
            /// <param name="reference"></param>
            /// <returns>true for matched</returns>
            static bool IsPreviewMatched(cv::Mat source, cv::Mat reference);

        private:
            bool IsDrawMatched();//const boost::shared_ptr<HomographyMatrix> &homography, const boost::shared_ptr<SURFObjectRecognize> &target_recognizer);

            static const int MinKeyPointsCount = 50;
    };
}

#endif
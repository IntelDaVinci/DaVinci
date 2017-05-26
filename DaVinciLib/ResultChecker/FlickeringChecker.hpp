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

#ifndef __FLICKERING_CHECHER_HPP__
#define __FLICKERING_CHECHER_HPP__

#include "boost/filesystem.hpp"
#include "FeatureMatcher.hpp"
#include "AKazeObjectRecognize.hpp"
#include "ScriptReplayer.hpp"
#include "QScript/VideoCaptureProxy.hpp"
#include "opencv2/gpu/gpu.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class FlickeringChecker
    {
    public:
        FlickeringChecker();
        virtual ~FlickeringChecker();

        DaVinciStatus VideoQualityEvaluation(const string videoName);

    private:
        struct BufferPSNR                                     // Optimized GPU versions
        {   // Data allocations are very expensive on GPU. Use a buffer to solve: allocate once reuse later.
            gpu::GpuMat gI1, gI2, gs, t1,t2;

            gpu::GpuMat buf;
        };

        struct BufferMSSIM                                     // Optimized GPU versions
        {   // Data allocations are very expensive on GPU. Use a buffer to solve: allocate once reuse later.
            gpu::GpuMat gI1, gI2, gs, t1,t2;

            gpu::GpuMat I1_2, I2_2, I1_I2;
            vector<gpu::GpuMat> vI1, vI2;

            gpu::GpuMat mu1, mu2;
            gpu::GpuMat mu1_2, mu2_2, mu1_mu2;

            gpu::GpuMat sigma1_2, sigma2_2, sigma12;
            gpu::GpuMat t3;

            gpu::GpuMat ssim_map;

            gpu::GpuMat buf;
        };
        void SaveFlickingVideoClips(const string &videoFile, int startIndex, int endIndex, Size frmSize, string saveVideoPath);

        double GetPSNR(const Mat& I1, const Mat& I2);
        double GetPSNRGPUOptimized(const Mat& I1, const Mat& I2, BufferPSNR& b);
    };
}

#endif
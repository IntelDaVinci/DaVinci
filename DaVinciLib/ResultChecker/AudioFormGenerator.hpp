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

#ifndef __AUDIO_FORM_GENERATOR_HPP__
#define __AUDIO_FORM_GENERATOR_HPP__

#include "sndfile.h"
#include "AudioFile.hpp"
#include "opencv2/opencv.hpp"
#include "boost/filesystem.hpp"
#include "ObjectCommon.hpp"
#include <vector>

namespace DaVinci
{
    class AudioFormGenerator
    {
    public:
        AudioFormGenerator();
        virtual ~AudioFormGenerator(void);

        DaVinciStatus SaveAudioWave(const string &audioFileName);
        string CheckQualityResult(const string &audioFileName);

    private:
        SNDFILE* ReadAudioFile(const string &audioFileName);
        void DrawAudioWave(cv::Mat &wave, bool isRecordWave = true);
        int GetAudioFrames();

        SNDFILE *fileHandler;
        SF_INFO sInfo;

        int waveWidth;
        int waveHeight;

        short maxVal;
        short minVal;
        int noiseFrmCount;
        int silentFrmCount;
        float noiseFrmPercentage;
        float silentFrmPercentage;
        vector<short> audioSamples;
        cv::Mat wave;

        cv::Scalar backgroundImgColor;
    };
}

#endif


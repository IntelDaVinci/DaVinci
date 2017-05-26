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

#include "AudioFormGenerator.hpp"
#include "boost/integer.hpp"
#include "DaVinciDefs.hpp"

namespace DaVinci
{
    AudioFormGenerator::AudioFormGenerator(): waveWidth(1024), waveHeight(768),
        wave(1024, 768, cv::DataType<float>::type), minVal(0), maxVal(0), noiseFrmCount(0), silentFrmCount(0),
        noiseFrmPercentage(0.9), silentFrmPercentage(0.9), fileHandler(nullptr)
    {
        // backgroud image color is gray
        backgroundImgColor = cv::Scalar(200,200,200);
        sInfo.channels = 1;
        sInfo.format = SF_FORMAT_WAV;
        sInfo.frames = 0;
        sInfo.samplerate = 0;
        sInfo.sections = 0;
        sInfo.seekable = 0;
    }


    AudioFormGenerator::~AudioFormGenerator(void)
    {
    }

    int AudioFormGenerator::GetAudioFrames()
    {
        return (int)sInfo.frames;
    }

    SNDFILE* AudioFormGenerator::ReadAudioFile(const string &audioFileName)
    {
        memset(&sInfo, 0, sizeof(SF_INFO));

        fileHandler = sf_open(audioFileName.c_str(), SFM_READ, &sInfo);
        if (fileHandler == nullptr)
        {
            return nullptr;
        }
        if (sInfo.channels != 1 || sInfo.samplerate != 44100)
        {
            DAVINCI_LOG_ERROR << string("Can only support monoic audio or the sample rate of the audio should be 44100 Hz!");
            return nullptr;
        }

        audioSamples.resize(GetAudioFrames());
        sf_count_t readSampleCount = sf_read_short(fileHandler, &audioSamples[0], (int)sInfo.frames);
        if (readSampleCount == 0)
        {
            DAVINCI_LOG_ERROR << string("No audio data read!");
            return nullptr;
        }

        return fileHandler;
    }

    void AudioFormGenerator::DrawAudioWave(cv::Mat &wave, bool isRecordWave)
    {
        noiseFrmCount = 0;
        silentFrmCount = 0;
        short shortMax = boost::integer_traits<short>::const_max;
        // Draw background
        DrawRect(wave, cv::Rect(0,0,waveWidth,waveHeight), backgroundImgColor, CV_FILLED);

        // Draw wave
        int size = (int)(audioSamples.size());
        for (int pxlIndex = 0; pxlIndex < waveWidth; pxlIndex++)
        {
            int start = (int)((float)pxlIndex * (float)size / (float)waveWidth);
            int end = (int)((float)(pxlIndex + 1) * (float)size / (float)waveWidth);
            end = end > size ? size : end; 
            minVal = boost::integer_traits<short>::const_max;
            maxVal = boost::integer_traits<short>::const_min;
            for (int i=start; i<end; i++)
            {
                short val = audioSamples[i];
                minVal = val < minVal ? val : minVal;
                maxVal = val > maxVal ? val : maxVal;
            }

            // Mapping the min and max values to the wave picture space
            int yMin = (int)((minVal + shortMax) * waveHeight / (shortMax * 2));
            int yMax = (int)((maxVal + shortMax) * waveHeight / (shortMax * 2));
            if (yMax-yMin >= waveHeight/10)
            {
                noiseFrmCount++;
            }
            if (yMax-yMin <= waveWidth/10)
            {
                silentFrmCount++;
            }
            LineSegment line(cv::Point(pxlIndex, yMin), cv::Point(pxlIndex, yMax));
            if (isRecordWave == false)
            {
                DrawLine(wave, line, Red, 1);
            }
            else
            {
                DrawLine(wave, line, Blue, 1);
            }
        }
    }

    DaVinciStatus AudioFormGenerator::SaveAudioWave(const string &audioFileName)
    {
        boost::filesystem::path p(audioFileName);
        boost::filesystem::path saveWaveName = p.replace_extension("png");

        cv::Mat audioWaveImg(waveHeight,waveWidth, CV_8UC3);
        SNDFILE *audioFileReader = ReadAudioFile(audioFileName);
        if (audioFileReader == nullptr)
        {
            DAVINCI_LOG_ERROR << string("Cannot read the audio file correctly!");
            return DaVinciStatus(errc::bad_file_descriptor);
        }

        if (audioFileName.find("_replay.wav") != string::npos)
        {
            DrawAudioWave(audioWaveImg, false);
        }
        else
        {
            DrawAudioWave(audioWaveImg);
        }
        SaveImage(audioWaveImg, saveWaveName.string());
        return DaVinciStatusSuccess;
    }

    string AudioFormGenerator::CheckQualityResult(const string &audioFile)
    {
        if ((float)noiseFrmCount / (float)waveWidth >= noiseFrmPercentage)
        {
            DAVINCI_LOG_WARNING << "Audio file: " + audioFile + " might have noise! Please double confirm it!";
            return "Noise";
        }
        else if ((float)silentFrmCount / (float)waveWidth >= silentFrmPercentage)
        {
            DAVINCI_LOG_WARNING << "Audio file: " + audioFile + " might have NO SOUND! Please double confirm it!";
            return "Silent";
        }
        else
        {
            return "Normal";
        }
    }
}

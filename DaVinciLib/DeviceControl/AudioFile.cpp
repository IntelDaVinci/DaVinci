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

#include <vector>
#include "AudioFile.hpp"

namespace DaVinci
{

    boost::shared_ptr<AudioFile> AudioFile::CreateWriteWavFile(const string &wavFile)
    {
        SF_INFO sfInfo;
        memset(&sfInfo, 0, sizeof(SF_INFO));
        sfInfo.channels = 1;
        sfInfo.samplerate = 44100;
        sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        auto f = boost::shared_ptr<AudioFile>(new AudioFile(wavFile, SFM_WRITE, &sfInfo));
        if (f->fileHandle == nullptr)
        {
            return nullptr;
        }
        return f;
    }

    boost::shared_ptr<AudioFile> AudioFile::CreateReadFile(const string &audioFile)
    {
        SF_INFO sfInfo;
        memset(&sfInfo, 0, sizeof(SF_INFO));
        sfInfo.format = 0;
        auto f = boost::shared_ptr<AudioFile>(new AudioFile(audioFile, SFM_READ, &sfInfo));
        if (f->fileHandle == nullptr)
        {
            return nullptr;
        }
        return f;
    }

    DaVinciStatus AudioFile::TrimWavFile(const string &inWavFile, const string &outWavFile, uint64_t start, uint64_t end)
    {
        auto inWave = CreateReadFile(inWavFile);
        auto outWave = CreateWriteWavFile(outWavFile);
        if (inWave == nullptr || outWave == nullptr || inWave->Seek(start) < 0)
        {
            return errc::invalid_argument;
        }
        const int bufSampleSize = 1024;
        vector<short> buf;
        buf.resize(bufSampleSize * inWave->Info().channels);
        sf_count_t startSample = inWave->TimeToPos(start);
        sf_count_t endSample = inWave->TimeToPos(end);
        sf_count_t totalSamplesToRead = endSample - startSample;
        sf_count_t samplesToRead = totalSamplesToRead < bufSampleSize ? totalSamplesToRead : bufSampleSize;
        sf_count_t samplesRead;
        while (samplesToRead > 0 && (samplesRead = sf_readf_short(inWave->Handle(), &buf[0], samplesToRead)) > 0)
        {
            for (sf_count_t i = 0; i < samplesRead; i++)
            {
                sf_write_short(outWave->Handle(), &buf[static_cast<size_t>(i * inWave->Info().channels)], 1);
            }
            totalSamplesToRead -= samplesRead;
            samplesToRead = totalSamplesToRead < bufSampleSize ? totalSamplesToRead : bufSampleSize;
        }
        return DaVinciStatusSuccess;
    }

    AudioFile::AudioFile(const string &audioFile, int mode, const SF_INFO *info)
    {
        memcpy(&sfInfo, info, sizeof(SF_INFO));
        fileHandle = sf_open(audioFile.c_str(), mode, &sfInfo);
    }

    AudioFile::~AudioFile()
    {
        if (fileHandle != nullptr)
        {
            sf_close(fileHandle);
        }
    }

    const SF_INFO &AudioFile::Info()
    {
        return sfInfo;
    }

    SNDFILE *AudioFile::Handle()
    {
        return fileHandle;
    }

    sf_count_t AudioFile::Seek(uint64_t ms)
    {
        return sf_seek(fileHandle, TimeToPos(ms), SEEK_SET);
    }

    sf_count_t AudioFile::TimeToPos(uint64_t ms) const
    {
        return static_cast<sf_count_t>((1.0 * sfInfo.samplerate * ms) / 1000) * sfInfo.channels;
    }
}
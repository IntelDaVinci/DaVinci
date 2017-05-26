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

#include "kissfft.hpp"
#include "boost/lexical_cast.hpp"
#include "AudioRecognize.hpp"
#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{

    AudioRecognize::AudioFeatureExtractor::AudioFeatureExtractor(int window) :
        spectrogram(window / 2, spectrogramWidth, cv::DataType<float>::type),
        imageNum(0), w(window), dataIdxOffset(0), dataIdx(0)
    {
        fftBuffer.resize(window);
        feature = boost::shared_ptr<AudioFeature>(new AudioFeature());
    }

    void AudioRecognize::AudioFeatureExtractor::AddSampleBuffer(vector<short> &sampleBuffer)
    {
        for (int i = 0; i < w; i++)
        {
            fftBuffer[i].real(sampleBuffer[i]);
            fftBuffer[i].imag(0);
        }
        kissfft<float> fft(w, false);
        vector<complex<float>> fftout;
        fftout.resize(w);
        fft.transform(&fftBuffer[0], &fftout[0]);
        for (int i = 0; i < w / 2; i++)
        {
            spectrogram.at<float>(i, dataIdx) = fftout[i].real() * fftout[i].real() + fftout[i].imag() * fftout[i].imag();
        }
        dataIdx++;
        if (dataIdx == spectrogramWidth)
        {
            ProcessSpectrogram();
            dataIdxOffset += spectrogramWidth - 1;
            dataIdx = 1;
        }
    }

    void AudioRecognize::AudioFeatureExtractor::ComputeFeature(const cv::Mat &image, vector<cv::KeyPoint> &keyPoints, cv::Mat &descriptors)
    {
        cv::AKAZE akaze(DESCRIPTOR_MLDB, 0, 3, 0.00001f);
        akaze(image, cv::noArray(), keyPoints, descriptors);
    }

    void AudioRecognize::AudioFeatureExtractor::ProcessSpectrogram()
    {
        double maxValue;
        cv::minMaxIdx(spectrogram, NULL, &maxValue);
        cv::Mat image;
        spectrogram.convertTo(image, CV_8U, 255 / maxValue);
        vector<cv::KeyPoint> keyPoints;
        cv::Mat descriptors;
        ComputeFeature(image, keyPoints, descriptors);
        for (size_t i = 0; i < keyPoints.size(); i++)
        {
            keyPoints[i].pt.x += dataIdxOffset;
        }
        feature->keyPoints.insert(feature->keyPoints.end(), keyPoints.begin(), keyPoints.end());
        if (feature->descriptors.empty())
        {
            feature->descriptors = descriptors;
        }
        else if (!descriptors.empty())
        {
            cv::Mat concat;
            cv::vconcat(feature->descriptors, descriptors, concat);
            feature->descriptors = concat;
        }
#ifdef _DEBUG
            DrawSpectrogram();
#endif
        // What's the purpose of the following code?
        for (int j = 0; j < spectrogram.rows; j++)
        {
            spectrogram.at<float>(j, 0) = spectrogram.at<float>(j, dataIdx - 1);
        }
    }
#ifdef _DEBUG
    void AudioRecognize::AudioFeatureExtractor::DrawSpectrogram()
    {
        double maxValue;
        cv::minMaxIdx(spectrogram, NULL, &maxValue);
        cv::Mat image(spectrogram.mul(255 / maxValue));
        cv::imwrite("spectrogram-" + boost::lexical_cast<string>(imageNum) + ".png", image);
        imageNum++;
    }
#endif
    void AudioRecognize::AudioFeatureExtractor::FinishAddSampleBuffer()
    {
        if (dataIdx > 0)
        {
            ProcessSpectrogram();
        }
    }

    boost::shared_ptr<AudioRecognize::AudioFeature> AudioRecognize::AudioFeatureExtractor::GetFeature()
    {
        return feature;
    }

    AudioRecognize::AudioRecognize(const string &filename)
    {
        this->modelFeature = ExtractFeature(filename);
    }

    boost::shared_ptr<AudioRecognize::AudioFeature> AudioRecognize::ExtractFeature(const string &filename)
    {
        auto audioFile = AudioFile::CreateReadFile(filename);
        if (audioFile == nullptr)
        {
            return boost::shared_ptr<AudioFeature>(new AudioFeature());
        }
        int sampleRate = 44100;
        if (audioFile->Info().samplerate != sampleRate || audioFile->Info().channels != 1)
        {
            DAVINCI_LOG_ERROR << filename << ": Only support 44100Hz and mono stream but got " <<
                audioFile->Info().samplerate << "Hz and " << audioFile->Info().channels << " channels";
            return boost::shared_ptr<AudioFeature>(new AudioFeature());
        }
        vector<short> sampleBuffer;
        sampleBuffer.resize(window);
        const int samplesPerRead = window - overlap;
        sf_count_t numSamples = sf_read_short(audioFile->Handle(), &sampleBuffer[0], overlap);
        if (numSamples != overlap)
        {
            return boost::shared_ptr<AudioFeature>(new AudioFeature());
        }
        auto featureExtractor = boost::shared_ptr<AudioFeatureExtractor>(new AudioFeatureExtractor(window));
        while ((numSamples = sf_read_short(audioFile->Handle(), &sampleBuffer[overlap], samplesPerRead)) == samplesPerRead)
        {
            featureExtractor->AddSampleBuffer(sampleBuffer);
            memmove(&sampleBuffer[0], &sampleBuffer[samplesPerRead], overlap);
        }
        featureExtractor->FinishAddSampleBuffer();
        return featureExtractor->GetFeature();
    }

    bool AudioRecognize::Detect(const string &filename, float matchThreshold)
    {
        int64_t ms;
        return Detect(filename, ms, matchThreshold);
    }

    bool AudioRecognize::Detect(const string &filename, int64_t &ms, float matchThreshold)
    {
        boost::shared_ptr<AudioFeature> observedFeature = ExtractFeature(filename);
        return MatchAudioFeature(observedFeature, ms, matchThreshold);
    }

    bool AudioRecognize::MatchAudioFeature(const boost::shared_ptr<AudioFeature> &observedFeature, int64_t &ms, float matchThreshold)
    {
        ms = 0;
        if (modelFeature->descriptors.empty() && observedFeature->descriptors.empty())
        {
            return true;
        }
        else if (modelFeature->descriptors.empty() || observedFeature->descriptors.empty())
        {
            return false;
        }
        int nonZeroCountThreshold;
        if (modelFeature->keyPoints.size() > 50)
        {
            nonZeroCountThreshold = 10;
        }
        else
        {
            nonZeroCountThreshold = 3;
        }
        int k = 2;
        vector<vector<cv::DMatch>> matches;
        cv::BFMatcher matcher(cv::NORM_L2);
        matcher.knnMatch(modelFeature->descriptors,observedFeature->descriptors, matches, k);

        cv::Mat indices = cv::Mat::zeros(modelFeature->descriptors.rows, k, CV_32SC1);
        for (int i = 0; i < indices.rows; ++i)
        {
            for(int j = 0; j < k; ++j)
            {
                int matchSize = (int)matches[i].size();
                if (matchSize > j)
                    indices.at<int>(i,j)=matches[i][j].trainIdx;
                else
                {
                    cv::DMatch dmatch;
                    matches[i].push_back(dmatch);
                }
            }
        }
        cv::Mat mask = cv::Mat::ones((int)matches.size(), 1, CV_8UC1);
        VoteForUniqueness(matches, 0.8, mask);
        int nonZeroCount = countNonZero(mask);
        int totalVotes;
        if (nonZeroCount > nonZeroCountThreshold)
        {
            totalVotes = nonZeroCount;
        }
        else
        {
            // usually this happens on the audio track having repetitive melody
            // we try to match entire key points
            totalVotes = (int)modelFeature->keyPoints.size();
            mask = cv::Mat::ones((int)matches.size(), 1, CV_8UC1);
        }

        // compute a sparse histogram based on the time difference of matching key points.
        int maxVote = 0;
        int maxVoteKey = INT_MAX;
        unordered_map<int, int> diffHist;
        for (int i = 0; i < indices.rows; i++)
        {
            if (mask.at<uchar>(i, 0) != 0)
            {
                int diff = static_cast<int>(observedFeature->keyPoints[indices.at<int>(i, 0)].pt.x - modelFeature->keyPoints[i].pt.x);
                if (diffHist.find(diff) == diffHist.end())
                {
                    diffHist[diff] = 0;
                }
                int vote = diffHist[diff];
                vote++;
                if (vote >= maxVote)
                {
                    maxVote = vote;
                    maxVoteKey = diff;
                }
                diffHist[diff] = vote;
            }
        }

        DAVINCI_LOG_DEBUG << "maxVote = " << maxVote << ", model key points = " << modelFeature->keyPoints.size() << ", totalVotes = " << totalVotes << ", thresholded value = " << totalVotes * matchThreshold;
        if (maxVote >= totalVotes * matchThreshold && diffHist.find(maxVoteKey) != diffHist.end())
        {
            ms = static_cast<int64_t>(diffHist[maxVoteKey] * (window - overlap) / 44.1f);
            return true;
        }
        else
        {
            return false;
        }
    }

}
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

#ifndef __AUDIO_RECOGNIZE_HPP__
#define __AUDIO_RECOGNIZE_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include "AudioFile.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// A class to recognize one audio clip from the other audio clip.
    /// </summary>
    class AudioRecognize : public boost::enable_shared_from_this<AudioRecognize>
    {
    public:
        /// <summary>
        /// Initialize the AudioRecognize class with a specified model media file.
        /// Currently it only supports 44100Hz mono format.
        /// </summary>
        /// <param name="filename">The media file</param>
        AudioRecognize(const string &filename);

        /// <summary>
        /// See Detect(string, out long)
        /// </summary>
        /// <param name="filename">The observed media file.</param>
        /// <param name="matchThreshold">
        /// The ratio of matching key points over the total number of key points, deciding a match.
        /// </param>
        /// <returns>True if matching, or False otherwise.</returns>
        bool Detect(const string &filename, float matchThreshold = 0.2f);

        /// <summary>
        /// Check whether the model audio clip matches the whole or part of audio clip in the specified observed media file.
        /// The media file should have 44100Hz and mono format.
        /// </summary>
        /// <param name="filename">The observed media file.</param>
        /// <param name="ms">The starting timestamp of the observed clip that matches the model clip.</param>
        /// <param name="matchThreshold">
        /// The ratio of matching key points over the total number of key points, deciding a match.
        /// </param>
        /// <returns>True if matching, or False otherwise.</returns>
        bool Detect(const string &filename, int64_t &ms, float matchThreshold = 0.2f);

    private:
        /// <summary>
        /// Audio feature described as SURF of its spectrogram.
        /// </summary>
        struct AudioFeature
        {
            vector<cv::KeyPoint> keyPoints;
            cv::Mat descriptors;
        };

        /// <summary>
        /// A class to extract audio clip feature. The idea is to use SURF feature of its
        /// spectrogram as its feature.
        /// </summary>
        class AudioFeatureExtractor
        {
        public:
            /// <summary>
            /// The extractor works on "window" samples of audio with "sampleInterval" sample interval.
            /// </summary>
            /// <param name="window"></param>
            /// <param name="sampleInterval"></param>
            AudioFeatureExtractor(int window);

            /// <summary>
            /// Add "window" samples to the extractor to work on.
            /// </summary>
            /// <param name="sampleBuffer"></param>
            void AddSampleBuffer(vector<short> &sampleBuffer);

            void FinishAddSampleBuffer();

            boost::shared_ptr<AudioFeature> GetFeature();
        private:
            static const int spectrogramWidth = 1024;

            int w;
            int dataIdxOffset;
            int imageNum;
            int dataIdx;
            cv::Mat spectrogram;
            vector<complex<float>> fftBuffer;
            boost::shared_ptr<AudioFeature> feature;


            /// <summary>
            /// Extract SURF features from a ready spectrogram, combine the features to
            /// the previously calculated features.
            /// TODO: optimize with GPU.
            /// </summary>
            void ProcessSpectrogram();
#ifdef _DEBUG
            /// <summary>
            /// Debug method to draw a ready spectrogram
            /// </summary>
            void DrawSpectrogram();
#endif
            void ComputeFeature(const cv::Mat &image, vector<cv::KeyPoint> &keyPoints, cv::Mat &descriptors);
        };

        /// <summary>
        /// The window of STFT in samples
        /// </summary>
        static const int window = 2048;
        /// <summary>
        /// The overlapped samples of STFT window.
        /// </summary>
        static const int overlap = 1024;
        /// <summary>
        /// 
        /// </summary>

        boost::shared_ptr<AudioFeature> modelFeature;

        boost::shared_ptr<AudioFeature> ExtractFeature(const string &filename);

        /// <summary>
        /// This is the main method doing the match job.
        /// </summary>
        /// <param name="observedFeature"></param>
        /// <param name="ms"></param>
        /// <param name="matchThreshold">
        /// The ratio of matching key points over the total number of key points, deciding a match.
        /// </param>
        /// <returns></returns>
        bool MatchAudioFeature(const boost::shared_ptr<AudioFeature> &observedFeature, int64_t &ms, float matchThreshold);
    };
}

#endif
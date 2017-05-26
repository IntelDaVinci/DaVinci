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

#ifndef __AUDIO_PLAYER_HPP__
#define __AUDIO_PLAYER_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <vector>
#include <queue>
#include "boost/function.hpp"
#include "boost/atomic.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/recursive_mutex.hpp"
#include "AL/al.h"
#include "AL/alc.h"

#include "DaVinciStatus.hpp"
#include "AudioFile.hpp"

namespace DaVinci
{
    using namespace std;

    class AudioPlayer : public boost::enable_shared_from_this<AudioPlayer>
    {
    public:
        
        /// <summary>
        /// The player on the specified waveout context and
        /// plays the samples given by the callback or "Play" function call.
        /// </summary>
        ///
        /// <param name="waveOutContext"> Context for the wave out. </param>
        explicit AudioPlayer(const boost::shared_ptr<ALCcontext> waveOutContext);

        ~AudioPlayer();

        /// <summary> Plays the given samples. </summary>
        ///
        /// <param name="samples"> The samples. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Play(const boost::shared_ptr<vector<short>> &samples);

        /// <summary> Plays the samples from callback </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Play(const boost::shared_ptr<AudioFile> file);

        /// <summary> Starts the player </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Start();

        /// <summary> Stops playback </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus Stop();

    private:
        static const int pollingIntervalMs = 100;
        static const size_t numBuffers = 3;
        static const int frequency = 44100;
        static const size_t numSamples = frequency;
        static const size_t maxSamplesQueueSize = 10;

        /// <summary> The play lock protecting flag, thread object and sample buffer </summary>
        boost::recursive_mutex playLock;
        boost::shared_ptr<boost::thread> playThread;
        boost::shared_ptr<AudioFile> audioFile;
        queue<boost::shared_ptr<vector<short>>> samplesQueue;
        boost::atomic<bool> shouldStop;

        boost::shared_ptr<ALCcontext> waveOut;
        ALuint alSrc;
        ALuint alBuffers[numBuffers];
        bool alBufferQueued[numBuffers];
        boost::shared_ptr<vector<short>> alBufferSamples[numBuffers];

        /// <summary> Sets the context of this player as current. The context lock should be held already. </summary>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus SetContextAsCurrent();

        void StartPlayThread();

        void DoPlay();

        boost::shared_ptr<vector<short>> GetSamples();

        DaVinciStatus PlaySamples(const boost::shared_ptr<vector<short>> &samples);

        /// <summary> Unqueue processed buffers assuming al context is set already and locked. </summary>
        void UnqueueProcessedBuffers();
    };
}

#endif
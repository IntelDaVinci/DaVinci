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

#include "AudioPlayer.hpp"
#include "AudioManager.hpp"

namespace DaVinci
{
    AudioPlayer::AudioPlayer(const boost::shared_ptr<ALCcontext> waveOutContext) : waveOut(waveOutContext)
    {
        boost::lock_guard<boost::recursive_mutex> lock(AudioManager::ContextLock);
        SetContextAsCurrent();
        alGenSources(1, &alSrc);
        alSourcei(alSrc, AL_BUFFER, AL_NONE);
        for (size_t i = 0; i < numBuffers; i++)
        {
            alBuffers[i] = AL_NONE;
            alBufferQueued[i] = false;
        }
    }

    AudioPlayer::~AudioPlayer()
    {
        Stop();
        boost::lock_guard<boost::recursive_mutex> lock(AudioManager::ContextLock);
        SetContextAsCurrent();
        alDeleteSources(1, &alSrc);
    }

    DaVinciStatus AudioPlayer::Play(const boost::shared_ptr<vector<short>> &samples)
    {
        boost::lock_guard<boost::recursive_mutex> lock(playLock);
        if (playThread == nullptr || audioFile != nullptr)
        {
            return errc::operation_not_supported;
        }
        if (samplesQueue.size() >= maxSamplesQueueSize)
        {
            DAVINCI_LOG_DEBUG << "AudioPlayer buffer full.";
            samplesQueue.pop();
        }
        samplesQueue.push(samples);
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPlayer::Play(const boost::shared_ptr<AudioFile> file)
    {
        boost::lock_guard<boost::recursive_mutex> lock(playLock);
        if (playThread == nullptr || audioFile != nullptr || samplesQueue.size() > 0)
        {
            return errc::operation_not_supported;
        }
        audioFile = file;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPlayer::Start()
    {
        boost::lock_guard<boost::recursive_mutex> lock(playLock);
        if (playThread != nullptr)
        {
            return errc::operation_not_supported;
        }
        StartPlayThread();
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPlayer::Stop()
    {
        boost::unique_lock<boost::recursive_mutex> lock(playLock);
        boost::shared_ptr<boost::thread> thePlayThread = playThread;
        shouldStop = true;
        lock.unlock(); // avoid deadlock in the following thread join
        if (thePlayThread != nullptr)
        {
            thePlayThread->join();
        }
        lock.lock();
        playThread = nullptr;
        audioFile = nullptr;
        queue<boost::shared_ptr<vector<short>>> empty;
        std::swap(samplesQueue, empty);
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPlayer::SetContextAsCurrent()
    {
        ALCcontext * currentContext = alcGetCurrentContext();
        if (currentContext != waveOut.get())
        {
            alcMakeContextCurrent(waveOut.get());
        }
        return DaVinciStatusSuccess;
    }

    void AudioPlayer::StartPlayThread()
    {
        boost::lock_guard<boost::recursive_mutex> lock(playLock);
        shouldStop = false;
        playThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(
            &AudioPlayer::DoPlay, this)));
        SetThreadName(playThread->get_id(), "AudioPlayer");
    }

    void AudioPlayer::DoPlay()
    {
        {
            boost::lock_guard<boost::recursive_mutex> lock(AudioManager::ContextLock);
            SetContextAsCurrent();
            alGenBuffers(numBuffers, alBuffers);
            alSourcePlay(alSrc);
        }

        while (!shouldStop)
        {
            auto samples = GetSamples();
            if (samples == nullptr)
            {
                DAVINCI_LOG_DEBUG << "Unable to get audio samples for playing, exit playback thread.";
                break;
            }
            else if (samples->empty())
            {
                ThreadSleep(pollingIntervalMs);
            }
            else
            {
                while (!shouldStop)
                {
                    DaVinciStatus status = PlaySamples(samples);
                    if (status == errc::device_or_resource_busy)
                    {
                        ThreadSleep(pollingIntervalMs);
                    }
                    else if (!DaVinciSuccess(status))
                    {
                        DAVINCI_LOG_ERROR << "Error playing samples with code: " << status.message();
                        shouldStop = true;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        {
            boost::lock_guard<boost::recursive_mutex> lock(AudioManager::ContextLock);
            SetContextAsCurrent();
            alSourceStop(alSrc);
            alDeleteBuffers(numBuffers, alBuffers);
        }
    }

    boost::shared_ptr<vector<short>> AudioPlayer::GetSamples()
    {
        auto samples = boost::shared_ptr<vector<short>>(new vector<short>());
        boost::lock_guard<boost::recursive_mutex> lock(playLock);
        if (!samplesQueue.empty())
        {
            samples = samplesQueue.front();
            samplesQueue.pop();
            return samples;
        }
        else if (audioFile != nullptr)
        {
            samples = boost::shared_ptr<vector<short>>(new vector<short>());
            samples->resize(numSamples);
            sf_count_t samplesRead = sf_read_short(audioFile->Handle(), &(*samples)[0], numSamples);
            if (samplesRead > 0 && samplesRead < numSamples)
            {
                samples->resize(static_cast<size_t>(samplesRead));
            }
            if (samplesRead > 0)
            {
                return samples;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return samples;
        }
    }

    DaVinciStatus AudioPlayer::PlaySamples(const boost::shared_ptr<vector<short>> &samples)
    {
        assert(samples != nullptr);
        boost::lock_guard<boost::recursive_mutex> lock(AudioManager::ContextLock);
        SetContextAsCurrent();
        // Get a free AL buffer we can use
        int freeBufferId = -1;
        for (size_t i = 0; i < numBuffers; i++)
        {
            if (!alBufferQueued[i])
            {
                freeBufferId = static_cast<int>(i);
                break;
            }
        }
        if (freeBufferId < 0)
        {
            // Try to dequeue an AL buffer to use
            ALint numBufferProcessed = 0;
            ALuint alBuf = AL_NONE;
            alGetSourcei(alSrc, AL_BUFFERS_PROCESSED, &numBufferProcessed);
            if (numBufferProcessed > 0)
            {
                alSourceUnqueueBuffers(alSrc, 1, &alBuf);
                for (size_t i = 0; i < numBuffers; i++)
                {
                    if (alBuffers[i] == alBuf)
                    {
                        freeBufferId = static_cast<int>(i);
                        alBufferQueued[i] = false;
                        break;
                    }
                }
            }
        }
        if (freeBufferId >= 0)
        {
            ALint playState;
            alGetSourcei(alSrc, AL_SOURCE_STATE, &playState);
            if (playState == AL_STOPPED)
            {
                UnqueueProcessedBuffers();
            }
            alBufferQueued[freeBufferId] = true;
            alBufferSamples[freeBufferId] = samples;
            alBufferData(alBuffers[freeBufferId], AL_FORMAT_MONO16,
                &(*samples)[0], (ALsizei)(samples->size()) * 2, frequency);
            alSourceQueueBuffers(alSrc, 1, &alBuffers[freeBufferId]);
            if (playState != AL_PLAYING)
            {
                alSourcePlay(alSrc);
            }
            return DaVinciStatusSuccess;
        }
        else
        {
            return errc::device_or_resource_busy;
        }
    }

    void AudioPlayer::UnqueueProcessedBuffers()
    {
        ALint numBufferProcessed;
        ALuint alBuf[numBuffers];
        alGetSourcei(alSrc, AL_BUFFERS_PROCESSED, &numBufferProcessed);
        if (numBufferProcessed > 0)
        {
            alSourceUnqueueBuffers(alSrc, numBufferProcessed, alBuf);
            for (size_t i = 0; i < numBuffers; i++)
            {
                for (ALint j = 0; j < numBufferProcessed; j++)
                {
                    if (alBuffers[i] == alBuf[j])
                    {
                        alBufferQueued[i] = false;
                        break;
                    }
                }
            }
        }
    }
}
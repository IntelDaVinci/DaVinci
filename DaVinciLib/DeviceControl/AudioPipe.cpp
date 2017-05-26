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

#include "AudioPipe.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    AudioPipe::AudioPipe(const boost::shared_ptr<ALCdevice> waveInDevice) : waveIn(waveInDevice), shouldStop(false)
    {

    }

    AudioPipe::AudioPipe(const string &audioFile, uint64_t position) : shouldStop(false)
    {
        boost::shared_ptr<AudioFile> source = AudioFile::CreateReadFile(audioFile);
        if (source == nullptr)
        {
            throw DaVinciError(DaVinciStatus(errc::bad_file_descriptor));
        }
        if (source->Seek(position) < 0)
        {
            throw DaVinciError(DaVinciStatus(errc::operation_not_supported));
        }
        waveFileSource = source;
    }

    AudioPipe::~AudioPipe()
    {
        Stop();
    }

    DaVinciStatus AudioPipe::Start()
    {
        boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
        if (workThread != nullptr)
        {
            return errc::operation_not_permitted;
        }
        if (waveIn != nullptr)
        {
            for (auto player : playDevices)
            {
                player->Start();
            }
            shouldStop = false;
            workThread = boost::shared_ptr<boost::thread>(new boost::thread(
                boost::bind(&AudioPipe::WaveInSourceHandler, this)));
            SetThreadName(workThread->get_id(), "AudioPipeWaveIn");
            return DaVinciStatusSuccess;
        }
        else if (waveFileSource != nullptr && playDevices.size() > 0)
        {
            auto player = playDevices[0];
            player->Start();
            return player->Play(waveFileSource);
        }
        else
        {
            return errc::operation_not_permitted;
        }
    }

    DaVinciStatus AudioPipe::Stop()
    {
        boost::unique_lock<boost::recursive_mutex> lock(pipeLock);
        for (auto player : playDevices)
        {
            player->Stop();
        }
        shouldStop = true;
        auto theWorkThread = workThread;
        lock.unlock(); // avoid deadlock in the following thread join
        if (theWorkThread != nullptr)
        {
            theWorkThread->join();
        }
        lock.lock();
        workThread = nullptr;
        RemoveSinks();
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPipe::AddSink(const RecorderCallback &cb)
    {
        boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
        callbacks.push_back(cb);
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioPipe::AddSink(const boost::shared_ptr<ALCcontext> &playDevice)
    {
        boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
        DaVinciStatus status;
        auto player = boost::shared_ptr<AudioPlayer>(new AudioPlayer(playDevice));
        status = player->Start();
        if (DaVinciSuccess(status))
        {
            playDevices.push_back(player);
        }
        return status;
    }

    DaVinciStatus AudioPipe::AddSink(const string &wavFile)
    {
        boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
        auto f = AudioFile::CreateWriteWavFile(wavFile);
        if (f != nullptr)
        {
            waveFiles.push_back(f);
            return DaVinciStatusSuccess;
        }
        else
        {
            return errc::invalid_argument;
        }
    }

    void AudioPipe::RemoveSinks()
    {
        boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
        callbacks.clear();
        playDevices.clear();
        waveFiles.clear();
    }

    void AudioPipe::WaveInSourceHandler()
    {
        alcCaptureStart(waveIn.get());
        while (!shouldStop)
        {
            ALCint numSamples;
            alcGetIntegerv(waveIn.get(), ALC_CAPTURE_SAMPLES, 1, &numSamples);
            if (numSamples > 0)
            {
                auto samples = boost::shared_ptr<vector<short>>(new vector<short>());
                samples->resize(numSamples);
                alcCaptureSamples(waveIn.get(), &(*samples)[0], numSamples);
                {
                    boost::lock_guard<boost::recursive_mutex> lock(pipeLock);
                    for (auto cb : callbacks)
                    {
                        cb(samples);
                    }
                    for (auto f : waveFiles)
                    {
                        sf_write_short(f->Handle(), &(*samples)[0], numSamples);
                    }
                    for (auto player : playDevices)
                    {
                        player->Play(samples);
                    }
                }
            }
            else
            {
                ThreadSleep(pollingIntervalMs);
            }
        }
        alcCaptureStop(waveIn.get());
    }
}
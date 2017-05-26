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

#include "AL/al.h"

#include "AudioManager.hpp"

namespace DaVinci
{
    boost::recursive_mutex AudioManager::ContextLock;
    boost::shared_ptr<ALCdevice> AudioManager::NullALCdevice(nullptr, ALCCapDeviceDeleter);
    boost::shared_ptr<ALCcontext> AudioManager::NullALCcontext(nullptr, ALCcontextDeleter);

    AudioManager::AudioManager()
        : recordFromDevice(-1), playToDevice(-1), recordFromUser(-1), playDeviceAudio(-1)
    {
    }

    DaVinciStatus AudioManager::InitAudioDevices(
        const string &hostRecordDevice, const string &hostPlayDevice,
        const string &targetRecordDevice, const string &targetPlayDevice
        )
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        waveOutDeviceNames.push_back("Disabled");
        waveInDeviceNames.push_back("Disabled");
        GetStrListFromALStr(waveOutDeviceNames, alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER));
        GetStrListFromALStr(waveInDeviceNames, alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER));

        DaVinciStatus status = SetRecordFromUser(hostRecordDevice);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Unable to set audio device (" << hostRecordDevice << ") to record from user: " << status.message();
        }
        status = SetPlayDeviceAudio(hostPlayDevice);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Unable to set audio device (" << hostPlayDevice << " )to play device audio: " << status.message();
        }
        if (!targetPlayDevice.empty())
        {
            status = SetPlayToDevice(targetPlayDevice);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Unable to set audio device (" << targetPlayDevice << ") to play to device: " << status.message();
            }
        }
        if (!targetRecordDevice.empty())
        {
            status = SetRecordFromDevice(targetRecordDevice);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Unable to set audio device (" << targetRecordDevice << ") to record from device: " << status.message();
            }
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioManager::CloseAudioDevices()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        StopAllPipes();
        // explicitly destroy all audio devices and context
        recordFromDeviceWaveIn = NullALCdevice;
        recordFromUserWaveIn = NullALCdevice;
        playToDeviceWaveOut = NullALCcontext;
        playDeviceAudioWaveOut = NullALCcontext;
        waveOutDeviceNames.clear();
        waveInDeviceNames.clear();
        return DaVinciStatusSuccess;
    }

    template<typename D>
    DaVinciStatus AudioManager::SetDevice(const string &deviceName, const vector<string> deviceNames, boost::shared_ptr<D> &device, int &deviceIndex, boost::shared_ptr<D>(*creator)(const string &), void (*deleter)(D *))
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        if (deviceNames.size() <= 0)
            return errc::invalid_argument;
        if (deviceNames[deviceIndex + 1] != deviceName)
        {
            StopAllPipes();
            device = boost::shared_ptr<D>(nullptr, deleter);
            deviceIndex = -1;
            for (size_t i = 1; i < deviceNames.size(); i++)
            {
                if (deviceNames[i] == deviceName)
                {
                    device = creator(deviceName);
                    if (device != nullptr)
                    {
                        deviceIndex = (int)(i - 1);
                    }
                    else
                    {
                        deviceIndex = -1;
                    }
                    break;
                }
            }
        }
        if (deviceNames[deviceIndex + 1] == deviceName)
        {
            return DaVinciStatusSuccess;
        }
        else
        {
            return errc::no_such_device;
        }
    }

    DaVinciStatus AudioManager::SetRecordFromUser(const string &deviceName)
    {
        return SetDevice(deviceName, waveInDeviceNames, recordFromUserWaveIn, recordFromUser, CreateWaveInDevice, ALCCapDeviceDeleter);
    }

    DaVinciStatus AudioManager::SetRecordFromDevice(const string &deviceName)
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        DaVinciStatus status = SetDevice(deviceName, waveInDeviceNames, recordFromDeviceWaveIn, recordFromDevice, CreateWaveInDevice, ALCCapDeviceDeleter);
        if (DaVinciSuccess(status))
        {
            if (fromDevicePipe != nullptr)
            {
                fromDevicePipe->Stop();
                fromDevicePipe = nullptr;
            }
            StartFromDevicePipe();
        }
        return status;
    }

    DaVinciStatus AudioManager::SetPlayToDevice(const string &deviceName)
    {
        return SetDevice(deviceName, waveOutDeviceNames, playToDeviceWaveOut, playToDevice, CreateWaveOutDevice, ALCcontextDeleter);
    }

    DaVinciStatus AudioManager::SetPlayDeviceAudio(const string &deviceName)
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        DaVinciStatus status = SetDevice(deviceName, waveOutDeviceNames, playDeviceAudioWaveOut, playDeviceAudio, CreateWaveOutDevice, ALCcontextDeleter);
        if (DaVinciSuccess(status))
        {
            if (fromDevicePipe != nullptr)
            {
                fromDevicePipe->Stop();
                fromDevicePipe = nullptr;
            }
            StartFromDevicePipe();
        }
        return status;
    }

    bool AudioManager::RecordFromDeviceDisabled()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        return recordFromDeviceWaveIn == nullptr;
    }

    DaVinciStatus AudioManager::StartRecordFromUser(const string &file)
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        if (fromUserPipe != nullptr)
        {
            fromUserPipe->Stop();
        }
        fromUserPipe = boost::shared_ptr<AudioPipe>(new AudioPipe(recordFromUserWaveIn));
        fromUserPipe->AddSink(file);
        if (playToDeviceWaveOut != nullptr)
        {
            if (audioFileToDevicePipe != nullptr)
            {
                audioFileToDevicePipe->Stop();
            }
            fromUserPipe->AddSink(playToDeviceWaveOut);
        }
        DaVinciStatus status = fromUserPipe->Start();
        if (!DaVinciSuccess(status))
        {
            fromUserPipe = nullptr;
        }
        return status;
    }

    DaVinciStatus AudioManager::StopRecordFromUser()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        if (fromUserPipe != nullptr)
        {
            fromUserPipe->Stop();
            fromUserPipe = nullptr;
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AudioManager::PlayToDevice(const string &fileName, uint64_t position)
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        if (fromUserPipe != nullptr)
        {
            fromUserPipe->Stop();
        }
        if (playToDeviceWaveOut != nullptr)
        {
            try
            {
                DaVinciStatus status;
                audioFileToDevicePipe = boost::shared_ptr<AudioPipe>(new AudioPipe(fileName, position));
                audioFileToDevicePipe->AddSink(playToDeviceWaveOut);
                status = audioFileToDevicePipe->Start();
                if (!DaVinciSuccess(status))
                {
                    audioFileToDevicePipe = nullptr;
                }
                return status;
            }
            catch (...)
            {
                return errc::operation_not_supported;
            }
        }
        return DaVinciStatusSuccess;
    }

    void AudioManager::GetStrListFromALStr(vector<string> &strList, const char *alStr)
    {
        const size_t maxAlStrLen = 1000;
        if (alStr != nullptr)
        {
            while (*alStr != '\0')
            {
                bool found = false;
                for (size_t i = 0; i < strList.size(); i++)
                {
                    if (strList[i] == alStr)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    strList.push_back(alStr);
                }
                size_t alStrLen = strnlen(alStr, maxAlStrLen);
                if (alStrLen < maxAlStrLen)
                {
                    alStr += alStrLen + 1;
                }
                else
                {
                    return;
                }
            }
        }
    }

    boost::shared_ptr<ALCdevice> AudioManager::CreateWaveInDevice(const string &deviceName)
    {
        ALCdevice *device = alcCaptureOpenDevice(deviceName.c_str(), 44100, AL_FORMAT_MONO16, 44100);
        if (device == nullptr)
        {
            return NullALCdevice;
        }
        return boost::shared_ptr<ALCdevice>(device, ALCCapDeviceDeleter);
    }

    boost::shared_ptr<ALCcontext> AudioManager::CreateWaveOutDevice(const string &deviceName)
    {
        ALCdevice *device = alcOpenDevice(deviceName.c_str());
        if (device == nullptr)
        {
            return NullALCcontext;
        }
        ALCcontext *context = alcCreateContext(device, NULL);
        return boost::shared_ptr<ALCcontext>(context, ALCcontextDeleter);
    }

    void AudioManager::ALCCapDeviceDeleter(ALCdevice *d)
    {
        if (d != nullptr)
        {
            alcCaptureCloseDevice(d);
        }
    }

    void AudioManager::ALCcontextDeleter(ALCcontext *c)
    {
        if (c != nullptr)
        {
            boost::lock_guard<boost::recursive_mutex> lock(ContextLock);
            if (c == alcGetCurrentContext())
            {
                alcMakeContextCurrent(NULL);
            }
            ALCdevice *d = alcGetContextsDevice(c);
            alcDestroyContext(c);
            alcCloseDevice(d);
        }
    }

    vector<string> AudioManager::GetWaveInDeviceNames()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        return waveInDeviceNames;
    }

    vector<string> AudioManager::GetWaveOutDeviceNames()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        return waveOutDeviceNames;
    }

    void AudioManager::SetRecorderCallback(const AudioPipe::RecorderCallback &cb)
    {
        recorderCallback = cb;
    }

    DaVinciStatus AudioManager::StartFromDevicePipe()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        assert(fromDevicePipe == nullptr);
        if (recordFromDeviceWaveIn != nullptr)
        {
            fromDevicePipe = boost::shared_ptr<AudioPipe>(new AudioPipe(recordFromDeviceWaveIn));
            if (!recorderCallback.empty())
            {
                fromDevicePipe->AddSink(recorderCallback);
            }
            if (playDeviceAudioWaveOut != nullptr)
            {
                fromDevicePipe->AddSink(playDeviceAudioWaveOut);
            }
            DaVinciStatus status = fromDevicePipe->Start();
            if (!DaVinciSuccess(status))
            {
                fromDevicePipe = nullptr;
            }
            return status;
        }
        return DaVinciStatusSuccess;
    }

    void AudioManager::StopAllPipes()
    {
        boost::lock_guard<boost::recursive_mutex> lock(deviceLock);
        if (fromDevicePipe != nullptr)
        {
            fromDevicePipe->Stop();
            fromDevicePipe = nullptr;
        }
        if (audioFileToDevicePipe != nullptr)
        {
            audioFileToDevicePipe->Stop();
            audioFileToDevicePipe = nullptr;
        }
        if (fromUserPipe != nullptr)
        {
            fromUserPipe->Stop();
            fromUserPipe = nullptr;
        }
    }
}
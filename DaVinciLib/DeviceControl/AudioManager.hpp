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

#ifndef __AUDIO_MANAGER_HPP__
#define __AUDIO_MANAGER_HPP__

#include "boost/function.hpp"
#include "AL/alc.h"
#include "AudioFile.hpp"
#include "DaVinciCommon.hpp"
#include "AudioPipe.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// Manage all audio stuff
    /// </summary>
    class AudioManager : public SingletonBase<AudioManager>
    {
    public:
        /// <summary>
        /// The current OpenAL context is global for the process. Declare the mutex
        /// to protect the access.
        /// </summary>
        static boost::recursive_mutex ContextLock;

        // used to represent nullptr with incomplete type
        static boost::shared_ptr<ALCdevice> NullALCdevice;
        static boost::shared_ptr<ALCcontext> NullALCcontext;

        static boost::shared_ptr<ALCdevice> CreateWaveInDevice(const string &deviceName);
        static boost::shared_ptr<ALCcontext> CreateWaveOutDevice(const string &deviceName);

        static void GetStrListFromALStr(vector<string> &strList, const char *alStr);

        /// <summary>
        /// Trim a wav file according to the start and end time in millisecond.
        /// </summary>
        /// <param name="inWavFile">Input wav file to trim</param>
        /// <param name="outWavFile">Output trimmed wav file</param>
        /// <param name="start">Start millisecond</param>
        /// <param name="end">End millisecond</param>
        /// <returns>true if the trimming operation is successful, false otherwise</returns>
        static bool TrimWavFile(const string &inWavFile, const string &outWavFile, double start, double end);

        AudioManager();

        /// <summary>
        /// Initialize the audio recording devices
        /// </summary>
        DaVinciStatus InitAudioDevices(const string &hostRecordDevice, const string &hostPlayDevice, const string &targetRecordDevice = "", const string &targetPlayDevice = "");
        /// <summary>
        /// Close the audio recording devices
        /// </summary>
        DaVinciStatus CloseAudioDevices();

        /// <summary>
        /// Whether audio recording is disabled
        /// </summary>
        /// <returns></returns>
        bool RecordFromDeviceDisabled();

        /// <summary>
        /// Get device names for audio recording devices
        /// </summary>
        /// <returns></returns>
        vector<string> GetWaveInDeviceNames();

        /// <summary>
        /// Get device names for audio playback devices
        /// </summary>
        /// <returns></returns>
        vector<string> GetWaveOutDeviceNames();

        /// <summary>
        /// Start recording audio from the user.
        /// The method should always be called from the UI thread.
        /// </summary>
        /// <param name="wavFile">
        /// The wav audio file to be saved. It can be empty, then no file is saved.
        /// </param>
        DaVinciStatus StartRecordFromUser(const string &wavFile);

        /// <summary>
        /// Stop recording audio from the user.
        /// The method should always be called from the UI thread.
        /// </summary>
        DaVinciStatus StopRecordFromUser();

        /// <summary>
        /// Play the specified audio file to the device
        /// </summary>
        /// <param name="fileName">The audio file name to play</param>
        /// <param name="position">The start position (ms) to play</param>
        DaVinciStatus PlayToDevice(const string &fileName, uint64_t position = 0);
        
        /// <summary>
        /// Set device id for playback to device based on name
        /// Can only be called from GUI thread
        /// </summary>
        /// <param name="deviceName"></param>
        DaVinciStatus SetPlayToDevice(const string &deviceName);

        /// <summary>
        /// Set device id for recording from user based on name.
        /// Can only be called from GUI thread
        /// </summary>
        /// <param name="deviceName"></param>
        DaVinciStatus SetRecordFromUser(const string &deviceName);
        
        /// <summary>
        /// Set device id for playback device audio
        /// Can only be called from GUI thread
        /// </summary>
        /// <param name="deviceName"></param>
        DaVinciStatus SetPlayDeviceAudio(const string &deviceName);

        /// <summary>
        /// Select the audio recording device base on name
        /// Can only be called from GUI thread
        /// </summary>
        /// <param name="deviceName"></param>
        DaVinciStatus SetRecordFromDevice(const string &deviceName);

        void SetRecorderCallback(const AudioPipe::RecorderCallback &cb);

    private:
        static void ALCCapDeviceDeleter(ALCdevice *d);
        static void ALCcontextDeleter(ALCcontext *c);

        /// <summary>
        /// The lock to protect device configuration changes.
        /// </summary>
        boost::recursive_mutex deviceLock;

        int recordFromDevice;
        int playToDevice;
        int recordFromUser;
        int playDeviceAudio;

        boost::shared_ptr<ALCdevice> recordFromDeviceWaveIn;
        boost::shared_ptr<ALCdevice> recordFromUserWaveIn;
        boost::shared_ptr<ALCcontext> playToDeviceWaveOut;
        boost::shared_ptr<ALCcontext> playDeviceAudioWaveOut;
        vector<string> waveInDeviceNames;
        vector<string> waveOutDeviceNames;
        AudioPipe::RecorderCallback recorderCallback;
        boost::shared_ptr<AudioPipe> fromDevicePipe; // recordFromDeviceWaveIn -> playDeviceAudioWaveOut/recorderCallback
        boost::shared_ptr<AudioPipe> audioFileToDevicePipe; // audioFileToDevice -> playToDeviceWaveOut
        boost::shared_ptr<AudioPipe> fromUserPipe; // recordFromUserWaveIn -> playToDeviceWaveOut/audioFileFromUser

        DaVinciStatus StartFromDevicePipe();
        /// <summary> Must hold the deviceLock before calling the function </summary>
        void StopAllPipes();

        template<typename D>
        DaVinciStatus SetDevice(const string &deviceName, const vector<string> deviceNames, boost::shared_ptr<D> &device, int &deviceIndex, boost::shared_ptr<D>(*creator)(const string &), void(*deleter)(D*));

        DaVinciStatus SetRecordDevice(const string &deviceName, const vector<string> deviceNames, boost::shared_ptr<ALCdevice> &recordDevice, int &recordDeviceIndex);
        DaVinciStatus SetPlaybackDevice(const string &deviceName, const vector<string> deviceNames, boost::shared_ptr<ALCcontext> &playbackDevice, int &playbackDeviceIndex);

    };
}

#endif
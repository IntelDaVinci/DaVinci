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

#ifndef __AUDIO_PIPE_HPP__
#define __AUDIO_PIPE_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <string>
#include <vector>

#include "boost/thread/thread.hpp"
#include "boost/thread/recursive_mutex.hpp"
#include "boost/function.hpp"
#include "AL/alc.h"
#include "AudioFile.hpp"
#include "AudioPlayer.hpp"

#include "DaVinciStatus.hpp"

namespace DaVinci
{
    using namespace std;

    class AudioPipe : public boost::enable_shared_from_this<AudioPipe>
    {
    public:
        typedef boost::function<void(const boost::shared_ptr<vector<short>> &)> RecorderCallback;

        explicit AudioPipe(const boost::shared_ptr<ALCdevice> waveInDevice);
        explicit AudioPipe(const string &audioFile, uint64_t position = 0);
        ~AudioPipe();
        DaVinciStatus Start();
        DaVinciStatus Stop();
        DaVinciStatus AddSink(const RecorderCallback &cb);
        DaVinciStatus AddSink(const boost::shared_ptr<ALCcontext> &playDevice);
        DaVinciStatus AddSink(const string &wavFile);

    private:
        static const int pollingIntervalMs = 100;

        boost::recursive_mutex pipeLock;
        boost::atomic<bool> shouldStop;
        boost::shared_ptr<boost::thread> workThread;
        boost::shared_ptr<ALCdevice> waveIn;
        boost::shared_ptr<AudioFile> waveFileSource;
        vector<RecorderCallback> callbacks;
        vector<boost::shared_ptr<AudioPlayer>> playDevices;
        vector<boost::shared_ptr<AudioFile>> waveFiles;

        void WaveInSourceHandler();
        void RemoveSinks();
    };
}

#endif
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

#ifndef __AUDIO_FILE_HPP__
#define __AUDIO_FILE_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <string>
#include "sndfile.h"
#include "DaVinciStatus.hpp"

namespace DaVinci
{
    using namespace std;

    class AudioFile : public boost::enable_shared_from_this<AudioFile>
    {
    public:
        static boost::shared_ptr<AudioFile> CreateReadFile(const string &audioFile);
        static boost::shared_ptr<AudioFile> CreateWriteWavFile(const string &wavFile);

        /// <summary> Trim the source WAV file into a mono WAV file from start to end. </summary>
        ///
        /// <param name="inWavFile">  The in WAV file, could be mono or dual channels. </param>
        /// <param name="outWavFile"> The out WAV file, mono channel. </param>
        /// <param name="start">      The start in milliseconds. </param>
        /// <param name="end">        The end in milliseconds. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        static DaVinciStatus TrimWavFile(const string &inWavFile, const string &outWavFile, uint64_t start, uint64_t end);

        AudioFile(const string &audioFile, int mode, const SF_INFO *info);
        ~AudioFile();
        SNDFILE *Handle();
        const SF_INFO &Info();
        sf_count_t Seek(uint64_t ms);
    private:
        SNDFILE *fileHandle;
        SF_INFO sfInfo;

        /// <summary> Time to position in samples. </summary>
        ///
        /// <param name="ms"> The milliseconds. </param>
        ///
        /// <returns> The sample number. </returns>
        sf_count_t TimeToPos(uint64_t ms) const;
    };
}

#endif
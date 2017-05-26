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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __AUDIO_CHECKER_HPP__
#define __AUDIO_CHECKER_HPP__

#include "Checker.hpp"
#include "ScriptReplayer.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary> A base class for resultchecker. </summary>
    class AudioChecker : public Checker
    {
    public:
        explicit AudioChecker(const boost::weak_ptr<ScriptReplayer> obj);

        virtual ~AudioChecker();

        int GetMismatches();

        void DoOfflineChecking();

    private:
        boost::shared_ptr<QScript> qs;
        int mismatches;

        bool IsAudioClipMatched(int lineNumber, const std::string &sourceAudioName, double sourceStart, double sourceEnd, const std::string &targetAudioName, double targetStart, double targetEnd);

        /// <summary> Gets audio clip name constructed with the line number </summary>
        ///
        /// <param name="audioName">  Name of the audio. </param>
        /// <param name="lineNumber"> The line number. </param>
        ///
        /// <returns> The audio clip name. </returns>
        std::string GetAudioClipName(const std::string &audioName, int lineNumber);
    };
}

#endif
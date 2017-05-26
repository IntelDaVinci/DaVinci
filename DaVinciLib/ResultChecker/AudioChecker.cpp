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

#include "AudioChecker.hpp"
#include "AudioRecognize.hpp"
#include "AudioFile.hpp"
#include "AudioFormGenerator.hpp"

namespace DaVinci
{
    AudioChecker::AudioChecker(const boost::weak_ptr<ScriptReplayer> obj) : Checker(), mismatches(0)
    {
        SetCheckerName("AudioChecker");

        boost::shared_ptr<ScriptReplayer> currentScriptTmp = obj.lock();

        if ((currentScriptTmp == nullptr) || (currentScriptTmp->GetQS() == nullptr))
        {
             DAVINCI_LOG_ERROR << "Failed to Init AudioChecker()!";
             throw 1;
        }

        qs = currentScriptTmp->GetQS();
    }

    AudioChecker::~AudioChecker()
    {

    }

    int AudioChecker::GetMismatches()
    {
        return mismatches;
    }

    void AudioChecker::DoOfflineChecking()
    {
        SetResult(CheckerResult::Pass);
        assert(qs != nullptr);
        if (qs == nullptr)
        {
            DAVINCI_LOG_FATAL << "      QS file is missing when trying to check Audio result!";
            return;
        }
        string sourceAudioName = qs->GetAudioFileName();
        string targetAudioName = qs->GetResourceFullPath(qs->GetScriptName() + "_replay.wav");
        if (!boost::filesystem::exists(sourceAudioName))
        {
            DAVINCI_LOG_WARNING << "      Audio track not found: " << sourceAudioName;
            return;
        }
        if (!boost::filesystem::exists(targetAudioName))
        {
            DAVINCI_LOG_WARNING << "      Audio track not found: " << targetAudioName;
            return;
        }
        boost::shared_ptr<AudioFormGenerator> audioFormGen = boost::shared_ptr<AudioFormGenerator>(new AudioFormGenerator());
        audioFormGen->SaveAudioWave(sourceAudioName);
        audioFormGen->SaveAudioWave(targetAudioName);
        double sourceTsOn = 0, targetTsOn = 0;
        bool inMatch = false;
        int matchOnLine = 0;
        for (int ip = 1; ip <= static_cast<int>(qs->NumEventAndActions()); ip++)
        {
            boost::shared_ptr<QSEventAndAction> qsEvent = qs->EventAndAction(ip);
            if (qsEvent == nullptr)
            {
                break;
            }
            if (qsEvent->Opcode() == QSEventAndAction::OPCODE_AUDIO_MATCH_ON)
            {
                if (!inMatch)
                {
                    inMatch = true;
                    matchOnLine = qsEvent->LineNumber();
                    sourceTsOn = qsEvent->TimeStamp();
                    // TODO: use timestamp_playing only compare the last time
                    // when the opcode is executed. It will miss the cases when
                    // the opcode is executed 
                    targetTsOn = qsEvent->TimeStampReplay();
                }
            }
            else if (qsEvent->Opcode() == QSEventAndAction::OPCODE_AUDIO_MATCH_OFF)
            {
                if (inMatch)
                {
                    double sourceTsOff = qsEvent->TimeStamp();
                    double targetTsOff = qsEvent->TimeStampReplay();
                    inMatch = false;
                    if (!IsAudioClipMatched(matchOnLine, sourceAudioName, sourceTsOn, sourceTsOff, targetAudioName, targetTsOn, targetTsOff))
                    {
                        DAVINCI_LOG_INFO << "Audio clip mismatches @" << matchOnLine;
                        mismatches++;
                        SetResult(CheckerResult::Fail);
                    }
                }
            }
        }
    }

    std::string AudioChecker::GetAudioClipName(const std::string &audioName, int lineNumber)
    {
        boost::filesystem::path audioPath(audioName);
        return qs->GetResourceFullPath(audioPath.stem().string() + "_" + boost::lexical_cast<string>(lineNumber) + audioPath.extension().string());
    }

    bool AudioChecker::IsAudioClipMatched(int lineNumber, const std::string &sourceAudioName, double sourceStart, double sourceEnd, const std::string &targetAudioName, double targetStart, double targetEnd)
    {
        std::string sourceAudioClip = GetAudioClipName(sourceAudioName, lineNumber);
        std::string targetAudioClip = GetAudioClipName(targetAudioName, lineNumber);

        DAVINCI_LOG_DEBUG << "Source audio clip " << sourceAudioClip << " start: " << sourceStart << ", end: " << sourceEnd;
        DAVINCI_LOG_DEBUG << "Target audio clip " << targetAudioClip << " start: " << targetStart << ", end: " << targetEnd;

        if (DaVinciSuccess(AudioFile::TrimWavFile(sourceAudioName, sourceAudioClip, static_cast<uint64_t>(sourceStart), static_cast<uint64_t>(sourceEnd))) &&
            DaVinciSuccess(AudioFile::TrimWavFile(targetAudioName, targetAudioClip, static_cast<uint64_t>(targetStart), static_cast<uint64_t>(targetEnd))))
        {
            AudioRecognize audioRecognize(targetAudioClip);
            return audioRecognize.Detect(sourceAudioClip, 0.1f);
        }
        else
        {
            return false;
        }
    }
}
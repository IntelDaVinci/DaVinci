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

#ifndef __SCRIPT_ENGINE_BASE_HPP__
#define __SCRIPT_ENGINE_BASE_HPP__

#include "boost/chrono/chrono.hpp"

#include "QScript.hpp"
#include "TestInterface.hpp"
#include "StopWatch.hpp"
#include "AudioFile.hpp"
#include "VideoWriterProxy.hpp"

namespace DaVinci
{
    using namespace std;

    class ScriptEngineBase : public TestInterface
    {
    public:
        ScriptEngineBase(const string &script);
        virtual ~ScriptEngineBase();
        virtual bool Init() override;
        virtual void Destroy() override;
        virtual void CloseMediaFiles();
        virtual Mat ProcessFrame(const Mat &frame, const double timeStamp) override;
        virtual void ProcessWave(const boost::shared_ptr<vector<short>> &samples) override;
        boost::shared_ptr<QScript> GetQS();
        vector<double> GetTimeStampList();

    protected:
        /// <summary> The qs holding the recording or replaying script </summary>
        boost::shared_ptr<QScript> qs;
        /// <summary> Filename of the qs file. </summary>
        string qsFileName;
        /// <summary> Filename of the recorded or replayed qts file </summary>
        string qtsFileName;
        /// <summary> Filename of the recorded or replayed video file. </summary>
        string videoFileName;
        /// <summary> Filename of the audio file. </summary>
        string audioFileName;
        /// <summary> Filename of the sensor check file. </summary>
        string sensorCheckFileName;
        /// <summary> true to record video. </summary>
        bool needVideoRecording;
        /// <summary> portrait and landscape navigation bar height
        vector<int> navigationBarHeights;
        /// <summary> true to multiLayer mode. </summary>
        bool isMultiLayerMode;
        /// <summary> The watch tracking QS playback or recording. </summary>
        StopWatch qsWatch;
        /// <summary> TimeStamp of each frame. </summary>
        vector<double> timeStampList;
        /// <summary> Current frame index in recorded file. </summary>
        std::atomic<size_t> currentFrameIndex;
        

    private:
        boost::mutex videoMutex;
        VideoWriterProxy videoWriter;
        Size videoFrameSize;
        boost::mutex audioMutex;
        boost::shared_ptr<AudioFile> audioFile;
        
    };
}

#endif
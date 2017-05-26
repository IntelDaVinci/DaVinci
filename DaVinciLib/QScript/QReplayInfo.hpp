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


#ifndef __QREPLAY_INFO_HPP__
#define __QREPLAY_INFO_HPP__

#include <string>

#include "DaVinciCommon.hpp"
#include "QScript.hpp"
#include "VideoChecker.hpp"

namespace DaVinci
{
    using namespace std;
    
    class QReplayInfo
    {
    public:

        class QReplayFrameInfo
        {
        public: 
            
            QReplayFrameInfo(double ts, FrameTypeInVideo type=FrameTypeInVideo::Normal, string lineLabel = "",  string value="");

            FrameTypeInVideo & FrameType();                
            double & TimeStamp();    
            string & Value();
            string & LineLabel();
        private:
            double timeStamp;
            FrameTypeInVideo frameType;
            string lineLabel;
            string value;
        };

        QReplayInfo(const vector<double> timeStamps, const vector<QScript::TraceInfo> traces);
        QReplayInfo();
        QReplayInfo(const string &replayQtsFile);
        void SaveToFile(const string &fileName);
        static boost::shared_ptr<QReplayInfo> Load(const string &filename);

        vector<QReplayFrameInfo> & Frames();
        
    private:
        vector<QReplayFrameInfo> frames;   
        
        void PopulateFrames(const vector<double> timeStamps, const vector<QScript::TraceInfo> traces);
    };

}


#endif
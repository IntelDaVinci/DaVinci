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

#include "QReplayInfo.hpp"
#include "QScript.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string/join.hpp"
#include "boost/algorithm/string.hpp"


using namespace boost::filesystem;

namespace DaVinci
{
    QReplayInfo::QReplayInfo(const vector<double> timeStamps, const vector<QScript::TraceInfo> traces) 
    {   
        PopulateFrames(timeStamps, traces);  
        
    }    

    QReplayInfo::QReplayInfo()
    {}

    QReplayInfo::QReplayInfo( const string & qtsFile)
    {
        assert(qtsFile != "");
        
        path qtsPath(qtsFile);
        if (is_regular_file(qtsFile))
        {           
           vector<double> timeStamps;           
           vector<QScript::TraceInfo> traces;
           if(QScript::ParseTimeStampFile(qtsFile, timeStamps, traces) == DaVinciStatusSuccess)
           {
               PopulateFrames(timeStamps, traces);                
           }

           
        }
        else
        {
            DAVINCI_LOG_WARNING << "Q script replay detail file invalid or not found: " << qtsFile;
        }
    }

    QReplayInfo::QReplayFrameInfo::QReplayFrameInfo(double ts, FrameTypeInVideo type, string lineLabel, string value):timeStamp(ts),frameType(type),lineLabel(lineLabel),value(value)
    {
    
    }


    void QReplayInfo::PopulateFrames(const vector<double> timeStamps, const vector<QScript::TraceInfo> traces)
    {
        size_t tLen = traces.size();
        
        for(size_t i = 0, j = 0; i < timeStamps.size(); i++)
        {
            if(j < tLen)
            {  
                QScript::TraceInfo tInfo = traces[j];
                if(timeStamps[i] >= tInfo.TimeStamp())
                {
                    while(j < tLen  && timeStamps[i] >= tInfo.TimeStamp())
                    {
                        frames.push_back(QReplayFrameInfo(timeStamps[i], FrameTypeInVideo::Event, boost::lexical_cast<string>(tInfo.LineNumber())));                        
                        j++;                        
                        if(j < tLen)
                        {
                            tInfo = traces[j];
                        }                        
                    }                    
                }
                else
                {
                    frames.push_back(QReplayFrameInfo(timeStamps[i], FrameTypeInVideo::Normal));
                }
            }
            else
            {
                frames.push_back(QReplayFrameInfo(timeStamps[i], FrameTypeInVideo::Normal));
            }
        }
    }
    double & QReplayInfo::QReplayFrameInfo::TimeStamp()
    {
        return timeStamp;
    }

    FrameTypeInVideo & QReplayInfo::QReplayFrameInfo::FrameType()
    {
        return frameType;
    }
    string & QReplayInfo::QReplayFrameInfo::Value()
    {
        return value;
    }

    string & QReplayInfo::QReplayFrameInfo::LineLabel()
    {
        return lineLabel;
    }

    vector<QReplayInfo::QReplayFrameInfo> & QReplayInfo::Frames()
    {
        return frames;
    }
   

    void QReplayInfo::SaveToFile(const string &fileName)
    {
       // assert(fileName != nullptr);
       ofstream ris;
        ris.open(fileName, ios::out | ios::trunc);
        if (ris.is_open())
        {

            size_t i = 0, sz = frames.size();

            while(i < sz)
            {
                QReplayFrameInfo ts = frames[i];
                ris << static_cast<int64_t>(ts.TimeStamp()) << ";";
                switch(ts.FrameType())
                {
                case FrameTypeInVideo::Normal:
                    ris << "NORMAL";
                    break;
                case FrameTypeInVideo::Static:
                    ris << "STATIC";
                    break;
                case FrameTypeInVideo::Event:
                    ris << "EVENT";
                    break;
                case FrameTypeInVideo::Warning:
                    ris << "WARNING";
                    break;
                case FrameTypeInVideo::Error:
                    ris << "ERROR";
                    break;
                default:
                    ris << "NORMAL";

                }

                ris << ";" << ts.LineLabel();
                while(i < sz-1 && ts.TimeStamp() == frames[i+1].TimeStamp())
                {                    
                    ris << "," << frames[i+1].LineLabel();
                    i++;
                }
                ris << ";"  << ts.Value();
                ris << endl;
                i++;
            }        
            
            ris.close();
        }
        else
        {
            DAVINCI_LOG_ERROR << "Unable to open QTS file " << fileName << " for writing.";
        }

    }

   
 
    boost::shared_ptr<QReplayInfo> QReplayInfo::Load(const string &filename)
    {
        path scriptPath(filename);
        if (!is_regular_file(scriptPath))
        {
            DAVINCI_LOG_WARNING << "Q script replay detail file invalid or not found: " << filename;
            return nullptr;
        }        

        DAVINCI_LOG_INFO << "Loading Q script replay detail file: " << filename;
        vector<string> replayLines;
        DaVinciStatus status = ReadAllLines(filename, replayLines);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Cannot load Q script replay file " << filename << " for parsing: " << status.message();
            return nullptr;
        }
        vector<QReplayFrameInfo> frameInfo;
        for(string info : replayLines)
        {
            vector<string> strs;
            boost::algorithm::split(strs, info, boost::algorithm::is_any_of(";"));
            
            if(strs.size() < 4)
            {
                continue;
            }

            double timeStamp = boost::lexical_cast<double>( strs[0]);
            
            FrameTypeInVideo fType = FrameTypeInVideo::Normal;

            string  s = strs[1];
            to_upper(s);

            if(s == "ERROR")
            {
                fType = FrameTypeInVideo::Error;    
            }
            else if(s == "WARNING")
            {
                fType = FrameTypeInVideo::Warning;    
            }
            else if(s == "EVENT")
            {
                fType = FrameTypeInVideo::Event;    
            }
            else if(s == "STATIC")
            {
                fType = FrameTypeInVideo::Static;    
            }


            string value = strs[3];
            size_t valueIndex = 4;
            while(valueIndex < strs.size())
            {
                value += ";" + strs[valueIndex];
                valueIndex++;
            }

            vector<string> ops;

            boost::algorithm::split(ops, strs[2], boost::algorithm::is_any_of(","));
            unsigned int opStartIndex = 0;
            if(ops.size() > 0)
            {
                while(opStartIndex < ops.size())
                {
                    string op = ops[opStartIndex];
                    QReplayFrameInfo qFrameInfo = QReplayFrameInfo(timeStamp, fType, op, value);  
                    frameInfo.push_back(qFrameInfo);    
                    opStartIndex++;
                } 
            }
            else
            {
                QReplayFrameInfo qFrameInfo = QReplayFrameInfo(timeStamp, fType, "", value);  
                frameInfo.push_back(qFrameInfo);                
            }            
        }
        shared_ptr<QReplayInfo> qReplayInfo = boost::shared_ptr<QReplayInfo>(new QReplayInfo());
        qReplayInfo->Frames() = frameInfo;
        return qReplayInfo;

    }
   
   
   
 

   
}
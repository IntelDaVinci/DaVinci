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

#include "NoResponseChecker.hpp"




using namespace std;
using namespace cv;

namespace DaVinci
{
    NoResponseChecker::NoResponseChecker(const string &videoFileName, const string &qfdFileName, const string& qsFile):replayVideoFileName(videoFileName),replayqfdFileName(qfdFileName),qsFileName(qsFile)
    {
        qReplayInfo = QReplayInfo::Load(replayqfdFileName);
        qs =  QScript::Load(qsFileName);
        
    }
    NoResponseChecker::~NoResponseChecker()
    {
    }

    bool NoResponseChecker::IsStaticMatched(const cv::Mat &currentFrame, const cv::Mat &referenceFrame, double threshold)
    {
        cv::Mat diffFrame;
        vector<Mat> diffChannels;
        double totalCount = 1, noneZeroCount = 0;

        if ((!referenceFrame.empty()) && (!currentFrame.empty()))
        {
            totalCount = currentFrame.size().width * currentFrame.size().height;

            compare(currentFrame, referenceFrame, diffFrame, CMP_NE);
            split(diffFrame, diffChannels);
            noneZeroCount = countNonZero(diffChannels[0]);// + countNonZero(diffChannels[1]) + countNonZero(diffChannels[2]);

            if ((noneZeroCount != 0) && (totalCount != 0) && ((noneZeroCount/totalCount) > threshold))
                return false;

            return true;
        }
        else
            return false;
    }

    void NoResponseChecker::CheckStaticFrames()
    {

        VideoCaptureProxy replayCapture;

        
        cv::Mat startFrame, cur;

        replayCapture.open(replayVideoFileName);                    

        double maxCaptureFrame = replayCapture.get(CV_CAP_PROP_FRAME_COUNT);
        size_t index = 0, qfd_size = qReplayInfo->Frames().size();

        size_t qfdIndex = index;
        if(maxCaptureFrame < 2)
        {
            return;
        }

        replayCapture.read(startFrame);
        index++;
        qfdIndex++;
        replayCapture.set(CV_CAP_PROP_POS_FRAMES, (double)index);       

        while(index < maxCaptureFrame && qfdIndex < qfd_size)
        {
            replayCapture.read(cur);
            if(IsStaticMatched(cur, startFrame))
            {
                // set no event frame to static
                if(qReplayInfo->Frames()[qfdIndex].FrameType() == FrameTypeInVideo::Event)
                {
                    while(qfdIndex < qfd_size-1 && qReplayInfo->Frames()[qfdIndex].TimeStamp() == qReplayInfo->Frames()[qfdIndex+1].TimeStamp())
                    {                        
                        qfdIndex++;
                    }                           
                }
                else
                {                    
                    qReplayInfo->Frames()[qfdIndex].FrameType() = FrameTypeInVideo::Static;     
                }                
            }
            else
            {
                startFrame = cur; 
            }
            index++;
            qfdIndex++;
            if(index < maxCaptureFrame)
            {
                replayCapture.set(CV_CAP_PROP_POS_FRAMES, (double)index);
            }
            
        }
        
    }

    bool NoResponseChecker::HasNoResponse()
    {

        bool ret = false;

        VideoCaptureProxy replayCapture;

        replayCapture.open(replayVideoFileName);            

        if ((!replayCapture.isOpened()))
        {
            
            DAVINCI_LOG_ERROR <<"Cannot open replay video";
            return false;
        }

        if(replayqfdFileName != "" && !boost::filesystem::exists(replayqfdFileName))
        {
            
            DAVINCI_LOG_ERROR <<"Cannot open replay video detail file";
            return false;
        }        
        CheckStaticFrames();        
        size_t staticStart = 0;        
        int touchCount = 0;
        vector<QReplayInfo::QReplayFrameInfo> & frames = qReplayInfo->Frames();
        size_t frameIndex = 1;
        size_t frameSize = frames.size();
        int maxTouchNoResponse = 2;

        while(frameIndex < frameSize)
        {
            QReplayInfo::QReplayFrameInfo frame = frames[frameIndex];
            
            if(frame.FrameType() == FrameTypeInVideo::Static || frame.FrameType() == FrameTypeInVideo::Event)
            {
                if(frame.LineLabel() != "")
                {
                    vector<string> ops;
                    boost::algorithm::split(ops, frame.LineLabel(), boost::algorithm::is_any_of(";"));
                    for(string op : ops)
                    {   
                        if(IsTouchEvent(boost::lexical_cast<int>(op)))
                        {
                            // skip the frame before click
                            if(staticStart == 0)
                            {
                                staticStart = frameIndex;
                            }
                            touchCount++;
                        }
                    }                                        
                }

                if(frameIndex == frameSize-1)
                {
                    if(touchCount > maxTouchNoResponse)
                    {       
                        for(auto i = staticStart; i < frameIndex; i++)
                        {
                            if(frames[i].FrameType() != FrameTypeInVideo::Event)
                            {
                                frames[i].FrameType() = FrameTypeInVideo::Warning;
                            }                            
                        }                          
                        ret = true;
                        SaveNoResponseVideoClips((int)staticStart, (int)frameIndex);
                    }
                }
            }
            else
            {       
                if(touchCount > maxTouchNoResponse)
                {       
                    for(auto i = staticStart; i < frameIndex; i++)
                    {
                        if(frames[i].FrameType() != FrameTypeInVideo::Event)
                        {
                            frames[i].FrameType() = FrameTypeInVideo::Warning;
                        }                            
                    }                          
                    ret = true;
                    SaveNoResponseVideoClips((int)staticStart, (int)frameIndex);
                }
                staticStart = frameIndex;
                touchCount= 0;
            }
            frameIndex++;
        }        
        qReplayInfo->SaveToFile(replayqfdFileName);
        return ret;
    }

    
    bool NoResponseChecker::IsTouchEvent(int lineLabel)
    {   
        int opcode = qs->EventAndAction( qs->LineNumberToIp(lineLabel))->Opcode();

        if(opcode == QSEventAndAction::OPCODE_TOUCHDOWN ||
            opcode == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL
            || opcode == QSEventAndAction::OPCODE_MULTI_TOUCHDOWN
            || opcode == QSEventAndAction::OPCODE_MULTI_TOUCHDOWN_HORIZONTAL
            || opcode == QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY)
        {
            return true;
        }
        else
        {
            return false;
        }       
    }

    void NoResponseChecker::SaveNoResponseVideoClips(int startIndex, int endIndex)
    {
        // Need to save 20 frames before and after real blank frames into video.

        VideoCaptureProxy capture;
        cv::Mat curFrame;
        DAVINCI_LOG_INFO << "Detect no response frames from frame: " << boost::lexical_cast<string>(startIndex) << " to frame: " << boost::lexical_cast<string>(endIndex) << endl;
        double frameRate = 5.0;
        VideoWriter vWriter;
        // If abnormal frames detected, save a piece of video to a folder named "Video_Abnormal_Frames"
        string saveVideoFolder = "NoResponse_Frames";
        string logFolder="";
        if(TestReport::currentQsLogPath == "")
        {
            boost::filesystem::path filePath(replayVideoFileName);
            filePath = system_complete(filePath);
            logFolder = filePath.parent_path().string();
        }
        else
        {
            logFolder = TestReport::currentQsLogPath;
        }
        string videoPath = logFolder + "/" + saveVideoFolder;
        boost::filesystem::path saveVideo(videoPath);
        if (!boost::filesystem::is_directory(saveVideo))
        {
            boost::filesystem::create_directory(saveVideo);
        }
        boost::filesystem::path orgVideoPath(replayVideoFileName);
        saveVideo /= orgVideoPath.stem().string() + "_" + boost::lexical_cast<string>(startIndex) + "_" + boost::lexical_cast<string>(endIndex) + ".avi";
        string blankVideo = saveVideo.string();
        try
        {
            capture.open(replayVideoFileName);
            
            for (double i = startIndex; i<=endIndex; i++)
            {
                capture.set(CV_CAP_PROP_POS_FRAMES, i);
                capture.read(curFrame);

                if(!vWriter.isOpened()){
                    Size vSize(curFrame.cols, curFrame.rows);                    
                    if (!vWriter.open(blankVideo, CV_FOURCC('M', 'J', 'P', 'G'), frameRate, vSize))
                    {
                        DAVINCI_LOG_ERROR << "      ERROR: Cannot create the video file for save no response result!";                        
                        return ;
                    }
                }
                
                if (!curFrame.empty())
                {
                    vWriter.write(curFrame);
                }
            }
            vWriter.release();
            capture.release();
        }
        catch(...)
        {
            DAVINCI_LOG_ERROR << "Write video frames exception!";
        }
    }    
}
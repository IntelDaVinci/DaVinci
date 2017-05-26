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

#include "VideoChecker.hpp"
#include "ImageChecker.hpp"
#include "FileCommon.hpp"
#include <codecvt>
#include <boost/date_time/posix_time/posix_time.hpp>  

namespace DaVinci
{
    VideoChecker::VideoChecker(const boost::weak_ptr<ScriptReplayer> obj) : Checker(),
                            videoMismatches(0),
                            videoFlicks(0),
                            previewMismatches(0),
                            objectMismatches(0),
                            objectCounts(0),
                            COUNT_MATCH_STR("COUNT")
    {
        SetCheckerName("VideoChecker");

        replayVideoFilename = "";
        replayQtsFilename = "";
        videoFilename = "";
        qtsFilename = "";
        replayWaveFileName = "";
        qAudioFilename = "";
        currentQSDir = "";

        recordTimeStampList = std::vector<double>();
        recordStartTimestamp = 0;
        recordFrameCount = 0;
        recordQsEventNum = 0;

        replayTimeStampList = std::vector<double>();
        replayStartTimestamp = 0;
        replayFrameCount = 0;
        replayQsEventNum = 0;

        altRatio = 1.0;

        boost::shared_ptr<ScriptReplayer> currentScriptTmp = obj.lock();

        if ((currentScriptTmp == nullptr) || (currentScriptTmp->GetQS() == nullptr))
        {
             DAVINCI_LOG_ERROR << "Failed to Init VideoChecker()!";
             throw 1;
        }

        std::string recordingVideoType = currentScriptTmp->GetQS()->GetConfiguration(QScript::ConfigReplayVideoType);
        if (recordingVideoType.length() > 0)
        {
            boost::to_lower(recordingVideoType);
            replayVideoFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + "_replay" + recordingVideoType);
        }
        else
        {
            replayVideoFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + "_replay.avi");
        }

        replayQtsFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + "_replay.qts");
        replayWaveFileName = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + "_replay.wav");

        std::string recordedVideoType = currentScriptTmp->GetQS()->GetConfiguration(QScript::ConfigRecordedVideoType);
        if (recordedVideoType.length() > 0)
        {
            boost::to_lower(recordedVideoType);
            videoFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + recordedVideoType);
        }
        else
        {
            videoFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + ".avi");
        }

        qtsFilename = currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + ".qts");
        qAudioFilename =  currentScriptTmp->GetQS()->GetResourceFullPath(currentScriptTmp->GetQS()->GetScriptName() + ".wav");

        currentQSDir =  boost::filesystem::path(videoFilename).parent_path().string();
        outputDir = TestReport::currentQsLogPath;

        currentQScript = currentScriptTmp;
    }

    VideoChecker::~VideoChecker()
    {
    }

    /// <summary>
    /// Get mismatch count.
    /// </summary>
    /// <returns>mismatch count.</returns>
    int VideoChecker::GetVideoMismatches()
    {
        return videoMismatches;
    }

    /// <summary>
    /// Get mismatch count.
    /// </summary>
    /// <returns>mismatch count.</returns>
    /*
    int VideoChecker::GetObjectMismatches()
    {
        return objectMismatches;
    }
    */

    /// <summary>
    /// Get flicking count.
    /// </summary>
    /// <returns>flicking count.</returns>
    int VideoChecker::GetFlicks()
    {
        return videoFlicks;
    }

    /// <summary>
    /// Get preview mismatch count.
    /// </summary>
    /// <returns>preview mismatch count.</returns>
    int VideoChecker::GetPreviewMismatches()
    {
        return previewMismatches;
    }

    /// <summary>
    /// Calculate the average num
    /// </summary>
    /// <param name="list"></param>
    /// <param name="start"></param>
    /// <param name="end"></param>
    /// <returns>average value</returns>
    int VideoChecker::CalcAverage(std::vector<int> &list, int start, int end)
    {
        int sum = 0;

        for (int i = start; i < end; i++)
        {
            if (list[i] > 0)
                sum += list[i];
        }

        return static_cast<int>(sum / (end - start));
    }

    /// <summary>
    /// Evaluate whether the flicking frame has enough intervals
    /// </summary>
    /// <param name="list"></param>
    /// <param name="i"></param>
    /// <param name="interval"></param> 
    /// <returns>true for enough</returns>
    bool VideoChecker::IsEnoughInterval(std::vector<double> &list, int i, int interval)
    {
        bool flag = false;

        if (((list[i] + interval) < list[i + 1]) && ((list[i + 1] + interval) < (list[i + 2])))
            flag = true;

        return flag;
    }

    /// <summary>
    /// Detect video flicking between two frames
    /// </summary>
    /// <param name="capture"></param>
    /// <param name="start"></param>
    /// <param name="end"></param>
    /// <param name="threshold"></param>
    /// <returns></returns>
    bool VideoChecker::DetectVideoFlicking(const std::string &videoFilename, double start, double end, int threshold)
    {
        bool isFlicking = false;
        cv::Mat currentFrame;
        cv::Mat previousFrame;
        cv::Mat diffFrame;
        vector<Mat> diffChannels;

        int width = 0, height = 0;
        std::vector<double> startList = std::vector<double>(), endList = std::vector<double>();
        int average = 0;
        std::vector<int> noneZeroList = std::vector<int>();
        int zeroCount = 0;
        VideoCaptureProxy capture;
        double startCheckPoint, endCheckPoint;

        noneZeroList.clear();
        startCheckPoint = start;
        endCheckPoint = end;

        try
        {
            capture.open(videoFilename);
            if (!capture.isOpened())
            {
                DAVINCI_LOG_DEBUG << "The source capture didn't opened." ;
                return isFlicking;
            }
            double maxCaptureFrame = capture.get(CV_CAP_PROP_FRAME_COUNT);

            if (endCheckPoint > maxCaptureFrame)
                endCheckPoint = maxCaptureFrame;

            for (double i = startCheckPoint; i < endCheckPoint; i++)
            {
                capture.set(CV_CAP_PROP_POS_FRAMES, i);
                capture.read(currentFrame);

                if (currentFrame.empty())
                    continue;

                width = currentFrame.size().width;
                height = currentFrame.size().height;
                if (!previousFrame.empty())
                {
                    cv::absdiff(currentFrame, previousFrame, diffFrame);
                    split(diffFrame, diffChannels);
                    zeroCount = countNonZero(diffChannels[0]);
                    noneZeroList.push_back(zeroCount);
                }
                previousFrame = currentFrame.clone();
            }

            int averagetNoneZero = static_cast<int>(width * height * 0.9);
			double flickStart = 0, flickEnd = 0;
			int endPoint = 0;
			int nonZeroSize = (int)(noneZeroList.size());
			if (nonZeroSize > threshold)
				endPoint = nonZeroSize - threshold;

            for (int i = 0; i < endPoint; i++)
            {
                average = CalcAverage(noneZeroList, i, i + threshold);
                if (average > averagetNoneZero)
                {
                    startList.push_back(i);
                    endList.push_back(i + threshold);
                }
            }

            bool isIntervalEnough = false;

            if (startList.size() > 0)
            {
                if (startList.size() == 1)
                {
                    flickStart = startList[0] + startCheckPoint;
                    flickEnd = flickStart + threshold;
                    isFlicking = true;
                    videoFlicks++;
                }
                else
                {
					endPoint = 0;
					if ((int)(startList.size()) > 2)
						endPoint = (int)(startList.size()) - 2;

                    for (int i = 0; i < endPoint; i++)
                    {
                        if (IsEnoughInterval(startList, i, threshold))
                        {
                            isIntervalEnough = true;
                            flickStart = i + 1 + startCheckPoint;
                            flickEnd = flickStart + threshold;
                            isFlicking = true;
                            videoFlicks++;
                        }
                    }
                }
            }

            if (!isIntervalEnough && (int)(startList.size()) > 0 && !isFlicking)
            {
                if (startList[(int)(startList.size()) - 1] - startList[0] < end * 0.5)
                {
                    flickStart = startList[0];
                    flickEnd = startList[(int)(startList.size()) - 1];
                    isFlicking = true;
                    videoFlicks++;
                }
            }

/* Disable write down flicking video part feature due to flicking detect not mature enough.
            if (isFlicking)
            {
                DAVINCI_LOG_INFO << "Video has flicking from frame " << flickStart << std::string(" to ") << flickEnd << std::endl;
                int frameRate = 30;
                VideoWriter videoWriter;
                Size videoFrameSize;
                string flickingFilename = std::string("tmp_flicking_") + boost::lexical_cast<std::string>(flickStart) + std::string("_") + boost::lexical_cast<std::string>(flickEnd) + std::string(".avi");

                videoFrameSize.width = width;
                videoFrameSize.height = height;

                try
                {
                    videoWriter.open(flickingFilename, CV_FOURCC('M', 'J', 'P', 'G'), frameRate, videoFrameSize);

                    assert(videoWriter.isOpened());

                    for (double i = flickStart; i <= flickEnd; i++)
                    {
                        capture.set(CV_CAP_PROP_POS_FRAMES, i);
                        capture.read(currentFrame);

                        if (!currentFrame.empty())
                            videoWriter.write(currentFrame);
                    }

                    videoWriter.release();
                }
                catch (...)
                {
                    DAVINCI_LOG_ERROR << "WriteVideoFlicking file exception!";
                }

                AddToFailedImageList(flickingFilename);
            }
*/
            capture.release();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "DetectVideoFlicking exception!";
        }
        return isFlicking;
    }

    bool VideoChecker::IsVideoFlicking(const std::string &videoFilename, int threshold)
    {
        bool isFlicking = false;
        bool matchOn = false;
        VideoCaptureProxy currentCapture;
        int qsIP = 0;
        std::vector<double> startTimestampList = std::vector<double>();
        std::vector<double> endTimestampList = std::vector<double>();
        double start = 1, end = 1;

        boost::shared_ptr<QSEventAndAction> qsEvent = nullptr;
        boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();

        if(!boost::filesystem::exists(videoFilename))
        {
            DAVINCI_LOG_DEBUG << "video does not exists: " + videoFilename;
            return isFlicking;
        }

        try
        {
            replayTimeStampList = currentQScriptTmp->GetTimeStampList();
            recordQsEventNum = currentQScriptTmp->GetQS()->NumEventAndActions();

            if ((replayTimeStampList.empty()) || (recordQsEventNum == 0))
            {
                currentCapture.open(videoFilename);
                if (!currentCapture.isOpened())
                {
                    DAVINCI_LOG_DEBUG << "The source capture didn't opened." ;
                    return isFlicking;
                }

                end = currentCapture.get(CV_CAP_PROP_FRAME_COUNT);
                currentCapture.release();

                isFlicking = DetectVideoFlicking(videoFilename, start, end, threshold);

                return isFlicking;
            }

            while (qsIP++ <= recordQsEventNum)
            {
                qsEvent = currentQScriptTmp->GetQS()->EventAndAction(qsIP);

                if (qsEvent != nullptr)
                {
                    if (qsEvent->Opcode() == QSEventAndAction::OPCODE_FLICK_ON)
                    {
                        matchOn = true;
                        startTimestampList.push_back(qsEvent->TimeStampReplay());
                        continue;
                    }
                    else if (qsEvent->Opcode() == QSEventAndAction::OPCODE_FLICK_OFF)
                    {
                        matchOn = false;
                        endTimestampList.push_back(qsEvent->TimeStampReplay());
                        continue;
                    }
                }
            }

            // it is possible to miss the last OPCODE_FLICK_OFF
            if ((qsEvent != nullptr) && (startTimestampList.size() == endTimestampList.size() + 1))
                endTimestampList.push_back(qsEvent->TimeStampReplay());

            if (startTimestampList.empty() && (endTimestampList.size() == 1))
                return false;

            if (startTimestampList.size() != endTimestampList.size())
                DAVINCI_LOG_DEBUG <<"Warning: flick match opcodes on and off does not match.";

            for (int i = (int)(endTimestampList.size()) - 1; i >= 0; i--)
            {
                start = 0;

                while (start < (replayTimeStampList.size() - 1))
                {
                    if(startTimestampList[i] > replayTimeStampList[static_cast<unsigned int>(start)])
                        start++;
                    else
                        break;
                }

                end = start;
                while (end < (replayTimeStampList.size() - 1))
                {
                    if(endTimestampList[i] > replayTimeStampList[static_cast<unsigned int>(end)])
                        end++;
                    else
                        break;
                }

                if (DetectVideoFlicking(videoFilename, start, end, threshold))
                {
                    isFlicking = true;
                    videoFlicks++;
                }
            }
        }
        catch (...)
        {
            DAVINCI_LOG_DEBUG << "IsVideoFlicking exception. ";
        }

        return isFlicking;
    }   

    bool VideoChecker::IsCurrentScriptNeedVideoCheck()
    {
        bool needVideoCheck = false;
        int qsIp = 0;

        boost::shared_ptr<QSEventAndAction> qsEvent = nullptr;
        boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();
        recordQsEventNum = currentQScriptTmp->GetQS()->NumEventAndActions();

        while (qsIp++ <= recordQsEventNum)
        {
            qsEvent = currentQScriptTmp->GetQS()->EventAndAction(qsIp);

            if (qsEvent == nullptr)
                continue;

            // if the events and actions are irrelevant to application behavior, then don't compare the video frames
            if (qsEvent->Opcode() == QSEventAndAction::OPCODE_MATCH_ON)
            {
                needVideoCheck = true;
                break;
            }
        }

        return needVideoCheck;
    }

    bool VideoChecker::InitVideoMatchResource()
    {
        bool initResult = false;

        replayFrameCount = 0;
        recordFrameCount = 0;
        recordQsEventNum = 0;
        altRatio = 1.0;
        replayStartTimestamp = 0;
        recordStartTimestamp = 0;

        try
        {
            boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();

            recordQsEventNum = currentQScriptTmp->GetQS()->NumEventAndActions();
            recordTimeStampList = currentQScriptTmp->GetQS()->TimeStamp();
            replayTimeStampList = currentQScriptTmp->GetTimeStampList();

            if ((recordTimeStampList.empty()) || (replayTimeStampList.empty()))
            {
                DAVINCI_LOG_ERROR <<"The timestamp information from qts file is incorrect!";
                return initResult;
            }

            if ((!boost::filesystem::exists(videoFilename)) || (!boost::filesystem::exists(replayVideoFilename)))
            {
                DAVINCI_LOG_ERROR <<"      Comparing Q script failure due to missing resources!";
                return initResult;
            }

            DAVINCI_LOG_INFO <<"      Record Q script video: " + videoFilename;
            DAVINCI_LOG_INFO <<"      Replay Q script video: " + replayVideoFilename;

            replayCapture.open(replayVideoFilename);
            recordCapture.open(videoFilename);

            if ((!replayCapture.isOpened()) || (!recordCapture.isOpened()))
            {
                ReleaseVideoMatchResource();

                DAVINCI_LOG_ERROR <<"Cannot open record/replay video";
                return initResult;
            }

            double replayFrameWidth = replayCapture.get(CV_CAP_PROP_FRAME_WIDTH);
            double replayFrameHeight = replayCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
            double recordFrameWidth = recordCapture.get(CV_CAP_PROP_FRAME_WIDTH);
            double recordFrameHeight = recordCapture.get(CV_CAP_PROP_FRAME_HEIGHT);

            if (replayFrameWidth != 0 && replayFrameHeight != 0 && recordFrameWidth != 0 && recordFrameHeight != 0)
                altRatio = (static_cast<double>(replayFrameWidth) / replayFrameHeight) / (static_cast<double>(recordFrameWidth) / replayFrameHeight);

            recordCapture.set(CV_CAP_PROP_POS_FRAMES,0);
            recordCapture.read(currentRecordFrame);

            replayCapture.set(CV_CAP_PROP_POS_FRAMES,0);
            replayCapture.read(currentReplayFrame);

            initResult = true;
        }
        catch (...)
        {
            initResult = false;
            ReleaseVideoMatchResource();
            DAVINCI_LOG_INFO << " Exception found when InitVideoMatchResource!";
        }

        return initResult;
    }

    bool VideoChecker::ReleaseVideoMatchResource()
    {
        recordCapture.release();
        replayCapture.release();

        return true;
    }

    bool VideoChecker::IsVideoMatched()
    {
        bool matchResult = true;
        bool matchOn = false;
        int qsIp = 0;

/*         
        std::shared_ptr<ButtonDetection> buttonDetection;
        string trainFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/appbutton_iter_4000"); 
        string netFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/appbutton.prototxt"); 
        string meanFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/mean.binaryproto"); 
        buttonDetection = std::shared_ptr<ButtonDetection>(new ButtonDetection(netFile,trainFile, meanFile));
*/
        try
        {
            if (!IsCurrentScriptNeedVideoCheck())
                return matchResult;
            
            if (!InitVideoMatchResource())
            {
                matchResult = false;
                videoMismatches++;
                DAVINCI_LOG_ERROR <<"Failed to init resources for video check!";
                return matchResult;
            }

            double tenPercentsIp = (recordQsEventNum / 10.0);
            VideoMatchType matchType = VideoMatchType::IMAGE;
            int showArray[10] = {0};
            string countPercentageStr;
            double countPercentage = 30;
            string saveImageName;

            boost::shared_ptr<QSEventAndAction> qsEvent = nullptr;
            boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();

            matchResult = true;
            while (qsIp++ <= recordQsEventNum)
            {
                qsEvent = currentQScriptTmp->GetQS()->EventAndAction(qsIp);

                if (qsEvent == nullptr)
                    continue;

                if (qsIp == recordQsEventNum)
                    DAVINCI_LOG_INFO << "      Comparing Q script video: 100% done at Line#" << qsEvent->LineNumber();
                else if (tenPercentsIp >= 1.0)
                {
                    double v1 = qsIp / tenPercentsIp;
                    int v2 = static_cast<int>(qsIp / tenPercentsIp);

                    if (v1 + 1 > v2 && v2 != 9)
                    {
                        if (showArray[v2] == 0)
                        {
                            DAVINCI_LOG_INFO << "      Comparing Q script video: " << ((v2 + 1) * 10) << "% done at Line#" << qsEvent->LineNumber();
                            showArray[v2] = 1;
                        }
                    }
                }

                // if the events and actions are irrelevant to application behavior, then don't compare the video frames
                if (qsEvent->Opcode() == QSEventAndAction::OPCODE_MATCH_ON)
                {
                    countPercentageStr = qsEvent->StrOperand(0);
                    if (countPercentageStr == "")
                        matchType = VideoMatchType::IMAGE;
                    else
                    {
                        if (countPercentageStr != COUNT_MATCH_STR)
                        {
                            countPercentageStr = countPercentageStr.substr(countPercentageStr.find("|") + 1);
                            countPercentageStr = countPercentageStr.substr(0, countPercentageStr.length() - 1);
                        }

                        TryParse(countPercentageStr, countPercentage);
                        matchType = VideoMatchType::COUNT;
                    }
                    matchOn = true;
                    continue;
                }
                else if (qsEvent->Opcode() == QSEventAndAction::OPCODE_MATCH_OFF)
                {
                    matchOn = false;
                    continue;
                }
                else if ((qsEvent->Opcode() != QSEventAndAction::OPCODE_TOUCHDOWN) && (qsEvent->Opcode() != QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL))
                    continue;

                // matchOn/match_off command can turn on/off matching here
                if (!matchOn)
                    continue;

                // source video is the temp video just caputured during playing
                // so the timestamp is stored in timestamp_playing in QSEvent.
                // roll forward playing video until the snapshot right before the current qsEvent
                if (replayFrameCount == 0)
                {
                    replayCapture.set(CV_CAP_PROP_POS_FRAMES, 0);
                    replayCapture.read(currentReplayFrame);
                    replayFrameCount++;
                }

                while (replayFrameCount < (replayTimeStampList.size() - 1))
                {
                    if ((qsEvent->TimeStampReplay() - replayStartTimestamp) > replayTimeStampList[static_cast<unsigned int>(replayFrameCount)]) 
                        replayFrameCount++;
                    else
                        break;
                }

                if (replayFrameCount < replayTimeStampList.size() )
                {
                    replayCapture.set(CV_CAP_PROP_POS_FRAMES,replayFrameCount);
                    replayCapture.read(currentReplayFrame);
                    //saveImageName = GetOutputDir() + std::string("/") + std::string("CurrentFrame_1_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + "_" + boost::lexical_cast<std::string>(replayFrameCount)+ std::string(".png");
                    //imwrite(saveImageName, currentReplayFrame);
                }
                else
                    DAVINCI_LOG_ERROR << "      Comparing Q script video: cannot get frames from replay videos!!!";

                // target video is the video recording during training
                // so the timestamp is stored in timestamp in QSEvent.
                // roll forward training video until the snapshot right before the current qsEvent
                if (recordFrameCount == 0)
                {
                    recordCapture.set(CV_CAP_PROP_POS_FRAMES,0);
                    recordCapture.read(currentRecordFrame);
                    recordFrameCount++;
                }

                while (recordFrameCount < (recordTimeStampList.size() - 1))
                {
                    if ((qsEvent->TimeStamp() - recordStartTimestamp) > recordTimeStampList[static_cast<unsigned int>(recordFrameCount)])
                        recordFrameCount++;
                    else
                        break;
                }

                if (recordFrameCount < recordTimeStampList.size())
                {
                    recordCapture.set(CV_CAP_PROP_POS_FRAMES, recordFrameCount);
                    recordCapture.read(currentRecordFrame);
                    //saveImageName = GetOutputDir() + std::string("/") + std::string("RecordFrame_1_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + "_" + boost::lexical_cast<std::string>(RecordFrameCount)+ std::string(".png");
                    //imwrite(saveImageName, currentRecordFrame);
                }
                else
                    DAVINCI_LOG_ERROR << "      Comparing Q script video: cannot get frames from record videos!!!";

                // if cannot get a frame, then stop matching 
                if ((currentReplayFrame.empty()) || (currentRecordFrame.empty()))
                {
                    matchResult = false;
                    DAVINCI_LOG_INFO << "      Comparing Q script video: cannot get frames from source or target videos!!!";
                    break;
                }

                /* disable this feature due to it not accuracy. 
                if (matchType == VideoMatchType::COUNT)
                {
                    vector<Rect> sourceRects;
                    vector<Rect> targetRects;
                    int sourceCount = 0;
                    int targetCount = 0;

                    objectCounts++;
                    buttonDetection->Detect(currentReplayFrame, sourceRects, false, -1,10,10, 100000, 5,1000, 50000);
                    buttonDetection->Detect(currentRecordFrame, targetRects, false, -1,10,10, 100000, 5,1000, 50000);

                    sourceCount = sourceRects.size();
                    targetCount = targetRects.size();

                    DAVINCI_LOG_INFO << "Source clickable Object count:" <<sourceCount << " ,Target Object count:" <<targetCount ;

                    if (sourceCount == 0)
                    {
                        if (targetCount == 0)
                        {
                            if (!IsImageMatched(currentReplayFrame, currentRecordFrame, 300, altRatio, qsEvent->LineNumber()))
                            {
                                videoMismatches++;
                                matchResult = false;
                            }
                        }
                        else
                        {
                            videoMismatches++;
                            saveImageName = GetOutputDir() + std::string("/") + std::string("CurrentFrame_Count_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + std::string(".png");
                            imwrite(saveImageName, currentReplayFrame);

                            saveImageName = GetOutputDir() + std::string("/") + std::string("RecordFrame_Count_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + std::string(".png");
                            imwrite(saveImageName, currentRecordFrame);
                            matchResult = false;
                        }
                    }
                    else
                    {
                        double deltaPercentage = 0;
                        if (targetCount != 0)
                            deltaPercentage = (abs(sourceCount - targetCount) * 100 / targetCount);

                        if (deltaPercentage > countPercentage)
                        {
                            videoMismatches++;
                            saveImageName = GetOutputDir() + std::string("/") + std::string("CurrentFrame_Count_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + std::string(".png");
                            imwrite(saveImageName, currentReplayFrame);

                            saveImageName = GetOutputDir() + std::string("/") + std::string("RecordFrame_Count_") + boost::lexical_cast<std::string>(qsEvent->LineNumber()) + std::string(".png");
                            imwrite(saveImageName, currentRecordFrame);
                            matchResult = false;
                        }
                        else
                        {
                            if (!IsImageMatched(currentReplayFrame, currentRecordFrame, 300, altRatio, qsEvent->LineNumber()))
                            {
                                videoMismatches++;
                                matchResult = false;
                            }
                        }
                    }
                }
                else if (matchType == VideoMatchType::IMAGE)
                {
                    if (!IsImageMatched(currentReplayFrame, currentRecordFrame, 300, altRatio, qsEvent->LineNumber()))
                    {
                        videoMismatches++;
                        matchResult = false;
                    }
                }
               */
                if (!IsImageMatched(currentReplayFrame, currentRecordFrame, 300, altRatio, qsEvent->LineNumber()))
                {
                    videoMismatches++;
                    matchResult = false;
                }
            }
        }
        catch (...)
        {
            videoMismatches++;
            matchResult = false;
            DAVINCI_LOG_INFO << " Exception found when IsVideoMatched!";
        }

        ReleaseVideoMatchResource();

        return matchResult;
    }

    bool VideoChecker::IsImageMatched(const cv::Mat &currentFrame, const cv::Mat &referenceFrame, 
        int size, double altRatio, int lineNumber)
    {
        string saveImageName;

        if (!ImageChecker::IsImageMatched(currentFrame, referenceFrame, 300, altRatio))
        {
            DAVINCI_LOG_INFO << "      Comparing Q script video: unmatched at Line #" + boost::lexical_cast<std::string>(lineNumber);

            saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())) + std::string("_CurrentFrame_") + boost::lexical_cast<std::string>(lineNumber) + std::string(".png");
            imwrite(saveImageName, currentFrame);
            AddToFailedImageList(saveImageName);

            saveImageName = GetOutputDir() + std::string("/") + boost::lexical_cast<std::string>(boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time())) + std::string("_ReferenceFrame_") + boost::lexical_cast<std::string>(lineNumber) + std::string(".png");
            imwrite(saveImageName, referenceFrame);
            AddToReferenceImageList(saveImageName);
            return false;
        }
        else
            DAVINCI_LOG_INFO <<"      Comparing Q script video: matched at Line #" + boost::lexical_cast<std::string>(lineNumber);

        return true;
    }

    bool VideoChecker::IsObjectMatched()
    {
        vector<boost::filesystem::path> unmatchedFiles;
        bool flag = true;
        wstring curFilename;
        boost::shared_ptr<ScriptReplayer> currentQScriptTmp = currentQScript.lock();

        getFiles(currentQSDir, unmatchedFiles);

        string preFileName = currentQScriptTmp->GetQS()->GetScriptName() + "_UnmatchedFrame_";
        std::wstring wpreFileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(preFileName);
        string suffixFileName = ".png";
        std::wstring wsuffixFileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(suffixFileName);

        for (auto curFile : unmatchedFiles)
        {
            curFilename = curFile.filename().wstring();

            if ((!curFilename.empty()) && 
                (curFilename.find(wpreFileName) != string::npos ) && 
                (curFilename.find(wsuffixFileName) != string::npos))
            {
                objectMismatches++;
                flag = false;
            }
        }
        return flag;
    }

    void VideoChecker::DoOfflineChecking()
    {
        SetResult(CheckerResult::Pass);

        if (IsVideoMatched())
            DAVINCI_LOG_INFO << "      Comparing Q script video: matched!!!";
        else
        {
            SetResult(CheckerResult::Fail);
            DAVINCI_LOG_INFO << "      Comparing Q script video: matching failed!!!";
        }

        DAVINCI_LOG_INFO << "      Checking flicks in replayed video......";

/* Comment it due to this feature not mature enough. 20151125
        if (IsVideoFlicking(replayVideoFilename, 30))
        {
            SetResult(CheckerResult::Fail);
            DAVINCI_LOG_INFO << "      Flicking in Q script video!!!";
        }
        else
            DAVINCI_LOG_INFO << "      Flicking not in Q script video!!!";
*/
        DAVINCI_LOG_INFO << "      Checking object in replayed video......";

        if (IsObjectMatched())
            DAVINCI_LOG_INFO << "      Object matches!!!";
        else
            DAVINCI_LOG_INFO << "      Object mismatches!!!";
        //{
        //    SetResult(CheckerResult::Fail);
        //    DAVINCI_LOG_INFO << "      Object mismatches!!!";
        //}
    }
}
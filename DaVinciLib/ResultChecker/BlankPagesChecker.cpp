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

#include "BlankPagesChecker.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    BlankPagesChecker::BlankPagesChecker():barHeight(0), flagHasBlankFrames(false),frameCount(0),frameVectorSize(0),frmsPerSubVideo(0),
        maxThreads(0),result(false),startCheckIndex(0),logFolder(""),intervalTime(5 * 1000), qtsFile("")
    {
    }

    BlankPagesChecker::~BlankPagesChecker()
    {
    }

    string BlankPagesChecker::LocateTimeStampFile(const string &vidFile)
    {
        using namespace boost::filesystem;
        const string fileExt = ".qts";
        path videoFilePath(vidFile);
        path qtsFilePath = videoFilePath.parent_path()/path(videoFilePath.stem().string() + fileExt);
        if (is_regular_file(qtsFilePath))
        {
            return qtsFilePath.string();
        }
        else
        {
            return "";
        }
    }

    double BlankPagesChecker::FrameIndex2TimeStamp(int index)
    {
        return index > (int)frameCount? timeStamps.at((int)frameCount-1):timeStamps.at(index);
    }

    int BlankPagesChecker::TimeStamp2FrameIndex(double timeStamp)
    {
        int index = 0;
        for (vector<double>::iterator it = timeStamps.begin(); it != timeStamps.end(); ++it)
        {
            if (*it < timeStamp)
                index++;
            else
                break;
        }
        return index-1 > 0 ? index-1 : 0;
    }

    vector<double> BlankPagesChecker::GetTimeStamp()
    {
        vector<QScript::TraceInfo> traces;

        if (QScript::ParseTimeStampFile(qtsFile, timeStamps, traces) != DaVinciStatusSuccess)
        {
            DAVINCI_LOG_ERROR << "Cannot parse timestamp file correctly!";
        }
        return timeStamps;
    }

    bool BlankPagesChecker::HasVideoBlankFrames(const string &videoFile)
    {
        frameCount = 0;
        flagHasBlankFrames = false;
        cv::Mat analysisFrame;
        maxThreads = 4;

        boost::thread_group analyzeGroup;
        boost::asio::io_service ioService;
        if (!boost::filesystem::is_regular_file(videoFile))
        {
            DAVINCI_LOG_ERROR << "No video file exist!";
            return false;
        }
        qtsFile = LocateTimeStampFile(videoFile);
        if ((qtsFile != "") && (boost::filesystem::exists(qtsFile)))
        {
            timeStamps = GetTimeStamp();
            if (timeStamps.empty())
            {
                DAVINCI_LOG_ERROR << "Timestamp file is empty!";
                return false;
            }
        }
        else
        {
            DAVINCI_LOG_ERROR << "Cannot find replay qts file!";
            return false;
        }
        replayCapture.open(videoFile);
        if (!replayCapture.isOpened())
        {
            DAVINCI_LOG_ERROR <<"Cannot open record/replay video";
            replayCapture.release();
            return false;
        }
        frameCount = replayCapture.get(CV_CAP_PROP_FRAME_COUNT);
        if(frameCount > 0)
        {
            replayCapture.set(CV_CAP_PROP_POS_FRAMES, 0);
            replayCapture.read(analysisFrame);
            // Resize the analysisFrame
            cv::Mat resizeFrame = analysisFrame.clone();
            resizeFrame.cols /= 8;
            MappingBarsRect(resizeFrame, naviBarRects, statusBarRects);
        }
        else
        {
            DAVINCI_LOG_WARNING << "Empty video: " << videoFile;
            return false;
        }

        double startTick = GetCurrentMillisecond();
        double endTick;

        // If the video is short, then no need to create 4 threads to check the video.
        if (frameCount < 200)
        {
            maxThreads = 1;
        }

        frmsPerSubVideo = (int)frameCount / maxThreads + 1;
        startCheckIndex = 0;

        for (int threadIndex = 0; threadIndex < maxThreads; threadIndex++)
        {
            analyzeGroup.create_thread(boost::bind(&BlankPagesChecker::CheckFrmsInSubVideos, this, videoFile, startCheckIndex));
            {
                boost::mutex::scoped_lock lock(lockUpdateStartIndex);
                startCheckIndex += frmsPerSubVideo;
            }
        }
        analyzeGroup.join_all();
        // Combine the neighbor pieces of video together.
        // e.g.: [5,10,15,20] will be saved 3 pieces of blank videos: [5,10],[10,15] and [15,20] at the beginning,
        // and now it will just be saved as 1 single blank video [5, 20], this also can improve performance since reduce IO actions.
        int startIndex = 0;
        int endIndex = 0;
        Size frmSize(analysisFrame.cols, analysisFrame.rows);
        saveVideoFolder = "Video_BlankPages";
        sort(saveFrmIndexVec.begin(), saveFrmIndexVec.end());
        while(!saveFrmIndexVec.empty())
        {
            for (int i=0; i < (int)(saveFrmIndexVec.size()); i++)
            {
                if ((i+1 >= (int)saveFrmIndexVec.size())||(saveFrmIndexVec.at(i+1) - saveFrmIndexVec.at(i) > frameVectorSize))
                {
                    endIndex = i;
                    break;
                }
            }
            SaveVideoClips(videoFile, saveFrmIndexVec.at(startIndex), saveFrmIndexVec.at(endIndex), frmSize);
            saveFrmIndexVec.erase(saveFrmIndexVec.begin(), saveFrmIndexVec.begin() + endIndex+1);
        }

        DAVINCI_LOG_INFO << "Total frames: " << frameCount << endl;
        endTick = GetCurrentMillisecond();
        DAVINCI_LOG_INFO << "Analyse time consume: " << (endTick-startTick)/1000 << " s" << endl;

        replayCapture.release();
        if (flagHasBlankFrames)
        {
            string videoPath = logFolder + "/" + saveVideoFolder;
            DAVINCI_LOG_INFO << "      Checking blank frames: Might have blank frames in Replay Video!!! Please double check video pieces at folder: ";
            DAVINCI_LOG_INFO << "file://" + videoPath;
        }
        else
        {
            DAVINCI_LOG_INFO << "      Checking blank frames: No blank frames!!!";
        }
        return flagHasBlankFrames;
    }

    void BlankPagesChecker::CalculateBarsRect(Size devSize, Orientation ori, Rect &statusBarRect, Rect &naviBarRect)
    {
        if (ori == Orientation::Portrait)
        {
            statusBarRect = Rect(0, 0, devSize.width, barHeight);
            naviBarRect = Rect(0, devSize.height - barHeight, devSize.width, barHeight);
        }
        else
        {
            statusBarRect = Rect(0, 0, devSize.height, barHeight);
            naviBarRect = Rect(0, devSize.width - barHeight, devSize.height, barHeight);
        }
    }

    void BlankPagesChecker::MappingBarsRect(cv::Mat &frame, vector<Rect> &nBarRect, vector<Rect> &sBarRect)
    {
        // Mapping the bar height from device to frame in 4 orientations
        boost::shared_ptr<TargetDevice> dut = DeviceManager::Instance().GetCurrentTargetDevice();
        boost::shared_ptr<AndroidTargetDevice> androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        vector<Orientation> orientations;
        if (androidDevice != nullptr)
        {
            orientations.push_back(Orientation::Portrait);
            orientations.push_back(Orientation::Landscape);
            std::vector<std::string> windowLines;
            AndroidTargetDevice::WindowBarInfo windowBarInfo = androidDevice->GetWindowBarInfo(windowLines);
            int sBarHeight = windowBarInfo.statusbarRect.height;
            int nBarHeight = windowBarInfo.navigationbarRect.height;
            sBarHeight > nBarHeight ? (barHeight = sBarHeight):(barHeight = nBarHeight);
            int devWidth = dut->GetDeviceWidth();
            int devHeight = dut->GetDeviceHeight();
            Size deviceSize(devWidth, devHeight);
            vector<Orientation>::iterator oriIt;
            Rect sBar;
            Rect nBar;
            for (oriIt = orientations.begin(); oriIt != orientations.end(); ++oriIt)
            {
                CalculateBarsRect(deviceSize, *oriIt, sBar, nBar);
                // Transform bar rect from device to frame
                Point nbPoint(nBar.x, nBar.y);
                nBarRect.push_back(AndroidTargetDevice::GetWindowRectangle(
                    nbPoint, 
                    nBar.width, 
                    nBar.height, 
                    deviceSize, 
                    *oriIt,
                    frame.cols));
                Point sbPoint(sBar.x, sBar.y);
                sBarRect.push_back(AndroidTargetDevice::GetWindowRectangle(
                    sbPoint, 
                    sBar.width, 
                    sBar.height, 
                    deviceSize, 
                    *oriIt,
                    frame.cols));
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << "No target device connected!!! The blank checking result may not correct!!!";
            nBarRect.push_back(Rect(0,0,0,0));
            sBarRect.push_back(Rect(0,0,0,0));
        }
    }

    void BlankPagesChecker::SaveVideoClips(const string &videoFile, int startIndex, int endIndex, Size frmSize)
    {
        // Need to save 5 seconds before and after real blank frames into video.
        int updateStartIndex = startIndex - 50;
        int updateEndIndex = endIndex + 50;
        if (updateStartIndex < 0)
        {
            updateStartIndex = 0;
        }
        if (updateEndIndex >= frameCount)
        {
            updateEndIndex = (int)frameCount - 1;
        }
        VideoCaptureProxy capture;
        cv::Mat curFrame;
        DAVINCI_LOG_INFO << "Detect blank pages from frame: " << boost::lexical_cast<string>(updateStartIndex) << " to frame: " << boost::lexical_cast<string>(updateEndIndex) << endl;
        VideoWriterProxy vWriter;
        // If abnormal frames detected, save a piece of video to a folder named "Video_BlankPages"
        logFolder="";
        if(TestReport::currentQsLogPath == "")
        {
            boost::filesystem::path filePath(videoFile);
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
        boost::filesystem::path orgVideoPath(videoFile);
        saveVideo /= orgVideoPath.stem().string() + "_" + boost::lexical_cast<string>(updateStartIndex) + "_" + boost::lexical_cast<string>(updateEndIndex) + ".avi";
        string blankVideo = saveVideo.string();
        try
        {
            capture.open(videoFile);
            vWriter.open(blankVideo, CV_FOURCC('M','J','P','G'), 30, frmSize);
            for (double i = updateStartIndex; i<=updateEndIndex; i++)
            {
                capture.set(CV_CAP_PROP_POS_FRAMES, i);
                capture.read(curFrame);
                if((i >= startIndex) && (i <= endIndex))
                {
                    DrawRect(curFrame, Rect(0,0,curFrame.cols,curFrame.rows), Red, 10);
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

    void BlankPagesChecker::CheckFrmsInSubVideos(const string &videoFile, int startIndex)
    {
        for (int frameIndex=startIndex; frameIndex<startIndex+frmsPerSubVideo, frameIndex <= frameCount;)
        {
            cv::Mat analysisFrame;
            {
                {
                    boost::mutex::scoped_lock lock(lockReadFrm);
                    replayCapture.set(CV_CAP_PROP_POS_FRAMES, frameIndex);
                    replayCapture.read(analysisFrame);
                    analysisFrame = ResizeFrame(analysisFrame, 8);
                }
                result = ObjectUtil::Instance().IsPureSingleColor(analysisFrame, naviBarRects[0], statusBarRects[0]) ||
                    ObjectUtil::Instance().IsPureSingleColor(analysisFrame, naviBarRects[1], statusBarRects[1]);
            }
            // If one frame detected as abnormal page, then check whether the next frameVectorSize counts of frames 
            // are abnormal frames, too.
            int m = 0;
            if (result)
            {
#if _DEBUG
                cv::imwrite("BlankPages_DebugFolder/BlankPages" + boost::lexical_cast<string>(frameIndex) + ".png", analysisFrame);
#endif
                cv::Mat nextFrame;
                double time = FrameIndex2TimeStamp(frameIndex);
                frameVectorSize = TimeStamp2FrameIndex(time + intervalTime) - TimeStamp2FrameIndex(time);
                for (int j=frameIndex+1; j<=frameIndex+frameVectorSize; j++)
                {
                    boost::mutex::scoped_lock lock(lockReadFrm);
                    replayCapture.set(CV_CAP_PROP_POS_FRAMES, j);

                    replayCapture.read(nextFrame);
                    nextFrame = ResizeFrame(nextFrame, 8);
                    result = ObjectUtil::Instance().IsPureSingleColor(nextFrame, naviBarRects[0], statusBarRects[0]) ||
                    ObjectUtil::Instance().IsPureSingleColor(nextFrame, naviBarRects[1], statusBarRects[1]);
#if _DEBUG
                    cv::imwrite("BlankPages_DebugFolder/BlankPages" + boost::lexical_cast<string>(j) + ".png", nextFrame);
#endif
                    if (!result)
                    {
                        m = j;
                        break;
                    }
                }
                if (result)
                {
                    boost::lock_guard<boost::mutex> lock(lockUpdateResult);
                    // Save the start index of the blank page
                    saveFrmIndexVec.push_back(frameIndex);
                    flagHasBlankFrames = true;
                    frameIndex += frameVectorSize;
                    // Save the end index of the blank page
                    saveFrmIndexVec.push_back(frameIndex);
                    break;
                }
                frameIndex += m;
            }
            else
            {
                frameIndex++;
            }
        }
    }

    cv::Mat BlankPagesChecker::ResizeFrame(cv::Mat orgFrame, int ratio)
    {
        cv::Mat rtnMat;
        resize(orgFrame, rtnMat, Size(orgFrame.cols/ratio, orgFrame.rows/ratio));
        return rtnMat;
    }
}
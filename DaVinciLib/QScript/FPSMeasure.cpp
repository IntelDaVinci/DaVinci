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

#include "FPSMeasure.hpp"
#include "TestManager.hpp"
#include "DeviceManager.hpp"
#include "FeatureMatcher.hpp"
#include "HighResolutionTimer.hpp"

#include "boost/filesystem.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "boost/math/special_functions/fpclassify.hpp"
#include "boost/tokenizer.hpp"
#include "zlib.h"

#include <limits>
#include <exception>
#include <thread>
#include <sstream>
#include <unordered_set>

namespace DaVinci
{
    const char *videoFileName = "rawvideo";

    // parameters for GoodFeaturesToTrack
    const int maxFeaturesPerChannel = 500;
    const double qualityLevel = 0.01;
    const double minDistance = 5.0;
    const int blockSize = 3;
    const bool useHarris = false;
    const double k = 0.04;

    // parameters for calcOpticalFlowPyrLK
    const int maxLevel = 5;
    const cv::Size winSize(31, 31);
    const cv::TermCriteria criteria(TermCriteria::COUNT+TermCriteria::EPS, 20, 0.03);

    const int maxFrameQueueSize = 5;
    const int framesPerMatch = 30;


    static bool LauchShell(const string& cmd, const string& outputFile, bool appendFlag = false)
    {
        try
        {
            string dumpCmd = cmd;
            if (outputFile.size() > 0)
            {
                // fix Bug 2887 - Cannot get service FPS when the QS containing spaces is run under GUI mode
                string newPath = EscapePath(outputFile);
                if (appendFlag)
                    dumpCmd.append(" >> " + newPath);
                else
                    dumpCmd.append(" > " + newPath);
            }
            auto sts = RunShellCommand(dumpCmd, nullptr, "", 10*1000);
            if (!DaVinciSuccess(sts))
            {
                DAVINCI_LOG_ERROR << "Cannot run or timeout: " << dumpCmd;
                return false;
            }
            return true;
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non standard exception happened.";
        }
        return false;
    }

    static string GetClockString()
    {
        using namespace std::chrono;
        system_clock::duration unixNow = system_clock::now().time_since_epoch();
        auto us = duration_cast<microseconds>(unixNow);
        return boost::lexical_cast<string>(us.count());
    }

    static string GetTargetLayer(istream& istream, const string& appName)
    {
        string ret;
        string target = "Hardware Composer state";
        int numLayers = -1;
        while (istream.good())
        {
            string line;
            getline(istream, line);
            if (numLayers != -1)
            {
                size_t idx = line.find('|');
                if (idx != string::npos)
                {
                    list<string> fileds;
                    boost::char_separator<char> sep("| \r");
                    auto token = boost::tokenizer<boost::char_separator<char>>(line, sep);
                    for (auto& elm : token)
                    {
                        fileds.push_back(elm);
                    }
                    if (0 == fileds.back().find(appName))
                        ret = fileds.back();

                    if (fileds.back() == "SurfaceView")
                    {
                        ret = fileds.back();
                        return ret;
                    }

                    numLayers --;
                    if (0 == numLayers)
                        return ret;
                }         
            }
            else
            {
                size_t idx = line.find(target);

                if (idx != string::npos)
                {
                    if (target == "Hardware Composer state")
                    {
                        DAVINCI_LOG_DEBUG << "Found layer table";
                        target = "numHwLayers=";
                    }
                    else if (target == "numHwLayers=")
                    {
                        size_t idxComma = line.find(',');
                        if (idxComma != string::npos && idxComma > idx)
                        {
                            numLayers = boost::lexical_cast<int>(line.substr(idx + target.size(),
                                idxComma - idx - target.size()));
                            // incuding table header
                            numLayers ++;
                        }
                    }
                }
            }

        }

        return ret;
    }

    struct TripleTimestamp
    {
        // unit is nanoseconds
        uint64 appTms;
        uint64 vsyncTms;
        uint64 commitTms;
        TripleTimestamp(uint64 a, uint64 v, uint64 c)
            :appTms(a),vsyncTms(v),commitTms(c) {}
    };

    static void UnderstandLatency(const string& fileName, double& maxFPS,
        double& minFPS, double& avgFPS, double& jankRatio)
    {
        ifstream istream = ifstream(fileName, ios::binary);
        string firstLine;
        unordered_set<string> setTms;
        list<unique_ptr<TripleTimestamp>> lstTms;

        // vsync interval, unit microseconds
        uint64 refreshInterval = 0;
        uint64 jankLap = 0;
        long jankCount = 0;
        while (istream.good())
        {
            string line;
            getline(istream, line);
            if (firstLine.empty())
            {
                firstLine = line;
                size_t idx;
                while (idx = firstLine.find_last_of('\r'), idx != string::npos)
                {
                    firstLine.erase(idx);
                }
                try
                {
                    refreshInterval = boost::lexical_cast<uint64>(firstLine);
                }
                catch (const boost::bad_lexical_cast&)
                {
                    DAVINCI_LOG_DEBUG << "Meet unexpected line:" << firstLine;
                    firstLine = "";
                    continue;
                }
            }
            else
            {
                // skip next refresh interval line
                if (line == firstLine)
                    continue;

                vector<string> fileds;
                boost::char_separator<char> sep("\t\r");
                auto token = boost::tokenizer<boost::char_separator<char>>(line, sep);
                for (auto& elm : token)
                {
                    fileds.push_back(elm);
                }

                if (fileds.size() == 3)
                {
                    if (fileds[0] != "0" && setTms.find(fileds[0]) == setTms.end())
                    {
                        uint64 aTms = boost::lexical_cast<uint64>(fileds[0]);
                        uint64 vTms = boost::lexical_cast<uint64>(fileds[1]);
                        uint64 cTms = boost::lexical_cast<uint64>(fileds[2]);

                        if (vTms == INT64_MAX || cTms == INT64_MAX)
                        {
                            // suspicious record
                            DAVINCI_LOG_DEBUG << "Met pending frame, while app timestamp is " << fileds[0] + ".";
                        }
                        else
                        {
                            if (refreshInterval != 0)
                            {
                                uint64 curreantJankLap = (vTms - aTms) / refreshInterval / 1000;
                                if (jankLap != curreantJankLap)
                                {
                                    jankCount ++;
                                    jankLap = curreantJankLap;
                                }
                            }
                            setTms.insert(fileds[0]);
                            unique_ptr<TripleTimestamp> elm = unique_ptr<TripleTimestamp>(
                                new TripleTimestamp(aTms, vTms, boost::lexical_cast<uint64>(fileds[2])));
                            lstTms.push_back(std::move(elm));
                        }
                    }
                }
            }
        }
        istream.close();

        ofstream ostream = ofstream(fileName + ".out", ios::binary);
        for (auto &i : lstTms)
        {
            if (ostream.good())
            {
                stringstream ss;
                ss << i->appTms << "\t" << i->vsyncTms << "\t" << i->commitTms << "\n";
                ostream << ss.rdbuf();
            }
        }
        ostream.close();

        long long frameCount = 0;
        uint64 timeCurrent = 0, timeStart = 0, timeLast = 0;
        for (auto &i : lstTms)
        {
            timeCurrent = i->commitTms;
            if (0 == timeStart)
            {
                timeStart = timeCurrent;
            }
            else
            {
                double frameFps = 1.0e9 / (timeCurrent - timeLast);
                if (frameFps > maxFPS)
                    maxFPS = frameFps;
                if (boost::math::isnan(minFPS) ||
                    frameFps < minFPS)
                    minFPS = frameFps;
            }
            timeLast = timeCurrent;
            frameCount ++;
        }
        if (frameCount > 1)
        {
            avgFPS = frameCount * 1.0e9 / (timeLast - timeStart);
            jankRatio = 100.0 * jankCount / frameCount;
        }
        avgFPS = boost::math::isnan(avgFPS) ? 0.0 : avgFPS;
        minFPS = boost::math::isnan(minFPS) ? 0.0 : minFPS;       
        DAVINCI_LOG_INFO << "FPS:" + boost::str(boost::format("%.2f") % avgFPS)
            + ",max:" + boost::str(boost::format("%.2f") % maxFPS)
            + ",min:" + boost::str(boost::format("%.2f") % minFPS)
            <<  ",jank:" << boost::str(boost::format("%.2f%%") % jankRatio);
    }

    string FPSMeasure::GetDumpsysOutputFile()
    {
        return dumpFileName.empty() ? dumpFileName : dumpFileName + ".out";
    }

    void FPSMeasure::DumpTimerHandler(HighResolutionTimer& timer)
    {
        if (!IsFinished())
        {
            if (isStartedAtInit)
            {
                auto dutPtr = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
                if (dutPtr == nullptr)
                    return;
                auto oStr = boost::shared_ptr<ostringstream>(new ostringstream());
                assert(oStr != nullptr);
                auto ret = dutPtr->AdbCommand("shell dumpsys SurfaceFlinger", oStr, 1000);
                if (!DaVinciSuccess(ret))
                {
                    if (ret == errc::timed_out)
                        DAVINCI_LOG_ERROR << "shell dumpsys SurfaceFlinger times out";
                    else
                        DAVINCI_LOG_ERROR << "Cannot run shell dumpsys SurfaceFlinger";
                    return;
                }
                stringstream str(oStr->str());
                string layer = GetTargetLayer(str, dutAppName);
                if (layer == "SurfaceView")
                {
                    sfDumpCmd = dutPtr->GetAdbPath() + " -s " + dutPtr->GetDeviceName() + " shell dumpsys SurfaceFlinger --latency " + layer;
                    isStartedAtInit = false;
                }                
            }
            LauchShell(sfDumpCmd, dumpFileName, true);
        }
    }

    void FPSMeasure::StartDumpsys()
    {
        if (dumpTimer == nullptr)
        {
            auto dutPtr = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
            if (dutPtr == nullptr)
                return;

            string dutID = dutPtr->GetDeviceName();
            string dumpCmd = dutPtr->GetAdbPath() + " -s " + dutID +" shell dumpsys SurfaceFlinger";

            dumpTimer = boost::shared_ptr<HighResolutionTimer>(new HighResolutionTimer());
            dumpTimer->SetTimerHandler(boost::bind(&FPSMeasure::DumpTimerHandler, this, _1));
            dumpTimer->SetPeriod(1000);

            string fileName = TestReport::currentQsLogPath + "\\" + GetClockString() + ".sf.dump";
            if (LauchShell(dumpCmd, fileName))
            {
                auto istream = ifstream(fileName, ios::binary);
                if (istream.good())
                {
                    string layer = GetTargetLayer(istream, dutAppName);
                    DAVINCI_LOG_DEBUG << layer;
                    if (layer.size() > 0)
                    {
                        if (layer == "SurfaceView")
                            isStartedAtInit = false;

                        sfDumpCmd = dutPtr->GetAdbPath() + " -s " + dutID + " shell dumpsys SurfaceFlinger --latency " + layer;
                        dumpFileName = TestReport::currentQsLogPath + "\\" + GetClockString() + ".sf.latency.dump";
                        dumpTimer->Start();
                    }
                }
            }
        }
    }

    void FPSMeasure::StartTrace()
    {
        if (isStarted)
            return;
        else
            isStarted = true;

        try
        {
            startTime = GetCurrentTimeString();

            StartDumpsys();

            DAVINCI_LOG_INFO << "Started dumpsys";
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non standard exception happened.";
        }
    }

    void FPSMeasure::Start()
    {
        StartTrace();
    }

    void FPSMeasure::StopTrace()
    {
        if (isTraceStop)
            return;
        else
            isTraceStop = true;

        try
        {
            stopTime = GetCurrentTimeString();

            if (!dumpFileName.empty())
            {
                dumpTimer->Stop();
                DumpTimerHandler(*dumpTimer);
                UnderstandLatency(dumpFileName, maxTraceFPS, minTraceFPS, avgTraceFPS, jankRatio);
            }

            if (testReportSummary != nullptr )
            {
                string caseName = this->caseName;
                if (caseName.empty())
                    return;

                string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
                string totalreasultvalue = "N/A";
                string totalmessagevalue = "N/A";
                string logcattxt = "N/A";
                string totalresult = "N/A";
                string failReason = "N/A";
                string avgfpsvalue = boost::str(boost::format("%.2f") % avgTraceFPS);
                string maxfpsvalue = boost::str(boost::format("%.2f") % maxTraceFPS);
                string minfpsvalue = boost::str(boost::format("%.2f") % (boost::math::isnan(minTraceFPS) ? 0.0 : minTraceFPS));

                testReportSummary->appendFPSResultNode(currentReportSummaryName,
                    "type",
                    "RNR",
                    "name",
                    caseName,
                    totalreasultvalue,
                    totalmessagevalue,
                    logcattxt,
                    "FPS", //sub case name
                    "FPS", // result
                    avgfpsvalue,
                    maxfpsvalue,
                    minfpsvalue,
                    startTime, //start time
                    stopTime, //end time
                    "N/A", //message
                    "N/A"); //link
            }
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non standard exception happened.";
        }

    }

    void FPSMeasure::PreDestroy()
    {
        if (IsFinished())
            return;

        SetActive(false);

        StopTrace();

        boost::unique_lock<boost::mutex> lk(storeFrameQueueLock);
        while (storeFrameQueue.size() > 0)
        {
            storeFrameQueueEvt.notify_one();
            lk.unlock();
            ThreadSleep(10);
            lk.lock();
        }
        lk.unlock();

        if (storeThread != nullptr)
        {
            storeThread->StopAndJoin();
            storeThread = nullptr;
        }
        if (matcherThread != nullptr)
        {
            matcherThread->StopAndJoin();
            matcherThread = nullptr;
        }

        DAVINCI_LOG_DEBUG << "Meaningful frame count:" << cameraFrameCnt;

        DestroyInternal();
    }

    void FPSMeasure::DestroyInternal()
    {
        try
        {
            if (nullptr == rawVideoFileStrm)
                return;
            rawVideoFileStrm->close();

            ifstream istrm = ifstream(rawVideoFileName, ios::binary);
            string head = "";
            int width = 0, height = 0;
            boost::scoped_array<char> buf;
            boost::scoped_array<char> bufPrev;
            double timeCurrent = .0,  tms = .0;

            while (istrm.good())
            {
                if (head == "")
                {
                    getline(istrm, head);
                    if (head.size() <= 0)
                        break;

                    vector<string> fileds;
                    boost::char_separator<char> sep(" ");
                    auto token = boost::tokenizer<boost::char_separator<char>>(head, sep);
                    for (auto& elm : token)
                    {
                        fileds.push_back(elm);
                    }

                    width = boost::lexical_cast<int>(fileds[0]);
                    height = boost::lexical_cast<int>(fileds[1]);
                    if (width == 0 || height == 0)
                        break;

                    if (boost::math::isnan(motionMinDistance))
                    {
                        motionMinDistance = sqrt(1.0*width*width/dispWidth/dispWidth
                            + 1.0*height*height/dispHeight/dispHeight);
                    }
                    buf.reset(new char[width*height]);
                    bufPrev.reset(new char[width*height]);
                    if (640 == width)
                        motionMinDistance = 0.5F;
                }
                else
                {
                    string line;
                    getline(istrm, line);
                    if (line.size() > 0)
                    {
                        DAVINCI_LOG_DEBUG << line;
                        tms = boost::lexical_cast<double>(line);
                    }

                    if (!istrm.eof() && buf != nullptr)
                    {
                        istrm.read(buf.get(), width*height);

                        Mat gray(height, width, CV_8UC1, buf.get());
                        //imwrite("C:\\offline-" + line +".png", gray);
                        AnalyseFrame(gray);
                        buf.swap(bufPrev);

                        if (FEquals(tms, timeCurrent) == false)
                        {
                            double frameFps = 1000 * 1.0 / (tms - timeCurrent);
                            if (frameFps > maxFPS)
                                maxFPS = frameFps;
                            if (boost::math::isnan(minFPS) ||
                                frameFps < minFPS)
                                minFPS = frameFps;
                        }

                        timeCurrent = tms;
                    }
                }
            }

            if (movingFrameCnt > 0 && timeCurrent > firstStoreTime)
                avgFPS = 1000.0 * movingFrameCnt / (timeCurrent - firstStoreTime);

            avgFPS = boost::math::isnan(avgFPS) ? 0.0 : avgFPS;
            DAVINCI_LOG_INFO << "Camera FPS:" + boost::str(boost::format("%.2f") % avgFPS)
                + ",max:" + boost::str(boost::format("%.2f") % maxFPS)
                + ",min:" + boost::str(boost::format("%.2f") % (boost::math::isnan(minFPS) ? 0.0 : minFPS));

            rawVideoFileStrm = nullptr;

            if (testReportSummary != nullptr)
            {
                string caseName = this->caseName;
                if (caseName.empty())
                    return;

                string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
                string totalreasultvalue = "N/A";
                string totalmessagevalue = "N/A";
                string logcattxt = "N/A";
                string totalresult = "N/A";
                string failReason = "N/A";
                string avgfpsvalue = boost::str(boost::format("%.2f") % avgFPS);
                string maxfpsvalue = boost::str(boost::format("%.2f") % maxFPS);
                string minfpsvalue = boost::str(boost::format("%.2f") % (boost::math::isnan(minFPS) ? 0.0 : minFPS));

                testReportSummary->appendFPSResultNode(currentReportSummaryName,
                    "type",
                    "RNR",
                    "name",
                    caseName,
                    totalreasultvalue,
                    totalmessagevalue,
                    logcattxt,
                    "Camera FPS", //sub case name
                    "FPS", // result
                    avgfpsvalue,
                    maxfpsvalue,
                    minfpsvalue,
                    startTime, //start time
                    stopTime, //end time
                    "N/A", //message
                    "N/A"); //link
            }
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non standard exception happened.";
        }
    }

    void FPSMeasure::AnalyseFrame(const Mat &frame)
    {
        if (!prevGray.empty() && !corners[0].empty())
        {
            int mvPointCnt = 0;
            bool isMoving = false;
            vector<uchar> status;
            vector<float> err;

            calcOpticalFlowPyrLK(prevGray, frame, corners[0], corners[1],
                status, err, winSize, maxLevel, criteria);
            for (unsigned int i = 0; i < corners[0].size(); i++)
            {
                if (0 == status[i])
                {
                    //doesn't match
                    continue;
                }
                float dx, dy;
                dx = corners[1][i].x - corners[0][i].x;
                dy = corners[1][i].y - corners[0][i].y;
                if (dx*dx + dy*dy > motionMinDistance*motionMinDistance)
                {
                    // motion found
                    isMoving = true;
                    mvPointCnt ++;
                }
            }
            if (isMoving)
                movingFrameCnt++;
        }

        if (corners[0].empty())
        {
            DAVINCI_LOG_DEBUG << "No feature points detected.";
        }

        Mat mask;
        goodFeaturesToTrack(frame, corners[0], maxFeaturesPerChannel,
            qualityLevel, minDistance, mask, blockSize, useHarris, k);
        prevGray = frame;
    }

    static bool MatcherFuncInside(boost::shared_ptr<FeatureMatcher> matcher,
        const Mat& frame, boost::shared_ptr<double> ratio, PreprocessType preType)
    {
        double dummy = 0.0;
        Mat ret = matcher->Match(frame, 1.0F, 1.0, preType, &dummy);
        *ratio = dummy;
        if (!ret.empty())// matched
            return true;
        return false;
    }

    void FPSMeasure::SetDisplaySize(int width, int height)
    {
        if (width > 0 && height > 0)
        {
            dispWidth = width;
            dispHeight = height;
            motionMinDistance = numeric_limits<double>::quiet_NaN();
        }
    }

    void FPSMeasure::GetDisplaySize(int & width, int & height)
    {
        width = dispWidth;
        height = dispHeight;
    }

    bool FPSMeasure::StoreFunc()
    {
        using namespace boost::chrono;
        FrameInfo info;
        try
        {                       
            boost::unique_lock<boost::mutex> lk(storeFrameQueueLock);
            if (boost::cv_status::no_timeout == storeFrameQueueEvt.wait_for(lk, milliseconds(100)))
            {
                while (storeFrameQueue.size() > 0)
                {
                    info = storeFrameQueue.front();
                    storeFrameQueue.pop();

                    if (!info.image.empty())
                    {
                        Mat gray;
                        cv::cvtColor(info.image, gray, CV_BGR2GRAY);
                        stringstream ss;
                        ss.precision(6);
                        ss << fixed <<  info.timestamp << endl;

                        if (rawVideoFileStrm != nullptr)
                        {
                            *rawVideoFileStrm << ss.rdbuf();
                            rawVideoFileStrm->write((char*)gray.data, gray.rows*gray.cols*gray.elemSize());
                        }
                        //imwrite("debug.png", gray);
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
            return false;
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non stand exception happened.";
            return false;
        }       
        return true;
    }

    bool FPSMeasure::MatcherFunc()
    {
        try
        {
            Mat frame;
            auto doubPtr = boost::shared_ptr<double>(new double());

            boost::unique_lock<boost::mutex> guard(matcherFrameQueueLock);
            if (matcherFrameQueue.size() > 0)
            {
                frame = matcherFrameQueue.front();
                matcherFrameQueue.pop();
            }

            guard.unlock();

            if (frame.empty())
            {
                boost::this_thread::yield();
                return true;
            }

            if (!isStarted
                && !matchStartImage.empty())
            {
                DAVINCI_LOG_DEBUG  << "matcherStart->Match";
                if (MatcherFuncInside(matcherStart, frame, doubPtr, obsPreprocessType))
                {
                    DAVINCI_LOG_INFO << string("FPS test started, unmatched ratio is: ") << *doubPtr;
                    imwrite(TestReport::currentQsLogPath + "\\" + GetClockString() + ".matchedB.png", frame);
                    Start();
                }
            }
            else
            {
                DAVINCI_LOG_DEBUG  << "matcherStop->Match";
                if (MatcherFuncInside(matcherStop, frame, doubPtr, obsPreprocessType))
                {
                    DAVINCI_LOG_INFO << string("FPS test stopped, unmatched ratio is: ") << *doubPtr;
                    imwrite(TestReport::currentQsLogPath + "\\" + GetClockString() + ".matchedE.png", frame);

                    SetActive(false);

                    StopTrace();

                    return false;
                }
            }
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
            return false;
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non stand exception happened.";
            return false;
        }

        return true;
    }

    Mat FPSMeasure::ProcessFrame(const Mat& frame, const double timeStamp)
    {
        if (!IsActive())
            return frame;

        try
        {
            if (totalFrameCnt % framesPerMatch == 0
                && ( (!matchStopImage.empty() && isStarted)
                || (!matchStartImage.empty() && !isStarted)))
            {
                if (matcherThread == nullptr)
                {
                    matcherThread.reset(new GracefulThread(boost::bind(
                        &FPSMeasure::MatcherFunc, this), true));
                    matcherThread->Start();
                }

                if (isTraceStop)
                {
                    totalFrameCnt ++;
                    return frame;
                }

                Mat newFrame = frame.clone();
                boost::lock_guard<boost::mutex> guard(matcherFrameQueueLock);
                if (matcherFrameQueue.size() == maxFrameQueueSize)
                {
                    DAVINCI_LOG_WARNING << "Frame dropped while matching.";
                    matcherFrameQueue.pop();
                }
                matcherFrameQueue.push(newFrame);
            }

            totalFrameCnt ++;

            if (!isStarted)
                return frame;

            if (!enableCam)
                return frame;

            if (0 == cameraFrameCnt)
            {
                boost::filesystem::path rawVideoFileNamePath(TestReport::currentQsLogPath);
                string fileName = GetClockString() + "." + videoFileName;
                rawVideoFileNamePath /= boost::filesystem::path(fileName);
                rawVideoFileName = rawVideoFileNamePath.string();
                rawVideoFileStrm.reset(new ofstream(rawVideoFileName, ios::binary | ios::trunc));
                if (!rawVideoFileStrm->good())
                {
                    DAVINCI_LOG_ERROR << "Cannot create file: " + rawVideoFileName;
                    rawVideoFileStrm = nullptr;
                    return frame;
                }
                stringstream ss;
                Size sz = frame.size();
                ss << sz.width << " " << sz.height << endl;
                *rawVideoFileStrm << ss.rdbuf();
            }

            cameraFrameCnt ++;

            if (rawVideoFileStrm != nullptr)
            {
                if (storeThread == nullptr)
                {
                    storeThread.reset(new GracefulThread(boost::bind(
                        &FPSMeasure::StoreFunc, this), true));
                    storeThread->Start();
                }

                if (boost::math::isnan(firstStoreTime))
                {
                    firstStoreTime = timeStamp;
                }

                FrameInfo info;
                info.image = frame.clone();
                info.timestamp = timeStamp;
                boost::unique_lock<boost::mutex> guard(storeFrameQueueLock);
                if (storeFrameQueue.size() == maxFrameQueueSize)
                {
                    DAVINCI_LOG_WARNING << "Frame dropped while storing.";
                    droppedFramesCount ++;
                    storeFrameQueue.pop();
                }
                storeFrameQueue.push(info);
                storeFrameQueueEvt.notify_one();
            }
        }
        catch (const std::exception& ex)
        {
            DAVINCI_LOG_ERROR << ex.what();
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Non stand exception happened.";
        }

        return frame;
    }

    void FPSMeasure::SetFPSStartImage(const Mat &image, PreprocessType refPrepType, PreprocessType obsPrepType)
    {
        matchStartImage = image;
        matcherStart.reset(new FeatureMatcher());
        matcherStart->SetModelImage(image, false, 1.0F, refPrepType);
        refPreprocessType = refPrepType;
        obsPreprocessType = obsPrepType;
    }

    void FPSMeasure::SetFPSStopImage(const Mat &image, PreprocessType refPrepType, PreprocessType obsPrepType)
    {
        matchStopImage = image;
        matcherStop.reset(new FeatureMatcher());
        matcherStop->SetModelImage(image, false, 1.0F, refPrepType);
        refPreprocessType = refPrepType;
        obsPreprocessType = obsPrepType;
    }

    void FPSMeasure::InitTrace()
    {
        traceFrameCount = 0;

        maxTraceFPS = 0.0;
        minTraceFPS = numeric_limits<double>::quiet_NaN();
        avgTraceFPS = numeric_limits<double>::quiet_NaN();
    }

    bool FPSMeasure::Init()
    {
        cameraFrameCnt = 0;

        maxFPS = .0;
        minFPS = numeric_limits<double>::quiet_NaN();
        avgFPS = numeric_limits<double>::quiet_NaN();
        jankRatio = .0;

        movingFrameCnt = 0;

        totalFrameCnt = 0;

        SetName("FPSMeasure");

        InitTrace();

        testReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());

        firstStoreTime = numeric_limits<double>::quiet_NaN();

        return true;
    }

    FPSMeasure::~FPSMeasure()
    {
        if (boost::filesystem::exists(rawVideoFileName))
            boost::filesystem::remove(rawVideoFileName);
    }
}
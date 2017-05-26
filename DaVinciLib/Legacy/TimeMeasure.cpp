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

#include "TimeMeasure.hpp"
#include "DeviceManager.hpp"
#include "QScript/VideoCaptureProxy.hpp"
#include "boost/filesystem.hpp"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include "TestReport.hpp"

#include <fstream>

using namespace cv;

namespace DaVinci
{
    int64 DeviceFrameIndexInfo::devCurTimeStamp = -1;

    bool DeviceFrameIndexInfo::ParseFrameIndexInfo()
    {
        if (!DeviceManager::Instance().UsingHyperSoftCam())
            return false;

        const std::string FRAME_TIMESTAMP_FILE("frame_timestap_info");
        if (boost::filesystem::exists(FRAME_TIMESTAMP_FILE))
            boost::filesystem::remove(FRAME_TIMESTAMP_FILE);

        dut = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
        assert(dut != NULL);
        dut->AdbCommand(std::string("pull /data/local/tmp/") + FRAME_TIMESTAMP_FILE + std::string(" ."));
        dut->AdbCommand(std::string("shell rm /data/local/tmp/") + FRAME_TIMESTAMP_FILE);

        std::ifstream frameIndexFile(FRAME_TIMESTAMP_FILE);
        if (frameIndexFile.is_open()) {
            std::string frameIndexItem;
            while (getline(frameIndexFile, frameIndexItem)) {
                std::vector<std::string> frameIndexSubItems;
                boost::split(frameIndexSubItems, frameIndexItem, boost::is_any_of("#"), boost::token_compress_on);
                if (frameIndexSubItems.size() != 2) {
                    DAVINCI_LOG_ERROR << "Failed in parsing OnDevice frame index file";
                    frameIndexFile.close();
                    return false;
                }

                frameIndexInfo[boost::lexical_cast<int>(frameIndexSubItems[0])] = 
                    boost::lexical_cast<int64>(frameIndexSubItems[1]);
            }

            frameIndexFile.close();
            boost::filesystem::remove(FRAME_TIMESTAMP_FILE);
            return true;
        }

        DAVINCI_LOG_ERROR << "Failed in opening OnDevice frame index file";
        return false;
    }

    int64 DeviceFrameIndexInfo::CalculateElapsedTime(int frameIndex)
    {
        return frameIndexInfo[frameIndex] - devCurTimeStamp;
    }

    TimeMeasure::TimeMeasure(const std::string &iniFilename):timeout(0),hit(false),startTime(0),
        ini_filename(iniFilename),defaultUnmatchRatioThreshold(1.0),unmatchRatioThreshold(defaultUnmatchRatioThreshold),
        lastElapsed(0.0),lastHitElapsed(-1.0),lastResult(Mat()), isOffline(false),saveVideo(false),
        defaultRoiUnmatchRatioThreshold(1.0),targetRoi(0,0,0,0), roiUnmatchedRatioThreshold(defaultRoiUnmatchRatioThreshold)
    {
        pMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());        
    }

    bool TimeMeasure::Init()
    {
        SetName("TimeMeasure");
        boost::shared_ptr<TargetDevice> device = DeviceManager::Instance().GetCurrentTargetDevice();
        testReport = boost::shared_ptr<TestReport>(new TestReport());

        if(device == nullptr || device->GetDeviceStatus() == DeviceConnectionStatus::Offline)
        {
            DAVINCI_LOG_ERROR << (std::string("Error: no device connected - ") + ini_filename);
            SetFinished();
            return false;
        }

        device->GetDeviceWidth();

        if (!boost::filesystem::is_regular(ini_filename))
        {
            DAVINCI_LOG_WARNING << (std::string("Warning: ini file doesn't exist - ") + ini_filename);
            SetFinished();
            return false;
        }
        DAVINCI_LOG_INFO<<(std::string("TimeMeasure Module: ini file name = ") + ini_filename);
        vector<string> lines;
        if(!DaVinciSuccess(ReadAllLines(ini_filename,lines)))
        {
            SetFinished();
            return false;
        }
        if (lines.size() < 3)
        {
            DAVINCI_LOG_INFO<<(std::string("Bad ini file ") + ini_filename);
            SetFinished();
            return false;
        }
        apk_class = lines[0];
        std::string cmd_param = lines[1];
        std::string object_filename = lines[2];

        // Two kinds of object file path are supported in ini file:
        // 1. the absolute path. Example: C:\DaVinci\examples\launch_tests\object.jpg.
        // 2. just file name. assumed object file is put in same folder with ini file (C:\DaVinci\examples\launch_tests\object). Example: object.jpg.
        if((boost::filesystem::path(object_filename)).is_absolute() == false)
        {
            boost::filesystem::path p(ini_filename);
            boost::filesystem::path parentPath = p.parent_path();
            parentPath/=object_filename;
            object_filename = parentPath.generic_string();
        }
        if (!boost::filesystem::is_regular_file(object_filename))
        {
            DAVINCI_LOG_WARNING <<(std::string("Warning: object file doesn't exist - ") + object_filename + std::string(". Please use absolute path or relative path to ini file."));
            SetFinished();
            return false;
        }
        referenceImage = object_filename;

        if(lines.size() >= 4 && lines[3] == "TRUE")
        {
            saveVideo = true;
        }

        if(lines.size() >= 5 && !(TryParse(lines[4],timeout)))
        {
            timeout=180;
        }

        if (lines.size() >= 6)
        {
            if(!TryParse(lines[5],unmatchRatioThreshold))
            {
                unmatchRatioThreshold = defaultUnmatchRatioThreshold;
            }
        }

        string roiFile;
        if(lines.size() >= 7)
        {
            roiFile = lines[6];
            boost::trim(roiFile);
            if(!roiFile.empty() && (boost::filesystem::path(roiFile)).is_absolute() == false)
            {
                boost::filesystem::path p(ini_filename);
                boost::filesystem::path parentPath = p.parent_path();
                parentPath/=roiFile;
                roiFile = parentPath.generic_string();
            }
            if (!boost::filesystem::is_regular_file(roiFile))
            {
                DAVINCI_LOG_WARNING <<(std::string("Warning: object file doesn't exist - ") + roiFile + std::string(". Please use absolute path or relative path to ini file."));
                SetFinished();
                return false;
            }
        }

        refMat = imread(object_filename);
        refSize.width = refMat.cols;
        refSize.height = refMat.rows;

        if(!roiFile.empty() && roiFile != object_filename)
        {

            cv::Mat object_roi = imread(roiFile);
            pMatcher->SetModelImage(object_roi);
            cv::Mat homo =  pMatcher->Match(refMat);
            if(!homo.empty())
            {
                targetRoi = pMatcher->GetBoundingRect();
                if(targetRoi.x < 0)
                {
                    targetRoi.x = 0;
                }
                if(targetRoi.y < 0)
                {
                    targetRoi.y = 0;
                }
                if(targetRoi.width + targetRoi.x > refSize.width)
                {
                    targetRoi.width = refSize.width - targetRoi.x;

                }
                if(targetRoi.height + targetRoi.y > refSize.height)
                {
                    targetRoi.height = refSize.height - targetRoi.y;
                }

            }
        }
        else
        {
            pMatcher->SetModelImage(refMat);
        }
        if (lines.size() >= 8)
        {
            if(targetRoi.area() != 0)
            {
                if(!TryParse(lines[7],roiUnmatchedRatioThreshold))
                {
                    roiUnmatchedRatioThreshold = defaultRoiUnmatchRatioThreshold;
                }
            }
        }


        /*else
        {
        if(targetRoi.area() != 0)
        {               
        unmatchRatioThreshold = defaultRoiUnmatchRatioThreshold;
        }
        else
        {
        unmatchRatioThreshold = defaultUnmatchRatioThreshold;    
        }
        }*/


        //DAVINCI_LOG_DEBUG<<(std::string("image_size = ") +boost::lexical_cast<string>( image_size));
        //cv::Mat object_ori =imread (object_filename);
        //int widthOri = object_ori.cols;
        //DAVINCI_LOG_DEBUG<<(std::string("object size = ") + boost::lexical_cast<string>(object_ori.cols) + std::string("x") + boost::lexical_cast<string>(object_ori.rows));
        ////int obj_size = (object_ori.cols + object_ori.rows) / 2;
        /*if (image_size == 0)
        {*/
        //object_resize = object_ori;
        /*}
        else if (object_ori.cols > image_size && object_ori.rows > image_size)
        {
        resize(object_ori,object_resize,Size(image_size,image_size * object_ori.rows / object_ori.cols),0.0,0.0,CV_INTER_AREA);

        }
        else
        {
        object_resize = object_ori;
        }*/
        //pMatcher->SetModelImage(object_resize,false,(float)object_resize.cols/widthOri);

        //pMatcher->pmObjectRec->DumpKeyPointsNumber();
        lastElapsed = 0.0;
        lastHitElapsed = -1.0;
        lastResult=Mat();
        hit = false;

        startTime = 0;
        tempFilePath = TestManager::Instance().GetDaVinciResourcePath("Temp");

        if(!boost::filesystem::exists(boost::filesystem::path(tempFilePath))){
            boost::filesystem::create_directory(boost::filesystem::path(tempFilePath));
        }
        if(TestManager::Instance().IsOfflineCheck()){
            isOffline = true;
        }else{
            isOffline = false;
        }
        boost::filesystem::path tmpVideoFilePath(TestReport::currentQsLogPath + "/" + apk_class + ".avi");
        tmpVideoFile = boost::filesystem::system_complete(tmpVideoFilePath).string();
        testReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());        
        return true;
    }

    CaptureDevice::Preset TimeMeasure::CameraPreset()
    {
        return CaptureDevice::Preset::HighResolution;
    }

    void TimeMeasure::Destroy()
    {
        boost::shared_ptr<AndroidTargetDevice> device = boost::dynamic_pointer_cast<AndroidTargetDevice>((DeviceManager::Instance()).GetCurrentTargetDevice());
        if (device != nullptr)
        {
            // tap home              input keyevent 3
            device->AdbShellCommand("input keyevent 3");
            device->StopActivity(apk_class);
        }

        if (videoWriter.isOpened())
        {
            videoWriter.release();
        }

        //generate logcat
        if (testReport != nullptr)
        {
            testReport->GetLogcat("logcat");
            testReport->CopyDaVinciLog();
        }
    }

    void TimeMeasure::Start()
    {
        startTime = GetCurrentMillisecond();
        TestInterface::Start();
    }

    double TimeMeasure::GetElapsed(double timeStamp)
    {
        if (FEquals(startTime, 0.0))
        {
            return 0.0;
        }
        // unit: second
        return (double)(timeStamp - startTime) / 1000.0;
    }

    cv::Mat TimeMeasure::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        double elapsed = GetElapsed(timeStamp);
        //drop the frame that before test group start.
        if(FSmaller(elapsed, 0.0))
        {
            return frame;
        }

        bool last_hit = hit;
        float scale = 1.0f;
        cv::Mat result;
        cv::Mat image_resize;
        if(isOffline){
            if (elapsed < timeout){

                if(!videoWriter.isOpened()){
                    videoFrameSize.width = frame.cols;
                    videoFrameSize.height = frame.rows;
                    if (!videoWriter.open(tmpVideoFile, CV_FOURCC('M', 'J', 'P', 'G'), 30, videoFrameSize))
                    {
                        DAVINCI_LOG_ERROR << "      ERROR: Cannot create the video file for training or replaying!";
                        SetFinished();
                        return frame;
                    }
                }

                Mat frameResize = frame;
                assert(frame.size > 0);
                if ( (frame.cols != videoFrameSize.width || frame.rows != videoFrameSize.height))
                {
                    DAVINCI_LOG_ERROR << "ERROR: The frame size changed from (" << videoFrameSize.width << ","
                        << videoFrameSize.height << ") to (" << frame.cols <<"," << frame.rows << ")";
                    SetFinished();
                    return frame;
                }
                //timeStampList.push_back(qsWatch.ElapsedMilliseconds());
                videoWriter.write(frameResize);
                boost::shared_ptr<FrameInfo> fInfo(new FrameInfo(-1, false, 1.0, curFrameIndex));
                fInfo->elapsed = elapsed;
                frameInfos.push_back(fInfo);
            }else{

                if (videoWriter.isOpened())
                {
                    videoWriter.release();
                }
                //If reference much bigger than observe feature point may miss matched.
                if(targetRoi.area() == 0 && frame.cols <= (refMat.cols * 0.5))
                {
                    cv::Mat resized;
                    resize(refMat, resized, cv::Size(frame.cols, frame.cols *refMat.rows/refMat.cols));
                    pMatcher->SetModelImage(resized);
                }

                int hittedFrame = matchFirstFrame();
                if(hittedFrame > 0){
                    if (devFrameIndexInfo.ParseFrameIndexInfo()) {
                        for (auto it = frameInfos.begin(); it != frameInfos.end(); it++) {
                            double elapsedSecs = devFrameIndexInfo.CalculateElapsedTime((*it)->deviceFrameIndex) / 1000.0;
                            (*it)->elapsed = elapsedSecs;
                        }
                    }

                    saveResultFrames(hittedFrame);
                    testReportSummary->appendTxtReportSummary("\n========\n");
                    testReportSummary->appendTxtReportSummary("[LaunchTimeTest]:[" + TestReport::currentQsLogPath + "]\n");
                    testReportSummary->appendTxtReportSummary("TimeMeasure Module: ini file name = " + ini_filename);
                    testReportSummary->appendTxtReportSummary("Hit " + boost::lexical_cast<string>(boost::format("%.3f") % frameInfos[hittedFrame]->elapsed) +  " seconds.\n");
                } else {
                    DAVINCI_LOG_WARNING << "Timeout "  << timeout << " seconds" << endl;
                    testReportSummary->appendTxtReportSummary("TimeMeasure Module: ini file name = " + ini_filename);
                    testReportSummary->appendTxtReportSummary("Timeout " +  boost::lexical_cast<string>(timeout) + " seconds\n");

                }

                if(!saveVideo)
                {
                     try
                    {
                        boost::filesystem::remove(tmpVideoFile);
                    }
                    catch(boost::filesystem::filesystem_error e)
                    {
                        DAVINCI_LOG_WARNING << e.what();
                    }
                }
                SetFinished();
            }

        } else{
            image_resize = frame.clone();
            double unmatchRatio = 1.0;
            result = DetectObject(pMatcher, image_resize,  hit, scale, unmatchRatio);


            if (last_hit == true)
            {
                hit = true;
            }
            if (last_hit == false && hit == true)
            {
                lastHitElapsed = elapsed;
                DAVINCI_LOG_INFO<<(apk_class + std::string("\n"));

                //how to set the precision

                DAVINCI_LOG_INFO << std::string("Hit ")
                    << setprecision(3)
                    << ((elapsed + lastElapsed) / 2.0)
                    << std::string(" +/- ")
                    << ((elapsed - lastElapsed) / 2.0)
                    << std::string(" seconds.\n");

                string launchTimeValue = boost::lexical_cast<string>(boost::format("%.3f") % ((elapsed + lastElapsed) / 2.0)) + std::string(" +/- ") + boost::lexical_cast<string>(boost::format("%.3f") % ((elapsed - lastElapsed) / 2.0));

                testReportSummary->appendTxtReportSummary("\n========\n");
                testReportSummary->appendTxtReportSummary(std::string("[LaunchTimeTest]:[") +  TestReport::currentQsLogPath + std::string("]\n"));
                testReportSummary->appendTxtReportSummary(std::string("TimeMeasure Module: ini file name = ") + ini_filename);
                testReportSummary->appendTxtReportSummary(std::string("Hit ") + launchTimeValue  + std::string(" seconds.\n"));
                testReportSummary->appendTxtReportSummary("\n\n");

                string resultFileName = TestReport::currentQsLogPath + "\\" +  string("result") + string(".png");
                imwrite(boost::filesystem::system_complete(resultFileName).string(), result);



                string resultFileName1 = resultFileName;

                if (!lastResult.empty())
                {
                    resultFileName1 = TestReport::currentQsLogPath + "\\" + string("result-1") 
                        //+ boost::lexical_cast<string>((int)GetCurrentMillisecond())
                        + string(".png");
                    imwrite(boost::filesystem::system_complete(resultFileName1).string(), lastResult);
                }


                string referenceImage_ = TestReport::currentQsLogPath + string("\\reference.png");
                CopyFile(referenceImage.c_str(), referenceImage_.c_str(), true);

                PopulateReport("reference.png", resultFileName,  resultFileName1, launchTimeValue);    



            }

            lastResult = result.clone();
            if (FSmaller((double)timeout, elapsed)  && !IsFinished() && !hit)
            {
                DAVINCI_LOG_INFO<< "Failed to get launch time result " <<(std::string("Timeout ") + boost::lexical_cast<string>(timeout) + std::string(" seconds\n"));
                
                testReportSummary->appendTxtReportSummary(std::string("TimeMeasure Module: ini file name = ") + ini_filename);
                testReportSummary->appendTxtReportSummary(std::string("Timeout ") + boost::lexical_cast<string>(timeout) + std::string(" seconds\n"));

                string resultFileName = TestReport::currentQsLogPath + "/" + string("result-1") 
                    //+ boost::lexical_cast<string>((int)GetCurrentMillisecond())
                    + string(".png");
                imwrite(boost::filesystem::system_complete(resultFileName).string(), result);
                SetFinished();
            }            
            else if ((FSmaller(lastHitElapsed, 0.0) == false)
                && (FSmaller(elapsed - lastHitElapsed, 5.0) == false))
            {
                SetFinished();
            }

            lastElapsed = elapsed;
        }

        return result;
    }

    int TimeMeasure::matchFirstFrame(){

        VideoCaptureProxy cap(tmpVideoFile);

        cv::Mat vf;
        double unmatchedRatio = 1.0;
        float scale = 1.0f; 


        unsigned int shouldBeTheFrame = 0;

        for(unsigned int i = shouldBeTheFrame; i < frameInfos.size(); i += 10)
        {
            cap.read(vf);
            /*boost::filesystem::path imgPath = (tempFilePath + "/" + boost::lexical_cast<std::string>(i) + ".png");
            cv::imwrite(boost::filesystem::system_complete(imgPath).string(), vf);*/
            unmatchedRatio = 1.0;
            hit = false;
            DetectObject(pMatcher, vf, hit, scale, unmatchedRatio);
            if(hit)
            {
                shouldBeTheFrame = (i-20) > 0 ? (i-20): 0;
                break;
            }
            cap.set(CV_CAP_PROP_POS_FRAMES, i);
        }

        cap.set(CV_CAP_PROP_POS_FRAMES, shouldBeTheFrame);

        while(shouldBeTheFrame < frameInfos.size()){
            cap.read(vf);
            /*boost::filesystem::path imgPath = (tempFilePath + "/" + boost::lexical_cast<std::string>(shouldBeTheFrame) + ".png");
            cv::imwrite(boost::filesystem::system_complete(imgPath).string(), vf);*/

            unmatchedRatio = 1.0;
            hit = false;
            DetectObject(pMatcher, vf,  hit, scale, unmatchedRatio);

            DAVINCI_LOG_INFO << "frame: " << boost::lexical_cast<int>(shouldBeTheFrame) << "\t" << boost::lexical_cast<double>(unmatchedRatio) ;

            frameInfos[shouldBeTheFrame]->frameIndex = shouldBeTheFrame;

            if(hit){
                return shouldBeTheFrame;
            }

            cap.set(CV_CAP_PROP_POS_FRAMES, shouldBeTheFrame);
            shouldBeTheFrame++;
        }

        return -1;
    }

    

    void TimeMeasure::saveResultFrames(int hitFrame){
        VideoCaptureProxy cap(tmpVideoFile);
        cv::Mat vf;
        if(hitFrame > 0){
            boost::filesystem::path resultPath;

            string logPath  = TestReport::currentQsLogPath;


            cap.set(CV_CAP_PROP_POS_FRAMES, hitFrame-2);
            cap.read(vf);

            resultPath = (logPath  + "/result-1" + ".png");


            cv::imwrite(boost::filesystem::system_complete(resultPath).string(), vf);


            cap.set(CV_CAP_PROP_POS_FRAMES, hitFrame-1);
            cap.read(vf);

            resultPath = (logPath  + "/result" + ".png");
            cv::imwrite(boost::filesystem::system_complete(resultPath).string(), vf);


            string referenceImage_ = TestReport::currentQsLogPath + string("\\reference.png");
            CopyFile(referenceImage.c_str(), referenceImage_.c_str(), true);



            PopulateReport(referenceImage_, "result.png",  "result-1.png", boost::lexical_cast<string>(boost::format("%.3f") % frameInfos[hitFrame]->elapsed)) ;   



            for (int k = 10; (k > 2) && ( hitFrame - k > 0 ); k-- ) {
                cap.set(CV_CAP_PROP_POS_FRAMES,hitFrame - k);
                cap.read(vf);

                resultPath = (logPath  + "/result-" + boost::lexical_cast<std::string>( k-1 ) + "(" + boost::lexical_cast<std::string>(frameInfos[hitFrame - k]->elapsed) + ").png");

                cv::imwrite(boost::filesystem::system_complete(resultPath).string(), vf);
            }
        }
    }

    cv::Mat TimeMeasure::DetectObject( boost::shared_ptr<FeatureMatcher> modelRecognize, const cv::Mat &observedImage, bool &isHit, float scale, double &unmatchedRatio)
    {
        Mat homography;
        isHit = false;

        vector<Point2f> pointArray;

        if(targetRoi.area() != 0)
        {            
            double xRatio = targetRoi.x / (double)refSize.width;
            double yRatio = targetRoi.y / (double)refSize.height;
            double wRatio = targetRoi.width / (double)refSize.width;
            double hRatio = targetRoi.height / (double)refSize.height;

            Rect roi((int)(xRatio * observedImage.cols), (int)(yRatio * observedImage.rows), (int)(wRatio * observedImage.cols), (int)(hRatio * observedImage.rows));
            Mat roiImage = observedImage(roi);

            homography = modelRecognize->Match(observedImage(roi), &unmatchedRatio);

        }
        else
        {
            // the alternative aspect ratio ensure it works for both 16:9 device and 4:3 device
            homography = modelRecognize->Match(observedImage, scale, 
                4.0 * 9.0 / (3.0 * 16.0),PreprocessType::Keep, &unmatchedRatio,
                &pointArray);
        }

        cv::Mat result = observedImage.clone();

#ifdef _DEBUG
        if (pointArray.size() > 0)
        {
            DAVINCI_LOG_DEBUG<<(boost::lexical_cast<string>(pointArray.size()) + std::string(" points matched"));

            int arraySize = (int)(pointArray.size());
            for ( int i = 0; i <arraySize; i++)
            {
                circle(result,pointArray[i],2,Yellow,2);

            }
        }
#endif

        double threshold = unmatchRatioThreshold;
        if (targetRoi.area() != 0)
            threshold = roiUnmatchedRatioThreshold;

        if (homography.empty())
        {
            DAVINCI_LOG_DEBUG << "unmatched ratio: " << unmatchedRatio
                << ", threshold: " << threshold;
        }
        else if (FSmaller(unmatchedRatio, threshold)) //check
        {
            isHit = true;
            vector<Point2f> pts = modelRecognize->GetPts();
            vector<Point> ptsInt;
            vector<vector<Point>> ptss(1);

            ptsInt.resize(pts.size());
            for (size_t i = 0; i<pts.size(); i++)
                ptsInt[i] = pts[i];
            ptss[0] = ptsInt;
            polylines(result, ptss, true, Red, 5);
        }
        else
        {
            DAVINCI_LOG_WARNING << "unmatched ratio: " << unmatchedRatio
                << ", threshold: " << threshold;
        }

        return result;
    }

    void TimeMeasure::PopulateReport(string referenceImage, string targetImage,  string beforeImage, string launchValue)
    {
        //append xml result
        if (testReportSummary != nullptr )
        {
            string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
            string totalreasultvalue = "N/A";
            string totalmessagevalue = "N/A";
            string logcattxt = "N/A";
            string caseName = ini_filename.substr(ini_filename.find_last_of('\\') + 1);
            string subcasename = "Launchtime";

            if(DeviceManager::Instance().UsingHyperSoftCam())
            {
                subcasename = "Launchtime (HyperSoftCam may have overhead on device, Camera is more accurate.)";
            }

            string launchtimevalue = launchValue;
            string referenceimagename = referenceImage.substr(referenceImage.find_last_of('\\') + 1);
            string referenceimageurl = "./" + referenceimagename; 
            string targetimagename = targetImage.substr(targetImage.find_last_of('\\') + 1); 
            string targetimageurl = "./" + targetimagename;
            string beforeimagename = beforeImage.substr(beforeImage.find_last_of('\\') + 1);
            string beforeimageurl = "./" + beforeimagename; 
            string starttime = "N/A";
            string endtime = "N/A";

            testReportSummary->appendLaunchTimeResultNode(currentReportSummaryName,
                "type",
                "RNR",
                "name",
                caseName,
                totalreasultvalue,
                totalmessagevalue,
                logcattxt,
                subcasename, //sub case name
                "Launchtime", // result
                launchtimevalue,
                referenceimagename,
                referenceimageurl,
                targetimagename,
                targetimageurl,
                beforeimagename,
                beforeimageurl,
                starttime, //start time
                endtime, //end time
                "N/A", //message
                "N/A"); //link
        }    
    }
}

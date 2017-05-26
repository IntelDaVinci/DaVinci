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

#include "TempleRunPlayBase.hpp"
#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include "ObjectRecognize.hpp"
#include "FeatureMatcher.hpp"
#include "FPSMeasure.hpp"
#include "DaVinciCommon.hpp"
#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

namespace DaVinci
{
    using namespace boost::filesystem;

    TempleRunPlayBase::TempleRunPlayBase()
    {
        jump_t = 0;
        turn_t = 0;
        down_t = 0;
        tilt_t = 0;

        check = 0;
        played_stage = 0;
        replay_t = 0;
        runCountSavingImages = 3;
        replayInterval = 4000; // tick count
        matchReplayCount = 0;
        maxMatchReplayCount = 4;
        played_frame_count = 0;
        runCount = 0;
        matchReplayCount = 0;
        image_num = 0;
    }

    /// <summary>
    /// constructor for TempleRunPlayBase. Read parameters from ini file
    /// </summary>
    /// <param name="ini_filename"></param>
    TempleRunPlayBase::TempleRunPlayBase(string ini_filename)
    {
        // read application package and activity name from ini file
        char temp[255];
        image_num = 0;
        start_fname = "";
        resume_fname = "";
        run_again_fname = "";
        resultxml = "";
        DLSVMxml = "";

        jump_t = 0;
        turn_t = 0;
        down_t = 0;
        tilt_t = 0;

        check = 0;
        played_stage = 0;
        replay_t = 0;
        runCountSavingImages = 3;
        replayInterval = 4000; // tick count
        matchReplayCount = 0;
        maxMatchReplayCount = 4;
        played_frame_count = 0;
        runCount = 0;
        matchReplayCount = 0;

        if (ini_filename.length() <= 0)
        {
            DAVINCI_LOG_ERROR << string("Wrong ini file: ") << ini_filename << string("for") << GetName() << endl;
            return;
        }

        int ret = GetPrivateProfileString("app_info", "package", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
            package = temp;
        ret = GetPrivateProfileString("app_info", "activity", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
            activity = temp;
        ret = GetPrivateProfileString("images", "image_num", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {
            image_num = boost::lexical_cast<int>(temp);

            for (int i = 0; i < image_num; i++)
            {
                ret = GetPrivateProfileString("images", (string("image") + to_string(i)).c_str(), "", temp, 255, ini_filename.c_str());
                if (ret > 0)
                {
                    boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
                    resPath /= temp;
                    image_fname.push_back(resPath.string());
                }
                else
                {
                    DAVINCI_LOG_ERROR << string("Error: image ") << i << string(" name in ") << ini_filename;
                    return;
                }
            }
        }

        ret = GetPrivateProfileString("images", "start_image", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {
            boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
            resPath /= temp;
            start_fname = resPath.string();
        }
        else
        {
            DAVINCI_LOG_ERROR << string("Error: name of start image in " + ini_filename);
            return;
        }
        ret = GetPrivateProfileString("images", "resume_image", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {
            boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
            resPath /= temp;
            resume_fname = resPath.string();
        }
        else
        {
            DAVINCI_LOG_ERROR << string("Error: name of resume image in " + ini_filename);
            return;
        }
        ret = GetPrivateProfileString("images", "run_again_image", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {

            boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
            resPath /= temp;
            run_again_fname = resPath.string();
        }
        else
        {
            DAVINCI_LOG_ERROR << string("Error: name of run again image in " + ini_filename);
            return;
        }
        ret = GetPrivateProfileString("machine_xml", "result", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {
            boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
            resPath /= temp;
            resultxml = resPath.string();
        }
        else
        {
            DAVINCI_LOG_ERROR << string("Error: name of machine XML in " + ini_filename);
            return;
        }
        ret = GetPrivateProfileString("dlsvm_xml", "dlsvm", "", temp, 255, ini_filename.c_str());
        if (ret > 0)
        {
            boost::filesystem::path resPath(TestManager::Instance().GetDaVinciHome());
            resPath /= temp;
            DLSVMxml = resPath.string();
        }

        currentDevice = DeviceManager::Instance().GetCurrentTargetDevice();

        runCountSavingImages = 3;
        replayInterval = 4000; // tick count

        matchReplayCount = 0;
        maxMatchReplayCount = 4;
    }


    /// <summary>
    /// get the ROI with the specified rectangle
    /// </summary>
    /// <param name="image">observed image</param>
    /// <param name="left">left point of ROI</param>
    /// <param name="right">right point of ROI</param>
    /// <param name="top">top point of ROI</param>
    /// <param name="bottom">bootoom </param>
    /// <param name="width">ROI width</param>
    /// <param name="height">ROI height</param>
    /// <returns></returns>
    cv::Mat TempleRunPlayBase::getROI(cv::Mat const &image, double left, double right, double top, double bottom, int width, int height)
    {
        int x1 = (int)(image.cols * left);
        int x2 = (int)(image.cols * right);
        int y1 = (int)(image.rows * top);
        int y2 = (int)(image.rows * bottom);

        cv::Rect rect(x1, y1, x2 - x1, y2 - y1);

        Mat imageClone = image(rect).clone();
        Mat dst;
        resize(imageClone, dst, Size(width,height), 0, 0, CV_INTER_LINEAR);

        return dst;
    }

    /// <summary>
    /// init camera and related variables
    /// </summary>
    /// <returns>return true if initialization is finished correctly, otherwise false</returns>
    bool TempleRunPlayBase::Init()
    {
        if (start_fname.empty()
            || resume_fname.empty()
            || run_again_fname.empty())
        {
            DAVINCI_LOG_INFO << string("Stop playing as null image file names");
            SetFinished();
            return false;
        }


        startImage = imread(start_fname);

        boost::shared_ptr<ObjectRecognize> object_start_fname = ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm::Orb);
        orbPlay = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(object_start_fname));
        orbPlay->SetModelImage(startImage, false);

        resumeImage = imread(resume_fname);
        boost::shared_ptr<ObjectRecognize> object_resume_fname = ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm::Orb);
        orbResume = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(object_resume_fname));
        orbResume->SetModelImage(resumeImage, false);

        replayImage = imread(run_again_fname);
        boost::shared_ptr<ObjectRecognize> object_replay_fname = ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm::Orb);
        orbReplay = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(object_replay_fname));
        orbReplay->SetModelImage(replayImage, false);

        for (int i = 0; i < image_num; i++)
        {
            if (image_fname[i].empty())
            {
                DAVINCI_LOG_INFO << string("Stop playing as error image file in ini file");
                SetFinished();
                return false;
            }
            cv::Mat tempImage = imread(image_fname[i]);
            boost::shared_ptr<ObjectRecognize> object_temp = ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm::Orb);
            orbImageArray.push_back(boost::shared_ptr<FeatureMatcher>(new FeatureMatcher(object_temp)));
            orbImageArray[i]->SetModelImage(tempImage, false);
        }

        assert(currentDevice!= nullptr);

        if (currentDevice != nullptr)
        {
            string temp = package + "/" + activity;
            currentDevice->StopActivity(package);
            currentDevice->StartActivity(temp);
        }

        replay_t = GetTickCount();

        jump_t = replay_t;
        turn_t = replay_t;
        down_t = replay_t;

        check = 0;
        played_stage = 0;
        played_frame_count = 0;
        runCount = 0;
        matchReplayCount = 0;

        if (resultxml.empty() || exists(resultxml) == false)
        {
            DAVINCI_LOG_INFO << string("Stop playing as training file is NOT found: " + resultxml);
            SetFinished();
            return false;
        }

        return true;
    }

    /// <summary>
    /// main entry for getting frame in Capture thread 
    /// </summary>
    /// <param name="frame">input frame</param>
    /// <param name="timeStamp">timestamp related to the input frame</param>
    /// <returns>output frame to be displayed on UI</returns>
    cv::Mat TempleRunPlayBase::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        const int FRAME_INTERVAL = 30;

        if(frame.empty())
        {
            return frame;
        }
        boost::shared_ptr<ObjectRecognize> object_temp = ObjectRecognize::CreateObjectRecognize(PatchFeatureAlgorithm::Orb);
        FeatureMatcher orbFrame(object_temp);
        orbFrame.SetModelImage(frame, false);

        Mat homography;
        Size frameSize;
        if (frame.cols < frame.rows)
        {
            frameSize.width = frame.rows;
            frameSize.height = frame.cols;
        }
        else
        {
            frameSize.width = frame.cols;
            frameSize.height = frame.rows;
        }

        if (boost::lexical_cast<long>(GetTickCount()) - replay_t > replayInterval)
        {
            if (played_stage < 2)
            {
                if (played_frame_count == 0)
                {
                    Point ptsCenter;

                    //ORBObjectRecognize orbPlayOrResume = orbPlay;
                    homography = orbPlay->Match(orbFrame);
                    if (homography.empty())
                    {
                        homography = orbResume->Match(orbFrame);
                        if (!homography.empty())
                        {
                            ptsCenter = orbResume->GetObjectCenter();
                        }
                    }
                    else
                    {
                        ptsCenter = orbPlay->GetObjectCenter();
                    }

                    if (!homography.empty())
                    {
                        ClickOnFrame(currentDevice, frameSize, ptsCenter.x, ptsCenter.y, currentDevice->GetCurrentOrientation(true), true);
                        DAVINCI_LOG_INFO << string("START/RESUME");

                        if (played_stage == 0)
                        {
                            played_stage = 1;
                        }
                    }
                    else
                    {
                        if (played_stage == 1)
                        {
                            replay_t = GetTickCount();
                            // only start the game if the first frame contains "start" object, and the next 10 frames don't contain it
                            // GetGroup()->Start();
                            // SetActive(true);
                            played_stage = 2;
                        }
                    }
                }

                played_frame_count = (played_frame_count + 1) % FRAME_INTERVAL;

                return frame;
            }

            bool replay = false;

            if (check == 0)
            {
                homography = orbReplay->Match(orbFrame);
                replay = (!homography.empty());
                if (replay)
                {
                    if (matchReplayCount < maxMatchReplayCount)
                    {
                        matchReplayCount++;
                    }
                    else
                    {
                        //if (TestManager.GetRepeatCount() > 0)
                        //{
                        //    runCount++;
                        //    if (runCount >= TestManager.GetRepeatCount())
                        //    {
                        //        SetFinished();
                        //    }
                        //}
                        DAVINCI_LOG_INFO << string("RUN AGAIN");
                        Point ptsCenter = orbReplay->GetObjectCenter();
                        //frame.DrawPolyline(Array.ConvertAll<PointF, Point>(
                        //    orbReplay.GetPts(), Point.Round),
                        //    true, new Bgr(Color.Red), 5);
                        ClickOnFrame(currentDevice, frameSize, ptsCenter.x, ptsCenter.y, currentDevice->GetCurrentOrientation(true), true);

                        // reset count and timestamp
                        matchReplayCount = 0;
                        replay_t = GetTickCount();
                    }
                }


                // if user stop the test in the middle of templerun playing, we should count a more time
                if (IsFinished())
                {
                    if (replay == false)
                        runCount++;
                }
            }


            if (replay == false && check == 0)
            {
                for (int i = 0; i < image_num; i++)
                {
                    homography = orbImageArray[i]->Match(orbFrame);
                    if (!homography.empty())
                    {
                        Point ptsCenter = orbImageArray[i]->GetObjectCenter();
                        //frame.DrawPolyline(Array.ConvertAll<PointF, Point>(
                        //    orbImageArray[i].GetPts(), Point.Round),
                        //    true, new Bgr(Color.Red), 5);
                        ClickOnFrame(currentDevice, frameSize, ptsCenter.x, ptsCenter.y, currentDevice->GetCurrentOrientation(true), true);

                    }
                }
            }
            check = (check + 1) % 100;
        }

        return frame;

    }

    /// <summary>
    /// set finished flag and kill app
    /// </summary>
    void TempleRunPlayBase::Destroy()
    {
        //TestManager::Instance().TiltCenter();
        SetFinished();

        if (currentDevice != nullptr)
        {
            currentDevice->StopActivity(package);
        }

        if (TestManager::Instance().IsConsoleMode())
            DAVINCI_LOG_INFO << string("Number of running TempleRun: ") << runCount;
    }

    /// <summary>
    /// camera preset for TempleRun AI
    /// </summary>
    /// <returns>the camera preset</returns>
    CaptureDevice::Preset TempleRunPlayBase::CameraPreset()
    {
        return CaptureDevice::Preset::AIResolution;
    }
}

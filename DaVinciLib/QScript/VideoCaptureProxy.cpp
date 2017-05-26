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

#include "VideoCaptureProxy.hpp"

#include <boost/filesystem.hpp>

#include "DaVinciCommon.hpp"
#include "VideoCaptureAdapter.hpp"

namespace DaVinci
{
    VideoCaptureProxy::VideoCaptureProxy()
    {
        mVideoCapIndex   = 0;
        mVideoFrameCount = 0;
    }

    VideoCaptureProxy::VideoCaptureProxy(const std::string& filename)
    {
        mVideoCapIndex   = 0;
        mVideoFrameCount = 0;

        open(filename);
    }

    VideoCaptureProxy::VideoCaptureProxy(int device)
    {
        mVideoCapIndex   = 0;
        mVideoFrameCount = 0;

        open(device);
    }

    VideoCaptureProxy::~VideoCaptureProxy()
    {
        release();
    }

    bool VideoCaptureProxy::open(const std::string& filename)
    {
        if (isOpened())
            release();

        mVideoFileName = filename;

        boost::filesystem::path videoFilePath(filename);
        if (0 == videoFilePath.extension().compare(std::string(".avi")))
        {
            VideoCapInfo VideoCapInfo;
            if (!CreateVideo(filename, VideoCapInfo))
            {
                DAVINCI_LOG_ERROR << "Cannot open AVI video file: " << filename;
                return false;
            }
            mVideoCaps.push_back(VideoCapInfo);

            boost::filesystem::path videoFileName(mVideoFileName);
            boost::filesystem::path videoParentPath = videoFileName.parent_path();
            boost::filesystem::path videoName = videoFileName.filename().stem();
            for (int i = 1;; i++)
            {
                std::string nextVideoFileName = videoName.string() + "_" + std::to_string(i) + ".avi";
                boost::filesystem::path nextVideoFilePath = videoParentPath;
                nextVideoFilePath /= nextVideoFileName;
                if (!CreateVideo(nextVideoFilePath.string(), VideoCapInfo))
                    break;
                mVideoCaps.push_back(VideoCapInfo);
            }
        }
        else
        {
            VideoCapInfo VideoCapInfo;
            VideoCapInfo.mFrameNum = 0;
            VideoCapInfo.mFrameBeg = 0;
            VideoCapInfo.mFrameEnd = 0;
            VideoCapInfo.mVideoCap = boost::shared_ptr<VideoCapture>(new VideoCaptureAdapter(filename));
            if (VideoCapInfo.mVideoCap->isOpened())
                mVideoCaps.push_back(VideoCapInfo);
        }

         return isOpened();
    }

    bool VideoCaptureProxy::open(int device)
    {
        if (isOpened())
            release();

        VideoCapInfo VideoCapInfo;
        VideoCapInfo.mVideoCap = boost::shared_ptr<VideoCapture>(new VideoCaptureAdapter(device));
        VideoCapInfo.mFrameNum = 0;
        VideoCapInfo.mFrameBeg = 0;
        VideoCapInfo.mFrameEnd = 0;
        mVideoCaps.push_back(VideoCapInfo);
        return isOpened();
    }

    bool VideoCaptureProxy::isOpened() const
    {
        if (mVideoCaps.size() == 0)
            return false;

        if (!mVideoCaps[mVideoCapIndex].mVideoCap->isOpened())
            return false;
        return true;
    }

    void VideoCaptureProxy::release()
    {
        for (uint32_t i = 0; i < mVideoCaps.size(); i++)
        {
            mVideoCaps[i].mVideoCap->release();
        }
        mVideoCaps.clear();
    }

    bool VideoCaptureProxy::grab()
    {
        if (!isOpened())
            return false;

        bool ret = mVideoCaps[mVideoCapIndex].mVideoCap->grab();
        double nextFrame = get(CV_CAP_PROP_POS_FRAMES) + 1;
        set(CV_CAP_PROP_POS_FRAMES, nextFrame);
        return ret;
    }

    VideoCaptureProxy& VideoCaptureProxy::operator >>(Mat& image)
    {
        if (isOpened())
            mVideoCaps[mVideoCapIndex].mVideoCap->operator>>(image);
        return *this;
    }

    bool VideoCaptureProxy::retrieve(CV_OUT cv::Mat& image, int channel)
    {
        if (!isOpened())
            return false;
        else
            return mVideoCaps[mVideoCapIndex].mVideoCap->retrieve(image, channel);
    }

    bool VideoCaptureProxy::read(CV_OUT cv::Mat& image)
    {
        return cv::VideoCapture::read(image);
    }

    bool VideoCaptureProxy::set(int propId, double value)
    {
        if (!isOpened())
            return false;

        if (propId == CV_CAP_PROP_POS_FRAMES)
        {
            // If the value exceeds max frame count of all videos, we use last video capture to handle it.
            for (mVideoCapIndex = 0; mVideoCapIndex < mVideoCaps.size() - 1; mVideoCapIndex++)
            {
                double frames = mVideoCaps[mVideoCapIndex].mVideoCap->get(CV_CAP_PROP_FRAME_COUNT);
                // value is frame index starts from 0
                if (value >= frames)
                    value = value - frames;
                else
                    break;
            }
        }
        else if (propId == CV_CAP_PROP_POS_MSEC)
        {
            double fps = mVideoCaps[mVideoCapIndex].mVideoCap->get(CV_CAP_PROP_FPS);

            // If the value exceeds max frame count of all videos, we use last video capture to handle it.
            for (mVideoCapIndex = 0; mVideoCapIndex < mVideoCaps.size() - 1; mVideoCapIndex++)
            {
                double frames = mVideoCaps[mVideoCapIndex].mVideoCap->get(CV_CAP_PROP_FRAME_COUNT);
                double period = 1000.0 / fps * frames;
                if (value > period)
                    value = value - period;
                else
                    break;
            }
        }
        else if (propId == CV_CAP_PROP_POS_AVI_RATIO)
        {
            double theFrame = get(CV_CAP_PROP_FRAME_COUNT) * value;
            // Convert to frame index property
            return set(CV_CAP_PROP_POS_FRAMES, theFrame);
        }

        return mVideoCaps[mVideoCapIndex].mVideoCap->set(propId, value);
    }

    double VideoCaptureProxy::get(int propId)
    {
        if (!isOpened())
            return 0.0;

        if (propId == CV_CAP_PROP_FRAME_COUNT)
        {
            double ret = 0.0;
            for (uint32_t i = 0; i < mVideoCaps.size(); i++)
            {
                ret += mVideoCaps[i].mVideoCap->get(CV_CAP_PROP_FRAME_COUNT);
            }
            return ret;
        }
        else if (propId == CV_CAP_PROP_POS_FRAMES)
        {
            double readFrames = 0.0;
            for (uint32_t i = 0; i < mVideoCapIndex; i++)
                readFrames += mVideoCaps[i].mVideoCap->get(CV_CAP_PROP_FRAME_COUNT);
            double currentFrame = mVideoCaps[mVideoCapIndex].mVideoCap->get(propId);
            return readFrames + currentFrame;
        }
        else if (propId == CV_CAP_PROP_POS_MSEC)
        {
            double fps = get(CV_CAP_PROP_FPS);
            double readFrames = get(CV_CAP_PROP_POS_FRAMES);
            return readFrames * 1000.0 / fps;
        }
        else if (propId == CV_CAP_PROP_POS_AVI_RATIO)
        {
            double frameCount = get(CV_CAP_PROP_FRAME_COUNT);
            double currentFrame = get(CV_CAP_PROP_POS_FRAMES);
            return (currentFrame + 1) / frameCount;
        }
        else
        {
            return mVideoCaps[mVideoCapIndex].mVideoCap->get(propId);
        }
    }

    double VideoCaptureProxy::getPropertyStep(int propId)
    {
        if (!isOpened())
            return -1.0;
        return mVideoCaps[mVideoCapIndex].mVideoCap->getPropertyStep(propId);
    }

    double VideoCaptureProxy::getPropertyMax(int propId)
    {
        if (!isOpened())
            return -1.0;
        return mVideoCaps[mVideoCapIndex].mVideoCap->getPropertyMax(propId);
    }

    double VideoCaptureProxy::getPropertyMin(int propId)
    {
        if (!isOpened())
            return -1.0;
        return mVideoCaps[mVideoCapIndex].mVideoCap->getPropertyMin(propId);
    }

    bool VideoCaptureProxy::CreateVideo(const std::string filename, VideoCapInfo &VideoCapInfo)
    {
        boost::filesystem::path videoFilePath(filename);
        if (boost::filesystem::exists(videoFilePath))
        {
            boost::shared_ptr<VideoCapture> aviCap(new VideoCaptureAdapter(filename));
            if (!aviCap->open(filename))
            {
                return false;
            }
            else
            {
                VideoCapInfo.mVideoCap = aviCap;
                double frameCount = aviCap->get(CV_CAP_PROP_FRAME_COUNT);
                uint64_t aviFrameCount = boost::numeric_cast<uint64_t, double>(frameCount);
                VideoCapInfo.mFrameNum = aviFrameCount;
                VideoCapInfo.mFrameBeg = mVideoFrameCount;
                VideoCapInfo.mFrameEnd = mVideoFrameCount + aviFrameCount - 1;
                mVideoFrameCount += aviFrameCount;
                return true;
            }
        }

        return false;
    }
}

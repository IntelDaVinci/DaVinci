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

#include "VideoWriterProxy.hpp"

#include <boost/filesystem.hpp>

#include "DaVinciCommon.hpp"

namespace DaVinci
{
    VideoWriterProxy::VideoWriterProxy()
    {
        mVideoFileIndex = 1;
        mFOURCC         = -1;
        mFPS            = 0.0;
        mIsColor        = true;
        mVideoFileIndex = 1;
        mInitialized    = false;
    }

    VideoWriterProxy::VideoWriterProxy(const std::string& filename, int fourcc, double fps, cv::Size frameSize, bool isColor)
        : mInitialized(false), mVideoWriter(filename, fourcc, fps, frameSize, isColor)
    {
        mFileName       = filename;
        mFOURCC         = fourcc;
        mFPS            = fps;
        mFrameSize      = frameSize;
        mIsColor        = isColor;
        mVideoFileIndex = 1;
    }

    VideoWriterProxy::~VideoWriterProxy()
    {
        release();
    }

    bool VideoWriterProxy::open(const std::string& filename, int fourcc, double fps, cv::Size frameSize, bool isColor)
    {
        if (mFileName.empty())
            mFileName  = filename;

        mFOURCC    = fourcc;
        mFPS       = fps;
        mFrameSize = frameSize;
        mIsColor   = isColor;

        // Remove exists video files at first time
        if (!mInitialized)
        {
            boost::system::error_code ec;
            boost::filesystem::path videoFileName(mFileName);

            boost::filesystem::remove(videoFileName, ec);

            for (int i = 1; ; i++)
            {
                boost::filesystem::path nextVideoFilePath = FormatVideoFileName(i);
                if (boost::filesystem::exists(nextVideoFilePath))
                {
                    boost::filesystem::remove(nextVideoFilePath, ec);
                }
                else
                {
                    break;
                }
            }

            mInitialized = true;
        }

        return mVideoWriter.open(filename, mFOURCC, mFPS, mFrameSize, mIsColor);
    }

    bool VideoWriterProxy::isOpened() const
    {
        return mVideoWriter.isOpened();
    }

    void VideoWriterProxy::release()
    {
        mVideoWriter.release();
    }

    VideoWriterProxy& VideoWriterProxy::operator << (const cv::Mat& image)
    {
        mVideoWriter << image;
        return *this;
    }

    void VideoWriterProxy::write(const cv::Mat& image)
    {
        // AVI_MAX_SIZE is equal to (4G - 64M). 64M is 4096 * 4096 * 4(RGBA)
        const uint64 AVI_MAX_SIZE = (uint64(2) << 31) - (uint64(2) << 25);
        if (getVideoSize() >= AVI_MAX_SIZE)
        {
            release();

            boost::filesystem::path nextVideoFilePath = FormatVideoFileName(mVideoFileIndex++);
            if (!open(nextVideoFilePath.string(), mFOURCC, mFPS, mFrameSize, mIsColor))
            {
                DAVINCI_LOG_ERROR << "Cannot create AVI video file";
                return;
            }
        }

        mVideoWriter.write(image);
    }

    uint64 VideoWriterProxy::getVideoSize()
    {
        return mVideoWriter.getVideoSize();
    }

    boost::filesystem::path VideoWriterProxy::FormatVideoFileName(int videoFileIndex)
    {
        boost::filesystem::path videoFileName(mFileName);
        boost::filesystem::path videoParentPath = videoFileName.parent_path();
        boost::filesystem::path videoName = videoFileName.filename().stem();

        std::string nextVideoFileName = videoName.string() + "_" + std::to_string(videoFileIndex) + ".avi";
        boost::filesystem::path nextVideoFilePath = videoParentPath;
        nextVideoFilePath /= nextVideoFileName;
        return nextVideoFilePath;
    }
}

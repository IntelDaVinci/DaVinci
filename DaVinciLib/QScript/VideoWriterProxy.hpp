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

#ifndef __VIDEO_WRITER_PROXY_HPP__
#define __VIDEO_WRITER_PROXY_HPP__

#include "opencv2/highgui/highgui.hpp"
#include <boost/filesystem.hpp>
#include <string>

namespace DaVinci
{
    class VideoWriterProxy : public cv::VideoWriter
    {
    public:
        VideoWriterProxy(void);
        VideoWriterProxy(const std::string& filename, int fourcc, double fps, cv::Size frameSize, bool isColor=true);

        virtual ~VideoWriterProxy(void);

        virtual bool open(const std::string& filename, int fourcc, double fps, cv::Size frameSize, bool isColor=true);
        virtual bool isOpened() const;
        virtual void release();
        virtual VideoWriterProxy& operator << (const cv::Mat& image);
        virtual void write(const cv::Mat& image);
        virtual uint64 getVideoSize();

    private:
        boost::filesystem::path FormatVideoFileName(int videoFileIndex);

    private:
        cv::VideoWriter mVideoWriter;
        std::string     mFileName;
        int             mFOURCC;
        double          mFPS;
        cv::Size        mFrameSize;
        bool            mIsColor;
        int             mVideoFileIndex;
        bool            mInitialized;
    };
}

#endif


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

#ifndef __VIDEO_CAPTURE_PROXY_HPP__
#define __VIDEO_CAPTURE_PROXY_HPP__

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "opencv2/highgui/highgui.hpp"

namespace DaVinci
{
    typedef struct {
        boost::shared_ptr<cv::VideoCapture>  mVideoCap;
        uint64_t                             mFrameNum;
        uint64_t                             mFrameBeg;
        uint64_t                             mFrameEnd;
    } VideoCapInfo;

    class VideoCaptureProxy : public cv::VideoCapture
    {
    public:
        VideoCaptureProxy();
        VideoCaptureProxy(const std::string& filename);
        VideoCaptureProxy(int device);

        virtual ~VideoCaptureProxy();
        virtual bool open(const std::string& filename);
        virtual bool open(int device);
        virtual bool isOpened() const;
        virtual void release();

        virtual bool grab();
        virtual bool retrieve(CV_OUT cv::Mat& image, int channel=0);
        virtual VideoCaptureProxy& operator >> (CV_OUT cv::Mat& image);
        virtual bool read(CV_OUT cv::Mat& image);

        virtual bool set(int propId, double value);
        virtual double get(int propId);
        virtual double getPropertyStep(int propId);
        virtual double getPropertyMax(int propId);
        virtual double getPropertyMin(int propId);

    private:
        bool CreateVideo(const std::string filename, VideoCapInfo &VideoCapInfo);

    private:
        std::vector<VideoCapInfo> mVideoCaps;
        std::string               mVideoFileName;
        uint32_t                  mVideoCapIndex;
        uint64_t                  mVideoFrameCount;
    };
}

#endif


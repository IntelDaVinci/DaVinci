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

#include "FlickeringChecker.hpp"
#include <codecvt>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>   // for strings
#include <sstream>  // string to number conversion

#include <opencv2/imgproc/imgproc.hpp>  // Gaussian Blur
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O

using namespace std;
using namespace cv;

namespace DaVinci
{
    FlickeringChecker::FlickeringChecker()
    {
    }
    FlickeringChecker::~FlickeringChecker()
    {
    }

    void FlickeringChecker::SaveFlickingVideoClips(const string &videoFile, int startIndex, int endIndex, Size frmSize, string saveVideoPath)
    {
        VideoCaptureProxy capture;
        cv::Mat curFrame;
        double frameRate = 30.0;
        VideoWriter vWriter;
        string videoPath = "";
        videoPath = saveVideoPath + "/" + "FlickingVideos";
        boost::filesystem::path saveVideo(videoPath);
        if (!boost::filesystem::is_directory(saveVideo))
        {
            boost::filesystem::create_directory(saveVideo);
        }
        boost::filesystem::path orgVideoPath(videoFile);
        saveVideo /= orgVideoPath.stem().string() + "_" + boost::lexical_cast<string>(startIndex) + "_" + boost::lexical_cast<string>(endIndex) + ".avi";
        string blankVideo = saveVideo.string();
        capture.open(videoFile);
        try
        {
            vWriter.open(blankVideo, CV_FOURCC('M','J','P','G'), frameRate, frmSize);
            for (double i = startIndex; i<=endIndex; i++)
            {
                capture.set(CV_CAP_PROP_POS_FRAMES, i);
                capture.read(curFrame);
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
            DAVINCI_LOG_ERROR << "Cut video clips exception!";
        }
    }

    /// <summary>
    /// it tests the similarity of two input videos first with PSNR, and for the frames below a PSNR trigger value, also with MSSIM.
    /// </summary>
    /// <param name="videoname">videoname</param>
    /// <returns>true for enough</returns>    
    DaVinciStatus FlickeringChecker::VideoQualityEvaluation(const string videoName)
    {
        double psnrTriggerValue = 30.0; //PSNR Threshold
        double psnrV = .0;
        //_currentFrame is for original frame, currentFrame is for resized frame
        cv::Mat _currentFrame, currentFrame;
        cv::Mat previousFrame, tempReferenceFrame;
        int tempNumbers=0, tempReferenceFrameNumber=0;

        int width = 0, height = 0;
        VideoCaptureProxy capture;
        bool noDiff = true;
        bool hasFlickering = false;

        string folder="";
        if(TestReport::currentQsLogPath == "")
        {
            boost::filesystem::path filePath(videoName);
            filePath = system_complete(filePath);
            folder = filePath.parent_path().string();
        }
        else
        {
            folder = TestReport::currentQsLogPath;
        }

        try
        {
            capture.open(videoName);
            if (!capture.isOpened())
            {
                DAVINCI_LOG_DEBUG << "The source capture didn't opened." ;
                return DaVinciStatus(errc::bad_file_descriptor);
            }

            double maxCaptureFrame = capture.get(CV_CAP_PROP_FRAME_COUNT);
            for (double i = 0; i < maxCaptureFrame; i++)
            {
                capture.set(CV_CAP_PROP_POS_FRAMES, i);
                capture.read(_currentFrame);

                if (_currentFrame.empty())
                {
                    continue;
                }

                width = _currentFrame.size().width;
                height = _currentFrame.size().height;
                Size _frmSize(width, height);

                //Resize to 1/16.
                Size frmSize(width/4, height/4);
                resize(_currentFrame, currentFrame, frmSize);

                if(!previousFrame.empty())
                {
                    psnrV = GetPSNR(previousFrame, currentFrame);
                    if (psnrV < psnrTriggerValue)
                    {                        
                        //Big changes, re start to check.
                        if(psnrV<8)
                        {
                            tempNumbers = 0;
                            noDiff = true;
                            previousFrame = currentFrame.clone();
                            continue;
                        }

                        if(!noDiff  && !tempReferenceFrame.empty())
                        {
                            psnrV = GetPSNR(tempReferenceFrame, currentFrame);
                            if (psnrV > psnrTriggerValue)
                            {
                                //Catch Flicking, print Error
                                DAVINCI_LOG_WARNING << "Found Flickering Frame from: " << tempReferenceFrameNumber << " to " << int(i);
                                hasFlickering = true;
                                SaveFlickingVideoClips(videoName, tempReferenceFrameNumber, int(i), _frmSize, folder);

                                // re start to check.
                                tempNumbers = 0;
                                noDiff = true;
                                previousFrame = currentFrame.clone();
                                continue;
                            }
                            else
                            {
                                //Big changes with reference, it says that frame changed, re start to check.
                                tempNumbers = 0;
                                noDiff = true;
                                previousFrame = currentFrame.clone();
                                continue;
                            }
                        }

                        tempReferenceFrame = previousFrame.clone();
                        noDiff = false;
                        tempReferenceFrameNumber = int(i) - 1;
                    }
                    else
                    {
                        if(!noDiff)
                        {
                            tempNumbers++;

                            if(tempNumbers > 3)
                            {
                                tempNumbers = 0;
                                noDiff = true;
                            }
                        }
                    }
                }
                previousFrame = currentFrame.clone();
            }

            capture.release();
        }
        catch (Exception e)
        {
            DAVINCI_LOG_ERROR << e.msg;
        }

        if(hasFlickering)
            return DaVinciStatusSuccess; //Have Flickering
        else
            return DaVinciStatus(errc::not_supported); //No Flickering
    }

    double FlickeringChecker::GetPSNR(const Mat& I1, const Mat& I2)
    {
        if (!DeviceManager::Instance().GpuAccelerationEnabled())
        {
            Mat s1;
            absdiff(I1, I2, s1);       // |I1 - I2|
            s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
            s1 = s1.mul(s1);           // |I1 - I2|^2

            Scalar s = sum(s1);         // sum elements per channel

            double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

            if( sse <= 1e-10) // for small values return zero
                return 0;
            else
            {
                double  mse =sse /(double)(I1.channels() * I1.total());
                double psnr = 10.0*log10((255*255)/mse);
                return psnr;
            }
        }
        else
        {
            BufferPSNR bufferPSNR;
            return GetPSNRGPUOptimized(I1, I2, bufferPSNR);
        }
    }

    double FlickeringChecker::GetPSNRGPUOptimized(const Mat& I1, const Mat& I2, BufferPSNR& b)
    {
        b.gI1.upload(I1);
        b.gI2.upload(I2);

        b.gI1.convertTo(b.t1, CV_32F);
        b.gI2.convertTo(b.t2, CV_32F);

        gpu::absdiff(b.t1.reshape(1), b.t2.reshape(1), b.gs);
        gpu::multiply(b.gs, b.gs, b.gs);

        double sse = gpu::sum(b.gs, b.buf)[0];

        if( sse <= 1e-10) // for small values return zero
            return 0;
        else
        {
            double mse = sse /(double)(I1.channels() * I1.total());
            double psnr = 10.0*log10((255*255)/mse);
            return psnr;
        }
    }
}
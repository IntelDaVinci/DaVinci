#ifndef __LAUNCHGENERATOR__
#define __LAUNCHGENERATOR__

#include "TestInterface.hpp"
#include "FeatureMatcher.hpp"
#include "VideoWriterProxy.hpp"

#include "opencv/cv.h"
#include <string>
#include <vector>
#include <cmath>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <DaVinciCommon.hpp>

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/


namespace DaVinci
{
    class LaunchGenerator : public TestInterface
    {
    private:
        double startTime;
        int timeout;
        std::string tempFilePath;
        std::string tmpVideoFile;
        VideoWriterProxy videoWriter;
        Size videoFrameSize;
        std::string launchActivity;
        std::string packageName;
        
        void generateIni(const cv::Mat frame);
        

    public:
        LaunchGenerator(const std::string &packageActivity, const int seconds);

        virtual bool Init() override;

        virtual CaptureDevice::Preset CameraPreset() override;

        virtual void Destroy() override;        

        double GetElapsed(double timeStamp);

        virtual cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

        virtual void extractImage(const string filePath);
       
        

    };
}


#endif	//#ifndef __LAUNCHGENERATOR__

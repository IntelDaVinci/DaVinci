#include "LaunchGenerator.hpp"
#include "DeviceManager.hpp"
#include "QScript/VideoCaptureProxy.hpp"
#include "boost/filesystem.hpp"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include "opencv2/opencv.hpp"


#include <fstream>

using namespace cv;

namespace DaVinci
{
    LaunchGenerator::LaunchGenerator(const std::string &packageActivity, const int seconds)
    {
        startTime = 0;
        timeout = seconds;
        basic_string <char>::size_type idx = packageActivity.find('/');
        if(idx == std::string::npos)
        {
            DAVINCI_LOG_WARNING << "Invalid package/Activity name";
            SetFinished();
        }
        else
        {
            packageName = packageActivity.substr(0, (int)idx);
            launchActivity =   packageActivity.substr((int)idx+1);
        }
    }

    bool LaunchGenerator::Init()
    {
        SetName("LaunchGenerator");
        boost::shared_ptr<TargetDevice> device=(DeviceManager::Instance()).GetCurrentTargetDevice();

        if(device == nullptr || device->GetDeviceStatus() == DeviceConnectionStatus::Offline)
        {
            DAVINCI_LOG_ERROR << "Error: no device connected";
            SetFinished();
            return false;
        }

        tempFilePath = TestManager::Instance().GetDaVinciResourcePath("Temp");

        if(!boost::filesystem::exists(boost::filesystem::path(tempFilePath))){
            boost::filesystem::create_directory(boost::filesystem::path(tempFilePath));
        }

        boost::filesystem::path tmpVideoFilePath(tempFilePath + "/" + packageName + ".avi");
        tmpVideoFile = boost::filesystem::system_complete(tmpVideoFilePath).string();

        device->StopActivity(packageName);
        device->StartActivity(packageName + "/" + launchActivity);
        startTime = GetCurrentMillisecond();

        return true;
    }

    CaptureDevice::Preset LaunchGenerator::CameraPreset()
    {
        return CaptureDevice::Preset::HighResolution;
    }

    void LaunchGenerator::Destroy()
    {
        if (videoWriter.isOpened())
        {
            videoWriter.release();
        }

        try
        {
            boost::filesystem::remove(tmpVideoFile);
        }
        catch(boost::filesystem::filesystem_error e)
        {
            DAVINCI_LOG_WARNING << e.what();
        }
    }   

    double LaunchGenerator::GetElapsed(double timeStamp)
    {
        return (double)(timeStamp - startTime) / 1000.0;
    }

    cv::Mat LaunchGenerator::ProcessFrame(const cv::Mat &frame, const double timeStamp)
    {
        double elapsed = GetElapsed(timeStamp);
        //drop the frame that before test group start.
        if(FSmaller(elapsed, 0.0))
        {
            return frame;
        }

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

            videoWriter.write(frameResize);
        }else{      
            extractImage(tmpVideoFile);
            SetFinished();
        }
        return frame;
    }

    void LaunchGenerator::extractImage(const string filePath)
    {
        VideoCaptureProxy cap(filePath);    

        double frameCount = cap.get(CV_CAP_PROP_FRAME_COUNT);

        double index = frameCount-120;

        cap.set(CV_CAP_PROP_POS_FRAMES, index);

        BackgroundSubtractorMOG2 backgrounSubstractor =   BackgroundSubtractorMOG2(40, 4);

        cv::Mat frame;
        cv::Mat background;

        while(index < frameCount)
        {
            cap.read(frame);
            backgrounSubstractor(frame, background);   
            index += 1;
            cap.set(CV_CAP_PROP_POS_FRAMES, index);
        }

        cvtColor(frame, frame, CV_RGB2GRAY);        

        cv::Mat launch = frame-background;

        generateIni(launch);

    }

    void LaunchGenerator::generateIni(const cv::Mat frame)
    {
        string iniFolder = tempFilePath + "\\" + packageName;
        boost::filesystem::create_directory(iniFolder);
        string iniFile = iniFolder + "\\" + packageName + ".ini";
        imwrite(iniFolder + "\\" + packageName + ".png", frame);
        std::ofstream  ofs(iniFile);
        ofs << packageName << "\n";
        ofs << launchActivity << "\n";
        ofs << packageName + ".png" << "\n";
        ofs <<  "TRUE\n";
        ofs << timeout << "\n";
    }
}

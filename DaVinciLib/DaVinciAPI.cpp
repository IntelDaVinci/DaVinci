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

#include <string>

#include "opencv2/opencv.hpp"
#include "boost/filesystem.hpp"

#include "DaVinciAPI.h"
#include "TestManager.hpp"
#include "DeviceManager.hpp"
#include "ScriptRecorder.hpp"
#include "ScriptReplayer.hpp"
#include "CameraCalibrate.hpp"
#include "IndexReport.hpp"
#include "AudioManager.hpp"
#include "TempleRunSVMPlay.hpp"
#include "QScript/VideoCaptureProxy.hpp"
#include "MainTest.hpp"

using namespace DaVinci;
using namespace std;

int DAVINCI_API Init(const char *daVinciHome, int argc, const char *argv[])
{
    vector<string> args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
    return TestManager::Instance().Init(daVinciHome, args).value();
}

int DAVINCI_API Shutdown()
{
    return TestManager::Instance().Shutdown().value();
}

void DAVINCI_API SetMessageEventHandler(MessageEventHandler handler)
{
    TestManager::Instance().SetMessageEventHandler(handler);
}

void DAVINCI_API SetImageEventHandler(ImageEventHandler handler)
{
    TestManager::Instance().SetImageEventHandler(handler);
}

void DAVINCI_API SetDrawDotHandler(DrawDotEventHandler drawHandler)
{
    TestManager::Instance().SetDrawDotHandler(drawHandler);
}

void DAVINCI_API SetDrawLinesHandler(DrawLinesEventHandler drawHandler)
{
    TestManager::Instance().SetDrawLinesHandler(drawHandler);
}

ImageEventHandler qScriptImageEventHandler = nullptr;
void DAVINCI_API SetScriptVideoImgHandler(ImageEventHandler handler)
{
    qScriptImageEventHandler = handler;
}

ImageInfo DAVINCI_API RotateImage(ImageInfo *frame, int rotateDegree)
{
    cv::Mat *befRotateFrame;
    cv::Mat rotatedFrame;
    ImageInfo img = *frame;

    if (frame->mat != nullptr)
    {
        befRotateFrame = (cv::Mat *)frame->mat;
    }
    else
    {
        return img;
    }

    switch (rotateDegree)
    {
    case 90:
        rotatedFrame = RotateImage90(*befRotateFrame);
        break;
    case 180:
        rotatedFrame = RotateImage180(*befRotateFrame);
        break;
    case 270:
        rotatedFrame = RotateImage270(*befRotateFrame);
        break;
    default:
        rotatedFrame = *befRotateFrame;
        break;
    }
    img.iplImage = new IplImage(rotatedFrame);
    img.mat = new cv::Mat(rotatedFrame);
    return img;
}

void DAVINCI_API ReleaseImage(const ImageInfo *imageInfo)
{
    if (imageInfo->iplImage != nullptr)
    {
        delete (IplImage *)imageInfo->iplImage;
    }
    if (imageInfo->mat != nullptr)
    {
        delete (cv::Mat *)imageInfo->mat;
    }
}

int DAVINCI_API SaveConfigureData()
{
    return DeviceManager::Instance().SaveConfig(TestManager::Instance().GetConfigureFile()).value();
}

BoolType DAVINCI_API SaveSnapshot(const char *fileName)
{
    if (TestManager::Instance().Snapshot(fileName) == DaVinciStatusSuccess)
    {
        return BoolTrue;
    }
    else
    {
        return BoolFalse;
    }
}

void DAVINCI_API SetDeviceOrder(const char *curDeviceName)
{
    DeviceManager::Instance().UpdateDeviceOrder(curDeviceName);
}

int DAVINCI_API StartTest(const char *testName)
{
    string testNameStr(testName);
    if (testNameStr.empty())
    {
        return TestManager::Instance().StartCurrentTest().value();
    }
    else
    {
        boost::shared_ptr<TestGroup> builtinTest = TestManager::Instance().GetBuiltinTestByName(testNameStr);
        if (builtinTest != nullptr)
        {
            return TestManager::Instance().StartTest(builtinTest).value();
        }
        else
        {
            return static_cast<int>(errc::invalid_argument);
        }
    }
}

int DAVINCI_API StopTest()
{
    boost::shared_ptr<TestGroup> testGroup = TestManager::Instance().GetTestGroup();
    if (testGroup != nullptr)
    {
        testGroup->SetFinished();
    }
    return 0;
}

void DAVINCI_API SetTestStatusEventHandler(TestStatusEventHandler handler)
{
    TestManager::Instance().SetTestStatusEventHandler(handler);
}

int DAVINCI_API SetTestProjectConfig(const TestProjectConfig *config)
{
    return TestManager::Instance().SetTestProjectConfigData(config).value();
}

void DAVINCI_API SetRecordWithID(int id)
{
    TestManager::Instance().SetRecordWithID(id);
}

void* DAVINCI_API GetTestProjectConfig()
{
    TestProjectConfig * tstProjConfig = new TestProjectConfig();
    TestProjectConfigData tstConfigData = TestManager::Instance().GetTestProjectConfigData();
    static string strActivity = "";
    static string strApkName = "";
    static string strPackageName = "";
    static string strBaseFolder = "";
    static string strSource = "";
    static string strTarget = "";
    static string strName = "";

    strActivity = tstConfigData.activityName;
    tstProjConfig->ActivityName = strActivity.c_str();

    strApkName = tstConfigData.apkName;
    tstProjConfig->Application = strApkName.c_str();

    strPackageName = tstConfigData.packageName;
    tstProjConfig->PackageName = strPackageName.c_str();

    strBaseFolder = tstConfigData.baseFolder;
    tstProjConfig->BaseFolder = strBaseFolder.c_str();

    strSource = tstConfigData.pushDataSource;
    tstProjConfig->PushDataSource = strSource.c_str();

    strTarget = tstConfigData.pushDataTarget;
    tstProjConfig->PushDataTarget = strTarget.c_str();

    strName = tstConfigData.name;
    tstProjConfig->Name = strName.c_str();

    return tstProjConfig;
}

int DAVINCI_API ReplayQScript(const char *scriptFile)
{
    string scriptName(scriptFile);
    boost::shared_ptr<TestInterface> replayer;
    if (boost::iends_with(scriptName, ".qs"))
    {
        replayer = boost::shared_ptr<TestInterface>(new ScriptReplayer(scriptFile));
    }
    else if (boost::iends_with(scriptName, ".xml"))
    {
        replayer = boost::shared_ptr<TestInterface>(new MainTest(scriptFile));
    }
    if (replayer == nullptr)
    {
        return DaVinciStatus(errc::invalid_argument).value();
    }
    return TestManager::Instance().StartTest(replayer).value();
}

int DAVINCI_API RecordQScript(const char *scriptFile)
{
    boost::shared_ptr<TestInterface> recorder = boost::shared_ptr<TestInterface>(new ScriptRecorder(scriptFile));
    return TestManager::Instance().StartTest(recorder).value();
}

void* DAVINCI_API OpenVideo(const char *videoFile, int *frmCount)
{
    if (!boost::filesystem::exists(videoFile))
    {
        DAVINCI_LOG_INFO << string("Cannot find the video file: ") << videoFile;
        return nullptr;
    }
    VideoCaptureProxy *vidCap = new VideoCaptureProxy(videoFile);
    if (vidCap != nullptr)
    {
        if (!vidCap->isOpened())
        {
            DAVINCI_LOG_ERROR << string("Cannot open the video file: ") << videoFile;
            *frmCount = 0;
        }
        else
        {
            DAVINCI_LOG_INFO << string("Opened video file is: ") << videoFile;
            *frmCount = (int)vidCap->get(CV_CAP_PROP_FRAME_COUNT);
        }
    }
    return vidCap;
}

void DAVINCI_API ShowVideoFrame(void* video, int frameIndex)
{
    if (video != nullptr)
    {
        VideoCaptureProxy *videoInstance = (VideoCaptureProxy*)video;
        if (videoInstance->isOpened())
        {
            videoInstance->set(CV_CAP_PROP_POS_FRAMES, (double)frameIndex);
            if(videoInstance->grab())
            {
                Mat image;
                videoInstance->read(image);
                Size imageSize = image.size();

                if (image.empty() || imageSize.width == 0 || imageSize.height == 0)
                {
                    DAVINCI_LOG_ERROR << "Null frame captured!" << endl;
                }
                // call the function in QScriptEditor Window to show the image
                if (qScriptImageEventHandler != nullptr)
                {
                    ImageInfo imageInfo;
                    imageInfo.iplImage = new IplImage(image);
                    imageInfo.mat = new cv::Mat(image);
                    qScriptImageEventHandler(&imageInfo);
                }
            }
        }
    }
}

void DAVINCI_API CloseVideo(void* video)
{
    if (video != nullptr)
    {
        ((VideoCaptureProxy*)video)->release();
        delete video;
    }
}

BoolType DAVINCI_API GetGpuSupport()
{
    if( DeviceManager::Instance().GpuAccelerationEnabled())
    {
        return BoolTrue;
    }
    else
    {
        return BoolFalse;
    }
}

int DAVINCI_API GetTargetDeviceHeight(const char *devName)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetTargetDevice(devName);
    return targetDevice->GetDeviceHeight();
}

int DAVINCI_API GetTargetDeviceWidth(const char *devName)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetTargetDevice(devName);
    return targetDevice->GetDeviceWidth();
}

void DAVINCI_API GetCurrentDevice(char *name)
{
    string curName = DeviceManager::Instance().GetCurrentDeviceName();
    strncpy_s(name, strlen(curName.c_str()) + 1, curName.c_str(), strlen(curName.c_str()));
}

int DAVINCI_API EnableDisableGpuSupport(BoolType enable)
{
    if (enable)
    {
        if (DeviceManager::Instance().HasCudaSupport())
        {
            DeviceManager::Instance().EnableGpuAcceleration(true);
            return 0;
        }
        else
        {
            return static_cast<int>(errc::not_supported);
        }
    }
    else
    {
        DeviceManager::Instance().EnableGpuAcceleration(false);
        return 0;
    }
}

BoolType DAVINCI_API SetCaptureResolution(const char * resName)
{
    CaptureDevice::Preset preset = CaptureDevice::Preset::PresetDontCare;
    if (boost::algorithm::iequals(resName, "HighResolution"))
    {
        preset = CaptureDevice::Preset::HighResolution;
    }
    else if (boost::algorithm::iequals(resName, "HighSpeed"))
    {
        preset = CaptureDevice::Preset::HighSpeed;
    }
    else if (boost::algorithm::iequals(resName, "AIResolution"))
    {
        preset = CaptureDevice::Preset::AIResolution;
    }
    else
    {
        DAVINCI_LOG_ERROR << "Unsupported capture resolution: " << resName;
        return BoolFalse;
    }

    boost::shared_ptr<CaptureDevice> cameraCap = DeviceManager::Instance().GetCurrentCaptureDevice();
    if (cameraCap != nullptr)
    {
        if (cameraCap->InitCamera(preset))
            return BoolTrue;
    }

    return BoolFalse;
}

BoolType DAVINCI_API SetCurrentCaptureDevice(const char * captureName)
{
    if (DeviceManager::Instance().InitCaptureDevice(captureName) == DaVinciStatusSuccess)
        return BoolTrue;
    return BoolFalse;
}

BoolType DAVINCI_API SetCaptureDeviceForTargetDevice(const char * deviceName, const char * captureName)
{
    if (DeviceManager::Instance().SetCaptureDeviceForTargetDevice(deviceName, captureName) == DaVinciStatusSuccess)
        return BoolTrue;
    return BoolFalse;
}

BoolType DAVINCI_API SetHWAccessoryControllerForTargetDevice(const char * deviceName, const char * comName)
{
    if (DeviceManager::Instance().SetHWAccessoryControllerForTargetDevice(deviceName, comName) == DaVinciStatusSuccess)
        return BoolTrue;
    return BoolFalse;
}

BoolType DAVINCI_API SetCurrentTargetDevice(const char * deviceName)
{
    if (DeviceManager::Instance().InitTestDevices(deviceName) == DaVinciStatusSuccess)
        return BoolTrue;
    return BoolFalse;
}
BoolType DAVINCI_API SetCurrentHWAccessoryController(const char * comName)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
    if (targetDevice == nullptr)
        return BoolFalse;
    targetDevice->SetHWAccessoryController(comName);
    if (DeviceManager::Instance().InitHWAccessoryControllers(comName) == DaVinciStatusSuccess)
        return BoolTrue;
    return BoolFalse;
}

BoolType DAVINCI_API GetCaptureDeviceForTargetDevice(const char * deviceName, char * captureName, int maxSizeInBytes)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
    if (targetDevice == nullptr)
        return BoolFalse;
    strncpy_s(captureName, maxSizeInBytes, ScreenSourceToString(targetDevice->GetScreenSource()).c_str(), maxSizeInBytes);
    return BoolTrue;
}

BoolType DAVINCI_API GetHWAccessoryControllerForTargetDevice(const char * deviceName, char * comName, int maxSizeInBytes)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
    if (targetDevice == nullptr)
        return BoolFalse;
    strncpy_s(comName, maxSizeInBytes, targetDevice->GetHWAccessoryController().c_str(), maxSizeInBytes);
    return BoolTrue;
}

BoolType DAVINCI_API GetCurrentTargetDevice(char * deviceName, int maxSizeInBytes)
{
    boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
    if (targetDevice == nullptr)
        return BoolFalse;
    strncpy_s(deviceName, maxSizeInBytes, targetDevice->GetDeviceName().c_str(), maxSizeInBytes);
    return BoolTrue;
}

int DAVINCI_API GetTargetDeviceNames(char * deviceNames[], int maxLen, int arraySize)
{
    vector<string> targetDeviceNames = DeviceManager::Instance().GetTargetDeviceNames();
    if (deviceNames == NULL || targetDeviceNames.size() > (unsigned int)arraySize)
    {
        return (int)targetDeviceNames.size();
    }

    int count = 0;
    for (auto targetDevice = targetDeviceNames.begin(); targetDevice != targetDeviceNames.end(); targetDevice++)
    {
        const char * str = (*targetDevice).data();
        int length = (int)(strlen(str) + 1);
        if (length > maxLen)
            length = maxLen;
        memcpy(deviceNames[count], str + '\0', length);
        count++;
    }
    return count;
}

int DAVINCI_API GetCaptureDeviceNames(char * deviceNames[], int maxLen, int arraySize)
{
    vector<string> captureDeviceNames = DeviceManager::Instance().GetCaptureDeviceNames();
    if (deviceNames == NULL || captureDeviceNames.size() > (unsigned int)arraySize)
    {
        return (int)captureDeviceNames.size();
    }

    int count = 0;
    for (auto captureDevice = captureDeviceNames.begin(); captureDevice != captureDeviceNames.end(); captureDevice++)
    {
        const char * str = (*captureDevice).data();
        int length = (int)(strlen(str) + 1);
        if (length > maxLen)
            length = maxLen;
        memcpy(deviceNames[count], str + '\0', length);
        count++;
    }
    return count;
}

int DAVINCI_API GetHWAccessoryControllerNames(char * deviceNames[], int maxLen, int arraySize)
{
    vector<string> ffrdDeviceNames = DeviceManager::Instance().GetHWAccessoryControllerNames();
    if (deviceNames == NULL || ffrdDeviceNames.size() > (unsigned int)arraySize)
    {
        return (int)ffrdDeviceNames.size();
    }

    int count = 0;
    for (auto ffrdDevice = ffrdDeviceNames.begin(); ffrdDevice != ffrdDeviceNames.end(); ffrdDevice++)
    {
        const char * str = (*ffrdDevice).data();
        int length = (int)(strlen(str) + 1);
        if (length > maxLen)
            length = maxLen;
        memcpy(deviceNames[count], str + '\0', length);
        count++;
    }
    return count;
}

int DAVINCI_API RefreshTargetDevice()
{
    DeviceManager &devManager = DeviceManager::Instance();
    return devManager.DetectDevices().value();
}

char* DAVINCI_API GetSpecifiedDeviceStatus(const char *devName)
{
    boost::shared_ptr<TargetDevice> device = DeviceManager::Instance().GetTargetDevice(devName);

    if (device != nullptr)
    {
        if (device->GetDeviceStatus() == DeviceConnectionStatus::Connected)
        {
            return "Connected";
        }
        else if (device->GetDeviceStatus() == DeviceConnectionStatus::ConnectedByOthers)
        {
            return "ConnectedByOthers";
        }
        else if (device->GetDeviceStatus() == DeviceConnectionStatus::ReadyForConnection)
        {
            return "ReadyForConnection";
        }
        else
        {
            // TODO: may define another status for unknown condition
            return "Offline";
        }
    }
    else
    {
        return "Offline";
    }
}

int DAVINCI_API Connect(const char *deviceName)
{
    auto selectedDevice = DeviceManager::Instance().GetTargetDevice(deviceName);

    DeviceManager::Instance().CheckAndUpdateGlobalStatus(DeviceCategory::Target, selectedDevice->GetDeviceName(), true);
    return selectedDevice->Connect().value();
}

int DAVINCI_API Disconnect(const char *deviceName)
{
    auto selectedDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
    
    selectedDevice->Disconnect();
    DeviceManager::Instance().CheckAndUpdateGlobalStatus(DeviceCategory::Target, selectedDevice->GetDeviceName(), false);
    return DaVinciStatusSuccess;
}

BoolType DAVINCI_API IsDeviceConnected(const char* devName)
{
    auto curDevice = DeviceManager::Instance().GetTargetDevice(devName);
    DeviceConnectionStatus connectStatus = curDevice->GetDeviceStatus();
    // Note: caused by bool type convertion alignment issue between C# and C++ not alignment,
    // return type would use "int" to represent "bool" type, 1 for true and 0 for false.
    // Either return value or parameters, both of them will be updated to int if they are bool originally.
    if (connectStatus == DeviceConnectionStatus::Connected)
    {
        return BoolTrue;
    }
    else
    {
        return BoolFalse;
    }
}

int DAVINCI_API GetPackageActivity(char *appsInfo[], const char *appLocation)
{
    vector<string> pkgsActivity = AndroidTargetDevice::GetPackageActivity(appLocation);
    if (appsInfo == NULL)
    {
        return (int)pkgsActivity.size();
    }
    int count = 0;
    for (auto pkgActivity = pkgsActivity.begin(); pkgActivity != pkgsActivity.end(); pkgActivity++)
    {
        const char *str = (*pkgActivity).data();
        int length = (int)(strlen(str) + 1);
        memcpy(appsInfo[count], str + '\0', length);
        count++;
    }
    return count;
}

int DAVINCI_API OnTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2)
{
    return TestManager::Instance().OnTilt(action, degree1, degree2, speed1, speed2).value();
}

int DAVINCI_API OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed)
{
    return TestManager::Instance().OnZRotate(zAxisAction, degree, speed).value();
}

int DAVINCI_API OnAppAction(AppLifeCycleAction action, char *apkInfo1, char *apkInfo2)
{
    string apkInfoPara1(apkInfo1 == nullptr? "": apkInfo1);
    string apkInfoPara2(apkInfo2 == nullptr? "": apkInfo2);
    return TestManager::Instance().OnAppAction(action, apkInfoPara1, apkInfoPara2).value();
}

// positive up, negative down
int DAVINCI_API OnMoveHolder(int distance)
{
    return TestManager::Instance().OnMoveHolder(distance).value();
}

int DAVINCI_API OnPowerButtonPusher(PowerButtonPusherAction action)
{
    return TestManager::Instance().OnPowerButtonPusher(action).value();
}

int DAVINCI_API OnUsbSwitch(UsbSwitchAction action)
{
    return TestManager::Instance().OnUsbSwitch(action).value();
}

int DAVINCI_API OnEarphonePuller(EarphonePullerAction action)
{
    return TestManager::Instance().OnEarphonePuller(action).value();
}

int DAVINCI_API OnRelayController(RelayControllerAction action)
{
    return TestManager::Instance().OnRelayController(action).value();
}

int DAVINCI_API OnButtonAction(ButtonAction action, ButtonActionType mode)
{
    return TestManager::Instance().OnButtonAction(action, mode).value();
}

int DAVINCI_API OnMouseAction(const ImageBoxInfo *imageBoxInfo, const MouseAction *action)
{
    return TestManager::Instance().OnMouseAction(imageBoxInfo, action).value();
}

int DAVINCI_API OnSetRotateImageBoxInfo(ImageBoxInfo *imageBoxInfoPara)
{
    return TestManager::Instance().SetRotateImageBoxInfo(imageBoxInfoPara).value();
}

int DAVINCI_API OnSetText(const char *text)
{
    return TestManager::Instance().OnSetText(text).value();
}

int DAVINCI_API OnOcr()
{
    return TestManager::Instance().OnOcr().value();
}

TestConfiguration DAVINCI_API GetTestConfiguration()
{
    return TestManager::Instance().GetTestConfiguration();
}

int DAVINCI_API Calibrate(int calibrateType)
{
    boost::shared_ptr<TestInterface> calibrateTest = boost::shared_ptr<CameraCalibrate>(new CameraCalibrate(static_cast<CameraCalibrationType>(calibrateType)));
    return TestManager::Instance().StartTest(calibrateTest).value();
}

TestStatus DAVINCI_API GetTestStatus()
{
    if (TestManager::Instance().CheckTestState())
    {
        return TestStatusStopped;
    }
    else
    {
        return TestStatusStarted;
    }
}

int DAVINCI_API OnAudioRecord(BoolType startRecord)
{
    if (startRecord == BoolFalse)
    {
        return TestManager::Instance().OnAudioRecord(false).value();
    }
    else
    {
        return TestManager::Instance().OnAudioRecord(true).value();
    }
}

int DAVINCI_API GenerateReportIndex(char* qs_path)
{
    boost::shared_ptr<IndexReport> generateIndexReport = boost::shared_ptr<IndexReport>(new IndexReport(string(qs_path)));
    return generateIndexReport->GenerateReportIndex().value();
}


Bounding DAVINCI_API GetImageBounding(ImageInfo *frame, const char* modelFileName)
{
    cv::Mat image;
    Bounding b;
    b.X = 0;
    b.Y = 0;
    b.Width = 0;
    b.Height = 0;
    if (frame->mat != nullptr)
    {
        image = *(cv::Mat *)frame->mat;
        cv::Mat modelImg = cv::imread(modelFileName);
        auto imageMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
        imageMatcher->SetModelImage(image);
        auto modelMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
        modelMatcher->SetModelImage(modelImg);

        if(!modelMatcher->Match(*imageMatcher).empty())
        {
            cv::Rect rect = modelMatcher->GetBoundingRect();
            if(rect.x < 0)
            {
                b.X = 0;
            }
            else 
            {
                b.X = rect.x;
            }

            if(rect.y < 0)
            {
                b.Y = 0;
            }
            else
            {
                b.Y = rect.y;
            }

            if(rect.width + b.X > image.cols)
            {
                b.Width = image.cols-b.X;
            }
            else
            {
                b.Width = rect.width;
            }

            if(rect.height + b.Y > image.rows)
            {
                b.Height = image.rows-b.Y;
            }
            else
            {
                b.Height = rect.height;
            }

            return b;
        }
    }
    return b;
}

BoolType DAVINCI_API IsROIMatch(ImageInfo *frame, Bounding bounding, const char* objFileName)
{
    cv::Mat image;

    if (frame->mat != nullptr)
    {
        image = *(cv::Mat *)frame->mat;
        cv::Rect rect(bounding.X, bounding.Y, bounding.Width, bounding.Height);
        cv::Mat modelImg = image(rect);
        auto imageMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
        imageMatcher->SetModelImage(image);
        auto modelMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
        modelMatcher->SetModelImage(modelImg);
        if(!modelMatcher->Match(*imageMatcher).empty())
        {
            cv::imwrite(objFileName, modelImg);
            return BoolTrue;
        }
    }
    return BoolFalse;
}

ImageInfo DAVINCI_API CopyImageInfo(ImageInfo *frame)
{
    ImageInfo img = *frame;
    if (frame->mat != nullptr)
    {
        img.iplImage = new IplImage(*(cv::Mat *)frame->mat);
        img.mat = new cv::Mat(*(cv::Mat *)frame->mat);
    }
    return img;
}


int DAVINCI_API GetAudioRecordDeviceNames(char * deviceNames[], int length, int arraySize)
{
    vector<string> waveInDeviceNames = AudioManager::Instance().GetWaveInDeviceNames();
    if (deviceNames != nullptr && arraySize >= 0)
    {
        for (int i = 0; i < arraySize; i++)
        {
            if (i < static_cast<int>(waveInDeviceNames.size()))
            {
#ifdef WIN32
                strncpy_s(deviceNames[i], length + 1, &(waveInDeviceNames[i])[0], length);
#else
                strncpy(deviceNames[i], &(waveInDeviceNames[i])[0], length);
                deviceNames[i][length] = '\0';
#endif
            }
            else
            {
                break;
            }
        }
    }
    return static_cast<int>(waveInDeviceNames.size());
}

int DAVINCI_API GetAudioPlayDeviceNames(char * deviceNames[], int length, int arraySize)
{
    vector<string> waveOutDeviceNames = AudioManager::Instance().GetWaveOutDeviceNames();
    if (deviceNames != nullptr && arraySize >= 0)
    {
        for (int i = 0; i < arraySize; i++)
        {
            if (i < static_cast<int>(waveOutDeviceNames.size()))
            {
#ifdef WIN32
                strncpy_s(deviceNames[i], length + 1, &(waveOutDeviceNames[i])[0], length);
#else
                strncpy(deviceNames[i], &(waveOutDeviceNames[i])[0], length);
                deviceNames[i][length] = '\0';
#endif
            }
            else
            {
                break;
            }
        }
    }
    return static_cast<int>(waveOutDeviceNames.size());
}

int DAVINCI_API SetAudioDevice(const char * deviceName, AudioDeviceType deviceType, const char * audioDeviceName)
{
    DaVinciStatus status(errc::invalid_argument);
    switch (deviceType)
    {
    case AudioDeviceTypeRecordFromDevice:
        status = DeviceManager::Instance().SetRecordFromDevice(deviceName, audioDeviceName);
        break;
    case AudioDeviceTypePlayToDevice:
        status = DeviceManager::Instance().SetPlayToDevice(deviceName, audioDeviceName);
        break;
    case AudioDeviceTypePlayDeviceAudio:
        status = DeviceManager::Instance().SetAudioPlayDevice(audioDeviceName);
        break;
    case AudioDeviceTypeRecordFromUser:
        status = DeviceManager::Instance().SetAudioRecordDevice(audioDeviceName);
        break;
    default:
        break;
    }
    return status.value();
}

int DAVINCI_API GetAudioDevice(const char * deviceName, AudioDeviceType deviceType, char * audioDeviceName, int length)
{
    string audioDevice;
    switch (deviceType)
    {
    case AudioDeviceTypeRecordFromDevice:
        {
            auto targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
            if (targetDevice != nullptr)
            {
                audioDevice = targetDevice->GetAudioRecordDevice();
            }
        }
        break;
    case AudioDeviceTypePlayToDevice:
        {
            auto targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
            if (targetDevice != nullptr)
            {
                audioDevice = targetDevice->GetAudioPlayDevice();
            }
        }
        break;
    case AudioDeviceTypePlayDeviceAudio:
        audioDevice = DeviceManager::Instance().GetAudioPlayDevice();
        break;
    case AudioDeviceTypeRecordFromUser:
        audioDevice = DeviceManager::Instance().GetAudioRecordDevice();
        break;
    default:
        break;
    }
    if (!audioDevice.empty())
    {
#ifdef WIN32
        strncpy_s(audioDeviceName, length + 1, &audioDevice[0], length);
#else
        strncpy(audioDeviceName, &audioDevice[0], length);
        audioDeviceName[length] = '\0';
#endif
        return DaVinciStatusSuccess;
    }
    else
    {
        return errc::invalid_argument;
    }
}

void DAVINCI_API AddNewTargetDevice(const char *deviceName)
{
    vector<boost::shared_ptr<TargetDevice>>& processDevice = DeviceManager::Instance().GetTargetDeviceList();
    boost::shared_ptr<AndroidTargetDevice> addDevice = boost::shared_ptr<AndroidTargetDevice>(new AndroidTargetDevice(deviceName));
    processDevice.push_back(addDevice);
}

void DAVINCI_API DeleteTargetDevice(const char *deviceName)
{
    int index = 0;
    vector<boost::shared_ptr<TargetDevice>>& processDevice = DeviceManager::Instance().GetTargetDeviceList();
    for (auto device= processDevice.begin(); device != processDevice.end(); device++)
    {
        string tempStrName = device->get()->GetDeviceName();
        const char *tempName = tempStrName.c_str();
        if (strcmp(deviceName, tempName) == 0)
        {
            processDevice.erase(processDevice.begin() + index);
            break;
        }
        index++;
    }
}

BoolType DAVINCI_API SetDeviceCurrentMultiLayerMode(const char *deviceName, bool enable)
{
    auto targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
    if (targetDevice == nullptr)
    {
        return BoolFalse;
    }
    targetDevice->SetDefaultMultiLayerMode(enable);
    return BoolTrue;
}

BoolType DAVINCI_API GetDeviceCurrentMultiLayerMode(const char *deviceName)
{
    auto targetDevice = DeviceManager::Instance().GetTargetDevice(deviceName);
    if ((targetDevice != nullptr) && (targetDevice->GetDefaultMultiLayerMode()))
    {
        return BoolTrue;
    }
    else
    {
        return BoolFalse;
    }
}

void DAVINCI_API RunDiagnosticCommand(const char *command)
{
    string runcommands(command);

    if (TestManager::Instance().systemDiagnostic != nullptr)
    {
        TestManager::Instance().systemDiagnostic->ClearErrorCount();
        TestManager::Instance().systemDiagnostic->LoadCheckList();

        if (runcommands.find("CheckHost") != -1)
        {
            TestManager::Instance().systemDiagnostic->CheckHostInfo();
            TestManager::Instance().systemDiagnostic->RunHostCheckList();
        }

        if (runcommands.find("CheckDevice") != -1)
        {
            TestManager::Instance().systemDiagnostic->CheckDeviceInfo();
            TestManager::Instance().systemDiagnostic->RunDeviceCheckList();
        }

/*        if (runcommands.find("StopAll") != -1)
        {
            TestManager::Instance().systemDiagnostic->StopAllCheckList();
        }
*/
    }
}

BoolType DAVINCI_API GetDiagnosticResult()
{
    if (TestManager::Instance().systemDiagnostic != nullptr)
    {
        if (TestManager::Instance().systemDiagnostic->GetResult() == CheckerResult::Fail)
            return BoolFalse;
        else
            return BoolTrue;
    }
    return BoolTrue;
}

int DAVINCI_API SVMPlayNoTilt()
{
    DaVinciStatus status;
    auto targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
    if (targetDevice == nullptr)
    {
        status = errc::no_such_device;
        return status.value();
    }
    string inifilename = TestManager::Instance().GetDaVinciResourcePath("examples/templerun/TempleRunHOGSVM.ini");
    if (!boost::filesystem::is_regular_file(inifilename))
    {
        DAVINCI_LOG_ERROR << string("ini file doesn't exist: ") << inifilename;
        return errc::no_such_file_or_directory;
    }
    boost::shared_ptr<TestInterface> svmplaynotilt = boost::shared_ptr<TestInterface>(new TempleRunSVMPlay(inifilename));
    return TestManager::Instance().StartTest(svmplaynotilt).value();
}

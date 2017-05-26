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

#ifndef __DAVINCI_API_H__
#define __DAVINCI_API_H__

#include "DaVinciDefs.h"
#if defined(__cplusplus)
extern "C"
{
#endif

    /////////////////////////////////////////////////
    /// Common definitions
    /////////////////////////////////////////////////
    typedef enum BoolType
    {
        BoolFalse,
        BoolTrue,
    }BoolType;

    typedef enum TargetDeviceType
    {
        TargetDeviceTypeAndroid,
        TargetDeviceTypeWindows,
        TargetDeviceTypeWindowsMobile,
        TargetDeviceTypeChrome,
        TargetDeviceTypeiOS,
    } TargetDeviceType;

    typedef enum Orientation
    {
        OrientationPortrait,
        OrientationLandscape,
        OrientationReversePortrait,
        OrientationReverseLandscape
    } DeviceOrientation;

    /////////////////////////////////////////////////
    /// DaVinci System APIs
    /////////////////////////////////////////////////

    typedef struct ImageInfo
    {
        void *iplImage;
        void *mat;
    } ImageInfo;

    typedef struct ActionDotInfo
    {
        float coorX;
        float coorY;
        DeviceOrientation orientation;
    }ActionDotInfo;

    typedef void (DAVINCI_API *MessageEventHandler)(const char *line);

    typedef void (DAVINCI_API *ImageEventHandler)(const ImageInfo *imageInfo);

    typedef void (DAVINCI_API *DrawDotEventHandler)(const ActionDotInfo *actDotInfo);

    typedef void (DAVINCI_API *DrawLinesEventHandler)(const ActionDotInfo *dot1, const ActionDotInfo *dot2, int frameWidth, int frameHeight);

    DAVINCI_EXPORT int DAVINCI_API Init(const char *daVinciHome, int argc, const char *argv[]);

    DAVINCI_EXPORT int DAVINCI_API Shutdown();

    DAVINCI_EXPORT void DAVINCI_API SetMessageEventHandler(MessageEventHandler handler);

    DAVINCI_EXPORT void DAVINCI_API SetImageEventHandler(ImageEventHandler handler);

    DAVINCI_EXPORT void DAVINCI_API SetScriptVideoImgHandler(ImageEventHandler handler);

    DAVINCI_EXPORT void DAVINCI_API SetDrawDotHandler(DrawDotEventHandler drawHandler);

    DAVINCI_EXPORT void DAVINCI_API SetDrawLinesHandler(DrawLinesEventHandler drawHandler);

    DAVINCI_EXPORT ImageInfo DAVINCI_API RotateImage(ImageInfo *frame, int rotateDegree);

    DAVINCI_EXPORT void DAVINCI_API ReleaseImage(const ImageInfo *imageInfo);

    DAVINCI_EXPORT int DAVINCI_API SaveConfigureData();

    DAVINCI_EXPORT BoolType DAVINCI_API SaveSnapshot(const char* fileName);

    DAVINCI_EXPORT void DAVINCI_API SetDeviceOrder(const char *curDeviceName);

    /////////////////////////////////////////////////
    /// Test control APIs
    /////////////////////////////////////////////////

    typedef struct TestConfiguration
    {
        int offlineConcurrentMatch; // a boolean to control whether we are doing offline concurrent matching
        int timeout; // the timeout (second) allowed for a test run, 0: means infinite
        BoolType suppressImageBox; // True if we do not show image box
        BoolType checkflicker;// a boolean to control whether we are doing flickering check
    } TestConfiguration;

    typedef enum TestStatus
    {
        TestStatusStarted,
        TestStatusStopped
    } TestStatus;


    typedef void (DAVINCI_API *TestStatusEventHandler)(TestStatus status);

    // start a builtin test or if testName is null, start the pre-configured test
    DAVINCI_EXPORT int DAVINCI_API StartTest(const char *testName);

    DAVINCI_EXPORT int DAVINCI_API StopTest();

    DAVINCI_EXPORT void DAVINCI_API SetTestStatusEventHandler(TestStatusEventHandler handler);

    DAVINCI_EXPORT TestConfiguration DAVINCI_API GetTestConfiguration();

    DAVINCI_EXPORT int DAVINCI_API Calibrate(int calibrateType);

    DAVINCI_EXPORT TestStatus DAVINCI_API GetTestStatus();
    /////////////////////////////////////////////////
    /// Q Script APIs
    /////////////////////////////////////////////////
    typedef struct TestProjectConfig
    {
        TargetDeviceType deviceType;
        const char *Name;
        const char *BaseFolder;
        const char *Application;
        const char *PackageName;
        const char *ActivityName;
        const char *PushDataSource;
        const char *PushDataTarget;
    } TestProjectConfig;

    DAVINCI_EXPORT int DAVINCI_API SetTestProjectConfig(const TestProjectConfig *config);

    DAVINCI_EXPORT void* DAVINCI_API GetTestProjectConfig();

    DAVINCI_EXPORT int DAVINCI_API ReplayQScript(const char *scriptFile);

    DAVINCI_EXPORT int DAVINCI_API RecordQScript(const char *scriptFile);

    /////////////////////////////////////////////////
    /// Device control APIs
    /////////////////////////////////////////////////

    typedef enum TargetDeviceConnStatus
    {
        TargetDeviceConnStatusConnected,
        TargetDeviceConnStatusConnectedByOthers,
        TargetDeviceConnStatusReadyToConnect,
        TargetDeviceConnStatusOffline
    } TargetDeviceConnStatus;

    typedef struct TargetDeviceStatus
    {
        const char *name;
        TargetDeviceType deviceType;
        TargetDeviceConnStatus connStatus;
    } TargetDeviceStatus;

    typedef enum AudioDeviceType
    {
        AudioDeviceTypePlayDeviceAudio,
        AudioDeviceTypeRecordFromUser,
        AudioDeviceTypePlayToDevice,
        AudioDeviceTypeRecordFromDevice
    } AudioDeviceType;


    typedef void (DAVINCI_API *TargetDeviceStatusEventHandler)(const TargetDeviceStatus *status);

    DAVINCI_EXPORT int DAVINCI_API EnableDisableGpuSupport(BoolType enable);

    DAVINCI_EXPORT char* DAVINCI_API GetSpecifiedDeviceStatus(const char *devName);

    DAVINCI_EXPORT int DAVINCI_API  RefreshTargetDevice();

    DAVINCI_EXPORT int DAVINCI_API GetPackageActivity(char *appInfo[], const char *appLocation);

    DAVINCI_EXPORT int DAVINCI_API Connect(const char *deviceName);

    DAVINCI_EXPORT int DAVINCI_API Disconnect(const char *deviceName);

    DAVINCI_EXPORT BoolType DAVINCI_API IsDeviceConnected(const char *devName);

    DAVINCI_EXPORT void DAVINCI_API SetRecordWithID(int record);

    DAVINCI_EXPORT BoolType DAVINCI_API GetGpuSupport();

    DAVINCI_EXPORT int DAVINCI_API GetTargetDeviceHeight(const char *devName);

    DAVINCI_EXPORT int DAVINCI_API GetTargetDeviceWidth(const char *devName);

    DAVINCI_EXPORT void DAVINCI_API GetCurrentDevice(char *curDeviceName);

    DAVINCI_EXPORT BoolType DAVINCI_API SetCaptureResolution(const char * resName);
    DAVINCI_EXPORT BoolType DAVINCI_API SetCurrentCaptureDevice(const char * captureName);
    DAVINCI_EXPORT BoolType DAVINCI_API SetCurrentTargetDevice(const char * deviceName);
    DAVINCI_EXPORT BoolType DAVINCI_API SetCurrentHWAccessoryController(const char * comName);
    DAVINCI_EXPORT BoolType DAVINCI_API SetCaptureDeviceForTargetDevice(const char * deviceName, const char * captureName);
    DAVINCI_EXPORT BoolType DAVINCI_API SetHWAccessoryControllerForTargetDevice(const char * deviceName, const char * comName);
    DAVINCI_EXPORT int DAVINCI_API SetAudioDevice(const char * deviceName, AudioDeviceType deviceType, const char * audioDeviceName);
    DAVINCI_EXPORT int DAVINCI_API GetAudioDevice(const char * deviceName, AudioDeviceType deviceType, char * audioDeviceName, int length);
    DAVINCI_EXPORT BoolType DAVINCI_API SetDeviceCurrentMultiLayerMode(const char *deviceName, bool enable);

    DAVINCI_EXPORT BoolType DAVINCI_API GetCurrentTargetDevice(char * deviceName, int maxSizeInBytes);
    DAVINCI_EXPORT BoolType DAVINCI_API GetCaptureDeviceForTargetDevice(const char * deviceName, char * captureName, int maxSizeInBytes);
    DAVINCI_EXPORT BoolType DAVINCI_API GetHWAccessoryControllerForTargetDevice(const char * deviceName, char * comName, int maxSizeInBytes);
    DAVINCI_EXPORT BoolType DAVINCI_API GetDeviceCurrentMultiLayerMode(const char *deviceName);

    DAVINCI_EXPORT int DAVINCI_API GetTargetDeviceNames(char * deviceNames[], int length, int arraySize);
    DAVINCI_EXPORT int DAVINCI_API GetCaptureDeviceNames(char * deviceNames[], int length, int arraySize);
    DAVINCI_EXPORT int DAVINCI_API GetHWAccessoryControllerNames(char * deviceNames[], int length, int arraySize);
    DAVINCI_EXPORT int DAVINCI_API GetAudioRecordDeviceNames(char * deviceNames[], int length, int arraySize);
    DAVINCI_EXPORT int DAVINCI_API GetAudioPlayDeviceNames(char * deviceNames[], int length, int arraySize);

    DAVINCI_EXPORT void DAVINCI_API AddNewTargetDevice(const char *deviceName);
    DAVINCI_EXPORT void DAVINCI_API DeleteTargetDevice(const char *deviceName);

    /////////////////////////////////////////////////
    /// GUI event handling
    /////////////////////////////////////////////////

    typedef enum TiltAction
    {
        TiltActionReset,
        TiltActionUp,
        TiltActionDown,
        TiltActionLeft,
        TiltActionRight,
        TiltActionUpLeft,
        TiltActionUpRight,
        TiltActionDownLeft,
        TiltActionDownRight,
        TiltActionTo
    } TiltAction;

    typedef enum ZAXISTiltAction
    {
        ZAXISTiltActionReset,
        ZAXISTiltActionClockwise,
        ZAXISTiltActionAnticlockwise,
        ZAXISTiltActionTo
    } ZAXISTiltAction;

    typedef enum ZAxisRotationDirection
    {
        Clockwise,
        Anticlockwise
    } ZAxisRotationDirection;

    typedef enum PowerButtonPusherAction
    {
        PowerButtonPusherPress,
        PowerButtonPusherRelease,
        PowerButtonPusherShortPress,
        PowerButtonPusherLongPress
    }PowerButtonPusherAction;

    typedef enum UsbSwitchAction
    {
        UsbOnePlugIn,
        UsbOnePullOut,
        UsbTwoPlugIn,
        UsbTwoPullOut
    }UsbSwitchAction;

    typedef enum EarphonePullerAction
    {
        EarphonePullerPlugIn,
        EarphonePullerPullOut
    }EarphonePullerAction;

    typedef enum RelayControllerAction
    {
        RelayOneConnect,
        RelayOneDisconnect,
        RelayTwoConnect,
        RelayTwoDisconnect,
        RelayThreeConnect,
        RelayThreeDisconnect,
        RelayFourConnect,
        RelayFourDisconnect
    }RelayControllerAction;

    typedef enum AppLifeCycleAction
    {
        AppActionUninstall,
        AppActionInstall,
        AppActionPushData,
        AppActionClearData,
        AppActionStart,
        AppActionStop
    } AppLifeCycleAction;

    typedef enum SwipeAction
    {
        SwipeActionUp,
        SwipeActionDown,
        SwipeActionLeft,
        SwipeActionRight,
    } SwipeAction;

    typedef enum ButtonAction
    {
        ButtonActionBack,
        ButtonActionMenu,
        ButtonActionHome,
        ButtonActionLightUp,
        ButtonActionVolumeUp,
        ButtonActionVolumeDown,
        ButtonActionPower,
        ButtonActionBrightnessUp,
        ButtonActionBrightnessDown,
        ButtonActionDumpUiLayout
    } ButtonAction;

    typedef enum ButtonActionType
    {
        Up,
        Down,
        DownAndUp,
    } ButtonActionType;

    typedef struct ImageBoxInfo
    {
        int width;
        int height;
        DeviceOrientation orientation;
    } ImageBoxInfo;

    typedef enum MouseButton
    {
        MouseButtonLeft,
        MouseButtonRight,
        MouseButtonMiddle
    } MouseButton;

    typedef enum MouseActionType
    {
        MouseActionTypeDown,
        MouseActionTypeUp,
        MouseActionTypeMove,
        MouseActionTypeWheel
    } MouseActionType;

    typedef struct MouseAction
    {
        MouseActionType actionType;
        MouseButton button;
        int ptr;
        int x;
        int y;
        int delta; // wheel
        int clicks;
    } MouseAction;

    DAVINCI_EXPORT int DAVINCI_API OnTilt(TiltAction action, int degree1, int degree2, int speed1 = 0, int speed2 = 0);

    DAVINCI_EXPORT int DAVINCI_API OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed = 2);

    DAVINCI_EXPORT int DAVINCI_API OnAppAction(AppLifeCycleAction action, char *info1, char *info2);

    // positive up, negative down
    DAVINCI_EXPORT int DAVINCI_API OnMoveHolder(int distance);

    DAVINCI_EXPORT int DAVINCI_API OnPowerButtonPusher(PowerButtonPusherAction action);

    DAVINCI_EXPORT int DAVINCI_API OnUsbSwitch(UsbSwitchAction action);

    DAVINCI_EXPORT int DAVINCI_API OnEarphonePuller(EarphonePullerAction action);

    DAVINCI_EXPORT int DAVINCI_API OnRelayController(RelayControllerAction action);

    DAVINCI_EXPORT int DAVINCI_API OnButtonAction(ButtonAction action, ButtonActionType mode = ButtonActionType::DownAndUp);

    DAVINCI_EXPORT int DAVINCI_API OnMouseAction(const ImageBoxInfo *imageBoxInfo, const MouseAction *action);

    DAVINCI_EXPORT int DAVINCI_API OnOcr();

    DAVINCI_EXPORT int DAVINCI_API OnSetText(const char *text);

    DAVINCI_EXPORT int DAVINCI_API OnSetRotateImageBoxInfo(ImageBoxInfo *imageBoxInfo);

    DAVINCI_EXPORT int DAVINCI_API OnAudioRecord(BoolType startRecord);

    DAVINCI_EXPORT int DAVINCI_API GenerateReportIndex(char* dirName);

    DAVINCI_EXPORT int DAVINCI_API SVMPlayNoTilt();

    typedef struct Bounding
    {
        int X;
        int Y;
        int Width;
        int Height;
    } Bounding;   


    DAVINCI_EXPORT Bounding DAVINCI_API GetImageBounding(ImageInfo *frame, const char* qsFileName);

    DAVINCI_EXPORT BoolType DAVINCI_API IsROIMatch(ImageInfo *frame, Bounding roi, const char* objFileName);

    DAVINCI_EXPORT ImageInfo DAVINCI_API CopyImageInfo(ImageInfo *frame);

    DAVINCI_EXPORT void DAVINCI_API RunDiagnosticCommand(const char *command);

    DAVINCI_EXPORT BoolType DAVINCI_API GetDiagnosticResult();

    DAVINCI_EXPORT void* DAVINCI_API OpenVideo(const char *fileName, int *frmCount);

    DAVINCI_EXPORT void DAVINCI_API ShowVideoFrame(void *video, int frmIndex);

    DAVINCI_EXPORT void DAVINCI_API CloseVideo(void *video);

#if defined(__cplusplus)
}
#endif

#endif
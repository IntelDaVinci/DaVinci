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

using System.Runtime.InteropServices;
using System.Text;

namespace DaVinci
{
    /// <summary>
    /// 
    /// </summary>
    public class DaVinciAPI
    {
#if DEBUG
        private const string dllName = "DaVinciLib_d.dll";
#else
        private const string dllName = "DaVinciLib.dll";
#endif
        /// <summary>
        /// 
        /// </summary>
        public enum BoolType
        {
            /// <summary>
            /// 
            /// </summary>
            BoolFalse,
            /// <summary>
            /// 
            /// </summary>
            BoolTrue,
        }
        /// <summary>
        /// 
        /// </summary>
        public enum TargetDeviceType
        {
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceTypeAndroid,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceTypeWindows,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceTypeWindowsMobile,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceTypeChrome,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceTypeiOS,
        }
        /// <summary>
        /// 
        /// </summary>
        public enum Orientation
        {
            /// <summary>
            /// 
            /// </summary>
            OrientationPortrait,
            /// <summary>
            /// 
            /// </summary>
            OrientationLandscape,
            /// <summary>
            /// 
            /// </summary>
            OrientationReversePortrait,
            /// <summary>
            /// 
            /// </summary>
            OrientationReverseLandscape,
        }

        /// Return Type: void
        ///line: char*
        public delegate void MessageEventHandler([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string line);

        /// <summary>
        /// 
        /// </summary>
        [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential)]
        public struct ImageInfo
        {

            /// void*
            public System.IntPtr iplImage;

            /// void*
            public System.IntPtr mat;
        }

        /// <summary>
        /// 
        /// </summary>
        [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential)]
        public struct ActionDotInfo
        {
            ///
            public float coorX;

            ///
            public float coorY;

            ///
            public Orientation orientation;
        }

        /// Return Type: void
        ///imageInfo: ImageInfo*
        public delegate void ImageEventHandler(ref ImageInfo imageInfo);

        /// Return Type: void
        ///dotInfo: ActionDotInfo* 
        public delegate void DrawDotEventHandler(ref ActionDotInfo actDotInfo);

        /// Return Type: void
        ///
        public delegate void DrawLinesEventHandler(ref ActionDotInfo dot1, ref ActionDotInfo dot2, int width, int height);

        /// <summary>
        /// 
        /// </summary>
        public enum TestStatus
        {
            /// <summary>
            /// 
            /// </summary>
            TestStatusStarted,
            /// <summary>
            /// 
            /// </summary>
            TestStatusStopped,
        }

        /// Return Type: void
        ///status: TestStatus
        public delegate void TestStatusEventHandler(TestStatus status);

        /// <summary>
        /// 
        /// </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct TestProjectConfig
        {

            /// TargetDeviceType
            public TargetDeviceType deviceType;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string Name;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string BaseFolder;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string Application;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string PackageName;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string ActivityName;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string PushDataSource;

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string PushDataTarget;
        }

        /// <summary>
        /// 
        /// </summary>
        public enum TargetDeviceConnStatus
        {
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceConnStatusConnected,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceConnStatusConnectedByOthers,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceConnStatusReadyToConnect,
            /// <summary>
            /// 
            /// </summary>
            TargetDeviceConnStatusOffline,
        }
        /// <summary>
        /// 
        /// </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct TargetDeviceStatus
        {

            /// char*
            [MarshalAsAttribute(UnmanagedType.LPStr)]
            public string name;

            /// TargetDeviceType
            public TargetDeviceType deviceType;

            /// TargetDeviceConnStatus
            public TargetDeviceConnStatus connStatus;
        }

        /// Return Type: void
        ///status: TargetDeviceStatus*
        public delegate void TargetDeviceStatusEventHandler(ref TargetDeviceStatus status);
        /// <summary>
        /// 
        /// </summary>
        public enum TiltAction
        {
            /// <summary>
            /// 
            /// </summary>
            TiltActionReset,
            /// <summary>
            /// 
            /// </summary>
            TiltActionUp,
            /// <summary>
            /// 
            /// </summary>
            TiltActionDown,
            /// <summary>
            /// 
            /// </summary>
            TiltActionLeft,
            /// <summary>
            /// 
            /// </summary>
            TiltActionRight,
            /// <summary>
            /// 
            /// </summary>
            TiltActionUpLeft,
            /// <summary>
            /// 
            /// </summary>
            TiltActionUpRight,
            /// <summary>
            /// 
            /// </summary>
            TiltActionDownLeft,
            /// <summary>
            /// 
            /// </summary>
            TiltActionDownRight,
            /// <summary>
            /// 
            /// </summary>
            TiltActionTo
        }
        /// <summary>
        /// 
        /// </summary>
        public enum ZAXISTiltAction
        {
            /// <summary>
            /// 
            /// </summary>
            ZAXISTiltActionReset,
            /// <summary>
            /// 
            /// </summary>
            ZAXISTiltActionClockwise,
            /// <summary>
            /// 
            /// </summary>
            ZAXISTiltActionAnticlockwise,        
            /// <summary>
            /// 
            /// </summary>
            ZAXISTiltActionTo
        }
        /// <summary>
        /// 
        /// </summary>
        public enum ZAxisRotationDirection
        {
            /// <summary>
            /// 
            /// </summary>
            Clockwise,
            /// <summary>
            /// 
            /// </summary>
            Anticlockwise
        }
        /// <summary>
        /// 
        /// </summary>
        public enum PowerButtonPusherAction
        {
            /// <summary>
            /// 
            /// </summary>
            PowerButtonPusherPress,
            /// <summary>
            /// 
            /// </summary>
            PowerButtonPusherRelease,
            /// <summary>
            /// 
            /// </summary>
            PowerButtonPusherShortPress,
            /// <summary>
            /// 
            /// </summary>
            PowerButtonPusherLongPress
        }
        /// <summary>
        /// 
        /// </summary>
        public enum UsbSwitchAction
        {
            /// <summary>
            /// 
            /// </summary>
            UsbOnePlugIn,
            /// <summary>
            /// 
            /// </summary>
            UsbOnePullOut,
            /// <summary>
            /// 
            /// </summary>
            UsbTwoPlugIn,
            /// <summary>
            /// 
            /// </summary>
            UsbTwoPullOut
        }
        /// <summary>
        /// 
        /// </summary>
        public enum EarphonePullerAction
        {
            /// <summary>
            /// 
            /// </summary>
            EarphonePullerPlugIn,
            /// <summary>
            /// 
            /// </summary>
            EarphonePullerPullOut
        }
        /// <summary>
        /// 
        /// </summary>
        public enum RelayControllerAction
        {
            /// <summary>
            /// 
            /// </summary>
            RelayOneConnect,
            /// <summary>
            /// 
            /// </summary>
            RelayOneDisconnect,
            /// <summary>
            /// 
            /// </summary>
            RelayTwoConnect,
            /// <summary>
            /// 
            /// </summary>
            RelayTwoDisconnect,
            /// <summary>
            /// 
            /// </summary>
            RelayThreeConnect,
            /// <summary>
            /// 
            /// </summary>
            RelayThreeDisconnect,
            /// <summary>
            /// 
            /// </summary>
            RelayFourConnect,
            /// <summary>
            /// 
            /// </summary>
            RelayFourDisconnect
        }
        /// <summary>
        /// 
        /// </summary>
        public enum AppLifeCycleAction
        {
            /// <summary>
            /// 
            /// </summary>
            AppActionUninstall,
            /// <summary>
            /// 
            /// </summary>
            AppActionInstall,
            /// <summary>
            /// 
            /// </summary>
            AppActionPushData,
            /// <summary>
            /// 
            /// </summary>
            AppActionClearData,
            /// <summary>
            /// 
            /// </summary>
            AppActionStart,
            /// <summary>
            /// 
            /// </summary>
            AppActionStop
        }
        /// <summary>
        /// 
        /// </summary>
        public enum SwipeAction
        {
            /// <summary>
            /// 
            /// </summary>
            SwipeActionUp,
            /// <summary>
            /// 
            /// </summary>
            SwipeActionDown,
            /// <summary>
            /// 
            /// </summary>
            SwipeActionLeft,
            /// <summary>
            /// 
            /// </summary>
            SwipeActionRight,
        }
        /// <summary>
        /// 
        /// </summary>
        public enum ButtonAction
        {
            /// <summary>
            /// 
            /// </summary>
            ButtonActionBack,
            /// <summary>
            /// 
            /// </summary>
            ButtonActionMenu,
            /// <summary>
            /// 
            /// </summary>
            ButtonActionHome,
            ///
            ButtonActionLightUp,
            ///
            ButtonActionVolumeUp,
            ///
            ButtonActionVolumeDown,
            ///
            ButtonActionPower,
            ///
            ButtonActionBrightnessUp,
            ///
            ButtonActionBrightnessDown,
            ///
            ButtonActionDumpUiLayout
        }

        /// <summary>
        /// 
        /// </summary>
        public enum ButtonActionType
        {
            /// <summary>
            /// 
            /// </summary>
            Up,
            /// <summary>
            /// 
            /// </summary>
            Down,
            /// <summary>
            /// 
            /// </summary>
            DownAndUp,
        }

        /// <summary>
        /// 
        /// </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct ImageBoxInfo
        {

            /// int
            public int width;

            /// int
            public int height;

            /// Orientation
            public Orientation orientation;
        }
        /// <summary>
        /// 
        /// </summary>
        public enum MouseButton
        {
            /// <summary>
            /// 
            /// </summary>
            MouseButtonLeft,
            /// <summary>
            /// 
            /// </summary>
            MouseButtonRight,
            /// <summary>
            /// 
            /// </summary>
            MouseButtonMiddle,
        }
        /// <summary>
        /// 
        /// </summary>
        public enum MouseActionType
        {
            /// <summary>
            /// 
            /// </summary>
            MouseActionTypeDown,
            /// <summary>
            /// 
            /// </summary>
            MouseActionTypeUp,
            /// <summary>
            /// 
            /// </summary>
            MouseActionTypeMove,
            /// <summary>
            /// 
            /// </summary>
            MouseActionTypeWheel,
        }
        /// <summary>
        /// 
        /// </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct MouseAction
        {

            /// MouseActionType
            public MouseActionType actionType;

            /// MouseButton
            public MouseButton button;

            /// int
            public int ptr;

            /// int
            public int x;

            /// int
            public int y;

            /// int
            public int delta;

            /// int
            public int clicks;
        }


        /// <summary>
        /// 
        /// </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct Bounding
        {

            /// <summary>
            /// 
            /// </summary>
            public int X;
            /// <summary>
            /// 
            /// </summary>
            public int Y;

            /// int
            public int Width;

            /// int
            public int Height;


        }

        /// <summary> Values that represent AudioDeviceType. </summary>
        public enum AudioDeviceType
        {
            /// <summary>
            /// 
            /// </summary>
            AudioDeviceTypePlayDeviceAudio,
            /// <summary>
            /// 
            /// </summary>
            AudioDeviceTypeRecordFromUser,
            /// <summary>
            /// 
            /// </summary>
            AudioDeviceTypePlayToDevice,
            /// <summary>
            /// 
            /// </summary>
            AudioDeviceTypeRecordFromDevice
        }


        /// <summary> A test configuration. </summary>
        [StructLayoutAttribute(LayoutKind.Sequential)]
        public struct TestConfiguration
        {
            /// <summary> a boolean to control whether we are doing offline concurrent matching </summary>
            public int offlineConcurrentMatch;
            /// <summary> the timeout (second) allowed for a test run, 0: means infinite </summary>
            public int timeout;
            /// <summary> True if we do not show image box </summary>
            public BoolType suppressImageBox;
            ///<summary> a boolean to control whether we are doing flickering check </summary>
            public BoolType checkflicker;
        }

        /// <summary>
        /// 
        /// </summary>
        public partial class NativeMethods
        {

            /// Return Type: int
            ///daVinciHome: char*
            ///argc: int
            ///argv: char**
            [DllImportAttribute(dllName, EntryPoint = "Init")]
            public static extern int Init([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string daVinciHome, int argc, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] argv);


            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "Shutdown")]
            public static extern int Shutdown();


            /// Return Type: void
            ///handler: MessageEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetMessageEventHandler")]
            public static extern void SetMessageEventHandler(MessageEventHandler handler);


            /// Return Type: void
            ///handler: ImageEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetScriptVideoImgHandler")]
            public static extern void SetScriptVideoImgHandler(ImageEventHandler handler);

            /// Return Type: void
            ///handler: ImageEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetImageEventHandler")]
            public static extern void SetImageEventHandler(ImageEventHandler handler);

            /// Return Type: void
            ///handler: DrawDotEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetDrawDotHandler")]
            public static extern void SetDrawDotHandler(DrawDotEventHandler drawHandler);

            /// Return Type: void
            ///handler: DrawLinesEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetDrawLinesHandler")]
            public static extern void SetDrawLinesHandler(DrawLinesEventHandler drawHandler);

            /// Return Type: void
            ///mat: ImageInfo*
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "ReleaseImage")]
            public static extern void ReleaseImage(ref ImageInfo mat);

            /// Return Type: int
            /// 
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "SaveConfigureData")]
            public static extern int SaveConfigureData();

            /// Return Type: bool
            /// 
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "SaveSnapshot")]
            public static extern BoolType SaveSnapshot(string imageName);

            /// Return Type: void
            /// 
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "SetDeviceOrder")]
            public static extern void SetDeviceOrder(string curDeviceName);

            /// Return Type: ImageInfo
            ///mat: ImageInfo*
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "RotateImage")]
            public static extern ImageInfo RotateImage(ref ImageInfo frame, int rotateDegree);


            /// Return Type: int
            ///testName: char*
            [DllImportAttribute(dllName, EntryPoint = "StartTest")]
            public static extern int StartTest([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string testName);


            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "StopTest")]
            public static extern int StopTest();


            /// Return Type: void
            ///handler: TestStatusEventHandler
            [DllImportAttribute(dllName, EntryPoint = "SetTestStatusEventHandler")]
            public static extern void SetTestStatusEventHandler(TestStatusEventHandler handler);


            /// Return Type: void
            ///config: TestProjectConfig*
            [DllImportAttribute(dllName, EntryPoint = "SetTestProjectConfig")]
            public static extern int SetTestProjectConfig(ref TestProjectConfig config);

            /// Return Type: void
            ///config: TestProjectConfig
            [DllImportAttribute(dllName, EntryPoint = "GetTestProjectConfig")]
            public static extern System.IntPtr GetTestProjectConfig();

            /// Return Type: int
            ///scriptFile: char*
            [DllImportAttribute(dllName, EntryPoint = "ReplayQScript")]
            public static extern int ReplayQScript([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string scriptFile);


            /// Return Type: int
            ///scriptFile: char*
            [DllImportAttribute(dllName, EntryPoint = "RecordQScript")]
            public static extern int RecordQScript([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string scriptFile);


            /// Return Type: int
            ///enable: boolean
            [DllImportAttribute(dllName, EntryPoint = "EnableDisableGpuSupport")]
            public static extern int EnableDisableGpuSupport([MarshalAsAttribute(UnmanagedType.I4)] BoolType enable);

            /// Return Type: int
            ///devName: char*
            [DllImportAttribute(dllName, EntryPoint = "GetTargetDeviceHeight")]
            public static extern int GetTargetDeviceHeight([MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: int
            ///devName: char*
            [DllImportAttribute(dllName, EntryPoint = "GetTargetDeviceWidth")]
            public static extern int GetTargetDeviceWidth([MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: void
            ///devName: char*
            [DllImportAttribute(dllName, EntryPoint = "GetCurrentDevice")]
            public static extern void GetCurrentDevice(StringBuilder deviceName);


            /// Return Type: TargetDeviceStatus
            [DllImportAttribute(dllName, EntryPoint = "GetSpecifiedDeviceStatus")]
            public static extern System.IntPtr GetSpecifiedDeviceStatus([MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "RefreshTargetDevice")]
            public static extern int RefreshTargetDevice();

            /// Return Type: int
            ///action: TiltAction
            [DllImportAttribute(dllName, EntryPoint = "OnTilt")]
            public static extern int OnTilt(TiltAction action, int degree1, int degree2, int speed1 = 0, int speed2 = 0);


            /// Return Type: int
            ///rotateDirection: ZAxisRotationDirection
            ///zRotateTimeDuration: int
            [DllImportAttribute(dllName, EntryPoint = "OnZRotate")]
            public static extern int OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed);


            /// Return Type: int
            ///action: GenOpcodeAction
            [DllImportAttribute(dllName, EntryPoint = "OnAppAction")]
            public static extern int OnAppAction(AppLifeCycleAction action, [MarshalAsAttribute(UnmanagedType.LPStr)] string apkInfo1, [MarshalAsAttribute(UnmanagedType.LPStr)] string info2);


            /// Return Type: int
            ///distance: int
            [DllImportAttribute(dllName, EntryPoint = "OnMoveHolder")]
            public static extern int OnMoveHolder(int distance);


            /// Return Type: int
            ///action: PowerButtonPusherAction
            [DllImportAttribute(dllName, EntryPoint = "OnPowerButtonPusher")]
            public static extern int OnPowerButtonPusher(PowerButtonPusherAction action);


            /// Return Type: int
            ///action: UsbSwitchAction
            [DllImportAttribute(dllName, EntryPoint = "OnUsbSwitch")]
            public static extern int OnUsbSwitch(UsbSwitchAction action);


            /// Return Type: int
            ///action: EarphonePullerAction
            [DllImportAttribute(dllName, EntryPoint = "OnEarphonePuller")]
            public static extern int OnEarphonePuller(EarphonePullerAction action);


            /// Return Type: int
            ///action: RelayControllerAction
            [DllImportAttribute(dllName, EntryPoint = "OnRelayController")]
            public static extern int OnRelayController(RelayControllerAction action);


            /// Return Type: int
            ///action: ButtonAction
            [DllImportAttribute(dllName, EntryPoint = "OnButtonAction")]
            public static extern int OnButtonAction(ButtonAction action, ButtonActionType mode = ButtonActionType.DownAndUp);


            /// Return Type: int
            ///imageBoxInfo: ImageBoxInfo*
            ///action: MouseAction*
            [DllImportAttribute(dllName, EntryPoint = "OnMouseAction")]
            public static extern int OnMouseAction(ref ImageBoxInfo imageBoxInfo, ref MouseAction action);


            /// Return Type: int
            ///text: char*
            [DllImportAttribute(dllName, EntryPoint = "OnSetText")]
            public static extern int OnSetText([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string text);

            /// Return Type: int
            ///
            [DllImportAttribute(dllName, EntryPoint = "OnOcr")]
            public static extern int OnOcr();

            /// Return Type: int
            ///startRecord: BoolType
            [DllImportAttribute(dllName, EntryPoint = "OnAudioRecord")]
            public static extern int OnAudioRecord(BoolType startRecord);

            /// Return Type: int
            ///deviceName: char*
            [DllImportAttribute(dllName, EntryPoint = "Connect")]
            public static extern int Connect([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: int
            ///deviceName: char*
            [DllImportAttribute(dllName, EntryPoint = "Disconnect")]
            public static extern int Disconnect([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: int
            ///deviceName: char*
            [DllImportAttribute(dllName, EntryPoint = "IsDeviceConnected")]
            public static extern BoolType IsDeviceConnected([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: void
            ///id: int
            [DllImportAttribute(dllName, EntryPoint = "SetRecordWithID")]
            public static extern void SetRecordWithID(int id);

            /// Return Type: int
            ///imageBoxInfo: ImageBoxInfo
            [DllImportAttribute(dllName, EntryPoint = "OnSetRotateImageBoxInfo")]
            public static extern void OnSetRotateImageBoxInfo(ref ImageBoxInfo imageBoxInfo);


            /// Return Type: int
            ///text: char*
            [DllImportAttribute(dllName, EntryPoint = "Calibrate")]
            public static extern int Calibrate(int calibType);

            /// Return Type: void
            /// videoFile: char*
            /// frmCount: int
            [DllImportAttribute(dllName, EntryPoint = "OpenVideo")]
            public static extern System.IntPtr OpenVideo([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string videoFile, ref int frmCount);

            /// Return Type: void
            /// videoFile: char*
            [DllImportAttribute(dllName, EntryPoint = "ShowVideoFrame")]
            public static extern void ShowVideoFrame(System.IntPtr videoTpl, int frameIndex);

            /// Return Type: void
            /// videoFile: char*
            [DllImportAttribute(dllName, EntryPoint = "CloseVideo")]
            public static extern void CloseVideo(System.IntPtr videoTpl);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "GetGpuSupport")]
            public static extern BoolType GetGpuSupport();


            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetCaptureResolution")]
            public static extern BoolType SetCaptureResolution([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string captureName);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetCurrentCaptureDevice")]
            public static extern BoolType SetCurrentCaptureDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string captureName);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetCaptureDeviceForTargetDevice")]
            public static extern BoolType SetCaptureDeviceForTargetDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName,
                [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string captureName);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetHWAccessoryControllerForTargetDevice")]
            public static extern BoolType SetHWAccessoryControllerForTargetDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName,
                [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string comName);
            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetCurrentTargetDevice")]
            public static extern BoolType SetCurrentTargetDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetCurrentHWAccessoryController")]
            public static extern BoolType SetCurrentHWAccessoryController([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string ffrdName);

            /// <summary>
            /// 
            /// </summary>
            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "SetDeviceCurrentMultiLayerMode")]
            public static extern BoolType SetDeviceCurrentMultiLayerMode(string deviceName, bool enable);

            /// <summary>
            /// 
            /// </summary>
            /// <param name="deviceName"></param>
            /// <param name="maxSize"></param>
            [DllImportAttribute(dllName, EntryPoint = "GetCurrentTargetDevice")]
            public static extern BoolType GetCurrentTargetDevice(
                StringBuilder deviceName, int maxSize);

            /// Return Type: BoolType
            [DllImportAttribute(dllName, EntryPoint = "GetCaptureDeviceForTargetDevice")]
            public static extern BoolType GetCaptureDeviceForTargetDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName,
                StringBuilder captureName, int maxSize);
            /// <summary>
            /// 
            /// </summary>
            /// <param name="deviceName"></param>
            /// <param name="comName"></param>
            /// <param name="maxSize"></param>
            /// <returns></returns>
            [DllImportAttribute(dllName, EntryPoint = "GetHWAccessoryControllerForTargetDevice")]
            public static extern BoolType GetHWAccessoryControllerForTargetDevice([InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string deviceName,
                StringBuilder comName, int maxSize);

            /// <summary>
            /// 
            /// </summary>
            [DllImportAttribute(dllName, EntryPoint = "GetDeviceCurrentMultiLayerMode")]
            public static extern BoolType GetDeviceCurrentMultiLayerMode(string deviceName);

            /// Return Type: bool
            [DllImportAttribute(dllName, EntryPoint = "GetTargetDeviceNames")]
            public static extern int GetTargetDeviceNames([In, Out,
                MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] deviceNames, int length, int arraySize);

            /// Return Type: bool
            [DllImportAttribute(dllName, EntryPoint = "GetCaptureDeviceNames")]
            public static extern int GetCaptureDeviceNames([In, Out,
                MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] deviceNames, int length, int arraySize);

            /// Return Type: bool
            [DllImportAttribute(dllName, EntryPoint = "GetHWAccessoryControllerNames")]
            public static extern int GetHWAccessoryControllerNames([In, Out,
                MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] deviceNames, int length, int arraySize);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "GetAudioRecordDeviceNames")]
            public static extern int GetAudioRecordDeviceNames([In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] deviceNames, int length, int arraySize);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "GetAudioPlayDeviceNames")]
            public static extern int GetAudioPlayDeviceNames([In, Out, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] deviceNames, int length, int arraySize);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "SetAudioDevice")]
            public static extern int SetAudioDevice(
                [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string device, AudioDeviceType deviceType, [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string audioDevice);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "GetAudioDevice")]
            public static extern int GetAudioDevice(
                [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string device, AudioDeviceType deviceType, [In, Out, MarshalAsAttribute(UnmanagedType.LPStr)] System.Text.StringBuilder audioDevice, int length);

            /// Return Type: void
            [DllImportAttribute(dllName, EntryPoint = "DeleteTargetDevice")]
            public static extern void DeleteTargetDevice(string deviceName);

            /// Return Type: void
            [DllImportAttribute(dllName, EntryPoint = "AddNewTargetDevice")]
            public static extern void AddNewTargetDevice(string deviceName);

            /// Return type: int
            [DllImportAttribute(dllName, EntryPoint = "GetPackageActivity")]
            public static extern int GetPackageActivity([In, Out,
                MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] appsInfo, [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string appLoaction);

            /// Return Type: int
            [DllImportAttribute(dllName, EntryPoint = "GenerateReportIndex")]
            public static extern int GenerateReportIndex(string dirName);

            /// Return Type: ImageInfo
            ///mat: ImageInfo*
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "GetImageBounding")]
            public static extern Bounding GetImageBounding(ref ImageInfo frame, [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)] string modelImageName);

            /// Return Type: ImageInfo
            ///mat: ImageInfo*
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "IsROIMatch")]
            public static extern BoolType IsROIMatch(ref ImageInfo frame, Bounding roi, [InAttribute()] [MarshalAsAttribute(UnmanagedType.LPStr)]string objFileName);


            /// Return Type: ImageInfo
            ///mat: ImageInfo*
            [System.Runtime.InteropServices.DllImportAttribute(dllName, EntryPoint = "CopyImageInfo")]
            public static extern ImageInfo CopyImageInfo(ref ImageInfo frame);


            /// Return Type: DaVinciAPI.TestStatus
            [DllImportAttribute(dllName, EntryPoint = "GetTestStatus")]
            public static extern TestStatus GetTestStatus();

            /// Return Type: void
            [DllImportAttribute(dllName, EntryPoint = "SVMPlayNoTilt")]
            public static extern void SVMPlayNoTilt();

            /// Return Type: TestConfiguration
            [DllImportAttribute(dllName, EntryPoint = "GetTestConfiguration")]
            public static extern TestConfiguration GetTestConfiguration();
        }
    }
}

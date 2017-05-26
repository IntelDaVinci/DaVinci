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

#ifndef __PHYSICAL_CAMERA_CAPTURE_HPP__
#define __PHYSICAL_CAMERA_CAPTURE_HPP__

#include "CaptureDevice.hpp"
#include "boost/thread/recursive_mutex.hpp"

namespace DaVinci
{
    using namespace cv;


#define  HOMOGRAPHY_ROWS  3
#define  HOMOGRAPHY_COLS  3
#define  HOMOGRAPHY_STRING "HomographyMatrix"


    class PhysicalCameraCapture : public CaptureDevice
    {
    public:
        PhysicalCameraCapture(int index, string name, string shortName);

        virtual ~PhysicalCameraCapture();

        virtual bool InitCamera(CaptureDevice::Preset preset) override;
        virtual string GetCameraName() override;
        virtual cv::Mat RetrieveFrame(int *frameIndex = NULL) override;
        virtual bool IsFrameAvailable() override;
        virtual DaVinciStatus Start() override;
        virtual DaVinciStatus Stop() override;
        virtual DaVinciStatus Restart() override;

        DaVinciStatus OpenCapture();
        void ReleaseCapture();

        int GetCameraIndex();

        /************************************************************************************/
        // Functions for calibration homography
        void ResetCalibrateHomography(bool shouldSave=true);

        void SaveCalibrateHomographyToXml(string homoStr);
        void SaveCalibrateHomographyToIni(string iniFile, string homoStr);
        void ReadHomographyString(string & homoStr);
        void SaveCalibrateHomography();

        void LoadCalibrateHomographyFromXml(string & homoStr);
        void LoadCalibrateHomographyFromIni(string homoFile, string & homoStr);
        void WriteHomographyString(string homoStr);
        void LoadCalibrateHomography();

        bool CalculateCalibrateHomography(Mat homoCalibrated, CaptureDevice::Preset presetCalibrated, 
            CaptureDevice::Preset preset, Mat & homography, int & width, int & height);
        void SetCalibrateHomography(Mat homography, CaptureDevice::Preset preset, int width, int height, bool shouldSave = true);
        /************************************************************************************/

        bool IsHighSpeedCamera();

        virtual void SetCaptureProperty(int prop, double value, bool shouldCheck = false);
        virtual double GetCaptureProperty(int prop);
        virtual double GetCapturePropertyMax(int prop);
        virtual double GetCapturePropertyMin(int prop);
        virtual double GetCapturePropertyStep(int prop);


        void CopyCameraConfig();
        void SaveCameraConfig(int keyIndex, int value);
        bool SaveCameraConfigToIni(string configFile);
        void SaveCameraConfigToXml(int keyIndex, int value);

        bool LoadCameraConfig();
        bool LoadCameraConfigFromXml();
        bool LoadCameraConfigFromIni();
        bool LoadCameraConfigFromIni(string configFile);

        static DaVinciStatus DetectPhysicalCameras(vector<boost::shared_ptr<PhysicalCameraCapture>> & cameraList, int currentPhysCamsIndex = -1);


        double GetCameraConfig(int index)
        {
            return cameraConfigs[index];
        }

        int GetCalibrateWidth(int index)
        {
            return calibrateWidth[index];
        }

        int GetCalibrateHeight(int index)
        {
            return calibrateHeight[index];
        }

        Mat GetCalibrateHomography(int index)
        {
            return homographyMats[index];
        }

        bool GetCalibrationSize(CaptureDevice::Preset preset, int &presetWidth, int &presetHeight);

        static const int defaultCalibrationWidth[CaptureDevice::Preset::PresetNumber];

        static const int LIFECAM_INDEX = 1;
        static const int HIGHSPEED_CAMERA_INDEX = 3;
        static const int CAMERA_COUNT = 4;
        static const string supportedCameraName[CAMERA_COUNT];

        static const int highResWidth = 1920, highResHeight = 1080;
        static const int aiResWidth = 640, aiResHeight = 360;
        static const int highSpeedWidth = 640, highSpeedHeight = 480;

        enum CameraConfigIndex
        {
            /// <summary>
            /// white balance index in camera config array
            /// </summary>
            whiteBalanceIndexInCamCfg = 0,
            /// <summary>
            /// contrast index in camera config array
            /// </summary>
            contrastIndexInCamCfg = 1,
            /// <summary>
            /// saturation index in camera config array
            /// </summary>
            saturationIndexInCamCfg = 2,
            /// <summary>
            /// gain index in camera config array
            /// </summary>
            gainIndexInCamCfg = 3,
            /// <summary>
            /// gamma index in camera config array
            /// </summary>
            gammaIndexInCamCfg = 4,
            /// <summary>
            /// hue index in camera config array
            /// </summary>
            hueIndexInCamCfg = 5,
            /// <summary>
            /// sharpness index in camera config array
            /// </summary>
            sharpnessIndexInCamCfg = 6,
            /// <summary>
            /// brightness index in camera config array
            /// </summary>
            brightnessIndexInCamCfg = 7,
            /// <summary>
            /// exposure index in camera config array
            /// </summary>
            exposureIndexInCamCfg = 8,
            /// <summary>
            /// focus index in camera config array
            /// </summary>
            focusIndexInCamCfg = 9,

            maxCameraConfigs = 10
        };

        /// <summary>
        /// string array for camera config.
        /// </summary>
        static String camCfgString[maxCameraConfigs];
        static double defaultCamCfgs[maxCameraConfigs];
        double cameraConfigs[maxCameraConfigs];

    private:
        void Init(int);

        string cameraName;
        string cameraShortName;
        int cameraIndex;
        boost::shared_ptr<VideoCapture> captureObject;
        boost::recursive_mutex captureObjectMutex;

        // calibration data
        boost::mutex lockCalibrateData;
        int calibrateWidth[CaptureDevice::Preset::PresetNumber];
        int calibrateHeight[CaptureDevice::Preset::PresetNumber];
        Mat homographyMats[CaptureDevice::Preset::PresetNumber];
    };

}

#endif

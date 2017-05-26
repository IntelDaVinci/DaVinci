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

#include "PhysicalCameraCapture.hpp"
#include "TestManager.hpp"
#include "DeviceManager.hpp"
#include "QScript/VideoCaptureProxy.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


namespace DaVinci
{
    const int PhysicalCameraCapture::defaultCalibrationWidth[CaptureDevice::Preset::PresetNumber] = {
        640, 1280, 640
    };

    const string PhysicalCameraCapture::supportedCameraName[] = 
    { 
        "Camera-less",
        "LifeCam Studio",
        "C920",
        "Basler GenICam Source" 
    };

    /// <summary>
    /// string array for camera config.
    /// </summary>
    String PhysicalCameraCapture::camCfgString[] = 
    {
        "whitebalance",
        "contrast",
        "saturation",
        "gain",
        "gamma",
        "hue",
        "sharpness",
        "brightness",
        "exposure",
        "focus"
    };

    double PhysicalCameraCapture::defaultCamCfgs[] = { 4550, 5, 103, 100, 0, 0, 25, 50, -8, 0 };

    PhysicalCameraCapture::PhysicalCameraCapture(int index, string name, string shortName)
    {
        Init(index);
        cameraName = name;
        cameraShortName = shortName;
    }

    void PhysicalCameraCapture::Init(int index)
    {
        cameraIndex = index;

        for (int i=0; i<maxCameraConfigs; i++)
        {
            cameraConfigs[i] = -1;
        }

        ResetCalibrateHomography(false);
    }

    PhysicalCameraCapture::~PhysicalCameraCapture()
    {
        Stop();
    }

    /// <summary> Pre-condition: captureObjectMutex is locked. </summary>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus PhysicalCameraCapture::OpenCapture()
    {
        {
            boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
            captureObject = boost::shared_ptr<VideoCapture>(new VideoCaptureProxy(cameraIndex));
            if (captureObject == nullptr)
            {
                DAVINCI_LOG_ERROR << "Cannot initialize camera " << cameraIndex << ": " << cameraName;;
                return DaVinciStatus(errc::operation_not_permitted);;
            }

            if (!captureObject->isOpened())
            {
                DAVINCI_LOG_ERROR << "Cannot open camera " << cameraIndex << ": " << cameraName;;
                return DaVinciStatus(errc::no_such_device);
            }
        }

        // can we remove this init?
        InitCamera(Preset::HighResolution);

        return DaVinciStatusSuccess;
    }

    void PhysicalCameraCapture::ReleaseCapture()
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            captureObject->release();
    }

    DaVinciStatus PhysicalCameraCapture::Start()
    {
        DaVinciStatus status = OpenCapture();
        if (status != DaVinciStatusSuccess)
            return status;

        LoadCalibrateHomography();
        status = CaptureDevice::Start();

        if (status != DaVinciStatusSuccess)
            return status;

        return DaVinciStatusSuccess;
    }

    DaVinciStatus PhysicalCameraCapture::Stop()
    {
        DaVinciStatus status = CaptureDevice::Stop();

        ReleaseCapture();

        if (cameraName.empty() == false)
            cameraName.clear();

        return status;
    }

    DaVinciStatus PhysicalCameraCapture::Restart()
    {
        ReleaseCapture();
        return OpenCapture();
    }

    int PhysicalCameraCapture::GetCameraIndex()
    {
        return cameraIndex;
    }

    string PhysicalCameraCapture::GetCameraName()
    {
        if (cameraName.empty() == false)
        {
            return cameraName;
        }
        else
        {
            cameraName = cvGetCameraName(cameraIndex);
            if (cameraName.empty() == false)
                return cameraName;
        }

        return "PhysicalCameraCapture";
    }

    cv::Mat PhysicalCameraCapture::RetrieveFrame(int *frameIndex)
    {
        Mat image, resultImage;

        {
            boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
            if (IsFrameAvailable())
                captureObject->retrieve(image);
        }

        if ((image.empty() == false)
            &&(currentPreset != CaptureDevice::Preset::PresetDontCare)
            && (homographyMats[(int)currentPreset].empty() == false)
            && (calibrateWidth[(int)currentPreset] > 0)
            && (calibrateHeight[(int)currentPreset] > 0))
        {
            warpPerspective(image, resultImage, 
                homographyMats[(int)currentPreset],
                Size(calibrateWidth[(int)currentPreset],
                calibrateHeight[(int)currentPreset]),
                CV_WARP_FILL_OUTLIERS | WARP_INVERSE_MAP);
            image = resultImage;
        }

        return image;
    }

    bool PhysicalCameraCapture::IsFrameAvailable()
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            return captureObject->grab();
        return false;
    }

    // detect all physical cameras connected and update the list
    DaVinciStatus PhysicalCameraCapture::DetectPhysicalCameras(vector<boost::shared_ptr<PhysicalCameraCapture>> & currentList,
        int currentPhysCamsIndex)
    {
        // detect physical camera devices
        int cameraNumber = cvGetCameraNumber();
        if (cameraNumber <= 0)
        {
            currentList.clear();
            return DaVinciStatusSuccess;
        }

        vector<boost::shared_ptr<PhysicalCameraCapture>> tempList;
        for (int cameraIndex = 0; cameraIndex < cameraNumber; cameraIndex++)
        {
            string cameraName(cvGetCameraName(cameraIndex));
            if (cameraName.empty() == true)
            {
                DAVINCI_LOG_ERROR << "Camera " << cameraIndex << " name is null string!";
                continue;
            }

            for (unsigned int j=0; j<sizeof(PhysicalCameraCapture::supportedCameraName) / sizeof(string); j++)
            {
                // filter out the unsupported cameras
                if (boost::contains(cameraName, PhysicalCameraCapture::supportedCameraName[j]) == false)
                    continue;

                bool isConnected = true;
                bool isInList = false;
                unsigned int currentListIndex = (unsigned int)currentList.size() + 1;
                // high speed camera may be returned from opencv function even if it's not connected.
                // if current list is empty and the newly added camera is high speed camera,
                // we will try to open it and see if it's really connected or not.
                if (currentList.size() != 0)
                {
                    // otherwise, we'll use the object in list to open the high speed camera.
                    unsigned int listIndex;
                    for (listIndex = 0; listIndex < currentList.size(); listIndex++)
                    {
                        // we need to check both index and name. and remove it if one of them is different
                        if ((currentList[listIndex]->GetCameraIndex() == cameraIndex)
                            && (boost::algorithm::iequals(currentList[listIndex]->GetCameraName(), cameraName)))
                        {
                            if (currentList[listIndex]->IsStarted() == false)
                            {
                                // under console mode, we only check the camera with same index value of currentPhysCamsIndex;
                                // under GUI mode, currentPhysCamsIndex = -1, we need to check all cameras
                                if ((j == HIGHSPEED_CAMERA_INDEX)
                                    && ((currentPhysCamsIndex == cameraIndex) || (currentPhysCamsIndex == -1)))
                                {
                                    currentList[listIndex]->ReleaseCapture();
                                    if (currentList[listIndex]->OpenCapture() != DaVinciStatusSuccess)
                                        isConnected = false;
                                }
                            }
                            else
                            {
                                currentListIndex = listIndex;
                            }
                            isInList = true;
                            break;
                        }
                    }
                }

                if (isInList == false)
                {
                    if ((j == HIGHSPEED_CAMERA_INDEX)
                        && ((currentPhysCamsIndex == cameraIndex) || (currentPhysCamsIndex == -1)))
                    {
                        // we need to release all of high speed cameras in list,
                        // and try to open the high speed camera with new index to check it's connected or not
                        unsigned int listIndex;
                        for (listIndex = 0; listIndex < currentList.size(); listIndex++)
                        {
                            // we need to check both index and name. and remove it if one of them is different
                            if (boost::algorithm::iequals(currentList[listIndex]->GetCameraName(), 
                                supportedCameraName[HIGHSPEED_CAMERA_INDEX]))
                            {
                                currentList[listIndex]->ReleaseCapture();
                            }
                        }

                        VideoCaptureProxy tempCaptureObject(cameraIndex);
                        if (tempCaptureObject.isOpened())
                        {
                            tempCaptureObject.release();
                        }
                        else
                            isConnected = false;
                    }
                }

                // if the newly added camera is not included in the list, and is really connected,
                // we need to create an object for it and add it to temporary list
                if (isConnected)
                {
                    boost::shared_ptr<PhysicalCameraCapture> tempCapDevice;
                    if (currentListIndex < currentList.size())
                    {
                        tempCapDevice = currentList[currentListIndex];
                    }
                    else
                    {
                        tempCapDevice = boost::shared_ptr<PhysicalCameraCapture>(new PhysicalCameraCapture(cameraIndex,
                            cameraName, PhysicalCameraCapture::supportedCameraName[j]));
                    }
                    DAVINCI_LOG_DEBUG << "Camera " << cameraIndex << ": " << cameraName;
                    tempList.push_back(tempCapDevice);
                }

                break;
            }
        }

        // update the list of newly added cameras into current camera list
        currentList.clear();
        if (tempList.size() > 0)
        {
            currentList = tempList;
        }

        return DaVinciStatusSuccess;
    }

    void PhysicalCameraCapture::ResetCalibrateHomography(bool shouldSave)
    {
        for (int i=(int)CaptureDevice::Preset::HighSpeed; i < (int)CaptureDevice::Preset::PresetNumber; i++)
        {
            calibrateWidth[i] = -1;
            calibrateHeight[i] = -1;
            if (homographyMats[i].empty() == false)
                homographyMats[i].release();
        }
        if (shouldSave)
            SaveCalibrateHomography();
    }

    void PhysicalCameraCapture::SaveCalibrateHomographyToXml(string homoStr)
    {
        if (homoStr.empty() == false)
        {
            boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
            if (targetDevice != nullptr)
                targetDevice->SetCalibrationData(homoStr);
        }
    }

    void PhysicalCameraCapture::SaveCalibrateHomographyToIni(string iniFile, string homoStr)
    {
        if (homoStr.empty() == false)
        {
            ofstream outFile(iniFile);
            outFile << homoStr;
            outFile.close();
        }
    }

    void PhysicalCameraCapture::ReadHomographyString(string & homoStr)
    {
        for (int i=(int)CaptureDevice::Preset::HighSpeed; i < (int)CaptureDevice::Preset::PresetNumber; i++)
        {
            homoStr.append("width" + boost::lexical_cast<string>(i)
                + "=" + boost::lexical_cast<string>(calibrateWidth[i]) + ";");
            homoStr.append("height" + boost::lexical_cast<string>(i)
                + "=" + boost::lexical_cast<string>(calibrateHeight[i]) + ";");

            for (int j=0; j<HOMOGRAPHY_ROWS; j++)
            {
                for (int k=0; k<HOMOGRAPHY_COLS; k++)
                {
                    double elementValue = 0.0;
                    if (homographyMats[i].empty() == false)
                        elementValue = homographyMats[i].at<double>(j,k);
                    string elementString = boost::lexical_cast<string>(elementValue);

                    homoStr.append(
                        "homography" + boost::lexical_cast<string>(i)
                        + "_" + boost::lexical_cast<string>(j)
                        + "_" + boost::lexical_cast<string>(k)
                        + "=" + elementString + ";");
                }
            }
        }
    }

    void PhysicalCameraCapture::SaveCalibrateHomography()
    {
        string homographyString;

        ReadHomographyString(homographyString);

        if (homographyString.empty() == false)
        {
            SaveCalibrateHomographyToXml(homographyString);

            SaveCalibrateHomographyToIni(TestManager::Instance().GetDaVinciResourcePath("homography.txt"), homographyString);
        }
    }

    void PhysicalCameraCapture::LoadCalibrateHomographyFromXml(string & homoStr)
    {
        boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
            return;

        // firstly try to read calibration data from davinci.config
        homoStr = targetDevice->GetCalibrationData();

        if (homoStr.empty() == true)
        {
            DAVINCI_LOG_INFO << "No homography data is saved.";
            return;
        }

        WriteHomographyString(homoStr);
    }

    void PhysicalCameraCapture::LoadCalibrateHomographyFromIni(string homoFile, string & homoStr)
    {
        // if there is no calibration data in davinci.config, try to read it from homography.txt file
        ifstream inFile(homoFile);
        if (inFile.fail())
        {
            DAVINCI_LOG_INFO << "Homography file doesn't exist: " << homoFile;
            return;
        }
        inFile >> homoStr;

        if (homoStr.empty() == true)
        {
            DAVINCI_LOG_ERROR << "No homography data saved in " << homoFile;
            return;
        }

        WriteHomographyString(homoStr);
    }


    void PhysicalCameraCapture::WriteHomographyString(string homoStr)
    {
        vector<std::string> homoStrings;
        boost::split(homoStrings, homoStr, boost::is_any_of(";"));

        for (auto iter = homoStrings.begin(); iter != homoStrings.end(); iter++)
        {
            std::string currentString = (*iter);
            int index = 0;
            string indexString;
            if (boost::algorithm::contains(currentString, "width"))
            {
                vector<std::string> equationStrings; 
                boost::split(equationStrings, currentString, boost::is_any_of("="));
                indexString = equationStrings[0].substr(strlen("width"),
                    equationStrings[0].length() - strlen("width"));
                if (TryParse(indexString, index))
                {
                    int width = 0;
                    if (TryParse(equationStrings[1], width)
                        && (width == defaultCalibrationWidth[index]))
                        calibrateWidth[index] = width;
                }
            }
            else if (boost::algorithm::contains(currentString, "height"))
            {
                vector<std::string> equationStrings; 
                boost::split(equationStrings, currentString, boost::is_any_of("="));
                indexString = equationStrings[0].substr(strlen("height"),
                    equationStrings[0].length() - strlen("height"));
                if (TryParse(indexString, index))
                {
                    int height = 0;
                    // Assumption: height should be less than width
                    if (TryParse(equationStrings[1], height)
                        && (height > 0)
                        && (height <= defaultCalibrationWidth[index]))
                        calibrateHeight[index] = height;
                }
            }
            else if (boost::algorithm::contains(currentString, "homography"))
            {
                // homography[Index]_[row]_[col]=value
                vector<string> equationStrings;
                boost::split(equationStrings, currentString, boost::is_any_of("="));

                indexString = equationStrings[0].substr(strlen("homography"),
                    equationStrings[0].length() - strlen("homography"));
                vector<string> subStrs;
                boost::split(subStrs, indexString, boost::is_any_of("_"));
                int rowIndex = 0, colIndex = 0;
                if (TryParse(subStrs[0], index) == false
                    || (index <= (int)CaptureDevice::Preset::PresetDontCare)
                    || (index >= (int)CaptureDevice::Preset::PresetNumber))
                    continue;
                if  (TryParse(subStrs[1], rowIndex) == false
                    || TryParse(subStrs[2], colIndex) == false
                    || (rowIndex >= HOMOGRAPHY_ROWS)
                    || (rowIndex < 0)
                    || (colIndex >= HOMOGRAPHY_COLS)
                    || (colIndex < 0))
                    continue;

                double homoValue = 0.0;
                if (TryParse(equationStrings[1], homoValue) == false)
                    continue;

                // Assumption: width and height should be read before homography value
                if ((calibrateWidth[index] > 0)
                    && (calibrateHeight[index] > 0))
                {
                    if (homographyMats[index].empty())
                        homographyMats[index].create(HOMOGRAPHY_ROWS,
                        HOMOGRAPHY_COLS, CV_64F);
                    homographyMats[index].at<double>(rowIndex,colIndex)
                        = homoValue;
                }
            }
        }

        if (calibrateWidth[(int)CaptureDevice::Preset::HighResolution] > 0
            && calibrateHeight[(int)CaptureDevice::Preset::HighResolution] > 0
            && homographyMats[(int)CaptureDevice::Preset::HighResolution].empty() == false)
        {
            Mat homography;
            int width = 0, height = 0;

            // calculate and save homography for AI resolution if it's null
            if (calibrateWidth[(int)CaptureDevice::Preset::AIResolution] <= 0
                || calibrateHeight[(int)CaptureDevice::Preset::AIResolution] <= 0
                || homographyMats[(int)CaptureDevice::Preset::AIResolution].empty())
            {
                if (true == CalculateCalibrateHomography(homographyMats[(int)CaptureDevice::Preset::HighResolution], 
                    CaptureDevice::Preset::HighResolution,
                    CaptureDevice::Preset::AIResolution,
                    homography, width, height))
                {
                    SetCalibrateHomography(homography, CaptureDevice::Preset::AIResolution, width, height);
                    homography.release();
                }
            }

            // calculate and save homography for high speed resolution if it's null
            if (calibrateWidth[(int)CaptureDevice::Preset::HighSpeed] <= 0
                || calibrateHeight[(int)CaptureDevice::Preset::HighSpeed] <= 0
                || homographyMats[(int)CaptureDevice::Preset::HighSpeed].empty())
            {
                if (true == CalculateCalibrateHomography(homographyMats[(int)CaptureDevice::Preset::HighResolution], 
                    CaptureDevice::Preset::HighResolution,
                    CaptureDevice::Preset::HighSpeed,
                    homography, width, height))
                {
                    SetCalibrateHomography(homography, CaptureDevice::Preset::HighSpeed, width, height);
                    homography.release();
                }
            }
        }
    }

    void PhysicalCameraCapture::LoadCalibrateHomography()
    {
        ResetCalibrateHomography(false);

        string homographyString;
        // firstly try to read calibration data from homography.txt, as in console mode, davinci.config isn't updated.
        LoadCalibrateHomographyFromIni(
            TestManager::Instance().GetDaVinciResourcePath("homography.txt"),
            homographyString);
        if (homographyString.empty() == true)
        {
            // if there is no calibration data in homography.txt, try to read it from davinci.config file
            LoadCalibrateHomographyFromXml(homographyString);
        }

        if (homographyString.empty() == true)
        {
            DAVINCI_LOG_ERROR << "No homography for calibration!";
            return;
        }
    }

    bool PhysicalCameraCapture::GetCalibrationSize(CaptureDevice::Preset preset, int &presetWidth, int &presetHeight)
    {
        if (IsHighSpeedCamera() == false)
        {
            if (preset == CaptureDevice::Preset::HighResolution || preset == CaptureDevice::Preset::HighSpeed)
            {
                presetWidth = highResWidth;
                presetHeight = highResHeight;
            }
            else if (preset == CaptureDevice::Preset::AIResolution)
            {
                presetWidth = aiResWidth;
                presetHeight = aiResHeight;
            }
            else
            {
                return false;
            }
        }
        else
        {
            presetWidth = highSpeedWidth;
            presetHeight = highSpeedHeight;
        }

        return true;
    }

    bool PhysicalCameraCapture::CalculateCalibrateHomography(Mat homoCalibrated, CaptureDevice::Preset presetCalibrated, CaptureDevice::Preset preset, Mat & homography, int & width, int & height)
    {
        int presetWidth = 0, presetHeight = 0;
        int presetCalibWidth = 0, presetCalibHeight = 0;

        if ((presetCalibrated <= CaptureDevice::Preset::PresetDontCare)
            || (presetCalibrated >= CaptureDevice::Preset::PresetNumber))
        {
            DAVINCI_LOG_ERROR << "Invalid preset to be calculated: " << (int)presetCalibrated;
            return false;
        }

        if ((GetCalibrationSize(presetCalibrated, presetCalibWidth, presetCalibHeight) == false)
            ||  (GetCalibrationSize(preset, presetWidth, presetHeight) == false))
        {
            DAVINCI_LOG_ERROR << "Cannot get calibration size for preset: " << (int)presetCalibrated;
            homography.release();
            width = 0;
            height = 0;
            return false;
        }

        double frameWidthRatio = static_cast<double>(presetCalibWidth) / presetWidth;
        double frameHeightRatio = static_cast<double>(presetCalibHeight) / presetHeight;

        width = defaultCalibrationWidth[(int)preset];
        height = (calibrateHeight[(int)presetCalibrated] * defaultCalibrationWidth[(int)preset]) / calibrateWidth[(int)presetCalibrated];
        double calibrateWidthRatio = static_cast<double>(calibrateWidth[(int)presetCalibrated]) / width;
        double calibrateHeightRatio = static_cast<double>(calibrateHeight[(int)presetCalibrated]) / height;

        homography = homoCalibrated.clone();

        // high resolution: [x1, y1, 1]T = H[x2, y2, 1]T
        //     width and height of input frame: w1 * h1, calibrated lotus image: w2 * h2
        // other resolution: [x1', y1', 1]T = H'[x2', y2', 1]T
        //     width and height of input frame: w1' * h1', calibrated lotus image: w2' * h2'
        // x1' = w1' * x1 / w1. y1' = h1' * y1 / h1.
        // x2' = w2' * x2 / w2. y2' = h2' * y2 / h2.
        homography.at<double>(0, 0) = (homography.at<double>(0, 0) * calibrateWidthRatio) / frameWidthRatio;
        homography.at<double>(0, 1) = (homography.at<double>(0, 1) * calibrateHeightRatio) / frameWidthRatio;
        homography.at<double>(0, 2) /= frameWidthRatio;
        homography.at<double>(1, 0) = (homography.at<double>(1, 0) * calibrateWidthRatio) / frameHeightRatio;
        homography.at<double>(1, 1) = (homography.at<double>(1, 1) * calibrateHeightRatio) / frameHeightRatio;
        homography.at<double>(1, 2) /= frameHeightRatio;
        homography.at<double>(2, 0) *= calibrateWidthRatio;
        homography.at<double>(2, 1) *= calibrateHeightRatio;

        return true;
    }

    void PhysicalCameraCapture::SetCalibrateHomography(Mat homography, CaptureDevice::Preset preset, int width, int height, bool shouldSave)
    {
        if ((preset <= CaptureDevice::Preset::PresetDontCare)
            || (preset >= CaptureDevice::Preset::PresetNumber))
        {
            DAVINCI_LOG_ERROR << "Invalid preset to be set: " << (int)preset;
            return;
        }

        {
            boost::lock_guard<boost::mutex> lock(lockCalibrateData);
            calibrateWidth[(int)preset] = width;
            calibrateHeight[(int)preset] = height;
            homographyMats[(int)preset] = homography.clone();
        }

        if (shouldSave == true)
            SaveCalibrateHomography();
    }

    bool PhysicalCameraCapture::IsHighSpeedCamera()
    {
        return boost::algorithm::contains(cameraName, supportedCameraName[HIGHSPEED_CAMERA_INDEX]);
    }

    /// <summary> Sets capture property. The property id is defined in opencv highgui </summary>
    ///
    /// <param name="prop">  The property. </param>
    /// <param name="value"> The value. </param>
    /// <param name="shouldCheck"> should check current value is same with value to be set before setting. default value is false.</param>
    void PhysicalCameraCapture::SetCaptureProperty(int prop, double value, bool shouldCheck)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
        {
            if (shouldCheck)
            {
                double oldValue = captureObject->get(prop);
                if (FEquals(oldValue, value))
                {
                    DAVINCI_LOG_DEBUG << "Camera property (" << prop << ") value to be set ("
                        << value << ") is same with the value set in camera (" << oldValue << ").";
                    return;
                }
            }
            captureObject->set(prop, value);
        }
        else
        {
            DAVINCI_LOG_ERROR << "Camera is closed. Cannot set capture property (" << prop << ")";
        }
    }

    /// <summary> Gets capture property. The property id is defined in opencv highgui </summary>
    ///
    /// <param name="prop"> The property. </param>
    ///
    /// <returns> The capture property. </returns>
    double PhysicalCameraCapture::GetCaptureProperty(int prop)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            return captureObject->get(prop);
        return 0;
    }

    /// <summary> Finds the capturepropertymax of the given arguments. </summary>
    ///
    /// <param name="prop"> The property. </param>
    ///
    /// <returns> The calculated capture property maximum. </returns>
    double PhysicalCameraCapture::GetCapturePropertyMax(int prop)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            return captureObject->getPropertyMax(prop);
        return 0;
    }

    /// <summary> Finds the capturepropertymin of the given arguments. </summary>
    ///
    /// <param name="prop"> The property. </param>
    ///
    /// <returns> The calculated capture property minimum. </returns>
    double PhysicalCameraCapture::GetCapturePropertyMin(int prop)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            return captureObject->getPropertyMin(prop);
        return 0;
    }

    /// <summary> Gets capture property step. </summary>
    ///
    /// <param name="prop"> The property. </param>
    ///
    /// <returns> The capture property step. </returns>
    double PhysicalCameraCapture::GetCapturePropertyStep(int prop)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
        if (captureObject != nullptr)
            return captureObject->getPropertyStep(prop);

        return 0;
    }

    void PhysicalCameraCapture::CopyCameraConfig()
    {
        std::string defCamCfg = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("CameraConfig", cameraShortName + ".ini"));

        DAVINCI_LOG_DEBUG << "Copy Camera config: " << defCamCfg;

        LoadCameraConfigFromIni(defCamCfg);
        std::string userCamCfg = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("CameraConfig", cameraShortName + ".ini.user"));
        SaveCameraConfigToIni(userCamCfg);
    }


    bool PhysicalCameraCapture::LoadCameraConfigFromIni()
    {
        // legacy code. should be removed after saving davinci.config is ready
        std::string userCamCfg = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("CameraConfig", cameraShortName + ".ini.user"));

        DAVINCI_LOG_INFO << "Try to load camera configuration from " << userCamCfg;

        if (LoadCameraConfigFromIni(userCamCfg) == false)
        {
            // load from default config file
            std::string defCamCfg = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("CameraConfig", cameraShortName + ".ini"));

            DAVINCI_LOG_INFO << "Try to load camera configuration from " << defCamCfg;

            return LoadCameraConfigFromIni(defCamCfg);
        }
        return true;
    }

    bool PhysicalCameraCapture::LoadCameraConfigFromIni(string configFile)
    {
        if (boost::filesystem::exists(configFile) == false)
        {
            DAVINCI_LOG_ERROR << "Camera config file doesn't exist: " << configFile;
            return false;
        }

        std::string settings = "";
        boost::property_tree::ptree iniPtr;
        try
        {
            boost::property_tree::ini_parser::read_ini(configFile, iniPtr);
        }
        catch (const boost::property_tree::ptree_error &e)
        {
            DAVINCI_LOG_ERROR << "Error during reading ini: " << e.what();
            return false;
        }

        if (iniPtr.empty() == true)
        {
            DAVINCI_LOG_ERROR << "Incorrect ini file: " << configFile;
            return false;
        }

        for (int i = 0; i < maxCameraConfigs; i++)
        {
            string section;
            try
            {
                section = cameraShortName + "." + camCfgString[i];
                string value = iniPtr.get<string>(section);
                DAVINCI_LOG_DEBUG << "Load " << section << ", value = " << value;
                settings += camCfgString[i] + "=" + value  +";";
                int intValue = 0;
                if (TryParse(value, intValue))
                    cameraConfigs[i] = intValue;
            }
            catch(...)
            {
                DAVINCI_LOG_DEBUG << "No camera config: " << section << " in ini (" << configFile << ").";
                continue;
            }
        }

        if (settings.empty() == true)
        {
            DAVINCI_LOG_DEBUG << "Cannot find any camera config in ini (" << configFile << ").";
            return false;
        }
        DAVINCI_LOG_INFO << "Load camera config successfully from " << configFile;

        boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "No target device is connected.";
            return false;
        }
        targetDevice->SetCameraSetting(settings);
        return true;
    }


    bool PhysicalCameraCapture::SaveCameraConfigToIni(string configFile)
    {
        boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
            return false;
        string settings = targetDevice->GetCameraSetting();
        boost::property_tree::ptree iniPtr;

        DAVINCI_LOG_INFO << "Save Camera configs to file:" << configFile;

        if (settings.length() > 0)
        {
            vector<string> settingStrs;
            boost::split(settingStrs, settings, boost::is_any_of(";"), boost::token_compress_on);

            vector<string>::iterator iter = settingStrs.begin();
            while (iter != settingStrs.end())
            {
                if ((*iter).length() > 0)
                {
                    vector<string> rowStrs;
                    boost::split(rowStrs, (*iter), boost::is_any_of("="), boost::token_compress_on);
                    if (rowStrs.size() == 2)
                    {
                        DAVINCI_LOG_DEBUG << "name = " << rowStrs[0] << ", value = " << rowStrs[1];
                        string section = cameraShortName + "." + rowStrs[0];
                        iniPtr.put(section, rowStrs[1]);
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "Error value in setting: " << (*iter);
                    }
                }
                iter++;
            }
            try
            {
                boost::property_tree::ini_parser::write_ini(configFile, iniPtr);
            }
            catch (const boost::property_tree::ptree_error &e)
            {
                DAVINCI_LOG_ERROR << "Error during writing ini: " << e.what();
                return false;
            }
        }

        return true;
    }

    void PhysicalCameraCapture::SaveCameraConfigToXml(int keyIndex, int value)
    {
        boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
            return;
        string settings = targetDevice->GetCameraSetting();
        string newSettings;

        DAVINCI_LOG_INFO << "Save Camera configuration: " + camCfgString[keyIndex]
        + " = " + boost::lexical_cast<string>(value);

        // analyze current settings. update the value if the setting exist, otherwise append it
        if (settings.length() > 0)
        {
            vector<string> settingStrs;
            boost::split(settingStrs, settings, boost::is_any_of(";"), boost::token_compress_on);

            vector<string>::iterator iter = settingStrs.begin();
            bool found = false;
            while (iter != settingStrs.end())
            {
                if ((*iter).length() > 0)
                {
                    vector<string> rowStrs;
                    boost::split(rowStrs, (*iter), boost::is_any_of("="), boost::token_compress_on);
                    if (rowStrs.size() == 2)
                    {
                        if (boost::algorithm::iequals(rowStrs[0], camCfgString[keyIndex]))
                        {
                            // update the value
                            rowStrs[1] = boost::lexical_cast<string>(value);
                            found = true;
                        }
                        DAVINCI_LOG_DEBUG << "name = " << rowStrs[0] << ", value = " << rowStrs[1];
                        newSettings += rowStrs[0] + "=" + rowStrs[1] + ";";
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "Error value in ini: " << (*iter);
                    }
                }
                iter++;
            }
            if (found == false)
            {
                newSettings += camCfgString[keyIndex] + "=" + boost::lexical_cast<string>(value) + ";";
            }
        }
        else
        {
            newSettings = camCfgString[keyIndex] + "=" + boost::lexical_cast<string>(value) + ";";
        }
        // save updated settings
        targetDevice->SetCameraSetting(newSettings);
    }

    void PhysicalCameraCapture::SaveCameraConfig(int keyIndex, int value)
    {
        SaveCameraConfigToXml(keyIndex, value);

        // legacy: save settings to ini file
        std::string userCamCfg = TestManager::Instance().GetDaVinciResourcePath(ConcatPath("CameraConfig", cameraShortName + ".ini.user"));
        SaveCameraConfigToIni(userCamCfg);
    }

    bool PhysicalCameraCapture::LoadCameraConfigFromXml()
    {
        boost::shared_ptr<TargetDevice> targetDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (targetDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "No target device is connected!";
            return false;
        }

        string setting = targetDevice->GetCameraSetting();
        if (setting.length() <= 0)
        {
            DAVINCI_LOG_ERROR << "No camera config for device " << targetDevice->GetDeviceName();
            return false;
        }

        vector<string> settingStrs;
        boost::split(settingStrs, setting, boost::is_any_of(";"), boost::token_compress_on);

        vector<string>::iterator iter = settingStrs.begin();

        bool retValue = true;
        while (iter != settingStrs.end())
        {
            if ((*iter).length() <= 0)
            {
                // focus value at last may be empty
                if ((iter + 1) == settingStrs.end())
                {
                    iter++;
                    continue;
                }
                DAVINCI_LOG_ERROR << "Error camera config item from XML: " << (*iter);
                retValue = false;
                break;
            }
            vector<string> rowStrs;
            boost::split(rowStrs, (*iter), boost::is_any_of("="), boost::token_compress_on);
            if (rowStrs.size() != 2)
            {
                DAVINCI_LOG_ERROR << "Error format of camera config item from XML: " << (*iter);
                retValue = false;
                break;
            }
            DAVINCI_LOG_DEBUG << "name = " << rowStrs[0] << ", value = " << rowStrs[1];
            string propStr = rowStrs[0];
            int value = 0;
            if (TryParse(rowStrs[1], value) == false)
            {
                DAVINCI_LOG_ERROR << "Error value in camera config item (" << rowStrs[0] << "): " << rowStrs[1];
                retValue = false;
                break;
            }
            //Cross Validation: fix a bug here: camCfgString->length will return the length of first element of camCfgString array
            for (unsigned int i=0; i<sizeof(camCfgString) / sizeof(string); i++)
            {
                if (boost::algorithm::equals(camCfgString[i], propStr))
                {
                    cameraConfigs[i] = value;
                    break;
                }
            }

            iter++;
        }

        return retValue;
    }

    bool PhysicalCameraCapture::LoadCameraConfig()
    {
        DAVINCI_LOG_INFO << "Try to load camera configuration from ini...";

        if (LoadCameraConfigFromIni() == false)
        {
            DAVINCI_LOG_INFO << "No camera configuration saved in ini file.";
            return LoadCameraConfigFromXml();
        }

        return true;
    }

    /// <summary> Pre-condition: the captureObjectMutex is locked. </summary>
    ///
    /// <param name="preset"> The preset. </param>
    ///
    /// <returns> true if it succeeds, false if it fails. </returns>
    bool PhysicalCameraCapture::InitCamera(CaptureDevice::Preset preset)
    {
        if ((preset < CaptureDevice::Preset::PresetDontCare)
            || (preset >= CaptureDevice::Preset::PresetNumber))
        {
            DAVINCI_LOG_ERROR << "Invalid preset for camera initialization: " << (int)preset;
            return false;
        }
        DAVINCI_LOG_DEBUG << "Initialize camera properties for preset: "  << CapturePresetName[(int)preset+1];

        if (currentPreset == preset)
        {
            DAVINCI_LOG_DEBUG << "The preset was already initialized.";
            return true;
        }

        currentPreset = preset;

        {
            boost::lock_guard<boost::recursive_mutex> lock(captureObjectMutex);
            if (captureObject == nullptr)
            {
                DAVINCI_LOG_DEBUG << "Cannot initialize camera properties as null capture object.";
                return false;
            }
        }

        LoadCameraConfig();

        int frame_width = highSpeedWidth, frame_height = highSpeedHeight;
        int fps = 120;
        int fourcc = 0;
        // We prefer a fixed camera format setting to avoid reseting camera too often.
        // We observed that reseting the camera frequently would cause some camera drivers out of memory, such as LifeCam.
        if (IsHighSpeedCamera() == false)
        {
            if (preset == Preset::HighSpeed || preset == Preset::HighResolution)
            {
                fps = 30;
                frame_width = highResWidth;
                frame_height = highResHeight;
            }
            else if (preset == Preset::AIResolution)
            {
                fps = 30;
                frame_width = aiResWidth;
                frame_height = aiResHeight;
            }
            fourcc = CV_FOURCC('M', 'J', 'P', 'G');
        }
        else
        {
            // fourcc: RGB24
            fourcc = (int)0xe436eb7d;
            if (preset == Preset::AIResolution)
            {
                frame_width = aiResWidth;
                frame_height = aiResHeight;
            }
        }

        SetCaptureProperty(CV_CAP_PROP_FPS, fps, true);
        SetCaptureProperty(CV_CAP_PROP_FOURCC, fourcc, true);
        SetCaptureProperty(CV_CAP_PROP_FRAME_WIDTH, frame_width, true);
        SetCaptureProperty(CV_CAP_PROP_FRAME_HEIGHT, frame_height, true);

        if (IsHighSpeedCamera() == false)
        {
            SetCaptureProperty(CV_CAP_PROP_EXPOSURE, cameraConfigs[exposureIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_AUTO_EXPOSURE, 0, true);
            SetCaptureProperty(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, cameraConfigs[whiteBalanceIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_CONTRAST, cameraConfigs[contrastIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_SATURATION, cameraConfigs[saturationIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_GAIN, cameraConfigs[gainIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_GAMMA, cameraConfigs[gammaIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_HUE, cameraConfigs[hueIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_SHARPNESS, cameraConfigs[sharpnessIndexInCamCfg], true);
            SetCaptureProperty(CV_CAP_PROP_BRIGHTNESS, cameraConfigs[brightnessIndexInCamCfg], false);

            if (FEquals(cameraConfigs[focusIndexInCamCfg], -1.0) == false)
            {
                SetCaptureProperty(CV_CAP_PROP_FOCUS, cameraConfigs[focusIndexInCamCfg], true);
            }
        }
        else
        {
            SetCaptureProperty(CV_CAP_PROP_GAIN, cameraConfigs[gainIndexInCamCfg], true);
        }

        return true;
    }
}
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

#include "opencv2/gpu/gpu.hpp"

#include "boost/core/null_deleter.hpp"

#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/sax/HandlerBase.hpp"
#include "xercesc/util/XMLString.hpp"

#include "DeviceManager.hpp"
#include "HyperSoftCamCapture.hpp"
#include "PseudoCameraCapture.hpp"
#include "PhysicalCameraCapture.hpp"
#include "TestManager.hpp"
#include "AudioManager.hpp"
#include <windows.h> 
#include <tlhelp32.h> 

using namespace boost::process; 
using namespace xercesc;

namespace DaVinci
{
    const string DeviceManager::ConfigRoot = "DaVinciConfig";
    const string DeviceManager::ConfigUseGpuAcceleration = "useGpuAcceleration";
    const string DeviceManager::ConfigAudioRecordDevice = "audioRecordDevice";
    const string DeviceManager::ConfigAudioPlayDevice = "audioPlayDevice";
    const string DeviceManager::ConfigAndroidDevices = "AndroidDevices";
    const string DeviceManager::ConfigWindowsDevices = "WindowsDevices";
    const string DeviceManager::ConfigChromeDevices = "ChromeDevices";
    const string DeviceManager::ConfigIdentifiedByIp = "identifiedByIp";
    const string DeviceManager::ConfigMultiLayerSupported = "multiLayerSupported";
    const string DeviceManager::ConfigHeight = "height";
    const string DeviceManager::ConfigWidth = "width";
    const string DeviceManager::ConfigScreenSource = "screenSource";
    const string DeviceManager::ConfigCalibrateData = "calibrationData";
    const string DeviceManager::ConfigCameraSetting = "cameraSetting";
    const string DeviceManager::ConfigHwAccessoryController = "hwAccessoryController";
    const string DeviceManager::ConfigPortOfFFRD = "portOfFFRD";
    const string DeviceManager::ConfigAndroidTargetDevice = "AndroidTargetDevice";
    const string DeviceManager::ConfigDeviceModel = "deviceModel";
    const string DeviceManager::ConfigQagentPort = "qagentPort";
    const string DeviceManager::ConfigMagentPort = "magentPort";
    const string DeviceManager::ConfigHyperSoftCamPort = "hypersoftcamPort";

    DeviceManager::DeviceManager() :
        testedCudaSupport(false), hasCudaSupport(false), gpuAccelerationEnabled(false), enableTargetDeviceAgent(true),
        audioRecordDevice("Disabled"), audioPlayDevice("Disabled"), forceFFRDDetect(false)
    {
        globalDeviceStatus = boost::shared_ptr<GlobalDeviceStatus>(new GlobalDeviceStatus());
    }

    DaVinciStatus DeviceManager::InitGlobalDeviceStatus()
    {
        DaVinciStatus status = errc::device_or_resource_busy;

        if (globalDeviceStatus->IsInitialized())
            status = DaVinciStatusSuccess;

        return status;
    }

    DaVinciStatus DeviceManager::InitDevices(const string &deviceName)
    {
        DaVinciStatus status = InitTestDevices(deviceName);
        if (!DaVinciSuccess(status))
        {
            return status;
        }
        return InitHostDevices();
    }

    DaVinciStatus DeviceManager::DetectDevices(String currentTargetDeviceName)
    {
        DaVinciStatus status;

        // if there is one device connected, currentTargetDeviceName will be updated with its name
        // otherwise, the variable is same with value of input parameter.
        status = ScanTargetDevices(currentTargetDeviceName);
        if (!DaVinciSuccess(status))
            return status;

        if (TestManager::Instance().IsConsoleMode() == false)
        {
            // 1. GUI mode, we should detect all serial ports and physical cameras
            status = ScanHWAccessories();
            if (!DaVinciSuccess(status))
                return status;
            status = ScanCaptureDevices();
        }
        else
        {
            String currentHWAccessory = "";
            int currentPhysCamsIndex = ScreenSourceToIndex(ScreenSource::Undefined);

            if (boost::algorithm::iequals(currentTargetDeviceName, "") == false)
            {
                boost::shared_ptr<TargetDevice> theTargetDevice = nullptr;
                boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);

                for (auto iter = targetDevices.begin(); iter != targetDevices.end(); iter++)
                {
                    boost::shared_ptr<TargetDevice> targetDevice = boost::dynamic_pointer_cast<TargetDevice>(*iter);
                    if ((targetDevice != nullptr) && (targetDevice->GetDeviceName() == currentTargetDeviceName))
                    {
                        theTargetDevice = targetDevice;
                        break;
                    }
                }
                if (theTargetDevice != nullptr)
                {
                    currentHWAccessory = theTargetDevice->hwAccessoryController;
                    ScreenSource screenSource = theTargetDevice->GetScreenSource();
                    currentPhysCamsIndex = ScreenSourceToIndex(screenSource);
                }
            }

            // in 2.4 release, we may save "None" same with "", which doesn't specify hw accessory for target device
            if (boost::algorithm::iequals(currentHWAccessory, "None"))
                currentHWAccessory = "";

            // if the variable is set to true, we should scan all of serial ports
            if (forceFFRDDetect)
                currentHWAccessory = "";

            // 1) without config
            //     1.1) 1 device connected or is specified in command if multiple devices connected
            //          currentTargetDeviceName!="", currentHWAccessory == "Undefined"
            //          serial port detection: YES (to detect all ports)
            //     1.2) multiple devices is connected and no device is specified in command
            //          currentTargetDeviceName=="", currentHWAccessory == ""
            //          serial port detection: NO
            // 2) with config
            //     2.1) 1 device connected or is specified in command if multiple devices connected
            //          currentTargetDeviceName!=""
            //     2.1.1) currentHWAccessory != ""
            //            serial port detection: YES (to detect the specified one)
            //     2.1.2) currentHWAccessory == ""
            //            serial port detection: NO
            //                as other DaVinci instance may use the serial port.
            //                detection of serial port may cause exception.
            //     2.2) multiple devices is connected and no device is specified in command
            //          currentTargetDeviceName=="", currentHWAccessory == ""
            //          serial port detection: NO
            if (forceFFRDDetect
                || boost::algorithm::iequals(currentHWAccessory, "") == false)
            {
                if (boost::algorithm::iequals(currentHWAccessory, "Undefined"))
                    currentHWAccessory = "";
                status = ScanHWAccessories(currentHWAccessory);
                if (!DaVinciSuccess(status))
                    return status;
            }

            // 1) without config
            //     1.1) 1 device connected or is specified in command if multiple devices connected
            //          currentTargetDeviceName!="", currentPhysCamsIndex == 0
            //          physical camera detection: YES (to detect all ports)
            //     1.2) multiple devices is connected and no device is specified in command
            //          currentTargetDeviceName=="", currentPhysCamsIndex == -3
            //          physical camera detection: NO
            // 2) with config
            //     2.1) 1 device connected or is specified in command if multiple devices connected
            //          currentTargetDeviceName!=""
            //     2.1.1) currentPhysCamsIndex >= 0
            //            physical camera detection: YES (to detect the specified one)
            //     2.1.2) currentPhysCamsIndex <= -1 && currentPhysCamsIndex >= -3
            //            physical camera detection: NO
            //                as other DaVinci instance may use the high speed camera.
            //                detection of high speed camera may cause other test stopping.
            //     2.2) multiple devices is connected and no device is specified in command
            //          currentTargetDeviceName=="", currentPhysCamsIndex == -3
            //          physical camera detection: NO
            // 3) no device is connected and device agent is disabled
            //          physical camera detection: YES (to detect all ports)
            if (currentPhysCamsIndex >= 0 || GetEnableTargetDeviceAgent() == false)
            {
                status = ScanCaptureDevices(currentPhysCamsIndex);
            }
        }

        return status;
    }

    DaVinciStatus DeviceManager::ParseConfigRoot(const string &configFile, const DOMDocument *doc)
    {
        DOMElement *rootNode = doc->getDocumentElement();
        string rootName = XMLStrToStr(rootNode->getNodeName());
        if (!boost::algorithm::iequals(rootName, ConfigRoot))
        {
            return errc::invalid_argument;
        }
        DOMNodeList *firstLevelNodes = rootNode->getChildNodes();
        for (XMLSize_t i = 0; i < firstLevelNodes->getLength(); i++)
        {
            DOMNode *firstLevelNode = firstLevelNodes->item(i);
            string nodeName = XMLStrToStr(firstLevelNode->getNodeName());
            if (boost::algorithm::iequals(nodeName, ConfigUseGpuAcceleration))
            {
                if (boost::algorithm::iequals(XMLStrToStr(firstLevelNode->getTextContent()), "true"))
                {
                    if (HasCudaSupport())
                        EnableGpuAcceleration(true);
                }
                else
                {
                    // by default, gpu is disabled.
                    // if it's set to disabled here, we could not enable gpu by command parameter.
                    // EnableGpuAcceleration(false);
                }
            }
            else if (boost::algorithm::iequals(nodeName, ConfigAudioRecordDevice))
            {
                audioRecordDevice = XMLStrToStr(firstLevelNode->getTextContent());
            }
            else if (boost::algorithm::iequals(nodeName, ConfigAudioPlayDevice))
            {
                audioPlayDevice = XMLStrToStr(firstLevelNode->getTextContent());
            }
            else if (boost::algorithm::iequals(nodeName, ConfigAndroidDevices))
            {
                DaVinciStatus status = ParseConfigAndroid(configFile, firstLevelNode);
                if (!DaVinciSuccess(status))
                {
                    return status;
                }
            }
            else if (boost::algorithm::iequals(nodeName, ConfigWindowsDevices))
            {
                DaVinciStatus status = ParseConfigWindows(configFile, firstLevelNode);
                if (!DaVinciSuccess(status))
                {
                    return status;
                }
            }
            else if (boost::algorithm::iequals(nodeName, ConfigChromeDevices))
            {
                DaVinciStatus status = ParseConfigChrome(configFile, firstLevelNode);
                if (!DaVinciSuccess(status))
                {
                    return status;
                }
            }
            else if (!boost::algorithm::iequals(nodeName, "#text"))
            {
                DAVINCI_LOG_WARNING << "Unsupported first level node '" << nodeName << "' in " << configFile;
            }
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::ParseConfigTargetDevice(const boost::shared_ptr<TargetDevice> &device, const DOMNodeList *attrList)
    {
        for (XMLSize_t attrIdx = 0; attrIdx < attrList->getLength(); attrIdx++)
        {
            DOMNode *attrNode = attrList->item(attrIdx);
            string attrNodeName = XMLStrToStr(attrNode->getNodeName());
            string attrNodeText = XMLStrToStr(attrNode->getTextContent());
            if (boost::algorithm::iequals(attrNodeName, ConfigIdentifiedByIp))
            {
                // TODO: do not access var directly
                device->identifiedByIp = true;
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigAudioRecordDevice))
            {
                device->SetAudioRecordDevice(attrNodeText);
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigAudioPlayDevice))
            {
                device->SetAudioPlayDevice(attrNodeText);
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigMultiLayerSupported))
            {
                // TODO: do not access var directly
                if (boost::algorithm::iequals(attrNodeText, "true"))
                {
                    device->multiLayerSupported = true;
                }
                else
                {
                    device->multiLayerSupported = false;
                }
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigHeight))
            {
                int height;
                if (TryParse(attrNodeText, height))
                {
                    device->SetDeviceHeight(height);
                }
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigWidth))
            {
                int width;
                if (TryParse(attrNodeText, width))
                {
                    device->SetDeviceWidth(width);
                }
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigScreenSource))
            {
                device->SetScreenSource(StringToScreenSource(attrNodeText));
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigCalibrateData))
            {
                device->SetCalibrationData(attrNodeText);
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigCameraSetting))
            {
                device->SetCameraSetting(attrNodeText);
            }
            else if (boost::algorithm::iequals(attrNodeName, ConfigHwAccessoryController)
                || boost::algorithm::iequals(attrNodeName, ConfigPortOfFFRD))
            {
                string hwAccessoryString = attrNodeText;
                if (boost::algorithm::iequals(hwAccessoryString, "Undefined"))
                    hwAccessoryString = "";

                device->SetHWAccessoryController(hwAccessoryString);
            }
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::ParseConfigAndroid(const string &configFile, const DOMNode *devicesNode)
    {
        DOMNodeList *nodeList = devicesNode->getChildNodes();
        for (XMLSize_t i = 0; i < nodeList->getLength(); i++)
        {
            DOMNode *node = nodeList->item(i);
            string nodeName = XMLStrToStr(node->getNodeName());
            if (boost::algorithm::iequals(nodeName, ConfigAndroidTargetDevice))
            {
                boost::shared_ptr<AndroidTargetDevice> androidDevice;
                DOMNodeList *attrList = node->getChildNodes();
                for (XMLSize_t attrIdx = 0; attrIdx < attrList->getLength(); attrIdx++)
                {
                    DOMNode *attrNode = attrList->item(attrIdx);
                    string attrNodeName = XMLStrToStr(attrNode->getNodeName());
                    if (boost::algorithm::iequals(attrNodeName, "name"))
                    {
                        androidDevice = boost::shared_ptr<AndroidTargetDevice>(new AndroidTargetDevice(XMLStrToStr(attrNode->getTextContent())));
                        break;
                    }
                }
                if (androidDevice == nullptr)
                {
                    DAVINCI_LOG_ERROR << "Unable to find the mandatory name node";
                    return errc::invalid_argument;
                }
                DaVinciStatus status = ParseConfigTargetDevice(boost::dynamic_pointer_cast<TargetDevice>(androidDevice), attrList);
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Unable to parse common target device configuration";
                    return status;
                }
                for (XMLSize_t attrIdx = 0; attrIdx < attrList->getLength(); attrIdx++)
                {
                    DOMNode *attrNode = attrList->item(attrIdx);
                    string attrNodeName = XMLStrToStr(attrNode->getNodeName());
                    string attrNodeText = XMLStrToStr(attrNode->getTextContent());
                    if (boost::algorithm::iequals(attrNodeName, "qagentPort"))
                    {
                        unsigned short port;
                        if (TryParse(attrNodeText, port))
                        {
                            androidDevice->SetQAgentPort(port);
                        }
                    }
                    else if (boost::algorithm::iequals(attrNodeName, "magentPort"))
                    {
                        unsigned short port;
                        if (TryParse(attrNodeText, port))
                        {
                            androidDevice->SetMAgentPort(port);
                        }
                    }
                    // for backward compatibility
                    else if (boost::algorithm::iequals(attrNodeName, "monkeyPort"))
                    {
                        unsigned short port;
                        if (TryParse(attrNodeText, port))
                        {
                            androidDevice->SetMAgentPort(port);
                        }
                    }
                    else if (boost::algorithm::iequals(attrNodeName, ConfigHyperSoftCamPort))
                    {
                        unsigned short port;
                        if (TryParse(attrNodeText, port))
                        {
                            androidDevice->SetHyperSoftCamPort(port);
                        }
                    }
                }
                {
                    bool inTargetDevices = false;
                    boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
                    for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
                    {
                        auto theAndroidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                        if (theAndroidDevice != nullptr && theAndroidDevice->GetDeviceName() == androidDevice->GetDeviceName())
                        {
                            inTargetDevices = true;
                            break;
                        }
                    }
                    if (inTargetDevices == false)
                        targetDevices.push_back(androidDevice);
                }
            }
            else if (!boost::algorithm::iequals(nodeName, "#text"))
            {
                DAVINCI_LOG_WARNING << "Unsupported node '" << nodeName << "' under AndroidDevices in " << configFile;
            }
        }
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::ParseConfigWindows(const string &configFile, const DOMNode *devicesNode)
    {
        /* TODO: add windows device
        DOMNodeList *nodeList = devicesNode->getChildNodes();
        for (XMLSize_t i = 0; i < nodeList->getLength(); i++)
        {
        DOMNode *node = nodeList->item(i);
        string nodeName = XMLStrToStr(node->getNodeName());
        if (boost::algorithm::iequals(nodeName, "WindowsTargetDevice"))
        {
        DOMNode *nameNode = node->getFirstChild();
        if (nameNode == nullptr ||
        !boost::algorithm::iequals(XMLStrToStr(nameNode->getNodeName()), "name"))
        {
        DAVINCI_LOG_WARNING << "Mandatory first node 'name' not found under WindowsTargetDevice in " << configFile;
        continue;
        }
        auto windowsDevice = boost::shared_ptr<WindowsTargetDevice>(new WindowsTargetDevice(XMLStrToStr(nameNode->getTextContent())));
        DOMNodeList *attrList = node->getChildNodes();
        DaVinciStatus status = ParseConfigTargetDevice(boost::dynamic_pointer_cast<TargetDevice>(windowsDevice), attrList);
        if (!DaVinciSuccess(status))
        {
        DAVINCI_LOG_ERROR << "Unable to parse common target device configuration";
        return status;
        }
        }
        else
        {
        DAVINCI_LOG_WARNING << "Unsupported node '" << nodeName << "' under WindowsDevices in " << configFile;
        }
        }
        */
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::ParseConfigChrome(const string &configFile, const DOMNode *devicesNode)
    {
        /* TODO: add chrome device
        DOMNodeList *nodeList = devicesNode->getChildNodes();
        for (XMLSize_t i = 0; i < nodeList->getLength(); i++)
        {
        DOMNode *node = nodeList->item(i);
        string nodeName = XMLStrToStr(node->getNodeName());
        if (boost::algorithm::iequals(nodeName, "ChromeTargetDevice"))
        {
        DOMNode *nameNode = node->getFirstChild();
        if (nameNode == nullptr ||
        !boost::algorithm::iequals(XMLStrToStr(nameNode->getNodeName()), "name"))
        {
        DAVINCI_LOG_WARNING << "Mandatory first node 'name' not found under ChromeTargetDevice in " << configFile;
        continue;
        }
        auto chromeDevice = boost::shared_ptr<ChromeTargetDevice>(new ChromeTargetDevice(XMLStrToStr(nameNode->getTextContent())));
        DOMNodeList *attrList = node->getChildNodes();
        DaVinciStatus status = ParseConfigTargetDevice(boost::dynamic_pointer_cast<TargetDevice>(chromeDevice), attrList);
        if (!DaVinciSuccess(status))
        {
        DAVINCI_LOG_ERROR << "Unable to parse common target device configuration";
        return status;
        }
        }
        else
        {
        DAVINCI_LOG_WARNING << "Unsupported node '" << nodeName << "' under ChromeDevices in " << configFile;
        }
        }
        */
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::LoadConfig(const string &configFile)
    {
        auto parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        auto errorHandler = boost::shared_ptr<ErrorHandler>(new HandlerBase());
        parser->setErrorHandler(errorHandler.get());
        try
        {
            parser->parse(configFile.c_str());
            return ParseConfigRoot(configFile, parser->getDocument());
        }
        catch (const XMLException& ex)
        {
            char *message = XMLString::transcode(ex.getMessage());
            DAVINCI_LOG_ERROR << "XMLException (" << message << ") when parsing DaVinci configuration: " << configFile;
            XMLString::release(&message);
            return errc::invalid_argument;
        }
        catch (const DOMException& ex)
        {
            char *message = XMLString::transcode(ex.getMessage());
            DAVINCI_LOG_ERROR << "DOMException (" << message << ") when parsing DaVinci configuration: " << configFile;
            XMLString::release(&message);
            return errc::invalid_argument;
        }
        catch (...)
        {
            DAVINCI_LOG_ERROR << "Unknown error when parsing DaVinci configuration: " << configFile;
            return errc::invalid_argument;
        }
    }

    void DeviceManager::AppendRootElements(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &root)
    {
        auto useGpuAcceleration = CreateDOMElementNullDeleter(doc, ConfigUseGpuAcceleration);
        if (GpuAccelerationEnabled())
        {
            useGpuAcceleration->setTextContent(StrToXMLStr("true").get());
        }
        else
        {
            useGpuAcceleration->setTextContent(StrToXMLStr("false").get());
        }
        root->appendChild(useGpuAcceleration.get());

        auto recordDevice = CreateDOMElementNullDeleter(doc, ConfigAudioRecordDevice);
        recordDevice->setTextContent(StrToXMLStr(audioRecordDevice).get());
        root->appendChild(recordDevice.get());

        auto playDevice = CreateDOMElementNullDeleter(doc, ConfigAudioPlayDevice);
        playDevice->setTextContent(StrToXMLStr(audioPlayDevice).get());
        root->appendChild(playDevice.get());

        auto androidElements = CreateDOMElementNullDeleter(doc, ConfigAndroidDevices);
        root->appendChild(androidElements.get());
        AppendAndroidDevices(doc, androidElements);
        auto windowsElements = CreateDOMElementNullDeleter(doc, ConfigWindowsDevices);
        root->appendChild(windowsElements.get());
        AppendWindowsDevices(doc, windowsElements);

        auto chromeElements = CreateDOMElementNullDeleter(doc, ConfigChromeDevices);
        root->appendChild(chromeElements.get());
        AppendChromeDevices(doc, chromeElements);
    }

    void DeviceManager::AppendTargetDevice(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &deviceElement, const boost::shared_ptr<TargetDevice> &targetDevice)
    {
        auto name = CreateDOMElementNullDeleter(doc, "name");
        name->setTextContent(StrToXMLStr(targetDevice->GetDeviceName()).get());
        deviceElement->appendChild(name.get());

        auto identifiedByIp = CreateDOMElementNullDeleter(doc, ConfigIdentifiedByIp);
        if (targetDevice->IsIdentifiedByIp())
        {
            identifiedByIp->setTextContent(StrToXMLStr("true").get());
        }
        else
        {
            identifiedByIp->setTextContent(StrToXMLStr("false").get());
        }
        deviceElement->appendChild(identifiedByIp.get());

        auto recordDevice = CreateDOMElementNullDeleter(doc, ConfigAudioRecordDevice);
        recordDevice->setTextContent(StrToXMLStr(targetDevice->GetAudioRecordDevice()).get());
        deviceElement->appendChild(recordDevice.get());

        auto playDevice = CreateDOMElementNullDeleter(doc, ConfigAudioPlayDevice);
        playDevice->setTextContent(StrToXMLStr(targetDevice->GetAudioPlayDevice()).get());
        deviceElement->appendChild(playDevice.get());

        auto multiLayerSupported = CreateDOMElementNullDeleter(doc, ConfigMultiLayerSupported);
        if (targetDevice->GetCurrentMultiLayerMode())
        {
            multiLayerSupported->setTextContent(StrToXMLStr("true").get());
        }
        else
        {
            multiLayerSupported->setTextContent(StrToXMLStr("false").get());
        }
        deviceElement->appendChild(multiLayerSupported.get());

        auto height = CreateDOMElementNullDeleter(doc, ConfigHeight);
        height->setTextContent(StrToXMLStr(boost::lexical_cast<string>(targetDevice->GetDeviceHeight())).get());
        deviceElement->appendChild(height.get());

        auto width = CreateDOMElementNullDeleter(doc, ConfigWidth);
        width->setTextContent(StrToXMLStr(boost::lexical_cast<string>(targetDevice->GetDeviceWidth())).get());
        deviceElement->appendChild(width.get());

        auto screen = CreateDOMElementNullDeleter(doc, ConfigScreenSource);
        screen->setTextContent(StrToXMLStr(ScreenSourceToString(targetDevice->GetScreenSource())).get());
        deviceElement->appendChild(screen.get());
        auto calibrateData = CreateDOMElementNullDeleter(doc, ConfigCalibrateData);
        calibrateData->setTextContent(StrToXMLStr(targetDevice->GetCalibrationData()).get());
        deviceElement->appendChild(calibrateData.get());

        auto cameraSetting = CreateDOMElementNullDeleter(doc, ConfigCameraSetting);
        cameraSetting->setTextContent(StrToXMLStr(targetDevice->GetCameraSetting()).get());
        deviceElement->appendChild(cameraSetting.get());

        auto hwAcc = CreateDOMElementNullDeleter(doc, ConfigHwAccessoryController);
        hwAcc->setTextContent(StrToXMLStr(targetDevice->GetHWAccessoryController()).get());
        deviceElement->appendChild(hwAcc.get());
    }

    void DeviceManager::AppendAndroidDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &androidElements)
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto targetDevice : targetDevices)
        {
            boost::shared_ptr<AndroidTargetDevice> androidTargetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(targetDevice);
            if (androidTargetDevice != nullptr)
            {
                auto androidElement = CreateDOMElementNullDeleter(doc, ConfigAndroidTargetDevice);
                androidElements->appendChild(androidElement.get());
                AppendTargetDevice(doc, androidElement, targetDevice);
                auto deviceModel = CreateDOMElementNullDeleter(doc, ConfigDeviceModel);
                deviceModel->setTextContent(StrToXMLStr((androidTargetDevice->model)).get());
                androidElement->appendChild(deviceModel.get());
                auto qagentPort = CreateDOMElementNullDeleter(doc, ConfigQagentPort);
                qagentPort->setTextContent(StrToXMLStr(boost::lexical_cast<string>(androidTargetDevice->qagentPort)).get());
                androidElement->appendChild(qagentPort.get());
                auto magentPort = CreateDOMElementNullDeleter(doc, ConfigMagentPort);
                magentPort->setTextContent(StrToXMLStr(boost::lexical_cast<string>(androidTargetDevice->magentPort)).get());
                androidElement->appendChild(magentPort.get());
                if (androidTargetDevice->GetScreenSource() == ScreenSource::HyperSoftCam)
                {
                    auto hyperSoftCamPort = CreateDOMElementNullDeleter(doc, ConfigHyperSoftCamPort);
                    hyperSoftCamPort->setTextContent(StrToXMLStr(boost::lexical_cast<string>(androidTargetDevice->hyperSoftCamPort)).get());
                    androidElement->appendChild(hyperSoftCamPort.get());
                }
            }
        }
    }

    void DeviceManager::AppendWindowsDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &windowsElements)
    {
        // TODO: support windows configuration
    }

    void DeviceManager::AppendChromeDevices(const boost::shared_ptr<DOMDocument> &doc, const boost::shared_ptr<DOMElement> &chromeElements)
    {
        // TODO: support chrome configuration
    }

    DaVinciStatus DeviceManager::SaveConfig(const string &configFile)
    {
        auto domImpl = boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(StrToXMLStr("Core").get()), boost::null_deleter());
        auto doc = boost::shared_ptr<DOMDocument>(domImpl->createDocument(0, StrToXMLStr(ConfigRoot).get(), 0), &XmlDocumentPtrDeleter<DOMDocument>);
        auto root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);
        AppendRootElements(doc, root);
        return WriteDOMDocumentToXML(domImpl, doc, configFile);
    }

    string DeviceManager::GetCurrentDeviceName()
    {
        boost::shared_ptr<TargetDevice> device = currentTargetDevice;
        if (device != nullptr)
            return device->GetDeviceName();
        else
            return "";
    }

    void DeviceManager::UpdateDeviceOrder(const string &curDevName)
    {
        int index = 0;
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto device = targetDevices.begin(); device != targetDevices.end(); device++)
        {
            auto targetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*device);
            if ((targetDevice != nullptr) && (targetDevice->GetDeviceName() == curDevName))
            {
                if (index != 0)
                    swap(targetDevices[0], targetDevices[index]);
                break;
            }
            index++;
        }
    }

    DaVinciStatus DeviceManager::ScanTargetDevices(String & currentTargetDeviceName)
    {
        // discover Android devices
        vector<string> connectedDevices;
        bool inTargetDevices = false;

        DaVinciStatus status = AndroidTargetDevice::AdbDevices(connectedDevices);
        if (!DaVinciSuccess(status))
        {
            return status;
        }

        // Update connected/new added devices' status
        for (auto connectedDevice = connectedDevices.begin(); connectedDevice != connectedDevices.end(); connectedDevice++)
        {
            if ((boost::algorithm::iequals(currentTargetDeviceName, "") == false)
                && (boost::algorithm::iequals(currentTargetDeviceName, *connectedDevice) == false))
            {
                continue;
            }

            inTargetDevices = false;
            boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
            for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
            {
                auto androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                if (androidDevice != nullptr && androidDevice->GetDeviceName() == *connectedDevice)
                {
                    inTargetDevices = true;

                    // set the device status to ready for connection if we discover it from ADB devices and
                    // it was set offline originally
                    if (IsDeviceConnectedByOthers(DeviceCategory::Target, androidDevice->GetDeviceName()))
                            androidDevice->SetDeviceStatus(DeviceConnectionStatus::ConnectedByOthers);
                    else
                    {
                        if (androidDevice->GetDeviceStatus() != DeviceConnectionStatus::Connected)
                            androidDevice->SetDeviceStatus(DeviceConnectionStatus::ReadyForConnection);
                        // TODO: notify GUI side
                    }

                    break;
                }
            }

            if (!inTargetDevices)
            {
                // if it is not in the target device list, we add it.
                auto targetDevice = boost::shared_ptr<AndroidTargetDevice>(new AndroidTargetDevice(*connectedDevice));

                boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);

                if (IsDeviceConnectedByOthers(DeviceCategory::Target, targetDevice->GetDeviceName()))
                    targetDevice->SetDeviceStatus(DeviceConnectionStatus::ConnectedByOthers);
                else
                {
                    if (targetDevice->GetDeviceStatus() != DeviceConnectionStatus::Connected)
                        targetDevice->SetDeviceStatus(DeviceConnectionStatus::ReadyForConnection);
                }

                targetDevices.push_back(targetDevice);

                targetDevice->AllocateAgentPort(targetDevices);
                // TODO: notify GUI side
            }
        }

        // Update removed device's status to offline.
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
        {
            inTargetDevices = false;

            for (auto connectedDevice = connectedDevices.begin(); connectedDevice != connectedDevices.end(); connectedDevice++)
            {
                auto androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                if (androidDevice != nullptr && androidDevice->GetDeviceName() == *connectedDevice)
                {
                    inTargetDevices = true;
                    break;
                }
            }

            if (!inTargetDevices)
            {
                auto androidDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                if (androidDevice != nullptr)
                    androidDevice->SetDeviceStatus(DeviceConnectionStatus::Offline);
            }
        }

        if ((currentTargetDeviceName == "") && (connectedDevices.size() == 1))
            currentTargetDeviceName = connectedDevices[0];

        if (IsDeviceConnectedByOthers(DeviceCategory::Target, currentTargetDeviceName))
        {
            DAVINCI_LOG_ERROR << "The target device:" << currentTargetDeviceName <<" is connected by another DaVinci instance. Please check the running DaVinci instances and the configuration file!";
            return errc::operation_not_permitted;
        }

        return DaVinciStatusSuccess;
        // TODO: scan Windows and Chrome devices
    }

    DaVinciStatus DeviceManager::ScanCaptureDevices(int currentPhysCamsIndex)
    {
        return PhysicalCameraCapture::DetectPhysicalCameras(physicalCams, currentPhysCamsIndex);
    }

    DaVinciStatus DeviceManager::ScanHWAccessories(String currentHWAccessory)
    {
        DAVINCI_LOG_INFO << "----------------- Scan HWAccessories Time Report Start -----------------";

        //Keep the status of connected hwAccessoryControllers and serial ports to avoid exception!
        boost::lock_guard<boost::recursive_mutex> lock(hwAccessoriesMutex);
        if(hwAccessoryControllers.empty() == false)
        {
            int connected_hwAccessory_num = (int)hwAccessoryControllers.size();
            vector<string> connected_ComPorts = vector<string>();
            for (int i = 0; i < connected_hwAccessory_num; i++)
            {
                DAVINCI_LOG_INFO << string("        ") << hwAccessoryControllers[i]->comName << string(" has been opened!");
                connected_ComPorts.push_back(hwAccessoryControllers[i]->comName);
            }

            vector<boost::shared_ptr<HWAccessoryController>> new_hwAccessoryControllers;
            new_hwAccessoryControllers = HWAccessoryController::DetectNewSerialPorts(connected_ComPorts);
            int new_hwAccessory_num = (int)new_hwAccessoryControllers.size();
            for (int j = 0; j < new_hwAccessory_num; j++)
            {
                hwAccessoryControllers.push_back(new_hwAccessoryControllers[j]);
            }
        }
        else
        {
            hwAccessoryControllers = HWAccessoryController::DetectAllSerialPorts(currentHWAccessory);
        }

        DAVINCI_LOG_INFO << "----------------- Scan HWAccessories Time Report End -----------------";

        return DaVinciStatusSuccess;
    }

    bool DeviceManager::HasCudaSupport()
    {
        if (!testedCudaSupport)
        {
            hasCudaSupport = cv::gpu::getCudaEnabledDeviceCount() > 0;
            testedCudaSupport = true;
        }
        return hasCudaSupport;
    }

    void DeviceManager::EnableGpuAcceleration(bool enable)
    {
        gpuAccelerationEnabled = enable;
    }

    bool DeviceManager::GpuAccelerationEnabled()
    {
        return gpuAccelerationEnabled;
    }

    string DeviceManager::QueryGpuInfo()
    {
        int deviceId = cv::gpu::getDevice();
        if (deviceId >= 0)
        {
            cv::gpu::DeviceInfo deviceInfo(deviceId);
            ostringstream info;

            info << "GPU: " <<deviceInfo.majorVersion() << "." 
                << deviceInfo.minorVersion() <<" " 
                << ((deviceInfo.isCompatible() == true) ? "Compatible" : "Incompatible")
                <<endl;
            return info.str();
        }
        else
        {
            return "N/A";
        }
    }

    DaVinciStatus DeviceManager::EnableTargetDeviceAgent(bool enable)
    {
        enableTargetDeviceAgent = enable;
        if (!enable)
        {
            // put current device to local just in case currentTargetDevice
            // is being modified elsewhere.
            boost::shared_ptr<TargetDevice> device = currentTargetDevice;
            if (device != nullptr)
            {
                device->Disconnect();
            }
        }
        return DaVinciStatusSuccess;
    }

    bool DeviceManager::GetEnableTargetDeviceAgent()
    {
        return enableTargetDeviceAgent;
    }

    DaVinciStatus DeviceManager::InitTestDevices(const string &deviceName)
    {
        DaVinciStatus status = InitTargetDevice(deviceName);
        if (!DaVinciSuccess(status) && TestManager::Instance().IsConsoleMode())
        {
            return status;
        }
        status = InitHWAccessoryControllers();
        if (!DaVinciSuccess(status) && TestManager::Instance().IsConsoleMode())
        {
            return status;
        }
        status = InitCaptureDevice();
        return status;
    }

    DaVinciStatus DeviceManager::InitHostDevices()
    {
        auto current = GetCurrentTargetDevice();
        AudioManager::Instance().SetRecorderCallback(boost::bind(&TestManager::ProcessWave, &TestManager::Instance(), _1));
        AudioManager::Instance().InitAudioDevices(
            audioRecordDevice, audioPlayDevice,
            current != nullptr ? current->audioRecordDevice : "",
            current != nullptr ? current->audioPlayDevice : "");
        return DaVinciStatusSuccess;
    }

    void DeviceManager::ClearPhysicalCameraList()
    {
        for (auto captureDevice = physicalCams.begin(); captureDevice != physicalCams.end(); captureDevice++)
        {
            if ((*captureDevice) != nullptr)
            {
                (*captureDevice)->Stop();
                (*captureDevice) = nullptr;
            }
        }
        physicalCams.clear();
    }

    void DeviceManager::CloseCaptureDevices()
    {
        StopCaptureMonitor();
        {
            boost::lock_guard<boost::recursive_mutex> lock(captureDevicesMutex);
            ClearPhysicalCameraList();
            if (currentCaptureDevice != nullptr)
            {
                currentCaptureDevice->Stop();
                currentCaptureDevice = nullptr;
            }
        }
    }

    void DeviceManager::CloseTargetDevices()
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
        {
            if ((*targetDevice) != nullptr &&
                (*targetDevice)->GetDeviceStatus() == DeviceConnectionStatus::Connected)
            {
                (*targetDevice)->Disconnect();
                CheckAndUpdateGlobalStatus(DeviceCategory::Target, (*targetDevice)->GetDeviceName(), false);
                (*targetDevice) = nullptr;
            }
        }

        if (currentTargetDevice != nullptr)
        {
            currentTargetDevice->Disconnect();
            CheckAndUpdateGlobalStatus(DeviceCategory::Target, currentTargetDevice->GetDeviceName(), false);
            currentTargetDevice = nullptr;
        }
        targetDevices.clear();
    }

    void DeviceManager::CloseHWAccessoryControllers()
    {
        boost::lock_guard<boost::recursive_mutex> lock(hwAccessoriesMutex);
        for (auto hwAccessory = hwAccessoryControllers.begin(); hwAccessory != hwAccessoryControllers.end(); hwAccessory++)
        {
            if ((*hwAccessory) != nullptr)
            {
                (*hwAccessory)->Disconnect();
                (*hwAccessory) = nullptr;
            }
        }
        if (currentHWAccessoryController != nullptr)
        {
            currentHWAccessoryController->Disconnect();
            currentHWAccessoryController = nullptr;
        }
        hwAccessoryControllers.clear();
    }

    DaVinciStatus DeviceManager::CloseDevices()
    {
        AudioManager::Instance().CloseAudioDevices();
        AudioManager::Instance().SetRecorderCallback(AudioPipe::RecorderCallback());
        CloseCaptureDevices();
        // Close HW accessories
        CloseHWAccessoryControllers();
        CloseTargetDevices();
        // TODO: close audio devices
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::InitTargetDevice(const string &deviceName)
    {
        boost::shared_ptr<TargetDevice> device = nullptr;
        {
            boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
            if (deviceName.empty())
            {
                boost::shared_ptr<TargetDevice> onlyAvailableDevice = FindOnlyAvailableDevice();
                if ((onlyAvailableDevice != nullptr) && \
                    (!IsDeviceConnectedByOthers(DeviceCategory::Target, onlyAvailableDevice->GetDeviceName())))
                {
                    device = onlyAvailableDevice;
                }
                else
                {
                    if (TestManager::Instance().IsConsoleMode() && GetEnableTargetDeviceAgent())
                    {
                        DAVINCI_LOG_ERROR << "Unable to get the default test device. Please use option [-device] to specific device name and try again.";
                        return errc::no_such_device;
                    }
                }
            }
            else
            {
                device = GetTargetDevice(deviceName);
                if (TestManager::Instance().IsConsoleMode() && GetEnableTargetDeviceAgent())
                {
                    if ((device == nullptr) || \
                        (device->GetDeviceStatus() == DeviceConnectionStatus::Offline) || \
                        (IsDeviceConnectedByOthers(DeviceCategory::Target, device->GetDeviceName())))
                    {
                        DAVINCI_LOG_ERROR << "The device " << deviceName <<" not connected.";
                        return errc::no_such_device;
                    }
                }
            }

            // Disconnect current connected device when switch devices.
            if ((currentTargetDevice != nullptr) && (currentTargetDevice->GetDeviceName() != deviceName))
            {
                CheckAndUpdateGlobalStatus(DeviceCategory::Target, currentTargetDevice->GetDeviceName(), false);

                currentTargetDevice->Disconnect();
            }

            currentTargetDevice = device;
        }

        if (device != nullptr && GetEnableTargetDeviceAgent()) 
        {
            if (CheckAndUpdateGlobalStatus(DeviceCategory::Target, device->GetDeviceName(), true))
            {
                DaVinciStatus status = device->Connect();
                if (!DaVinciSuccess(status))
                    return status;

                if (TestManager::Instance().systemDiagnostic != nullptr)
                    TestManager::Instance().systemDiagnostic->CheckDeviceInfo();
            }
            else
            {
                DAVINCI_LOG_ERROR << "The device " << device->GetDeviceName() <<" is connected by other applications.";
                return errc::connection_aborted;
            }
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::SetCaptureDeviceForTargetDevice(string deviceName, string captureName)
    {
        ScreenSource source = StringToScreenSource(captureName);
        if (source == ScreenSource::Undefined)
            return errc::invalid_argument;

        {
            boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
            for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
            {
                if (((*targetDevice) != nullptr)
                    && (boost::algorithm::iequals((*targetDevice)->GetDeviceName(), deviceName) == true))
                {

                    (*targetDevice)->SetScreenSource(source);
                    if (source == ScreenSource::HyperSoftCam)
                    {
                        boost::shared_ptr<AndroidTargetDevice> androidTargetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                        if ((androidTargetDevice != nullptr) && \
                            (androidTargetDevice->GetHyperSoftCamPort() == 0))
                        {
                            boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
                            androidTargetDevice->AllocateHyperSoftCamPort(targetDevices);
                        }
                    }
                    return DaVinciStatusSuccess;
                }
            }
        }
        return errc::no_such_device;
    }

    DaVinciStatus DeviceManager::SetHWAccessoryControllerForTargetDevice(string deviceName, string comName)
    {
        {
            boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
            for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
            {
                if (((*targetDevice) != nullptr)
                    && (boost::algorithm::iequals((*targetDevice)->GetDeviceName(), deviceName) == true))
                {
                    (*targetDevice)->SetHWAccessoryController(comName);
                    return DaVinciStatusSuccess;
                }
            }
        }
        return errc::no_such_device;
    }

    DaVinciStatus DeviceManager::InitCaptureDevice(string name)
    {
        if (boost::algorithm::iequals(name, "MAP"))
        {
            // TODO: support mapping camera
            DAVINCI_LOG_DEBUG << "Map Camera";
            return errc::not_supported;
        }
        else if (boost::algorithm::contains(name, "CameraIndex"))
        {
            vector<string> indexStrs;
            boost::split(indexStrs, name, boost::is_any_of("-"), boost::token_compress_on);

            if (indexStrs.size() == 2)
            {
                return InitCaptureDevice(StringToScreenSource(indexStrs[0]));
            }
            else
                return errc::invalid_argument;
        }
        else
            return InitCaptureDevice(StringToScreenSource(name));
    }

    DaVinciStatus DeviceManager::InitCaptureDevice(ScreenSource preferredSource, bool isUpdateScreenSource)
    {
        StopCaptureMonitor();
        boost::lock_guard<boost::recursive_mutex> lock(captureDevicesMutex);
        DaVinciStatus status;
        boost::shared_ptr<CaptureDevice> oldCapture = currentCaptureDevice;
        currentCaptureDevice = nullptr;
        if (oldCapture != nullptr)
        {
            status = oldCapture->Stop();
            if (!DaVinciSuccess(status))
            {
                return status;
            }
        }

        ScreenSource source = preferredSource;
        boost::shared_ptr<TargetDevice> targetDevice = currentTargetDevice;
        if (source == ScreenSource::Undefined)
        {
            if (targetDevice != nullptr)
            {
                source = targetDevice->GetScreenSource();
            }
            if (source == ScreenSource::Undefined)
            {
                if (physicalCams.empty() == false)
                {
                    // find the first available camera
                    source = IndexToScreenSource(0);
                }
                else if (targetDevice != nullptr && targetDevice->IsHyperSoftCamSupported())
                {
                    source = ScreenSource::HyperSoftCam;
                }
                else
                {
                    source = ScreenSource::Disabled;
                }
            }
        }
        else
        {
            if (targetDevice != nullptr)
            {
                if (source == ScreenSource::HyperSoftCam)
                {
                    if (!targetDevice->IsHyperSoftCamSupported())
                        source = ScreenSource::Disabled;
                    else
                    {
                        boost::shared_ptr<AndroidTargetDevice> androidTargetDevice
                            = boost::dynamic_pointer_cast<AndroidTargetDevice>(targetDevice);
                        if ((androidTargetDevice != nullptr)
                            && (androidTargetDevice->IsDevicesAttached() == false))
                            source = ScreenSource::Disabled;
                    }
                }
            }
            else
            {
                // TODO: check if we can set the specified physical camera
                source = ScreenSource::Disabled;
            }
        }

        boost::shared_ptr<CaptureDevice> captureDevice;
        switch (source)
        {
        case ScreenSource::HyperSoftCam:
            try
            {
                // we cannot turn to disabled mode here if device agent is disabled,
                // since launch time test disables device agent but it would use 
                // hypersoftcam mode to run.

                boost::shared_ptr<HyperSoftCamCapture> hyperSoftCamCapture = boost::shared_ptr<HyperSoftCamCapture>(new HyperSoftCamCapture(targetDevice->GetDeviceName()));
                captureDevice = boost::dynamic_pointer_cast<CaptureDevice>(hyperSoftCamCapture);
                if (hyperSoftCamCapture != nullptr)
                {
                    boost::shared_ptr<AndroidTargetDevice> androidTargetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(targetDevice);
                    if (androidTargetDevice != nullptr)
                    {
                        if (androidTargetDevice->GetHyperSoftCamPort() == 0)
                        {
                            androidTargetDevice->AllocateHyperSoftCamPort(targetDevices);
                        }
                        hyperSoftCamCapture->SetLocalPort((unsigned short)androidTargetDevice->GetHyperSoftCamPort());
                    }
                }
            }
            catch (...)
            {
                DAVINCI_LOG_ERROR << "Cannot create HyperSoftCam capture";
                captureDevice = boost::shared_ptr<CaptureDevice>(new PseudoCameraCapture());
                source = ScreenSource::Disabled;
            }
            break;
        case ScreenSource::Disabled:
            captureDevice = boost::shared_ptr<CaptureDevice>(new PseudoCameraCapture());
            break;
        default:
            bool found = false;
            if (source >= ScreenSource::CameraIndex0 && source <= ScreenSource::CameraIndex7)
            {
                int index = ScreenSourceToIndex(source);
                // The number of CameraIndex in screen source is the index of physcialCams array, not the camera index
                if (((int)physicalCams.size() > index) && (physicalCams[index] != nullptr))
                {
                    found = true;
                    captureDevice = physicalCams[index];
                    break;
                }
            }

            if (!found)
            {
                captureDevice = boost::shared_ptr<CaptureDevice>(new PseudoCameraCapture());
                source = ScreenSource::Disabled;
            }
            break;
        }

        assert(captureDevice != NULL);

        int capResLongerSidePxLen = TestManager::Instance().GetCapResLongerSidePxLen();
        if (capResLongerSidePxLen > 0 && targetDevice != NULL) {
            int deviceWidth = targetDevice->GetDeviceWidth();
            int deviceHeight = targetDevice->GetDeviceHeight();

            int capResOtherSidePxLen = CalPxLengthOfSide(capResLongerSidePxLen,
                deviceWidth,
                deviceHeight);
            if (capResOtherSidePxLen <= 0)
            {
                DAVINCI_LOG_ERROR << "Failed in calculating HyperSoftCam resolution. "
                    << "Device Width:" << deviceWidth << " "
                    << "Device Height:" << deviceHeight;
                return DaVinciStatus(errc::invalid_argument);
            }
            else
            {
                captureDevice->SetCaptureProperty(CV_CAP_PROP_FRAME_WIDTH, capResLongerSidePxLen);
                captureDevice->SetCaptureProperty(CV_CAP_PROP_FRAME_HEIGHT, capResOtherSidePxLen);
            }
        }

        if (TestManager::Instance().GetCapFPS() > 0)
        {
            captureDevice->SetCaptureProperty(CV_CAP_PROP_FPS, TestManager::Instance().GetCapFPS());
        }

        status = captureDevice->Start();
        if (!DaVinciSuccess(status))
        {
            return status;
        }

        if (isUpdateScreenSource && (targetDevice != nullptr))
        {
            targetDevice->SetScreenSource(source);
        }
        StartCaptureMonitor();
        currentCaptureDevice = captureDevice;
        CaptureDevice::FrameHandlerCallback cb = boost::bind(
            &DeviceManager::ProcessFrameEntry, this, _1);
        currentCaptureDevice->SetFrameHandler(cb);
        return DaVinciStatusSuccess;
    }

    DaVinciStatus DeviceManager::InitHWAccessoryControllers(const string &comPort)
    {
        boost::lock_guard<boost::recursive_mutex> lock(hwAccessoriesMutex);
        DaVinciStatus status;
        //boost::shared_ptr<HWAccessoryController> oldHWAccessoryController = currentHWAccessoryController;

        //Disconnect action will lead the control of HWAccessory Controller lost!
        /*
        if (oldHWAccessoryController != nullptr)
        {
        status = oldHWAccessoryController->Disconnect();
        if (!DaVinciSuccess(status))
        {
        return status;
        }
        }
        */

        currentHWAccessoryController = nullptr;

        string tempComPort = comPort;

        if ((tempComPort == "") && (currentTargetDevice != nullptr))
        { 
            tempComPort = currentTargetDevice->GetHWAccessoryController();
        }
        if (tempComPort == "" || tempComPort == "Undefined")
        {
            if(hwAccessoryControllers.empty() == false && currentTargetDevice == nullptr)
            {
                currentHWAccessoryController = hwAccessoryControllers[0];
            }
        }
        else
        {
            currentHWAccessoryController = DeviceManager::GetHWAccessoryControllerDevice(tempComPort);
        }

        if (currentHWAccessoryController != nullptr)
        {
            currentHWAccessoryController->Connect();
            /*
            currentHWAccessoryController->GetFFRDHardwareInfo(0);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(1);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(2);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(3);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(4);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(5);      //for debug!
            currentHWAccessoryController->GetFFRDHardwareInfo(6);      //for debug!
            */
        }

        return DaVinciStatusSuccess;
    }

    bool DeviceManager::IsAndroidDevice(const boost::shared_ptr<TargetDevice> &device) const
    {
        return boost::dynamic_pointer_cast<AndroidTargetDevice>(device) != nullptr;
    }

    boost::shared_ptr<TargetDevice> DeviceManager::FindOnlyAvailableDevice()
    {
        boost::shared_ptr<TargetDevice> result = nullptr;
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto device = targetDevices.begin(); device != targetDevices.end(); device++)
        {
            if (((*device)->GetDeviceStatus() != DeviceConnectionStatus::Offline) && \
                (!IsDeviceConnectedByOthers(DeviceCategory::Target, (*device)->GetDeviceName())))
            {
                if (result == nullptr)
                {
                    result = (*device);
                }
                //else
                //{
                //    return nullptr;
                //}
            }
        }
        return result;
    }

    boost::shared_ptr<TargetDevice> DeviceManager::GetTargetDevice(const string &name)
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto device = targetDevices.begin(); device != targetDevices.end(); device++)
        {
            if (name == (*device)->GetDeviceName())
            {
                return *device;
            }
        }
        return nullptr;
    }

    vector<boost::shared_ptr<TargetDevice>>& DeviceManager::GetTargetDeviceList()
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        return targetDevices;
    }


    boost::shared_ptr<HWAccessoryController> DeviceManager::GetHWAccessoryControllerDevice(const string &name)
    {
        boost::lock_guard<boost::recursive_mutex> lock(hwAccessoriesMutex);
        for (auto hwAccessory = hwAccessoryControllers.begin(); hwAccessory != hwAccessoryControllers.end(); hwAccessory++)
        {
            if (name == (*hwAccessory)->GetComPortName())
            {
                return *hwAccessory;
            }
        }
        return nullptr;
    }

    bool DeviceManager::UsingHyperSoftCam()
    {
        return boost::dynamic_pointer_cast<HyperSoftCamCapture>(currentCaptureDevice) != nullptr;
    }

    bool DeviceManager::UsingDisabledCam()
    {
        return boost::dynamic_pointer_cast< PseudoCameraCapture>(currentCaptureDevice) != nullptr;
    }

    string DeviceManager::GetCurrentCaptureStr()
    {
        boost::shared_ptr<CaptureDevice> capture = currentCaptureDevice;
        if (capture != nullptr)
        {
            return capture->GetCameraName();
        }
        else
        {
            return "";
        }
    }

    CaptureDevice::Preset DeviceManager::GetCurrentCapturePreset()
    {
        boost::shared_ptr<CaptureDevice> capture = currentCaptureDevice;
        if (capture != nullptr)
        {
            return capture->GetPreset();
        }
        else
        {
            return CaptureDevice::Preset::HighResolution;
        }
    }

    string DeviceManager::GetAudioRecordDevice() const
    {
        return audioRecordDevice;
    }

    string DeviceManager::GetAudioPlayDevice() const
    {
        return audioPlayDevice;
    }

    vector<string> DeviceManager::GetTargetDeviceNames()
    {
        vector<string> deviceList;
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        for (auto targetDevice = targetDevices.begin(); targetDevice != targetDevices.end(); targetDevice++)
        {
            if ((*targetDevice) != nullptr)
                deviceList.push_back((*targetDevice)->GetDeviceName());
        }
        return deviceList;
    }

    vector<string> DeviceManager::GetCaptureDeviceNames()
    {
        vector<string> deviceList;
        {
            deviceList.push_back("Disabled");
            deviceList.push_back("HyperSoftcam");
            for (int index = (int)ScreenSource::CameraIndex0; index <=(int)ScreenSource::CameraIndex7; index++)
            {
                boost::lock_guard<boost::recursive_mutex> lock(captureDevicesMutex);
                // The number of CameraIndex in screen source is the index of physcialCams array, not the camera index
                if (((int)physicalCams.size() > index) && (physicalCams[index] != nullptr))
                {
                    deviceList.push_back(
                        ScreenSourceToString(IndexToScreenSource(index)) + "-" + physicalCams[index]->GetCameraName());
                }
                else
                {
                    deviceList.push_back(
                        ScreenSourceToString(IndexToScreenSource(index)));
                }
            }
            deviceList.push_back("MAP");
        }
        return deviceList;
    }

    vector<string> DeviceManager::GetHWAccessoryControllerNames()
    {
        vector<string> deviceList;
        {
            boost::lock_guard<boost::recursive_mutex> lock(hwAccessoriesMutex);
            for (auto ffrdController = hwAccessoryControllers.begin(); ffrdController != hwAccessoryControllers.end(); ffrdController++)
            {
                if ((*ffrdController) != nullptr)
                    deviceList.push_back((*ffrdController)->GetComPortName());
            }
        }
        return deviceList;
    }

    void DeviceManager::ProcessFrameEntry(boost::shared_ptr<CaptureDevice> captureDevice)
    {
        captureLiveWatch.Reset();
        TestManager::Instance().ProcessFrame(captureDevice);
        captureLiveWatch.Start();
    }

    void DeviceManager::HandleDiedCapture(bool needRestart)
    {
        boost::lock_guard<boost::recursive_mutex> lock(captureDevicesMutex);
        // we expect the capture device always exists and pseudo capture never fails
        bool invalidState = (currentCaptureDevice == nullptr)
            || (boost::dynamic_pointer_cast<PseudoCameraCapture>(currentCaptureDevice) != nullptr);
        assert(!invalidState);
        if (invalidState)
        {
            DAVINCI_LOG_FATAL << "Capture device not exist or pseudo capture failed.";
            captureLiveWatch.Reset();
            return;
        }

        bool needUpdate = false;
        bool success = false;
        if (needRestart)
        {
            // we will restart the camera if processing frame is timeout.
            DaVinciStatus ret = currentCaptureDevice->Restart();
            if (DaVinciSuccess(ret))
            {
                DAVINCI_LOG_INFO << "Restart camera successfully!";
                success = true;
            }
            else
            {
                if (ret == DaVinciStatus(errc::not_supported)
                    || ret == DaVinciStatus(errc::state_not_recoverable))
                {
                    needUpdate = true;
                }
            }
        }

        if (!success)
        {
            ScreenSource nextScreenSource = ScreenSource::HyperSoftCam;
            if (currentTargetDevice != nullptr)
            {
                // if current screen source is physical camera, then switch to hypersoftcam
                // if current screen source is hypersoftcam, then switch to disable
                // otherwise, print error information
                ScreenSource currScreenSource = currentTargetDevice->GetScreenSource();
                if ( currScreenSource == ScreenSource::HyperSoftCam)
                    nextScreenSource = ScreenSource::Disabled;
                else if (currScreenSource <= ScreenSource::Disabled)
                {
                    DAVINCI_LOG_FATAL << "Unexpected camera source: " << ScreenSourceToString(currScreenSource);
                }
            }

            // if failing to restart or switch to screen source, switch to pseudo camera
            if (DaVinciSuccess(DeviceManager::Instance().InitCaptureDevice(nextScreenSource, needUpdate)))
            {
                DAVINCI_LOG_INFO << "Switch camera to " << ScreenSourceToString(nextScreenSource) << " mode successfully!";
            }
            else
            {
                DAVINCI_LOG_FATAL << "Unable to initialize pseudo capture.";
                return;
            }
        }

        captureLiveWatch.Restart();
    }

    void DeviceManager::ResetNullFrameCount()
    {
        nullFrameCount = 0;
    }
    void DeviceManager::IncreaseNullFrameCount()
    {
        nullFrameCount++;
    }
    int DeviceManager::GetNullFrameCount()
    {
        return nullFrameCount;
    }

    bool willRevertScreenSource = false;

    void DeviceManager::CaptureMonitorEntry()
    {
        while (!captureMonitorShouldExit)
        {
            if (captureLiveWatch.ElapsedMilliseconds() >= captureMonitorTimeout)
            {
                HandleDiedCapture(true);
                if (currentTargetDevice != nullptr)
                {
                    ScreenSource source = currentTargetDevice->GetScreenSource();
                    if (source == ScreenSource::HyperSoftCam)
                        willRevertScreenSource = true;
                }
            }
            else if (GetNullFrameCount() >= maxNullFrameCount)
            {
                ResetNullFrameCount();
                HandleDiedCapture(false);
                if (currentTargetDevice != nullptr)
                {
                    ScreenSource source = currentTargetDevice->GetScreenSource();
                    if (source >= ScreenSource::CameraIndex0 && source <= ScreenSource::CameraIndex7)
                        willRevertScreenSource = true;
                }
            }
            else
            {
                // if current capture source is different than the capture source in target device
                // set it back to capture source in target device
                // for example, if device is disconnected, hypersoftcam will switch to disabled mode
                // if device is connected after a while, the logic here will switch back to hypersoftcam mode
                if (willRevertScreenSource)
                {
                    bool isCameraReady = false;

                    if (currentTargetDevice != nullptr)
                    {
                        ScreenSource source = currentTargetDevice->GetScreenSource();
                        if (source == ScreenSource::HyperSoftCam)
                        {
                            // last screen source is hypersoftcam
                            if (currentTargetDevice->IsAgentConnected())
                            {
                                isCameraReady = true;
                            }
                        }
                        else if (source >= ScreenSource::CameraIndex0 && source <= ScreenSource::CameraIndex7)
                        {
                            ScanCaptureDevices();
                            boost::lock_guard<boost::recursive_mutex> lock(captureDevicesMutex);
                            // The number of CameraIndex in screen source is the index of physcialCams array, not the camera index
                            if (((int)physicalCams.size() > (int)source) && (physicalCams[(int)source] != nullptr))
                            {
                                string cameraName = physicalCams[(int)source]->GetCameraName();
                                DAVINCI_LOG_DEBUG << "Camera name: " << cameraName;
                                // last screen source is camera
                                if (cameraName.size() > 0)
                                {
                                    isCameraReady = true;
                                }
                            }
                        }

                        if (isCameraReady)
                        {
                            if (DaVinciSuccess(DeviceManager::Instance().InitCaptureDevice(source, false)))
                            {
                                DAVINCI_LOG_INFO << "Recover camera to " << ScreenSourceToString(source) << " mode successfully!";
                            }
                            else
                            {
                                DAVINCI_LOG_INFO << "Cannot recover to " << ScreenSourceToString(source) << " mode!";
                            }
                            willRevertScreenSource = false;
                        }
                    }
                }

                ThreadSleep(captureMonitorTimeout / 5);
            }
        }
    }

    void DeviceManager::StartCaptureMonitor()
    {
        boost::mutex::scoped_lock lock(captureMonitorMutex, boost::try_to_lock);
        if (lock)
        {
            assert(captureMonitor == nullptr || boost::this_thread::get_id() == captureMonitor->get_id());
            captureMonitorShouldExit = false;
            if (captureMonitor == nullptr)
            {
                captureMonitor = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&DeviceManager::CaptureMonitorEntry, this)));
                SetThreadName(captureMonitor->get_id(), "Capture live monitor");
            }
            captureLiveWatch.Restart();
            ResetNullFrameCount();
        }
    }

    void DeviceManager::StopCaptureMonitor()
    {
        if (captureMonitor != nullptr && boost::this_thread::get_id() != captureMonitor->get_id())
        {
            // the function should wait the thread quit if it's called by another thread than capture monitor thread
            boost::mutex::scoped_lock  lock(captureMonitorMutex);
            captureLiveWatch.Reset();
            captureMonitorShouldExit = true;
            captureMonitor->join();
            captureMonitor = nullptr;
        }
        else
        {
            // otherwise, it would try to acquire the lock to reset the state
            // and do nothing if it cannot acquire the lock
            boost::mutex::scoped_lock lock(captureMonitorMutex, boost::try_to_lock);
            if (lock)
            {
                captureLiveWatch.Reset();
                captureMonitorShouldExit = true;
            }
        }
    }

    DaVinciStatus DeviceManager::SetAudioRecordDevice(const string &audioDevice)
    {
        DaVinciStatus status = AudioManager::Instance().SetRecordFromUser(audioDevice);
        if (DaVinciSuccess(status))
        {
            audioRecordDevice = audioDevice;
        }
        return status;
    }

    DaVinciStatus DeviceManager::SetAudioPlayDevice(const string &audioDevice)
    {
        DaVinciStatus status = AudioManager::Instance().SetPlayDeviceAudio(audioDevice);
        if (DaVinciSuccess(status))
        {
            audioPlayDevice = audioDevice;
        }
        return status;
    }

    DaVinciStatus DeviceManager::SetRecordFromDevice(const string &targetDevice, const string &audioDevice)
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        auto dev = GetTargetDevice(targetDevice);
        if (dev != nullptr)
        {
            DaVinciStatus status;
            dev->SetAudioRecordDevice(audioDevice);
            if (currentTargetDevice == dev)
            {
                status = AudioManager::Instance().SetRecordFromDevice(audioDevice);
            }
            return status;
        }
        else
        {
            return errc::invalid_argument;
        }
    }

    DaVinciStatus DeviceManager::SetPlayToDevice(const string &targetDevice, const string &audioDevice)
    {
        boost::lock_guard<boost::recursive_mutex> lock(targetDevicesMutex);
        auto dev = GetTargetDevice(targetDevice);
        if (dev != nullptr)
        {
            DaVinciStatus status;
            dev->SetAudioPlayDevice(audioDevice);
            if (currentTargetDevice == dev)
            {
                status = AudioManager::Instance().SetPlayToDevice(audioDevice);
            }
            return status;
        }
        else
        {
            return errc::invalid_argument;
        }
    }

    void DeviceManager::SetForceFFRDDetect(bool flag)
    {
        forceFFRDDetect = flag;
    }

    /// <summary>
    /// Get the device's connect status. 
    /// </summary>
    ///
    /// <returns> True for connect, False for idle. </returns>
    bool DeviceManager::IsDeviceConnectedByOthers(const DeviceCategory category, const string &name)
    {
        return globalDeviceStatus->IsConnectedByOthers(category, name);

    }

        /// <summary>
        /// Check and update the device's connected status. 
        /// </summary>
        ///
        /// <returns> True for when the device is not connected by others. </returns>
    bool DeviceManager::CheckAndUpdateGlobalStatus(const DeviceCategory category, const string &name, bool connect)
    {
        return  globalDeviceStatus->CheckAndUpdateStatus(category, name, connect);
    }
}
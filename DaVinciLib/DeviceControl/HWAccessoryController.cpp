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

#include "HWAccessoryController.hpp"

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

# pragma comment(lib, "wbemuuid.lib")

using namespace boost::asio;
using namespace boost::algorithm;

namespace DaVinci
{  
    //AutoResetEvent recvDataEvent;     //works! same effect with deadline_timer
    
    const double HWAccessoryController::g = 9.80665;

    //Ready
    HWAccessoryController::HWAccessoryController(const string &comPort) : comName(comPort)
    {
        comName = comPort;
        minTiltDegree = 50;
        maxTiltDegree= 130;
        arm0Degree = defaultTiltDegree;
        arm1Degree = defaultTiltDegree;
        miniZAxisTiltDegree = 0;
        maxZAxisTiltDegree = 720;
        zAxisDegree = defaultZAxisTiltDegree;
        connectFlag = false;
        xyTilterFlag = false;
        zTilterFlag = false;
        lifterFlag = false;
        powerPusherFlag = false;
        earphonePluggerFlag = false;
        usbCutterFlag = false;
        relayControllerFlag = false;
        sensorCheckerFlag = false;
        fasterSerialPortSpeedFlag = false;
        fastComputerFlag = true;
    }

    //Ready
    HWAccessoryController::~HWAccessoryController()
    {
        Disconnect();
    }

    //Ready
    string HWAccessoryController::GetComPortName()
    {
        if (comName.empty() == false)
        {
            return comName;
        }
        else
        {
            return "NoneComPortName";
        }
    }

    //Ready
    bool HWAccessoryController::Connect()
    {
        if (comName == "")
        {
            return false;
        }
        try
        {
            if (InitSerialPort(comName) == true)
            {
                //deadline_timer readTimeoutTimer(io_sev);
                //deadline_timer writeTimeoutTimer(io_sev);
                //readTimeoutTimer.expires_from_now(boost::posix_time::millisec(4200));	// depend on the longest command: holder up/down
                //writeTimeoutTimer.expires_from_now(boost::posix_time::millisec(2000));
                Init();
            }
            else
            {
                return false;
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when connecting Arduino board: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return false;
        }
        connectFlag = true;
        return true;
    }

    //Ready
    bool HWAccessoryController::IsConnected()
    {
        if (this->connectFlag)
        {
            DAVINCI_LOG_INFO << this->comName << string(" is in connected status.");
        }
        else
        {
            DAVINCI_LOG_INFO << this->comName << string(" is in disconnected status.");
        }
        return this->connectFlag;
    }

    //Ready
    bool HWAccessoryController::Disconnect()
    {
        try
        {
            //Bug 4296: DaVinci will crash if start 2 instances if COM ports connect with hardware
            //Because the Destroy function will be called automatically if new serialPort fails
            if (serialPort == nullptr)
            {
                return false;
            }

            if (serialPort->is_open())
            {
                //This function causes all outstanding asynchronous read or write operations to finish immediately
                //May not needed, since the close() may include this function
                //serialPort->cancel();
                //This function is used to close the serial port. Any asynchronous read or write operations will be cancelled immediately
                serialPort->close();
            }
            //io_sev.stop();
            //io_sev.reset();
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when disconnecting Arduino board: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return false;
        }
        connectFlag = false;
        return true;
    }

    //Ready
    vector<boost::shared_ptr<HWAccessoryController>> HWAccessoryController::DetectAllSerialPorts(String currentHWAccessory)
    {
        DAVINCI_LOG_INFO << string("Start to detect serial ports by WMI...");
        vector<string> portsWmiInfo = GetSerialPortsFromWMI();
        DAVINCI_LOG_INFO << portsWmiInfo.size() << string(" serial ports are detected by WMI...");
        vector<string> potentialHWControllerComPorts;
        potentialHWControllerComPorts = GetPotentialHWControllerComPorts(portsWmiInfo);
        
        DAVINCI_LOG_INFO << string("Start to detect serial ports by Registry...");
        vector<string> ports = GetComPortsFromRegistry();
        DAVINCI_LOG_INFO << ports.size() << string(" serial ports are detected by Registry...");
        vector<boost::shared_ptr<HWAccessoryController>> controllers = vector<boost::shared_ptr<HWAccessoryController>>();
        for (unsigned int i = 0; i < ports.size(); i++)
        {
            if ((boost::algorithm::iequals(currentHWAccessory, "") == false)
                && (boost::algorithm::iequals(currentHWAccessory, ports[i]) == false))
            {
                continue;
            }

            if (false == ContainVectorElement(potentialHWControllerComPorts, ports[i]))
            {
                DAVINCI_LOG_INFO<< string("        ") << string("Skip ") << ports[i] << string(" according to WMI detection results!");
                continue;
            }

            boost::shared_ptr<HWAccessoryController> controller = boost::shared_ptr<HWAccessoryController>(new HWAccessoryController(ports[i]));
            if (controller->Connect())
            {
                DAVINCI_LOG_INFO << "***************** Try to update HWAccessories baud rate *****************";
                controller->TryUpdateBaudRate();
                DAVINCI_LOG_INFO << "***************** End to update HWAccessories baud rate *****************";

                if (controller->ReturnFirmwareName())
                {
                    controllers.push_back(controller);
                    DAVINCI_LOG_INFO<< string("        ") << ports[i] << string(" is connected with an Arduino/Galileo/Edison device!");
                    DAVINCI_LOG_INFO << string("        ") << string("Begin to initialize all the Hardware Accessories infomation of ") << ports[i] << string(" !");
                    controller->InitFFRDHardwareInfo();
                    DAVINCI_LOG_INFO << string("        ") << string("Finish all the Hardware Accessories infomation initialization of ") << ports[i] << string(" !");
                    controller->Disconnect();
                }
                else
                {
                    DAVINCI_LOG_INFO << string("        ") << ports[i] << string(" is not connected with an Arduino/Galileo/Edison device!");
                    controller->Disconnect();
                }
            }
        }
        DAVINCI_LOG_INFO << string("Finish detecting serial ports.");
        return controllers;
    }

    //Ready
    vector<boost::shared_ptr<HWAccessoryController>> HWAccessoryController::DetectNewSerialPorts(vector<string> connectedComPorts)
    {
        DAVINCI_LOG_INFO << string("Start to detect serial ports by WMI...");
        vector<string> portsWmiInfo = GetSerialPortsFromWMI(); 
        DAVINCI_LOG_INFO << portsWmiInfo.size() << string(" serial ports are detected by WMI...");
        vector<string> potentialHWControllerComPorts;
        potentialHWControllerComPorts = GetPotentialHWControllerComPorts(portsWmiInfo);
        
        DAVINCI_LOG_INFO << string("Start to detect serial ports by Registry...");
        vector<string> ports = GetComPortsFromRegistry();
        DAVINCI_LOG_INFO << ports.size() << string(" serial ports are detected by Registry...");
        vector<boost::shared_ptr<HWAccessoryController>> controllers = vector<boost::shared_ptr<HWAccessoryController>>();
        for (unsigned int i = 0; i < connectedComPorts.size(); i++)
        {
            DAVINCI_LOG_INFO << string("        ") << connectedComPorts[i] << string(" is connected with an Arduino/Galileo/Edison device!");
        }

        for (unsigned int i = 0; i < ports.size(); i++)
        {
            if (false == ContainVectorElement(connectedComPorts, ports[i]))
            {
                if (false == ContainVectorElement(potentialHWControllerComPorts, ports[i]))
                {
                    DAVINCI_LOG_INFO<< string("        ") << string("Skip ") << ports[i] << string(" according to WMI detection results!");
                    continue;
                }

                boost::shared_ptr<HWAccessoryController> controller = boost::shared_ptr<HWAccessoryController>(new HWAccessoryController(ports[i]));
                if (controller->Connect())
                {
                    DAVINCI_LOG_INFO << "***************** Try to update HWAccessories baud rate *****************";
                    controller->TryUpdateBaudRate();
                    DAVINCI_LOG_INFO << "***************** End to update HWAccessories baud rate *****************";

                    if (controller->ReturnFirmwareName())
                    {
                        controllers.push_back(controller);
                        DAVINCI_LOG_INFO << string("        ") << ports[i] << string(" is connected with an Arduino/Galileo/Edison device!");
                        DAVINCI_LOG_INFO << string("        ") << string("Begin to initialize all the Hardware Accessories infomation of ") << ports[i] << string(" !");
                        controller->InitFFRDHardwareInfo();
                        DAVINCI_LOG_INFO << string("        ") << string("Finish all the Hardware Accessories infomation initialization of ") << ports[i] << string(" !");
                        controller->Disconnect();
                    }
                    else
                    {
                        DAVINCI_LOG_INFO << string("        ") << ports[i] << string(" is not connected with an Arduino/Galileo/Edison device!");
                        controller->Disconnect();
                    }
                }
            }
        }
        DAVINCI_LOG_INFO << string("Finish detecting new serial ports.");
        return controllers;
    }

    //Ready
    int HWAccessoryController::TryUpdateBaudRate()
    {
        int result = 0;
        //Feng: Suggest not to set the flag in the ReturnFirmwareName function!
        fasterSerialPortSpeedFlag = false;
        if (ReturnFirmwareName() == true)   //Use Baud rate: 9600 to have a try
        {
            DAVINCI_LOG_INFO<< string("        ") << string("Baud rate is 9600!");
            result = 1;
            return result;
        }
        else                                //Use Baud rate: 115200 to have a try
        {
            Disconnect();
            fasterSerialPortSpeedFlag = true;
            if (InitSerialPort(comName) == true)
            {
                if (ReturnFirmwareName() == true)
                {
                    DAVINCI_LOG_INFO<< string("        ") << string("Baud rate is 115200 with normal computer serial port!");
                    result = 2;
                    return result;
                }
                else
                {
                    fastComputerFlag = false;
                    if (ReturnFirmwareName() == true)
                    {
                        DAVINCI_LOG_INFO<< string("        ") << string("Baud rate is 115200 with slow computer serial port!");
                        result = 3;
                        return result;
                    }
                    else
                    {
                        fastComputerFlag = true;            //Recover the flag
                        fasterSerialPortSpeedFlag = false;  //Recover the flag
                        result = 0;
                        return result;
                    }
                }
            }
            else    //InitSerialPort fail
            {
                fasterSerialPortSpeedFlag = false;  //Recover the flag
                result = 0;
                return result;
            }
        }
    }

    //Ready
    int HWAccessoryController::GetFFRDCapability(int arg)
    {
        int result = 0;
        int cmd = BuildArduinoCommandWithParams(Q_CAPABILITY, 0, arg);
        string capStr = SendCommandAndReturnString(cmd, "Get FFRD Capability");
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_ERROR << "Firmware - Unknown argument: " << capStr;
            }
            else
            {
                TryParse(capStr, result);
            }
        }
        return result;
    }

    //Ready
    bool HWAccessoryController::ReturnFirmwareName()
    {
        int cmd = BuildArduinoCommand(Q_NAME);
        try
        {
            if (!serialPort->is_open())
            {
                DAVINCI_LOG_ERROR << string("Error: serial port is not connected!");
                return false;
            }
            string firmwareName = SendCommandAndReturnString(cmd, "Read Firmware Name");
            if (firmwareName != "")
            {
                //Ready: simple firmware handle function
                //DAVINCI_LOG_DEBUG << string("        ") << firmwareName << endl;
                //if (firmwareName.find("DaVinci") != string::npos)
                //{
                //    if ((firmwareName.find("1.") != string::npos) || (firmwareName.find("2.") != string::npos))
                //    {
                //        DAVINCI_LOG_DEBUG << string("\n        ") << comName << string(": please update Arduino firmware!");
                //        return false;
                //    }
                //    else
                //    {
                //        return true;
                //    }
                //}

                //Ready, regex_replace function
                //boost::regex expr("[^\\d.\\d+]");
                //string fmt("");   
                //string firmwareStr = boost::regex_replace(firmwareName, expr, fmt);

                //Ready, regex_search function
                boost::smatch reg_mat;
                boost::regex reg_expr("\\d+.\\d+");
                bool reg_search_result = boost::regex_search(firmwareName, reg_mat, reg_expr);
                if (reg_search_result == true)
                {
                    string firmwareStr = reg_mat[0].str();
                    vector<string> versionStr;
                    boost::algorithm::split(versionStr, firmwareStr, boost::algorithm::is_any_of("."));
                    int majorVersion = 0, minorVersion = 0;
                    TryParse(versionStr[0], majorVersion);
                    TryParse(versionStr[1], minorVersion);
                    if (majorVersion > 3)
                    {
                        // the firmware with majorVersion from 4, 5... use the update serial port speed
                        //fasterSerialPortSpeedFlag = true;
                        return true;
                    }
                    else if (majorVersion == 3 && minorVersion >= 8)
                    {
                        // the firmware with majorVersion 3, minorVersion from 8... use the update serial port speed
                        //fasterSerialPortSpeedFlag = true;
                        return true;
                    }
                    else if (majorVersion == 3 && minorVersion < 8)
                    {
                        // the firmware with majorVersion 3, minorVersion from 1~7... use the original serial port speed
                        //fasterSerialPortSpeedFlag = false;
                        return true;
                    }
                    else
                    {
                        // as we change the firmware to 3.1, user must upgrade the firmware if they're using latest DaVinci
                        //fasterSerialPortSpeedFlag = false;
                        DAVINCI_LOG_WARNING << string("\n        ") << comName << string(": please update Arduino firmware!");
                        return false;
                    }
                }           

                else
                {
                    DAVINCI_LOG_ERROR << string("Error: cannot return firmware name in right format!");
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception in ReturnFirmwareName: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return false;
        }
    }

    //Ready
    int HWAccessoryController::ReturnMajorVersionOfFirmware()
    {
        int cmd = BuildArduinoCommand(Q_NAME);
        try
        {
            if (!serialPort->is_open())
            {
                DAVINCI_LOG_ERROR << string("Error: serial port is not connected!");
                return -1;
            }
            string firmwareName = SendCommandAndReturnString(cmd, "Read Major Version of Firmware");
            if (firmwareName != "")
            {
                //Ready, regex_replace function
                //boost::regex expr("[^\\d.\\d+]");
                //string fmt("");   
                //string firmwareStr = boost::regex_replace(firmwareName, expr, fmt);

                //Ready, regex_search function
                boost::smatch reg_mat;
                boost::regex reg_expr("\\d+.\\d+");
                bool reg_search_result = boost::regex_search(firmwareName, reg_mat, reg_expr);
                if (reg_search_result == true)
                {
                    string firmwareStr = reg_mat[0].str();
                    vector<string> versionStr;
                    boost::algorithm::split(versionStr, firmwareStr, boost::algorithm::is_any_of("."));
                    int majorVersion = 0;
                    TryParse(versionStr[0], majorVersion);
                    return majorVersion;
                }
                else
                {
                    DAVINCI_LOG_ERROR << string("Error: cannot return firmware name in right format!");
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception in ReturnMajorVersionOfFirmware: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return -1;
        }
    }

    //Ready
    int HWAccessoryController::GetSepcificFFRDHardwareInfo(int type, const string &cmdName)
    {
        int resultFlag = 0;
        int cmd = 0;
        cmd = BuildArduinoCommandWithParams(Q_HARDWARE, 0, type);
        string sepcificFFRDStr = SendCommandAndReturnString(cmd, cmdName);
        if (sepcificFFRDStr != "" && sepcificFFRDStr.find("Unknown command") == string::npos)
        {
            if (sepcificFFRDStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_ERROR << "Firmware - Unknown argument: " << sepcificFFRDStr;
                resultFlag ++;
            }
            if (sepcificFFRDStr.find("doesn't") != string::npos)
            {
                resultFlag ++;
            }
        }
        else
        {
            resultFlag ++;
        }
        return resultFlag;
    }

    //Ready
    void HWAccessoryController::InitFFRDHardwareInfo()
    {
        int resultFlag;
        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_XY_TILTER, "Get XY Tilter");
        this->xyTilterFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_Z_TILTER, "Get Z Tilter");
        this->zTilterFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_LIFTER, "Get Lifter");
        this->lifterFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_POWER_PUSHER, "Get Power Pusher");
        this->powerPusherFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_EARPHONE_PLUGGER, "Get Earphone Plugger");
        this->earphonePluggerFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_USB_CUTTER, "Get USB Cutter");
        this->usbCutterFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_RELAY_CONTROLLER, "Get Extended Relay Controller");
        this->relayControllerFlag = (resultFlag == 0) ? true : false;

        resultFlag = GetSepcificFFRDHardwareInfo(Q_HARDWARE_SENSOR_MODULE, "Get Sensor Checker Module");
        this->sensorCheckerFlag = (resultFlag == 0) ? true : false; 
    }

    //Ready
    bool HWAccessoryController::GetFFRDHardwareInfo(int type)
    {
        int resultFlagSummary = 0;
        switch (type)
        {
        case Q_HARDWARE_FFRD_ID:            //0
            {
                resultFlagSummary += GetSepcificFFRDHardwareInfo(Q_HARDWARE_FFRD_ID, "Get FFRD ID");
                break;
            }
        case Q_HARDWARE_XY_TILTER:          //1
            {
                return this->xyTilterFlag;
            }
        case Q_HARDWARE_Z_TILTER:           //2
            {
                return this->zTilterFlag;
            }
        case Q_HARDWARE_LIFTER:             //3
            {
                return this->lifterFlag;
            }
        case Q_HARDWARE_POWER_PUSHER:       //4
            {
                return this->powerPusherFlag;
            }
        case Q_HARDWARE_EARPHONE_PLUGGER:   //5
            {
                return this->earphonePluggerFlag;
            }
        case Q_HARDWARE_USB_CUTTER:         //6
            {
                return this->usbCutterFlag;
            }
        case Q_HARDWARE_RELAY_CONTROLLER:   //7
            {
                return this->relayControllerFlag;
            }
        case Q_HARDWARE_SENSOR_MODULE:      //8
            {
                return this->sensorCheckerFlag;
            }
        default:
            {
                DAVINCI_LOG_ERROR << "Wrong hardware type parameter:" << type;
                resultFlagSummary ++;
                break;
            }
        }

        if (resultFlagSummary == 0)
        {
            return true;    //hardware component exist
        }
        else
        {
            return false;   //hardware component doesn't exist
        }
    }

    /*
    //Ready
    int HWAccessoryController::GetArm0Degree()
    {
    return this->arm0Degree;
    }

    //Ready
    int HWAccessoryController::GetArm1Degree()
    {
    return this->arm1Degree;
    }
    */

    //Ready
    bool HWAccessoryController::TiltLeft(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        if (((arm0Degree - degrees) >= minTiltDegree) && ((arm0Degree - degrees) <= maxTiltDegree))
        {
            this->arm0Degree -= degrees;
            int cmd;
            if (speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM0, this->arm0Degree);
                //SendCommandAndReturnString(cmd, "Tilt Left");
                SyncSendCommandAndReturnString(cmd, "Tilt Left");   //chong: to avoid too long echo time!
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM0, this->arm0Degree, speed);
                //SendCommandAndReturnString(cmd, "Tilt Left with Defined Speed");
                SyncSendCommandAndReturnString(cmd, "Tilt Left with Defined Speed");   //chong: to avoid too long echo time!
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::TiltRight(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        if (((arm0Degree + degrees) >= minTiltDegree) && ((arm0Degree + degrees) <= maxTiltDegree))
        {
            this->arm0Degree += degrees;
            int cmd;
            if (speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM0, this->arm0Degree);
                //SendCommandAndReturnString(cmd, "Tilt Right");
                SyncSendCommandAndReturnString(cmd, "Tilt Right");      //chong: to avoid too long echo time!
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM0, this->arm0Degree, speed);
                //SendCommandAndReturnString(cmd, "Tilt Right with Defined Speed");
                SyncSendCommandAndReturnString(cmd, "Tilt Right with Defined Speed");   //chong: to avoid too long echo time!
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::TiltUp(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        if (((arm1Degree - degrees) >= minTiltDegree) && ((arm1Degree - degrees) <= maxTiltDegree))
        {
            this->arm1Degree -= degrees;
            int cmd;
            if (speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM1, this->arm1Degree);
                //SendCommandAndReturnString(cmd, "Tilt Up");
                SyncSendCommandAndReturnString(cmd, "Tilt Up");     //chong: to avoid too long echo time!
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM1, this->arm1Degree, speed);
                //SendCommandAndReturnString(cmd, "Tilt Up with Defined Speed");
                SyncSendCommandAndReturnString(cmd, "Tilt Up with Defined Speed");   //chong: to avoid too long echo time!
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::TiltDown(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        if (((arm1Degree + degrees) >= minTiltDegree) && ((arm1Degree + degrees) <= maxTiltDegree))
        {
            this->arm1Degree += degrees;
            int cmd;
            if (speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM1, this->arm1Degree);
                //SendCommandAndReturnString(cmd, "Tilt Down");
                SyncSendCommandAndReturnString(cmd, "Tilt Down");       //chong: to avoid too long echo time!
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM1, this->arm1Degree, speed);
                //SendCommandAndReturnString(cmd, "Tilt Down with Defined Speed");
                SyncSendCommandAndReturnString(cmd, "Tilt Down with Defined Speed");   //chong: to avoid too long echo time!
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::TiltTo(int arm0Degree, int arm1Degree, int arm0Speed, int arm1Speed)
    {
        if ((arm0Speed < 0) || (arm0Speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed of Arm 0 is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            arm0Speed = 150;
        }
        if ((arm1Speed < 0) || (arm1Speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed of Arm 1 is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            arm1Speed = 150;
        }

        int cmd;
        bool ret = false;

        if (arm0Degree >= minTiltDegree && arm0Degree <= maxTiltDegree)
        {
            this->arm0Degree = arm0Degree;
            if (arm0Speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM0, this->arm0Degree);
                //if (SendCommandAndReturnString(cmd, "Tilt To 1") == "")
                if (SyncSendCommandAndReturnString(cmd, "Tilt To 1") == "")    //chong: to avoid too long echo time!
                {
                    return false;
                }
                ret = true;
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM0, this->arm0Degree, arm0Speed);
                //if (SendCommandAndReturnString(cmd, "Tilt Arm 0 with Defined Speed") == "")
                if (SyncSendCommandAndReturnString(cmd, "Tilt Arm 0 with Defined Speed") == "")    //chong: to avoid too long echo time!
                {
                    return false;
                }
                ret = true;
            }
        }
        if (arm1Degree >= minTiltDegree && arm1Degree <= maxTiltDegree)
        {
            this->arm1Degree = arm1Degree;
            if (arm1Speed == 0)
            {
                cmd = BuildArduinoCommandWithParams(Q_TILT, Q_TILT_ARM1, this->arm1Degree);
                //if (SendCommandAndReturnString(cmd, "Tilt To 2") == "")
                if (SyncSendCommandAndReturnString(cmd, "Tilt To 2") == "")    //chong: to avoid too long echo time!
                {
                    return false;
                }
                ret = true;
            }
            else
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_TILT_SPEED, Q_TILT_ARM1, this->arm1Degree, arm1Speed);
                //if (SendCommandAndReturnString(cmd, "Tilt Arm 1 with Defined Speed") == "")
                if (SyncSendCommandAndReturnString(cmd, "Tilt Arm 1 with Defined Speed") == "")    //chong: to avoid too long echo time!
                {
                    return false;
                }
                ret = true;
            }
        }

        //Solve the Bug 4751: Davinci tilt XY axis can't reset after run tiltcontinuous rotate.
        if (arm0Degree == 90 && arm1Degree == 90)
        {
            this->arm0Degree = arm0Degree;
            this->arm1Degree = arm1Degree;
            if (arm0Speed == 0)
            {
                arm0Speed = 150;
            }
            cmd = BuildArduinoCommandWithThreeParams(Q_TILT_XY_AXES, Q_TILT_ARM0, this->arm0Degree, 150);
            if (SyncSendCommandAndReturnString(cmd, "Tilt X and Y To 90 degrees") == "")    //chong: to avoid too long echo time!
            {
                return false;
            }
            return true;
        }

        return ret;
    }

    //Ready
    bool HWAccessoryController::InnerClockwiseContinuousTilt(int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        int cmd = BuildArduinoCommandWithParams(Q_TILT_CONTINOUS_CW, Q_TILT_ARM1, speed);
        //SendCommandAndReturnString(cmd, "Inner plane Tilt Clockwise Continuously with Defined Speed");
        SyncSendCommandAndReturnString(cmd, "Inner plane Tilt Clockwise Continuously with Defined Speed");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::InnerAnticlockwiseContinuousTilt(int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        int cmd = BuildArduinoCommandWithParams(Q_TILT_CONTINOUS_ACW, Q_TILT_ARM1, speed);
        //SendCommandAndReturnString(cmd, "Inner plane Tilt Anticlockwise Continuously with Defined Speed");
        SyncSendCommandAndReturnString(cmd, "Inner plane Tilt Anticlockwise Continuously with Defined Speed");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::OuterClockwiseContinuousTilt(int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        int cmd = BuildArduinoCommandWithParams(Q_TILT_CONTINOUS_CW, Q_TILT_ARM0, speed);
        //SendCommandAndReturnString(cmd, "Outer plane Tilt Clockwise Continuously with Defined Speed");
        SyncSendCommandAndReturnString(cmd, "Outer plane Tilt Clockwise Continuously with Defined Speed");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::OuterAnticlockwiseContinuousTilt(int speed)
    {
        if ((speed < 0) || (speed > 300))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 150, instead of user defined speed!");
            speed = 150;
        }
        int cmd = BuildArduinoCommandWithParams(Q_TILT_CONTINOUS_ACW, Q_TILT_ARM0, speed);
        //SendCommandAndReturnString(cmd, "Outer plane Tilt Anticlockwise Continuously with Defined Speed");
        SyncSendCommandAndReturnString(cmd, "Outer plane Tilt Anticlockwise Continuously with Defined Speed");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::ZAxisClockwiseTilt(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 4))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 2 rpm, instead of user defined speed!");
            speed = 2;
        }
        if ((degrees < minZAxisDegreeParameter) || (degrees > maxZAxisDegreeParameter))
        {
            DAVINCI_LOG_ERROR << "The value of degree operand is out of normal range, don't execute this action!";
            return false;
        }
        if (((zAxisDegree + degrees) >= miniZAxisTiltDegree) && ((zAxisDegree + degrees) <= maxZAxisTiltDegree))
        {
            this->zAxisDegree += degrees;
            int cmd = BuildArduinoCommandWithThreeParams(Q_Z_ROTATE, zAxisClockwise, degrees, speed);
            //SendCommandAndReturnString(cmd, "Z Axis Tilt Clockwise with Defined Speed!");
            SyncSendCommandAndReturnString(cmd, "Z Axis Tilt Clockwise with Defined Speed!");    //chong: to avoid too long echo time!        
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::ZAxisAnticlockwiseTilt(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 4))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 2 rpm, instead of user defined speed!");
            speed = 2;
        }
        if ((degrees < minZAxisDegreeParameter) || (degrees > maxZAxisDegreeParameter))
        {
            DAVINCI_LOG_ERROR << "The value of degree operand is out of normal range, don't execute this action!";
            return false;
        }
        if (((zAxisDegree - degrees) >= miniZAxisTiltDegree) && ((zAxisDegree - degrees) <= maxZAxisTiltDegree))
        {
            this->zAxisDegree -= degrees;
            int cmd = BuildArduinoCommandWithThreeParams(Q_Z_ROTATE, zAxisAnticlockwise, degrees, speed);
            //SendCommandAndReturnString(cmd, "Z Axis Tilt Anticlockwise with Defined Speed!");
            SyncSendCommandAndReturnString(cmd, "Z Axis Tilt Anticlockwise with Defined Speed!");    //chong: to avoid too long echo time!
            return true;
        }
        else
        {
            return false;
        }
    }

    //Ready
    bool HWAccessoryController::ZAxisTiltTo(int degrees, int speed)
    {
        if ((speed < 0) || (speed > 4))
        {
            DAVINCI_LOG_WARNING << string("Warning: User defined speed is out of normal range!");
            DAVINCI_LOG_INFO << string("Use default speed: 2 rpm, instead of user defined speed!");
            speed = 2;
        }
        if ((degrees < minZAxisDegreeParameter) || (degrees > maxZAxisDegreeParameter))
        {
            DAVINCI_LOG_ERROR << "The value of degree operand is out of normal range, don't execute this action!";
            return false;
        }
        int cmd;
        bool ret = false;
        if (degrees >= miniZAxisTiltDegree && degrees <= maxZAxisTiltDegree)
        {
            if (this->zAxisDegree < degrees)
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_Z_ROTATE, zAxisClockwise, (degrees - this->zAxisDegree), speed);
                //SendCommandAndReturnString(cmd, "Z Axis Tilt Clockwise with Defined Speed!");
                SyncSendCommandAndReturnString(cmd, "Z Axis Tilt Clockwise with Defined Speed!");    //chong: to avoid too long echo time!
                ret = true;
            }
            else if (this->zAxisDegree > degrees)
            {
                cmd = BuildArduinoCommandWithThreeParams(Q_Z_ROTATE, zAxisAnticlockwise, (this->zAxisDegree - degrees), speed);
                //SendCommandAndReturnString(cmd, "Z Axis Tilt Anticlockwise with Defined Speed!");
                SyncSendCommandAndReturnString(cmd, "Z Axis Tilt Anticlockwise with Defined Speed!");    //chong: to avoid too long echo time!
                ret = true;
            }
            else // this->zAxisDegree == degrees
            {
                DAVINCI_LOG_DEBUG << string("        ") << string("Z Axis Keeps at Same Degrees!");
                ret = true;
            }            
            this->zAxisDegree = degrees;
        }
        return ret;
    }

    //Ready
    bool HWAccessoryController::HolderUp(int distance)
    {
        if (distance < minHolderDistStep || distance > maxHolderDistStep)
        {
            return false;
        }

        int cmd = BuildArduinoCommandWithParams(Q_HOLDER, Q_HOLDER_UP, distance);
        //SendCommandAndReturnString(cmd, "Holder Up");
        SyncSendCommandAndReturnString(cmd, "Holder Up");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::HolderDown(int distance)
    {
        if (distance < minHolderDistStep || distance > maxHolderDistStep)
        {
            return false;
        }

        int cmd = BuildArduinoCommandWithParams(Q_HOLDER, Q_HOLDER_DOWN, distance);
        //SendCommandAndReturnString(cmd, "Holder Down");
        SyncSendCommandAndReturnString(cmd, "Holder Down");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::PlugUSB(int pattern)
    {
        int cmd = 0;
        string cmdName;
        if (pattern == 1)
        {
            cmd = BuildArduinoCommand(Q_ONE_PLUG_IN);
            cmdName = "Plug In No.1 USB";
        }
        else if (pattern == 2)
        {
            cmd = BuildArduinoCommand(Q_ONE_PLUG_OUT);
            cmdName = "Plug Out No.1 USB";
        }
        else if (pattern == 3)
        {
            cmd = BuildArduinoCommand(Q_TWO_PLUG_IN);
            cmdName = "Plug In No.2 USB";
        }
        else if (pattern == 4)
        {
            cmd = BuildArduinoCommand(Q_TWO_PLUG_OUT);
            cmdName = "Plug Out No.2 USB!";
        }
        else
        {
            DAVINCI_LOG_ERROR << string("The value of parameter: pattern is wrong!");
            return false;
        }
        //SendCommandAndReturnString(cmd, cmdName);
        SyncSendCommandAndReturnString(cmd, cmdName);     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::UnplugUSB()
    {
        if (!serialPort->is_open())
        {
            DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
            return false;
        }
        bool ret = true, ret1, ret2;
        DAVINCI_LOG_DEBUG << string("Plug out the USB devices!");
        ret1 = PlugUSB(patternPlugOutUSB1);
        ret2 = PlugUSB(patternPlugOutUSB2);
        if (ret1 || ret2)
        {
            DAVINCI_LOG_DEBUG << string("Wait ") << (delayReplugUSB / 1000) << string(" seconds!");
            ThreadSleep(delayReplugUSB);
        }
        else
        {
            DAVINCI_LOG_WARNING << string("No hardware supports replugging USB devices!");
            ret = false;
        }
        return ret;
    }

    //Ready
    bool HWAccessoryController::PlugUSB()
    {
        if (!serialPort->is_open())
        {
            DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
            return false;
        }
        bool ret = true, ret1, ret2;
        DAVINCI_LOG_DEBUG << string("Plug in the USB devices!");
        ret1 = PlugUSB(patternPlugInUSB1);
        ret2 = PlugUSB(patternPlugInUSB2);
        if (ret1 || ret2)
        {
            DAVINCI_LOG_DEBUG << string("Wait ") << (delayReplugUSB / 1000) << string(" seconds!");
            ThreadSleep(delayReplugUSB);
        }
        else
        {
            DAVINCI_LOG_WARNING << string("No hardware supports replugging USB devices!");
            ret = false;
        }
        return ret;
    }

    bool HWAccessoryController::StartMeasuringPower()
    {
        return UnplugUSB();
    }

    bool HWAccessoryController::StopMeasuringPower()
    {
        return PlugUSB();
    }

    //Ready
    bool HWAccessoryController::PressPowerButton()
    {
        int cmd = BuildArduinoCommandWithParams(Q_BUTTON_PRESS, 0, 0);
        //SendCommandAndReturnString(cmd, "Press Power Button!");
        SyncSendCommandAndReturnString(cmd, "Press Power Button!");       //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::ReleasePowerButton()
    {
        int cmd = BuildArduinoCommandWithParams(Q_BUTTON_RELEASE, 0, 0);
        //SendCommandAndReturnString(cmd, "Release Power Button!");
        SyncSendCommandAndReturnString(cmd, "Release Power Button!");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::ShortPressPowerButton(int timeDuration)
    {
        int cmd = BuildArduinoCommandWithParams(Q_SHORT_PRESS_BUTTON, 0, timeDuration);
        //SendCommandAndReturnString(cmd, "Short Press Power Button!");
        SyncSendCommandAndReturnString(cmd, "Short Press Power Button!");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::LongPressPowerButton(int timeDuration)
    {
        int cmd = BuildArduinoCommandWithParams(Q_LONG_PRESS_BUTTON, 0, timeDuration);
        //SendCommandAndReturnString(cmd, "Long Press Power Button!");
        SyncSendCommandAndReturnString(cmd, "Long Press Power Button!");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::PlugInEarphone()
    {
        int cmd = BuildArduinoCommandWithParams(Q_EARPHONE_PLUG_IN, 0, 0);
        //SendCommandAndReturnString(cmd, "Plug in Earphone!");
        SyncSendCommandAndReturnString(cmd, "Plug in Earphone!");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::PlugOutEarphone()
    {
        int cmd = BuildArduinoCommandWithParams(Q_EARPHONE_PLUG_OUT, 0, 0);
        //SendCommandAndReturnString(cmd, "Plug out Earphone!");
        SyncSendCommandAndReturnString(cmd, "Plug out Earphone!");     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::ConnectRelay(int port)
    {
        int cmd = 0;
        string cmdName;
        if (port == 1)
        {
            cmd = BuildArduinoCommand(Q_RELAY_ONE_CONNECT);
            cmdName = "Connect No.1 Relay";
        }
        else if (port == 2)
        {
            cmd = BuildArduinoCommand(Q_RELAY_TWO_CONNECT);
            cmdName = "Connect No.2 Relay";
        }
        else if (port == 3)
        {
            cmd = BuildArduinoCommand(Q_RELAY_THREE_CONNECT);
            cmdName = "Connect No.3 Relay";
        }
        else if (port == 4)
        {
            cmd = BuildArduinoCommand(Q_RELAY_FOUR_CONNECT);
            cmdName = "Connect No.4 Relay";
        }
        else
        {
            DAVINCI_LOG_ERROR << string("The value of parameter: port is wrong!");
            return false;
        }
        //SendCommandAndReturnString(cmd, cmdName);
        SyncSendCommandAndReturnString(cmd, cmdName);     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    bool HWAccessoryController::DisconnectRelay(int port)
    {
        int cmd = 0;
        string cmdName;
        if (port == 1)
        {
            cmd = BuildArduinoCommand(Q_RELAY_ONE_DISCONNECT);
            cmdName = "Disconnect No.1 Relay";
        }
        else if (port == 2)
        {
            cmd = BuildArduinoCommand(Q_RELAY_TWO_DISCONNECT);
            cmdName = "Disconnect No.2 Relay";
        }
        else if (port == 3)
        {
            cmd = BuildArduinoCommand(Q_RELAY_THREE_DISCONNECT);
            cmdName = "Disconnect No.3 Relay";
        }
        else if (port == 4)
        {
            cmd = BuildArduinoCommand(Q_RELAY_FOUR_DISCONNECT);
            cmdName = "Disconnect No.4 Relay";
        }
        else
        {
            DAVINCI_LOG_ERROR << string("The value of parameter: port is wrong!");
            return false;
        }
        //SendCommandAndReturnString(cmd, cmdName);
        SyncSendCommandAndReturnString(cmd, cmdName);     //chong: to avoid too long echo time!
        return true;
    }

    //Ready
    int HWAccessoryController::GetMotorSpeed(int id)
    {
        int result = 0;
        int cmd = BuildArduinoCommandWithParams(Q_MOTOR_SPEED, 0, id);
        //string capStr = SendCommandAndReturnString(cmd, "Get Servo Motor Speed");
        string capStr = SyncSendCommandAndReturnString(cmd, "Get Servo Motor Speed");     //chong: to avoid too long echo time!
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_WARNING << "Cannot find the right format of speed value!";
                DAVINCI_LOG_WARNING << "Captured string from firmware" << capStr;
            }
            else
            {
                boost::regex expr("[^\\d:\\d+]");
                string fmt("");   
                string numCapStr = boost::regex_replace(capStr, expr, fmt);
                vector<string> speedStr;
                boost::algorithm::split(speedStr, numCapStr, boost::algorithm::is_any_of(":"));
                if (speedStr.size() == 2)
                {
                    TryParse(speedStr[1], result);
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Cannot find the right format of speed value!";
                    DAVINCI_LOG_WARNING << "Captured string from firmware" << capStr;
                }
            }
        }
        return result;
    }

    //Ready
    int HWAccessoryController::GetMotorAngle(int id)
    {
        int result = 0;
        int cmd = BuildArduinoCommandWithParams(Q_MOTOR_ANGLE, 0, id);
        //string capStr = SendCommandAndReturnString(cmd, "Get Servo Motor Angle");
        string capStr = SyncSendCommandAndReturnString(cmd, "Get Servo Motor Angle");     //chong: to avoid too long echo time!
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_WARNING << "Cannot find the right format of angle value!";
                DAVINCI_LOG_WARNING << "Captured string from firmware" << capStr;
            }
            else
            {
                boost::regex expr("[^\\d:\\d+]");
                string fmt("");   
                string numCapStr = boost::regex_replace(capStr, expr, fmt);
                vector<string> angleStr;
                boost::algorithm::split(angleStr, numCapStr, boost::algorithm::is_any_of(":"));
                if (angleStr.size() == 2)
                {
                    TryParse(angleStr[1], result);
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Cannot find the right format of angle value!";
                    DAVINCI_LOG_WARNING << "Captured string from firmware" << capStr;
                }
            }
        }
        return result;
    }

    string HWAccessoryController::GetSepcificSensorCheckerTypeDataFromSensorModule(int type, bool& resultFlag)
    {
        bool getResultFlag = false;
        string sensorCheckerDataAsString = "";
        switch (type)
        {
        case Q_SENSOR_CHECKER_STOP:         //0
            {
                DAVINCI_LOG_INFO << "Ignore the sensor checker stop action from sensor module!";
                sensorCheckerDataAsString = string("Ignore the sensor checker stop action from sensor module!");
                getResultFlag = true;       //Chong: false?
                break;
            }
        case Q_SENSOR_CHECKER_START:        //1
            {
                DAVINCI_LOG_INFO << "Ignore the sensor checker start action from sensor module!";
                sensorCheckerDataAsString = string("Ignore the sensor checker start action from sensor module!");
                getResultFlag = true;       //Chong: false?
                break;
            }
        case Q_SENSOR_ACCELEROMETER:        //100
            {
                bool result = false;
                double x = 0, y = 0, z = 0;
                sensorCheckerDataAsString = GetAccelerometerDataFromSensorModule(result, x, y, z);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_GYROSCOPE:            //101
            {
                bool result = false;
                double x = 0, y = 0, z = 0; 
                sensorCheckerDataAsString = GetGyroscopeDataFromSensorModule(result, x, y, z);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_COMPASS:              //102
            {
                bool result = false;
                double x = 0, y = 0, z = 0; 
                sensorCheckerDataAsString = GetCompassDataFromSensorModule(result, x, y, z);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_TEMPERATURE:          //103
            {
                bool result = false;
                double temperature = 0;
                sensorCheckerDataAsString = GetTemperatureDataFromSensorModule(result, temperature);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_PRESSURE:             //104
            {
                bool result = false;
                double pressure = 0;
                sensorCheckerDataAsString = GetPressureDataFromSensorModule(result, pressure);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_ATMOSPHERE:           //105
            {
                bool result = false;
                double atmosphere = 0;
                sensorCheckerDataAsString = GetAtmosphereDataFromSensorModule(result, atmosphere);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_ALTITUDE:             //106
            {
                bool result = false;
                double altitude = 0;
                sensorCheckerDataAsString = GetAltitudeDataFromSensorModule(result, altitude);
                getResultFlag = result;
                break;
            }
        case Q_SENSOR_LINEAR_ACCELERATION:  //107            
        case Q_SENSOR_GRAVITY:              //108            
        case Q_SENSOR_ROTATION_VECTOR:      //109
            {
                DAVINCI_LOG_ERROR << "Unsupported sensor checker type parameter: " << type << " from sensor module!";
                sensorCheckerDataAsString = string("Unsupported sensor checker type parameter: ") + boost::lexical_cast<string>(type) + string(" from sensor module!");
                getResultFlag = false;
                break;
            }
        default:
            {
                DAVINCI_LOG_ERROR << "Wrong sensor checker type parameter: " << type;
                sensorCheckerDataAsString = string("Wrong sensor checker type parameter: ") + boost::lexical_cast<string>(type);
                getResultFlag = false;
                break;
            }
        }
        resultFlag = getResultFlag;
        return sensorCheckerDataAsString;
    }

    vector<double> HWAccessoryController::ParseSensorData(const string &capString)
    {
        vector<double> sensorData = vector<double>();
        DAVINCI_LOG_INFO << string("Return sensor data from sensor module: ") << capString;
        string regExpression = "-?\\d+.\\d+";
        boost::regex reg_expr(regExpression);       
        boost::sregex_iterator it(capString.begin(), capString.end(), reg_expr);
        boost::sregex_iterator end;
        for (; it != end; ++it)
        {
            sensorData.push_back(boost::lexical_cast<double>(it->str()));
        }
        return sensorData;
    }

    string HWAccessoryController::GetAccelerometerDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z)
    {
        //resultFlag will be false if DaVinci cannot get accelerometer data from sensor module
        bool sensorDataFlag = false;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_ACCELEROMETER, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get accelerometer data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get accelerometer data from sensor module!");                
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get accelerometer data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 3)
                {
                    x = parsedSensorData[0] * g;
                    y = parsedSensorData[1] * g;
                    z = parsedSensorData[2] * g;
                    DAVINCI_LOG_INFO << string("Accelerometer data get from sensor module (m/s^2): X: ") << x << string(", Y: ") << y << string(", Z: ") << z;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Accelerometer data get from sensor module (m/s^2): X: ") + boost::lexical_cast<string>(x) + string(", Y: ") + boost::lexical_cast<string>(y) + string(", Z: ") + boost::lexical_cast<string>(z);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get accelerometer data from sensor module!");
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get accelerometer data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get accelerometer data from sensor module!");
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get accelerometer data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }

    string HWAccessoryController::GetGyroscopeDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z)
    {
        //resultFlag will be false if DaVinci cannot get gyroscope data from sensor module
        bool sensorDataFlag = false;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_GYROSCOPE, 0, 0);

        string capStr = SendCommandAndReturnString(cmd, "Get gyroscope data from sensor module!", waitTimeBeforeCommandUpdateForIIC);

        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get gyroscope data from sensor module!");
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get gyroscope data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 3)
                {
                    x = parsedSensorData[0] * M_PI / 180;
                    y = parsedSensorData[1] * M_PI / 180;
                    z = parsedSensorData[2] * M_PI / 180;
                    DAVINCI_LOG_INFO << string("Gyroscope data get from sensor module (rad/s): X: ") << x << string(", Y: ") << y << string(", Z: ") << z;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Gyroscope data get from sensor module (rad/s): X: ") + boost::lexical_cast<string>(x) + string(", Y: ") + boost::lexical_cast<string>(y) + string(", Z: ") + boost::lexical_cast<string>(z);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get gyroscope data from sensor module!");
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get gyroscope data from sensor module!");
                }               
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get gyroscope data from sensor module!");
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get gyroscope data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }
    
    string HWAccessoryController::GetCompassDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z)
    {
        //resultFlag will be false if DaVinci cannot get compass data from sensor module
        bool sensorDataFlag = false;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_COMPASS, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get compass data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get compass data from sensor module!");
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get compass data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 3)
                {
                    x = parsedSensorData[0];
                    y = parsedSensorData[1];
                    z = parsedSensorData[2];
                    DAVINCI_LOG_INFO << string("Compass data get from sensor module: X: ") << x << string(", Y: ") << y << string(", Z: ") << z;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Compass data get from sensor module: X: ") + boost::lexical_cast<string>(x) + string(", Y: ") + boost::lexical_cast<string>(y) + string(", Z: ") + boost::lexical_cast<string>(z);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get compass data from sensor module!");
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get compass data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get compass data from sensor module!");
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get compass data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }
    
    string HWAccessoryController::GetTemperatureDataFromSensorModule(bool& resultFlag, double& temperature)
    {
        //resultFlag will be false, sensorData = -300, if DaVinci cannot get temperature data from sensor module, less than -273.15 degrees
        double sensorData = 0;
        bool sensorDataFlag = false;        
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_TEMPERATURE, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get temperature data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get temperature data from sensor module!");
                sensorData = -300;
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get temperature data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 1)
                {
                    sensorData = parsedSensorData[0];
                    DAVINCI_LOG_INFO << string("Temperature data get from sensor module (C): ") << sensorData;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Temperature data get from sensor module (C): ") + boost::lexical_cast<string>(sensorData);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get temperature data from sensor module!");
                    sensorData = -300;
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get temperature data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get temperature data from sensor module!");            
            sensorData = -300;
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get temperature data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }
    
    string HWAccessoryController::GetPressureDataFromSensorModule(bool& resultFlag, double& pressure)
    {
        //resultFlag will be false, sensorData = -1, if DaVinci cannot get pressure data from sensor module
        bool sensorDataFlag = false;
        double sensorData = 0;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_PRESSURE, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get pressure data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get pressure data from sensor module!");
                sensorData = -1;
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get pressure data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 1)
                {
                    sensorData = parsedSensorData[0];
                    DAVINCI_LOG_INFO << string("Pressure data get from sensor module (Pa): ") << sensorData;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Pressure data get from sensor module (Pa): ") + boost::lexical_cast<string>(sensorData);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get pressure data from sensor module!");
                    sensorData = -1;
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get pressure data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get pressure data from sensor module!");
            sensorData = -1;
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get pressure data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }

    string HWAccessoryController::GetAtmosphereDataFromSensorModule(bool& resultFlag, double& atmosphere)
    {
        //resultFlag will be false, sensorData = -1, if DaVinci cannot get related atmosphere data from sensor module
        bool sensorDataFlag = false;
        double sensorData = 0;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_ATMOSPHERE, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get related atmosphere data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get related atmosphere data from sensor module!");
                sensorData = -1;
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get related atmosphere data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 1)
                {
                    sensorData = parsedSensorData[0];
                    DAVINCI_LOG_INFO << string("Related atmosphere data get from sensor module: ") << sensorData;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Related atmosphere data get from sensor module: ") + boost::lexical_cast<string>(sensorData);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get related atmosphere data from sensor module!");
                    sensorData = -1;
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get related atmosphere data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get related atmosphere data from sensor module!");
            sensorData = -1;
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get related atmosphere data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }
    
    string HWAccessoryController::GetAltitudeDataFromSensorModule(bool& resultFlag, double& altitude)
    {
        //resultFlag will be false, sensorData = -1, if DaVinci cannot get altitude data from sensor module
        bool sensorDataFlag = false;
        double sensorData = 0;
        string sensorCheckerDataAsString = "";
        int cmd = BuildArduinoCommandWithParams(Q_ALTITUDE, 0, 0);
        string capStr = SendCommandAndReturnString(cmd, "Get altitude data from sensor module!", waitTimeBeforeCommandUpdateForIIC);
        if (capStr != "" && capStr.find("Unknown command") == string::npos)
        {
            if (capStr.find("Unknown argument") != string::npos)
            {
                DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
                DAVINCI_LOG_INFO << string("Cannot get altitude data from sensor module!");
                sensorData = -1;
                sensorDataFlag = false;
                sensorCheckerDataAsString = string("Cannot get altitude data from sensor module!");
            }
            else
            {
                vector<double> parsedSensorData = ParseSensorData(capStr);
                if (parsedSensorData.size() == 1)
                {
                    sensorData = parsedSensorData[0];
                    DAVINCI_LOG_INFO << string("Altitude data get from sensor module (m): ") << sensorData;
                    sensorDataFlag = true;
                    sensorCheckerDataAsString = string("Altitude data get from sensor module (m): ") + boost::lexical_cast<string>(sensorData);
                }
                else
                {
                    DAVINCI_LOG_INFO << string("Cannot get altitude data from sensor module!");
                    sensorData = -1;
                    sensorDataFlag = false;
                    sensorCheckerDataAsString = string("Cannot get altitude data from sensor module!");
                }
            }
        }
        else
        {
            DAVINCI_LOG_DEBUG << string("Return value: ") << capStr;
            DAVINCI_LOG_INFO << string("Cannot get altitude data from sensor module!");
            sensorData = -1;
            sensorDataFlag = false;
            sensorCheckerDataAsString = string("Cannot get altitude data from sensor module!");
        }
        resultFlag = sensorDataFlag;
        return sensorCheckerDataAsString;
    }


    //Ready
    bool HWAccessoryController::InitSerialPort(const string &comPort)
    {
        comName = comPort;
        if (comName == "")
        {
            return false;
        }
        try
        {
            //Bug 4296: DaVinci will crash if start 2 instances if COM ports connect with hardware
            try
            {
                serialPort = boost::shared_ptr<boost::asio::serial_port>(new serial_port(io_sev, comPort));
                DAVINCI_LOG_INFO << comName << string(" is opened.");
            }
            catch (...)
            {
                DAVINCI_LOG_ERROR << string("Exception when try to new the serial port!");
                DAVINCI_LOG_ERROR << string("The COM port(s) are occupied!");
                return false;
            }

            if (fasterSerialPortSpeedFlag == true)
            {
                serialPort->set_option(serial_port::baud_rate(ARDUINO_BAND_RATE_HIGHER));
            }
            else
            {
                serialPort->set_option(serial_port::baud_rate(ARDUINO_BAND_RATE));
            }
            serialPort->set_option(serial_port::flow_control(serial_port::flow_control::none));
            serialPort->set_option(serial_port::parity(serial_port::parity::none));
            serialPort->set_option(serial_port::stop_bits(serial_port::stop_bits::one));
            serialPort->set_option(serial_port::character_size(8));

            ThreadSleep(waitTimeComIitialization);  //Ensure the Arduino COM port is ready! (chong: 1500 is OK, 1000 is not OK)
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when initialize the serial port: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return false;
        }
        return true;
    }

    //Ready
    void HWAccessoryController::Init()
    {
        int degree = GetFFRDCapability(Q_CAP_TILT_DEGREE);
        if (degree > 0)
        {
            DAVINCI_LOG_DEBUG << string("Maximum tilting degree: ") << degree;
            minTiltDegree = defaultTiltDegree - degree;
            maxTiltDegree = defaultTiltDegree + degree;
        }
        else if (degree == 0)
        {
            DAVINCI_LOG_DEBUG << string("Cannot read the maximum tilting degree from EEPROM!");
            degree = 90;
            DAVINCI_LOG_DEBUG << string("Maximum tilting degree (Default): ") << degree;
            minTiltDegree = defaultTiltDegree - degree;
            maxTiltDegree = defaultTiltDegree + degree;
        }
    }

    //Ready
    void HWAccessoryController::HandleWrite(boost::system::error_code ec, std::size_t bytesTransferred)
    {
        //cin.read(buf, bytesTransferred);
        //recvDataEvent.Set();     //works! same effect with deadline_timer
    }

    //Async serial communication - ready
    void HWAccessoryController::WriteCommand(int cmd, const string &cmdName)
     {
        if (!serialPort->is_open())
        {
            DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
            return;
        }
        try
        {
            //Todo: serialPort->DiscardInBuffer();
            //Todo: serialPort->DiscardOutBuffer();
            DAVINCI_LOG_DEBUG << string("        ") << string("Write Command: ") << cmdName << string(" !");

            //Todo: serialPort->WriteLine(cmd.ToString());
            stringstream s_s;
            string cmdString;
            s_s << cmd;
            s_s >> cmdString;

            // Two methods to write data to serial port, both OK!
            //size_t bytes_written = write(serialPort, boost::asio::buffer(cmdString));
            //or
            //size_t bytes_written = serialPort->write_some(boost::asio::buffer(cmdString));
            //DAVINCI_LOG_DEBUG << bytes_written <<string(" bytes are written to Arduino board!");

            //write(serialPort, boost::asio::buffer(cmdString));

            serialPort->async_write_some(boost::asio::buffer(cmdString), boost::bind(&HWAccessoryController::HandleWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            deadline_timer writeTimeoutTimer(io_sev);

            if (fasterSerialPortSpeedFlag == true)
            {
                writeTimeoutTimer.expires_from_now(boost::posix_time::millisec(waitTimeoutOfWriteCommandUpdate));   // Timeout 15ms later
            }
            else
            {
                writeTimeoutTimer.expires_from_now(boost::posix_time::millisec(waitTimeoutOfWriteCommand));	// Timeout 300ms later (250ms is enough! Original value is 500ms)
            }
            
            // After timeout, call the cancel() function of serialPort to giveup reading more characters
            writeTimeoutTimer.async_wait(boost::bind(&serial_port::cancel, boost::ref(serialPort)));
            io_sev.run();                  
            io_sev.reset();                    //for ensure: executing test in the next time, io_service can run successfully
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when writing command to Arduino board: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
        }
    }

    /*
    //Sync serial communication - ready
    void HWAccessoryController::SyncWriteCommand(int cmd, const string &cmdName)
    {
    if (!serialPort->is_open())
    {
    DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
    return;
    }
    try
    {
    DAVINCI_LOG_DEBUG << string("        ") << string("Sync Write Command: ") << cmdName << string(" !");

    stringstream s_s;
    string cmdString;
    s_s << cmd;
    s_s >> cmdString;

    // Two methods to write data to serial port, both OK!
    //size_t bytes_written = write(serialPort, boost::asio::buffer(cmdString));
    //or
    //size_t bytes_written = serialPort->write_some(boost::asio::buffer(cmdString));
    //DAVINCI_LOG_DEBUG << bytes_written << string(" bytes are written to Arduino board!");

    serialPort->write_some(boost::asio::buffer(cmdString));
    }
    catch (boost::system::error_code &ec)
    {
    DAVINCI_LOG_ERROR << string("Exception when sync writing command to Arduino board: ");
    DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
    }
    }
    */

    //Ready
    void HWAccessoryController::HandleRead(char *buf, boost::system::error_code ec, std::size_t bytesTransferred)
    {
        //cout.write(buf, bytesTransferred);
        //recvDataEvent.Set();     //works! same effect with deadline_timer
    }

    //Async serial communication - ready
    string HWAccessoryController::ReadCommand(int waitTimeBeforeCommandParameter)
    {
        string response = "";
        if (!serialPort->is_open())
        {
            DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
            return response;
        }

        if (fasterSerialPortSpeedFlag == true)
        {
            ThreadSleep(waitTimeBeforeCommandParameter);    //Use parameter to define use waitTimeBeforeCommandUpdate or waitTimeBeforeCommandUpdateForIIC
        }
        else
        {
            ThreadSleep(waitTimeBeforeCommand);	//Buffering time!
        }

        try
        {
            const int inBufferLength = 100;
            char inBuffer[inBufferLength] = "\0";

            //async_read(serialPort, buffer(inBuffer), boost::bind(&HWAccessoryController::HandleRead, this, inBuffer, _1, _2));	    //chong: cannot work        

            serialPort->async_read_some(boost::asio::buffer(inBuffer, inBufferLength), boost::bind(&HWAccessoryController::HandleRead, this, inBuffer, _1, _2));
            deadline_timer readTimeoutTimer(io_sev);            
            if (fasterSerialPortSpeedFlag == true)
            {
                if (fastComputerFlag == true)
                {
                    readTimeoutTimer.expires_from_now(boost::posix_time::millisec(waitTimeoutOfReadCommandUpdateFastComputer));	// Timeout 5ms later
                }
                else
                {
                    readTimeoutTimer.expires_from_now(boost::posix_time::millisec(waitTimeoutOfReadCommandUpdateSlowComputer));	// Timeout 30ms later
                }
            }
            else
            {
                readTimeoutTimer.expires_from_now(boost::posix_time::millisec(waitTimeoutOfReadCommand));	// Timeout 500ms later
            }
            // After timeout, call the cancel() function of serialPort to giveup reading more characters
            readTimeoutTimer.async_wait(boost::bind(&serial_port::cancel, boost::ref(serialPort)));
            io_sev.run();                  
            io_sev.reset();                    //for ensure: executing test in the next time, io_service can run successfully
            //recvDataEvent.WaitOne(5000);     //works! same effect with deadline_timer

            int bytesToRead = 0;
            for (int i = 0; i < inBufferLength; i++)
            {
                if (inBuffer[i] != '\0')
                    bytesToRead++;
                else
                    break;
            }

            if (bytesToRead != 0)
            {
                response = inBuffer;
                boost::trim(response);
                DAVINCI_LOG_DEBUG << string("        ") << string("Response from Arduino: ") << response << string(" !");
                for (int i = 0; i < inBufferLength; i++)
                {
                    inBuffer[i] = NULL;
                }
                //serialPort->cancel();
            }
            else
            {
                DAVINCI_LOG_DEBUG << string("        ") << string("No response from Arduino!");
                for (int i = 0; i < inBufferLength; i++)
                {
                    inBuffer[i] = NULL;
                }
                //serialPort->cancel();
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when reading command from Arduino: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
        }
        return response;
    }

    //Sync serial communication - ready
    string HWAccessoryController::SyncReadCommand()
    {
        string response = "";
        if (!serialPort->is_open())
        {
            DAVINCI_LOG_WARNING << string("Warning: serial port is NOT open!");
            return response;
        }

        if (fasterSerialPortSpeedFlag == true)
        {
            ThreadSleep(waitTimeBeforeCommandUpdate);
        }
        else
        {
            ThreadSleep(waitTimeBeforeCommand);	//Buffering time!
        }

        try
        {
            const int inBufferLength = 100;
            char inBuffer[inBufferLength] = "\0";

            size_t bytesToRead = serialPort->read_some(boost::asio::buffer(inBuffer, inBufferLength));
            //DAVINCI_LOG_DEBUG << bytesToRead << string(" bytes are read from Arduino board!");
            /*
            int bytesToRead = 0;
            for (int i = 0; i < inBufferLength; i++)
            {
            if (inBuffer[i] != '\0')
            bytesToRead++;
            else
            break;
            }
            */
            if (bytesToRead != 0)
            {
                response = inBuffer;
                boost::trim(response);
                DAVINCI_LOG_DEBUG << string("        ") << string("Response from Arduino: ") << response << string(" !");
                for (int i = 0; i < inBufferLength; i++)
                {
                    inBuffer[i] = NULL;
                }
            }
            else
            {
                DAVINCI_LOG_DEBUG << string("        ") << string("No response from Arduino!");
                for (int i = 0; i < inBufferLength; i++)
                {
                    inBuffer[i] = NULL;
                }
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception when sync reading command from Arduino: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
        }
        return response;
    }

    /*
    //Ready
    bool HWAccessoryController::ReadStatus()
    {
        // read status
        WriteCommand(BuildArduinoCommand(Q_STATUS), "Read Status");
        string resp = ReadCommand();
        if (resp == "")
        {
            return false;
        }
        // "Unknown command" is used to compatible with older version of firmware
        if (resp.find("Unknown command") != string::npos || resp.find("ready") != string::npos)
        {
            return true;
        }		 
        return false;
    }
    */

    /*
    //Ready
    bool HWAccessoryController::SendCommand(int cmd, const string &cmdName, bool ignoreRespond)
    {
        const int maxWaitCount = 100;
        bool pass = false;
        try
        {
            for (int i = 0; i < maxWaitCount; i++)
            {
                if (ReadStatus() == true)
                {
                    pass = true;
                    break;
                }
            }
            if (pass == false)
            {
                DAVINCI_LOG_DEBUG << string("Controller is busy and cannot execute command: ") << cmdName;
                return false;
            }
            WriteCommand(cmd, cmdName);
            // TODO: ensure the received string is corresponding with the command being sent
            string resp = ReadCommand();
            if (resp == "" || resp.find("Unknown command") != string::npos)
            {
                return false;
            }
            pass = false;
            // wait until the command is executed
            for (int i = 0; i < maxWaitCount; i++)
            {
                if (ReadStatus() == true)
                {
                    pass = true;
                    break;
                }
            }
        }
        catch (boost::system::error_code &ec)
        {
            DAVINCI_LOG_ERROR << string("Exception in SendCommand: ");
            DAVINCI_LOG_ERROR << boost::system::system_error(ec).what() << string(" !");
            return false;
        }
        return pass;
    }
    */

    //Ready
    string HWAccessoryController::SendCommandAndReturnString(int cmd, const string &cmdName, int waitTimeBeforeCommandParameter)
    {
        WriteCommand(cmd, cmdName);

        if (fasterSerialPortSpeedFlag == true)
        {
            ThreadSleep(waitTimeBeforeCommandParameter);
        }
        else
        {
            ThreadSleep(waitTimeBeforeCommand);	//Buffering time!
        }

        return ReadCommand(waitTimeBeforeCommandParameter);
    }

    //Ready
    string HWAccessoryController::SyncSendCommandAndReturnString(int cmd, const string &cmdName)
    {
        //SyncWriteCommand(cmd, cmdName);     //May lead error!
        WriteCommand(cmd, cmdName);

        if (fasterSerialPortSpeedFlag == true)
        {
            ThreadSleep(waitTimeBeforeCommandUpdate);
        }
        else
        {
            ThreadSleep(waitTimeBeforeCommand);	//Buffering time!
        }

        return SyncReadCommand();
    }

    /*
    //Ready
    int HWAccessoryController::GetArduinoCmdIndex(int cmd)
    {
    return ((cmd >> 24) & 0xffff);
    }
    */

    //Ready
    int HWAccessoryController::BuildArduinoCommand(int command)
    {
        return ((command & 0xffff) << 24);
    }

    //Ready
    int HWAccessoryController::BuildArduinoCommandWithParams(int command, int param1, int param2)
    {
        return ((command & 0xffff) << 24) | ((param1 & 0xff) << 16) | (param2 & 0xffff);
    }

    //Ready
    int HWAccessoryController::BuildArduinoCommandWithThreeParams(int command, int param1, int param2, int param3)
    {
        return ((command & 0xffff) << 24) | ((param1 & 0x3) << 22) | ((param2 & 0x3ff) << 12) | (param3 & 0xfff);
    }

    //Ready
    vector<string> HWAccessoryController::GetComPortsFromRegistry()  
    {  
        int i = 0; 
        TCHAR name[256];  
        UCHAR szPortName[256];
        //UCHAR portFriendlyName[256];       //chong: useless!
        LONG ret = 0; 
        DWORD dwIndex = 0;  
        DWORD dwName;  
        DWORD dwSizeofPortName;  
        DWORD type; 
        HKEY hKey;
        LPCTSTR path = "HARDWARE\\DEVICEMAP\\SERIALCOMM\\";  
        vector<string> ports;

#if (defined WIN32)
        ret = (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey)); 
        //ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey);	    //Open an established registry key, if successful, return ERROR_SUCCESS, that is value "0"
        if (ret != ERROR_SUCCESS)
        {
            return ports;
        }
        else
        {		 
            do  
            {
                dwName = sizeof(name);  
                dwSizeofPortName = sizeof(szPortName); 
                ret = RegEnumValue(hKey, dwIndex++, name, &dwName, NULL, &type, szPortName, &dwSizeofPortName);		//read the key value   
                //RegQueryValueEx(hKey, "FRIENDLYNAME", NULL, NULL, portFriendlyName, &dwSizeofPortName);           //chong: useless!
                if ((ret == ERROR_SUCCESS) || (ret == ERROR_MORE_DATA)) 
                {
                    TCHAR tchartmp[256];  
                    memset(tchartmp, 0, sizeof(tchartmp));  
                    for(unsigned int j = 0, k = 0; j != dwSizeofPortName; ++j)  
                    {  
                        if(szPortName[j] != '\0')  
                        {  
                            tchartmp[k] = szPortName[j];  
                            ++k;  
                        }  
                    }  
                    ports.push_back(string(tchartmp));  
                    ++i;  
                } 
                else
                {
                    break;
                }
            } while ((ret == ERROR_SUCCESS) || (ret == ERROR_MORE_DATA));  
            RegCloseKey(hKey);  
            sort(ports.begin(), ports.end()); 
            return ports;  
        }
#else
        DAVINCI_LOG_WARNING << "Not supported now.";
        return ports; 
#endif
    }

    //Ready
    vector<string> HWAccessoryController::GetSerialPortsFromWMI()
    {
        vector<string> serialPorts;
#if (defined WIN32)
        HRESULT hres;

        // Step 1: --------------------------------------------------
        // Initialize COM. ------------------------------------------
        //hres =  CoInitializeEx(0, COINIT_MULTITHREADED);
        //chong: comment for avoid the following error: 0x80010106 Cannot change thread mode after it is set.
        hres =  CoInitializeEx(0, COINIT_APARTMENTTHREADED); 
        
        // Only record that we initialized COM if that is what we really did.
        // An error code of RPC_E_CHANGED_MODE means that the call to CoInitialize failed because COM had already been initialized on another mode
        //- which isn't a fatal condition and so in this case we don't want to call CoUninitialize
        if (FAILED(hres))
        {
            if (hres == RPC_E_CHANGED_MODE)
            {
                DAVINCI_LOG_WARNING << "The COM library has already been initialized!";
            }
            else
            {
                DAVINCI_LOG_ERROR << "Failed to initialize COM library. Error code = 0x" << hex << hres;
                return serialPorts;          // Program has failed.
            }
        }


        // Step 2: --------------------------------------------------
        // Set general COM security levels --------------------------
        // chong: comment for avoid the following error: 0x80010119 Security must be initialized before any interfaces are marshalled or unmarshalled. It cannot be changed once initialized.
        hres =  CoInitializeSecurity(
            NULL, 
            -1,                          // COM authentication
            NULL,                        // Authentication services
            NULL,                        // Reserved
            RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
            RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation 
            NULL,                        // Authentication info
            EOAC_NONE,                   // Additional capabilities
            NULL                         // Reserved
            );
        if (FAILED(hres))
        {
            if (hres == RPC_E_TOO_LATE)
            {
                DAVINCI_LOG_WARNING << "The COM security levels have already been initialized!";
            }
            else
            {
                cout << "Failed to initialize security. Error code = 0x" << hex << hres;
                CoUninitialize();
                return serialPorts;          // Program has failed.
            }
        }


        // Step 3: ---------------------------------------------------
        // Obtain the initial locator to WMI -------------------------
        IWbemLocator *pLoc = NULL;
        hres = CoCreateInstance(
            CLSID_WbemLocator, 
            0, 
            CLSCTX_INPROC_SERVER, 
            IID_IWbemLocator, (LPVOID *) &pLoc);
        if (FAILED(hres))
        {
            DAVINCI_LOG_ERROR << "Failed to create IWbemLocator object." << " Err code = 0x" << hex << hres;
            CoUninitialize();
            return serialPorts;          // Program has failed.
        }


        // Step 4: -----------------------------------------------------
        // Connect to WMI through the IWbemLocator::ConnectServer method
        IWbemServices *pSvc = NULL;
        // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
        hres = pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
            NULL,                    // User name. NULL = current user
            NULL,                    // User password. NULL = current
            0,                       // Locale. NULL indicates current
            NULL,                    // Security flags.
            0,                       // Authority (for example, Kerberos)
            0,                       // Context object
            &pSvc                    // pointer to IWbemServices proxy
            );
        if (FAILED(hres))
        {
            DAVINCI_LOG_ERROR << "Could not connect. Error code = 0x" << hex << hres;
            pLoc->Release();
            CoUninitialize();
            return serialPorts;      // Program has failed.
        }
        //cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;			//chong: testing!


        // Step 5: --------------------------------------------------
        // Set security levels on the proxy -------------------------
        hres = CoSetProxyBlanket(
            pSvc,                        // Indicates the proxy to set
            RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
            RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
            NULL,                        // Server principal name
            RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
            RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
            NULL,                        // client identity
            EOAC_NONE                    // proxy capabilities
            );
        if (FAILED(hres))
        {
            DAVINCI_LOG_ERROR << "Could not set proxy blanket. Error code = 0x" << hex << hres;
            pSvc->Release();
            pLoc->Release();  
            CoUninitialize();
            return serialPorts;          // Program has failed.
        }


        // Step 6: --------------------------------------------------
        // Use the IWbemServices pointer to make requests of WMI ----
        // Get the name of the serial ports
        IEnumWbemClassObject* pEnumerator = NULL;
        hres = pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT * FROM Win32_SerialPort"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);
        if (FAILED(hres))
        {
            DAVINCI_LOG_ERROR << "Query for serial port information failed." << " Error code = 0x" << hex << hres;
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return serialPorts;         // Program has failed.
        }


        // Step 7: -------------------------------------------------
        // Get the data from the query in step 6 -------------------
        IWbemClassObject *pclsObj = nullptr;	//chong
        ULONG uReturn = 0;
        while (pEnumerator)						//chong: fixed Klocwork issue: line 1130
        {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if(0 == uReturn)
            {
                break;
            }
            VARIANT vtProp;

            // Get the value of the Name property
            hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            //wcout << " Serial Port : " << vtProp.bstrVal << endl;		//chong: testing!

            //chong: Convert BSTR to string
            string temp_serial_info = (_bstr_t)vtProp.bstrVal;
            serialPorts.push_back(temp_serial_info);

            VariantClear(&vtProp);

            pclsObj->Release();
        }

        // Cleanup
        // ========
        pSvc->Release();
        pLoc->Release();
        if (pEnumerator != nullptr)		//chong: fixed Klocwork issue: Pointer 'pEnumerator' checked for NULL at line 1130 will be dereferenced at line 1159.
        {
            pEnumerator->Release();		//chong: fixed Klocwork issue: line 1159
        }

        //chong: may cause following crash when no real/unreal COM device is connected on PC! Comment now!
        //An unhandled exception of type 'System.AccessViolationException' occurred in DaVinci.exe
        //Additional information: Attempted to read or write protected memory. This is often an indication that other memory is corrupt.
        //if(!pclsObj) pclsObj->Release();	

        CoUninitialize();

        return serialPorts;   // Program successfully completed.
#else
        DAVINCI_LOG_WARNING << "Not supported now.";
        return serialPorts;
#endif
    } 

    //Ready
    vector<string> HWAccessoryController::GetPotentialHWControllerComPorts(vector<string> portsWmiInfo)
    {
        vector<string> potentialHWControllerComPorts;
        for (unsigned int j = 0; j < portsWmiInfo.size(); j++)
        {
            boost::smatch reg_mat;
            boost::regex reg_expression("COM\\d+");
            bool reg_search_result = boost::regex_search(portsWmiInfo[j], reg_mat, reg_expression);
            if (reg_search_result == true)
            {
                string comNumberStr = reg_mat[0].str();
                DAVINCI_LOG_INFO << string("        ") << comNumberStr << string(" detailed information from WMI: ") << portsWmiInfo[j];
                if (portsWmiInfo[j].find("Arduino") != string::npos)
                {
                    potentialHWControllerComPorts.push_back(comNumberStr);
                    DAVINCI_LOG_INFO << string("        ") << comNumberStr << string(" may be connected with an Arduino device!");
                }
                else if (portsWmiInfo[j].find("Galileo") != string::npos)
                {
                    potentialHWControllerComPorts.push_back(comNumberStr);
                    DAVINCI_LOG_INFO << string("        ") << comNumberStr << string(" may be connected with a Galileo device!");
                }
                else if (portsWmiInfo[j].find("Edison") != string::npos)
                {
                    potentialHWControllerComPorts.push_back(comNumberStr);
                    DAVINCI_LOG_INFO << string("        ") << comNumberStr << string(" may be connected with an Edison device!");
                }
                else
                {
                    DAVINCI_LOG_INFO << string("        ") << comNumberStr << string(" is not connected with an Arduino/Galileo/Edison device!");
                }
            }
        }
        return potentialHWControllerComPorts;
    }

}
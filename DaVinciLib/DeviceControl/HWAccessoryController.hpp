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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __HW_ACCESSORY_CONTROLLER_HPP__
#define __HW_ACCESSORY_CONTROLLER_HPP__

#include <iostream>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "boost/thread/thread.hpp"
#include <boost/regex.hpp>

#define _USE_MATH_DEFINES
#include <math.h>       //Use M_PI const value

#include "DaVinciCommon.hpp"
#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "DeviceControlCommon.hpp"

namespace DaVinci
{
    using namespace std;

    enum HardwareInfo
    {
        Q_HARDWARE_FFRD_ID = 0,
        Q_HARDWARE_XY_TILTER = 1,
        Q_HARDWARE_Z_TILTER = 2,
        Q_HARDWARE_LIFTER = 3,
        Q_HARDWARE_POWER_PUSHER = 4,
        Q_HARDWARE_EARPHONE_PLUGGER = 5,
        Q_HARDWARE_USB_CUTTER = 6,
        Q_HARDWARE_RELAY_CONTROLLER = 7,
        Q_HARDWARE_SENSOR_MODULE = 8
    };

    enum SensorCheckerType
    {
        Q_SENSOR_CHECKER_STOP = 0,
        Q_SENSOR_CHECKER_START = 1,
        Q_SENSOR_ACCELEROMETER = 100,
        Q_SENSOR_GYROSCOPE = 101,
        Q_SENSOR_COMPASS = 102,
        Q_SENSOR_TEMPERATURE = 103,
        Q_SENSOR_PRESSURE = 104,
        Q_SENSOR_ATMOSPHERE = 105,
        Q_SENSOR_ALTITUDE = 106,
        Q_SENSOR_LINEAR_ACCELERATION = 107,
        Q_SENSOR_GRAVITY = 108,
        Q_SENSOR_ROTATION_VECTOR = 109
    };

    /// <summary> Hardware accessory controller. </summary>
    class HWAccessoryController : public boost::enable_shared_from_this<HWAccessoryController>
    {
    public:
        explicit HWAccessoryController(const string &comPort);

        virtual ~HWAccessoryController();

        static const int defaultTiltDegree = 90;
        static const int defaultZAxisTiltDegree = 360;
        /// <summary> Pre-defined step of tilting plane. </summary>
        static const int tiltingStep = 5;
        static const int minZAxisDegreeParameter = 0;
        static const int maxZAxisDegreeParameter = 360;
        /// <summary> Tilting angle minimum limitation, must initialize in the cpp for non-const member. Initial value 50. </summary>
        //static int minTiltDegree;		//static int may cause the minTiltDegree and maxTiltDegree changed by another COM device
        int minTiltDegree;
        /// <summary> Tilting angle maximum limitation, must initialize in the cpp for non-const member. Initial value 130. </summary>
        int maxTiltDegree;
        /// <summary> Serial port, must initialize in the cpp for non-const member. Initial value "". </summary>
        int miniZAxisTiltDegree;
        int maxZAxisTiltDegree;
        string comName;

    private:
        static const int ARDUINO_BAND_RATE = 9600;
        /// <summary> Used for higher speed serial port transmission. </summary>
        static const int ARDUINO_BAND_RATE_HIGHER = 115200;
        /// <summary> Minimum step of holder distance. </summary>
        static const int minHolderDistStep = 0;
        /// <summary> Maximum step of holder distance. </summary>
        static const int maxHolderDistStep = 5;
        static const int Q_TILT_XY_AXES = 0x50;
        static const int Q_TILT_CONTINOUS_CW = 0X51;
        static const int Q_TILT_CONTINOUS_ACW = 0X52;
        static const int Q_TILT = 0x70;
        static const int Q_TILT_ARM0 = 0;
        static const int Q_TILT_ARM1 = 1;
        static const int Q_HOLDER = 0x71;
        static const int Q_HOLDER_UP = 0;
        static const int Q_HOLDER_DOWN = 1;
        static const int Q_NAME = 0x72;
        static const int Q_EARPHONE_PLUG_IN = 0x73;
        static const int Q_EARPHONE_PLUG_OUT = 0x74;
        static const int Q_ONE_PLUG_IN = 0x75;
        static const int Q_ONE_PLUG_OUT = 0x76;
        static const int Q_TWO_PLUG_IN = 0x77;
        static const int Q_TWO_PLUG_OUT = 0x78;
        static const int Q_BUTTON_PRESS = 0X79;
        static const int Q_BUTTON_RELEASE = 0X7a;
        static const int Q_STATUS = 0X7b;
        static const int Q_CAPABILITY = 0x7c;
        static const int Q_CAP_TILT_DEGREE = 0;
        static const int Q_MOTOR_SPEED = 0x7d;
        static const int Q_MOTOR_ANGLE = 0x7e;
        static const int Q_TILT_SPEED = 0x7f;
        static const int Q_SHORT_PRESS_BUTTON = 0x60;   //Do short press power button action in the Arduino, to avoid long echo time!
        static const int Q_LONG_PRESS_BUTTON = 0x61;    //Do long press power button action in the Arduino, time control is more accurate!
        static const int Q_Z_ROTATE = 0x62;          
        static const int Q_HARDWARE = 0x63;
        static const int Q_RELAY_ONE_CONNECT = 0x64;
        static const int Q_RELAY_ONE_DISCONNECT = 0x65;
        static const int Q_RELAY_TWO_CONNECT = 0x66;
        static const int Q_RELAY_TWO_DISCONNECT = 0x67;
        static const int Q_RELAY_THREE_CONNECT = 0x68;
        static const int Q_RELAY_THREE_DISCONNECT = 0x69;
        static const int Q_RELAY_FOUR_CONNECT = 0x6a;
        static const int Q_RELAY_FOUR_DISCONNECT = 0x6b;
        static const int Q_SENSOR_INITIALIZATION = 0x40;
        static const int Q_SENSOR_CONNECTION = 0x41;
        static const int Q_ACCELEROMETER = 0x42;
        static const int Q_GYROSCOPE = 0x43;
        static const int Q_COMPASS = 0x44;
        static const int Q_TEMPERATURE = 0x45;
        static const int Q_PRESSURE = 0x46;
        static const int Q_ATMOSPHERE = 0x47;
        static const int Q_ALTITUDE = 0x48;
        /// <summary> pattern = 1, The arduino device will Turn on No.1 Relay to plug in No.1 USB. </summary>
        static const int patternPlugInUSB1 = 1;
        /// <summary> pattern = 2, The arduino device will Turn off No.1 Relay to plug out No.1 USB. </summary>
        static const int patternPlugOutUSB1 = 2;
        /// <summary> pattern = 3, The arduino device will Turn on No.2 Relay to plug in No.2 USB. </summary>
        static const int patternPlugInUSB2 = 3;
        /// <summary> pattern = 4, The arduino device will Turn off No.2 Relay to plug out No.2 USB. </summary>
        static const int patternPlugOutUSB2 = 4;
        static const int delayReplugUSB = 5000;
        /// <summary> millisecond. </summary>
        static const int waitTimeComIitialization = 1500;       //chong: 1500 is ok for most Arduino device!
        /// <summary> millisecond. </summary>
        static const int waitTimeBeforeCommand = 400;           //chong: 400 is ok for most Arduino device!
        /// <summary> millisecond. </summary>
        static const int waitTimeBeforeCommandUpdate = 0;
        /// <summary> millisecond. </summary>
        static const int waitTimeBeforeCommandUpdateForIIC = 10;
        /// <summary> millisecond. </summary>
        static const int waitTimeoutOfWriteCommand = 300;       //chong: Timeout 300ms later (250ms is enough! Original value is 500ms)
        /// <summary> millisecond. </summary>
        static const int waitTimeoutOfWriteCommandUpdate = 15;
        /// <summary> millisecond. </summary>
        static const int waitTimeoutOfReadCommand = 500;        //chong: Timeout 500ms later
        /// <summary> millisecond. </summary>
        static const int waitTimeoutOfReadCommandUpdateFastComputer = 5;    //chong: 4ms is OK for detecting the FFRD info with normal computer
        /// <summary> millisecond. </summary>
        static const int waitTimeoutOfReadCommandUpdateSlowComputer = 30;   //chong: 30ms is OK for detecting the FFRD info with slow computer
        static const int zAxisClockwise = 0;
        static const int zAxisAnticlockwise = 1;
        static const double g;   //static const double cannot have an in-class initializer
        int arm0Degree;
        int arm1Degree;
        int zAxisDegree;
        bool connectFlag;
        bool xyTilterFlag;
        bool zTilterFlag;
        bool lifterFlag;
        bool powerPusherFlag;
        bool earphonePluggerFlag;
        bool usbCutterFlag;
        bool relayControllerFlag;
        bool sensorCheckerFlag;
        
        //false: baud rate is 9600, wait times are long
        //true:  baud rate is 115200, wait times are short
        //use false as default settings
        bool fasterSerialPortSpeedFlag;

        //false: serial port communication speed with 115200 baud rate is slow
        //true:  serial port communication speed with 115200 baud rate is normal
        //use true as default settings
        bool fastComputerFlag;

        /// <summary> io_service object which is necessary in boost::asio library. </summary>
        boost::asio::io_service io_sev;
        boost::shared_ptr<boost::asio::serial_port> serialPort;


    public:
        /// <summary>
        /// Get COM port name of the hardware accessory. 
        /// </summary>
        /// <returns> Return COM port name. </returns>
        virtual string GetComPortName();

        /// <summary>
        /// Connect to arudino. 
        /// </summary>
        /// <returns> Return true for successful connection. </returns>
        virtual bool Connect();

        /// <summary>
        /// API function to query whether the arduino is in connected status. 
        /// </summary>
        /// <returns> Return true for successful connection status. </returns>
        virtual bool IsConnected();

        /// <summary>
        /// Disconnect to arudino. 
        /// </summary>
        /// <returns> Return true for successful disconnection. </returns>
        virtual bool Disconnect();

        /// <summary>
        /// Detect all serial ports connected.
        /// </summary>
        static vector<boost::shared_ptr<HWAccessoryController>> DetectAllSerialPorts(String currentHWAccessory="");

        /// <summary>
        /// Detect new serial ports while keep all connected serial ports not being influenced.
        /// </summary>
        static vector<boost::shared_ptr<HWAccessoryController>> DetectNewSerialPorts(vector<string> connectedComPorts);

        /// <summary>
        /// This API will try to connect Arduino board with different baud rate: 9600 first, then 115200.
        /// </summary>
        /// <returns>Return 0 if Arduino board cannot connect with baud rate: 9600 or 115200</returns>
        /// <returns>Return 1 if Arduino board can connect with baud rate: 9600</returns>
        /// <returns>Return 2 if Arduino board can connect with baud rate: 115200</returns>
        virtual int TryUpdateBaudRate();

        /// <summary>
        /// Get FFRD capability, including maximum tilting degree
        /// </summary>
        /// <param name="arg">the kind of capability</param>
        /// <returns>the value of capbability</returns>
        virtual int GetFFRDCapability(int arg);

        /// <summary>
        /// This API will send NAME CMD to Arduino board.
        /// The Arduino board will return the firmware name as "DaVinci Firmware version".
        /// </summary>
        /// <returns>Return true if successful</returns>
        virtual bool ReturnFirmwareName();

        /// <summary>
        /// This API will send NAME CMD to Arduino board.
        /// The Arduino board will return the major version of firmware.
        /// </summary>
        /// <returns>Return -1 if unsuccessful</returns>
        virtual int ReturnMajorVersionOfFirmware();

        /// <summary>
        /// Get specific FFRD hardware item
        /// </summary>
        /// <param name="type">>Indicated the type of hardware component.</param>
        /// <param name="cmdName">>Indicated the command name.</param>
        /// <returns>Return resultFlag.</returns>
        virtual int GetSepcificFFRDHardwareInfo(int type, const string &cmdName);

        /// <summary>
        /// Init FFRD hardware summary, then GetFFRDHardwareInfo don't need to send command to Arduino every time.
        /// </summary>
        virtual void InitFFRDHardwareInfo();  

        /// <summary>
        /// Get FFRD hardware summary
        /// </summary>
        /// <param name="type">>Indicated the type of hardware component.</param>
        /// <returns>Return true if hardware component exist, return false if hardware component doesn't exist.</returns>
        virtual bool GetFFRDHardwareInfo(int type);     
        
        //virtual int GetArm0Degree();

        //virtual int GetArm1Degree();

        /// <summary>
        /// This API will send TILT CMD to Arduino board.
        /// The Arduino board will control arm0 to tilt left for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 0 as the default setting. (Provide backward compatibility for old firmware version)
        /// The arm0 will stay at 90 degree at start up.
        /// </summary>
        /// <param name="degrees">>Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>Return true if successful</returns>
        virtual bool TiltLeft(int degrees, int speed = 0);

        /// <summary>
        /// This API will send TILT CMD to Arduino board.
        /// The Arduino board will control arm0 to tilt right for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 0 as the default setting. (Provide backward compatibility for old firmware version)
        /// The arm0 will stay at 90 degree at start up.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>Return true if successful</returns>
        virtual bool TiltRight(int degrees, int speed = 0);

        /// <summary>
        /// This API will send TILT CMD to Arduino board.
        /// The Arduino board will control arm1 to tilt up for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 0 as the default setting. (Provide backward compatibility for old firmware version)
        /// The arm1 will stay at 90 degree at start up.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>Return true if successful</returns>
        virtual bool TiltUp(int degrees, int speed = 0);

        /// <summary>
        /// This API will send TILT CMD to Arduino board.
        /// The Arduino board will control arm1 to tilt down for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 0 as the default setting. (Provide backward compatibility for old firmware version)
        /// The arm1 will stay at 90 degree at start up.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>Return true if successful</returns>
        virtual bool TiltDown(int degrees, int speed = 0);

        /// <summary>
        /// This API will send TILT CMD to Arduino board.
        /// The Arduino board will control arm0 to tilt to arm0Degree provided, arm1 to tilt to arm1Degree provided with defined speed.
        /// Speed parameter is optional. Will use 0 as the default setting. (Provide backward compatibility for old firmware version)
        /// </summary>
        /// <param name="arm0Degree">Indicated the degree arm0 moved to.</param>
        /// <param name="arm1Degree">Indicated the degree arm1 moved to.</param>
        /// <param name="arm0Speed">>Indicated the arm0 speed to move.</param>
        /// <param name="arm1Speed">>Indicated the arm1 speed to move.</param>
        /// <returns>Return true if successful</returns>
        virtual bool TiltTo(int arm0Degree, int arm1Degree, int arm0Speed = 0, int arm1Speed = 0);

        /// <summary>
        /// This API will send TILT Continuous CMD to Arduino board.
        /// The Arduino board will control inner plane to tilt clockwise continuously with defined speed.
        /// Speed parameter is optional. Will use 60 as the default setting.
        /// </summary>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool InnerClockwiseContinuousTilt(int speed = 150);

        /// <summary>
        /// This API will send TILT Continuous CMD to Arduino board.
        /// The Arduino board will control inner plane to tilt anticlockwise continuously with defined speed.
        /// Speed parameter is optional. Will use 60 as the default setting.
        /// </summary>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool InnerAnticlockwiseContinuousTilt(int speed = 150);

        /// <summary>
        /// This API will send TILT Continuous CMD to Arduino board.
        /// The Arduino board will control outer plane to tilt clockwise continuously with defined speed.
        /// Speed parameter is optional. Will use 60 as the default setting.
        /// </summary>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool OuterClockwiseContinuousTilt(int speed = 150);

        /// <summary>
        /// This API will send TILT Continuous CMD to Arduino board.
        /// The Arduino board will control outer plane to tilt anticlockwise continuously with defined speed.
        /// Speed parameter is optional. Will use 60 as the default setting.
        /// </summary>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool OuterAnticlockwiseContinuousTilt(int speed = 150);

        /// <summary>
        /// This API will send Z axis TILT CMD to Arduino board.
        /// The Arduino board will control plane to tilt clockwise in Z axis for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 2 rpm as the default setting.
        /// The Z axis will stay at 180 degree at start up.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool ZAxisClockwiseTilt(int degrees, int speed = 2);

        /// <summary>
        /// This API will send Z axis TILT CMD to Arduino board.
        /// The Arduino board will control plane to tilt anti-clockwise in Z axis for degrees provided with defined speed.
        /// Speed parameter is optional. Will use 2 rpm as the default setting.
        /// The Z axis will stay at 180 degree at start up.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool ZAxisAnticlockwiseTilt(int degrees, int speed = 2);

        /// <summary>
        /// This API will send Z axis TILT CMD to Arduino board.
        /// The Arduino board will control plane to tilt to defined degrees in Z axis with defined speed.
        /// Speed parameter is optional. Will use 2 rpm as the default setting.
        /// </summary>
        /// <param name="degrees">Indicated the number of degrees to move.</param>
        /// <param name="speed">>Indicated the speed to move.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool ZAxisTiltTo(int degrees, int speed = 2);

        /// <summary>
        /// This API will send HOLDER CMD to Arduino board.
        /// The Arduino board will control holder to lift up for distance mm.
        /// </summary>
        /// <param name="distance">Indicated the number mm of distance to move. Only allow 0-5mm move limit </param>
        /// <returns>Return true if successful</returns>
        virtual bool HolderUp(int distance);

        /// <summary>
        /// This API will send HOLDER CMD to Arduino board.
        /// The Arduino board will control holder to lift down for distance mm.
        /// </summary>
        /// <param name="distance">Indicated the number mm of distance to move. Only allow 0-5mm move limit </param>
        /// <returns>Return true if successful</returns>
        virtual bool HolderDown(int distance);

        /// <summary>
        /// This API will send PLUG USB CMD to Arduino board.
        /// </summary>
        /// <param name="pattern">Indicated the pattern of USB plug in/out. Only allow 1,2,3,4 as its value </param>
        /// pattern = 1, The Arduino board will Turn on No.1 Relay to plug in No.1 USB.
        /// pattern = 2, The Arduino board will Turn off No.1 Relay to plug out No.1 USB.
        /// pattern = 3, The Arduino board will Turn on No.2 Relay to plug in No.2 USB.
        /// pattern = 4, The Arduino board will Turn off No.2 Relay to plug out No.2 USB.
        /// <returns>Return true if successful</returns>
        virtual bool PlugUSB(int pattern);

        /// <summary>
        /// unplug all the USB devices connected to the USB Replugger Device
        /// </summary>\
        /// <returns></returns>
        virtual bool UnplugUSB();

        /// <summary>
        /// plug all the USB devices connected to the USB Replugger Device
        /// </summary>
        /// <returns></returns>
        virtual bool PlugUSB();

        /// <summary>
        /// Start measuring power process
        /// </summary>
        /// <returns></returns>
        virtual bool StartMeasuringPower();

        /// <summary>
        /// Stop measuring power process
        /// </summary>
        /// <returns></returns>
        virtual bool StopMeasuringPower();

        /// <summary>
        /// This API provides the capability of pressing power button by mechanical device.
        /// </summary>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool PressPowerButton();

        /// <summary>
        /// This API provides the capability of pressing power button by mechanical device.
        /// </summary>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool ReleasePowerButton();

        /// <summary>
        /// This API provides the capability of short pressing power button by mechanical device.
        /// </summary>
        /// <param name="timeDuration">Indicated the time duration (milliseconds) of power pusher short press action.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool ShortPressPowerButton(int timeDuration = 400);

        /// <summary>
        /// This API provides the capability of long pressing power button by mechanical device.
        /// </summary>
        /// <param name="timeDuration">Indicated the time duration (milliseconds) of power pusher long press action.</param>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool LongPressPowerButton(int timeDuration = 7000);

        /// <summary>
        /// This API provides the capability of plugging in earphone by mechanical device.
        /// </summary>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool PlugInEarphone();

        /// <summary>
        /// This API provides the capability of plugging out earphone by mechanical device.
        /// </summary>
        /// <returns>return true if the commnd is executed successfully, otherwise false</returns>
        virtual bool PlugOutEarphone();

        /// <summary>
        /// This API will send CONNECT RELAY CMD to Arduino Mega board.
        /// </summary>
        /// <param name="port">Indicated the port of Relay. Only allow 1,2,3,4 as its value </param>
        /// port = 1, The Arduino Mega board will connect No.1 Relay. (default settings)
        /// port = 2, The Arduino Mega board will connect No.2 Relay.
        /// port = 3, The Arduino Mega board will connect No.3 Relay.
        /// port = 4, The Arduino Mega board will connect No.4 Relay.
        /// <returns>Return true if successful</returns>
        virtual bool ConnectRelay(int port);

        /// <summary>
        /// This API will send DISCONNECT RELAY CMD to Arduino Mega board.
        /// </summary>
        /// <param name="port">Indicated the port of Relay. Only allow 1,2,3,4 as its value </param>
        /// port = 1, The Arduino Mega board will disconnect No.1 Relay. (default settings)
        /// port = 2, The Arduino Mega board will disconnect No.2 Relay.
        /// port = 3, The Arduino Mega board will disconnect No.3 Relay.
        /// port = 4, The Arduino Mega board will disconnect No.4 Relay.
        /// <returns>Return true if successful</returns>
        virtual bool DisconnectRelay(int port);

        /// <summary>
        /// Get the current speed value of servo motor by providing the identification
        /// </summary>
        /// <param name="id">the identification of servo motor</param>
        /// <returns>the current speed value of specified servo motor</returns>
        virtual int GetMotorSpeed(int id);

        /// <summary>
        /// Get the current angle value of servo motor by providing the identification
        /// </summary>
        /// <param name="id">the identification of servo motor</param>
        /// <returns>the current angle value of specified servo motor</returns>
        virtual int GetMotorAngle(int id);

        /// <summary>
        /// Get specific FFRD sensor checker item
        /// </summary>
        /// <param name="type">>Indicated the type of sensor checker.</param>
        /// <param name="resultFlag">>Return true if can get sensor data, return false if cannot get sensor data.</param>
        /// <returns>Return the string of acquired sensor data.</returns>
        virtual string GetSepcificSensorCheckerTypeDataFromSensorModule(int type, bool& resultFlag);

        virtual vector<double> ParseSensorData(const string &capString);

        virtual string GetAccelerometerDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z);

        virtual string GetGyroscopeDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z);

        virtual string GetCompassDataFromSensorModule(bool& resultFlag, double& x, double& y, double& z);

        virtual string GetTemperatureDataFromSensorModule(bool& resultFlag, double& temperature);

        virtual string GetPressureDataFromSensorModule(bool& resultFlag, double& pressure);

        virtual string GetAtmosphereDataFromSensorModule(bool& resultFlag, double& atmosphere);

        virtual string GetAltitudeDataFromSensorModule(bool& resultFlag, double& altitude);

        static vector<string> GetComPortsFromRegistry();

        static vector<string> GetSerialPortsFromWMI();

        static vector<string> GetPotentialHWControllerComPorts(vector<string> portsWmiInfo);

    private:
        virtual bool InitSerialPort(const string &comPort);

        virtual void Init();

        virtual void HandleWrite(boost::system::error_code ec, std::size_t bytesTransferred);

        virtual void WriteCommand(int cmd, const string &cmdName);

        //virtual void SyncWriteCommand(int cmd, const string &cmdName);

        virtual void HandleRead(char *buf, boost::system::error_code ec, std::size_t bytesTransferred);

        virtual string ReadCommand(int waitTimeBeforeCommandParameter = waitTimeBeforeCommandUpdate);

        virtual string SyncReadCommand();

        /*
        /// <summary>
        /// read status of Arduino board.
        /// </summary>
        /// <returns>return true if can read the status of Arduino board from serial port successfully</returns>
        virtual bool ReadStatus();
        */

        //virtual bool SendCommand(int cmd, const string &cmdName, bool ignoreRespond = true);

        /// <summary>
        /// send command and get return value
        /// </summary>
        /// <param name="cmd"></param>
        /// <param name="cmdName"></param>
        /// <param name="waitTimeBeforeCommandParameter"> parameter to define whether need more time for sensor data collecting</param>
        /// <returns></returns>
        virtual string SendCommandAndReturnString(int cmd, const string &cmdName, int waitTimeBeforeCommandParameter = waitTimeBeforeCommandUpdate);

        /// <summary>
        /// sync send command and get return value
        /// </summary>
        /// <param name="cmd"></param>
        /// <param name="cmdName"></param>
        /// <returns></returns>
        virtual string SyncSendCommandAndReturnString(int cmd, const string &cmdName);

        //virtual int GetArduinoCmdIndex(int cmd);		

        virtual int BuildArduinoCommand(int command);

        virtual int BuildArduinoCommandWithParams(int command, int param1, int param2);

        virtual int BuildArduinoCommandWithThreeParams(int command, int param1, int param2, int param3);
    };
}

#endif
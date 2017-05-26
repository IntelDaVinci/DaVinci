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

#include <Servo.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <math.h>
#include "ServoCds55.h"
#include "pins_arduino.h"
#include "MyStepper.h"

// I2Cdev and MPU9250 .cpp/.h files must be in the include path of project
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU9250.h"
#include "BMP180.h"

#define Q_TILT_XY_AXES            0x50
#define Q_TILT_CONTINOUS_CW       0x51
#define Q_TILT_CONTINOUS_ACW      0x52
#define Q_TILT                    0x70
#define Q_HOLDER                  0x71
#define Q_NAME                    0x72
#define Q_EARPHONE_PLUG_IN        0x73
#define Q_EARPHONE_PLUG_OUT       0x74
#define Q_ONE_PLUG_IN             0x75
#define Q_ONE_PLUG_OUT            0x76
#define Q_TWO_PLUG_IN             0x77
#define Q_TWO_PLUG_OUT            0x78
#define Q_BUTTON_PRESS            0x79
#define Q_BUTTON_RELEASE          0x7a
#define Q_STATUS                  0x7b
#define Q_CAPABILITY              0x7c
#define Q_MOTOR_SPEED             0x7d
#define Q_MOTOR_ANGLE             0x7e
#define Q_TILT_SPEED              0x7f
#define Q_SHORT_PRESS_BUTTON      0x60
#define Q_LONG_PRESS_BUTTON       0x61
#define Q_Z_ROTATE                0x62
#define Q_HARDWARE                0x63
#define Q_RELAY_ONE_CONNECT       0x64
#define Q_RELAY_ONE_DISCONNECT    0x65
#define Q_RELAY_TWO_CONNECT       0x66
#define Q_RELAY_TWO_DISCONNECT    0x67
#define Q_RELAY_THREE_CONNECT     0x68
#define Q_RELAY_THREE_DISCONNECT  0x69
#define Q_RELAY_FOUR_CONNECT      0x6a
#define Q_RELAY_FOUR_DISCONNECT   0x6b
#define Q_Z_RESET                 0x6c
#define Q_SENSOR_INITIALIZATION   0x40
#define Q_SENSOR_CONNECTION       0x41
#define Q_ACCELEROMETER           0x42
#define Q_GYROSCOPE               0x43
#define Q_COMPASS                 0x44
#define Q_TEMPERATURE             0x45
#define Q_PRESSURE                0x46
#define Q_ATMOSPHERE              0x47
#define Q_ALTITUDE                0x48

#define Q_TILT_ARM0        0
#define Q_TILT_ARM1        1
#define Q_Z_CLOCKWISE      0
#define Q_Z_ANTICLOCKWISE  1
#define Q_HOLDER_UP        0
#define Q_HOLDER_DOWN      1

#define Q_CAP_TILT_DEGREE 0
#define Q_HARDWARE_FFRD_ID           0
#define Q_HARDWARE_XY_TILTER         1
#define Q_HARDWARE_Z_TILTER          2
#define Q_HARDWARE_LIFTER            3
#define Q_HARDWARE_POWER_PUSHER      4
#define Q_HARDWARE_EARPHONE_PLUGGER  5
#define Q_HARDWARE_USB_CUTTER        6
#define Q_HARDWARE_RELAY             7
#define Q_HARDWARE_SENSOR_MODULE     8

#define Q_VALUE_MAX_TILT_DEGREE              90
#define Q_POWER_BUTTON_PUSHER_PRESS_ANGLE    60
#define Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE  80
#define Q_EARPHONE_PLUG_IN_ANGLE             82
#define Q_EARPHONE_PLUG_OUT_ANGLE            112

//Use Macro definition instead for some MCUs have different EEPROM
#define FFRD_ID           0000    //0000 for internal testing, others for release
#define XY_TILTER         1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 0 for accessory.
#define Z_TILTER          1       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 0 for accessory.
#define LIFTER            1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 0 for accessory.
#define POWER_PUSHER      0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 1 for power pusher accesory, 0 for other accessory.
#define EARPHONE_PLUGGER  0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 1 for earphone plugger accessory, 0 for other accessory.
#define USB_CUTTER        1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 1 for USB cutter accessory, 0 for other accessory.
#define RELAY             1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for Mega full version, 0 for UNO version.
#define SENSOR_MODULE     0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for Mega full version, 0 for UNO version.

#define CMD_INDEX(cmd) (cmd >> 24)
#define CMD_ARM_INDEX(cmd) ((cmd & 0x00FF0000) >> 16)
#define CMD_ARG(cmd) (cmd & 0x0000FFFF)

#define HOLDER_UP_PIN             22
#define HOLDER_DOWN_PIN           23
#define RELAY_ONE_PIN             24
#define RELAY_TWO_PIN             25
#define STEPPER_DIRECTION_PIN     52
#define STEPPER_PIN               53
#define PRESS_POWER_BUTTON_PIN    9
#define EARPHONE_PIN              10
#define Z_ENDPOINT_PIN            51
#define RELAY_CONNECT_ONE_PIN     26
#define RELAY_CONNECT_TWO_PIN     27
#define RELAY_CONNECT_THREE_PIN		28
#define RELAY_CONNECT_FOUR_PIN		29

ServoCds55 arm;         // create servo object to control a servo
int arm_pos[2] = { 90, 90 };
Servo touchButtonServo; // create servo object for pressing power button
Servo earphoneServo;    // create servo object for plugging in/out earphone
int micro_switch_value;

const int stepsPerRevolution = 800;   // change this to fit the number of steps per revolution for your motor (400 on the driver, real experimental result: 800)
MyStepper ZStepper(stepsPerRevolution, STEPPER_PIN, STEPPER_DIRECTION_PIN); // initialize the stepper library
int stepCount = 0;         // number of steps the motor has taken

//variables used for sensor checker
MPU9250 accelgyro;
I2Cdev I2C_M;
uint8_t buffer_m[6];
int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t mx, my, mz;
float heading;
float tiltheading;
float Axyz[3];
float Gxyz[3];
float Mxyz[3];
#define sample_num_mdate  5000
volatile float mx_sample[3];
volatile float my_sample[3];
volatile float mz_sample[3];
static float mx_centre = 0;
static float my_centre = 0;
static float mz_centre = 0;
volatile int mx_max = 0;
volatile int my_max = 0;
volatile int mz_max = 0;
volatile int mx_min = 0;
volatile int my_min = 0;
volatile int mz_min = 0;
float temperature;
float pressure;
float atm;
float altitude;
BMP180 Barometer;

void setup()
{
  arm.startSerial(19200);           // softwareSerial
  arm.setAngleLimit(0x01, 0, 300);  // Recover to Servo mode
  arm.setAngleLimit(0x02, 0, 300);
  //phone arm's default degree is 90.
  arm.WritePos(0x01, 150, 150);  //ID:1  Pos:90  velocity:150
  delay(10);
  arm.WritePos(0x02, 150, 150);  //ID:2  Pos:90  velocity:150

  // attach button servo on pin 9
  touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
  // default degree is 100 for new version and 80 for old version
  touchButtonServo.write(Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE);
  // attach earphone servo on pin 10
  earphoneServo.attach(EARPHONE_PIN);
  // default degree is 112
  earphoneServo.write(Q_EARPHONE_PLUG_OUT_ANGLE);

  ZStepper.setSpeed(1);  // set the speed at 1 rpm;

  // sensor checker: join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();

  // in the new firmware: start serial port at 115200 bps and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // Serial.setTimeout() sets the maximum milliseconds to wait for serial data when using Serial.readBytesUntil(), Serial.readBytes(), Serial.parseInt() or Serial.parseFloat(). It defaults to 1000 milliseconds.
  Serial.setTimeout(4);
  Serial.flush();

  if (SENSOR_MODULE == 1)
  {
    // initialize sensor checker device
    //Serial.println("Initializing I2C devices...");
    accelgyro.initialize();
    Barometer.init();
    // verify sensor checker connection
    //Serial.println("Testing device connections...");
    //Serial.println(accelgyro.testConnection() ? "MPU9250 connection successful" : "MPU9250 connection failed");
  }

  // initialize the HOULDER_UP_PIN and HOULDER_DOWN_PIN pins as output IOs:
  pinMode(HOLDER_UP_PIN, OUTPUT);
  pinMode(HOLDER_DOWN_PIN, OUTPUT);
  digitalWrite(HOLDER_UP_PIN, LOW);
  digitalWrite(HOLDER_DOWN_PIN, LOW);

  pinMode(RELAY_ONE_PIN, OUTPUT);
  pinMode(RELAY_TWO_PIN, OUTPUT);
  digitalWrite(RELAY_ONE_PIN, LOW);
  digitalWrite(RELAY_TWO_PIN, LOW);

  pinMode(RELAY_CONNECT_ONE_PIN, OUTPUT);
  pinMode(RELAY_CONNECT_TWO_PIN, OUTPUT);
  pinMode(RELAY_CONNECT_THREE_PIN, OUTPUT);
  pinMode(RELAY_CONNECT_FOUR_PIN, OUTPUT);
  digitalWrite(RELAY_CONNECT_ONE_PIN, LOW);
  digitalWrite(RELAY_CONNECT_TWO_PIN, LOW);
  digitalWrite(RELAY_CONNECT_THREE_PIN, LOW);
  digitalWrite(RELAY_CONNECT_FOUR_PIN, LOW);

  pinMode(Z_ENDPOINT_PIN, INPUT);
}

void loop()
{
  while (Serial.available() > 0)
  {
    // On Arduino Uno, size of int is 16bit, and long is 32bit.
    long cmd = Serial.parseInt();

    //Serial.print("Received cmd: ");
    //Serial.print(cmd);
    //Serial.print(" # ");

    //Serial.print("Command index: ");
    //Serial.print(CMD_INDEX(cmd));
    //Serial.print(" # ");

    //Serial.print("Serial port content: ");
    //Serial.print(Serial.read());
    //Serial.print(" # ");

    if ((Serial.read() == '\n') || (Serial.read() == -1))
    {
      int pos = 0;
      if (CMD_INDEX(cmd) == Q_TILT)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int arm_index = CMD_ARM_INDEX(cmd);
        int degree = CMD_ARG(cmd);

        if (degree != arm_pos[arm_index])
        {
          if (degree < arm_pos[arm_index])
          {
            Serial.println("Tilting to smaller degree!");
            Serial.flush();
            if (arm_pos[arm_index] - degree > 60)
            {
              arm.WritePos(arm_index + 1, degree + 60, 150);
            }
            else if ((arm_pos[arm_index] - degree > 30) && (arm_pos[arm_index] - degree <= 60))
            {
              arm.WritePos(arm_index + 1, degree + 60, 200);
            }
            else if ((arm_pos[arm_index] - degree > 10) && (arm_pos[arm_index] - degree <= 30))
            {
              arm.WritePos(arm_index + 1, degree + 60, 250);
            }
            else
            {
              arm.WritePos(arm_index + 1, degree + 60, 300);
            }
          }
          else
          {
            Serial.println("Tilting to larger degree!");
            Serial.flush();
            if (degree - arm_pos[arm_index] > 60)
            {
              arm.WritePos(arm_index + 1, degree + 60, 150);
            }
            else if ((degree - arm_pos[arm_index] > 30) && (degree - arm_pos[arm_index] <= 60))
            {
              arm.WritePos(arm_index + 1, degree + 60, 200);
            }
            else if ((degree - arm_pos[arm_index] > 10) && (degree - arm_pos[arm_index] <= 30))
            {
              arm.WritePos(arm_index + 1, degree + 60, 250);
            }
            else
            {
              arm.WritePos(arm_index + 1, degree + 60, 300);
            }
          }
          arm_pos[arm_index] = degree;
        }
        else
        {
          // no tilting but return response
          Serial.println("Tilting to same degree");
          Serial.flush();
        }

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_Z_ROTATE)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int rotate_direction = ((cmd & 0x00C00000) >> 22);		//0 for clockwise, 1 for anti-clockwise
        int degree = ((cmd & 0x003FF000) >> 12);
        int speed_z = (cmd & 0x00000FFF);

        if (speed_z >= 5)
        {
          speed_z = 4;   // For saft concern
        }

        double degree_servo = degree * 2.414;  //70:29
        //Serial.print("Raw degree for servo: ");
        //Serial.println(degree_servo);
        int degree_near_int = lround(degree_servo);
        //Serial.print("Int nearest degree for servo: ");
        //Serial.println(degree_near_int);

        double speed_servo = speed_z * 2.414;  //70:29
        //Serial.print("Raw speed for servo: ");
        //Serial.println(speed_servo);
        int speed_near_int = lround(speed_servo);
        //Serial.print("Int nearest speed for servo: ");
        //Serial.println(speed_near_int);

        if (rotate_direction == Q_Z_CLOCKWISE)
        {
          Serial.print("Z axis clockwise tilt: ");
          Serial.print(degree);
          Serial.print(", speed: ");
          Serial.print(speed_z);
          Serial.println(" rpm!");
          Serial.flush();
          ZStepper.setSpeed(speed_near_int);
          ZStepper.anticlockwiseRotate(degree_near_int); //Chong: need confirm with the mechanism
        }
        else if (rotate_direction == Q_Z_ANTICLOCKWISE)
        {
          Serial.print("Z axis anticlockwise tilt: ");
          Serial.print(degree);
          Serial.print(", speed: ");
          Serial.print(speed_z);
          Serial.println(" rpm!");
          Serial.flush();
          ZStepper.setSpeed(speed_near_int);
          ZStepper.clockwiseRotate(degree_near_int); //Chong: need confirm with the mechanism
        }
        else
        {
          Serial.println("Wrong parameter of Z axis tilt!");
          Serial.flush();
        }

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_Z_RESET)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int clockwise_count = 0;
        int anticlockwise_count = 0;
        Serial.print("Total steps for 360 degrees: ");
        Serial.println(stepsPerRevolution);
        ZStepper.setSpeed(2);  // set the speed at 2 rpm;

        //Serial.println("Z axis clockwise rotate to end point !");
        micro_switch_value = digitalRead(Z_ENDPOINT_PIN);  // read input value
        //If no micro switch attached, micro_switch_value == LOW, so will not block normal FFRD
        while (micro_switch_value == HIGH)
        {
          //Serial.println("Micro switch is not pressed!");
          ZStepper.anticlockwiseRotate(1);  //motor: anticlockwise; the tilting pad will rotate clockwise due to the gear!
          clockwise_count++;
          delay(5);
          micro_switch_value = digitalRead(Z_ENDPOINT_PIN);  // read input value
        }

        //Serial.println("Micro switch is pressed!");
        Serial.print("Rotate: ");
        Serial.print(clockwise_count);
        Serial.println(" steps in clockwise direction!");

        if (stepsPerRevolution < clockwise_count * 2)
        {
          ZStepper.clockwiseRotate(5); //Avoid the micrro switch is still pressed!
        }
        micro_switch_value = digitalRead(Z_ENDPOINT_PIN);  // read input value
        //If no micro switch attached, micro_switch_value == LOW, so will not block normal FFRD
        while (micro_switch_value == HIGH)
        {
          //Serial.println("Micro switch is not pressed!");
          ZStepper.clockwiseRotate(1);  //motor: anticlockwise; the tilting pad will rotate clockwise due to the gear!
          anticlockwise_count++;
          delay(5);
          micro_switch_value = digitalRead(Z_ENDPOINT_PIN);  // read input value
        }

        //Serial.println("Micro switch is pressed!");
        Serial.print("Rotate: ");
        Serial.print(anticlockwise_count);
        Serial.println(" steps in anticlockwise direction!");

        //Serial.flush();
        //Stay at 180 degrees where the micro switch is set

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_HOLDER)
      {
        int direct = CMD_ARM_INDEX(cmd);
        int distance = CMD_ARG(cmd);
        if (direct == Q_HOLDER_UP && distance > 0 && distance <= 5)
        {
          Serial.println("Move Holder Up!");
          Serial.flush();
          digitalWrite(HOLDER_UP_PIN, HIGH);
          digitalWrite(HOLDER_DOWN_PIN, LOW);
        }
        if (direct == Q_HOLDER_DOWN && distance > 0 && distance <= 5)
        {
          Serial.println("Move Holder Down!");
          Serial.flush();
          digitalWrite(HOLDER_UP_PIN, LOW);
          digitalWrite(HOLDER_DOWN_PIN, HIGH);
        }
        for (pos = 0; pos < distance; pos++)
          delay(300);
        digitalWrite(HOLDER_UP_PIN, LOW);
        digitalWrite(HOLDER_DOWN_PIN, LOW);
        delay(600);
      }
      else if (CMD_INDEX(cmd) == Q_NAME)
      {
        Serial.println("DaVinci Firmware <version 5.0>");
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_ONE_PLUG_IN)
      {
        Serial.println("Turn off No.1 Relay to plug in No.1 USB!");
        Serial.flush();
        digitalWrite(RELAY_ONE_PIN, LOW);  // turn No.1 Relay off
      }
      else if (CMD_INDEX(cmd) == Q_TWO_PLUG_IN)
      {
        Serial.println("Turn off No.2 Relay to plug in No.2 USB!");
        Serial.flush();
        digitalWrite(RELAY_TWO_PIN, LOW);  // turn No.2 Relay off
      }
      else if (CMD_INDEX(cmd) == Q_ONE_PLUG_OUT)
      {
        Serial.println("Turn on No.1 Relay to plug out No.1 USB!");
        Serial.flush();
        digitalWrite(RELAY_ONE_PIN, HIGH);  // turn No.1 Relay on
      }
      else if (CMD_INDEX(cmd) == Q_TWO_PLUG_OUT)
      {
        Serial.println("Turn on No.2 Relay to plug out No.2 USB!");
        Serial.flush();
        digitalWrite(RELAY_TWO_PIN, HIGH);  // turn No.2 Relay on
      }
      else if (CMD_INDEX(cmd) == Q_BUTTON_PRESS)
      {
        Serial.println("Press the power button!");
        Serial.flush();
        // move the button to the specific position
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_PRESS_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_BUTTON_RELEASE)
      {
        Serial.println("Release the power button!");
        Serial.flush();
        // move the button to default degree
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_SHORT_PRESS_BUTTON)
      {
        Serial.print("Short press the power button for: ");
        int delayParam = CMD_ARG(cmd);
        if (delayParam == 0)
        {
          delayParam = 400;
        }
        Serial.print(delayParam);
        Serial.println(" milliseconds!");
        Serial.flush();
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_PRESS_ANGLE);
        delay(delayParam);
        // move the button to default degree
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_LONG_PRESS_BUTTON)
      {
        Serial.print("Long Press the power button for: ");
        int delayParam = CMD_ARG(cmd);
        Serial.print(delayParam);
        if (delayParam == 0)
        {
          delayParam = 7000;
        }
        Serial.println(" milliseconds!");
        Serial.flush();
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_PRESS_ANGLE);
        delay(delayParam);
        // move the button to default degree
        touchButtonServo.write(Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_EARPHONE_PLUG_IN)
      {
        Serial.println("Plug in the earphone!");
        Serial.flush();
        // plug in the earphone to the specific position
        earphoneServo.write(Q_EARPHONE_PLUG_IN_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_EARPHONE_PLUG_OUT)
      {
        Serial.println("Plug out the earphone!");
        Serial.flush();
        // plug out the earphone to the default degree
        earphoneServo.write(Q_EARPHONE_PLUG_OUT_ANGLE);
        delay(15);
      }
      else if (CMD_INDEX(cmd) == Q_STATUS)
      {
        Serial.println("Read status: ready");
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_CAPABILITY)
      {
        int kind = CMD_ARG(cmd);
        if (kind == Q_CAP_TILT_DEGREE)
        {
          byte value = Q_VALUE_MAX_TILT_DEGREE;
          Serial.println(value);
          Serial.flush();
        }
        else
        {
          Serial.print("Unknown argument: ");
          Serial.println(CMD_ARG(cmd));
          Serial.flush();
        }
      }
      else if (CMD_INDEX(cmd) == Q_HARDWARE)
      {
        int type = CMD_ARG(cmd);
        if (type == Q_HARDWARE_FFRD_ID)
        {
          Serial.print("FFRD ID: ");
          Serial.println(FFRD_ID);
          Serial.flush();
        }
        else if (type == Q_HARDWARE_XY_TILTER)
        {
          if (XY_TILTER == 1)
          {
            Serial.println("X-Y axis tilting plate exists!");
            Serial.flush();
          }
          else if (XY_TILTER == 0)
          {
            Serial.println("X-Y axis tilting plate doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_Z_TILTER)
        {
          if (Z_TILTER == 1)
          {
            Serial.println("Z axis tilting plate exists!");
            Serial.flush();
          }
          else if (Z_TILTER == 0)
          {
            Serial.println("Z axis tilting plate doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_LIFTER)
        {
          if (LIFTER == 1)
          {
            Serial.println("Camera holder lifter exists!");
            Serial.flush();
          }
          else if (LIFTER == 0)
          {
            Serial.println("Camera holder lifter doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_POWER_PUSHER)
        {
          if (POWER_PUSHER == 1)
          {
            Serial.println("Power pusher exists!");
            Serial.flush();
          }
          else if (POWER_PUSHER == 0)
          {
            Serial.println("Power pusher doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_EARPHONE_PLUGGER)
        {
          if (EARPHONE_PLUGGER == 1)
          {
            Serial.println("Earphone plugger exists!");
            Serial.flush();
          }
          else if (EARPHONE_PLUGGER == 0)
          {
            Serial.println("Earphone plugger doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_USB_CUTTER)
        {
          if (USB_CUTTER == 1)
          {
            Serial.println("USB cutter exists!");
            Serial.flush();
          }
          else if (USB_CUTTER == 0)
          {
            Serial.println("USB cutter doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_RELAY)
        {
          if (RELAY == 1)
          {
            Serial.println("Relay controller exists!");
            Serial.flush();
          }
          else if (RELAY == 0)
          {
            Serial.println("Relay controller doesn't exist!");
            Serial.flush();
          }
        }
        else if (type == Q_HARDWARE_SENSOR_MODULE)
        {
          if (SENSOR_MODULE == 1)
          {
            Serial.println("Sensor checker module exists!");
            Serial.flush();
          }
          else if (SENSOR_MODULE == 0)
          {
            Serial.println("Sensor checker module doesn't exist!");
            Serial.flush();
          }
        }
        else
        {
          Serial.print("Unknown argument: ");
          Serial.println(type);
          Serial.flush();
        }
      }
      else if (CMD_INDEX(cmd) == Q_MOTOR_SPEED)
      {
        int id = CMD_ARG(cmd);
        if ((id == 1) || (id == 2))
        {
          Serial.print("The speed of servo motor No.");
          Serial.print(CMD_ARG(cmd));
          int motor_speed = arm.readCurrentSpeed(id);
          Serial.print(" is: ");
          Serial.println(motor_speed);
          Serial.flush();
        }
        else
        {
          Serial.print("Unknown argument: ");
          Serial.println(CMD_ARG(cmd));
          Serial.flush();
        }
      }
      else if (CMD_INDEX(cmd) == Q_MOTOR_ANGLE)
      {
        int id = CMD_ARG(cmd);
        if ((id == 1) || (id == 2))
        {
          Serial.print("The angle of servo motor No.");
          Serial.print(CMD_ARG(cmd));
          int motor_angle = arm.readCurrentAngle(id);
          Serial.print(" is: ");
          Serial.println(motor_angle);
          Serial.flush();
        }
        else
        {
          Serial.print("Unknown argument: ");
          Serial.println(CMD_ARG(cmd));
          Serial.flush();
        }
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_ONE_CONNECT)
      {
        Serial.println("Connect Relay No.1 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_ONE_PIN, HIGH);  // turn No.1 Relay Contoller on
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_ONE_DISCONNECT)
      {
        Serial.println("Disconnect Relay No.1 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_ONE_PIN, LOW);  // turn No.1 Relay Contoller off
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_TWO_CONNECT)
      {
        Serial.println("Connect Relay No.2 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_TWO_PIN, HIGH);  // turn No.2 Relay Contoller on
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_TWO_DISCONNECT)
      {
        Serial.println("Disconnect Relay No.2 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_TWO_PIN, LOW);  // turn No.2 Relay Contoller off
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_THREE_CONNECT)
      {
        Serial.println("Connect Relay No.3 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_THREE_PIN, HIGH);  // turn No.3 Relay Contoller on
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_THREE_DISCONNECT)
      {
        Serial.println("Disconnect Relay No.3 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_THREE_PIN, LOW);  // turn No.3 Relay Contoller off
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_FOUR_CONNECT)
      {
        Serial.println("Connect Relay No.4 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_FOUR_PIN, HIGH);  // turn No.4 Relay Contoller on
      }
      else if (CMD_INDEX(cmd) == Q_RELAY_FOUR_DISCONNECT)
      {
        Serial.println("Disconnect Relay No.4 port!");
        Serial.flush();
        digitalWrite(RELAY_CONNECT_FOUR_PIN, LOW);  // turn No.4 Relay Contoller off
      }
      else if (CMD_INDEX(cmd) == Q_TILT_SPEED)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int arm_index = ((cmd & 0x00C00000) >> 22);
        int degree = ((cmd & 0x003FF000) >> 12);
        int speed = (cmd & 0x00000FFF);

        if (degree != arm_pos[arm_index])
        {
          Serial.print("Arm ");
          Serial.print(arm_index);
          Serial.print(" tilting to degree: ");
          Serial.print(degree);
          Serial.print(", speed: ");
          Serial.println(speed);
          Serial.flush();
          arm.WritePos(arm_index + 1, degree + 60, speed);
          arm_pos[arm_index] = degree;
        }
        else
        {
          // no tilting but return response
          Serial.print("Arm ");
          Serial.print(arm_index);
          Serial.print(" tilting to degree: ");
          Serial.print(degree);
          Serial.print(", speed: ");
          Serial.println(speed);
          Serial.flush();
        }

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_TILT_XY_AXES)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int arm_index = ((cmd & 0x00C00000) >> 22);
        int degree = ((cmd & 0x003FF000) >> 12);
        int speed = (cmd & 0x00000FFF);
        int arm_0_index = Q_TILT_ARM0;
        int arm_0_degree = degree;
        int arm_0_speed = speed;
        int arm_1_index = Q_TILT_ARM1;
        int arm_1_degree = degree;
        int arm_1_speed = speed;

        Serial.print("Arm 0 to degree: ");
        Serial.print(arm_0_degree);
        Serial.print(", speed: ");
        Serial.print(arm_0_speed);
        Serial.flush();
        Serial.print(" # Arm 1 to degree: ");
        Serial.print(arm_1_degree);
        Serial.print(", speed: ");
        Serial.println(arm_1_speed);
        Serial.flush();
        arm.WritePos(arm_0_index + 1, arm_0_degree + 60, arm_0_speed);
        arm.WritePos(arm_1_index + 1, arm_1_degree + 60, arm_1_speed);
        arm_pos[arm_0_index] = arm_0_degree;
        arm_pos[arm_1_index] = arm_1_degree;

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_TILT_CONTINOUS_CW)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int arm_index = CMD_ARM_INDEX(cmd);
        int speed = CMD_ARG(cmd);

        Serial.print("Arm ");
        Serial.print(arm_index);
        Serial.print(" clockwise continous-tilting, speed: ");
        Serial.println(speed);
        Serial.flush();

        //Serial.print("Delay: ");
        //Serial.println((2100 / speed) * 150);
        arm.setClockwiseContinuousRotate(arm_index + 1, speed);
        delay((2100 / speed) * 150);
        arm.setAngleLimit(arm_index + 1, 0, 300);     // Recover to Servo mode, stay at the end point of continuous tilting
        //arm.WritePos(arm_index + 1, 150, 150);        // ID:arm_index + 1  Pos:90  velocity:150

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_TILT_CONTINOUS_ACW)
      {
        touchButtonServo.detach();
        earphoneServo.detach();

        int arm_index = CMD_ARM_INDEX(cmd);
        int speed = CMD_ARG(cmd);

        Serial.print("Arm ");
        Serial.print(arm_index);
        Serial.print(" anticlockwise continous-tilting, speed: ");
        Serial.println(speed);
        Serial.flush();

        //Serial.print("Delay: ");
        //Serial.println((2100 / speed) * 150);
        arm.setAntiClockwiseContinuousRotate(arm_index + 1, speed);
        delay((2100 / speed) * 150);
        arm.setAngleLimit(arm_index + 1, 0, 300);			// Recover to Servo mode, stay at the end point of continuous tilting
        //arm.WritePos(arm_index + 1, 150, 150);  			// ID:arm_index + 1  Pos:90  velocity:150

        touchButtonServo.attach(PRESS_POWER_BUTTON_PIN);
        earphoneServo.attach(EARPHONE_PIN);
      }
      else if (CMD_INDEX(cmd) == Q_SENSOR_INITIALIZATION)
      {
        Serial.println("Initializing I2C sensor checker modules!");
        Serial.flush();
        accelgyro.initialize();
        Barometer.init();
      }
      else if (CMD_INDEX(cmd) == Q_SENSOR_CONNECTION)
      {
        bool connectionState = accelgyro.testConnection();
        if (connectionState == true)
        {
          Serial.println("MPU9250 sensor checker modules connection is successful!" );
        }
        else
        {
          Serial.println("MPU9250 sensor checker modules connection is failed!");
        }
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_ACCELEROMETER)
      {
        getAccel_Data();
        Serial.print("Acceleration (g) of X,Y,Z: ");
        Serial.print(Axyz[0]);
        Serial.print(",");
        Serial.print(Axyz[1]);
        Serial.print(",");
        Serial.println(Axyz[2]);
      }
      else if (CMD_INDEX(cmd) == Q_GYROSCOPE)
      {
        getGyro_Data();
        Serial.print("Gyro (degrees/s) of X,Y,Z: ");
        Serial.print(Gxyz[0]);
        Serial.print(",");
        Serial.print(Gxyz[1]);
        Serial.print(",");
        Serial.println(Gxyz[2]);
      }
      else if (CMD_INDEX(cmd) == Q_COMPASS)
      {
        getCompass_Data(); // compass data has not been calibrated here
        Serial.print("Compass Value of X,Y,Z: ");
        Serial.print(Mxyz[0]);
        Serial.print(",");
        Serial.print(Mxyz[1]);
        Serial.print(",");
        Serial.println(Mxyz[2]);
      }
      else if (CMD_INDEX(cmd) == Q_TEMPERATURE)
      {
        temperature = Barometer.bmp180GetTemperature(Barometer.bmp180ReadUT()); //Get the temperature, bmp180ReadUT MUST be called first
        Serial.print("Temperature: ");
        Serial.print(temperature, 2); //display 2 decimal places
        Serial.println(" deg C");
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_PRESSURE)
      {
        pressure = Barometer.bmp180GetPressure(Barometer.bmp180ReadUP()); //Get the pressure
        Serial.print("Pressure: ");
        //Serial.print(pressure, 0); //whole number only.
        Serial.print(pressure, 2);  //display 2 decimal places
        Serial.println(" Pa");
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_ATMOSPHERE)
      {
        pressure = Barometer.bmp180GetPressure(Barometer.bmp180ReadUP()); //Get the pressure
        atm = pressure / 101325;
        Serial.print("Related Atmosphere: ");
        Serial.println(atm, 4); //display 4 decimal places
        Serial.flush();
      }
      else if (CMD_INDEX(cmd) == Q_ALTITUDE)
      {
        altitude = Barometer.calcAltitude(pressure); //Uncompensated caculation - in Meters
        Serial.print("Altitude: ");
        Serial.print(altitude, 2); //display 2 decimal places
        Serial.println(" m");
        Serial.flush();
      }
      else
      {
        Serial.print("Unknown command: ");
        Serial.println(CMD_INDEX(cmd));
        Serial.flush();
        continue;
      }
    }
    else
    {
      Serial.println("Command is not finished!");
      Serial.flush();
    }
  }
}

void getAccel_Data(void)
{
  accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
  Axyz[0] = (double) ax / 16384;
  Axyz[1] = (double) ay / 16384;
  Axyz[2] = (double) az / 16384;
}

void getGyro_Data(void)
{
  accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
  Gxyz[0] = (double) gx * 250 / 32768;
  Gxyz[1] = (double) gy * 250 / 32768;
  Gxyz[2] = (double) gz * 250 / 32768;
}

void getCompass_Data(void)
{
  I2C_M.writeByte(MPU9150_RA_MAG_ADDRESS, 0x0A, 0x01); //enable the magnetometer
  delay(10);
  I2C_M.readBytes(MPU9150_RA_MAG_ADDRESS, MPU9150_RA_MAG_XOUT_L, 6, buffer_m);

  mx = ((int16_t)(buffer_m[1]) << 8) | buffer_m[0] ;
  my = ((int16_t)(buffer_m[3]) << 8) | buffer_m[2] ;
  mz = ((int16_t)(buffer_m[5]) << 8) | buffer_m[4] ;

  Mxyz[0] = (double) mx * 1200 / 4096;
  Mxyz[1] = (double) my * 1200 / 4096;
  Mxyz[2] = (double) mz * 1200 / 4096;
}

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
#include "ServoCds55.h"
#include "pins_arduino.h"

#define Q_TILT                 0x70
#define Q_HOLDER               0x71
#define Q_NAME                 0x72
#define Q_EARPHONE_PLUG_IN     0x73
#define Q_EARPHONE_PLUG_OUT    0x74
#define Q_ONE_PLUG_IN          0x75
#define Q_ONE_PLUG_OUT         0x76
#define Q_TWO_PLUG_IN          0x77
#define Q_TWO_PLUG_OUT         0x78
#define Q_BUTTON_PRESS         0x79
#define Q_BUTTON_RELEASE       0x7a
#define Q_STATUS               0x7b
#define Q_CAPABILITY           0x7c
#define Q_MOTOR_SPEED          0x7d
#define Q_MOTOR_ANGLE          0x7e
#define Q_SHORT_PRESS_BUTTON   0x60
#define Q_LONG_PRESS_BUTTON    0x61
#define Q_Z_ROTATE             0x62
#define Q_HARDWARE             0x63

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

#define Q_VALUE_MAX_TILT_DEGREE              90
#define Q_POWER_BUTTON_PUSHER_PRESS_ANGLE    60
#define Q_POWER_BUTTON_PUSHER_RELEASE_ANGLE  80
#define Q_EARPHONE_PLUG_IN_ANGLE             82
#define Q_EARPHONE_PLUG_OUT_ANGLE            112

#define ADDRESS_FFRD_ID             0  // 4 bytes
#define ADDRESS_HAVING_TILTER       4  // have tilting plate?
#define ADDRESS_HAVING_LIFTER       5  // have lifter for camera
#define ADDRESS_HAVING_POWERPUSHER  6  // have power pusher
#define ADDRESS_RESERVED            7  // reserved
#define ADDRESS_MAX_TILT_DEGREE     8  // 1 byte

//Use Macro definition instead for some MCUs have different EEPROM
#define FFRD_ID           0000    //0000 for internal testing, others for release
#define XY_TILTER         1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 0 for accessory.
#define Z_TILTER          0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 0 for accessory.
#define LIFTER            1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 0 for accessory.
#define POWER_PUSHER      0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 1 for power pusher accesory, 0 for other accessory.
#define EARPHONE_PLUGGER  0       //0 for not having this hardware, 1 for having this hardware. Default value is 0 for full version, 1 for earphone plugger accessory, 0 for other accessory.
#define USB_CUTTER        1       //0 for not having this hardware, 1 for having this hardware. Default value is 1 for full version, 1 for USB cutter accessory, 0 for other accessory.

#define CMD_INDEX(cmd) (cmd >> 24)
#define CMD_ARM_INDEX(cmd) ((cmd & 0x00FF0000) >> 16)
#define CMD_ARG(cmd) (cmd & 0x0000FFFF)

//#define ARM0_PIN                  2
//#define ARM1_PIN                  3
#define HOLDER_UP_PIN             5
#define HOLDER_DOWN_PIN           6
#define RELAY_ONE_PIN             4
#define RELAY_TWO_PIN             7
#define PRESS_POWER_BUTTON_PIN    9
#define EARPHONE_PIN              10
#define Z_CLOCKWISE_PIN           2
#define Z_ANTICLOCKWISE_PIN       3

ServoCds55 arm;         // create servo object to control a servo
int arm_pos[2] = { 90, 90 };
Servo touchButtonServo; // create servo object for pressing power button
Servo earphoneServo;    // create servo object for plugging in/out earphone

void setup() 
{
  arm.startSerial(19200);        // softwareSerial
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

  // start serial port at 9600 bps and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.flush();

  // initialize the HOULDER_UP_PIN and HOULDER_DOWN_PIN pins as output IOs:
  pinMode(HOLDER_UP_PIN, OUTPUT);      
  pinMode(HOLDER_DOWN_PIN, OUTPUT);
  digitalWrite(HOLDER_UP_PIN, LOW);
  digitalWrite(HOLDER_DOWN_PIN, LOW);

  pinMode(RELAY_ONE_PIN, OUTPUT);
  pinMode(RELAY_TWO_PIN, OUTPUT);
  digitalWrite(RELAY_ONE_PIN, LOW);
  digitalWrite(RELAY_TWO_PIN, LOW);

  pinMode(Z_CLOCKWISE_PIN, OUTPUT);
  pinMode(Z_ANTICLOCKWISE_PIN, OUTPUT);
  digitalWrite(Z_CLOCKWISE_PIN, LOW);      //motor break
  digitalWrite(Z_ANTICLOCKWISE_PIN, LOW);  //motor break  
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
        int arm_index = CMD_ARM_INDEX(cmd);
        int degree = CMD_ARG(cmd);

        if (degree != arm_pos[arm_index])
        {
          if (degree < arm_pos[arm_index]) 
          {
            Serial.println("Tilting 1!");
            Serial.flush();
            arm.WritePos(arm_index+1, degree+60, 150);  //ID:arm_index+1  Pos:pos  velocity:150          
          }
          else 
          {
            Serial.println("Tilting 2!");
            Serial.flush();
            arm.WritePos(arm_index+1, degree+60, 150);  //ID:arm_index+1  Pos:pos  velocity:150
          }
          arm_pos[arm_index] = degree; 
        }
        else
        {
          // no tilting but return response
          Serial.println("Tilting 0");
          Serial.flush();
        }
      }
      else if (CMD_INDEX(cmd) == Q_Z_ROTATE)
      {
        int rotate_direction = CMD_ARM_INDEX(cmd);  //0 for clockwise, 1 for anti-clockwise
        int rotate_time = CMD_ARG(cmd);
        
        if (rotate_direction == Q_Z_CLOCKWISE && rotate_time >= 0)
        {        
          Serial.print("Z axis clockwise rotate for: ");
          Serial.print(rotate_time);
          Serial.println(" milliseconds!");
          Serial.flush();
          digitalWrite(Z_CLOCKWISE_PIN, HIGH);
          digitalWrite(Z_ANTICLOCKWISE_PIN, LOW);
          delay(rotate_time);
          digitalWrite(Z_CLOCKWISE_PIN, LOW);      //motor break
          digitalWrite(Z_ANTICLOCKWISE_PIN, LOW);  //motor break
          delay(15); 
        }
        else if (rotate_direction == Q_Z_ANTICLOCKWISE && rotate_time >= 0)
        {        
          Serial.print("Z axis anticlockwise rotate for: ");
          Serial.print(rotate_time);
          Serial.println(" milliseconds!");
          Serial.flush();
          digitalWrite(Z_CLOCKWISE_PIN, LOW);
          digitalWrite(Z_ANTICLOCKWISE_PIN, HIGH);
          delay(rotate_time);
          digitalWrite(Z_CLOCKWISE_PIN, LOW);      //motor break
          digitalWrite(Z_ANTICLOCKWISE_PIN, LOW);  //motor break
          delay(15); 
        }
        else
        {
          Serial.print("Wrong parameter of Z axis rotation time: ");
          Serial.print(rotate_time);
          Serial.println(" milliseconds!");
          Serial.flush();
          delay(15); 
        }  
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
        for(pos = 0; pos < distance; pos++)
          delay(300);
        digitalWrite(HOLDER_UP_PIN, LOW);
        digitalWrite(HOLDER_DOWN_PIN, LOW);
        delay(600);
      }
      else if (CMD_INDEX(cmd) == Q_NAME)
      {
        Serial.println("DaVinci Firmware <version 3.2>");
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
          //byte value = EEPROM.read(ADDRESS_MAX_TILT_DEGREE);    //For Arduino UNO R3
          byte value = Q_VALUE_MAX_TILT_DEGREE;
          //Serial.print("Max_tilt_degree: ");
          Serial.print(value, DEC);
          Serial.println();
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

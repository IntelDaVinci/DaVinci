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
 ServoCds55.cpp - Library for Digital Servo Driver Shield
 May 15th, 2013
 Version 0.1
 */

#include "ServoCds55.h"
#include "Arduino.h"
#include "SoftwareSerial.h"
SoftwareSerial mySerial(11, 12);

byte command = 0;
boolean Servomode;
boolean PositionLimit;
boolean Motormode;
boolean IDreset;
boolean ServoRst;
int Command[20];
int reclength = 0;
int Pos;
int ID,newID;
int velocity;
int PosLimit;
int upperLimit;

#define startByte 0xFF
#define CLOCKWISE      1
#define ANTICLOCKWISE  0

#define P_MODEL_NUMBER_L				0	//0x00
#define P_MODEL_NUMBER_H				1	//0x01
#define P_VERSION					    2	//0x02
#define P_ID						    3	//0x03
#define P_BAUD_RATE					    4	//0x04
#define P_RETURN_DELAY_TIME				5	//0x05
#define P_CW_ANGLE_LIMIT_L				6	//0x06		// CW: clockwise
#define P_CW_ANGLE_LIMIT_H				7	//0x07
#define P_CCW_ANGLE_LIMIT_L				8	//0x08		// CCW: counter-clockwise
#define P_CCW_ANGLE_LIMIT_H				9	//0x09
#define P_SYSTEM_DATA2					10	//0x0A
#define P_LIMIT_TEMPERATURE				11	//0x0B
#define P_MIN_LIMIT_VOLTAGE				12	//0x0C
#define P_MAX_LIMIT_VOLTAGE				13	//0x0D
#define P_MAX_TORQUE_L					14	//0x0E
#define P_MAX_TORQUE_H					15	//0x0F
#define P_RESPONSE_LEVEL				16	//0x10
#define P_ALARM_LED					    17	//0x11
#define P_UNLOADING_CONDITION			18	//0x12
#define P_OPERATING_MODE				19	//0x13
#define P_MIN_CALIBRATION_L				20	//0x14
#define P_MIN_CALIBRATION_H				21	//0x15
#define P_MAX_CALIBRATION_L				22	//0x16
#define P_MAX_CALIBRATION_H				23	//0x17
#define P_TORQUE_ENABLE					24	//0x18
#define P_LED_ENABLE					25	//0x19
#define P_CW_DEAD_ZONE					26	//0x1A
#define P_CCW_DEAD_ZONE					27	//0X1B
#define P_CW_PROPORTION_COEFFICIENT		28	//0x1C
#define P_CCW_PROPORTION_COEFFICIENT	29	//0x1D
#define P_GOAL_POSITION_L				30	//0x1E
#define P_GOAL_POSITION_H				31	//0x1F
#define P_OPERATION_SPEED_L				32	//0x20
#define P_OPERATION_SPEED_H				33	//0x21
#define P_ACCELERATION					34	//0x22
#define P_DECELERATION					35	//0x23
#define P_PRESENT_POSITION_L			36	//0x24
#define P_PRESENT_POSITION_H			37	//0x25
#define P_PRESENT_SPEED_L				38	//0x26
#define P_PRESENT_SPEED_H				39	//0x27
#define P_PRESENT_LOAD_L				40	//0x28
#define P_PRESENT_LOAD_H				41	//0x29
#define P_PRESENT_VOLTAGE				42	//0x2A
#define P_PRESENT_TEMPERATURE			43	//0x2B
#define P_REG_WRITE_FLAG				44	//0x2C
#define P_PAUSE_TIME					45	//0x2D
#define P_OPERATION_FLAG				46	//0x2E
#define P_LOCK_FLAG					    47	//0x2F
#define P_MIN_PWM_L					    48	//0x30
#define P_MIN_PWM_H					    49	//0x31

//— Instruction —
#define INST_PING					    0x01
#define INST_READ					    0x02
#define INST_WRITE					    0x03
#define INST_REG_WRITE					0x04
#define INST_ACTION					    0x05
#define INST_RESET					    0x06
#define INST_DIGITAL_RESET				0x07
#define INST_SYSTEM_READ				0x0C
#define INST_SYSTEM_WRITE				0x0D
#define INST_SYNC_WRITE					0x83
#define INST_SYNC_REG_WRITE				0x84


ServoCds55::ServoCds55()
{
  velocity_temp = 150;
  upperLimit_temp = 240;
}

void ServoCds55::startSerial(long baudRate)
{
  mySerial.begin(baudRate);
}

int ServoCds55::readBaudrate(int ID)
{
  int rev[10];
  int i = 0;
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x04);
  mySerial.write(0x02);
  mySerial.write(0x04);
  mySerial.write(0x01);
  mySerial.write((~(ID + 0x04 + 0x02 + 0x04 + 0x01)) & 0xFF);

  delay(10);
  while (mySerial.available() > 0)
  {
    rev[i] = mySerial.read();
    i++;
  }
  i = 0;
  return rev[5];
}

void ServoCds55::setBaudrate(int ID, int baudRate)
{
  Serial.begin(1000000);
  int messageLength = 4;
  Serial.write(startByte);              // send some data
  Serial.write(startByte);
  Serial.write(ID);
  Serial.write(messageLength);
  Serial.write(INST_WRITE);
  Serial.write(0x04);
  Serial.write(baudRate);
  Serial.write((~(ID + (messageLength) + INST_WRITE + 0x04 + baudRate)) & 0xFF);

  delayMicroseconds(10);
  while(Serial.read() > 0);
  Serial.end();
}

void ServoCds55::setVelocity(int velocity)
{
  //set servo velocity
  velocity_temp = velocity;
}

void ServoCds55::setPoslimit(int posLimit)
{
  // set servo pos limit
  upperLimit_temp =  posLimit;
}

void ServoCds55::write(int ID, int Pos)
{
  //  Servo Mode
  SetServoUpperLimit(ID, upperLimit_temp);
  WritePos(ID, Pos, velocity_temp);			// default velocity:150
}

void ServoCds55::WritePos(int ID, int Pos, int velocity)
{
  int messageLength = 7;
  int rev[30];
  int i = 0;
  velocity = map(velocity, 0, 300, 0x0000, 0x03FF);
  Pos = map(Pos, 0, 300, 0x0000, 0x03FF);

  byte pos2 = (Pos >> 8 & 0xff);		// chong: high-order position
  byte pos = (Pos & 0xff);			    // chong: low-order position

  byte vel2 =  (velocity >> 8 & 0xff);
  byte vel =  (velocity & 0xff);

  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(P_GOAL_POSITION_L);
  mySerial.write(pos);
  mySerial.write(pos2);
  mySerial.write(vel);
  mySerial.write(vel2);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + P_GOAL_POSITION_L + pos + pos2 + vel + vel2)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

//Clockwise
void ServoCds55::SetServoUpperLimit(int ID, int upperLimit)
{
  int i;
  int rev[20];
  int messageLength = 5;
  PosLimit = map(upperLimit, 0, 300, 0x0000, 0x03FF);
  byte PosLimitB = (upperLimit >> 8 & 0xff);
  byte PosLimitS = (upperLimit & 0xff);
  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(0x08);
  mySerial.write(PosLimitB);
  mySerial.write(PosLimitS);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + 0x08 + PosLimitB + PosLimitS)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

//Anticlockwise
void ServoCds55::SetServoLowerLimit(int ID, int lowerLimit)
{
  int i;
  int rev[20];
  int messageLength = 5;
  PosLimit = map(lowerLimit, 0, 300, 0x0000, 0x03FF);
  byte PosLimitB = (lowerLimit >> 8 & 0xff);
  byte PosLimitS = (lowerLimit & 0xff);
  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(0x06);
  mySerial.write(PosLimitB);
  mySerial.write(PosLimitS);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + 0x06 + PosLimitB + PosLimitS)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

void ServoCds55::SetID(int ID, int newID)
{
  int i;
  int rev[20];
  int messageLength = 4;
  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(P_ID);
  mySerial.write(newID);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + P_ID + newID)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

void ServoCds55::Reset(int ID)
{
  int i;
  int rev[20];
  // renew ID:1    Baund:1M
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x02);
  mySerial.write(0x06);
  mySerial.write((~(ID + 0x02 + 0x06)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

int ServoCds55::readCurrentAngle(int ID)
{
  int rev[20];
  int angle = 0;
  int i = 0;
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x04);
  mySerial.write(0x02);
  mySerial.write(0x24);		//P_PRESENT_POSITION_L
  mySerial.write(0x02);
  mySerial.write((~(ID + 0x04 + 0x02 + 0x24 + 0x02)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
  angle = rev[5]+ rev[6] * 256;
  angle = map(angle, 0, 0x03FF, 0, 300);
  return angle;
}

int ServoCds55::readCurrentSpeed(int ID)
{
  int rev[20];
  int speed = 0;
  int i = 0;
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x04);
  mySerial.write(0x02);
  mySerial.write(0x26);		//P_PRESENT_POSITION_L
  mySerial.write(0x02);
  mySerial.write((~(ID + 0x04 + 0x02 + 0x26 + 0x02)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
  speed = rev[5]+ rev[6] * 256;
  speed = map(speed, 0, 0x03FF, 0, 300);
  return speed;
}

void ServoCds55::setCurrentSpeed(int ID, int velocity)
{
  int messageLength = 5;
  int rev[20];
  int i = 0;
  velocity = map(velocity, 0, 300, 0x0000, 0x03FF);	    
 
  byte velH =  (velocity >> 8 & 0xff);  // chong: high-order
  byte velL =  (velocity & 0xff);       // chong: low-order

  mySerial.write(startByte);            // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(P_OPERATION_SPEED_L);
  mySerial.write(velL);
  mySerial.write(velH);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + P_OPERATION_SPEED_L + velL + velH)) & 0xFF);
  
  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

void ServoCds55::syncWrite(int Pos1, int velocity1, int Pos2, int velocity2)
{
  int messageLength = (4 + 1) * 2 + 4;

  velocity1 = map(velocity1, 0, 300, 0x0000, 0x03FF);
  Pos1 = map(Pos1, 0, 300, 0x0000, 0x03FF);

  velocity2 = map(velocity2, 0, 300, 0x0000, 0x03FF);
  Pos2 = map(Pos2, 0, 300, 0x0000, 0x03FF);

  byte pos1_2 = (Pos1 >> 8 & 0xff);
  byte pos1_1 = (Pos1 & 0xff);

  byte vel1_2 =  (velocity >> 8 & 0xff);
  byte vel1_1 =  (velocity & 0xff);

  byte pos2_2 = (Pos2 >> 8 & 0xff);
  byte pos2_1 = (Pos2 & 0xff);

  byte vel2_2 =  (velocity2 >> 8 & 0xff);
  byte vel2_1 =  (velocity2 & 0xff);

  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(0xFE);//broadcast
  mySerial.write(messageLength);
  mySerial.write(INST_SYNC_WRITE);
  mySerial.write(P_GOAL_POSITION_L);//0x1E
  mySerial.write(0x04);//L
  mySerial.write(0x01);//1st servo
  mySerial.write(pos1_1);
  mySerial.write(pos1_2);
  mySerial.write(vel1_1);
  mySerial.write(vel1_2);
  mySerial.write(0x02);//2nd servo
  mySerial.write(pos2_1);
  mySerial.write(pos2_2);
  mySerial.write(vel2_1);
  mySerial.write(vel2_2);
  mySerial.write((~(0xFE + (messageLength) + INST_SYNC_WRITE + P_GOAL_POSITION_L + 0x04 + 0x01 + pos1_1 + pos1_2 + vel1_1 + vel1_2 + 0x02 + pos2_1 + pos2_2 + vel2_1 + vel2_2)) & 0xFF);
  //broadcast : return none
}

int ServoCds55::readAcceleration(int ID)
{
  int rev[10];
  int i = 0;
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x04);
  mySerial.write(0x02);
  mySerial.write(0x22);
  mySerial.write(0x01);
  mySerial.write((~(ID + 0x04 + 0x02 +0x22 + 0x01)) & 0xFF);

  delay(10);
  while (mySerial.available() > 0)
  {
    rev[i] = mySerial.read();
    i++;
  }
  i = 0;
  return rev[5];
}

void ServoCds55::setAcceleration(int ID, int acceleration)
{
  int i;
  int rev[20];
  acceleration = map(acceleration, 0, 255, 0x00, 0xFF);
  int messageLength = 4;
  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(0x22);
  mySerial.write(acceleration);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + 0x22 + acceleration)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

int ServoCds55::readDeceleration(int ID)
{
  int rev[10];
  int i = 0;
  mySerial.write(startByte);
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(0x04);
  mySerial.write(0x02);
  mySerial.write(0x23);
  mySerial.write(0x01);
  mySerial.write((~(ID + 0x04 + 0x02 +0x23 + 0x01)) & 0xFF);

  delay(10);
  while (mySerial.available() > 0)
  {
    rev[i] = mySerial.read();
    i++;
  }
  i = 0;
  return rev[5];
}

void ServoCds55::setDeceleration(int ID, int deceleration)
{
  int i;
  int rev[20];
  deceleration = map(deceleration, 0, 255, 0x00, 0xFF);
  int messageLength = 4;
  mySerial.write(startByte);              // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(0x23);
  mySerial.write(deceleration);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + 0x23 + deceleration)) & 0xFF);

  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

void ServoCds55::setAngleLimit(int ID, int lowerLimit, int upperLimit)
{
  SetServoUpperLimit(ID, upperLimit);   //Anticlockwise
  SetServoLowerLimit(ID, lowerLimit);   //Clockwise  
}

void ServoCds55::setContinuousMode(int ID)
{
  setAngleLimit(ID, 0, 0);
}

void ServoCds55::setContinuousVelocity(int ID, int direction, int velocity)
{
  if ((direction != 0) && (direction != 1))
  {
      direction = 0;
  }

  int messageLength = 4;
  int rev[20];
  int i = 0;
  velocity = map(velocity, 0, 300, 0x0000, 0x03FF);	    

  byte velH =  (velocity >> 8 & 0xff) + (direction << 2);   // chong: high-order
  byte velL =  (velocity & 0xff);                           // chong: low-order

  mySerial.write(startByte);            // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(P_OPERATION_SPEED_L);
  mySerial.write(velL);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + P_OPERATION_SPEED_L + velL)) & 0xFF);
  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }

  mySerial.write(startByte);            // send some data
  mySerial.write(startByte);
  mySerial.write(ID);
  mySerial.write(messageLength);
  mySerial.write(INST_WRITE);
  mySerial.write(P_OPERATION_SPEED_H);
  mySerial.write(velH);
  mySerial.write((~(ID + (messageLength) + INST_WRITE + P_OPERATION_SPEED_H + velH)) & 0xFF);
  delay(10);
  i = 0;
  while (mySerial.available())
  {
    rev[i] = mySerial.read();
    //     delay(1);
    i++;
  }
}

void ServoCds55::setClockwiseContinuousRotate(int ID, int velocity)
{
  setContinuousMode(ID);
  setContinuousVelocity(ID, CLOCKWISE, velocity);
}

void ServoCds55::setAntiClockwiseContinuousRotate(int ID, int velocity)
{
  setContinuousMode(ID);
  setContinuousVelocity(ID, ANTICLOCKWISE, velocity);
}

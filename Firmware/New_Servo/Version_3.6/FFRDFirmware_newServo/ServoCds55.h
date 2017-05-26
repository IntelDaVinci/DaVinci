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
 ServoCds55.h - Library for Digital Servo Driver Shield
 May 15th, 2013
 Version 0.1
 */


#ifndef ServoCds55_H
#define ServoCds55_H
#include "Arduino.h"
#include "SoftwareSerial.h"


class ServoCds55
{
public:
  ServoCds55();
  void startSerial(long baudRate);
  int readBaudrate(int ID);
  void setBaudrate(int ID,int baudRate);
  //    void WritePos(int ID,int Pos);
  void WritePos(int ID,int Pos,int velocity);
  void write(int ID,int Pos);
  void setVelocity(int velocity);
  void setPoslimit(int posLimit);
  void SetServoLimit(int ID,int upperLimit);
  void SetID(int ID, int newID);
  void Reset(int ID);
  int readCurrentAngle(int ID);
  int readCurrentSpeed(int ID);
  void syncWrite(int Pos1,int velocity1,int Pos2,int velocity2);
  int readAcceleration(int ID);
  void setAcceleration(int ID,int acceleration);
  int readDeceleration(int ID);
  void setDeceleration(int ID,int deceleration);

private:
  int velocity_temp;
  int upperLimit_temp;
};

#endif





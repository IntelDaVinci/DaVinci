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

#define startByte 0xFF

#define P_GOAL_POSITION_L (30)

#define P_ID	3	//0x03

#define INST_WRITE 0x03



void setup() {

}

void loop() {
  // put your main code here, to run repeatedly:
  
    setBaudrate(0x01,0x67);

    delay(1000);
    
    int SET_ID_OK = 0;
    if(SET_ID_OK != 1)  //only for new ID
    {
      SetID(0x01,0x02);
      SET_ID_OK = 1;
    }
    if(SET_ID_OK)
    {
      WritePos(0x02,150,200);
      delay(1000);
      WritePos(0x02,90,200);
      delay(1000);
    }else
    {
      WritePos(0x01,150,200);
      delay(1000);
      WritePos(0x01,90,200);
      delay(1000);
    }
}


void setBaudrate(int ID,int baudRate)
{
  int i=0;
  int rev[30];
  int messageLength = 4;
  Serial.begin (1000000);
  Serial.write(0xFF);              // send some data
  Serial.write(0xFF);
  Serial.write(ID);
  Serial.write(messageLength);
  Serial.write(0x03);
  Serial.write(0x04);
  Serial.write(baudRate);
  Serial.write((~(ID + (messageLength) + 0x03+ 0x04 + baudRate))&0xFF);
  
  delay(10);
  while (Serial.available() > 0)  
  {
    rev[i] = Serial.read(); 
    i++;
  }
  Serial.end();
}


void WritePos(int ID,int Pos,int velocity){
  int messageLength = 7;
  int rev[30];
  int i=0;
  Serial.begin (19200);
  velocity=map(velocity,0,300,0x0000,0x03FF);
  Pos=map(Pos,0,300,0x0000,0x03FF);
  
  byte pos2 = (Pos>>8 & 0xff);
  byte pos = (Pos & 0xff); 	

  byte vel2 =  (velocity>>8 & 0xff);
  byte vel =  (velocity & 0xff);

  Serial.write(startByte);              // send some data
  Serial.write(startByte);
  Serial.write(ID);
  Serial.write(messageLength);
  Serial.write(INST_WRITE);
  Serial.write(P_GOAL_POSITION_L);
  Serial.write(pos);
  Serial.write(pos2);
  Serial.write(vel);
  Serial.write(vel2);
  Serial.write((~(ID + (messageLength) + INST_WRITE + P_GOAL_POSITION_L + pos + pos2 + vel + vel2))&0xFF);
  
  delay(10);
  while (Serial.available() > 0)  
  {
    rev[i] = Serial.read(); 
    i++;
  }
  Serial.end();
}


void SetID(int ID, int newID)
{
  int i;
  int rev[20];
  int messageLength = 4;
  Serial.begin (19200);
  Serial.write(startByte);              // send some data
  Serial.write(startByte);
  Serial.write(ID);
  Serial.write(messageLength);
  Serial.write(INST_WRITE);
  Serial.write(P_ID);
  Serial.write(newID);
  Serial.write((~(ID + (messageLength) + INST_WRITE + P_ID + newID)) & 0xFF);

  delay(10);
  i = 0;
  while (Serial.available())  
  {
    rev[i] = Serial.read(); 
    //     delay(1); 
    i++;
  }
  Serial.end();
}

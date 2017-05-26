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

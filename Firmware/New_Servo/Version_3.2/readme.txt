When firstly use the new servo, please upload the Initialize_newServo.ino to Arduino board. It will initialize the Baud rate and ID number of the new servo. If it doesnot have to change the ID number, please comment out 
if(SET_ID_OK != 1)  //only for new ID
{
    SetID(0x01,0x02);
    SET_ID_OK = 1;
} 
Then please upload FFRDFirmware_newServo.ino to Arduino board before launching DaVinci project.
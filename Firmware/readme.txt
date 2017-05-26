EEPROMWriter: write the device ID and capability into EEPROM.
    Upload it before uploading firmware.

FFRDFirmware: the source code of FFRD firmware.


TODO:
1. write a script on windows/linux to:
    1) read description file for FFRD. 
    1) generate EEPROMWriter with the device ID, capability
       (including having tilting plate, having lifter, 
       having power pusher, maximum tilting degree and etc.)
       and upload it to Arduino board.
    2) upload FFRDFirmware.
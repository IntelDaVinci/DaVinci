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

#include <EEPROM.h>

#define ADDRESS_FFRD_ID             0  // 4 bytes
#define ADDRESS_HAVING_TILTER       4  // have tilting plate?
#define ADDRESS_HAVING_LIFTER       5  // have lifter for camera
#define ADDRESS_RESERVED1           6  // reserved
#define ADDRESS_RESERVED2           7  // reserved
#define ADDRESS_MAX_TILT_DEGREE     8  // 1 byte

#define VALUE_HAVING_TILTER 1
#define VALUE_HAVING_LIFTER 1
#define VALUE_MAX_TILT_DEGREE  90

void setup() 
{
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);

  EEPROM.write(ADDRESS_HAVING_TILTER, VALUE_HAVING_TILTER);
  EEPROM.write(ADDRESS_HAVING_LIFTER, VALUE_HAVING_LIFTER);
  EEPROM.write(ADDRESS_MAX_TILT_DEGREE, VALUE_MAX_TILT_DEGREE);
  
  // turn the LED on when we're done
  digitalWrite(13, HIGH);
  
}

void loop() 
{
} 


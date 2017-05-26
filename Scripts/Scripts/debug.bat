rem
rem ----------------------------------------------------------------------------------------
rem
rem "Copyright 2014-2015 Intel Corporation.
rem
rem The source code, information and material ("Material") contained herein is owned by Intel Corporation
rem or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
rem or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
rem The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
rem be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
rem in any way without Intel's prior express written permission. No license under any patent, copyright or 
rem other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
rem by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
rem must be express and approved by Intel in writing.
rem
rem Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
rem embedded in Materials by Intel or Intel's suppliers or licensors in any way."
rem -----------------------------------------------------------------------------------------
rem

:author : bruce
Set dev=%1
mkdir logcat
cd logcat
mkdir anr
mkdir bugreport
mkdir tombstones
cd ..

Set ADBCmd=adb -s %dev%
IF "%dev%"=="" (
	Set ADBCmd=adb
)

:export main、events、system、radio、anr、tombstones log
%ADBCmd% logcat -d -b main -v time > ./logcat/main.log
%ADBCmd% logcat -d -b events -v time > ./logcat/events.log
%ADBCmd% logcat -d -b system -v time > ./logcat/system.log
%ADBCmd% logcat -d -b radio -v time > ./logcat/radio.log
%ADBCmd% pull ./data/anr/ ./logcat/anr
%ADBCmd% pull ./data/tombstones/　./logcat/tombstones

:need root permission
%ADBCmd% shell dmesg > ./logcat/kernel.log

:screencap
%ADBCmd% shell screencap -p ./storage/sdcard0/screenshot.png
%ADBCmd% pull ./storage/sdcard0/screenshot.png ./logcat
%ADBCmd% bugreport > ./logcat/bugreport/bugreport.log



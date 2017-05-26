@echo off
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

REM usage
IF [%1]==[] GOTO usage
IF [%2]==[] GOTO usage
IF [%1]==[-h] GOTO usage
IF [%1]==[-help] GOTO usage
IF [%1]==[/?] GOTO usage
IF [%1]==[?] GOTO usage

REM Judge if input is directory and if DaVinci.exe is exist.
set originFolder=%cd%
set currentFolder=%~dp0
set DaVinciHome=%~f1
set ApkFolder=%~f2
set ConfigFile=%~f3
set WorkFolder=%4
set DeviceConfig=%ConfigFile%.txt
IF %currentFolder:~-1%==\ SET currentFolder=%currentFolder:~0,-1%
IF %DaVinciHome:~-1%==\ SET DaVinciHome=%DaVinciHome:~0,-1%
IF %ApkFolder:~-1%==\ SET ApkFolder=%ApkFolder:~0,-1%
IF [%ConfigFile%]==[] (
GOTO cfgNotExist
)
IF [%WorkFolder%]==[] set WorkFolder=False
IF %WorkFolder:~-1%==\ SET WorkFolder=%WorkFolder:~0,-1%

REM echo %DaVinciHome%
REM echo %ApkFolder%
REM echo %ConfigFile%

for /f "tokens=2 delims==" %%a in ('C:\Windows\System32\Wbem\wmic.exe OS Get localdatetime /value') do set "dt=%%a"
set year=%dt:~0,4%
set month=%dt:~4,2%
set day=%dt:~6,2%
set hour=%dt:~8,2%
set minute=%dt:~10,2%
set second=%dt:~12,2%
if "%hour:~0,1%"==" " set hour=0%hour:~1%

echo Test Time: %year%-%month%-%day% %hour%:%minute%:%second%
set download_mode=downloaded
if NOT "%ConfigFile%"=="False" (
if EXIST %ConfigFile% (
for /f "tokens=2 delims=>" %%i in ('findstr "<APK_download_mode>.*</APK_download_mode>" %ConfigFile%') do (
for /f "delims=<" %%i in ("%%i") do set download_mode=%%i)
) else GOTO cfgNotExist
)

if EXIST %ConfigFile% (
for /f "tokens=2 delims=>" %%i in ('findstr "<TestType>.*</TestType>" %ConfigFile%') do (
for /f "delims=<" %%i in ("%%i") do set test_type=%%i)
) 
    )

if "%test_type%"=="LaunchTime" GOTO runlt
if "%test_type%"=="ApkInstallation" GOTO runinstallation
if "%test_type%"=="Power" GOTO runpowertest
if "%test_type%"=="Storage" GOTO runrwtest

echo APK Downloading Mode: %download_mode%

IF "%DaVinciHome%"=="False" (
for /f "tokens=2 delims=>" %%i in ('findstr "<DaVinciPath>.*</DaVinciPath>" %ConfigFile%') do (
for /f "delims=<" %%i in ("%%i") do set DaVinciHome=%%i)
)

IF NOT EXIST %DaVinciHome% GOTO daVinciHomeNotExist
IF NOT EXIST %ApkFolder% (
if "%download_mode%"=="on-device" (
mkdir %ApkFolder% 
) else GOTO ApkFolderNotExist
)

IF NOT EXIST %DaVinciHome%\DaVinci.exe GOTO daVinciExeNotExist

IF "%WorkFolder%"=="False" (
IF not %ApkFolder:~0,2%==\\ (
set WorkFolder=%ApkFolder%)
)

REM ApkFolder is a sharefolder, need to create work folder to save qscripts
IF %ApkFolder:~0,2%==\\ (
IF "%WorkFolder%"=="False" (
set WorkFolder=%DaVinciHome%\%year%%month%%day%%hour%%minute%%second%
) ELSE (
REM user defined Work folder is a relative path
IF %WorkFolder:~0,1%==. (
set WorkFolder=%DaVinciHome%\%WorkFolder%
) ELSE (
REM user defined Work folder is an absolute path
set WorkFolder=%WorkFolder%)
))

IF NOT EXIST %WorkFolder% MKDIR %WorkFolder% 
echo Will create Qscripts under %WorkFolder%

set DaVinciHome="%DaVinciHome%"
set ApkFolder="%ApkFolder%"
GOTO runPython

:usage
echo ------------------------------------------------------------------------
echo Usage:
echo     run.bat FolderPathOfDaVinci FolderPathOfApks ^<ConfigFile^>
echo Please input the folder path of DaVinci.exe and the folder path of apks. 
echo ------------------------------------------------------------------------
GOTO exit

:daVinciHomeNotExist
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't find %DaVinciHome%
echo ------------------------------------------------------------------------
GOTO exit

:ApkFolderNotExist
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't find %ApkFolder%
echo ------------------------------------------------------------------------
GOTO exit

:daVinciExeNotExist
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't find %DaVinciHome%\DaVinci.exe
echo ------------------------------------------------------------------------
GOTO exit

:cfgNotExist
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't find config file %ConfigFile%
echo ------------------------------------------------------------------------
GOTO exit

:generateConfigError
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't generate device config file
echo ------------------------------------------------------------------------
GOTO exit

:runQscriptError
echo ------------------------------------------------------------------------
echo Error Message:
echo     couldn't run All of the cases successfully
echo ------------------------------------------------------------------------
GOTO exit
                          
:runPython
echo Now we will generate device config file for the current connected devices.
echo.
cd  %currentFolder%

IF "%ConfigFile%"=="False" (
python run_qs.py %DaVinciHome% -g
) ELSE (
IF EXIST default_device_cfg.txt (
del default_device_cfg.txt
)
python run_qs.py %DaVinciHome% -g -x %ConfigFile% -c %DeviceConfig%
)
REM ECHO ERRORLEVEL IS %ERRORLEVEL%
IF NOT "%ERRORLEVEL%"=="0" GOTO generateConfigError
echo Generate device config file done!
echo.


echo Now we will run the Qscript.
echo.
IF "%ConfigFile%"=="False" (
python run_qs.py %DaVinciHome% -r -p %ApkFolder% -w %WorkFolder%
) ELSE (
python run_qs.py %DaVinciHome% -r -p %ApkFolder% -x %ConfigFile% -c %DeviceConfig% -w %WorkFolder%)
)
REM echo Run Qscript done!
REM echo.
IF NOT "%ERRORLEVEL%"=="0" GOTO runQscriptError

GOTO exit

:runlt
cd %currentFolder%
rem python %currentFolder%\run_qs.py %DaVinciHome% -g -x %ConfigFile% -p %ApkFolder% -c %DeviceConfig%
python run_lt.py %DaVinciHome% %ApkFolder% %ConfigFile%
GOTO exit

:runinstallation
cd %currentFolder%

python run_installation.py %DaVinciHome% %ApkFolder% %ConfigFile%
GOTO exit

:runpowertest
cd %currentFolder%

python run_power.py %DaVinciHome% %ApkFolder% %ConfigFile%
GOTO exit

:runrwtest
cd %currentFolder%

python run_rw.py %DaVinciHome% %ApkFolder% %ConfigFile%
GOTO exit

:exit
cd %originFolder%

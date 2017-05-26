@echo off

setlocal

set MYDIR=%~dp0
set ADB=%MYDIR%adb
set AGENT_PATH=/system/bin
set AGENT=agent
set AGENT_PORT=9999
set ADBD_IP=%1

if "%1" == "" echo "Usage: .\start_agent_network.bat ADBD_IP" && goto :eof

%ADB% connect %ADBD_IP%
%ADB% root && call :sleep 1
%ADB% push %MYDIR%%AGENT% /sdcard/
%ADB% remount
%ADB% shell "if [ -e %AGENT_PATH%/%AGENT% ]; then echo 0; else echo 1; fi" > result.tmp
set /p ret=<result.tmp
del result.tmp
@rem don't have diff and killall commands currently
@rem if %ret% == 0 call :diff_and_start_agent && goto :eof
@rem if %ret% == 1 call :forward_and_start_agent "copy_agent" && goto :eof
@rem call :forward_and_start_agent "copy_agent" && goto :eof
%ADB% shell "cp /sdcard/%AGENT% %AGENT_PATH%; chmod 777 %AGENT_PATH%/%AGENT%;"
%ADB% forward tcp:%AGENT_PORT% tcp:%AGENT_PORT%
start /b %ADB% shell "%AGENT_PATH%/%AGENT%" >nul 2>nul

goto :eof
:sleep

set /a sec=%1+1
@rem emulate sleep function
ping 127.0.0.1 -n %sec% -w 1000 >nul 2>nul
set /p=^.<nul

goto :eof
:diff_and_start_agent

%ADB% shell "diff %AGENT_PATH%/%AGENT% /sdcard/%AGENT% > /dev/null; if [ 0 -eq $? ]; then echo 0; else echo 1; fi" > result.tmp
set /p ret=<result.tmp
del result.tmp
if %ret% == 0 call :forward_and_start_agent "" && goto :eof
if %ret% == 1 %ADB% shell "killall %AGENT%" && call :sleep 3 && call :forward_and_start_agent "copy_agent" && goto :eof

goto :eof
:forward_and_start_agent

if %1 == "copy_agent" %ADB% shell "cp /sdcard/%AGENT% %AGENT_PATH%; chmod 777 %AGENT_PATH%/%AGENT%;"
%ADB% forward tcp:%AGENT_PORT% tcp:%AGENT_PORT%
start /b %ADB% shell "%AGENT_PATH%/%AGENT%" >nul 2>nul

goto :eof


@echo off

setlocal

set MYDIR=%~dp0
set ADB=%MYDIR%adb
set AGENT_PATH=/system/bin
set AGENT=agent
set AGENT_PORT=9999

%ADB% -d root && call :sleep 1
%ADB% -d push %MYDIR%%AGENT% /sdcard/
%ADB% -d remount
%ADB% -d shell "killall %AGENT%" && call :sleep 3
%ADB% -d shell "cp /sdcard/%AGENT% %AGENT_PATH%; chmod 777 %AGENT_PATH%/%AGENT%;"
%ADB% forward tcp:%AGENT_PORT% tcp:%AGENT_PORT%
start /b %ADB% -d shell "%AGENT_PATH%/%AGENT%" >nul 2>nul

goto :eof
:sleep

set /a sec=%1+1
@rem emulate sleep function
ping 127.0.0.1 -n %sec% -w 1000 >nul 2>nul
set /p=^.<nul

goto :eof
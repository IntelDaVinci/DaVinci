#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import msvcrt
import os
import time
import subprocess
import select
import signal
import datetime
import logging
import math
import shutil
import re
import logging.config
import wmi
import win32api
import win32con
import psutil
import codecs, csv, locale
import glob

reload(sys) 
sys.setdefaultencoding('utf-8')

proplist = ["ro.build.version.release", "ro.product.model", "ro.build.id", "ro.product.cpu.abi"]
propheaderlist = ["OS Ver.","Device Model","Device Build", "CPU"]
headerwidth = [5,15,8,15,12,5]
header = ["No.","Device name"] + propheaderlist
tab_length = 4


def CallADBCmd(davinci_folder, cmd, dev, timeout=120):
    res = ADBCmd(davinci_folder, cmd, dev, timeout)
    lines =  "".join(res)
    if lines.find("ADB server didn't ACK") > -1:
        # In Restart_adb(), if adb server failed to started, the program will quit. So if return back here, adb server is started successfully.
        Restart_adb(davinci_folder)
        res = ADBCmd(davinci_folder, cmd, dev, timeout)
    return res


def ADBCmd(davinci_folder, cmd, dev, timeout=120):
    res = []
    adb_res = False
    count = 0

    if dev <> "":
        command =  "%s\\platform-tools\\adb.exe -s %s %s" %(davinci_folder, dev, cmd)
    else:
        command =  "%s\\platform-tools\\adb.exe %s" %(davinci_folder, cmd)

    # Jia Enhance CallADBCmd, add timeout function
    while adb_res == False and count < 2:
        count += 1
        p = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False)
        t_beginning = time.time()
        seconds_passed = 0
        while True:
            if p.poll() is not None:
                res = p.stdout.readlines()
                adb_res = True
                break
            seconds_passed = time.time() - t_beginning
            if seconds_passed > timeout:
                PrintAndLogErr( "Execute adb command timeout: %s" % command)
                p.terminate()
                res = ["FAIL"]
                adb_res = False
                # The value of t_beginning will be covered when breaked from this while cycle, no need to use it here.
                #t_beginning = time.time()
                break
            time.sleep(0.2)
    #res = p.stdout.readlines()
    DebugLog("%s\n%s" % (command, "".join(res)))
    return res


#If host/device can not response, need to call this function.
def Restart_adb(davinci_folder):
    count = 20

    # Kill all adb process instead of adb kill-server. Since sometime adb process may hung there which will cause kill-server hung.
    while count > 0:
        count -=1
        tasks = os.popen("tasklist").readlines()
        found_adb = False
        for task in tasks:
            if task.startswith("adb.exe "):
                found_adb = True
                res = os.popen("taskkill /f /im adb.exe").readlines()
                resinfo = "".join(res)
                DebugLog("taskkill /f /im adb.exe")
                DebugLog(resinfo)
                if resinfo.find("could not be terminated") > -1:
                    PrintAndLogErr("  - Cannot kill adb process on host. Please check.")
                break
                time.sleep(3)
        if found_adb == False:
            break

    # Kill the port 5037 process
    KillPort()
    
    # Still need to start the adb server after kill adb    
    time.sleep(3)
    output = ADBCmd(davinci_folder, "start-server", "", "")
    DebugLog("\nCommand: %s \nResult: %s" % ("adb start-server", "".join(output)))  

    str = "".join(output)
    str = str.strip().strip('\n')
    
    if str <> "" and str.find("* daemon started successfully *") < 0:
        PrintAndLogErr("".join(output))
        PrintAndLogErr("Failed to start adb server. Quit the program.")
        CleanupToQuit()
        sys.exit(-1)
    DebugLog("adb server is started and running again.")


def KillPort():
    count = 2
    process_name = ""
    while True:
        try:
            output = os.popen("netstat -aon | findstr \"5037\"").readlines()
            DebugLog("Command: netstat -aon | findstr \"5037\". \nResult: %s" % "".join(output))
            if len(output) == 0:
                break
            process_list = []
            for line in output:
                if line.strip() <> "":
                    p = re.compile(r'\s+')
                    tmp = p.sub(r' ', line.strip())
                    info = tmp.split(' ')
                    # Proto  Local Address          Foreign Address        State           PID
                    if len(info) <> 5:
                        return -1
                    local_addr = info[1]
                    forn_addr = info[2]
                    state = info[3].strip()
                    PID = int(info[4])
                    if state.upper() <> "LISTENING":
                        continue
                    if PID == 0:
                        continue

                    port1 = ""
                    port2 = ""
                    if local_addr.find(':')> 0:
                        port1 = local_addr.split(':')[1]
                    if forn_addr.find(':')> 0:
                        port2 = forn_addr.split(':')[1]

                    if int(port1) == 5037 or int(port2) == 5037 :
                        output = os.popen("tasklist /FI \"PID eq %s\" /FO \"LIST\"" % (PID)).readlines()
                        # output[1] is the info of process image name.
                        if output[1] is not None:
                            if len(output[1]) > 6 and output[1].find(":") <> -1:
                                process_name = output[1].split(':')[1]
                                if process_name.strip() <> "adb.exe":
                                    process_list.append(process_name.strip())
                                    DebugLog("To kill process: \n %s\n" % line)
                                    DebugLog("To be killed PID: %s/n Process Name: %s" % (PID, process_name.strip()))
            if len(process_list) == 0:
                break
            if count == 0:
                print "  - Failed to kill port 5037 process"
                return -1

            # Kill all process using Port 5037
            for name in process_list:
                DebugLog("To be killed process: %s" % (name))
                output = os.popen("TASKKILL /F /IM %s" % (name)).readlines()
                DebugLog("Process Killed: %s/n result: %s" % (name, "".join(output)))
                time.sleep(5)
            count -= 1
        except Exception, e:
            s=sys.exc_info()
            logger = logging.getLogger('debug')
            logger.debug("[Error@%s]:%s" % (str(s[2].tb_lineno),s[1],))
            logger.debug("  - Exception: Release port 5037 failed. " + str(e))
    return 0

def PrintAndLogInfo(message):
    print message
    logger = logging.getLogger('result')
    logger.info(message) 

def PrintAndLogErr(err):
    print err
    logger = logging.getLogger('result')
    logger.error(err)

def ErrorLog(message):
    logger = logging.getLogger('error')
    logger.debug(message)

def DebugLog(message):
    logger = logging.getLogger('debug')
    logger.debug(message)

def PerfLog(message):
    logger = logging.getLogger('perf')
    logger.debug(message)

def copyFile(src, dest):
    try:
        shutil.copy(src, dest)
    except Exception as e:
        print('')

def deleteFile(fileName):
    if os.path.exists(fileName):
        os.remove(fileName)

def GetPackageName(davinci_folder, apk_name):
    cmd = davinci_folder + "\\platform-tools\\aapt.exe dump badging \"" + apk_name + "\""
    matchStr = "package: name='"
    packageInfo = os.popen(cmd).readlines()
    for pInfo in packageInfo:
        if pInfo.startswith(matchStr):
            packageName = pInfo[len(matchStr):]
            packageName = (packageName.split('\''))[0]
            break
    return packageName

def GetAllDevices(davinci_folder):
    output = CallADBCmd(davinci_folder, "devices", "", "")
    header_found = False

    devices = []
    for line in output:
        if line.find("List of devices attached") >= 0:
            header_found = True
            continue
        if header_found:
            if line.strip() <> "":
                device_name = line.split('\t')[0]
                device_status = (line.split('\t')[1]).strip('\n').strip('\r')
                if device_status == "device":
                    devices.append(device_name)
    return devices

def GetDeviceBattery(davinci_folder, dev):
    level = "N/A"
    output = CallADBCmd(davinci_folder, "shell dumpsys battery", dev, "")
    for line in output:
        if line.split(":")[0].strip() == "level":
            level = line.split(":")[1].strip()
            break
    return level

def CheckInstallSuccessfullyOrNot(output):
    for item in output:
        if (item.lower().strip().find("success") > -1):
            return True
    return False

def ShowDevTable(dev, title, davinci_folder):
    # Print header
    split_line = "--------------------------------------------------------------------------------"    
    print "\n\n" + title +":"
    print split_line
    index = 0
    header_col = []
    group_dict = {}
    for col in header:
        while len(col) + tab_length < headerwidth[index]:
            col = col + " "
        index +=1
        header_col.append(col)
    print '\t'.join(header_col)
    print split_line

    # Get proper for all the devices
    for device_name in dev:         
        index = 1
        row = device_name
        while len(row) + tab_length < headerwidth[index]:
            row  = row  + " "   
        for prop_name in proplist:
            index += 1
            device_prop = os.popen(davinci_folder + "\\platform-tools\\adb.exe -s %s shell getprop %s" % (device_name, prop_name)).readlines()[0]
            device_prop = device_prop.strip('\n').strip('\r')
            while len(device_prop) + tab_length < headerwidth[index]:
                device_prop = device_prop + " "
            row += "\t" + device_prop
        group_dict[device_name] = row

    resorted_dev = []
    index = 0
    for obj in sorted(group_dict.iteritems(), key=lambda d:d[0], reverse = False ) :
        key = obj[0]
        line_info = obj[1]
        index += 1
        print str(index) + "\t"  + line_info
        resorted_dev.append(key)
    return resorted_dev

def FindFreePort(basePort, index, step):
    tmp_port = basePort + index
    found = False
    while tmp_port < 65535:
        output = os.popen("netstat -aon | findstr \"%s\"" % str(tmp_port)).readlines()
        if len(output) == 0:
            found = True
            break
        tmp_port += step

    time.sleep(index)
    if found:
        return tmp_port
    else:
        return -1

def KillDavinciProcess(PID):
    count = 3
    res = False
    while count > 0:
        count -= 1
        cmd = 'taskkill /F /FI "PID eq %s' % PID
        output = os.popen(cmd).readlines()
        DebugLog('\nCommand: %s\nResult:%s' % (cmd, '\n'.join(output)))
        cmd = 'tasklist /FI "PID eq %s"' % PID
        output = os.popen(cmd).readlines()
        DebugLog('\nCommand: %s\nResult:%s' % (cmd, '\n'.join(output)))
        if "No tasks are running which match the specified criteria." in output:
            res = True
            break
    return res

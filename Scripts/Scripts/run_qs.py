'''
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
'''

#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import os
import time
import subprocess
import signal
import datetime
import logging
import math
import shutil
import re
import logging.config
import wmi
import psutil
import csv
import locale
import glob
from generate import generateQSfile, getApkPackageName, CompareAndMergeApplistXML, anyRnRTest, generateRnRQS, MatchRnR
import json
import upstream_composer
from xml.etree import ElementTree
from ondevicedownloader import download_apk_on_device, fetch_apk
from getdeviceid import save_config
from downloadFromGooglePlay import get_apk_list, getAppDetailInfo
from generate_summary import Generate_Summary_Report, getAppVersion, ContainNoneASCII
from apkhelper import get_user_input, get_string_hash
import dvcsubprocess
from dvcutility import DeviceInfo, AAPTHelper, InstallWifiApk, InstallAndLaunchApk, Enviroment
import watchcat
import adbhelper
import traceback
from common import copyTreeExt, copyFileExt

reload(sys)

propheaderlist = ["OS Ver.", "Device Model", "Device Build", "CPU"]
headerwidth = [5, 20, 8, 15, 12, 5, 17, 10, 5]
header = ["No.", "Device name"] + propheaderlist + ["Houdini Version", "Group#", "Group Name"]
tab_length = 4

pp_qs = "power_pusher.qs"


def exception_logger(logger=None, extra_msg='', exit=True):
    def print_only(data):
        print data

    def exception_decorator(fn):
        def wrapper(*args, **kwds):
            try:
                return fn(*args, **kwds)
            except Exception as e:
                info = sys.exc_info()
                out_handle = logger if logger else print_only
                for file, lineno, function, text in traceback.extract_tb(info[2]):
                    logger_info = 'File:"{}", in {}, in {}, {}'.format(file, lineno, function, text)
                    out_handle(logger_info)

                if extra_msg:
                    out_handle("{} : {}".format(extra_msg, str(e)))
                else:
                    out_handle(str(e))

                if exit:
                    sys.exit(-1)
        return wrapper
    return exception_decorator


def CleanupToQuit():
    # Cleanup work folder
    target = [os.path.join(work_folder, i) for i in os.listdir(work_folder)
              if os.path.splitext(i)[1].strip() in (".qs", ".xml", ".qfd")]
    map(Enviroment.remove_item, target)
    # Unwatch all the devices in the WatchCat Manager device watch list
    map(g_wcm.unwatch_device, g_wcm.list_watching_devices())
    time.sleep(2)


def CallDebug(device_name, folder_name):
    error_debug = ReadXMLConfig('ErrorDebug')
    if error_debug:
        if not os.path.isabs(error_debug):
            error_debug = script_path + "\\" + error_debug
        if not os.path.exists(error_debug):
            PrintAndLogErr("  -Couldn't find the error debug script %s." % error_debug)
            return False
        else:
            PrintAndLogInfo("  - Wait to get debug log ...")
    else:
        return False
    debug_log_folder = r'%s\debuglog' % folder_name
    if not os.path.exists(debug_log_folder):
        os.mkdir(debug_log_folder)

    cmd = "%s %s" % (error_debug, device_name)
    process = subprocess.Popen(cmd, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False, cwd=debug_log_folder)
    start = datetime.datetime.now()
    timeout = 20

    period = timeout / 100.00
    speed = False

    for i in xrange(100):
        progress(50, (i + 1), True)  # +1 because xrange is only 99

        if speed:
            time.sleep(0.00001)  # Slow it down for demo
        else:
            time.sleep(period)  # Slow it down for demo
        if process.poll() is None:
            now = datetime.datetime.now()
            # If timeout, terminate the process.
            if (now - start).seconds > timeout:
                process.terminate()
                break
        else:
            speed = True
    res = process.stdout.readlines()
    DebugLog("".join(res))
    return True


def AnyDevice():
    devices = SearchDevice()
    if len(devices) == 0:
        StartADB_Server()

        devices = SearchDevice()
        if len(devices) == 0:
            PrintAndLogInfo("No device connected.")
            sys.exit(-1)
    return devices


def SearchDevice():
    output = CallADBCmd("devices", "", "")

    header_found = False

    devices = []
    for line in output:
        if line.find("List of devices attached") >= 0:
            header_found = True
            continue
        if header_found:
            if line.strip():
                device_name = line.split('\t')[0]
                device_status = (line.split('\t')[1]).strip('\n').strip('\r')
                if device_status == "device":
                    devices.append(device_name)

    return devices


def CallADBCmd(cmd, dev, para="", timeout=120):
    res = ADBCmd(cmd, dev, para, timeout)
    lines = "".join(res)
    if lines.find("ADB server didn't ACK") != -1 or lines.find("out of date") != -1:
        # In Restart_adb(), if adb server failed to started, the program will quit.
        # So if return back here, adb server is started successfully.
        Restart_adb()
        res = ADBCmd(cmd, dev, para, timeout)

    return res


def ADBCmd(cmd, dev, para, timeout=120):
    support = True
    res = []
    adb_res = False
    count = 0
    file_name = "%s\\adb_err_%s.txt" % (script_path, cmd)
    if os.path.exists(file_name):
        file = open(file_name, 'r')
        for line in file:
            if dev == line.strip('\n'):
                PrintAndLogErr("    - Device %s cannot support command 'adb %s %s'." % (dev, cmd, para))
                support = False
                break
        file.close()
    if support:
        if dev:
            command = "%s\\platform-tools\\adb.exe -s %s %s" % (Davinci_folder, dev, cmd)
        else:
            command = "%s\\platform-tools\\adb.exe %s" % (Davinci_folder, cmd)
        # Jia Enhance CallADBCmd, add timeout function
        while not adb_res and count < 2:
            count += 1
            p = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False)
            t_beginning = time.time()
            while True:
                if p.poll() is not None:
                    res = p.stdout.readlines()
                    adb_res = True
                    p.stdout.flush()
                    break
                seconds_passed = time.time() - t_beginning
                if seconds_passed > timeout:
                    PrintAndLogErr("Execute adb command timeout: %s" % command)
                    p.terminate()
                    res = ["FAIL"]
                    adb_res = False
                    # The value of t_beginning will be covered when broke from this while cycle, no need to use it here.
                    # t_beginning = time.time()
                    break
                time.sleep(0.2)
        # res = p.stdout.readlines()
        DebugLog("%s\n%s" % (command, "".join(res)))

    # If adb server is stopped, adb command execution will start adb server first,
    # and the two messages shall be cleaned up.
    clean_res = []
    for line in res:
        if line.startswith('* daemon not running. starting it now') or \
                line.startswith('* daemon started successfully *') or\
                line.startswith('adb server is out of date.  killing...'):
            continue
        clean_res.append(line)

    return clean_res

def CallValADB(cmd, devices):
    timeout = 60
    process_list = []
    print ""
    print "Checking 'adb %s'" % cmd
    file_name = "%s\\adb_err_%s.txt" % (script_path, cmd)
    Enviroment.remove_item(file_name)

    for device_name in devices:
        command = "%s\\platform-tools\\adb.exe -s %s %s" % (Davinci_folder, device_name, cmd)
        FNULL = open(os.devnull, 'w')
        process = subprocess.Popen(command, stdout=FNULL, stderr=FNULL)
        process_list.append(process)
        FNULL.close()
    period = timeout / 100.00
    for i in xrange(100):
        progress(50, (i + 1), True)  # +1 because xrange is only 99
        finished = True
        for process in process_list:
            if process.poll() is None:
                finished = False
        if finished:
            if cmd == "reboot" or cmd == "shell reboot":
                # Check all devices whether they are rebooted
                for device_name in devices:
                    status = CallADBCmd("get-state", device_name, "")
                    status_info = ("".join(status)).strip()
                    if status_info != "device":
                        finished = False
        if finished:
            time.sleep(0.00001)
        else:
            time.sleep(period)

    # After timeout, check the hung process
    count = 2
    hung_list = []
    while count > 0:
        count -= 1
        index = 0
        for process in process_list:
            if process.poll() is None:
                hung_list.append("%s\\platform-tools\\adb.exe -s %s %s" % (Davinci_folder, devices[index], cmd))
                with open(file_name, 'a') as f:
                    f.write(devices[index] + "\n")
                process.terminate()
                time.sleep(5)
            index += 1
    if len(hung_list) > 0:
        # If any adb hung, need to restart the adb to recover it.
        Restart_adb()
    return hung_list

def ADBCmdVal(devices):
    hung_list = []
    reboot_err_list = []
    for cmd in ("reboot", "shell getprop", "shell dumpsys battery", "shell reboot"):
        hung_list += CallValADB(cmd, devices)

        # check whether the devices are rebooted
        if cmd == "reboot" or cmd == "shell reboot":
            print "\nChecking devices are rebooted"
            timeout = 60
            period = timeout / 100.00
            for i in xrange(100):
                progress(50, (i + 1), True)  # +1 because xrange is only 99
                rebooted = True
                for device_name in devices:
                    status = os.popen(
                        Davinci_folder + "\\platform-tools\\adb.exe -s %s get-state" % device_name).readlines()
                    status_info = ("".join(status)).strip()
                    if status_info != "device":
                        rebooted = False
                if rebooted:
                    time.sleep(0.00001)
                else:
                    time.sleep(period)

            for device_name in devices:
                status = os.popen(
                    Davinci_folder + "\\platform-tools\\adb.exe -s %s get-state" % device_name).readlines()
                status_info = ("".join(status)).strip()
                if status_info != "device":
                    # print "Warning: command 'adb.exe -s %s reboot' failed to reboot"  % (device_name)
                    reboot_err_list.append(Davinci_folder + "\\platform-tools\\adb.exe -s %s " % device_name)

    print ""
    if not hung_list and not reboot_err_list:
        print "All adb commands validation pass!"
    else:
        if hung_list:
            print "Warning: commands below hung:"
            for cmd in hung_list:
                print "   " + cmd

        if reboot_err_list:
            print "Warning: 'adb reboot' failed:"
            for cmd in reboot_err_list:
                print " " + cmd


def PrintAndLogInfo(message):
    print message
    logger = logging.getLogger('result')
    logger.info(message)
    sys.stdout.flush()


def PrintAndLogErr(err):
    print err
    logger = logging.getLogger('result')
    logger.error(err)
    sys.stdout.flush()


def DebugLog(message):
    logger = logging.getLogger('debug')
    logger.debug(message)


def PerfLog(message):
    logger = logging.getLogger('perf')
    logger.debug(message)


def ErrorLog(message):
    logger = logging.getLogger('error')
    logger.debug(message)


def CreateCSV(csvfile, apkname, criticalpath):
    cripath = open(csvfile, 'ab')
    # cripath.write(codecs.BOM_UTF8) It doesn't make any sense to try to put BOM inside a Unicode string.

    w = csv.writer(cripath)
    if os.path.getsize(csvfile) == 0:
        w.writerow(['Application', 'CriticalPath'])
    w.writerow([apkname.encode('utf-8'), criticalpath])
    cripath.close()


def GetPowerPusher(device_name):
    if not os.path.exists(config_file_name):
        return ""
    config_file = open(config_file_name, "r")
    COM_ID = ""
    for line in config_file:
        tmp_line = line.strip().strip('\n')
        if tmp_line:
            device = (tmp_line.split("<>*<>")[1]).strip()
            if device == device_name:
                COM_ID = (tmp_line.split("<>*<>")[2]).strip('\n')
    config_file.close()

    return "" if COM_ID == "NULL" else COM_ID



def ShowDevTable(dev, title, dev_dict, group_dict):
    # Print header
    split_line = "-" * 132

    print "\n\n" + title + ":"
    print split_line
    index = 0
    header_col = []
    for col in header:
        while len(col) + tab_length < headerwidth[index]:
            col = col + " "
        index += 1
        header_col.append(col)
    print '\t'.join(header_col)
    print split_line

    # Get proper for all the devices
    for device_name in dev:
        index = 1
        row = device_name
        while len(row) + tab_length < headerwidth[index]:
            row = row + " "
        for prop_name in ("ro.build.version.release", "ro.product.model", "ro.build.id", "ro.product.cpu.abi"):
            index += 1
            res = CallADBCmd("shell getprop %s" % prop_name, device_name)
            if res:
                device_prop = res[0]
                device_prop = device_prop.strip('\n').strip('\r')
                while len(device_prop) + tab_length < headerwidth[index]:
                    device_prop = device_prop + " "
                row += "\t" + device_prop
        houdini_version = GetHoudiniVersion(device_name)
        row += "\t" + houdini_version
        dev_dict[device_name] = row
        group_dict[device_name] = row
    resorted_dev = []

    index = 0
    for obj in sorted(group_dict.iteritems(), key=lambda d: d[0], reverse=False):
        key = obj[0]
        line_info = obj[1]
        index += 1
        print str(index) + "\t" + line_info + "\tNA\tNA"
        resorted_dev.append(key)
    return resorted_dev


def GetHoudiniVersion(device_name):
    return_results = CallADBCmd("shell houdini --version", device_name, "")
    out = [i for i in return_results if i.strip() and i.lower().strip().find("houdini version") != -1]
    return out[0].split(':')[1].strip() if out and out[0].find(':') != -1 and len(out[0].split(':')) > 1 else "NA"


def GetDevIndex(dev_dict, device_name):
    index = 0
    for obj in sorted(dev_dict.iteritems(), key=lambda d: d[0], reverse=False):
        key = obj[0]
        if device_name == key:
            break
        index += 1
    return index


def progress(width, percent, newline):
    marks = math.floor(width * (percent / 100.0))
    spaces = math.floor(width - marks)
    loader = '[' + ('=' * int(marks)) + (' ' * int(spaces)) + ']'
    sys.stdout.write("%s %d%%\r" % (loader, percent))
    if percent >= 100 and newline:
        sys.stdout.write("\n")


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

    return tmp_port if found else -1


def uninstallpackage(package_name, current_target_dev):
    if CheckPackageInstalled(current_target_dev, package_name):
        # Force stop the APK and uninstall it.
        CallADBCmd("shell am force-stop %s" % package_name, current_target_dev, "", 30)
        CallADBCmd(" uninstall %s" % package_name, current_target_dev, "", 30)

    installed = CheckPackageInstalled(current_target_dev, package_name)
    if installed:
        RebootDevice(current_target_dev)
        CallADBCmd(" uninstall %s" % package_name, current_target_dev, "", 30)


def get_device_brand():
    out = ''.join(CallADBCmd("devices -l", '')).strip()
    devices_list = out.split('\r\n')
    if len(devices_list) > 1:
        return dict(zip((i.split()[0] for i in devices_list[1:]),
                        (i.split()[3].split(':')[1] for i in devices_list[1:])))
    else:
        return {}


def set_device_orientation(device_name, portrait=True):
    prefix = "shell content insert --uri content://settings/system --bind"

    CallADBCmd('{} name:s:accelerometer_rotation --bind value:i:0'.format(prefix), device_name)

    if portrait:
        CallADBCmd('{} name:s:user_rotation --bind value:i:0'.format(prefix), device_name)
    else:
        CallADBCmd('{} name:s:user_rotation --bind value:i:1'.format(prefix), device_name)

    CallADBCmd('{} name:s:accelerometer_rotation --bind value:i:1'.format(prefix), device_name)


def click_system_cache(device_name, swipe_coordinate, click_cache_coordinate, click_ok_coordinate):
    set_device_orientation(device_name)

    CallADBCmd('shell am start -a android.settings.INTERNAL_STORAGE_SETTINGS', device_name)
    time.sleep(2)
    CallADBCmd('shell input swipe {}'.format(swipe_coordinate), device_name)
    time.sleep(2)
    CallADBCmd('shell input tap {}'.format(click_cache_coordinate), device_name)
    time.sleep(2)
    CallADBCmd('shell input tap {}'.format(click_ok_coordinate), device_name)


def clean_system_cache(device_name):
    CallADBCmd("shell am force-stop com.android.vending", device_name, "", 30)
    dev_resolution = GetDeviceInfo(device_name).get("Resolution", "480x800").split('x')
    coordinate_x, coordinate_y = dev_resolution[0], dev_resolution[1]
    brand_ratio = {'asus': {'click_cache': {'x': 0.6, 'y': 0.75}, 'click_ok': {'x': 0.6, 'y': 0.63}},
                   'nexus_7': {'click_cache': {'x': 0.6, 'y': 0.65}, 'click_ok': {'x': 0.8, 'y': 0.53}},
                   'nexus_5': {'click_cache': {'x': 0.6, 'y': 0.75}, 'click_ok': {'x': 0.8, 'y': 0.53}},
                   'skytek': {'click_cache': {'x': 0.6, 'y': 0.45}, 'click_ok': {'x': 0.83, 'y': 0.58}}}

    swipe_coordinate = (int(coordinate_x) / 2, int(coordinate_y) / 2, int(coordinate_x) / 2, int(coordinate_y) / 10)

    devices_id_brand_mapping = get_device_brand()

    if device_name in devices_id_brand_mapping:
        model_name = devices_id_brand_mapping[device_name]
        for brand in brand_ratio:
            if brand in model_name.lower():
                click_chaced_coordinate = (int(coordinate_x) * brand_ratio[brand]['click_cache']['x'],
                                           int(coordinate_y) * brand_ratio[brand]['click_cache']['y'])
                click_ok_coordinate = (int(coordinate_x) * brand_ratio[brand]['click_ok']['x'],
                                       int(coordinate_y) * brand_ratio[brand]['click_ok']['y'])

                click_system_cache(device_name,
                                   ' '.join(map(str, swipe_coordinate)),
                                   ' '.join(map(str, click_chaced_coordinate)),
                                   ' '.join(map(str, click_ok_coordinate)))

                break


def unlock_device(device_id):
    resolution = str(GetDeviceInfo(device_id).get("Resolution", "480x800"))

    if 'x' in resolution:
        dev_resolution = resolution.split('x')

        unlock_coordinate_x, unlock_coordinate_y = dev_resolution[0], dev_resolution[1]

        unlock_coordinate = (
            str(int(unlock_coordinate_x) / 10),
            str(int(unlock_coordinate_y) * 4 / 5),
            str(int(unlock_coordinate_x) * 4 / 5),
            str(int(unlock_coordinate_y) * 4 / 5))

        unlock_coordinate_vertical = (
            str(int(unlock_coordinate_x) / 2),
            str(int(unlock_coordinate_y) / 2),
            str(int(unlock_coordinate_x) / 2),
            str(int(unlock_coordinate_y) / 20))

        CallADBCmd('shell input touchscreen swipe {}'.format(' '.join(unlock_coordinate)), device_id)
        time.sleep(2)
        # try to unlock again
        CallADBCmd('shell input touchscreen swipe {}'.format(' '.join(unlock_coordinate)), device_id)
        time.sleep(2)

        CallADBCmd('shell input touchscreen swipe {}'.format(' '.join(unlock_coordinate_vertical)), device_id)
        time.sleep(2)

        CallADBCmd('shell input touchscreen swipe {}'.format(' '.join(unlock_coordinate_vertical)), device_id)


def ParseXML(xml, keyword):
    root = ElementTree.parse(xml)
    values = root.findall(keyword)
    return [i.text for i in values if i.text is not None]

def CheckAndPrepareDevices():
    try:
        devices = AnyDevice()
        if not devices:
            return -1

        if not os.path.exists(script_path + "\\" + pp_qs):
            print "Default powerpusher recovery QScript %s is missing. Please check." % (script_path + "\\" + pp_qs)

        Enviroment.remove_item(config_file_name)
        all_dev = []
        bad_dev = []
        for line in devices:
            device_name = (line.split('\t'))[0]
            status_info = CallADBCmd("get-state", device_name, "")
            logger = logging.getLogger('debug')
            logger.debug("\nCommand: get-state \nResult: %s" % "".join(status_info))

            device_status = ("".join(status_info)).strip()
            if device_status.strip() != "device":
                bad_dev.append("{0}\t{1}".format(device_name, device_status.strip()))
                continue
            all_dev.append(device_name)
            # Prepare the powerpusher recovery qs for each device.
            dev_pp_qs = script_path + "\\" + os.path.splitext(pp_qs)[0] + "_" + device_name + ".qs"
            if not os.path.exists(dev_pp_qs):
                shutil.copy(script_path + "\\" + pp_qs, dev_pp_qs)

        if bad_dev:
            print "\nWARNING: Please make sure devices below are power on and connected correctly!!!"
            print "-------------------------------------------------------------------"
            print "Device Name       \tStatus:"
            print "-------------------------------------------------------------------"
            # print "Device Name\t\tStatus"
            for bad_info in bad_dev:
                print(bad_info)

        if len(all_dev) == 0:
            print("\n\nNo available device.")
            return -1
        print "\n\nReading information of attached devices. Please wait ......"

        dev_dict = {}
        group_dict = {}
        all_dev = ShowDevTable(all_dev, "Device Group Information", dev_dict, group_dict)

        group_dict = {}
        target_name = ParseXML(xml_config, 'Target/DeviceID')
        device_name = "".join(target_name)
        defined_dev = target_name
        group_dict[1] = []
        group_dict[1].append("NA")  # Group name
        group_dict[1].append(device_name)  # Device name
        for device_id in defined_dev:
            if device_id not in all_dev:
                PrintAndLogErr("Please check the DeviceID %s in XML, "
                               "couldn't find it from the test environment" % device_id)
                sys.exit(-1)
        print("\n\nChecking Power Button Pusher and if exists, prepare for it ...")
        dict = MapPowerpusher(len(defined_dev))

        need_preparation = ReadXMLConfig("RunPreparation").strip()
        if need_preparation == "True":
            print("\n\nMake preparation for devices. Estimate about %s seconds..." % (str(90 * len(defined_dev) + 120)))
            # Preparation
            PrepareBeforeSmokeTest(defined_dev)

            # Validate adb commands
            ADBCmdVal(defined_dev)

            print("\n\nPreparation is done.")
    except Exception, e:
        print str(e)
        sys.exit(-1)
    return target_name, group_dict, dev_dict, dict

def CreateNewCfg(camera_mode):
    # call adb devices print "The list of connected devices:"
    try:
        target_name, group_dict, dev_dict, dict = CheckAndPrepareDevices()

        # Generate the DaVinci.config and device config.
        xml_cfg = ""
        config_content = ""
        for key in group_dict.keys():
            group_name = (group_dict[key])[0]
            dev_list = (group_dict[key])[1:]
            for one_device in dev_list:
                COM_ID = ""
                if one_device in dict:
                    COM_ID = dict[one_device]

                dev_index = GetDevIndex(dev_dict, one_device)
                qagent_port = FindFreePort(8888, dev_index, len(dev_dict))
                if qagent_port == -1:
                    PrintAndLogErr("Failed to get QAgent Port for %s\n. Quit the program." % one_device)
                    sys.exit(-1)
                magent_port = FindFreePort(9888, dev_index, len(dev_dict))
                if magent_port == -1:
                    PrintAndLogErr("Failed to get MAgent Port for %s\n. Quit the program." % one_device)
                    sys.exit(-1)
                hyper_port = FindFreePort(23456, dev_index, len(dev_dict))
                if hyper_port == -1:
                    PrintAndLogErr("Failed to get HyperCamera Port for %s\n. Quit the program." % one_device)
                    sys.exit(-1)

                xml_cfg = xml_cfg + """
<AndroidTargetDevice>
  <name>""" + one_device + """</name>
  <screenSource>""" + camera_mode + """</screenSource>
  <qagentPort>""" + str(qagent_port) + """</qagentPort>
  <magentPort>""" + str(magent_port) + """</magentPort>
  <hypersoftcamPort>""" + str(hyper_port) + """</hypersoftcamPort>"""
                config_content = config_content + "[target_dev]<>*<>%s" % (one_device.strip())
                if COM_ID:
                    xml_cfg = xml_cfg + """
  <hwAccessoryController>""" + COM_ID + """</hwAccessoryController>
</AndroidTargetDevice>"""
                else:
                    COM_ID = "NULL"
                    xml_cfg = xml_cfg + """
</AndroidTargetDevice>"""
                all_dev_info = dev_dict[one_device].split('\t')
                dev_info = all_dev_info[1:]
                dev_str = ""
                for info in dev_info:
                    dev_str += "<>*<>" + info.strip()
                config_content = config_content + "<>*<>" + COM_ID + "<>*<>" + str(key) + "<>*<>" + group_name + dev_str + "\n"


        xml = """\
<?xml version="1.0"?>
<DaVinciConfig>
 <androidDevices>""" + xml_cfg + """
 </androidDevices>
 <windowsDevices />
 <chromeDevices />
</DaVinciConfig>
            """

        with open(Davinci_folder + "\\DaVinci.config", "w") as f:
            f.write(xml)

        with open(config_file_name, "w") as f:
            f.write(config_content)

        print "\n\n--------------------------Done-------------------------------"
        # print "Config file '%s' is created. " % (config_file_name)
        # print config_content

    except Exception, e:
        print str(e)
        return -1
    return 0


def PrepareBeforeSmokeTest(devices):
    process_list = []
    print "\nPre-set device system settings"
    for device_name in devices:
        os.popen(
            Davinci_folder + "\\platform-tools\\adb.exe -s %s push PrepareSmoke.jar /data/local/tmp" % device_name)
        cmd = "%s\\platform-tools\\adb.exe -s %s shell uiautomator runtest PrepareSmoke.jar -c com.intel.davinci.Smoke"\
              % (Davinci_folder, device_name)
        FNULL = open(os.devnull, 'w')
        process = subprocess.Popen(cmd, stdout=FNULL, stderr=FNULL)
        process_list.append(process)
        FNULL.close()

    period = 90 / 100.00
    for i in xrange(100):
        progress(50, i + 1, True)  # +1 because xrange is only 99
        finished = True
        for process in process_list:
            if process.poll() is None:
                finished = False
        if finished:
            time.sleep(0.00001)
        else:
            time.sleep(period)

    map(clean_system_cache, devices)


def HostRecovery(dev):
    PrintAndLogInfo("\n* Try to recover host environment.")
    KillADBProcess(dev)
    res = KillPort()

    return res


def DeviceColdRecovery(dev):
    # No device found. Let's recover the bad one
    if GetPowerPusher(dev) == "":
        PrintAndLogErr("    - No powerpusher for %s. Please restart it manually or setup"
                       " powerpusher to automatically recover it." % dev)
        return False

    some_recovered = False
    PrintAndLogInfo("\n* Try to recover offline devices: %s" % dev)
    dev_status = False
    tmp = 0
    while not dev_status and tmp < 3:
        tmp += 1
        # Try to recover the device if there is powerpusher
        PrintAndLogInfo("    - Cold reboot by power pusher. Please wait 2 minutes... ")
        # Search for the device specific powerpusher qs
        dev_pp_qs = script_path + "\\" + os.path.splitext(pp_qs)[0] + "_" + dev + ".qs"
        if os.path.exists(dev_pp_qs):
            powerpusher_qs = dev_pp_qs
        elif os.path.exists(script_path + "\\" + pp_qs):
            powerpusher_qs = script_path + "\\" + pp_qs
        else:
            PrintAndLogErr("    - Cannot recover with powerpusher since no %s or %s. Please check."
                           % (dev_pp_qs, script_path + "\\" + pp_qs))
            break

        # Change the screen source setting to disabled in Device Config, and back up it.
        configfile = Davinci_folder + "\\DaVinci.config"
        if os.path.exists(configfile):
            shutil.copy(configfile, configfile + '.bak')
            f = open(configfile + '.bak', 'r')
            sc = f.read()
            p = re.compile(r'<screenSource>.*</screenSource>')
            sc = p.sub(r'<screenSource>disabled</screenSource>', sc)
            f1 = open(configfile, 'w+')
            f1.write(sc)
            f1.close()
            f.close()
        time.sleep(1)

        # os.popen(Davinci_folder + "\\Davinci.exe -device %s -noagent -p %s" %(dev, powerpusher_qs))
        command = Davinci_folder + "\\Davinci.exe -device %s -noagent -p %s" % (dev, powerpusher_qs)
        p = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False)
        t_beginning = time.time()
        timeout = 200
        while True:
            time.sleep(0.5)
            if p.poll() is not None:
                break
            seconds_passed = time.time() - t_beginning
            if seconds_passed > timeout:
                PrintAndLogErr("Execute command timeout: %s" % command)
                p.terminate()
                break

        # Change back to the original device config
        shutil.move(configfile + '.bak', configfile)

        status = os.popen(Davinci_folder + "\\platform-tools\\adb.exe -s %s get-state" % dev).readlines()
        status_info = ("".join(status)).strip()
        if status_info.lower() == "device":
            dev_status = True

    if dev_status:
        # Some device is recovered successfully.
        PrintAndLogInfo("    - Recovered Successfully.")
        some_recovered = True
    else:
        PrintAndLogErr("    - Cannot recover with powerpusher. Please check it manually.")
    return some_recovered


def WaitForSoftRecovery(bad_dev):
    if len(bad_dev) == 0:
        return bad_dev
    timeout = 200
    start = datetime.datetime.now()
    now = datetime.datetime.now()
    while (now - start).seconds <= timeout:
        dev_list = []
        PrintAndLogInfo("\n* Wait for device self-recover. Wait for 20 sec ...")
        time.sleep(20)
        for dev in bad_dev:
            if ':' in dev:
                CallADBCmd("connect {}".format(dev), '', '')
            status = CallADBCmd("get-state", dev, "")
            logger = logging.getLogger('debug')
            logger.debug("\nCommand: get-state \nResult: %s" % "".join(status))
            status_info = ("".join(status)).strip()
            if status_info.lower() != "device":
                dev_list.append(dev)
            else:
                PrintAndLogInfo("    - Device %s recovered." % dev)
        bad_dev = dev_list
        if len(bad_dev) == 0:
            break
        now = datetime.datetime.now()
    return bad_dev


@exception_logger(PrintAndLogErr)
def CheckWIFI(device_name):
    # Preparing to check network state
    PrintAndLogInfo("\n* Prepare network ...")
    import dvcutility
    nt = dvcutility.DeviceNetwork(r'%s\platform-tools\adb.exe' % Davinci_folder, device_name,
                                  logging.getLogger('debug'))
    wifi_screenshot_folder = r'%s\wifi_screenshot_%s' % (work_folder, timestamp_nm)
    if not os.path.exists(wifi_screenshot_folder):
        os.mkdir(wifi_screenshot_folder)
    now_timestamp_nm = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
    wifi_screenshot_file = r'%s\%s_%s.png' % (wifi_screenshot_folder, device_name, now_timestamp_nm)

    # Checking wifi and turn on it if wifi is disabled.
    wifi_state, net_ssid, wifi_state_enabled, net_state_connected = nt.TurnOnWiFi(wifi_screenshot_file)
    if wifi_state == 'not found':
        PrintAndLogInfo("  - Failed to check network status.")
        return 'NA', ''
    # Try to connect to the preferred hotspot if it is configured.
    preferred_hotspot = ReadXMLConfig('PreferredHotSpot')

    if len(preferred_hotspot.strip()) > 0 and wifi_state_enabled and preferred_hotspot != net_ssid:
        tmp_res, net_ssid = nt.ConnectHotSpot(preferred_hotspot, wifi_screenshot_file)
        if tmp_res == 'NA':
            net_state_connected = False

    # If no network connected
    final_res = 'true'
    if net_state_connected:
        PrintAndLogInfo("  - Network connected to: %s." % net_ssid)
    else:
        final_res = 'false'
        if not wifi_state_enabled:
            PrintAndLogInfo("  - Wi-Fi is not enabled. Please enable it first.")
        else:
            PrintAndLogInfo("  - Network status is abnormal. Please check it.")

    return final_res, wifi_screenshot_file


def CheckBattery(dev_list, max, current_threshold, is_charging):
    offline_dev = []
    battery_dev = []
    tmp_max = -1
    tmp_dev = ""
    max_dev = ''
    for dev in dev_list:
        status = CallADBCmd("get-state", dev, "")
        logger = logging.getLogger('debug')
        logger.debug("\nCommand: get-state \nResult: %s" % "".join(status))
        status_info = ("".join(status)).strip()
        if status_info.lower() != "device":
            offline_dev.append(dev)
        else:
            # Check the battery information
            battery_info = CallADBCmd("shell dumpsys battery", dev, "")
            logger = logging.getLogger('debug')
            logger.debug("\nCommand: shell dumpsys battery \nResult: %s" % "".join(battery_info))
            get_battery_info = False
            for info in battery_info:
                if info.split(":")[0].strip() == "level":
                    get_battery_info = True
                    level = int(info.split(":")[1].strip())
                    if tmp_max < level:
                        tmp_max = level
                        tmp_dev = dev
                    if level >= current_threshold and level > max:
                        max_dev = dev
                        max = level
                        # When some device are charged enough, no need to charge again.
                        is_charging = False
                    # Jia For 2 battery levels or more, the avaliable level should be the first one,
                    # such as that on Lenovo X2
                    break
                # Jia To avoid batter overheat issue, add battery temperature checking here:
                if info.split(":")[0].strip() == "temperature":
                    temperature = int(info.split(":")[1].strip())
                    if temperature > 450:
                        PrintAndLogInfo(
                            "\n The battery temperature of %s is overheat, will wait 5 minutes to cool it" % dev)
                        time.sleep(300)
            if not get_battery_info:
                PrintAndLogErr(
                    "  - Failed to get the battery info of %s. Please check the log for more details." % dev)
                battery_dev.append(dev)

    return offline_dev, battery_dev, tmp_dev, max_dev, tmp_max, max, is_charging


# tar_flag means do not care battery threshold, just use the one with max battery.
# sleep_time: to charge 1% battery, how long to wait.
# sleep_sec: how long of the period to check the device battery when charging.
def ChooseDevice(dev_list, tar_flag, threshold=10, charging_threshold=15, sleep_time=3600, sleep_sec=300):
    timeout = -1
    PrintAndLogInfo("\n* Choosing device with max battery...")
    max_dev = dev_list[0]
    max = 0
    start = datetime.datetime.now()
    last_max = -1
    is_charging = False
    rec_off_count = 1
    rec_dec_count = 3
    while True:

        # find the device with max battery.
        current_threshold = threshold
        if is_charging:
            current_threshold = charging_threshold

        offline_dev, battery_dev, tmp_dev, max_dev, tmp_max, max, is_charging = CheckBattery(dev_list, max, current_threshold, is_charging)

        # If max battery device is found, stop choosing.
        if max > 0:
            break

        # The special case is "level=0" in which case devices has no API to provide battery level
        # but its get-state is OK. So, it is OK to use it.
        if tmp_max == 0:
            max_dev = tmp_dev
            break

        # If no need to meet threshold, just return the max battery one.
        if tmp_max > -1 and tar_flag == 0:
            max_dev = tmp_dev
            max = tmp_max
            break

        ####################################################################
        # The following logics handle the cases that no device battery meets threshold
        ####################################################################

        if tmp_max > -1:
            #####################################################
            # Case 1: Max battery information is gotten.
            #####################################################
            PrintAndLogInfo("  - No device has battery level >= %s%s. Max one is %s%s."
                            % (str(current_threshold), "%", str(tmp_max), "%"))

            # Check if time out.
            now = datetime.datetime.now()
            if -1 < timeout <= (now - start).seconds:
                # If battery still below threshold, no device returned.
                PrintAndLogInfo("\n* Timeout after sleep %s seconds." % (str(timeout)))
                # Although timeout, and no device meet charging threshold but some meet threshold, just use it.
                if tmp_max >= threshold:
                    max_dev = tmp_dev
                    max = tmp_max
                else:
                    max_dev = ""
                    max = 0

                break

            # Check if the battery level decreased since last time. Will reboot the device.
            # But just try the times defined in rec_dec_count
            if tmp_max < last_max:
                PrintAndLogInfo("\n* Charging failed. Battery decreased from %s to %s. After sleep %s seconds."
                                % (str(last_max), str(tmp_max), str((now - start).seconds)))
                if tmp_dev and rec_dec_count > 0:
                    rec_dec_count -= 1
                    RebootDevice(tmp_dev)
            else:
                last_max = tmp_max

            # Charging until battery meet the charging_threshold.
            if timeout == -1:
                timeout = sleep_time * (charging_threshold - tmp_max)

            PrintAndLogInfo("\n* Wait to charge battery until battery exceed %s%s. Sleep %ssec..."
                            % (str(charging_threshold), "%", str(sleep_sec)))
            time.sleep(sleep_sec)
            is_charging = True
        else:
            #####################################################
            # Case 2: No battery information and there is on-line device.
            # It means the on line device is in abnormal state, need to reboot them.
            #####################################################
            some_recovered = False
            if battery_dev:
                PrintAndLogInfo("\n* Try to recover all devices with battery issue: ")
                for dev in battery_dev:
                    res = RebootDevice(dev)
                    if res:
                        some_recovered = True

            #####################################################
            # Case 3: No battery information and no device found. Let's try to recover the offline ones.
            #  But just try for the times defined by rec_off_count.
            #####################################################
            if rec_off_count > 0 and offline_dev:
                rec_off_count -= 1
                PrintAndLogInfo("\n* Try to recover all offline devices: ")
                # Try to recover by Re-Plug USB
                new_bad = []
                for dev in offline_dev:
                    res = RePlugDevice(dev)
                    if res:
                        # newly-recovered device. Go on to choose device again.
                        some_recovered = True
                        break
                    else:
                        new_bad.append(dev)

                offline_dev = new_bad
                # Wait for self recovery
                new_bad = WaitForSoftRecovery(offline_dev)
                if len(new_bad) < len(offline_dev):
                    some_recovered = True
                # For those still bad, try to recovery by powerpusher.
                for dev in new_bad:
                    res = DeviceColdRecovery(dev)
                    if res:
                        some_recovered = True

            # newly-recovered device. Go on to choose device again.
            if some_recovered:
                continue
            else:
                PrintAndLogInfo("\n* Warning: No available device.")
                # No new recovered device , and no on-line devices. Let wait some time and continue to try again.
                wait_for_device = ReadXMLConfig("WaitForDevice")
                if wait_for_device.lower() <> 'false':
                    sleep_for_device_time = 200
                    print("   - Wait for {} seconds ...".format(sleep_for_device_time))
                    time.sleep(sleep_for_device_time)
                else:
                    max_dev = ""
                    max = -1
                    break

    if max_dev:
        PrintAndLogInfo("\n* %s (%s%s) will be used." % (max_dev, str(max), "%"))
        return max_dev
    else:
        # PrintAndLogInfo("\n* Warning: No available device.")
        return ""


def GetLastLineOfReport(index):
    report_file_name = work_folder + "\\" + "Smoke_Test_Report.csv"
    blk_size_max = 2048
    last_line = ""
    lines = ""
    if os.path.exists(report_file_name):
        report_file = open(report_file_name, "r")
        report_file.seek(0, os.SEEK_END)
        cur_pos = report_file.tell()
        line_count = 0
        while cur_pos > 0 and (last_line == "" or line_count < index):
            blk_size = min(blk_size_max, cur_pos)
            report_file.seek(cur_pos - blk_size, os.SEEK_SET)
            blk_data = report_file.read(blk_size)
            lines += blk_data
            cur_pos -= blk_size
            line_num = len(lines.split("\n"))

            last_line = lines.split("\n")[line_num - 1]
            i = line_num - 1
            while i > 0 and (last_line == "" or line_count < index):
                i -= 1
                last_line = lines.split("\n")[i]
                if last_line:
                    line_count += 1
        report_file.close()
    info = last_line.split(",")
    if len(info) < 1:
        last_line = ""
    elif info[0].find("TestTime") > 0:  # It is the header line.
        last_line = ""
    return last_line


@exception_logger(DebugLog, "  - Exception: Release port 5037 failed. ", False)
def KillADBProcess(device):
    c = wmi.WMI()
    for process in c.Win32_Process(name="adb.exe"):
        # Find all the adb process that are called on this device. And kill it.
        DebugLog("\nadb.exe process: %s/n Commandline: %s\n" % (str(process.ProcessId), str(process.Commandline)))
        if process is None:
            continue
        if process.Commandline is None:
            continue
        if re.search(r'\s+\-s\s+%s\s+' % device, str(process.Commandline)):
            DebugLog("\nKill adb.exe: %s/n Commandline: %s\n" % (str(process.ProcessId), str(process.Commandline)))
            try:
                process.Terminate()
            except Exception, e:
                PrintAndLogInfo("  - Exception: kill process failed. " + str(e))
                pass
            time.sleep(1)


@exception_logger(DebugLog, "  - Exception: Release port 5037 failed. ", False)
def KillPort():
    count = 2
    while True:
        output = os.popen("netstat -aon | findstr \"5037\"").readlines()
        DebugLog("Command: netstat -aon | findstr \"5037\". \nResult: %s" % "".join(output))
        if len(output) == 0:
            break
        process_list = []
        for line in output:
            if line.strip():
                p = re.compile(r'\s+')
                tmp = p.sub(r' ', line.strip())
                info = tmp.split(' ')
                # Proto  Local Address          Foreign Address        State           PID
                if len(info) != 5:
                    return -1
                local_addr = info[1]
                forn_addr = info[2]
                state = info[3].strip()
                PID = int(info[4])
                if state.upper() != "LISTENING":
                    continue
                if PID == 0:
                    continue

                port1 = ""
                port2 = ""
                if local_addr.find(':') > 0:
                    port1 = local_addr.split(':')[1]
                if forn_addr.find(':') > 0:
                    port2 = forn_addr.split(':')[1]

                if int(port1) == 5037 or int(port2) == 5037:
                    output = os.popen("tasklist /FI \"PID eq %s\" /FO \"LIST\"" % PID).readlines()
                    if (len(output) > 1):
                        # output[1] is the info of process image name.
                        if output[1] is not None:
                            if len(output[1]) > 6 and output[1].find(":") != -1:
                                process_name = output[1].split(':')[1]
                                if process_name.strip() != "adb.exe":
                                    process_list.append(process_name.strip())
                                    DebugLog("To kill process: \n %s\n" % line)
                                    DebugLog("To be killed PID: %s/n Process Name: %s" % (PID, process_name.strip()))
        if not process_list:
            break
        if count == 0:
            PrintAndLogInfo("  - Failed to kill port 5037 process")
            return -1

        # Kill all process using Port 5037
        for name in process_list:
            DebugLog("To be killed process: %s" % name)
            output = os.popen("TASKKILL /F /IM %s" % name).readlines()
            DebugLog("Process Killed: %s/n result: %s" % (name, "".join(output)))
            time.sleep(5)
        count -= 1

    return 0


def StartADB_Server():
    is_adb = False
    KillPort()
    c = wmi.WMI()
    processes = c.Win32_Process(name="adb.exe")
    if processes:
        is_adb = True

    if not is_adb:
        print "Start ADB Server ..."
        output = CallADBCmd("start-server", "", "")
        logger = logging.getLogger('debug')
        logger.debug("\nCommand: %s \nResult: %s" % ("adb start-server", "".join(output)))

        str = "".join(output)
        if str.find("* daemon started successfully *") < 0:
            PrintAndLogErr("".join(output))
            PrintAndLogErr("Failed to start adb server. Quit the program")
            sys.exit(-1)


# If host/device can not response, need to call this function.
def Restart_adb():
    count = 20

    # Kill all adb process instead of adb kill-server.
    # Since sometime adb process may hung there which will cause kill-server hung.
    while count > 0:
        count -= 1
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
                time.sleep(3)
        if found_adb is False:
            break

    # Kill the port 5037 process
    KillPort()

    # Still need to start the adb server after kill adb
    time.sleep(3)
    output = ADBCmd("start-server", "", "")
    DebugLog("\nCommand: %s \nResult: %s" % ("adb start-server", "".join(output)))

    out_str = "".join(output)
    out_str = out_str.strip().strip('\n')

    if out_str and out_str.find("* daemon started successfully *") < 0:
        PrintAndLogErr("".join(output))
        PrintAndLogErr("Failed to start adb server. Quit the program.")
        CleanupToQuit()
        sys.exit(-1)
    DebugLog("adb server is started and running again.")


# Jia function to get the processes num
def ProcessNumDetect(process):
    count = 0
    tasks = os.popen("tasklist").readlines()
    for i in tasks:
        if i.startswith(process):
            count += 1
    if count == 0:
        PrintAndLogInfo("There is no %s process existed." % process)
    elif count == 1:
        PrintAndLogInfo("There is 1 %s process existed." % process)
    else:
        PrintAndLogInfo("There are %s %s processes existed." % (count, process))
    return count


# Jia Enhance Reboot Device
def RebootDevice(device):
    if ReadXMLConfig('DisableRebootFunction').lower() == 'true':
        return True

    timeout = int(ReadXMLConfig('DeviceRebootTimeout')) if ReadXMLConfig('DeviceRebootTimeout').isdigit() else 600

    PrintAndLogInfo("\n* Reboot the device %s for device recovery" % device)
    is_alive = False
    res = CallADBCmd("reboot", device, "")
    logger = logging.getLogger('debug')
    logger.debug("\nCommand: reboot \nResult: %s" % "".join(res))
    res_info = ("".join(res)).strip()
    if res_info == "":
        PrintAndLogInfo("\n* Pass to execute command adb reboot. Please wait a minute")

        t_beginning = time.time()
        h_beginning = time.time()
        seconds_passed = 0
        heartbeat = 3
        while seconds_passed < timeout:
            time.sleep(1)
            heartbeat_passed = time.time() - h_beginning
            if heartbeat_passed > heartbeat:
                if ':' in device:
                    CallADBCmd("connect {}".format(device), '', '')
                status = CallADBCmd("shell getprop sys.boot_completed", device, "")
                logger = logging.getLogger('debug')
                logger.debug("\nCommand: shell getprop sys.boot_completed \nResult: %s" % "".join(status))
                status_info = ("".join(status)).strip()
                if status_info.lower() == "1":
                    PrintAndLogInfo("  - Device %s reboot successful!" % device)
                    is_alive = True
                    unlock_device(device)
                    break

                h_beginning = time.time()

            seconds_passed = time.time() - t_beginning
    else:
        PrintAndLogErr(
            "\n* Failed to reboot device %s with command adb reboot. Please try PowerPusher if possible." % device)
    return is_alive


def RebootDevices(devices):
    if ReadXMLConfig('DisableRebootFunction').lower() == 'true':
        return True

    timeout = int(ReadXMLConfig('DeviceRebootTimeout')) if ReadXMLConfig('DeviceRebootTimeout').isdigit() else 600

    for device in devices:
        PrintAndLogInfo("  - Reboot %s..." % device)
        res = CallADBCmd("reboot", device, "")
        logger = logging.getLogger('debug')
        logger.debug("\nCommand: reboot \nResult: %s" % "".join(res))
        res_info = ("".join(res)).strip()
        if res_info:
            PrintAndLogErr(
                "\n* Failed to reboot device %s with command adb reboot. Please try PowerPusher if possible." % device)

    t_beginning = time.time()
    h_beginning = time.time()
    seconds_passed = 0
    heartbeat = 3
    device_checklist = devices[:]
    while seconds_passed < timeout:
        time.sleep(1)
        heartbeat_passed = time.time() - h_beginning
        tmp_list = device_checklist
        if heartbeat_passed > heartbeat:
            for device in device_checklist:
                if ':' in device:
                    CallADBCmd("connect {}".format(device), '', '')
                status = CallADBCmd("shell getprop sys.boot_completed", device, "")
                logger = logging.getLogger('debug')
                logger.debug("\nCommand: shell getprop sys.boot_completed \nResult: %s" % "".join(status))
                status_info = ("".join(status)).strip()
                if status_info.lower() == "1":
                    PrintAndLogInfo("  - %s reboot successfully!" % device)
                    tmp_list.remove(device)
                h_beginning = time.time()
            device_checklist = tmp_list
            if len(tmp_list) == 0:
                break
        seconds_passed = time.time() - t_beginning
    for device in devices:
        unlock_device(device)


def RePlugDevice(dev):
    import ctypes

    max_retry = 3
    res = False
    while True:
        # Check if dev is on-line
        status = CallADBCmd("get-state", dev, "")
        status_info = ("".join(status)).strip()
        if status_info == "device":
            res = True
            break

        max_retry -= 1
        if max_retry == 0:
            break
        PrintAndLogInfo("\n* USB Replug the device ...")
        cmd = "{0}\\Davinci.exe -device {1} -noagent -p {2}\\{3}".format(Davinci_folder, dev,
                                                                         script_path, "USB_Recover.qs")

        logfile = open(os.devnull, 'w')
        SEM_NOGPFAULTERRORBOX = 0x0002  # From MSDN
        ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX)
        CREATE_NO_WINDOW = 0x8000000
        subprocess_flags = CREATE_NO_WINDOW

        p = subprocess.Popen(cmd, stdout=logfile, stderr=subprocess.STDOUT, creationflags=subprocess_flags)

        t_beginning = time.time()
        timeout = 200
        while True:
            time.sleep(3)
            if p.poll() is not None:
                break
            seconds_passed = time.time() - t_beginning
            if seconds_passed > timeout:
                PrintAndLogErr("Execute command timeout: %s" % cmd)
                p.terminate()
                break
        logfile.close()
    return res


def MapPowerpusher(device_num):
    res = 255
    res_dict = {}
    if not os.path.exists("%s\\MappingLogFile.txt" % Davinci_folder):
        print "\nConfigurate Power Button Pusher"
        cmd = "%s\\Davinci.exe -noagent -mappowerpusher" % Davinci_folder
        import ctypes

        timeout = 500 * device_num
        timestamp = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())

        logfile_name = "%s\\mapping_%s.log" % (result_folder_name, timestamp)
        errfile_name = "%s\\mapping_err_%s.log" % (result_folder_name, timestamp)
        logfile = open(logfile_name, "w")
        errfile = open(errfile_name, "w")
        start = datetime.datetime.now()
        if Is_Debug:
            process = subprocess.Popen(cmd, stdout=logfile, stderr=errfile)
        else:
            # Do not show crash dialog
            SEM_NOGPFAULTERRORBOX = 0x0002  # From MSDN
            ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX)
            CREATE_NO_WINDOW = 0x8000000
            subprocess_flags = CREATE_NO_WINDOW
            process = subprocess.Popen(cmd, stdout=logfile, stderr=errfile, creationflags=subprocess_flags)

        period = timeout / 100.00
        speed = False
        for i in xrange(100):
            progress(50, (i + 1), True)  # +1 because xrange is only 99

            if speed:
                time.sleep(0.00001)  # Slow it down for demo
            else:
                time.sleep(period)  # Slow it down for demo
            if process.poll() is None:
                now = datetime.datetime.now()
                # If timeout
                if (now - start).seconds > timeout:
                    # PrintAndLogInfo("  - Mapping Timeout %s sec." % (str((now - start).seconds)))
                    # If debug, do not terminate the process so that we can investigate.
                    res = -3

                    if not Is_Debug:
                        process.terminate()
                    # kill all the other processes using 5037 port
                    temp_res = KillPort()
                    if temp_res:
                        res = -4
                    break
            else:
                speed = True
        logfile.close()
        errfile.close()

    if res == 255 and os.path.exists("%s\\MappingLogFile.txt" % Davinci_folder):
        resfile = open("%s\\MappingLogFile.txt" % Davinci_folder, "r")
        mapping_section = False
        for line in resfile:
            if line.find("[Mapping]") > -1:
                mapping_section = True
                continue
            if line.find("[Result]:") > -1:
                break
            if mapping_section and line.find(':') > -1:
                COM_ID = (line.split(":")[0]).strip('\n')
                device_name = (line.split(":")[1]).strip('\n')
                res_dict[device_name.strip()] = COM_ID.strip()
        resfile.close()

    # PrintAndLogInfo("\nPowerpusher checking is done. \n\n\n" )
    return res_dict


def MarkLastLine(data_dict, timestamp):
    report_file_name = work_folder + "\\" + "Smoke_Test_Report.csv"

    if not os.path.exists(report_file_name):
        return -1

    with open(report_file_name) as rf:
        original_content = rf.readlines()

    key_index = dict()

    if original_content:
        table_tile = [i.strip() for i in original_content[0].split(',')]
        for k, _ in data_dict.iteritems():
            key_index[table_tile.index(k)] = k  # {'2': 'apkname'}
    else:
        return -1

    out_put = list()

    out_put.append(original_content[0])

    for each_line in original_content[1:]:
        if each_line.split(',')[0].find(timestamp) != -1:
            sub_fields = each_line.split(',')
            tail_pos = len(original_content[0].split(',')) - 1
            for k, _ in key_index.iteritems():
                sub_fields[k] = data_dict[key_index[k]] + '\n' if k == tail_pos else data_dict[key_index[k]]
            each_line = ','.join(sub_fields)

        out_put.append(each_line)

    with open(report_file_name, 'w') as nr:
        nr.writelines(out_put)

    return 0


def remove_replay_video(apk_folder, package_name, seed_value=1):
    replay_video = apk_folder + "\\" + package_name + "_seed_{}_replay.avi".format(seed_value)
    Enviroment.remove_item(replay_video, True)
    replay_qts = apk_folder + "\\" + package_name + "_seed_{}_replay.qts".format(seed_value)
    Enviroment.remove_item(replay_qts, True)


def KillProcess(PID):
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


# This function is added to monitor the adb process. To make debug easier.
def FindADBProcess():
    c = wmi.WMI()
    for process in c.Win32_Process(name="adb.exe"):
        # Find all the adb process that are called on this device.
        DebugLog("\nCheck adb.exe process after one APK test: %s/n Commandline: %s\n"
                 % (str(process.ProcessId), str(process.Commandline)))
        if process is None:
            continue
        if process.Commandline is None:
            continue


def CleanupRnRStuff(work_folder, rnr_folder):
    dependency_folder = r'%s\dependency_folder' % work_folder
    if os.path.exists(dependency_folder):
        map(Enviroment.remove_item, (i for i in os.listdir(dependency_folder) if os.path.isfile(i) and i != "Smoke_Test_Report.csv"))

    rnr_postfix = ('.xml', '.qs', '.qts', '.avi', '.h264', '.tree', '.window')

    if os.path.exists(rnr_folder):
        file_collection = (os.path.join(work_folder, os.path.basename(i)) for i in os.listdir(rnr_folder))
        map(Enviroment.remove_item,
            (i for i in file_collection if os.path.exists(i) and os.path.splitext(i)[1].strip() in rnr_postfix))


def CleanProcess(pattern, process_name):
    c = wmi.WMI()
    target_process = c.Win32_Process(name="{}".format(process_name))

    if len(target_process) == 0:
        DebugLog("\nProcess:{} doesn't exist!\n".format(process_name))

    for process in target_process:
        if process is None or process.Commandline is None:
            continue

        # Find all the adb process that are called on this device. And kill it.
        DebugLog("\nCheck {} after one APK test: {}/n Commandline: {}\n".format(process_name,
                                                                                process.ProcessId,
                                                                                process.Commandline))
        if process.Commandline.find(pattern) > -1:
            DebugLog("\n Kill {} process ID: {};Command: {} ".format(process_name,
                                                                     process.ProcessId,
                                                                     process.Commandline))
            KillProcess(process.ProcessId)


def ChangeCameraMode(newMode):
    pattern = re.compile('.*<screenSource>(.*)</screenSource>.*')
    with open(Davinci_folder + "\\DaVinci.config", 'r') as f:
        config_txt = f.readlines()
    old_mode = ''
    with open(Davinci_folder + "\\DaVinci.config", 'w') as f:
        for txt in config_txt:
            m = pattern.match(txt)
            if m:
                old_mode = m.group(1)
                f.write(txt.replace(old_mode, newMode))
            else:
                f.write(txt)
    return old_mode


@exception_logger(PrintAndLogErr)
def RunRnREnvSetup(device_name, showname, dependencies_list):
    PrintAndLogInfo('\n  Setup Environment ...')
    i = 1
    # Backup Smoke_Test_Report.csv
    report_file = r'%s\Smoke_Test_Report.csv' % work_folder
    backup_name = r'%s\____temp_backup_Smoke_Test_Report.csv' % work_folder
    prepare_report = r'%s\Environment_Setup_Report_%s.csv' % (resultpath,
                                                              os.path.splitext(os.path.basename(showname))[0])

    Enviroment.remove_item(backup_name)

    if os.path.exists(report_file):
        os.rename(report_file, backup_name)

    for dependancy in dependencies_list:
        PrintAndLogInfo('  - Step %s. Run %s...' % (str(i), dependancy))
        if not os.path.exists(dependancy):
            PrintAndLogInfo('       - %s does not exist.' % dependancy)
            continue

        start_timestamp = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())

        rnr_script_name = os.path.basename(dependancy)
        logfile_name = "%s\\%s_%s.log" % (result_folder_name, rnr_script_name, start_timestamp)
        errfile_name = "%s\\%s_err_%s.log" % (result_folder_name, rnr_script_name, start_timestamp)
        logfile = open(logfile_name, "w")
        errfile = open(errfile_name, "w")
        worker = dvcsubprocess.DVCSubprocess()
        str_timeout = ReadXMLConfig('SmokeTestTimeout')
        timeout = 900
        if str_timeout:
            timeout = int(str_timeout)
        worker.Popen("{0}\\DaVinci.exe -device {1} -p {2}".format(Davinci_folder, device_name, dependancy),
                     timeout, call_out=logfile, call_err=errfile)
        logfile.close()
        errfile.close()
        if os.path.exists(logfile_name):
            shutil.copyfile(logfile_name, "%s\\%s_%s.log" % (debug_resultpath, rnr_script_name, start_timestamp))
        if os.path.exists(errfile_name):
            shutil.copyfile(errfile_name, "%s\\%s_err_%s.log" % (debug_resultpath,
                                                                 rnr_script_name, start_timestamp))
        i += 1
    # Now report file contains the preparation scripts results, need to move the report file to result folder.
    if os.path.exists(report_file):
        shutil.move(report_file, prepare_report)
    if os.path.exists(backup_name):
        os.rename(backup_name, report_file)

    PrintAndLogInfo('\n')


def PrepareRnRStuff(rnr_xml, dependencies_list, on_device_installed):
    rnr_xml_dir = os.path.dirname(rnr_xml)
    from generate import copyFolder
    copyFolder(rnr_xml_dir, work_folder, ['Smoke_Test_Report.csv', '.xml'], ['_RECORD_', '_REPLAY_'],
               on_device_installed)
    shutil.copy(rnr_xml, work_folder)
    rnr_xml = r'%s\%s' % (work_folder, os.path.basename(rnr_xml))
    dep_list = []
    # Extract package folder name from rnr_xml file path
    # which is rnr_Path\package_name\[optional sub folder]\file_name.xml
    rnr_folder = os.path.dirname(rnr_xml_dir.replace(rnr_path + "\\", ''))
    index = rnr_folder.find("\\")
    if index > 1:
        rnr_folder = rnr_folder[0:index - 1]
    rnr_folder = rnr_path + "\\" + rnr_folder
    dependency_work_folder = r'%s\dependency_folder' % work_folder
    if dependencies_list and not os.path.exists(dependency_work_folder):
        os.mkdir(dependency_work_folder)

    dependency_folder = []
    for dependency in dependencies_list:
        try:
            if not os.path.isabs(dependency):
                dependency = r'%s\%s' % (rnr_folder, dependency)
            # If dependency file is not at the same folder of rnr script
            if os.path.exists(dependency):
                recorded_dependency_folder = os.path.dirname(dependency)
                if recorded_dependency_folder != os.path.dirname(rnr_xml) and not recorded_dependency_folder in dependency_folder:
                    copyFolder(recorded_dependency_folder, dependency_work_folder, ['Smoke_Test_Report.csv', '.xml'],
                               ['_RECORD_', '_REPLAY_'], on_device_installed)
                    shutil.copy(dependency, dependency_work_folder)
                    dependency_folder.append(recorded_dependency_folder)
                dep_list.append(r'%s\%s' % (dependency_work_folder, os.path.basename(dependency)))

        except Exception, e:
            PrintAndLogInfo('  - Failed to copy file %s: %s' % (dependency, str(e)))

    return rnr_xml, dep_list


def RunOneDavinciTest(device_name, qs_name, timeout, apk_name, test_description):
    logger = logging.getLogger('debug')

    Enviroment.remove_item(work_folder + "\\_RECORD_", True, logger.debug,
                           "  - Exception: Failed to cleanup env before run DaVinci.")
    Enviroment.remove_item(work_folder + "\\_REPLAY_", True, logger.debug,
                           "  - Exception: Failed to cleanup env before run DaVinci.")
    if camera_mode.lower() == "disabled":
        cmd = "%s\\DaVinci.exe -device %s -p \"%s\" -noimageshow -timeout %s" % (Davinci_folder, device_name,
                                                                                 qs_name, str(timeout))
    else:
        cmd = "%s\\DaVinci.exe -device %s -p \"%s\" -timeout %s" % (Davinci_folder, device_name,
                                                                    qs_name, str(timeout))

    start = datetime.datetime.now()
    start_timestamp = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
    start_time_stamp = int(time.strftime('%Y%m%d%H%M%S', time.localtime()))

    PrintAndLogInfo("  - Start at %s" % time.strftime('%Y-%m-%d %H:%M:%S', time.localtime()))
    import ctypes
    PerfLog("\n######################New Test#############################\nTest Device: %s\nQScript:%s" % (
        device_name, qs_name))

    qs_file_name = os.path.basename(qs_name)
    logfile_name = "%s\\%s_%s.log" % (result_folder_name, qs_file_name, start_timestamp)
    errfile_name = "%s\\%s_err_%s.log" % (result_folder_name, qs_file_name, start_timestamp)
    rescsv_name = "%s\\%s.csv" % (work_folder, "Critical_Path")
    logfile = open(logfile_name, "w")
    errfile = open(errfile_name, "w")
    if Is_Debug:
        process = subprocess.Popen(cmd, stdout=logfile, stderr=errfile)
    else:
        # Do not show crash dialog
        SEM_NOGPFAULTERRORBOX = 0x0002  # From MSDN
        ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX)
        CREATE_NO_WINDOW = 0x8000000
        subprocess_flags = CREATE_NO_WINDOW
        process = subprocess.Popen(cmd, stdout=logfile, stderr=errfile, creationflags=subprocess_flags)

    pid = process.pid
    while process.poll() is None:
        try:
            p = psutil.Process(pid)

            cpu_percent = p.cpu_percent(interval=0.1)
            cpu_count = int(psutil.cpu_count())
            cpu_maxload = 100 * cpu_count
            cpu_workload = (cpu_percent / cpu_maxload) * 100
            pmem = p.memory_info()
            rss = int(pmem.rss / 1000)
            PerfLog("CPU: %s%s" % (str(cpu_workload), "%"))
            PerfLog("Mem Usage: %s(K)" % (str(rss)))
        except Exception, e:
            DebugLog(str(e))

            # some time Davinci crashed, its PID cannot be found but the process is not terminate
            # due to sub process is blocked.
            # Need to terminate process right now without waiting for timeout.
            process.terminate()
            break

        time.sleep(10)
        # Check CPU and memory usage
        now = datetime.datetime.now()
        # Time out...
        if (now - start).seconds > timeout:
            PrintAndLogInfo("  - Test Timeout %ssec." % (str((now - start).seconds)))
            # If debug, do not terminate the process so that we can investigate.
            with open(no_result_app_list, 'a') as f:
                f.write("Timeout|---|" + os.path.basename(apk_name) + "|---|" + errfile_name + "|---|" +
                        test_description + os.linesep)
            if not Is_Debug:
                process.terminate()
    PrintAndLogInfo("  - End at %s" % time.strftime('%Y-%m-%d %H:%M:%S', time.localtime()))
    # process is done. Calculate the execution time.
    now = datetime.datetime.now()
    minute = ((now - start).seconds / 60)
    PrintAndLogInfo("  - Finished in %s minutes." % (str(minute)))

    logfile.close()
    errfile.close()
    return errfile_name, rescsv_name, start_time_stamp


def CopyReplayFolder(log_folder, apk_name, pkg_name, seed_value):
    # Copy the replay folder to result folder.
    try:
        i = log_folder.index('"')
        j = log_folder.index('"', i + 1)
        log_folder = log_folder[i + 1:j]
    except:
        pass
    log_folder = os.path.dirname(log_folder)
    replay_folder = '{0}\Replay'.format(log_folder)
    if os.path.exists(replay_folder):
        relay_log_folder = r'{0}\Replay'.format(resultpath)
        replay_apk_folder = r'{0}\APKs'.format(relay_log_folder)
        replay_rnr_root = r'{0}\RnR'.format(relay_log_folder)
        if not os.path.exists(relay_log_folder):
            os.mkdir(relay_log_folder)
            os.mkdir(replay_apk_folder)
            os.mkdir(replay_rnr_root)
        if not os.path.exists(r'{0}\{1}'.format(replay_apk_folder, os.path.basename(apk_name))):
            apk_full_path = apk_name
            if not os.path.isabs(apk_full_path):
                apk_full_path = r'{0}\{1}'.format(work_folder, apk_name)
            if os.path.exists(apk_full_path):
                copyFileExt(apk_full_path, replay_apk_folder)
        replay_result_folder = r'{0}\{1}'.format(replay_rnr_root, pkg_name)
        if not os.path.exists(replay_result_folder):
            copyTreeExt(replay_folder, replay_result_folder)
            # Copy avi to result rnr folder.
            replay_video = apk_folder + "\\" + pkg_name + "_seed_{}_replay.avi".format(seed_value)
            if os.path.exists(replay_video):
                copyFileExt(replay_video, replay_result_folder)


def ValidateOneTestResult(start_time_stamp, errfile_name, apk_name, device_name, test_description):
    DebugLog("\n#######APK test is done: %s#######" % apk_name)
    CleanProcess('  -device %s ' % device_name, "DaVinci.exe")
    CleanProcess(r'%s\platform-tools\aapt.exe' % Davinci_folder, "aapt.exe")

    cameraless_issue_keyword = ["Error starting Camera-less with error status code",
                                "Camera-less OnError status"]


    error_found, key_found = CheckDavinciLog(errfile_name, cameraless_issue_keyword)
    err_msg = "  - Camera-less has problem."
    fail_reason = "Camera-less error"
    mode_changed = False
    if key_found:
        # will change to disabled mode.
        old_mode = ChangeCameraMode('Disabled')
        if old_mode != 'Disabled':
            mode_changed = True
    else:
        # QAgent disconnect means the Davinci test did not execute completely.
        qagent_issue_keyword = [" QAgent Connection = False",
                                " Failed: Connecting to MAgent failure",
                                " Failed: Connecting to QAgent failure",
                                " refer to use Diagnostic.exe to get more detail information.",
                                " GetDataFromMAgent failure, isMAgentConnected: false, sendStr size",
                                " Failed to get orientation data from device. Data incorrect: -1",
                                " Error: Could not access the Package Manager.  Is the system running?",
                                " Failed to connect device during installation stage",
                                " Failed: The installation log is empty",
                                " Unable to call adb get-state on device"]
        error_found, key_found = CheckDavinciLog(errfile_name, qagent_issue_keyword)
        err_msg = "  - Connection to device has problem."
        fail_reason = 'Device Connection Error'

    # Need to check the result, if there is result created, mark it as SKIP
    res = 'ok'
    result_line = GetLastLineOfReport(1)
    if key_found and result_line or Enviroment.get_keyword_times(errfile_name, "Connecting to QAgent") > 2:
        PrintAndLogErr(err_msg)
        res_timestamp = int(result_line.split(',')[0].replace("T", ""))
        if res_timestamp > start_time_stamp:
            # test_res = result_line.split(',')[11]
            app_name = result_line.split(',')[2]
            dev_name = result_line.split(',')[1]

            apk_name_type = os.path.splitext(apk_name)
            # Sometime, if no local .apk file or failed to fetch the apk file
            # we will save package name as apk_name.
            if len(apk_name_type) > 1 and apk_name_type[1].lower() != ".apk":
                # define app_name as package name
                app_name = result_line.split(',')[4]
            # Mark the result as skip.
            if dev_name.strip() == device_name.strip() and app_name == os.path.basename(apk_name):
                timestamp_tmp = result_line.split(',')[0]

                res_con = {'Launch': 'SKIP', 'Random': 'SKIP', 'Back': 'SKIP', 'Result': 'SKIP',
                           'FailReason': fail_reason, 'TestDescription': test_description}
                MarkLastLine(res_con, timestamp_tmp)
        res = 'error'
    return res, mode_changed


def CheckTestResult(qs_name, apk_name, errfile_name, test_description, start_time_stamp, device_name, seed_value,
                    error_found, ondevice_download, rescsv_name):
    result_line = GetLastLineOfReport(1)
    logcat_flag = False
    if result_line == "":
        logcat_flag = True
        PrintAndLogErr("  - No result for this %s in the report." % qs_name)
        logger = logging.getLogger('debug')
        logger.debug("\nresult line:%s\n" % result_line)
        res = -2
        with open(no_result_app_list, 'a') as f:
            f.write("No Result|---|" + os.path.basename(apk_name) + "|---|" + errfile_name + "|---|" +
                    test_description + os.linesep)
    else:
        test_res = result_line.split(',')[11]
        pkg_name = result_line.split(',')[4]
        # tmp_qs_name = pkg_name + ".qs"
        app_name = result_line.split(',')[2]
        dev_name = result_line.split(',')[1]
        timestamp_tmp = result_line.split(',')[0]
        apk_name_type = os.path.splitext(apk_name)
        # Sometime, if no local .apk file or failed to fetch the apk file,
        # we will save package name as apk_name.
        if len(apk_name_type) > 1 and apk_name_type[1].lower() != ".apk":
            # define app_name as package name
            app_name = result_line.split(',')[4]

        # Compare the time stamp, and if the timestamp of last record is earlier than the start time,
        # it means no result.
        if int(timestamp_tmp.replace('T', '')) > start_time_stamp:
            # if dev_name.strip() == device_name.strip() and app_name == os.path.basename(apk_name):
            # Pushing the result to Cloud client if 'CloudMode' node is "True"
            if ReadXMLConfig('CloudMode') == "True":
                web_result = upstream_composer.UpstreamComposer.compose_overall_result(result_line,
                                                                                       options.xml_file)
                tm_utility.send_msg(json.dumps(web_result))
            if test_res.lower() == "fail":
                PrintAndLogErr("  - Test Failed")
                log_folder = result_line.split(',')[13]

                pkg_name = result_line.split(',')[4]
                CopyReplayFolder(log_folder, apk_name, pkg_name, seed_value)

                CallDebug(device_name, log_folder)
                res = 1
            elif test_res.lower() == "pass":
                remove_replay_video(apk_folder, pkg_name, seed_value)

                PrintAndLogInfo("  - Test Pass")
                res = 0
            elif test_res.lower() == "skip":
                remove_replay_video(apk_folder, pkg_name, seed_value)

                fail_reason = result_line.split(',')[12]
                res = -1
                msg = "  - To Skip Test"
                if fail_reason.strip() == "":
                    if error_found:
                        msg = "  - Execution is abnormal."
                        MarkLastLine({'FailReason': 'Execution is abnormal'}, timestamp_tmp)
                PrintAndLogInfo(msg)
            elif test_res.lower() == "warning":
                remove_replay_video(apk_folder, pkg_name, seed_value)

                PrintAndLogInfo("  - Test Warning. Need to double check.")
                res = 2
            else:
                PrintAndLogErr("  - Unknown result: %s" % test_res)
                PrintAndLogErr("  - No result for this %s in the report!" % qs_name)
                res = -4

            if ondevice_download:
                MarkLastLine({'OnDeviceDownload': 'Yes'}, timestamp_tmp)
                apk_name_type = os.path.splitext(apk_name)
                # Sometime, if no local .apk file or failed to fetch the apk file, we will save package name
                # as apk_name, and will get its Appname information from pre-pared dict.
                if len(apk_name_type) > 1 and apk_name_type[1].lower() != ".apk":
                    if apk_name in package_info_dict:
                        MarkLastLine({'AppName': (package_info_dict[apk_name])[0]}, timestamp_tmp)

                    else:
                        MarkLastLine({'AppName': 'Failed to get APP name'}, timestamp_tmp)

            MarkLastLine({'TestDescription': test_description}, timestamp_tmp)
        else:
            logcat_flag = True
            PrintAndLogErr("  - No result for this %s in the report." % qs_name)
            logger = logging.getLogger('debug')
            logger.debug("\nresult line:%s\n" % result_line)
            logger.debug("\ndevice in result:%s\nQS testing:%s\nQS in result:%s\n"
                         % (dev_name, os.path.basename(qs_name), app_name))
            res = -2
            with open(no_result_app_list, 'a') as f:
                f.write("No Result|---|" + os.path.basename(apk_name) + "|---|" + errfile_name + "|---|" +
                        test_description + os.linesep)

        if os.path.splitext(qs_name)[1].strip() == '.xml':
            result = ''
            f = open(errfile_name)
            for line in f:
                if line.find("PASS RATE") != -1:
                    result = "  - %s:\t %s:\t %s\n" % (timestamp, apk_name, line.split(":")[-1].strip())
                    break
                else:
                    result = "  - %s:\t %s:\t %s\n" % (timestamp, apk_name, "Test Error, no result found!")
            f.close()
            if result and result.find(':') > -1:
                CreateCSV(rescsv_name, apk_name, result.split(":")[-1].strip())
    return logcat_flag, res


@exception_logger(PrintAndLogErr)
def RunDavinci(device_name, apk_name, qs_name='', dependencies_list=None, test_description='', seed_value=1,
               abi_value='java', needInstall="yes", need_uninstall="yes", ondevice_download=False):
    if dependencies_list is None:
        dependencies_list = []
    # Before generate qs, cleanup the existing ones.
    map(Enviroment.remove_item,
        (os.path.join(work_folder, i) for i in os.listdir(work_folder)
         if os.path.splitext(i)[1].strip() in [".qs", ".xml"]))
    try:
        showname = apk_name.encode(syslocale)
    except:
        showname = apk_name.encode('utf-8')

    PrintAndLogInfo('  [Testing Application]:\t' + showname)
    PrintAndLogInfo('  [Testing device]:\t\t' + device_name)

    if qs_name != '' and os.path.splitext(qs_name)[1] == '.xml':
        PrintAndLogInfo('  [RnR Folder]:\t\t\t' + rnr_path)
        PrintAndLogInfo('  [RnR selected]:\t\t' + qs_name.replace(rnr_path + '\\', ''))
        qs_name, dependencies_list = PrepareRnRStuff(qs_name, dependencies_list, ondevice_download)
        if dependencies_list:
            RunRnREnvSetup(device_name, showname, dependencies_list)
        generateRnRQS(apk_name, os.path.dirname(qs_name), work_folder, ondevice_download,
                      needInstall, need_uninstall)
    else:
        # Couldn't get test result if it is failed to generate QSfile for on apk.
        # If it's on-device download, global apk_folder shall be "".
        # And work_folder shall be set as what user input.
        PrintAndLogInfo('  [Seed Value]:\t\t\t' + str(seed_value))
        if abi_value != 'java':
            PrintAndLogInfo('  [ABI]:\t\t\t' + str(abi_value))
        qs_name = generateQSfile(Davinci_folder, script_path, apk_folder, apk_name, work_folder,
                                 seed_value, abi_value, ondevice_download, needInstall, need_uninstall)
    if not qs_name:
        PrintAndLogErr("  Error: Failed to create QScript for %s" % apk_name)
        return 2

    PrintAndLogInfo('  Please wait...')
    str_timeout = ReadXMLConfig('SmokeTestTimeout')
    timeout = 900
    if str_timeout:
        timeout = int(str_timeout)

    re_test = 2
    mode_changed = False
    errfile_name = ''
    start_time_stamp = 0
    rescsv_name = ''
    error_found = False
    while re_test > 0:
        re_test -= 1

        errfile_name, rescsv_name, start_time_stamp = RunOneDavinciTest(device_name, qs_name, timeout, apk_name,
                                                                        test_description)

        check_res, mode_changed = ValidateOneTestResult(start_time_stamp, errfile_name, apk_name, device_name, test_description)
        if check_res == 'error' and re_test > 0:
            PrintAndLogErr("\n* Rerun the test.")
            # Reboot the device and then rerun.
            RebootDevice(device_name)
        else:
            break

    logcat_flag, res = CheckTestResult(qs_name, apk_name, errfile_name, test_description, start_time_stamp, device_name,
                                       seed_value, error_found, ondevice_download, rescsv_name)

    # save logcat file if no result or timeout case occurs
    if logcat_flag:
        PrintAndLogErr("\n Saving logcat file for timeout/no result case for %s" % showname)

        adb_obj = adbhelper.AdbHelper(r'%s\platform-tools\adb.exe' % Davinci_folder)
        adb_obj.device_id = device_name
        logcat_result = adb_obj.adb_execution("logcat", 120)[0]

        with open("logcat_no_result_timeout_{}.txt".format(timestamp.replace(":", "-")), "wb") as file_obj:
            file_obj.write(logcat_result)

    # If time out, or run error. Properbaly, the enviornments has some problem. Need to recover them.
    if res == -3 or res == -2 or error_found:
        # Jia Device state will be destroied sometimes when DaVinci is abnormal,
        # such as surfaceflinger issue on Lenovo X2, so device reboot is needed.
        # Try to recover host's environment.
        RecoverDevice(device_name)

    # Track ADB process information for debug purpose
    FindADBProcess()

    # For RnR script, need to clean the stuff
    if os.path.splitext(qs_name)[1].strip() in ['.xml'] and rnr_path != '':
        # in function generateQSfile(), we copied all stuff in rnr_folder to working folder to run rnr test.
        # After test is done, we shall remove all these stuff if these stuff is copied from rnr_path
        CleanupRnRStuff(work_folder, os.path.dirname(qs_name))

    # If mode changed, recover the config.
    if mode_changed:
        ChangeCameraMode('HyperSoftCam')
    return res


@exception_logger(PrintAndLogErr)
def GetDeviceInfo(device_name):
    priority_items = ReadXMLConfig('RnRPriorityItems').split(',')

    dev_config_str = ','.join( [i for i in priority_items if i.lower() != 'appver'])
    device_info = DeviceInfo(r'%s\platform-tools\adb.exe' % Davinci_folder, device_name, xml_config)
    # Complete the device info in config.
    device_info_dict = device_info.GetDevInfoFromConfig(dev_config_str)
    return device_info_dict


@exception_logger(PrintAndLogErr)
def VerifyDeviceConfig():

    target_type = {}
    target_dev = []
    type_name = {}
    # Check there is any devices connected.
    devices = AnyDevice()
    if len(devices) == 0:
        return -1

    # Get the device list from the configuration file.
    dev_config_file = open(config_file_name, "r")
    for dev in dev_config_file:
        dev_info = dev.split('<>*<>')
        # all_dev.append(dev_info[1].strip())
        if dev_info[0].strip() == '[target_dev]':
            target_dev.append(dev_info[1].strip())
            if len(dev_info) >= 4:
                type_value = dev_info[3].strip().strip('\n')
                type_index = int(type_value)
                if len(dev_info) >= 5:
                    type_name[type_index] = dev_info[4].strip().strip('\n')
                if type_index not in target_type:
                    target_type[type_index] = []
                (target_type[type_index]).append(dev_info[1].strip())
    dev_config_file.close()

    # Check that the devices defined in config files are connected.
    all_dev = target_dev
    bad_dev = [i for i in all_dev if i not in devices]

    if bad_dev:
        PrintAndLogErr("\nWARNING: Please make sure devices below are power on and connected correctly!!!")
        PrintAndLogErr(','.join(bad_dev))
        return -1

    # Complete the dev info in the SmartRunner.config
    if target_dev:
        PrintAndLogInfo('\nFetching Device Information ...')
        PrintAndLogInfo('Device: ' + target_dev[0])
        config_items = ReadXMLConfig('RnRPriorityItems').split(',')
        dev_config_str = ','.join([i for i in config_items if i.lower() != 'appver'])
        device_info = DeviceInfo(r'%s\platform-tools\adb.exe' % Davinci_folder, target_dev[0], xml_config)
        # Complete the device info in config.
        device_info.CompleteDevInfo(dev_config_str)
        # Get the device informaiton
        device_info_dict = GetDeviceInfo(target_dev[0])
        for key, value in device_info_dict.iteritems():
            PrintAndLogInfo('%s : %s' % (key, value))
        PrintAndLogInfo('\n')

    return target_type, target_dev, type_name


@exception_logger(PrintAndLogErr)
def RunTestAll(list_file):
    res = VerifyDeviceConfig()
    if res == -1:
        return res

    target_type, target_dev, type_name = res
    # Check for the last qs name and last target device name
    result_line = GetLastLineOfReport(1)
    last_apk = ""
    last_device_name = ""
    has_breakpoint = False
    if result_line:
        last_device_name = result_line.split(',')[1]
        if last_device_name in target_dev:
            # package_name = result_line.split(',')[4]
            # last_qs = package_name + ".qs"
            last_apk = result_line.split(',')[2]
        else:
            PrintAndLogInfo("Device is changed. Last device %s is not connected." % last_device_name)
            dev_name = last_device_name

            if ':' in last_device_name:
                dev_name = last_device_name.replace(':', '_')

            PrintAndLogInfo("Will rename Smoke_Test_Report.csv to Smoke_Test_Report_%s.csv" % dev_name)

            os.rename(work_folder + "\\" + "Smoke_Test_Report.csv",
                      work_folder + "\\Smoke_Test_Report_%s.csv" % dev_name)

    last_device_type = -1
    for key in target_type.keys():
        devices = target_type[key]
        if last_device_name in devices:
            last_device_type = key
            break

    types_to_run = []
    type_num = len(target_type.keys())
    if last_apk:
        if xml_config:
            ch = "".join(ParseXML(xml_config, 'Breakpoint'))
            if ch == "True":
                print "Last time, Device (type%s:%s) is in testing and stopped at APK '%s'" % (
                    str(last_device_type), last_device_name, last_apk)
                user_selection = get_user_input("Will you continue from this breakpoint? "
                                                "If no, start a totally new round test(y/n):", 'y')
                if user_selection.lower() != "n":
                    has_breakpoint = True
        if has_breakpoint is False:
            # Before start a totally new round test, need to remove the related reports.
            Enviroment.remove_item(os.path.join(work_folder, "Smoke_Test_Report.csv"))
            Enviroment.remove_item(os.path.join(work_folder, "Smoke_Test_Summary.csv"))

            last_apk = ""
            types_to_run = target_type.keys()

    # Install watch cat on all dev.
    all_dev = target_dev
    InstallWatchCat(all_dev)
    Uninstall3rdApps(all_dev)

    logging.basicConfig(filename='wifi.log', level=logging.INFO)
    logger = logging.getLogger('wifi')
    Wifi_obj = InstallWifiApk(r'%s\platform-tools\adb.exe' % Davinci_folder, script_path, logger)
    Wifi_obj.InstallWiFiChecker(all_dev)

    pre_install_folder = ReadXMLConfig("PreInstallApksFolder")

    if pre_install_folder and os.path.exists(pre_install_folder):
        logging.basicConfig(filename='pre_install.log', level=logging.INFO)
        logger = logging.getLogger('pre_install')
        pre_install_obj = InstallAndLaunchApk(r'%s\platform-tools\adb.exe' % Davinci_folder,
                                              r'%s\platform-tools\aapt.exe' % Davinci_folder, pre_install_folder,
                                              logger)
        pre_install_obj.InstallApks(all_dev)
        pre_install_obj.LaunchApks(all_dev)

    for index in range(1, type_num + 1):
        if types_to_run and (not index in types_to_run):
            continue
        if has_breakpoint and index < last_device_type:
            continue
        devices = target_type[index]
        RunAllQS(devices, last_apk, list_file)

        # The breakpoint is invalid once it is used.
        has_breakpoint = False
        last_apk = ""

    # Clean up watch cat process
    CleanupToQuit()


def signal_handler():
    # Clean up watch cat process
    PrintAndLogInfo("Terminate the tests, please wait to clean up... ")
    CleanupToQuit()
    time.sleep(2)
    sys.exit(-1)


@exception_logger(PrintAndLogErr)
def PrepareTestList(list_file):
    files = []
    if list_file:
        PrintAndLogInfo("APK list: %s " % list_file)
        PrintAndLogInfo("Checking APK list...(Use APK name or absolute path. And APK exist.)")
        with open(list_file, 'r') as f:
            tmp_files = f.readlines()
        for i in tmp_files:
            try:
                tmp_apk = i.strip().decode("utf8")
            except Exception, e:
                PrintAndLogInfo(e.message)
                PrintAndLogInfo("[Warning]: Please make sure the APK list file is encoded as UTF8.")

            if tmp_apk.endswith(".apk") and ((os.path.isabs(tmp_apk) and os.path.exists(tmp_apk)) or (
                    tmp_apk.find("\\") < 0 and os.path.exists(apk_folder + "\\" + tmp_apk))):
                files.append(tmp_apk)
            else:
                try:
                    tmp_apk = i.strip().encode(syslocale)
                    PrintAndLogInfo(r'   - Will not test %s' % tmp_apk)
                except:
                    tmp_apk = i.strip().encode('utf8')
                    PrintAndLogInfo('   - Will not test %s' % tmp_apk)

    else:
        files = glob.glob(ur'%s\*.apk' % apk_folder)
    return files


@exception_logger(PrintAndLogErr)
def PeriodicalReboot(reboot_timing, start, all_dev):
    # If we will reboot after each APK test, to save time, skip periodically reboot.
    next_reboot_time = start
    if ReadXMLConfig('RebootForEachAPK').lower() != 'true':
        now = datetime.datetime.now()
        if now > start:
            next_reboot_time = datetime.datetime.now() + datetime.timedelta(hours=reboot_timing)
            PrintAndLogInfo("\n* It's time to clear cache and reboot all devices to avoid device offline ...")
            ClearCache(all_dev)
            RebootDevices(all_dev)

            # after rebooting device, need to launch apps again
            pre_install_folder = ReadXMLConfig("PreInstallApksFolder")
            if pre_install_folder and os.path.exists(pre_install_folder):
                logging.basicConfig(filename='launch_pre_install_apk.log', level=logging.INFO)
                logger = logging.getLogger('launch_pre_install_apk')
                pre_install_obj = InstallAndLaunchApk(r'%s\platform-tools\adb.exe' % Davinci_folder,
                                                      r'%s\platform-tools\aapt.exe' % Davinci_folder,
                                                      pre_install_folder, logger)
                pre_install_obj.LaunchApks(all_dev)
    return next_reboot_time


@exception_logger(PrintAndLogErr)
def DecideAPKabi(filename):
    multi_abi_flag = ReadXMLConfig("MultiAbi")
    apk_abis = []
    if not multi_abi_flag or multi_abi_flag == 'n':
        apk_abis = ['java']
    if ContainNoneASCII(filename):
        nonascii_apk_folder = os.path.join(work_folder, 'nonasciiapk')
        if not os.path.exists(nonascii_apk_folder):
            os.mkdir(nonascii_apk_folder)
        tmp_apk_name = os.path.join(nonascii_apk_folder, get_string_hash(filename) + '.apk')
        if not os.path.exists(tmp_apk_name):
            shutil.copy(filename, tmp_apk_name)
        app_version = getAppVersion(Davinci_folder, tmp_apk_name).decode('utf-8')

        if multi_abi_flag == 'y':
            aapthelper = AAPTHelper(r'%s\platform-tools\aapt.exe' % Davinci_folder)
            apk_abis = aapthelper.get_apk_abi(tmp_apk_name)
    else:
        app_version = getAppVersion(Davinci_folder, filename).decode('utf-8')
        if multi_abi_flag == 'y':
            aapthelper = AAPTHelper(r'%s\platform-tools\aapt.exe' % Davinci_folder)
            apk_abis = aapthelper.get_apk_abi(filename)
    return multi_abi_flag, apk_abis, app_version


@exception_logger(PrintAndLogErr)
def DecideExecutionParameters(suite_count):
    is_rnr_test = False
    test_count = suite_count
    if test_count > 0:
        PrintAndLogInfo("\n* Totally %s RnR Tests Found." % str(test_count))
        is_rnr_test = True
        seed_value_set = [1] * test_count
    else:
        PrintAndLogInfo("\n* Will launch Smoke Test.")
        seed_value_str = ReadXMLConfig("SeedValue")
        seed_value_set = [int(i) for i in seed_value_str.split(',') if i]
        test_count = len(seed_value_set)

        seed_value_input = get_user_input(
            "Would you like to use seed:{} for testing? Or please enter the new seeds".format(
                seed_value_str), seed_value_str, hidden_flag=False, timeout=10)

        if seed_value_input:
            match_pattern = r"^\d+-\d+$|^(\d+,)*\d+$"
            match = re.match(match_pattern, seed_value_input)
            if match:
                seed_value_res = match.group()
                if seed_value_res.find(',') == -1 and seed_value_res.isdigit() \
                        and seed_value_res.count('-') == 0:

                    seed_value_tmp = seed_value_res

                elif seed_value_res.count('-') == 1:
                    range_value = seed_value_res.split('-')
                    left_value = range_value[0]
                    right_value = range_value[1]
                    if left_value.isdigit() and right_value.isdigit() \
                            and 0 < int(left_value) < int(right_value):
                        seed_value_tmp = ','.join(str(i) for i in range(int(left_value),
                                                                        int(right_value) + 1))
                    else:
                        print 'invalid input, will use the default seed values'
                        seed_value_tmp = seed_value_str
                else:
                    seed_value_tmp = ','.join(seed_value_res.split(','))

            else:
                print 'invalid input, will use the default seed values'
                seed_value_tmp = seed_value_str

            seed_value_set = [int(i) for i in seed_value_tmp.split(',') if i]
            test_count = len(seed_value_set)
    return is_rnr_test, test_count, seed_value_set


@exception_logger(PrintAndLogErr)
def FirstTestExecutuion(rerun_max, force_online_download_apk_list, package_name,
                        current_target_dev, filename, rnr_script, dep_list, test_description,
                        each_round_seed, each_round_abi
                        ):

    need_install = 'yes'

    if force_online_download_apk_list and package_name in force_online_download_apk_list:
        # online downloading
        installed, is_compatible, has_error = InstallPackageFromDevice(current_target_dev,
                                                                       package_name,
                                                                       test_description)

        if installed:
            res = RunDavinci(current_target_dev, filename, rnr_script, dep_list, test_description,
                             each_round_seed, each_round_abi, "no", "no", True)
            need_install = 'no'
        else:
            if not is_compatible:
                PrintAndLogErr("  - [Force] Incompatible APK.")
            if has_error:
                PrintAndLogErr("  - [Force] Error during on-device download.")
            else:
                PrintAndLogErr("  - [Force] Failed to install.")

            res = RunDavinci(current_target_dev, filename, rnr_script, dep_list, test_description,
                             each_round_seed, each_round_abi)
    else:
        res = RunDavinci(current_target_dev, filename, rnr_script, dep_list, test_description,
                         each_round_seed, each_round_abi)
    # Re-test for timeout & no result cases until to fetch the test result.
    rerun_count = 0
    if res == -3 or res == -2 or res == -5:
        rerun_count = 1
        while (res == -3 or res == -2 or res == -5) and rerun_count <= rerun_max:
            # Run again and again until Davinci finish the test ...
            PrintAndLogInfo("\n* No result. Re-testing for 1st Round...")
            res = RunDavinci(current_target_dev, filename, rnr_script, dep_list, test_description,
                             each_round_seed, each_round_abi)
            rerun_count += 1
    return res, need_install, rerun_count


@exception_logger(PrintAndLogErr)
def FirstRoundTest(force_online_download_apk_list, filename, rnr_script, dep_list, current_target_dev, package_name,
                   test_description, each_round_seed, each_round_abi, app_version):

    rerun_max = int(ReadXMLConfig('RerunValue')) if ReadXMLConfig('RerunValue').isdigit() else 1

    res, need_install, rerun_count = FirstTestExecutuion(rerun_max, force_online_download_apk_list, package_name,
                                                         current_target_dev, filename, rnr_script, dep_list,
                                                         test_description, each_round_seed, each_round_abi)

    if ReadXMLConfig('SkipFirstRoundNoResult').lower() == 'true' and rerun_count >= rerun_max:
        # no result of  round 1 testing
        with open(no_result_app_list, 'a') as f:
            f.write("++++++ Tried %s times. Still no result for:%s:%s(%s:%s:%s) %s"
                    % (str(1 + rerun_count), os.path.basename(filename), test_description,
                       package_name, app_version, current_target_dev, os.linesep))

        uninstallpackage(package_name, current_target_dev)
        PrintAndLogInfo("\n* Test Done")
        return None

    # If pass or APK skipped, go ahead
    execution_criteria = ReadXMLConfig('ExecutionCriteria').strip()
    if execution_criteria == '':
        execution_criteria = '1'
    if (execution_criteria == '1' and res == 0) or res == -1:
        # Reboot after each APK test.
        if ReadXMLConfig('RebootForEachAPK').lower() == 'true':
            RebootDevice(current_target_dev)

        uninstallpackage(package_name, current_target_dev)
        return None

    need_uninstall = 'yes'
    reinstall_ondevice = False
    apk_file_path = filename
    pulled_file_path = ''
    # If timeout, no result, or fail, firstly, try to redownload from google play before retest.
    if res == -3 or res == -2 or res == 1:
        # If config as "re-download", Call downloader and retest again.
        redownload = ReadXMLConfig('ReDownloadOnDevice')

        if redownload.lower().strip() == 'true':
            # Check the package is installed on device or not.
            PrintAndLogInfo(
                "  - Re-downloading %s from Google Play on device. Please wait ..." % package_name)
            is_installed, is_compatible, has_error = InstallPackageFromDevice(current_target_dev,
                                                                              package_name,
                                                                              test_description)
            if is_installed:
                # If reinstall on device successfully, no need to add "OPCODE_INSTALL_APP" in qs.
                need_install = 'no'
                need_uninstall = 'no'
                reinstall_ondevice = True

                # Pull the apk if installed.
                pack_list = []
                pack_list.append(package_name)
                failed_list = []
                new_apk_folder = r'%s\ondevice_downloaded_APKs_%s' % (work_folder,
                                                                      current_target_dev)
                pulled_file_path = r'%s\%s.apk' % (new_apk_folder, package_name)
                if os.path.exists(new_apk_folder) and os.path.exists(pulled_file_path):
                    os.remove(pulled_file_path)
                else:
                    os.mkdir(new_apk_folder)
                fetch_apk(pack_list, work_folder, new_apk_folder, 2,
                          current_target_dev, failed_list)

                # Rename the pull apk which is named after package name.
                if not failed_list and os.path.exists(pulled_file_path):
                    apk_file_path = r'%s\%s' % (new_apk_folder, os.path.basename(filename))
                    shutil.move(pulled_file_path, apk_file_path)
                else:
                    pulled_file_path = ''

            else:
                if not is_compatible:
                    PrintAndLogErr("  - Incompatible APK on Google Play.")
                if has_error:
                    PrintAndLogErr("  - Error during on-device download.")
                else:
                    PrintAndLogErr("  - Failed to install.")
    return res, reinstall_ondevice, need_install, need_uninstall, apk_file_path, pulled_file_path


@exception_logger(PrintAndLogErr)
def NextTestRounds(showname, current_target_dev, apk_file_path, rnr_script, dep_list, test_description, each_round_seed,
                   each_round_abi, need_install, need_uninstall, reinstall_ondevice, first_res):
    test_round = 2
    ondevice_download = False
    pass_number = 0

    execution_criteria = ReadXMLConfig('ExecutionCriteria').strip()
    if execution_criteria == '':
        execution_criteria = '1'

    test_times = int(ReadXMLConfig('TestNumber')) if ReadXMLConfig('TestNumber').isdigit() else 3

    # If installed on device, skip the first round. So start from round 1 again.
    no_result_reported = True
    if reinstall_ondevice:
        test_round = 1
        ondevice_download = True
    else:
        # If fail or warning, it means davinci reported result.
        if first_res == 1 or first_res == 2 or first_res == 0:
            no_result_reported = False
        if first_res == 0:
            pass_number = 1

    # By default, pass criteria is test time -1.
    pass_criteria = test_times - 1
    if pass_criteria == 0:
        pass_criteria = 1
    try:
        pass_criteria = int(ReadXMLConfig('PassCriteria'))
    except:
        pass

    result = ""
    while True:
        PrintAndLogInfo("\n* Start Round %s Testing ..." % str(test_round))
        # If it is the last time, need to uninstall APK.
        if test_round == test_times:
            need_uninstall = 'yes'
        res = RunDavinci(current_target_dev, apk_file_path, rnr_script, dep_list,
                         test_description, each_round_seed, each_round_abi,
                         need_install, need_uninstall, ondevice_download)

        # pass (0), fail (1), warning (2), skip (-1)
        if res == 0 or res == 1 or res == 2 or res == -1:
            no_result_reported = False

        if res == 0:
            pass_number += 1

        # In execution_criteria 1, 2, If meet the pass criteria, skip.
        if execution_criteria == '1' or execution_criteria == '2':
            if pass_number == pass_criteria:
                result = "PASS"
                PrintAndLogInfo("\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  - "
                                "Total rounds(s): %s\n  - Executed round(s):%s\n  - Passed: %s\n" %
                                (showname, result, pass_criteria, test_times, test_round,
                                 pass_number))
                break

            # If remaining test rounds cannot meet pass criteria, skip
            if test_times >= test_round and test_times - test_round + pass_number < pass_criteria:
                result = "FAIL"
                PrintAndLogInfo("\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  - "
                                "Total rounds(s): %s\n  - Executed round(s):%s\n  - Passed: %s\n" %
                                (showname, result, pass_criteria, test_times, test_round,
                                 pass_number))
                break

        # In execution_criteria 3, will test until test times is met.
        if execution_criteria == '3' and test_round == test_times:
            if pass_number >= pass_criteria:
                result = "PASS"
                PrintAndLogInfo(
                    "\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  -"
                    " Total rounds(s): %s\n  - Executed round(s):%s\n  - Passed: %s\n" %
                    (showname, result, pass_criteria, test_times, test_round,
                     pass_number))
            else:
                result = "FAIL"
                PrintAndLogInfo(
                    "\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  -"
                    " Total rounds(s): %s\n  - Executed round(s):%s\n  - Passed: %s\n" %
                    (showname, result, pass_criteria, test_times, test_round,
                     pass_number))

        if test_round == test_times:
            break

        # Next test round
        test_round += 1

        # Reboot after each APK test.
        if ReadXMLConfig('RebootForEachAPK').lower() == 'true':
            RebootDevice(current_target_dev)
    return result, no_result_reported


@exception_logger(PrintAndLogErr)
def TestOneAPK(force_online_download_apk_list, filename, rnr_script, dep_list, current_target_dev, package_name,
               test_description, each_round_seed, each_round_abi, app_version, showname ):
    res, wifi_screenshot_file = CheckWIFI(current_target_dev)
    if res == 'false':
        PrintAndLogErr("  - [Warning] Network is not available. The testing may be impacted.")

    return_obj = FirstRoundTest(force_online_download_apk_list, filename, rnr_script, dep_list, current_target_dev,
                                package_name, test_description, each_round_seed, each_round_abi, app_version)

    # If no returned value, go to next APK.
    if return_obj is None:
        return

    res, reinstall_ondevice, need_install, need_uninstall, apk_file_path, pulled_file_path = return_obj
    if pulled_file_path != '':
        showname = pulled_file_path

    # Next test round number.
    first_res = res
    result, no_result_reported = NextTestRounds(showname, current_target_dev, apk_file_path, rnr_script, dep_list,
                                                test_description, each_round_seed, each_round_abi,
                                                need_install, need_uninstall, reinstall_ondevice, first_res)

    if no_result_reported:
        if ContainNoneASCII(filename):
            nonascii_apk_folder = os.path.join(work_folder, 'nonasciiapk')
            if not os.path.exists(nonascii_apk_folder):
                os.mkdir(nonascii_apk_folder)
            tmp_apk_name = os.path.join(nonascii_apk_folder, get_string_hash(filename) + '.apk')
            if not os.path.exists(tmp_apk_name):
                shutil.copy(filename, tmp_apk_name)
            app_version = getAppVersion(Davinci_folder, tmp_apk_name).decode('utf-8')
        else:
            app_version = getAppVersion(Davinci_folder, filename).decode('utf-8')

        with open(no_result_app_list, 'a') as f:
            f.write("++++++ Tried %s times. Still no result for:%s:%s(%s:%s:%s) %s"
                    % (str(test_times), os.path.basename(filename),
                       test_description, package_name, app_version,
                       current_target_dev, os.linesep))


@exception_logger(PrintAndLogErr)
def RunAllQS(target_device, last_apk, list_file):
    break_index = 0
    has_breakpoint = True if last_apk else False
    executed_num = 0
    current_index = 0

    reboot_timing = int(ReadXMLConfig('RebootValue')) if ReadXMLConfig('RebootValue').isdigit() else 1

    bad_apk_list = work_folder + "\\DaVinciEmptyPackageName.txt"
    PrintAndLogInfo("\n\n******************** Start to run all of Applications ****************************")
    PrintAndLogInfo("Folder for Applications: '%s'." % apk_folder)

    force_apk_list = ReadXMLConfig("ForceApkList")

    if force_apk_list and os.path.exists(force_apk_list):
        with open(force_apk_list) as f:
            force_online_download_apk_list = [i.strip() for i in f.readlines()]
    else:
        force_online_download_apk_list = []

    Enviroment.remove_item(bad_apk_list)
    # Get test list
    files = PrepareTestList(list_file)
    total_num = len(files)
    PrintAndLogInfo("Total APK number: %s" % (str(total_num)))

    # Run each qs.
    total_index = 0
    tmp_apk = ""
    start = datetime.datetime.now() + datetime.timedelta(hours=reboot_timing)

    priority_items = ReadXMLConfig('RnRPriorityItems').split(',')
    for filename in files:
        try:
            showname = filename.encode(syslocale)
        except:
            showname = filename.encode('utf-8')

        # For each hour, let's reboot all the devices to avoid offline
        start = PeriodicalReboot(reboot_timing, start,  target_device)

        if os.path.splitext(filename)[1] != '.apk':
            continue

        total_index = total_index + 1
        # Will seek to the breakpoint.
        if has_breakpoint and tmp_apk != last_apk:
            tmp_apk = os.path.basename(filename)
            continue
        if has_breakpoint and tmp_apk == last_apk:
            has_breakpoint = False
            break_index = total_index - 1
            current_index = total_index - 1

        # Run qs.
        PrintAndLogInfo("\n**********************************************************************************")
        # Jia Print num of adb process before running DaVinci
        ProcessNumDetect("adb.exe")
        PrintAndLogInfo("[%s]. '%s' ... " % (str(total_index), showname))
        package_name = getApkPackageName(Davinci_folder, apk_folder, filename)

        if not package_name:
            PrintAndLogErr("\n* Failed to get the package name of the apk: %s" % showname)
            with open(bad_apk_list, 'a') as f:
                f.write(filename + os.linesep)
        else:
            # Check the device to avoid the battery issue.
            current_target_dev = ChooseDevice(target_device, 1)
            if current_target_dev == "":
                break

            # Prepare all the information the needed in test execution.
            device_info_dict = GetDeviceInfo(current_target_dev)
            multi_abi_flag, apk_abis, app_version = DecideAPKabi(filename)
            rnr_configinfo_suite_list, description_list, rnr_folder = anyRnRTest(package_name, apk_folder,
                                                                                 rnr_path, priority_items)
            suite_count = len(rnr_configinfo_suite_list)
            is_rnr_test, test_count, seed_value_set = DecideExecutionParameters(suite_count)

            executed_num = executed_num + 1
            current_index = current_index + 1
            rnr_script = ''
            dep_list = []

            for each_round_abi in apk_abis:
                index = 0

                while index < test_count:
                    each_round_seed = seed_value_set[index]
                    if not multi_abi_flag or multi_abi_flag == 'n':
                        test_description = 'Smoke Test [Seed:{}]'.format(each_round_seed)
                    else:
                        test_description = 'Smoke Test [Seed:{} ABI:{}]'.format(each_round_seed, each_round_abi)
                    if is_rnr_test:
                        # If RnR config file is detected, description_list shall not be empty.
                        if description_list:
                            PrintAndLogInfo("\n--------  [%s] ------------" % description_list[index - 1])
                            rnr_script, dep_list = MatchRnR(rnr_configinfo_suite_list[index - 1], rnr_folder,
                                                            app_version, device_info_dict, priority_items)

                            if not os.path.exists(rnr_script) or not os.path.isfile(rnr_script):
                                PrintAndLogInfo("\n* %s doesn't exist. Change to Smoke Test." % rnr_script)
                                rnr_script = ''
                            else:
                                test_description = 'RnR Test' + description_list[index - 1]
                        else:
                            # If no RnR config file detected,
                            # rnr_configinfo_suite_list will return the .xml file path found under rnr folder.
                            rnr_script = rnr_configinfo_suite_list[index - 1]
                            test_description = 'RnR Test'
                    index += 1

                    TestOneAPK(
                        force_online_download_apk_list, filename, rnr_script, dep_list,
                        current_target_dev,
                        package_name,
                        test_description,
                        each_round_seed, each_round_abi, app_version, showname)

            uninstallpackage(package_name, current_target_dev)
            print "\nProgress: %s/%s complete." % (current_index, total_num)

    PrintAndLogInfo("\n\n+++++++++++++++++++++++++++++++ Summary +++++++++++++++++++++++++++++++")
    if has_breakpoint:
        PrintAndLogInfo("'%s' is the last apk. All tests were done." % last_apk)
    else:
        PrintAndLogInfo("Executed:\t" + str(executed_num))
        if total_num != executed_num + break_index:
            PrintAndLogInfo("Remaining:\t" + str(total_num - executed_num - break_index))
        if break_index > 0:
            PrintAndLogInfo("Breakpiont:\t" + str(break_index))
        PrintAndLogInfo("Total:\t\t" + str(total_num))
    return 0


def InstallWatchCat(devices):
    # Set logger.propagate to False since SmartRunner will print all the messages to root handler
    watchcat.module_logger.propagate = False

    # Register a log file handler to the watchcat module
    timestamp = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
    logfile_name = "%s\\watchcat_%s.log" % (result_folder_name, timestamp)
    file_handler = logging.FileHandler(logfile_name)
    file_handler.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    file_handler.setFormatter(formatter)
    watchcat.module_logger.addHandler(file_handler)

    # Watch all the device in the 'devices' list
    map(g_wcm.watch_device, devices)


def CheckPackageInstalled(device_name, package_name):
    cmd = "shell pm list package %s" % package_name
    result = CallADBCmd(cmd, device_name, "")
    installed = False
    for line in result:
        if line.find("package:") > -1:
            installed = True
            break
    return installed


def CheckCompatibilityResult(log_folder):
    for files in os.walk(log_folder):
        for fileName in files:
            if "incompatible.txt" in fileName:
                return False
    return True


def FindDownloadPic(log_folder):
    log_folder += "\\on_device_qscript\\_Logs\\"
    pattern_pic = re.compile(r'downloading\d{8}T\d{6}\.png')
    download_pic = []
    for paths, dirs, files in os.walk(log_folder):
        for file in files:
            if pattern_pic.match(file):
                download_pic.append(paths + "\\" + file)

    return download_pic[len(download_pic) - 1] if download_pic else ''


def RecoverDevice(device_name):
    # If the device is on line, just reboot it.
    # If downloading timeout, sometime Google Play is hung there. Better to recover the device.
    HostRecovery(device_name)

    if ':' in device_name:
        CallADBCmd("connect {}".format(device_name), '', '')

    # Check the device status
    dev_list = []
    status = CallADBCmd("get-state", device_name, "")
    logger = logging.getLogger('debug')
    logger.debug("\nCommand: get-state \nResult: %s" % "".join(status))
    status_info = ("".join(status)).strip()
    # If the device is on line, just reboot it.
    recovered = False
    if status_info.lower() == "device":
        recovered = RebootDevice(device_name)

    if not recovered:
        recovered = RePlugDevice(device_name)
        if not recovered:
            dev_list.append(device_name)
            # Try to wait for self recovery.
            dev_list = WaitForSoftRecovery(dev_list)
            # If self recovery does not work, try to use powerpusher.
            if dev_list:
                # PrintAndLogErr("  - Fail to reboot device %s with adb reboot" % device_name)
                PrintAndLogErr("\n* Try to recovery the device %s by PowerButtonPusher" % device_name)
                recovered = DeviceColdRecovery(device_name)
    return recovered


def WaitForInstallResult(device_name, package_name, test_description, output_fld):
    beginning = datetime.datetime.now()
    download_link = FindDownloadPic(output_fld)
    if download_link == "":
        download_link = output_fld
    download_log = r'%s\on_device_qscript\%s_log.txt' % (output_fld, package_name)
    # The key message means the accept button is clicked and then in downloading.
    key_list = [' - start to click accept button', ' Click point ']
    error_found, key_found = CheckDavinciLog(download_log, key_list, key_match_mode='all')
    is_timeout = False
    installed = False
    is_compatible = True
    if not key_found:
        # Check if it is a incompatible APK.
        is_compatible = CheckCompatibilityResult(output_fld)
        if not is_compatible:
            PrintAndLogInfo("  - Incompatible APK.")
            with open(no_result_app_list, 'a') as f:
                f.write("Download Failed|---|%s|---|%s|---|incompatible|---|%s%s"
                        % (package_name, download_link, test_description, os.linesep))
        else:
            # If the APP is the sys apk and is the latest one, will test it anyway.
            installed = CheckPackageInstalled(device_name, package_name)
            if not installed:
                PrintAndLogInfo("  - Failed to download from Google Play.")
            with open(no_result_app_list, 'a') as f:
                f.write("Download Failed|---|%s|---|%s|---|notclicked|---|%s%s"
                        % (package_name, download_link, test_description, os.linesep))
    else:
        # Wait for the package to be installed.
        ondevice_timeout_str_value = ReadXMLConfig('OnDeviceInstallTimeout')
        timeout = int(ondevice_timeout_str_value) if ondevice_timeout_str_value.isdigit() else 1200
        sleep_time = 10
        start = datetime.datetime.now()
        # wait the installation complete and check it.
        while True:
            installed = CheckPackageInstalled(device_name, package_name)
            now = datetime.datetime.now()
            if installed:
                PrintAndLogInfo("  - On-device download and installed successfully in %s seconds." % str(
                    (now - beginning).seconds))
                break
            if (now - start).seconds < timeout:
                print "    Wait for %s seconds..." % str(sleep_time)
                time.sleep(sleep_time)
            else:
                PrintAndLogInfo(
                    "  - Failed to download from Google Play after trying %s seconds." % str(
                        (now - start).seconds))
                is_timeout = True
                with open(no_result_app_list, 'a') as f:
                    f.write("Download Failed|---|%s|---|%s|---|timeout|---|%s%s"
                            % (package_name, download_link, test_description, os.linesep))
                # Kill google player
                CallADBCmd("shell am force-stop com.android.vending", device_name, "", 30)
                break
    return error_found, key_found, installed, is_timeout, is_compatible

def InstallPackageFromDevice(device_name, package_name, test_description):
    package_list = []
    package_list.append(package_name)
    installed = False
    is_compatible = True
    has_error = False
    try:
        # Call API to download and install from Google play on device.
        if not CheckPackageInstalled(device_name, 'com.android.vending'):
            PrintAndLogInfo("  - No Google Play on device.")
            has_error = True
        else:
            if CheckPackageInstalled(device_name, package_name):
                uninstallpackage(package_name, device_name)
            # save_config('default_davinci_path', Davinci_folder)
            retry_count = 2
            while retry_count > 0:
                retry_count -= 1
                is_started, output_fld = download_apk_on_device(package_list, 600, False, os.getcwd(), device_name)
                if not is_started:
                    # Something wrong with parameter input for download_apk_on_device. Just return.
                    has_error = True
                    break
                else:
                    error_found, key_found, installed, is_timeout, is_compatible = WaitForInstallResult(device_name, package_name,
                                                                                         test_description, output_fld)
                    if installed:
                        try:
                            if output_fld is not None:
                                Enviroment.remove_item(output_fld, False)
                        except Exception, e:
                            PrintAndLogErr("  - Removing folder %s failed. %s " % (output_fld, str(e)))
                            has_error = True
                        break

                    else:
                        # 1. If Davinci quit without clicking "accept" button due to error issue,
                        # 2. Downloading has started, but timeout. For example, properly Google Play is hung.
                        # The device is probably unstable. So recover the device by rebooting it.
                        if (not key_found and not is_compatible and error_found) or (is_timeout and error_found):
                            # If the device is on line, just reboot it.
                            # If downloading timeout, sometime Google Play is hung there. Better to recover the device.
                            RecoverDevice(device_name)
                        if not is_compatible:
                            break

                        if retry_count > 0:
                            PrintAndLogInfo("  - Try to on-device install again ...")
    except:
        s = sys.exc_info()
        PrintAndLogErr("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
        has_error = True
    return installed, is_compatible, has_error


def ReadXMLConfig(config_option):
    if xml_config == "":
        return ""

    if not os.path.exists(xml_config):
        PrintAndLogErr("Couldn't find the config file %s. Please create it first." % xml_config)
        return ""
    else:
        value = "".join(ParseXML(xml_config, config_option))
        # If RnRPriorityItems is not defined, use the default value.
        if config_option == 'RnRPriorityItems' and value == '':
            value = 'AppVer,Resolution,DPI,OSVer,Model'
        return value


@exception_logger(PrintAndLogErr)
def RunOnlineTest():
    res = VerifyDeviceConfig()
    if res == -1:
        return -1

    target_type, target_dev, type_name = res
    # Check for the last qs name and last target device name
    result_line = GetLastLineOfReport(1)

    if result_line:
        last_device_name = result_line.split(',')[1]
        if last_device_name in target_dev:
            PrintAndLogInfo("Device has not changed")
        else:
            PrintAndLogInfo("Device is changed. Last device %s is not connected." % last_device_name)

            dev_name = last_device_name

            if ':' in last_device_name:
                dev_name = last_device_name.replace(':', '_')

            PrintAndLogInfo("Will rename Smoke_Test_Report.csv to Smoke_Test_Report_%s.csv" % dev_name)

            os.rename(work_folder + "\\" + "Smoke_Test_Report.csv",
                      work_folder + "\\Smoke_Test_Report_%s.csv" % dev_name)

    apk_list = []

    breakpoint_index = 0
    has_breakpoint = False
    if ReadXMLConfig("Breakpoint").lower() == "true" and os.path.exists(executed_list):
        # Check for breakpoint
        with open(executed_list, 'r') as f:
            executed = f.readlines()
        for line in executed:
            if line.strip() and not line.startswith(timestamp_tag):
                apk_info = line.split(separator)

                if len(apk_info) == 1:
                    apk_list.append(line.strip(os.linesep))
                else:
                    breakpoint_index += 1
        PrintAndLogInfo("The last test did not finish. Breakpoint is #%s.\n\n" % str(breakpoint_index + 1))
        user_selection = get_user_input("Will you continue from this breakpoint? If no, "
                                        "start a totally new round test(y/n):", 'y')
        if user_selection.lower() != "n":
            has_breakpoint = True
            GetAPKInfo(apk_list)
            # Save timestamp for this execution into executed file
            with open(executed_list, 'a') as f:
                f.write(os.linesep + timestamp_tag + timestamp_nm)
        else:
            Enviroment.remove_item(executed_list, False)
            Enviroment.remove_item(os.path.join(work_folder, "Smoke_Test_Report.csv"))
            breakpoint_index = 0

    if not has_breakpoint:
        package_list = ReadXMLConfig("package_list")
        if package_list:
            if not os.path.exists(package_list):
                PrintAndLogErr("Couldn't find the package list %s." % package_list)
                return -1

            with open(package_list) as f:
                apk_list = [apk for apk in f.readlines() if apk.strip()]
            GetAPKInfo(apk_list)

        else:
            apk_list = GetRuntimeAPKList()

        with open(executed_list, 'w') as f:
            f.writelines(os.linesep.join(apk_list))
            f.write(os.linesep + timestamp_tag + timestamp_nm)

    DownloadAndTest(target_dev, apk_list, breakpoint_index)


@exception_logger(PrintAndLogErr)
def GetAPKInfo(apk_list):
    import socks
    import socket
    import config

    sock = config.default_socks_proxy.split(':')
    socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock[0], int(sock[1]))

    socket.socket = socks.socksocket
    user_define_proxy = {config.default_proxy_type: config.default_socks_proxy}
    print "Getting packages information from Google Play..."
    remote_apk_info = getAppDetailInfo(apk_list, user_define_proxy)
    duplicated_pck = []
    content = 'Package|AppName|Version' + os.linesep
    for apk_info in remote_apk_info:
        if len(apk_info) < 4:
            continue
        package_name = apk_info[0]
        app_version = apk_info[1]
        try:
            app_name = (apk_info[3]).encode("utf-8")
        except:
            app_name = (apk_info[3])
        content += package_name + separator + app_name + separator + app_version + os.linesep
        if package_name in package_info_dict:
            duplicated_pck.append(package_name)
            continue
        app_detail = []

        app_detail.append(app_name)
        app_detail.append(app_version)
        package_info_dict[package_name] = app_detail

    # If it is not the first execution, no need to create such file again.
    if ReadXMLConfig("Breakpoint").lower() != "true" or not os.path.exists(executed_list):
        with open(work_folder + '/apk_version_info.txt', 'w') as f:
            f.writelines(content)


@exception_logger(PrintAndLogErr)
def GetRuntimeAPKList():
    import socks
    import socket
    import config

    sock = config.default_socks_proxy.split(':')
    socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock[0], int(sock[1]))

    socket.socket = socks.socksocket
    user_define_proxy = {config.default_proxy_type: config.default_socks_proxy}

    category_selection = 1
    start_rank = 1
    dl_num = 10
    try:
        category_selection = int(ReadXMLConfig("category_selection"))
        start_rank = int(ReadXMLConfig("start_rank"))
        dl_num = int(ReadXMLConfig("dl_num"))
    except:
        pass
    apk_list = get_apk_list(user_define_proxy, start_rank, start_rank - 1 + dl_num, category_selection)
    remote_apk_info = getAppDetailInfo(apk_list, user_define_proxy)
    content = ['Google' + '|' + str(start_rank) + '|' +
               str(start_rank + dl_num - 1) + '|' + timestamp_nm + os.linesep]

    separator = '|---|'
    i = start_rank - 1
    duplicated_pck = []
    for apk_info in remote_apk_info:
        if len(apk_info) < 4:
            continue
        i += 1
        package_name = apk_info[0]
        app_version = apk_info[1]
        app_name = apk_info[3]
        try:
            if category_selection == 1:
                content.append((str(i) + separator + package_name + separator + 'app' + separator + app_name +
                                separator + app_version + os.linesep).encode('utf-8'))
            elif category_selection == 2:
                content.append((str(i) + separator + package_name + separator + 'game' + separator + app_name +
                                separator + app_version + os.linesep).encode('utf-8'))
            elif category_selection == 3:
                if i <= dl_num:
                    content.append((str(i) + separator + package_name + separator + 'app' + separator + app_name +
                                    separator + app_version + os.linesep).encode('utf-8'))
                else:
                    content.append((str(i - dl_num) + separator + package_name + separator + 'game' + separator +
                                    app_name + separator + app_version + os.linesep).encode('utf-8'))

            with open(work_folder + '/version_info.txt', 'w') as f:
                f.writelines(content)

        except Exception, e:
            print str(e)

        if package_name in package_info_dict:
            duplicated_pck.append(package_name)
            continue
        app_detail = []
        app_detail.append(app_name)
        app_detail.append(app_version)
        package_info_dict[package_name] = app_detail

    tmp_list = list(set(apk_list))
    tmp_list.sort(key=apk_list.index)

    msg = "\n\n%s duplicated APKs:" % str(len(duplicated_pck))
    msg += os.linesep
    msg += "\n".join(duplicated_pck)
    msg += "\n%s APK will be testd." % str(len(tmp_list))
    with open(work_folder + '/version_info.txt', 'a') as f:
        f.writelines(msg)
    PrintAndLogInfo(msg)
    return tmp_list


@exception_logger(PrintAndLogErr)
def MarkExecutedList(package_name, result_value):
    if not os.path.exists(executed_list):
        return
    with open(executed_list, 'r') as f:
        result_list = f.readlines()
    index = -1
    for package_line in result_list:
        index += 1
        pck_result = package_line.strip(os.linesep).split(separator)
        # Result is saved already.
        if len(pck_result) == 2:
            continue

        pck_name = pck_result[0]
        if pck_name != package_name:
            continue
        tmp = result_list[index].strip(os.linesep)
        result_list[index] = tmp + separator + result_value + os.linesep
        break

    Enviroment.remove_item(executed_list, False)
    with open(executed_list, 'w') as f:
        f.writelines("".join(result_list))


@exception_logger(PrintAndLogErr)
def PrepareOndeviceDownloadTest(all_dev):
    InstallWatchCat(all_dev)
    Uninstall3rdApps(all_dev)
    ClearCache(all_dev)

    logging.basicConfig(filename='wifi.log', level=logging.INFO)
    logger = logging.getLogger('wifi')
    Wifi_obj = InstallWifiApk(r'%s\platform-tools\adb.exe' % Davinci_folder, script_path, logger)
    Wifi_obj.InstallWiFiChecker(all_dev)
    # RebootDevices(all_dev)

    pre_install_folder = ReadXMLConfig("PreInstallApksFolder")

    # install and launch apps before testing
    if pre_install_folder and os.path.exists(pre_install_folder):
        logging.basicConfig(filename='pre_install.log', level=logging.INFO)
        logger = logging.getLogger('pre_install')
        pre_install_obj = InstallAndLaunchApk(r'%s\platform-tools\adb.exe' % Davinci_folder,
                                              r'%s\platform-tools\aapt.exe' % Davinci_folder,
                                              pre_install_folder, logger)
        pre_install_obj.InstallApks(all_dev)
        pre_install_obj.LaunchApks(all_dev)
        return pre_install_obj
    else:
        return None


@exception_logger(PrintAndLogErr)
def OndeviceDownload(current_target_dev, package_name, test_description, pack_list):
    res, wifi_screenshot_file = CheckWIFI(current_target_dev)
    ret = ""
    if res == 'false':
        with open(no_result_app_list, 'a') as f:
            f.write("Download Failed|---|%s|---|%s|---|wifiissue|---|%s%s"
                    % (package_name, wifi_screenshot_file, test_description, os.linesep))

            if package_name in package_info_dict:
                f.write("++++++ Network Issue. No execution for:%s:%s(%s:%s:%s) %s"
                        % (package_name, test_description, package_info_dict[package_name][0],
                           package_info_dict[package_name][1], current_target_dev, os.linesep))
            else:
                f.write("++++++ Network Issue. No execution for:%s:%s(%s:%s:%s) %s"
                        % (package_name, test_description, 'NA', 'NA', current_target_dev, os.linesep))
        PrintAndLogErr("  - Failed to run test.")
        ret = 'break'
    else:
        # Download and install on-device from Google Play
        PrintAndLogInfo("\n* Please wait to install from Google Play ...")
        installed, is_compatible, has_error = InstallPackageFromDevice(current_target_dev, package_name,
                                                                       test_description)
        if installed:
            PrintAndLogInfo("  - Installation completed.")

            if not package_name in pack_list:
                failed_list = []
                tmp_list = []
                tmp_list.append(package_name)
                fetch_apk(tmp_list, work_folder, work_folder, 2, current_target_dev, failed_list)
                if len(failed_list) == 0:
                    pack_list.append(package_name)
                    ret = "fetched"
                else:
                    ret = "installed"
        else:
            if not is_compatible:
                PrintAndLogErr("  - Incompatible APK.")
                ret = "break"
            else:
                if has_error:
                    PrintAndLogErr("  - Error during on-device download.")
                else:
                    PrintAndLogErr("  - Failed to install.")
                ret = "continue"
    return ret


@exception_logger(PrintAndLogErr)
def ExecuteTest(current_target_dev, index, apk_name, package_name, rnr_test, rnr_configinfo_suite_list,
                rnr_folder, priority_items, description_surfix):
    device_info_dict = GetDeviceInfo(current_target_dev)
    adb_helper = adbhelper.AdbHelper(r'%s\platform-tools\adb.exe' % Davinci_folder)
    app_version = adb_helper.GetPackageVer(current_target_dev, package_name)

    # Find the scheduled rnr script
    rnr_script = ''
    dep_list = []
    test_description = 'Smoke Test'
    if rnr_test:
        if description_surfix != '':
            # Find the scheduled rnr script
            rnr_script, dep_list = MatchRnR(rnr_configinfo_suite_list[index - 1], rnr_folder,
                                            app_version, device_info_dict, priority_items)
            if not os.path.exists(rnr_script) or not os.path.isfile(rnr_script):
                PrintAndLogInfo("\n* %s doesn't exist. Change to Smoke Test." % rnr_script)
                rnr_script = ''
            else:
                test_description = 'RnR Test' + description_surfix
        else:
            # If no RnR config file detected
            # rnr_configinfo_suite_list will return the .xml file path found under rnr folder.
            rnr_script = rnr_configinfo_suite_list[index - 1]
            test_description = 'RnR Test'
    # Test it.
    PrintAndLogInfo("\n  Please wait for testing ...")
    ####################################################################################################
    # DaVinci Test Results return code:
    # pass (0), fail (1), warning (2), skip (-1), no result (-2), Timeout (-3),
    # ADB Hung or environment error (-4), device abnormal cause test skipped (-5)
    ####################################################################################################
    result = "FAIL"
    res = RunDavinci(current_target_dev, apk_name, rnr_script, dep_list, test_description, 1, 'java',
                     'no', 'yes', True)

    return test_description, result, res


@exception_logger(PrintAndLogErr)
def RetryTest(executed_times, test_times, pass_number, pass_criteria,
              current_target_dev, apk_name, rnr_script, dep_list, test_description):
    no_result_reported = True
    while executed_times < test_times:
        executed_times += 1
        PrintAndLogInfo("\n* Start Round %s Testing ..." % str(executed_times))
        # PrintAndLogInfo("  [Testing Application]:\t%s" % (package_name))
        # PrintAndLogInfo("  [Testing device]:\t\t%s" % (current_target_dev))
        res = RunDavinci(current_target_dev, apk_name, rnr_script, dep_list, test_description, 1,
                         'java', 'yes', 'yes', False)
        # Track the Davinci test has come out with test result in the report file.
        if res == 0 or res == 1 or res == 2 or res == -1:
            no_result_reported = False

        if res == 0:
            pass_number += 1
            if pass_number == pass_criteria:
                break

        # If remaining test rounds cannot meet pass criteria, skip
        if test_times > executed_times and test_times - executed_times + pass_number < pass_criteria:
            break
    return no_result_reported, executed_times, pass_number


@exception_logger(PrintAndLogErr)
def DownloadAndTestOneAPK(target_device, package_name, priority_items):
    apk_name = ""
    global apk_folder
    apk_folder = ""

    rnr_configinfo_suite_list, description_list, rnr_folder = anyRnRTest(package_name, apk_folder, rnr_path,
                                                                         priority_items)

    test_count = len(rnr_configinfo_suite_list)
    rnr_script = ''
    dep_list = []
    rnr_test = False
    if test_count > 0:
        PrintAndLogInfo("\n* Totally %s RnR Tests Found." % str(test_count))
        rnr_test = True
    else:
        PrintAndLogInfo("\n* Will launch Smoke Test.")
        test_count = 1
    index = 0
    is_tested = False
    current_target_dev = ""
    no_result_reported = True
    executed_times = 0
    pass_number = 0
    test_description = ''
    result = ''
    execution_criteria = ReadXMLConfig('ExecutionCriteria')
    while index < test_count:
        index += 1
        no_result_reported = True
        test_round = 0
        executed_times = 0
        pass_number = 0
        test_description = 'Smoke Test'
        # Find the scheduled rnr script
        if rnr_test:
            if description_list:
                PrintAndLogInfo("\n--------  [%s] ------------" % description_list[index - 1])

        pack_list = []
        while test_round < test_times:
            test_round += 1
            # Check the device to avoid the battery issue.
            current_target_dev = ChooseDevice(target_device, 1)
            if current_target_dev == "":
                break

            PrintAndLogInfo("\n* Start Round %s Testing ..." % str(executed_times + 1))
            ret = OndeviceDownload(current_target_dev, package_name,
                                   test_description, pack_list
                                   )
            if ret == 'break':
                break
            if ret == 'continue':
                continue
            if ret == "fetched":
                apk_name = r'%s.apk' % package_name
                apk_folder = work_folder
            if ret == 'installed':
                apk_name = package_name

            executed_times += 1

            description_surfix = ''
            if description_list:
                description_surfix = description_list[index - 1]
            test_description, result, res = ExecuteTest(current_target_dev, index, apk_name, package_name, rnr_test,
                                                        rnr_configinfo_suite_list, rnr_folder, priority_items,
                                                        description_surfix)

            is_tested = True
            # Track the Davinci test has come out with test result in the report file.
            result = "FAIL"
            if res == 0 or res == 1 or res == 2 or res == -1:
                no_result_reported = False

            if res == -1:
                result = "SKIP"
                break
            if res == 0:
                pass_number += 1
                if test_round == 1 and execution_criteria == '1':
                    result = "PASS"
                    break
                if pass_number == pass_criteria:
                    result = "PASS"
                    break

            # If remaining test rounds cannot meet pass criteria, skip
            if (test_times - test_round > 0) and (test_times - test_round + pass_number < pass_criteria):
                break

        retrial_chance = test_times - executed_times

        if result == "FAIL" and os.path.splitext(apk_name)[1] == ".apk" and (
                        pass_number + retrial_chance >= pass_criteria):
            PrintAndLogInfo(
                "\n* Installation Failed %s times. Will test with pulled APK..." % (test_round - executed_times))

            no_result_reported, executed_times, pass_number = RetryTest(executed_times, test_times, pass_number,
                                                                        pass_criteria, current_target_dev, apk_name,
                                                                        rnr_script, dep_list, test_description)

    return current_target_dev, result, is_tested, no_result_reported, executed_times, pass_number, test_description


@exception_logger(PrintAndLogErr)
def DownloadAndTest(target_device, package_list, breakpoint_index):
    current_index = breakpoint_index

    reboot_timing = int(ReadXMLConfig('RebootValue')) if ReadXMLConfig('RebootValue').isdigit() else 1
    all_dev =  target_device
    PrintAndLogInfo("\n\nPreparing for the test ...")
    pre_install_obj = PrepareOndeviceDownloadTest(all_dev)

    # By default, each APK will be tested 3 times.
    package_list = [i.strip() for i in package_list if i.strip()]
    total_num = len(package_list) + breakpoint_index

    test_times = int(ReadXMLConfig('TestNumber')) if ReadXMLConfig('TestNumber').isdigit() else 3

    PrintAndLogInfo("\n\nStart to run all of Applications ...")
    executed_num = 0
    start = datetime.datetime.now()
    priority_items = ReadXMLConfig('RnRPriorityItems').split(',')

    for package_name in package_list:
        # If we will reboot after each APK test, to save time, skip periodically reboot.
        if ReadXMLConfig('RebootForEachAPK').lower != 'true':
            # For each hour, let's reboot all the devices to avoid offline
            now = datetime.datetime.now()
            if now - start > datetime.timedelta(hours=reboot_timing):
                start = datetime.datetime.now()
                PrintAndLogInfo("\n* It's time to reboot all devices to avoid device offline ...")
                ClearCache(all_dev)
                RebootDevices(all_dev)

                # after rebooting device, need to launch apps again
                if not pre_install_obj is None:
                    pre_install_obj.LaunchApks(all_dev)

        current_index += 1
        PrintAndLogInfo("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        PrintAndLogInfo("[%s]. '%s' ... " % (str(current_index), package_name))

        # Jia Print num of adb process before running DaVinci
        ProcessNumDetect("adb.exe")

        current_target_dev, result, is_tested, no_result_reported, executed_times, pass_number, test_description = DownloadAndTestOneAPK(target_device, package_name, priority_items)
        execution_criteria = ReadXMLConfig('ExecutionCriteria')
        # Only when execution_criteria == 1 and executed_times > 1, or execution_criteria == 2, pass criteria is considered.
        if result == "PASS" and ((execution_criteria == 1 and executed_times > 1) or (execution_criteria == 2)):
            PrintAndLogInfo(
                "\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  - Total rounds(s): %s\n  - Executed round(s):%s\n  - Passed: %s\n" % (
                    package_name, result, pass_criteria, test_times, executed_times, pass_number))
        if result == "FAIL":
            PrintAndLogInfo("\n* Test Result for %s : %s\n  - Pass Criteria: Passed >= %s\n  - Total rounds(s):"
                            " %s\n  - Executed round(s):%s\n  - Passed: %s\n" % (package_name, result,
                                                                                 pass_criteria, test_times,
                                                                                 executed_times, pass_number))

        if no_result_reported:
            with open(no_result_app_list, 'a') as f:
                if package_name in package_info_dict:
                    f.write("++++++ Tried %s times. Still no result for :%s:%s(%s:%s:%s) %s" %
                            (str(executed_times), package_name, test_description,
                             package_info_dict[package_name][0], package_info_dict[package_name][1],
                             current_target_dev, os.linesep))
                else:
                    f.write("++++++ Tried %s times. Still no result for :%s:%s(%s:%s:%s) %s" %
                            (str(executed_times), package_name, test_description, 'NA', 'NA',
                             current_target_dev, os.linesep))
        if is_tested:
            executed_num += 1
        if ReadXMLConfig('RebootForEachAPK').lower() == 'true':
            RebootDevice(current_target_dev)

        print "\nProgress: %s/%s complete." % (current_index, total_num)
        MarkExecutedList(package_name, result)

        uninstallpackage(package_name, current_target_dev)

    PrintAndLogInfo("\n\n+++++++++++++++++++++++++++++++ Summary +++++++++++++++++++++++++++++++")
    PrintAndLogInfo("Executed:\t" + str(executed_num))
    PrintAndLogInfo("Breakpoint:\t" + str(breakpoint_index))
    if total_num != executed_num + breakpoint_index:
        PrintAndLogInfo("Install Failed:\t" + str(total_num - executed_num - breakpoint_index))
    PrintAndLogInfo("Total:\t\t" + str(total_num))
    # Clean up watch cat process
    CleanupToQuit()
    return 0


def ClearCache(all_dev):
    for dev in all_dev:
        output = os.popen(
            Davinci_folder + "\\platform-tools\\adb.exe -s %s shell pm list packages -s" % dev).readlines()
        DebugLog(Davinci_folder + "\\platform-tools\\adb.exe -s %s shell pm list packages -s" % dev + os.linesep)
        DebugLog("\n".join(output))
        for package_info in output:
            if package_info.startswith("package:"):
                package = package_info.split(":")[1].strip()
                if package == "com.android.providers.downloads":
                    CallADBCmd("shell pm clear %s" % package, dev, "", 15)
                    break


def Uninstall3rdApps(all_dev):
    for dev in all_dev:
        res = CallADBCmd("shell pm list packages -3", dev, "")
        each_dev_3rd_app_list = [i.strip().split('package:')[1] for i in res if len(i.strip().split('package:')) >= 2]
        if "com.example.qagent" in each_dev_3rd_app_list:
            each_dev_3rd_app_list.remove("com.example.qagent")

        if "com.google.android.inputmethod.latin" in each_dev_3rd_app_list:
            each_dev_3rd_app_list.remove("com.google.android.inputmethod.latin")

        if ':' in dev and 'com.google.android.play.games' in each_dev_3rd_app_list:
            each_dev_3rd_app_list.remove('com.google.android.play.games')

        for pkg_name in each_dev_3rd_app_list:
            CallADBCmd("uninstall {0}".format(pkg_name), dev, "")


@exception_logger(PrintAndLogErr)
def CheckDavinciLog(log_file, key_list, key_match_mode='any'):
    if not os.path.exists(log_file):
        PrintAndLogErr("%s does not exist." % log_file)
        return None, None
    with open(log_file, 'r') as f:
        lines = f.readlines()
    line_index = 0
    error_found = False
    error_body = False
    key_found = False
    special_error = False
    key_match = key_list
    for line in lines:
        line_index += 1
        line_info = line.strip().strip(os.linesep)
        for key_str in key_list:
            if line.find(key_str) > -1:
                key_found = True
                del key_match[key_list.index(key_str)]

        pattern = re.compile(r"^\[.*\]\[(.*)\]")
        m = pattern.match(line_info)
        pattern2 = re.compile(r"^\[(.*)\] ")
        m2 = pattern2.match(line_info)
        if (m and (m.group(1) == "error" or m.group(1) == "fatal")) or \
                (m2 and (m2.group(1) == "error" or m2.group(1) == "fatal")):
            if line_info.find("Comparing Q script failure due to missing resources!") > -1:
                continue
            if line_info.find(r'Diagnostic: ') > -1:
                continue
            if line_info.find(r' Firmware - Unknown argument:') > -1:
                continue
            if line_info.find(r'The script exited unexpectedly at IP: ') > -1:
                special_error = True
            error_found = True
            error_body = True
            ErrorLog(log_file + ": Line " + str(line_index))
            ErrorLog(line_info)
            continue
        # check if it is the next log line, do not save as error body.
        if (m or m2) and error_body:
            error_body = False
        # Save the following lines for error details.
        if error_body:
            ErrorLog(line_info)
            # If go to next message...

    if key_match_mode == 'all':
        if len(key_match) == 0:
            key_found = True
        else:
            key_found = False

    # For special error, when Agent is OK, this error means smoke test quit since test result is fail.
    # So just ignore this error.
    if special_error and not key_found:
        error_found = False

    return error_found, key_found


if __name__ == '__main__':
    try:
        from optparse import OptionParser
        # Is_PowerPusher: 0--no power pusher; 1--one power pusher; 2--more than 2 devices on COM port.
        Is_PowerPusher = 0
        script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
        config_file_name = script_path + "\\default_device_cfg.txt"
        usage = "usage: %prog [options] %DavinciFolder%. \nPlease use %prog -h for more details."
        version = "Copyright (c) 2014 Intel Corporation. All Rights Reserved."
        parser = OptionParser(usage=usage, version=version)
        parser.add_option("-g", "--generate_config", action="store_true", dest="to_gen_config", default=False,
                          help="To generate device config file for the current connected devices.")
        parser.add_option("-r", "--run_qs", action="store_true", dest="to_run_qs", default=False,
                          help="To run qs.")
        parser.add_option("-c", "--config_file", dest="config_file", default=config_file_name,
                          help="To specify the config file when running. '%s' will be used by default if no such option." % (
                          config_file_name))
        parser.add_option("-p", "--path", dest="apk_folder", default="",
                          help="To input the folder path for the Applications.")
        parser.add_option("-d", "--debug", action="store_true", dest="is_debug", default=False,
                          help="Flag for debug. With this flag, if DaVinci crash, the tests will terminate.")
        parser.add_option("-x", "--xml", dest="xml_file", default="",
                          help="To input the Treadmill xml config file for silence mode.")

        parser.add_option("-w", "--workspace", dest="work_folder", default="",
                          help="To input the path of workspace.")

        (options, args) = parser.parse_args()
        if len(args) != 1:
            print "Please input Davinci's folder. \nPlease use -h for more details."
            sys.exit(-1)

        Davinci_folder = args[0]
        if not os.path.exists(Davinci_folder + "\\DaVinci.exe"):
            if Davinci_folder != 'False':
                print "'%s\\DaVinci.exe' does not exist." % Davinci_folder
                sys.exit(-1)

        # To save config from the every beginning. This config is used by download module
        save_config('default_davinci_path', Davinci_folder)
        if not options.to_gen_config and not options.to_run_qs:
            print "Please select one of -g and -r. \nPlease use -h for more details."
            sys.exit(-1)

        if options.to_gen_config and options.to_run_qs:
            print "Please select one of -g and -r. \nPlease use -h for more details."
            sys.exit(-1)

        Is_Debug = options.is_debug

        apk_folder = ""
        work_folder = ""
        list_file = ""
        rnr_path = ""

        logCfgFile = script_path + "\\logconfig.txt"
        if not os.path.exists(logCfgFile):
            print "%s Log Config File does not exist. Please reinstall to recover it." % logCfgFile
            sys.exit(-1)

        # Prepare the loggers.
        result_folder_name = script_path + "\\result"
        if not os.path.exists(result_folder_name):
            os.mkdir(result_folder_name)

        timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
        timestamp_nm = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
        template_file = open(logCfgFile, 'r')
        newLogCfgFile = "%s\\logconfig_new_%s.txt" % (script_path, timestamp_nm)
        new_file = open(newLogCfgFile, 'w')
        execution_log = result_folder_name + "\\result_%s.log" % timestamp_nm
        for line in template_file:
            if line.find("'result.log'") > -1:
                line = line.replace("'result.log'", "r'%s'" % execution_log)
            if line.find("'debug.log'") > -1:
                line = line.replace("'debug.log'", "r'" + result_folder_name + "\\debug_%s.log'" % timestamp_nm)
            if line.find("'performance.log'") > -1:
                line = line.replace("'performance.log'",
                                    "r'" + result_folder_name + "\\performance_%s.log'" % timestamp_nm)
            if line.find("'error.log'") > -1:
                line = line.replace("'error.log'", "r'" + result_folder_name + "\\error_%s.log'" % timestamp_nm)
            new_file.write(line)
        template_file.close()
        new_file.close()

        logging.config.fileConfig(newLogCfgFile)
        Enviroment.remove_item(newLogCfgFile, False)
        camera_mode = ""
        xml_config = options.xml_file
        # Read all options from treadmill config file.
        if options.xml_file:
            if not os.path.exists(options.xml_file):
                PrintAndLogErr("Couldn't find the config file %s , please create it first." % options.xml_file)
                sys.exit(-1)
            else:
                camera_mode = "".join(ParseXML(options.xml_file, 'CameraMode'))
                list_file = "".join(ParseXML(options.xml_file, 'ApkList'))
                rnr_path = "".join(ParseXML(options.xml_file, 'RnRfolder'))
                if Davinci_folder == 'False':
                    Davinci_folder = "".join(ParseXML(options.xml_file, 'DaVinciPath'))
                    if not os.path.exists(Davinci_folder + "\\DaVinci.exe"):
                        PrintAndLogErr("'%s\\DaVinci.exe' does not exist." % Davinci_folder)
                        sys.exit(-1)
        else:
            # If no Treadmill config file, read the options.
            # is_agressive = options.is_agressive
            print "'Please input SmartRunner.config file."
            sys.exit(-1)


        if options.to_gen_config:
            config_file_name = options.config_file

            res = CreateNewCfg(camera_mode)
            sys.exit(res)

        # WatchCat Manager Initialization
        g_wcm = watchcat.WatchCatManager({"arm": "watchcat_arm", "x86": "watchcat_x86"},
                                         os.path.join(Davinci_folder, "platform-tools", "adb.exe"))

        if hasattr(os.sys, 'winver'):
            signal.signal(signal.SIGBREAK, signal_handler)
        else:
            signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGINT, signal_handler)

        if options.to_run_qs:
            if options.apk_folder == "":
                print "Please input folder path for APK."
                sys.exit(-1)
            else:
                if ReadXMLConfig('APK_download_mode') != "on-device":
                    if not os.path.exists(options.apk_folder):
                        PrintAndLogErr("Path '%s' does not exist." % options.apk_folder)
                        sys.exit(-1)
                    apk_folder = os.path.abspath(options.apk_folder)
                else:
                    # If it is on-device download, global apk_folder shall be "".
                    # And work_folder shall be set as what user input.
                    if not os.path.exists(options.apk_folder):
                        os.mkdir(options.apk_folder)

                str_test_times = ReadXMLConfig('TestNumber')
                try:
                    test_times = int(str_test_times)
                    if test_times < 2:
                        PrintAndLogInfo('TestNumber shall >= 2, since if failed at 1st round, the rerun is required.')
                        sys.exit(-1)
                except Exception, e:
                    PrintAndLogInfo('Invalid TestNumber %s in config file' % str_test_times)
                    sys.exit(-1)

                str_pass_criteria = ReadXMLConfig('PassCriteria')
                try:
                    pass_criteria = int(str_pass_criteria)
                    if pass_criteria < 1 or pass_criteria > test_times:
                        PrintAndLogInfo('Invalid PassCriteria %s in config file' % str_pass_criteria)
                        sys.exit(-1)
                except Exception, e:
                    PrintAndLogInfo('Invalid PassCriteria %s in config file' % str_pass_criteria)
                    sys.exit(-1)

            work_folder = os.path.abspath(options.work_folder) if options.work_folder else os.path.abspath(options.apk_folder)
            # The priority of command options for list_file and error_debug is higher than the Treadmill config.


            if not os.path.exists(options.config_file):
                PrintAndLogErr("Please specify a device config file. You can generate it by run_qs.py -g option.")
                sys.exit(-1)

            print "Config file '%s' used." % options.config_file
            config_file_name = options.config_file

            # update DaVinci_Applist.xml under DaVinci folder
            CompareAndMergeApplistXML(script_path, Davinci_folder)

            syslocale = locale.getdefaultlocale()[1]
            logging.info("\n\n\n\n################################ New Test %s "
                         "#####################################\n\n\n" % timestamp)

            # If we have a "CloudMode" in Treadmill.config which value is "True", we will start tm_utility thread.
            cloud_mode = ReadXMLConfig('CloudMode')
            if cloud_mode == "True":
                import tm_utility

                tm_utility.initial_communication()

            package_info_dict = {}
            no_result_app_list = r"%s\no_result_app_list_%s.txt" % (work_folder, timestamp_nm)
            executed_list = r"%s\executed_list.txt" % work_folder
            separator = '|---|'
            timestamp_tag = "Executed Timestamp:"
            resultpath = "%s\\TestResult_%s" % (work_folder, timestamp_nm)
            os.mkdir(resultpath)
            debug_resultpath = r'%s\debug' % resultpath
            os.mkdir(debug_resultpath)

            if ReadXMLConfig('APK_download_mode') == "on-device":
                RunOnlineTest()
            else:
                RunTestAll(list_file)

            if cloud_mode == "True":
                time.sleep(1.2)
                tm_utility.stop_network()

            no_result_file_list = []
            # copy result file to test result folder
            if os.path.exists(executed_list):
                with open(executed_list, 'r') as f:
                    contents = f.readlines()
                for line in contents:
                    if line.startswith(timestamp_tag):
                        tmp = line.split(":")[1].strip().strip(os.linesep)
                        previous_execute_log = result_folder_name + "\\result_%s.log" % tmp
                        if os.path.exists(previous_execute_log):
                            shutil.copyfile(previous_execute_log,
                                            resultpath + "\\" + os.path.basename(previous_execute_log))
                        previous_err_log = result_folder_name + "\\error_%s.log" % tmp
                        if os.path.exists(previous_err_log):
                            shutil.copyfile(previous_err_log,
                                            debug_resultpath + "\\" + os.path.basename(previous_err_log))
                        previous_debug_log = result_folder_name + "\\debug_%s.log" % tmp
                        if os.path.exists(previous_debug_log):
                            shutil.copyfile(previous_debug_log,
                                            debug_resultpath + "\\" + os.path.basename(previous_debug_log))
                        previous_no_result_app_list = r"%s\no_result_app_list_%s.txt" % (work_folder, tmp)
                        if os.path.exists(previous_no_result_app_list):
                            shutil.move(previous_no_result_app_list,
                                        debug_resultpath + "\\" + os.path.basename(previous_no_result_app_list))
                            no_result_file_list.append(
                                debug_resultpath + "\\" + os.path.basename(previous_no_result_app_list))
                shutil.move(executed_list, resultpath + "\\" + os.path.basename(executed_list))
            else:
                previous_execute_log = result_folder_name + "\\result_%s.log" % timestamp_nm
                if os.path.exists(previous_execute_log):
                    shutil.copyfile(previous_execute_log, resultpath + "\\" + os.path.basename(previous_execute_log))
                previous_err_log = result_folder_name + "\\error_%s.log" % timestamp_nm
                if os.path.exists(previous_err_log):
                    shutil.copyfile(previous_err_log, debug_resultpath + "\\" + os.path.basename(previous_err_log))
                previous_debug_log = result_folder_name + "\\debug_%s.log" % timestamp_nm
                if os.path.exists(previous_debug_log):
                    shutil.copyfile(previous_debug_log, debug_resultpath + "\\" + os.path.basename(previous_debug_log))
                previous_no_result_app_list = r"%s\no_result_app_list_%s.txt" % (work_folder, timestamp_nm)
                if os.path.exists(previous_no_result_app_list):
                    shutil.move(previous_no_result_app_list,
                                debug_resultpath + "\\" + os.path.basename(previous_no_result_app_list))
                    no_result_file_list.append(debug_resultpath + "\\" + os.path.basename(previous_no_result_app_list))

            Generate_Summary_Report(work_folder, Davinci_folder, config_file_name, xml_config, no_result_file_list,
                                    timestamp)
            Enviroment.remove_item(os.path.join(work_folder, "nonasciiapk"))
            # Must make sure these file be moved to test result folder. If not, show warning.
            report_file_name_list = ['Smoke_Test_Report.csv', 'Smoke_Test_Summary.csv', 'Smoke_Test_Summary.xlsx']
            for report in report_file_name_list:
                report_path = r'%s\%s' % (work_folder, report)
                if os.path.exists(report_path):
                    try:
                        shutil.move(report_path, resultpath + "\\" + report)
                    except Exception, e:
                        s = sys.exc_info()
                        DebugLog("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
                        shutil.copy(report_path, resultpath + "\\" + report)
                        PrintAndLogInfo("Please delete the file %s manually." % report_path)

            for resultfile in os.listdir(work_folder):
                if os.path.splitext(resultfile)[1].strip() in ['.png', '.avi', '.qts']:
                    try:
                        shutil.move(work_folder + "\\" + resultfile, resultpath)
                    except Exception, e:
                        s = sys.exc_info()
                        DebugLog("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
                        shutil.copy(work_folder + "\\" + resultfile, resultpath + "\\" + resultfile)

            if os.path.exists(execution_log):
                shutil.copyfile(execution_log, resultpath + "\\" + os.path.basename(execution_log))

            rnr_log_folder = work_folder + "\\_REPLAY_%s" % timestamp_nm
            if os.path.exists(rnr_log_folder):
                shutil.move(rnr_log_folder, debug_resultpath)
            PrintAndLogErr("Finish all the test. Please find the test result under %s" % resultpath)

    except Exception as e:
        info = sys.exc_info()
        for file, lineno, function, text in traceback.extract_tb(info[2]):
            PrintAndLogErr('File:"{}", in {}, in {}, {}'.format(file, lineno, function, text))
        print str(e)
        sys.exit(-1)

else: # pragma: no cover
    print '****Welcome to test this script ****'
    print "Initialize the global variables for unittest"

    from SmartRunnerUnittestForRunQS import Globals
    Davinci_folder = Globals.Davinci_folder
    script_path = Globals.script_path
    apk_folder = Globals.apk_folder
    work_folder = Globals.work_folder
    xml_config = Globals.xml_config
    config_file_name = Globals.config_file_name
    logCfgFile = script_path + "\\logconfig.txt"
    camera_mode = "".join(ParseXML(xml_config, 'CameraMode'))
    Is_Debug = False
    # Prepare the loggers.
    result_folder_name = script_path + "\\result"
    if not os.path.exists(result_folder_name):
        os.mkdir(result_folder_name)
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
    timestamp_nm = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
    resultpath = "%s\\TestResult_%s" % (work_folder, timestamp_nm)
    os.mkdir(resultpath)
    template_file = open(logCfgFile, 'r')
    newLogCfgFile = "%s\\logconfig_new_%s.txt" % (script_path, timestamp_nm)
    new_file = open(newLogCfgFile, 'w')
    execution_log = result_folder_name + "\\result_%s.log" % timestamp_nm
    for line in template_file:
        if line.find("'result.log'") > -1:
            line = line.replace("'result.log'", "r'%s'" % execution_log)
        if line.find("'debug.log'") > -1:
            line = line.replace("'debug.log'", "r'" + result_folder_name + "\\debug_%s.log'" % timestamp_nm)
        if line.find("'performance.log'") > -1:
            line = line.replace("'performance.log'", "r'" + result_folder_name + "\\performance_%s.log'" % timestamp_nm)
        if line.find("'error.log'") > -1:
            line = line.replace("'error.log'", "r'" + result_folder_name + "\\error_%s.log'" % timestamp_nm)
        new_file.write(line)
    template_file.close()
    new_file.close()

    logging.config.fileConfig(newLogCfgFile)
    Enviroment.remove_item(newLogCfgFile, False)

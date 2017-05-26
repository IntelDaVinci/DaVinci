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
import os
import sys
import time
import ctypes
import platform
import xml.etree.ElementTree as ET
import socket
import xlsxwriter
import re
import adbhelper
import dvcsubprocess
import serial.tools.list_ports
import win32com.client
import shutil
from xml.etree.ElementTree import Element


class StorageInfo(object):
    def  __init__(self, adb_path):
        self.adb_path = adb_path
        self.adb_hp = adbhelper.AdbHelper(self.adb_path)


    def CheckFreeStorage(self, device_id):
        data_partition = []

        # check free storage on /data partition
        self.adb_hp.device_id =device_id
        data_partition = self.adb_hp.adb_execution("shell df /data", 120)
        data_partition = data_partition[0].split("\r\r\n")
        free_storage_val = ''
        if len(data_partition) > 1:
            free_storage_index = data_partition[0].split().index("Free")
            free_storage_val = data_partition[1].split()[free_storage_index]

        # check free RAM status
        result = self.adb_hp.adb_execution("shell cat /proc/meminfo", 120)
        searchObj = re.search(r'(MemFree:)\s+(\d+ \w+)', result[0])
        free_ram_val = ''
        if searchObj:
            free_ram_val = searchObj.group(2)

        return free_storage_val, free_ram_val

class InstallWifiApk(object):
    def __init__(self, adb_path, DaVinci_script_path, logger):
        self.adb_path = adb_path
        self.adb_hp = adbhelper.AdbHelper(self.adb_path)
        self.DaVinci_script_path = DaVinci_script_path
        self.logger = logger

    def InstallWiFiChecker(self, devices):
        WiFiChecker_apk = self.DaVinci_script_path + r'\mini_wifichecker.apk'
        if not os.path.exists(WiFiChecker_apk):
            msg = "WiFiChecker_apk does not exist."
            print msg
            self.logger.info(msg)

        for each_device in devices:
            self.adb_hp.device_id = each_device
            self.adb_hp.adb_execution(r'install %s' % WiFiChecker_apk, 120)


class InstallAndLaunchApk(object):
    def __init__(self, adb_path, aapt_path, apk_folder, logger):
        self.adb_path = adb_path
        self.aapt_path = aapt_path
        self.adb_hp = adbhelper.AdbHelper(self.adb_path)
        self.aapt_hp = AAPTHelper(self.aapt_path)
        self.apk_folder = apk_folder
        self.logger = logger

    def InstallApks(self, devices):
        if not os.path.exists(self.apk_folder):
            print "Error: %s does not exist." % self.apk_folder
            return
        for file_name in os.listdir(self.apk_folder):
            if file_name.endswith(".apk"):
                apk_path = '%s\%s' % (self.apk_folder, os.path.basename(file_name))
                for each_device in devices:
                    self.adb_hp.device_id = each_device
                    self.adb_hp.adb_execution(r'install %s' % apk_path, 200)

    def LaunchApks(self, devices):
        if not os.path.exists(self.apk_folder):
            print "Error: %s does not exist." % self.apk_folder
            return
        for file_name in os.listdir(self.apk_folder):
            if file_name.endswith(".apk"):
                apk_path = '%s\%s' % (self.apk_folder, os.path.basename(file_name))
                pkg_name, _, _ = self.aapt_hp.get_package_version_apk_name(apk_path)
                activity_name = self.aapt_hp.get_package_launchable_activity(apk_path)
                if pkg_name!="" and activity_name!="":
                    for each_device in devices:
                        self.adb_hp.device_id = each_device
                        self.adb_hp.launch_apk(pkg_name, activity_name)
                else:
                    print "Error: package name or activity name is empty"


class RnRConfigInfo(object):
    def __init__(self, rnr_config_path):
        self.package_name = os.path.basename(os.path.dirname(rnr_config_path))
        self.rnr_config_path = rnr_config_path

    def GetRnRConfigsInfo(self, config_items):
        try:
            config_info_list = []
            description_list = []
            if os.path.exists(self.rnr_config_path):
                try:
                    root = ET.parse(self.rnr_config_path)
                except Exception, e:
                    s = sys.exc_info()
                    print "Error in %s: %s" % (self.rnr_config_path, s[1])
                    return config_info_list, description_list
                                               
                rnr_nodes = root.findall('Test')
                count = 0
                for rnr_node in rnr_nodes:
                    count += 1
                    description = 'Test %s' % str(count)
                    if rnr_node.attrib.has_key('description'):
                        description_defined = rnr_node.attrib['description']
                        # : and (, ) is used in no_result_xxx.log file as special delims char.
                        for ch in [":", "(", ")"]:
                            description_defined = description_defined.replace(ch, " ")
                        description = '%s - %s' % (description, description_defined)
                    description_list.append(description)
                    nodes = rnr_node.findall('XMLFile')
                    configs_info = []
                    for node in nodes:
                        path = ''
                        config_dic = {}
                        if node.attrib.has_key('path'):
                            path = node.attrib['path']
                        if path == '':
                            continue
                        config_dic['path'] = path
                        all_dependencies = [i.text.strip() for i in node.findall('Dependency') if i is not None and i.text.strip()]
                        if all_dependencies:
                            config_dic['dependencies'] = ','.join(all_dependencies)
                            
                        for config_item in config_items:
                            value = node.find(config_item)
                            if value is None:
                                if config_item.lower() == 'resolution':
                                    config_dic[config_item] = '1200x1920'
                                elif config_item.lower() == 'dpi':
                                    config_dic[config_item] = '320'
                                else:
                                    config_dic[config_item] = 'NA'
                            else:
                                config_dic[config_item] = value.text
                        # Set the default value for resolution and DPI
                        if not config_dic.has_key('Resolution'):
                            config_dic['Resolution'] = '1200x1920'
                        if not config_dic.has_key('DPI'):
                            config_dic['DPI'] = '320'
                        configs_info.append(config_dic)
                    config_info_list.append(configs_info)
        
        except Exception, e:
            s = sys.exc_info()
            print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
            sys.exit(-1)    
        return config_info_list, description_list


class DeviceInfo(object):
    def __init__(self, adb_path, device_name, xml_config):
        self.adb_path = adb_path
        self.device_name = device_name
        self.xml_config = xml_config
        
    def GetDevInfoFromConfig(self, info_strs):
        try:
            parameterlist = []
            root = ET.parse(self.xml_config)
            nodes = root.findall('Target')
            values = []
            info_list = info_strs.split(',')
            d = dict.fromkeys(info_list, 0)
            for node in nodes:
                dev_name = node.find('DeviceID').text
                if dev_name == self.device_name:
                    for info_str in info_list:
                        value = node.find(info_str)
                        if value is None:
                            d[info_str] = 'NA'
                        else:
                            d[info_str] = value.text
                    break
        except Exception, e:
            s = sys.exc_info()
            print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
            sys.exit(-1)    
        return d

    def SaveDevInfoToConfig(self, info_dic):
        try:
            root = ET.parse(self.xml_config)
            nodes = root.findall('Target')
            for node in nodes:
                dev_name = node.find('DeviceID').text
                if dev_name == self.device_name:
                    for key in info_dic.keys():
                        value = node.find(key)
                        if value is None:
                            node.append(Element(key))
                        node.find(key).text = info_dic[key]
                    break
            root.write(self.xml_config, encoding='utf-8')
        
        except Exception, e:
            s = sys.exc_info()
            print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
            sys.exit(-1)    
        
    def GetDevInfo(self, info_name):
        try:
            adb_hp = adbhelper.AdbHelper(self.adb_path)
            adb_hp.device_id = self.device_name
            value = 'NA'
            if info_name.lower() == 'osver':
                value = adb_hp.adb_execution('shell getprop ro.build.version.release', 30, shell_flag=True)[0]
                value = value.strip().strip('\n').strip('\r')
            if info_name.lower() == 'model':
                value = adb_hp.adb_execution('shell getprop ro.product.model', 30, shell_flag=True)[0]
                value = value.strip().strip('\n').strip('\r')
            if info_name.lower() == 'dpi':
                temp_value = adb_hp.adb_execution('shell wm density', 30, shell_flag=True)[0]
                temp_value = temp_value.strip().strip('\n').strip('\r')
                if temp_value.lower().find('override density:') > -1:
                    info_value = temp_value.split(':')            
                    if info_value[0].strip().lower() == 'override density':
                        value = info_value[1].strip().lower()
                elif temp_value.lower().find('physical density:') > -1:
                    info_value = temp_value.split(':')            
                    if info_value[0].strip().lower() == 'physical density':
                        value = info_value[1].strip().lower()
                    else:
                        value = 'NA'
                else:
                    temp_value = adb_hp.adb_execution('shell dumpsys window', 30, shell_flag=True)[0]
                    lines = temp_value.strip().split('\r')
                    flag = False
                    for line in lines:
                        if line.strip().startswith('DisplayContents:'):
                            flag = True
                            continue
                        if flag and line.strip().startswith('init='):
                            temp_value = line.split('=')[1].strip().lower()
                            if temp_value.find('dpi') > -1:
                                value = temp_value.split(' ')[1].replace('dpi', '')
                    
            if info_name.lower() == 'resolution':
                value = adb_hp.adb_execution('shell wm size', 30, shell_flag=True)[0]
                value = value.strip().strip('\n').strip('\r')
                if value.lower().find('physical size:') > -1:
                    info_value = value.split(':')            
                    if info_value[0].strip().lower() == 'physical size':
                        value = info_value[1].strip().lower()
                    else:
                        value = 'NA'
                else:
                    temp_value = adb_hp.adb_execution('shell dumpsys window', 30, shell_flag=True)[0]
                    lines = temp_value.strip().split('\r')
                    flag = False
                    for line in lines:
                        if line.strip().startswith('DisplayContents:'):
                            flag = True
                            continue
                        if flag and line.strip().startswith('init='):
                            temp_value = line.split('=')[1].strip().lower()
                            value = temp_value.split(' ')[0]
           
        except Exception, e:
            s = sys.exc_info()
            print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
            sys.exit(-1)    
        return value
    
    def CompleteDevInfo(self, info_strs):
        try:
            # Get dev info from xml
            config_dicconfig_dic = {}
            for key in info_strs.split(','):
                value = self.GetDevInfo(key)                    
                if value == 'NA':
                    # If resolution and dpi is not provided, and failed to fetch from device. Use the default value.
                    if key.lower() == 'resolution':
                        value = '1200x1920'
                    elif key.lower() == 'dpi':
                        value = '320'
                config_dicconfig_dic[key] = value
            # Update Config with dev info
            self.SaveDevInfoToConfig(config_dicconfig_dic)
        
        except Exception, e:
            s = sys.exc_info()
            print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
            sys.exit(-1)    
        
    
class DeviceNetwork(object):
    def __init__(self, adb_path, device_name, logger):
        self.adb_path = adb_path
        self.adb_hp = adbhelper.AdbHelper(self.adb_path)
        self.adb_hp.device_id = device_name
        self.logger = logger

    def TurnOnWiFi(self, screenshot_file="", timeout=30):
        wifi_state = ''
        net_ssid = ''
        wifi_state_enabled = False
        net_state_connected = False
        sleep_time = 10
        while timeout > 0:
            scr_file = ""
            if timeout == sleep_time:
                # It is the last trial, need to have screenshot to record the status.
                scr_file = screenshot_file
            wifi_state, net_ssid, wifi_state_enabled, net_state_connected = self.CheckNetworkState(screenshot_file=scr_file)
            if net_state_connected:
                break
            msg = "\n* Trying to enable WiFi ..."
            print msg
            self.logger.info(msg)
            self.adb_hp.adb_execution("shell am start -n \"com.intel.wifichecker/com.intel.wifichecker.TurnOnWiFi\" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER", 120 )
            time.sleep(sleep_time)
            self.adb_hp.adb_execution('shell am force-stop com.intel.wifichecker', 120)
            timeout -= sleep_time
        return wifi_state, net_ssid, wifi_state_enabled, net_state_connected
    
    def CheckNetworkState(self, screenshot_file=""):
        wifi_state_enabled = False
        wifi_state = 'not found'
        net_state_connected = False
        net_ssid = 'not found'    
        # Sometime, the device is just rebooted, the command may fail. We need to sleep some time and try again.
        self.adb_hp.adb_execution("shell am start -n \"com.intel.wifichecker/com.intel.wifichecker.CheckWiFiActivity\" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER", 120)
        time.sleep(5)
        output, err = self.adb_hp.adb_execution("shell dumpsys activity .CheckWiFiActivity", 120)
        lines = output.split('\n')
        for info in lines:
            if info.startswith("WiFi Status:"):
                wifi_state = info.split(':')[1]
                if wifi_state.lower().strip() == 'enabled':
                    wifi_state_enabled = True
                    
            if info.startswith("Network SSID:"):
                net_ssid = info.split(':')[1].strip().strip('"')
            if info.startswith("Network Status:") and info.split(':')[1].lower().strip() == 'connected':
                net_state_connected = True
            
        # If screenshot_file is not empty, we need to create the screen shot to host named as screenshot_file.
        if screenshot_file != '' and not net_state_connected:
            self.adb_hp.adb_execution("shell rm -f /storage/sdcard0/wifi_screenshot.png", 120)
            self.adb_hp.adb_execution("shell screencap -p /storage/sdcard0/wifi_screenshot.png", 120)
            self.adb_hp.adb_execution('pull /storage/sdcard0/wifi_screenshot.png %s' % screenshot_file, 120)
            time.sleep(5)
 
        self.adb_hp.adb_execution('shell am force-stop com.intel.wifichecker', 120)

        return wifi_state, net_ssid, wifi_state_enabled, net_state_connected
    
    def ConnectHotSpot(self, hotspot, screenshot_file="", timeout=60):
        if hotspot == "":
            return 'Selected hotspot is empty.', ''
        wifi_state = ''
        net_ssid = ''
        wifi_state_enabled = False
        net_state_connected = False
        res = 'false'
        while timeout > 0:
            scr_file = ""
            sleep_time = 15
            
            msg = "\n* Trying to connect to %s ..." % hotspot
            print msg
            self.logger.info(msg)
            self.adb_hp.adb_execution('shell am start -n "com.intel.wifichecker/com.intel.wifichecker.ConnectToHotSpotActivity" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER', 120)
            time.sleep(2)          
            self.adb_hp.adb_execution('shell input text "%s#"' % hotspot, 120)
            # More time to waiting for connection progress done especailly in poor network status.
            time.sleep(sleep_time)          
            self.adb_hp.adb_execution('shell am force-stop com.intel.wifichecker', 120)  
            if timeout == sleep_time:
                # It is the last trial, need to have screenshot to record the status.
                scr_file = screenshot_file
            wifi_state, net_ssid, wifi_state_enabled, net_state_connected = self.CheckNetworkState(screenshot_file=scr_file)
            if net_state_connected and net_ssid.strip().lower() == hotspot.strip().lower():
                res = 'true'
                break
            timeout -= sleep_time
        if not net_state_connected:
            res = 'NA'
          
        return res, net_ssid


class Enviroment(object):
    def __init__(self):
        pass

    @staticmethod
    def is_win8_or_above():
        # 6.2 stands for Windows 8 or Windows Server 2012
        # https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
        '''
        return true when the os is windows 8 or higher version
        '''
        return True if float(platform.version()[:3]) >= 6.2 else False

    @staticmethod
    def is_cloud_mode():
        return False

    @staticmethod
    def remove_item(path, check_existence=True, logger=None, msg=''):
        try:
            if check_existence and not os.path.exists(path):
                return

            if os.path.isdir(path):
                shutil.rmtree(path)
            elif os.path.isfile(path):
                os.remove(path)
            else:
                print "{} is a unknown type.".format(path)
        except Exception as e:
            out_info = "{} {}".format(msg, str(e)) if msg else "Failed to delete {} due to {}".format(path, str(e))
            out_handle = logger if logger else str
            out_handle(out_info)

    @staticmethod
    def get_com_port():
        port_list = list(serial.tools.list_ports.comports())
        if port_list:
            return sorted([list(i)[0] for i in port_list if list(i)[1].find("Arduino") != -1])
        else:
            return []

    @staticmethod
    def get_connected_camera_list():
        wmi = win32com.client.GetObject("winmgmts:")

        return [i.Name for i in wmi.InstancesOf("Win32_UsbHub") if i.Name.find("Logitech USB Camera") != -1]

    @staticmethod
    def generate_dvc_config(dvc_bin_path, device_sn, camera_mode, height, width, com_port="None"):
        config = "<?xml version=\"1.0\"?>\n" \
                 "<DaVinciConfig>\n" \
                 "<androidDevices>\n" \
                 "<AndroidTargetDevice>\n" \
                 "<name>%s</name>\n" \
                 "<height>%s</height>\n" \
                 "<width>%s</width>\n" \
                 "<screenSource>%s</screenSource>\n" \
                 "<qagentPort>%s</qagentPort>\n" \
                 "<magentPort>%s</magentPort>\n" \
                 "<hypersoftcamPort>%s</hypersoftcamPort>\n" \
                 "<hwAccessoryController>%s</hwAccessoryController>\n" \
                 "</AndroidTargetDevice>\n" \
                 "</androidDevices>\n" \
                 "<windowsDevices />\n" \
                 "<chromeDevices />\n" \
                 "</DaVinciConfig>" % (device_sn, height, width, camera_mode, Enviroment.find_free_port(),
                                       Enviroment.find_free_port(), Enviroment.find_free_port(), com_port)

        with open(os.path.join(dvc_bin_path, "DaVinci.config"), "w") as davinci_config_file:
            davinci_config_file.write(config)

    @staticmethod
    def find_free_port():
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(('', 0))
        return sock.getsockname()[1]

    @staticmethod
    def get_sub_folder_name(folder_path, num):
        # print os.path.isdir(folder_path)

        if os.path.isdir(folder_path):
            all_folder = sorted([(i, os.path.getmtime(os.path.join(folder_path, i)))
                                 for i in os.listdir(folder_path) if i != "indexformat" and
                                 os.path.isdir(os.path.join(folder_path, i))], key=lambda i: i[1], reverse=True)
            result = all_folder[:num]
            return [os.path.join(folder_path, i[0]) for i in result]
        else:
            return []

    @staticmethod
    def find_download_pic(log_folder):
        log_folder += "\\on_device_qscript\\_Logs\\"
        pattern_pic = re.compile(r'downloading\d{8}T\d{6}\.png')
        download_pic = []
        for paths, dirs, files in os.walk(log_folder):
            for each_file in files:
                if pattern_pic.match(each_file):
                    download_pic.append(paths + "\\" + each_file)

        if download_pic:
            return download_pic[len(download_pic) - 1]
        else:
            return ""

    @staticmethod
    def check_compatibility_result(log_folder):
        for files in os.walk(log_folder):
            for fileName in files:
                if "incompatible.txt" in fileName:
                    return False
        return True

    @staticmethod
    def search_dvc_log(log_file, key_list):
        if not os.path.exists(log_file):
            return False

        with open(log_file) as f:
            content = f.readlines()
        content = [i.strip() for i in content if i.strip()]
        
        result = [each_line for each_line in content for key in key_list if key in each_line]
        
        return True if result else False

    @staticmethod
    def get_average_value(data):
        if not data or not isinstance(data, list):
            return 'N/A'

        if len(data) == 1:
            return str(data[0])

        return str(float((reduce(lambda x, y: x + y, data) / len(data))))

    @staticmethod
    def get_std_deviation(data):
        if not data or not isinstance(data, list) or len(data) == 1:
            return 'N/A'
        average_value = (reduce(lambda x, y: x + y, data) / len(data))

        sum_dev = reduce(lambda x, y: x + y, ((i - average_value)**2 for i in data)) / (len(data) - 1)

        return sum_dev**0.5

    @staticmethod
    def get_file_list_with_postfix(path, postfix):
        exist_files = [i for i in os.listdir(path) if os.path.isfile(path + os.path.sep + i)]
        target = [path + os.path.sep + i for i in exist_files if i.endswith(postfix)]

        return target[0] if target else None

    @staticmethod
    def get_value_from_qs(qs_path, value):
        if not os.path.exists(qs_path):
            return ''

        with open(qs_path) as f:
            content = f.readlines()
        sentence = [i.strip() for i in content if i.startswith(value)]
        result = sentence[0].split('=') if sentence else []
        return result[1] if len(result) > 1 else ''

    @staticmethod
    def get_cur_time_str(format="%Y_%m_%d_%H_%M_%S"):
        return time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())

    @staticmethod
    def get_keyword_times(path, kw):
        if not os.path.exists(path) or not os.path.isfile(path):
            return 0

        with open(path) as f:
            content = f.readlines()

        return len([i for i in content if i.find(kw) != -1])

    @staticmethod
    def check_dvc_log(log_file, key_list):
        try:
            if not os.path.exists(log_file):
                print("{0} does not exist.".format(log_file))
                return None, None
            with open(log_file, 'r') as f:
                lines = f.readlines()
            line_index = 0
            error_found = False
            error_body = False
            key_found = False
            special_error = False
            for line in lines:
                line_index += 1
                line_info = line.strip().strip(os.linesep)
                for key_str in key_list:
                    if line.find(key_str) > -1:
                        key_found = True
                pattern = re.compile(r"^\[.*\]\[(.*)\]")
                m = pattern.match(line_info)
                pattern2 = re.compile(r"^\[(.*)\] ")
                m2 = pattern2.match(line_info)
                if (m and (m.group(1) == "error" or m.group(1) == "fatal")) or\
                        (m2 and (m2.group(1) == "error" or m2.group(1) == "fatal")):
                    if line_info.find("Comparing Q script failure due to missing resources!") > -1:
                        continue
                    if line_info.find(r'Diagnostic: ') > -1:
                        continue
                    if line_info.find(r'The script exited unexpectedly at IP: ') > -1:
                        special_error = True
                    error_found = True
                    error_body = True
                    # ErrorLog(log_file + ": Line " + str(line_index))
                    # ErrorLog(line_info)
                    continue
                # check if it is the next log line, do not save as error body.
                if (m or m2) and error_body:
                    error_body = False
                # Save the following lines for error details.
                # if error_body:
                    # ErrorLog(line_info)
                    # If go to next message...

            # For special error, when Agent is OK, this error means smoke test quit since test result is fail.
            # So just ignore this error.
            if special_error and not key_found:
                error_found = False
            return error_found, key_found

        except Exception, e:
            s = sys.exc_info()
            print "[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],)
            sys.exit(-1)


class XmlOperator(object):
    def __init__(self, xml_file=None):
        self.xml_file = xml_file
        if self.xml_file is not None and os.path.exists(self.xml_file):
            # self.root = xml.etree.ElementTree.parse(self.xml_file)
            self.tree = ET.parse(self.xml_file)
            self.root = self.tree.getroot()
        else:
            self.tree = None
            self.root = None

    def getTextsByTagName(self, tag_name):
        if self.tree is not None:
            collections = self.tree.iterfind(tag_name)
            return [elem.text for elem in collections if elem.text]
        else:
            return []

    def getAttributeValue(self, attr_name):
        if self.root is not None:

            return [elem.attrib for elem in self.root.iter(attr_name)]
        else:
            return []

    def getTextByXpath(self, xpath):
        if self.root is not None:
            return [elem.text for elem in self.root.iterfind(xpath)]
        else:
            []

    def updateTextByTagName(self, tag_name, origin_value, new_value):
        if self.root is None or not origin_value or not new_value or str(origin_value) == str(new_value):
            return

        for elem in self.root.iterfind(tag_name):
            if elem.text == str(origin_value):
                elem.text = str(new_value)
                break

        self.tree.write(self.xml_file)

    @staticmethod
    def createXml(xml_file, device_id, qagent_port, magent_port, hypercam_port):
        # if os.path.exists(xml_file):
        #     return
        root = ET.Element('DaVinciConfig')

        useGpuAcceleration = ET.SubElement(root, 'useGpuAcceleration')
        useGpuAcceleration.text = 'False'

        audioRecordDevice = ET.SubElement(root, 'audioRecordDevice')
        audioRecordDevice.text = 'False'

        audioPlayDevice = ET.SubElement(root, 'audioPlayDevice')
        audioPlayDevice.text = 'False'
        
        AndroidDevices = ET.SubElement(root, 'AndroidDevices')
        AndroidTargetDevice = ET.SubElement(AndroidDevices, 'AndroidTargetDevice')
        name = ET.SubElement(AndroidTargetDevice, 'name')
        name.text = str(device_id)

        screenSource = ET.SubElement(AndroidTargetDevice, 'screenSource')
        screenSource.Text = 'HyperSoftCam'

        qagentPort = ET.SubElement(AndroidTargetDevice, 'qagentPort')
        qagentPort.text = str(qagent_port)
        magentPort = ET.SubElement(AndroidTargetDevice, 'magentPort')
        magentPort.text = str(magent_port)
        hypersoftcamPort = ET.SubElement(AndroidTargetDevice, 'hypersoftcamPort')
        hypersoftcamPort.text = str(hypercam_port)

        tree = ET.ElementTree(root)
        tree.write(xml_file, encoding='utf-8')


class ExcelWriter(object):

    def __init__(self, excel_file_name):
        self.excel_file = excel_file_name
        self.book = xlsxwriter.Workbook(self.excel_file, {'strings_to_urls': True})

    def generate_excel(self, sheets):
        for sheet in sheets:
            cur_sheet = self.book.add_worksheet(sheet[0])

            cur_sheet.write_row('A1', sheet[1], self.book.add_format({'bold': True}))
            cur_sheet.write_row('A2', sheet[2])
            cur_sheet.write_row('A4', sheet[3], self.book.add_format({'bold': True}))
            for index, content in enumerate(sheet[4:]):
                cur_sheet.write_row('A' + str(index + 5), content)

        self.book.close()


class AAPTHelper(object):
    def __init__(self, aapt_path):
        self.aapt_path = aapt_path
        self.worker = dvcsubprocess.DVCSubprocess()

    def get_package_version_apk_name(self, apk_path):
        cmd = '{0} d badging "{1}"'.format(self.aapt_path, apk_path)
        output = self.worker.Popen(cmd)[0].split(os.linesep)
        pkg_ver = [i for i in output if i.startswith("package: name=")]
        apk_name_match = [i for i in output if i.startswith("application-label:")]

        apk_name = apk_name_match[0].split("'")[1] if apk_name_match else os.path.basename(apk_path)

        if pkg_ver:
            details = pkg_ver[0].split("'")
            pkg_name = details[1]
            version = details[5]
        else:
            pkg_name = ''
            version = ''

        return pkg_name, version, apk_name.decode('utf-8')

    def get_package_launchable_activity(self, apk_path):
        cmd = '{0} d badging "{1}"'.format(self.aapt_path, apk_path)
        output = self.worker.Popen(cmd)[0].split(os.linesep)
        activity_res = [i for i in output if i.startswith("launchable-activity:")]
        if activity_res:
            return activity_res[0].split("'")[1]
        else:
            return ''

    def get_apk_abi(self, apk_path):
        cmd = '{0} d badging {1}'.format(self.aapt_path, apk_path)
        output = self.worker.Popen(cmd)[0].split(os.linesep)

        if output:
            res = [i for i in output if i.startswith('native-code:')]
            if res:
                return [i for i in  res[0].split("native-code:")[1].split("'")
                        if i.strip() and i.strip().find("lib") == -1]
            else:
                return ['java']
        else:
            return ['java']

    def get_apk_type(self, apk_path):
        cmd = '{0} d badging {1}'.format(self.aapt_path, apk_path)
        output = self.worker.Popen(cmd)[0].split(os.linesep)

        if output:

            res = [i for i in output if i.startswith('native-code:')]
            if not res:
                return 'java'
            if res and res[0].find('arm') != -1:
                return 'arm'
            elif res and res[0].find('x86') != -1:
                return 'x86'
            else:
                return 'unknown'
        else:
            return 'unknown'


class DVCLogger(type(sys)):
    '''
    This class is used for printing colorful log
    '''
    DEBUG = 10
    INFO = 20
    WARNING = 30
    ERROR = 40
    CRITICAL = 50
    # NOTSET = 0
    WINDOWS_STD_OUT_HANDLE = -11
    GREEN_COLOR = 2
    RED_COLOR = 4
    YELLOW_COLOR = 6
    WHITE_COLOR = 7

    def __init__(self, *args, **kwargs):
        self.level = self.__class__.INFO
        self.output = None

    def __set_color(self, color):
        out_handler = ctypes.windll.kernel32.GetStdHandle(self.__class__.WINDOWS_STD_OUT_HANDLE)
        ctypes.windll.kernel32.SetConsoleTextAttribute(out_handler, color)

    def __log(self, level, fmt, *args, **kwargs):
        time_format = "%Y-%m-%d %H:%M:%S"
        sys.stderr.write('{0} {1} {2}\n'.format(level, time.strftime(time_format, time.localtime()), fmt % args))
        if level >= self.level and self.output is not None:
            with open(self.output, 'a') as f:
                f.write('{0} {1} {2}\n'.format(level, time.strftime(time_format, time.localtime()), fmt % args))

    def format(color, level):
        def wrapped(func):
            def log(self, fmt, *args, **kwargs):
                self.__set_color(color)
                self.__log(level, fmt, *args, **kwargs)
                self.__set_color(self.__class__.WHITE_COLOR)
            return log
        return wrapped

    def config(self, *args, **kwargs):
        if 'outfile' in kwargs:
            self.output = kwargs['outfile']

        if 'level' in kwargs:
            self.level = kwargs['level']
        else:
            self.level = int(kwargs.get('level', self.__class__.INFO))

    @format(GREEN_COLOR, 'DEBUG')
    def debug(self, fmt, *args, **kwargs):
        pass

    @format(WHITE_COLOR, 'INFO')
    def info(self, fmt, *args, **kwargs):
        pass

    @format(YELLOW_COLOR, 'WARNING')        
    def warning(self, fmt, *args, **kwargs):
        pass

    @format(RED_COLOR, 'ERROR')
    def error(self, fmt, *args, **kwargs):
        pass

    @format(RED_COLOR, 'CRITICAL')
    def critical(self, fmt, *args, **kwargs):
        pass

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
import subprocess
import re
import config
import apkhelper

APK_NAME = r'com.redphx.deviceid-1.apk'

ADB_CMD = ['shell am force-stop com.redphx.deviceid',
           'uninstall com.redphx.deviceid',
           'install com.redphx.deviceid-1.apk',
           'shell am start -n com.redphx.deviceid/.MainActivity',
           'shell uiautomator dump',
           'pull /storage/emulated/legacy/window_dump.xml',
           'pull /storage/sdcard0/window_dump.xml']

OUT_XML = r'window_dump.xml'

CONFIG_FILE = r'config.py'

DEFAULT_CONFIG_CONTENT = "default_http_proxy = \"proxy02.cd.intel.com:911\"\n\
default_socks_proxy = \"proxy.ir.intel.com:1080\"\n\
default_proxy_type = \"http\"\n\
default_davinci_path = \"C:/Intel/BiTs/DaVinci/bin\"\n\
email = \" \"\n\
default_apk_save_path = \"C:/Intel/BiTs/DaVinci/Scripts\"\n\
device_id = \"1234567890abcdef\"\n\
default_on_device_download_id = \"\"\n\
password = \" \"\n\
default_store_index = \"2\"\n\
default_apk_category = \"3\"\n\
default_download_begin = \"1\"\n\
default_download_end = \"100\"\n\
default_google_apk_list = \"c:/script/googleapk.txt\"\n\
country = \"USA\"\n\
operator = \"AT&T\"\n\
device_name = \"\"\n\
sdklevel = 17"

INVALID_DEVICE_ID = '1234567890abcdef'

def get_connected_device_list():
    cmd = r"{0:s}\platform-tools\adb devices".format(config.default_davinci_path.replace('/', '\\'))
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        devices_content = p.communicate()[0].split('\r\n')
        device_list = [i.split('\tdevice')[0] for i in devices_content if i.strip() != '' and i.endswith('\tdevice')]
    except Exception, e:
        return None
    else:
        return device_list


def select_one_device():
    device_list = get_connected_device_list()

    if not device_list:
        user_input = apkhelper.get_user_input(
            "If you have connected the device to the pc, press 'Y' to continue, press 'N' to exit script:\n", 'y',
            False, 60)

        if user_input == 'n' or user_input == 'N':
            print '[LogInfo]Bye'
            sys.exit(0)

    device_list = get_connected_device_list()

    if not device_list:
        return None
    print '*' * 20
    for i in range(1, len(device_list) + 1):
        print str(i) + '.   ' + device_list[i - 1]
    print '*' * 20
    device_option = apkhelper.get_user_input('Please select one device for downloading', '1')
    new_download_id = device_list[int(device_option) - 1 if int(device_option) <= len(device_list) else 0]
    return new_download_id


def check_config_file_exists():
    config_file = os.getcwd() + os.path.sep + 'config.py'
    if not os.path.exists(config_file):
        return False

    with open(config_file) as f:
        content = f.readlines()

    if len(content) < 17:
        os.remove(config_file)
        return False
    return True

def save_config(tag_name, tag_value):
    with open(CONFIG_FILE, 'r') as f:
        data = f.read()

    if tag_name == 'default_davinci_path' or tag_name == 'default_apk_save_path':
        tag_value = tag_value.replace('\\', '/')
        if tag_value.startswith('..'):
            cur_pwd = os.getcwd()
            os.chdir(os.getcwd() + os.path.sep + tag_value)
            tag_value = os.getcwd().replace('\\', '/')
            os.chdir(cur_pwd)

    with open(CONFIG_FILE, 'w') as f:
        pos = data.find(tag_name)
        if pos == -1:
            f.write(data + '\%s = "%s"' % (tag_name, tag_value))
        else:
            quato_1st_pos = data.find('"', pos + 1)
            quato_2nd_pos = data.find('"', quato_1st_pos + 1)

            content = data[:quato_1st_pos + 1] + tag_value + data[quato_2nd_pos:]

            f.write(content)


# def remove_value_from_config(tag_name, tag_value):
#     with open(CONFIG_FILE, 'r') as f:
#         data = f.read()
#
#     with open(CONFIG_FILE, 'w') as f:
#         pos = data.find(tag_name)
#         if pos == -1:
#             return
#         else:
#             quato_1st_pos = data.find('"', pos + 1)
#             quato_2nd_pos = data.find('"', quato_1st_pos + 1)
#             origin_value = data[quato_1st_pos + 1: quato_2nd_pos].split(' ')
#             if origin_value.count(tag_value) > 0:
#                 origin_value.remove(tag_value)
#             content = data[:quato_1st_pos + 1] + ' '.join(origin_value) + data[quato_2nd_pos:]
#
#             f.write(content)


def fetch_device_id(data):
    m = re.search("[a-z0-9A-Z]{16}", data)
    if m is None:
        print '[WarningInfo]No valid device ID was found!'

        return INVALID_DEVICE_ID
    else:
        return m.group(0)

def get_device_id(davinci_path="", target_device=''):
    if not os.path.exists(davinci_path + os.path.sep + "platform-tools" + os.path.sep + "adb.exe"):
        print "'%s'\\platform-tools\\adb.exe does not exist." % davinci_path
        sys.exit(-1)

    adb_cmd = "{0}{1}platform-tools{2}adb -s {3} ".format(davinci_path.replace('/', "\\"), os.path.sep, os.path.sep,
                                                          target_device)
    if os.path.exists(APK_NAME):
        print 'Fetching Android device id...'
        for i in ADB_CMD:
            try:
                subprocess.check_call(adb_cmd + i)
            except subprocess.CalledProcessError, e:
                if i != ADB_CMD[5] and i != ADB_CMD[6]:
                    print 'Got device id failed'
                    print 'Please check the device connection and enable adb debugging option.'
                    sys.exit(0)
    else:
        print 'com.redphx.deviceid-1.apk was lost.'
        sys.exit(-1)

    if os.path.exists(OUT_XML):
        with open(OUT_XML) as f:
            data = f.read()
    else:
        print 'Got device id failed due to %s was not found.' % OUT_XML
        sys.exit(0)

    android_device_id = fetch_device_id(data)

    if not android_device_id:
        android_device_id = INVALID_DEVICE_ID

    if not os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, 'w') as f:
            f.write(DEFAULT_CONFIG_CONTENT)

    if os.path.exists(OUT_XML):
        os.remove(OUT_XML)

    if android_device_id != INVALID_DEVICE_ID:
        save_config('device_id', android_device_id)
        print 'Got Android device id successfully.\n'

    return android_device_id

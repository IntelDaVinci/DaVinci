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
import launchtime
import dvcutility
from getdeviceid import save_config

if __name__ == '__main__':
    # print sys.argv

    if len(sys.argv) < 4:
        print 'Invalid Parameter, please restart the script'
        sys.exit(0)

    dvc_folder = sys.argv[1]
    apk_folder = sys.argv[2]
    treadmill_config = sys.argv[3]
    para = {}

    if not os.path.exists(dvc_folder):
        print 'The DaVinci folder is not exists!'
        sys.exit(0)

    if not os.path.exists(treadmill_config):
        print 'Treadmill.config file is not exists!'
        sys.exit(0)

    if not os.path.exists(apk_folder):
        try:
            os.mkdir(apk_folder)
        except Exception as e:
            print 'Failed to create {0} due to {1}'.format(apk_folder, str(e))
            apk_folder = os.getcwd()

    xml_object = dvcutility.XmlOperator(treadmill_config)

    launchtime_running_mode = xml_object.getTextsByTagName("launchtime_running_mode")[0]

    result = xml_object.getTextsByTagName("lt_running_times")

    lt_running_times = int(result[0]) if result else 1
    
    lt_device_id = xml_object.getTextsByTagName("Target/DeviceID")[0]

    lt_test_interval = xml_object.getTextsByTagName("lt_test_interval")[0]

    result = xml_object.getTextsByTagName("fresh_run_flag")
    fresh_run_flag = True if result and result[0] == 'enabled' else False

    result = xml_object.getTextsByTagName("clear_cache_flag")
    clear_cache_flag = True if result and result[0] == 'enabled' else False

    result = xml_object.getTextsByTagName("usb_replug_flag")
    usb_replug_flag = True if result and result[0] == "enabled" else False

    camera_mode = xml_object.getTextsByTagName("camera_mode")[0]
    para['camera_mode'] = camera_mode

    result = xml_object.getTextsByTagName("camera_index")
    if result:
        para['camera_index'] = result[0]

    # print lt_device_id
    para['dvc_path'] = os.path.abspath(dvc_folder)
    para['device_id'] = lt_device_id
    para['running_times'] = lt_running_times
    para['apk_folder'] = apk_folder
    para['launchtime_running_mode'] = launchtime_running_mode
    para['interval'] = lt_test_interval
    para['fresh_run_flag'] = fresh_run_flag
    para['clear_cache_flag'] = clear_cache_flag
    para['usb_replug_flag'] = usb_replug_flag

    result = xml_object.getTextsByTagName("pre_launch_apk_folder")
    if result:
        para['pre_launch_apk_folder'] = result[0]

    save_config('default_davinci_path', para['dvc_path'])

    if launchtime_running_mode == '2':
        result = xml_object.getTextsByTagName("APK_download_mode")
        lt_apk_mode = result[0]
        para['apk_mode'] = lt_apk_mode

        para['generate_ini_timeout'] = xml_object.getTextsByTagName("lt_timeout")[0]

        if lt_apk_mode == 'on-device':
            para['category'] = xml_object.getTextsByTagName("category_selection")[0]
            para['start_rank'] = xml_object.getTextsByTagName("start_rank")[0]
            para['dl_num'] = xml_object.getTextsByTagName("dl_num")[0]
            para['ondevice_timeout'] = xml_object.getTextsByTagName("OnDeviceInstallTimeout")[0]
    elif launchtime_running_mode == '3':
        para['logcat_list_path'] = xml_object.getTextsByTagName("logcat_list_path")[0]
    else:
        para['existed_lt_case_path'] = xml_object.getTextsByTagName("existed_lt_case_path")[0]
    try:
        lt = launchtime.LaunchTimeGenerator(**para).get_tester()
        lt.start()
    except Exception as e:
        s = sys.exc_info()
        print "[Error@{0}]:{1} due to {2}".format(s[2].tb_lineno, s[1], str(e))

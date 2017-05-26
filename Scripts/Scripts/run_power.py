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
import power
import dvcutility
from getdeviceid import save_config

if __name__ == '__main__':

    if len(sys.argv) < 4:
        print 'Invalid Parameter, please restart the script'
        sys.exit(0)

    dvc_folder = sys.argv[1]
    apk_folder = sys.argv[2]
    smart_runner_config = sys.argv[3]
    para = {}

    if not os.path.exists(dvc_folder):
        print 'The DaVinci folder is not exists!'
        sys.exit(0)

    if not os.path.exists(smart_runner_config):
        print 'Smart runner configuration file is not exists!'
        sys.exit(0)

    if not os.path.exists(apk_folder):
        try:
            os.mkdir(apk_folder)
        except Exception as e:
            print 'Failed to create {0} due to {1}'.format(apk_folder, str(e))
            apk_folder = os.getcwd()

    xml_object = dvcutility.XmlOperator(smart_runner_config)

    result = xml_object.getTextsByTagName("power_running_times")

    power_running_times = int(result[0]) if result else 1

    result = xml_object.getTextsByTagName("power_qs_path")
    power_qs_path = result[0] if result else os.getcwd()

    device_id = xml_object.getTextsByTagName("Target/DeviceID")[0]

    para['dvc_path'] = os.path.abspath(dvc_folder)
    para['device_id'] = device_id

    para['running_times'] = power_running_times

    para['apk_folder'] = apk_folder
    para['power_qs_path'] = power_qs_path

    save_config('default_davinci_path', para['dvc_path'])

    try:
        powertesting = power.PowerGenerator(**para).get_tester()
        powertesting.start()
    except Exception as e:
        s = sys.exc_info()
        print "[Error@{0}]:{1} due to {2}".format(s[2].tb_lineno, s[1], str(e))

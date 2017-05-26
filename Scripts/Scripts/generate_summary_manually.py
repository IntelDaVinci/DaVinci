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

import sys
import os
import time
import glob
from generate_summary import Generate_Summary_Report

reload(sys) 
sys.setdefaultencoding('utf-8')

if __name__ == "__main__":
    no_result_list = []
    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
    config_file_name = script_path + "\\SmartRunner.config"
    config_file_txt = script_path + "\\SmartRunner.config.txt"

    if not os.path.exists(config_file_name):
        print "Treadmill config file %s does not exist." % config_file_name
        sys.exit(-1)

    if not os.path.exists(config_file_txt):
        print "Treadmill config file %s does not exist." % config_file_txt
        sys.exit(-1)

    davinci_folder = sys.argv[1]
    if not os.path.exists(davinci_folder):
        print "DaVinci folder %s does not exist." % davinci_folder
        sys.exit(-1)

    apk_folder = os.path.abspath(sys.argv[2])
    if not os.path.exists(apk_folder):
        print "Apk folder %s does not exist." % apk_folder
        sys.exit(-1)

    dummy_start_time = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())


    executed_list = r"%s\executed_list.txt" % (apk_folder)
    no_result_file_list = []
    # copy result file to test result folder
    timestamp_tag = "Executed Timestamp:"
    if os.path.exists(executed_list):
        with open(executed_list, 'r') as f:
            contents = f.readlines()
        for line in contents:
            if line.startswith(timestamp_tag):
                tmp = line.split(":")[1].strip().strip(os.linesep)   
                previous_no_result_app_list = r"%s\no_result_app_list_%s.txt" % (apk_folder, tmp)
                if os.path.exists(previous_no_result_app_list):                   
                    no_result_file_list.append(previous_no_result_app_list)
    else:
        no_result = glob.glob1(apk_folder, r'no_result_app_list_*')
        for item in no_result:
            no_result_file_list.append(r"%s\%s" % (apk_folder, item))

    Generate_Summary_Report(apk_folder, davinci_folder, config_file_txt, config_file_name, no_result_file_list, dummy_start_time)

    print "Please check the summary report under %s" % apk_folder

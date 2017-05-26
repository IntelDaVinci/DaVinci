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
from common import *

reload(sys) 
sys.setdefaultencoding('utf-8')

if __name__ == "__main__":
    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
    seed_range = sys.argv[1]       # 1-5
    action = sys.argv[2]
    davinci_folder = os.path.abspath(sys.argv[3])
    apk_folder = os.path.abspath(sys.argv[4])

    smoke_config = script_path + "\\SmokeConfig.csv"
    config_file_txt = script_path + "\\SmartRunner.config"

    if not os.path.exists(smoke_config):
        print "Smoke config file %s does not exist." % smoke_config
        sys.exit(-1)

    if not os.path.exists(config_file_txt):
        print "Config file %s does not exist." % config_file_txt
        sys.exit(-1)

    if not os.path.exists(davinci_folder):
        print "DaVinci folder %s does not exist." % davinci_folder
        sys.exit(-1)

    if not os.path.exists(apk_folder):
        print "Apk folder %s does not exist." % apk_folder
        sys.exit(-1)

    try:
        action_number = int(action)
    except:
        print "Action number is not correct"
        sys.exit(-1)

    time_out = action_number * 360 / 10

    start_seed = 0
    end_seed = 0
    seeds = seed_range.split('-')
    if (len(seeds) > 1):
        try:
            start_seed = int(seeds[0])
            end_seed = int(seeds[1])
        except:
            print "Input seed range is not correct, eg, 1-5"
            sys.exit(-1)
    else:
        print "Input seed range is not correct, eg, 1-5"
        sys.exit(-1)

    current_seed = start_seed
    while (current_seed <= end_seed):

        print "current seed: %s" % current_seed
        print "action: %s" % action
        # Update SmokeConfig.csv using new seed and action
        lines = open(smoke_config, 'r')
        config_lines = ""
        for line in lines:
            if line.startswith("Seed Value"):
                config_lines += "Seed Value (Seed is used to generate random number):," + str(current_seed) + "\n"
            elif line.startswith("Total Action Number"):
                config_lines += "Total Action Number (click action number + swipe action number):," + str(action_number) + "\n"
            else:
                config_lines += line
        writeFile(smoke_config, config_lines)
        lines.close()

        # Update SmartRunner.config.txt using new time out
        lines = open(config_file_txt, 'r')
        config_lines = ""
        for line in lines:
            if line.startswith("<SmokeTestTimeout>"):
                config_lines += "<SmokeTestTimeout>" + str(time_out) + "</SmokeTestTimeout>\n"
            else:
                config_lines += line
        writeFile(config_file_txt, config_lines)
        lines.close()

        # Call run.bat
        cmd = "%s\\run.bat %s %s SmartRunner.config" % (script_path, davinci_folder, apk_folder)

        print cmd
        process = os.system(cmd)

        current_seed = current_seed + 1

    print "Finish Running all the test with different seeds"

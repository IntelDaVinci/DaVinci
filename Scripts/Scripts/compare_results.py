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

import sys, os

def compare_res():
    # Get the list to run
    target_dev = []
    ref_dev = []
    dev_config_file = open(config_file_name, "r")
    for dev in dev_config_file:
        dev_info = dev.split(':')
        if dev_info[0].strip() == '[target_dev]':
            target_dev.append(dev_info[1].strip())
        if dev_info[0].strip() == '[ref_dev]':
            ref_dev.append(dev_info[1].strip())
    dev_config_file.close()
    
    res_dic = {}
    first_results = open(res_file1, 'r')
    first_line = True
    for result_line in first_results:
        if result_line.strip() == "":
            continue
        if first_line:
            first_line = False
            continue
        if len(result_line.split(',')) <> 13:
            continue
        test_res = "%s(%s)" %((result_line.split(',')[10]).strip(), (result_line.split(',')[1]).strip())
        apk_name = (result_line.split(',')[2]).strip()
        dev_name = (result_line.split(',')[1]).strip()

        if dev_name in target_dev:
            if not res_dic.has_key(apk_name):
                result = []
                result.append(test_res)
                result.append("")
                res_dic[apk_name] = result
            else:
                (res_dic[apk_name])[0] = test_res
    first_results.close()
    
    second_results = open(res_file2, 'r')
    for result_line in second_results:
        test_res = "%s(%s)" %((result_line.split(',')[10]).strip(), (result_line.split(',')[1]).strip())
        apk_name = (result_line.split(',')[2]).strip()
        dev_name = (result_line.split(',')[1]).strip()

        if dev_name in target_dev:
            if not res_dic.has_key(apk_name):
                    result = []
                    result.append("")
                    result.append(test_res)
                    res_dic[apk_name] = result
            else:
                (res_dic[apk_name])[1] = test_res
                
    second_results.close()
    
    regression = []                
    for key in res_dic.keys():
        first_result = (res_dic[key][0]).split('(')[0]
        second_result = (res_dic[key][1]).split('(')[0]
        if first_result.lower() == "pass" and second_result.lower() == "fail":
            regression.append( "%s\nFirst Result:%s\nSecond Result%s\n" % (key, res_dic[key][0], res_dic[key][1]))
    return regression

try:
    config_file_name = "default_device_cfg.txt"
    res_file1 = sys.argv[1]
    res_file2 = sys.argv[2]
    if not os.path.exists(res_file1):
        print "%s does not exist." % res_file1
        sys.exit(-1)
    if not os.path.exists(res_file2):
        print "%s does not exist." % res_file2
        sys.exit(-1)
    if not os.path.exists(config_file_name):
        print "device config file %s does not exist." % config_file_name
        sys.exit(-1)
        
    regression = compare_res()
    print "----------------------------------------------------------------------------"
    print "Regression APKs:"
    print "----------------------------------------------------------------------------"
    print '\n'.join(regression)
    print "----------------------------------------------------------------------------"
    print "Total: %s" % (len(regression))
    





except Exception, e:
    print str(e)
    
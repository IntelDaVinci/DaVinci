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


class UpstreamComposer():
        @classmethod
        def compose_overall_result(cls,result_line,xml_file):
                web_result = {}
                each_qs_execution_result = {}
                each_qs_execution_result['result_num'] = 13
                detail_data =  result_line.split(',')
                test_res = detail_data[11]
                tmp_qs_name = detail_data[15]
                dev_name = detail_data[1]
                each_qs_execution_result['serialnumber'] = detail_data[1]
                each_qs_execution_result['application'] = detail_data[2]
                each_qs_execution_result['appname'] = detail_data[3]
                each_qs_execution_result['package'] = detail_data[4]
                each_qs_execution_result['install'] = detail_data[6]
                each_qs_execution_result['launch'] = detail_data[7]
                each_qs_execution_result['random'] = detail_data[8]
                each_qs_execution_result['back'] = detail_data[9]
                each_qs_execution_result['uninstall'] = detail_data[10]
                each_qs_execution_result['overallresult'] = detail_data[11].lower()
                each_qs_execution_result['link'] = ''
                web_result['result'] = each_qs_execution_result
                web_result['configfile'] = xml_file
                resultfolder = result_line.split(',')[13]
                try:
                    i = resultfolder.index('"')

                    j = resultfolder.index('"', i+1)
                    resultfolder =  resultfolder[i+1:j]
                    resultfolder = os.path.dirname(resultfolder)
                except Exception, e:
                    resultfolder = "N/A"
                web_result['resultfolder'] = resultfolder
                web_result['action'] = 'update'
                return web_result

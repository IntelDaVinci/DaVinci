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
import xlrd
from generate import copyFolder, copyFile

reload(sys) 
sys.setdefaultencoding('utf-8')

if __name__ == "__main__":
    try:
        summary_file = sys.argv[1]
        if not os.path.exists(summary_file):
            print "summary xlsx %s does not exist." % summary_file
            sys.exit(-1)

        log_folder = os.path.abspath(sys.argv[2])
        if not os.path.exists(log_folder):
            print "_Logs folder %s does not exist." % log_folder
            sys.exit(-1)

        dest_folder = os.path.abspath(sys.argv[3])
        if not os.path.exists(dest_folder):
            os.mkdir(dest_folder)

        # step 1: read summary xlsx file
        xl_workbook = xlrd.open_workbook(summary_file)
        timestamp_folders = []
        apk_names =[]

        # fail app
        failed_app_sheet = 2
        warning_app_sheet = 3
        untest_app_sheet = 4

        total_len = 4
        timestamp_index = 2

        xl_sheet = xl_workbook.sheet_by_index(failed_app_sheet)
        num_cols = xl_sheet.ncols
        for row_idx in range(0, xl_sheet.nrows):
            for col_idx in range(0, num_cols):
                cell_obj = xl_sheet.cell(row_idx, col_idx)
                if ("_Logs" in cell_obj.value):
                    log_names = cell_obj.value.split('\\')
                    if (len(log_names) == total_len):
                        timestamp_folders.append(log_names[timestamp_index])
            apk_names.append(xl_sheet.cell(row_idx, 0).value.encode('gbk'))

        # warning app
        xl_sheet = xl_workbook.sheet_by_index(warning_app_sheet)
        num_cols = xl_sheet.ncols
        for row_idx in range(0, xl_sheet.nrows):
            for col_idx in range(0, num_cols):
                cell_obj = xl_sheet.cell(row_idx, col_idx)
                if ("_Logs" in cell_obj.value):
                    log_names = cell_obj.value.split('\\')
                    if (len(log_names) == total_len):
                        timestamp_folders.append(log_names[timestamp_index])
            apk_names.append(xl_sheet.cell(row_idx, 0).value.encode('gbk'))

        # untest app
        xl_sheet = xl_workbook.sheet_by_index(untest_app_sheet)
        num_cols = xl_sheet.ncols
        for row_idx in range(0, xl_sheet.nrows):
            for col_idx in range(0, num_cols):
                cell_obj = xl_sheet.cell(row_idx, col_idx)
                if ("_Logs" in cell_obj.value):
                    log_names = cell_obj.value.split('\\')
                    if (len(log_names) == total_len):
                        timestamp_folders.append(log_names[timestamp_index])
            apk_names.append(xl_sheet.cell(row_idx, 0).value.encode('gbk'))

        # step 2: go through _Logs folder and copy folder
        for child in os.listdir(log_folder):
            if child in timestamp_folders:
                src = os.path.join(log_folder, child)
                dest = os.path.join(dest_folder, child)
                copyFolder(src, dest)

        # step 3: copy applicaitons 
        apk_folder = os.path.dirname(log_folder)
        for child in os.listdir(apk_folder):
            if child in apk_names:
                src = os.path.join(apk_folder, child)
                dest = os.path.join(dest_folder, child)
                copyFile(src, dest)

        print "\nCopy log folder for failed cases done, please check %s folder" % dest_folder

    except Exception as ex:
        print("\nFailed to copy log folder: %s" % str(ex))




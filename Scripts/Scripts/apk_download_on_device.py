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
#encoding:utf-8
import os
import sys
import shutil
import time

result_log = ['APK_Name,','Result,','VersionCode,','Version\n']

result_file = 'result.csv'
davinci_path = r'C:\Release'

def uninstall_apk(apk_list):
    for name in apk_list:
        uninstall_cmd = davinci_path + '\\platform-tools\\adb  uninstall ' + name
        # print uninstall_cmd
        os.system(uninstall_cmd)
        time.sleep(2)

def generate_download_qs(apk_list, template, save_dir):
    with open(template) as f:
        qs_content = f.readlines()

    for n in apk_list:
        update_content = []
        for i in qs_content:
            if  i.find('OPCODE_SET_TEXT') != -1:
                # print i[i.rfind(' '):]
                i = i.replace(i[i.rfind(' ') + 1:], n + '\n')
                # print i
            update_content.append(i)
            with open(save_dir + '\\' + n  + '.qs', 'w') as q:
                q.writelines(update_content)


def create_apk_folder_with_timestamp():
    dest_folder_path = os.getcwd() + '\\'+  time.strftime('%Y_%m_%d_%H_%M_%S',time.localtime(time.time()))
    if os.path.exists(dest_folder_path) == True:
        shutil.rmtree(dest_folder_path)
    os.mkdir(dest_folder_path)
    os.mkdir(dest_folder_path + '\\qscript')
    shutil.copy('appdownload.xml', dest_folder_path + '\\qscript')
    return dest_folder_path


def download_apk(apk_list, qs_dir):
    qs_name = [qs_dir + '\\' + i.strip() + '.qs' for i in apk_list]
    for qs in qs_name:
        cmd = davinci_path + "\\DaVinci.exe" + ' -p ' + qs
        os.popen(cmd)

def get_version_info(file_path):
    cmd = davinci_path + "\\platform-tools\\aapt d badging " + file_path
    content = os.popen(cmd).readlines()
    # name = content[0].split("'")[1]
    try:
        data = content[0].split("'")
    except Exception, e:
        pass
    else:
        if len(data) >= 5:
            return data[3] + ',' + data[5] #versionCode + ',' + versionName

def fetch_apk(apk_list, save_folder):
    retry_time = 5
    while retry_time > 0 and apk_list != []:
        retry_time = retry_time - 1
        print 'retry_time is %d'%retry_time
        apk_name = apk_list[:]
        for name in apk_name:
            print '>>>>>' + name
            cmd = davinci_path + '\\platform-tools\\adb pull /data/app/' + name + '-1/base.apk'+ ' ' + save_folder + '\\' + name + "-1.apk"
            r = os.system(cmd)
            if not os.path.exists(save_folder + '\\' + name + "-1.apk"):
                cmd = davinci_path + '\\platform-tools\\adb pull /data/app/' + name + '-1.apk'+ ' ' + save_folder
                # print cmd
                r = os.system(cmd)
            print cmd
            apk_file_prefix = save_folder + '\\qscript\\' + name

            if os.path.exists(apk_file_prefix + '.qs'):
                os.remove(apk_file_prefix + '.qs')
            if r == 0:
                #print save_folder + '\\qscript\\' + name[:-6] + '.qs'
                info = name + ',succ,' + get_version_info(save_folder + '\\' + name + '-1.apk') + '\n'
                print info
                result_log.append(info)
                if os.path.exists(apk_file_prefix + '.qs'):
                    os.remove(save_folder + '\\qscript\\' + name + '.qs')
                if os.path.exists(apk_file_prefix + '_replay.avi'):
                    os.remove(apk_file_prefix+ '_replay.avi')
                if os.path.exists(apk_file_prefix + '_replay.qts'):
                    os.remove(apk_file_prefix + '_replay.qts')
                apk_list.remove(name)
                time.sleep(3)
                print '<<<<< ' + str(len(apk_list))
        #     else:
        #         continue

        # print '************'
        # print len(apk_list)
    for i in apk_list:
        result_log.append(i  + 'fail')

    with open(save_folder + '\\' + result_file, 'w') as f:
        f.writelines(result_log)


if __name__ == '__main__':

    apks_file_path = r'apklist.txt'

    apk_list = []
    if not os.path.exists(apks_file_path):
        print "apk list file doesn't exist!"
        sys.exit(0)

    with open(apks_file_path, 'r') as f:
        apk_list = f.readlines()

    apk_list = [i.strip() for i in apk_list if i.strip() != '']

    apk_folder = create_apk_folder_with_timestamp()
    generate_download_qs(apk_list, "appdownload.qs", apk_folder + "\\qscript")

    uninstall_apk(apk_list)
    print 'Start to download APK...'
    download_apk(apk_list,apk_folder + "\\qscript")
    print 'Please wait %d seconds for downloading finish...'%120*len(apk_list)
    time.sleep(120*len(apk_list))

    fetch_apk(apk_list, apk_folder)
    time.sleep(10)

    print 'Done'
    sys.exit(0)
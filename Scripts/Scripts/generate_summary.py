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
import sys, os
import codecs
import shutil
import xlsxwriter
import time
import datetime
from xml.etree import ElementTree
from generate import getApkPackageName
from collections import OrderedDict
from apkhelper import get_string_hash

def copyFile(src, dest):
    try:
        shutil.copy(src, dest)
    except Exception as e:
        print('')


def ContainNoneASCII(apkName):
    for ch in apkName:
        if ch > u'\u007f':
            return True
    return False


def getAppVersion(davincifolder, apk_path):
    apk_version = ""
    cmd = davincifolder + "\\platform-tools\\aapt.exe dump badging \"" + apk_path + "\""
    cmd = cmd.encode("utf-8")
    apk_info = os.popen(cmd).readlines()
    if (len(apk_info) >= 1):
        apk_detail_info = apk_info[0].split("versionName='")
        if (len(apk_detail_info) >= 2):
            apk_detail_version = apk_detail_info[1].strip().split("'")
            if (apk_detail_version >= 1):
                apk_version = apk_detail_version[0].strip()
    apk_version = apk_version.replace(",",".")
    return apk_version


def getAllDeviceInfo(davincifolder, config_file_name):
    all_devices_info = []

    dev_config_file = open(config_file_name, "r")
    for line in dev_config_file:
        dev_info = line.split('<>*<>')
        if len(dev_info) < 2:
            continue
        if dev_info[0].strip() == '[target_dev]' or dev_info[0].strip() == '[ref_dev]':
            one_device_info = {}
            one_device_info['device_id'] = dev_info[1].strip()
            if (len(dev_info) > 8):
                one_device_info['android_version'] = dev_info[5].strip()
                one_device_info['device_model'] = dev_info[6].strip()
                one_device_info['device_build_number'] = dev_info[7].strip()
                one_device_info['cpu'] = dev_info[8].strip()
                one_device_info['houdini_version'] = dev_info[9].strip()
            else:
                one_device_info['android_version'] = ""
                one_device_info['device_model'] = ""
                one_device_info['device_build_number'] = ""
                one_device_info['cpu'] = ""
                one_device_info['houdini_version'] = ""
            device_group=""
            if (len(dev_info) > 3):
                device_group = dev_info[3].strip()
            one_device_info['device_group'] = device_group

            if dev_info[0].strip() == '[target_dev]':
                one_device_info['target_or_reference'] = "target"
                all_devices_info.append(one_device_info)
            if dev_info[0].strip() == '[ref_dev]':
                 one_device_info['target_or_reference'] = "reference"
                 all_devices_info.append(one_device_info)
    dev_config_file.close()
    return all_devices_info


def haveReferenceDevice(all_devices_info):
    have_reference = False
    for dev in all_devices_info:
        if dev['target_or_reference'] == "reference":
            have_reference = True
    return have_reference


def isReferenceDevice(all_devices_info, device_id):
    is_reference_device = False
    for dev in all_devices_info:
        if (dev['device_id'] == device_id):
            if (dev['target_or_reference']== "target"):
                return False
            else:
                return True
    return is_reference_device


def getDeviceGroup(all_devices_info, device_id):
    device_group = ""
    for dev in all_devices_info:
        if (dev['device_id'] == device_id):
            return dev['device_group']
    return device_group


def getAllVerInfo(apk_folder):
    all_rank_info = []
    rank_file = open(apk_folder + "\\apk_version_info.txt", "r")
    first_line = True
    for line in rank_file:
        if line.strip() == "":
            continue
        if first_line:
            first_line = False
            continue
        rank_info = line.split('|---|')
        # skip bad line
        if len(rank_info) < 3:
            continue
        if len(rank_info) == 3:
            one_rank_info = {}  
            one_rank_info['apk_name'] = rank_info[0].strip()
            one_rank_info['version'] = rank_info[2].strip()        
            all_rank_info.append(one_rank_info)
            
    rank_file.close()
    return all_rank_info


def getAllRankInfo(apk_folder):
    apk_name_index = 1
    rank_index = 2
    version_index = 3
    min_len = 3

    all_rank_info = []
    rank_file = open(apk_folder + "\\version_info.txt", "r")
    first_line = True
    for line in rank_file:
        if line.strip() == "":
            continue
        if first_line:
            first_line = False
            continue
        rank_info = line.split('|---|')
        # skip bad line
        if len(rank_info) < min_len:
            continue
        one_rank_info = {}
        one_rank_info['apk_name'] = rank_info[apk_name_index].strip()
        one_rank_info['rank'] = rank_info[rank_index].strip() + " rank " + rank_info[0].strip()
        if len(rank_info) >= version_index + 1:
            one_rank_info['version'] = rank_info[version_index].strip()

        all_rank_info.append(one_rank_info)
            
    rank_file.close()
    return all_rank_info


def get_one_info(all_rank, package_name):
    rank = ""
    version = ""
    for rank_info in all_rank:
        if rank_info['apk_name'] == package_name.encode("utf-8"):
            if "rank" in rank_info:
                rank = rank_info['rank']
            version = rank_info['version']
    return rank, version


def get_rank_info(all_rank, apk_name):
    rank = ""
    if (len(all_rank) == 0):
        return rank
    for rank_info in all_rank:
        apk_file = rank_info['apk_name'] + ".apk"
        if apk_file == apk_name.encode("utf-8"):
            return rank_info['rank']
    return rank


def getPlayStoreInfo(apk_folder):
    play_store_info = ""
    rank_file = open(apk_folder + "\\version_info.txt", "r")
    first_line = True
    store_info = []
    for line in rank_file:
        if first_line:
            store_info = line.split('|')
            first_line = False

    if len(store_info) < 4:
        return play_store_info
    play_store_info = store_info[0] + " play store rank(" + store_info[3].strip() + ")"
    return play_store_info


def splitTargetResultRefResult(apk_result, num, all_devices_info, reference_results):
    target_results = []
    local_apk_result = []
    google_apk_result = []

    for result in apk_result:
        device_id = result[1].strip()
        result_reason_link = {}

        result_reason_link['each_stage'] = ",".join((result[num + i].strip() for i in range(0, 5)))
        result_reason_link['result'] = result[num + 5].strip()
        result_reason_link['reason'] = result[num + 6].strip()
        result_reason_link['link'] = result[num + 7].strip()
        if (result_reason_link['reason'].strip().lower() == "device connection error"):
            result_reason_link['link'] = result_reason_link['link'].replace("ReportSummary.xml", "DaVinci.log.txt")
        result_reason_link['login'] = result[num + 8].strip()

        if (len(result) > num + 12):
            result_reason_link['description'] = result[num + 12].strip()
        else:
            result_reason_link['description'] = 'Smoke'

        if (isReferenceDevice(all_devices_info, device_id) == False):
            if (len(result) > num + 11 and result[num + 11].lower().strip() == "yes"):
                google_apk_result.append(result_reason_link)
            else:
                local_apk_result.append(result_reason_link)
        else:
            reference_results.append(result_reason_link)

    # if have test result of the apk which is downloading from google, skip the result of local apk 
    if (len(google_apk_result) > 0):
        target_results = google_apk_result
    else:
        target_results = local_apk_result

    return target_results

def generateMergeResult(target_pass_number, target_fail_number, target_skip_number, target_warning_number, pass_criteria, total_test_number, reference_pass_number, reference_fail_number, reference_warning_number):
    # if the first result is PASS, needn't check pass criteria
    merge_result = ""
    if target_pass_number == 1 and target_fail_number == 0 and target_skip_number == 0 and target_warning_number == 0:
        merge_result = "PASS"
    else:
        # merge targe result, check with pass criteria
        if (target_pass_number >= pass_criteria):
            merge_result = "PASS"
        elif ((target_pass_number != 0) and (target_pass_number == total_test_number)):
            merge_result = "PASS"
        else:
            if (target_fail_number > 0):
                merge_result = "FAIL"
            else:
                if(target_warning_number > 0):
                    merge_result = "WARNING"
                else:
                    merge_result = "SKIP"

    # merge reference result
    ref_merge_result = ""
    if (reference_pass_number > 0):
        ref_merge_result = "PASS"
    else:
        if (reference_fail_number > 0):
            ref_merge_result = "FAIL"
        else:
            if(reference_warning_number > 0):
                ref_merge_result = "WARNING"
            else:
                ref_merge_result = "SKIP"
    return merge_result, ref_merge_result

def generateLoginResult(login_pass_number, login_fail_number, login_skip_number, pass_criteria):
    login_result = ""
    total_login_number = login_pass_number + login_fail_number + login_skip_number
    if (total_login_number == 1):
        # only one record
        if (login_pass_number == 1):
            login_result = "PASS"
        elif (login_fail_number == 1):
            login_result = "FAIL"
        else:
            login_result = "SKIP"
    else:   
        # more than 1 record, need to check pass criteria
        if (login_pass_number >= pass_criteria):
            login_result = "PASS"
        elif (login_fail_number > 0):
            login_result = "FAIL"
        else:
            login_result = "SKIP"
    return login_result

def dealFinalResult(apk_result, num, all_devices_info, final_result_reason_link, total_test_times, pass_criteria):
    target_results = []
    reference_results = []
    
    target_results = splitTargetResultRefResult(apk_result, num, all_devices_info, reference_results)

    target_fail_number = 0
    target_pass_number = 0
    target_skip_number = 0
    target_invalid_skip_number = 0
    target_warning_number = 0
    reference_fail_number = 0
    reference_pass_number = 0
    reference_skip_number = 0
    reference_warning_number = 0

    target_fail_result = []
    target_pass_result = []
    target_skip_result = []
    target_invalid_skip_result = []
    target_warning_result = []
    final_result_reason_link['link'] = ""

    login_pass_number = 0
    login_fail_number = 0
    login_skip_number = 0

    valid_skip_reason = ["activity name is empty"]
    invalid_skip_reason = ["", "camera-less error", "device connection error"]

    for each_result in target_results:
        final_result_reason_link['link'] += each_result['link'] + ","

        if (each_result['login'].lower().strip() == "pass"):
            login_pass_number = login_pass_number + 1
        if (each_result['login'].lower().strip() == "fail"):
            login_fail_number = login_fail_number + 1
        if (each_result['login'].lower().strip() == "skip"):
            login_skip_number = login_skip_number + 1

        if (each_result['result'].lower().strip() == "pass"):
            target_pass_number = target_pass_number + 1
            target_pass_result.append(each_result)
        if (each_result['result'].lower().strip() == "fail"):
            target_fail_number = target_fail_number + 1
            target_fail_result.append(each_result)
        if (each_result['result'].lower().strip() == "skip" and each_result['reason'].lower().strip() in valid_skip_reason):   # if DaVinci result is SKIP and reason is 'activity is empty'
            target_skip_number = target_skip_number + 1
            target_skip_result.append(each_result)
        if (each_result['result'].lower().strip() == "skip" and each_result['reason'].lower().strip() in invalid_skip_reason):   # invalid result, final result is SKIP and no failed reason or camera-less error ...
            target_invalid_skip_number = target_invalid_skip_number + 1
            target_invalid_skip_result.append(each_result)
        if (each_result['result'].lower().strip() == "warning"):
            target_warning_number = target_warning_number + 1
            target_warning_result.append(each_result)

    for each_result in reference_results:
        final_result_reason_link['link'] += each_result['link'] + ","
        if (each_result['result'].lower().strip() == "pass"):
            reference_pass_number = reference_pass_number + 1
        if (each_result['result'].lower().strip() == "fail"):
            reference_fail_number = reference_fail_number + 1
        if (each_result['result'].lower().strip() == "skip"):
            reference_skip_number = reference_skip_number + 1
        if (each_result['result'].lower().strip() == "warning"):
            reference_warning_number = reference_warning_number + 1

    pass_rate = " " + str(target_pass_number) + "/" + str(target_pass_number + target_fail_number +
                                                          target_skip_number + target_warning_number)
    total_test_number = target_pass_number + target_fail_number + target_skip_number + target_warning_number

    merge_result, ref_merge_result = generateMergeResult(target_pass_number, target_fail_number, target_skip_number, target_warning_number, pass_criteria, total_test_number, reference_pass_number, reference_fail_number, reference_warning_number)

    final_result_reason_link['pass_rate'] = pass_rate
    final_result_reason_link['result'] = ""
    final_result_reason_link['each_stage'] = ""
    final_result_reason_link['reason'] = ""

    if (merge_result == "PASS"):
        final_result_reason_link['result'] = "PASS"
        final_result_reason_link['each_stage'] = target_pass_result[0]['each_stage']
        final_result_reason_link['reason'] = target_pass_result[0]['reason']

    if (merge_result == "FAIL"):
        final_result_reason_link['each_stage'] = target_fail_result[0]['each_stage']
        if (ref_merge_result == "FAIL"):
            final_result_reason_link['result'] = "Bad App"
            final_result_reason_link['reason'] = "Also fail on reference device"
        else:
            final_result_reason_link['result'] = "FAIL"
            final_result_reason_link['reason'] = target_fail_result[0]['reason']

    if merge_result == "WARNING":
        final_result_reason_link['each_stage'] = target_warning_result[0]['each_stage']
        if (ref_merge_result == "FAIL"):
            final_result_reason_link['result'] = "Bad App"
            final_result_reason_link['reason'] = "Also fail on reference device"
        else:
            final_result_reason_link['result'] = "WARNING"
            final_result_reason_link['reason'] = target_warning_result[0]['reason']

    if merge_result == "SKIP":
        # We have two kinds of SKIP, the first one is activity name empty, the second one is invalid result
        if (len(target_skip_result) > 0):
            final_result_reason_link['result'] = "SKIP"
            final_result_reason_link['each_stage'] = target_skip_result[0]['each_stage']
            final_result_reason_link['reason'] = target_skip_result[0]['reason']
        elif (len(target_invalid_skip_result) > 0):
            final_result_reason_link['result'] = "SKIP"
            final_result_reason_link['each_stage'] = target_invalid_skip_result[0]['each_stage']
            final_result_reason_link['reason'] = target_invalid_skip_result[0]['reason']

    # set login result
    final_result_reason_link["login"] = generateLoginResult(login_pass_number, login_fail_number, login_skip_number, pass_criteria)    

    if (len(target_results) > 0):
        final_result_reason_link['description'] = target_results[0]['description']
    else:
        final_result_reason_link['description'] = ""
    return True


def get_apk_path(apk_folder, qscript_name):
    apk_path = ""
    qscript_file = open(apk_folder + "\\" + qscript_name, "r")
    for line in qscript_file:
        if (line.lower().startswith("apkname=")):
            apk_path = line[8:].strip()
            break
    qscript_file.close()
    if (apk_path.find("\\") < 0):
        apk_path = apk_folder + "\\" + apk_path
    return apk_path


def finalResultForOneApk(apk_result, all_devices_info, append_rank, all_rank, have_reference, davinci_folder, apk_folder, total_test_times, pass_criteria, apk_list):
    one_final_result = ""
    if (len(apk_result) < 1):
        return ""

    # the first line is the result of target device
    target_device_id = apk_result[0][1].strip()

    # get target device information
    android_version = ""
    device_model = ""
    device_build_number = ""
    android_version = ""
    cpu = ""
    houdini_version = ""
    for dev in all_devices_info:
        if dev['device_id'] == target_device_id:
            android_version = dev['android_version']
            device_model = dev['device_model']
            device_build_number = dev['device_build_number']
            android_version = dev['android_version']
            cpu = dev['cpu']
            houdini_version = dev['houdini_version']

    # get apk information
    apk_name = apk_result[0][2].strip().decode("utf-8")
    app_name = apk_result[0][3].strip().decode("utf-8")
    package_name = apk_result[0][4].strip().decode("utf-8")
    qscript_name = apk_result[0][15].strip().decode("utf-8")
    apk_path = apk_folder + "\\" + apk_name
    if os.path.exists(apk_list):
        # if use apk list mode, need to reset apk_path according to the file
        with open(apk_list, 'r') as f:
            for line in f.readlines():
                if ("\\" + apk_name) in line.decode('utf-8'):
                    apk_path = line.decode('utf-8').strip()
    if apk_name <> "":
        app_version = ""
        if ContainNoneASCII(apk_name):
            nonascii_apk_folder = os.path.join(apk_folder, 'nonasciiapk')
            if not os.path.exists(nonascii_apk_folder):
                os.mkdir(nonascii_apk_folder)
            tmp_apk_name = os.path.join(nonascii_apk_folder, get_string_hash(apk_name) + '.apk')
            if not os.path.exists(tmp_apk_name):
                copyFile(apk_path, tmp_apk_name)
            app_version = getAppVersion(davinci_folder, tmp_apk_name).decode('utf-8')
        else:
            app_version = getAppVersion(davinci_folder, apk_path).decode('utf-8')

        # get rank
        app_rank = ""
        if (append_rank):
            app_rank = get_rank_info(all_rank, apk_name)
    else:
        app_rank, app_version = get_one_info(all_rank, package_name)
        

    # get final result, reason, link
    final_result_reason_link = {}
    if not dealFinalResult(apk_result, 6, all_devices_info, final_result_reason_link , total_test_times, pass_criteria):
        return ''

    each_stage_result = final_result_reason_link['each_stage']
    final_result = final_result_reason_link['result']
    reason = final_result_reason_link['reason']
    link = final_result_reason_link['link']
    pass_rate = final_result_reason_link['pass_rate']
    login_result = final_result_reason_link['login']
    test_description = final_result_reason_link['description']

    if (append_rank):
        str_final_result = apk_name + "," + app_name + "," + package_name + ","  + device_model + "," + cpu + ","+ android_version + "," + \
        device_build_number + "," + houdini_version + "," + app_version + "," + app_rank + "," + each_stage_result + "," + final_result+ "," + pass_rate + "," + \
        reason + "," +  login_result + "," + test_description + "," + link + "\n"
    else:
        str_final_result = apk_name + "," + app_name + "," + package_name + "," + device_model + "," + cpu + "," + android_version + "," + \
        device_build_number + "," + houdini_version + "," + app_version + "," + each_stage_result + "," + final_result + "," + pass_rate + "," + \
        reason + "," + login_result + "," + test_description + "," + link + "\n"

    return str_final_result


def getEmptyPackageAPK(apk_folder):
    broken_app = []
    apk_file = apk_folder + "\\DaVinciEmptyPackageName.txt"
    if not os.path.exists(apk_file):
        return broken_app

    lines = open(apk_file, "r")
    for line in lines:
        if line.strip() == "":
            continue
        broken_app.append(line)
    lines.close()
    return broken_app


def insertString(orig_string, insert_string, insert_position):
    total_string = ""
    lines = orig_string.split(',')
    index = 0
    for line in lines:
        if index == insert_position:
            total_string += insert_string + "," + line + ","
        else:
            total_string += line + ","
        index = index + 1
    return total_string


def generateCSVResult(title, total_result, apk_folder, no_result_apk, append_rank, all_rank):
    # write title
    summary_csv = codecs.open(apk_folder + "\\Smoke_Test_Summary.csv", "w", "utf-8")
    summary_csv.write(u'\ufeff')
    summary_csv.write(title)
    for results in total_result:
        summary_csv.write(results)

    if append_rank:
        install_index = 11
    else:
        install_index = 10
    result_index = install_index + 5

    # insert no result app into Smoke_Test_Summary.csv
    for no_result_package in no_result_apk:
        str_no_result = no_result_package["apk_name"] + "," + no_result_package["app_name"] + "," + \
                        no_result_package["package_name"] + "," + no_result_package["device_model"] + "," + \
                        no_result_package["cpu"]
        str_no_result += "," + no_result_package["android_version"] + "," + \
                            no_result_package["device_build_number"] + "," + no_result_package["houdini_version"] +\
                            "," + no_result_package["app_version"]
        i = install_index
        if append_rank:
            str_no_result += "," + get_one_info(all_rank, no_result_package["package_name"])[0]
        while (i <= result_index+1 and i >= install_index):
            str_no_result += "," + "SKIP"
            i = i + 1
        str_no_result += "," + no_result_package["failed_reason"] + "," + "SKIP"
        links = no_result_package["link_folder"].split('|---|')
        if len(links) > 0:
            for link in links:
                str_no_result += "," + link
        str_no_result += "\n"
        summary_csv.write(str_no_result)
    summary_csv.close()

def UpdateLinkOfTotalResult(total_result, dic_apk_name_type, apk_folder):
    update_total_result = []

    # insert apk type in total_result, update link to relative path
    update_link_result = []
    apk_folder_abs = os.path.abspath(apk_folder)
    for results in total_result:
        results = results.replace(apk_folder_abs, "..")
        apk_name = results.split(',')[0].strip()
        if dic_apk_name_type.has_key(apk_name):
            results = insertString(results, dic_apk_name_type[apk_name], 8)
        else:
            results = insertString(results, "", 8)
        update_link_result.append(results)
    update_total_result = update_link_result
    update_total_result = sorted(set(update_total_result), key = update_total_result.index)
    return update_total_result

def getInvalidResult(have_download_result, all_download_info):
    # get invalid apk names and download fail apk names
    invalid_apk_names = []
    download_fail_names = []
    if have_download_result:
        for one_download_info in all_download_info:
            if (one_download_info["result"].lower().strip()) == "invalid":
                invalid_apk_names.append(one_download_info)
            if (one_download_info["result"].lower().strip()) == "fail":
                download_fail_names.append(one_download_info)
    return invalid_apk_names, download_fail_names

def getDifferentResult(append_rank, total_result, final_pass_apk, final_fail_apk, final_warning_apk, final_skip_apk, final_bad_apk, valid_no_result_apk, fail_result, warning_result, bad_app_result, skip_result):
    each_stage_number = {}
    
    if append_rank:
        install_index = 11
    else:
        install_index = 10
    launch_index = install_index + 1
    random_index = install_index + 2
    back_index = install_index + 3
    uninstall_index = install_index + 4
    result_index = install_index + 5

    # calculate pass rate, fail rate
    install_fail_number = 0
    install_warning_number = 0
    launch_fail_number = 0
    launch_warning_number = 0
    random_fail_number = 0
    random_warning_number = 0
    back_fail_number = 0
    back_warning_number = 0
    uninstall_fail_number = 0
    uninstall_warning_number = 0

    pre_apk_name = ''
    for result in total_result:
        detail_results = result.split(',')
        cur_apk_name = detail_results[2]

        if cur_apk_name != pre_apk_name:
            if (detail_results[install_index].lower() == "fail" ):
                install_fail_number = install_fail_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[install_index].lower() == "warning"):
                install_warning_number = install_warning_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[launch_index].lower() == "fail"):
                launch_fail_number = launch_fail_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[launch_index].lower() == "warning"):
                launch_warning_number = launch_warning_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[random_index].lower() == "fail"):
                random_fail_number = random_fail_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[random_index].lower() == "warning"):
                random_warning_number = random_warning_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[back_index].lower() == "fail"):
                back_fail_number = back_fail_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[back_index].lower() == "warning"):
                back_warning_number = back_warning_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[uninstall_index].lower() == "fail"):
                uninstall_fail_number = uninstall_fail_number + 1
                pre_apk_name = cur_apk_name
            elif (detail_results[uninstall_index].lower() == "warning"):
                uninstall_warning_number = uninstall_warning_number + 1
                pre_apk_name = cur_apk_name

        if (detail_results[result_index].lower() == "pass"):
            final_pass_apk.append(detail_results[0])
        if (detail_results[result_index].lower() == "fail"):
            final_fail_apk.append(detail_results[0])
            fail_result.append(result)
        if (detail_results[result_index].lower() == "warning"):
            final_warning_apk.append(detail_results[0])
            warning_result.append(result)
        if (detail_results[result_index].lower() == "skip"):
            final_skip_apk.append(detail_results[0])
            skip_result.append(result)
        if (detail_results[result_index].lower() == "bad app"):
            final_bad_apk.append(detail_results[0])
            bad_app_result.append(result)

    each_stage_number['install_fail_number'] = install_fail_number
    each_stage_number['install_warning_number'] = install_warning_number
    each_stage_number['launch_fail_number'] = launch_fail_number
    each_stage_number['launch_warning_number'] = launch_warning_number
    each_stage_number['random_fail_number'] = random_fail_number
    each_stage_number['random_warning_number'] = random_warning_number
    each_stage_number['back_fail_number'] = back_fail_number
    each_stage_number['back_warning_number'] = back_warning_number
    each_stage_number['uninstall_fail_number'] = uninstall_fail_number
    each_stage_number['uninstall_warning_number'] = uninstall_warning_number

    return each_stage_number

def generateTotalResult(title, total_result, apk_folder, append_rank, report_title, have_download_result, all_download_info, dic_apk_name_type, no_result_apk, start_time, all_rank, warning_as_pass):
    try:
        summary_file_name = apk_folder + "\\Smoke_Test_Summary.xlsx"
        book = xlsxwriter.Workbook(summary_file_name)
        sheet1 = book.add_worksheet("summary")
        sheet2 = book.add_worksheet("all test result")
        sheet3 = book.add_worksheet("failed test result")
        sheet4 = book.add_worksheet("warning test result")
        sheet5 = book.add_worksheet("untest result")

        bold = book.add_format({'bold': True})

        # write title
        title = insertString(title, "App Type", 8)
        title_contents = title.split(',')
        title_number = 0
        for title_content in title_contents:
            sheet2.write(0, title_number, title_content.strip(), bold)
            sheet3.write(0, title_number, title_content.strip(), bold)
            sheet4.write(0, title_number, title_content.strip(), bold)
            sheet5.write(0, title_number, title_content.strip(), bold)
            title_number = title_number + 1
    
        total_result = UpdateLinkOfTotalResult(total_result, dic_apk_name_type, apk_folder)
        apk_folder_abs = os.path.abspath(apk_folder)

        # write total result into sheet2
        writeToSheet(sheet2, total_result)

        # get broken apk names, invalid apk names
        broken_apk_names = getEmptyPackageAPK(apk_folder)
        invalid_apk_names, download_fail_names = getInvalidResult(have_download_result, all_download_info)

        invalid_apk_number = len(invalid_apk_names)
        download_fail_number = len(download_fail_names)
        broken_apk_number = len(broken_apk_names)

        each_stage_number = {}
        final_pass_apk = []
        final_fail_apk = []
        final_warning_apk = []
        final_skip_apk = []
        final_bad_apk = []
        valid_no_result_apk = []

        fail_result = []
        warning_result = []
        bad_app_result = []
        skip_result = []
        each_stage_number = getDifferentResult(append_rank, total_result, final_pass_apk, final_fail_apk, final_warning_apk, \
            final_skip_apk, final_bad_apk, valid_no_result_apk, fail_result, warning_result, bad_app_result, skip_result)

        # sheet 3 contains fail app and bad app
        fail_result.extend(bad_app_result)
        writeToSheet(sheet3, fail_result)
        writeToSheet(sheet4, warning_result)
        writeToSheet(sheet5, skip_result)

        # remove duplicated apk
        final_bad_apk = set(final_bad_apk) - set(final_fail_apk)
        final_fail_number = len(set(final_fail_apk)) + len(set(final_bad_apk))        # consider bad app as failed app

        final_warning_apk = set(final_warning_apk) - set(final_bad_apk) - set(final_fail_apk)
        final_warning_number = len(set(final_warning_apk))

        final_pass_apk = set(final_pass_apk) - set(final_warning_apk) - set(final_bad_apk) - set(final_fail_apk)
        final_pass_number = len(set(final_pass_apk))

        final_skip_apk = set(final_skip_apk) - set(final_pass_apk) - set(final_warning_apk) - set(final_bad_apk) - set(final_fail_apk)
        final_skip_number = len(set(final_skip_apk))

        for app in no_result_apk:
            valid_no_result_apk.append(app["apk_name"])
        valid_no_result_apk = set(valid_no_result_apk) - set(final_skip_apk) - set(final_pass_apk) - set(final_warning_apk) - set(final_bad_apk) - set(final_fail_apk)
        no_result_number = len(valid_no_result_apk)

        final_untest_number = final_skip_number + invalid_apk_number + download_fail_number + \
                              broken_apk_number + no_result_number
        # Subtract download failed number from total test number
        total_apk_number = (float)(final_pass_number + final_fail_number + final_warning_number +
                                   final_untest_number - download_fail_number - invalid_apk_number)

        if total_apk_number > 0:
            pass_rate = round(final_pass_number * 100 / total_apk_number, 2)
            fail_rate = round(final_fail_number * 100 / total_apk_number, 2)
            warning_rate = round(final_warning_number * 100 / total_apk_number, 2)
            untest_rate = round(final_untest_number * 100 / total_apk_number, 2)
        else:
            pass_rate = 0.00
            fail_rate = 0.00
            warning_rate = 0.00
            untest_rate = 0.00

        if (final_skip_number > 0):
            append_row = final_skip_number
        else:
            append_row = 0

        if append_rank:
            install_index = 11
        else:
            install_index = 10
        result_index = install_index + 5

        if (have_download_result):
            # append invalid app names
            for invalid_apk_name in invalid_apk_names:
                append_row = append_row + 1
                sheet5.write(append_row,0, invalid_apk_name["apk_name"])
                i = 1
                while (i < install_index):
                    sheet5.write(append_row,i, "N/A")
                    i = i + 1
                while (i <= result_index+1):
                    sheet5.write(append_row,i, "SKIP")
                    i = i + 1
                sheet5.write(append_row, i, "Invalid url")
                sheet5.write(append_row, i + 1, "SKIP")     # For login result
                sheet5.write_string(append_row, i + 2, invalid_apk_name["url"])

            ## append download failed app names
            for download_fail_apk_name in download_fail_names:
                append_row = append_row + 1
                sheet5.write(append_row,0, download_fail_apk_name["apk_name"])
                i = 1
                while (i < install_index):
                    sheet5.write(append_row,i, "N/A")
                    i = i + 1
                while (i <= result_index+1):
                    sheet5.write(append_row,i, "SKIP")
                    i = i + 1
                sheet5.write(append_row,i, "Download fail")
                sheet5.write(append_row,i + 1, "SKIP")
                sheet5.write_string(append_row,i + 2, download_fail_apk_name["url"])

        if (len(broken_apk_names) > 0):
            for broken_apk in broken_apk_names:
                append_row = append_row + 1
                sheet5.write(append_row,0, broken_apk.decode("utf-8"))
                i = 1
                while (i < install_index):
                    sheet5.write(append_row,i, "N/A")
                    i = i + 1
                while (i <= result_index+1):
                    sheet5.write(append_row,i, "SKIP")
                    i = i + 1
                sheet5.write(append_row,i, "Broken apk")

        for no_result_package in no_result_apk:
            append_row = append_row + 1
            i = 0
            sheet5.write(append_row, i, no_result_package["apk_name"])
            sheet5.write(append_row, i + 1, no_result_package["app_name"])
            sheet5.write(append_row, i + 2, no_result_package["package_name"])
            sheet5.write(append_row, i + 3, no_result_package["device_model"])
            sheet5.write(append_row, i + 4, no_result_package["cpu"])
            sheet5.write(append_row, i + 5, no_result_package["android_version"])
            sheet5.write(append_row, i + 6, no_result_package["device_build_number"])
            sheet5.write(append_row, i + 7, no_result_package["houdini_version"])
            sheet5.write(append_row, i + 8, "N/A")
            sheet5.write(append_row, i + 9, no_result_package["app_version"])
            if (append_rank):
                sheet5.write(append_row,i + 10, get_one_info(all_rank, no_result_package["package_name"])[0])
                i = i + 11
            else:
                i = i + 10

            while i <= result_index + 1 and i >= install_index:
                sheet5.write(append_row,i, "SKIP")
                i = i + 1
            sheet5.write(append_row, i, no_result_package["failed_reason"])
            sheet5.write(append_row, i + 1, "SKIP")
            sheet5.write(append_row, i + 2, no_result_package["test_description"])  # test description
            i = i + 3
            links = no_result_package["link_folder"].split('|---|')
            if (len(links) > 0):
                seen = []
                for link in links:
                    if (link == ""):
                        continue
                    # If APK both timeout and no result, two same links will be recorded,
                    # so need to filter the duplicate line.
                    if link in seen:
                        continue
                    seen.append(link)
                    link = link.replace(apk_folder_abs, "..")   # use relative path
                    sheet5.write_url(append_row, i, "external:" + link)
                    i = i + 1

        sheet1.write(0,0,report_title,bold)
        sheet1.write(1,0,"#Total App")
        sheet1.write(1,1,total_apk_number)

        row = 2
        sheet1.write(row,0,"#Pass")
        sheet1.write_number(row,1,final_pass_number)
        sheet1.write(row,2,"Rate")
        sheet1.write(row,3,str(pass_rate) + "%")

        row = row + 1
        sheet1.write(row,0,"#Fail")
        sheet1.write_number(row,1,final_fail_number)
        sheet1.write(row,2,"Rate")
        sheet1.write(row,3,str(fail_rate) + "%")

        row = row + 1
        sheet1.write(row,0,"#Warning")
        sheet1.write_number(row,1,final_warning_number)
        sheet1.write(row,2,"Rate")
        sheet1.write(row,3,str(warning_rate) + "%")

        row = row + 1
        sheet1.write(row,0,"#Untest")
        sheet1.write_number(row,1,final_untest_number)
        sheet1.write(row,2,"Rate")
        sheet1.write(row,3,str(untest_rate) + "%")

        if final_fail_number > 0:
            row = row + 2
            sheet1.write(row,0,"Fail Distribution", bold)

            row = row + 1
            install_fail_rate = round(each_stage_number['install_fail_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Install Fail")
            sheet1.write(row, 1, each_stage_number['install_fail_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(install_fail_rate) + "%")

            row = row + 1
            launch_fail_rate = round(each_stage_number['launch_fail_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Launch Fail")
            sheet1.write(row, 1, each_stage_number['launch_fail_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(launch_fail_rate) + "%")

            row = row + 1
            random_fail_rate = round(each_stage_number['random_fail_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Random Fail")
            sheet1.write(row, 1, each_stage_number['random_fail_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(random_fail_rate) + "%")

            row = row + 1
            back_fail_rate = round(each_stage_number['back_fail_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Back Fail")
            sheet1.write(row, 1, each_stage_number['back_fail_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(back_fail_rate) + "%")

            row = row + 1
            uninstall_fail_rate = round(each_stage_number['uninstall_fail_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Uninstall Fail")
            sheet1.write(row, 1, each_stage_number['uninstall_fail_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(uninstall_fail_rate) + "%")

        if (final_warning_number > 0):
            row = row + 2
            sheet1.write(row, 0, "Warning Distribution", bold)

            row = row + 1
            install_warning_rate = round(each_stage_number['install_warning_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Install warning")
            sheet1.write(row, 1, each_stage_number['install_warning_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(install_warning_rate) + "%")

            row = row + 1
            launch_warning_rate = round(each_stage_number['launch_warning_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Launch warning")
            sheet1.write(row, 1, each_stage_number['launch_warning_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(launch_warning_rate) + "%")

            row = row + 1
            random_warning_rate = round(each_stage_number['random_warning_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Random warning")
            sheet1.write(row, 1, each_stage_number['random_warning_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(random_warning_rate) + "%")

            row = row + 1
            back_warning_rate = round(each_stage_number['back_warning_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Back warning")
            sheet1.write(row, 1, each_stage_number['back_warning_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(back_warning_rate) + "%")

            row = row + 1
            uninstall_warning_rate = round(each_stage_number['uninstall_warning_number']*100/total_apk_number, 2)
            sheet1.write(row, 0, "#Uninstall warning")
            sheet1.write(row, 1, each_stage_number['uninstall_warning_number'])
            sheet1.write(row, 2, "Rate")
            sheet1.write(row, 3, str(uninstall_warning_rate) + "%")

        end_time = time.strftime('%Y-%m-%d %H:%M:%S',time.localtime())
        duration = measureTestDuration(start_time, end_time)
        if (duration != ''):
            row = row + 2
            sheet1.write(row,0,"Test Duration", bold)
            sheet1.write(row,1, str(duration) + " minutes")

        if (total_apk_number > 0):
            chart1 = book.add_chart({'type': 'pie'})

            # Configure the series. Note the use of the list syntax to define ranges:
            chart1.add_series({
                'name':       'Number',
                'categories': '=summary!$A$3:$A$6',
                'values':     '=summary!$B$3:$B$6',
                'points': [
                    {'fill': {'color': '#008800'}},
                    {'fill': {'color': '#CC0000'}},
                    {'fill': {'color': '#FF8800'}},
                    {'fill': {'color': '#C9C9C9'}},
                    {'fill': {'color': '#00BBFF'}},
                    ],
                'data_labels': {'value': True},
            })

            # Add a title.
            chart1.set_title({'name': 'Test Result Distribution'})

            # Insert the chart into the worksheet (with an offset).
            sheet1.insert_chart('E2', chart1, {'x_offset': 35, 'y_offset': 15})

        book.close()
    except Exception as ex:
        print str(ex)


def measureTestDuration(start_time, end_time):
    try:
        tFormat = '%Y-%m-%d %H:%M:%S'
        duration = datetime.datetime.strptime(end_time, tFormat) - datetime.datetime.strptime(start_time, tFormat)
        return duration.days * 24 * 60 + duration.seconds / 60
    except Exception as e:
        print e.message
        return ''


def writeToSheet(sheet, total_result):
    total_row_number = 1
    for result in total_result:
        total_column_number = 0
        detail_results = result.split(',')
        for detail_result in detail_results:
            detail_result = detail_result.strip()
            if (detail_result.startswith("=HYPERLINK")):
                detail_result = detail_result[12:-2]
                sheet.write_url(total_row_number,total_column_number,"external:" + detail_result)
            else:
                sheet.write(total_row_number, total_column_number, detail_result)
            total_column_number = total_column_number + 1
        total_row_number = total_row_number + 1


def sortByDownloadOrder(all_download_info, total_result):
    sort_total_result = []
    for one_download_info in all_download_info:
        apk_name = one_download_info["apk_name"]
        for result in total_result:
            result_apk = result.split(',')[0]
            if apk_name == result_apk:
                sort_total_result.append(result)
                break
    return sort_total_result


def ParseXML(xml,keyword):
    parameterlist = []
    root = ElementTree.parse(xml)
    values = root.findall(keyword)
    for item in values:
        parameter = item.text
        if parameter <> None:
            parameterlist.append(parameter)
        #print "The value for the %s is %s" % (keyword, parameterlist)
    return parameterlist

def getFinalNoResultApk(dic_final_no_result, incompatible_apk, wifi_issue_apk, download_fail_network_apk, davinci_no_result_apk, dic_download_fail, dic_no_result, dic_packge_apk, all_devices_info):
    one_file_no_result_apk = []
    for (dic_key, v) in dic_final_no_result.items():
        key_list = dic_key.split("|---|")
        package_name = key_list[0]
        test_des = key_list[1]
        one_no_result_info = {}
        one_no_result_info["apk_name"] = ""
        one_no_result_info["package_name"] = package_name
        one_no_result_info["failed_reason"] = ""
        one_no_result_info["link_folder"] = ""
        one_no_result_info["app_name"] = ""
        one_no_result_info["device_model"] = ""
        one_no_result_info["cpu"] = ""
        one_no_result_info["android_version"] = ""
        one_no_result_info["device_build_number"] = ""
        one_no_result_info["houdini_version"] = ""
        one_no_result_info["app_version"] = ""
        one_no_result_info["test_description"] = test_des

        app_info = v.split(':')
        if (len(app_info) >= 3):
            one_no_result_info["app_name"] = app_info[0]
            one_no_result_info["app_version"] = app_info[1]
            device_id = app_info[2].split(')')[0]
            for i in all_devices_info:
                if i['device_id'] == device_id:
                    one_no_result_info["android_version"] = i['android_version']
                    one_no_result_info["device_model"] = i['device_model']
                    one_no_result_info["device_build_number"] = i['device_build_number']
                    one_no_result_info["cpu"] = i['cpu']
                    one_no_result_info["houdini_version"] = i['houdini_version']
                    break

        if dic_key in dic_packge_apk:
            one_no_result_info["apk_name"] = dic_packge_apk[dic_key]
        else:
            one_no_result_info["apk_name"] = package_name + ".apk"

        if (dic_key in incompatible_apk):
            one_no_result_info["failed_reason"] = "Device incompatible"
        elif (dic_key in wifi_issue_apk):
            one_no_result_info["failed_reason"] = "WIFI issue"
        elif (dic_key in download_fail_network_apk):
            one_no_result_info["failed_reason"] = "Network issue"
        elif (dic_key in davinci_no_result_apk):
            one_no_result_info["failed_reason"] = "No result"

        if dic_key in dic_download_fail:
            one_no_result_info["link_folder"] = dic_download_fail[dic_key]
        elif dic_key in dic_no_result:
            one_no_result_info["link_folder"] = dic_no_result[dic_key]

        one_file_no_result_apk.append(one_no_result_info)
    return one_file_no_result_apk

def getNoResultApkInfo(no_result_file_list, all_devices_info, davinci_folder, apk_folder, apk_list):
    no_result_apk = []
    incompatible_apk = []
    wifi_issue_apk = []
    download_fail_network_apk = []
    davinci_no_result_apk = []
    for no_result_app_file in no_result_file_list:
        if os.path.exists(no_result_app_file):
            dic_download_fail = {}
            dic_no_result ={}
            dic_packge_apk = {}
            dic_final_no_result = OrderedDict()  # try 3 times, no result all the times

            no_result_file = open(no_result_app_file, "r")
            for line in no_result_file:
                line = line.strip().replace("[Seed:","[Seed-")
                if (line.startswith("Download Failed")):
                    download_info = line.split('|---|')         # dic_download_fail, key: package name, value: link of log folder
                    if (len(download_info) >= 5):
                        pkg_name = download_info[1].strip()
                        test_des = download_info[4].strip()
                        dic_key = pkg_name + "|---|" + test_des
                        if (download_info[3].strip().lower() == "incompatible"):
                            incompatible_apk.append(dic_key)
                        elif (download_info[3].strip().lower() == "wifiissue"):
                            wifi_issue_apk.append(dic_key)
                        else:
                            download_fail_network_apk.append(dic_key)
                        dic_download_fail[dic_key] = download_info[2].strip()
                elif (line.startswith("No Result") or line.startswith("Timeout")):
                    no_result_info = line.split('|---|')        # dic_no_result, key: package name, value: link of davinci log
                    if (len(no_result_info) >= 4):
                        apk_name = no_result_info[1].strip()
                        pkg_name = getApkPackageName(davinci_folder, apk_folder, apk_name)
                        test_des = no_result_info[3].strip()
                        dic_key = pkg_name + "|---|" + test_des
                        davinci_no_result_apk.append(dic_key)
                        if not dic_packge_apk.has_key(dic_key):   # dic_packge_apk, key: package name, value: apk name
                            dic_packge_apk[dic_key] = apk_name
                        if not dic_no_result.has_key(dic_key):
                            dic_no_result[dic_key] = no_result_info[2].strip()
                        else:
                            dic_no_result[dic_key] = dic_no_result[dic_key]  + "|---|" + no_result_info[2].strip()
                elif (line.startswith("++++++ Tried") or line.startswith("++++++ Network Issue")):
                    split_index = line.find('(');
                    if (split_index > 0):     # can find '('
                        result_apk = line[0:split_index]
                        app_name_version = line[split_index+1:-1]
                        apk_device_info = result_apk.split(':')
                        if (len(apk_device_info) >= 3):
                            test_des = apk_device_info[2].strip()
                            if (apk_device_info[1].strip().endswith(".apk")):
                                # if use apk list mode, need to reset apk_folder
                                if os.path.exists(apk_list):
                                    with open(apk_list, 'r') as f:
                                        for line in f.readlines():
                                            if ("\\" + apk_device_info[1].strip()) in line.decode('utf-8'):
                                                apk_folder = line.split(apk_device_info[1].strip())[0]
                                pkg_name = getApkPackageName(davinci_folder, apk_folder, apk_device_info[1].strip())
                            else:
                                pkg_name = apk_device_info[1].strip()
                            dic_key = pkg_name + "|---|" + test_des 
                            if not dic_final_no_result.has_key(dic_key):       # dic_final_no_result contains all the package names which have no result after trying 3 times
                                dic_final_no_result[dic_key] = app_name_version.strip()
            no_result_file.close()

            one_file_no_result_apk = getFinalNoResultApk(dic_final_no_result, incompatible_apk, wifi_issue_apk, download_fail_network_apk, davinci_no_result_apk, dic_download_fail, dic_no_result, dic_packge_apk, all_devices_info)
            no_result_apk.extend(one_file_no_result_apk)
    return no_result_apk


def getRawTotalResult(davinci_folder, apk_folder, warning_as_pass, all_devices_info, append_rank, all_rank, have_reference, total_test_times, pass_criteria, apk_list):
    total_result = []
    dic_apk_name_type = {}

    # Read detail result line by line, skip the first line
    result_file = apk_folder + "\\Smoke_Test_Report.csv"

    if os.path.exists(result_file):
        detail_result = open(result_file, 'r')
        first_line_result = True

        tmp_apk_name = ""
        tmp_apk_group = ""
        tmp_description = ""
        tmp_apk_result = []

        # total column number of Smoke_Test_Report.csv
        total_column_number = 17
        final_result_index = 11
        test_description_index = 18
        for result_line in detail_result:
            if result_line.strip() == "":
                continue
            if first_line_result:
                first_line_result = False
                continue
            # skip bad line
            if len(result_line.split(',')) < total_column_number:
                continue

            # split total result using apk name
            line_info = result_line.split(',')

            if warning_as_pass == True:
                if line_info[final_result_index].lower().strip() == "warning":
                    line_info[final_result_index] = "PASS"
            if (len(line_info) > test_description_index):
                test_description = line_info[test_description_index].strip()
            else:
                test_description = ""
            key_index = 2

            # get apk type
            if len(result_line.split(',')) > 16:
                one_apk_name = line_info[key_index].strip().decode("utf-8")
                one_apk_type = line_info[16].strip()
                if not dic_apk_name_type.has_key(one_apk_name):
                    dic_apk_name_type[one_apk_name] = one_apk_type

            device_group_line = getDeviceGroup(all_devices_info, line_info[1])
            if (tmp_apk_name == ""):
                tmp_apk_name = line_info[key_index]
                tmp_description = test_description
                tmp_apk_group = getDeviceGroup(all_devices_info, line_info[1])
                tmp_apk_result.append(line_info)
                continue
            if (tmp_apk_name == line_info[key_index] and tmp_description == test_description and (device_group_line == "NA" or device_group_line == tmp_apk_group)):
                tmp_apk_result.append(line_info)
            else:
                # generate final result for one app
                str_final_result = finalResultForOneApk(tmp_apk_result, all_devices_info, append_rank, all_rank,
                                                        have_reference, davinci_folder, apk_folder, total_test_times,
                                                        pass_criteria, apk_list)
                if str_final_result != '':
                    total_result.append(str_final_result)

                del tmp_apk_result[:]
                tmp_apk_name = line_info[key_index]
                tmp_description = test_description
                tmp_apk_group = device_group_line
                tmp_apk_result.append(line_info)
        # generate final result for the last app

        str_final_result = finalResultForOneApk(tmp_apk_result, all_devices_info, append_rank, all_rank, have_reference,
                                                davinci_folder, apk_folder, total_test_times, pass_criteria, apk_list)
        if str_final_result != '':
            total_result.append(str_final_result)
    return total_result, dic_apk_name_type


def Generate_Summary_Report(apk_folder, davinci_folder, config_file_name, treadmill_config, no_result_file_list, start_time):
    print "Please wait for a while to generate summary report ...\n"

    if not os.path.exists(davinci_folder + "\\platform-tools\\aapt.exe"):
        print "'%s'\\platform-tools\\aapt.exe does not exist." % (davinci_folder)
        return

    if not os.path.exists(config_file_name):
        print "device config file %s does not exist." % config_file_name
        return

    total_test_times = 3
    pass_criteria = 2
    apk_download_mode = "downloaded"
    warning_as_pass = False
    apk_list = ""
    if os.path.exists(treadmill_config):
        test_times = ParseXML(treadmill_config, 'TestNumber')
        pass_times = ParseXML(treadmill_config, 'PassCriteria')
        download_modes = ParseXML(treadmill_config, 'APK_download_mode')
        deal_warning = ParseXML(treadmill_config, 'WarningAsPass')
        apk_list_name = ParseXML(treadmill_config, 'ApkList')
        if (len(test_times) > 0):
            total_test_times = (int)(test_times[0])
        if (len(pass_times) > 0):
            pass_criteria = (int)(pass_times[0])
        if (len(download_modes) > 0):
            apk_download_mode = download_modes[0]
        if (len(deal_warning) > 0):
            if (deal_warning[0].lower().strip() == 'true'):
                warning_as_pass = True
        if (len(apk_list_name) > 0):
            apk_list = apk_list_name[0]
    # Append Rank info or not
    append_rank = False
    rank_info_file = apk_folder + "\\version_info.txt"
    all_rank = []
    play_store_info = ""
    title = ""
    if os.path.exists(rank_info_file):
        append_rank = True
        all_rank = getAllRankInfo(apk_folder)
        play_store_info = getPlayStoreInfo(apk_folder)
        title = "Application,App name,Package name,Device Model,CPU,Android Version,Device Build Number,Houdini Version,App Version," + play_store_info + ",Install,Launch,Random,Back,Uninstall,Result,PassRate,Reason,Login,TestDescription,Link\n"
    else:
        if os.path.exists(apk_folder + "\\apk_version_info.txt"):
            all_rank = getAllVerInfo(apk_folder)
        title = "Application,App name,Package name,Device Model,CPU,Android Version,Device Build Number,Houdini Version,App Version,Install,Launch,Random,Back,Uninstall,Result,PassRate,Reason,Login,TestDescription,Link\n"

    # Get target device and reference device information
    all_devices_info = getAllDeviceInfo(davinci_folder, config_file_name)
    # Have Reference device or not
    have_reference = haveReferenceDevice(all_devices_info)
    
    total_result, dic_apk_name_type = getRawTotalResult(davinci_folder, apk_folder, warning_as_pass, all_devices_info, append_rank, all_rank, have_reference, total_test_times, pass_criteria, apk_list)
    
    have_download_result = False
    download_result_file = apk_folder + "\\download_result.txt"
    all_download_info = []

    if os.path.exists(download_result_file):
        have_download_result = True
        lines = open(download_result_file, "r")
        for line in lines:
            download_results = line.split("<><>")
            if len(download_results) >= 3:
                one_download_info = {}
                one_download_info["apk_name"] = os.path.basename(download_results[0].strip().decode("utf-8"))
                one_download_info["result"] = download_results[1].strip().decode("utf-8")
                one_download_info["url"] = download_results[2].strip().decode("utf-8")
                all_download_info.append(one_download_info)
        total_result = sortByDownloadOrder(all_download_info, total_result)

    no_result_apk = []
    no_result_apk = getNoResultApkInfo(no_result_file_list, all_devices_info, davinci_folder, apk_folder, apk_list)

    report_title = "Summary report"
    if (append_rank == True):
        report_title = report_title + " - " + play_store_info.replace("rank", '')

    generateCSVResult(title, total_result, apk_folder, no_result_apk, append_rank, all_rank)
    generateTotalResult(title, total_result, apk_folder, append_rank, report_title, have_download_result,
                        all_download_info, dic_apk_name_type, no_result_apk, start_time, all_rank, warning_as_pass)

    print "Generate summary done!"

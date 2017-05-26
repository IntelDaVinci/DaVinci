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

from __future__ import division
import sys
import os
import re
from common import *
import shutil
import stat
from xml.etree import ElementTree
from dvcutility import Enviroment
reload(sys)
sys.setdefaultencoding('utf-8')


def generateOneQS(relaunch_number, apkName, packageName, actionNum, waitTime, videoRecording, apkFolder, workFolder,
                  install_application, uninstall_application, back_action, data_info, record_video_failed_app,
                  each_seed_value, each_abi):
    apkPath = ""
    if apkFolder <> "":
        apkPath = apkName
        #If apk path is for a APK name list (not path). path. APK folder shall be combined into the whole apk path.
        if apkPath.find("\\") < 0:
            apkPath = apkFolder + "\\" + apkName
    data_source = ""
    data_dest = ""
    for dinfo in data_info:
        data_paths = dinfo.split(',')
        if (len(data_paths) < 3):
            continue
        if (data_paths[0] == packageName):
            data_source = data_paths[1].strip()
            data_dest = data_paths[2].strip()

    qsLines = ""
    qsLines += "[Configuration]\n"
    qsLines += "CapturedImageHeight=787\n"
    qsLines += "CapturedImageWidth=447\n"
    qsLines += "CameraPresetting=HIGH_RES\n"
    qsLines += "PackageName=" + packageName + "\n"
    qsLines += "ActivityName=\n"
    # qsLines += "APKName=" + apkName.encode("utf-8") +"\n"
    qsLines += "APKName=" + apkPath +"\n"
    if (data_source <> "" and data_dest <> ""):
        qsLines += "PushData=" + data_source + " " + data_dest + "\n"
    else:
        qsLines += "PushData=\n"

    if (videoRecording.lower().strip() == "yes" or record_video_failed_app.lower().strip() == "yes"):
        qsLines += "VideoRecording=true\n"
    else:
        qsLines += "VideoRecording=false\n"

    lineNum = 1
    qsLines += "[Events and Actions]\n"
    qsLines += str(lineNum) + "                0: OPCODE_CRASH_OFF\n"
    lineNum = lineNum + 1
    qsLines += str(lineNum) + "                0: OPCODE_HOME\n"
    lineNum = lineNum + 1

    if install_application.lower().strip() == "yes":
        if each_abi == 'java':
            qsLines += str(lineNum) + "                0: OPCODE_INSTALL_APP\n"
        else:
            qsLines += str(lineNum) + '                0: OPCODE_INSTALL_APP  0 options="--abi {}"\n'.format(each_abi)
        lineNum = lineNum + 1
    if (data_source <> "" and data_dest <> ""):
        qsLines += str(lineNum) + "                0: OPCODE_PUSH_DATA\n"
        lineNum = lineNum + 1
    qsLines += str(lineNum) + "                0: OPCODE_START_APP\n"
    lineNum = lineNum + 1

    i = 0
    timeStamp = 0
    while (i < relaunch_number):
        timeStamp = timeStamp + waitTime
        qsLines += str(lineNum) + "            " + str(timeStamp) + ": OPCODE_STOP_APP\n"
        lineNum = lineNum + 1
        timeStamp = timeStamp+3000
        qsLines += str(lineNum) + "            " + str(timeStamp) + ": OPCODE_START_APP\n"
        lineNum = lineNum + 1
        i = i + 1

    timeStamp = timeStamp + waitTime
    qsLines += str(lineNum) + "             " + str(timeStamp) + ": OPCODE_SET_VARIABLE n " + str(actionNum) + "\n"
    lineNum = lineNum + 1
    loopNum = lineNum
    timeStamp = timeStamp + 1000
    qsLines += str(lineNum) + "             " + str(timeStamp) + ": OPCODE_EXPLORATORY_TEST " + "bias_{}.xml\n".format(each_seed_value)
    lineNum = lineNum + 1
    timeStamp = timeStamp + 1000
    qsLines += str(lineNum) + "             " + str(timeStamp) + ": OPCODE_LOOP n " + str(loopNum) + "\n"
    lineNum = lineNum + 1
    if back_action.lower().strip() == "yes":
        timeStamp = timeStamp + 3000
        qsLines += str(lineNum) + "             " + str(timeStamp) + ": OPCODE_BACK\n"
        lineNum = lineNum + 1

    if uninstall_application.lower().strip() == "yes":
        timeStamp = timeStamp+3000
        qsLines += str(lineNum) + "            " + str(timeStamp) + ": OPCODE_STOP_APP\n"
        lineNum = lineNum + 1
        timeStamp = timeStamp+3000
        qsLines += str(lineNum) + "            " + str(timeStamp) + ": OPCODE_CLEAR_DATA\n"
        lineNum = lineNum + 1
        timeStamp = timeStamp+3000
        qsLines += str(lineNum) + "            " + str(timeStamp) + ": OPCODE_UNINSTALL_APP\n"

    qs_src = workFolder + "\\" + packageName + "_seed_{}.qs".format(each_seed_value)
    writeFile(qs_src, qsLines)
    return qs_src


def MatchRnR(rnr_configinfo, rnr_folder, app_ver, device_info_dict, priority_items):
    # If there is rnr config suite information
    rnr_xml = ''
    dep_list = []
    # schedule the xml script.
    dependencies_list = []
    rnr_xml, dependencies_list = scheduleRnR(app_ver, device_info_dict, rnr_configinfo, priority_items)
    if not os.path.isabs(rnr_xml):
        # For relative path, combine the full path.
        rnr_xml = r'%s\%s' % (rnr_folder , rnr_xml)
    return rnr_xml, dependencies_list


def anyRnRTest(package_name, apk_folder, rnrpath, priority_items):
    try:
        rnr_folder = apk_folder + "\\" + package_name
        if rnrpath <> "" and os.path.exists(rnrpath):
            rnr_folder = rnrpath + "\\" + package_name
        from dvcutility import RnRConfigInfo
        # Check if RnR folder is there.
        rnr_config_suite_found = False
        rnr_configinfo_suite_list =[]
        des_list = []
        if os.path.exists(rnr_folder) and os.path.isdir(rnr_folder):
            # Get all rnr config info for this package
            rnr_config_file = r'%s\RnR_Scripts_Config.xml' % (rnr_folder)
            if os.path.exists(rnr_config_file):
                rnr_config = RnRConfigInfo(rnr_config_file)
                rnr_configinfo_suite_list, des_list = rnr_config.GetRnRConfigsInfo(priority_items)
            else:
                # Search for the .xml script
                for list in os.listdir(rnr_folder):
                    #When RnR script replaying, coverage file xxx_cov.xml and xxx_all_cov.xml will be created, so filter these two _cov.xml
                    if os.path.splitext(list)[1].strip() == '.xml' and not list.endswith('_cov.xml') and list <> 'RnR_Scripts_config.xml': 
                        rnr_xml = r'%s\%s' % (rnr_folder, list)
                        rnr_configinfo_suite_list.append(rnr_xml)
                        break            
        else:
            rnr_folder = ''

    except Exception, e:
        s = sys.exc_info()
        print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
        sys.exit(-1)

    return rnr_configinfo_suite_list, des_list, rnr_folder
 
                
def generateRnRQS(apk_name, rnr_folder, work_folder, ondevice_download, install_application,uninstall_application):
    try:
        for list in os.listdir(rnr_folder):
            if os.path.splitext(list)[1].strip() == '.qs':
                rnrfile = work_folder + "\\" + list
                package_name = ""
                if os.path.exists(rnrfile):
                    lines = open(rnrfile, 'r')
                    qsLines = ""
                    account = ""
                    password = ""
                    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
                    smartlogin = ""
                    for line in lines:
                        if line.find(" LOGIN_START") > 0 and account <> "" and password <> "":
                            smartlogin = "start"
                        if line.find(" LOGIN_END") > 0 and smartlogin == "start":
                            smartlogin = ""
                        if smartlogin == "start" and line.find("OPCODE_SET_TEXT ") > 0:
                            smartlogin = "account"
                            tmp_lines = line.split(":")
                            qsLines += tmp_lines[0] + ' OPCODE_SET_TEXT "' + account + '"\n'
                            continue
                        if smartlogin == "account" and line.find("OPCODE_SET_TEXT ") > 0:
                            smartlogin = ""
                            tmp_lines = line.split(":")
                            qsLines += tmp_lines[0] + ' OPCODE_SET_TEXT "' + password + '"\n'
                            continue
                        if line.startswith("PackageName="):
                            qsLines += line
                            package_name = line.split("=")[1].strip()
                            config_file_name = script_path + "\\SmokeConfig.csv"
                            config_content = readConfigFile(config_file_name)
                            account_info = config_content['account_info']
                            for account in account_info:
                                package_account_password = account.split(',')
                                if (len(package_account_password) < 3):
                                    continue
                                if (package_account_password[0] == package_name):
                                    account = package_account_password[1]
                                    password = package_account_password[2]
                                    break
                        elif line.startswith("APKName="):
                             qsLines += "APKName=" + apk_name + "\n"
                        elif "OPCODE_INSTALL_APP" in line:
                            if ondevice_download:
                                continue
                            if install_application.lower().strip() == "yes":
                                end_index = line.index("OPCODE_INSTALL_APP") + len("OPCODE_INSTALL_APP")
                                qsLines += line[0:end_index] + "\n"
                        elif "OPCODE_UNINSTALL_APP" in line:
                            if ondevice_download:
                                continue
                            if uninstall_application.lower().strip() == "yes":
                                end_index = line.index("OPCODE_UNINSTALL_APP") + len("OPCODE_UNINSTALL_APP")
                                qsLines += line[0:end_index] + "\n"
                        else:
                            qsLines += line
                    writeFile(rnrfile, qsLines)
                    lines.close()

    except Exception, e:
        s = sys.exc_info()
        print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
        sys.exit(-1)


def normalize_values(rnr_config_dict_list, device_info_dict):
    try:
        base_resolution_large = int(device_info_dict['Resolution'].strip().split('x')[1])
        base_resolution_small = int(device_info_dict['Resolution'].strip().split('x')[0])
        base_DPI = int(device_info_dict['DPI'])
        for rnr_config_dict in rnr_config_dict_list:
            # If resolution is *, do not need to normalize
            if rnr_config_dict['Resolution'].strip() == '*':
                rnr_config_dict['normalized_score'] = '0'
                continue
            resolution_large = int(rnr_config_dict['Resolution'].strip().split('x')[1])
            resolution_small = int(rnr_config_dict['Resolution'].strip().split('x')[0])
            if resolution_large == base_resolution_large and base_resolution_small == resolution_small:
                rnr_config_dict['normalized_score'] = '1'
                continue

            rnr_config_dict['normalized_score'] = '0'
            normalized_resolution_large = 0
            normalized_resolution_small = 0
            normalized_DPI = 0
            if base_resolution_large > resolution_large:
                factor = base_resolution_large / resolution_large
                normalized_resolution_large = int(round(resolution_large * factor))
                normalized_resolution_small = int(round(resolution_small * factor))
                if rnr_config_dict['DPI'].strip() <> '*':
                    normalized_DPI = int(round(int(rnr_config_dict['DPI']) * factor, 0))
            else:
                factor = resolution_large / base_resolution_large
                normalized_resolution_large = int(round(resolution_large / factor))
                normalized_resolution_small = int(round(resolution_small / factor))
                if rnr_config_dict['DPI'].strip() <> '*':
                    normalized_DPI = int(round(int(rnr_config_dict['DPI']) / factor, 0))
                else:
                    normalized_DPI = -1

            # After normalization, if the value is base_value+-1, will consider they are the same.
            if normalized_resolution_large == base_resolution_large + 1 or normalized_resolution_large == base_resolution_large - 1:
                normalized_resolution_large = base_resolution_large
            if normalized_resolution_small == base_resolution_small + 1 or normalized_resolution_small == base_resolution_small - 1:
                normalized_resolution_small = base_resolution_small
            if normalized_DPI == base_DPI + 1 or normalized_DPI == base_DPI - 1:
                normalized_DPI = base_DPI

            if normalized_DPI <> -1:
                rnr_config_dict['DPI'] = str(normalized_DPI)
            rnr_config_dict['Resolution'] = '%sx%s' % (str(normalized_resolution_small), str(normalized_resolution_large))

    except Exception, e:
        s = sys.exc_info()
        print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
        sys.exit(-1)


def scheduleRnR(app_ver, device_info_dict, rnr_configinfo_list, config_priority_list):
    try:
        max_score = -1
        cur_mode_score = -1
        cur_xml = ''
        dependencies = ''
        cur_normalized_score = -1
        # Copy the rnr_configinfo_list and use the copy to normalize
        rnr_configinfo_list_normalized = []
        normalized_score = []
        for rnr_configinfo in rnr_configinfo_list:
            rnr_configinfo_list_normalized.append(dict.copy(rnr_configinfo))
        normalize_values(rnr_configinfo_list_normalized, device_info_dict)

        # Schedule according to the normalized values
        for rnr_configinfo in rnr_configinfo_list_normalized:
            device_info_dict_tmp = device_info_dict
            matched_score = 0
            mode_score = 0
            # Calculate the match score and mode socre according to config priority
            for config in config_priority_list:
                # AppVer is provided by parameter
                if config.lower() == 'appver':
                    device_value = app_ver.strip()
                else:
                    if device_info_dict_tmp.has_key(config):
                        device_value = device_info_dict_tmp[config].strip()
                if rnr_configinfo.has_key(config):
                    config_value = rnr_configinfo[config].strip()

                #For resolution, 00: no matching. 01: match with *. 10: match with normalized value. 11: accurate matching.
                if config.lower() == 'resolution':
                    if config_value == '*':
                        #  01: match with *
                        matched_score = matched_score << 1
                        matched_score += 1
                    elif device_value <> config_value:
                        # 00: no matching
                        matched_score = matched_score << 1
                    else:
                        if rnr_configinfo['normalized_score'] == '0':
                            # 10: match with normalized value.
                            matched_score += 1
                            matched_score = matched_score << 1
                        else:
                            # 11: accurate matching.
                            matched_score += 1
                            matched_score = matched_score << 1
                            matched_score += 1
                else:
                    # If config is *, it means vague matching, mode score is 0;
                    if config_value == '*':
                        matched_score += 1
                    # Other wise, if it is accurate match, mode score is 1.
                    else:
                        if device_value == config_value:
                            matched_score += 1
                            mode_score += 1
                # Left shift score 1 bit.
                matched_score = matched_score << 1
                mode_score = mode_score << 1

            # Find the max score. If the match score is the same with max score, check mode score.
            # That is to say, to check the priority of the matched item.
            if ( matched_score > max_score ) or ( matched_score == max_score and mode_score > cur_mode_score ):
                max_score = matched_score
                cur_mode_score = mode_score
                cur_xml = rnr_configinfo['path']
                if rnr_configinfo.has_key('dependencies'):
                    dependencies = rnr_configinfo['dependencies']

        dependencies_list = []
        if dependencies.strip() <> '':
            dependencies_list = dependencies.split(',')

        return cur_xml, dependencies_list
    except Exception, e:
        s = sys.exc_info()
        print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))
        sys.exit(-1)


def generateQSfile(davinci_folder, script_path, apk_folder, apk_name, work_folder, seed_value, abi_value,
                   ondevice_download, needInstall='yes',needUninstall='yes'):
    # Only one case that apk_name is not a .apk file: on-device download, but failed to fetch. In this case, apk_name's value is package name.
    if os.path.splitext(apk_name)[1] <> '.apk':
        package_name = apk_name
        apk_name = ""
    else:
        package_name = getApkPackageName(davinci_folder, work_folder, apk_name)

    QSfile = ""
    rnrfile = ""
    configContent = {}
    config_file_name = script_path + "\\SmokeConfig.csv"
##    file_path = work_folder + "\\DaVinciEmptyPackageName.txt"
    if not os.path.exists(config_file_name):
        print "Smoke test config file %s does not exist." % config_file_name
        sys.exit(-1)
    config_content = readConfigFile(config_file_name)
    # seed_value = config_content['seed_value']
    total_action_number = config_content['total_action_number']
    wait_time = config_content['wait_time']
    relaunch_number = config_content['relaunch_number']
    back_action = config_content['back_action']
    record_video = config_content['record_video']
    record_video_failed_app = config_content['record_video_failed_app']
    click_percentage = config_content['click_percentage']
    swipe_percentage = config_content['swipe_percentage']
    dispatch_keyword_pos = config_content['dispatch_keyword_pos']
    dispatch_ads_pos = config_content['dispatch_ads_pos']
    test_login = config_content['test_login']
    account_info = config_content['account_info']
    data_info = config_content['data_info']
    logcat_package_name = ""
    if ('logcat_package_name' in config_content.keys()):
        logcat_package_name = config_content['logcat_package_name']

    # If the apk is ondevice downloaded, run_qs.py set install or not. 
    # For local apk, SmokeConfig.csv set install or not.
    if (ondevice_download):
        install_application = needInstall
        uninstall_application = needUninstall
    else:
        install_application = config_content['install_application']
        uninstall_application = config_content['uninstall_application']

    try:
        # generate smoke script
        if apk_name <> "" or package_name <> "":
            generateOneBias(script_path, package_name, logcat_package_name, click_percentage, swipe_percentage,
                                account_info, work_folder, test_login, seed_value, dispatch_keyword_pos, dispatch_ads_pos)

            QSfile = generateOneQS(int(relaunch_number), apk_name, package_name, total_action_number, int(wait_time),
                                       record_video, apk_folder, work_folder, install_application, uninstall_application,
                                       back_action, data_info, record_video_failed_app, seed_value, abi_value)

        return QSfile
    
    except Exception as ex:
        print("Failed to create qsfile: %s" % str(ex))
        return ""    

def generateOneBias(script_path, package_name, logcat_package_name, click_percentage, swipe_percentage, account_info,
                    work_folder, test_login, seed_value, dispatch_keyword_pos, dispatch_ads_pos):
    testLogin = False
    account = ""
    password = ""
    configContent = {}
    config_file_name = script_path + "\\SmokeConfig.csv"
    if not os.path.exists(config_file_name):
        print "Smoke test config file %s does not exist." % config_file_name
        sys.exit(-1)

    if test_login.lower().startswith("y"):
        for account in account_info:
            package_account_password = account.split(',')
            if (len(package_account_password) < 3):
                continue
            if (package_account_password[0] == package_name):
                account = package_account_password[1]
                password = package_account_password[2]
                testLogin = True
                break
    biasLines = ""
    biasLines += "<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>\n"
    biasLines += "<DaVinci>\n"
    biasLines += "<Seed value=\""+ str(seed_value) +"\"/>\n"
    biasLines += "<Test type=\"SMOKE\" mode=\"OBJECT\" silent=\"NO\" coverage=\"YES\">\n"
    biasLines += "<Action type=\"CLICK\" value=\"" + click_percentage + "%\">\n"
    biasLines += "<Click type=\"PLAIN\" value=\"100%\"/>\n"
    biasLines += "</Action>" + "\n"
    biasLines += "<Action type=\"SWIPE\" value=\"" + swipe_percentage + "%\"/>\n"
    if testLogin == True:
        biasLines += "<Action type=\"ACCOUNT\" value=\"YES\">\n"
        biasLines += "<Account name=\"" + account + "\" password=\"" + password + "\" misc=\"\"/>\n"
        biasLines += "</Action>\n"
    if len(logcat_package_name) > 0:
        for pname in logcat_package_name.split(';'):
            if len(pname.strip()) > 0:
                biasLines += "<Action type=\"Log\">\n"
                biasLines += "<Package name=\"" + pname.strip() + "\"/>\n"
                biasLines += "</Action>\n"
    biasLines += "</Test>\n"
    biasLines += "<ActionPossibility>\n"
    biasLines += "<CommonKeywordPossibility value=\"" + dispatch_keyword_pos + "%\"/>\n"
    biasLines += "<AdsPossibility value=\"" + dispatch_ads_pos + "%\"/>\n"
    biasLines += "</ActionPossibility>\n"

    biasLines += "</DaVinci>"
    writeFile(work_folder + "\\" + "bias_{}.xml".format(seed_value), biasLines)


def isWritable(src):
    fileAtt = os.stat(src)[0]
    if (not fileAtt & stat.S_IWRITE):
        return False
    return True

def copyFile(src, dest):
    try:
        shutil.copy(src, dest)
    except Exception as ex:
        print('')

def copyFolder(sourceDir, targetDir, file_filter = [], folder_filter = [], ondevice_download = False):
    try:
        for f in os.listdir(sourceDir):
            sourceF = os.path.join(sourceDir, f)
            targetF = os.path.join(targetDir, f)
            if os.path.isfile(sourceF):
                # In on-device download mode, if APK is fetched already, we shall not overwrite it.
                if ondevice_download and os.path.splitext(f)[1] == '.apk' and os.path.exists(targetF):
                    continue
                    
                if f in file_filter:
                   continue 
                if os.path.splitext(f)[1] in file_filter:
                   continue
                # create folder
                if not os.path.exists(targetDir):
                    os.makedirs(targetDir)
                # file is not exist or size is different, replace it
                if not os.path.exists(targetF) or (os.path.exists(targetF) and (os.path.getsize(targetF) != os.path.getsize(sourceF))):
                    open(targetF, "wb").write(open(sourceF, "rb").read())
            if os.path.isdir(sourceF):
                if f in folder_filter:
                    continue
                copyFolder(sourceF, targetF, file_filter, folder_filter, ondevice_download)
    except Exception,e:
        s = sys.exc_info()
        print("[Error@%s]:%s" % (str(s[2].tb_lineno), s[1],))


def ContainNoneASCII(apkName):
    for ch in apkName:
        if ch > u'\u007f':
            return True
    return False

def CallAPKInfo(davinciFolder, apkFolder, apkName, info):
    infoValue=""
    apkPath = apkName
    if (apkPath.find("\\") < 0):
        apkPath = r'%s\%s' %(apkFolder, apkName)
    cmd = davinciFolder + "\\platform-tools\\aapt.exe dump badging \"" + apkPath + "\""
    #cmd = cmd.encode("utf-8")
    packageInfo = os.popen(cmd).readlines()
    for pInfo in packageInfo:
        if pInfo.startswith(info):
            infoValue = pInfo[len(info):]
            infoValue = (infoValue.split('\''))[0]
            break
    return infoValue

def getApkInfo(davinci_folder, apk_folder, apk_name, info):
    apk_name = apk_name.decode('utf-8')
    info_value = ""
    if apk_folder == "":
        apk_folder = ".\\"
    apk_folder = apk_folder.replace("/","\\")
    if apk_folder[-1]!="\\":
        apk_folder = apk_folder + "\\"
    dest_folder = apk_folder

    if apk_name[-4:] == ".apk":
        if ContainNoneASCII(apk_name):
            # check if the apk is writeable, since we need to copy and delete file
            srcPath = apk_name
            if srcPath.find("\\") < 0:
                srcPath = apk_folder + apk_name
            if apk_folder.startswith("\\\\"):       # if apk_folder is a sharefolder, copy the apk to local host machine
                dest_folder = os.getcwd()
            if os.path.exists(srcPath):
                if isWritable(srcPath) == True:
                    copyFile(srcPath, os.path.join(dest_folder,"testapk"))
                    info_value = CallAPKInfo(davinci_folder, dest_folder, "testapk", info)
                    Enviroment.remove_item(os.path.join(dest_folder,"testapk"), False)
            else:
                print "Couldn't find the apk file for testing!"
                #sys.exit(-1)
        else:
            info_value = CallAPKInfo(davinci_folder, apk_folder, apk_name, info)
    return info_value

def getApkPackageName(davinci_folder, apk_folder, apk_name):
    apk_name = apk_name.decode('utf-8')
    package_name = getApkInfo(davinci_folder, apk_folder, apk_name,"package: name='")
    return package_name

def initConfigKeyValue():
    configKeyValue = {}
    configKeyValue['seed_value'] = "seed value"
    configKeyValue['total_action_number'] = "total action number"
    configKeyValue['click_percentage'] = "click percentage"
    configKeyValue['swipe_percentage'] = "swipe percentage"
    configKeyValue['wait_time'] = "time to wait"
    configKeyValue['relaunch_number'] = "relaunch application"
    configKeyValue['install_application'] = "install application"
    configKeyValue['back_action'] = "perform back action"
    configKeyValue['uninstall_application'] = "uninstall application"
    configKeyValue['record_video'] = "record video or not"
    configKeyValue['record_video_failed_app'] = "record video for failed app"
    configKeyValue['test_login'] = "test login"
    configKeyValue['logcat_package_name'] = "specific package name"
    configKeyValue['dispatch_keyword_pos'] = "dispatch common keyword action percentage"
    configKeyValue['dispatch_ads_pos'] = "dispatch advertisement action percentage"
    return configKeyValue


def readConfigFile(config_file_name):
    configContent = {}
    accountInfo = []
    dataInfo = []
    inputmethodApps = []
    launcherApps = []
    accounts_line = False
    data_line = False
    inputmethod_line = False
    launcher_line = False
    configKeyValue = initConfigKeyValue()
    config_file = open(config_file_name, "r")
    for line in config_file:
        if line.strip() == "":
            continue
        content = line.split(',')
        # skip bad line
        if len(content) < 2:
            continue

        if (accounts_line == False and data_line == False):
            for (k,v) in configKeyValue.items():
                if (content[0].lower().strip().startswith(v)):
                    configContent[k] = content[1].strip()

        if (accounts_line == True and data_line == False):
            accountInfo.append(line.strip())

        if (data_line == True and inputmethod_line == False):
            dataInfo.append(line.strip())

        if (inputmethod_line == True and launcher_line == False):
            inputmethodApps.append(content[1].strip())

        if (launcher_line == True):
            launcherApps.append(content[1].strip())

        if (content[1].lower().strip() == "account name"):
            accounts_line = True
        if (content[1].lower().strip() == "source path"):
            data_line = True
        if (content[1].lower().strip() == "input method apps"):
            inputmethod_line = True
        if (content[1].lower().strip() == "launcher apps"):
            launcher_line = True

    accountInfo.pop()   # should pop up the last item, the last item is the header of push data:'Package Name,Source Path,Dest Path'
    dataInfo.pop()
    inputmethodApps.pop()

    configContent['account_info'] = accountInfo
    configContent['data_info'] = dataInfo
    configContent['inputmethod_apps'] = inputmethodApps
    configContent['launcher_apps'] = launcherApps

    config_file.close()
    return configContent

def ReadDaVinciApplistXML(applist_xml):
    inputmethod_apps = []
    launcher_apps = []
    root = ElementTree.parse(applist_xml).getroot()
    for child in root:
        if (child.attrib['type'] == "InputMethodApps"):
            for item in child.findall('AppName'):
                inputmethod_apps.append(item.get('value'))
        if (child.attrib['type'] == "LauncherApps"):
            for item in child.findall('AppName'):
                launcher_apps.append(item.get('value'))
    return inputmethod_apps, launcher_apps

def WriteDaVinciApplistXML(applist_xml, new_inputmethod, new_launcher):
    tree = ElementTree.parse(applist_xml)
    root = tree.getroot()
    for child in root:
        if (child.attrib['type'] == "InputMethodApps"):
            for item in new_inputmethod:
                new_node = ElementTree.Element("AppName")
                new_node.set("value", item)
                child.insert(0, new_node)       # insert new input method app which comes from SmokeConfig.csv
        if (child.attrib['type'] == "LauncherApps"):
            for item in new_launcher:
                new_node = ElementTree.Element("AppName")
                new_node.set("value", item)
                child.insert(0, new_node)       # insert new launcher app which comes from SmokeConfig.csv
    tree.write(applist_xml)
    return 0

def CompareAndMergeApplistXML(script_folder, davinci_folder):
    config_file_name = script_folder + "\\SmokeConfig.csv"
    if not os.path.exists(config_file_name):
        print "Smoke test config file %s does not exist." % config_file_name
        sys.exit(-1)

    applist_xml = davinci_folder + "\\DaVinci_Applist.xml"
    if not os.path.exists(applist_xml):
        print "DaVinci app list file %s does not exist." % applist_xml
        sys.exit(-1)

    # read input method and launcher apps from SmokeConfig.csv
    config_content = readConfigFile(config_file_name)
    config_inputmethod = config_content['inputmethod_apps']
    config_launcher = config_content['launcher_apps']

    # read input method and launcher app from DaVinci_Applist.xml
    init_inputmethod, init_launcher = ReadDaVinciApplistXML(applist_xml)

    # compare two kinds of input method and launcher apps
    new_inputmethod = []
    new_launcher = []

    for app in config_inputmethod:
        if app not in init_inputmethod:
            new_inputmethod.append(app)
    for app in config_launcher:
        if app not in init_launcher:
            new_launcher.append(app)

    # merge new input method app and launcher app into DaVinci_Applist.xml
    if (len(new_inputmethod) != 0 or len(new_launcher) != 0):
        WriteDaVinciApplistXML(applist_xml, new_inputmethod, new_launcher)
    return 0

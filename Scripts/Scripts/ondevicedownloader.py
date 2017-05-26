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
import shutil
import time
import subprocess
import config
import datetime
import threading
import ctypes
from dvcutility import Enviroment

def uninstall_apk(apk_list, device_id):
    for name in apk_list:
        print 'Uninstalling %s' % name
        if device_id is not None:
            uninstall_cmd = '%s\\platform-tools\\adb -s %s uninstall %s' % (
                config.default_davinci_path.replace('/', '\\'), device_id, name)
        else:
            uninstall_cmd = config.default_davinci_path.replace('/', '\\') + '\\platform-tools\\adb  uninstall ' + name
        fnull = open(os.devnull, 'w')
        subprocess.call(uninstall_cmd, stdout=fnull, stderr=fnull)
        fnull.close()
        time.sleep(2)


# def update_version_info(pkg_name, version, apk_info_with_version):
#     for i in apk_info_with_version:
#         if i[0] == pkg_name:
#             i[1] = version
#             break
#
#
# def get_label_name(apk_info, pkg_name):
#     for i in apk_info:
#         if i[0] == pkg_name:
#             return i[3]
#
#
# def update_apk_with_version(apk_name, apk_path, apk_info_with_version):
#     existed_apk = (i for i in apk_name if os.path.exists(i + ".apk"))
#     for i in existed_apk:
#         cmd = r"%s\platform-tools\aapt d badging %s.apk|findstr versionName" % (
#             config.default_davinci_path.replace('/', '\\'), i)
#         p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
#         content = p.communicate()[0]
#         try:
#             version = content.strip().split('versionName=')[1][1:-1]
#             label_name = get_label_name(apk_info_with_version, i)
#         except Exception, e:
#             pass
#         else:
#             # print version
#             if version is not None:
#                 try:
#                     shutil.copy(i + '.apk', label_name + "_" + version + '.apk')
#                     update_version_info(i, version, apk_info_with_version)
#                 except Exception, e:
#                     print str(e)
#                     pass


def generate_download_xml(apk_list, template, save_dir):
    with open(template) as f:
        template_content = f.read()

    keyword_app = 'app='
    keyword_index = '">'
    p1 = template_content.index(keyword_app)
    p2 = template_content.index(keyword_index)
    pre_part = template_content[:p1] + keyword_app
    post_part = template_content[p2:][template_content[p2:].index('>'):]

    for i in apk_list:
        output = pre_part + '"' + i + '"' + post_part
        try:
            with open(save_dir + '\\' + i + '.xml', 'w') as f:
                f.write(output)
        except Exception:
            pass


def generate_download_qs(apk_list, template, save_dir):
    with open(template) as f:
        qs_content = f.readlines()

    for i in range(0, len(apk_list)):
        update_content = []
        for j in qs_content:
            if j.find('OPCODE_EXPLORATORY_TEST') != -1:
                j = j.replace(j[j.rfind(' ') + 1:], apk_list[i] + '.xml' + '\n')

            update_content.append(j)

        try:
            with open(save_dir + '\\' + apk_list[i] + '.qs', 'w') as q:
                q.writelines(update_content)
        except Exception:
            pass


def create_apk_folder_with_timestamp():
    dest_folder_path = os.getcwd() + '\\' + time.strftime('%Y_%m_%d_%H_%M_%S', time.localtime(time.time()))
    if os.path.exists(dest_folder_path):
        try:
            shutil.rmtree(dest_folder_path)
        except Exception:
            pass
    try:
        os.mkdir(dest_folder_path)
    except Exception:
        pass

    try:
        os.mkdir(dest_folder_path + '\\on_device_qscript')
    except Exception:
        pass

    return dest_folder_path


def download_apk(apk_list, qs_dir, device_id, davinci_timeout):
    qs_name = [qs_dir + '\\' + i.strip() + '.qs' for i in apk_list]
    for qs in qs_name:
        print 'Downloading ' + qs.split('\\')[-1][:-3] + '...'
        cmd = config.default_davinci_path.replace('/', '\\') + "\\DaVinci.exe" + ' -p "' + qs + '" -timeout ' + str(davinci_timeout)

        if device_id is not None:
            cmd += ' -device ' + device_id
        # Do not show crash dialog
        SEM_NOGPFAULTERRORBOX = 0x0002  # From MSDN
        ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX)
        CREATE_NO_WINDOW = 0x8000000
        subprocess_flags = CREATE_NO_WINDOW

        output_log_file = qs_dir + os.path.sep + qs.split('\\')[-1][:-3] + '_log.txt'
        log_object = open(output_log_file, 'w')
        p = subprocess.Popen(cmd, stdout=log_object, stderr=log_object, creationflags=subprocess_flags)
        process_timer = threading.Timer(davinci_timeout, p.kill)
        process_timer.start()
        out, err = p.communicate()
        process_timer.cancel()
        log_object.close()


def get_device_resolution(device_id):
    cmd = r"%s\platform-tools\adb -s %s shell dumpsys window displays|findstr init" % (
        config.default_davinci_path.replace('/', '\\'), device_id)
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
        content = p.communicate()[0]
        result = content.strip().split(' ')[0].split('=')[1].split('x')
    except Exception:
        return str(480), str(800)
    else:
        return result[0], result[1]


def unlock_device(device_id):
    unlock_coordinate_x, unlock_coordinate_y = get_device_resolution(device_id)
    unlock_cmd_prefix = r"%s\platform-tools\adb -s %s shell input touchscreen swipe " % (
        config.default_davinci_path.replace('/', '\\'), device_id)
    unlock_coordinate = (
        str(int(unlock_coordinate_x) / 10),
        str(int(unlock_coordinate_y) * 4 / 5),
        str(int(unlock_coordinate_x) * 4 / 5),
        str(int(unlock_coordinate_y) * 4 / 5))

    unlock_coordinate_vertical = (
        str(int(unlock_coordinate_x) / 2),
        str(int(unlock_coordinate_y) / 2),
        str(int(unlock_coordinate_x) / 2),
        str(int(unlock_coordinate_y) / 20))

    subprocess.call(unlock_cmd_prefix + ' '.join(unlock_coordinate), shell=True)
    time.sleep(2)
    # try to unlock again
    subprocess.call(unlock_cmd_prefix + ' '.join(unlock_coordinate), shell=True)
    time.sleep(2)

    subprocess.call(unlock_cmd_prefix + ' '.join(unlock_coordinate_vertical), shell=True)
    time.sleep(2)
    subprocess.call(unlock_cmd_prefix + ' '.join(unlock_coordinate_vertical), shell=True)


def get_uninstall_list(device_id):
    cmd = r"%s\platform-tools\adb -s %s shell pm list packages -f" % (
        config.default_davinci_path.replace('/', '\\'), device_id)
    apk_info = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].split(os.linesep)
    return [i.strip().split('=')[1] for i in apk_info if i.strip() != '']


def quick_uninstall(target_apk_list, device_id):
    existed_apk = get_uninstall_list(device_id)
    need_to_uninstall_apk = (i for i in target_apk_list if i in existed_apk)
    uninstall_apk(need_to_uninstall_apk, device_id)


def get_all_apk_path(device_id, out_result):
    cmd = r"%s\platform-tools\adb -s %s shell pm list packages -f" % (
        config.default_davinci_path.replace('/', '\\'), device_id)

    apk_info = subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].split(os.linesep)
    apk_info = (i.strip() for i in apk_info if i.strip() != '')
    for i in apk_info:
        out_result[i.split('=')[1]] = i.split(':')[1].split('=')[0]


def fetch_apk(apk_list, tmp_folder, save_folder, retry_time, device_id, apk_need_to_re_download):
    apk_path_dict = {}
    get_all_apk_path(device_id, apk_path_dict)

    while retry_time > 0 and apk_list != []:
        retry_time -= 1
        apk_name = apk_list[:]
        for name in apk_name:
            target_apk = save_folder + '\\' + name + ".apk"

            print 'Fetching %s...' % (target_apk.split('\\')[-1])
            Enviroment.remove_item(target_apk)

            if name not in apk_path_dict.keys():
                timestamp = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
                output_log_file = tmp_folder + os.path.sep + name + '_list_' + timestamp + '.txt'

                with open(output_log_file, 'w') as f:
                    f.writelines(list(apk_path_dict))
                if name not in apk_need_to_re_download:
                    apk_need_to_re_download.append(name)
                continue

            device_str = ""
            if device_id is not None:
                device_str = " -s %s " % device_id

            cmd = r'%s\platform-tools\adb %s pull %s %s' % (
                config.default_davinci_path.replace('/', '\\'), device_str, apk_path_dict[name], target_apk)
            fnull = open(os.devnull, 'w')
            subprocess.call(cmd, stdout=fnull, stderr=fnull)
            fnull.close()
            # subprocess.call(cmd)
            if os.path.exists(target_apk):
                apk_list.remove(name)
                if name in apk_need_to_re_download:
                    apk_need_to_re_download.remove(name)
                time.sleep(2)


def reboot_device(device_id, timeout=180):
    print "It's time to reboot %s" % device_id
    reboot_cmd = '%s\\platform-tools\\adb -s %s reboot' % (config.default_davinci_path.replace('/', '\\'), device_id)
    check_cmd = '%s\\platform-tools\\adb -s %s shell getprop sys.boot_completed' % (
        config.default_davinci_path.replace('/', '\\'), device_id)
    fnull = open(os.devnull, 'w')

    subprocess.call(reboot_cmd, stdout=fnull, stderr=fnull)
    fnull.close()
    time.sleep(2)
    t_beginning = time.time()
    h_beginning = time.time()
    seconds_passed = 0
    heartbeat = 3
    is_alive = False

    while seconds_passed < timeout:
        time.sleep(1)
        heartbeat_passed = time.time() - h_beginning
        if heartbeat_passed > heartbeat:
            p = subprocess.Popen(check_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            content = p.communicate()[0]

            if content.strip() == "1":
                is_alive = True
                break
            h_beginning = time.time()
        seconds_passed = time.time() - t_beginning

    time.sleep(5)

    unlock_device(device_id)

    return is_alive


def download_apk_on_device(apk_name_list, davinci_timeout_in_seconds=600, need_pull_apk=False, apk_store_folder='.',
                           device_id=None, retry_time=5):
    if apk_store_folder == '.':
        apk_store_folder = os.getcwd()

    if need_pull_apk and not os.path.exists(apk_store_folder):
        return False, None

    apk_list = [i.strip() for i in apk_name_list if i.strip() != '']
    if not apk_list:
        return False, None

    tmp_folder = create_apk_folder_with_timestamp()
    generate_download_qs(apk_list, "appdownload.qs", tmp_folder + "\\on_device_qscript")
    generate_download_xml(apk_list, "appdownload.xml", tmp_folder + "\\on_device_qscript")
    reload(config)

    each_download_num = 10
    apk_need_to_re_download = []
    apk_num_for_reboot = 40
    for i in range(0, len(apk_list), each_download_num):
        d1 = datetime.datetime.now()
        if i and i % apk_num_for_reboot == 0 and not reboot_device(device_id):
            print 'Reboot failed'

        each_apk_list = apk_list[i: i + each_download_num if i + each_download_num < len(apk_list) else len(apk_list)]
        download_apk(each_apk_list, tmp_folder + "\\on_device_qscript", device_id, davinci_timeout_in_seconds)
        if need_pull_apk:
            expect_apk_list = each_apk_list[:]

            # time.sleep(60)
            fetch_apk(each_apk_list, tmp_folder, apk_store_folder, retry_time, device_id, apk_need_to_re_download)
            downloaded_apk_list = [i for i in expect_apk_list if i not in apk_need_to_re_download]
            uninstall_apk(downloaded_apk_list, device_id)
        d2 = datetime.datetime.now()
        print 'ELAPSED TIME IS ' + str(d2 - d1)

    if need_pull_apk:
        if apk_need_to_re_download:
            print 'Sleep five minutes, please wait...'
            time.sleep(300)
            print 'Sleep finished'
            retry_fetch_list = apk_need_to_re_download[:]
            fetch_apk(retry_fetch_list, tmp_folder, apk_store_folder, retry_time, device_id, apk_need_to_re_download)

        start_index = 1
        end_index = 5

        if apk_need_to_re_download:
            reboot_device(device_id)

        while start_index <= end_index and apk_need_to_re_download:
            round_apk_list = apk_need_to_re_download[:]
            start_index += 1
            if start_index == 4:
                print 'Sleep five minutes, please wait...'
                time.sleep(300)
                print 'Sleep finished'

            for i in range(0, len(round_apk_list), each_download_num):
                each_apk_list = round_apk_list[i: i + each_download_num
                                               if i + each_download_num < len(round_apk_list) else len(round_apk_list)]
                download_apk(each_apk_list, tmp_folder + "\\on_device_qscript", device_id, davinci_timeout_in_seconds)
                # print 'fetching again..'
                # print 'apk_need_to_re_download is %d'%len(apk_need_to_re_download)
                time.sleep(60)

                fetch_apk(each_apk_list, tmp_folder, apk_store_folder, retry_time, device_id, apk_need_to_re_download)

        quick_uninstall(list(set(apk_list) - set(apk_need_to_re_download)), device_id)

        if not apk_need_to_re_download:
            shutil.rmtree(tmp_folder)
            Enviroment.remove_item(tmp_folder, False)
        else:
            apk_need_to_remove_files = set(apk_list) - set(apk_need_to_re_download)
            with open('download_fail_apk.txt', 'w') as f:
                f.writelines([i + os.linesep for i in apk_need_to_re_download])

            postfix_collection = ('_replay.avi', '_replay.qts', '.qs', '.xml', '_log.txt')

            target = (os.path.join(tmp_folder, "on_device_qscript", package_name, postfix)
                      for package_name in apk_need_to_remove_files for postfix in postfix_collection)

            map(Enviroment.remove_item, target)

    return True, tmp_folder

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
import sys
import msvcrt
import subprocess
import hashlib

APK_NAME_VERSION_SEPARATOR = '---==---'


def get_valid_name(origin_name):
    win_invalid_char_set = ['/', '\\', ':', '*', '?', '"', '<', '>', ',']

    for j in win_invalid_char_set:
        if origin_name.count(j) > 0:
            origin_name = origin_name.replace(j, ' ')

    return origin_name


def get_user_input(caption, default, hidden_flag=False, timeout=15):
    start_time = time.time()
    sys.stdout.write('%s(%s):' % (caption, default))
    user_input = ''
    while True:
        if msvcrt.kbhit():
            if hidden_flag:
                user_input_char = msvcrt.getch()
            else:
                user_input_char = msvcrt.getche()
            if ord(user_input_char) == 13:  # enter_key
                break
            elif ord(user_input_char) >= 32:  # space_char
                user_input += user_input_char
                if hidden_flag:
                    sys.stdout.write('*')
            elif ord(user_input_char) == 8:  # backspace
                user_input = user_input[:-1]
        
        if (time.time() - start_time) > timeout:
            break

    print ''  # need to move to next line
    if len(user_input) > 0:
        return user_input
    else:
        return default


def create_apk_folder(dest_folder_path):
    try:
        if not os.path.exists(dest_folder_path):
            os.makedirs(dest_folder_path)
    except WindowsError, e:
        print 'create {0} failed, using {1} as save path'.format(dest_folder_path, os.getcwd())
    else:
        os.chdir(dest_folder_path)


def copy_apk(src, dest, apk_list, without_version=False):
    if src == dest:
        return
    second_copy_list = []

    for i in apk_list:
        try:
            print 'Copying {0} ...'.format(i)
        except Exception, e:
            pass

        try:
            if without_version:
                shutil.copy(src + os.path.sep + i, dest + os.path.sep + i.split(APK_NAME_VERSION_SEPARATOR)[0] + i[-4:])
            else:
                shutil.copy(src + os.path.sep + i, dest + os.path.sep + i)
        except IOError as e:
            second_copy_list.append(i)
        except:
            # print "Unexpected error:", sys.exc_info()[0]
            pass

    if len(second_copy_list) > 0:
        for i in second_copy_list:
            try:
                print 'Copying %s again...' % i
            except Exception, e:
                pass

            try:
                if without_version:
                    shutil.copy(src + os.path.sep + i,
                                dest + os.path.sep + i.split(APK_NAME_VERSION_SEPARATOR)[0] + i[-4:])
                else:
                    shutil.copy(src + os.path.sep + i, dest + os.path.sep + i)
            except IOError as e:
                pass
            except:
                print "Unexpected error:", sys.exc_info()[0]


def get_string_hash(string):
    m = hashlib.md5()
    m.update(string)
    return m.hexdigest()


def check_apk_integrity(apk_path):
    import config
    aapt_path = os.path.join(config.default_davinci_path.replace('/', os.path.sep), "platform-tools", "aapt.exe")

    if not os.path.exists(aapt_path) or not os.path.exists(apk_path) or not apk_path.endswith(".apk"):
        # no need to check, always return true
        return True

    cmd = "{0} d badging {1}".format(aapt_path, apk_path)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, _ = p.communicate()

    return True if out else False


def get_local_apk_version(apk_folder_path, remote_apk_info, need_copy_apk_list):
    apk_info = {}

    if os.path.exists(apk_folder_path):

        for i in remote_apk_info:
            expectation_file_name = i[0] + APK_NAME_VERSION_SEPARATOR + i[1] + i[3]
            if os.path.exists(expectation_file_name):
                need_copy_apk_list.append(expectation_file_name)
            else:
                apk_info[i[2]] = i[0] + APK_NAME_VERSION_SEPARATOR + i[1] + i[3]

    return apk_info


def get_local_apk_version_for_google(apk_folder_path, remote_apk_info, need_copy_apk_list):
    apk_info = []

    if os.path.exists(apk_folder_path):

        for i in remote_apk_info:
            expectation_file_name = i[3] + APK_NAME_VERSION_SEPARATOR + i[1] + i[2]
            if os.path.exists(expectation_file_name):
                need_copy_apk_list.append(expectation_file_name)
            else:
                apk_info.append([i[0], i[3]])

    return apk_info


def generate_folder_with_timestamp(apk_rank_list, apk_info_with_version, rank_begin, dl_num, category_selection,
                                   category_list, current_apk_path, without_version_flag=False):
    cur_time = time.localtime()
    timestamp_postfix = time.strftime("%Y_%m_%d_%H_%M_%S", cur_time)
    treadmill_folder_path = os.path.abspath('..') + os.path.sep + os.path.basename(
        os.getcwd()) + '_' + timestamp_postfix
    os.mkdir(treadmill_folder_path)
    copy_apk(current_apk_path, treadmill_folder_path, apk_info_with_version, without_version_flag)

    content = [os.path.basename(current_apk_path) + '|' + str(rank_begin) + '|' + str(rank_begin + dl_num - 1)
               + '|' + time.strftime("%Y%m%d%H%M", cur_time) + os.linesep]

    separator = '|---|'
    # print len(apk_rank_list)
    # print len(apk_info_with_version)
    try:
        if category_selection == 1:
            for i in range(rank_begin, rank_begin + dl_num):
                content.append((str(i) + separator + apk_rank_list[i - rank_begin] + separator + 'app' + separator
                                + '.'.join(apk_info_with_version[i - rank_begin].split('.')[:-1]).split(APK_NAME_VERSION_SEPARATOR)[1]
                                + os.linesep).encode('utf-8'))
        elif category_selection == 2:
            for i in range(rank_begin, rank_begin + dl_num):
                content.append((str(i) + separator + apk_rank_list[i - rank_begin] + separator + 'game' + separator
                                + '.'.join(apk_info_with_version[i - rank_begin].split('.')[:-1]).split(APK_NAME_VERSION_SEPARATOR)[1]
                                + os.linesep).encode('utf-8'))
        elif category_selection == 3:
            for i in range(rank_begin, rank_begin + dl_num):
                content.append((str(i) + separator + apk_rank_list[i - rank_begin] + separator + 'app' + separator
                                + '.'.join(apk_info_with_version[i - rank_begin].split('.')[:-1]).split(APK_NAME_VERSION_SEPARATOR)[1]
                                + os.linesep).encode('utf-8'))
            for i in range(rank_begin, rank_begin + dl_num):
                content.append((str(i) + separator + apk_rank_list[
                    i + dl_num - rank_begin] + separator + 'game' + separator
                                + '.'.join(apk_info_with_version[i + dl_num - rank_begin].split('.')[:-1]).split(APK_NAME_VERSION_SEPARATOR)[1]
                                + os.linesep).encode('utf-8'))
        else:
            # 1|---|com.douguo.recipe|---|Life|---|5.6.4.4
            for i in range(rank_begin, rank_begin + dl_num):
                content.append((str(i) + separator + apk_rank_list[i - rank_begin] + separator
                                + category_list[category_selection] + separator
                                + '.'.join(apk_info_with_version[i - rank_begin].split('.')[:-1]).split(APK_NAME_VERSION_SEPARATOR)[1]
                                + os.linesep).encode('utf-8'))
        with open(treadmill_folder_path + '/version_info.txt', 'w') as f:
            f.writelines(content)

    except Exception, e:
        # print str(e)
        pass
    finally:
        print '\n*********************************************************'
        print 'Please use below path as treadmill script path parameter.\n'
        print treadmill_folder_path
        print '\n*********************************************************'
        print 'Done'


def generate_folder_with_timestamp_for_china(apk_info, rank_begin, dl_num, category_selection,
                                             category_list, current_apk_path):
    apk_rank_list, apk_info_with_version = zip(*((i[0], i[0] + APK_NAME_VERSION_SEPARATOR + i[1] + i[3])
                                                 for i in apk_info))

    return generate_folder_with_timestamp(apk_rank_list, apk_info_with_version, rank_begin, dl_num,
                                          category_selection, category_list,
                                          current_apk_path, True)

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
import sys
import requests
import prcstoredownloader
import datetime
import downloadFromGooglePlay as gp
from getdeviceid import *
from apkhelper import *
# from config import *
import config

MAX_CATEGORY_NUM = 500

APK_DOWNLOADER_VERSION = '2.6.2'

APK_STORE_MAIN_PAGE = ('https://www.google.com',
                       'http://www.baidu.com',
                       'http://apps.wandoujia.com',
                       'http://zhushou.360.cn/',
                       'http://www.anzhi.com',
                       'http://www.gfan.com',
                       'http://www.lenovomm.com',
                       'http://app.mi.com',
                       'http://app.mi.com',
                       'http://android.myapp.com',
                       'http://apk.hiapk.com/',
                       'http://play.91.com/android/',
                       'http://android.d.cn/',
                       'http://www.muzhiwan.com/')
APK_STORE = ('Press 0 to exit', 'Google Play(deprecated)', 'Baidu Store', 'WanDouJia Store', '360 Store', 'Anzhi Store',
             'Gfan Store', 'Lenovo LeStore', 'Xiaomi Store', 'XiaomiHD Store', 'Tencent Store', 'HiApk Store',
             '91Market Store', 'DangLe Store', 'Muzhiwan Store')
PROXY_TYPE = ('Press 0 to exit', 'http', 'https', 'socks4', 'socks5')
APK_CATEGORY = ('', 'Download TopFreeApps Only', 'Download TopFreeGames Only', 'Download TopFreeApps and TopFreeGames')
SUB_CATEGORY = [None,
                ['System Tool', 'Theme and Wallpaper', 'Social Communication', 'Life', 'Video',
                       'Study', 'Photography','Travel', 'Finance', 'Casual Puzzle', 'Role Playing', 'Shooting',
                       'Strategy', 'Sports', 'Racing', 'Chess', 'Simulation'],
                None, None, None, None, None, None, None, None, None, None, None]

def check_store_connection(index, proxy=None):
    print '[LogInfo]:Connecting to %s...' % APK_STORE[index + 1]
    try:
        r = requests.get(APK_STORE_MAIN_PAGE[index], proxies=proxy, timeout=3, verify=False)
    except Exception as e:
        print '[LogInfo]:Connect to %s failed.' % APK_STORE[index + 1]
        return False
    else:
        if r.status_code == 200:
            print '[LogInfo]:Connect to %s success.' % APK_STORE[index + 1]
            return True


if __name__ == '__main__':
    print '****Welcome to Apk Downloader ' + APK_DOWNLOADER_VERSION + '****'

    start_time = datetime.datetime.now()

    while not os.path.exists(config.default_davinci_path + os.path.sep + 'DaVinci.exe'):
        user_define_davinci_path = raw_input("Please input DaVinci.exe full path, such as c:/Intel/Bits/DaVinci/bin/\n")
        if os.path.exists(user_define_davinci_path + os.path.sep + 'DaVinci.exe'):
            user_define_davinci_path = user_define_davinci_path.replace('\\', '/')
            config.default_davinci_path = user_define_davinci_path
            save_config("default_davinci_path", user_define_davinci_path)
        else:
            print '[LogInfo] DaVinci path is not valid, please re-input.'
    # print '[LogInfo]:Please select store before downloading'
    reload(config)
    print '********Store Menu*************'
    for i in range(1, len(APK_STORE)):
        print str(i) + ', ' + APK_STORE[i]
    print '*******************************'

    while True:
        user_selection = get_user_input('Please enter number before store name, Press 0 to exit',
                                        config.default_store_index)
        try:
            store_selection = int(user_selection)
        except ValueError, e:
            print '[LogInfo]:Sorry,Input is invalid, please re-enter it!'
        else:
            if 0 < store_selection < len(APK_STORE):
                # print '[LogInfo]:Ok, we\'ll download apks from ' + APK_STORE[int(store_selection)]
                if user_selection != config.default_store_index:
                    save_config('default_store_index', user_selection)
                break

            elif user_selection == '0':
                print '[LogInfo]:Bye'
                sys.exit()
            else:
                print '[LogInfo]:Sorry,Input is invalid, please re-enter it!'

    if store_selection == APK_STORE.index('Google Play(deprecated)'):
        if not config.device_id.strip():
            target_device = select_one_device()
            if target_device:
                get_device_id(config.default_davinci_path, target_device)
            else:
                print 'No valid device id.'
                sys.exit(0)
        else:
            device_option = get_user_input('Do you want to use ' + config.device_id + ' for downloading? \n' +
                                           "If Yes, please press key \'Y\';If not, please press key \'N\'", 'y')
            if device_option.lower() == 'n':
                target_device = select_one_device()
                if target_device:
                    get_device_id(config.default_davinci_path, target_device)
                else:
                    print 'No valid device id'
                    sys.exit(0)
        reload(config)

    while True:
        force_proxy_option = get_user_input('Do you want to use the existed proxy setting for downloading? \n' +
                                           "If Yes, please press key \'Y\';If not, please press key \'N\'", config.force_proxy)
        if force_proxy_option.isalpha() and force_proxy_option.lower() in ('y', 'n'):
            save_config('force_proxy', force_proxy_option.lower())
            break
        else:
            print '[LogInfo]:Sorry,Input is invalid, please re-enter it!'

    user_define_proxy = {}
    
    if force_proxy_option.lower() == 'y' or not check_store_connection(store_selection - 1, user_define_proxy):

        if config.default_proxy_type == 'http' or config.default_proxy_type == 'https' and config.default_http_proxy:
            user_define_proxy = {config.default_proxy_type: config.default_http_proxy}
        elif config.default_proxy_type == 'socks4' or config.default_proxy_type == 'socks5' and config.default_socks_proxy:
            user_define_proxy = {config.default_proxy_type: config.default_socks_proxy}
            sock_proxy_addr = config.default_socks_proxy[: config.default_socks_proxy.rfind(':')]
            sock_proxy_port = int(config.default_socks_proxy[config.default_socks_proxy.rfind(':') + 1:])
            import socks
            import socket

            if config.default_proxy_type == 'socks4':
                socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS4, sock_proxy_addr, int(sock_proxy_port))
            else:
                socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock_proxy_addr, int(sock_proxy_port))
            socket.socket = socks.socksocket


    # By judging the user_define_proxy is empty or not, we can conclude we need to re-check the connection or not.
    if user_define_proxy and not check_store_connection(store_selection - 1, user_define_proxy):
        print '[LogInfo]:Do we need to set proxy for downloading?'
        proxy_option = get_user_input('If Yes, please press key \'Y\'; If not, please press key \'N\':', 'n')
        if proxy_option == 'y' or proxy_option == 'Y':
            while True:
                # print ('[LogInfo]:Please enter number before store_selection name, press 0 to exit')
                print '*********Proxy Menu*************'
                for i in range(1, 5):
                    print str(i) + ', ' + PROXY_TYPE[i]
                print '*******************************'
                proxy_type_selection = raw_input("Please select the proxy type: ")

                if proxy_type_selection == '1' or proxy_type_selection == '2':
                    print '[LogInfo]:The http(s) proxy url format is http(s)://proxy.intel.com:8087,' \
                          '"http://"or "https://" prefix is required'
                    proxy_url = raw_input('Please input proxy url:')
                    user_define_proxy = {PROXY_TYPE[int(proxy_type_selection)]: proxy_url}
                    print user_define_proxy
                    if config.default_proxy_type.startswith('socks'):
                        socks.setdefaultproxy()
                    if check_store_connection(store_selection - 1, user_define_proxy):
                        save_config('default_proxy_type', PROXY_TYPE[int(proxy_type_selection)])
                        save_config('default_http_proxy', proxy_url)
                        break
                    else:
                        print 'Test Connection failed, exit the script'
                        sys.exit(-1)

                elif proxy_type_selection == '3' or proxy_type_selection == '4':
                    print '[LogInfo]:The socks4/5 proxy url format is proxy.intel.com:512,' \
                          '"http://"or "https://" prefix is not required'
                    proxy_url = raw_input("Please input your proxy url: ")
                    print proxy_url
                    sock_proxy_addr = proxy_url[:proxy_url.rfind(':')]

                    sock_proxy_port = int(proxy_url[proxy_url.rfind(':') + 1:])
                    import socks
                    import socket

                    if proxy_type_selection == '3':
                        socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS4, sock_proxy_addr, int(sock_proxy_port))
                    else:
                        socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock_proxy_addr, int(sock_proxy_port))

                    socket.socket = socks.socksocket
                    user_define_proxy = {PROXY_TYPE[int(proxy_type_selection)]: proxy_url}
                    if check_store_connection(store_selection - 1, user_define_proxy):
                        save_config('default_proxy_type', PROXY_TYPE[int(proxy_type_selection)])
                        save_config('default_socks_proxy', proxy_url)
                        break
                    else:
                        print 'Test Connection failed, exit the script'
                        sys.exit(-1)

                elif proxy_type_selection == '0':
                    print '[LogInfo]:Bye'
                    sys.exit(0)
                else:
                    print '[LogInfo]:Sorry,Input is invalid, please re-enter it'
        elif proxy_option == 'n':
            print 'Test Connection failed, exit the script'
            sys.exit(-1)

    current_category = []
    current_category.extend(list(APK_CATEGORY))
    if SUB_CATEGORY[store_selection - 1]:
        current_category.extend(SUB_CATEGORY[store_selection - 1])

    print '\n********Category Menu*************'
    for i in range(1, len(current_category)):
        print str(i) + ', ' + current_category[i]
    print '**********************************'
    while True:
        input_number = get_user_input('Please select category: ', config.default_apk_category)
        if input_number.isdigit() and 0 < int(input_number) <= len(current_category) - 1:
            category_selection = int(input_number)
            print '[LogInfo]:Download category is %s' % current_category[category_selection]
            if input_number != config.default_apk_category:
                save_config('default_apk_category', input_number)
            break
        else:
            print '[LogInfo]:Input is not valid number!'

    while True:
        user_input_begin = get_user_input('\nPlease enter the download start number of top app rank,'
                                          ' press Enter to download from the top: ', config.default_download_begin)
        if user_input_begin == '':
            rank_begin = 1
            save_config('default_download_begin', str(rank_begin))
            break
        elif user_input_begin.isdigit():
            rank_begin = int(user_input_begin)
            if rank_begin == 0:
                print '[LogInfo]:Input is not valid number!'
            else:
                print '[LogInfo]:The downloading will start from rank %s' % rank_begin
                if user_input_begin != config.default_download_begin:
                    save_config('default_download_begin', user_input_begin)
                break
        else:
            print '[LogInfo]:Input is not valid number!'

    while True:
        user_input_end = get_user_input('\nPlease enter the number of apps to be downloaded: ',
                                        config.default_download_end)
        if user_input_end.isdigit():
            dl_num = int(user_input_end)
            if dl_num == 0:
                print '[LogInfo]:Input is not valid number!'
            else:
                print '[LogInfo]:Download number is %s' % dl_num
                if user_input_end != config.default_download_end:
                    save_config('default_download_end', user_input_end)
                break
        else:
            print '[LogInfo]:Input is not valid number!'

    if (rank_begin + dl_num - 1) > MAX_CATEGORY_NUM and store_selection == 1:
        print '[LogInfo]:Google Play only support TOP-%s download! ' \
              'The apk download number will be set to %s' % (MAX_CATEGORY_NUM, MAX_CATEGORY_NUM)
        dl_num = MAX_CATEGORY_NUM - rank_begin + 1

    if not config.default_apk_save_path.strip():
        config.default_apk_save_path = r"C:/Intel/BiTs/DaVinci/script"

    path_msg = "\nDo you want to use folder {0} to store APK files?If Yes, please press key \'Y\';" \
               " If not, please press key \'N\':".format(config.default_apk_save_path)
    while True:
        path_option = get_user_input(path_msg, 'y')
        if path_option == 'y' or path_option == 'Y':
            user_define_path = config.default_apk_save_path
            break
        elif path_option == 'n' or path_option == 'N':
            user_define_path = raw_input("Please enter the folder path to store APK files."
                                         "e.g c:\\Intel\\BiTs\\DaVinci\\apk:\n")
            # create_dir(user_define_path)
            # default_apk_save_path = user_define_path
            if user_define_path == '':
                user_define_path = "C:/Intel/BiTs/DaVinci/script"

            save_config("default_apk_save_path", user_define_path.replace("\\", "/"))
            break
        else:
            print 'Input is invalid, please re-input.'

    rank_end = rank_begin + dl_num - 1

    try:
        if store_selection == 1:
            gp.download_google_entry(user_define_proxy, rank_begin, rank_end,
                                     config.default_apk_save_path, user_define_path,
                                     category_selection, current_category)
        else:
            downloader = prcstoredownloader.PRCDownloader(APK_STORE[store_selection].split(' ')[0],
                                                          user_define_proxy, rank_begin, rank_end,
                                                          config.default_apk_save_path, user_define_path,
                                                          category_selection, category_list=current_category)
            downloader.start_download()
    except Exception, e:
        s = sys.exc_info()

        print "Error '%s' happened on line %d" % (s[1], s[2].tb_lineno)
    else:
        pass

    end_time = datetime.datetime.now()
    print 'The elapsed time for downloading is ' + str(end_time - start_time)

    sys.exit(0)

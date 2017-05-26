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
import threading
import xlrd
import socket
import time
import downloadFromGooglePlay as gp
from getdeviceid import *
from apkhelper import *

reload(sys)
sys.setdefaultencoding("utf-8")
APK_STORE_MAIN_PAGE = ('https://www.google.com',
                       'http://www.baidu.com')
APK_STORE = ('Press 0 to exit', 'Google Play', 'Baidu Store', 'WanDouJia Store', 'Lenovo LeStore')
PROXY_TYPE = ('Press 0 to exit', 'http', 'https', 'socks4', 'socks5')

DOWNLOAD_TYPE = ('APK list in excel format', 'APK list in excel format with multi sheets', 'APK list in text format')
SPLIT_CHAR = '<><>'
OUTPUT_FILE = "download_result.txt"


def check_apk_size(download_result):
    output = []
    for i in download_result:
        try:
            if not os.path.exists(i[0]):
                output.append(i[0] + SPLIT_CHAR + 'FAIL' + SPLIT_CHAR + i[1] + os.linesep)
            else:
                output.append(i[0] + SPLIT_CHAR + 'OK' + SPLIT_CHAR + i[1] + os.linesep)

        except WindowsError, e:
            output.append(i[0] + SPLIT_CHAR + i[1] + SPLIT_CHAR + 'INVALID')

    with open(OUTPUT_FILE, 'w') as f:
        f.writelines(output)


class mt_downloader(threading.Thread):
    def __init__(self, target_info, proxy):
        threading.Thread.__init__(self)
        self.info = target_info
        self.proxy = proxy

    def run(self):
        for k, v in self.info.iteritems():
            url = k.strip()
            filename = v
            try:
                if filename[-4:] != '.apk':
                    filename += '.apk'
                print '\n' + '[LogInfo]:Downloading app from ' + url + '...' + time.ctime() + '\n'

                r = requests.get(url, proxies=self.proxy)
            except requests.RequestException, e:
                pass
            except socket.error, e:
                pass
            else:
                if r.status_code == 200:
                    temp_apk_name = get_string_hash(url) + ".apk"
                    with open(temp_apk_name, 'wb') as apk:
                        apk.write(r.content)
                    if not check_apk_integrity(temp_apk_name):
                        os.remove(temp_apk_name)
                    else:
                        os.rename(temp_apk_name, filename.decode('utf-8'))


def downloader(download_link, proxy):
    max_thread_number = 3

    urls = download_link.keys()
    while urls:
        if threading.activeCount() <= max_thread_number:
            info = {}
            info[urls[0]] = download_link[urls[0]]
            t = mt_downloader(info, proxy)
            t.start()
            urls.remove(urls[0])
        else:
            time.sleep(3)
    while threading.activeCount() > 1:
        time.sleep(10)
        print '[LogInfo]:Please wait, the APK downloading is still underway...'


def get_apk_list_from_excel(download_link_xls, rank_begin, rank_end, download_result):
    data = xlrd.open_workbook(download_link_xls)
    table = data.sheets()[0]

    dl_info = {}

    if rank_end + 1 > table.nrows:
        rank_end = table.nrows - 1
    for i in range(rank_begin, rank_end + 1):
        apk_name = get_valid_name(str(table.row(i)[0].value).decode('utf-8')) + ".apk"
        if apk_name == '.apk':
            continue
        apk_name = os.path.join(os.getcwd(), apk_name)
        download_result.append([apk_name, table.row(i)[1].value])
        if not os.path.exists(apk_name):
            # print 'need to download %s'%table.row(i)[0].value
            dl_info[table.row(i)[1].value] = apk_name
    return dl_info


def create_apk_folder(dest_folder_path):
    try:
        if not os.path.exists(dest_folder_path):
            os.makedirs(dest_folder_path)
    except WindowsError, e:
        print '[LogInfo]:create {0} failed, using {1} as save path'.format(dest_folder_path, os.getcwd())
    else:
        os.chdir(dest_folder_path)


def get_apk_list_from_excel_all_sheets(download_link_xls, download_result):
    data = xlrd.open_workbook(download_link_xls)

    dl_info = {}

    for sheet in data.sheets():
        # print sheet.name
        sheet_info = {}
        for i in range(1, sheet.nrows):
            if sheet.row(i)[0].value.strip():
                apk_name = get_valid_name(str(sheet.row(i)[0].value).decode('utf-8')) + ".apk"
                download_result.append([apk_name, sheet.row(i)[1].value])
                sheet_info[sheet.row(i)[1].value] = apk_name

        dl_info[get_valid_name(str(sheet.name).decode('utf-8'))] = sheet_info
    return dl_info


def download_apk_from_excel_with_range(proxy, download_link_xls, rank_begin, rank_end, download_result, apk_path):
    folder_post_fix = str(rank_begin) + '_' + str(rank_end) + '_' + time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())
    target_folder_path = os.path.join(apk_path, folder_post_fix).replace('/',  os.path.sep)

    create_apk_folder(target_folder_path)
    downloader(get_apk_list_from_excel(download_link_xls, rank_begin, rank_end, download_result), proxy)
    print '*'*40
    print 'Download finished, plese fetch the apk from below link\n'
    print target_folder_path
    print '*'*40

def check_store_connection(index, proxy=None):
    print '[LogInfo]:Checking Network Connection Status...'
    try:
        r = requests.get(APK_STORE_MAIN_PAGE[index], proxies=proxy, timeout=3, verify=False)
    except:
        print '[LogInfo]:Network Connection is not available.'
        return False
    else:
        if r.status_code == 200:
            print '[LogInfo]:Network Connection is available now.'
            return True


def download_apk_from_excel(proxy, download_link_xls, download_result, apk_path):
    dl_info = get_apk_list_from_excel_all_sheets(download_link_xls, download_result)
    folder_post_fix = 'excel_all_sheets_' + time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())
   
    target_folder_path = os.path.join(apk_path, folder_post_fix).replace('/',  os.path.sep)
    create_apk_folder(target_folder_path)
   
    excel_root_path = os.getcwd()
    for sheet_name in dl_info.keys():
        create_apk_folder(sheet_name)

        downloader(dl_info[sheet_name], proxy)
        sheet_info = [i for i in download_result if i[0] in dl_info[sheet_name].values()]
        check_apk_size(sheet_info)
        os.chdir(excel_root_path)
    
    print '*'*40
    print 'Download finished, plese fetch the apk from below link\n'
    print target_folder_path
    print '*'*40

if __name__ == '__main__':
    print '****Welcome to Apk Downloader for local list****'
    print '[LogInfo]:This version is only works with user-specified apk list.'
    if not check_config_file_exists():
        get_device_id()
    from config import *

    print '********APK list Menu*************'
    for i in range(0, len(DOWNLOAD_TYPE)):
        print str(i + 1) + ', ' + DOWNLOAD_TYPE[i]
    print '*******************************'

    while True:
        user_selection = raw_input('Please enter number before APK list type\n')
        try:
            download_type = int(user_selection)
        except ValueError, e:
            print '[LogInfo]:Sorry,Input is invalid, please re-enter it!'
        else:
            if 0 < download_type < len(DOWNLOAD_TYPE) + 1:
                break
            else:
                print '[LogInfo]:Sorry,Input is invalid, please re-enter it!'

    if download_type == 1 or download_type == 2:
        test_connection_store_index = 1
    else:
        test_connection_store_index = 0

    user_define_proxy = {}
    if not check_store_connection(test_connection_store_index, user_define_proxy):
        if default_proxy_type == 'http' or default_proxy_type == 'https' and default_http_proxy:
            user_define_proxy = {default_proxy_type: default_http_proxy}
        elif default_proxy_type == 'socks4' or default_proxy_type == 'socks5' and default_socks_proxy:
            user_define_proxy = {default_proxy_type: default_socks_proxy}
            sock_proxy_addr = default_socks_proxy[: default_socks_proxy.rfind(':')]
            sock_proxy_port = int(default_socks_proxy[default_socks_proxy.rfind(':') + 1:])
            import socks
            import socket
            if default_proxy_type == 'socks4':
                socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS4, sock_proxy_addr, int(sock_proxy_port))
            else:
                socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock_proxy_addr, int(sock_proxy_port))
            socket.socket = socks.socksocket

    if not check_store_connection(test_connection_store_index, user_define_proxy):
        print '[LogInfo]:Do we need to set proxy for downloading?'
        proxy_option = raw_input('If Yes, please press key \'Y\'; If not, please press key \'N\':')
        if proxy_option == 'y' or proxy_option == 'Y':
            while True:
                # print ('[LogInfo]:Please enter number before store_selection name, press 0 to exit')
                print '*********Proxy Menu*************'
                for i in range(1, 5):
                    print str(i) + ', ' + PROXY_TYPE[i]
                print '*******************************'
                proxy_type_selection = raw_input("Please select the proxy type: ")

                if proxy_type_selection == '1' or proxy_type_selection == '2':
                    print '[LogInfo]:The http(s) proxy url format is http(s)://proxy.intel.com:8087,"http://"or "https://" prefix is required'
                    proxy_url = raw_input('Please input proxy url:')
                    user_define_proxy = {PROXY_TYPE[int(proxy_type_selection)]: proxy_url}
                    # socks.setdefaultproxy()
                    if default_proxy_type.startswith('socks'):
                        socks.setdefaultproxy()

                    if check_store_connection(test_connection_store_index, user_define_proxy):
                        save_config('default_proxy_type', PROXY_TYPE[int(proxy_type_selection)])
                        save_config('default_http_proxy', proxy_url)
                        break
                    else:
                        print '[LogInfo]:Network Connection is invalid, exit the script.'
                        sys.exit(-1)

                elif proxy_type_selection == '3' or proxy_type_selection == '4':
                    print '[LogInfo]:The socks4/5 proxy url format is proxy.intel.com:512,' \
                          ' "http://"or "https://" prefix is NOT required'
                    proxy_url = raw_input("Please input your proxy url: ")
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
                    if check_store_connection(test_connection_store_index, user_define_proxy):
                        save_config('default_proxy_type', PROXY_TYPE[int(proxy_type_selection)])
                        save_config('default_socks_proxy', proxy_url)
                        break
                    else:
                        print '[LogInfo]:Network Connection is invalid, exit the script'
                        sys.exit(-1)

                elif proxy_type_selection == '0':
                    print '[LogInfo]:Bye'
                    sys.exit(0)
                else:
                    print '[LogInfo]:Sorry,Input is invalid, please re-enter it'

    if not default_apk_save_path.strip():
        default_apk_save_path = r"C:/Intel/BiTs/DaVinci/Scripts"

    path_msg = "\nDo you want to use folder " + default_apk_save_path + " to store APK files? If Yes, please press key \'Y\'; If not, please press key \'N\':"
    while True:
        path_option = raw_input(path_msg)
        if path_option == 'y' or path_option == 'Y':
            user_define_path = default_apk_save_path
            break
        elif path_option == 'n' or path_option == 'N':
            user_define_path = raw_input("\nPlease enter the folder path to store APK files.e.g c:\\Intel\\BiTs\\DaVinci\\apk:\n")
            # default_apk_save_path = user_define_path
            if not user_define_path:
                user_define_path = r"C:/Intel/BiTs/DaVinci/Scripts"
                print '[LogInfo]:Use the {0} as the APK store path.'.format(user_define_path)
            save_config("default_apk_save_path", user_define_path.replace("\\", "/"))
            break
        else:
            print '[LogInfo]:Input is invalid, please re-input.'

    if download_type == 1 or download_type == 2:
        download_result = []
        while True:
            user_define_excel = raw_input('\nPlease enter the excel file path, such as c:\\script\\apk.xls...\n')
            if user_define_excel.count(':') == 0:
                user_define_excel = os.getcwd() + '\\' + user_define_excel
            if os.path.isfile(user_define_excel):
                break
            else:
                print "[LogInfo]:The excel file doesn't exists, please re-enter the file path."
        if download_type == 1:
            while True:
                user_input_begin = raw_input('\nPlease enter the starting row number of apk list, '
                                             'press Enter to download from the beginning: ')
                if user_input_begin == '':
                    rank_begin = 1
                    break
                elif user_input_begin.isdigit():
                    rank_begin = int(user_input_begin)
                    if rank_begin == 0:
                        print '[LogInfo]:Input is not valid number!'
                    else:
                        print '[LogInfo]:The downloading will start from rank {0}'.format(rank_begin)
                        break
                else:
                    print '[LogInfo]:Input is not valid number!'

            while True:
                user_input_end = raw_input('\nPlease enter the ending row number of apk list: ')
                if user_input_end.isdigit():
                    rank_end = int(user_input_end)
                    if rank_end == 0 or rank_end <= rank_begin:
                        print '[LogInfo]:Input is not valid number!'
                    else:
                        print '[LogInfo]:Download range is {0}-{1}'.format(rank_begin, rank_end)
                        break
                else:
                    print '[LogInfo]:Input is not valid number!'

            download_apk_from_excel_with_range(user_define_proxy, user_define_excel,
                                               rank_begin, rank_end, download_result, user_define_path)
            check_apk_size(download_result)

        else:
            download_apk_from_excel(user_define_proxy, user_define_excel, download_result, user_define_path)
    else:
        gp.download_google_with_specified_package(user_define_proxy, user_define_path)

    sys.exit(0)
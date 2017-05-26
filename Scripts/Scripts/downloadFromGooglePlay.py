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
import base64
import zlib
import re
import requests
import lxml.html
import time
import config
from apkhelper import *
from getdeviceid import *

ONE_MIN = 60
ONE_HOUR = 3600
ResultLog = r'result.txt'
OPERATOR_LIST = {"USA": {'Wireless 2000': '31011', 'Powertel': '31027',
                         'BellSouth': '31015', 'Aerial': '31031', 'Iowa Wireless': '31077',
                         'Western Wireless': '31026', 'Cingular': '31017',
                         'AT&T': '31038', 'Sprint': '31002', 'T-Mobile': '31020'}}

requests.packages.urllib3.disable_warnings()


def get_google_token(email, password, proxy):
    params = {
        "Email": email,
        "Passwd": password,
        "service": "androidsecure",
        "accountType": "HOSTED_OR_GOOGLE"
    }

    headers = {"Content-type": "application/x-www-form-urlencoded"}

    r = requests.post('https://android.clients.google.com/auth',
                      data=params, headers=headers, proxies=proxy, verify=False)
    result = r.content.split('\n')

    if result[0] == 'Error=BadAuthentication':
        print 'Login failed, please check password and restart the script.'
        sys.exit(0)

    auth = [i for i in result if i.find('Auth=') != -1]
    if auth:
        account_token = auth[0].split('=')[1]
        if account_token is not None:
            return account_token
        else:
            raise Exception("Get token failed!")


def getAppDetailInfo(package_list, net_proxy):
    appinfo = []
    package_list = (i.strip() for i in package_list if i.strip())
    for package_name in package_list:
        app_url = "https://play.google.com/store/apps/details?id=" + package_name
        try:
            content = requests.get(app_url, proxies=net_proxy).content.decode('utf-8')
            tree = lxml.html.fromstring(content)
            allcontent = tree.xpath("//div[@class='content']/text()")
            appname = tree.xpath("//div[@class='document-title']/div/text()")[0].replace(",", " ").\
                replace("&amp;", "&").strip()
            version = allcontent[3].replace(',', '').strip()
            # Size = allContent[1].replace(',','').strip()
            # price = tree.xpath("//meta[@itemprop='price']")[0].values()[0].strip()
            # AppInfo['AppName'] = AppName
            # AppInfo['version'] = version
            # AppInfo['Size'] = Size
            # AppInfo['price'] = price
            print package_name, version
            # AppInfo[package_name] = version
            appinfo.append([package_name, version, ".apk", get_valid_name(appname)])
        except Exception, e:
            appinfo.append([package_name, 'NA', ".apk", package_name])

    return appinfo


# def generate_apk_download_list(remote_apk_info, local_apk_info):
#     apk_list = []

#     remote_apk_list = remote_apk_info.keys()

#     for k, v in remote_apk_info.iteritems():
#         apk_name_version = k + '_' + v
#         if local_apk_info.has_key(apk_name_version):
#             if remote_apk_info[k] != local_apk_info[apk_name_version]:
#                 # print remote_apk_info[i], local_apk_info[i]
#                 apk_list.append(k)
#         else:
#             apk_list.append(k)
#     return apk_list


def get_apk_url(request, token, device_id, proxy):
    params = {"version": 2, "request": request}
    headers = {"Content-type": "application/x-www-form-urlencoded",
               "Accept-Language": "en_US",
               "Authorization": "GoogleLogin auth=%s" % token,
               "X-DFE-Enabled-Experiments": "cl:billing.select_add_instrument_by_default",
               "X-DFE-Unsupported-Experiments":
                   "nocache:billing.use_charging_poller,market_emails,buyer_currency,prod_baseline,"
                   "checkin.set_asset_paid_app_field,shekel_test,content_ratings    ,"
                   "buyer_currency_in_app,nocache:encrypted_apk,recent_changes",
               "X-DFE-Device-Id": device_id,
               "X-DFE-Client-Id": "am-android-google",
               "User-Agent": "Android-Finsky/3.7.13 (api=3,versionCode=8013013,sdk=16,"
                             "device=crespo,hardware=herring,product=soju)",
               "X-DFE-SmallestScreenWidthDp": "320",
               "X-DFE-Filter-Level": "3",
               "Accept-Encoding": "",
               "Host": "android.clients.google.com"}
    r = requests.post('https://android.clients.google.com/market/api/ApiRequest',
                      data=params, headers=headers, verify=False, proxies=proxy)
    if r.status_code == 429:
        raise Exception("Too many request")
    elif r.status_code == 403:
        raise Exception("Forbidden")
    elif r.status_code == 401:
        raise Exception("Unauthorized")
    elif r.status_code != 200:
        print r.status_code
        raise Exception('Unexpected Resp')
    gzipped_content = r.content

    response = zlib.decompress(gzipped_content, 16 + zlib.MAX_WBITS)
    dl_url    = ""
    dl_cookie = ""

    match = re.search("(https?://[^:]+)", response)
    if match is None:
        raise Exception("Unexpected https response")
    else:
        dl_url = match.group(1)

    match = re.search("MarketDA.*?(\d+)", response)
    if match is None:
        raise Exception("Get cookie failed")
    else:
        dl_cookie = match.group(1)

    return dl_url + "#" + dl_cookie


def google_encode(buf, number):
    while number:
        if number < 128:
            mod = number
            number = 0
        else:
            mod = number % 128
            mod += 128
            number /= 128
        buf.append(mod)


def update_data(buf, data, raw=False):
    if raw is False:
        data_type = type(data).__name__
        if data_type == "bool":
            buf.append(1 if data is True else 0)
        elif data_type == "int":
            google_encode(buf, data)
        elif data_type == "str":
            google_encode(buf, len(data))
            for c in data:
                buf.append(ord(c))
        else:
            raise Exception("Unhandled data type : " + data_type)
    else:
        buf.append(data)


def generate_request(para):
    tmp = []
    pad = [10]
    result = []
    header_len = 0
    url_config = [[16], [24], [34], [42], [50],
                  [58], [66], [74], [82], [90],
                  [19, 82], [10], [20]]
    for i in range(13):
        if i == 4:
            update_data(tmp, '%s:%d' % (para[4], para[2]))
        elif i == 10:
            update_data(tmp, para[i])
            header_len = len(tmp) + 1
        elif i == 11:
            update_data(tmp, len(para[i]) + 2)
        else:
            update_data(tmp, para[i])

        tmp += url_config[i]

    update_data(result, header_len)
    result = pad + result + pad + tmp

    stream = ""
    for data in result:
        stream += chr(data)
    return base64.b64encode(stream, "-_")


def get_one_collection(cur_opener, url_prefix, index, page_num, proxy):
    content = cur_opener.get(url_prefix + "start=" + str(index) + "&num=" + str(page_num), proxies=proxy).content
    tree = lxml.html.fromstring(content)
    elements = tree.find_class("title")
    app_list = []
    for element in elements:
        item = lxml.html.tostring(element, pretty_print=True, encoding='utf-8')
        if "href" in item:
            package = item.split('"')[3].split('=')[1]
            app_list.append(package)
    return app_list


def download_apk(apk_url, package_name, app_name, proxy):
    headers = {"User-Agent": "AndroidDownloadManager/4.2.1 (Linux; U; Android 4.2.1; Galaxy Nexus Build/JRO03E)",
               "Accept-Encoding": ""}
    real_url = apk_url.split('#')[0]
    cookies = {"MarketDA": apk_url.split('#')[1]}
    response = requests.get(real_url, headers=headers, cookies=cookies, verify=False, proxies=proxy)
    print response.status_code
    with open(package_name + '.apk', 'wb') as op:
        op.write(response.content)
    aapt_cmd = config.default_davinci_path + '/platform-tools/aapt.exe d badging ' + package_name + '.apk'
    content = os.popen(aapt_cmd).read()
    if content.find('ERROR') == -1:
        info = content.split("'")
        try:
            version_index = info.index(" versionName=") + 1
        except ValueError:
            version_postfix = "unknown"
        else:
            version_postfix = info[version_index]

        app_name = get_valid_name(app_name)
        if os.path.exists(app_name + APK_NAME_VERSION_SEPARATOR + version_postfix + '.apk'):
            # os.remove(app_name + '_' + version_postfix +'.apk')
            os.remove(package_name + '.apk')
            # print 'No need copy %s'%package
        else:
            os.rename(package_name + '.apk', app_name + APK_NAME_VERSION_SEPARATOR + version_postfix + '.apk')
        return app_name + APK_NAME_VERSION_SEPARATOR + version_postfix + ".apk"


def get_apk_list(net_proxy, rank_begin, dl_num, category_selection):
    if category_selection == 1:
        category_list = ["https://play.google.com/store/apps/collection/topselling_free?hl=en&"]
    elif category_selection == 2:
        category_list = ["https://play.google.com/store/apps/category/GAME/collection/topselling_free?hl=en&"]
    elif category_selection == 3:
        category_list = ["https://play.google.com/store/apps/collection/topselling_free?hl=en&",
                         "https://play.google.com/store/apps/category/GAME/collection/topselling_free?hl=en&"]
    elif category_selection == 4:
        category_list = ["https://play.google.com/store/apps/collection/topselling_paid?hl=en&"]
    elif category_selection == 5:
        category_list = ["https://play.google.com/store/apps/category/GAME/collection/topselling_paid?hl=en&"]
    else:
        category_list = ["https://play.google.com/store/apps/collection/topselling_paid?hl=en&",
                        "https://play.google.com/store/apps/category/GAME/collection/topselling_paid?hl=en&"]

    if os.path.isfile(ResultLog):
        os.remove(ResultLog)

    print "\nInitializing..."
    opener = requests.session()
    remote_apk_list = []
    rank_begin -= 1
    actual_dl_num = dl_num - rank_begin

    if actual_dl_num < 50:
        page_num = actual_dl_num
    elif actual_dl_num <= 200:
        page_num = 50
    else:
        page_num = 100

    for category_url in category_list:

        for index in range(rank_begin, dl_num, page_num):
            if dl_num - index < page_num:
                page_num = dl_num - index
            remote_apk_list.extend(get_one_collection(opener, category_url, index, page_num, net_proxy))
            # print remote_apk_list
    return remote_apk_list


def download_google_by_account(email, password, net_proxy, input_package_list, print_flag=True):
    downloaded_apk_list = []
    downloaded_apk_with_version_list = []

    package_list = list(zip(*input_package_list)[0])
    app_name_dict = dict(input_package_list)
    google_token = get_google_token(email, password, net_proxy)

    while package_list:
        for package_name in package_list:
            if print_flag:
                print "Downloading %s.apk" % package_name
                print time.ctime()

            input_para = [google_token, True,
                          config.sdklevel, config.device_id,
                          config.device_name,
                          "en", "us", config.operator,
                          config.operator,
                          OPERATOR_LIST[config.country][config.operator],
                          OPERATOR_LIST[config.country][config.operator],
                          package_name, package_name]
            try:
                request = generate_request(input_para)
            except Exception:
                package_list.remove(package_name)
                continue

            try:
                apk_url = get_apk_url(request, google_token, config.device_id, net_proxy)
            except Exception, e:
                if str(e) == "Too many request" or str(e) == "Unauthorized":
                    print 'Due to Google Play Store Policy, the downloading request was blocked temporarily.'
                    print 'Generally, the account will be unblocked several hours later.'
                    print 'Please wait...'
                    time.sleep(ONE_HOUR)
                    print 'Retrying...'
                elif str(e) == "Forbidden":
                    print "The device id doesn't match Google account %s." % email
                    return False
                elif str(e) == "Unexpected https response":
                    package_list.remove(package_name)
                    print 'Download %s failed due to Unexpected https response' % package_name
                    with open(ResultLog, 'a') as op:
                        op.write(package_name + ",Fail,Unexpected https response,maybe incompitiable with current \
                                                device,Please download it manually\n")
                else:
                    # print 'Other error: %s'%str(e)
                    with open(ResultLog, 'a') as op:
                        op.write(package_name + ",Fail,Unkonwn error," + str(e) + os.linesep)
                    time.sleep(ONE_MIN * 5)
                break
            else:
                try:
                    apk_name_with_version = download_apk(apk_url, package_name, app_name_dict[package_name], net_proxy)
                except Exception, e:
                    # print e
                    pass
                else:
                    # with open(ResultLog, 'a') as op:
                    #     op.write(package_name + ",Succ\n")
                    downloaded_apk_list.append(package_name)
                    downloaded_apk_with_version_list.append(apk_name_with_version)
        package_list = list(set(package_list) - set(downloaded_apk_list))
        # print 'left apk number is %d'%(len(package_list))

    return downloaded_apk_with_version_list


def get_valid_account_list(account_list, password, proxy, package_name):
    temp_apk = []
    temp_apk.append(package_name)

    account_list = [i for i in account_list if download_google_by_account(i, password, proxy, temp_apk, False)]
    if account_list:
        print account_list
    return account_list


def download_google_with_specified_package(net_proxy, current_apk_path):

    while True:
        package_file = raw_input('Please enter the package list file path, such as c:\\script\\googleapk.txt\n')
        if package_file.count(':') == 0:
            package_file = os.getcwd() + '\\' + package_file

        if os.path.isfile(package_file):
            break
        else:
            print "%s doesn't exists, please re-enter the path" % package_file

    with open(package_file, 'r') as f:
        apk_list = [i for i in f.readlines() if i.strip()]

    if not apk_list:
        print 'Blank list!'
        sys.exit(0)

    apk_list = [[i.strip(), i.strip()] for i in apk_list]

    email_list, password = get_email_and_password()
    
    valid_email_list = get_valid_account_list(email_list, password, net_proxy,
                                              ['com.redphx.deviceid', 'com.redphx.deviceid'])
    if not valid_email_list:
        print 'No valid google account was found!'
        print "You may need to logout and login again, or register a new Google account"
        sys.exit(0)
    
    create_apk_folder(current_apk_path + '/Google_user_specified_' + time.strftime("%Y_%m_%d_%H_%M_%S",
                                                                                   time.localtime()))

    map(lambda x: os.remove(x),
        [i for i in os.listdir(os.getcwd()) if i.startswith('com.redphx.deviceid') and i[-4:] == '.apk'])

    if os.path.isfile(ResultLog):
        os.remove(ResultLog)

    dl_para = generate_dl_para(apk_list, valid_email_list, password)

    for i in dl_para:
        print 'using %s' % i[0]
        download_google_by_account(i[0], i[1], net_proxy, apk_list[i[2]:i[3]])


def get_email_and_password():
    if not config.email.strip():
        email_option = 'N'
    else:
        email_msg = '\nDo you want to use account ' + config.email + \
                    ' for Google Play login?\nIf Yes, please press key \'Y\'; If not, please press key \'N\':'
        # email_option = raw_input(email_msg)
        email_option = get_user_input(email_msg, 'y')

    if email_option == 'n' or email_option == 'N':
        while True:
            print 'Please enter all your Google accounts, separated by space.'
            print 'These accounts should have the same password and register on the same device.'
            input_email = raw_input('\n')
            if input_email.count('@') != len(input_email.split(' ')):
                print '[LogInfo]: Invalid email address, please re-input it'
            else:
                download_email = input_email.split(' ')
                email_list = [i.strip() for i in download_email]
                save_config('email', ' '.join(email_list))
                break
    else:
        email_list = config.email.split(' ')

    if config.password.strip():

        password_option = get_user_input('Do you want to use the Google account password :' + config.password, 'y')

        if password_option.lower() == 'y':
            password = config.password

        else:
            new_password = raw_input('Please enter your Google account password:')
            save_config('password', new_password)
            password = new_password
    else:
        # the default password is blank
        new_password = raw_input('Please enter your Google account password:')
        save_config('password', new_password)
        password = new_password

    return email_list, password


def generate_dl_para(apk_list, valid_email_list, password):
    begin = 0
    step = len(apk_list) / len(valid_email_list)
    dl_para = []

    for i in valid_email_list:
        dl_para.append([i,
                        password,
                        begin,
                        begin + step if i != valid_email_list[-1] else len(apk_list)])
        begin += step

    return dl_para


def download_google_entry(net_proxy, rank_begin, rank_end, pre_apk_save_path, current_apk_path, category_selection, category_list):

    email_list, password = get_email_and_password()

    valid_email_list = get_valid_account_list(email_list, password, net_proxy,
                                              ['com.redphx.deviceid', 'com.redphx.deviceid'])
    if not valid_email_list:
        print 'No valid google account was found!'
        print "You may need to logout and login again, or register a new Google account"
        sys.exit(0)

    apk_rank_list = get_apk_list(net_proxy, rank_begin, rank_end, category_selection)
    print 'Got APK list successfully.'
    unique_apk_list = list(set(apk_rank_list))
    unique_apk_list.sort(key=apk_rank_list.index)

    create_apk_folder(current_apk_path + '/Google')

    need_copy_apk_list = []

    print 'Fetching APKs version info...'

    remote_apk_info = getAppDetailInfo(unique_apk_list, net_proxy)
    all_data = zip(*remote_apk_info)
    pkg_lable_dict = dict(zip(all_data[0], all_data[3]))
    apk_lable_rank_list = [pkg_lable_dict[i] for i in apk_rank_list]
    apk_list = get_local_apk_version_for_google(pre_apk_save_path + '/Google', remote_apk_info, need_copy_apk_list)

    copy_apk(pre_apk_save_path + '/Google', current_apk_path + '/Google', need_copy_apk_list)

    # map(lambda x: os.remove(x),
    #     [i for i in os.listdir(os.getcwd()) if i.startswith('com.redphx.deviceid') and i[-4:] == '.apk'])

    if os.path.isfile(ResultLog):
        os.remove(ResultLog)

    dl_para = generate_dl_para(apk_list, valid_email_list, password)
    
    apk_info_with_version = []

    for i in dl_para:
        print 'using %s' % i[0]
        apk_info_with_version.extend(download_google_by_account(i[0], i[1], net_proxy, apk_list[i[2]:i[3]]))

    apk_info_with_version.extend(need_copy_apk_list)

    lable_version_dict = dict(zip([i.split(APK_NAME_VERSION_SEPARATOR)[0] for i in apk_info_with_version],
                                  [i.split(APK_NAME_VERSION_SEPARATOR)[1][:-4] for i in apk_info_with_version]))

    fail_list = [i for i in apk_rank_list if pkg_lable_dict[i] not in lable_version_dict]
    for i in fail_list:
        lable_version_dict[pkg_lable_dict[i]] = 'fail'
    apk_info_with_version = [pkg_lable_dict[i] + APK_NAME_VERSION_SEPARATOR + lable_version_dict[pkg_lable_dict[i]] + '.apk' for i in apk_rank_list]

    generate_folder_with_timestamp(apk_lable_rank_list,
                                   apk_info_with_version,
                                   rank_begin, rank_end - rank_begin + 1,
                                   category_selection, category_list,
                                   current_apk_path + '/Google')

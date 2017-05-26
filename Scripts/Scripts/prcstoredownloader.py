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
import sys
import os
import json
import lxml.html
import socket
import requests
import time
import threading
import dvcutility
from apkhelper import *


# -----------------------------------------------------
# Class for multi-thread downloading
# -----------------------------------------------------
class MultiDownloader(threading.Thread):
    def __init__(self, target_info, proxy):
        threading.Thread.__init__(self)
        self.info = target_info
        self.proxy = proxy
        self.logger = dvcutility.DVCLogger()

    def run(self):
        for k, v in self.info.iteritems():
            url = k.strip()
            filename = v

            try:
                self.logger.info('Downloading app from ' + url + '...')
                headers = {"User-Agent": "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:39.0) Gecko/20100101 Firefox/39.0"}

                r = requests.get(url, proxies=self.proxy, headers=headers)
            except requests.RequestException, e:
                self.logger.error(str(e))
            except socket.error, e:
                self.logger.error(str(e))
            else:
                try:
                    if r.status_code == 200:
                        temp_apk_name = get_string_hash(url) + ".apk"
                        with open(temp_apk_name, 'wb') as apk:
                            apk.write(r.content)
                        if not check_apk_integrity(temp_apk_name):
                            dvcutility.Enviroment.remove_item(temp_apk_name, False)
                        else:
                            os.rename(temp_apk_name, filename)
                except Exception as e:
                    self.logger.error("Failed to write {} due to {}".format(filename, str(e)))


class PRCDownloader(object):
    """docstring for PRCDownloader"""
    GENERAL_FILE_EXTENSION = ".apk"
    DANGLE_GAME_APP_EXTENTION = ".dpk"
    MUZHIWAN_GAME_APP_EXTENTION = ".gpk"

    def __init__(self, store_name, proxy, rank_begin, rank_end, previous_path, user_define_path, category_selection,
                 *args, **kargs):
        self.store_name = store_name
        self.proxy = proxy
        self.rank_begin = rank_begin
        self.rank_end = rank_end
        self.previous_path = previous_path
        self.user_define_path = user_define_path
        self.category_selection = category_selection
        self.logger = dvcutility.DVCLogger()
        self.logger.config(outfile='download.log')
        self.fetch_store_info = {
            'baidu': self.get_baidu_apk_list,
            'wandoujia': self.get_wandoujia_apk_list,
            'lenovo': self.get_lenovo_apk_list,
            'xiaomi': self.get_xiaomi_apk_list,
            'xiaomihd': self.get_xiaomi_hd_apk_list,
            'tencent': self.get_tencent_apk_list,
            'anzhi': self.get_anzhi_apk_list,
            'gfan': self.get_gfan_apk_list,
            'hiapk': self.get_hiapk_list,
            '91market': self.get_91market_list,
            'dangle': self.get_dangle_list,
            'muzhiwan': self.get_muzhiwan_list,
            '360': self.get_360_list,
        }
        self.category_list = kargs.get('category_list', [])

    def start_download(self):
        create_apk_folder(self.user_define_path + os.path.sep + self.store_name)

        apk_info = self.get_apk_info()
        self.downloader_multi_thread(self.pre_download(apk_info))

        generate_folder_with_timestamp_for_china(apk_info,
                                                 self.rank_begin,
                                                 self.rank_end - self.rank_begin + 1,
                                                 self.category_selection,
                                                 self.category_list,
                                                 self.user_define_path + '/' + self.store_name)

    def get_url_content(self, url, proxy=None):
        try:
            headers = {"User-Agent": "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:39.0) Gecko/20100101 Firefox/39.0"}
            req = requests.get(url, proxies=proxy, headers=headers)
        except requests.exceptions.ConnectionError as e:
            self.logger.info('Connection was lost, please check the network, %s' % str(e))
            sys.exit(0)
        except Exception, e:
            print e
            sys.exit(0)
        else:
            return req

    def pre_download(self, remote_apk_info):
        for i in remote_apk_info:
            apk_name = get_valid_name(i[0])
            if apk_name != i[0]:
                i[0] = apk_name

        need_copy_apk_list = []

        dl_link = get_local_apk_version(self.previous_path, remote_apk_info, need_copy_apk_list)

        copy_apk(self.previous_path, self.user_define_path, need_copy_apk_list)

        return dl_link

    def downloader_multi_thread(self, download_link):
        max_thread_number = 3

        urls = download_link.keys()
        while urls:
            if threading.activeCount() <= max_thread_number:
                info = dict()
                info[urls[0]] = download_link[urls[0]]
                t = MultiDownloader(info, self.proxy)
                t.start()
                urls.remove(urls[0])
            else:
                time.sleep(3)

        while threading.activeCount() > 1:
            time.sleep(10)
            self.logger.warning('Please wait, the APP downloading is still underway...')

    def get_apk_info(self):
        return self.fetch_store_info[self.store_name.lower()]()

    def __get_download_page_num(self, apk_number_per_page):
        page = self.rank_end / apk_number_per_page

        if self.rank_end < apk_number_per_page or self.rank_end % apk_number_per_page:
            page += 1

        return page

    def get_baidu_apk_list(self):
        all_dl_info = []
        # 101:APP 102:GAME
        # 501: System Tool 502:Theme and Wallpaper
        # 503:Social Communication 504:Life 506:Video
        # 507:Study 508:Photography 509:Travel #510:Finance
        # 401: Casual Puzzle 402:Role Playing 403:Shooting
        # 404: Strategy 405:Sports 406:Racing 407:Chess 408:Simulation
        baidu_category = ['101', '102', ['101', '102'],
                          '501', '502', '503', '504', '506', '507', '508', '509', '510',
                          '401', '402', '403', '404', '405', '406', '407', '408']

        apk_category = baidu_category[self.category_selection - 1]
        apk_number_per_page = 40

        if not isinstance(apk_category, list):
            apk_category = apk_category.split()

        for cid in apk_category:
            count = 0

            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = "http://as.baidu.com/a/rank?cid=" + cid + "&s=1&pn=" + str(index + 1)
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)
                all_match = tree.xpath("//a[@data-download_url]")

                for i in all_match:
                    count += 1
                    if count < self.rank_begin:
                        continue
                    if count > self.rank_end:
                        break

                    all_dl_info.append([i.values()[7],
                                        i.values()[5],
                                        i.values()[8],
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_wandoujia_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            app_category = ["app"]
        elif self.category_selection == 2:
            app_category = ["game"]
        else:
            app_category = ["app", "game"]

        url_prefix = "http://apps.wandoujia.com/api/v1/apps?type=weeklytop"

        for category in app_category:
            start = 0
            step = 12
            total = self.rank_end
            count = 0

            while start < total:
                url = url_prefix + category + "&max=" + str(step) + "&start=" + \
                    str(start) + "&opt_fields=packageName,apks.versionName"
                req = self.get_url_content(url, self.proxy)
                allcontent = json.loads(req.content)

                for i in allcontent:
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break

                    all_dl_info.append([i['packageName'],
                                        i['apks'][0]['versionName'],
                                        'http://apps.wandoujia.com/apps/' + i['packageName'] + '/download',
                                        self.__class__.GENERAL_FILE_EXTENSION])

                start += step

        return all_dl_info

    def get_lenovo_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            all_category = {'qbyy_hotest_flat_': "//a[@class='icons btn-3 f12 download fwhite']"}
        elif self.category_selection == 2:
            all_category = {'qbyx_hotest_flat_': "//a[@class='icons f12 btn-3 download fwhite']"}
        else:
            all_category = {'qbyy_hotest_flat_': "//a[@class='icons btn-3 f12 download fwhite']",
                            'qbyx_hotest_flat_': "//a[@class='icons f12 btn-3 download fwhite']"}
        le_store_prefix = 'http://www.lenovomm.com/category/'
        le_store_detail_url_prefix = 'http://www.lenovomm.com/appdetail/'

        apk_number_per_page = 36

        for k, v in all_category.iteritems():
            count = 0
            for i in range(self.__get_download_page_num(apk_number_per_page)):
                url = le_store_prefix + k + str(i + 1) + '.html'
                req = self.get_url_content(url, self.proxy)

                content = req.content.decode("utf-8")

                tree = lxml.html.fromstring(content)
                all_match = tree.xpath(v)

                for each_match in all_match:
                    count += 1
                    if count < self.rank_begin:
                        continue
                    if count > self.rank_end:
                        break
                    apk_url = each_match.values()[0]
                    apk_name = each_match.values()[1]
                    det_url = le_store_detail_url_prefix + apk_name + '/0'
                    det_tree = lxml.html.fromstring(self.get_url_content(det_url, self.proxy).content.decode("utf-8"))
                    det_match = det_tree.xpath("//li[@class]//a[@href]")
                    apk_version = det_match[0].values()[5]

                    all_dl_info.append([apk_name,
                                        apk_version,
                                        apk_url,
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_xiaomi_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["topList"]
        elif self.category_selection == 2:
            apk_category = ["gTopList"]
        else:
            apk_category = ["topList", "gTopList"]

        xiaomi_store_prefix = "http://app.mi.com/"

        apk_number_per_page = 48

        for category in apk_category:
            count = 0
            for i in range(self.__get_download_page_num(apk_number_per_page)):
                url = xiaomi_store_prefix + category + '?page=' + str(i + 1)
                req = self.get_url_content(url, self.proxy)

                content = req.content.decode("utf-8")

                tree = lxml.html.fromstring(content)
                all_match = (i for i in tree.xpath("//h5//a[@href]") if i.values())

                for each_match in all_match:
                    count += 1
                    if count < self.rank_begin:
                        continue
                    if count > self.rank_end:
                        break
             
                    detail_url = xiaomi_store_prefix[:-1] + each_match.values()[0]
                    req = self.get_url_content(detail_url, self.proxy)
                    content = req.content.decode("utf-8")

                    tree = lxml.html.fromstring(content)
                    dl_url_match = tree.xpath("//a[@class='download']")

                    detail_info_match = tree.xpath("//ul[@class=' cf']//li")
                    # if we want to save apk name in Chinese, please set the apk_name eqaul i.text
                    # apk_name = i.text
                    apk_name = detail_info_match[7].text
                    apk_version = detail_info_match[3].text
                    apk_dl_url = xiaomi_store_prefix[:-1] + dl_url_match[0].values()[0]

                    all_dl_info.append([apk_name,
                                        apk_version,
                                        apk_dl_url,
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_xiaomi_hd_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["topList"]
        elif self.category_selection == 2:
            apk_category = ["gTopList"]
        else:
            apk_category = ["topList", "gTopList"]

        xiaomi_store_prefix = "http://app.mi.com/"

        apk_number_per_page = 48

        for category in apk_category:
            count = 0
            for i in range(1, self.__get_download_page_num(apk_number_per_page) + 1):
                url = xiaomi_store_prefix + category + '?type=pad&page=' + str(i)

                req = self.get_url_content(url, self.proxy)

                content = req.content.decode("utf-8")

                tree = lxml.html.fromstring(content)
                all_match = (i for i in tree.xpath("//h5//a[@href]") if i.values())

                for each_match in all_match:
                    count += 1
                    if count < self.rank_begin:
                        continue
                    if count > self.rank_end:
                        break
               
                    apk_id = each_match.values()[0][1:].split('/')[1]
                    detail_url = xiaomi_store_prefix[:-1] + each_match.values()[0] + '?id=' + apk_id + '&type=pad'
                    req = self.get_url_content(detail_url, self.proxy)
                    content = req.content.decode("utf-8")

                    tree = lxml.html.fromstring(content)

                    detail_info_match = tree.xpath("//ul[@class=' cf']//li")
                    apk_name = detail_info_match[7].text
                    apk_version = detail_info_match[3].text
                    apk_dl_url = detail_url.replace('detail', 'download')

                    all_dl_info.append([apk_name,
                                        apk_version,
                                        apk_dl_url,
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_tencent_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["1"]
        elif self.category_selection == 2:
            apk_category = ["2"]
        else:
            apk_category = ["1", "2"]
        tencent_store_prefix = "http://android.myapp.com/myapp/cate/appList.htm?orgame="

        for category in apk_category:
            count = 0
            url = tencent_store_prefix + category + '&categoryId=0&pageSize=' + str(self.rank_end) + '&pageContext=0'
            req = self.get_url_content(url, self.proxy)
            content = req.content.decode("utf-8")
            data = json.loads(content)
            apk_number = data['count']
            for i in range(apk_number):
                count += 1
                if count < self.rank_begin:
                    continue

                if count > self.rank_end:
                    break

                all_dl_info.append([data['obj'][i]['pkgName'],
                                    data['obj'][i]['versionName'],
                                    data['obj'][i]['apkUrl'],
                                    self.__class__.GENERAL_FILE_EXTENSION])
        return all_dl_info

    def get_anzhi_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["1"]
        elif self.category_selection == 2:
            apk_category = ["2"]
        else:
            apk_category = ["1", "2"]
        anzhi_store_prefix = "http://www.anzhi.com/list_"

        apk_number_per_page = 15

        for category in apk_category:
            count = 0
            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = anzhi_store_prefix + category + '_' + str(index + 1) + '_hot.html'
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)
                apk_version_url_match = tree.xpath("//span[@class='app_version l']")
                apk_name_match = tree.xpath("//a[@style='float:left;']")
                for i, j in enumerate(apk_name_match):
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break

                    all_dl_info.append([j.text,
                                        apk_version_url_match[i].text[3:],
                                        'http://www.anzhi.com/dl_app.php?s=' + j.values()[1][6:-5] + '&n=5',
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_gfan_apk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["apps_7_1_"]
        elif self.category_selection == 2:
            apk_category = ["games_8_1_"]
        else:
            apk_category = ["apps_7_1_", "games_8_1_"]
        gfan_store_prefix = "http://apk.gfan.com/"

        apk_number_per_page = 30

        for category in apk_category:
            count = 0
            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = gfan_store_prefix + category + str(index + 1) + '.html'
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)

                apk_name_match = tree.xpath("//b//a[@target='_blank']")
                detail_url_match = tree.xpath("//li[@class='app-ico']//a[@href]")
                apk_url_match = tree.xpath("//li//a[@class='app-down-bt']")

                for i, j in enumerate(apk_name_match):
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break
                    ver_content = self.get_url_content(gfan_store_prefix + detail_url_match[i].values()[0], self.proxy)
                    ver_tree = lxml.html.fromstring(ver_content.content.decode("utf-8"))
                    full_version = ver_tree.xpath("//div[@class]//div[@class='app-infoAintro']//div[@class]//p")[0].text

                    all_dl_info.append([j.text.strip(),
                                        full_version[6:],
                                        apk_url_match[i].values()[0],
                                        self.__class__.GENERAL_FILE_EXTENSION])
        return all_dl_info

    def get_hiapk_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["apps?sort=5&pi="]
        elif self.category_selection == 2:
            apk_category = ["games?sort=5&pi="]
        else:
            apk_category = ["apps?sort=5&pi=", "games?sort=5&pi="]
        hiapk_store_prefix = "http://apk.hiapk.com/"

        apk_number_per_page = 10

        for category in apk_category:
            count = 0
            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = hiapk_store_prefix + category + str(index + 1)
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)

                apk_name_match = tree.xpath("//li[@class='list_item']//div//dl//dt//span//a")
                apk_version_match = tree.xpath("//li[@class='list_item']//div//dl//dt//span[2]")
                apk_url_match = tree.xpath("//li[@class='list_item']//div/div[2]//a")

                for i, j in enumerate(apk_url_match):
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break

                    all_dl_info.append([apk_name_match[i].text,
                                        apk_version_match[i].text[1:-1],
                                        hiapk_store_prefix[:-1] + j.values()[0],
                                        self.__class__.GENERAL_FILE_EXTENSION])
        return all_dl_info

    def get_91market_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["Soft/hot-16-"]
        elif self.category_selection == 2:
            apk_category = ["Game/hot-1-"]
        else:
            apk_category = ["Soft/hot-16-", "Game/hot-1-"]
        market_store_prefix = "http://play.91.com/android/"

        apk_number_per_page = 27

        for category in apk_category:
            count = 0
            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = market_store_prefix + category + str(index + 1) + '.html'
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)
                apk_name_match = tree.xpath("//div//div//h4//a")
                apk_version_match = tree.xpath("//span[@class='ss-list-ver']")
                apk_url_match = tree.xpath("//div[@class='ss-list-top']//a[@class='ss-btn']")
                for i, j in enumerate(apk_url_match):
                    # print type(apk_version_match[i].text)
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break

                    all_dl_info.append([apk_name_match[i].text,
                                        apk_version_match[i].text,
                                        j.values()[1],
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

    def get_muzhiwan_list(self):
        all_dl_info = []

        muzhiwan_store_prefix = "http://www.muzhiwan.com/top/rank/"

        apk_number_per_page = 20

        count = 0
        for index in range(self.__get_download_page_num(apk_number_per_page)):
            url = muzhiwan_store_prefix + str(index + 1) + '/'
            req = self.get_url_content(url, self.proxy)
            content = req.content.decode("utf-8")
            tree = lxml.html.fromstring(content)

            apk_name_match = tree.xpath("//dl//dd//h3//a")

            for i, j in enumerate(apk_name_match):
                count += 1
                if count < self.rank_begin:
                    continue

                if count > self.rank_end:
                    break

                ver_content = self.get_url_content("http://www.muzhiwan.com" +
                                                   j.values()[0][:-5] + "/download/#other_version", self.proxy)
                version_tree = lxml.html.fromstring(ver_content.content.decode("utf-8"))
                full_version = version_tree.xpath("//div[@class='mt20 g_r_info']//ul//li")[0].text.split('v')[1]
                dl_url = version_tree.xpath("//div[@class='pt20 clear']//a")[0].values()[1]

                if dl_url.find("http://www.muzhiwan.com") == -1:
                    dl_url = "http://www.muzhiwan.com" + dl_url

                all_dl_info.append([j.text,
                                    full_version,
                                    dl_url,
                                    self.__class__.MUZHIWAN_GAME_APP_EXTENTION])

        return all_dl_info

    def get_dangle_list(self):
        all_dl_info = []
        if self.category_selection == 1:
            apk_category = ["software/list_2_0_0_"]
        elif self.category_selection == 2:
            apk_category = ["game/list_2_0_0_0_0_0_0_0_0_0_0_"]
        else:
            apk_category = ["software/list_2_0_0_", "game/list_2_0_0_0_0_0_0_0_0_0_0_"]
        ddotcn_prefix = "http://android.d.cn/"

        apk_number_per_page = 30

        for category in apk_category:
            count = 0
            for index in range(self.__get_download_page_num(apk_number_per_page)):
                url = ddotcn_prefix + category + str(index + 1)
                if url.find('game') != -1:
                    url += '_0'
                url += '.html'
                req = self.get_url_content(url, self.proxy)
                content = req.content.decode("utf-8")
                tree = lxml.html.fromstring(content)

                apk_name_match = tree.xpath("//a[@class='app-img-out']")
                apk_version_match = tree.xpath("//p[@class='down-ac']")
                for i, j in enumerate(apk_name_match):
                    count += 1
                    if count < self.rank_begin:
                        continue

                    if count > self.rank_end:
                        break

                    apk_version = apk_version_match[i].text.strip()[3:]
                    apk_name = j.values()[3]

                    apk_identifier_info = j.values()[0]

                    apk_type = '1' if apk_identifier_info.find("software") == -1 else '2'

                    apk_identifier = apk_identifier_info[apk_identifier_info.rfind('/') + 1: -5]

                    req = requests.post(ddotcn_prefix + "rm/red/" + apk_type + "/" + apk_identifier, proxies=self.proxy)
                    dl_url = json.loads(req.content)['pkgs'][0]['pkgUrl']

                    extension = self.__class__.DANGLE_GAME_APP_EXTENTION \
                        if u"\u542b\u6570\u636e\u5305" in apk_name else self.__class__.GENERAL_FILE_EXTENSION

                    all_dl_info.append([apk_name,
                                        apk_version,
                                        dl_url,
                                        extension])

        return all_dl_info

    def get_360_list(self):
        all_dl_info = []

        if self.category_selection == 1:
            apk_category = ["1"]
        elif self.category_selection == 2:
            apk_category = ["2"]
        else:
            apk_category = ["1", "2"]

        store_prefix = "http://zhushou.360.cn/list/index/cid/"

        apk_number_per_page = 49

        for category in apk_category:
            count = 0
            for i in range(self.__get_download_page_num(apk_number_per_page)):
                url = store_prefix + category + '/order/download/?page=' + str(i + 1)
                req = self.get_url_content(url, self.proxy)

                content = req.content.decode("utf-8")

                tree = lxml.html.fromstring(content)
                all_match = (i for i in tree.xpath("//ul//li//h3//a[@href]") if i.values())

                for each_match in all_match:
                    count += 1
                    if count < self.rank_begin:
                        continue
                    if count > self.rank_end:
                        break
                    detail_url = "http://zhushou.360.cn" + each_match.values()[1]
                    req = self.get_url_content(detail_url, self.proxy)
                    content = req.content.decode("utf-8")

                    tree = lxml.html.fromstring(content)
                    dl_url_match = tree.xpath("//div//div//div//dl//dd/a[@href]")
                    apk_dl_url = dl_url_match[0].values()[1].rsplit('=&url=')[-1]
                    apk_name_match = tree.xpath("//div//div//div//dl//dd/h2//span")
                    apk_name = apk_name_match[0].text

                    apk_version_match = tree.xpath("//div[@class='base-info']//table//tbody//tr//td//strong")
                    apk_version = apk_version_match[2].tail

                    all_dl_info.append([apk_name,
                                        apk_version,
                                        apk_dl_url,
                                        self.__class__.GENERAL_FILE_EXTENSION])

        return all_dl_info

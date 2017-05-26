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
import dvcsubprocess
import adbhelper
import threading
import time
import base
import dvcutility
import shutil
import datetime
import csv
import sys
from ondevicedownloader import download_apk_on_device


class LaunchTimeTest(base.TestBase):
    activity_whitelist = {"com.facebook.orca": "com.facebook.orca.auth.StartScreenActivity",
                          "com.facebook.katana": ".LoginActivity",
                          "com.google.android.youtube": "com.google.android.apps.youtube.app.WatchWhileActivity",
                          "com.skype.raider": "com.skype.raider.Main",
                          "com.netflix.mediaclient": "UIWebViewActivity",
                          "com.amazon.clouddrive.photos": "com.amazon.clouddrive.photos.display.LauncherActivity",
                          "com.amazon.mShop.android.shopping": "com.amazon.mShop.android.home.HomeActivity",
                          "com.android.chrome": "com.google.android.apps.chrome.Main",
                          "com.audible.application": "com.audible.application.SplashScreen",
                          "com.facebook.moments": "com.facebook.moments.MomentsStartActivity",
                          "com.google.android.calendar": "com.android.calendar.AllInOneActivity",
                          "com.google.android.gm": "com.google.android.gm.ConversationListActivityGmail",
                          "com.google.android.music": "com.android.music.activitymanagement.TopLevelActivity",
                          "com.google.android.talk": "com.google.android.talk.SigningInActivity",
                          "com.gamecircus.CoinDozerPirates": "com.gamecircus.player.GCPlayerAlias",
                          "com.gamecircus.PrizeClaw": "com.gamecircus.player.GCPlayerAlias",
                          "com.groupon": "com.groupon.activity.TodaysDeal",
                          "com.infonow.bofa": "com.infonow.bofa.Launcher",
                          "com.leftover.CoinDozer": "com.gamecircus.player.GCPlayerAlias",
                          "com.lookout": "com.lookout.ui.LoadDispatch",
                          "com.shopkick.app": "com.shopkick.app.activity.AppScreenActivity",
                          "com.ubercab": "com.ubercab.UBUberActivity",
                          "fishnoodle.snowfall_free": "fishnoodle.snowfall_free.HowToActivity",
                          "org.mozilla.firefox": "org.mozilla.firefox.App",
                          "tunein.player": "tunein.leanback.LeanBackActivity",
                          }

    def __init__(self, *args, **kwargs):
        super(LaunchTimeTest, self).__init__(*args, **kwargs)

        self.dvc_folder = os.path.abspath(self.kwargs['dvc_path'])
        self.dvc = os.path.join(self.dvc_folder, "DaVinci.exe")
        self.adb_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "adb.exe")
        self.aapt_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "aapt.exe")
        self.device_id = self.kwargs['device_id']
        self.existed_lt_case_path = self.kwargs.get('existed_lt_case_path')
        self.launchtime_running_mode = self.kwargs.get('launchtime_running_mode', False)
        self.running_times = self.kwargs.get('running_times', 1)
        self.lt_timeout = int(self.kwargs.get('generate_ini_timeout', 60))
        self.interval = int(self.kwargs.get('interval', 30))
        self.fresh_run_flag = self.kwargs.get('fresh_run_flag', False)
        self.clear_cache_flag = self.kwargs.get('clear_cache_flag', False)
        self.usb_replug_flag = self.kwargs.get('usb_replug_flag', False)
        self.pre_launch_apk_folder = self.kwargs.get('pre_launch_apk_folder', '')
        self.logcat_list_path = self.kwargs.get('logcat_list_path', '')
        self.apk_mode = self.kwargs.get("apk_mode", "downloaded")
        self.apk_folder = self.kwargs.get("apk_folder")
        self.ondevice_timeout = self.kwargs.get("ondevice_timeout", 600)
        self.camera_mode = 'HypersoftCamera' if self.kwargs.get("camera_mode") == 'hsc' else\
            'CameraIndex' + self.kwargs.get("camera_index", '0')
        self.worker = dvcsubprocess.DVCSubprocess()
        self.adb = adbhelper.AdbHelper(self.adb_path)
        self.aapt = dvcutility.AAPTHelper(self.aapt_path)
        self.adb.device_id = self.device_id
        self.dvc_timeout = 300
        self.apk_launch_sleep = 60
        self.work_dir = os.path.join(os.path.abspath(self.apk_folder),
                                     "LaunchTime_Test_" + dvcutility.Enviroment.get_cur_time_str())
        self.logger = dvcutility.DVCLogger()
        self.logger.config(outfile=os.path.join(self.work_dir, 'launchtime_log.txt'))

    def start(self):
        if self.running_flag:
            return -1
        self.running_flag = True
        self.__run_entry()

    def stop(self):
        self.running_flag = False

    def force_stop(self):
        self.worker.kill()

    def __get_device_port(self, com_list):
        width, height = self.adb.get_device_resolution()
        result = ''
        port_found = False
        for port in com_list:
            dvcutility.Enviroment.generate_dvc_config(self.dvc_folder, self.device_id,
                                                      self.camera_mode, height, width, port)
            self.logger.info("Disconnecting #1 interface of {0}...".format(port))
            self.worker.Popen("{0} -device {1} -p {2} -noagent".format(self.dvc, self.device_id, "usb_out_1.qs"))

            if self.device_id not in self.adb.get_connected_devices():
                port_found = True

            self.logger.info("Connecting #1 interface of {0}...".format(port))

            self.worker.Popen("{0} -device {1} -p {2} -noagent".format(self.dvc, self.device_id, "usb_in_1.qs"))
            if port_found:
                result = port
                break
            self.logger.info("Disconnecting #2 interface of {0}...".format(port))

            self.worker.Popen("{0} -device {1} -p {2} -noagent".format(self.dvc, self.device_id, "usb_out_2.qs"))
            if self.device_id not in self.adb.get_connected_devices():
                port_found = True

            self.logger.info("Connecting #2 interface of {0}...".format(port))
            self.worker.Popen("{0} -device {1} -p {2} -noagent".format(self.dvc, self.device_id, "usb_in_2.qs"))

            if port_found:
                result = port
                break

        return result

    def __calibration(self):
        camera_list = dvcutility.Enviroment.get_connected_camera_list()

        if not camera_list or len(camera_list) < int(self.camera_mode.split('CameraIndex')[1]) + 1:
            self.logger.warning("{} was lost.".format(self.camera_mode))
            return False

        calibration_log_file = os.path.join(self.work_dir, 'calibration_' +
                                            dvcutility.Enviroment.get_cur_time_str() + '.txt')
        logfile = open(calibration_log_file, 'w')
        self.logger.info("Reset calibration settings...")
        self.worker.Popen(self.dvc + ' -device ' + self.device_id + ' -c -1', self.dvc_timeout,
                          call_out=logfile, call_err=logfile)
        self.logger.info("Start overall calibration.")
        self.worker.Popen(self.dvc + ' -device ' + self.device_id + ' -c 6', self.dvc_timeout * 5,
                          call_out=logfile, call_err=logfile)
        self.logger.info("Finished calibration.")
        logfile.close()
        kw = ('Overall Calibration Done',)
        return dvcutility.Enviroment.search_dvc_log(calibration_log_file, kw)

    def preparation(self):
        try:
            os.mkdir(self.work_dir)
        except Exception as e:
            self.logger.warning("Failed to create {0} due to {1}".format(self.work_dir, str(e)))
            self.work_dir = os.getcwd()



        if self.usb_replug_flag:
            com_port_list = dvcutility.Enviroment.get_com_port()

            if not com_port_list:
                self.logger.error("No available com port, please connect the usb switch to host and re-test.")
                sys.exit(-1)

            valid_com_port = self.__get_device_port(com_port_list)
            if not valid_com_port:
                self.logger.error("No available com port, please connect the usb switch to host and re-test.")
                sys.exit(-1)

            self.logger.info("{0} is connected to {1}".format(self.device_id, valid_com_port))

        else:
            width, height = self.adb.get_device_resolution()
            dvcutility.Enviroment.generate_dvc_config(self.dvc_folder, self.device_id, self.camera_mode, height, width)

        self.logger.info("Camera Info: {}".format(self.camera_mode))

        if self.camera_mode != 'HypersoftCamera' and self.launchtime_running_mode != '3' and not self.__calibration():
            self.logger.info("Set camera mode to HypersoftCamera.")
            dvc_config = dvcutility.XmlOperator(os.path.join(self.dvc_folder, "DaVinci.config"))
            dvc_config.updateTextByTagName("androidDevices//AndroidTargetDevice//screenSource",
                                           self.camera_mode, "HypersoftCamera")
            self.camera_mode = 'HypersoftCamera'

        if self.launchtime_running_mode == '2':
            if self.apk_mode == 'on-device':
                # fetch the list firstly
                return self.__get_remote_apk_info()
            else:
                return [os.path.join(self.apk_folder, i) for i in os.listdir(self.apk_folder) if i.endswith(".apk")]
        elif self.launchtime_running_mode == '3':
            if self.logcat_list_path and os.path.exists(self.logcat_list_path):
                with open(self.logcat_list_path) as f:
                    content = f.readlines()
                return [i.strip() for i in content if i.strip()]
            else:
                return []

        else:
            return [os.path.join(self.existed_lt_case_path, i) for i in os.listdir(self.existed_lt_case_path)
                    if os.path.isdir(os.path.join(self.existed_lt_case_path, i))]

    def __get_remote_apk_info(self):
        try:
            import socks
            import socket
            import config
            from downloadFromGooglePlay import get_apk_list, getAppDetailInfo

            sock = config.default_socks_proxy.split(':')
            socks.setdefaultproxy(socks.PROXY_TYPE_SOCKS5, sock[0], int(sock[1]))

            socket.socket = socks.socksocket
            user_define_proxy = {config.default_proxy_type: config.default_socks_proxy}
            category_selection = 1
            start_rank = 1
            dl_num = 10

            try:
                category_selection = int(self.kwargs.get("category"))
                start_rank = int(self.kwargs.get("start_rank"))
                dl_num = int(self.kwargs.get("dl_num"))
            except Exception as e:
                self.logger.warning("Failed to covert para due to {0}".format(str(e)))

            apk_list = get_apk_list(user_define_proxy, start_rank, start_rank - 1 + dl_num, category_selection)
            remote_apk_info = getAppDetailInfo(apk_list, user_define_proxy)
            if category_selection == 1:
                for index, apk_info in enumerate(remote_apk_info):
                    apk_info[2] = 'app_' + str(index + start_rank)
            elif category_selection == 2:
                for index, apk_info in enumerate(remote_apk_info):
                    apk_info[2] = 'game_' + str(index + start_rank)
            else:
                for i in range(dl_num):
                    remote_apk_info[i][2] = 'app_' + str(i + start_rank)
                    remote_apk_info[i + dl_num][2] = 'game_' + str(i + start_rank)

            return remote_apk_info
        
        except Exception as e:
            self.logger.error("got {0} on __get_remote_apk_info func".format(e))
            return []

    def __install_and_launch_apk(self, apk_path):
        pkg_name, _, _ = self.aapt.get_package_version_apk_name(apk_path)
                
        if not pkg_name:
            self.logger.error('Failed to detect package_name of {0}'.format(apk_path))
            return

        self.adb.install_apk(apk_path)

        if not self.adb.check_package_installed(pkg_name):
            self.logger.error('Failed to detect {0} installed'.format(pkg_name))
            return

        if pkg_name in self.activity_whitelist:
            activity_name = self.activity_whitelist[pkg_name]
        else:
            activity_name = self.aapt.get_package_launchable_activity(apk_path)

        if activity_name:
            self.adb.launch_apk(pkg_name, activity_name)
        else:
            self.logger.error('Failed to detect activity_name of {0}'.format(apk_path))

    def __dirty_launch_apks(self):
        if self.pre_launch_apk_folder and os.path.exists(self.pre_launch_apk_folder):
            apks = [os.path.join(self.pre_launch_apk_folder, i) for i in os.listdir(self.pre_launch_apk_folder)
                    if i.endswith(".apk")]
            map(self.__install_and_launch_apk, apks)

    def __create_clear_cache_qs(self, dst, pkg_name):
        qs_prefix = "[Configuration]\nCapturedImageHeight=1280\n" \
                    "CapturedImageWidth=800\nCameraPresetting=HIGH_RES\n" \
                    "PackageName=\nActivityName=\nAPKName=\nPushData=\n" \
                    "StartOrientation=\nCameraType=Cameraless\nVideoRecording=True\n" \
                    "RecordedVideoType=H264\n[Events and Actions]\n"
        qs_postfix = "1    3000: OPCODE_TOUCHDOWN 2786 2283 0 id=app:id/clear_cache_button(0)&ratio=(0.3419,0.4844)\n" \
                     "2    3300: OPCODE_TOUCHUP 2786 2283 0\n"
        adb_cmd = "{0} -s {1} shell am start -a android.settings.APPLICATION_DETAILS_SETTINGS -n" \
                  " com.android.settings/com.android.settings.applications.InstalledAppDetails" \
                  " -d {2}".format(self.adb_path, self.device_id, pkg_name)
        opcode_cmd = "0       100: OPCODE_CALL_EXTERNAL_SCRIPT {0}\n".format(adb_cmd)
        with open(os.path.join(dst, 'clearcache.qs'), 'w') as f:
            f.write(qs_prefix + opcode_cmd + qs_postfix)

    def __run_entry(self):

        self.adb.uninstall_3rd_app()
        test_target = self.preparation()
        self.__dirty_launch_apks()

        if self.launchtime_running_mode == '2':  
            if self.apk_mode == "on-device":
                t = threading.Thread(target=self.__run_lt_test_online, name='ltonline', args=(test_target,))
            else:
                t = threading.Thread(target=self.__run_lt_test_with_local_apk, name='ltlocalapk', args=(test_target,))
        elif self.launchtime_running_mode == '3':
            t = threading.Thread(target=self.__run_lt_test_by_logcat, name='ltlogcat', args=(test_target,))
        else:
            t = threading.Thread(target=self.__run_lt_test_with_existed_cases, name='ltlocalcases', args=(test_target,))
        
        t.start()

    def __run_qs(self, qs_file):
        self.worker.Popen(self.dvc + ' -device ' + self.device_id + ' -p ' + qs_file, self.dvc_timeout)

    def __run_launch_time(self, ini_filename, running_times, pkg_name):

        max_retry = running_times * 2
        timeout_result = []
        while self.running_flag and running_times > 0 and max_retry > 0:
                
            out_put_file_name = ini_filename + '_' + dvcutility.Enviroment.get_cur_time_str() + '.txt'
            try:
                logfile = open(out_put_file_name, 'w')
            except Exception as e:
                self.logger.warning("Failed to create {0} due to {1}".format(out_put_file_name, str(e)))
                logfile = open(os.devnull, 'w')

            if self.fresh_run_flag and not self.adb.clear_data_by_package(pkg_name):
                self.logger.warning('Failed to clear {0} data at round {1}'.format(pkg_name, running_times))
                continue

            if self.clear_cache_flag:
                self.logger.info('Clearing cache for {0} ...'.format(pkg_name))
                cmd = "{0} -device {1} -p {2}".format(self.dvc, self.device_id,
                                                      os.path.join(os.path.dirname(ini_filename), "clearcache.qs"))

                self.worker.Popen(cmd, self.dvc_timeout, call_out=logfile, call_err=logfile)
            
            self.logger.info('Running {0}...'.format(ini_filename))

            self.worker.Popen(self.dvc + ' -res 360 -device ' + self.device_id + ' -l ' + ini_filename + ' -offline ',
                              self.dvc_timeout, call_out=logfile, call_err=logfile)
            
            logfile.close()

            hsc_keyword = ("Error starting Camera-less with error status code", "Camera-less OnError status")
            lt_timeout_keyword = ("Failed to get launch time result",)

            hsc_issue_flag = dvcutility.Enviroment.search_dvc_log(out_put_file_name, hsc_keyword)
            lt_timeout_flag = dvcutility.Enviroment.search_dvc_log(out_put_file_name, lt_timeout_keyword)

            if hsc_issue_flag or lt_timeout_flag:
                log_folder_path = os.path.join(os.path.dirname(ini_filename), "_Logs")
                logs_file = [os.path.join(log_folder_path, i) for i in os.listdir(log_folder_path)
                             if os.path.isdir(os.path.join(log_folder_path, i))]
                logs_file.sort(reverse=True)
                if len(logs_file) > 1:
                    dvcutility.Enviroment.remove_item(logs_file[1], False, self.logger.error)

                if hsc_issue_flag:
                    self.logger.info('Rebooting device {0}, please wait ...'.format(self.device_id))
                    if not self.adb.reboot_device() and self.usb_replug_flag and not self.__replug_device():
                        self.logger.info("Failed to recover device.")
            else:
                timeout_result.append(dvcutility.Enviroment.search_dvc_log(out_put_file_name, ("Timeout",)))

                running_times -= 1

            max_retry -= 1
            time.sleep(self.interval)

        return all(timeout_result)

    def __pre_launch_apk(self, pkg_name, activity_name):
        if not self.fresh_run_flag:
            self.logger.info('Launching {0} ...'.format(pkg_name))

            self.adb.launch_apk(pkg_name, activity_name)

            time.sleep(self.apk_launch_sleep)

            self.logger.info('Force stopping {0}...'.format(pkg_name))
            self.adb.force_stop_by_package(pkg_name)

    def __generate_ini_file(self, dst_path, pkg_name, activity_name, timeout):
        max_retry = 3
        self.logger.info("Generating launch time ini file for {0}".format(pkg_name))

        cmd = self.dvc + ' -device ' + self.device_id + ' -generate ' \
                                                        '{0}/{1} {2}'.format(pkg_name, activity_name, timeout)

        src_path = os.path.join(self.dvc_folder, "Temp", pkg_name)
        while max_retry:
            dvcutility.Enviroment.remove_item(src_path, True, self.logger.warning)

            self.worker.Popen(cmd, self.dvc_timeout)

            max_retry -= 1

            try:
                shutil.copy(os.path.join(src_path, pkg_name + ".ini"), dst_path)
                shutil.copy(os.path.join(src_path, pkg_name + ".png"), dst_path)
            except Exception as e:
                self.logger.error('Failed to copy {0} due to {1}'.format(dst_path, e))
            finally:
                self.adb.force_stop_by_package(pkg_name)
                time.sleep(5)

            if os.path.exists(os.path.join(dst_path, pkg_name + ".ini")) \
                    and os.path.exists(os.path.join(dst_path, pkg_name + ".png")):
                break

    def __replug_device(self, timeout=200):
        self.logger.info('Replugging device {0}, please wait ...'.format(self.device_id))
        max_retry = 3
        res = False
        while True:
            status = self.adb.get_state()
            if status == "device":
                res = True
                break

            max_retry -= 1
            if max_retry == 0:
                break
            self.logger.info("USB Replug the device...")

            if not os.path.exists("USB_Recover.qs"):
                self.logger.warning("USB_Recover.qs was missing.")
                break

            cmd = "{0} -device {1} -noagent -p {2}".format(self.dvc, self.device_id, "USB_Recover.qs")

            self.worker.Popen(cmd, timeout, False, None, None)

        return res

    @staticmethod
    def __convert_result_in_ms(data):
        res = []
        for i in data:
            if i[:-2].find('s') != -1:
                res.append(int(i[:i.index('s')]) * 1000 + int(i[i.index('s') + 1:-2]))
            elif i[:-2].isdigit():
                res.append(int(i[:-2]))
        return res

    def __install_package_from_device(self, device_name, package_name):
        try:
            # Call API to download and install from Google play on device.
            package_list = []
            package_list.append(package_name)
            installed = False
            is_compatible = True
            has_error = False
            if not self.adb.check_package_installed('com.android.vending'):
                self.logger.warning("Google play doesn't exist on the device")
                has_error = True
            else:
                # save_config('default_davinci_path', Davinci_folder)
                retry_count = 2
                while retry_count > 0:
                    retry_count -= 1
                    is_started, output_fld = download_apk_on_device(package_list, 600, False, os.getcwd(), device_name)
                    beginning = datetime.datetime.now()
                    if not is_started:
                        # Something wrong with parameter input for download_apk_on_device. Just return.
                        has_error = True
                        break
                    else:
                        download_log = r'%s\on_device_qscript\%s_log.txt' % (output_fld, package_name)
                        # # The key message means the accept button is clicked and then in downloading.
                        key_list = [' - start to click accept button', ' Click point ']
                        error_found, key_found = dvcutility.Enviroment.check_dvc_log(download_log, key_list)
                        is_timeout = False

                        if not key_found:
                            # Check if it is a incompatible APK.
                            is_compatible = dvcutility.Enviroment.check_compatibility_result(output_fld)
                            if not is_compatible:
                                self.logger.info("{0} is a incompatible apk".format(package_name))
                            else:
                                installed = self.adb.check_package_installed(package_name)

                                if not installed:
                                    self.logger.info("Failed to download {0}".format(package_name))
                        else:
                            # Wait for the package to be installed.
                            timeout = 900
                            try:
                                value = self.ondevice_timeout
                                if value:
                                    timeout = int(value)
                            except Exception, e:
                                has_error = True
                                pass

                            sleep_time = 10
                            start = datetime.datetime.now()
                            # wait the installation complete and check it.
                            while True:
                                installed = self.adb.check_package_installed(package_name)
                                self.logger.info('Checking {0} installation status...'.format(package_name))
                                if installed:
                                    self.logger.info('{0} has been installed'.format(package_name))
                                    break
                                if (datetime.datetime.now() - start).seconds < timeout:
                                    self.logger.info("    Wait for %s seconds..." % str(sleep_time))
                                    time.sleep(sleep_time)
                                else:
                                    is_timeout = True
   
                                    self.adb.force_stop_by_package("com.android.vending")
                                    break

                        if installed:
                            elapsed_time = (datetime.datetime.now() - beginning).seconds
                            self.logger.info("{0} downloaded and installed in {1}".format(package_name, elapsed_time))
                            try:
                                if output_fld is not None:
                                    shutil.rmtree(output_fld)
                            except Exception as e:
                                self.logger.warning("Failed to delete {0} due to {1}".format(output_fld, str(e)))
                                has_error = True
                            break

                        else:
                            if not is_compatible:
                                break

                            if retry_count > 0:
                                self.logger.info("  - Try to on-device install again ...")
        except Exception as e:
            self.logger.warning("Failed to download apk due to {0}".format(str(e)))
            has_error = True

        return installed, is_compatible, has_error

    def __run_lt_test_online(self, pkg_info_list):
        self.logger.info('start to run launch time with on-device downloading...')

        reboot_threshold = 5

        max_re_generate = 4

        all_data = zip(*pkg_info_list)
        pkg_lable_dict = dict(zip(all_data[0], all_data[2]))

        for index, pkg_info in enumerate(pkg_info_list):
            print '*' * 50

            package_name = pkg_info[0]
            if not self.running_flag:
                break

            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(pkg_info_list)))
            # 0 download apk
            installed, is_compatible, has_error = self.__install_package_from_device(self.device_id, package_name)

            if installed:
                dst_path = os.path.join(self.work_dir, package_name)
                try:
                    os.mkdir(dst_path)
                except Exception as e:
                    self.logger.warning('Failed to create {0} due to {1}'.format(dst_path, e))

                downloaded_apk_path = os.path.join(dst_path, package_name + ".apk")
                self.adb.pull_file(self.adb.get_apk_path(package_name), downloaded_apk_path)

                # 1 fetch apk to get activity name
                pkg_name, _, _ = self.aapt.get_package_version_apk_name(downloaded_apk_path)

                if not pkg_name:
                    self.logger.error('Failed to detect package_name of {0}'.format(downloaded_apk_path))
                    continue

                if pkg_name in self.activity_whitelist:
                    activity_name = self.activity_whitelist[pkg_name]
                else:
                    activity_name = self.aapt.get_package_launchable_activity(downloaded_apk_path)

                    if not activity_name:
                        self.logger.error('Failed to detect activity_name of {0}'.format(downloaded_apk_path))
                        continue

                self.logger.info('Package Name: {0}'.format(pkg_name))
                self.logger.info('Activity Name: {0}'.format(activity_name))
                print '*' * 50
                self.adb.check_battery(20, 600)
                for multiple in range(1, max_re_generate):
                    self.__pre_launch_apk(pkg_name, activity_name)

                    self.__generate_ini_file(dst_path, pkg_name, activity_name, self.lt_timeout * multiple)
                    # run launch time testing

                    ini_name = dvcutility.Enviroment.get_file_list_with_postfix(dst_path, ".ini")
                    qs_name = dvcutility.Enviroment.get_file_list_with_postfix(dst_path, ".qs")

                    if not ini_name:
                        self.logger.error('Failed to detect ini_name on {0}'.format(dst_path))
                        continue

                    # step 2 run qs
                    if qs_name:
                        self.__run_qs(qs_name)

                    # step 3
                    if reboot_threshold > 0:
                        reboot_threshold -= 1
                    else:
                        self.logger.info('Rebooting device {0} ...'.format(self.device_id))

                        if not self.adb.reboot_device() and self.usb_replug_flag and not self.__replug_device():
                            self.logger.info("Failed to recover device.")
                            continue

                        reboot_threshold = 5

                    self.logger.info('Run {0} for {1} time ...'.format(ini_name, self.running_times))

                    if not self.__run_launch_time(ini_name, self.running_times, pkg_name):
                        break

                self.adb.uninstall(package_name)

        self.__generate_report(self.work_dir, '.', pkg_lable_dict)

    def __run_lt_test_with_local_apk(self, apk_path_set):
        self.logger.info('start to run launch time with local apk...')
        reboot_threshold = 5

        max_re_generate = 4

        for index, apk_path in enumerate(apk_path_set):
            print '*' * 50
            pkg_name, _, _ = self.aapt.get_package_version_apk_name(apk_path)
            if not pkg_name:
                self.logger.error('Failed to detect package_name of {0}'.format(apk_path))
                continue

            if pkg_name in self.activity_whitelist:
                activity_name = self.activity_whitelist[pkg_name]
            else:
                activity_name = self.aapt.get_package_launchable_activity(apk_path)
                if not activity_name:
                    self.logger.error('Failed to detect activity_name of {0}'.format(apk_path))
                    continue

            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(apk_path_set)))
            self.logger.info('Package Name: {0}'.format(pkg_name))
            self.logger.info('Activity Name: {0}'.format(activity_name))
            print '*' * 50

            self.adb.check_battery(20, 600)

            dst_path = os.path.join(self.work_dir, pkg_name)

            if not os.path.exists(dst_path):
                try:
                    os.mkdir(dst_path)
                    shutil.copy(apk_path, dst_path)
                except Exception as e:
                    self.logger.warning('Failed to create {0} due to {1}'.format(dst_path, e))

            self.logger.info("Uninstalling {0} ...".format(pkg_name))
            self.adb.uninstall(pkg_name)

            self.logger.info("Installing {0} ...".format(apk_path))
            self.adb.install_apk(apk_path)
            if not self.adb.check_package_installed(pkg_name):
                self.logger.error('Failed to detect {0} installed'.format(pkg_name))
                continue

            for multiple in range(1, max_re_generate):
                self.__pre_launch_apk(pkg_name, activity_name)

                self.__generate_ini_file(dst_path, pkg_name, activity_name, self.lt_timeout * multiple)

                ini_name = dvcutility.Enviroment.get_file_list_with_postfix(dst_path, ".ini")
                qs_name = dvcutility.Enviroment.get_file_list_with_postfix(dst_path, ".qs")

                if not ini_name:
                    self.logger.error('Failed to detect ini_name on {0}'.format(dst_path))
                    continue

                # step 2 run qs
                if qs_name:
                    self.__run_qs(qs_name)
                # step 3
                if reboot_threshold > 0:
                    reboot_threshold -= 1
                else:
                    self.logger.info('Rebooting device {0} ...'.format(self.device_id))

                    if not self.adb.reboot_device() and self.usb_replug_flag and not self.__replug_device():
                        self.logger.info("Failed to recover device.")
                        continue

                    reboot_threshold = 5

                if self.clear_cache_flag:
                    self.__create_clear_cache_qs(dst_path, pkg_name)

                self.logger.info('Run {0} for {1} time ...'.format(ini_name, self.running_times))

                if not self.__run_launch_time(ini_name, self.running_times, pkg_name):
                    break

            self.adb.uninstall(pkg_name)

        self.__generate_report(self.work_dir, '.')

    def __run_lt_test_with_existed_cases(self, test_path_collection):
        self.logger.info('start to run launch time with existed cases...')

        reboot_threshold = 5

        for index, path in enumerate(test_path_collection):
            print '*' * 50

            if not self.running_flag:
                break

            ini_name = dvcutility.Enviroment.get_file_list_with_postfix(path, ".ini")
            if not ini_name:
                self.logger.info('Skipping {0}/{1} due to missing ini.'.format(index + 1, len(test_path_collection)))
                continue

            apk_name = dvcutility.Enviroment.get_file_list_with_postfix(path, ".apk")

            if not apk_name:
                self.logger.info('Skipping {0}/{1} due to missing apk.'.format(index + 1, len(test_path_collection)))
                continue

            pkg_name, _, _ = self.aapt.get_package_version_apk_name(apk_name)

            if pkg_name in self.activity_whitelist:
                activity_name = self.activity_whitelist[pkg_name]
            else:
                activity_name = self.aapt.get_package_launchable_activity(apk_name)

            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(test_path_collection)))
            self.logger.info('Package Name: {0}'.format(pkg_name))
            self.logger.info('Activity Name: {0}'.format(activity_name))
            print '*' * 50

            self.adb.check_battery(20, 600)

            # 0 uninstall apk
            if pkg_name:
                self.logger.info("Uninstalling {0} ...".format(pkg_name))
                self.adb.uninstall(pkg_name)

            # step 1 install apk
            self.logger.info("Installing {0} ...".format(apk_name))
            self.adb.install_apk(apk_name)

            if not self.adb.check_package_installed(pkg_name):
                self.logger.error('Failed to detect {0}'.format(pkg_name))
                continue

            qs_name = dvcutility.Enviroment.get_file_list_with_postfix(path, ".qs")

            self.__pre_launch_apk(pkg_name, activity_name)
            # step 2 run qs if exists
            if qs_name:
                self.__run_qs(qs_name)

            # step 3
            if reboot_threshold > 0:
                reboot_threshold -= 1
            else:
                self.logger.info('Rebooting device {0} ...'.format(self.device_id))

                if not self.adb.reboot_device() and self.usb_replug_flag and not self.__replug_device():
                    self.logger.info("Failed to recover device.")
                    continue

                reboot_threshold = 5

            self.logger.info('Run {0} for {1} time ...'.format(ini_name, self.running_times))

            self.__run_launch_time(ini_name, self.running_times, pkg_name)

            self.adb.uninstall(pkg_name)

        self.__generate_report(self.existed_lt_case_path, '..')

    def __run_launch_time_by_logcat(self, running_times, pkg_name, activity_name):
        tmp_file = os.path.join(os.getcwd(), "logcat_tmp.txt")
        result = [pkg_name, activity_name]

        while self.running_flag and running_times > 0:
            self.adb.force_stop_by_package(pkg_name)
            self.adb.logcat_clear()
            self.logger.info('Launching {0} ...'.format(pkg_name))
            self.adb.launch_apk(pkg_name, activity_name)
            retry_times = self.lt_timeout / 10 if self.lt_timeout > 10 else 1
            running_times -= 1
            while retry_times:
                s = self.adb.get_top_activity()
                if s.find(pkg_name + '/' + activity_name) != -1:
                    time.sleep(5)
                    self.adb.logcat_to_file(tmp_file)
                    with open(tmp_file) as f:
                        logcat_content = f.read()

                    if logcat_content.find("Displayed") != -1:
                        break
                    else:
                        retry_times -= 1
                        time.sleep(5)
                else:
                    retry_times -= 1
                    time.sleep(5)

            if os.path.exists(tmp_file):
                with open(tmp_file) as f:
                    logcat_content = f.readlines()
                logcat_content = [i.strip() for i in logcat_content if i.strip() and i.strip().find("Displayed") != -1]
                lt_value = logcat_content[-1].split(': +')[1] if logcat_content else 'N/A'
                if lt_value.find(' (total') != -1:
                    lt_value = lt_value.split(' (total')[0]
            else:
                lt_value = 'N/A'

            result.append(lt_value)
            time.sleep(self.interval)

        self.adb.logcat_clear()

        return result

    def __run_lt_test_by_logcat(self, test_apk_activity):
        self.logger.info('start to run launch time with logcat...')

        all_result = []

        for index, i in enumerate(test_apk_activity):

            pkg_name = i.split('/')[0]
            activity_name = i.split('/')[1]

            print '*' * 50
            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(test_apk_activity)))
            self.logger.info('Package Name: {0}...'.format(pkg_name))
            self.logger.info('Activity Name: {0}'.format(activity_name))
            print '*' * 50

            self.adb.check_battery(20, 600)

            if not self.adb.check_package_installed(pkg_name):
                self.logger.error('Failed to detect {0} installed'.format(pkg_name))
                continue
            
            each_result = self.__run_launch_time_by_logcat(self.running_times, pkg_name, activity_name)
            all_result.append(each_result)

        self.__generate_report_for_logcat(self.work_dir, all_result)

    def __generate_report(self, dst, relative_path, rank_info={}):
        print '*' * 50
        self.logger.info('Generating report on {0}...'.format(dst))

        tested_folder = (os.path.join(dst, i) for i in os.listdir(dst) if os.path.isdir(os.path.join(dst, i)))

        all_result = (dvcutility.Enviroment.get_sub_folder_name(os.path.join(i, "_Logs"),
                                                                self.running_times) for i in tested_folder)
        valid_result = map(sorted, [i for i in all_result if i])
        result = []
        result_content = []
        csv_result = []

        all_sheets = []
        result.append('Launch Time Test Summary')

        device_cpu_type = self.adb.get_prop_value('ro.product.cpu.abi')
        device_build = self.adb.get_prop_value('ro.build.id')
        device_model = self.adb.get_prop_value('ro.product.model')
        device_release = self.adb.get_prop_value('ro.build.version.release')
        houdini_version = self.adb.get_houdini_version()

        result.append(['Device Model', 'Android Version', 'Device CPU', 'Device Build', 'Houdini Version', 'Fresh Run',
                       'Camera Mode'])

        result.append([device_model, device_release, device_cpu_type, device_build,
                       houdini_version, 'Yes' if self.fresh_run_flag else 'No',
                       'Camera' if self.camera_mode != 'HypersoftCamera' else 'HypersoftCamera'])

        result_head = ['App Name', 'Package Name', 'App Version', 'App Type']

        csv_result_head = ['Package name', 'App Version', 'App Type', 'Device Model', 'Android Version',
                           'Device CPU', 'Device Build', 'Houdini Version']

        for i in range(self.running_times):
            result_head.append('Round ' + str(i + 1) + '(ms)')
            csv_result_head.append('Round ' + str(i + 1))
        result_head.extend(['Average Result(ms)', 'Standard Deviation(ms)', 'Detail Link', 'Rank'])
        result.append(result_head)

        csv_result_head.extend(['Detail Link'])
        csv_result.append(csv_result_head)

        for case_result in valid_result:
            case_csv_result = []
            valid_result = []
            delete_apk_flag = True
            sub_case_path = os.path.dirname(os.path.dirname(case_result[0]))
            apk_path = dvcutility.Enviroment.get_file_list_with_postfix(sub_case_path, ".apk")
            if apk_path is None:
                self.logger.warning("{} doesn't contain apk".format(sub_case_path))
                continue
            pkg_name, apk_version, app_name = self.aapt.get_package_version_apk_name(apk_path)
            apk_type = self.aapt.get_apk_type(apk_path)

            lt_value_result = [app_name, pkg_name, apk_version, apk_type]
            lt_value_result_csv = [pkg_name, apk_version, apk_type, device_model, device_release,
                                   device_cpu_type, device_build, houdini_version]
            for each_result in case_result:
                if not os.path.exists(os.path.join(each_result, 'ReportSummary.xml')):
                    continue

                xml = dvcutility.XmlOperator(os.path.join(each_result, 'ReportSummary.xml'))
                subcase_result = xml.getAttributeValue("subcase")
                pkg_name_result = xml.getAttributeValue("case")

                if subcase_result and pkg_name_result:
                    lt_value = subcase_result[0]["launchtime"]
                    valid_result.append(float(lt_value) * 1000)
                    lt_value_result.append(float(lt_value) * 1000)
                    lt_value_result_csv.append(float(lt_value))
                else:
                    lt_value_result.append("N/A")
                    lt_value_result_csv.append("N/A")
                    delete_apk_flag = False

            if self.launchtime_running_mode == '2' and self.apk_mode != "on-device" and delete_apk_flag:
                dvcutility.Enviroment.remove_item(apk_path, False, self.logger.warning)
            
            lt_value_result.append(dvcutility.Enviroment.get_average_value(valid_result))
            lt_value_result.append(dvcutility.Enviroment.get_std_deviation(valid_result))
            case_folder_name = os.path.basename(sub_case_path)
            lt_value_result.append('=HYPERLINK("{0}")'.format(os.path.join(relative_path, case_folder_name)))
            lt_value_result_csv.append('=HYPERLINK("{0}")'.format(os.path.join(relative_path, case_folder_name)))
            lt_value_result.append(rank_info.get(pkg_name, "N/A"))

            result_content.append(lt_value_result)
            case_csv_result.append(lt_value_result_csv)
            csv_result.extend(case_csv_result)

        if rank_info:
            sorted_result = sorted(result_content, key=lambda x: (x[-1].split('_')[0], int(x[-1].split('_')[1])))
            result.extend(sorted_result)
        else:
            result.extend(result_content)

        all_sheets.append(result)
        result_excel = dvcutility.ExcelWriter(os.path.join(self.work_dir, 'LaunchTimeReport.xlsx'))
        result_excel.generate_excel(all_sheets)

        writer = csv.writer(file(os.path.join(self.work_dir, 'LaunchTimeReport.csv'), 'wb'))
        map(writer.writerow, csv_result)

        print '*' * 50

    def __generate_report_for_logcat(self, dst, lt_data):
        result = []

        all_sheets = []
        print '*' * 50
        self.logger.info('Generating report on {0}...'.format(dst))

        device_cpu_type = self.adb.get_prop_value('ro.product.cpu.abi')
        device_build = self.adb.get_prop_value('ro.build.id')
        device_model = self.adb.get_prop_value('ro.product.model')
        device_release = self.adb.get_prop_value('ro.build.version.release')
        houdini_version = self.adb.get_houdini_version()

        result.append('Launch Time Test Summary')
        result.append(['Device Model', 'Android Version', 'Device CPU', 'Device Build', 'Houdini Version'])

        result.append([device_model, device_release, device_cpu_type, device_build, houdini_version])
        result_head = ['Package name', 'Activity Name']

        for i in range(self.running_times):
            result_head.append('Round ' + str(i + 1))
        result_head.append("Average Result")
        result_head.append("Standard Deviation")
        result.append(result_head)

        for i in lt_data:
            average_value = dvcutility.Enviroment.get_average_value((self.__convert_result_in_ms(i[2:]))) + 'ms'
            i.append(average_value)
            i.append(str(dvcutility.Enviroment.get_std_deviation(self.__convert_result_in_ms(i[2:-1]))) + 'ms')
            result.append(i)

        all_sheets.append(result)
        result_excel = dvcutility.ExcelWriter(os.path.join(dst, 'LaunchTimeReport(logcat).xlsx'))
        result_excel.generate_excel(all_sheets)

        print '*' * 50


class LaunchTimeGenerator(base.TestFactory):
    """docstring for ClassName"""
    def __init__(self, *args, **kwargs):
        super(LaunchTimeGenerator, self).__init__(*args, **kwargs)
        self.args = args
        self.kwargs = kwargs
    
    def get_tester(self):
        return LaunchTimeTest(*self.args, **self.kwargs)

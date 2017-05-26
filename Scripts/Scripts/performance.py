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


class InstallationTest(base.TestBase):
    def __init__(self, *args, **kwargs):
        super(InstallationTest, self).__init__(*args, **kwargs)

        self.dvc_folder = os.path.abspath(self.kwargs['dvc_path'])
        self.dvc = os.path.join(self.dvc_folder, "DaVinci.exe")
        self.adb_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "adb.exe")
        self.aapt_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "aapt.exe")
        self.device_id = self.kwargs['device_id']

        self.running_times = self.kwargs.get('running_times', 1)

        self.interval = int(self.kwargs.get('interval', 30))
        self.apk_folder = self.kwargs.get("apk_folder")

        self.worker = dvcsubprocess.DVCSubprocess()
        self.adb = adbhelper.AdbHelper(self.adb_path)
        self.aapt = dvcutility.AAPTHelper(self.aapt_path)

        self.adb.device_id = self.device_id
        self.dvc_timeout = 300

        self.work_dir = os.path.join(os.path.abspath(self.apk_folder), "Installation_Test_" +
                                     dvcutility.Enviroment.get_cur_time_str())
        self.logger = dvcutility.DVCLogger()
        self.logger.config(outfile=os.path.join(self.work_dir, 'installation_log.txt'))

        self.qs = '[Configuration]\nCameraPresetting=HIGH_RES\nPackageName=\nActivityName=\nIconName=\nAPKName=\n' \
                  'PushData= \nStatusBarHeight=33-33\nNavigationBarHeight=64-64\nStartOrientation=portrait\n' \
                  'CameraType=\nVideoRecording=\nMultiLayerMode=False\nRecordedVideoType=\nReplayVideoType=\n' \
                  '[Events and Actions]\n' \
                  '1  504.8290: OPCODE_INSTALL_APP 0 \n' \
                  '2  852.6450: OPCODE_EXIT'

    def start(self):
        if self.running_flag:
            return -1
        self.running_flag = True
        self.__run_entry()

    def stop(self):
        self.running_flag = False

    def force_stop(self):
        self.worker.kill()

    @staticmethod
    def __get_launch_time_value_from_dvc_log(dvc_log_path):
        with open(dvc_log_path) as f:
            content = [i.strip() for i in f.readlines() if i.strip()]
        result = [i for i in content if i.find('Install Package Time: ') != -1]
        return result[0].split('Install Package Time: ')[1] if result else 'N/A'

    def preparation(self):
        try:
            os.mkdir(self.work_dir)
        except Exception as e:
            self.work_dir = os.getcwd()
            self.logger.warning("Failed to create {0} due to {1}".format(self.work_dir, str(e)))
        width, height = self.adb.get_device_resolution()
        dvcutility.Enviroment.generate_dvc_config(self.dvc_folder, self.device_id, 'HyperSoftCam', height, width)
   
        return [os.path.join(self.apk_folder, i) for i in os.listdir(self.apk_folder) if i.endswith(".apk")]

    def __run_entry(self):
        self.adb.uninstall_3rd_app()
        test_target = self.preparation()
 
        t = threading.Thread(target=self.__run_installation_test_worker, name='installationtest', args=(test_target,))
        
        t.start()

    def __run_qs(self, qs_file):

        self.worker.Popen(self.dvc + ' -device ' + self.device_id + ' -p ' + qs_file, self.dvc_timeout)

    def __run_installation_testing(self, running_times, apk_folder_path, pkg_name):
        max_retry = running_times * 2

        apk_name = dvcutility.Enviroment.get_file_list_with_postfix(apk_folder_path, ".apk")
        qs_template = self.qs
        content = qs_template.split('\n')
        content[-2] = content[-2] + apk_name

        qs_content = [i + '\n' for i in content]
        qs_file_path = os.path.join(apk_folder_path, 'installation.qs')

        with open(qs_file_path, 'w') as f:
            f.writelines(qs_content)

        while self.running_flag and running_times > 0 and max_retry > 0:
            max_retry -= 1
            self.adb.uninstall(pkg_name)
            time.sleep(5)
            self.logger.info("Testing {0}...".format(apk_name))
            self.__run_qs(qs_file_path)
            log_folder_path = os.path.join(apk_folder_path, "_Logs")
            logs_file = [os.path.join(log_folder_path, i) for i in os.listdir(log_folder_path)
                         if os.path.isdir(os.path.join(log_folder_path, i))]

            logs_file.sort(reverse=True)
            dvc_log_file = os.path.join(logs_file[1], "DaVinci.log.txt") if len(logs_file) > 1 else ''
            hsc_keyword = ["Error starting Camera-less with error status code", "Camera-less OnError status"]
            hsc_issue_flag = dvcutility.Enviroment.search_dvc_log(dvc_log_file, hsc_keyword) if dvc_log_file else False
            if hsc_issue_flag:
                dvcutility.Enviroment.remove_item(logs_file[1], False, self.logger.error)

                self.logger.info('Rebooting device {0} due to detect hps issue ...'.format(self.device_id))
                self.adb.reboot_device()
            else:
                running_times -= 1

            time.sleep(self.interval)

    def __run_installation_test_worker(self, apk_path_set):
        self.logger.info('start to run installation testing with local apk...')

        reboot_threshold = 5

        for index, apk_path in enumerate(apk_path_set):
            pkg_name, _, _ = self.aapt.get_package_version_apk_name(apk_path)

            print '*' * 50
            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(apk_path_set)))
            self.logger.info('Package Name: {0}...'.format(pkg_name))
            print '*' * 50

            self.adb.check_battery(20, 600)

            self.logger.info("Uninstalling {0} ...".format(pkg_name))
            self.adb.uninstall(pkg_name)
            dst_path = os.path.join(self.work_dir, pkg_name)

            if not os.path.exists(dst_path):
                try:
                    os.mkdir(dst_path)
                    shutil.copy(apk_path, dst_path)
                except Exception as e:
                    self.logger.warning('Failed to create {0} due to {1}'.format(dst_path, e))

            # step 3
            if reboot_threshold > 0:
                reboot_threshold -= 1
            else:
                self.logger.info('Rebooting device {0} ...'.format(self.device_id))
                self.adb.reboot_device()
                reboot_threshold = 5

            self.__run_installation_testing(self.running_times, dst_path, pkg_name)

            self.adb.uninstall(pkg_name)

        self.__generate_report(self.work_dir, '.')

    def __generate_report(self, dst, relative_path):
        print '*' * 50
        self.logger.info('Generating report on {0}...'.format(dst))

        tested_folder = (os.path.join(dst, i) for i in os.listdir(dst) if os.path.isdir(os.path.join(dst, i)))

        all_result = (dvcutility.Enviroment.get_sub_folder_name(os.path.join(i, "_Logs"), self.running_times)
                      for i in tested_folder)
        valid_result = map(sorted, [i for i in all_result if i])
        result = []
        all_sheets = []
        result.append('APK Installation Test Summary')

        device_cpu_type = self.adb.get_prop_value('ro.product.cpu.abi')
        device_build = self.adb.get_prop_value('ro.build.id')
        device_model = self.adb.get_prop_value('ro.product.model')
        device_release = self.adb.get_prop_value('ro.build.version.release')
        houdini_version = self.adb.get_houdini_version()

        result.append(['Device Model', 'Android Version', 'Device CPU', 'Device Build', 'Houdini Version'])

        result.append([device_model, device_release, device_cpu_type, device_build, houdini_version])

        result_head = ['App Name', 'Package name', 'App Version', 'App Type']
        for i in range(self.running_times):
            result_head.append("Round {0}".format(i + 1))

        result_head.append('Average Result')
        result_head.append('Standard Deviation')
        result_head.append('Result Folder')
        result.append(result_head)

        for case_result in valid_result:
            apk_folder_path = os.path.dirname(os.path.dirname(case_result[0]))
            apk_path = dvcutility.Enviroment.get_file_list_with_postfix(apk_folder_path, ".apk")
            pkg_name, apk_version, app_name = self.aapt.get_package_version_apk_name(apk_path)
            apk_folder_path = os.path.dirname(apk_path)
            apk_type = self.aapt.get_apk_type(apk_path)
            each_row = [app_name, pkg_name, apk_version, apk_type]
            valid_result = []
            for each_result in case_result:
                log_path = os.path.join(each_result, "DaVinci.log.txt")
                if not os.path.exists(log_path):
                    continue

                installation_time = self.__get_launch_time_value_from_dvc_log(log_path)
                if installation_time != 'N/A':
                    valid_result.append(int(installation_time[:-2]))
                each_row.append(installation_time)

            each_row.append(dvcutility.Enviroment.get_average_value(valid_result) + ' ms')
            each_row.append(str(dvcutility.Enviroment.get_std_deviation(valid_result)) + ' ms')
            # each_row.append('=HYPERLINK("{0}")'.format(os.path.dirname(apk_path)))
            each_row.append('=HYPERLINK("{0}")'.format(os.path.join(relative_path, os.path.basename(apk_folder_path))))
            result.append(each_row)

        all_sheets.append(result)
        result_excel = dvcutility.ExcelWriter(os.path.join(self.work_dir, 'InstallationReport.xlsx'))
        result_excel.generate_excel(all_sheets)
        print '*' * 50


class InstallationGenerator(base.TestFactory):
    """docstring for ClassName"""
    def __init__(self, *args, **kwargs):
        super(InstallationGenerator, self).__init__(*args, **kwargs)
        self.args = args
        self.kwargs = kwargs
    
    def get_tester(self):
        return InstallationTest(*self.args, **self.kwargs)

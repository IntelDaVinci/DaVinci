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
import sys


class PowerTest(base.TestBase):
    def __init__(self, *args, **kwargs):
        super(PowerTest, self).__init__(*args, **kwargs)

        self.dvc_folder = os.path.abspath(self.kwargs['dvc_path'])
        self.dvc = os.path.join(self.dvc_folder, "DaVinci.exe")
        self.adb_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "adb.exe")
        self.aapt_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "aapt.exe")
        self.device_id = self.kwargs['device_id']
        self.running_times = self.kwargs.get('running_times', 1)
        self.power_qs_path = self.kwargs.get("power_qs_path")
        self.apk_folder = self.kwargs.get("apk_folder")
        self.worker = dvcsubprocess.DVCSubprocess()
        self.adb = adbhelper.AdbHelper(self.adb_path)
        self.adb.device_id = self.device_id
        self.dvc_timeout = 300

        self.work_dir = os.path.join(os.path.abspath(self.apk_folder), "PowerTest_" +
                                     dvcutility.Enviroment.get_cur_time_str())

        self.logger = dvcutility.DVCLogger()
        self.logger.config(outfile=os.path.join(self.work_dir, 'powertest_log.txt'))

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
    def __update_qs_videorecording(file_path):
        with open(file_path) as f:
            original_content = f.read()

        if original_content.find("VideoRecording=True") != -1:
            original_content = original_content.replace("VideoRecording=True", "VideoRecording=False")
            with open(file_path, 'w') as f:
                f.write(original_content)

    def __get_device_port(self, com_list):
        width, height = self.adb.get_device_resolution()
        result = ''
        port_found = False
        for port in com_list:
            dvcutility.Enviroment.generate_dvc_config(self.dvc_folder, self.device_id, "Disabled", height, width, port)
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

    def preparation(self):
        try:
            os.mkdir(self.work_dir)
        except Exception as e:
            self.work_dir = os.getcwd()
            self.logger.warning("Failed to create {0} due to {1}".format(self.work_dir, str(e)))

        if self.adb.get_prop_value('ro.product.cpu.abi') != 'x86':
            self.logger.error("Device {0} doesn't support power test.".format(self.device_id))
            sys.exit(-1)

        com_port_list = dvcutility.Enviroment.get_com_port()

        if not com_port_list:
            self.logger.error("No available com port, please connect the usb switch to host and re-test.")
            sys.exit(-1)

        valid_com_port = self.__get_device_port(com_port_list)
        if not valid_com_port:
            self.logger.error("No available com port, please connect the usb switch to host and re-test.")
            sys.exit(-1)

        self.logger.info("{0} is connected to {1}".format(self.device_id, valid_com_port))

        qs_files = [os.path.join(self.power_qs_path, i) for i in os.listdir(self.power_qs_path) if i.endswith(".qs")]
        map(self.__update_qs_videorecording, qs_files)

        return qs_files

    def __run_entry(self):
        test_target = self.preparation()

        t = threading.Thread(target=self.__run_power_test_worker, name='powertest', args=(test_target,))

        t.start()

    def __run_power_test_worker(self, qs_file_set):
        self.logger.info('start to run power testing...')

        for index, qs in enumerate(qs_file_set):

            print '*' * 50
            self.logger.info('Testing Progress: {0}/{1}...'.format(index + 1, len(qs_file_set)))
            self.logger.info('QS Name: {0}...'.format(qs))
            print '*' * 50

            dst_path = os.path.join(self.work_dir, os.path.basename(qs))
            try:
                os.mkdir(dst_path)
                shutil.copy(qs, dst_path)
            except Exception as e:
                self.logger.warning("Failed to copy {0} to {1} due to {2}".format(qs, dst_path, str(e)))
            target_qs = os.path.join(dst_path, os.path.basename(qs))
            self.__run_power_testing(target_qs, self.running_times)

        self.__generate_report(self.work_dir, '.')

    def __run_power_testing(self, qs_file, running_times):
        while self.running_flag and running_times > 0:

            self.adb.reboot_device(self.device_id)

            cmd = "{0} -device {1} -p {2} -noagent -power mpm".format(self.dvc, self.device_id, qs_file)
            self.logger.info("Testing {0}...".format(qs_file))

            self.worker.Popen(cmd, self.dvc_timeout)
            running_times -= 1

    def __generate_report(self, dst, relative_path):
        print '*' * 50
        self.logger.info('Generating report on {0}...'.format(dst))

        tested_folder = (os.path.join(dst, i) for i in os.listdir(dst) if os.path.isdir(os.path.join(dst, i)))

        all_result = (dvcutility.Enviroment.get_sub_folder_name(os.path.join(i, "_Logs"), self.running_times)
                      for i in tested_folder)
        valid_result = map(sorted, [i for i in all_result if i])
        result = []
        all_sheets = []
        result.append('Power Test Summary')

        device_cpu_type = self.adb.get_prop_value('ro.product.cpu.abi')
        device_build = self.adb.get_prop_value('ro.build.id')
        device_model = self.adb.get_prop_value('ro.product.model')
        device_release = self.adb.get_prop_value('ro.build.version.release')
        houdini_version = self.adb.get_houdini_version()

        result.append(['Device Model', 'Android Version', 'Device CPU', 'Device Build', 'Houdini Version'])

        result.append([device_model, device_release, device_cpu_type, device_build, houdini_version])

        result_head = ['QS Name', 'Package Name', 'Activity Name']
        for i in range(self.running_times):
            result_head.append("Round {0}".format(i + 1))

        result_head.append('Average Result')
        result_head.append('Standard Deviation')
        result_head.append('Result Folder')
        result.append(result_head)

        for case_result in valid_result:
            apk_folder_path = os.path.dirname(os.path.dirname(case_result[0]))
            print '1st path {0}'.format(apk_folder_path)
            qs_path = dvcutility.Enviroment.get_file_list_with_postfix(apk_folder_path, ".qs")
            pkg_name = dvcutility.Enviroment.get_value_from_qs(qs_path, "PackageName")
            activity_name = dvcutility.Enviroment.get_value_from_qs(qs_path, "ActivityName")
            apk_folder_path = os.path.dirname(qs_path)
            print '2nd path {0}'.format(apk_folder_path)

            each_row = [os.path.basename(qs_path), pkg_name, activity_name]
            valid_result = []
            for each_result in case_result:
                log_path = os.path.join(each_result, "ReportSummary.xml")
                if not os.path.exists(log_path):
                    continue

                xml = dvcutility.XmlOperator(os.path.join(each_result, 'ReportSummary.xml'))
                power_result = xml.getAttributeValue("case")
                elapsed_power = power_result[0]["totalreasult"] if power_result else 'N/A'

                if elapsed_power != 'N/A':
                    valid_result.append(float(elapsed_power[:-3]))
                each_row.append(elapsed_power)

            each_row.append(dvcutility.Enviroment.get_average_value(valid_result) + ' mWh')
            each_row.append(str(dvcutility.Enviroment.get_std_deviation(valid_result)) + ' mWh')
            each_row.append('=HYPERLINK("{0}")'.format(os.path.join(relative_path, os.path.basename(apk_folder_path))))
            result.append(each_row)

        all_sheets.append(result)
        result_excel = dvcutility.ExcelWriter(os.path.join(self.work_dir, 'PowerTestReport.xlsx'))
        result_excel.generate_excel(all_sheets)
        print '*' * 50


class PowerGenerator(base.TestFactory):
    def __init__(self, *args, **kwargs):
        super(PowerGenerator, self).__init__(*args, **kwargs)
        self.args = args
        self.kwargs = kwargs

    def get_tester(self):
        return PowerTest(*self.args, **self.kwargs)

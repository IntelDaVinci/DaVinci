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
import random


class StorageTest(base.TestBase):
    storage_test_type = ("mtpoveremmc", "emmcdd")

    def __init__(self, *args, **kwargs):
        super(StorageTest, self).__init__(*args, **kwargs)

        self.adb_path = os.path.join(self.kwargs['dvc_path'], "platform-tools", "adb.exe")
        self.device_id = self.kwargs['device_id']
        self.test_type = self.storage_test_type[self.kwargs['storage_test_type'] - 1]
        self.running_times = self.kwargs.get('running_times', 1)
        self.tmp_file_size = self.kwargs.get('tmp_file_size', 100)
        self.apk_folder = self.kwargs.get("apk_folder")
        self.worker = dvcsubprocess.DVCSubprocess()
        self.adb = adbhelper.AdbHelper(self.adb_path)
        self.adb.device_id = self.device_id
        self.adb.timeout = 600

        self.work_dir = os.path.join(os.path.abspath(self.apk_folder), "Storage_Test_" +
                                     dvcutility.Enviroment.get_cur_time_str())

        self.logger = dvcutility.DVCLogger()
        self.logger.config(outfile=os.path.join(self.work_dir, 'powertest_log.txt'))
        self.writeresult = []
        self.readresult = []
        self.files_to_del = []

    def start(self):
        if self.running_flag:
            return -1
        self.running_flag = True
        self.__run_entry()

    def stop(self):
        self.running_flag = False

    def force_stop(self):
        self.worker.kill()

    def __generate_tmp_file(self):
        tmp_file = os.path.join(self.work_dir, "test.txt")
        with open(tmp_file, 'w') as f:
            f.write(random.choice('davinci') * self.tmp_file_size * 1024 * 1024)

        self.files_to_del.append(tmp_file)

        return tmp_file

    def preparation(self):
        try:
            os.mkdir(self.work_dir)
        except Exception as e:
            self.work_dir = os.getcwd()
            self.logger.warning("Failed to create {0} due to {1}".format(self.work_dir, str(e)))

        if self.test_type != "emmcdd":
            return self.__generate_tmp_file()
        else:
            return []

    def __run_entry(self):
        test_target = self.preparation()

        if self.test_type == "mtpoveremmc":
            t = threading.Thread(target=self.__run_mtprwemmc_test_worker, name='mtprwemmc', args=(test_target,))
        else:
            t = threading.Thread(target=self.__run_emmcrw_test_worker, name='emmcrw', args=(test_target,))

        t.start()

    def path_exist_checker(path):
        def path_checker(func):
            def wrapped(self, *args, **kwargs):
                if not self.adb.check_path_existence(path):
                    self.logger.error("{} doesn't exist.".format(path))

                    self.stop()
                else:
                    self.logger.info('start to run {} testing...'.format(self.test_type))

                return func(self, *args, **kwargs)
            return wrapped
        return path_checker

    @path_exist_checker("/mnt/sdcard/")
    def __run_emmcrw_test_worker(self, tmp_file):
        self.__run_emmcrw_testing(self.running_times)
        self.__generate_report(self.work_dir)

    def __run_emmcrw_testing(self, running_times):
        self.adb.reboot_device(self.device_id)
        self.adb.timeout = 1200
        while self.running_flag and running_times > 0:
            self.logger.info("Testing Write file to eMMC...")
            self.writeresult.append(self.adb.emmc_write())

            self.logger.info("Testing read file from eMMC...")
            self.readresult.append(self.adb.emmc_read())

            running_times -= 1

    def __run_mtprwemmc_test_worker(self, tmp_file):
        self.logger.info('start to run mtp over eMMc testing...')

        print '*' * 50
        self.logger.info('Testing File Size: {0}MB'.format(self.tmp_file_size))
        print '*' * 50

        self.__run_mtprwemmc_testing(tmp_file, self.running_times)

        self.__generate_report(self.work_dir)

    def __run_mtprwemmc_testing(self, tmp_file, running_times):
        self.adb.reboot_device(self.device_id)

        while self.running_flag and running_times > 0:
            self.logger.info("Testing Write file to internal storage...")
            self.writeresult.append(self.adb.push_file(tmp_file, "/data/local/tmp"))
            self.logger.info("Testing read file from internal storage...")
            tmp_read_file_name = "{}.txt".format(dvcutility.Enviroment.get_cur_time_str())
            tmp_read_file_path = os.path.join(self.work_dir, tmp_read_file_name)
            self.readresult.append(self.adb.pull_file("/data/local/tmp/{}".format(os.path.basename(tmp_file)),
                                                      tmp_read_file_path))
            if os.path.exists(tmp_read_file_path):
                self.files_to_del.append(tmp_read_file_path)

            running_times -= 1

        map(dvcutility.Enviroment.remove_item, self.files_to_del)

    def __generate_report(self, dst):
        if not self.running_flag:
            return

        print '*' * 50
        self.logger.info('Generating report on {0}...'.format(dst))

        result = []
        all_sheets = []
        result.append(self.test_type + 'Summary')
        #
        device_cpu_type = self.adb.get_prop_value('ro.product.cpu.abi')
        device_build = self.adb.get_prop_value('ro.build.id')
        device_model = self.adb.get_prop_value('ro.product.model')
        device_release = self.adb.get_prop_value('ro.build.version.release')
        houdini_version = self.adb.get_houdini_version()
        #
        result.append(['Device Model', 'Android Version', 'Device CPU', 'Device Build', 'Houdini Version'])
        #
        result.append([device_model, device_release, device_cpu_type, device_build, houdini_version])
        #
        if self.test_type != "emmcdd":
            result_head = ['Test File Size']
        else:
            result_head = []

        for i in range(self.running_times):
            result_head.append("Round {0} (Write)".format(i + 1))

        result_head.append('Average Result(Write)')
        result_head.append('Standard Deviation(Write)')

        for i in range(self.running_times):
            result_head.append("Round {0} (Read)".format(i + 1))
        result_head.append('Average Result(Read)')
        result_head.append('Standard Deviation(Read)')

        result.append(result_head)
        if self.test_type != "emmcdd":
            data_result = [str(self.tmp_file_size) + 'MB']
        else:
            data_result = []

        write_result = [i.split(' (')[0] for i in self.writeresult]
        write_result_without_unit = [int(i.split(' (')[0].split(' ')[0]) for i in self.writeresult]
        write_unit = write_result[0].split(' ')[1]
        data_result.extend(write_result)
        data_result.append(dvcutility.Enviroment.get_average_value(write_result_without_unit) + ' ' + write_unit)
        data_result.append(str(dvcutility.Enviroment.get_std_deviation(write_result_without_unit)) + ' ' + write_unit)

        read_result = [i.split(' (')[0] for i in self.readresult]
        read_result_without_unit = [int(i.split(' (')[0].split(' ')[0]) for i in self.readresult]
        read_unit = read_result[0].split(' ')[1]
        data_result.extend(read_result)
        data_result.append(dvcutility.Enviroment.get_average_value(read_result_without_unit) + ' ' + read_unit)
        data_result.append(str(dvcutility.Enviroment.get_std_deviation(read_result_without_unit)) + ' ' + read_unit)

        result.append(data_result)
        all_sheets.append(result)
        result_excel = dvcutility.ExcelWriter(os.path.join(self.work_dir, self.test_type + '_Report.xlsx'))
        result_excel.generate_excel(all_sheets)
        print '*' * 50


class StorageTestGenerator(base.TestFactory):
    def __init__(self, *args, **kwargs):
        super(StorageTestGenerator, self).__init__(*args, **kwargs)
        self.args = args
        self.kwargs = kwargs

    def get_tester(self):
        return StorageTest(*self.args, **self.kwargs)
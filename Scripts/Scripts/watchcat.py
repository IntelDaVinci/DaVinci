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

import logging
import time
import sys
import os
import subprocess
import threading
import signal
import getopt
import random
import string
import traceback
from adbhelper import AdbHelper
from threading import Semaphore

MODULE_NAME = "WatchCat." + ''.join(random.choice(string.ascii_uppercase) for _ in range(6))
module_logger = logging.getLogger(MODULE_NAME)
module_logger.setLevel(logging.DEBUG)
# stream_handler = logging.StreamHandler(sys.stdout)
# stream_handler.setLevel(logging.NOTSET)
file_handler = logging.FileHandler('watchcat.log')
file_handler.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
file_handler.setFormatter(formatter)
# stream_handler.setFormatter(formatter)
# module_logger.addHandler(stream_handler)
module_logger.addHandler(file_handler)


class WatchCatBreakException(BaseException):
    pass


class WatchCatManager:
    logger = None

    def __init__(self, watchcat_path, adb_path):
        # {device_name:watchcat_thread_instance, ...}
        self.device_list = {}
        self.watchcat_path = watchcat_path

        self.adb_path = adb_path
        self.__prepare()
        WatchCatManager.logger.info("WatchCatManager started.")

    def __prepare(self):
        self.__set_logger()
        pass

    def watch_device(self, device):
        cat_instance = WatchCat(device, self.watchcat_path, self.adb_path)
        cat_instance.start()
        WatchCatManager.logger.info("Watch device " + device)
        self.device_list[device] = cat_instance

    def unwatch_device(self, device):
        cat_instance = self.device_list[device]
        cat_instance.stop()
        WatchCatManager.logger.info("Unwatch device " + device)
        del self.device_list[device]

    def get_watchcat_instance(self, device):
        cat_instance = self.device_list[device]
        return cat_instance

    def list_watching_devices(self):
        self.logger.info(self.device_list.keys())
        return self.device_list.keys()

    def __set_logger(self):
        WatchCatManager.logger = logging.getLogger(MODULE_NAME+".Manager")


class WatchCat(threading.Thread):

    popen_sem = Semaphore(1)

    def __init__(self, device, watchcat_path, adb_path):
        self.device = device
        self.watchcat_path_arm = watchcat_path["arm"]
        self.watchcat_path_x86 = watchcat_path["x86"]
        self.adb_path = adb_path
        self.pid = "0"
        self.daemon_process = None
        self.start_point_log_filename = None
        self.end_point_log_filename = None
        self.is_alive = True
        super(WatchCat, self).__init__(name="WatchCat")

    def __prepare(self):
        self.__set_logger()
        self.adb = AdbHelper(self.adb_path)
        self.adb.device_id = self.device
        self.start_point_log_filename = self.__get_reboot_history_file()

    def __set_logger(self):
        # self.logger = WatchCatManager.logger
        self.logger = logging.getLogger(MODULE_NAME+".instance-"+self.device)

    def run(self):
        self.__prepare()

        if self.__check_installation(force_install=True) is True:
            while self.is_alive:
                try:
                    if self.pid is not "0":
                        self.logger.info("++kill watchcat pid: %s  in %s " % (self.pid, self.device))
                        self.__kill_watchcat_daemon(self.pid)
                        self.pid = "0"
                        self.logger.info("--kill watchcat pid")
                    self.__check_installation()
                    self.logger.info("Exec watchcat daemon")
                    self.daemon_process = self.__start_watchcat_daemon()
                    self.logger.info("###Start sending watchcat alive signal.###")
                    while self.daemon_process.poll() is None and self.is_alive:
                        # self.logger.info("Sending watchcat alive signal.")
                        ret = self.__send_alive_signal()
                        try:
                            time.sleep(3)
                        except KeyboardInterrupt as ki:
                            self.logger("Received KeyboardInterrupt:", ki)
                            break
                        except:
                            self.logger("Received UnknownInterrupt:")
                            break
                    else:
                        self.logger.info("###Stop sending watchcat alive signal.###")
                        res = self.daemon_process.stdout.readlines()
                        self.logger.info("Watchcat output: %s" % res)
                        if len(res):
                            reslist = res[0].split(':')
                            if len(reslist) >= 2:
                                self.pid = reslist[1]
                                self.pid = self.pid.strip()
                                self.logger.info(self.pid)
                        else:
                            self.logger.error("Watchcat process outputs nothing, seems died already.")
                except Exception as e:
                    self.logger.error("Exception in the WatchCat Main Loop! :")
                    exctype, value, tb = sys.exc_info()
                    self.logger.error('Type:' + str(exctype))
                    self.logger.error('Value:')
                    self.logger.error(value)
                    self.logger.error("Traceback:")
                    self.logger.error("".join(traceback.format_list(traceback.extract_tb(tb))))
                    self.logger.error(e)
                    self.logger.error("Watchcat instance break.")
                    break
        self.logger.info("watchcat process ended")

    def stop(self):
        try:
            self.is_alive = False
            self.end_point_log_filename = self.__get_reboot_history_file()
            self.daemon_process.terminate()
            res = self.daemon_process.stdout.readlines()
            self.logger.info("Watchcat output: %s" % res)
            if len(res) == 0:
                self.logger.info("Watchcat process seems died already. Watchcat script ends.")
                self.logger.info("Please check the reboot history(in device GMT time) watchcat issued in /data/local/tmp/"
                                 "timestamp_watchbog.")
                raise WatchCatBreakException("Watchcat break, the instance process seems died already.")
            reslist = res[0].split(':')
            if len(reslist) >= 2:
                self.pid = reslist[1]
                self.pid = self.pid.strip()
                self.__kill_watchcat_daemon(self.pid)
                time.sleep(3)
            self.logger.info("===================watchcat reboot log=================")
            self.logger.info(self.__make_result())
            self.logger.info("=======================================================")
        except WatchCatBreakException as be:
            self.logger.warning(be)
        except Exception as e:
            self.logger.error("Exception in the WatchCat stop()! :")
            exctype, value, tb = sys.exc_info()
            self.logger.error('Type:' + str(exctype))
            self.logger.error('Value:')
            self.logger.error(value)
            self.logger.error("Traceback:")
            self.logger.error("".join(traceback.format_list(traceback.extract_tb(tb))))
            self.logger.error("Watchcat instance stopped unexpectedly.")

    def __kill_watchcat_daemon(self, pid):
        kill_command = "wait-for-device shell kill -3 %s" % pid
        self.logger.info("execute: %s" % kill_command)
        out = self.__adb_execution(kill_command, timeout=sys.maxint)
        self.logger.info("result: %s" % out)
        self.logger.info('kill_device_process:watchcat ended')
        return out

    def __start_watchcat_daemon(self):
        self.popen_sem.acquire()
        start_command = "%s -s %s shell /data/local/tmp/watchcat -d" % (self.adb_path, self.device)
        bog_process = subprocess.Popen(start_command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        self.popen_sem.release()
        return bog_process

    def __send_alive_signal(self):
        life_command = "shell rm /data/local/tmp/wd_sign"
        out = self.__adb_execution(life_command, timeout=480)
        return out

    def __send_stop_signal(self):
        stop_command = "shell touch /data/local/tmp/wd_stop"
        out = self.__adb_execution(stop_command, timeout=480)
        return out

    def __check_installation(self, force_install=False):
        if self.__check_watchdog_existence() and force_install is False:
            self.logger.info("watchcat exists")
            return True
        else:
            self.logger.info("Start installing watchcat...")
            if self.__install_watchcat():
                self.logger.info("watchcat installed successfully")
                return True
            else:
                self.logger.info("watchcat installed unsuccessfully")
                return False

    def __install_watchcat(self):
        if self.__is_device_x86():
            daemon_path = self.watchcat_path_x86
        else:
            daemon_path = self.watchcat_path_arm
        cmd_push_daemon = "push %s /data/local/tmp/watchcat" % daemon_path
        self.logger.info(cmd_push_daemon)
        self.__adb_execution(cmd_push_daemon, timeout=960)
        cmd_chmod_daemon = "shell chmod 777 /data/local/tmp/watchcat"
        self.logger.info(cmd_chmod_daemon)
        self.__adb_execution(cmd_chmod_daemon, timeout=960)
        self.logger.info("Installation ends,recheck..")
        if self.__check_watchdog_existence():
            return True
        return False

    def __is_device_x86(self):
        cmd_check_x86 = "shell getprop ro.product.cpu.abi"
        result = self.__adb_execution(cmd_check_x86, timeout=960)
        if result.find("86") != -1:
            return True
        return False

    def __check_watchdog_existence(self):
        cmd_check_watchdog = "shell ls -l /data/local/tmp/watchcat"
        result = self.__adb_execution(cmd_check_watchdog, timeout=960)
        self.logger.info(result)
        if result.find("rwxrwxrwx") != -1:
            return True
        return False

    def __get_reboot_history_file(self):
        cmd_check_existence = "shell \"if [ -f /data/local/tmp/timestamp_watchbog ]; then echo exist_ok ;fi\""
        result = self.__adb_execution(cmd_check_existence, timeout=960)
        if result.find("exist_ok") != -1:
            log_name = ".\\result\\watchcat_rebootlog_%s_%s.log" % (self.device, self.__get_time())
            process_cat_history = "pull /data/local/tmp/timestamp_watchbog " + log_name
            result = self.__adb_execution(process_cat_history, timeout=960)
            return log_name
        return None

    def __get_time(self):
        return time.strftime("%Y%m%d%H%M%S", time.localtime())

    def __make_result(self):
        if self.start_point_log_filename is not None:
            with open(self.start_point_log_filename, 'rU') as pre_file:
                start_point_history_length = len(pre_file.readlines())
        else:
            start_point_history_length = 0

        if self.end_point_log_filename is not None:
            with open(self.end_point_log_filename, 'rU') as history_file:
                history_list = history_file.readlines()
                result = "".join(history_list[start_point_history_length:])
                return result
        else:
            return "No history reboot files in the device"

    def __adb_execution(self, *args, **kwargs):
        ret_list = self.adb.adb_execution(*args, **kwargs)
        self.logger.debug(args)
        self.logger.debug(ret_list)
        if ret_list:
            return ret_list[0]
        else:
            self.logger.error("Exception in the Adb helper")
            # self.is_alive = False
            return ""


if __name__ == '__main__':
    def usage():
        print("Usage:%s [-a|--adb=] <adb.exe path> [-s|--serial=] <device serial>"%sys.argv[0])

    try:
        opts, args = getopt.getopt(sys.argv[1:], "s:a:", ["adb=", "serial="])
        device = None
        adb_path = None
        for opt, arg in opts:
            if opt in ("-s", "--serial"):
                print("cmd parser:Serial:%s"%(arg))
                device = arg
            elif opt in ("-a", "--adb"):
                print("cmd parser:adb_path:%s"%(arg))
                adb_path = arg
            else:
                print("No parameter!")
    except getopt.GetoptError:
        print("getopt error!")
        usage()
        sys.exit(0)

    # Set streamhandler for command-line use
    sh = logging.StreamHandler(sys.stdout)
    sh.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    sh.setFormatter(formatter)
    module_logger.addHandler(sh)

    wcm = WatchCatManager({"arm": "watchcat_arm", "x86": "watchcat_x86"}, adb_path)
    wcm.watch_device(device)

    def signal_handler(*args):
        print "Received signal"
        wcm.unwatch_device(device)
        sys.exit(0)
    if hasattr(os.sys, 'winver'):
        signal.signal(signal.SIGBREAK, signal_handler)
    else:
        signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    while True:
        time.sleep(1)

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
# encoding:utf-8
import os
import sys
import time
import subprocess
import dvcsubprocess


# -----------------------------------------------------
# ADB class which contains operation related with adb
# -----------------------------------------------------
class AdbHelper:
    """docstring for AdbHelper"""

    def __init__(self, adb_path, timeout=120):
        self.adb_path = adb_path.replace('/', '\\')
        self.subprocess = dvcsubprocess.DVCSubprocess()
        self.timeout = timeout
        self.device_id = ''

    @property
    def device_id(self):
        return self.device_id

    @property
    def timeout(self):
        return self.timeout

    @device_id.setter
    def device_id(self, target_device_id):
        self.device_id = target_device_id

    @timeout.setter
    def timeout(self, user_define_timeout):
        self.timeout = user_define_timeout

    def adb_check(func):
        def wrapped(self, *args, **kwargs):
            result, error = self.subprocess.Popen(self.adb_path + ' devices', self.timeout)
            if error or result and result[0].find("didn't ACK") != -1 and result[0].find("out of date") != -1:
                self.restart_adb()
            return func(self, *args, **kwargs)

        return wrapped

    def wireless_dev_connection_check(func):
        def wrapped(self, *args, **kwargs):
            if ':' in self.device_id:
                self.subprocess.Popen("{} connect {}".format(self.adb_path, self.device_id), self.timeout)
            return func(self, *args, **kwargs)

        return wrapped

    @wireless_dev_connection_check
    @adb_check
    def adb_execution(self, cmd_para, timeout, shell_flag=False, use_device_id=True, call_out=subprocess.PIPE,
                      call_err=subprocess.PIPE):
        try:
            if use_device_id and self.device_id.strip():
                cmd = self.adb_path + ' -s ' + self.device_id + ' ' + cmd_para
            else:
                cmd = self.adb_path + ' ' + cmd_para
            output, err = self.subprocess.Popen(cmd, timeout, shell_flag, call_out, call_err)
        except Exception, e:
            return []
        else:
            keywords = ('* daemon not running. starting it now on port 5037 *',
                        '* daemon started successfully *',
                        'adb server is out of date.  killing...')
            for i in keywords:
                output = output.replace(i, '').lstrip()

            return [output, err]
        
    def GetPackageVer(self, device_name, package_name):
        output = self.adb_execution(r'shell dumpsys package %s' % package_name, 120)[0]
        lines = output.strip().split('\r')
        version = ''
        for line in lines:
            if line.strip().startswith('versionName='):
                version = line.strip().split('=')[1]
                break
        return version
    
    def emmc_write(self):
        out, _ = self.adb_execution('shell time dd if=/dev/zero of=/mnt/sdcard/test.dbf bs=4k count=1000000',
                                    self.timeout)
        return out.strip().split('\r\r\n')[2].split('(')[1][:-1]

    def emmc_read(self):
        out, _ = self.adb_execution('shell time dd if=/mnt/sdcard/test.dbf of=/dev/null bs=4k', self.timeout)
        return out.strip().split('\r\r\n')[2].split('(')[1][:-1]

    def check_path_existence(self, path):
        out, _ = self.adb_execution("shell ls {0}".format(path), self.timeout, shell_flag=False)
        return False if out.find("No such file or directory") != -1 else True

    def install_apk(self, pkg_name):
        self.adb_execution('install "' + pkg_name + '"', self.timeout)

    def uninstall(self, pkg_name):
        self.adb_execution('uninstall ' + pkg_name, self.timeout)

    def get_connected_devices(self):
        try:
            devices_content = self.adb_execution('devices', self.timeout, shell_flag=False, use_device_id=False)[0]
            device_list = [i.split('\tdevice')[0] for i in devices_content.split('\r\n')
                           if i.strip() and i.endswith('\tdevice')]
        except Exception, e:
            return []
        else:
            return device_list

    def get_device_resolution(self):
        try:
            content = self.adb_execution('shell dumpsys window displays|findstr init', self.timeout, shell_flag=True)[0]

            result = content.strip().split()[0].split('=')[1].split('x')
        except Exception, e:
            return str(480), str(800)
        else:
            return result[0], result[1]

    def unlock_device(self):
        unlock_coordinate_x, unlock_coordinate_y = self.get_device_resolution()

        unlock_coordinate = (str(int(unlock_coordinate_x) / 10), str(int(unlock_coordinate_y) * 4 / 5),
                             str(int(unlock_coordinate_x) * 4 / 5), str(int(unlock_coordinate_y) * 4 / 5))

        self.adb_execution('shell input touchscreen swipe ' + ' '.join(unlock_coordinate), self.timeout,
                           shell_flag=True)
        time.sleep(2)
        # try to unlock again
        self.adb_execution('shell input touchscreen swipe ' + ' '.join(unlock_coordinate), self.timeout,
                           shell_flag=True)

    def get_all_apk_path(self, out_result):
        output = self.adb_execution('shell pm list packages -f', self.timeout)[0].split(os.linesep)

        apk_info = (i.strip() for i in output if i.strip() != '')
        for i in apk_info:
            out_result[i.split('=')[1]] = i.split(':')[1].split('=')[0]

    def get_apk_path(self, pkg_name):
        output = self.adb_execution('shell pm list packages -f | findstr {0}'.format(pkg_name), self.timeout,
                                    shell_flag=True)[0].split(os.linesep)
        if output:
            return output[0].split(':')[1].split('=')[0]
        else:
            return ''

    def check_battery(self, threshold, sleep_time):
        while True:
            print 'Checking device battery...'
            dev_battery = self.get_device_battery_info()
            print "Battery is {0}...".format(dev_battery)
            if dev_battery and dev_battery.isdigit() and int(dev_battery) < threshold:
                print "Charging device..."
                time.sleep(sleep_time)
            else:
                break

    def pull_file(self, src_path, dest_path):
        return self.adb_execution("pull {} {}".format(src_path, dest_path), self.timeout)[1].strip()

    def push_file(self, src_path, dest_path):
        return self.adb_execution("push {} {}".format(src_path, dest_path), self.timeout)[1].strip()

    def get_device_battery_info(self):
        output = self.adb_execution('shell dumpsys battery|findstr level', self.timeout, shell_flag=True)[0].strip()
        return output.split(':')[1].strip() if output and output.find('level') != -1 else 'N/A'

    def reboot_device(self, timeout=180):
        print "It's time to reboot %s" % self.device_id
        self.adb_execution('reboot', self.timeout)

        time.sleep(2)
        t_beginning = time.time()
        h_beginning = time.time()
        seconds_passed = 0
        heartbeat = 3
        is_alive = False

        while seconds_passed < timeout:
            time.sleep(2)
            heartbeat_passed = time.time() - h_beginning
            if heartbeat_passed > heartbeat:
                content = self.adb_execution('shell getprop sys.boot_completed', self.timeout, shell_flag=True)[0]
                print 'Checking device {0} status...'.format(self.device_id)
                if content.find('1') >= 0:
                    is_alive = True
                    break

                h_beginning = time.time()

            seconds_passed = time.time() - t_beginning

        time.sleep(5)

        self.unlock_device()

        return is_alive

    def force_stop_by_package(self, pkg_name):
        self.adb_execution("shell am force-stop {0}".format(pkg_name), self.timeout)

    def get_prop_value(self, prop_field):
        return self.adb_execution('shell getprop ' + prop_field, self.timeout)[0].strip()

    def get_houdini_version(self):
        result = self.adb_execution('shell houdini --version', self.timeout)[0]
        # result: Houdini version: 5.0.1a_x.46587
        return result.strip().split(':')[1].strip() if result.count(':') is 1 else 'NA'

    def clear_data_by_package(self, pkg_name):
        result = self.adb_execution('shell pm clear {0}'.format(pkg_name), self.timeout)[0].strip()
        return True if result == 'Success' else False

    def uninstall_3rd_app(self):
        result = self.adb_execution("shell pm list packages -3", self.timeout)[0].split(os.linesep)
        dev_3rd_app_list = [i.strip().split('package:')[1] for i in result if i.find('package:') != -1]
        if "com.example.qagent" in dev_3rd_app_list:
            dev_3rd_app_list.remove("com.example.qagent")
        map(self.uninstall, dev_3rd_app_list)

    def get_installed_apk_list(self):
        apk_info = self.adb_execution('shell pm list packages -f', self.timeout)[0].split(os.linesep)
        return [i.strip().split('=')[1] for i in apk_info if i.strip() != '']

    def check_package_installed(self, pkg_name):
        result = self.adb_execution('shell pm list packages ' + pkg_name, self.timeout)[0]
        return True if result.find('package:') > -1 else False

    def terminate_adb(self, useless):
        result = self.subprocess.Popen('taskkill /f /im adb.exe')[0]
        return False if result.find('could not be terminated') > -1 else True

    def get_adb_process_num(self):
        tasks = self.subprocess.Popen('TASKLIST')[0]
        return [i for i in tasks.split('\r\n') if i.startswith('adb')]

    def start_adb_server(self):
        result = self.subprocess.Popen(self.adb_path + ' start-server')
        return True if not result[0] or result[0] and result[0].find(' daemon started successfully ') > -1 else False

    def restart_adb(self):
        count = 20

        # Kill all adb process instead of adb kill-server.
        # Since sometime adb process may hung there which will cause kill-server hung.
        adb_tasks = self.get_adb_process_num()
        while count > 0 and adb_tasks:
            count -= 1
            result = map(self.terminate_adb, adb_tasks)
            adb_tasks = self.get_adb_process_num()
            if not any(result):
                break
                # time.sleep(1)
        # Kill the port 5037 process
        self.kill_port()

        # Still need to start the adb server after kill adb
        time.sleep(3)
        if not self.start_adb_server():
            # CleanupToQuit()
            sys.exit(-1)

    def launch_apk(self, pkg_name, activity_name):
        self.adb_execution("shell am start {0}/{1} ".format(pkg_name, activity_name), self.timeout)

    def get_state(self):
        result = self.adb_execution("get-state", self.timeout)
        return result[0].strip() if result else ''

    def get_adb_port_info(self):
        content = self.subprocess.Popen("netstat -aon|findstr 5037", self.timeout, shell_flag=True)[0]
        return content.strip().split('\r\n') if content.strip() else []

    def get_specified_port_info(self, port):
        content = \
            self.subprocess.Popen("tasklist /FI \"PID eq " + port + "\" /FO \"LIST\"", self.timeout, shell_flag=True)[0]
        return content.strip().split('\r\n')

    def kill_process(self, process_name):
        content = self.subprocess.Popen("taskkill /F /IM " + process_name, self.timeout, shell_flag=True)[0]
        return content

    def kill_port(self):
        count = 2

        while True:
            try:
                output = self.get_adb_port_info()
                # DebugLog("Command: netstat -aon | findstr \"5037\". \nResult: %s" % "".join(output))
                if not output:
                    break

                process_list = []
                target = [i for i in output if
                          i.find('LISTENING') != -1 and i.find(':5037') != -1 and int(i.split(' ')[-1])]

                for line in target:
                    info = [i for i in line.split(' ') if i != '']
                    # Proto  Local Address          Foreign Address        State           PID
                    # info[-1] stands for PID
                    output = self.get_specified_port_info(info[-1])
                    if output and output[0].find(":") != -1 and output[0].split(':')[1].strip() != 'adb.exe':
                        process_list.append(output[0].split(':')[1].strip())

                if not process_list:
                    break

                if not count:
                    print "  - Failed to kill port 5037 process"
                    return -1

                map(self.kill_process, process_list)

                count -= 1
            except Exception, e:
                print str(e)
        return 0

    def get_top_activity(self):
        return self.adb_execution('shell dumpsys activity top|findstr "ACTIVITY"', self.timeout,
                                  shell_flag=True)[0].strip()

    def logcat_clear(self):
        self.adb_execution("logcat -c", self.timeout)

    def logcat_to_file(self, file_path):
        self.adb_execution("logcat -d >{0}".format(file_path), self.timeout, shell_flag=True)

    def logcat_search(self, keyword):
        out = self.adb_execution('logcat', self.timeout)[0]
        return True if out.find(keyword) != -1 else False

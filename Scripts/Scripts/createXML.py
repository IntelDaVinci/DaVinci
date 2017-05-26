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
import os
import sys
import re
from optparse import OptionParser
from xml.dom.minidom import Document
import dvcutility
import re
import logging
import adbhelper

proplist = ["ro.build.version.release", "ro.product.model", "ro.build.id", "ro.product.cpu.abi"]
headerwidth = [5, 15, 8, 15, 12, 5]
header = ["No.", "Device name", "OS Ver.", "Device Model", "Device Build", "CPU"]
tab_length = 4
device_id_selected = []


class TreadmillConfig:
    def __init__(self, filename):
        self.doc = Document()
        self.settings = self.doc.createElement('SmartRunner')
        self.doc.appendChild(self.settings)
        self.filename = filename

    def add(self, text, element, child=''):
        node = self.doc.createElement(element)
        self.settings.appendChild(node)
        if child == '':
            value = self.doc.createTextNode(text)
            node.appendChild(value)
        else:
            subnode = self.doc.createElement(child)
            node.appendChild(subnode)
            value = self.doc.createTextNode(text)
            subnode.appendChild(value)

    def save(self):
        with open(self.filename, 'w') as f:
            f.write(self.doc.toprettyxml(indent=''))


class LaunchTimeMode(object):
    apk_downloaded_mode = '1'
    ondevice_download_mode = '2'
    logcat_mode = '3'


def AnyDevice():

    adb_obj = adbhelper.AdbHelper(r'%s\platform-tools\adb.exe' % Davinci_folder)
    output = adb_obj.adb_execution("devices", 120)
    output = output[0].split('\r\n')
    header_found = False
    any_device = False

    index = 0
    devices = []
    for line in output:
        index += 1
        if line.find("List of devices attached") >= 0:
            header_found = True
            continue
        if header_found:
            if line.strip():
                devices.append(line)
                any_device = True

    if not devices:
        print "No available devices."

    return devices


def ShowDevTable(dev, title):
    # Print header
    split_line = '-' * 80
    print "\n\n" + title + ":"
    print split_line
    index = 0
    header_col = []
    group_dict = {}
    for col in header:
        while len(col) + tab_length < headerwidth[index]:
            col = col + " "
        index += 1
        header_col.append(col)
    print '\t'.join(header_col)
    print split_line

    # Get proper for all the devices
    for device_name in dev:
        index = 1
        row = device_name
        while len(row) + tab_length < headerwidth[index]:
            row = row + " "
        for prop_name in proplist:
            index += 1
            device_prop = os.popen(Davinci_folder + "\\platform-tools\\adb.exe -s %s shell getprop %s"
                                   % (device_name, prop_name)).readlines()[0]
            device_prop = device_prop.strip('\n').strip('\r')
            while len(device_prop) + tab_length < headerwidth[index]:
                device_prop = device_prop + " "
            row += "\t" + device_prop
        group_dict[device_name] = row

    resorted_dev = []
    index = 0
    for obj in sorted(group_dict.iteritems(), key=lambda d: d[0], reverse=False):
        key = obj[0]
        line_info = obj[1]
        index += 1
        print str(index) + "\t" + line_info
        resorted_dev.append(key)
    return resorted_dev


def TestTypeSelection():
    # Don't modify below three lines!
    print '*' * 40
    print 'Welcome to Smart Runner configuration'
    print '*' * 40
    test_type = raw_input("\nPlease select the testing type:\n\t1:Basic Test(Smoke)"
                          " \n\t2:Performance Test\n\t3:Power Test\n"
                          "The default type is Basic Test:")
    if not test_type:
        test_type = '1'

    while test_type:
        try:
            if 1 <= int(test_type) <= 3:
                break
        except:
            pass
        test_type = raw_input("[Error] Invalid Number. Please input again:")

    if test_type == "2":
        performance_test_menu = ("1:Launch Time", "2:Apk Installation", "3:Storage")
        performance_test_type = ("LaunchTime", "ApkInstallation", "Storage")
        prompt = "\nPlease select the performance testing type:\n\t{}\n" \
                 "The default type is Launch Time:".format('\n\t'.join(performance_test_menu))

        sub_test_type = raw_input(prompt)
        if not sub_test_type:
            test_type = "LaunchTime"
        else:
            while sub_test_type:
                try:
                    if 1 <= int(sub_test_type) <= len(performance_test_menu):
                        break
                except:
                    pass
                sub_test_type = raw_input("[Error] Invalid Number. Please input again:")

            test_type = performance_test_type[int(sub_test_type) - 1]
    elif test_type == '3':
        test_type = "Power"
    else:
        test_type = "Smoke"

    Treadmill.add(test_type, "TestType")

    return test_type


def power_option():
    # qs folder
    power_test_qs_folder = raw_input("\n\nPlease enter the full path of Power test qs folder:\n")
    while power_test_qs_folder and not os.path.exists(power_test_qs_folder):
        power_test_qs_folder = raw_input("[Error] Invalid folder path. Please input again:")

    Treadmill.add(power_test_qs_folder, "power_qs_path")
    # running times
    each_power_test_times = raw_input("\nPlease enter number of cycle for running each power testing case."
                                      "The default value is 5:\n")

    if not each_power_test_times:
        each_power_test_times = '5'

    while each_power_test_times:
        try:
            if each_power_test_times.isdigit() and int(each_power_test_times) > 0:
                break
        except:
            pass
        each_power_test_times = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(each_power_test_times, "power_running_times")


def launchtime_camera_mode(lt_mode):
    camera_list = dvcutility.Enviroment.get_connected_camera_list()

    if camera_list and lt_mode != LaunchTimeMode.logcat_mode:
        selected_camera_mode = raw_input('\nPlease select camera mode for testing.'
                                         '\n\t1.HypersoftCamera\n\t2.Camera\nThe default mode is HypersoftCamera:\n')
        if not selected_camera_mode:
            selected_camera_mode = '1'
        while selected_camera_mode:
            try:
                if 1 <= int(selected_camera_mode) <= 2:
                    break
            except:
                pass

            selected_camera_mode = raw_input("[Error] Invalid Number. Please input again:")

        camera_mode = "camera" if selected_camera_mode == '2' else "hsc"

        Treadmill.add(camera_mode, "camera_mode")

        if selected_camera_mode == '2':
            selected_camera_index = '1'

            if len(camera_list) > 1:
                print 'Please select a camera for further calibration'
                for i, j in enumerate(camera_list):
                    print '\t{} {}'.format(str(i + 1), j)

                selected_camera_index = raw_input(':')
                if not selected_camera_index:
                    selected_camera_index = '1'
                while selected_camera_index:
                    try:
                        if 1 <= int(selected_camera_index) <= len(camera_list):
                            break
                    except:
                        pass

                    selected_camera_index = raw_input("[Error] Invalid Number. Please input again:")

            Treadmill.add(str(int(selected_camera_index) - 1), "camera_index")
    else:
        Treadmill.add("hsc", "camera_mode")


def launch_time_general_options(lt_mode):
    each_lt_test_times = raw_input("\nPlease enter number of cycle for running each launch time testing case."
                                   "The default value is 5:\n")


    if not each_lt_test_times:
        each_lt_test_times = '5'

    while each_lt_test_times:
        try:
            if each_lt_test_times.isdigit() and int(each_lt_test_times) > 0:
                break
        except:
            pass
        each_lt_test_times = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(each_lt_test_times, "lt_running_times")

    lt_test_interval = raw_input("\nPlease enter interval for running each launch time testing case."
                                 "The default value is 30 seconds:\n")

    if not lt_test_interval:
        lt_test_interval = '30'

    while lt_test_interval:
        try:
            if lt_test_interval.isdigit():
                break
        except:
            pass
        lt_test_interval = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(lt_test_interval, "lt_test_interval")

    if lt_mode != LaunchTimeMode.logcat_mode:
        pre_launch_apk_folder = raw_input("\nPlease enter the path of pre-install apks folder, press Enter to skip:\n")
        if pre_launch_apk_folder:
            while not os.path.exists(pre_launch_apk_folder):
                pre_launch_apk_folder = raw_input("[Error] Invalid folder path. Please input again:")

            Treadmill.add(pre_launch_apk_folder, "pre_launch_apk_folder")

    usb_replug_flag = raw_input("\nDo you want to enable usb switch for device recovery?"
                                "Press 'y' to run it or Enter to skip: ")

    if not usb_replug_flag:
        usb_replug_flag = 'n'

    while usb_replug_flag:
        try:
            if usb_replug_flag.isalpha():
                break
        except:
            pass
        usb_replug_flag = raw_input("[Error] Invalid Input. Please input again:")

    if usb_replug_flag.strip().lower() == 'y':
        Treadmill.add("enabled", "usb_replug_flag")
    else:
        Treadmill.add("disabled", "usb_replug_flag")


def launchtime_downloaded_apk_options(lt_mode):
    download_mode = raw_input("\n\n# Please select the APK downloading mode: [1, 2]?\n\t1: APK already downloaded\n"
                              "\t2: Download on device from Google Play\nThe default mode is [1]: ")


    while download_mode and download_mode != LaunchTimeMode.apk_downloaded_mode and download_mode != LaunchTimeMode.ondevice_download_mode:
        download_mode = raw_input("[Error] Invalid Number. Please input again:")

    if download_mode == LaunchTimeMode.ondevice_download_mode:
        CollectOnDeviceDownloadConfigItems(False)
        download_mode = "on-device"
    else:
        download_mode = "downloaded"

    Treadmill.add(download_mode, "APK_download_mode")

    lt_timeout = raw_input("\n\nPlease enter timeout value for generating launch time ini file."
                           "The default value is 60 seconds:\n")

    if not lt_timeout:
        lt_timeout = '60'

    while lt_timeout:
        try:
            if lt_timeout.isdigit():
                break
        except:
            pass
        lt_timeout = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(lt_timeout, "lt_timeout")

    fresh_run_flag = raw_input("\nWould you like to run launch time case under fresh run mode? Press 'y' or 'n'."
                               "The default option is 'n':\n")

    if not fresh_run_flag:
        fresh_run_flag = 'n'

    while fresh_run_flag:
        try:
            if fresh_run_flag.strip().isalpha():
                break
        except:
            pass
        fresh_run_flag = raw_input("[Error] Invalid Input. Please input again:")

    if fresh_run_flag.strip().lower() == 'y':
        Treadmill.add('enabled', "fresh_run_flag")
    else:
        Treadmill.add('disabled', "fresh_run_flag")

    if fresh_run_flag.strip().lower() == 'n':
        clear_cache_flag = raw_input("\nWould you like to clear app cache before each round launch time testing? "
                                     "Press 'y' or 'n'.The default option is 'n':\n")
        if not clear_cache_flag:
            clear_cache_flag = 'n'

        while clear_cache_flag:
            try:
                if clear_cache_flag.strip().isalpha():
                    break
            except:
                pass
            clear_cache_flag = raw_input("[Error] Invalid Input. Please input again:")

        if clear_cache_flag.strip().lower() == 'y':
            Treadmill.add('enabled', "clear_cache_flag")
        else:
            Treadmill.add('disabled', "clear_cache_flag")
    else:
        Treadmill.add('disabled', "clear_cache_flag")


def launchtime_option():

    launchtime_running_mode = raw_input("\n\nPlease select the Launch time running mode:"
                                        "\n\t1:Run testing with existed cases"
                                        "\n\t2:Run testing with cases generated by DVC"
                                        "\n\t3:Run testing by using Logcat\n"
                                        "The default mode is [1]:")
    if not launchtime_running_mode:
        launchtime_running_mode = LaunchTimeMode.apk_downloaded_mode

    while launchtime_running_mode:
        try:
            if 1 <= int(launchtime_running_mode) <= 3:
                break
        except:
            pass

        launchtime_running_mode = raw_input("[Error] Invalid Number. Please input again:")

    launchtime_camera_mode(launchtime_running_mode)

    if launchtime_running_mode == LaunchTimeMode.apk_downloaded_mode:
        launch_time_case_folder = raw_input("\n\nPlease enter the path of Launch time cases folder:\n")
        while launch_time_case_folder and not os.path.exists(launch_time_case_folder):
            launch_time_case_folder = raw_input("[Error] Invalid folder path. Please input again:")

        Treadmill.add(launch_time_case_folder, "existed_lt_case_path")
    elif launchtime_running_mode == LaunchTimeMode.logcat_mode:
        print 'Smart runner only supports android internal apps now!'
        logcat_list_path = raw_input("\n\nPlease enter the path of app list which will be tested by using logcat.\n"
                                     "Press Enter will test Browser/Settings/Dial apps:")
        if logcat_list_path:
            while logcat_list_path and not os.path.exists(logcat_list_path):
                logcat_list_path = raw_input("[Error] Invalid file path. Please input again:")
        else:
            apps_list = ["com.android.browser/.BrowserActivity\n", "com.android.settings/.Settings\n",
                         "com.android.dialer/.DialtactsActivity\n"]
            logcat_list_path = os.path.join(os.getcwd(), 'default_logcat_list.txt')
            with open(logcat_list_path, 'w') as f:
                f.writelines(apps_list)

        Treadmill.add(logcat_list_path, "logcat_list_path")
    else:
        launchtime_downloaded_apk_options(launchtime_running_mode)

    Treadmill.add(launchtime_running_mode, "launchtime_running_mode")

    launch_time_general_options(launchtime_running_mode)


def DeviceSelection():
    changed = False
    all_dev = []
    bad_dev = []
    adb_obj = adbhelper.AdbHelper(r'%s\platform-tools\adb.exe' % Davinci_folder)

    devices = AnyDevice()
    if not devices:
        sys.exit(-1)

    for line in devices:
        device_name = line.split('\t')[0]
        adb_obj.device_id = device_name
        status_info = adb_obj.adb_execution("get-state", 120)
        device_status = ("".join(status_info)).strip()
        if device_status.strip() != "device":
            changed = True
            bad_dev.append("%s\t%s" % (device_name, device_status))
            continue
        all_dev.append(device_name)

    if bad_dev:
        print "\nWARNING: Please make sure devices below are power on and connected correctly!!!"
        print "-" * 70
        print "Device Name       \tStatus:"
        print "-" * 70
        for bad_info in bad_dev:
            print bad_info

    if not all_dev:
        print("\n\nNo available device.")
        sys.exit(-1)

    print "\n\nReading information of attached devices. Please wait ......"
    all_dev = ShowDevTable(all_dev, "Device Group Information")
    config_items = []
    target_name = ''
    complete = False
    group_dict = {}
    while not complete:
        test_dev = raw_input("\n# Please choose one device to test. Input index:")
        try:
            int(test_dev)
        except:
            print "[Error]: Invalid number {0}".format(test_dev)
            continue
        if int(test_dev.strip()) < 1 or int(test_dev.strip()) > len(all_dev):
            print "[Error]: Invalid number {0}".format(test_dev)
        else:
            target_name = all_dev[int(test_dev) - 1]

            storage_obj = dvcutility.StorageInfo(r'%s\platform-tools\adb.exe' % Davinci_folder)
            free_ram_status = 'na'
            free_storage_status = 'na'
            free_storage_val_str, free_ram_val_str = storage_obj.CheckFreeStorage(target_name)
            if free_ram_val_str != '':
                free_ram_unit = re.search(r'(\d+) (\w+)', free_ram_val_str).group(2)
                free_ram_val = int(re.search(r'(\d+) (\w+)', free_ram_val_str).group(1))

                if (free_ram_unit == "kB"):
                    free_ram_val = float(free_ram_val/float(1000))
                if (free_ram_unit == "G"):
                    free_ram_val = float(free_ram_val * 1000)
                print 'Free Memory: {}M'.format(str(free_ram_val))

                if free_ram_val > 12:  # checking if free ram > 12M
                    free_ram_status = 'ok'
                else:
                    free_ram_status = 'low'
                    print "[Warning]: Selected target device has less than 12M free memory. There may random error during testing."
            else:
                print "[Warning]:Failed to get memory information."

            if free_storage_val_str != '':
                free_storage_val = float(re.search(r'([\d.]+)([\w])', free_storage_val_str).group(1))

                if (re.search(r'([\d.]+)([\w])', free_storage_val_str).group(2)) == 'M':
                    free_storage_val = float(free_storage_val / float(1000))
                print "Free Storage: {}G".format(str(free_storage_val))
                if free_storage_val > 0.512: # checking if free storage > 0.512G
                    free_storage_status = 'ok'
                else:
                    free_storage_status = 'low'
                    print "[Warning]: Selected target device has less than 512M free storage. There may random error during testing."
            else:
                print "[Warning]:Failed to get storage information."

            if (free_ram_status == 'low' or free_storage_status == 'low'):
                option = raw_input( "Press Enter to re-select. Press 'a' to accept this device: ")
                if option.lower() == '':
                    continue
            config_items.append(target_name)
            group_dict[1] = []
            group_dict[1].append("NA")  # Group name
            group_dict[1].append(target_name)  # Device name
            complete = True
            break

    Treadmill.add(target_name, "Target", "DeviceID")
    device_id_selected.append(target_name)




def TestNumberConfig():
    print "\n\n\n****************************** Begin to Config Test Number and Criteria ******************************"
    default_rerun_time = '1'
    rerun_times = raw_input("\n# If test has no result, retest %s time by default." % default_rerun_time +
                            " Input you preferred number. Or press Enter to continue:")

    while rerun_times:
        try:
            if int(rerun_times) > 0:
                break
        except Exception, e:
            pass
        rerun_times = raw_input("Input the rerun number as you prefer. Or press Enter to continue:")

    if not rerun_times:
        rerun_times = default_rerun_time

    default_test_times = '3'
    test_times = raw_input(
        "\n\n# Input the test number as you prefer. Or press Enter to continue with default value %s:" % default_test_times)
    while test_times:
        try:
            if int(test_times) > 1:
                break
        except Exception, e:
            pass
        test_times = raw_input(
            "Input the re-test number as you prefer.Input the test number as you prefer. Or press Enter to continue:")

    if not test_times:
        test_times = default_test_times

    test_times = str(int(test_times))
    default_pass_number = int(test_times) - 1
    if default_pass_number == 0:
        default_pass_number = 1

    pass_number = raw_input('\n\n# Pass criteria is %s Pass out of %s. Input the pass number as you prefer.'
                            'Or press Enter to continue:' % (str(default_pass_number), test_times))
    while pass_number:
        try:
            if 0 < int(pass_number) <= int(test_times):
                break
        except Exception, e:
            pass
        pass_number = raw_input("Input the test number as you prefer. Or press Enter to continue:")

    if not pass_number:
        pass_number = str(default_pass_number)

    print("\n\n# Several execution criteria below for each APK test: \n"
          + "   1. Most time-saving mode: If first round test is PASS, the test is marked as PASS and go to next APK.\n"
          + "   2. Most criteria-cared mode: Test at most %s rounds until test criteria is met.\n" % test_times
          + "   3. Most stress mode: Always test %s rounds whatever test result is. " % test_times)
    execution_criteria = ''
    while True:
        execution_criteria = raw_input("Please input your selection (by default option 1 is selected): ")
        if execution_criteria in ['1', '2', '3', '']:
            break
        else:
            print ("Invalid input.")
    if execution_criteria == '':
        execution_criteria = '1'
    Treadmill.add(test_times, "TestNumber")
    Treadmill.add(pass_number, "PassCriteria")
    Treadmill.add(execution_criteria, "ExecutionCriteria")
    Treadmill.add(rerun_times, "RerunValue")

    print '****************************** Config Test Number and Criteria is DONE.******************************\n'

def GeneralExecutionSettings():
    print "\n\n\n****************************** Begin to Config General Exectuion Settings ******************************"
    logging.basicConfig(filename='wifi.log', level=logging.INFO)
    logger = logging.getLogger('wifi')
    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))

    print("\nSearching for Connected WiFi HotSpot, Please wait.......... ")
    Wifi_obj = dvcutility.InstallWifiApk(r'%s\platform-tools\adb.exe' % Davinci_folder, script_path, logger)
    Wifi_obj.InstallWiFiChecker(device_id_selected)
    network_obj = dvcutility.DeviceNetwork(r'%s\platform-tools\adb.exe' % Davinci_folder, device_id_selected[0], logger)
    wifi_state, net_ssid, wifi_state_enabled, net_state_connected = network_obj.CheckNetworkState()

    while net_ssid == "not found":
        raw_input("\nWiFi is not enabled, please enable WiFi and press Enter......")
        wifi_state, net_ssid, wifi_state_enabled, net_state_connected = network_obj.CheckNetworkState()

    prefer_hotspot = raw_input(
        "\n\n# By default, Connecting to the 'connected' WiFI hotspot: " + net_ssid + "\nPress 'Enter' to continue")
    if prefer_hotspot == "":
        prefer_hotspot = net_ssid

    Treadmill.add(prefer_hotspot, 'PreferredHotSpot')

    default_reboot_time = '1'
    reboot_times = raw_input("\n\n# Periodic reboot time is set to %s hour by default."
                             " Input the reboot time in hours as you prefer. Or press Enter to continue:" % default_reboot_time)

    while reboot_times:
        try:
            if int(reboot_times) > 0:
                break
        except Exception, e:
            pass
        reboot_times = raw_input("Input the reboot time in hours as you prefer. Or press Enter to continue:")

    if not reboot_times:
        reboot_times = default_reboot_time

    Treadmill.add(reboot_times, "RebootValue")

    print '****************************** Config General Exectuion Settings is DONE ******************************\n'

def GeneralExecutionControlConfig():
    print "\n\n\n****************************** Begin for General Exectuion Control Config ******************************"
    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
    print("\n# When test failed, some debug script can be called to get more information.")
    debug_script_name = raw_input("Please input the relative path of the script. Or press Enter to skip: ")
    if debug_script_name:
        abs_debug_script = r'%s\%s' % (script_path, debug_script_name)
        while not os.path.exists(abs_debug_script):
            print "Error: - The debug script '%s' does not exist. Please try it again!" % abs_debug_script
            debug_script_name = raw_input("Please input the relative path of the script. Or press Enter to skip: ")
            if not debug_script_name:
                break
            abs_debug_script = r'%s\%s' % (script_path, debug_script_name)

    if debug_script_name:
        Treadmill.add(debug_script_name, "ErrorDebug")

    warning_as_pass = raw_input("\n\n# Do you want to treat warning as pass? Press 'y' to treat warning as pass: ")
    if warning_as_pass.lower().strip() == 'y' or warning_as_pass.lower().strip() == 'yes':
        warning_as_pass = 'y'
    else:
        warning_as_pass = 'n'

    if warning_as_pass == 'y':
        Treadmill.add('True', "WarningAsPass")
    else:
        Treadmill.add('False', "WarningAsPass")

    disable_reboot_function = raw_input("\n\n# Do you want to disable reboot function? Press 'y' to disable the reboot function: ")
    if disable_reboot_function.lower().strip() == 'y' or disable_reboot_function.lower().strip() == 'yes':
        disable_reboot_function = 'y'
    else:
        disable_reboot_function = 'n'
    if disable_reboot_function == 'y':
        Treadmill.add('True', "DisableRebootFunction")
    else:
        Treadmill.add('False', "DisableRebootFunction")

    skip_first_round_no_result = raw_input(
        "\n\n# Do you want to skip the second round testing if there is no result during the first round testing? Press 'y' to skip the second round testing: ")
    if skip_first_round_no_result.lower().strip() == 'y' or skip_first_round_no_result.lower().strip() == 'yes':
        skip_first_round_no_result = 'y'
    else:
        skip_first_round_no_result = 'n'

    if skip_first_round_no_result == 'y':
        Treadmill.add('True', "SkipFirstRoundNoResult")
    else:
        Treadmill.add('False', "SkipFirstRoundNoResult")

    wait_for_device = raw_input("\n\n# If device offline, will you keep waiting or quit? Press 'Enter' to wait. Press 'n' to quit:")
    if wait_for_device.lower().strip() == 'n' or wait_for_device.lower().strip() == 'no':
        wait_for_device = 'false'
    else:
        wait_for_device = 'true'
    Treadmill.add(wait_for_device, 'WaitForDevice' )

    print '****************************** General Exectuion Control Config is DONE ******************************\n'

def TestConfig():
    print '\n\n\n****************************** Begin for General Test Config ******************************'
    need_preparation = raw_input("\n# Will you run device environment preparation before testing? Press 'y' to run it or Enter to skip: ")
    if need_preparation != 'y':
        need_preparation = 'n'

    pre_install_path = raw_input(
        "\n\n# If you want to pre-install and launch applications before testing, please input the pre-install apks folder. Or press Enter to skip: ")
    while pre_install_path and not os.path.exists(pre_install_path):
        print "Error: - The pre-install apks folder '%s' does not exist. Please input again!" % pre_install_path
        pre_install_path = raw_input(
            "If you want to pre-install and launch applications before testing, please input the pre-install apks folder. Or press Enter to skip: ")

    camera_mode = raw_input("\n\n# Please select the working mode: [1, 2]?\t1:HyperSoftCam \t2:Disabled \n"
                            "The default mode is HyperSoftCam.\n: ")
    while camera_mode:
        try:
            if int(camera_mode) == 1 or int(camera_mode) == 2:
                break
        except:
            pass
        camera_mode = raw_input("[Error] Invalid Number. Please input again:")

    if camera_mode == "2":
        camera_mode = "Disabled"
    else:
        camera_mode = "HyperSoftCam"

    rnr_path = raw_input(
        "\n\n# If there is any RnR cases to test, please input its relative/absolute path. Or press Enter to skip: ")
    while rnr_path and not os.path.exists(rnr_path):
        print "Error: - The RnR path '%s' does not exist. Please input again!" % rnr_path
        rnr_path = raw_input(
            "If there is any RnR cases to test, please input its relative/absolute path. Or press Enter to skip: ")
    if rnr_path:
        Treadmill.add('AppVer,Resolution,DPI,OSVer,Model', 'RnRPriorityItems')
    Treadmill.add(camera_mode, "CameraMode")

    if rnr_path:
        Treadmill.add(rnr_path, "RnRfolder")

    if pre_install_path:
        Treadmill.add(pre_install_path, "PreInstallApksFolder")

    if need_preparation == 'y':
        Treadmill.add('True', "RunPreparation")
    else:
        Treadmill.add('False', "RunPreparation")
    print '****************************** General Test Config is DONE. ******************************\n'

def CollectGeneralConfigItems():
    TestConfig()
    GeneralExecutionSettings()
    GeneralExecutionControlConfig()
    Treadmill.add("True", "Breakpoint")
    Treadmill.add("False", "RebootForEachAPK")
    Treadmill.add("900", "SmokeTestTimeout")

def DefineOnDeviceTimeout():
    on_device_timeout = raw_input("\n\n# Please check your network bandwidth and set the timeout to wait for on-device"
                                  " download/install. Our suggestion is:\n"
                                  "If network bandhwidth=5Mb, timeout=1500seconds.\n"
                                  "If network bandhwidth=10Mb, timeout=900seconds.\n"
                                  "If network bandhwidth=15Mb, timeout=600seconds.\n"
                                  "Plesae input the timeout(default timeout=900), or press 'Enter' to skip it:")
    while True:
        try:
            if not on_device_timeout:
                break
            if int(on_device_timeout) > 0:
                break
        except:
            pass
        on_device_timeout = raw_input("[Error] Invalid Number. Please input again:")
    if on_device_timeout:
        Treadmill.add(on_device_timeout, "OnDeviceInstallTimeout")
    else:
        Treadmill.add('900', "OnDeviceInstallTimeout")


def CollectAPKConfigItems():
    print "\n\n\n***************************** Begin for Specific Config for Downloaded Mode ******************************"
    force_apk_list = raw_input("\n# Please input the path of package name list if you want to enable force"
                               " downloading according to the list, or press Enter to skip it: ")
    while force_apk_list and not os.path.exists(force_apk_list):
        print "Error: - The list file '%s' does not exist. Please try it again!" % force_apk_list
        force_apk_list = raw_input("Please input the correct path of force downloading apk list: ")

    if force_apk_list:
        Treadmill.add(force_apk_list, "ForceApkList")
        DefineOnDeviceTimeout()

    apk_list = raw_input("\n\n# Please input the path of apk list if you will test according to the list,"
                         "or press Enter to skip it: ")
    while apk_list and not os.path.exists(apk_list):
        print "Error: - The list file '%s' does not exist. Please try it again!" % apk_list
        apk_list = raw_input("Please input the correct path of apk list: ")

    if apk_list:
        Treadmill.add(apk_list, "ApkList")

    print ("\n\n# When APK test failed, you can choose to re-download it from Google Play and re-test."
           " Will you enable such feature?")
    redownload_option = raw_input("Press 'y' to enable, otherwise press 'Enter' to skip it:")
    if redownload_option != 'y':
        redownload_option = 'n'
    elif not force_apk_list:  # no need to set the timeout again if we enabled the force-online-downloading feature
        DefineOnDeviceTimeout()

    if redownload_option == 'y':
        Treadmill.add('True', "ReDownloadOnDevice")

    print "\n\nPlease enter the seed values for exploratory testing."
    print "The seed values's format could be splitted by the comma or in range."
    print "e.g 1,3,5,6 or 1-5, press enter to skip."
    seed_value_input = raw_input("")
    if not seed_value_input:
        seed_value_input = '1'
    match_pattern = r"^\d+-\d+$|^(\d+,)*\d+$"
    seed_value = ''
    while seed_value_input:
        match = re.match(match_pattern, seed_value_input)
        if match:
            seed_value_res = match.group()
            if seed_value_res.find(',') == -1 and seed_value_res.isdigit() and seed_value_res.count('-') == 0:
                seed_value = seed_value_res
                break
            elif seed_value_res.count('-') == 1:
                range_value = seed_value_res.split('-')
                left_value = range_value[0]
                right_value = range_value[1]
                if left_value.isdigit() and right_value.isdigit() and 0 < int(left_value) < int(right_value):
                    seed_value = ','.join(str(i) for i in range(int(left_value), int(right_value) + 1))
                    break
                else:
                    seed_value_input = raw_input("[Error] Invalid input, please input again:")
            else:
                seed_value = ','.join(seed_value_res.split(','))
                break
        else:
            seed_value_input = raw_input("[Error] Invalid input, please input again:")

    Treadmill.add(seed_value, 'SeedValue')

    multi_abi_testing = raw_input(
        "\n\nIf you want to enable the multi-abi testing, press 'y' to enable it.Or press Enter to skip: ")
    if not multi_abi_testing:
        multi_abi_testing = 'n'
    if multi_abi_testing and multi_abi_testing.isalpha() and multi_abi_testing.lower() == 'y':
        Treadmill.add('y', "MultiAbi")
    else:
        Treadmill.add('n', "MultiAbi")

    print '****************************** Specific Config for Downloaded Mode is DONE. ******************************\n'

def CollectOnDeviceDownloadConfigItems(enable_pkg_list=True):
    print "\n****************************** Begin for Specific Config for On Device Download Mode ******************************"
    DefineOnDeviceTimeout()

    if enable_pkg_list:
        package_list = raw_input("\n# Please input the path of package list if you will test according to the list,"
                                 " or press Enter to skip it: ")
        while package_list and not os.path.exists(package_list):
            print "Error: - The list file '%s' does not exist. Please try it again!" % package_list
            package_list = raw_input("Please input the correct path of package list: ")
    else:
        package_list = ''

    if not package_list:
        google_play_option = ("1. Download TopFreeApps Only",
                              "2. Download TopFreeGames Only",
                              "3. Download TopFreeApps and TopFreeGame",
                              "4. Download TopPaidApps Only",
                              "5. Download TopPaidGames Only",
                              "6. Download TopPaidApps and TopPaidGames")
        category_selection = raw_input("\n\n# Please select which APK Category to download[1..{}]:\n{}:"
                                       .format(len(google_play_option), '\n'.join(google_play_option)))
        while True:
            try:
                if 0 < int(category_selection) <= len(google_play_option):
                    break
            except:
                pass
            category_selection = raw_input("[Error] Invalid Number. Please input again:")

        start_rank = raw_input("\n\n# From which ranking number to download APK?:")
        while True:
            try:
                if int(start_rank) >= 1:
                    break
            except:
                pass
            start_rank = raw_input("[Error] Invalid Number. Please input again:")

        dl_num = raw_input("\n\n# How many APKs to download?:")
        while True:
            try:
                if int(dl_num) >= 1:
                    break
            except:
                pass
            dl_num = raw_input("[Error] Invalid Number. Please input again:")
        Treadmill.add(category_selection, "category_selection")
        Treadmill.add(start_rank, "start_rank")
        Treadmill.add(dl_num, "dl_num")
    else:
        Treadmill.add(package_list, "package_list")

    print '****************************** Specific Config for On Device Download Mode is DONE.******************************\n'

def apk_installation_option():

    installation_times = raw_input("\n\nPlease enter number of cycle for running each installation testing case."
                                   "The default value is 5:\n")
    
    if not installation_times:
        installation_times = '5'
    
    while installation_times:
        try:
            if installation_times.isdigit():
                break
        except:
            pass
        installation_times = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(installation_times, "installation_times")


def storage_test_option(test_type):

    Treadmill.add(test_type, "storage_test_type")

    if test_type != '2':
        tmp_file_size = raw_input("\n\nPlease enter temporarily file size(in units of MB) for testing."
                                  "The default size is 100MB:\n")

        if not tmp_file_size:
            tmp_file_size = '100'

        while tmp_file_size:
            try:
                if tmp_file_size.isdigit():
                    break
            except:
                pass
            tmp_file_size = raw_input("[Error] Invalid Number. Please input again:")

        Treadmill.add(tmp_file_size, "tmp_file_size")

    testing_times = raw_input("\n\nPlease enter number of cycle for running read/write testing over"
                              " eMMC.The default value is 5:\n")

    if not testing_times:
        testing_times = '5'

    while testing_times:
        try:
            if testing_times.isdigit():
                break
        except:
            pass
        testing_times = raw_input("[Error] Invalid Number. Please input again:")

    Treadmill.add(testing_times, "testing_times")


def CreateXML():
    # Treadmill = TreadmillConfig(xml)

    test_category = TestTypeSelection()
    if test_category == "LaunchTime":
        DeviceSelection()
        launchtime_option()
    elif test_category == "ApkInstallation":
        DeviceSelection()
        apk_installation_option()
    elif test_category == "Power":
        DeviceSelection()
        power_option()
    elif test_category == "Storage":
        storage_test_menu = ("1:MTP R/W over eMMc", "2:eMMC R/W(dd)")
        prompt = "\nPlease select the storage  testing type:\n\t{}\n" \
                 "The default type is MTP R/W over eMMc:".format('\n\t'.join(storage_test_menu))

        storage_input = raw_input(prompt)
        if not storage_input:
            storage_input = '1'

        while storage_input and storage_input.isdigit():
            if 1 <= int(storage_input) <= len(storage_test_menu):
                break
            else:
                storage_input = raw_input("[Error] Invalid Number. Please input again:")

        DeviceSelection()
        storage_test_option(storage_input)
    else:
        DeviceSelection()

        download_mode = raw_input("\n\n# Please select the APK downloading mode: [1, 2]?\n\t1: APK already downloaded\n"
                                  "\t2: Download on device from Google Play\nThe default mode is [1].\n: ")

        while download_mode and download_mode != '1' and download_mode != '2':
            download_mode = raw_input("[Error] Invalid Number. Please input again:")

        if download_mode == "2":
            download_mode = "on-device"
            CollectOnDeviceDownloadConfigItems()
        else:
            download_mode = "downloaded"
            CollectAPKConfigItems()

        Treadmill.add(download_mode, "APK_download_mode")

        CollectGeneralConfigItems()
        TestNumberConfig()


if __name__ == "__main__":
    tardev = ''
    refdev = ''
    usage = "usage: %prog [options] %DavinciFolder%. \nPlease use %prog -h for more details."
    version = "Copyright (c) 2015 Intel Corporation. All Rights Reserved."
    parser = OptionParser(usage=usage, version=version)
    parser.add_option("-f", "--file", dest="config_file", default="SmartRunner.config",
                      help="The name of output config file is defined here for silent mode, "
                           "the default name is SmartRunner.config")
    (options, args) = parser.parse_args()
    if len(args) != 1:
        print "Please input Davinci's folder. \nPlease use -h for more details."
        sys.exit(-1)
    Davinci_folder = args[0]
    if not os.path.exists(Davinci_folder + "\\DaVinci.exe"):
        print "'%s'\\DaVinci.exe does not exist." % Davinci_folder
        sys.exit(-1)

    Treadmill = TreadmillConfig(options.config_file)
    CreateXML()
    Treadmill.save()

    print "\n\n---------The config file %s is created successfully!-----------------------" % options.config_file

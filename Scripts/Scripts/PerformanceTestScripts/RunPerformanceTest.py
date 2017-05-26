import sys
import os
import optparse
import re
import datetime
import time
import logging.config
import stat
import xlsxwriter
from Helper import *
from ParseResult import GenerateBenchmarkResult
from xml.etree import ElementTree
import glob
import subprocess
import psutil
import ctypes

reload(sys) 
sys.setdefaultencoding('utf-8')

# get all the output xml file names
def FindOutputFilesOnQscript(qscript_file):
    output_files = []
    qscript_name = ""

    if os.path.exists(qscript_file):
        qscript_folder = os.path.abspath(os.path.dirname(qscript_file))

        # using Get-RnR mode, need to find out qscript first
        if (qscript_file.endswith(".xml")):
            xml_file = open(qscript_file, 'r')
            for line in xml_file:
                if "Script name=\"" in line:
                    sfile = line.split("Script name=\"")[1].split("\"")[0]
                    qscript_name = os.path.join(qscript_folder, sfile)
                    break
        # qscript mode
        elif (qscript_file.endswith(".qs")):
            qscript_name = qscript_file

        if os.path.exists(qscript_name):
            q_file = open(qscript_name, 'r')
            for line in q_file:
                if ".xml" in line:
                    file_name = line.split(' ')[-1].strip()
                    if not "\\" in file_name:
                        output_files.append(qscript_folder + "\\" + file_name)  # only file name
                    else:
                        output_files.append(file_name)  # absolute path
            q_file.close()
    return output_files


def ReadConfigFile(config_file_name):
    all_test_info = []
    root = ElementTree.parse(config_file_name).getroot()
    for child in root:
        one_test_info = {}
        one_test_info["TestName"] = child.tag   # eg: CFBench, Antutu
        one_test_info["TimeOut"] = ""           # time out of running DaVinci -p ...qs
        one_test_info["TestTimes"] = 3          # test times, default value is 3
        one_test_info["SleepSeconds"] = 300     # sleep between each test, default value is 300s
        one_test_info["QscriptPath"] = ""       # qscript file path
        one_test_info["CheckPoints"] = []       # check points 
        one_test_info["OutputFiles"] = []       # output xml file names

        for item in child:
            if (item.tag == "TimeOut"):
                one_test_info["TimeOut"] = item.text.strip()
            elif (item.tag == "TestTimes"):
                one_test_info["TestTimes"] = int(item.text.strip())
            elif (item.tag == "SleepSeconds"):
                one_test_info["SleepSeconds"] = int(item.text.strip())
            elif (item.tag == "QscriptPath"):
                one_test_info["QscriptPath"] = item.text.strip()
                one_test_info["OutputFiles"] = FindOutputFilesOnQscript(one_test_info["QscriptPath"])
            elif (item.tag == "CheckPoints"):
                for child_node in item:
                    if (child_node.tag == "CheckItem"):
                        one_test_info["CheckPoints"].append(child_node.text.strip())
        all_test_info.append(one_test_info)
    return all_test_info


def PrepareLog(script_path, logCfgFile, timestamp_nm):
    # Prepare the loggers
    if not os.path.exists(result_folder_name):
        os.mkdir(result_folder_name)

    template_file = open(logCfgFile, 'r')
    newLogCfgFile = "%s\\logconfig_new_%s.txt" % (script_path, timestamp_nm)
    new_file = open(newLogCfgFile , 'w')
    execution_log = result_folder_name + "\\result_%s.log" % timestamp_nm
    for line in template_file:
        if line.find("'result.log'") > -1:
            line = line.replace("'result.log'", "r'%s'" % execution_log)
        if line.find("'debug.log'") > -1:
            line = line.replace("'debug.log'", "r'" + result_folder_name + "\\debug_%s.log'" % timestamp_nm )
        if line.find("'performance.log'") > -1:
            line = line.replace("'performance.log'", "r'" + result_folder_name + "\\performance_%s.log'" % timestamp_nm )
        if line.find("'error.log'") > -1:
            line = line.replace("'error.log'", "r'" + result_folder_name + "\\error_%s.log'" % timestamp_nm )
        new_file.write(line)
    template_file.close()
    new_file.close()

    logging.config.fileConfig(newLogCfgFile)
    os.remove(newLogCfgFile)

def CleanTestEnvironment(all_test_info):
    try:
        for test_info in all_test_info:
            xml_files = test_info["OutputFiles"]
            for xml_file in xml_files:
                xml_folder = os.path.dirname(os.path.abspath(xml_file))
                xml_file_name = os.path.basename(os.path.abspath(xml_file))

                for file in os.listdir(xml_folder):
                    if file.endswith(xml_file_name):                # since we will rename xml file names to '1_round...', so use endswith()
                        os.remove(os.path.join(xml_folder, file))   # remove xml files before current test
    except Exception, e:
        DebugLog(str(e))

def RunBenchmarkTests(davinci_folder, all_test_info, target_device, timestamp_nm):
    try:
        CleanTestEnvironment(all_test_info)     # if the previous testing is terminated by user, need to clean up test environment
        all_output_info = []
        for test_info in all_test_info:
            current_test_name = test_info["TestName"]
            test_time_out = test_info["TimeOut"]
            current_qs_path = test_info["QscriptPath"]
            test_times = test_info["TestTimes"]
            sleep_seconds = test_info["SleepSeconds"]
            output_files = test_info["OutputFiles"]
            current_test_time = 1
            one_output_info = {}

            if (current_test_name == "" or test_time_out == "" or current_qs_path == ""):
                PrintAndLogErr("missing configuration item, please check your config file")
                continue

            current_qs_path = os.path.abspath(current_qs_path)
            if not os.path.exists(current_qs_path):
                PrintAndLogErr("qscript file: %s does not exist" % current_qs_path)
                continue

            one_output_info["TestName"] = current_test_name
            one_output_info["OutputFiles"] = {}

            dic_output_files = {}
            while (current_test_time <= test_times):
                print ("Sleep %s seconds before testing to cool the device..." % sleep_seconds)
                time.sleep(sleep_seconds)

                check_battery(davinci_folder, target_device, 20, 600)
                RunEachTest(davinci_folder, current_qs_path, current_test_name, test_time_out, target_device)

                # rename output file 
                for file_name in output_files:
                    if os.path.exists(file_name):
                        base_name = os.path.basename(file_name)
                        folder_name = os.path.dirname(file_name)
                        new_base_name = str(current_test_time) + "_round_" + base_name
                        new_path = os.path.join(folder_name, new_base_name)
                        if os.path.exists(new_path):            # if there is an old file, need to remove it before rename to it
                            os.remove(new_path)
                        os.rename(file_name, new_path)
                        if not dic_output_files.has_key(current_test_time):
                            dic_output_files[current_test_time] = [new_path]
                        else:
                            dic_output_files[current_test_time].append(new_path)
                    else:
                        dic_output_files[current_test_time] = ""
                current_test_time = current_test_time + 1

            one_output_info["OutputFiles"] = dic_output_files
            all_output_info.append(one_output_info)
    except Exception, e:
        DebugLog(str(e))
        return all_output_info
    return all_output_info


def RunEachTest(davinci_folder, current_qs_path, current_test_name, test_time_out, target_device):
    qs_file_name = os.path.basename(current_qs_path)
    Is_Debug = False

    timestamp_tmp = time.strftime('%Y-%m-%d_%H-%M-%S',time.localtime())
    logfile_name = "%s\\%s_%s.log" % (result_folder_name, qs_file_name ,timestamp_tmp)
    errfile_name = "%s\\%s_err_%s.log" % (result_folder_name,qs_file_name ,timestamp_tmp)
    logfile = open(logfile_name , "w")
    errfile = open(errfile_name, "w")

    PrintAndLogInfo(("\n******************** New Test - %s ********************") % (current_test_name))
    PrintAndLogInfo("Target device: %s" % target_device)
    PrintAndLogInfo("Qscript  file: %s" % qs_file_name)
    PrintAndLogInfo("  - Start at: %s" % time.strftime('%Y-%m-%d %H:%M:%S',time.localtime()))
    PrintAndLogInfo("   Please wait several minutes for testing ...")
    cmd = "%s\\DaVinci.exe -device %s -p \"%s\" -timeout %s" %(davinci_folder, target_device, current_qs_path, test_time_out)
    start = datetime.datetime.now()
    timeout = int(test_time_out)
    no_result_app_list = r"%s\no_result_app_list_%s.txt" % (davinci_folder, timestamp_tmp)

    if Is_Debug:
        process = subprocess.Popen(cmd, stdout=logfile, stderr = errfile)
    else:
        # Do not show crash dialog
        SEM_NOGPFAULTERRORBOX = 0x0002 # From MSDN
        ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX);
        CREATE_NO_WINDOW = 0x8000000
        subprocess_flags = CREATE_NO_WINDOW
        FNULL = open(os.devnull, 'w')
        process = subprocess.Popen(cmd, stdout=logfile, stderr = errfile, creationflags=subprocess_flags)

    pid = process.pid
    while process.poll() is None:
        try:
            p = psutil.Process(pid)
            cpu_percent = p.cpu_percent(interval=0.1)
            cpu_count =  int(psutil.cpu_count())
            cpu_maxload = 100 * cpu_count
            cpu_workload = (cpu_percent / cpu_maxload) * 100
            pmem = p.memory_info()
            rss = int(pmem.rss / 1000)
            PerfLog("CPU: %s%s" %( str(cpu_workload), "%"))
            PerfLog("Mem Usage: %s(K)" % ( str(rss)))

            time.sleep(10)
            # Check CPU and memory usage
            now = datetime.datetime.now()
            # Time out...
            if (now - start).seconds> timeout:
                PrintAndLogInfo("  - Test Timeout %ssec." % (str((now - start).seconds)))
                # If debug, do not terminate the process so that we can investigate.
                res = -3
                with open(no_result_app_list, 'a') as f:
                    f.write("Timeout|---|" + qs_file_name + "|---|" + errfile_name + os.linesep)
                if not Is_Debug:
                    process.terminate()
                    KillDavinciProcess(str(pid))
        except Exception, e:
            DebugLog(str(e))
    PrintAndLogInfo("  - End   at: %s\n" % time.strftime('%Y-%m-%d %H:%M:%S',time.localtime()))


def GenerateDaVinciConfig(davinci_folder, target_device, camera_mode, test_dev_index, all_devices_number):
    xml_cfg = ""

    qagent_port = FindFreePort(8888, test_dev_index, all_devices_number)
    if qagent_port == -1:
        PrintAndLogErr("Failed to get QAgent Port for %s\n. Quit the program." % target_device)
        sys.exit(-1)
    magent_port = FindFreePort(9888, test_dev_index, all_devices_number)
    if magent_port == -1:
        PrintAndLogErr("Failed to get MAgent Port for %s\n. Quit the program." % target_device)
        sys.exit(-1)
    hyper_port = FindFreePort(23456 , test_dev_index, all_devices_number)
    if hyper_port == -1:
        PrintAndLogErr("Failed to get HyperCamera Port for %s\n. Quit the program." % target_device)
        sys.exit(-1)

    xml_cfg = xml_cfg + """
<AndroidTargetDevice>
  <name>"""+ target_device +"""</name>
  <screenSource>"""+ camera_mode +"""</screenSource>
  <qagentPort>"""+ str(qagent_port) +"""</qagentPort>
  <magentPort>"""+ str(magent_port) +"""</magentPort>
  <hypersoftcamPort>"""+ str(hyper_port) +"""</hypersoftcamPort>""" + """
</AndroidTargetDevice>"""

    xml_content =  """\
<?xml version="1.0"?>
<DaVinciConfig>
 <androidDevices>"""+ xml_cfg +"""
 </androidDevices>
 <windowsDevices />
 <chromeDevices />
</DaVinciConfig>
"""
    with open(davinci_folder + "\\DaVinci.config", "w") as f:
        f.write(xml_content)

def check_battery(davinci_folder, dev, threshold, sleep_time):
    while True:
        print 'Checking device battery...'
        dev_battery = GetDeviceBattery(davinci_folder, dev)
        print "Battery is {0}...".format(dev_battery)
        if dev_battery and dev_battery.isdigit() and int(dev_battery) < threshold:
            print "Charging device..."
            time.sleep(sleep_time)
        else:
            break

if __name__ == "__main__":
    script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
    config_file_name = script_path + "\\PerfomanceTest.xml"
    if not os.path.exists(config_file_name):
        print "Perfomance test config %s does not exist." % config_file_name
        sys.exit(-1)

    davinci_folder = os.path.abspath(sys.argv[1])
    if not os.path.exists(davinci_folder):
        print "DaVinci folder %s does not exist." % davinci_folder
        sys.exit(-1)

    logCfgFile = script_path + "\\logconfig.txt"
    if not os.path.exists(logCfgFile):
        print "%s Log Config File does not exist. Please reinstall to recover it." % logCfgFile
        sys.exit(-1)

    # prepare log, will write log into result\debug_%timestamp%.txt
    result_folder_name = script_path + "\\result"
    timestamp_nm = time.strftime('%Y-%m-%d_%H-%M-%S',time.localtime())
    PrepareLog(script_path, logCfgFile, timestamp_nm);

    # get all devices
    all_devices = GetAllDevices(davinci_folder)
    if (len(all_devices) == 0):
        PrintAndLogErr("No available device, exit!")
        sys.exit(-1)

    # read performance test config file
    all_test_info = ReadConfigFile(config_file_name)
    if (len(all_test_info) == 0):
        PrintAndLogErr("Couldn't find any test info in %s." % config_file_name)
        sys.exit(-1)

    # select target device and test mode
    target_device = ""
    all_devices = ShowDevTable(all_devices, "Device Group Information", davinci_folder)
    complete = False
    test_dev = ""
    while complete == False:
        test_dev = raw_input("\n\n# Please choose one device to test. Input index:")
        try:
            int(test_dev)
        except:
            print "[Error]: Invalid number %s" % (test_dev)
            continue
        if int(test_dev.strip()) < 1 or int(test_dev.strip()) > len(all_devices):
            print "[Error]: Invalid number %s" % (test_dev)
        else:
            target_device = all_devices[int(test_dev) -1]
            complete = True
            break

    # select camera mode
    camera_mode = raw_input("\n\n# Please select the working mode: [1, 2]?\t1:Disabled \t2:HyperSoftCam: ")
    while camera_mode <> "":
        try:
            if int(camera_mode) ==1 or int(camera_mode) == 2:
                break
        except:
            pass
        camera_mode = raw_input("[Error] Invalid Number. Please input again:")
    if camera_mode == "2":
        camera_mode = "HyperSoftCam"
    else:
        camera_mode = "Disabled"

    # generate davinci config file
    GenerateDaVinciConfig(davinci_folder, target_device, camera_mode, int(test_dev), len(all_devices))

    # run benchmark tests
    output_info = RunBenchmarkTests(davinci_folder, all_test_info, target_device, timestamp_nm)

    # Generate summary.xlsx
    GenerateBenchmarkResult(script_path, all_test_info, output_info, timestamp_nm)

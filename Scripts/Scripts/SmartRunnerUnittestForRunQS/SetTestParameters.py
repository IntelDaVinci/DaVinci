import subprocess
import sys


def check_options(metafunc):
    # Prepare the globals
    if metafunc.config.option.Davinci_folder == '':
        print "Please input Davinci_folder."
        sys.exit(-1)
    if metafunc.config.option.script_path == '':
        print "Please input script_path."
        sys.exit(-1)
    if metafunc.config.option.xml_config == '':
        print "Please input xml_config."
        sys.exit(-1)
    if metafunc.config.option.config_file == '':
        print "Please input config_file."
        sys.exit(-1)
    if metafunc.config.option.apk_folder == '':
        print "Please input apk_folder."
        sys.exit(-1)
    if metafunc.config.option.work_folder == '':
        print "Please input work_folder."
        sys.exit(-1)

def search_device(adb_path):
    print r'Search Device: {0}\adb.exe devices'.format(adb_path)
    p = subprocess.Popen(r'{0}\adb.exe devices'.format(adb_path), stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False)

    res = []
    while True:
        if p.poll() is not None:
            res = p.stdout.readlines()
            adb_res = True
            p.stdout.flush()
            break
    header_found = False

    clean_res = []
    for line in res:
        if line.startswith('* daemon not running. starting it now') or \
                line.startswith('* daemon started successfully *') or \
                line.startswith('adb server is out of date.  killing...'):
            continue
        clean_res.append(line)

    devices = []
    for line in clean_res:
        if line.find("List of devices attached") >= 0:
            header_found = True
            continue
        if header_found:
            if line.strip():
                device_name = line.split('\t')[0]
                device_status = (line.split('\t')[1]).strip('\n').strip('\r')
                if device_status == "device":
                    devices.append(device_name)

    return devices

def InitializeGlobals(metafunc):
    check_options(metafunc)

    global_dict_raw = {'Davinci_folder': metafunc.config.option.Davinci_folder,
                       'script_path': metafunc.config.option.script_path,
                       'xml_config': metafunc.config.option.xml_config,
                       'config_file_name': metafunc.config.option.config_file,
                       'apk_folder': metafunc.config.option.apk_folder,
                       'work_folder': metafunc.config.option.work_folder}

    para_dict = {}
    for key in global_dict_raw.keys():
        para_dict[key] = global_dict_raw[key].strip().strip('"').strip("'")

    devices = search_device(r'{0}\platform-tools'.format(para_dict['Davinci_folder']))

    if not devices:
        print "No device connected."
        sys.exit(-1)

    content = []
    ref_dev = []
    target_dev = []
    if len(devices) > 1:
        ref_dev.append(devices[1])
        content.append("ref_dev = ['{0}']".format(devices[1]))
    else:
        content.append("ref_dev = []")
    if len(devices) > 0:
        content.append("target_dev = ['{0}']".format(devices[0]))
        target_dev.append(devices[0])
    else:
        content.append("target_dev = []")

    for k in para_dict.keys():
        value = para_dict[k].replace('\\', '\\\\')
        content.append("{0} = '{1}'".format(k, value))

    with open('{0}\SmartRunnerUnittestForRunQS\Globals.py'.format(para_dict['script_path']), 'w') as f:
        f.write('\n'.join(content))
    return para_dict, target_dev, ref_dev

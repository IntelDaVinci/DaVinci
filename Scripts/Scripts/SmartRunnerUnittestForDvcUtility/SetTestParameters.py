import sys, os, subprocess

def check_Davinci_folder(metafunc):
    # Prepare the globals
    Davinci_folder = metafunc.config.option.Davinci_folder
    if Davinci_folder == '':
        print "Please input Davinci_folder."
        sys.exit(-1)
    if not os.path.exists(Davinci_folder):
        print "Davinci_folder does not exist. %s" %Davinci_folder
        sys.exit(-1)
    davinci_bin = r'{0}\\Davinci.exe'.format(Davinci_folder)
    if not os.path.exists(davinci_bin):
        print "No Davinci.exe under. %s" % Davinci_folder
        sys.exit(-1)

    adb_path = r'{0}\\platform-tools\\adb.exe'.format(Davinci_folder)
    if not os.path.exists(adb_path):
        print "No adb.exe under {0}\\platform-tools. %s" % Davinci_folder
        sys.exit(-1)

    return Davinci_folder

def check_script_path(metafunc):
    if metafunc.config.option.script_path == '':
        print "Please input script path."
        sys.exit(-1)
    if not os.path.exists(metafunc.config.option.script_path):
        print "script_path does not exist. %s" % metafunc.config.option.script_path
        sys.exit(-1)
    return metafunc.config.option.script_path


def search_device(adb_path):
    print r'Search Device: {0}\adb.exe devices'.format(adb_path)
    p = subprocess.Popen(r'{0}\adb.exe devices'.format(adb_path), stderr=subprocess.STDOUT,
                         stdout=subprocess.PIPE, shell=False)

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

        if not devices:
            print "No device connected."
            sys.exit(-1)
    return devices



from SetTestParameters import InitializeGlobals
import subprocess
import wmi

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    command = r'{0}\platform-tools\adb.exe '.format(global_dict['Davinci_folder'])
    subprocess.call([command, 'kill-server'])

    command = r'{0}\platform-tools\adb.exe'.format(global_dict['Davinci_folder'])
    subprocess.call([command, 'start-server'])

    # Prepare the parametors for the target function.
    metafunc.parametrize('clean_pattern, check_pattern, process_name, expected',
                         [('{0}\adb.exe'.format(global_dict['Davinci_folder']), r'fork-server server', 'adb.exe', True),
                          ('fork-server server', r'fork-server server', 'adb.exe', False)] )

def checkProcess(pattern, process_name):
    c = wmi.WMI()
    target_process = c.Win32_Process(name="{}".format(process_name))
    res = False
    for process in target_process:
        if process is None or process.Commandline is None:
            continue
        if (process.Commandline).find(pattern) > -1:
            res = True
    return res

def test_CleanProcess(clean_pattern, check_pattern, process_name, expected):
    res = checkProcess(check_pattern, process_name)

    import coverage
    from run_qs import CleanProcess

    cov = coverage.Coverage(data_suffix='cleanProcess', config_file=True)
    cov.start()

    CleanProcess(clean_pattern, process_name)
    cov.stop()
    cov.save()

    res = checkProcess(check_pattern, process_name)
    assert res == expected
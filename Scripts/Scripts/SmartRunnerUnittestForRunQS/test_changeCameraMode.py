from SetTestParameters import InitializeGlobals
import re

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    metafunc.parametrize('Davinci_folder, newMode',
                         [(global_dict['Davinci_folder'], 'Disabled'),(global_dict['Davinci_folder'], 'HyperSoftCam')] )

def CheckCameraMode(Davinci_folder):
    pattern = re.compile('.*<screenSource>(.*)</screenSource>.*')
    with open(Davinci_folder + "\\DaVinci.config", 'r') as f:
        config_txt = f.readlines()
    mode = ''
    for txt in config_txt:
        m = pattern.match(txt)
        if m:
            mode = m.group(1)
    return mode

def test_ChangeCameraMode(Davinci_folder, newMode):
    import coverage

    cov = coverage.Coverage(data_suffix='ChangeCameraMode', config_file=True)
    cov.start()
    from run_qs import ChangeCameraMode
    ChangeCameraMode(newMode)
    cov.stop()
    cov.save()
    res = CheckCameraMode(Davinci_folder)
    assert res == newMode
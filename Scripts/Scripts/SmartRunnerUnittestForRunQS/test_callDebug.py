from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    device_name = ''
    if target_dev:
        device_name = target_dev[0]
    logFolder = global_dict['work_folder']
    metafunc.parametrize("device_name, logFolder",
                         [(device_name, logFolder)] )

def test_callDebug(device_name, logFolder):
    import coverage

    cov = coverage.Coverage(data_suffix='callDebug', config_file=True)
    cov.start()
    from run_qs import CallDebug
    assert CallDebug(device_name, logFolder) == True
    cov.stop()
    cov.save()








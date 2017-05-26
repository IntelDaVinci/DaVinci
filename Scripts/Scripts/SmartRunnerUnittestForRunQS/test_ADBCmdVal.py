from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    device_name = ''
    if target_dev:
        device_name = target_dev[0]
    logFolder = metafunc.config.option.work_folder
    metafunc.parametrize("devices",
                         [(target_dev)] )

def test_ADBCmdVal(devices):
    import coverage

    cov = coverage.Coverage(data_suffix='ADBCmdVal', config_file=True)
    cov.start()
    from run_qs import ADBCmdVal
    ADBCmdVal(devices)
    cov.stop()
    cov.save()

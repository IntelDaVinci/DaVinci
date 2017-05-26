from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    device_name = ''
    if target_dev:
        device_name = target_dev[0]
    print target_dev
    metafunc.parametrize("device, expected",
                         [(device_name, True), ('Fake_Device', False)] )

def test_RecoverDevice(device, expected):
    import coverage

    cov = coverage.Coverage(data_suffix='RecoverDevice', config_file=True)
    cov.start()
    from run_qs import RecoverDevice
    res = RecoverDevice(device)
    assert  res == expected

    cov.stop()
    cov.save()

from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    device_name = target_dev[0]
    metafunc.parametrize('device_name, expected',
                         [(device_name, 'COM 2'), ('Fake_device', '')] )

def test_GetPowerPusher(device_name, expected):
    from run_qs import GetPowerPusher

    import coverage

    cov = coverage.Coverage(data_suffix='GetPowerPusher', config_file=True)
    cov.start()

    res = GetPowerPusher(device_name)
    cov.stop()
    cov.save()
    assert res == expected
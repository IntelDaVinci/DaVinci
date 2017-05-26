from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    metafunc.parametrize('dev, expected',
                         [('Fake_device', False), (target_dev[0], True)])

def test_DeviceColdRecovery(dev, expected):
    from run_qs import DeviceColdRecovery

    import coverage

    cov = coverage.Coverage(data_suffix='deviceColdRecovery', config_file=True)
    cov.start()

    res = DeviceColdRecovery(dev)
    cov.stop()
    cov.save()
    assert res == expected

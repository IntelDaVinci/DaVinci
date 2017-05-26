from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    metafunc.parametrize('dev, expected',
                         [('Fake_device', False), (target_dev[0], True)] )

def test_RePlugDevice(dev, expected):
    from run_qs import RePlugDevice

    import coverage

    cov = coverage.Coverage(data_suffix='RePlugDevice', config_file=True)
    cov.start()

    res = RePlugDevice(dev)
    cov.stop()
    cov.save()
    assert res == expected
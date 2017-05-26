from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    metafunc.parametrize('dev, expected',
                         [(['Fake_device'], 1),(target_dev, 0)] )


def test_WaitForSoftRecovery(dev, expected):
    from run_qs import WaitForSoftRecovery

    import coverage

    cov = coverage.Coverage(data_suffix='WaitForSoftRecovery', config_file=True)
    cov.start()

    res = WaitForSoftRecovery(dev)
    cov.stop()
    cov.save()
    assert  len(res)  == expected


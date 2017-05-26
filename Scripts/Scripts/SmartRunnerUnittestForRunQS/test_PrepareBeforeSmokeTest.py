from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    metafunc.parametrize('defined_dev',
                         [(target_dev)] )

def test_PrepareBeforeSmokeTest(defined_dev):
    from run_qs import PrepareBeforeSmokeTest

    import coverage

    cov = coverage.Coverage(data_suffix='PrepareBeforeSmokeTest', config_file=True)
    cov.start()

    PrepareBeforeSmokeTest(defined_dev)
    cov.stop()
    cov.save()

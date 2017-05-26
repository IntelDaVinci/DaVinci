from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

def test_Restart_adb():
    from run_qs import Restart_adb

    import coverage

    cov = coverage.Coverage(data_suffix='Restart_adb', config_file=True)
    cov.start()

    Restart_adb()
    cov.stop()
    cov.save()

from SetTestParameters import InitializeGlobals

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)



def test_StartADB_Server():
    from run_qs import StartADB_Server

    import coverage

    cov = coverage.Coverage(data_suffix='StartADB_Server', config_file=True)
    cov.start()

    StartADB_Server()
    cov.stop()
    cov.save()

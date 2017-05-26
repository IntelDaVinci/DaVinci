from SetTestParameters import InitializeGlobals
import glob, sys

def pytest_generate_tests(metafunc):
    # Prepare the globals
    global_dict, target_dev, ref_dev = InitializeGlobals(metafunc)

    # Prepare the parametors for the target function.
    apk_folder = global_dict['apk_folder']
    files = glob.glob(ur'%s\*.apk' % apk_folder)

    if  len(files) > 0:
        apk_path = files[0]
    else:
        print "no apk in apk_folder %s" % apk_folder
        sys.exit(-1)
    metafunc.parametrize('current_target_dev, apk_name',
                         [(target_dev[0], apk_path)] )

def test_RetryTest(current_target_dev, apk_name):
    from run_qs import RetryTest

    import coverage

    cov = coverage.Coverage(data_suffix='Restart_adb', config_file=True)
    cov.start()
    executed_times = 2
    test_times = 3
    pass_number = 1
    pass_criteria = 2
    rnr_script = ''
    dep_list = []
    test_description = 'Unittest'
    RetryTest(executed_times, test_times, pass_number, pass_criteria,
              current_target_dev, apk_name, rnr_script, dep_list, test_description
              )
    cov.stop()
    cov.save()

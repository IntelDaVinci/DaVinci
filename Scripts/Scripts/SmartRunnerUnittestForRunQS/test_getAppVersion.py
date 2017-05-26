import glob,sys

from generate_summary import getAppVersion
from SetTestParameters import InitializeGlobals

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
    metafunc.parametrize('davincifolder, apk_path',
                         [(global_dict['Davinci_folder'], apk_path)] )

def test_getAppVersion(davincifolder, apk_path):
    import coverage

    cov = coverage.Coverage(data_suffix='getAppVersion', config_file=True)
    cov.start()

    version = getAppVersion(davincifolder, apk_path)
    cov.stop()
    cov.save()
    print version
    assert version != ''



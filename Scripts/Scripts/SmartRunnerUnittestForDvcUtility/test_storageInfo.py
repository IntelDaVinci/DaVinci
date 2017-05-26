from SetTestParameters import check_Davinci_folder, search_device
import logging
def pytest_generate_tests(metafunc):
    Davinci_folder = check_Davinci_folder(metafunc)
    devices = search_device(r'{0}\platform-tools'.format(Davinci_folder))
    metafunc.parametrize("Davinci_folder, target_name",
                         [(metafunc.config.option.Davinci_folder, devices[0])])


def test_CheckFreeStorage(Davinci_folder, target_name):
    import coverage

    cov = coverage.Coverage(data_suffix='StorageInfo', config_file=True)
    cov.start()

    from dvcutility import StorageInfo
    logging.info("testing Enviroment.CheckFreeStorage")
    storage_obj = StorageInfo(r'%s\platform-tools\adb.exe' % Davinci_folder)
    free_storage_val_str, free_ram_val_str = storage_obj.CheckFreeStorage(target_name)
    cov.stop()
    cov.save()
    logging.info('free_storage_val_str:' + free_storage_val_str)
    logging.info('free_ram_val_str:' + free_ram_val_str)

    assert free_storage_val_str != ''
    assert  free_ram_val_str != ''



def pytest_addoption(parser):
    parser.addoption("--Davinci_folder", action="store", default='',
                     help="To specify the Davinci Folder.")
    parser.addoption("--script_path", action="store", default='',
                     help="To specify the scripts path.")
    parser.addoption("--config_file", action="store",  default='',
                      help="To specify the config file when running.")
    parser.addoption("--apk_folder", action="store", default="",
                      help="To input the folder path for the Applications.")
    parser.addoption("--work_folder", action="store", default="",
                      help="To input the path of workspace.")
    parser.addoption("--xml_config", action="store", default="",
                      help="To input the Treadmill xml config file for silence mode.")
    parser.addoption("--apk_path", action="store", default="",
                     help="To input the Treadmill xml config file for silence mode.")



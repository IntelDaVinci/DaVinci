
def pytest_addoption(parser):
    parser.addoption("--Davinci_folder", action="store", default='',
                     help="To specify the Davinci Folder.")
    parser.addoption("--script_path", action="store", default='',
                     help="To specify the scripts path.")



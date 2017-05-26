import pytest
import os, sys

class MyPlugin:
    def pytest_sessionfinish(self):
        print("\n\n*** test run reporting finishing")

from optparse import OptionParser
script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
config_file_name = script_path + "\\default_device_cfg.txt"
usage = "Please use %prog -h for more details."
version = "Copyright (c) 2016 Intel Corporation. All Rights Reserved."
parser = OptionParser(usage=usage, version=version)
parser.add_option("-d", "--Davinci_folder", dest="Davinci_folder", default='',
                  help="To define Davinci binary folder.")
parser.add_option("-s", "--script_path", dest="script_path", default='',
                  help="To define script path. By default, it is %Davinci_folder%\\..\\Scripts")
parser.add_option("-x", "--xml_config", dest="xml_config", default="",
                  help="To input the SmartRunner xml config file.")
parser.add_option("-c", "--config_file", dest="config_file", default='',
                  help="To specify the config file when running. By default, it is %xml_config%.txt")
parser.add_option("-p", "--apk_folder", dest="apk_folder", default="",
                  help="To input the folder path for the Applications.")
parser.add_option("-w", "--work_folder", dest="work_folder", default="",
                  help="To input the path of work folder. By default, it is the same with %apk_folder%")
parser.add_option("-t", "--testcase", dest="testcase", default="",
                  help="To input which test case to run.")
parser.add_option("-r", "--report_coverage", action="store_true", dest="report_coverage", default=False,
                  help="To generate the coverage report.")

(options, args) = parser.parse_args()

Davinci_folder = options.Davinci_folder
if not os.path.exists(Davinci_folder):
    print "Davinci binary folder does not exist: {0}".format(Davinci_folder)
    sys.exit(-1)

if not os.path.exists('{0}\\DaVinci.exe'.format(Davinci_folder)):
    print "Davinci binary does not exist: {0}\\DaVinci.exe".format(Davinci_folder)
    sys.exit(-1)

script_path = options.script_path
if script_path == '':
    script_path = '{0}\\..\\Scripts'.format(Davinci_folder)
if not os.path.exists(script_path):
    print "Invalid script_path."
    sys.exit(-1)

xml_config = options.xml_config
if not os.path.exists(xml_config):
    print "Invalid xml_config."
    sys.exit(-1)

config_file = options.config_file
if config_file == '':
    config_file = '{0}.txt'.format(xml_config)
if not os.path.exists(config_file):
    print "Invalid config_file."
    sys.exit(-1)

apk_folder = options.apk_folder
if not os.path.exists(apk_folder):
    print "Invalid apk_folder."
    sys.exit(-1)

work_folder = options.work_folder
if work_folder == '':
    work_folder = apk_folder

#pytest.main(r'-x C:\\Users\\jzhan66\\Documents\\uBT\\Davinci\\Scripts\\Scripts\\SmartRunnerUnittest --Davinci_folder C:\\Users\\jzhan66\\Documents\\uBT\\Davinci\\Scripts\\Release --script_path C:\\Users\\jzhan66\\Documents\\uBT\\Davinci\\Scripts\\Scripts --xml_config SmartRunner.config --config_file SmartRunner.config.txt --apk_folder C:\\Users\\jzhan66\\Documents\\uBT\\Davinci\\debug_apk --work_folder C:\\Users\\jzhan66\\Documents\\uBT\\Davinci\\debug_apk'
#            , plugins=[MyPlugin()])

unittest_path=os.path.dirname(os.path.abspath(sys.argv[0]))

if os.path.sep=='\\':
    unittest_path=unittest_path.replace('\\','\\\\')
    Davinci_folder=Davinci_folder.replace('\\','\\\\')
    script_path=script_path.replace('\\','\\\\')
    xml_config=xml_config.replace('\\','\\\\')
    config_file=config_file.replace('\\','\\\\')
    apk_folder=apk_folder.replace('\\','\\\\')
    work_folder=work_folder.replace('\\','\\\\')


if options.testcase:
    pytest.main(
        r'{0} --Davinci_folder {1} --script_path {2} --xml_config {3} --config_file {4} --apk_folder {5} --work_folder {6}'.format(
            options.testcase, Davinci_folder, script_path, xml_config, config_file, apk_folder, work_folder)
        , plugins=[MyPlugin()])
else:
    pytest.main(
        r'{0} --Davinci_folder {1} --script_path {2} --xml_config {3} --config_file {4} --apk_folder {5} --work_folder {6}'.format(
            unittest_path, Davinci_folder, script_path, xml_config, config_file, apk_folder, work_folder)
        , plugins=[MyPlugin()])


if options.report_coverage:
    print "To create coverage report ..."
    import coverage

    cov = coverage.Coverage(config_file=True)
    cov.combine([unittest_path])
    coverage_report_path = r'{0}\CoverageReport'.format(unittest_path)
    cov.html_report(directory=coverage_report_path)
    print "Coverage report is done. Please reer to " + coverage_report_path

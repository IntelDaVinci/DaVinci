import pytest
import os
import sys
import logging
import time

class MyPlugin:
    def pytest_sessionfinish(self):
        print("\n*** test run reporting finishing")

from optparse import OptionParser
usage = "Please use %prog -h for more details."
version = "Copyright (c) 2016 Intel Corporation. All Rights Reserved."
parser = OptionParser(usage=usage, version=version)
parser.add_option("-r", "--report_coverage", action="store_true", dest="report_coverage", default=False,
          help="To generate the coverage report.")
parser.add_option("-s", "--script_path", dest="script_path", default='',
                  help="To define the SmartRunner script path.")
parser.add_option("-d", "--Davinci_folder", dest="Davinci_folder", default='',
                  help="To define Davinci_folder.")
parser.add_option("-t", "--testcase", dest="testcase", default="",
                  help="To input which test case to run.")

(options, args) = parser.parse_args()
# Change this variable unittest for what unittest script you need to run.
unittest_path=os.path.dirname(os.path.abspath(sys.argv[0]))
Davinci_folder = options.Davinci_folder
script_path = options.script_path
para_dict = {}
para_str = ''
if os.path.sep=='\\':
    unittest_path=unittest_path.replace('\\','\\\\')
if Davinci_folder !=  '':
    if os.path.sep=='\\':
        Davinci_folder= Davinci_folder.replace('\\','\\\\')
        para_str += ' --Davinci_folder ' + Davinci_folder
if script_path != '':
    if os.path.sep == '\\':
        script_path=script_path.replace('\\','\\\\')
        para_str += ' --script_path ' + script_path


now_timestamp_nm = time.strftime('%Y-%m-%d_%H-%M-%S', time.localtime())
logging.basicConfig(filename='run_unittest_dvcutility_{0}.log'.format(now_timestamp_nm), level=logging.INFO)
logging.info('Begin to test')

if options.testcase:
    pytest.main(
    r'{0} {1}'.format(
        options.testcase,
        para_str)
    , plugins=[MyPlugin()])
else:
    pytest.main(
        r'{0} {1}'.format(
            unittest_path,
            para_str)
        , plugins=[MyPlugin()])

logging.info('done')


# After coverage report is combined and created, the raw coverage data will be lost.
# So by default, we will not create it.
if options.report_coverage:
    import coverage
    cov = coverage.Coverage(config_file=True)
    cov.combine([unittest_path])
    cov.html_report(directory=r'{0}\CoverageReport'.format(unittest_path))
    print "\nCoverage report is done."
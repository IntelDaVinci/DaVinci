import pytest
import os
import sys

class MyPlugin:
    def pytest_sessionfinish(self):
        print("\n*** test run reporting finishing")

from optparse import OptionParser
usage = "Please use %prog -h for more details."
version = "Copyright (c) 2016 Intel Corporation. All Rights Reserved."
parser = OptionParser(usage=usage, version=version)
parser.add_option("-t", "--testcase", dest="testcase", default="",
                  help="To input which test case to run.")
parser.add_option("-r", "--report_coverage", action="store_true", dest="report_coverage", default=False,
          help="To generate the coverage report.")
(options, args) = parser.parse_args()

# Change this variable unittest for what unittest script you need to run.
unittest_path=os.path.dirname(os.path.abspath(sys.argv[0]))

if options.testcase:
    ## To execute one specific unittest script.
    pytest.main(
        r'{0}'.format(
            options.testcase)
        , plugins=[MyPlugin()])
else:
    ## To execute all the unittest scripts under unittest_path..
    pytest.main(
        r'{0}'.format(
        unittest_path)
        , plugins=[MyPlugin()])

# After coverage report is combined and created, the raw coverage data will be lost.
# So by default, we will not create it.
if options.report_coverage:
    import coverage
    cov = coverage.Coverage(config_file=True)
    cov.combine([unittest_path])
    cov.html_report(directory=r'{0}\CoverageReport'.format(unittest_path))
    print "\nCoverage report is done."
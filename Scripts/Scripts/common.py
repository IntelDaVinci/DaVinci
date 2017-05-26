'''
 ----------------------------------------------------------------------------------------

 "Copyright 2014-2015 Intel Corporation.

 The source code, information and material ("Material") contained herein is owned by Intel Corporation
 or its suppliers or licensors, and title to such Material remains with Intel Corporation or its suppliers
 or licensors. The Material contains proprietary information of Intel or its suppliers and licensors. 
 The Material is protected by worldwide copyright laws and treaty provisions. No part of the Material may 
 be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed or disclosed
 in any way without Intel's prior express written permission. No license under any patent, copyright or 
 other intellectual property rights in the Material is granted to or conferred upon you, either expressly, 
 by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights 
 must be express and approved by Intel in writing.

 Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any other notice 
 embedded in Materials by Intel or Intel's suppliers or licensors in any way."
 -----------------------------------------------------------------------------------------
'''

import subprocess
import sys
import shutil

def readFile(filename):
    try:
        file=open(filename,"r")
    except IOError:
        print >> sys.stderr, "file " + filename + " could not be opened"
        sys.exit(1)

    return file

def writeFile(filename, content):
    file = open(filename, "w")
    file.write(content)
    file.close()

def run_command(str):
    # use Popen instead of os.system to avoid "command line too long" on Windows
    p = subprocess.Popen("cmd /c " + str)
    p.wait()
    return p.returncode


def copyTreeExt(source, destination):
    if len(source) > 255:
        source = "\\\\?\\" + source
    if len(destination) > 255:
        destination = "\\\\?\\" + destination
    shutil.copytree(source, destination)

def copyFileExt(source, destination):
    if len(source) > 255:
        source = "\\\\?\\" + source
        #debug
        print "long source"
    if len(destination) > 255:
        destination = "\\\\?\\" + destination
        #debug
        print "long dest"
    shutil.copy(source, destination)
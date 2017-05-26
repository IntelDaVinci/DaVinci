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
#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import subprocess
import threading
import random
import string
import win32job
import win32api
import win32con
import platform


class DVCSubprocess(object):
    """docstring for DVC_CALL"""
    def __init__(self):
        self.is_windows8_or_above_flag = self.__is_win8_or_above()
        self.is_cloud_mode_flag = self.__is_cloud_mode()
        self.win32_job_name = ''.join(random.choice(string.ascii_uppercase) for _ in range(6))
        self.__user_job_object_flag = False
        self.timeout_flag = False

    def __is_win8_or_above(self):
        # 6.2 stands for Windows 8 or Windows Server 2012
        # https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
        '''
        return true when the os is windows 8 or higher version
        '''
        return True if float(platform.version()[:3]) >= 6.2 else False

    def __is_cloud_mode(self):
        return False

    def __register_job_object(self):
        if self.is_windows8_or_above_flag or not self.is_cloud_mode_flag:
            self.child = None
            self.hJob = None
            self.hProcess = None
            self.hJob = win32job.CreateJobObject(None, self.win32_job_name)
            extended_info = win32job.QueryInformationJobObject(self.hJob, win32job.JobObjectExtendedLimitInformation)
            extended_info['BasicLimitInformation']['LimitFlags'] = win32job.JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            win32job.SetInformationJobObject(self.hJob, win32job.JobObjectExtendedLimitInformation, extended_info)
            # Convert process id to process handle:
            perms = win32con.PROCESS_TERMINATE | win32con.PROCESS_SET_QUOTA
            self.hProcess = win32api.OpenProcess(perms, False, self.__p.pid)
            try:
                win32job.AssignProcessToJobObject(self.hJob, self.hProcess)
                self.__user_job_object_flag = True
            except Exception as e:
                self.__user_job_object_flag = False
        else:
            self.__user_job_object_flag = False

    def call(self, cmd, shell_flag=False, call_out=None, call_err=None):
        # subprocess Doc points out that set stdout/stderr to subprocess.PIPE can deadlock
        # based on the child process output volume.
        if call_out is None and call_err is None or call_out == subprocess.PIPE or call_err == subprocess.PIPE:
            fnull = open(os.devnull, 'w')
            subprocess.call(cmd, shell=shell_flag, stdout=fnull, stderr=fnull)
            fnull.close()
        else:
            subprocess.call(cmd, shell=shell_flag, stdout=call_out, stderr=call_err)

    def kill(self):
        self.__p.kill()
        if self.__user_job_object_flag:
            try:
                win32job.TerminateJobObject(self.hJob, 1)
            except Exception as e:
                pass

        self.timeout_flag = True

    def get_timeout_status(self):
        return self.timeout_flag
    
    def Popen(self, cmd, timeout=120, shell_flag=False, call_out=subprocess.PIPE, call_err=subprocess.PIPE,
              wait_for_end=True):

        if call_out is None and call_err is None:
            fnull = open(os.devnull, 'w')
            self.__p = subprocess.Popen(cmd, shell=shell_flag, stdout=fnull, stderr=fnull)
            self.__register_job_object()
            if timeout > 0:
                process_timer = threading.Timer(timeout, self.kill)
                process_timer.start()
            self.__p.wait()
            if timeout > 0:
                process_timer.cancel()
            fnull.close()

        else:
            self.__p = subprocess.Popen(cmd, shell=shell_flag, stdout=call_out, stderr=call_err)
            self.__register_job_object()

            if timeout > 0:
                process_timer = threading.Timer(timeout, self.kill)
                process_timer.start()
            out = None
            error = None
            if wait_for_end:
                out, error = self.__p.communicate()
            if timeout > 0:
                process_timer.cancel()
            return [out, error]

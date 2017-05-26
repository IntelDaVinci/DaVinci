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
from abc import abstractmethod, ABCMeta

class TestBase(object):
    """docstring for TestFactory"""
    __metaclass__ = ABCMeta

    def __init__(self, *args, **kwargs):
        self.arg = args
        self.kwargs = kwargs
        self.running_flag = False

    @abstractmethod
    def start(self):
        pass

    @abstractmethod
    def stop(self):
        '''stop the test'''

    @abstractmethod
    def preparation(self):
        '''Environment preparation'''


class TestFactory(object):
    """docstring for TestFactory"""
    def __init__(self, *args, **kwargs):
        self.arg = args
        self.kwargs = kwargs

    def get_tester(self):
        pass

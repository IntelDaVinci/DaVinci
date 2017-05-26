/*
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
*/

#ifndef _H_GETDEVICEINFO_
#define _H_GETDEVICEINFO_

#include "DeviceManager.hpp"

namespace DaVinci
{
    using namespace std;

    class GetDeviceInfo  
    {  
    public:  
        GetDeviceInfo(void);  
        ~GetDeviceInfo(void);  

        static void GetDeviceName(string &strName, bool isRealtime = false);  

        static void GetManufactory(string &strName, bool isRealtime = false);  

        static void GetModel(string &strName, bool isRealtime = false);  

        static void GetOSInfo(double &dbOSVersion, string &strOSName, bool isRealtime = false);  

        static void GetCpuInfo(string &strProcessorName, UINT &dwMaxClockSpeed, bool isRealtime = false);  

        static void GetCpuUsage(long &dwUsage, bool isRealtime = false);  

        static void GetMemInfo(long &dwTotal,long &dwFree, bool isRealtime = false);  
  
        static void GetDiskInfo(long &dwTotal, long &dwFree, bool isRealtime = false);  

        static void GetResolution(UINT &dwWidth, UINT &dwHeight, bool isRealtime = false);  

        static void GetBatteryLevel(long &dwLevel, bool isRealtime = false);  

        static bool GetCurrentDevice(boost::shared_ptr<AndroidTargetDevice> &androidDut);

    private:
        static const int WaitForRunCommand = 30000;
    };  
}
  
#endif  

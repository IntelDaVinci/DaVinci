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

#include "GetDeviceInfo.hpp"  
#include "DeviceManager.hpp"

namespace DaVinci
{
    void GetDeviceInfo::GetDeviceName(string &strName, bool isRealtime) 
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;
    
        if (GetCurrentDevice(androidDut))
            strName = androidDut->GetDeviceName();
    }  

    void GetDeviceInfo::GetManufactory(string &strName, bool isRealtime) 
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;
        
        if (GetCurrentDevice(androidDut))
           strName = androidDut->GetSystemProp("ro.product.manufacturer");
    }  

    void GetDeviceInfo::GetModel(string &strName, bool isRealtime) 
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;
        
        if (GetCurrentDevice(androidDut))
            strName = androidDut->GetModel(isRealtime);
    }  

    void GetDeviceInfo::GetOSInfo(double &dbOSVersion, string &strOSName, bool isRealtime) 
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;
    
        if (GetCurrentDevice(androidDut))
        {
            string value;

            value = androidDut->GetOsVersion(isRealtime);
            TryParse(value, dbOSVersion);

            strOSName = androidDut->GetSystemProp("ro.build.version.incremental");
        }
    }  

    void GetDeviceInfo::GetCpuInfo(string &strProcessorName, UINT &dwMaxClockSpeed, bool isRealtime)  
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;

        if (GetCurrentDevice(androidDut))
            androidDut->GetCpuInfo(strProcessorName,  dwMaxClockSpeed);
    }  

    void GetDeviceInfo::GetCpuUsage(long &dwUsage, bool isRealtime)  
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;

        if (GetCurrentDevice(androidDut))
            dwUsage = androidDut->GetCpuUsage(isRealtime);
    }

    void GetDeviceInfo::GetBatteryLevel(long &dwLevel, bool isRealtime)  
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;

        if (GetCurrentDevice(androidDut))
            androidDut->GetBatteryLevel(dwLevel, isRealtime);
    }  

    void  GetDeviceInfo::GetMemInfo(long &dwTotal,long &dwFree, bool isRealtime)   
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut;

        if (GetCurrentDevice(androidDut))
            androidDut->GetMemoryInfo(dwTotal, dwFree, isRealtime);
    }

    void GetDeviceInfo::GetDiskInfo(long &dwTotal, long &dwFree, bool isRealtime)  
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut;

        if (GetCurrentDevice(androidDut))
            androidDut->GetDiskInfo(dwTotal, dwFree, isRealtime);
    }  

    void GetDeviceInfo::GetResolution(UINT &dwWidth, UINT &dwHeight, bool isRealtime)
    {  
        boost::shared_ptr<AndroidTargetDevice> androidDut;


        if (GetCurrentDevice(androidDut))
        {
            dwWidth = androidDut->GetDeviceWidth();
            dwHeight = androidDut->GetDeviceHeight();
        }
    }

    bool GetDeviceInfo::GetCurrentDevice(boost::shared_ptr<AndroidTargetDevice> &androidDut) 
    {
        androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());

        if (androidDut != nullptr)
            return true;
        else
            return false;
    }
}

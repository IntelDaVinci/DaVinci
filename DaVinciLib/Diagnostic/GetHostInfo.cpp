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

#include "GetHostInfo.hpp"  

namespace DaVinci
{
    void GetHostInfo::GetHostName(string &strName) 
    {  
        TCHAR buffer[256] = TEXT("");
        DWORD dwSize = sizeof(buffer);
        bool result = false;
    
        strName = "";

        result = (GetComputerName(buffer, &dwSize) == TRUE);
        assert(result);
        strName = strName.insert(0, buffer);
    }  

    void GetHostInfo::GetOSInfo(double &dbOSVersion, string &strOSName) 
    {  
       string str;  
       OSVERSIONINFOEX osvi;  
       bool result = false;

       ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));  
       osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);  
       result = (GetVersionEx ((OSVERSIONINFO *) &osvi) == TRUE);
       assert(result);
       
       dbOSVersion = osvi.dwMajorVersion + osvi.dwMinorVersion * 0.1;

       HKEY hKey = 0;  
       TCHAR szOSName[256] = ("Windows 7 Enterprise");
       DWORD dwBufLen = 256*sizeof(wchar_t);  
       TCHAR regkey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"); 

       strOSName = "";
       result = (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS);
       assert(result);

       result = (RegQueryValueEx(hKey, TEXT("ProductName"),  NULL, NULL, (LPBYTE) szOSName, &dwBufLen) == ERROR_SUCCESS);
       assert(result);
       
       strOSName = strOSName.insert(0, szOSName);

       RegCloseKey(hKey);  
//       cout << "OS Version: " << dbOSVersion << endl;
//       cout << "OS Name: " << strOSName << endl;
    }  

    void GetHostInfo::GetCpuInfo(string &strProcessorName, UINT &dwMaxClockSpeed)  
    {  
        bool result = false;
        HKEY hKey = 0;
        INT maxFreq = 0;  
        DWORD dataSize = sizeof(maxFreq);  
        TCHAR name[100];  
        TCHAR regkey[] = TEXT("Hardware\\Description\\System\\CentralProcessor\\0"); 

        strProcessorName = "";
        dwMaxClockSpeed = 0;

        
        result = (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)regkey, 0,KEY_READ,&hKey) == ERROR_SUCCESS);
        assert(result);
        dataSize=sizeof(name);
        ZeroMemory(name,dataSize);
        result = (RegQueryValueEx(hKey,TEXT("ProcessorNameString"),NULL,NULL, (LPBYTE)name,&dataSize) == ERROR_SUCCESS);
        assert(result);
        strProcessorName = name;

        dataSize = sizeof(maxFreq);  
        result = (RegQueryValueEx(hKey, TEXT("~MHz"), NULL, NULL, (LPBYTE)&maxFreq, &dataSize) == ERROR_SUCCESS);
        assert(result);
        dwMaxClockSpeed = maxFreq;
/*
            TCHAR id[100],name[100],vender[100];  
            dataSize=sizeof(id);  
            ZeroMemory(id,dataSize);  
            result = RegQueryValueEx(hKey,TEXT("Identifier"),NULL,NULL,(LPBYTE)id,&dataSize);  

            dataSize=sizeof(vender);  
            ZeroMemory(vender,dataSize);  
            result = RegQueryValueEx(hKey,TEXT("VendorIdentifier"),NULL,NULL,(LPBYTE)vender,&dataSize);  
*/

        RegCloseKey(hKey);  

//        cout << "CPU Name: " << strProcessorName << endl;
//        cout << "CPU Frequence : " << dwMaxClockSpeed << " MHZ" << endl;
//        cout << "CPU Usage: " << dwUsage << "%" << endl;

    }  

    void GetHostInfo::GetCpuUsage(long &dwUsage)  
    {  
        FILETIME last_idleTime, last_kernelTime, last_userTime;
        FILETIME cur_idleTime, cur_kernelTime, cur_userTime;
        ULARGE_INTEGER ul_last_idle, ul_last_kernel, ul_last_user;
        ULARGE_INTEGER ul_cur_idle, ul_cur_kernel, ul_cur_user;
        bool result = false;

        dwUsage = 0;
        result = (GetSystemTimes(&last_idleTime, &last_kernelTime, &last_userTime ) == TRUE);
        assert(result);

        ThreadSleep(100);
        result = (GetSystemTimes(&cur_idleTime, &cur_kernelTime, &cur_userTime ) == TRUE);
        assert(result);

        CopyMemory(&ul_last_idle, &last_idleTime, sizeof(FILETIME)); 
        CopyMemory(&ul_last_kernel, &last_kernelTime, sizeof(FILETIME)); 
        CopyMemory(&ul_last_user, &last_userTime, sizeof(FILETIME)); 
                 
        CopyMemory(&ul_cur_idle  , &cur_idleTime  , sizeof(FILETIME)); 
        CopyMemory(&ul_cur_kernel  , &cur_kernelTime  , sizeof(FILETIME)); 
        CopyMemory(&ul_cur_user  , &cur_userTime  , sizeof(FILETIME)); 

        if (((ul_cur_user.QuadPart - ul_last_user.QuadPart) + (ul_cur_kernel.QuadPart - ul_last_kernel.QuadPart)) != 0)
            dwUsage = static_cast<long>(((ul_cur_user.QuadPart - ul_last_user.QuadPart) + (ul_cur_kernel.QuadPart - ul_last_kernel.QuadPart) - (ul_cur_idle.QuadPart - ul_last_idle.QuadPart)) * 100 / ((ul_cur_user.QuadPart - ul_last_user.QuadPart) + (ul_cur_kernel.QuadPart - ul_last_kernel.QuadPart)));
    }  

    void GetHostInfo::GetMemInfo(long &dwTotal,long &dwFree)   
    {   
        MEMORYSTATUSEX  Mem;   
        bool result = false;
        dwTotal = 0;
        dwFree = 0;

        Mem.dwLength = sizeof (Mem);

        result = (GlobalMemoryStatusEx(&Mem) == TRUE);
        assert(result);
        dwTotal = static_cast<long>(Mem.ullTotalPhys/BIT2MB);   
        dwFree = static_cast<long>(Mem.ullAvailPhys/BIT2MB);  

//        cout << "Memory Total: " << dwTotal << " MB" << endl;
//        cout << "Memory Tree : " << dwFree << " MB" << endl;
    }  
  
    void GetHostInfo::GetDiskInfo(long &dwTotal, long &dwFree)  
    {  
        TCHAR strdriver[10] = TEXT("C:\\");
        ULONGLONG i64FreeBytesToCaller;  
        ULONGLONG i64TotalBytes = 0;  
        ULONGLONG i64FreeBytes = 0;  
        TCHAR currentPath[MAX_PATH] = {0};
        bool result = false;

        dwTotal = 0;
        dwFree = 0;

        result = (GetModuleFileName(NULL, currentPath, MAX_PATH) > 0);
        assert(result);
        CopyMemory(strdriver, currentPath, 3 * sizeof(TCHAR));

        result = (GetDiskFreeSpaceEx (strdriver, (PULARGE_INTEGER)&i64FreeBytesToCaller,  (PULARGE_INTEGER)&i64TotalBytes,  (PULARGE_INTEGER)&i64FreeBytes) == TRUE);
        assert(result);

        dwTotal = static_cast<long>(i64TotalBytes/BIT2MB);
        dwFree = static_cast<long>(i64FreeBytes/BIT2MB);

//        cout << "Disk Total: " << dwTotal <<  " GB" <<endl;
//        cout << "Disk Free : " << dwFree <<  " GB" <<endl;
    }  

    void GetHostInfo::GetResolution(UINT &dwWidth, UINT &dwHeight)
    {  
        RECT screen;
        const HWND hDesktop = GetDesktopWindow();
        bool result = false;

        dwWidth = 0;
        dwHeight = 0;
        screen.right = 0;
        screen.bottom = 0;

        result = (GetWindowRect(hDesktop, &screen) == TRUE);
        assert(result);
        dwWidth = screen.right;
        dwHeight = screen.bottom;
        return;  
    }
}

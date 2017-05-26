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

#include "Diagnostic.hpp"

namespace DaVinci
{
    Diagnostic::Diagnostic(): lastPrintTime(0)
    {
        SetCheckerName("Diagnostic");

        SetPeriod(PerformanceCheckerPeriod); // 30S
        LoadBenchmark();
            
        healthyChecker = boost::shared_ptr<HealthyChecker>(new HealthyChecker());
    }

    Diagnostic::~Diagnostic()
    {
    }

    void Diagnostic::CheckHostInfo()
    {
        GetHostInfo();

        DAVINCI_LOG_INFO << "DiagnosticReportStart" ;
        DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
        DAVINCI_LOG_INFO << "Host System Information:" ;

        DAVINCI_LOG_INFO << "Host Name     : " <<hostInfo.name;
        DAVINCI_LOG_INFO << "OS Version    : " <<hostInfo.osVersion;
        DAVINCI_LOG_INFO << "OS Edition    : " <<hostInfo.osInfo;
        DAVINCI_LOG_INFO << "CPU Info      : " <<hostInfo.cpuInfo;
        DAVINCI_LOG_INFO << "CPU Frequence : " <<hostInfo.cpuFreq <<  " MHZ";
        DAVINCI_LOG_INFO << "CPU Usage     : " <<hostInfo.cpuUsage <<  "%";
        DAVINCI_LOG_INFO << "Disk Total    : " <<hostInfo.diskTotal <<  " MB";
        DAVINCI_LOG_INFO << "Disk Free     : " <<hostInfo.diskFree <<  " MB";
        DAVINCI_LOG_INFO << "Memory Total  : " <<hostInfo.memTotal <<  " MB";
        DAVINCI_LOG_INFO << "Memory Free   : " <<hostInfo.memFree <<  " MB";
//      DAVINCI_LOG_INFO << "Battery Level : " <<hostInfo.batteryLevel << "%";
        DAVINCI_LOG_INFO << "Resolution    : (" <<hostInfo.displayWidth << " X " << hostInfo.displayHeight << ")";

        if (hostInfo.cpuUsage > hostInfoBenchmark.cpuUsage)
        {
            DAVINCI_LOG_WARNING << "Diagnostic: Host CPU Usage, Actual: " <<hostInfo.cpuUsage << "%, Expect: <" << hostInfoBenchmark.cpuUsage << "%";
            IncrementErrorCount();
        }

        if (hostInfo.memFree < hostInfoBenchmark.memFree)
        {
           DAVINCI_LOG_WARNING << "Diagnostic: Host Memory Free, Actual: " <<hostInfo.memFree << " MB, Expect: >" << hostInfoBenchmark.memFree << " MB";
           IncrementErrorCount();
        }

        if (hostInfo.osVersion < hostInfoBenchmark.osVersion)
        {
           DAVINCI_LOG_ERROR << "Diagnostic: Host OS Version, Actual: " <<hostInfo.osVersion << " Expect: >" << hostInfoBenchmark.osVersion;
           IncrementErrorCount();
        }

        if (hostInfo.diskFree < hostInfoBenchmark.diskFree)
        {
           DAVINCI_LOG_WARNING << "Diagnostic: Host Disk Free, Actual: " <<hostInfo.diskFree << " MB, Expect: >" << hostInfoBenchmark.diskFree << " MB";
           IncrementErrorCount();
        }

/*     if (hostInfo.batteryLevel < hostInfoBenchmark.batteryLevel)
           DAVINCI_LOG_WARNING << "Host Battery Level, Actual: " <<hostInfo.batteryLevel << " Expect: >" << hostInfoBenchmark.batteryLevel;
*/
        if (hostInfo.cpuFreq < hostInfoBenchmark.cpuFreq)
        {
           DAVINCI_LOG_ERROR << "Diagnostic: Host CPU Frequence, Actual: " <<hostInfo.cpuFreq << " MHZ, Expect: >" << hostInfoBenchmark.cpuFreq << " MHZ";
           IncrementErrorCount();
        }

        if ((hostInfo.displayHeight * hostInfo.displayWidth) < ( hostInfoBenchmark.displayHeight * hostInfoBenchmark.displayWidth))
        {
           DAVINCI_LOG_ERROR << "Diagnostic: Host Resolution, Actual: (" <<hostInfo.displayWidth << " X " << hostInfo.displayHeight << "), Expect: >(" <<hostInfoBenchmark.displayWidth << " X " << hostInfoBenchmark.displayHeight << ")";
           IncrementErrorCount();
        }
        
        DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
        DAVINCI_LOG_INFO << "DiagnosticReportEnd" ;
    }

    void Diagnostic::CheckDeviceInfo()
    {
        DAVINCI_LOG_INFO << "DiagnosticReportStart" ;
        DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
        DAVINCI_LOG_INFO << "Device System Information:" ;

        boost::shared_ptr<AndroidTargetDevice> androidDut;
        if (GetDeviceInfo::GetCurrentDevice(androidDut))
        {
            GetDeviceInfo();

            DAVINCI_LOG_INFO << "Device Name   : " <<deviceInfo.name;
            DAVINCI_LOG_INFO << "Manufactory   : " <<deviceInfo.manufactory;
            DAVINCI_LOG_INFO << "Model         : " <<deviceInfo.model;
            DAVINCI_LOG_INFO << "OS Version    : " <<deviceInfo.osVersion;
            DAVINCI_LOG_INFO << "OS Edition    : " <<deviceInfo.osInfo;
            DAVINCI_LOG_INFO << "CPU Info      : " <<deviceInfo.cpuInfo;
            DAVINCI_LOG_INFO << "CPU Frequence : " <<deviceInfo.cpuFreq <<  " MHZ";
            DAVINCI_LOG_INFO << "CPU Usage     : " <<deviceInfo.cpuUsage <<  "%";
            DAVINCI_LOG_INFO << "Disk Total    : " <<deviceInfo.diskTotal <<  " MB";
            DAVINCI_LOG_INFO << "Disk Free     : " <<deviceInfo.diskFree <<  " MB";
            DAVINCI_LOG_INFO << "Memory Total  : " <<deviceInfo.memTotal <<  " MB";
            DAVINCI_LOG_INFO << "Memory Free   : " <<deviceInfo.memFree <<  " MB";
            DAVINCI_LOG_INFO << "Battery Level : " <<deviceInfo.batteryLevel << "%";
            DAVINCI_LOG_INFO << "Resolution    : (" <<deviceInfo.displayWidth << " X " << deviceInfo.displayHeight << ")";

            if (deviceInfo.cpuUsage > deviceInfoBenchmark.cpuUsage)
            {
               DAVINCI_LOG_WARNING << "Diagnostic: Device CPU Usage, Actual: " <<deviceInfo.cpuUsage << "%, Expect: <" << deviceInfoBenchmark.cpuUsage << "%";
               IncrementErrorCount();
            }

            if (deviceInfo.memFree < deviceInfoBenchmark.memFree)
            {
               DAVINCI_LOG_WARNING << "Diagnostic: Device Memory Free, Actual: " <<deviceInfo.memFree << " MB, Expect: >" << deviceInfoBenchmark.memFree << " MB";
               IncrementErrorCount();
            }

            if (deviceInfo.batteryLevel < deviceInfoBenchmark.batteryLevel)
            {
               DAVINCI_LOG_WARNING << "Diagnostic: Device Battery Level, Actual: " <<deviceInfo.batteryLevel << "%,  Expect: >" << deviceInfoBenchmark.batteryLevel << "%";
               IncrementErrorCount();
            }

            if (deviceInfo.diskFree < deviceInfoBenchmark.diskFree)
            {
               DAVINCI_LOG_WARNING << "Diagnostic: Device Disk Free, Actual:" <<deviceInfo.diskFree << " MB, Expect: >" << deviceInfoBenchmark.diskFree << " MB";
               IncrementErrorCount();
            }

            if (deviceInfo.osVersion < deviceInfoBenchmark.osVersion)
            {
               DAVINCI_LOG_ERROR << "Diagnostic: Device OS Version, Actual: " <<deviceInfo.osVersion << " Expect: >" << deviceInfoBenchmark.osVersion;
               IncrementErrorCount();
            }

            if (deviceInfo.cpuFreq < deviceInfoBenchmark.cpuFreq)
            {
               DAVINCI_LOG_ERROR << "Diagnostic: Device CPU Frequence, Actual: " <<deviceInfo.cpuFreq << " MHZ, Expect: >" << deviceInfoBenchmark.cpuFreq << " MHZ";
               IncrementErrorCount();
            }

            if ((deviceInfo.displayHeight * deviceInfo.displayWidth) < ( deviceInfoBenchmark.displayHeight * deviceInfoBenchmark.displayWidth))
            {
               DAVINCI_LOG_ERROR << "Diagnostic: Device Resolution, Actual: (" <<deviceInfo.displayWidth << " X " << deviceInfo.displayHeight << "), Expect: >(" <<deviceInfoBenchmark.displayWidth << " X " << deviceInfoBenchmark.displayHeight << ")";
               IncrementErrorCount();
            }
        }
        else
        {
               DAVINCI_LOG_INFO << "Diagnostic: No Target Device connected!";
        }

        DAVINCI_LOG_INFO << "------------------------------------------------------------------" ;
        DAVINCI_LOG_INFO << "DiagnosticReportEnd" ;
    }

    void Diagnostic::SetHostInfoBenchmark(DeviceInfo devInfo)
    {
        hostInfoBenchmark.osVersion = devInfo.osVersion;
        hostInfoBenchmark.cpuFreq = devInfo.cpuFreq;
        hostInfoBenchmark.cpuUsage = devInfo.cpuUsage;
        hostInfoBenchmark.memFree = devInfo.memFree;
        hostInfoBenchmark.diskFree = devInfo.diskFree;
        hostInfoBenchmark.batteryLevel = devInfo.batteryLevel;
        hostInfoBenchmark.displayHeight = devInfo.displayHeight;
        hostInfoBenchmark.displayWidth = devInfo.displayWidth;
    }

    void Diagnostic::SetDeviceInfoBenchmark(DeviceInfo devInfo)
    {
        deviceInfoBenchmark.osVersion = devInfo.osVersion;
        deviceInfoBenchmark.cpuFreq = devInfo.cpuFreq;
        deviceInfoBenchmark.cpuUsage = devInfo.cpuUsage;
        deviceInfoBenchmark.memFree = devInfo.memFree;
        deviceInfoBenchmark.diskFree = devInfo.diskFree;
        deviceInfoBenchmark.batteryLevel = devInfo.batteryLevel;
        deviceInfoBenchmark.displayHeight = devInfo.displayHeight;
        deviceInfoBenchmark.displayWidth = devInfo.displayWidth;
    }

    void Diagnostic::SetDevicePerfBenchmark(DevicePerformance devPerf)
    {
        devicePerfBenchmark.frameRate = devPerf.frameRate;
        devicePerfBenchmark.cpuUsage = devPerf.cpuUsage;
        devicePerfBenchmark.memFree = devPerf.memFree;
        devicePerfBenchmark.diskFree = devPerf.diskFree;
        devicePerfBenchmark.batteryLevel = devPerf.batteryLevel;
    }

    void Diagnostic::SetHostPerfBenchmark(DevicePerformance devPerf)
    {
        hostPerfBenchmark.frameRate = devPerf.frameRate;
        hostPerfBenchmark.cpuUsage = devPerf.cpuUsage;
        hostPerfBenchmark.memFree = devPerf.memFree;
        hostPerfBenchmark.diskFree = devPerf.diskFree;
        hostPerfBenchmark.batteryLevel = devPerf.batteryLevel;
    }

    void Diagnostic::LoadBenchmark()
    {
        DeviceInfo devInfo;
        DevicePerformance devPerf;

        devInfo.osVersion = 5.0;
        devInfo.cpuFreq = 2400;
        devInfo.cpuUsage = 60;
        devInfo.memFree = 2048;
        devInfo.diskFree = 10240;
        devInfo.displayHeight = 800;
        devInfo.displayWidth = 1280;
        SetHostInfoBenchmark(devInfo); 

        devInfo.osVersion = 4.0;
        devInfo.cpuFreq = 100;
        devInfo.cpuUsage = 60;
        devInfo.memFree = 16;
        devInfo.diskFree = 512;
        devInfo.displayHeight = 600;
        devInfo.displayWidth = 1024;
        SetDeviceInfoBenchmark(devInfo); 

        devPerf.frameRate = 60;
        devPerf.cpuUsage = 90;
        devPerf.memFree = 1024;
        devPerf.diskFree = 10240;
        devPerf.batteryLevel = 50;
        SetHostPerfBenchmark(devPerf);

        devPerf.frameRate = 15;
        devPerf.cpuUsage = 90;
        devPerf.memFree = 10;
        devPerf.diskFree = 256;
        devPerf.batteryLevel = 25;
        SetDevicePerfBenchmark(devPerf);
    }

    bool Diagnostic::GetHostInfo()
    {
        hostInfo.batteryLevel = 100;
        hostInfo.cpuFreq = 0;
        hostInfo.cpuUsage = 100;
        hostInfo.diskFree = 0;
        hostInfo.memFree = 0;
        hostInfo.osVersion = 5.0;
        hostInfo.osInfo = "Windows 7 Enterprise";
        hostInfo.displayHeight = 800;
        hostInfo.displayWidth = 1280;

        GetHostInfo::GetHostName(hostInfo.name);
        GetHostInfo::GetOSInfo(hostInfo.osVersion, hostInfo.osInfo);
        GetHostInfo::GetCpuInfo(hostInfo.cpuInfo, hostInfo.cpuFreq);
        GetHostInfo::GetCpuUsage(hostInfo.cpuUsage);
        GetHostInfo::GetDiskInfo(hostInfo.diskTotal, hostInfo.diskFree);
        GetHostInfo::GetMemInfo(hostInfo.memTotal, hostInfo.memFree);
        GetHostInfo::GetResolution(hostInfo.displayWidth, hostInfo.displayHeight);

        hostPerf.batteryLevel = hostInfo.batteryLevel;
        hostPerf.cpuUsage = hostInfo.cpuUsage;
        hostPerf.diskFree = hostInfo.diskFree;
        hostPerf.memFree = hostInfo.memFree;

        return true;
    }

    bool Diagnostic::GetDeviceInfo()
    {
        deviceInfo.batteryLevel = 50;
        deviceInfo.cpuFreq = 1000;
        deviceInfo.cpuUsage = 100;
        deviceInfo.diskFree = 0;
        deviceInfo.memFree = 0;
        deviceInfo.osVersion = 4.0;

        GetDeviceInfo::GetDeviceName(deviceInfo.name, true);
        GetDeviceInfo::GetManufactory(deviceInfo.manufactory, true);
        GetDeviceInfo::GetModel(deviceInfo.model, true);
        GetDeviceInfo::GetOSInfo(deviceInfo.osVersion, deviceInfo.osInfo, true);
        GetDeviceInfo::GetCpuInfo(deviceInfo.cpuInfo, deviceInfo.cpuFreq, true);
        GetDeviceInfo::GetCpuUsage(deviceInfo.cpuUsage, true);
        GetDeviceInfo::GetDiskInfo(deviceInfo.diskTotal, deviceInfo.diskFree, true);
        GetDeviceInfo::GetMemInfo(deviceInfo.memTotal, deviceInfo.memFree, true);
        GetDeviceInfo::GetResolution(deviceInfo.displayWidth, deviceInfo.displayHeight, true);
        GetDeviceInfo::GetBatteryLevel(deviceInfo.batteryLevel, true);

        devicePerf.batteryLevel = deviceInfo.batteryLevel;
        devicePerf.cpuUsage = deviceInfo.cpuUsage;
        devicePerf.diskFree = deviceInfo.diskFree;
        devicePerf.memFree = deviceInfo.memFree;

        return true;
    }

    void Diagnostic::CheckSystemPerf()
    {
        GetHostInfo::GetDiskInfo(hostPerf.diskTotal, hostPerf.diskFree);
        GetHostInfo::GetCpuUsage(hostPerf.cpuUsage);
        GetHostInfo::GetMemInfo(hostPerf.memTotal, hostPerf.memFree);

        currentTime = GetCurrentMillisecond();  

        /*------------------------------------------------------------*/
        /* For Host Device                                          
        /*------------------------------------------------------------*/
        if (hostPerf.cpuUsage > hostPerfBenchmark.cpuUsage)
            DAVINCI_LOG_WARNING << "Performance: Host CPU Usage, Actual: " <<hostPerf.cpuUsage << "%, Expect: < " << hostPerfBenchmark.cpuUsage << "%";

        if (hostPerf.memFree < hostPerfBenchmark.memFree)
            DAVINCI_LOG_WARNING << "Performance: Host Memory Free, Actual: " <<hostPerf.memFree << " MB, Expect: > " << hostPerfBenchmark.memFree << " MB";
        
        /*------------------------------------------------------------*/
        /* For Target Device                                          
        /*------------------------------------------------------------*/
        boost::shared_ptr<AndroidTargetDevice> androidDut;
        if (GetDeviceInfo::GetCurrentDevice(androidDut))
        {
            GetDeviceInfo::GetDiskInfo(devicePerf.diskTotal, devicePerf.diskFree);
            GetDeviceInfo::GetBatteryLevel(devicePerf.batteryLevel);
            GetDeviceInfo::GetCpuUsage(devicePerf.cpuUsage);
            GetDeviceInfo::GetMemInfo(devicePerf.memTotal, devicePerf.memFree);
        
            if (devicePerf.cpuUsage > devicePerfBenchmark.cpuUsage)
            {
                DAVINCI_LOG_WARNING << "Performance: Device CPU Usage, Actual: " <<devicePerf.cpuUsage << "%, Expect: < " << devicePerfBenchmark.cpuUsage << "%";

                auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
                androidDut->AdbShellCommand(" top -n 1 -m 5", outStr, WaitForRunCommand);
                DAVINCI_LOG_INFO << "CPU Usage Top 5 Applications:";
                DAVINCI_LOG_INFO << outStr->str();
            }

            if (devicePerf.memFree < devicePerfBenchmark.memFree)
                DAVINCI_LOG_WARNING << "Performance: Device Memory Free, Actual: " <<devicePerf.memFree << " MB, Expect: > " << devicePerfBenchmark.memFree << " MB";

            // For avoid too much printing battery/disk warning.
            if ((currentTime - lastPrintTime) > PrintWarningInterval)
            {
                if (hostPerf.diskFree < hostPerfBenchmark.diskFree)
                    DAVINCI_LOG_WARNING << "Performance: Host Disk Free, Actual: " <<hostPerf.diskFree << " MB, Expect: > " << hostPerfBenchmark.diskFree << " MB";

                if (devicePerf.batteryLevel < devicePerfBenchmark.batteryLevel)
                    DAVINCI_LOG_WARNING << "Performance: Device Battery Level, Actual: " <<devicePerf.batteryLevel << "%,  Expect: > " << devicePerfBenchmark.batteryLevel << "%";

                if (devicePerf.diskFree < devicePerfBenchmark.diskFree)
                    DAVINCI_LOG_WARNING << "Performance: Device Disk Free, Actual:" <<devicePerf.diskFree << " MB, Expect: >" << devicePerfBenchmark.diskFree << " MB";

                lastPrintTime = currentTime;
            }
        }
    }

    /// <summary>
    /// Worker thread loop
    /// </summary>
    void Diagnostic::WorkerThreadLoop()
    {
        while (GetRunningFlag())
        {
            triggerEvent->WaitOne(checkInterval); 
 
            if (GetPauseFlag() || (!GetRunningFlag()))
                  continue;
            
            CheckSystemPerf();
        }
    }

    void  Diagnostic::StartPerfMonitor()
    {
        Start();
    }

    void  Diagnostic::StopPerfMonitor()
    {
        Stop();
    }

    void  Diagnostic::LoadCheckList()
    {
        healthyChecker->LoadCheckList();
        healthyChecker->ClearErrorCount();
    }

    void  Diagnostic::RunHostCheckList()
    {
        healthyChecker->RunHostCheckList();

        if (healthyChecker->GetResult() == Fail)
            IncrementErrorCount();
    }

    void  Diagnostic::RunDeviceCheckList()
    {
        healthyChecker->RunDeviceCheckList();

        if (healthyChecker->GetResult() == Fail)
            IncrementErrorCount();
    }

    void  Diagnostic::StopAllCheckList()
    {
        healthyChecker->StopAllTest();;
    }
}

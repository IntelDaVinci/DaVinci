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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __DIAGNOSTIC_HPP__
#define __DIAGNOSTIC_HPP__

#include <string>
#include <memory>
#include <vector>
#include "GetHostInfo.hpp"
#include "GetDeviceInfo.hpp"
#include "HealthyChecker.hpp"
#include "Checker.hpp"

namespace DaVinci
{
    using namespace std;

    static const int MAX_PERFORMANCE_COUNT = 30;

    struct DevicePerformance
    {
        int frameRate;  //30-> 30FPS
        long cpuUsage;    // 60 -> 60%
        long memTotal;   // 35 -> 35%
        long memFree;    // 1050 MB
        long diskUsage;  // 40 -> 40%
        long diskTotal;   // 2038 MB
        long diskFree;   // 2038 MB
        long batteryLevel; // 40 -> 40%
    };

    struct DeviceInfo
    {
        string name;

        string osInfo;      // Windows 7, 64bit
        double osVersion;   // like: 6.1
        string osLanguage;  // Chinese
        string manufactory; // Lenovo
        string model;       // K920

        string cpuInfo; //Intel(R) Core(TM) i7-4770 CPU @3.4GHZ 3.4GHZ
        UINT cpuFreq;  //3400 MHZ
        long cpuUsage;   //35 -> 35%
        long memTotal;   //16298MB
        long memFree;    //7233MB
        long memUsage;   //45% percentage
        long diskTotal;  //20482MB
        long diskFree;   //1125MB
        long batteryLevel;   // percentage for remaining.

        UINT displayHeight;
        UINT displayWidth;
    };

    /// <summary> Class for Diagnostic. </summary>
    class Diagnostic : public Checker
    {
    public:
        Diagnostic();

         ~Diagnostic();

        /// <summary>
        /// Get the host system information
        /// </summary>
        bool GetHostInfo();

        /// <summary>
        /// Get the device system information
        /// </summary>
        bool GetDeviceInfo();

        /// <summary>
        /// Check both host/device's system information
        /// </summary>
        void CheckHostInfo();

        /// <summary>
        /// Check both host/device's system information
        /// </summary>
        void CheckDeviceInfo();

        /// <summary>
        /// Load host/device CheckList from configure file.
        /// </summary>
        void LoadCheckList();

        /// <summary>
        /// Run host command set to checklist
        /// </summary>
        void RunHostCheckList();

        /// <summary>
        /// Run device command set to checklist
        /// </summary>
        void RunDeviceCheckList();

        /// <summary>
        /// Stop running all command set to checklist
        /// </summary>
        void StopAllCheckList();

        /// <summary>
        /// Check both host/device's system information
        /// </summary>
        void CheckSystemPerf();

        /// <summary>
        /// Start performance monitor thread
        /// </summary>
        void StartPerfMonitor();

        /// <summary>
        /// Stop performance monitor thread
        /// </summary>
        void StopPerfMonitor();

        /// <summary>
        /// Set DeviceInfo benchmark
        /// </summary>
        void SetHostInfoBenchmark(DeviceInfo devInfo); 

        /// <summary>
        /// Set DeviceInfo benchmark
        /// </summary>
        void SetDeviceInfoBenchmark(DeviceInfo devInfo); 

        /// <summary>
        /// Set DevicePerf benchmark
        /// </summary>
        void SetHostPerfBenchmark(DevicePerformance devPerf);

        /// <summary>
        /// Set DevicePerf benchmark
        /// </summary>
        void SetDevicePerfBenchmark(DevicePerformance devPerf);

        /// <summary>
        /// Worker thread loop for performance check.
        /// </summary>
        virtual void WorkerThreadLoop() override;

    private:
        static const int PerformanceCheckerPeriod = 30000;
        static const int PrintWarningInterval = 300000; // 300S
        static const int WaitForRunCommand = 30000;

        DeviceInfo hostInfoBenchmark;
        DeviceInfo deviceInfoBenchmark;
        DeviceInfo hostInfo;
        DeviceInfo deviceInfo;

        DevicePerformance hostPerfBenchmark;
        DevicePerformance devicePerfBenchmark;
        DevicePerformance hostPerf;
        DevicePerformance devicePerf;

        double lastPrintTime;
        double currentTime;

        boost::shared_ptr<HealthyChecker> healthyChecker;

        /// <summary>
        /// Load benchmark for both device and host
        /// </summary>
        void LoadBenchmark();
    };
}

#endif
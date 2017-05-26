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

#ifndef __MPM_POWER_MEASURE_HPP__
#define __MPM_POWER_MEASURE_HPP__

#include "PowerMeasure.hpp"

#include "DeviceManager.hpp"
#include "HWAccessoryController.hpp"

namespace bc = boost::chrono;

namespace DaVinci
{
    class MPMPowerMeasure : public PowerMeasure
    {
    public:
        MPMPowerMeasure(boost::shared_ptr<QScript> qs, boost::shared_ptr<TargetDevice> targetDev);
        virtual ~MPMPowerMeasure() {}

        virtual DaVinciStatus Start();
        virtual DaVinciStatus Stop();
        virtual DaVinciStatus CollectResult(void *);
        virtual DaVinciStatus GenerateReport();

    private:
        class ElapsedTime {
        public:
            ElapsedTime(double &elapsedTime) : mElapsedTime(elapsedTime)
            {
                mStopWatch.Start();
            }
            ~ElapsedTime()
            {
                mStopWatch.Stop();
                mElapsedTime = mStopWatch.ElapsedMilliseconds();
            }
        private:
            double    &mElapsedTime;
            StopWatch  mStopWatch;
        };

    private:
        bool GenerateOnDevQS();
        bool IntializeMPMEnv();
        bool StartupReplayOnDev();
        bool ParseRawReport(std::string rawReportPath);
        bool ReadZipEntry(
            const std::string rawReportPath,
            const std::string entryName,
            std::vector<std::string> &entryItems);

    private:
        boost::shared_ptr<QScript>               mQS;
        boost::shared_ptr<HWAccessoryController> mHWAcc;
        boost::shared_ptr<AndroidTargetDevice>   mAndroidDev;

        std::string mGeneratedQS;
        std::string mQSPath;
        std::string mDevName;
        std::string mSrcPWTBegSHPath;
        std::string mSrcPWTEndSHPath;
        std::string mDstPWTBegSHPath;
        std::string mDstPWTEndSHPath;
        double      mStartActionElapsedTime;
        bool        mDeviceConnected;
        bool        mCollectedReport;
        std::string mRawReportName;

        std::map<std::string, std::string> mCaseInfo;
        std::map<std::string, std::string> mBatteryEnergy;
        std::map<std::string, std::string> mDeviceInfo;

        const std::string MPM_SESSIONS_PATH;
        const std::string MPM_PACKAGE_NAME;
        const double      HW_LIGHT_TIME;
    };
}

#endif

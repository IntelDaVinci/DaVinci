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

#ifndef __ON_DEVICE_SCRIPT_RECORDER_HPP__
#define __ON_DEVICE_SCRIPT_RECORDER_HPP__

#include "AndroidTargetDevice.hpp"
#include "QScript.hpp"
#include "TestInterface.hpp"

#include "boost/process.hpp"
#include "boost/asio.hpp"
#include "boost/thread/thread.hpp"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

namespace DaVinci
{
    using namespace std;

    class OnDeviceScriptRecorder : public TestInterface
    {
    public:
        static DaVinciStatus PullApk(
            boost::shared_ptr<AndroidTargetDevice> androidDut,
            std::string packageName,
            std::string version,
            boost::filesystem::wpath copyDstPath);

        OnDeviceScriptRecorder();

        ~OnDeviceScriptRecorder()
        {
            DAVINCI_LOG_INFO << "On-device got to exit ";
        }

        virtual bool Init() override;
        virtual void Destroy() override;

        static void CalibrateClickAction(boost::shared_ptr<QScript> qs);
        static void CalibrateIDAction(boost::shared_ptr<QScript> qs, boost::filesystem::path qsPath);
        static string OnDeviceScriptRecorder::GetLayoutFile(const boost::shared_ptr<QSEventAndAction> &it, const boost::filesystem::path &xmlFolder, std::unordered_map<string, string> &keyValueMap);

        static void GenerateXMLConfiguration(string qsName, string xmlPath); 

    private:
        static const unsigned short                DefaultRnRServerPort = 10000;

        boost::asio::io_service                    ios;
        boost::shared_ptr<tcp::socket>             sock;
        boost::shared_ptr<AndroidTargetDevice>     dut;
        ScreenSource                               originalScreenSource;
        boost::shared_ptr<boost::thread>           recordThread;
        boost::shared_ptr<boost::process::process> record;
        std::string                                app;
        std::string                                package;
        std::string                                activity;
        std::string                                version;

        void InstallAllAPKs();
        void Service();
        DaVinciStatus CollectRnRScripts();
        DaVinciStatus ReadCommand(string &command);
        DaVinciStatus WriteCommand(const string &command);
        void PostProcessRnRScript(string packageName, string version, boost::filesystem::wpath wfullDir);
        

        // Because OnDevice recording would generate some touch move actions even user just takes
        // a click action, so we need to remove these touch move actions and calibrate touch down
        // and touch up action. The recognize process bases on the two principles as follows:
        // 1. The time interval between touch down and up should be less than 150ms
        // 2. The move distance between touch down and up should be less than 100

        void KillHSC();

        void Start()
        {
            recordThread = boost::shared_ptr<boost::thread>(
                new boost::thread(boost::bind(&OnDeviceScriptRecorder::Service, this)));
        }

        void Stop()
        {
            assert(recordThread != nullptr);
            assert(dut != nullptr);

            dut->AdbShellCommand("am force-stop com.DaVinci.rnr");
            recordThread->join();
            recordThread = nullptr;
            sock = nullptr;
        }

    };
}

#endif
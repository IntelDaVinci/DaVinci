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

#include "DaVinciCommon.hpp"
#include "OnDeviceScriptRecorder.hpp"

#include <boost/asio/detail/socket_option.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cmath>

#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include "ScriptReplayer.hpp"

namespace DaVinci
{
    using namespace boost::filesystem;
    using boost::asio::ip::tcp;

    DaVinciStatus OnDeviceScriptRecorder::ReadCommand(string &command)
    {
        boost::asio::streambuf input;
        std::istream is(&input);
        boost::system::error_code error;
        boost::asio::read_until(*sock, input, "\n", error);
        std::getline(is, command);
        return error;
    }

    DaVinciStatus OnDeviceScriptRecorder::WriteCommand(const string &command)
    {
        boost::asio::streambuf output;
        std::ostream os(&output);
        boost::system::error_code error;
        os << command << endl;
        boost::asio::write(*sock, output, error);
        return error;
    }

    void OnDeviceScriptRecorder::Service()
    {
        DaVinciStatus status;
        std::string dummy;
        std::string params;

        while (true)
        {
            status = ReadCommand(params);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Unable to read app parameters from the device: " << status.message();
                break;
            }
            if (!params.empty())
            {
                boost::char_separator<char> separator(":");
                boost::tokenizer<boost::char_separator<char>> tokens(params, separator);

                boost::tokenizer<boost::char_separator<char>>::iterator it = tokens.begin();
                app = *it;

                std::advance(it, 1);
                package = *it;

                std::advance(it, 1);
                activity = *it;

                DAVINCI_LOG_INFO << "Recording test script for " << app << " package name: " << package << " activity name: " << activity;
                string idRecordArg = "";
                if(TestManager::Instance().GetIdRecord())
                    idRecordArg = " --id";
                RunProcessAsync(AndroidTargetDevice::GetAdbPath(),
                    " -s " + dut->GetDeviceName() + " shell /data/local/tmp/camera_less --rnr \"" + params +"\"" + idRecordArg,
                    record);
                Sleep(2000);
                if (dut->FindProcess("camera_less") < 0)
                {
                    DAVINCI_LOG_ERROR << "Unable to start on-device recorder process";
                    break;
                }
                status = ReadCommand(dummy);
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Error during recording: " << status.message();
                    break;
                }

                KillHSC();

                WriteCommand(app);
                DAVINCI_LOG_INFO << "Application " << app << " completes recording. Start pulling scripts...";
                status = CollectRnRScripts();
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Unable to pull RnR scripts: " << status.message();
                    break;
                }
            }
        }
        SetFinished();
    }

    DaVinciStatus OnDeviceScriptRecorder::CollectRnRScripts()
    {
        DaVinciStatus               status;
        boost::system::error_code   error;

        // TODO: The "L" is just for windows platform, so we need abstract it.
        std::wstring currentPath = boost::filesystem::current_path().wstring();
        boost::filesystem::wpath wfullDir(currentPath);
        boost::filesystem::wpath wrnrDir(L"RnR");
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        boost::filesystem::wpath wappDir(conv.from_bytes(package));
        wfullDir /= wrnrDir;
        wfullDir /= wappDir;

        std::string currentTimestamp = boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time());
        wfullDir /= currentTimestamp;

        if (!boost::filesystem::create_directories(wfullDir, error)) {
            DAVINCI_LOG_ERROR << "Cannot create directory.";
            return errc::permission_denied;
        }

        DAVINCI_LOG_INFO << "Saving script using timestamp [" << currentTimestamp << "] into folder " << wfullDir.string();

        path qsPath(wfullDir);
        qsPath /= package + ".qs";
        path qtsPath(wfullDir);
        qtsPath /= package + ".qts";
        path videoPath(wfullDir);
        videoPath /= package + ".h264";
        path tracePath(wfullDir);
        tracePath /= package + ".trace";
        path treePath(wfullDir);
        string deviceScriptLocation = "/data/local/tmp/" + package + "/";

        dut->AdbCommand("pull " + deviceScriptLocation + package + ".qs " + qsPath.string());
        WriteCommand("bar:10");

        dut->AdbCommand("pull " + deviceScriptLocation + package + ".qts " + qtsPath.string());
        WriteCommand("bar:20");

        dut->AdbCommand("pull " + deviceScriptLocation + package + ".h264 " + videoPath.string());
        WriteCommand("bar:40");

        dut->AdbCommand("pull /data/local/tmp/FA7F8C87-995C-48E3-ADDA-9ED39D3A3E94 " + tracePath.string());
        WriteCommand("bar:50");

        string treeDir = "/data/local/tmp/rnr_tmp";
        dut->AdbShellCommand("rm " + treeDir + "/*");
        dut->AdbShellCommand("rmdir " + treeDir);
        dut->AdbShellCommand("mkdir " + treeDir);
        dut->AdbShellCommand("mv " + deviceScriptLocation + "record_*.tree " + treeDir);
        dut->AdbShellCommand("mv " + deviceScriptLocation + "record_*.window " + treeDir);
        dut->AdbCommand("pull " + treeDir + " " + treePath.string());
        WriteCommand("bar:60");

        string version = dut->GetPackageVersion(package);
        PullApk(dut, package, version, wfullDir);
        WriteCommand("bar:80");

        // cleanup the uploaded script files
        dut->AdbShellCommand("rm " + deviceScriptLocation + package + ".qs");
        dut->AdbShellCommand("rm " + deviceScriptLocation + package + ".qts");
        dut->AdbShellCommand("rm " + deviceScriptLocation + package + ".h264");
        dut->AdbShellCommand("rm " + deviceScriptLocation + "record_*.tree");
        dut->AdbShellCommand("rm " + deviceScriptLocation + "record_*.window");
        WriteCommand("bar:90");

        PostProcessRnRScript(package, version, wfullDir);
        path xmlPath(wfullDir);
        xmlPath /= package + ".xml";
        GenerateXMLConfiguration(package, xmlPath.string());
        WriteCommand("bar:100");

        WriteCommand("uploaded");

        return DaVinciStatusSuccess;
    }

    DaVinciStatus OnDeviceScriptRecorder::PullApk(
        boost::shared_ptr<AndroidTargetDevice> androidDut,
        std::string packageName,
        std::string version,
        boost::filesystem::wpath copyDstPath)
    {
        if (androidDut == nullptr)
        {
            return errc::invalid_argument;
        }
        DaVinciStatus status = errc::no_such_file_or_directory;
        auto stdoutStr = boost::shared_ptr<ostringstream>(new ostringstream());
        androidDut->AdbShellCommand("pm list packages -f", stdoutStr);
        istringstream stdoutStream(stdoutStr->str());
        while (!stdoutStream.eof())
        {
            string line;
            getline(stdoutStream, line);
            boost::algorithm::trim(line);
            // format of "pm list package -f" output:
            // package:<apk location on device>=<package name>
            if (boost::algorithm::ends_with(line, "=" + packageName) &&
                boost::algorithm::starts_with(line, "package:"))
            {
                size_t sizeOfPackageHdr = string("package:").size();
                string apkLocation = line.substr(sizeOfPackageHdr, line.size() - sizeOfPackageHdr - packageName.size() - 1);
                path localApkPath(copyDstPath);
                localApkPath /= packageName + "_" + version + ".apk";
                status = androidDut->AdbCommand("pull " + apkLocation + " \"" + localApkPath.string() + "\"");
                DAVINCI_LOG_INFO << "Pull APK from " << apkLocation << " to " << localApkPath.string() << ": " << status.message();
                break;
            }
        }
        if (status == errc::no_such_file_or_directory)
        {
            DAVINCI_LOG_WARNING << "Unable to find installed package " << packageName << " from device " << androidDut->GetDeviceName();
        }
        return status;
    }

    OnDeviceScriptRecorder::OnDeviceScriptRecorder() : originalScreenSource(ScreenSource::Undefined)
    {
        SetName("OnDeviceScriptRecorder");
    }

    bool OnDeviceScriptRecorder::Init()
    {
        DaVinciStatus status;
        std::string arch;

        dut = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
        if (dut == nullptr || dut->GetDeviceStatus() == DeviceConnectionStatus::Offline)
        {
            DAVINCI_LOG_ERROR << "Unable to find the target device for on-device recording!";
            return false;
        }

        originalScreenSource = dut->GetScreenSource();
        status = DeviceManager::Instance().InitCaptureDevice(ScreenSource::Disabled);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Unable to switch pseudo camera: " << status.message();
            return false;
        }

        InstallAllAPKs();
        dut->UninstallApp("com.DaVinci.rnr");
        DaVinciStatus ret = dut->InstallApp(TestManager::Instance().GetDaVinciResourcePath("libs/RnR/RnR.apk"));
        if (!DaVinciSuccess(ret))
        {
            DAVINCI_LOG_ERROR << "Cannot install RnR APP to device";
            return false;
        }

        // connect to device by tcp
        unsigned short port = GetAvailableLocalPort(DefaultRnRServerPort);
        dut->AdbCommand("forward --remove tcp:" + boost::lexical_cast<string>(port));
        dut->AdbCommand("forward tcp:" + boost::lexical_cast<string>(port) + " tcp:9999");

        // TODO: The Sleep function will be replaced by the feature of checking APP status
        dut->AdbShellCommand("am force-stop com.DaVinci.rnr");
        Sleep(2000);
        dut->AdbShellCommand("am start -n com.DaVinci.rnr/com.DaVinci.rnr.RnR");
        Sleep(2000);

        sock = boost::shared_ptr<tcp::socket>(new boost::asio::ip::tcp::socket(ios));
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port);
        boost::system::error_code error;
        sock->connect(endpoint, error);
        if (error != 0)
        {
            DAVINCI_LOG_ERROR << "Unable to establish socket connection with device: " << error.message();
            return false;
        }

        status = ReadCommand(arch);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Unable to get device arch: " << status.message();
            return false;
        }
        DAVINCI_LOG_INFO << "target device, cpu:" << arch;
        path agent(TestManager::Instance().GetDaVinciResourcePath("CameralessAgent/Android"));
        if (arch.find("x86") != string::npos)
        {
            agent /= "x86";
        }
        else
        {
            agent /= "arm";
        }

        int apiLevel = 19; // 4.4 by default
        TryParse(arch.substr(arch.find(":")+1), apiLevel);
        agent /= AndroidTargetDevice::ApiLevelToVersion(apiLevel);
        agent /= "camera_less";
        KillHSC();
        dut->AdbPushCommand(agent.string(), "/data/local/tmp/camera_less");
        dut->AdbShellCommand("chmod 777 /data/local/tmp/camera_less");

        WriteCommand("ready");
        Start();
        return true;
    }

    void OnDeviceScriptRecorder::KillHSC()
    {
        dut->KillProcess("camera_less", 3);

        const int MAX_LOOP_CNT = 4;
        int loop_cnt = 0;
        while (loop_cnt < MAX_LOOP_CNT)
        {
            if (dut->FindProcess("camera_less") < 0)
            {
                break;
            }
            else
            {
                Sleep(1000);
                loop_cnt++;
            }
        }
        if (loop_cnt == MAX_LOOP_CNT)
            DAVINCI_LOG_WARNING << "Cannot stop HyperSoftCam";
    }

    void OnDeviceScriptRecorder::InstallAllAPKs()
    {
        assert(dut != NULL);

        string strAPKsPlacedPath = TestManager::Instance().GetApksPath();
        boost::filesystem::wpath apksPlacedPath(strAPKsPlacedPath);
        if (boost::filesystem::is_directory(apksPlacedPath))
        {
            boost::filesystem::directory_iterator item(apksPlacedPath);
            boost::filesystem::directory_iterator endItem;
            for (item; item != endItem; ++item)
            {
                if (item->path().extension() == ".apk")
                {
                    vector<string> pkgsActivity = AndroidTargetDevice::GetPackageActivity(item->path().string());
                    assert(!pkgsActivity.empty());
                    if (!pkgsActivity[0].empty())
                    {
                        dut->UninstallApp(pkgsActivity[0]);
                        dut->InstallApp(item->path().string());
                    }
                    else
                    {
                        DAVINCI_LOG_ERROR << "Cannot get APK(" << item->path().string() << ") information";
                    }
                }
            }
        }
    }

    void OnDeviceScriptRecorder::CalibrateClickAction(boost::shared_ptr<QScript> qs)
    {
        vector<boost::shared_ptr<QSEventAndAction>> allActions = qs->GetAllQSEventsAndActions();

        //adjust timestamp
        vector<boost::shared_ptr<QSEventAndAction>>::iterator touchDown;
        vector<boost::shared_ptr<QSEventAndAction>>::iterator touchUp;
        for (auto it = allActions.begin(); it != allActions.end(); it++) {
            if ((*it)->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN ||
                (*it)->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL) {
                    touchDown = it;
            }

            if ((*it)->Opcode() == QSEventAndAction::OPCODE_TOUCHUP ||
                (*it)->Opcode() == QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL) {
                    touchUp = it;

                    bool ret = TargetDevice::IsClickAction((*touchDown)->IntOperand(0),
                        (*touchDown)->IntOperand(1),
                        (*touchDown)->TimeStamp(),
                        (*touchUp)->IntOperand(0),
                        (*touchUp)->IntOperand(1),
                        (*touchUp)->TimeStamp());
                    if (ret)
                    {
                        auto nextAction = touchDown + 1;

                        if (nextAction != touchUp)
                        {
                            (*touchDown)->SetIntOperand(0, (*touchUp)->IntOperand(0));
                            (*touchDown)->SetIntOperand(1, (*touchUp)->IntOperand(1));

                            for (; nextAction != touchUp; nextAction++)
                                qs->RemoveEventAndAction((*nextAction)->LineNumber());
                        }
                    }
            }
        }

        allActions.clear();
        allActions = qs->GetAllQSEventsAndActions();
        std::map<int, boost::shared_ptr<QSEventAndAction>> touchDownActions;

        for(int i = 0; i < (int)(allActions.size()) - 1; i++)
        {
            if (allActions[i]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN)
            {
                int key = i;
                boost::shared_ptr<QSEventAndAction> value = allActions[i];
                touchDownActions[key] = value;
            }
        }

        bool isFinished = false;
        do
        {
            isFinished = true;
            for(auto e=touchDownActions.begin(); e!=touchDownActions.end(); e++)
            {
                if(abs(allActions[e->first+1]->TimeStamp() - e->second->TimeStamp()) < 10)
                {
                    qs->AdjustTimeStamp(e->first);
                    isFinished = false;
                    continue;
                }
            }
        }
        while(!isFinished);
        DAVINCI_LOG_INFO << "Complete click calibration.";
    }

    void OnDeviceScriptRecorder::CalibrateIDAction(boost::shared_ptr<QScript> qs, boost::filesystem::path treePath)
    {
        if(qs == nullptr)
            return;

        // Remove ID at the background window or keyboard window
        string recordDeviceWidth = qs->GetResolutionWidth();
        string recordDeviceHeight = qs->GetResolutionHeight();
        Size deviceSize;
        bool isParsed = TryParse(recordDeviceWidth, deviceSize.width);
        assert(isParsed);
        isParsed = TryParse(recordDeviceHeight, deviceSize.height);
        assert(isParsed);
        boost::shared_ptr<ViewHierarchyParser> viewHierarchyParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());

        vector<boost::shared_ptr<QSEventAndAction>> allActions = qs->GetAllQSEventsAndActions();
        for(int i = 0; i < (int)(allActions.size()) - 1; i++)
        {
            if (allActions[i]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN || allActions[i]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL)
            {
                string treeFile1;
                string windowFile1;
                std::unordered_map<string, string> keyValueMap1;
                vector<string> recordLines1;
                boost::shared_ptr<ViewHierarchyTree> root1;
                vector<boost::shared_ptr<ViewHierarchyTree>> nodes1;
                std::vector<std::string> windowLines1;

                treeFile1 = GetLayoutFile(allActions[i], treePath, keyValueMap1);
                if(boost::empty(treeFile1))
                    continue;

                ReadAllLines(treeFile1, recordLines1);
                root1 = viewHierarchyParser->ParseTree(recordLines1);
                viewHierarchyParser->CleanTreeControls();
                viewHierarchyParser->TraverseTreeNodes(root1,nodes1);

                boost::replace_all(treeFile1, ".tree", ".window");
                windowFile1 = treeFile1;
                ReadAllLines(windowFile1, windowLines1);

                int x = allActions[i]->IntOperand(0);
                int y = allActions[i]->IntOperand(1);

                Point clickPoint = Point(x * deviceSize.width / maxWidthHeight, y * deviceSize.height / maxWidthHeight);
                Rect sbRect, nbRect, fwRect;
                bool hasKeyboard;
                ScriptHelper::ProcessWindowInfo(keyValueMap1, sbRect, nbRect, hasKeyboard, fwRect);
                if(hasKeyboard)
                {
                    Rect kbROIOnDevice = ObjectUtil::Instance().GetKeyboardROI(allActions[i]->GetOrientation(), deviceSize, nbRect, sbRect, fwRect);
                    if(kbROIOnDevice.contains(clickPoint))
                    {
                        qs->AdjustID(keyValueMap1, i);
                    }
                }

                for(int j = i + 1; j < (int)(allActions.size()); j++)
                {
                    if (allActions[j]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN || allActions[j]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL)
                    {
                        string treeFile2;
                        string windowFile2;
                        std::unordered_map<string, string> keyValueMap2;
                        vector<string> recordLines2;
                        boost::shared_ptr<ViewHierarchyTree> root2;
                        vector<boost::shared_ptr<ViewHierarchyTree>> nodes2;
                        std::vector<std::string> windowLines2;

                        bool isRootChanged, isWindowChanged;
                        treeFile2 = GetLayoutFile(allActions[j], treePath, keyValueMap2);

                        if(boost::empty(treeFile2))
                        {
                            continue;
                        }

                        ReadAllLines(treeFile2, recordLines2);
                        root2 = viewHierarchyParser->ParseTree(recordLines2);
                        viewHierarchyParser->CleanTreeControls();
                        viewHierarchyParser->TraverseTreeNodes(root2,nodes2);

                        isRootChanged = viewHierarchyParser->IsRootChanged(nodes1,nodes2);

                        boost::replace_all(treeFile2, ".tree", ".window");
                        windowFile2 = treeFile2;
                        ReadAllLines(windowFile2, windowLines2);

                        isWindowChanged = AndroidTargetDevice::IsWindowChanged(windowLines1, windowLines2);

                        if(!isRootChanged && isWindowChanged)
                        {
                            qs->AdjustID(keyValueMap2, j);
                        }

                        i = j - 1;
                        break;
                    }
                }
            }
        }

        // Remove ID from swipe action
        for(int i = 0; i < (int)(allActions.size()); i++)
        {
            if (allActions[i]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN || allActions[i]->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL)
            {
                // Adapt IsSwipeAction (qs->EventAndAction(ip): eventActions[ip - 1])
                if(ScriptReplayer::IsSwipeAction(qs, i + 1))
                {
                    string treeFile;
                    std::unordered_map<string, string> keyValueMap;

                    string treeFile2 = GetLayoutFile(allActions[i], treePath, keyValueMap);

                    if(boost::empty(treeFile2))
                    {
                        continue;
                    }
                    qs->AdjustID(keyValueMap, i);
                }
            }
        }

        DAVINCI_LOG_INFO << "Complete ID calibration.";
    }

    string OnDeviceScriptRecorder::GetLayoutFile(const boost::shared_ptr<QSEventAndAction> &it, const boost::filesystem::path &qsPath, std::unordered_map<string, string> &keyValueMap)
    {
        string treeFile;
        string keyValue;

        keyValue = it->StrOperand(0);
        keyValueMap = ScriptReplayer::ParseKeyValues(keyValue);
        string layoutKey = "layout";
        if(keyValueMap.find(layoutKey) != keyValueMap.end())
        {
            boost::filesystem::path layoutPath = qsPath / keyValueMap[layoutKey];
            if(boost::filesystem::exists(layoutPath))
            {                   
                treeFile = layoutPath.string();
            }
        }
        return treeFile;
    }

    void OnDeviceScriptRecorder::GenerateXMLConfiguration(string packageName, string xmlPath)
    {
        std::ofstream xmlStream(xmlPath, std::ios_base::app | std::ios_base::out);
        xmlStream << "" << std::flush;
        xmlStream << "<DaVinci>\n";
        xmlStream << "<Seed value=\"1\" />\n";
        xmlStream << "<Timeout value=\"150\" />\n";
        xmlStream << "<VideoRecording value=\"true\" />\n";
        xmlStream << "<AudioSaving value=\"true\" />\n";
        xmlStream << "<MatchPattern value=\"Stretch\" />\n";
        xmlStream << "<Test mode=\"OBJECT\" silent=\"NO\" type=\"EXPLORATORY\">\n";
        xmlStream << "<Action type=\"CLICK\" value=\"80%\">\n";
        xmlStream << "<Click type=\"PLAIN\" value=\"100%\" />\n";
        xmlStream << "</Action>\n";
        xmlStream << "<Action type=\"SWIPE\" value=\"20%\" />\n";
        xmlStream << "</Test>\n";
        xmlStream << "<Checkers>\n";
        xmlStream << "<Checker name=\"VideoBlankPage\" enable=\"True\" />\n";
        xmlStream << "<Checker name=\"VideoFlickering\" enable=\"True\" />\n";
        xmlStream << "<Checker name=\"Reference\" enable=\"True\" />\n";
        xmlStream << "</Checkers>\n";
        xmlStream << "<Scripts>\n";
        xmlStream << "<Script name=\"" << packageName  << ".qs\" />\n";
        xmlStream << "</Scripts>\n";
        xmlStream << "</DaVinci>\n";
        xmlStream.flush();
        xmlStream.close();
    }

    void OnDeviceScriptRecorder::PostProcessRnRScript(string packageName, string version, boost::filesystem::wpath wfullDir)
    {
        path qsPath(wfullDir);
        qsPath /= packageName + ".qs";
        boost::shared_ptr<QScript> qs = QScript::Load(qsPath.string());
        if (qs == nullptr)
        {
            DAVINCI_LOG_ERROR << "Invalid QScript";
            return;
        }

        qs->SetConfiguration(QScript::ConfigStatusBarHeight, boost::lexical_cast<string>(dut->GetStatusBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetStatusBarHeight()[1]));
        qs->SetConfiguration(QScript::ConfigNavigationBarHeight, boost::lexical_cast<string>(dut->GetNavigationBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetNavigationBarHeight()[1]));
        qs->SetConfiguration(QScript::ConfigApkName, packageName + "_" + version + ".apk");
        qs->SetConfiguration(QScript::ConfigOSversion, dut->GetOsVersion(true));
        qs->SetConfiguration(QScript::ConfigDPI, boost::lexical_cast<string>(dut->GetDPI()));
        qs->SetConfiguration(QScript::ConfigResolution, boost::lexical_cast<string>(dut->GetDeviceWidth()) + "x" + boost::lexical_cast<string>(dut->GetDeviceHeight()));

        CalibrateClickAction(qs);
        path qsFolder(wfullDir);
        CalibrateIDAction(qs, qsFolder);
        qs->Flush();
    }

    void OnDeviceScriptRecorder::Destroy()
    {
        DAVINCI_LOG_INFO << "On-device got to be destroyed ";

        if (recordThread != nullptr)
        {
            Stop();
        }
        if (originalScreenSource != ScreenSource::Undefined)
        {
            assert(dut != nullptr);
            dut->SetScreenSource(originalScreenSource);
        }
        dut = nullptr;
    }
}

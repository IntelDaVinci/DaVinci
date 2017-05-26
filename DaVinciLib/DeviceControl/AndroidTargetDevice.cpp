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
#include "TestManager.hpp"
#include "AndroidTargetDevice.hpp"
#include "InstallConfirmRecognizer.hpp"
#include "StopWatch.hpp"
#include "HyperSoftCamCapture.hpp"
#include "KeywordConfigReader.hpp"
#include "License.hpp"

#include <windows.h> 
#include <tlhelp32.h> 

using namespace std;
using namespace boost::filesystem;
using namespace boost::process; 
using boost::asio::ip::tcp;

namespace DaVinci
{
    const string defaultApplistConfig = "DaVinci_Applist.xml";

    AndroidTargetDevice::AndroidTargetDevice(const string &name, bool isIpAddress) : TargetDevice(name, isIpAddress),
        currentOrientation(Orientation::Unknown), defaultOrientation(Orientation::Unknown),
        isDisplayLotus(false), isMAgentConnected(false), isQAgentConnected(false), isFirstTimeConnected(true),
        isAutoReconnect(true), qagentPort(0), magentPort(0), hyperSoftCamPort(0), model("unknown"), 
        osVersion("4.4"), sdkVersion("19"), Language("en"),isAutoPerfUpdated(false),
        socketOfMAgent(ioService), socketOfQAgent(ioService), heartBeatCount(0), reduceTouchMoveFrequency(true),
        isSwipeAction(false)
    {
        InitInstallationConfirmPackages();
        InitInstallationConfirmManufacturers();
        InitKeyboardSpecialCharacters();
        InitIncompatiblePackages();
        InitInstallNoReturnModels();
        InitSpecialActivity();
        lastQAgentDataTime = boost::posix_time::microsec_clock::local_time();

        if (deviceName.empty())
        {
            DAVINCI_LOG_FATAL << "Could not construct AndroidTargetDevice if the target device name is empty()!";
            throw DaVinciError(DaVinciStatus(errc::no_such_device));
        }

        connectThreadReadyEvent.Reset();
        connectThreadExitEvent.Reset();
        requestConnectEvent.Reset();
        completeConnectEvent.Reset();

        QAgentReceiveQueueEvent.Reset();
        QAgentReceiveThreadReadyEvent.Reset();
        QAgentReceiveThreadExitEvent.Reset();

        barHeight.portraitStatus = 0;
        barHeight.landscapeStatus = 0;
        barHeight.portraitNavigation = 0;
        barHeight.landscapeNavigation = 0;
    }

    void AndroidTargetDevice::InitSpecialActivity()
    {
        std::string fullAppConfig = TestManager::Instance().GetDaVinciResourcePath(defaultApplistConfig);
        std::map<std::string, std::vector<std::string>> configApplistMap;

        if (fullAppConfig != "" && boost::filesystem::is_regular_file(fullAppConfig))
        {
            boost::shared_ptr<KeywordConfigReader> configReader = boost::shared_ptr<KeywordConfigReader>(new KeywordConfigReader());
            configApplistMap = configReader->ReadApplistConfig(fullAppConfig);
        }

        map<string, vector<string>>::const_iterator map_it = configApplistMap.begin();

        while (map_it != configApplistMap.end())
        {
            string map_key = map_it->first;
            vector<string> map_value = map_it->second;
            ++map_it;

            // specialPackageActivityName contains all the special full activity names, key: package name, value: full activity name
            if (map_key == "SpecialActivityName")
            {
                for (string activityName : map_value)
                {
                    if(boost::contains(activityName, "/"))
                    {
                        specialPackageActivityName[activityName.substr(0, activityName.find("/"))] = activityName;
                    }
                }
            }
        }
    }

    void AndroidTargetDevice::InitIncompatiblePackages()
    {
        incompatiblePackages.push_back("cn.goapk.market");
    }

    void AndroidTargetDevice::InitInstallNoReturnModels()
    {
        installNoReturnModels.push_back("Lenovo YT3-X90F");
    }

    void AndroidTargetDevice::InitInstallationConfirmPackages()
    {
        installConfirmPackages.push_back("com.lenovo.safecenter");
        installConfirmPackages.push_back("com.lenovo.security");
        installConfirmPackages.push_back("com.android.vending");
        installConfirmPackages.push_back("com.android.packageinstaller");
        installConfirmPackages.push_back("com.sec.android.app.capabilitymanager");
    }

    void AndroidTargetDevice::InitInstallationConfirmManufacturers()
    {
        // InstallConfirmPackage : installConfirmManufacturer = 1 : N
        installConfirmManufacturers.push_back("LENOVO");
        installConfirmManufacturers.push_back("LGE");
        installConfirmManufacturers.push_back("ASUS");
        installConfirmManufacturers.push_back("INTEL");
        installConfirmManufacturers.push_back("XIAOMI");
        installConfirmManufacturers.push_back("SAMSUNG");
    }

    bool AndroidTargetDevice::MatchInstallationConfirmManufacturer(string targetStr)
    {
        for(auto str : installConfirmManufacturers)
        {
            if(boost::contains(targetStr, str))
                return true;
        }
        return false;
    }

    void AndroidTargetDevice::InitKeyboardSpecialCharacters()
    {
        // Note: some special characters are not supported in standard Android keycode
        androidSpecialKeycodeList['~'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['`'] = Q_KEYCODE_GRAVE;
        androidSpecialKeycodeList['!'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['@'] = Q_KEYCODE_AT;
        androidSpecialKeycodeList['#'] = Q_KEYCODE_POUND;
        androidSpecialKeycodeList['$'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['%'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['^'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['&'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['*'] = Q_KEYCODE_START;
        androidSpecialKeycodeList['('] = Q_KEYCODE_NUMPAD_LEFT_PAREN;
        androidSpecialKeycodeList[')'] = Q_KEYCODE_NUMPAD_RIGHT_PAREN;
        androidSpecialKeycodeList['-'] = Q_KEYCODE_NUMPAD_SUBTRACT;
        androidSpecialKeycodeList['_'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['+'] = Q_KEYCODE_PLUS;
        androidSpecialKeycodeList['='] = Q_KEYCODE_EQUALS;
        androidSpecialKeycodeList['{'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['}'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['['] = Q_KEYCODE_LEFT_BRACKET;
        androidSpecialKeycodeList[']'] = Q_KEYCODE_RIGHT_BRACKET;
        androidSpecialKeycodeList['|'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['\\'] = Q_KEYCODE_BACKSLASH;
        androidSpecialKeycodeList[':'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList[';'] = Q_KEYCODE_SEMICOLON;
        androidSpecialKeycodeList['"'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['\''] = Q_KEYCODE_APOSTROPHE;
        androidSpecialKeycodeList['<'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['>'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList[','] = Q_KEYCODE_COMMA;
        androidSpecialKeycodeList['.'] = Q_KEYCODE_PERIOD;
        androidSpecialKeycodeList['?'] = Q_KEYCODE_NOTSUPPORT;
        androidSpecialKeycodeList['/'] = Q_KEYCODE_NUMPAD_DIVIDE;
    }

    DaVinciStatus AndroidTargetDevice::AdbCommandNoDevice(const string &adbArgs, const boost::shared_ptr<ostringstream> &outStr, int timeout)
    {
        return RunProcessSync(GetAdbPath(), " " + adbArgs, outStr, ".", timeout);
    }

    //
    // IMPORTANT NOTICE:
    // To protect the interests of Intel, you should revise this function before releasing
    // DaVinci to external customers, making sure it only recognizes x86 devices with a selected
    // number of ARM reference devices.
    //
    DaVinciStatus AndroidTargetDevice::AdbDevices(vector<string> &deviceNames)
    {
        License davinciLic;
        auto foundDevices = boost::shared_ptr<ostringstream>(new ostringstream());
        AndroidTargetDevice::AdbCommandNoDevice("devices", foundDevices);

        istringstream iss(foundDevices->str());
        while (!iss.eof())
        {
            string line; 
            getline(iss, line);
            vector<string> columns;
            boost::algorithm::split(columns, line, boost::algorithm::is_any_of("\t"));

            if (columns.size() > 1 && columns[1].find("device") != string::npos)
            {
                if (!davinciLic.IsValidDevice(columns[0]))
                {
                    DAVINCI_LOG_ERROR <<"Detected unsupported target device: "<< columns[0] << ", please check DaVinci license.";
                    continue;
                }

                deviceNames.push_back(columns[0]);
            }
        }
        return DaVinciStatusSuccess;
    }

    bool AndroidTargetDevice::ConvertApkName(string &apkFullName)
    {
        boost::filesystem::path p(apkFullName);
        boost::filesystem::path apkName = p.filename();

        std::wstring wstrApkName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkName.string());

        bool containNoneASCII = ContainNoneASCIIStr(wstrApkName);
        bool containSpecialChar = ContainSpecialChar(wstrApkName);

        if (containNoneASCII || containSpecialChar)
        {
            std::wstring wstrApkLocation = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkFullName);
            boost::filesystem::path srcPath(wstrApkLocation);
            boost::filesystem::path destPath(TestReport::currentQsDirName + "/test.apk");
            if (boost::filesystem::is_regular_file(srcPath) && boost::filesystem::is_directory(TestReport::currentQsDirName))
            {
                if(boost::filesystem::is_regular_file(destPath))
                    boost::filesystem::remove(destPath); 

                boost::filesystem::copy(srcPath, destPath);

                apkFullName = destPath.string();
            }
        }

        return true;
    }

    vector<string> AndroidTargetDevice::GetPackageActivity(const string &apkLocation)
    {
        auto aaptDumpOutStr = boost::shared_ptr<ostringstream>(new ostringstream());
        auto aaptAllOutStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> aaptDumpLines;
        vector<string> aaptAllLines;
        vector<string> appInfo;
        string apkFullName = apkLocation;

        ConvertApkName(apkFullName);
        RunProcessSync(GetAaptPath(), " dump badging \"" + apkFullName + "\"", aaptDumpOutStr);
        RunProcessSync(GetAaptPath(), " l -a " + apkFullName, aaptAllOutStr);

        ReadAllLinesFromStr(aaptDumpOutStr->str(), aaptDumpLines);
        ReadAllLinesFromStr(aaptAllOutStr->str(), aaptAllLines);

        ParsePackageActivityString(aaptDumpLines, aaptAllLines, appInfo);

        return appInfo;
    }

    string AndroidTargetDevice::ApiLevelToVersion(int apiLevel)
    {
        switch (apiLevel)
        {
        case 23:
            return "6.0";
        case 22:
            return "5.1";
        case 21:
            return "5.0";
        case 19:
            return "4.4";
        case 18:
            return "4.3";
        case 17:
            return "4.2";
        default:
            return "";
        }
    }

    Rect AndroidTargetDevice::GetWindowRectangle(const Point &pt, int width, int height, const Size &deviceSize, const Orientation &barOrientation, int frameWidth)
    {
        Rect rct;
        rct.x = pt.x;
        rct.y = pt.y;
        rct.width = width;
        rct.height = height;

        int shortSize = min(deviceSize.width, deviceSize.height);
        int longSize = max(deviceSize.width, deviceSize.height);

        int frameHeight = (int)(frameWidth * shortSize / (double)(longSize));
        Size frameSz(frameWidth, frameHeight);
        return GetTransformedRectangle(rct, frameSz, deviceSize, barOrientation);
    }

    std::unordered_map<string, string> AndroidTargetDevice::GetWindows(vector<string>& lines, string& focusWindow)
    {
        std::unordered_map<string, string> map;
        string windowKeyStr = "Window #";
        string focusKeyStr = "mCurrentFocus=";
        string popupStr = "PopupWindow";
        string focusWindowKeyStr;
        focusWindow = "";

        for (auto line: lines)
        {
            string trimLine = boost::trim_copy(line);
            if(boost::starts_with(trimLine, windowKeyStr))
            {
                vector<string> items;
                boost::algorithm::split(items, trimLine, boost::algorithm::is_any_of(" "));
                int itemSize = (int)(items.size());
                if(itemSize >= 5)
                {
                    if(itemSize == 5)
                    {
                        string keyStr = items[2];
                        vector<string> subItems;
                        boost::algorithm::split(subItems, keyStr, boost::algorithm::is_any_of("{"));
                        assert(subItems.size() == 2);
                        string key = subItems[1];
                        string valueStr = items[4];
                        valueStr = valueStr.substr(0, valueStr.length() - 2);
                        map[key] = valueStr;
                    }
                    else if(itemSize > 5)
                    {
                        string keyStr = items[2];
                        vector<string> subItems;
                        boost::algorithm::split(subItems, keyStr, boost::algorithm::is_any_of("{"));
                        assert(subItems.size() == 2);
                        string key = subItems[1];

                        //  mCurrentFocus=Window{cead5ba u0 Select input method}
                        string middleValue;
                        for(int i = 4; i < itemSize - 1; i++)
                            middleValue += items[i] + " ";
                        string lastItemValue = items[itemSize - 1];
                        lastItemValue = lastItemValue.substr(0, lastItemValue.length() - 2);
                        map[key] = middleValue + lastItemValue;
                    }
                }
            }
            else
            {
                if(boost::starts_with(trimLine, focusKeyStr))
                {
                    vector<string> items;
                    boost::algorithm::split(items, trimLine, boost::algorithm::is_any_of(" "));
                    if(items.size() >= 3)
                    {
                        string keyStr = items[0];
                        vector<string> subItems;
                        boost::algorithm::split(subItems, keyStr, boost::algorithm::is_any_of("{"));
                        assert(subItems.size() == 2);
                        string itemStr = subItems[1];
                        focusWindowKeyStr = itemStr;
                    }
                }
            }
        }

        if(MapContainKey(focusWindowKeyStr, map))
        {
            focusWindow = map[focusWindowKeyStr];
            if(boost::starts_with(focusWindow, popupStr))
            {
                focusWindow = popupStr;
            }
        }

        return map;
    }

    bool AndroidTargetDevice::IsWindowChanged(vector<string>& window1Lines, vector<string>& window2Lines)
    {
        std::unordered_map<string, string> map1, map2;
        string focusWindow1, focusWindow2;
        map1 = GetWindows(window1Lines, focusWindow1);
        map2 = GetWindows(window2Lines, focusWindow2);

        if(map1.size() != map2.size())
            return true;

        for(auto pair2: map2)
        {
            string value = pair2.second;

            bool valueExist = false;
            for(auto pair1: map1)
            {
                if(pair1.second == value)
                {
                    valueExist = true;
                    break;
                }
            }

            if(!valueExist)
            {
                return true;
            }
        }

        return false;
    }

    Rect AndroidTargetDevice::GetTransformedRectangle(const Rect& rect, const Size& frameSz, const Size& deviceSz, const Orientation &barOrientation)
    {
        int frameWidth = frameSz.width;
        int frameHeight = frameSz.height;
        Rect barLocation2;
        vector<Point2f> inputPoint, outputPoint;
        Mat affine_matrix;
        switch (barOrientation)
        {
        case Orientation::Portrait:
        case Orientation::ReversePortrait:
            {
                const Point2f src_pt[] = {Point2f(0.0F, 0.0F), Point2f(deviceSz.width*1.0F, 0.0F), Point2f(deviceSz.width*1.0F, deviceSz.height*1.0F)};
                const Point2f dst_pt[] = {Point2f(0.0F, frameHeight*1.0F), Point2f(0.0F, 0.0F), Point2f(frameWidth*1.0F, 0.0F)};
                affine_matrix = getAffineTransform(src_pt, dst_pt);
                break;
            }
        case Orientation::Landscape:
        case Orientation::ReverseLandscape:
            {
                const Point2f src_pt[] = {Point2f(0.0F, 0.0F), Point2f(deviceSz.height*1.0F, 0.0F), Point2f(deviceSz.height*1.0F, deviceSz.width*1.0F)};
                const Point2f dst_pt[] = {Point2f(0.0F, 0.0F), Point2f(frameWidth*1.0F, 0.0F), Point2f(frameWidth*1.0F, frameHeight*1.0F)};
                affine_matrix = getAffineTransform(src_pt, dst_pt);
                break;
            }
        default:
            DAVINCI_LOG_ERROR << string("Failed to get default orientation data from device.");
            return barLocation2;
        }

        inputPoint.push_back(Point2f(rect.x*1.0F, rect.y*1.0F));
        inputPoint.push_back(Point2f(rect.x*1.0F + rect.width, rect.y*1.0F + rect.height));
        transform(inputPoint, outputPoint, affine_matrix);
        barLocation2 = Rect(outputPoint[0], outputPoint[1]);
        if (barOrientation == Orientation::ReversePortrait
            ||barOrientation == Orientation::ReverseLandscape)
        {
            Point p1, p2;
            p1.x = frameWidth - barLocation2.x;
            p1.y = frameHeight - barLocation2.y;
            p2.x = p1.x - barLocation2.width;
            p2.y = p1.y - barLocation2.height;
            barLocation2 = Rect(p1, p2);
        }
        return barLocation2;
    }

    DaVinciStatus AndroidTargetDevice::Connect()
    {
        DaVinciStatus status;

        if (IsAgentConnected())
            return status;

        isAutoReconnect = true;
        if (connectThreadHandle == nullptr)
        {
            connectThreadHandle = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&AndroidTargetDevice::ConnectThreadEntry, this)));
            SetThreadName(connectThreadHandle->get_id(), "Agent connect");
            connectThreadReadyEvent.WaitOne(WaitForThreadReady);
        }

        requestConnectEvent.Set();

        if (!IsAgentConnected())
            completeConnectEvent.WaitOne(WaitForAgentsConnectionReady); // At most wait 60S for connection, then continue;

        if (!IsAgentConnected())
        {
            DAVINCI_LOG_WARNING << "Failed to connect android target device: " << GetDeviceName() << ", refer to use Diagnostic.exe to get more detail information.";
            status = errc::not_connected;
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::Disconnect()
    {
        boost::system::error_code error;

        DAVINCI_LOG_INFO << "Disconnect agent.";
        try
        {
            // Stop reconnection thread.
            isAutoReconnect = false;
            isFirstTimeConnected = true; 
            requestConnectEvent.Set();
            WaitConnectThreadExit();

            // Disconnect QAgent
            lockOfQAgent.lock();
            if (isQAgentConnected)
            {
                socketOfQAgent.send(boost::asio::buffer("exit\n"), 0, error);
                ThreadSleep(WaitForSocketClose);
            }
            socketOfQAgent.close();
            isQAgentConnected = false;
            lockOfQAgent.unlock();
            StopQAgent();
            WaitQAgentReceiveThreadExit();

            // Disconnect MAgent
            lockOfMAgent.lock();
            if (isMAgentConnected)
            {
                socketOfMAgent.send(boost::asio::buffer("exit"), 0, error);
                ThreadSleep(WaitForSocketClose);
            }
            socketOfMAgent.close();
            isMAgentConnected = false;
            lockOfMAgent.unlock();
            StopMAgent();
        }

        catch (std::exception &e)
        {
            DAVINCI_LOG_WARNING << "Exception in disconnect: " << e.what();
        }

        SetDeviceStatus(DeviceConnectionStatus::ReadyForConnection);

        return DaVinciStatusSuccess;
    }

    DaVinciStatus AndroidTargetDevice::Click(int x, int y, int ptr, Orientation orientation, bool tapmode)
    {
        DaVinciStatus status;

        CheckCoordinate(x, y);

        if ((tapmode) || (GetCurrentMultiLayerMode()))
        {
            if (IsHorizontalOrientation(orientation))
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_INPUT_HTAP, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_INPUT_TAP, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
        }
        else
        {
            status = TouchDown(x, y, ptr, orientation);
            if (DaVinciSuccess(status))
            {
                ThreadSleep(WaitForDownEvent);
                status = TouchUp(x, y, ptr, orientation);
            }
        }

        if (!DaVinciSuccess(status))
        {
            Point clickPoint = ConvertToDevicePoint(x, y, orientation);
            string sub_command = "input tap " + boost::lexical_cast<string>(clickPoint.x) + " " + boost::lexical_cast<string>(clickPoint.y);
            status = AdbShellCommand(sub_command);
            DAVINCI_LOG_WARNING << "Failed to Click() via Monkey, try to use #adb shell input tap , X:" << clickPoint.x << " , Y:" <<clickPoint.y;
        }

        return status;
    }

    void AndroidTargetDevice::MarkSwipeAction()
    {
        isSwipeAction = true;
    }

    /// <summary> Drags. </summary>
    ///
    /// <param name="x1"> Start position in x-axis. </param>
    /// <param name="y1"> Start position in y-axis. </param>
    /// <param name="x2"> End position in x-axis. </param>
    /// <param name="y2"> End position in y-axis. </param>
    ///
    /// <returns> The DaVinciStatus. </returns>
    DaVinciStatus AndroidTargetDevice::Drag(int x1, int y1, int x2, int y2, Orientation orientation, bool tapmode)
    {
        DaVinciStatus status;

        if ((tapmode) || (GetCurrentMultiLayerMode()))
        {
            if (!IsHorizontalOrientation(orientation))
            {
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_SWIPE_START, static_cast<unsigned int>(x1), static_cast<unsigned int>(y1))));
                if (DaVinciSuccess(status))
                    status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_SWIPE_END, static_cast<unsigned int>(x2), static_cast<unsigned int>(y2))));
            }
            else
            {
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_SWIPE_HORIZONTAL_START, static_cast<unsigned int>(x1), static_cast<unsigned int>(y1))));
                if (DaVinciSuccess(status))
                    status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SHELL_SWIPE_HORIZONTAL_END, static_cast<unsigned int>(x2), static_cast<unsigned int>(y2))));
            }
        }
        else
        {
            if (!IsHorizontalOrientation(orientation))
            {
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SWIPE_START, static_cast<unsigned int>(x1), static_cast<unsigned int>(y1))));
                if (DaVinciSuccess(status))
                    status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SWIPE_END, static_cast<unsigned int>(x2), static_cast<unsigned int>(y2))));
            }
            else
            {
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SWIPE_HORIZONTAL_START, static_cast<unsigned int>(x1), static_cast<unsigned int>(y1))));
                if (DaVinciSuccess(status))
                    status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SWIPE_HORIZONTAL_END, static_cast<unsigned int>(x2), static_cast<unsigned int>(y2))));
            }
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::PressHome(ButtonEventMode mode)
    {
        DaVinciStatus status;

        if (mode == ButtonEventMode::Down)
            pressHomeButtonTime[0] = boost::posix_time::microsec_clock::local_time();
        else if (mode == ButtonEventMode::Up)
        {
            boost::posix_time::time_duration diff;
            pressHomeButtonTime[1] = boost::posix_time::microsec_clock::local_time();

            diff = pressHomeButtonTime[1] - pressHomeButtonTime[0];
            // If press time is longer than 1S, then treat as long press
            if (diff.total_milliseconds() > LongTouchTime) 
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_APP_SWITCH, Q_KEYEVENT_DOWN_AND_UP)));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_HOME, Q_KEYEVENT_DOWN_AND_UP)));
        }
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_HOME, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::PressPower(ButtonEventMode mode)
    {
        DaVinciStatus status;

        if (mode == ButtonEventMode::Down)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_POWER, Q_KEYEVENT_DOWN)));
        else if (mode == ButtonEventMode::Up)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_POWER, Q_KEYEVENT_UP)));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_POWER, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::PressBack(ButtonEventMode mode)
    {
        DaVinciStatus status;

        if (mode == ButtonEventMode::Down)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BACK, Q_KEYEVENT_DOWN)));
        else if (mode == ButtonEventMode::Up)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BACK, Q_KEYEVENT_UP)));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BACK, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::PressMenu(ButtonEventMode mode)
    {
        DaVinciStatus status;

        // For N5 and N7, Menu button's behavior is app switch.Q_KEYCODE_MENU display system settings.
        if (mode == ButtonEventMode::Down)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_APP_SWITCH, Q_KEYEVENT_DOWN)));
        else if (mode == ButtonEventMode::Up)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_APP_SWITCH, Q_KEYEVENT_UP)));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_APP_SWITCH, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    vector<string> AndroidTargetDevice::GetAllInstalledPackageNames()
    {
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;
        vector<string> allPackageNames;

        AdbShellCommand(" pm list packages", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParsePackageNamesString(lines, allPackageNames);

        return allPackageNames;
    }

    vector<string> AndroidTargetDevice::GetSystemPackageNames()
    {
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;
        vector<string> allPackageNames;

        AdbShellCommand(" pm list packages -s", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParsePackageNamesString(lines, allPackageNames);

        return allPackageNames;
    }

    std::string AndroidTargetDevice::GetTopActivityName()
    {
        string topActivityName = "";
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;

        AdbShellCommand(" dumpsys activity top", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParseTopActivityNameString(lines, topActivityName);

        return topActivityName;
    }

    std::string AndroidTargetDevice::GetPackageVersion(const string &packageName)
    {
        string version = "";
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;

        AdbShellCommand(" dumpsys package " + packageName, outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParsePackageVersionString(lines, version);

        return version;
    }

    string AndroidTargetDevice::GetPackageTopInfo(const string &packageName)
    {
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;
        string pkgTopInfo = "";

        AdbShellCommand("top -n 1 ", outStr);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParsePackageTopInfoString(lines, packageName, pkgTopInfo);

        //String format (PID PR CPU% S  #THR     VSS     RSS PCY UID      Name)
        return pkgTopInfo;
    }

    vector<string> AndroidTargetDevice::GetTopActivityViewHierarchy()
    {
        string topActivityViewHierarchy = "";
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;

        AdbShellCommand(" dumpsys activity top", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);

        return lines;
    }

    std::string AndroidTargetDevice::GetTopPackageName()
    {
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        string topPackageName = "";
        vector<string> lines;

        AdbShellCommand(" dumpsys activity top", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        ParseTopPackageNameString(lines, topPackageName);

        return topPackageName;
    }

    bool AndroidTargetDevice::CheckSurfaceFlingerOutput(string matchStr)
    {
        bool flag = false;
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;

        AdbShellCommand(" dumpsys SurfaceFlinger", outStr);

        ReadAllLinesFromStr(outStr->str(), lines);
        if (!lines.empty())
        {
            boost::algorithm::to_lower(matchStr);
            for(int index = 0; index < (int)(lines.size()); index++)
            {
                string line = lines[index];
                boost::algorithm::to_lower(line);

                if (boost::contains(line, matchStr))
                {
                    flag = true;
                    break;
                }
            }
        }

        return flag;
    }

    std::string AndroidTargetDevice::GetHomePackageName()
    {
        string result = GetDataFromQAgent("gethomepackage");
        string value = GetValueFromString(result);

        return value;
    }

    bool AndroidTargetDevice::ClickOffInstallConfirmButton(cv::Mat frame, string topPackageName)
    {
        // Screencapture frame has the same orientation as other camera mode
        Size noResizeFrameSize;
        if(!IsAgentConnected())
        {
            // Get no size frame for device width and height
            cv::Mat noResizeFrame = GetScreenCapture(false);
            noResizeFrameSize = noResizeFrame.size();
        }

        SaveImage(frame, "InstallConfirmScreenCapBefore.png");
        // Need to rotate frame up for text-based recognition
        frame = RotateFrameUp(frame, GetCurrentOrientation(true));
        SaveImage(frame, "InstallConfirmScreenCapAfter.png");

        if (!frame.empty())
        {
            Size frameSize = frame.size();
            bool installConfirmRecognized = false;
            boost::shared_ptr<InstallConfirmRecognizer> installConfirmRecog = boost::shared_ptr<InstallConfirmRecognizer>(new InstallConfirmRecognizer());
            if (topPackageName == installConfirmPackages[DeviceInstallMode::INSTALL_LENOVO_0]
            || topPackageName == installConfirmPackages[DeviceInstallMode::INSTALL_LENOVO_1]
            || topPackageName == installConfirmPackages[DeviceInstallMode::INSTALL_GOOGLE]
            || topPackageName == installConfirmPackages[DeviceInstallMode::INSTALL_XIAOMI]
            || topPackageName == installConfirmPackages[DeviceInstallMode::INSTALL_SAMSUNG])
            {
                installConfirmRecognized = installConfirmRecog->RecognizeInstallConfirm(frame);
            }
            else
            {
                DAVINCI_LOG_ERROR << "Unsupported device install package: " << topPackageName << endl;
                // assert(false); it will impact debug Davinci when the target device is not included in the devicelist.
                return false;
            }

            if (installConfirmRecognized)
            {
                double widthRatio, heightRatio;
                int width, height;
                if(!IsAgentConnected())
                {
                    width = noResizeFrameSize.width;
                    height = noResizeFrameSize.height;
                }
                else
                {
                    width = GetDeviceWidth();
                    height = GetDeviceHeight();
                }

                if(frameSize.width > frameSize.height)
                {
                    if(width > height)
                    {
                        widthRatio = width / double(frameSize.width);
                        heightRatio = height / double(frameSize.height);
                    }
                    else
                    {
                        widthRatio = height / double(frameSize.width);
                        heightRatio = width / double(frameSize.height);
                    }
                }
                else
                {
                    if(width > height)
                    {
                        widthRatio = width / double(frameSize.height);
                        heightRatio = height / double(frameSize.width);
                    }
                    else
                    {
                        widthRatio = width / double(frameSize.width);
                        heightRatio = height / double(frameSize.height);
                    }
                }

                Rect okRect = installConfirmRecog->GetOKRect();
                Point okPoint = Point(okRect.x + okRect.width / 2, okRect.y + okRect.height / 2);

                okPoint.x = (int)(okPoint.x * widthRatio);
                okPoint.y = (int)(okPoint.y * heightRatio);
                string sub_command = "input tap " + boost::lexical_cast<string>(okPoint.x) + " " + boost::lexical_cast<string>(okPoint.y);
                AdbShellCommand(sub_command);
            }
            else
            {
                DAVINCI_LOG_INFO << "There is no install confirm dialog pop up.";
                DebugSaveImage(frame, "InstallConfirmScreenCapNoOK.png");
                return false;
            }
        }
        else
        {
            DAVINCI_LOG_INFO << "Cannot screencap for install confirm dialog.";
            return false;
        }

        return true;
    }

    void AndroidTargetDevice::StartClickOffInstallConfirm(string packageName)
    {
        String manufacturer = GetSystemProp("ro.product.manufacturer");
        boost::to_upper(manufacturer);

        if (manufacturer.empty() || (!MatchInstallationConfirmManufacturer(manufacturer)))
            return;

        bool installConfirmPopUp = false;
        string topPackageName;
        int timeout = WaitForInstallConfirmMinTimeout;
        boost::shared_ptr<StopWatch> installConfirmWatch = boost::shared_ptr<StopWatch>(new StopWatch());
        installConfirmWatch->Start();
        while (true)
        {
            topPackageName = GetTopPackageName();
            if(!installConfirmPopUp)
            {
                if(ContainVectorElement(installConfirmPackages, topPackageName))
                {
                    installConfirmPopUp = true;
                    if(topPackageName == installConfirmPackages[INSTALL_XIAOMI])
                        timeout = WaitForInstallConfirmMaxTimeout;

                    ThreadSleep(WaitForInstallConfirmScreenCap);
                    cv::Mat frame = GetScreenCapture();
                    ClickOffInstallConfirmButton(frame, topPackageName);
                }
                else
                {
                    ThreadSleep(WaitForInstallConfirm);
                }
            }
            else
            {
                if(ContainVectorElement(installConfirmPackages, topPackageName))
                {
                    ThreadSleep(WaitForInstallConfirmScreenCap);
                    cv::Mat frame = GetScreenCapture();
                    ClickOffInstallConfirmButton(frame, topPackageName);
                }
                else
                {
                    installConfirmWatch->Stop();
                    break;
                }
            }

            vector<string> installedPackages = GetAllInstalledPackageNames();
            topPackageName = GetTopPackageName();
            if((!ContainVectorElement(installConfirmPackages, topPackageName) && std::find(installedPackages.begin(), installedPackages.end(), packageName) != installedPackages.end()) || installConfirmWatch->ElapsedMilliseconds() > timeout)
            {
                installConfirmWatch->Stop();
                break;
            }
        }
    }

    DaVinciStatus AndroidTargetDevice::InstallAppWithLog(const string &apkName, string& log, const string &ops, bool toExtSD, bool x64)
    {
        boost::posix_time::ptime startCheckTime, apkInstallTime;
        boost::posix_time::time_duration diff;
        DaVinciStatus status = errc::invalid_argument;
        string apkToInstall(apkName);

        if (apkToInstall == "")
            return status;

        DAVINCI_LOG_INFO << (string("Start install application: ") + apkToInstall + string(" ... "));

        ConvertApkName(apkToInstall);

        if (apkToInstall.find(" ") != string::npos)
            apkToInstall = "\"" + apkToInstall + "\"";

        string subCommand;

        if (!x64)
            subCommand = "install -r -t ";
        else
            subCommand = "install -r -t --abi arm64-v8a ";

        if (toExtSD)
        {
            subCommand = subCommand + " -s ";
            DAVINCI_LOG_INFO << "Installation will install package to external SD card.";
        }

        if (!ops.empty())
            subCommand = subCommand + ops + " ";

        subCommand = subCommand + apkToInstall;

        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        string testPackageName = "";
        vector<string> appInfos = GetPackageActivity(apkToInstall);
        if(appInfos.size() == APP_INFO_SIZE)
            testPackageName = appInfos[PACKAGE_NAME];

        if(testPackageName != "")
        {
            installThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&AndroidTargetDevice::StartClickOffInstallConfirm, this, boost::ref(testPackageName))));

            SetThreadName(installThread->get_id(), "Click install confirm dialog");
            ThreadSleep(WaitForAdbInstallThread);
            startCheckTime = boost::posix_time::microsec_clock::local_time();

            status = AdbCommand(subCommand, outStr, WaitForInstallApp);
            ThreadSleep(WaitForAdbInstallThread);

            apkInstallTime = boost::posix_time::microsec_clock::local_time();

            log = outStr->str();
            DAVINCI_LOG_INFO << "----------------- Installation Report Start -----------------";
            if (toExtSD)
                DAVINCI_LOG_INFO << "Destination: External SD Card.";
            else
                DAVINCI_LOG_INFO << "Destination: Internal Memory.";
            //            diff = apkPushTime - startCheckTime;
            //            DAVINCI_LOG_INFO << "Push Package Time: " <<diff.total_milliseconds() << " ms";
            //            diff = apkInstallTime - apkPushTime;
            diff = apkInstallTime - startCheckTime;
            DAVINCI_LOG_INFO << "Install Package Time: " <<diff.total_milliseconds() << " ms";
            DAVINCI_LOG_INFO << "Logs: " << log;
            DAVINCI_LOG_INFO << "----------------- Installation Report End -----------------";
        }
        else
        {
            DAVINCI_LOG_ERROR << "Installation failed : no package name.";
            status = errc::bad_message;
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::InstallApp(const string &apkName, const string &ops, bool toExtSD)
    {
        string s;
        DaVinciStatus status;

        status = InstallAppWithLog(apkName, s, ops, toExtSD);

        if ((!DaVinciSuccess(status)) || 
            ((!ContainVectorElement(installNoReturnModels, GetModel(true))) && (s.find("Success") == string::npos)))
        {
            status = errc::bad_message;
            DAVINCI_LOG_ERROR << "Installation failed.";
        }
        else
            DAVINCI_LOG_INFO << "Installation succeed.";

        return status;
    }

    DaVinciStatus AndroidTargetDevice::UninstallAppWithLog(const string &packageName, string &log)
    {
        if (packageName == "")
            return DaVinciStatus(errc::invalid_argument);

        string subCommand = "uninstall " + packageName;
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        AdbCommand(subCommand, outStr, 180000);
        log = outStr->str();
        DAVINCI_LOG_INFO << "Uninstallation: " << log;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus AndroidTargetDevice::UninstallApp(const string &packageName)
    {
        string s;
        DaVinciStatus status;

        AdbShellCommand("am force-stop " + packageName);

        status = UninstallAppWithLog(packageName, s);

        if ((!DaVinciSuccess(status)) || (s.find("Success") == string::npos))
        {
            status = errc::bad_message;
            DAVINCI_LOG_ERROR << "Uninstallation failed.";
        }
        else
            DAVINCI_LOG_INFO << "Uninstallation succeed.";

        return status;
    }

    string AndroidTargetDevice::GetLaunchActivityByQAgent(const string &packageName)
    {
        string result = GetDataFromQAgent("launch:" + packageName);
        string value = GetValueFromString(result);

        return value;
    }

    DaVinciStatus AndroidTargetDevice::StartActivity(const string &activityFullName, bool syncMode)
    {
        string fullName(activityFullName);
        string command = "";
        DaVinciStatus status = errc::invalid_argument;

        if (fullName == "" || boost::algorithm::trim_copy(fullName) == "")
            return status;

        if (fullName.find("$") != string::npos)
            boost::algorithm::replace_all(fullName, "$", "\\$");

        if (fullName.find("/") != string::npos)
        {
            string packageName = fullName.substr(0, fullName.find("/"));
            if (specialPackageActivityName.find(packageName) != specialPackageActivityName.end())
                fullName = specialPackageActivityName[packageName];

            if (syncMode)
                command = string("am start -W ") + fullName;
            else
                command = string("am start ") + fullName;

            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            DAVINCI_LOG_INFO << command;
            status = AdbShellCommand(command, outStr, WaitForStartActivity);
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::StopActivity(const string &packageName)
    {
        return AdbShellCommand(string("am force-stop ") + packageName);
    }

    DaVinciStatus AndroidTargetDevice::ClearAppData(const string &packageName) 
    {
        return AdbShellCommand(string("pm clear ") + packageName);
    }

    DaVinciStatus AndroidTargetDevice::TouchDown(int x, int y, int ptr, Orientation orientation)
    {
        DaVinciStatus status;
        CheckCoordinate(x, y);

        if (!isSwipeAction)
        {
            if (IsHorizontalOrientation(orientation))
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_HDOWN, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_DOWN, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
        }
        else
        {
            swipeBegPoint = ConvertToDevicePoint(x, y, orientation);
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::TouchUp(int x, int y, int ptr, Orientation orientation)
    {
        DaVinciStatus status;
        CheckCoordinate(x, y);

        if (isSwipeAction)
        {
            Point swipeEndPoint = ConvertToDevicePoint(x, y, orientation);
            string swipeCommand = "input swipe "
                                  + boost::lexical_cast<string>(swipeBegPoint.x)
                                  + " "
                                  + boost::lexical_cast<string>(swipeBegPoint.y)
                                  + " "
                                  + boost::lexical_cast<string>(swipeEndPoint.x)
                                  + " "
                                  + boost::lexical_cast<string>(swipeEndPoint.y);
            status = AdbShellCommand(swipeCommand);

            // Current swipe action has been done
            isSwipeAction = false;
        }
        else
        {
            if (IsHorizontalOrientation(orientation))
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_HUP, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_UP, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::TouchMove(int x, int y, int ptr, Orientation orientation)
    {
        DaVinciStatus status(DaVinciStatusSuccess);

        static boost::posix_time::ptime gLastTouchMoveTime = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration elapsedTime = boost::posix_time::microsec_clock::local_time() - gLastTouchMoveTime;
        CheckCoordinate(x, y);

        // Because current actions is swipe action, so we should ignore touch move action
        if (isSwipeAction)
            return status;

        if (reduceTouchMoveFrequency
            && (elapsedTime.total_milliseconds() < MinTouchMoveInterval)) //Reduce the frequency of touchmove event.
            return status;

        if (IsHorizontalOrientation(orientation))
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_HMOVE, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_TOUCH_MOVE, static_cast<unsigned int>(x), static_cast<unsigned int>(y))));

        // Record execution time stamp of current touch move action
        gLastTouchMoveTime = boost::posix_time::microsec_clock::local_time();

        return status;
    }


    /// <summary>
    /// Convert windows key code to Android key code
    /// </summary>
    /// <returns>key code</returns>
    unsigned int AndroidTargetDevice::GetAndroidKeyCode(int keycode)
    {
        unsigned int retCode = Q_KEYCODE_NOTSUPPORT;

        switch (keycode)
        {
        case VK_RETURN:
            retCode = Q_KEYCODE_DPAD_CENTER;
            break;
        case VK_RIGHT: 
            retCode = Q_KEYCODE_DPAD_RIGHT;
            break;
        case VK_DOWN: 
            retCode = Q_KEYCODE_DPAD_DOWN;
            break;
        case VK_LEFT: 
            retCode = Q_KEYCODE_DPAD_LEFT;
            break;
        case VK_UP: 
            retCode = Q_KEYCODE_DPAD_UP;
            break;
        case VK_ESCAPE: 
            retCode = Q_KEYCODE_BACK;
            break;
        case VK_BACK: 
            retCode = Q_KEYCODE_DEL;
            break;
        case VK_SPACE:
            retCode = Q_KEYCODE_MEDIA_PLAY_PAUSE;
            break;
        default:
            retCode = Q_KEYCODE_NOTSUPPORT;
            break;
        }

        return retCode;
    }

    DaVinciStatus AndroidTargetDevice::Keyboard(bool isKeyDown, int keyCode)
    {
        unsigned int androidKeycode = GetAndroidKeyCode(keyCode);
        unsigned int androidKeyEvent = Q_KEYEVENT_DOWN;
        DaVinciStatus status = errc::not_supported;

        if (isKeyDown)
            androidKeyEvent = Q_KEYEVENT_DOWN;
        else
            androidKeyEvent = Q_KEYEVENT_UP;

        if (androidKeycode != Q_KEYCODE_NOTSUPPORT)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, androidKeycode, androidKeyEvent)));

        return status;
    }

    Orientation AndroidTargetDevice::GetDefaultOrientation(bool isRealTime)
    {
        int orientationInt = -1;
        Orientation orientation = Orientation::Portrait;

        if (isRealTime)
        {
            string result = GetDataFromQAgent("getdefaultorientation");
            string value = GetValueFromString(result);

            TryParse(value, orientationInt);

            switch (orientationInt)
            {
            case 0:
                orientation = Orientation::Portrait;
                break;
            case 1:
                orientation = Orientation::Landscape;
                break;
            default:
                DAVINCI_LOG_ERROR << string("Failed to get default orientation data from device. Data incorrect: ") << orientationInt;
                orientation = Orientation::Portrait;
                break;
            }

            defaultOrientation = orientation;
        }

        return defaultOrientation;
    }

    Orientation AndroidTargetDevice::GetCurrentOrientation(bool isRealTime)
    {
        int orientationInt = -1;
        Orientation orientation = Orientation::Portrait;;

        string sdkVersion =  GetSdkVersion(false);
        int apiLevel = 19;
        TryParse(sdkVersion, apiLevel);

        // For Android N Intel device, need to get orientation in real time
        if (isRealTime || apiLevel > 23)
        {
            string value;

            if (isQAgentConnected)
            {
                string result = GetDataFromQAgent("getorientation");
                value = GetValueFromString(result);
            }
            else
                value = GetOrientationFromSurfaceFlinger();

            TryParse(value, orientationInt);

            switch (orientationInt)
            {
            case 0:
                orientation = Orientation::Portrait;
                break;
            case 1:
                orientation = Orientation::Landscape;
                break;
            case 2:
                orientation = Orientation::ReversePortrait;
                break;
            case 3:
                orientation = Orientation::ReverseLandscape;
                break;
            default:
                DAVINCI_LOG_ERROR << string("Failed to get orientation data from device. Data incorrect: ") << orientationInt;
                orientation = Orientation::Portrait;
                break;
            }
            currentOrientation = orientation;
        }

        return currentOrientation;
    }

    DaVinciStatus AndroidTargetDevice::SetCurrentOrientation(Orientation orientation)
    {
        DaVinciStatus status;
        int count = 1;

        string orientationvalue = "0";

        switch (orientation)
        {
        case Orientation::Portrait:
            orientationvalue = "0";
            break;
        case Orientation::Landscape:
            orientationvalue = "1";
            break;
        case Orientation::ReversePortrait:
            orientationvalue = "2";
            break;
        case Orientation::ReverseLandscape:
            orientationvalue = "3";
            break;
        default:
            orientationvalue = "0";
            break;
        }

        while (count++ < 5)
        {
            string result = GetDataFromQAgent("setorientation:" + orientationvalue);
            ThreadSleep(count * WaitForStartOrientation);

            if (GetCurrentOrientation(true) == orientation)
                break;
        }

        if (count >= 5)
        {
            DAVINCI_LOG_ERROR << "SetCurrentOrientation failed, Please make sure the device is placed flat and its auto-rotation feature is enabled!";
            status = errc::not_supported;

            // Usually, when set orientation failure caused by QAgent Rotation Activity blocked or didn't enable auto rotation. Stop QAgent for auto recovery.
            // Comment it due to in current design even if set orientation fail, only will print errors and let it continue .
            // isQAgentConnected = false; 
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::VolumeDown(ButtonEventMode mode)
    {
        DaVinciStatus status;
        if (mode == ButtonEventMode::Down)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_DOWN, Q_KEYEVENT_DOWN)));
        else if (mode == ButtonEventMode::Up)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_DOWN, Q_KEYEVENT_UP)));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_DOWN, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::VolumeUp(ButtonEventMode mode)
    {
        DaVinciStatus status;
        if (mode == ButtonEventMode::Down)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_UP, Q_KEYEVENT_DOWN)));
        else if (mode == ButtonEventMode::Up)
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_UP, Q_KEYEVENT_UP)));
        else
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_VOLUME_UP, Q_KEYEVENT_DOWN_AND_UP)));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::BrightnessAction(ButtonEventMode mode, Brightness actionMode)
    {
        DaVinciStatus status;
        if (actionMode == Brightness::Up)
        {
            if (mode == ButtonEventMode::Down)
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_UP, Q_KEYEVENT_DOWN)));
            else if (mode == ButtonEventMode::Up)
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_UP, Q_KEYEVENT_UP)));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_UP, Q_KEYEVENT_DOWN_AND_UP)));
        }
        else if (actionMode == Brightness::Down)
        {
            if (mode == ButtonEventMode::Down)
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_DOWN, Q_KEYEVENT_DOWN)));
            else if (mode == ButtonEventMode::Up)
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_DOWN, Q_KEYEVENT_UP)));
            else
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_BRIGHTNESS_DOWN, Q_KEYEVENT_DOWN_AND_UP)));
        }
        return status;
    }

    DaVinciStatus AndroidTargetDevice::WakeUp(ButtonEventMode mode)
    {
        return SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_WAKE, 0, Q_KEYEVENT_DOWN_AND_UP)));
    }

    DaVinciStatus AndroidTargetDevice::ShowLotus()
    {
        DaVinciStatus status;

        isDisplayLotus = true;

        string result = GetDataFromQAgent("showlotus:0"); //0 = default orientation.
        string value = GetValueFromString(result, 2);

        if (value != "succeed")
            status = errc::permission_denied;

        return status;
    }

    DaVinciStatus AndroidTargetDevice::HideLotus()
    {
        DaVinciStatus status;

        isDisplayLotus = false;

        string result = GetDataFromQAgent("hidelotus");
        string value = GetValueFromString(result);

        if (value != "succeed")
            status = errc::permission_denied;

        return status;
    }

    DaVinciStatus AndroidTargetDevice::ClearInput()
    {
        DaVinciStatus status;

        for (int i = 0; i < 30; i++)
        {
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_DEL, Q_KEYEVENT_DOWN_AND_UP)));

            if (!DaVinciSuccess(status))
                break;
        }

        for (int i = 0; i < 30; i++)
        {
            status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_FORWARD_DEL, Q_KEYEVENT_DOWN_AND_UP)));

            if (!DaVinciSuccess(status))
                break;
        }
        return status;
    }

    DaVinciStatus AndroidTargetDevice::TypeString(const string &str)
    {
        // DaVinciRunner's issue about type space
        // We need to type space separately
        DaVinciStatus status;
        vector<string> strArray;
        boost::algorithm::split(strArray, str, boost::algorithm::is_any_of(" "));
        for (int i = 0; i < static_cast<int>(strArray.size()); i++)
        {
            string s = strArray[i];
            if (s != "")
            {
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(static_cast<unsigned int>(Q_SHELL_INPUT_TEXT & 0xff), static_cast<unsigned int>(s.length() & 0xfff), 0)));
                status = SendToMAgent(vector<unsigned char>(s.begin(), s.end()));
            }

            if (static_cast<int>(strArray.size()) > 1)
                status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_WHITESPACE, Q_KEYEVENT_DOWN_AND_UP)));
        }

        return status;
    }

    DaVinciStatus AndroidTargetDevice::PressEnter()
    {
        return SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_ENTER, Q_KEYEVENT_DOWN_AND_UP)));
    }

    DaVinciStatus AndroidTargetDevice::PressDelete()
    {
        return SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_DEL, Q_KEYEVENT_DOWN_AND_UP)));
    }

    DaVinciStatus AndroidTargetDevice::PressSearch()
    {
        return SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_PRESS_KEY, Q_KEYCODE_SEARCH, Q_KEYEVENT_DOWN_AND_UP)));
    }

    unsigned int AndroidTargetDevice::MakeMAgentCommand(unsigned int command, unsigned int para1, unsigned int para2)
    {
        return (command << 24) | (para1 << 12) | para2;
    }

    string AndroidTargetDevice::GetAdbPath()
    {
        return TestManager::Instance().GetDaVinciResourcePath("platform-tools/adb.exe");
    }

    string AndroidTargetDevice::GetAaptPath()
    {
        return TestManager::Instance().GetDaVinciResourcePath("platform-tools/aapt.exe");
    }

    string AndroidTargetDevice::GetQAgentPath()
    {
        return TestManager::Instance().GetDaVinciResourcePath("libs/QAgent/QAgent.apk");
    }

    DaVinciStatus AndroidTargetDevice::AdbCommand(const string &adbArgs, const boost::shared_ptr<ostringstream> &outStr, int timeout)
    {
        DaVinciStatus status = errc::not_connected;

        if (IsDevicesAttached())
            status = RunProcessSync(GetAdbPath(), " -s " + GetDeviceName() + " " + adbArgs, outStr, ".", timeout);

        return status;
    }

    DaVinciStatus AndroidTargetDevice::AdbPushCommand(const string &source, const string &target, int timeout)
    {
        string preSource = ProcessPathSpaces(source);
        string preTarget = ProcessPathSpaces(target);

        if (!boost::filesystem::exists(preSource))
            return DaVinciStatus(errc::no_such_file_or_directory);

        return AdbCommand("push " + preSource + " " + preTarget, nullptr, timeout);
    }

    DaVinciStatus AndroidTargetDevice::AdbShellCommand(const string &command, const boost::shared_ptr<ostringstream> &outStr, int timeout)
    {
        return AdbCommand("shell " + command, outStr, timeout);
    }

    int AndroidTargetDevice::FindProcess(const string &name)
    {
        int pid = -1;
        auto stdoutStr = boost::shared_ptr<ostringstream>(new ostringstream());

        AdbShellCommand("ps", stdoutStr);

        istringstream stdoutStream(stdoutStr->str());
        while (!stdoutStream.eof())
        {
            string line;
            getline(stdoutStream, line);

            vector<string> columnsWithEmptyStr;
            vector<string> columns;

            boost::algorithm::split(columnsWithEmptyStr, line, boost::algorithm::is_any_of(" "));
            for (auto column : columnsWithEmptyStr)
            {
                if (!column.empty())
                {
                    columns.push_back(column);
                }
            }
            // assume process name is the last column
            if (columns.size() >= 2 && columns[columns.size() - 1].find(name) != string::npos)
            {
                string pidString = columns[1];
                if (!TryParse(pidString, pid))
                {
                    pid = -1;
                }
                break;
            }
        }
        return pid;
    }

    DaVinciStatus AndroidTargetDevice::KillProcess(const string &name, int sig)
    {
        DaVinciStatus status = errc::no_such_process;
        int pid = FindProcess(name);

        if (pid > 0)
            status = AdbShellCommand(string(" kill -") + boost::lexical_cast<string>(sig) + string(" ") + boost::lexical_cast<string>(pid));

        return status;    }

    bool AndroidTargetDevice::IsDevicePortBusy(const int port)
    {
        bool busy = false;
        auto stdoutStr = boost::shared_ptr<ostringstream>(new ostringstream());
        string portStr = boost::lexical_cast<std::string>(port);

        DaVinciSuccess(AdbShellCommand("netstat", stdoutStr, 5000));

        istringstream stdoutStream(stdoutStr->str());
        while (!stdoutStream.eof())
        {
            string line;
            getline(stdoutStream, line);
            if (line.find(portStr) != string::npos)
            {
                busy = true;
                break;
            }
        }

        return busy;
    }

    string AndroidTargetDevice::GetCpuArch()
    {
        String value = GetSystemProp("ro.product.cpu.abi");

        return value;
    }

    string AndroidTargetDevice::GetOsVersion(bool isRealTime)
    {
        if (isRealTime)
        {
            string result = GetDataFromQAgent("getosversion");
            string value = GetValueFromString(result);
            if (value == "")
                value = GetSystemProp("ro.build.version.release");

            if (value.length() > 3)
                value = value.substr(0, 3);
            osVersion = value;
        }

        return osVersion;
    }

    string AndroidTargetDevice::GetSdkVersion(bool isRealTime)
    {
        if (isRealTime)
        {
            string result = GetDataFromQAgent("getsdkversion");
            string value = GetValueFromString(result);
            if (value == "")
                value = GetSystemProp("ro.build.version.sdk");

            sdkVersion = value;
        }

        return sdkVersion;
    }

    bool AndroidTargetDevice::IsARCdevice()
    {
        string model = GetModel(true);
        bool isARC = AppUtil::Instance().InARCModelNames(model);
        return isARC;
    }

    string AndroidTargetDevice::GetModel(bool isRealTime)
    {
        if (isRealTime)
            model = GetSystemProp("ro.product.model");

        return model;
    }

    double AndroidTargetDevice::GetDPI()
    {
        double dpi = 320; //default dpi = 320
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        string dpiStr = "";
        vector<string> lines;

        AdbShellCommand(" wm density", outStr);
        ReadAllLinesFromStr(outStr->str(), lines);
        if (ParseDPIString(lines, dpiStr))
            TryParse(dpiStr, dpi);

        return dpi;
    }

    DaVinciStatus AndroidTargetDevice::SendToMAgent(const vector<unsigned char> &buffer)
    {
        DaVinciStatus status = errc::no_message;

        if (buffer.empty())
            return status;

        string sendStr(buffer.begin(),buffer.end());
        string recvStr = GetDataFromMAgent(sendStr);

        if (!recvStr.empty())
            status = DaVinciStatusSuccess;

        return status;
    }

    string AndroidTargetDevice::GetDataFromMAgent(const string &sendStr)
    {
        char recvBuffer[128];
        int recvCount = 0;
        string recvStr = "";
        boost::system::error_code error = boost::asio::error::host_not_found;
        DaVinciStatus status;

        if ((!isMAgentConnected) || (sendStr.empty()) || (isDisplayLotus))
        {
            DAVINCI_LOG_ERROR << "GetDataFromMAgent failure, isMAgentConnected: " << isMAgentConnected <<", sendStr size: " << sendStr;
            return recvStr;
        }

        boost::lock_guard<boost::mutex> lock(lockOfMAgent);
        socketOfMAgent.send(boost::asio::buffer(sendStr), 0, error);
        status = (DaVinciStatus)(error);
        if (DaVinciSuccess(status))
        {
            memset(recvBuffer, 0, 128);
            recvCount = (int)socketOfMAgent.receive(boost::asio::buffer(recvBuffer), 0, error);

            status = (DaVinciStatus)(error);
            if (error != 0)
            {
                DAVINCI_LOG_ERROR << "GetDataFromMAgent Exception: " << error;
                KillProcess("com.android.commands.monkey");
                isMAgentConnected = false;
            }

            if (recvCount > 0)
            {
                recvStr = string(recvBuffer, recvBuffer + recvCount);
                // DAVINCI_LOG_DEBUG << "GetDataFromMAgent Received: " << recvStr;
            }
        }
        else
        {
            isMAgentConnected = false;
            DAVINCI_LOG_ERROR << "GetDataFromMAgent failure: " << status.value();
        }

        return recvStr;
    }

    DaVinciStatus AndroidTargetDevice::SendToQAgent(const string &sendStr)
    {
        boost::system::error_code error;
        string sendStrN = sendStr;

        if ((!isQAgentConnected) || (sendStr.empty()))
            return DaVinciStatus(errc::operation_not_permitted);

        // Send command must end with '\n'
        if (sendStr.find_last_of('\n') == string::npos)
            sendStrN = sendStr + '\n';

        socketOfQAgent.send(boost::asio::buffer(sendStrN), 0, error);

        DaVinciStatus status(error);
        if (!DaVinciSuccess(status))
        {
            isQAgentConnected = false;
            DAVINCI_LOG_DEBUG << "Failed to send to QAgent: " << status.value();
        }
        return status;
    }

    // This command for those commands have return strings from QAgent
    string AndroidTargetDevice::GetDataFromQAgent(const string &sendStr)
    {
        string recvStr = "";

        boost::lock_guard<boost::mutex> lock(lockOfQAgent);

        DaVinciStatus status = SendToQAgent(sendStr);

        if (DaVinciSuccess(status))
        {
            // Try 3 times to get the expected data from QAgent.
            for (int i = 0; i < 3; i++)
            {
                recvStr = GetFromQAgentQueue(WaitForQAgentExecution);

                if (!recvStr.empty())
                {
                    string cmdStr = GetValueFromString(sendStr, 0);

                    if (recvStr.find(cmdStr) != string::npos)
                        break;
                    else
                        DAVINCI_LOG_DEBUG << "Mismatched data from QAgent, Expected: " << cmdStr << " , Actually: " << recvStr;
                }
            }
        }
        else
            DAVINCI_LOG_DEBUG << "Failed to GetDataFromQAgent for command: " << sendStr;

        return recvStr;
    }

    // This command for those commands without return strings from QAgent
    DaVinciStatus AndroidTargetDevice::SendQAgentCommand(const string &cmd)
    {
        return SendToQAgent(cmd + string("\n"));
    }

    void AndroidTargetDevice::CheckCoordinate(int &x, int &y)
    {
        if (x < 0)
            x = 0;
        else if (x > Q_COORDINATE_X)
            x = Q_COORDINATE_X;

        if (y < 0)
            y = 0;
        else if (y > Q_COORDINATE_Y)
            y = Q_COORDINATE_Y;
    }

    bool AndroidTargetDevice::AddToQAgentQueue(string recvStr)
    {
        string strRet = "";
        boost::unique_lock<boost::mutex> lock(lockOfQAgentReceiveQueue);

        QAgentReceiveQueue.push(recvStr);
        QAgentReceiveQueueEvent.Set();

        return true;
    }

    string AndroidTargetDevice::GetFromQAgentQueue(int waitTimeout)
    {
        string strRet = "";

        if (QAgentReceiveQueueEvent.WaitOne(waitTimeout))
        {
            if (QAgentReceiveQueue.size() > 0)
            {
                boost::unique_lock<boost::mutex> lock(lockOfQAgentReceiveQueue);

                string tmpStr = QAgentReceiveQueue.front();
                QAgentReceiveQueue.pop();

                strRet = boost::algorithm::trim_copy(tmpStr);
            }
        }

        return strRet;
    }

    void AndroidTargetDevice::ResetQAgentQueue()
    {
        boost::unique_lock<boost::mutex> lock(lockOfQAgentReceiveQueue);
        queue<string> empty;

        std::swap(QAgentReceiveQueue, empty); 
    }

    bool AndroidTargetDevice::IsAgentConnected()
    {
        return (isMAgentConnected && isQAgentConnected);
    }

    bool AndroidTargetDevice::IsDevicesAttached()
    {
        string subCommand =  " get-state";
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        DaVinciStatus status = RunProcessSync(GetAdbPath(), " -s " + GetDeviceName() + " " + subCommand, outStr, ".", 10000); //This command will be used in AdbCommand, so must use RunProcessSync

        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Unable to call adb get-state on device " << GetDeviceName() << ": " << status.message();
            return false;
        }

        istringstream iss(outStr->str());
        while (!iss.eof())
        {
            string line;
            getline(iss, line);
            vector<string> columns;
            boost::algorithm::split(columns, line, boost::algorithm::is_any_of(" "));

            if (columns.size() == 1 && columns[0].find("device") != string::npos)
                return true;
        }

        DAVINCI_LOG_ERROR << "The target device: " << GetDeviceName() << " is disconnected.";
        SetDeviceStatus(DeviceConnectionStatus::Offline);
        return false;
    }

    bool AndroidTargetDevice::IsQAgentNeedUpgrade()
    {
        int version = 0;
        bool needUpgrade = true;

        string result = GetDataFromQAgent("getqagentversion");
        string value = GetValueFromString(result);

        TryParse(value, version);

        if (version == QAgentVersion)
        {
            DAVINCI_LOG_DEBUG << "QAgent Version is: " + value + ", needn't upgrade.";
            needUpgrade = false;
        }
        else
        {
            DAVINCI_LOG_DEBUG << "QAgent Version is: " + value + ", need upgrade.";
        }

        return needUpgrade;
    }

    bool AndroidTargetDevice::CheckAgentsStatus()
    {
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - lastQAgentDataTime;

        if (diff.total_milliseconds() > WaitForAutoCheckConnection)
            SendQAgentCommand("test"); 

        // Comments it for Treadmill may fail due to Monkey low response.
        // GetDataFromMAgent("test"); //Only receive heart beat data from MAgent, must keep it for track if MAgent alive.

        if (heartBeatCount ++ > MaxHeartbeatCount)
            isQAgentConnected = false;

        if (IsAgentConnected())
            return true;
        else
            return false;
    }

    // Check if current device's DaVinciRunner is active, if yes, kill it.
    DaVinciStatus AndroidTargetDevice::StopMAgent()
    {
        DaVinciStatus status = errc::permission_denied;
        HANDLE handle = INVALID_HANDLE_VALUE;
        PROCESSENTRY32 entry; 

        try
        {
            handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
            assert(handle != INVALID_HANDLE_VALUE);

            entry.dwSize = sizeof(entry); 
            Process32First(handle, &entry);

            for (unsigned int i = 0; i < entry.dwSize; i++)
            { 
                Process32Next(handle, &entry);
                boost::process::process p(entry.th32ProcessID); 
                string exefile(entry.szExeFile, 200);

                if (exefile.find("java.exe") != string::npos)
                {
                    string exeCmd = find_executable_in_path("wmic.exe");
                    string exeArgs = " path win32_process where processid=" + boost::lexical_cast<std::string>(p.get_id()) + " get Commandline" ;

                    auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());

                    RunProcessSync(exeCmd, exeArgs, outStr);

                    istringstream iss(outStr->str());
                    while (!iss.eof())
                    {
                        string line;
                        getline(iss, line);
                        if ((line.find(deviceName) != string::npos) && (line.find("MAgent.py") != string::npos) && (line.find("monkeyrunner.jar") != string::npos))
                        {
                            DAVINCI_LOG_INFO << "ExistingDaVinciRunner command: " << line;
                            p.terminate();
                            ThreadSleep(WaitForJavaTermation);
                            DAVINCI_LOG_INFO << "Kill ExistingDaVinciRunner succeed.";
                            break;
                        }
                    }
                }
            } 

            KillProcess("com.android.commands.monkey");
            status = DaVinciStatusSuccess;
        }
        catch(...)
        {
            DAVINCI_LOG_WARNING << "StopMAgent Exception!";
        }
        CloseHandle(handle); 

        return status;
    }

    DaVinciStatus AndroidTargetDevice::InstallDaVinciMonkey()
    {
        string davinciMonkeyLibs = TestManager::Instance().GetDaVinciResourcePath("libs/davincimonkey");

        boost::filesystem::path davinciMonkeyPath(davinciMonkeyLibs);
        
        string sdkVersion =  GetSdkVersion(true);
        int apiLevel = 19;
        TryParse(sdkVersion, apiLevel);
        std::string apiLevelString = AndroidTargetDevice::ApiLevelToVersion(apiLevel);

        DaVinciStatus ret = AdbPushCommand((davinciMonkeyLibs / "davinci-monkey").string(), "/data/local/tmp/davinci-monkey");
        if (!DaVinciSuccess(ret))
            return ret;

        ret = AdbPushCommand((davinciMonkeyLibs /apiLevelString / "davinci-monkey.jar").string(), "/data/local/tmp/davinci-monkey.jar");
        if (!DaVinciSuccess(ret))
            return ret;

        ret = AdbShellCommand("chmod 777 /data/local/tmp/davinci-monkey");
        return ret;
    }

    DaVinciStatus AndroidTargetDevice::StartMAgent()
    {
        vector<string> argList;
        context ctxout;
        boost::shared_ptr<boost::process::process> JavaProcess;
        string libsPath = TestManager::Instance().GetDaVinciResourcePath("libs");
        string DaVinciRunnerLibs = TestManager::Instance().GetDaVinciResourcePath("libs/davincirunner");

        if (!DaVinciSuccess(InstallDaVinciMonkey()))
        {
            DAVINCI_LOG_ERROR <<"Failed in installing DaVinci Monkey";
            return DaVinciStatus(errc::io_error);
        }

        try
        {
            string command = find_executable_in_path("java.exe");  // will throw exception when failed to find java.exe

            ctxout.work_directory = ".";

            argList.push_back("-Xmx512m");
            argList.push_back("-Djava.ext.dirs=" + DaVinciRunnerLibs + ';' + DaVinciRunnerLibs + "\\x86\\swt.jar");
            argList.push_back("-Dcom.android.monkeyrunner.bindir=" + libsPath);
            argList.push_back("-jar");
            argList.push_back(DaVinciRunnerLibs + "\\monkeyrunner.jar");
            argList.push_back(TestManager::Instance().GetDaVinciResourcePath("MAgent.py"));
            argList.push_back("null");
            argList.push_back(deviceName);
            argList.push_back(boost::lexical_cast<std::string>(magentPort));

            if (!DaVinciSuccess(StopMAgent()))
                StopMAgent();

            JavaProcess = boost::shared_ptr<boost::process::process>(new boost::process::process(launch(command, argList, ctxout)));
            ThreadSleep(WaitForJavaReady); //Wait 2S for Java & Monkeyrunner to connect with the device.
        }

        catch(...)
        {
            DAVINCI_LOG_ERROR <<"StartMAgent failed. Failed to find or startup java.exe, please make sure java.exe installed in your system!";
            return DaVinciStatus(errc::permission_denied);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus AndroidTargetDevice::InstallQAgent()
    {
        UninstallQAgent();

        return InstallApp(GetQAgentPath());
    }

    DaVinciStatus AndroidTargetDevice::UninstallQAgent()
    {
        return UninstallApp("com.example.qagent");
    }

    DaVinciStatus AndroidTargetDevice::StopQAgent()
    {
        return StopActivity("com.example.qagent");
    }

    DaVinciStatus AndroidTargetDevice::StartQAgent()
    {
        DaVinciStatus status;

        StopQAgent();
        status = AdbShellCommand("am startservice --user 0 -n com.example.qagent/.QAgentService");
        ThreadSleep(WaitForQAgentStartup);
        status = AdbCommand("forward --remove tcp:" + boost::lexical_cast<std::string>(qagentPort));
        status = AdbCommand("forward tcp:" + boost::lexical_cast<std::string>(qagentPort) + " tcp:" + boost::lexical_cast<std::string>(DeviceQAgentPort));

        return status;
    }

    DaVinciStatus AndroidTargetDevice::ConnectToQAgent()
    {
        int try_num = 0;
        tcp::endpoint qagent_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), (unsigned short)qagentPort);
        boost::system::error_code error = boost::asio::error::host_not_found;

        DAVINCI_LOG_INFO <<"Connecting to QAgent ...";
        while (isAutoReconnect)
        {
            StartQAgent();

            lockOfQAgent.lock();
            socketOfQAgent.close();
            socketOfQAgent.connect(qagent_endpoint, error);
            lockOfQAgent.unlock();

            if (error != 0)
            {
                isQAgentConnected = false;
                DAVINCI_LOG_INFO <<"QAgent connect exception!";
                InstallQAgent();
            }
            else
            {
                isQAgentConnected = true;
                if (QAgentReceiveThreadHandle == nullptr)
                {
                    QAgentReceiveThreadHandle = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&AndroidTargetDevice::QAgentReceiveThreadEntry, this)));
                    SetThreadName(QAgentReceiveThreadHandle->get_id(), "QAgent receive");
                    QAgentReceiveThreadReadyEvent.WaitOne(WaitForThreadReady);
                }
            }

            if (isQAgentConnected)
            {
                if (IsQAgentNeedUpgrade())
                {
                    isQAgentConnected = false;

                    InstallQAgent();
                }
                else
                {
                    AdbCommand("forward --remove tcp:" + boost::lexical_cast<std::string>(qagentPort));
                    DAVINCI_LOG_INFO <<"QAgent Connected.";
                    break;
                }
            }

            if (try_num++ > 5)
            {
                AdbCommand("forward --remove tcp:" + boost::lexical_cast<std::string>(qagentPort));
                isQAgentConnected = false;

                StopQAgent();

                if (IsDevicePortBusy(DeviceQAgentPort))
                    DAVINCI_LOG_ERROR <<"The Device's Port :" << DeviceQAgentPort << " is occupied by other applications.";

                DAVINCI_LOG_ERROR <<"Failed: Connecting to QAgent failure, tried 5 times.";
                break;
            }

            ThreadSleep(try_num * WaitForSocketConnection);
        }

        ResetQAgentQueue();

        return DaVinciStatusSuccess;
    }

    DaVinciStatus AndroidTargetDevice::ConnectToMAgent()
    {
        int try_num = 0;
        int sendTimeout = WaitForSocketData * 5; //5S
        int executeTimeout = WaitForSocketData * 15; // Expect the command will be complete in 15S
        tcp::endpoint magent_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), (unsigned short)magentPort);
        boost::system::error_code error = boost::asio::error::host_not_found;

        DAVINCI_LOG_INFO <<"Connecting to MAgent ...";

        while (isAutoReconnect)
        {
            lockOfMAgent.lock();
            socketOfMAgent.close();
            socketOfMAgent.connect(magent_endpoint, error);
            setsockopt(socketOfMAgent.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeout, sizeof(sendTimeout));
            setsockopt(socketOfMAgent.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&executeTimeout, sizeof(executeTimeout));
            lockOfMAgent.unlock();

            if (error != 0)
                DAVINCI_LOG_INFO << "MAgent connect exception!";
            else
            {
                isMAgentConnected = true;
                DAVINCI_LOG_INFO << "MAgent Connected.";
                break;
            }

            if (try_num++ > 10)
            {
                DAVINCI_LOG_ERROR << "Failed: Connecting to MAgent failure, tried 10 times. Continue...";
                break;
            }

            if (try_num % 5 == 0)
            {
                StopIncompatiblePacakges();

                if (IsDevicePortBusy(DeviceMonkeyPort))
                    DAVINCI_LOG_ERROR <<"The Device's Port :" << DeviceMonkeyPort << " is occupied by other applications.";

                // remove it due to x2 cannot support this command. will enabled it after stability check feature is ready.
                // AdbCommand("usb"); For fix bug1650, adb daemon status incorrect may cause MAgent cannot connect. 
                StartMAgent();
            }

            ThreadSleep(try_num * WaitForSocketConnection); //Auto adjust the wait time when connection failed.
        }

        return DaVinciStatusSuccess;
    }

    void AndroidTargetDevice::ConnectThreadEntry(void)
    {
        boost::posix_time::ptime  startCheckTime, qAgentConnectTime, mAgentConnectTime;
        boost::posix_time::time_duration diff;

        connectThreadReadyEvent.Set();

        while (isAutoReconnect)
        {
            requestConnectEvent.WaitOne(WaitForAutoCheckConnection); //Auto check the connection status every 30S
            if ((!isAutoReconnect) || (CheckAgentsStatus()))
            {
                completeConnectEvent.Set();
                continue;
            }

            if (!IsDevicesAttached())
            {
                isQAgentConnected = false;
                DAVINCI_LOG_DEBUG << "Warning: The device " << deviceName << " is offline, please check hardware conenction!";
                completeConnectEvent.Set();
                continue;
            }

            unsigned short base = MAgentDefaultPort;
            if (magentPort != 0)
                base = magentPort;
            magentPort = GetAvailableLocalPort(base);

            base = QAgentDefaultPort;
            if (qagentPort != 0)
                base = qagentPort;
            qagentPort = GetAvailableLocalPort(base);

            if (!isMAgentConnected) // For saving connecting time.
                StartMAgent();

            startCheckTime = boost::posix_time::microsec_clock::local_time();

            if (!isQAgentConnected)
                ConnectToQAgent();

            qAgentConnectTime = boost::posix_time::microsec_clock::local_time();

            if (!isMAgentConnected)
                ConnectToMAgent();

            if (isMAgentConnected && isQAgentConnected)
            {
                SetDeviceStatus(DeviceConnectionStatus::Connected);
                // The following system information only need update once when first time connected.
                GetSystemInformation();

                // Must make sure Monkey's resolution is aligned for each time when reconnected.
                SetMonkeyResolution();
            }

            mAgentConnectTime = boost::posix_time::microsec_clock::local_time();

            diff = qAgentConnectTime - startCheckTime;
            DAVINCI_LOG_DEBUG << " QAgent connect time   :" <<diff.total_milliseconds() << " ms";

            diff = mAgentConnectTime - qAgentConnectTime;
            DAVINCI_LOG_DEBUG << " MAgent connect time   :" <<diff.total_milliseconds() << " ms";

            completeConnectEvent.Set();
        }

        connectThreadHandle = nullptr;
        connectThreadExitEvent.Set();
    }

    bool AndroidTargetDevice::WaitConnectThreadExit()
    {
        if (connectThreadHandle != nullptr)
            connectThreadExitEvent.WaitOne(WaitForConnectThreadExit);

        return true;
    }

    void AndroidTargetDevice::QAgentReceiveThreadEntry(void)
    {
        char recvBuffer[1024];
        int recvCount = 0;
        boost::system::error_code error = boost::asio::error::host_not_found;

        QAgentReceiveThreadReadyEvent.Set();

        while(isAutoReconnect)
        {
            if (!isQAgentConnected) 
            {
                ThreadSleep(WaitForSocketConnection);
                continue;
            }

            memset(recvBuffer, 0, 1024);
            recvCount = (int)socketOfQAgent.receive(boost::asio::buffer(recvBuffer), 0, error);

            if (error != 0)
            {
                DAVINCI_LOG_DEBUG << "QAgentReceiveThread Exception: " << error;
                isQAgentConnected = false;
                ThreadSleep(WaitForSocketClose);
                continue; // Connection closed cleanly by peer.
            }

            if (recvCount > 0)
            {
                string recvStr(recvBuffer, recvBuffer + recvCount);

                // when received any information from device proves the device is alive.
                heartBeatCount = 0; 
                lastQAgentDataTime = boost::posix_time::microsec_clock::local_time();

                if (boost::starts_with(recvStr,"auto"))
                    QAgentAutoMsgHandler(recvStr); // Parse auto message;
                else
                    AddToQAgentQueue(recvStr);     // Send the message to Queue by default;
            }
            else
            {
                ThreadSleep(WaitForSocketConnection);
                continue;
            }
        }

        QAgentReceiveThreadHandle = nullptr;
        QAgentReceiveThreadExitEvent.Set();
    }

    bool AndroidTargetDevice::WaitQAgentReceiveThreadExit()
    {
        if (QAgentReceiveThreadHandle != nullptr)
            QAgentReceiveThreadExitEvent.WaitOne(WaitForQAgentReceiveThreadExit);

        return true;
    }

    /// <summary>
    /// Handle auto message from target device.
    /// </summary>
    /// <param name="recvStr"></param>
    /// <returns></returns>
    void AndroidTargetDevice::QAgentAutoMsgHandler(string recvStr)
    {
        int orientationInt = 0;

        if (boost::starts_with(recvStr,"auto:orientation"))
        {
            string value = GetValueFromString(recvStr, 2);

            TryParse(value, orientationInt);
            Orientation orientation;

            switch (orientationInt)
            {
            case 0:
                orientation = Orientation::Portrait;
                break;
            case 1:
                orientation = Orientation::Landscape;
                break;
            case 2:
                orientation = Orientation::ReversePortrait;
                break;
            case 3:
                orientation = Orientation::ReverseLandscape;
                break;
            default:
                orientation = Orientation::Portrait;
                break;
            }
            currentOrientation = orientation;
            DAVINCI_LOG_DEBUG << "QAgentReceived Auto Event: " << recvStr;
        } else if (boost::starts_with(recvStr,"auto:performance")) {
            //DAVINCI_LOG_DEBUG << "Device Performance data: " << recvStr;
            // "auto:performance:cpuusage:memtotal:memfree:batterylevel:disktotal:diskfree

            string value = "";
            long tmp = 0;
            value = GetValueFromString(recvStr, 2);
            TryParse(value, tmp);
            deviceCpuUsage = tmp;

            value = GetValueFromString(recvStr, 3);
            TryParse(value, tmp);
            deviceMemTotal = tmp;

            value = GetValueFromString(recvStr, 4);
            TryParse(value, tmp);
            deviceMemFree = tmp;

            value = GetValueFromString(recvStr, 5);
            TryParse(value, tmp);
            deviceBatteryLevel = tmp;

            value = GetValueFromString(recvStr, 6);
            TryParse(value, tmp);
            deviceDiskTotal = tmp;

            value = GetValueFromString(recvStr, 7);
            TryParse(value, tmp);
            deviceDiskFree = tmp;

            isAutoPerfUpdated = true;

        } else {
            DAVINCI_LOG_DEBUG << "QAgentReceived other Auto Event: " + recvStr;
        }
    }

    /// <summary>
    /// Init the system information of the target device
    /// Failed if both output are 0.
    /// Called when device is connected.
    /// </summary>
    void AndroidTargetDevice::GetSystemInformation()
    {
        // Only need to do this once when the device first time connected;
        if (isFirstTimeConnected) 
        {
            GetDeviceResolution(); 
            GetOsVersion(true);
            GetSdkVersion(true);
            GetLanguage(true);
            GetModel(true);
            GetDefaultOrientation(true);

            MeasureBarHeight();
            SetCurrentOrientation(GetDefaultOrientation()); 

            //Mark the flag after all the system information collected.
            isFirstTimeConnected = false;
        }
    }

    void AndroidTargetDevice::GetDeviceResolution()
    {
        int tmp = 0;
        string result = GetDataFromQAgent("getresolution");

        string value = GetValueFromString(result);
        TryParse(value, tmp);
        SetDeviceHeight(tmp);

        value = GetValueFromString(result, 2);
        TryParse(value, tmp);
        SetDeviceWidth(tmp);
    }

    bool AndroidTargetDevice::SetMonkeyResolution()
    {
        DaVinciStatus status;

        if (deviceHeight <= 0 || deviceWidth <= 0)
            return false;

        status = SendToMAgent(GetBytesFromUInt(MakeMAgentCommand(Q_SETRESOLUTION, static_cast<unsigned int>(deviceWidth), static_cast<unsigned int>(deviceHeight))));

        if (DaVinciSuccess(status))
            return true;
        else
            return false;
    }

    bool AndroidTargetDevice::IsHyperSoftCamSupported()
    {
        return true;
    }

    bool AndroidTargetDevice::GetBarHeightFromSurfaceFlinger(const cv::Size &deviceSize, int frameLongSide, int &statusBarHeight, int &navigationBarHeight)
    {
        bool result = false;
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        AdbShellCommand(" dumpsys SurfaceFlinger", outStr);
        std::vector<std::string> lines;
        ReadAllLinesFromStr(outStr->str(), lines);

        result = ParseBarHeightFromSurfaceFlingerString(lines, deviceSize, frameLongSide, statusBarHeight, navigationBarHeight);

        return result;
    }

    string AndroidTargetDevice::GetOrientationFromSurfaceFlinger()
    {
        string orientation = "";
        std::vector<std::string> lines;
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());

        AdbShellCommand(" dumpsys SurfaceFlinger", outStr);

        ReadAllLinesFromStr(outStr->str(), lines);

        ParseOrientationFromSurfaceFlingerString(lines, orientation);

        return orientation;
    }

    void AndroidTargetDevice::MeasureBarHeight(int frameWidth)
    {
        string result = GetDataFromQAgent("shownavigationbar:portrait");
        ThreadSleep(WaitForStartOrientation);
        Size deviceSize(deviceHeight, deviceWidth);
        int statusBarHeight = 0, navigationBarHeight = 0;
        GetBarHeightFromSurfaceFlinger(deviceSize, frameWidth, statusBarHeight, navigationBarHeight);
        barHeight.portraitStatus = statusBarHeight;
        barHeight.portraitNavigation = navigationBarHeight;
        result = GetDataFromQAgent("hidenavigationbar");
        ThreadSleep(WaitForHideOrientation);
        result = GetDataFromQAgent("hidenavigationbar"); //double confirm navigation bar was hiden.

        statusBarHeight = 0, navigationBarHeight = 0;
        result = GetDataFromQAgent("shownavigationbar:landscape");
        ThreadSleep(WaitForStartOrientation);
        GetBarHeightFromSurfaceFlinger(deviceSize, frameWidth, statusBarHeight, navigationBarHeight);
        barHeight.landscapeStatus = statusBarHeight;
        barHeight.landscapeNavigation = navigationBarHeight;
        result = GetDataFromQAgent("hidenavigationbar");
        ThreadSleep(WaitForHideOrientation);
        result = GetDataFromQAgent("hidenavigationbar"); //double confirm navigation bar was hiden.
    }

    bool AndroidTargetDevice::GetBarInfoFromWindowService(const vector<std::string> &lines, Point &pt, int &width, int &height, bool &isPopUp, const BarStyle &barStyle)
    {
        const string popUpWindowStr("PopupWindow:");
        const string focusedWindowStr("mCurrentFocus=Window{");
        string focusedWord("Window{");
        string statusbarMatchStr = "StatusBar}:";
        string navigationbarMatchStr = "NavigationBar}:";
        string inputmethodMatchStr = "InputMethod}:";
        bool isParsed = false;
        isPopUp = false;

        switch (barStyle)
        {
        case BarStyle::Navigationbar:
            {
                isParsed = GetInfoFromTargetString(lines, navigationbarMatchStr, pt, width, height);
                break;
            }
        case BarStyle::StatusBar:
            {
                isParsed = GetInfoFromTargetString(lines, statusbarMatchStr, pt, width, height);
                break;
            }
        case BarStyle::InputMethod:
            {
                isParsed = GetInfoFromTargetString(lines, inputmethodMatchStr, pt, width, height);
                break;
            }
        case BarStyle::FocusedWindow:
            {
                for (auto& line : lines)
                {
                    auto pos = line.find(focusedWindowStr);
                    if (pos != string::npos)
                    {
                        auto popPos = line.find(popUpWindowStr);
                        if (popPos != string::npos)
                            isPopUp = true;

                        auto begPos = pos + focusedWindowStr.size();
                        pos = line.find(" ", pos);
                        if (pos != string::npos)
                        {
                            focusedWord += line.substr(begPos, pos - begPos);
                        }                
                        break;
                    }
                }
                isParsed = GetInfoFromTargetString(lines, focusedWord, pt, width, height);
                break;
            }
        default:
            break;
        }

        return isParsed;
    }

    std::vector<std::string> AndroidTargetDevice::GetWindowBarLines()
    {
        vector<string> lines;
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        AdbCommand("shell dumpsys window windows", outStr, 30000);
        ReadAllLinesFromStr(outStr->str(), lines);
        return lines;
    }

    AndroidTargetDevice::WindowBarInfo AndroidTargetDevice::GetWindowBarInfo(std::vector<std::string>& lines)
    {
        Rect navigationBar;
        Rect statusBar;
        Rect focusedWindow;
        bool isParsed;
        bool isPopUp;

        Point pt;
        int width;
        int height;
        Size deviceSize;

        deviceSize = Size(GetDeviceWidth(), GetDeviceHeight());

        lines = GetWindowBarLines();

        isParsed = AndroidTargetDevice::GetBarInfoFromWindowService(lines, pt, width, height, isPopUp, BarStyle::Navigationbar);
        if(isParsed)
        {
            windowBarInfo.navigationbarRect = Rect(pt.x, pt.y, width, height);
        }

        isParsed = AndroidTargetDevice::GetBarInfoFromWindowService(lines, pt, width, height, isPopUp, BarStyle::StatusBar);
        if(isParsed)
        {
            windowBarInfo.statusbarRect = Rect(pt.x, pt.y, width, height);
        }

        isParsed = AndroidTargetDevice::GetBarInfoFromWindowService(lines, pt, width, height, isPopUp, BarStyle::FocusedWindow);
        if(isParsed)
        {
            windowBarInfo.focusedWindowRect = Rect(pt.x, pt.y, width, height);
            windowBarInfo.isPopUpWindow = isPopUp;
        }

        isParsed = AndroidTargetDevice::GetBarInfoFromWindowService(lines, pt, width, height, isPopUp, BarStyle::InputMethod);
        if(isParsed)
        {
            windowBarInfo.inputmethodExist = true;
        }
        else
        {
            windowBarInfo.inputmethodExist = false;
        }

        return windowBarInfo;
    }

    vector<int> AndroidTargetDevice::GetStatusBarHeight()
    {
        vector<int> statusBarHeights;

        statusBarHeights.push_back(barHeight.portraitStatus);
        statusBarHeights.push_back(barHeight.landscapeStatus);

        return statusBarHeights;
    }

    int AndroidTargetDevice::GetActionBarHeight()
    {
        int actionBarHeight = 0;

        string result = GetDataFromQAgent("getactionbarheight");
        string value = GetValueFromString(result);
        TryParse(value, actionBarHeight);

        return actionBarHeight;
    }

    vector<int> AndroidTargetDevice::GetNavigationBarHeight()
    {
        vector<int> navigationBarHeights;

        navigationBarHeights.push_back(barHeight.portraitNavigation);
        navigationBarHeights.push_back(barHeight.landscapeNavigation);

        return navigationBarHeights;
    }

    string AndroidTargetDevice::GetLanguage(bool isRealTime)
    {
        if (isRealTime)
        {
            string result = GetDataFromQAgent("getlanguage");
            string value = GetValueFromString(result);
            if (value.empty())
                value = GetSystemProp("persist.sys.language");
            if (value.empty()) //In some Rockchip devices, the persist.sys.language item will not exist when used default system language
                value = GetSystemProp("ro.product.locale.language");

            Language = value;
        }

        return Language;
    }

    /// <summary> Get system prop info </summary>
    ///
    /// <param name="propName">   The prop name. </param>
    /// <returns>  system prop info </returns>
    string AndroidTargetDevice::GetSystemProp(string propName)
    {
        string sysprop = "";
        string result = GetDataFromQAgent("getsysprop:" + propName);
        string value = GetValueFromString(result, 2);

        if (value.empty())
        {
            auto stdoutStr = boost::shared_ptr<ostringstream>(new ostringstream());
            AdbShellCommand("getprop " + propName, stdoutStr, 5000);
            value = stdoutStr->str();
        }

        sysprop = boost::algorithm::trim_copy(value);

        return sysprop;
    }

    bool AndroidTargetDevice::GetCpuInfo(string &strProcessorName, unsigned int &dwMaxClockSpeed)  
    {  
        auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
        vector<string> lines;

        AdbShellCommand("cat /proc/cpuinfo", outStr, WaitForRunCommand);
        ReadAllLinesFromStr(outStr->str(), lines);

        return ParseCpuInfoString(lines, strProcessorName, dwMaxClockSpeed);
    }  

    int AndroidTargetDevice::GetCpuUsage(bool isRealTime)
    {
        if ((!isRealTime) && (isAutoPerfUpdated))
            return deviceCpuUsage;

        int cpuUsage = 0;
        string result = GetDataFromQAgent("getperformance:cpuusage");
        string value = GetValueFromString(result, 2);

        if (!value.empty())
        {
            TryParse(value, cpuUsage);
        }
        else
        {
            DaVinciStatus status;
            auto outStr_start = boost::shared_ptr<ostringstream>(new ostringstream());
            auto outStr_end = boost::shared_ptr<ostringstream>(new ostringstream());
            vector<string> lines_start;
            vector<string> lines_end;
            ULONGLONG workjiffies_start = 0, workjiffies_end = 0;
            ULONGLONG totaljiffies_start = 0, totaljiffies_end = 0;

            AdbShellCommand("cat /proc/stat", outStr_start, WaitForRunCommand);

            ReadAllLinesFromStr(outStr_start->str(), lines_start);
            ParseCpuUsageString(lines_start, totaljiffies_start, workjiffies_start);

            AdbShellCommand("cat /proc/stat", outStr_end, WaitForRunCommand);

            ReadAllLinesFromStr(outStr_end->str(), lines_end);

            ParseCpuUsageString(lines_end, totaljiffies_end, workjiffies_end);

            if ((totaljiffies_end - totaljiffies_start) != 0)
            {
                cpuUsage = static_cast<int>((workjiffies_end - workjiffies_start) * 100 / (totaljiffies_end - totaljiffies_start));
            }
        }

        if (cpuUsage > 99) 
            cpuUsage = 99;

        deviceCpuUsage = cpuUsage;

        return deviceCpuUsage;
    }

    /// <summary>
    /// Get Free memory. 256 -> 256MB
    /// </summary>
    bool AndroidTargetDevice::GetMemoryInfo(long &memtotal, long &memfree, bool isRealTime)
    {
        memtotal = deviceMemTotal;
        memfree = deviceMemFree;

        if ((!isRealTime) && (isAutoPerfUpdated))
            return true;

        string result = GetDataFromQAgent("getperformance:memory");
        string valuetotal = GetValueFromString(result, 2);
        string valuefree = GetValueFromString(result, 3);

        if ((!valuetotal.empty()) && (!valuefree.empty()))
        {
            TryParse(valuetotal, memtotal);
            TryParse(valuefree, memfree);
        }
        else
        {
            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            vector<string> lines;

            AdbShellCommand("cat /proc/meminfo", outStr, WaitForRunCommand);
            ReadAllLinesFromStr(outStr->str(), lines);

            ParseMemoryInfoString(lines, memtotal, memfree);
        }

        deviceMemTotal = memtotal;
        deviceMemFree = memfree;

        return true;
    }

    /// <summary>
    /// Get free disk.1024->1024MB
    /// </summary>
    bool AndroidTargetDevice::GetDiskInfo(long &disktotal, long& diskfree, bool isRealTime)
    {
        disktotal = deviceDiskTotal;
        diskfree = deviceDiskFree;

        if ((!isRealTime) && (isAutoPerfUpdated))
            return true;

        string result = GetDataFromQAgent("getperformance:disk");
        string valuetotal = GetValueFromString(result, 2);
        string valuefree = GetValueFromString(result, 3);

        if ((!valuetotal.empty()) && (!valuefree.empty()))
        {
            TryParse(valuetotal, disktotal);
            TryParse(valuefree, diskfree);
        }
        else
        {
            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            vector<string> lines;

            AdbShellCommand("df /data", outStr, WaitForRunCommand);
            ReadAllLinesFromStr(outStr->str(), lines);

            ParseDiskInfoString(lines, disktotal, diskfree);
        }

        deviceDiskTotal = disktotal;
        deviceDiskFree = diskfree;

        return true;
    }

    /// <summary>
    /// Get battery level
    /// </summary>
    bool AndroidTargetDevice::GetBatteryLevel(long &batteryLevel, bool isRealTime)
    {
        batteryLevel = deviceBatteryLevel;

        if ((!isRealTime) && (isAutoPerfUpdated))
            return true;

        string result = GetDataFromQAgent("getperformance:batterylevel");
        string value = GetValueFromString(result, 2);

        if (!value.empty())
        {
            TryParse(value, batteryLevel);
        }
        else
        {
            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());

            AdbShellCommand("dumpsys battery", outStr, WaitForRunCommand);

            vector<string> lines;
            ReadAllLinesFromStr(outStr->str(), lines);
            if (!lines.empty())
            {
                for(int index = 0; index < (int)(lines.size()); index++)
                {
                    string line = lines[index];
                    vector<string> items;

                    value = "";
                    boost::trim(line);
                    if (boost::starts_with(line, "level"))
                    {
                        boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                        if(items.size() > 0)
                            value = items[1];
                        TryParse(value, batteryLevel);
                        break;
                    }
                }
            }
        }

        deviceBatteryLevel = batteryLevel;

        return true;
    }
 
    /// <summary>
    /// Get Sensor Data
    /// </summary>
    bool AndroidTargetDevice::GetSensorData(int type, string &data)
    {
        bool resultFlag = false;
        data = "";
        switch (type)
        {
        case Q_SENSOR_CHECKER_STOP:         //0
            {
                DAVINCI_LOG_INFO << "Stop the sensor checker action from QAgent!";
                data = string("Stop the sensor checker action from QAgent!");
                string result = GetDataFromQAgent("getsensordata:stop");
                string value = GetValueFromString(result, 2);
                if (value == "succeed")
                    resultFlag = true;

                break;
            }
        case Q_SENSOR_CHECKER_START:        //1
            {
                DAVINCI_LOG_INFO << "Start the sensor checker action from QAgent!";
                data = string("Start the sensor checker action from QAgent!");
                string result = GetDataFromQAgent("getsensordata:start");
                string value = GetValueFromString(result, 2);
                if (value == "succeed")
                    resultFlag = true;

                break;
            }
        case Q_SENSOR_ACCELEROMETER:        //100
            {
                string result = GetDataFromQAgent("getsensordata:accelerometer");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Accelerometer data get from QAgent (m/s^2): X: ") << GetValueFromString(result, 2) << string(", Y: ") << GetValueFromString(result, 3) << string(", Z: ") << GetValueFromString(result, 4);
                    data = string("Accelerometer data get from QAgent (m/s^2): X: ") + GetValueFromString(result, 2) + string(", Y: ") + GetValueFromString(result, 3) + string(", Z: ") + GetValueFromString(result, 4);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_GYROSCOPE:            //101
            {
                string result = GetDataFromQAgent("getsensordata:gyroscope");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Gyroscope data get from QAgent (rad/s): X: ") << GetValueFromString(result, 2) << string(", Y: ") << GetValueFromString(result, 3) << string(", Z: ") << GetValueFromString(result, 4);
                    data = string("Gyroscope data get from QAgent (rad/s): X: ") + GetValueFromString(result, 2) + string(", Y: ") + GetValueFromString(result, 3) + string(", Z: ") + GetValueFromString(result, 4);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_TEMPERATURE:          //103
            {
                string result = GetDataFromQAgent("getsensordata:temperature");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Temperature data get from QAgent (C): ") << GetValueFromString(result, 2);
                    data = string("Temperature data get from QAgent (C): ") + GetValueFromString(result, 2);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_PRESSURE:             //104
            {
                string result = GetDataFromQAgent("getsensordata:pressure");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Pressure data get from QAgent (Pa): ") << GetValueFromString(result, 2);
                    data = string("Pressure data get from QAgent (Pa): ") + GetValueFromString(result, 2);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_LINEAR_ACCELERATION:   //107
            {
                string result = GetDataFromQAgent("getsensordata:linearacceleration");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Linear Acceleration data get from QAgent (rad/s): X: ") << GetValueFromString(result, 2) << string(", Y: ") << GetValueFromString(result, 3) << string(", Z: ") << GetValueFromString(result, 4);
                    data = string("Linear Acceleration data get from QAgent (rad/s): X: ") + GetValueFromString(result, 2) + string(", Y: ") + GetValueFromString(result, 3) + string(", Z: ") + GetValueFromString(result, 4);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_GRAVITY:              //108
            {
                string result = GetDataFromQAgent("getsensordata:gravity");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Gravity data get from QAgent (rad/s): X: ") << GetValueFromString(result, 2) << string(", Y: ") << GetValueFromString(result, 3) << string(", Z: ") << GetValueFromString(result, 4);
                    data = string("Gravity data get from QAgent (rad/s): X: ") + GetValueFromString(result, 2) + string(", Y: ") + GetValueFromString(result, 3) + string(", Z: ") + GetValueFromString(result, 4);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_ROTATION_VECTOR:       //109
            {
                string result = GetDataFromQAgent("getsensordata:rotationvector");
                string value = GetValueFromString(result, 2);
                if ((value != "") && (value != "Unknown"))
                {
                    DAVINCI_LOG_INFO << string("Rotation data get from QAgent (rad/s): X: ") << GetValueFromString(result, 2) << string(", Y: ") << GetValueFromString(result, 3) << string(", Z: ") << GetValueFromString(result, 4);
                    data = string("Rotation data get from QAgent (rad/s): X: ") + GetValueFromString(result, 2) + string(", Y: ") + GetValueFromString(result, 3) + string(", Z: ") + GetValueFromString(result, 4);
                    resultFlag = true;
                }
                break;
            }

        case Q_SENSOR_COMPASS:              //102
        case Q_SENSOR_ATMOSPHERE:           //105
        case Q_SENSOR_ALTITUDE:             //106
            {
                DAVINCI_LOG_ERROR << "Unsupported sensor checker type parameter: " << type << " from QAgent!";
                data = string("Unsupported sensor checker type parameter: ") + boost::lexical_cast<string>(type) + string(" from QAgent!");
                resultFlag = false;
                break;
            }
        default:
            {
                DAVINCI_LOG_ERROR << "Wrong sensor checker type parameter: " << type;
                data = string("Wrong sensor checker type parameter: ") + boost::lexical_cast<string>(type);
                resultFlag = false;
                break;
            }
        }
        return resultFlag;
    }

    /// <summary>
    /// Get value from string
    /// </summary>
    /// <param name="input"></param>
    /// <param name="valueNum"> Start from 0, usually value 0 is the command string</param>
    /// <returns></returns>
    string AndroidTargetDevice::GetValueFromString(String input, int valueNum)
    {
        String value = "";

        if (!input.empty())
        {
            vector<string> dimension;
            boost::algorithm::replace_last(input, "\n", "");

            boost::algorithm::split(dimension, input, boost::algorithm::is_any_of(":"));
            if ((!dimension.empty()) && (dimension.size() > (unsigned int) valueNum) && (!dimension[valueNum].empty()))
                value = dimension[valueNum];
        }

        return value;
    }

    Mat AndroidTargetDevice::GetScreenCapture(bool needResize)
    {
        Mat screenCapMat;
        auto outStream = boost::shared_ptr<boost::interprocess::basic_ovectorstream<vector<char>>>(new boost::interprocess::basic_ovectorstream<vector<char>>());
        string subCommand = " -s " + GetDeviceName() + " shell screencap -p";

        if (!IsDevicesAttached())
            return screenCapMat;

        Orientation orientation = GetCurrentOrientation(true);
        RunProcessSyncRaw(GetAdbPath(), subCommand, outStream, ".", WaitForAdbScreenCap);
        const vector<char> outData = outStream->vector();

        ParseScreenCaptureString(outData, orientation, needResize, screenCapMat);

        return screenCapMat;
    }

    int AndroidTargetDevice::GetQAgentPort() const
    {
        return qagentPort;
    }

    int AndroidTargetDevice::GetMAgentPort() const
    {
        return magentPort;
    }

    int AndroidTargetDevice::GetHyperSoftCamPort() const
    {
        return hyperSoftCamPort;
    }

    void AndroidTargetDevice::SetQAgentPort(unsigned short p)
    {
        qagentPort = p;
    }

    void AndroidTargetDevice::SetMAgentPort(unsigned short p)
    {
        magentPort = p;
    }

    void AndroidTargetDevice::SetHyperSoftCamPort(unsigned short p)
    {
        hyperSoftCamPort = p;
    }

    void AndroidTargetDevice::AllocateAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList)
    {
        AllocateMAgentPort(targetDeviceList);
        AllocateQAgentPort(targetDeviceList);
    }

    void AndroidTargetDevice::AllocateMAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList)
    {
        bool occupied;
        unsigned short port = MAgentDefaultPort;

        do {
            occupied = false;
            port = GetAvailableLocalPort(port);

            DAVINCI_LOG_DEBUG << "Allocate MAgent Port: " << port;
            {

                for (auto targetDevice = targetDeviceList.begin(); targetDevice != targetDeviceList.end(); targetDevice++)
                {
                    if ((*targetDevice) != nullptr)
                    {
                        boost::shared_ptr<AndroidTargetDevice> dut = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                        if (dut->GetMAgentPort() == port)
                        {
                            port++;
                            occupied = true;
                            break;
                        }
                    }
                }
            }
        }while (occupied == true);

        SetMAgentPort(port);
    }

    void AndroidTargetDevice::AllocateQAgentPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList)
    {
        bool occupied;
        unsigned short port = QAgentDefaultPort;

        do {
            occupied = false;
            port = GetAvailableLocalPort(port);

            DAVINCI_LOG_DEBUG << "Allocate QAgent Port: " << port;
            {
                for (auto targetDevice = targetDeviceList.begin(); targetDevice != targetDeviceList.end(); targetDevice++)
                {
                    if ((*targetDevice) != nullptr)
                    {
                        boost::shared_ptr<AndroidTargetDevice> dut = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                        if (dut->GetQAgentPort() == port)
                        {
                            port++;
                            occupied = true;
                            break;
                        }
                    }
                }
            }
        }while (occupied == true);

        SetQAgentPort(port);
    }

    void AndroidTargetDevice::AllocateHyperSoftCamPort(vector<boost::shared_ptr<TargetDevice>> & targetDeviceList)//, boost::recursive_mutex &targetDeviceListMutex)
    {
        bool occupied;
        unsigned short port = HyperSoftCamCapture::localPortBase;

        do {
            occupied = false;
            port = GetAvailableLocalPort(port);

            DAVINCI_LOG_DEBUG << "Allocate HyperSoftCam Port: " << port;
            {
                // boost::lock_guard<boost::recursive_mutex> lock(targetDeviceListMutex);
                for (auto targetDevice = targetDeviceList.begin(); targetDevice != targetDeviceList.end(); targetDevice++)
                {
                    if ((*targetDevice) != nullptr)
                    {
                        // hypersoftcam only supports android target device
                        boost::shared_ptr<AndroidTargetDevice> dut = boost::dynamic_pointer_cast<AndroidTargetDevice>(*targetDevice);
                        if (dut->GetHyperSoftCamPort() == port)
                        {
                            port++;
                            occupied = true;
                            break;
                        }
                    }
                }
            }
        }while (occupied == true);

        hyperSoftCamPort = port;
    }

    void AndroidTargetDevice::StopIncompatiblePacakges()
    {
        string packageName;
        vector<string> installedPackage = GetAllInstalledPackageNames();

        for (int i = 0; i < static_cast<int>(incompatiblePackages.size()); i++)
        {
            for (int j = 0; j < static_cast<int>(installedPackage.size()); j++)
            {
                if (installedPackage[j].find(incompatiblePackages[i]) != string::npos)
                {
                    DAVINCI_LOG_ERROR << "Detected incompatible package: " << incompatiblePackages[i] << " installed!";
                    StopActivity(incompatiblePackages[i]);
                }
            }
        }

        return;
    }

    /// <summary>
    /// Get CPU usage. 40->40%
    /// </summary>
    bool AndroidTargetDevice::ParseCpuUsageString(const vector<string> &lines, ULONGLONG &totaljiffies, ULONGLONG &workjiffies)
    {
        totaljiffies = 0;
        workjiffies = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            vector<string> items;
            ULONGLONG tmpjiffies = 0;
            string value = "";

            boost::trim(line);
            if (boost::starts_with(line, "cpu "))
            {
                while(boost::algorithm::contains(line, "  "))
                {
                    boost::algorithm::replace_all(line, "  ", " ");
                }

                boost::algorithm::split(items, line, boost::algorithm::is_any_of(" "));
                // The meanings of the columns are as follows, from left to right:
                // http://www.mjmwired.net/kernel/Documentation/filesystems/proc.txt
                // user: normal processes executing in user mode
                // nice: niced processes executing in user mode
                // system: processes executing in kernel mode
                // idle: twiddling thumbs
                // iowait: waiting for I/O to complete
                // irq: servicing interrupts
                // softirq: servicing softirqs

                if(items.size() > 6)
                {
                    for(unsigned int i= 1; i < 3; i++)
                    {
                        value = items[i];
                        TryParse(value, tmpjiffies);
                        workjiffies = workjiffies + tmpjiffies;
                    }

                    for(unsigned int i= 1; i < items.size(); i++)
                    {
                        value = items[i];
                        TryParse(value, tmpjiffies);
                        totaljiffies = totaljiffies + tmpjiffies;
                    }
                }
                break;
            }
        }
        return true;
    }

    bool AndroidTargetDevice::ParseCpuInfoString(const vector<string> &lines, string &strProcessorName, unsigned int &dwMaxClockSpeed)  
    {  
        strProcessorName = "";
        dwMaxClockSpeed = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string value = "";
            string line = lines[index];
            vector<string> items;

            value = "";
            boost::trim(line);
            if (boost::starts_with(line, "model name"))
            {
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if (items.size() > 1)
                    strProcessorName = boost::algorithm::trim_copy(items[1]);
            } else if (boost::starts_with(line, "cpu MHz"))
            {
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if (items.size() > 1)
                {
                    value = items[1];
                    boost::algorithm::split(items, value, boost::algorithm::is_any_of("."));
                    if (items.size() > 0)
                        TryParse(items[0], dwMaxClockSpeed);
                    break;
                }
            }
        }

        return true;
    }

    bool AndroidTargetDevice::ParseMemoryInfoString(const vector<string> &lines, long &memtotal, long& memfree)
    {
        memtotal = 0;
        memfree = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            vector<string> items;

            string value = "";
            boost::trim(line);
            if (boost::starts_with(line, "MemTotal"))
            {
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if(items.size() > 1)
                {
                    value = items[1];
                    boost::algorithm::replace_all(value, "kB", "");
                    TryParse(value, memtotal);
                    memtotal = memtotal/KB2MB;
                }
            } else if (boost::starts_with(line, "MemFree"))
            {
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if(items.size() > 1)
                {
                    value = items[1];
                    boost::algorithm::replace_all(value, "kB", "");
                    TryParse(value, memfree);
                    memfree = memfree/KB2MB;
                }
                break;
            }
        }
        return true;
    }

    bool AndroidTargetDevice::ParseDiskInfoString(const vector<string> &lines, long &disktotal, long& diskfree)
    {
        disktotal = 0;
        diskfree = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            vector<string> items;
            vector<string> tmpitems;
            string value = "";

            boost::trim(line);

            if (boost::starts_with(line, "/data"))
            {
                while(boost::algorithm::contains(line, "  "))
                {
                    boost::algorithm::replace_all(line, "  ", " ");
                }

                boost::algorithm::split(items, line, boost::algorithm::is_any_of(" "));
                if(items.size() < 4)
                    return false;
                else
                {
                    value = items[1];

                    if (boost::algorithm::contains(value, "G"))
                    {
                        boost::algorithm::replace_all(value, "G", " ");
                        double totalInG;

                        TryParse(value, totalInG);
                        disktotal =  static_cast<long> (totalInG * 1024);
                    }
                    else if (boost::algorithm::contains(value, "M"))
                    {
                        double totalInM;
                        boost::algorithm::replace_all(value, "M", " ");
                        TryParse(value, totalInM);
                        disktotal =  static_cast<long> (totalInM);
                    }
                    else
                    {
                        double totalInM;
                        TryParse(value, totalInM);
                        disktotal =  static_cast<long> (totalInM);
                    }

                    value = items[3];
                    if (boost::algorithm::contains(value, "G"))
                    {
                        boost::algorithm::replace_all(value, "G", " ");

                        double freeInG;
                        TryParse(value, freeInG);
                        diskfree = static_cast<long>(freeInG * 1024);
                    }
                    else if (boost::algorithm::contains(value, "M"))
                    {
                        double freeInM;
                        boost::algorithm::replace_all(value, "M", " ");
                        TryParse(value, freeInM);
                        diskfree=  static_cast<long> (freeInM);
                    }
                    else
                    {
                        double freeInM;
                        TryParse(value, freeInM);
                        diskfree=  static_cast<long> (freeInM);
                    }
                }

                break;
            } 
        }
        return true;
    }

    bool AndroidTargetDevice::ParseBarHeightFromSurfaceFlingerString(const vector<std::string> &lines, const cv::Size &deviceSize, int &frameLongSide, int &statusBarHeight, int &navigationBarHeight)
    {
        bool result = false;
        std::string targetLine = "";
        std::string matchStr = "";
        int deviceShortSide = 0;
        int deviceLongSide = 0;
        int frameShortSide = 0;
        double shortSideRatio = 0.0;
        double longSideRatio = 0.0;
        int width = 0;
        int height = 0;
        bool findExpectedStr = false;

        statusBarHeight = 0;
        navigationBarHeight = 0;

        deviceShortSide = min(deviceSize.width, deviceSize.height);
        deviceLongSide = max(deviceSize.width, deviceSize.height);

        if ((lines.empty()) || (deviceShortSide == 0) || (deviceLongSide == 0))
            return result;

        frameShortSide = frameLongSide * deviceShortSide / deviceLongSide;
        shortSideRatio = (double)frameShortSide / deviceShortSide;
        longSideRatio = (double)frameLongSide / deviceLongSide;

        string sizeMatchStr = "size=", cropMatchStr = "crop=";

        for (int i = 1; i <= 2; i++)
        {
            matchStr = (i == 1 ? "StatusBar" : "NavigationBar");
            size_t targetIndex = 0;
            for (size_t index = 0; index < lines.size(); index++)
            {
                if (boost::contains(lines[index], matchStr))
                {
                    targetIndex = index;
                    findExpectedStr = true;
                    break;
                }
            }
            if (!findExpectedStr)
                continue;

            findExpectedStr = false;

            for (auto index = targetIndex; index < lines.size(); index++)
            {
                if (boost::contains(lines[index], sizeMatchStr))
                {
                    targetIndex = index;
                    findExpectedStr = true;
                    break;
                }
            }
            if (!findExpectedStr)
                continue;

            string targetLine = lines[targetIndex];

            int sizeIndex = (int)targetLine.find(sizeMatchStr);
            int cropIndex = (int)targetLine.find(cropMatchStr);
            std::string subStr = targetLine.substr(sizeIndex, cropIndex - sizeIndex);

            int widthIndex = (int)subStr.find_first_of("(");
            int separatorIndex = (int)subStr.find_first_of(",");
            std::string widthStr = subStr.substr(widthIndex+1, separatorIndex - (widthIndex + 1));

            int heightIndex = (int)subStr.find_first_of(")");
            std::string heightStr = subStr.substr(separatorIndex+1, heightIndex - (separatorIndex + 1));

            double rectWidth = 0.0;
            double rectHeight = 0.0;
            if (TryParse(widthStr, rectWidth) && TryParse(heightStr, rectHeight))
            {
                width = (int)rectWidth;
                height = (int)rectHeight;
            }

            if (width != 0 && height != 0)
            {
                result = true;

                int barShortSide = min(width, height);
                int barLongSide = max(width, height);

                if (barLongSide == deviceLongSide)
                {
                    barShortSide = int(barShortSide * shortSideRatio);
                }
                else if (barLongSide == deviceShortSide)
                {
                    barShortSide = int(barShortSide * longSideRatio);
                }

                if (i == 1)
                {
                    statusBarHeight = barShortSide;
                }
                else
                {
                    navigationBarHeight = barShortSide;
                }
            }
        }
        return result;
    }

    bool AndroidTargetDevice::ParsePackageVersionString(const vector<string> &lines, string &version)
    {
        version = "";

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (boost::starts_with(line, "versionName="))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of("="));
                if(items.size() > 1)
                {
                    version = items[1];
                    return true;
                }
            }
        }

        return false;
    }

    bool AndroidTargetDevice::ParseDPIString(const vector<string> &lines, string &dpi)
    {
        dpi = "";

        if (lines.empty())
            return false;

        string physicalDensityStr = "Physical density:";
        string overrideDensityStr = "Override density:";

        string physicalDensity = "", overrideDensity = "";

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (boost::starts_with(line, physicalDensityStr))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if(items.size() > 1)
                {
                    physicalDensity = boost::trim_copy(items[1]);
                }
            }
            else if (boost::starts_with(line, overrideDensityStr))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if(items.size() > 1)
                {
                    overrideDensity = boost::trim_copy(items[1]);
                }
            }
        }

        if(!boost::equals(overrideDensity, ""))
        {
            dpi = overrideDensity;
        }
        else
        {
            dpi = physicalDensity;
        }

        if(boost::empty(dpi))
            return false;
        else
            return true;
    }

    bool AndroidTargetDevice::ParsePackageTopInfoString(const vector<string> &lines, const string &packageName, string &topInfo)
    {
        topInfo = " PID    PR    CPU%    S    #THR    VSS    RSS    PCY    UID    Name \n";

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (line.find(packageName) != string::npos)
            {
                topInfo += line;
                return true;
            }
        }

        return false;
    }

    bool AndroidTargetDevice::GetInfoFromTargetString(const vector<std::string> &lines, const string &barstyleMatchStr, Point &pt, int &width, int &height)
    {
        bool found = false, foundFinal = false;
        const string visibilityStr("isReadyForDisplay()=true"), visibleRectStr("visible=["), windowStr("Window #");

        pt.x = 0;
        pt.y = 0;
        width = 0;
        height = 0;
        string matchStr = " " + barstyleMatchStr; // Avoid conflicts with mAttachedWindow=Window{42af64d8 - (same focus window)

        for (auto& line : lines)
        {
            if (!found && !foundFinal)
            {
                auto pos = line.find(matchStr);
                if (pos != string::npos)
                {
                    found = true;
                    continue;
                }
            }
            else if (found && !foundFinal)
            {
                auto pos = line.find(windowStr);
                if (pos != string::npos)
                {
                    foundFinal = false;
                    break;
                }

                pos = line.find(visibilityStr);
                if (pos != string::npos)
                {
                    foundFinal = true;
                    continue;
                }
            }
            else if (found && foundFinal)
            {
                auto pos = line.find(visibleRectStr);
                if (pos == string::npos)
                    continue;
                auto begPos = pos + visibleRectStr.size();
                pos = line.find(',', begPos);
                if (pos == string::npos)
                    break;
                auto lxStr = line.substr(begPos, pos - begPos);
                int lx = 0;
                if (TryParse(lxStr, lx))
                    pt.x = lx;

                begPos = pos + 1;
                pos = line.find(']', begPos);
                if (pos == string::npos)
                    break;
                auto lyStr = line.substr(begPos, pos - begPos);
                int ly = 0;
                if (TryParse(lyStr, ly))
                    pt.y = ly;

                begPos = pos + 2;
                pos = line.find(',', begPos);
                if (pos == string::npos)
                    break;
                auto rxStr = line.substr(begPos, pos - begPos);
                int w = 0;
                if (TryParse(rxStr, w))
                    width = w - lx;

                begPos = pos + 1;
                pos = line.find(']', begPos);
                if (pos == string::npos)
                    break;
                auto ryStr = line.substr(begPos, pos - begPos);
                int h = 0;
                if (TryParse(ryStr, h))
                    height = h - ly;

                break;
            }
        }

        return foundFinal;
    }

    bool AndroidTargetDevice::ParseTopActivityNameString(const vector<string> &lines, string &topActivityName)
    {
        topActivityName = "";
        int packageIndex = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (boost::starts_with(line, "ACTIVITY"))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(" "));
                if(items.size() > 1)
                    topActivityName = items[1];

                if(boost::contains(topActivityName, "/"))
                {
                    topActivityName = topActivityName.substr(topActivityName.find("/")+1);
                }

                packageIndex = index;
                break;
            }
        }

        for(int index = packageIndex + 1; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if(boost::contains(line, "java.io.IOException") || boost::contains(line, "Timeout"))
            {
                // Some apps dump the correct package name with exception and timeout: com.chaozh.iReaderFree
                // Practice shows those apps cannot be launched
                topActivityName = "";
            }
        }

        DAVINCI_LOG_DEBUG << "Dumped top activity: " << topActivityName;
        return true;
    }

    bool AndroidTargetDevice::ParseTopPackageNameString(const vector<string> &lines, string &topPackageName)
    {
        topPackageName = "";
        int packageIndex = 0;

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (boost::starts_with(line, "ACTIVITY"))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(" "));
                if(items.size() > 1)
                    topPackageName = items[1];

                if(boost::contains(topPackageName, "/"))
                {
                    topPackageName = topPackageName.substr(0, topPackageName.find("/"));
                }

                packageIndex = index;
                break;
            }
        }

        for(int index = packageIndex + 1; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if(boost::contains(line, "java.io.IOException") || boost::contains(line, "Timeout"))
            {
                // Some apps dump the correct package name with exception and timeout: com.chaozh.iReaderFree
                // Practice shows those apps cannot be launched
                topPackageName = "";
            }
        }

        DAVINCI_LOG_DEBUG << "Dumped top package: " << topPackageName;
        return true;
    }

    bool AndroidTargetDevice::ParsePackageNamesString(const vector<string> &lines, vector<string> &allPackageNames)
    {
        allPackageNames.clear();

        if (lines.empty())
            return false;

        for(int index = 0; index < (int)(lines.size()); index++)
        {
            string line = lines[index];
            boost::trim(line);
            if (boost::starts_with(line, "package"))
            {
                vector<string> items;
                boost::algorithm::split(items, line, boost::algorithm::is_any_of(":"));
                if ((items.size() >= 2) && (items[1] != ""))
                    allPackageNames.push_back(items[1]);
            }
        }

        return true;
    }

    bool AndroidTargetDevice::ParsePackageActivityString(const vector<string> &dumpLines, const vector<string> &allLines, vector<string> &list)
    {
        DaVinciStatus status;
        string package, activity;
        string iconName;

        if (dumpLines.empty() && allLines.empty())
        {
            list.push_back("");
            list.push_back("");
            list.push_back("");
            return false;
        }

        if (!dumpLines.empty())
        {
            string matchingStr = "package: name=";
            int i, index = 0;
            string line;
            for (i = 0; i < static_cast<int>(dumpLines.size()); i++)
            {
                line = dumpLines[i];

                // package: name='xx'
                if (boost::starts_with(line, matchingStr))
                {
                    index = i;
                    line = line.substr(matchingStr.length() + 1);
                    package = line.substr(0, line.find("'"));
                    break;
                }
            }

            bool iconFound = false;
            bool nextLine = false;

            for (i = index; i < static_cast<int>(dumpLines.size()); i++)
            {
                line = dumpLines[i];

                if (!iconFound)
                {
                    // application-label:'xx'
                    matchingStr = "application-label:";
                }

                if (!iconFound && boost::starts_with(line, matchingStr))
                {
                    line = line.substr(matchingStr.length() + 1);
                    if (line.find("'") != string::npos)
                    {
                        iconName = line.substr(0, line.find("'"));
                    }
                    else
                    {
                        iconName = line;
                    }
                    matchingStr = "launchable-activity: name=";
                    iconFound = true;
                    continue;
                }

                // launchable-activity: name='yy'
                // leanback-launchable-activity: name='' (TV)
                if (!nextLine)
                {
                    if (line.find(matchingStr) != string::npos)
                    {
                        line = line.substr(line.find(matchingStr) + matchingStr.length() + 1);
                        activity = line.substr(0, line.find("'"));

                        string temp = "";
                        matchingStr = "label=";
                        if (line.find(matchingStr) != string::npos)
                        {
                            line = line.substr(line.find(matchingStr) + matchingStr.length() + 1);
                            if (line.find("'") != string::npos)
                            {
                                temp = line.substr(0, line.find("'"));
                                if (temp != "")
                                {
                                    iconName = temp;
                                }

                                break;
                            }
                        }
                        else
                        {
                            nextLine = true;
                            continue;
                        }
                    }
                }
                else
                {
                    string temp;
                    matchingStr = "label=";
                    if (line.find(matchingStr) != string::npos)
                    {
                        line = line.substr(line.find(matchingStr) + matchingStr.length() + 1);
                        temp = line.substr(0, line.find("'"));
                        if (!temp.empty())
                        {
                            iconName = temp;
                        }

                        break;
                    }
                }
            }

        }

        // Handle other apps which launchable-activity is empty
        if (activity.empty())
        {
            if (allLines.size() > 0)
            {
                string package_match_str = "A: package=";
                string activity_match_str = "E: activity-alias";
                for (int i = 0; i < static_cast<int>(allLines.size()) - 1; i++)
                {
                    string line = boost::algorithm::trim_copy(allLines[i]);
                    string next_line = boost::algorithm::trim_copy(allLines[i + 1]);

                    if (boost::starts_with(line, package_match_str))
                    {
                        line = line.substr(package_match_str.length() + 1);
                        package = line.substr(0, line.find("\""));
                    }
                    else
                    {
                        if (boost::starts_with(line, activity_match_str))
                        {
                            activity_match_str = "A: android:name";
                            if (boost::starts_with(next_line, activity_match_str))
                            {
                                next_line = next_line.substr(next_line.find("=") + 2);
                                activity = next_line.substr(0, next_line.find("\""));
                                break;
                            }
                        }
                    }

                }
            }
        }

        if (package.empty() && activity.empty())
        {
            DAVINCI_LOG_WARNING << "Package is likely broken (cannot get package and activity name from aapt).";
        }

        list.push_back(package);
        list.push_back(activity);
        list.push_back(iconName);

        return true;
    }

    bool AndroidTargetDevice::ParseScreenCaptureString(const vector<char> &inputData, const Orientation &orientation, bool needResize, Mat &screenCapMat)
    {
        vector<char> imageData;
        int carriage = 0;

        if (inputData.empty())
            return false;

        for (auto c = inputData.begin(); c != inputData.end(); ++c)
        {
            if (carriage == 2)
            {
                if (*c != 0x0a && *c != 0x0d)
                {
                    imageData.push_back(0xd);
                    imageData.push_back(0xd);
                    carriage = 0;
                }
                else if (*c == 0x0a)
                {
                    carriage = 0;
                }
                imageData.push_back(*c);
            }
            else
            {
                if (*c == 0xd)
                {
                    carriage++;
                }
                else
                {
                    if (carriage == 1)
                    {
                        imageData.push_back(0xd);
                        carriage = 0;
                    }
                    imageData.push_back(*c);
                }
            }
        }

        for (; carriage > 0; carriage--)
        {
            imageData.push_back(0xd);
        }

        try
        {
            if (imageData.size() > 0)
            {
                screenCapMat = imdecode(imageData, CV_LOAD_IMAGE_COLOR);

                if(!screenCapMat.empty())
                {
                    Size screenCapSize = screenCapMat.size();
                    DebugSaveImage(screenCapMat, "screencap.png");

                    if(needResize)
                    {
                        int width = max(screenCapSize.width, screenCapSize.height);
                        int height = min(screenCapSize.width, screenCapSize.height);
                        if(width != 1280)
                        {
                            height = 1280 * height / width;
                            Size newSize;
                            if (screenCapSize.width > screenCapSize.height)
                            {
                                newSize = Size(1280, height);
                            }
                            else
                            {
                                newSize = Size(height, 1280);
                            }
                            resize(screenCapMat, screenCapMat, newSize, 0, 0, CV_INTER_CUBIC);
                            DebugSaveImage(screenCapMat, "screencap_resized.png");
                        }
                    }

                    if (screenCapSize.width > screenCapSize.height) // All width > height on Four orientations, like FFRD
                    {
                        switch (orientation)
                        {
                        case OrientationPortrait:
                        case OrientationLandscape:
                            break;
                        case OrientationReversePortrait:
                        case OrientationReverseLandscape:
                            screenCapMat = RotateImage180(screenCapMat);
                            break;
                        default:
                            break;
                        }
                    }
                    else // All width <= height on Four orientations, like N7
                    {
                        switch (orientation)
                        {
                        case OrientationPortrait:
                        case OrientationLandscape:
                            screenCapMat = RotateImage270(screenCapMat);
                            break;
                        case OrientationReversePortrait:
                        case OrientationReverseLandscape:
                            screenCapMat = RotateImage90(screenCapMat);
                            break;
                        default:
                            screenCapMat = RotateImage270(screenCapMat);
                            break;
                        }
                    }
                    DebugSaveImage(screenCapMat, "screencap_rotated.png");
                }
                else
                {
                    DAVINCI_LOG_ERROR << "Screencap frame is empty.";
                }
            }
            else
            {
                DAVINCI_LOG_ERROR << "Screencap image data is empty.";
            }
        }
        catch (...)
        {
        }

        return true;
    }

    bool AndroidTargetDevice::ParseOrientationFromSurfaceFlingerString(const vector<string> &lines, string &orientation)
    {
        string orientationMatchStr = "orientation=", displayMatchStr=",";
        int targetIndex = 0;
        bool findStr = false;

        if (lines.empty())
            return false;

        for (int index = 0; index < (int)lines.size(); index++)
        {
            if (boost::contains(lines[index], orientationMatchStr))
            {
                targetIndex = index;
                findStr = true;
                break;
            }
        }

        if (findStr == false)
            return false;

        string targetLine = lines[targetIndex];
        int startIndex = (int)targetLine.find(orientationMatchStr);
        int endIndex = (int)targetLine.find(displayMatchStr);
        std::string subStr = targetLine.substr(startIndex, endIndex - startIndex);
        int valueIndex = (int)subStr.find_first_of("=");
        orientation = subStr.substr(valueIndex+1);

        return true;
    }

    Point AndroidTargetDevice::ConvertToDevicePoint(int x, int y, Orientation orientation)
    {
        CheckCoordinate(x, y);

        Point ret;

        if (IsHorizontalOrientation(orientation))
        {
            ret.x = (x * deviceHeight)/Q_COORDINATE_Y;
            ret.y = (y * deviceWidth)/Q_COORDINATE_X;
        }
        else
        {
            ret.x = (x * deviceWidth)/Q_COORDINATE_X;
            ret.y = (y * deviceHeight)/Q_COORDINATE_Y;
        }

        return ret;
    }

}
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

#include "TestManager.hpp"
#include "ScriptRecorder.hpp"
#include "AudioManager.hpp"
#include "DeviceManager.hpp"
#include "assert.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace DaVinci
{
    ScriptRecorder::ScriptRecorder(const string &script) : ScriptEngineBase(script), waveFileId(0), inAudioRecording(false), treeId(0), delay(0)
    {
    }

    bool ScriptRecorder::Init()
    {
        SetName("ScriptRecorder - " + qsFileName);

        qs = QScript::New(qsFileName);
        if (qs == nullptr)
        {
            DAVINCI_LOG_ERROR << "Unable to create qs file " << qsFileName << " for recording";
            return false;
        }
        videoFileName = qs->GetResourceFullPath(qs->GetScriptName() + ".avi");
        audioFileName = qs->GetResourceFullPath(qs->GetScriptName() + ".wav");
        qtsFileName = qs->GetResourceFullPath(qs->GetScriptName() + ".qts");


        boost::shared_ptr<CaptureDevice> captureDevice = DeviceManager::Instance().GetCurrentCaptureDevice();
        if (captureDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "No capture device available.";
        }
        else
        {
            CaptureDevice::Preset preset = captureDevice->GetPreset();
            string presetString;
            if (preset == CaptureDevice::Preset::HighSpeed)
            {
                presetString = "HIGH_SPEED";
            }
            else if (preset == CaptureDevice::Preset::AIResolution)
            {
                presetString = "MIDDLE_RES";
            }
            else
            {
                // by default, the preset is high resolution
                presetString = "HIGH_RES";
            }
            qs->SetConfiguration(QScript::ConfigCameraPresetting, presetString);
        }

        tstProjConfigData = TestManager::Instance().GetTestProjectConfigData();
        qs->SetConfiguration(QScript::ConfigApkName, tstProjConfigData.apkName);
        qs->SetConfiguration(QScript::ConfigActivityName, tstProjConfigData.activityName);
        qs->SetConfiguration(QScript::ConfigPackageName, tstProjConfigData.packageName);
        string pushDataSource = qs->ProcessQuoteSpaces(tstProjConfigData.pushDataSource);
        string pushDataTarget = qs->ProcessQuoteSpaces(tstProjConfigData.pushDataTarget);
        qs->SetConfiguration(QScript::ConfigPushData, pushDataSource + " " + pushDataTarget);

        qsWatch.Start();
        dut = DeviceManager::Instance().GetCurrentTargetDevice();
        // configure start orientation correctly if we have such configuration
        if ((dut == nullptr) && (DeviceManager::Instance().GetEnableTargetDeviceAgent()))
            DAVINCI_LOG_ERROR << "Failed to get current target device.";
        else
        {
            if (dut->GetDefaultMultiLayerMode())
                qs->SetConfiguration(QScript::ConfigMultiLayerMode, "True");
            else 
                qs->SetConfiguration(QScript::ConfigMultiLayerMode, "False");

            qs->SetConfiguration(QScript::ConfigStatusBarHeight, boost::lexical_cast<string>(dut->GetStatusBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetStatusBarHeight()[1]));
            qs->SetConfiguration(QScript::ConfigNavigationBarHeight, boost::lexical_cast<string>(dut->GetNavigationBarHeight()[0]) + "-" + boost::lexical_cast<string>(dut->GetNavigationBarHeight()[1]));

            qs->SetConfiguration(QScript::ConfigStartOrientation, OrientationToString(dut->GetCurrentOrientation()));
            qs->SetConfiguration(QScript::ConfigOSversion, dut->GetOsVersion(true));
            qs->SetConfiguration(QScript::ConfigDPI, boost::lexical_cast<string>(dut->GetDPI()));
            qs->SetConfiguration(QScript::ConfigResolution, boost::lexical_cast<string>(dut->GetDeviceWidth()) + "x" + boost::lexical_cast<string>(dut->GetDeviceHeight()));
        }

        viewParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
        workWatch.Start();

        TestManager::Instance().RegisterUserActionListener(
            boost::dynamic_pointer_cast<TestManager::UserActionListener>(shared_from_this()));

        return ScriptEngineBase::Init();
    }

    void ScriptRecorder::Destroy()
    {
        if (inAudioRecording)
        {
            AudioManager::Instance().StopRecordFromUser();
            inAudioRecording = false;
        }
        TestManager::Instance().UnregisterUserActionListener(
            boost::dynamic_pointer_cast<TestManager::UserActionListener>(shared_from_this()));
        if (qs != nullptr)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_EXIT)));
        }

        if(TestManager::Instance().GetRecordWithID())
        {
            OnDeviceScriptRecorder::CalibrateIDAction(qs, qsFolder);
            TestManager::Instance().SetRecordWithID(0);
        }

        if (qs != nullptr)
            qs->Flush();

        ScriptEngineBase::Destroy();
    }

    void ScriptRecorder::OnKeyboard(bool isKeyDown, int keyCode)
    {
        qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_KEYBOARD_EVENT, isKeyDown ? 1 : 0, keyCode, 0, 0)));
    }

    void ScriptRecorder::RecordAdditionalInfo(int&x, int&y, string &info, const string &qsFileName, const string&packageName, boost::shared_ptr<TargetDevice> dut, int &treeId)
    {
        info = "";

        if(dut == nullptr)
            return;

        //double start = workWatch.ElapsedMilliseconds();
        Orientation deviceCurrentOrientation = dut->GetCurrentOrientation();
        Size deviceSize(dut->GetDeviceWidth(), dut->GetDeviceHeight());

        boost::filesystem::path qsPath = boost::filesystem::path(qsFileName);
        boost::filesystem::path qsFolder = qsPath.parent_path();
        TestManager::Instance().TransformImageBoxToDeviceCoordinate(deviceSize, deviceCurrentOrientation, x, y);
        vector<string> lines = dut->GetTopActivityViewHierarchy();
        string treeFileName = "record_" + packageName + ".qs_" + boost::lexical_cast<string>(++treeId) + ".tree";

        string treeFilePath = ConcatPath(qsFolder.string(), treeFileName);
        std::ofstream treeStream(treeFilePath, std::ios_base::app | std::ios_base::out);
        for(auto line: lines)
            treeStream << line << "\r\n" << std::flush;
        treeStream.flush();
        treeStream.close();

        boost::shared_ptr<ViewHierarchyParser> viewParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
        viewParser->ParseTree(lines);
        Rect rect;
        int groupIndex, itemIndex;

        boost::shared_ptr<AndroidTargetDevice> adut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        AndroidTargetDevice::WindowBarInfo windowBarInfo;
        vector<string> windowLines;
        if(adut != nullptr)
        {
            windowBarInfo = adut->GetWindowBarInfo(windowLines);
            string windowFileName = "record_" + packageName + ".qs_" + boost::lexical_cast<string>(treeId) + ".window";
            string windowFilePath = ConcatPath(qsFolder.string(), windowFileName);
            std::ofstream windowStream(windowFilePath, std::ios_base::app | std::ios_base::out);
            for(auto line: windowLines)
                windowStream << line << "\r\n" << std::flush;
            windowStream.flush();
            windowStream.close();
        }

        string id = viewParser->FindMatchedIdByPoint(Point(x, y), windowBarInfo.focusedWindowRect, rect, groupIndex, itemIndex);
        viewParser->CleanTreeControls();

        if(id != "")
        {
            info += "id=" + id;
            info += "(" + boost::lexical_cast<string>(groupIndex) + "-" + boost::lexical_cast<string>(itemIndex) + ")&";
            double ratioX = double(x - rect.x) / rect.width;
            double ratioY = double(y - rect.y) / rect.height;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(4);
            ss << ratioX;
            ss << ",";
            ss << ratioY;
            std::string ratioStr = ss.str();
            info += "ratio=(" + ratioStr + ")&";
            info += "layout=" + treeFileName + "&";

            std::ostringstream barss;
            barss << "sb=(";
            barss << windowBarInfo.statusbarRect.x;
            barss << ",";
            barss << windowBarInfo.statusbarRect.y;
            barss << ",";
            barss << (windowBarInfo.statusbarRect.x + windowBarInfo.statusbarRect.width);
            barss << ",";
            barss << (windowBarInfo.statusbarRect.y + windowBarInfo.statusbarRect.height);
            barss << ")&nb=(";
            barss << windowBarInfo.navigationbarRect.x;
            barss << ",";
            barss << windowBarInfo.navigationbarRect.y;
            barss << ",";
            barss << (windowBarInfo.navigationbarRect.x + windowBarInfo.navigationbarRect.width);
            barss << ",";
            barss << (windowBarInfo.navigationbarRect.y + windowBarInfo.navigationbarRect.height);
            barss << ")&kb=";
            if(windowBarInfo.inputmethodExist)
                barss << "1&fw=(";
            else
                barss << "0&fw=(";
            barss << windowBarInfo.focusedWindowRect.x;
            barss << ",";
            barss << windowBarInfo.focusedWindowRect.y;
            barss << ",";
            barss << (windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width);
            barss << ",";
            barss << (windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height);
            if(windowBarInfo.isPopUpWindow)
                barss << ")&popup=1";
            else
                barss << ")&popup=0";

            std::string windowStr = barss.str();
            info += windowStr;
        }
        else
        {
            info += "layout=" + treeFileName + "&";
            std::ostringstream ss;
            ss << "sb=(";
            ss << windowBarInfo.statusbarRect.x;
            ss << ",";
            ss << windowBarInfo.statusbarRect.y;
            ss << ",";
            ss << (windowBarInfo.statusbarRect.x + windowBarInfo.statusbarRect.width);
            ss << ",";
            ss << (windowBarInfo.statusbarRect.y + windowBarInfo.statusbarRect.height);
            ss << ")&nb=(";
            ss << windowBarInfo.navigationbarRect.x;
            ss << ",";
            ss << windowBarInfo.navigationbarRect.y;
            ss << ",";
            ss << (windowBarInfo.navigationbarRect.x + windowBarInfo.navigationbarRect.width);
            ss << ",";
            ss << (windowBarInfo.navigationbarRect.y + windowBarInfo.navigationbarRect.height);
            ss << ")&kb=";
            if(windowBarInfo.inputmethodExist)
                ss << "1&fw=(";
            else
                ss << "0&fw=(";
            ss << windowBarInfo.focusedWindowRect.x;
            ss << ",";
            ss << windowBarInfo.focusedWindowRect.y;
            ss << ",";
            ss << (windowBarInfo.focusedWindowRect.x + windowBarInfo.focusedWindowRect.width);
            ss << ",";
            ss << (windowBarInfo.focusedWindowRect.y + windowBarInfo.focusedWindowRect.height);
            if(windowBarInfo.isPopUpWindow)
                ss << ")&popup=1";
            else
                ss << ")&popup=0";

            std::string windowStr = ss.str();
            info += windowStr;
        }

        /*double end = workWatch.ElapsedMilliseconds();
        delay = int(end - start);*/
    }

    void ScriptRecorder::OnMouse(MouseButton button, MouseActionType actionType, int x, int y, int actionX, int actionY, int ptr, Orientation o)
    {
        boost::shared_ptr<QSEventAndAction> qsEvent;
        QSEventAndAction::MouseEventFlag mouseEventFlag = QSEventAndAction::MouseEventFlag::Wheel;
        bool isMouseEvent = false;

        switch (button)
        {
        case MouseButtonLeft:
            switch (actionType)
            {
            case MouseActionTypeDown:
                {
                    string info = "";
                    if(TestManager::Instance().GetRecordWithID()){
                        double start = workWatch.ElapsedMilliseconds();
                        RecordAdditionalInfo(actionX, actionY, info, qsFileName, tstProjConfigData.packageName, dut, treeId);
                        delay = int(workWatch.ElapsedMilliseconds() - start);                        
                    }
                        
                    qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                        QSEventAndAction::OPCODE_TOUCHDOWN, x, y, ptr, 0, info, o));
                    break;
                }
            case MouseActionTypeUp:
                {
                    qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds() + delay,
                        QSEventAndAction::OPCODE_TOUCHUP, x, y, ptr, 0, "", o));
                    delay = 0;
                    break;
                }
            case MouseActionTypeMove:
                {
                    qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds() + delay,
                        QSEventAndAction::OPCODE_TOUCHMOVE, x, y, ptr, 0, "", o));
                    break;
                }
            case MouseActionTypeWheel:
                {
                    isMouseEvent = true;
                    mouseEventFlag = QSEventAndAction::MouseEventFlag::Wheel;
                    break;
                }
            default:
                assert(false);
            }
            break;
        case MouseButtonRight:
            switch (actionType)
            {
            case MouseActionTypeDown:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::RightDown;
                break;
            case MouseActionTypeUp:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::RightUp;
                break;
            case MouseActionTypeMove:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::Move;
                break;
            case MouseActionTypeWheel:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::Wheel;
                break;
            default:
                assert(false);
            }
            break;
        case MouseButtonMiddle:
            switch (actionType)
            {
            case MouseActionTypeDown:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::MiddleDown;
                break;
            case MouseActionTypeUp:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::MiddleUp;
                break;
            case MouseActionTypeMove:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::Move;
                break;
            case MouseActionTypeWheel:
                isMouseEvent = true;
                mouseEventFlag = QSEventAndAction::MouseEventFlag::Wheel;
                break;
            default:
                assert(false);
            }
            break;
        default:
            break;
        }
        if (isMouseEvent)
        {
            assert(qsEvent == nullptr);
            qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_MOUSE_EVENT, static_cast<int>(mouseEventFlag), x, y, ptr));
        }
        if (qsEvent != nullptr)
        {
            qs->AppendEventAndAction(qsEvent);
        }
    }

    void ScriptRecorder::OnTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2)
    {
        int opcode1 = QSEventAndAction::OPCODE_INVALID;
        int opcode2 = QSEventAndAction::OPCODE_INVALID;
        switch (action)
        {
        case TiltActionLeft:
            opcode1 = QSEventAndAction::OPCODE_TILTLEFT;
            break;
        case TiltActionRight:
            opcode1 = QSEventAndAction::OPCODE_TILTRIGHT;
            break;
        case TiltActionDown:
            opcode1 = QSEventAndAction::OPCODE_TILTDOWN;
            break;
        case TiltActionUp:
            opcode1 = QSEventAndAction::OPCODE_TILTUP;
            break;
        case TiltActionTo:
            opcode1 = QSEventAndAction::OPCODE_TILTTO;
            break;
        case TiltActionUpLeft:
            opcode1 = QSEventAndAction::OPCODE_TILTUP;
            opcode2 = QSEventAndAction::OPCODE_TILTLEFT;
            break;
        case TiltActionUpRight:
            opcode1 = QSEventAndAction::OPCODE_TILTUP;
            opcode2 = QSEventAndAction::OPCODE_TILTRIGHT;
            break;
        case TiltActionDownLeft:
            opcode1 = QSEventAndAction::OPCODE_TILTDOWN;
            opcode2 = QSEventAndAction::OPCODE_TILTLEFT;
            break;
        case TiltActionDownRight:
            opcode1 = QSEventAndAction::OPCODE_TILTDOWN;
            opcode2 = QSEventAndAction::OPCODE_TILTRIGHT;
            break;
        case TiltActionReset:
            degree1 = HWAccessoryController::defaultTiltDegree;
            degree2 = HWAccessoryController::defaultTiltDegree;
            opcode1 = QSEventAndAction::OPCODE_TILTTO;
            break;
        }
        if (opcode1 != QSEventAndAction::OPCODE_INVALID)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode1, degree1, degree2, speed1, speed2)));
        }
        if (opcode2 != QSEventAndAction::OPCODE_INVALID)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode2, degree1, degree2, speed1, speed2)));
        }
    }

    void ScriptRecorder::OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        switch (zAxisAction)
        {
        case ZAXISTiltActionClockwise:
            opcode = QSEventAndAction::OPCODE_TILTCLOCKWISE;
            break;
        case ZAXISTiltActionAnticlockwise:
            opcode = QSEventAndAction::OPCODE_TILTANTICLOCKWISE;
            break;
        case ZAXISTiltActionTo:
            opcode = QSEventAndAction::OPCODE_TILTZAXISTO;
            break;
        case ZAXISTiltActionReset:
            degree = HWAccessoryController::defaultZAxisTiltDegree;
            opcode = QSEventAndAction::OPCODE_TILTZAXISTO;
            break;
        }
        if (opcode != QSEventAndAction::OPCODE_INVALID)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, degree, speed, 0, 0)));
        }
    }

    void ScriptRecorder::OnAppAction(AppLifeCycleAction action, string info1, string info2)
    {
        bool needOperand=  true;
        info1 = qs->ProcessQuoteSpaces(info1);
        info2 = qs->ProcessQuoteSpaces(info2);
        string infoCombine = info1;
        if (info2 != "")
        {
            infoCombine = info1 + " " + info2;
        }

        int opcode = QSEventAndAction::OPCODE_INVALID;
        boost::filesystem::path absPathApk;
        boost::filesystem::path absPathQs;
        switch (action)
        {
        case AppActionUninstall:
            opcode = QSEventAndAction::OPCODE_UNINSTALL_APP;
            if (info1 == tstProjConfigData.packageName)
            {
                needOperand = false;
            }
            break;
        case AppActionInstall:
            opcode = QSEventAndAction::OPCODE_INSTALL_APP;
            absPathApk = boost::filesystem::path(qs->GetResourceFullPath(info1));
            absPathQs = boost::filesystem::path(qsFileName);
            if (qs->GetResourceFullPath(info1) == qs->GetResourceFullPath(tstProjConfigData.apkName))
            {
                needOperand = false;
            }
            if (absPathApk.parent_path() == absPathQs.parent_path())
            {
                infoCombine = absPathApk.filename().string();
            }
            break;
        case AppActionPushData:
            opcode = QSEventAndAction::OPCODE_PUSH_DATA;
            break;
        case AppActionClearData:
            opcode = QSEventAndAction::OPCODE_CLEAR_DATA;
            if (info1 == tstProjConfigData.packageName)
            {
                needOperand = false;
            }
            break;
        case AppActionStart:
            opcode = QSEventAndAction::OPCODE_START_APP;
            if (info1 == (tstProjConfigData.packageName + "/" + tstProjConfigData.activityName))
            {
                needOperand = false;
            }
            break;
        case AppActionStop:
            opcode = QSEventAndAction::OPCODE_STOP_APP;
            if (info1 == tstProjConfigData.packageName)
            {
                needOperand = false;
            }
            break;
        default:
            DAVINCI_LOG_ERROR << "Ignore the unknown app action: " << (int)action;
            return;
        }
        if (needOperand == true)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, infoCombine)));
        }
        else
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode)));
        }
    }

    void ScriptRecorder::OnSetText(const string &text)
    {
        // Add double quotatation to avoid the spaces split issue
        string textSet = text;
        // Pre-process the double quotation in input string        boost::replace_all(textSet, "\"","\"\"");
        textSet = "\"" + textSet + "\"";
        qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_SET_TEXT, textSet)));
    }

    void ScriptRecorder::OnOcr()
    {
        qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
            QSEventAndAction::OPCODE_OCR)));
    }

    void ScriptRecorder::OnSwipe(SwipeAction action)
    {
        int x = -1, y = -1;
        switch (action)
        {
        case SwipeActionLeft:
            x = 1024;
            y = 2048;
            break;
        case SwipeActionRight:
            x = 3072;
            y = 2048;
            break;
        case SwipeActionUp:
            x = 2048;
            y = 1024;
            break;
        case SwipeActionDown:
            x = 2048;
            y = 3072;
            break;
        default:
            x = -1;
            y = -1;
            break;
        }
        if (x != -1)
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                QSEventAndAction::OPCODE_SWIPE, 2048, 2048, x, y)));
        }
    }

    void ScriptRecorder::OnButton(ButtonAction action, ButtonEventMode mode)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        int parameter = 2;

        switch (action)
        {
        case ButtonActionBack:
            opcode = QSEventAndAction::OPCODE_BACK;
            break;
        case ButtonActionMenu:
            opcode = QSEventAndAction::OPCODE_MENU;
            break;
        case ButtonActionHome:
            opcode = QSEventAndAction::OPCODE_HOME;
            break;
        case ButtonActionLightUp:
            opcode = QSEventAndAction::OPCODE_LIGHT_UP;
            break;
        case ButtonActionVolumeDown:
            opcode = QSEventAndAction::OPCODE_VOLUME_DOWN;
            break;
        case ButtonActionVolumeUp:
            opcode = QSEventAndAction::OPCODE_VOLUME_UP;
            break;
        case ButtonActionPower:
            opcode = QSEventAndAction::OPCODE_POWER;
            break;
        case ButtonActionBrightnessDown:
            opcode = QSEventAndAction::OPCODE_BRIGHTNESS;
            break;
        case ButtonActionBrightnessUp:
            opcode = QSEventAndAction::OPCODE_BRIGHTNESS;
            break;
        case ButtonActionDumpUiLayout:
            opcode = QSEventAndAction::OPCODE_DUMP_UI_LAYOUT;
            break;
        default:
            opcode = QSEventAndAction::OPCODE_INVALID;
            break;
        }

        switch (mode)
        {
        case ButtonEventMode::Up:
            parameter = 0;
            break;
        case ButtonEventMode::Down:
            parameter = 1;
            break;
        case ButtonEventMode::DownAndUp:
            parameter = 2;
            break;
        }

        // Need to distingish brightness up and down from OPCODE_BRIGHTNESS
        if (opcode == QSEventAndAction::OPCODE_BRIGHTNESS)
        {
            if (action == ButtonActionBrightnessUp)
            {
                parameter = 0;
            }
            else
            {
                parameter = 1;
            }
        }
        string dumpFileName = "";
        if (opcode == QSEventAndAction::OPCODE_DUMP_UI_LAYOUT)
        {
            char timeStr[20];
            time_t rawtime;
            struct tm timeinfo;
            time (&rawtime);
            localtime_s(&timeinfo,&rawtime);
            strftime(timeStr ,sizeof(timeStr) ,"%Y%m%d%H%M%S",&timeinfo);
            dumpFileName = "UiLayout_" + string(timeStr) + ".xml";
        }
        if (opcode != QSEventAndAction::OPCODE_INVALID)
        {
            vector<int> btnIntOperands;
            btnIntOperands.push_back(parameter);
            vector<string> btnStrOperands;
            btnStrOperands.push_back(dumpFileName);
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, btnIntOperands, btnStrOperands)));
        }
    }

    void ScriptRecorder::OnMoveHolder(int distance)
    {
        qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
            distance >= 0 ? QSEventAndAction::OPCODE_HOLDER_UP : QSEventAndAction::OPCODE_HOLDER_DOWN,
            abs(distance), 0, 0, 0)));
    }

    void ScriptRecorder::OnPowerButtonPusher(PowerButtonPusherAction action)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        int operand = 4;
        switch (action)
        {
        case PowerButtonPusherPress:
            opcode = QSEventAndAction::OPCODE_DEVICE_POWER;
            operand = 0;
            break;
        case PowerButtonPusherRelease:
            opcode = QSEventAndAction::OPCODE_DEVICE_POWER;
            operand = 1;
            break; 
        case PowerButtonPusherShortPress:
            opcode = QSEventAndAction::OPCODE_DEVICE_POWER;
            operand = 2;
            break;
        case PowerButtonPusherLongPress:
            opcode = QSEventAndAction::OPCODE_DEVICE_POWER;
            operand = 3;
            break;
        }
        if ((opcode != QSEventAndAction::OPCODE_INVALID) && (operand != 4))
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, operand, 0, 0, 0)));
        }
    }

    void ScriptRecorder::OnUsbSwitch(UsbSwitchAction action)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        int port = 0;
        switch (action)
        {
        case UsbOnePlugIn:
            opcode = QSEventAndAction::OPCODE_USB_IN;
            port = 1;
            break;
        case UsbOnePullOut:
            opcode = QSEventAndAction::OPCODE_USB_OUT;
            port = 1;
            break; 
        case UsbTwoPlugIn:
            opcode = QSEventAndAction::OPCODE_USB_IN;
            port = 2;
            break;
        case UsbTwoPullOut:
            opcode = QSEventAndAction::OPCODE_USB_OUT;
            port = 2;
            break;
        }
        if ((opcode != QSEventAndAction::OPCODE_INVALID) && (port != 0))
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, port, 0, 0, 0)));
        }
    }

    void ScriptRecorder::OnEarphonePuller(EarphonePullerAction action)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        int port = 0;
        switch (action)
        {
        case EarphonePullerPlugIn:
            opcode = QSEventAndAction::OPCODE_EARPHONE_IN;
            port = 1;
            break;
        case EarphonePullerPullOut:
            opcode = QSEventAndAction::OPCODE_EARPHONE_OUT;
            port = 1;
            break;
        }
        if ((opcode != QSEventAndAction::OPCODE_INVALID) && (port != 0))
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, port, 0, 0, 0)));
        }
    }

    void ScriptRecorder::OnRelayController(RelayControllerAction action)
    {
        int opcode = QSEventAndAction::OPCODE_INVALID;
        int port = 0;
        switch (action)
        {
        case RelayOneConnect:
            opcode = QSEventAndAction::OPCODE_RELAY_CONNECT;
            port = 1;
            break;
        case RelayOneDisconnect:
            opcode = QSEventAndAction::OPCODE_RELAY_DISCONNECT;
            port = 1;
            break;
        case RelayTwoConnect:
            opcode = QSEventAndAction::OPCODE_RELAY_CONNECT;
            port = 2;
            break;
        case RelayTwoDisconnect:
            opcode = QSEventAndAction::OPCODE_RELAY_DISCONNECT;
            port = 2;
            break;
        case RelayThreeConnect:
            opcode = QSEventAndAction::OPCODE_RELAY_CONNECT;
            port = 3;
            break;
        case RelayThreeDisconnect:
            opcode = QSEventAndAction::OPCODE_RELAY_DISCONNECT;
            port = 3;
            break;
        case RelayFourConnect:
            opcode = QSEventAndAction::OPCODE_RELAY_CONNECT;
            port = 4;
            break;
        case RelayFourDisconnect:
            opcode = QSEventAndAction::OPCODE_RELAY_DISCONNECT;
            port = 4;
            break;
        }
        if ((opcode != QSEventAndAction::OPCODE_INVALID) && (port != 0))
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                opcode, port, 0, 0, 0)));
        }
    }

    void ScriptRecorder::OnAudioRecord(bool startRecord)
    {
        if (inAudioRecording == startRecord) return;

        string wavFileName(qs->GetScriptName() + "_" + boost::lexical_cast<string>(waveFileId) + ".wav");
        DaVinciStatus status;
        if (startRecord)
        {
            status = AudioManager::Instance().StartRecordFromUser(qs->GetResourceFullPath(wavFileName));
            if (DaVinciSuccess(status))
            {
                inAudioRecording = true;
            }
        }
        else
        {
            AudioManager::Instance().StopRecordFromUser();
            inAudioRecording = false;
            waveFileId++;
        }
        if (DaVinciSuccess(status))
        {
            qs->AppendEventAndAction(boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(qsWatch.ElapsedMilliseconds(),
                startRecord ? QSEventAndAction::OPCODE_AUDIO_RECORD_START : QSEventAndAction::OPCODE_AUDIO_RECORD_STOP, 
                wavFileName)));
        }
    }
}
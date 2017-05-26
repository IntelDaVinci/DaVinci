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

#include "ScriptHelper.hpp"
#include "SerializationUtil.hpp"
#include "VideoCaptureProxy.hpp"
#include "ScriptReplayer.hpp"
#include "VideoWriterProxy.hpp"

namespace DaVinci
{
    using namespace boost::filesystem;
    using namespace std;

    ScriptHelper::ScriptHelper()
    {
        Init();
    }

    ScriptHelper::ScriptHelper(const boost::shared_ptr<QScript> &qs) : qsPtr(qs)
    {
        Init();
    }

    void ScriptHelper::Init()
    {
        shapeExtractor = boost::shared_ptr<ShapeExtractor>(new ShapeExtractor());
        keyboardRecognizer = boost::shared_ptr<KeyboardRecognizer>(new KeyboardRecognizer());
        barRecognizer = boost::shared_ptr<AndroidBarRecognizer>(new AndroidBarRecognizer());
        crossRecognizer = boost::shared_ptr<CrossRecognizer>(new CrossRecognizer());
    }

    void ScriptHelper::SetCurrentQS(const boost::shared_ptr<QScript> &qs)
    {
        qsPtr = qs;
    }

    vector<string> ScriptHelper::GetTargetWindows(vector<string>& lines, string packageName, string& focusWindow)
    {
        vector<string> windows;
        std::unordered_map<string, string> map = AndroidTargetDevice::GetWindows(lines, focusWindow);

        for(auto pair : map)
        {
            if(boost::starts_with(pair.second, packageName))
            {
                windows.push_back(pair.second);
            }
        }

        return windows;
    }

    map<int, ScriptHelper::QSPreprocessInfo> ScriptHelper::GenerateQSPreprocessInfo(string folder, string matchPattern)
    {
        map<int, ScriptHelper::QSPreprocessInfo> qsInfos;
        vector<double> timeStampList;

        if (qsPtr != nullptr)
        {
            timeStampList = qsPtr->TimeStamp();
        }
        else
        {
            DAVINCI_LOG_ERROR << "Cannot load Q script file." << endl;
        }

        if (timeStampList.size() == 0)
        {
            return qsInfos;
        }

        int bottomNavigationBarHeight = qsPtr->GetNavigationBarHeight()[0];
        int rightNavigationBarHeight = qsPtr->GetNavigationBarHeight()[1];
        int bottomStatusBarHeight = qsPtr->GetStatusBarHeight()[0];
        int rightStatusBarHeight = qsPtr->GetStatusBarHeight()[1];
        int navigationBarHeight = bottomNavigationBarHeight;
        int statusBarHeight = bottomStatusBarHeight;
        string osVersion = qsPtr->GetOSversion();
        string recordDeviceWidth = qsPtr->GetResolutionWidth();
        string recordDeviceHeight = qsPtr->GetResolutionHeight();
        Size deviceSize;
        bool isParsed = TryParse(recordDeviceWidth, deviceSize.width);
        assert(isParsed);
        isParsed = TryParse(recordDeviceHeight, deviceSize.height);
        assert(isParsed);

        string packageName = qsPtr->GetPackageName();
        string qsName = qsPtr->GetQsFileName();
        path scriptPath(qsName);
        string qsFilename = scriptPath.filename().string();

        string videoFile = qsPtr->GetVideoFileName();

        if (!boost::filesystem::exists(videoFile))
        {
            return qsInfos;
        }

        VideoCaptureProxy capture(videoFile);
        if (!capture.isOpened())
        {
            return qsInfos;
        }

        double maxFrameIndex = capture.get(CV_CAP_PROP_FRAME_COUNT);

        boost::filesystem::path directory = folder;
        if (directory.string() == ".")
        {
            directory = boost::filesystem::current_path();
        }

        if (!boost::filesystem::exists(directory))
        {
            boost::filesystem::create_directory(directory);
        }

        int line = 0;
        while (line <= (int)(qsPtr->NumEventAndActions()))
        {
            int start = line;
            int end = line;
            line += 1;
            int blockOpcodes[30][2] = {
                {QSEventAndAction::OPCODE_TOUCHDOWN, QSEventAndAction::OPCODE_TOUCHUP}, 
                {QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL, QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL},
                {QSEventAndAction::OPCODE_BACK, QSEventAndAction::OPCODE_BACK},
                {QSEventAndAction::OPCODE_HOME, QSEventAndAction::OPCODE_HOME},
                {QSEventAndAction::OPCODE_MENU, QSEventAndAction::OPCODE_MENU},
                {QSEventAndAction::OPCODE_SET_TEXT, QSEventAndAction::OPCODE_SET_TEXT},
                {QSEventAndAction::OPCODE_VOLUME_DOWN, QSEventAndAction::OPCODE_VOLUME_DOWN},
                {QSEventAndAction::OPCODE_VOLUME_UP, QSEventAndAction::OPCODE_VOLUME_UP},
                {QSEventAndAction::OPCODE_DEVICE_POWER, QSEventAndAction::OPCODE_DEVICE_POWER},
                {QSEventAndAction::OPCODE_BRIGHTNESS, QSEventAndAction::OPCODE_BRIGHTNESS},
                {QSEventAndAction::OPCODE_POWER, QSEventAndAction::OPCODE_POWER},
                {QSEventAndAction::OPCODE_LIGHT_UP, QSEventAndAction::OPCODE_LIGHT_UP},
                {QSEventAndAction::OPCODE_IMAGE_CHECK, QSEventAndAction::OPCODE_IMAGE_CHECK},
                {QSEventAndAction::OPCODE_INSTALL_APP, QSEventAndAction::OPCODE_INSTALL_APP},
                {QSEventAndAction::OPCODE_PUSH_DATA, QSEventAndAction::OPCODE_PUSH_DATA},
                {QSEventAndAction::OPCODE_CLEAR_DATA, QSEventAndAction::OPCODE_CLEAR_DATA},
                {QSEventAndAction::OPCODE_STOP_APP, QSEventAndAction::OPCODE_STOP_APP},
                {QSEventAndAction::OPCODE_UNINSTALL_APP, QSEventAndAction::OPCODE_UNINSTALL_APP},
                {QSEventAndAction::OPCODE_START_APP, QSEventAndAction::OPCODE_START_APP},
                {QSEventAndAction::OPCODE_MESSAGE, QSEventAndAction::OPCODE_MESSAGE},
                {QSEventAndAction::OPCODE_EXIT, QSEventAndAction::OPCODE_EXIT},
                {QSEventAndAction::OPCODE_FPS_MEASURE_START, QSEventAndAction::OPCODE_FPS_MEASURE_START},
                {QSEventAndAction::OPCODE_FPS_MEASURE_STOP, QSEventAndAction::OPCODE_FPS_MEASURE_STOP},
                {QSEventAndAction::OPCODE_DUMP_UI_LAYOUT, QSEventAndAction::OPCODE_DUMP_UI_LAYOUT},
                {QSEventAndAction::OPCODE_IF_MATCH_IMAGE, QSEventAndAction::OPCODE_IF_MATCH_IMAGE},
                {QSEventAndAction::OPCODE_CRASH_ON, QSEventAndAction::OPCODE_CRASH_ON},
                {QSEventAndAction::OPCODE_DRAG, QSEventAndAction::OPCODE_DRAG},
                {QSEventAndAction::OPCODE_CRASH_OFF, QSEventAndAction::OPCODE_CRASH_OFF},
                {QSEventAndAction::OPCODE_BUTTON, QSEventAndAction::OPCODE_BUTTON}
            };

            if (qsPtr->EventAndAction(start) == nullptr)
            {
                continue;
            }

            for (auto opcodes : blockOpcodes)
            {
                if (qsPtr->EventAndAction(start)->Opcode() == opcodes[0])
                {
                    int i = start;
                    while (i <= (int)(qsPtr->NumEventAndActions())) // Find touch up
                    {
                        line = i;
                        if (qsPtr->EventAndAction(i)->Opcode() == opcodes[1])
                        {
                            end = i;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                }
            }
            line = end + 1;

            size_t timeIndex = 0;
            for (timeIndex = 0; timeIndex < timeStampList.size() - 1; timeIndex++) // Find the corresponding timestamp
            {
                if (timeStampList[timeIndex] > qsPtr->EventAndAction(start)->TimeStamp())
                {
                    break;
                }
            }
            if (timeIndex > 0)
            {
                timeIndex--; // Position before the first action
            }

            double currentFrameIndex = static_cast<double>(timeIndex);
            if (FSmallerE(maxFrameIndex, currentFrameIndex))
            {
                DAVINCI_LOG_ERROR << "Exceed frame index at line #" << start << "!" << endl;
                break;
            }

            auto action = qsPtr->EventAndAction(start);
            int startQSIpOpcode = action->Opcode();
            capture.set(CV_CAP_PROP_POS_FRAMES, currentFrameIndex);
            Mat image;
            capture.read(image);

            if (image.empty())
            {
                DAVINCI_LOG_ERROR << "Null frame at line #" << start << "!" << endl;

                // Some script-end non-touch opcodes may map to empty frame
                if (QScript::IsOpcodeNonTouch(startQSIpOpcode))
                {
                    // Initialzie QS preprocess info
                    ScriptHelper::QSPreprocessInfo qsBlockInfo;
                    qsBlockInfo.endIp = end;
                    qsBlockInfo.startLine = action->LineNumber();
                    qsBlockInfo.endLine = qsBlockInfo.startLine + (end - start);

                    qsInfos[start] = qsBlockInfo;
                }
                continue;
            }

            Orientation orientation = GetActionOrientation(qsPtr, start);
            if (orientation == Orientation::Landscape || orientation == Orientation::ReverseLandscape)
            {
                navigationBarHeight = rightNavigationBarHeight;
                statusBarHeight = rightStatusBarHeight;
            }

            if (QScript::IsOpcodeTouch(startQSIpOpcode) || QScript::IsOpcodeNonTouch(startQSIpOpcode))
            {
                // Initialzie QS preprocess info
                ScriptHelper::QSPreprocessInfo qsBlockInfo;
                qsBlockInfo.endIp = end;
                qsBlockInfo.startLine = action->LineNumber();
                qsBlockInfo.endLine = qsBlockInfo.startLine + (end - start);

                Mat referenceFrame = image.clone();
                path resourcePath;

                for (int i = start; i <= end; i++)
                {
                    auto action = qsPtr->EventAndAction(i);
                    if (action == nullptr)
                    {
                        continue;
                    }
                    int x = action->IntOperand(0);
                    int y = action->IntOperand(1);
                    Point where = NormToFrameCoord(image.size(), Point(x, y), orientation); // Frame point

                    if (action->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN ||
                        action->Opcode() == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL)
                    {
                        ProcessTouchDownAction(qsBlockInfo, action->StrOperand(0), start, end, where, x, y, deviceSize, image, orientation, qsFilename, navigationBarHeight, statusBarHeight, directory, osVersion, matchPattern);
                        DrawCircle(referenceFrame, where, Yellow); // Yellow for touch down point
                    }
                    else if (action->Opcode() == QSEventAndAction::OPCODE_TOUCHMOVE ||
                        action->Opcode() == QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL)
                    {
                        // Process touch move
                        qsBlockInfo.isTouchMove = true;
                        DrawCircle(referenceFrame, where, Yellow);
                    }
                    else if (action->Opcode() == QSEventAndAction::OPCODE_TOUCHUP ||
                        action->Opcode() == QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL)
                    {
                        DrawCircle(referenceFrame, where, Yellow); // Yellow for touch up point
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_clickPoint.png");
                        SaveImage(referenceFrame, resourcePath.string());
                    }
                    else if (action->Opcode() == QSEventAndAction::OPCODE_HOME)
                    {
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_HOME;
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_" + QScript::OpcodeToString(action->Opcode()) + ".png");
                        SaveImage(image, resourcePath.string());
                    }
                    else if (startQSIpOpcode == QSEventAndAction::OPCODE_BACK)
                    {
                        qsBlockInfo.isAdvertisement = this->crossRecognizer->RecognizeCross(image);
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_BACK;
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_" + QScript::OpcodeToString(action->Opcode()) + ".png");
                        SaveImage(image, resourcePath.string());
                    }
                    else if (startQSIpOpcode == QSEventAndAction::OPCODE_MENU)
                    {
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_MENU;
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_" + QScript::OpcodeToString(action->Opcode()) + ".png");
                        SaveImage(image, resourcePath.string());
                    }
                    else if (startQSIpOpcode == QSEventAndAction::OPCODE_DRAG)
                    {
                        Point p1(x, y);
                        int x1 = action->IntOperand(2);
                        int y1 = action->IntOperand(3);
                        Point p2(x1, y1);
                        vector<Point> points;
                        points.push_back(p1);
                        points.push_back(p2);

                        SwipeDirection direction = GetDragDirection(points, orientation);
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference.png");
                        SaveImage(referenceFrame, resourcePath.string());

                        ExploratoryEngine::DrawPoints(referenceFrame, direction, Yellow, 5);
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_clickPoint.png");
                        SaveImage(referenceFrame, resourcePath.string());
                    }
                    else if (QScript::IsOpcodeNonTouch(startQSIpOpcode))
                    {
                        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference_" + QScript::OpcodeToString(action->Opcode()) + ".png");
                        SaveImage(image, resourcePath.string());
                    }
                    else
                    {
                        assert(false);
                        DAVINCI_LOG_ERROR << "Unsupported opcode in QSPreprocess: " + boost::lexical_cast<string>(startQSIpOpcode);
                    }
                }

                qsInfos[start] = qsBlockInfo;
            }
        }

        capture.release();

        return qsInfos;
    }

    SwipeDirection ScriptHelper::GetDragDirection(vector<Point>& points, Orientation orientation)
    {
        SwipeDirection direction = SwipeDirection::LEFT;
        if(points.size() != 2)
            return direction;

        Point p1 = points[0];
        Point p2 = points[1];

        if(p1.x == p2.x)
        {
            if(p1.y > p2.y)
            {
                direction = SwipeDirection::UP;
            }
            else
            {
                direction = SwipeDirection::DOWN;
            }
        }
        else
        {
            if(p1.x > p2.x)
            {
                direction = SwipeDirection::LEFT;
            }
            else
            {
                direction = SwipeDirection::RIGHT;
            }
        }

        // Correct direction to adapt the function call (rotated frame)
        if(orientation == DaVinci::Orientation::Portrait)
            direction = (SwipeDirection)((direction + 3) % 4);
        else if(orientation == DaVinci::Orientation::Landscape)
            direction = (SwipeDirection)((direction + 0) % 4);
        else if(orientation == DaVinci::Orientation::ReversePortrait)
            direction = (SwipeDirection)((direction + 1) % 4);
        else if(orientation == DaVinci::Orientation::ReverseLandscape)
            direction = (SwipeDirection)((direction + 2) % 4);
        else
            direction = (SwipeDirection)((direction + 3) % 4);

        return direction;
    }

    // Organize QS preprocessInfo for those keyboard QSEvents
    map<int, ScriptHelper::QSPreprocessInfo> ScriptHelper::OrganizeQSPreprocessInfoForKeyboard(map<int, ScriptHelper::QSPreprocessInfo> qsInfos)
    {
        map<int, ScriptHelper::QSPreprocessInfo> copyQSPreprocessInfo = qsInfos;
        vector<int> removedIps;
        string input;
        bool inputStart = false;
        for(auto pair :copyQSPreprocessInfo)
        {
            QSPreprocessInfo info = pair.second;

            if(!inputStart)
            {
                if(info.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Command)
                {
                    inputStart = true;
                    removedIps.push_back(pair.first);
                    input += info.keyboardInput;
                }
            }
            else
            {
                if(info.isVirtualKeyboard == KeyboardInputType::KeyboardInput_Command)
                {
                    removedIps.push_back(pair.first);
                    input += info.keyboardInput;
                }
                else
                {
                    inputStart = false;
                    int startInputIp = removedIps[0];
                    for(int i = 1; i < (int)removedIps.size(); i++)
                        qsInfos.erase(removedIps[i]);
                    QSPreprocessInfo info = qsInfos[startInputIp];
                    info.keyboardInput = input;
                    qsInfos[startInputIp] = info;

                    removedIps.clear();
                    input = "";
                }
            }
        }

        return qsInfos;
    }

    Rect ScriptHelper::StringToRect(string str)
    {
        Rect rect = EmptyRect;
        if(str.length() < 2)
            return rect;

        string rectStr = str.substr(1, str.length() - 2);
        vector<string> items;
        boost::algorithm::split(items, rectStr, boost::algorithm::is_any_of(","));
        if(items.size() != 4)
            return rect;

        TryParse(items[0], rect.x);
        TryParse(items[1], rect.y);
        TryParse(items[2], rect.width);
        TryParse(items[3], rect.height);

        rect.width = rect.width - rect.x;
        rect.height = rect.height - rect.y;

        return rect;
    }

    void ScriptHelper::ProcessWindowInfo(std::unordered_map<string, string> keyValueMap, Rect& sbRect, Rect& nbRect, bool& hasKeyboard, Rect& fwRect)
    {
        string sb = "sb", nb = "nb", kb = "kb", fw = "fw";
        hasKeyboard = false;
        sbRect = nbRect = fwRect = EmptyRect;

        if(keyValueMap.find(sb) != keyValueMap.end())
        {
            string sbStr = keyValueMap[sb];
            sbRect = StringToRect(sbStr);
        }
        if(keyValueMap.find(nb) != keyValueMap.end())
        {
            string nbStr = keyValueMap[nb];
            nbRect = StringToRect(nbStr);
        }
        if(keyValueMap.find(kb) != keyValueMap.end())
        {
            string kbStr = keyValueMap[kb];
            if(kbStr == "1")
                hasKeyboard = true;
        }
        if(keyValueMap.find(fw) != keyValueMap.end())
        {
            string fwStr = keyValueMap[fw];
            fwRect = StringToRect(fwStr);
        }
    }

    string ScriptHelper::FindAdditionalInfo(std::unordered_map<string, string> keyValueMap, string key)
    {
        string value = "";
        if(keyValueMap.find(key) != keyValueMap.end())
        {
            value = keyValueMap[key];
        }

        return value;
    }

    bool ScriptHelper::FillEnTexts(Mat textRoiFrame, Rect textRect, ScriptHelper::QSPreprocessInfo& qsBlockInfo)
    {
        string recognizedText = TextUtil::Instance().RecognizeEnglishText(textRoiFrame);
        recognizedText = TextUtil::Instance().PostProcessText(recognizedText);

        if (TextUtil::Instance().IsValidEnglishText(recognizedText))
        {
            qsBlockInfo.clickedTextRect = textRect;
            qsBlockInfo.clickedEnText = recognizedText;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ScriptHelper::FillZhTexts(Mat textRoiFrame, Rect textRect, ScriptHelper::QSPreprocessInfo& qsBlockInfo)
    {
        string recognizedText = TextUtil::Instance().RecognizeChineseText(textRoiFrame);
        recognizedText = TextUtil::Instance().PostProcessText(recognizedText);
        if (TextUtil::Instance().IsValidChineseText(recognizedText))
        {
            qsBlockInfo.clickedTextRect = textRect;
            qsBlockInfo.clickedZhText = recognizedText;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ScriptHelper::NeedShapePreprocess(string matchPattern)
    {
        if(boost::equals(matchPattern, ""))
            return true;

        if(matchPattern.find("shape") != -1)
            return true;

        return false;
    }

    bool ScriptHelper::NeedTextPreprocess(string matchPattern)
    {
        if(boost::equals(matchPattern, ""))
            return true;

        if(matchPattern.find("text") != -1)
            return true;

        return false;
    }

    void ScriptHelper::ProcessTouchDownAction(ScriptHelper::QSPreprocessInfo& qsBlockInfo, string keyStr, int start, int end, Point where, int x, int y, Size deviceSize, Mat image, Orientation orientation, string qsFilename, 
        int navigationBarHeight, int statusBarHeight, boost::filesystem::path directory, string osVersion, string matchPattern)
    {       
        qsBlockInfo.mainROI = cv::Rect(EmptyPoint, image.size()); // Updated if has bar
        qsBlockInfo.framePoint = where;

        qsBlockInfo.isTouchAction = true;

        // Process advertisement
        // FIXME: touch-move is mistaken for normal click
        if (end - start == 1)
        {
            qsBlockInfo.isAdvertisement = this->crossRecognizer->RecognizeCross(image, where);
        }

        Rect sbRect, nbRect, fwRect;
        bool hasKeyboardFromWindow;
        std::unordered_map<string, string> keyValueMap = ScriptReplayer::ParseKeyValues(keyStr);
        ProcessWindowInfo(keyValueMap, sbRect,  nbRect, hasKeyboardFromWindow, fwRect);

        qsBlockInfo.devicePoint = Point(x * deviceSize.width / maxWidthHeight, y * deviceSize.height / maxWidthHeight);
        qsBlockInfo.isPopUpWindow = ObjectUtil::Instance().IsPopUpDialog(deviceSize, orientation, sbRect, fwRect); // check popup upon window bar information
        // Keyboard will make the focus window looks like a pop up
        if(hasKeyboardFromWindow)
            qsBlockInfo.isPopUpWindow = false;

        string popUpStr = FindAdditionalInfo(keyValueMap, "popup");
        if(boost::equals(popUpStr, "1"))
            qsBlockInfo.isPopUpWindow = true;

        string packageName = qsPtr->GetPackageName();
        string layoutStr = FindAdditionalInfo(keyValueMap, "layout");
        boost::replace_first(layoutStr, ".tree", ".window");
        boost::filesystem::path parent = directory.parent_path();
        string windowFullPath = parent.string();
        windowFullPath = ConcatPath(windowFullPath, layoutStr);
        boost::filesystem::path windowPath(windowFullPath);

        if(boost::filesystem::is_regular_file(windowPath))
        {
            vector<string> windowsLines;
            ReadAllLines(windowFullPath, windowsLines);
            string focusWindow;
            vector<string> windows = ScriptHelper::GetTargetWindows(windowsLines, packageName, focusWindow);
            qsBlockInfo.windows = windows;
            qsBlockInfo.focusWindow = focusWindow;
        }

        string currentMatchPattern = matchPattern;
        string singeStepPattern = FindAdditionalInfo(keyValueMap, "match");
        boost::trim(singeStepPattern);
        if(!boost::equals(singeStepPattern, ""))
        {
            currentMatchPattern = singeStepPattern;
        }

        // Rotate frame for keyboard and navigation recognition
        Mat rotatedFrame = RotateFrameUp(image, orientation);
        Size rotatedFrameSize = rotatedFrame.size();
        Point rotatedPoint = TransformFramePointToRotatedFramePoint(rotatedFrameSize, where, orientation);
        Rect kbROIOnFrame = EmptyRect;

        string inputStr = FindAdditionalInfo(keyValueMap, "input");

        if (fwRect != EmptyRect)
        {
            Point fwPoint(fwRect.x, fwRect.y);
            qsBlockInfo.mainROI = AndroidTargetDevice::GetWindowRectangle(fwPoint, fwRect.width, fwRect.height, deviceSize, orientation, image.size().width);

            if (hasKeyboardFromWindow)
            {
                if(boost::equals(inputStr, "none"))
                {
                    qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_None;
                }
                else
                {
                    Rect kbROIOnDevice = ObjectUtil::Instance().GetKeyboardROI(orientation, deviceSize, nbRect, sbRect, fwRect);
                    Point kbROIPoint(kbROIOnDevice.x, kbROIOnDevice.y);
                    kbROIOnFrame = AndroidTargetDevice::GetWindowRectangle(kbROIPoint, kbROIOnDevice.width, kbROIOnDevice.height, deviceSize, orientation, image.size().width);

                    if(kbROIOnFrame.contains(where))
                    {
                        Mat kbFrame;
                        ObjectUtil::Instance().CopyROIFrame(image, kbROIOnFrame, kbFrame);

                        Mat kbRotatedFrame = RotateFrameUp(kbFrame, orientation);
                        Rect kbROIOnRotatedFrame = RotateROIUp(kbROIOnFrame, image.size(), orientation);
                        if(keyboardRecognizer->RecognizeKeyboard(rotatedFrame, osVersion, kbROIOnRotatedFrame))
                        {
                            // input=keyboard (keyboard input only)
                            if(boost::equals(inputStr, "keyboard"))
                                qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_Only;
                            else
                                qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_Command;

                            qsBlockInfo.keyboardInput = keyboardRecognizer->FindCharByPoint(rotatedPoint);
                        }
                        else
                        {
                            // input=keyboard (keyboard input only)
                            if(boost::equals(inputStr, "keyboard"))
                                qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_Only;
                            else
                                qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_None;
                        }
                    }
                    else
                    {
                        qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_None; // Correct virtual keyboard status if click is out of keyboard (1. click user name, keyboard pop up; 2. click password, keyboard there)
                    }
                }
            }

            // Note: user may use OPCODE_SET_TEXT to replace keyboard input, the last action has kb=1; still need to handle navigation bar recognition
            if(nbRect != EmptyRect)
            {
                Point nbPoint(nbRect.x, nbRect.y);
                Rect nbOnFrame = AndroidTargetDevice::GetWindowRectangle(nbPoint, nbRect.width, nbRect.height, deviceSize, orientation, image.size().width);

                qsBlockInfo.hasVirtualNavigation = true;
                qsBlockInfo.navigationBarRect = nbOnFrame;
                qsBlockInfo.barLocation = ObjectUtil::Instance().FindBarLocation(image.size(), nbOnFrame);

                Rect imageRect(EmptyPoint, image.size());
                if(ContainRect(imageRect, nbOnFrame))
                {
                    bool isRecognized = barRecognizer->RecognizeNavigationBarWithBarInfo(image, nbOnFrame);
                    if(isRecognized)
                    {
                        int buttonIndex = barRecognizer->FindButtonFromPoint(where);
                        if (buttonIndex == 0)
                        {
                            qsBlockInfo.isVirtualNavigation = true;
                            qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_BACK;
                        }
                        else if (buttonIndex == 1)
                        {
                            qsBlockInfo.isVirtualNavigation = true;
                            qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_HOME;
                        }
                        else if (buttonIndex == 2)
                        {
                            qsBlockInfo.isVirtualNavigation = true;
                            qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_MENU;
                        }
                    }
                    else
                    {
                        qsBlockInfo.hasVirtualNavigation = true;
                        qsBlockInfo.navigationBarRect = EmptyRect;
                        qsBlockInfo.barLocation = NavigationBarLocation::BarLocationUnknown;
                    }
                }
                else
                {
                    DAVINCI_LOG_WARNING << "Invalid navigation bar frame, please check window bar information.";
                    qsBlockInfo.hasVirtualNavigation = false;
                    qsBlockInfo.navigationBarRect = EmptyRect;
                    qsBlockInfo.barLocation = NavigationBarLocation::BarLocationUnknown;
                }
            }
            else
            {
                qsBlockInfo.hasVirtualNavigation = false;
                qsBlockInfo.navigationBarRect = EmptyRect;
                qsBlockInfo.barLocation = NavigationBarLocation::BarLocationUnknown;
            }
        }
        else // legacy keyboard and bar recognition
        {
            if(keyboardRecognizer->RecognizeKeyboard(rotatedFrame, osVersion) && (keyboardRecognizer->FindCharByPoint(rotatedPoint) != '?'))
            {
                qsBlockInfo.isVirtualKeyboard = KeyboardInputType::KeyboardInput_Command;
                qsBlockInfo.keyboardInput = keyboardRecognizer->FindCharByPoint(rotatedPoint);
                DAVINCI_LOG_DEBUG << "Preprocessing - isVirtualKeyboard (" << qsBlockInfo.keyboardInput << ") on @" << qsFilename << " #" << boost::lexical_cast<std::string>(start);
            }
            else 
            {
                if (barRecognizer->RecognizeNavigationBar(image, navigationBarHeight))
                {
                    // Process members about navigation bar
                    DAVINCI_LOG_DEBUG << "Preprocessing - isVirtualNavigation on @" << qsFilename << " #" << boost::lexical_cast<std::string>(start);
                    qsBlockInfo.hasVirtualNavigation = true;
                    qsBlockInfo.navigationBarRect = barRecognizer->GetNavigationBarRect();
                    qsBlockInfo.barLocation = barRecognizer->GetNavigationBarLocation();

                    int buttonIndex = barRecognizer->FindButtonFromPoint(where);
                    if (buttonIndex == 0)
                    {
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_BACK;
                    }
                    else if (buttonIndex == 1)
                    {
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_HOME;
                    }
                    else if (buttonIndex == 2)
                    {
                        qsBlockInfo.isVirtualNavigation = true;
                        qsBlockInfo.navigationEvent = QSEventAndAction::OPCODE_MENU;
                    }

                    Rect rect = ObjectUtil::Instance().CutNavigationBarROI(image.size(), qsBlockInfo.navigationBarRect, qsBlockInfo.barLocation);
                    qsBlockInfo.mainROI = ObjectUtil::Instance().CutStatusBarROI(rect.size(), statusBarHeight, orientation);
                }
                else
                {
                    qsBlockInfo.hasVirtualNavigation = false;
                }
            }
        }

        Mat recordMainFrame;
        ObjectUtil::Instance().CopyROIFrame(image, qsBlockInfo.mainROI, recordMainFrame);
        path resourcePath;

        if(NeedShapePreprocess(currentMatchPattern))
        {
            shapeExtractor->SetFrame(recordMainFrame);
            shapeExtractor->Extract();
            qsBlockInfo.shapes = shapeExtractor->GetShapes();
            DrawShapes(recordMainFrame, qsBlockInfo.shapes, true); 

            // Process source clicked shape
            Point pointOnMainFrame = ObjectUtil::Instance().RelocatePointByOffset(where, qsBlockInfo.mainROI, false);
            boost::shared_ptr<Shape> clickedShape = ShapeUtil::Instance().GetClosestShapeContainedPoint(qsBlockInfo.shapes, pointOnMainFrame);
            if (clickedShape != nullptr)
            {
                qsBlockInfo.clickedShape = clickedShape;
                DrawRect(recordMainFrame, clickedShape->boundRect, Green); // Green for clicked shape
            }

            DrawCircle(recordMainFrame, pointOnMainFrame, Yellow); // Yellow for touch down point

            resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_shapes_" + boost::lexical_cast<string>(where.x) + "_" + boost::lexical_cast<string>(where.y) + ".png");
            SaveImage(recordMainFrame, resourcePath.string());

            // Process shape alignment
            ShapeAlignOrientation alignOrientation;
            qsBlockInfo.alignShapes = ShapeUtil::Instance().AlignMaxShapeGroup(qsBlockInfo.shapes, alignOrientation);
            qsBlockInfo.alignOrientation = alignOrientation;
            if (alignOrientation != ShapeAlignOrientation::UNKNOWN)
            {
                for (int index = 0; index < (int)qsBlockInfo.alignShapes.size(); index++)
                {
                    boost::shared_ptr<Shape> alignShape = qsBlockInfo.alignShapes[index];
                    if (alignShape->boundRect.contains(pointOnMainFrame))
                    {
                        qsBlockInfo.clickedAlignShapeIndex = index;
                        break;
                    }
                }
            }
        }

        if(NeedTextPreprocess(currentMatchPattern) && !kbROIOnFrame.contains(where))
        {
            Rect textRect = TextUtil::Instance().ExtractTextShapes(rotatedFrame, rotatedPoint);

            if (textRect != EmptyRect)
            {
                Mat textRoiFrame;
                ObjectUtil::Instance().CopyROIFrame(rotatedFrame, textRect, textRoiFrame);

                resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_text.png");
                SaveImage(textRoiFrame, resourcePath.string());

                string language = FindAdditionalInfo(keyValueMap, "language");
                string text = FindAdditionalInfo(keyValueMap, "text");
                wstring wtext = StrToWstr(text.c_str());
                text = WstrToStr(wtext);
                boost::trim(text);

                if(language == "en")
                {
                    FillEnTexts(textRoiFrame, textRect, qsBlockInfo);
                    if(!boost::empty(text))
                    {
                        qsBlockInfo.clickedEnText = text;
                    }
                }
                else if(language == "zh")
                {
                    FillZhTexts(textRoiFrame, textRect, qsBlockInfo);
                    if(!boost::empty(text))
                    {
                        qsBlockInfo.clickedZhText = text;
                    }
                }
                else
                {
                    if(!FillEnTexts(textRoiFrame, textRect, qsBlockInfo))
                    {
                        FillZhTexts(textRoiFrame, textRect, qsBlockInfo);
                        if(!boost::empty(text))
                        {
                            qsBlockInfo.clickedZhText = text;
                        }
                    }
                    else
                    {
                        if(!boost::empty(text))
                        {
                            qsBlockInfo.clickedEnText = text; 
                        }
                    }
                }
            }
        }

        // Reference frame is original size (no crop and resize)
        resourcePath = directory / boost::filesystem::path("record_" + qsFilename + "_" + boost::lexical_cast<string>(start) + "_reference.png");
        SaveImage(image, resourcePath.string());
    }

    Orientation ScriptHelper::GetActionOrientation(boost::shared_ptr<QScript> script, int ip)
    {
        Orientation orientation = Orientation::Unknown;
        auto theAction = script->EventAndAction(ip);
        if (theAction == nullptr)
        {
            return orientation;
        }
        bool hasOrientationParam = false;
        switch (theAction->Opcode())
        {
        case QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL:
        case QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL:
        case QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL:
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN_HORIZONTAL:
        case QSEventAndAction::OPCODE_MULTI_TOUCHUP_HORIZONTAL:
        case QSEventAndAction::OPCODE_MULTI_TOUCHMOVE_HORIZONTAL:
            orientation = Orientation::Landscape;
            break;
        case QSEventAndAction::OPCODE_CLICK:
        case QSEventAndAction::OPCODE_TOUCHDOWN:
        case QSEventAndAction::OPCODE_TOUCHUP:
        case QSEventAndAction::OPCODE_TOUCHMOVE:
            // default to Portrait for these opcode
            orientation = Orientation::Portrait;
            hasOrientationParam = true;
            break;
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE:
            hasOrientationParam = true;
            break;
        case QSEventAndAction::OPCODE_MULTI_TOUCHDOWN:
        case QSEventAndAction::OPCODE_MULTI_TOUCHUP:
        case QSEventAndAction::OPCODE_MULTI_TOUCHMOVE:
            // default to Portrait for these opcode
            orientation = Orientation::Portrait;
            hasOrientationParam = true;
            break;
        case QSEventAndAction::OPCODE_DRAG:
            // default to Portrait for these opcode
            orientation = Orientation::Portrait;
            hasOrientationParam = true;
            break;
        case QSEventAndAction::OPCODE_CLICK_MATCHED_TXT:
        case QSEventAndAction::OPCODE_CLICK_MATCHED_REGEX:
            hasOrientationParam = true;
            break;
        case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
        case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
            hasOrientationParam = true;
            break;
        }
        if (hasOrientationParam)
        {
            orientation = theAction->GetOrientation();
        }
        if (orientation != Orientation::Unknown)
        {
            return orientation;
        }
        else
        {
            orientation = Orientation::Portrait;
            for (int ipBefore = ip - 1; ipBefore > 0; --ipBefore)
            {
                auto before = script->EventAndAction(ipBefore);
                if (before != nullptr && before->Opcode() == QSEventAndAction::OPCODE_SET_CLICKING_HORIZONTAL)
                {
                    if (before->IntOperand(0) == 1)
                    {
                        orientation = Orientation::Landscape;
                    }
                    break;
                }
            }
            return orientation;
        }
    }   


    bool ScriptHelper::IsRectMatch(const boost::shared_ptr<FeatureMatcher> &matcher, Rect expectedRoi, int boundThreshold)
    {
        Rect matchedROI = matcher->GetBoundingRect();
        return (abs(matchedROI.x - expectedRoi.x) < boundThreshold && abs(matchedROI.y - expectedRoi.y) < boundThreshold && abs(matchedROI.width - expectedRoi.width) < boundThreshold && abs(matchedROI.height - expectedRoi.height) < boundThreshold);
    }

    Rect ScriptHelper::GetBoundingRect(const Mat &image, Point where, int boundThreshold)
    {
        size_t numRequiredKeyPoints = 150;
        boost::shared_ptr<FeatureMatcher> matcher;

        if(qsPtr != nullptr && qsPtr->UsedHyperSoftCam())
        {   
            numRequiredKeyPoints = 80;
            return GetBoundingRect(image, where, numRequiredKeyPoints, matcher, nullptr, 5, boundThreshold);
        }
        else
        {            
            return GetBoundingRect(image, where, numRequiredKeyPoints, matcher, nullptr, 50, boundThreshold);
        }
    }

    Rect ScriptHelper::GetBoundingRect(const Mat &image, Point where, size_t &numRequiredKeyPoints, boost::shared_ptr<FeatureMatcher> &modelMatcher, const boost::shared_ptr<FeatureMatcher> &imageMatcher, int keyPointsStep, int boundThreshold)
    {
        boost::shared_ptr<FeatureMatcher> matcher;
        if (imageMatcher != nullptr)
        {
            matcher = imageMatcher;
        }
        else
        {
            matcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
            FTParameters param(0,0,0.00001f,1.0f);
            matcher->SetModelImage(image,false,1.0f,DaVinci::PreprocessType::Keep,&param);
        }
        vector<KeyPoint> keypoints = matcher->GetKeyPoints();
        if (keypoints.empty())
        {
            modelMatcher = nullptr;
            return Rect();
        }
        struct KeyPointComparor
        {
            Point center;

            explicit KeyPointComparor(const Point &c) : center(c)
            {
            }

            bool operator()(const KeyPoint &k1, const KeyPoint &k2)
            {
                return (k1.pt.x - center.x) * (k1.pt.x - center.x) + (k1.pt.y - center.y) * (k1.pt.y - center.y) <
                    (k2.pt.x - center.x) * (k2.pt.x - center.x) + (k2.pt.y - center.y) * (k2.pt.y - center.y);
            }
        } comparor(where);
        std::sort(keypoints.begin(), keypoints.end(), comparor);

        bool isTouchOnNavBar = false;
        bool haveNavBar = false;
        if(barRecognizer->RecognizeNavigationBar(image, 0))
        {
            haveNavBar = true;
            Rect barRect = barRecognizer->GetNavigationBarRect();
            NavigationBarLocation barLocation = barRecognizer->GetNavigationBarLocation();

            if ((barLocation == NavigationBarLocation::BarLocationBottom && where.y > barRect.y)
                || (barLocation == NavigationBarLocation::BarLocationRight && where.x > barRect.x)
                || (barLocation == NavigationBarLocation::BarLocationTop && where.x > 0 && where.y < barRect.height)
                || (barLocation == NavigationBarLocation::BarLocationLeft && where.y > 0 && where.x < barRect.width))
            {
                isTouchOnNavBar = true;
            }
        }

        size_t i=0;
        vector<Point2f> points;
        while (numRequiredKeyPoints < keypoints.size())
        {

            Rect barRect;
            if(haveNavBar &&!isTouchOnNavBar)
            {
                barRect = barRecognizer->GetNavigationBarRect();
            }
            while ( points.size() < numRequiredKeyPoints &&i<keypoints.size())
            {
                if(haveNavBar && !isTouchOnNavBar && barRect.contains(keypoints[i].pt))//cut the NavBar
                {
                    ++i;
                    continue;
                }
                points.push_back(keypoints[i].pt);
                ++i;
            }
            // verify the match
            Rect roi = boundingRect(points);
            boost::shared_ptr<FTParameters> modelParam;
            boost::shared_ptr<FTParameters> observedParam;
            modelMatcher = boost::shared_ptr<FeatureMatcher>(new FeatureMatcher());
            modelMatcher->SetModelImage(image(roi).clone());
            modelParam = modelMatcher->GetFTParameters();
            observedParam = matcher->GetFTParameters();


            if(abs(modelParam->p1-observedParam->p1)>0.00001||abs(modelParam->p2-observedParam->p2)>0.00001)//different parameters,then re calculate observed iamge
            {
                matcher->SetModelImage(image,false,1.0,PreprocessType::Keep,modelParam.get());
            }

            DAVINCI_LOG_DEBUG << "points number: "  << boost::lexical_cast<string>(points.size());
            if (!modelMatcher->Match(*matcher).empty() && IsRectMatch(modelMatcher, roi, boundThreshold))
            {
                return roi;
            }
            numRequiredKeyPoints += keyPointsStep;
        }
        modelMatcher = matcher;
        DAVINCI_LOG_ERROR << "Can't get TOUCH point object. Please adjust manually " ;
        return Rect(0, 0, image.cols, image.rows);
    }

    bool ScriptHelper::GenActionSequenceToRnR(string actionSequenceFile)
    {
        boost::filesystem::path sequencePath(actionSequenceFile);
        boost::filesystem::path fullSequencePath = system_complete(sequencePath);
        boost::filesystem::path parenetPath = fullSequencePath.parent_path();

        string qsName = "autoGen.qs";
        boost::filesystem::path qsFileName = parenetPath / qsName;
        if(boost::filesystem::exists(qsFileName))
            boost::filesystem::remove(qsFileName);

        boost::shared_ptr<QScript> qs = QScript::New(qsFileName.string());
        if (qs == nullptr)
        {
            DAVINCI_LOG_ERROR << "Unable to create qs file " << qsFileName.string();
            return false;
        }

        vector <string> lines;
        ReadAllLines(fullSequencePath.string(), lines);
        if (lines.size() <= 0)
        {
            DAVINCI_LOG_ERROR << "Empty qs file!";
            return false;
        }

        vector<Mat> mats;

        int timeInterval = 3000;
        TryParse(lines[0], timeInterval);

        Size frameSize;
        vector<pair<int, int>> points;
        vector<Orientation> orientations;

        for(int i = 1; i < (int)lines.size(); i++)
        {
            string line = lines[i];
            boost::trim(line);
            if(line.empty())
                continue;

            vector<string> items;
            boost::algorithm::split(items, line, boost::algorithm::is_any_of(" "));
            if(items.size() < 4)
            {
                DAVINCI_LOG_WARNING << "Invalid sequence format @" << fullSequencePath.string() << endl;
                return false;
            }

            boost::filesystem::path imagePath = parenetPath / items[0];
            Mat frame = imread(imagePath.string());

            if(!frame.empty())
            {
                mats.push_back(frame);
                frameSize = frame.size();
            }
            else
            {
                DAVINCI_LOG_WARNING << "Invalid image file @" << imagePath.string() << endl;
                return false;
            }
            Orientation o = Orientation::Portrait;
            if(items[1] == "1")
                o = Orientation::Landscape;

            orientations.push_back(o);
            int x = 0, y = 0;
            TryParse(items[2], x);
            TryParse(items[3], y);

            int nx, ny;
            if(o == Orientation::Portrait)
            {
                nx = (frameSize.height - y )* maxWidthHeight / frameSize.height;
                ny = x * maxWidthHeight / frameSize.width;
            }
            else
            {
                nx = x * maxWidthHeight / frameSize.width;
                ny = y * maxWidthHeight / frameSize.height;
            }

            points.push_back(make_pair(nx, ny));
        }

        vector<int> timeStampList;
        StopWatch qsWatch;
        AutoResetEvent waitEvent;
        boost::filesystem::path videoFileName = parenetPath / (qs->GetScriptName() + ".avi");
        VideoWriterProxy videoWriter;

        if(boost::filesystem::exists(videoFileName))
            boost::filesystem::remove(videoFileName);

        if(mats.size() == 0)
        {
            DAVINCI_LOG_WARNING << "No image sequence." << endl;
            return false;
        }

        if (!videoWriter.open(videoFileName.string(), CV_FOURCC('M', 'J', 'P', 'G'), 30, frameSize))
        {
            DAVINCI_LOG_ERROR << "      ERROR: Cannot create the video file from sequence!";
            return false;
        }

        int i = 1;
        int timestamp;
        Mat emptyMat = Mat::zeros(frameSize, CV_8UC1);
        bool isComplete = false;
        boost::shared_ptr<QSEventAndAction> qsEvent;

        qsWatch.Start();
        while(true)
        {
            timestamp = (int)qsWatch.ElapsedMilliseconds();
            timeStampList.push_back(timestamp);
            if(timestamp <= i * timeInterval)
            {
                videoWriter.write(emptyMat);
                if(isComplete)
                {
                    break;
                }
            }
            else
            {
                qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(timestamp,
                    QSEventAndAction::OPCODE_TOUCHDOWN, points[i-1].first, points[i-1].second, 0, 0, "", orientations[i-1]));
                qs->AppendEventAndAction(qsEvent);
                qsEvent = boost::shared_ptr<QSEventAndAction>(new QSEventAndAction(timestamp,
                    QSEventAndAction::OPCODE_TOUCHUP, points[i-1].first, points[i-1].second, 0, 0, "", orientations[i-1]));
                qs->AppendEventAndAction(qsEvent);

                videoWriter.write(mats[i - 1]);
                i++;
                if(i > (int)(mats.size()))
                    isComplete = true;
            }
            waitEvent.WaitOne(100);
        }

        qsWatch.Stop();
        videoWriter.release();
        qs->Flush();

        boost::filesystem::path qtsFileName = parenetPath / (qs->GetScriptName() + ".qts");
        if(boost::filesystem::exists(qtsFileName))
            boost::filesystem::remove(qtsFileName);
        std::ofstream qtsStream(qtsFileName.string(), std::ios_base::app | std::ios_base::out);
        qtsStream << "Version:1" << endl << std::flush;

        for(auto timestamp : timeStampList)
        {
            qtsStream << boost::lexical_cast<string>(timestamp) << endl << std::flush;
        }

        qtsStream.flush();
        qtsStream.close();

        return true;
    }
}
#include "CriticalPathConstructor.h"
#include "../TimeStampFileReader.h"
#include "../DaVinciConfig.h"

using namespace Emgu::CV;
using namespace Emgu::CV::Structure;
namespace DaVinci
{
    namespace Exploratory
    {

        CriticalPathConstructor::CriticalPathConstructor()
        {
        }

        CriticalPathConstructor::CriticalPathConstructor(std::vector<std::string> &l)
        {
            this->scriptList = l;
        }

#if !defined(ASUS)
        std::vector<DaVinci::QScriptEditor::actionlinetype> CriticalPathConstructor::getActionLineTypes(const std::string &scriptFile)
        {
            std::vector<DaVinci::QScriptEditor::actionlinetype> actionLineList = std::vector<QScriptEditor::actionlinetype>();
            if (File::Exists(scriptFile))
            {
                std::vector<std::string> qs_lines = System::IO::File::ReadAllLines(scriptFile);
                int line = 9; // start line index for actions
                while (line < qs_lines.size())
                {
                    std::shared_ptr<DaVinci::QScriptEditor::actionlinetype> action = DaVinci::QScriptEditor::parseActionLine(qs_lines[line]);
                    if (action != nullptr)
                    {
                        actionLineList.push_back(action);
                    }
                    else
                    {
                        std::cout << std::string("Unrecognized action in line ") << StringConverterHelper::toString(line << 1) << std::string(", which will be removed if you save the current scripts!") << std::endl;
                    }
                    line++;
                }

            }

            return actionLineList;
        }
#endif

#if !defined(ASUS)
        void CriticalPathConstructor::transformObjects(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList, const std::string &timeFile, const std::string &videoFile)
        {
            std::vector<double> timeStampList = std::vector<double>();
            std::vector<TestManager::traceInfo> qsTrace = std::vector<TestManager::traceInfo>();

            if (File::Exists(timeFile))
            {
                // load time stamp file
                std::shared_ptr<TimeStampFileReader> qtsReader = std::make_shared<TimeStampFileReader>(timeFile);
                timeStampList = qtsReader->getTimeStampList();
                qsTrace = qtsReader->getTrace();
            }
            else
            {
                std::cout << std::string("Time Stamp file \"") << timeFile << std::string("\" doesn't exist!") << std::endl;
            }

//C# TO C++ CONVERTER NOTE: The following 'using' block is replaced by its C++ equivalent:
//            using (Emgu.CV.Capture capture = new Emgu.CV.Capture(videoFile))
            std::shared_ptr<Emgu::CV::Capture> capture = std::make_shared<Emgu::CV::Capture>(videoFile);
            try
            {
                cv::Mat frame = capture->QueryFrame();
                double imageHeight = frame->Height, imageWidth = frame->Width;

                int line = 0;
                while (line < actionLineList.size())
                {
                    // look for a range that starts with OPCODE_TOUCHDOWN(_HORIZONTAL),
                    // followed by zero to more OPCODE_TOUCHMOVE(_HORIZONTAL) and ends with OPCODE_TOUCHUP(_HORIZONTAL).
                    // The ignored opcodes from ObjectScriptingOpcodeBypass() are allowed in-between TOUCHDOWN and TOUCHUP but will be bypassed.
                    bool isHorizontal;
                    int start = line;
                    int end = line;
                    line += 1;
                    std::vector<std::vector<std::string>> touchOpcodes = std::vector<std::vector<std::string>>
                    {
                        {"OPCODE_TOUCHDOWN", "OPCODE_TOUCHUP", "OPCODE_TOUCHMOVE"},
                        {"OPCODE_TOUCHDOWN_HORIZONTAL", "OPCODE_TOUCHUP_HORIZONTAL", "OPCODE_TOUCHMOVE_HORIZONTAL"}
                    };
                    for (auto opcodes : touchOpcodes)
                    {
                        if (actionLineList[start]->name == opcodes[0])
                        {
                            int i = start + 1;
                            while (i < actionLineList.size())
                            {
                                line = i;
                                if (actionLineList[i]->name == opcodes[1])
                                {
                                    end = i;
                                    break;
                                }
                                else if (actionLineList[i]->name == opcodes[2] || DaVinci::QScriptEditor::ObjectScriptingOpcodeBypass(actionLineList[i]->name))
                                {
                                    i++;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                    if (start == end)
                    {
                        continue;
                    }
                    if (actionLineList[start]->name == "OPCODE_TOUCHDOWN")
                    {
                        isHorizontal = false;
                    }
                    else
                    {
                        isHorizontal = true;
                    }
                    // the next good range, the calculation includes the newly added OPCODE_IF_MATCH_IMAGE_WAIT.
                    line = end + 2;
                    // now we found a good range from "start" to "end".
                    // firstly we locate the video frame at the beginning of the range.
                    int timeIndex;
                    for (timeIndex = 0; timeIndex < timeStampList.size() - 1; timeIndex++)
                    {
                        if (timeStampList[timeIndex] > actionLineList[start]->time)
                        {
                            break;
                        }
                    }
                    if (timeIndex > 0) // position before the first action
                    {
                        timeIndex--;
                    }
                    capture->SetCaptureProperty(Emgu::CV::CvEnum::CAP_PROP::CV_CAP_PROP_POS_FRAMES, static_cast<double>(timeIndex));
                    cv::Mat image = capture->QueryFrame();
                    if (image == nullptr || image->Width == 0 || image->Height == 0)
                    {
                        std::cout << std::string("Null frame at line #") << start << std::string("!") << std::endl;
                        break;
                    }
                    double xRatio = static_cast<double>(image->Width) / static_cast<double>(4096);
                    double yRatio = static_cast<double>(image->Height) / static_cast<double>(4096);
                    std::string picFileName = "";
                    Rectangle roiRect = Rectangle::Empty;
                    for (int i = start; i <= end; i++)
                    {
                        if (DaVinci::QScriptEditor::ObjectScriptingOpcodeBypass(actionLineList[i]->name))
                        {
                            continue;
                        }
                        int x, y;
                        if (!int::TryParse(actionLineList[i]->parameters[0], x) || !int::TryParse(actionLineList[i]->parameters[1], y))
                        {
                            std::cout << std::string("Invalid coordinates at line #") << i << std::string("!") << std::endl;
                            break;
                        }
                        Point where;
                        if (isHorizontal)
                        {
                            where = Point(static_cast<int>(x * xRatio), static_cast<int>(y * yRatio));
                        }
                        else
                        {
                            where = Point(static_cast<int>(y * xRatio), image->Height - static_cast<int>(x * yRatio));
                        }
                        if (actionLineList[i]->name == "OPCODE_TOUCHDOWN" || actionLineList[i]->name == "OPCODE_TOUCHDOWN_HORIZONTAL")
                        {
                            roiRect = DaVinci::QScriptEditor::GetBoundingRectSURF(image, nullptr, where);
                            image->ROI = roiRect;
                            std::string picFile = GetPictureShotFileName(videoFile);
                            std::cout << std::string("Line #") << i << std::string(": picture shot is saved to \"") << picFile << std::string("\"") << std::endl;
                            image->Save(picFile);
                            CvInvoke::cvResetImageROI(image);
                            picFileName = Path::GetFileNameWithoutExtension(picFile);
                        }
                        assert(roiRect != Rectangle::Empty);
                        int frameRelativeX = where.X - (roiRect.X + roiRect.Width / 2);
                        int frameRelativeY = where.Y - (roiRect.Y + roiRect.Height / 2);
                        int normRelativeX, normRelativeY;
                        if (isHorizontal)
                        {
                            normRelativeX = static_cast<int>(frameRelativeX / xRatio);
                            normRelativeY = static_cast<int>(frameRelativeY / yRatio);
                        }
                        else
                        {
                            normRelativeX = -static_cast<int>(frameRelativeY / yRatio);
                            normRelativeY = static_cast<int>(frameRelativeX / xRatio);
                        }
//C# TO C++ CONVERTER NOTE: The following 'switch' operated on a string variable and was converted to C++ 'if-else' logic:
//                        switch (actionLineList[i].name)
//ORIGINAL LINE: case "OPCODE_TOUCHDOWN":
                        if (actionLineList[i]->name == "OPCODE_TOUCHDOWN" || actionLineList[i]->name == "OPCODE_TOUCHDOWN_HORIZONTAL")
                        {
                                actionLineList[i]->name = "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY";
                        }
//ORIGINAL LINE: case "OPCODE_TOUCHUP":
                        else if (actionLineList[i]->name == "OPCODE_TOUCHUP" || actionLineList[i]->name == "OPCODE_TOUCHUP_HORIZONTAL")
                        {
                                actionLineList[i]->name = "OPCODE_TOUCHUP_MATCHED_IMAGE_XY";
                        }
//ORIGINAL LINE: case "OPCODE_TOUCHMOVE":
                        else if (actionLineList[i]->name == "OPCODE_TOUCHMOVE" || actionLineList[i]->name == "OPCODE_TOUCHMOVE_HORIZONTAL")
                        {
                                actionLineList[i]->name = "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY";
                        }
                        else
                        {
                                assert(false);
                        }
                        assert(picFileName != "");
                        actionLineList[i]->parameters = std::vector<std::string>(5);
                        actionLineList[i]->parameters[0] = picFileName + std::string(".png");
                        actionLineList[i]->parameters[1] = StringConverterHelper::toString(normRelativeX);
                        actionLineList[i]->parameters[2] = StringConverterHelper::toString(normRelativeY);
                        actionLineList[i]->parameters[3] = StringConverterHelper::toString(x);
                        actionLineList[i]->parameters[4] = StringConverterHelper::toString(y);
                    }
                    // finally we insert the image match check before the first TOUCHDOWN event
                    std::shared_ptr<DaVinci::QScriptEditor::actionlinetype> ifMatchImageWait = std::make_shared<DaVinci::QScriptEditor::actionlinetype>("OPCODE_IF_MATCH_IMAGE_WAIT", 4);
                    ifMatchImageWait->label = getUnusedLabel(actionLineList, qsTrace);
                    ifMatchImageWait->parameters[0] = picFileName + std::string(".png");
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to 'ToString':
                    ifMatchImageWait->parameters[1] = actionLineList[start]->label.ToString();
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to 'ToString':
                    ifMatchImageWait->parameters[2] = actionLineList[start]->label.ToString();
                    ifMatchImageWait->parameters[3] = "5000"; // 5 seconds timeout by default
                    // set the timestamp for OPCODE_IF_MATCH_IMAGE_WAIT, leave at most 500ms for image recognition to
                    // keep the original timing as much as possible.
                    double delta = 0;
                    if (start > 0)
                    {
                        delta = actionLineList[start]->time - actionLineList[start - 1]->time;
                    }
                    if (delta > 500 || start == 0)
                    {
                        ifMatchImageWait->time = actionLineList[start]->time - 500;
                    }
                    else
                    {
                        ifMatchImageWait->time = actionLineList[start - 1]->time;
                    }
                    actionLineList.Insert(start, ifMatchImageWait);
                }
            }
//C# TO C++ CONVERTER TODO TASK: There is no native C++ equivalent to the exception 'finally' clause:
            finally
            {
                if (capture != nullptr)
                {
                    capture.Dispose();
                }
            }
        }
#endif

#if !defined(ASUS)
        int CriticalPathConstructor::getUnusedLabel(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList, std::vector<TestManager::traceInfo> &qsTrace)
        {
            if (actionLineList.empty())
            {
                return 0;
            }
            int label = 1;
            while (isLabelInScript(label, actionLineList) || isLabelInQTS(label, qsTrace))
            {
                label++;
            }
            return label;
        }
#endif

#if !defined(ASUS)
        bool CriticalPathConstructor::isLabelInScript(int l, std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList)
        {
            for (int i = 0; i < actionLineList.size(); i++)
            {
                if (actionLineList[i]->label == l)
                {
                    return true;
                }
            }
            return false;
        }
#endif

#if !defined(ASUS)
        std::vector<ActionSet> CriticalPathConstructor::mapActionFromScript(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList)
        {
            std::vector<ActionSet> actionSetList = std::vector<ActionSet>();
            std::shared_ptr<ActionSet> actionSet = nullptr;
            std::vector<QSEventAndAction> actionList = std::vector<QSEventAndAction>();
            std::vector<QSEventAndAction> copyActionList = std::vector<QSEventAndAction>();
            double currentTimestamp = 0.0;

            int current_opcode = 0;
            std::string current_operand_string;
            int current_operand1, current_operand2, current_operand3, current_operand4, current_orientation = 0;

            for (auto actionLine : actionLineList)
            {
                if (actionLine->name == "OPCODE_IF_MATCH_IMAGE_WAIT")
                {
                    actionSet = std::make_shared<ActionSet>();
                    std::cout << actionLine->name << std::endl;
                    currentTimestamp = actionLine->time;
                    actionSet->imageName = actionLine->parameters[0];
                }
                else
                {
                    if (actionLine->name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY")
                    {
                        current_opcode = QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY;
                        current_operand_string = actionLine->parameters[0];
                        int::TryParse(actionLine->parameters[1], current_operand1);
                        int::TryParse(actionLine->parameters[2], current_operand2);
                        int::TryParse(actionLine->parameters[3], current_operand3);
                        int::TryParse(actionLine->parameters[4], current_operand4);
                        if (actionLine->parameters.size() > 5)
                        {
                            int::TryParse(actionLine->parameters[5], current_orientation);
                        }
                        std::shared_ptr<QSEventAndAction> eventAction = std::make_shared<QSEventAndAction>(actionLine->label, actionLine->time - currentTimestamp, current_opcode, current_operand1, current_operand2, current_operand3, current_operand4, current_operand_string,current_orientation);
                        actionList.push_back(eventAction);
                        currentTimestamp = actionLine->time;
                    }
                    else if (actionLine->name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY")
                    {
                        current_opcode = QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY;
                        current_operand_string = actionLine->parameters[0];
                        int::TryParse(actionLine->parameters[1], current_operand1);
                        int::TryParse(actionLine->parameters[2], current_operand2);
                        int::TryParse(actionLine->parameters[3], current_operand3);
                        int::TryParse(actionLine->parameters[4], current_operand4);
                        if (actionLine->parameters.size() > 5)
                        {
                            int::TryParse(actionLine->parameters[5], current_orientation);
                        }
                        std::shared_ptr<QSEventAndAction> eventAction = std::make_shared<QSEventAndAction>(actionLine->label, actionLine->time - currentTimestamp, current_opcode, current_operand1, current_operand2, current_operand3, current_operand4, current_operand_string,current_orientation);
                        actionList.push_back(eventAction);
                        currentTimestamp = actionLine->time;
                    }
                    else if (actionLine->name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY")
                    {
                        current_opcode = QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY;
                        current_operand_string = actionLine->parameters[0];
                        int::TryParse(actionLine->parameters[1], current_operand1);
                        int::TryParse(actionLine->parameters[2], current_operand2);
                        int::TryParse(actionLine->parameters[3], current_operand3);
                        int::TryParse(actionLine->parameters[4], current_operand4);
                        if (actionLine->parameters.size() > 5)
                        {
                            int::TryParse(actionLine->parameters[5], current_orientation);
                        }
                        std::shared_ptr<QSEventAndAction> eventAction = std::make_shared<QSEventAndAction>(actionLine->label, actionLine->time - currentTimestamp, current_opcode, current_operand1, current_operand2, current_operand3, current_operand4, current_operand_string,current_orientation);
                        actionList.push_back(eventAction);
                        copyActionList = std::vector<QSEventAndAction>(actionList);

                        if (actionSet != nullptr)
                        {
                            actionSet->actionList = copyActionList;
                        }
                        actionSetList.push_back(actionSet);
                        actionList.clear();
                    }
                }
            }

            return actionSetList;
        }
#endif

#if !defined(ASUS)
        std::shared_ptr<QSEventAndAction> CriticalPathConstructor::transformActionLineToEventAction(const std::shared_ptr<DaVinci::QScriptEditor::actionlinetype> &actionLine)
        {
            std::shared_ptr<QSEventAndAction> eventAction = nullptr;
            int current_opcode = 0;
            std::string current_operand_string;
            int current_operand1, current_operand2, current_operand3, current_operand4, current_orientation = 0;

//C# TO C++ CONVERTER NOTE: The following 'switch' operated on a string variable and was converted to C++ 'if-else' logic:
//            switch (actionLine.name)
//ORIGINAL LINE: case "OPCODE_CLICK_MATCHED_IMAGE_XY":
            if (actionLine->name == "OPCODE_CLICK_MATCHED_IMAGE_XY" || actionLine->name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY" || actionLine->name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY" || actionLine->name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY")
            {
//C# TO C++ CONVERTER NOTE: The following 'switch' operated on a string variable and was converted to C++ 'if-else' logic:
//                    switch (actionLine.name)
//ORIGINAL LINE: case "OPCODE_CLICK_MATCHED_IMAGE_XY":
                    if (actionLine->name == "OPCODE_CLICK_MATCHED_IMAGE_XY")
                    {
                            current_opcode = QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY;
                    }
//ORIGINAL LINE: case "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY":
                    else if (actionLine->name == "OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY")
                    {
                            current_opcode = QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY;
                    }
//ORIGINAL LINE: case "OPCODE_TOUCHUP_MATCHED_IMAGE_XY":
                    else if (actionLine->name == "OPCODE_TOUCHUP_MATCHED_IMAGE_XY")
                    {
                            current_opcode = QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY;
                    }
//ORIGINAL LINE: case "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY":
                    else if (actionLine->name == "OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY")
                    {
                            current_opcode = QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY;
                    }
                    else
                    {
                            assert(false);
                    }
                    current_operand_string = actionLine->parameters[0];
                    int::TryParse(actionLine->parameters[1], current_operand1);
                    int::TryParse(actionLine->parameters[2], current_operand2);
                    int::TryParse(actionLine->parameters[3], current_operand3);
                    int::TryParse(actionLine->parameters[4], current_operand4);
                    if (actionLine->parameters.size() > 5)
                    {
                        int::TryParse(actionLine->parameters[5], current_orientation);
                    }
                    eventAction = std::make_shared<QSEventAndAction>(actionLine->label, actionLine->time, current_opcode, current_operand1, current_operand2, current_operand3, current_operand4, current_operand_string,current_orientation);
            }
            else
            {
            }

            return eventAction;
        }
#endif

#if !defined(ASUS)
        std::vector<ActionSet> CriticalPathConstructor::generateAllCriticalActions(std::vector<std::string> &scriptFiles)
        {
            std::vector<ActionSet> allCriticalActions = std::vector<ActionSet>();
            std::string scriptDir, scriptName;
            for (auto scriptFile : scriptFiles)
            {
                scriptDir = Path::GetDirectoryName(scriptFile);
                scriptName = Path::GetFileNameWithoutExtension(scriptFile);
                std::vector<DaVinci::QScriptEditor::actionlinetype> actionLineList = getActionLineTypes(scriptFile);
                transformObjects(actionLineList, scriptDir + std::string("\\") + scriptName + std::string(".qts"), scriptDir + std::string("\\") + scriptName + std::string(".avi"));
                std::vector<ActionSet> currentCriticalActions = mapActionFromScript(actionLineList);
                allCriticalActions.AddRange(currentCriticalActions);
            }

            return allCriticalActions;
        }
#endif

        std::string CriticalPathConstructor::GetPictureShotFileName(const std::string &videoFile)
        {
            assert(File::Exists(videoFile));
            std::string dirName = Path::GetDirectoryName(videoFile);
            std::string picFileNamePre = dirName + std::string("\\") + Path::GetFileNameWithoutExtension(videoFile) + std::string("_PictureShot");
            int picFileNameIndex = 1;
            std::string picFileName = picFileNamePre + StringConverterHelper::toString(picFileNameIndex) + std::string(".png");
            while (File::Exists(picFileName))
            {
                picFileNameIndex++;
                picFileName = picFileNamePre + StringConverterHelper::toString(picFileNameIndex) + std::string(".png");
            }
            return picFileName;
        }

        bool CriticalPathConstructor::isLabelInQTS(int l, std::vector<TestManager::traceInfo> &qsTrace)
        {
            if (qsTrace.empty())
            {
                return false;
            }
            for (int i = 0; i < qsTrace.size(); i++)
            {
                if (qsTrace[i].label == l)
                {
                    return true;
                }
            }
            return false;
        }

        void CriticalPathConstructor::ExecuteQSInstructionForExploration(const std::shared_ptr<QSEventAndAction> &qs_event, const cv::Mat &frame, Point matchedCenter)
        {
            DaVinciConfig::Orientation orientation = static_cast<DaVinciConfig::Orientation>(qs_event->orientation);
            Size offset = TestManager::NormToFrameOffset(frame->Size, Size(qs_event->operand1, qs_event->operand2), orientation);
            int frameX = matchedCenter.X + offset.Width;
            int frameY = matchedCenter.Y + offset.Height;
            switch (qs_event->opcode)
            {
                case QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY:
                    TestManager::ClickOnFrame(frame, frameX, frameY, orientation);
                    break;
                case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
                    TestManager::TouchDownOnFrame(frame, frameX, frameY, orientation);
                    break;
                case QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY:
                    TestManager::TouchUpOnFrame(frame, frameX, frameY, orientation);
                    break;
                case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
                    TestManager::TouchMoveOnFrame(frame, frameX, frameY, orientation);
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        void CriticalPathConstructor::StartExploratoryTimer()
        {
            exploratoryTimer = std::make_shared<Multimedia::Timer>();
            exploratoryTimer->SetMode(Multimedia::TimerMode::Periodic);
            exploratoryTimer->SetPeriod(1);
            exploratoryTimer->SetResolution(1);
            exploratoryTimer->Tick += std::make_shared<System::EventHandler>(shared_from_this(), &CriticalPathConstructor::ExploratoryHandler);
            exploratoryTimer->Start();
        }

        void CriticalPathConstructor::StopExploratoryTimer()
        {
            exploratoryTimer->Stop();
        }

        double CriticalPathConstructor::GetQSElapsedMillisecondsForExploratory()
        {
            return exploratoryWatch->ElapsedTicks * 1000 / Stopwatch::Frequency;
        }

        void CriticalPathConstructor::ExploratoryHandler(const std::shared_ptr<void> &sender, const std::shared_ptr<EventArgs> &e)
        {
            std::shared_ptr<QSEventAndAction> action = nullptr;
            while (this->actionSet != nullptr && this->actionIndex < this->actionSet->actionList.size())
            {
                action = this->actionSet->actionList[this->actionIndex];

                elapsed = GetQSElapsedMillisecondsForExploratory();

                //if (elapsed + timestampOffset < qs_event.timestamp)
                //    break;

                std::shared_ptr<Stopwatch> exeWatch = std::make_shared<Stopwatch>();
                exeWatch->Start();
                ExecuteQSInstructionForExploration(action, this->currentFrame, this->matchedCenter);
                exeWatch->Stop();

                //exeWatch.ElapsedMilliseconds
            }

            StopExploratoryTimer();
            this->actionIndex = 0;
        }

        bool CriticalPathConstructor::isCriticalPath(const cv::Mat &frame)
        {
            return frame != nullptr;
        }

        void CriticalPathConstructor::executeCriticalActions(const std::shared_ptr<ActionSet> &currentSet, const cv::Mat &frame)
        {
            this->currentFrame = frame;
            if (this->currentFrame == nullptr)
            {
                std::cout << std::string("Warning: current frame is null.") << std::endl;
                return;
            }

            this->actionSet = currentSet;
            StartExploratoryTimer();
        }
    }
}

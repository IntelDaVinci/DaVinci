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

#include <sstream>
#include <iomanip>

#include "boost/filesystem.hpp"

#include "QScript.hpp"
#include "DaVinciCommon.hpp"
#include "AndroidTargetDevice.hpp"

#include <locale>
#include <codecvt>

using namespace boost::filesystem;
using namespace boost::algorithm;

namespace DaVinci
{

    QSEventAndAction::QSEventAndAction(double ts, int opc, const string &opStr, Orientation o) : lineNumber(0)
    {
        vector<string> strOprs;
        if (!opStr.empty())
        {
            strOprs.push_back(opStr);
        }
        Init(ts, opc, vector<int>(), strOprs, o);
    }

    QSEventAndAction::QSEventAndAction(double ts, int opc, int opr1, int opr2, int opr3, int opr4, const string &opStr, Orientation o) : lineNumber(0)
    {
        vector<int> intOprs;
        intOprs.push_back(opr1);
        intOprs.push_back(opr2);
        intOprs.push_back(opr3);
        intOprs.push_back(opr4);
        vector<string> strOprs;
        if (!opStr.empty())
        {
            strOprs.push_back(opStr);
        }
        Init(ts, opc, intOprs, strOprs, o);
    }

    QSEventAndAction::QSEventAndAction(double ts, int opc, const vector<int> &intOprs, const vector<string> &strOprs, Orientation o)
    {
        Init(ts, opc, intOprs, strOprs, o);
    }

    void QSEventAndAction::Init(double ts, int opc, const vector<int> &intOprs, const vector<string> &strOprs, Orientation o)
    {
        lineNumber = 0;
        timeStamp = ts;
        timeStampReplay = 0;
        opcode = opc;
        intOperands = intOprs;
        strOperands = strOprs;
        orientation = o;
        defaultIntOperand = 0;
    }

    int & QSEventAndAction::LineNumber()
    {
        return lineNumber;
    }

    unsigned int QSEventAndAction::NumIntOperands()
    {
        return (unsigned int)(intOperands.size());
    }

    int & QSEventAndAction::IntOperand(int index)
    {
        if (index < 0 || index >= static_cast<int>(intOperands.size()))
        {
            return defaultIntOperand;
        }
        return intOperands[index];
    }

    void QSEventAndAction::SetIntOperand(int index, int val)
    {
        if (index < 0 || index >= static_cast<int>(intOperands.size()))
        {
            return;
        }
        intOperands[index] = val;
    }


    unsigned int QSEventAndAction::NumStrOperands()
    {
        return (unsigned int)(strOperands.size());
    }

    string & QSEventAndAction::StrOperand(int index)
    {
        if (index < 0 || index >= static_cast<int>(strOperands.size()))
        {
            return defaultStrOperand;
        }
        return strOperands[index];
    }

    double & QSEventAndAction::TimeStamp()
    {
        return timeStamp;
    }

    void QSEventAndAction::SetTimeStamp(double srcTimeStamp)
    {
        timeStamp = srcTimeStamp;
    }

    double & QSEventAndAction::TimeStampReplay()
    {
        return timeStampReplay;
    }

    int & QSEventAndAction::Opcode()
    {
        return opcode;
    }

    string QSEventAndAction::ToString()
    {
        ostringstream oss;
        oss << "<Label: " << lineNumber << ", OPCODE=";
        if (QScript::opcodeNameTable.find(opcode) != QScript::opcodeNameTable.end())
        {
            oss << QScript::opcodeNameTable[opcode];
        }
        else
        {
            oss << opcode;
        }
        for (auto strOperand = strOperands.begin(); strOperand != strOperands.end(); strOperand++)
        {
            oss << ", " << *strOperand;
        }
        for (auto intOperand = intOperands.begin(); intOperand != intOperands.end(); intOperand++)
        {
            oss << ", " << *intOperand;
        }
        oss << ">";
        return oss.str();
    }

    Orientation & QSEventAndAction::GetOrientation()
    {
        return orientation;
    }

    vector<string> & QSEventAndAction::GetStrOperands()
    {
        return strOperands;
    }

    QSEventAndAction::Spec::Spec(int opc, unsigned int oprNum, unsigned int mandatoryOprNum, ...)
        : opcode(opc), numOperands(oprNum), numMandatoryOperands(mandatoryOprNum)
    {
        va_list argList;
        va_start(argList, mandatoryOprNum);
        assert(oprNum >= mandatoryOprNum);
        for (unsigned int i = 0; i < oprNum; i++)
        {
            operandTypes.push_back(va_arg(argList, OperandType));
        }
        va_end(argList);
    }

    int QSEventAndAction::Spec::Opcode() const
    {
        return opcode;
    }

    unsigned int QSEventAndAction::Spec::NumOperands() const
    {
        return numOperands;
    }

    unsigned int QSEventAndAction::Spec::NumMandatoryOperands() const
    {
        return numMandatoryOperands;
    }

    QSEventAndAction::Spec::OperandType QSEventAndAction::Spec::operator[](int index) const
    {
        return operandTypes[index];
    }

    const string QScript::ConfigCameraPresetting = "CameraPresetting";
    const string QScript::ConfigPackageName = "PackageName";
    const string QScript::ConfigActivityName = "ActivityName";
    const string QScript::ConfigIconName = "IconName";
    const string QScript::ConfigApkName = "APKName";
    const string QScript::ConfigPushData = "PushData";
    const string QScript::ConfigStatusBarHeight = "StatusBarHeight";
    const string QScript::ConfigNavigationBarHeight = "NavigationBarHeight";
    const string QScript::ConfigStartOrientation = "StartOrientation";
    const string QScript::ConfigCameraType = "CameraType";
    const string QScript::ConfigVideoRecording = "VideoRecording";
    const string QScript::ConfigMultiLayerMode = "MultiLayerMode";
    const string QScript::ConfigRecordedVideoType = "RecordedVideoType";
    const string QScript::ConfigReplayVideoType = "ReplayVideoType";
    const string QScript::ConfigVideoTypeH264 = "H264";
    const string QScript::ConfigOSversion = "OSVersion";
    const string QScript::ConfigDPI = "DPI";
    const string QScript::ConfigResolution = "Resolution";
    const string QScript::ConfigResolutionWidth = "Width";
    const string QScript::ConfigResolutionHeight = "Height";

    unordered_map<string, boost::shared_ptr<QSEventAndAction::Spec>> QScript::qscriptSpec;
    unordered_map<int, string> QScript::opcodeNameTable;

    DaVinciStatus QScript::InitSpec()
    {
        typedef QSEventAndAction::Spec Spec;
        typedef QSEventAndAction::Spec::OperandType OpType;
        qscriptSpec["OPCODE_CLICK"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLICK, 3, 2, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_HOME"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_HOME, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_BACK"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_BACK, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_MENU"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MENU, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_SWIPE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_SWIPE, 4, 4, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_DRAG"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_DRAG, 5, 4, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_POWER_MEASURE_START"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_POWER_MEASURE_START, 0, 0));
        qscriptSpec["OPCODE_POWER_MEASURE_STOP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_POWER_MEASURE_STOP, 0, 0));
        qscriptSpec["OPCODE_INSTALL_APP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_INSTALL_APP, 2, 0, OpType::Integer, OpType::String));
        qscriptSpec["OPCODE_UNINSTALL_APP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_UNINSTALL_APP, 1, 0, OpType::String));
        qscriptSpec["OPCODE_START_APP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_START_APP, 1, 0, OpType::String));
        qscriptSpec["OPCODE_STOP_APP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_STOP_APP, 1, 0, OpType::String));
        qscriptSpec["OPCODE_PUSH_DATA"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_PUSH_DATA, 2, 0, OpType::String, OpType::String));
        qscriptSpec["OPCODE_SET_CLICKING_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_SET_CLICKING_HORIZONTAL, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_TOUCHDOWN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHDOWN, 4, 2, OpType::Integer, OpType::Integer, OpType::Orientation, OpType::String));
        qscriptSpec["OPCODE_TOUCHUP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHUP, 3, 2, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_TOUCHMOVE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHMOVE, 3, 2, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_TOUCHDOWN_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL, 2, 2, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TOUCHUP_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL, 2, 2, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TOUCHMOVE_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL, 2, 2, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTUP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTUP, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTDOWN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTDOWN, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTLEFT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTLEFT, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTRIGHT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTRIGHT, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTTO"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTTO, 4, 2, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_OCR"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_OCR, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_PREVIEW"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_PREVIEW, 4, 4, OpType::String, OpType::String, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_MATCH_ON"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MATCH_ON, 1, 0, OpType::String));
        qscriptSpec["OPCODE_MATCH_OFF"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MATCH_OFF, 1, 0, OpType::String));
        qscriptSpec["OPCODE_IF_MATCH_IMAGE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_MATCH_IMAGE, 4, 3, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CLICK_MATCHED_IMAGE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE, 4, 3, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_MESSAGE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MESSAGE, 1, 0, OpType::All));
        qscriptSpec["OPCODE_ERROR"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_ERROR, 1, 0, OpType::All));
        qscriptSpec["OPCODE_GOTO"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_GOTO, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_EXIT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_EXIT, 0, 0));
        qscriptSpec["OPCODE_CALL_SCRIPT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CALL_SCRIPT, 1, 1, OpType::String));
        qscriptSpec["OPCODE_NEXT_SCRIPT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_NEXT_SCRIPT, 1, 1, OpType::String));
        qscriptSpec["OPCODE_MULTI_TOUCHDOWN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHDOWN, 4, 3, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_MULTI_TOUCHUP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHUP, 4, 3, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_MULTI_TOUCHMOVE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHMOVE, 4, 3, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_MULTI_TOUCHDOWN_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHDOWN_HORIZONTAL, 3, 3, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_MULTI_TOUCHUP_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHUP_HORIZONTAL, 3, 3, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_MULTI_TOUCHMOVE_HORIZONTAL"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MULTI_TOUCHMOVE_HORIZONTAL, 3, 3, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CALL_EXTERNAL_SCRIPT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CALL_EXTERNAL_SCRIPT, 1, 1, OpType::All));
        qscriptSpec["OPCODE_ELAPSED_TIME"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_ELAPSED_TIME, 0, 0));
        qscriptSpec["OPCODE_FPS_MEASURE_START"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_FPS_MEASURE_START, 1, 0, OpType::String));
        qscriptSpec["OPCODE_FPS_MEASURE_STOP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_FPS_MEASURE_STOP, 1, 0, OpType::String));
        qscriptSpec["OPCODE_CONCURRENT_IF_MATCH"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CONCURRENT_IF_MATCH, 3, 2, OpType::String, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_BATCH_PROCESSING"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_BATCH_PROCESSING, 1, 1, OpType::String));
        // TODO: the following four are not compatible since the first operand now does not support whitespace
        qscriptSpec["OPCODE_IF_MATCH_TXT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_MATCH_TXT, 4, 4, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_IF_MATCH_REGEX"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_MATCH_REGEX, 4, 4, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CLICK_MATCHED_TXT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLICK_MATCHED_TXT, 5, 4, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CLICK_MATCHED_REGEX"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLICK_MATCHED_REGEX, 5, 4, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_AUDIO_RECORD_START"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AUDIO_RECORD_START, 1, 1, OpType::String));
        qscriptSpec["OPCODE_AUDIO_RECORD_STOP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AUDIO_RECORD_STOP, 1, 1, OpType::String));
        qscriptSpec["OPCODE_AUDIO_PLAY"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AUDIO_PLAY, 1, 1, OpType::String));
        qscriptSpec["OPCODE_IF_MATCH_AUDIO"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_MATCH_AUDIO, 4, 4, OpType::String, OpType::String, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_AI_BUILTIN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AI_BUILTIN, 1, 1, OpType::String));
        qscriptSpec["OPCODE_IMAGE_MATCHED_TIME"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IMAGE_MATCHED_TIME, 1, 1, OpType::String));
        qscriptSpec["OPCODE_FLICK_ON"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_FLICK_ON, 0, 0));
        qscriptSpec["OPCODE_FLICK_OFF"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_FLICK_OFF, 0, 0));
        qscriptSpec["OPCODE_IF_MATCH_IMAGE_WAIT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT, 5, 4, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CLICK_MATCHED_IMAGE_XY"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLICK_MATCHED_IMAGE_XY, 6, 5, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY, 6, 5, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_TOUCHUP_MATCHED_IMAGE_XY"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY, 6, 5, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY, 6, 5, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Orientation));
        qscriptSpec["OPCODE_CLEAR_DATA"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CLEAR_DATA, 1, 0, OpType::String));
        qscriptSpec["OPCODE_MOUSE_EVENT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_MOUSE_EVENT, 4, 4, OpType::Integer, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_KEYBOARD_EVENT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_KEYBOARD_EVENT, 2, 2, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_IF_SOURCE_IMAGE_MATCH"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH, 5, 4, OpType::String, OpType::String, OpType::Integer, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_IF_SOURCE_LAYOUT_MATCH"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IF_SOURCE_LAYOUT_MATCH, 4, 4, OpType::String, OpType::String, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_CRASH_ON"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CRASH_ON, 0, 0));
        qscriptSpec["OPCODE_CRASH_OFF"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_CRASH_OFF, 0, 0));
        qscriptSpec["OPCODE_SET_VARIABLE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_SET_VARIABLE, 2, 2, OpType::String, OpType::Integer));
        qscriptSpec["OPCODE_LOOP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_LOOP, 2, 2, OpType::String, OpType::Integer));
        qscriptSpec["OPCODE_SET_TEXT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_SET_TEXT, 1, 1, OpType::String));
        qscriptSpec["OPCODE_EXPLORATORY_TEST"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_EXPLORATORY_TEST, 1, 1, OpType::String));
        qscriptSpec["OPCODE_POWER"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_POWER, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_LIGHT_UP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_LIGHT_UP, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_VOLUME_UP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_VOLUME_UP, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_VOLUME_DOWN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_VOLUME_DOWN, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_BRIGHTNESS"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_BRIGHTNESS, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_DEVICE_POWER"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_DEVICE_POWER, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_AUDIO_MATCH_ON"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AUDIO_MATCH_ON, 0, 0));
        qscriptSpec["OPCODE_AUDIO_MATCH_OFF"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_AUDIO_MATCH_OFF, 0, 0));
        qscriptSpec["OPCODE_HOLDER_UP"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_HOLDER_UP, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_HOLDER_DOWN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_HOLDER_DOWN, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_BUTTON"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_BUTTON, 1, 1, OpType::String));
        qscriptSpec["OPCODE_USB_IN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_USB_IN, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_USB_OUT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_USB_OUT, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_RANDOM_ACTION"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_RANDOM_ACTION, 1, 1, OpType::String));
        qscriptSpec["OPCODE_IMAGE_CHECK"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_IMAGE_CHECK, 3, 3, OpType::String, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTCLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTCLOCKWISE, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTANTICLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTANTICLOCKWISE, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_DUMP_UI_LAYOUT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_DUMP_UI_LAYOUT, 1, 1, OpType::String));
        qscriptSpec["OPCODE_EARPHONE_IN"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_EARPHONE_IN, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_EARPHONE_OUT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_EARPHONE_OUT, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_RELAY_CONNECT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_RELAY_CONNECT, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_RELAY_DISCONNECT"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_RELAY_DISCONNECT, 1, 1, OpType::Integer));
        qscriptSpec["OPCODE_TILTZAXISTO"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTZAXISTO, 2, 1, OpType::Integer, OpType::Integer));
        qscriptSpec["OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE, 1, 0, OpType::Integer));
        qscriptSpec["OPCODE_SENSOR_CHECK"] = boost::shared_ptr<Spec>(new Spec(QSEventAndAction::OPCODE_SENSOR_CHECK, 1, 1, OpType::Integer));

        for (auto spec : qscriptSpec)
        {
            opcodeNameTable[spec.second->Opcode()] = spec.first;
        }

        return DaVinciStatusSuccess;
    }

    string QScript::OpcodeToString(const int opcode)
    {
        if (QScript::opcodeNameTable.find(opcode) != QScript::opcodeNameTable.end())
        {
            return QScript::opcodeNameTable[opcode];
        }
        else
        {
            DAVINCI_LOG_ERROR << "Invalid opcode: " << boost::lexical_cast<string>(opcode);
            assert(false);
            return "";
        }
    }

    QScript::QScript(const string &filename) : qsFileName(system_complete(filename).string())
    {
        path scriptPath(qsFileName);
        scriptName = scriptPath.stem().string();
        qtsFileName = GetResourceFullPath(scriptName + ".qts");
        videoFileName = GetResourceFullPath(scriptName + ".avi");
        audioFileName = GetResourceFullPath(scriptName + ".wav");
        sensorCheckFileName = GetResourceFullPath(scriptName + "_SensorCheck.txt");
        // set default configurations
        SetConfiguration(ConfigCameraPresetting, "HIGH_RES");
        SetConfiguration(ConfigMultiLayerMode, "False");
    }

    QScript::~QScript()
    {
    }

    DaVinciStatus QScript::ParseConfigLine(const boost::shared_ptr<QScript> &qs, const string &line, unsigned int lineNumber)
    {
        string matchingStr = "CameraPresetting=";
        if (starts_with(line, matchingStr))
        {
            string presetStr = trim_copy(line.substr(matchingStr.length()));
            if (presetStr != "HIGH_SPEED" && presetStr != "HIGH_RES" && presetStr != "MIDDLE_RES")
            {
                DAVINCI_LOG_ERROR << "Error camera setting in Q script: " << presetStr;
            }
            qs->SetConfiguration(ConfigCameraPresetting, presetStr);
        }

        matchingStr = "PackageName=";
        if (starts_with(line, matchingStr))
        {
            string packageName = trim_copy(line.substr(matchingStr.length()));
            if (!packageName.empty())
            {
                qs->SetConfiguration(ConfigPackageName, packageName);
            }
        }

        matchingStr = "ActivityName=";
        if (starts_with(line, matchingStr))
        {
            string activityName = trim_copy(line.substr(matchingStr.length()));
            if (!activityName.empty())
            {
                qs->SetConfiguration(ConfigActivityName, activityName);
            }
        }

        matchingStr = "APKName=";
        if (starts_with(line, matchingStr))
        {
            string apkName = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigApkName, apkName);
            string apkLocation = qs->GetResourceFullPath(apkName);

            std::wstring wstrApkLocation = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(apkLocation);
            bool containNoneASCII = ContainNoneASCIIStr(wstrApkLocation);

            if (is_regular_file(apkLocation) || (containNoneASCII == true && is_regular_file(wstrApkLocation)))
            {
                vector<string> appInfo = AndroidTargetDevice::GetPackageActivity(apkLocation);
                if (appInfo.size() == AndroidTargetDevice::APP_INFO_SIZE)
                {
                    if (qs->GetConfiguration(ConfigPackageName).empty())
                    {
                        qs->SetConfiguration(ConfigPackageName, appInfo[AndroidTargetDevice::PACKAGE_NAME]);
                    }
                    if (qs->GetConfiguration(ConfigActivityName).empty())
                    {
                        qs->SetConfiguration(ConfigActivityName, appInfo[AndroidTargetDevice::ACTIVITY_NAME]);
                    }
                    if (qs->GetConfiguration(ConfigIconName).empty())
                    {
                        qs->SetConfiguration(ConfigIconName, appInfo[AndroidTargetDevice::APP_NAME]);
                    }
                }
            }
        }
        /*
        if (!packageName.empty() && !activityName.empty() && activityName.find("/") == string::npos)
        {
        activityName = packageName + "/" + activityName;
        }
        */

        matchingStr = "PushData=";
        if (starts_with(line, matchingStr))
        {
            string pushData = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigPushData, pushData);
        }

        matchingStr = "StatusBarHeight=";
        if (starts_with(line, matchingStr))
        {
            string statusBarHeight = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigStatusBarHeight, statusBarHeight);
        }

        matchingStr = "NavigationBarHeight=";
        if (starts_with(line, matchingStr))
        {
            string navigationBarHeight = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigNavigationBarHeight, navigationBarHeight);
        }

        matchingStr = "StartOrientation=";
        if (starts_with(line, matchingStr))
        {
            string startOrientation = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigStartOrientation, startOrientation);
        }

        matchingStr = "CameraType=";
        if (starts_with(line, matchingStr))
        {
            string cameraType = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigCameraType, cameraType);
        }

        matchingStr = "VideoRecording=";
        if (starts_with(line, matchingStr))
        {
            string videoRecording = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigVideoRecording, videoRecording);
        }

        matchingStr = "MultiLayerMode=";
        if (starts_with(line, matchingStr))
        {
            string multiLayerMode = trim_copy(line.substr(matchingStr.length()));
            if ((multiLayerMode.empty()) || (boost::to_lower_copy(multiLayerMode) == "false"))
                multiLayerMode = "False";
            else 
                multiLayerMode = "True";

            qs->SetConfiguration(ConfigMultiLayerMode, multiLayerMode);
        }

        matchingStr = ConfigRecordedVideoType + "=";
        if (starts_with(line, matchingStr))
        {
            string recordedVideoType = trim_copy(line.substr(matchingStr.length()));
            if (recordedVideoType.empty())
            {
                recordedVideoType = ".avi";
            }
            qs->SetConfiguration(ConfigRecordedVideoType, recordedVideoType);
        }

        matchingStr = ConfigReplayVideoType + "=";
        if (starts_with(line, matchingStr))
        {
            string replayVideoType = trim_copy(line.substr(matchingStr.length()));
            if (replayVideoType.empty())
            {
                replayVideoType = ".avi";
            }
            qs->SetConfiguration(ConfigReplayVideoType, replayVideoType);
        }

        matchingStr = "OSVersion=";
        if (starts_with(line, matchingStr))
        {
            string strOSversion = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigOSversion, strOSversion);
        }

        matchingStr = "DPI=";
        if (starts_with(line, matchingStr))
        {
            string dpi = trim_copy(line.substr(matchingStr.length()));
            qs->SetConfiguration(ConfigDPI, dpi);
        }

        matchingStr = "Resolution=";
        if (starts_with(line, matchingStr))
        {
            string resolution = trim_copy(line.substr(matchingStr.length()));
            boost::to_lower(resolution);
            vector<string> items;
            boost::algorithm::split(items, resolution, boost::algorithm::is_any_of("x"));
            if(items.size() != 2)
            {
                DAVINCI_LOG_WARNING << "Use default device resolution." << endl;
                qs->SetConfiguration(ConfigResolutionWidth, "1200");
                qs->SetConfiguration(ConfigResolutionHeight, "1920");
            }
            else
            {
                qs->SetConfiguration(ConfigResolutionWidth, items[0]);
                qs->SetConfiguration(ConfigResolutionHeight, items[1]);
            }
        }

        return DaVinciStatusSuccess;
    }

    void QScript::SplitEventsAndActionsLine(const string &line, vector<string> &columns)
    {
        // Columns are split by whitespaces, e.g.:
        // 6       44,824.5056: OPCODE_IF_MATCH_IMAGE_WAIT air.com.sarahnorthway.rebuild2_PictureShot1.png 7 7 5000
        // 
        // A column can be wrapped with quotes between which one or more whitespace
        // may appear, e.g.:
        // 6       44,824.5056: OPCODE_IF_MATCH_IMAGE_WAIT "c:\hello world\air.com.sarahnorthway.rebuild2_PictureShot1.png" 7 7 5000
        // The image name is c:\hello world\air.com.sarahnorthway.rebuild2_PictureShot1.png
        // 
        // Quotes are escaped with quote, e.g.:
        // 6       44,824.5056: OPCODE_IF_MATCH_IMAGE_WAIT "c:\hello world""\air.com.sarahnorthway.rebuild2_PictureShot1.png" 7 7 5000
        // The image name is c:\hello world"\air.com.sarahnorthway.rebuild2_PictureShot1.pn
        // 
        ostringstream oss;
        bool inQuote = false;
        bool behindQuote = false;
        for (size_t i = 0; i < line.length(); i++)
        {
            const char c = line[i];
            switch (c)
            {
            case '\"':
                if (behindQuote)
                {
                    behindQuote = false;
                    oss << c;
                }
                else
                {
                    behindQuote = true;
                }
                inQuote = !inQuote;
                break;
            case ' ':
            case '\t':
                behindQuote = false;
                if (inQuote)
                {
                    oss << c;
                }
                else if (oss.str().size() > 0)
                {
                    columns.push_back(oss.str());
                    oss.str("");
                    oss.clear();
                }
                break;
            default:
                behindQuote = false;
                oss << c;
                break;
            }
        }
        if (oss.str().size() > 0)
        {
            columns.push_back(oss.str());
        }
    }

    string QScript::ProcessQuoteSpaces(const string &processStr)
    {
        if (processStr.find(" ") != string::npos)
        {
            string tmpStr = processStr;
            if (processStr.find("\"") == string::npos)
            {
                tmpStr.insert(0, "\"");
                tmpStr.append("\"");
                return tmpStr;
            }
            else
            {
                boost::replace_all(tmpStr, "\"", "\"\"");
                tmpStr.insert(0, "\"");
                tmpStr.append("\"");
                return tmpStr;
            }
        }
        else
        {
            return processStr;
        }
    }

    DaVinciStatus QScript::ParseEventsAndActionsLine(const boost::shared_ptr<QScript> &qs, const string &line, unsigned int lineNumber)
    {
        vector<string> columns;
        SplitEventsAndActionsLine(line, columns);
        // Have at least three columns: line#, timestamp, opcode
        if (columns.size() < 3)
        {
            DAVINCI_LOG_ERROR << "Expect at least three columns";
            return errc::invalid_argument;
        }
        int label;
        if (!TryParse(columns[0], label))
        {
            DAVINCI_LOG_ERROR << "Expect integer label but got " << columns[0];
            return errc::invalid_argument;
        }
        double ts;
        if (columns[1].find(":") != string::npos)
        {
            columns[1] = columns[1].substr(0, columns[1].find(":"));
        }
        replace_all(columns[1], ",", "");
        if (!TryParse(columns[1], ts))
        {
            DAVINCI_LOG_ERROR << "Expect double timestamp but got " << columns[1];
            return errc::invalid_argument;
        }
        string opcStr = columns[2];
        if (qscriptSpec.find(opcStr) == qscriptSpec.end())
        {
            DAVINCI_LOG_ERROR << "Unknown opcode " << opcStr;
            return errc::invalid_argument;
        }
        QSEventAndAction::Spec & spec = *qscriptSpec[opcStr];
        int numOperands = (int)(columns.size() - 3);
        vector<int> intOprs;
        vector<string> strOprs;
        Orientation o = Orientation::Unknown;
        if (numOperands < static_cast<int>(spec.NumMandatoryOperands()))
        {
            DAVINCI_LOG_ERROR << "Expect at least " << spec.NumMandatoryOperands() << " but got " << numOperands;
            return errc::invalid_argument;
        }
        if (spec.NumOperands() == 1 && spec[0] == QSEventAndAction::Spec::OperandType::All)
        {
            ostringstream oss;
            for (int i = 0; i < numOperands; i++)
            {
                oss << columns[i + 3] << " ";
            }
            strOprs.push_back(oss.str());
        }
        else
        {
            for (unsigned int i = 0; i < spec.NumOperands() && static_cast<int>(i) < numOperands; i++)
            {
                if (spec[i] == QSEventAndAction::Spec::OperandType::Integer)
                {
                    int opr = 0;
                    if (!TryParse(columns[i + 3], opr))
                    {
                        DAVINCI_LOG_ERROR << "Expect integer operand but got " << columns[i + 3];
                        return errc::invalid_argument;
                    }
                    intOprs.push_back(opr);
                }
                else if (spec[i] == QSEventAndAction::Spec::OperandType::Orientation)
                {
                    int opr;
                    if (!TryParse(columns[i + 3], opr))
                    {
                        DAVINCI_LOG_ERROR << "Expect integer operand as orientation but got " << columns[i + 3];
                        return errc::invalid_argument;
                    }
                    o = IntergerToOrientation(opr);
                }
                else
                {
                    strOprs.push_back(columns[i + 3]);
                }
            }
        }
        boost::shared_ptr<QSEventAndAction> qevent = boost::shared_ptr<QSEventAndAction>(
            new QSEventAndAction(ts, spec.Opcode(), intOprs, strOprs, o));        
        qevent->LineNumber() = label;
        qs->AppendEventAndAction(qevent);
        return DaVinciStatusSuccess;
    }

    DaVinciStatus QScript::ParseTimeStampFile(const string &qts, vector<double> & timeStamps, vector<QScript::TraceInfo> & traces)
    {
        path scriptPath(qts);
        if (extension(scriptPath) != ".qts")
        {
            DAVINCI_LOG_ERROR << "Q timestamp file invalid: " << qts;
            return errc::invalid_argument;
        }
        else
        {
            if(!is_regular_file(qts))
            {
                DAVINCI_LOG_WARNING << "Q timestamp file not found: " << qts;
                return errc::no_such_file_or_directory;
            }
        }

        bool oldVersion = false;

        vector<string> qtsLines;
        DaVinciStatus status = ReadAllLines(qts, qtsLines);

        if (qtsLines[0].find("Version") == string::npos)
            oldVersion = true;

        if (qtsLines.size() >= 2 && qtsLines[1] == "")
        {
            string s;
            for (size_t i = 0; i < qtsLines.size() / 2; i++)
            {
                s = qtsLines[i * 2];
                boost::trim(s);
                if(s.length() > 0 && s[0] >= '0' && s[0] <= '9')
                {
                    timeStamps.push_back(boost::lexical_cast<double>(s));
                }
            }
        }
        else
        {
            string s;
            for (size_t i = 0; i < qtsLines.size(); i++)
            {
                s = qtsLines[i];

                // TODO: The code will be abstract a QTS class member method
                std::vector<std::string> split_vector;
                boost::split(split_vector, s, boost::is_any_of(" "), boost::token_compress_on);
                assert(split_vector.size() > 0);
                s = split_vector[0];

                boost::trim(s);
                if(s.length() > 0 && s[0] >= '0' && s[0] <= '9')
                {
                    timeStamps.push_back(boost::lexical_cast<double>(s));
                }
            }
        }

        vector<string> traceList;
        for(auto line : qtsLines)
        {
            if(line.length() > 0 && line[0] == ':')
                traceList.push_back(line);
        }

        if (oldVersion)
        {
            for (size_t i = 0; i < timeStamps.size(); i++)
                timeStamps[i] = timeStamps[i] / ((double)cv::getTickCount() * 1000.0 / cv::getTickFrequency());
        }

        for (size_t i = 0; i < traceList.size(); i++)
        {
            vector<string> components;
            string s = traceList[i];
            boost::trim(s);

            split(components, s, is_any_of(":"));

            if (components.size() >= 3)
            {
                int labelValue = boost::lexical_cast<int>(components[1]);
                double timeValue = boost::lexical_cast<double>(components[2]);
                if (oldVersion)
                {
                    timeValue = timeValue / ((double)cv::getTickCount() * 1000.0 / cv::getTickFrequency());
                }
                traces.push_back(TraceInfo(labelValue, timeValue));
            }
            else
                continue;
        }

        DAVINCI_LOG_INFO << "Q timestamp file loaded: " << qts;
        return DaVinciStatusSuccess;
    }

    bool QScript::IsTouchPaired()
    {
        // Sanity check if the jump targets are valid.
        bool touchDownMark = false;
        bool isPair = true;
        bool outOfLoop = false;
        for (int ip = 1; ip <= static_cast<int>(NumEventAndActions()); ip++)
        {
            boost::shared_ptr<QSEventAndAction> e = EventAndAction(ip);
            int op = e->Opcode();

            switch (op)
            {
            case QSEventAndAction::OPCODE_TOUCHDOWN:
            case QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL:
            case QSEventAndAction::OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY:
                {
                    if (!touchDownMark)
                    {
                        touchDownMark = true;
                        isPair = false;
                    }
                    else
                    {
                        outOfLoop = true;
                    }
                }
                break;
            case QSEventAndAction::OPCODE_TOUCHMOVE:
            case QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL:
            case QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY:
                {
                    if (!touchDownMark)
                    {
                        isPair = false;
                        outOfLoop = true;
                    }
                }
                break;
            case QSEventAndAction::OPCODE_TOUCHUP:
            case QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL:
            case QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY:
                {
                    if (!touchDownMark)
                    {
                        isPair = false;
                        outOfLoop = true;
                    }
                    else
                    {
                        isPair = true;
                        touchDownMark = false;
                    }
                }
                break;
            default:
                break;
            }

            if (outOfLoop)
                break;
        }

        if (!isPair)
        {
            DAVINCI_LOG_ERROR << "  Touch actions don't match.";
        }

        return isPair;
    }

    boost::shared_ptr<QScript> QScript::Load(const string &filename)
    {
        path scriptPath(filename);
        if (!is_regular_file(scriptPath) || extension(scriptPath) != ".qs")
        {
            DAVINCI_LOG_ERROR << "Q script file invalid or not found: " << filename;
            return nullptr;
        }
        auto qs = boost::shared_ptr<QScript>(new QScript(system_complete(filename).string()));

        DAVINCI_LOG_INFO << "Loading Q script file: " << qs->qsFileName;
        vector<string> qsLines;
        DaVinciStatus status = ReadAllLines(qs->qsFileName, qsLines);
        if (!DaVinciSuccess(status))
        {
            DAVINCI_LOG_ERROR << "Cannot load Q script file " << qs->qsFileName << " for parsing: " << status.message();
            return nullptr;
        }

        const int BEFORE_PARSING_CONFIG = 0;
        const int IN_PARSING_CONFIG = 1;
        const int IN_PARSING_EVENTS_AND_ACTIONS = 2;
        int parsingStage = BEFORE_PARSING_CONFIG;
        unsigned int lineNumber = 0;
        unsigned int numConfigEntries = 0;
        for (auto lineIter = qsLines.begin(); lineIter != qsLines.end(); lineIter++, lineNumber++)
        {
            string line = trim_copy(*lineIter);
            if (line.empty())
            {
                continue;
            }
            switch (parsingStage)
            {
            case BEFORE_PARSING_CONFIG:
                if (line != "[Configuration]")
                {
                    DAVINCI_LOG_ERROR << "Expect [Configuration] section at the beginning of Q script";
                    return nullptr;
                }
                parsingStage = IN_PARSING_CONFIG;
                break;
            case IN_PARSING_CONFIG:
                if (line == "[Events and Actions]")
                {
                    parsingStage = IN_PARSING_EVENTS_AND_ACTIONS;
                }
                else
                {
                    status = ParseConfigLine(qs, line, lineNumber);
                    if (!DaVinciSuccess(status))
                    {
                        DAVINCI_LOG_ERROR << "Cannot parse configuration @" << lineNumber << ": " << line;
                        return nullptr;
                    }
                    numConfigEntries++;
                }
                break;
            case IN_PARSING_EVENTS_AND_ACTIONS:
                status = ParseEventsAndActionsLine(qs, line, lineNumber);
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Cannot parse events and actions @" << lineNumber << ": " << line;
                    return nullptr;
                }
                break;
            default:
                assert(false);
                lineIter = qsLines.end();
                break;
            }
        }

        // Check whether touch actions are paired or not
        if (!qs->IsTouchPaired())
            return nullptr;

        // Sanity check if the jump targets are valid.
        for (int ip = 1; ip <= static_cast<int>(qs->NumEventAndActions()); ip++)
        {
            int ip1, ip2;
            boost::shared_ptr<QSEventAndAction> e = qs->EventAndAction(ip);
            switch (e->Opcode())
            {
            case QSEventAndAction::OPCODE_IF_MATCH_IMAGE:
            case QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT:
            case QSEventAndAction::OPCODE_IF_MATCH_AUDIO:
            case QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH:
            case QSEventAndAction::OPCODE_IF_SOURCE_LAYOUT_MATCH:
                ip1 = qs->LineNumberToIp(e->IntOperand(0));
                if (ip1 <= 0)
                {
                    DAVINCI_LOG_ERROR << "  Q script error at line " << e->LineNumber() << ": unknown jump target " << e->IntOperand(0);
                    return nullptr;
                }
                ip2 = qs->LineNumberToIp(e->IntOperand(1));
                if (ip2 <= 0)
                {
                    DAVINCI_LOG_ERROR << "  Q script error at line " << e->LineNumber() << ": unknown jump target " << e->IntOperand(1);
                    return nullptr;
                }
                break;
            case QSEventAndAction::OPCODE_GOTO:
            case QSEventAndAction::OPCODE_LOOP:
                ip1 = qs->LineNumberToIp(e->IntOperand(0));
                if (ip1 <= 0)
                {
                    DAVINCI_LOG_ERROR << "  Q script error at line " << e->LineNumber() << ": unknown jump target " << e->IntOperand(0);
                    return nullptr;
                }
                break;
            case QSEventAndAction::OPCODE_IF_MATCH_TXT:
            case QSEventAndAction::OPCODE_IF_MATCH_REGEX:
                ip1 = qs->LineNumberToIp(e->IntOperand(1));
                if (ip1 <= 0)
                {
                    DAVINCI_LOG_ERROR << "  Q script error at line " << e->LineNumber() << ": unknown jump target " << e->IntOperand(1);
                    return nullptr;
                }
                ip2 = qs->LineNumberToIp(e->IntOperand(2));
                if (ip2 <= 0)
                {
                    DAVINCI_LOG_ERROR << "  Q script error at line " << e->LineNumber() << ": unknown jump target " << e->IntOperand(2);
                    return nullptr;
                }
                break;
            default:
                break;
            }
        }

        qs->RefreshLineNumberToIpMap();
        DAVINCI_LOG_INFO << "  Loaded " << numConfigEntries << " configuration items, " << qs->NumEventAndActions() <<" events and actions successfully.";

        string qtsName = scriptPath.replace_extension(".qts").string();
        if (!DaVinciSuccess(qs->ParseTimeStampFile(qtsName, qs->TimeStamp(), qs->Trace())))
        {
            DAVINCI_LOG_WARNING << "Cannot parse timestamp file" << endl;
        }

        return qs;
    }

    boost::shared_ptr<QScript> QScript::New(const string &filename)
    {
        path scriptPath(filename);
        if (exists(scriptPath))
        {
            return nullptr;
        }
        auto qs = boost::shared_ptr<QScript>(new QScript(system_complete(filename).string()));
        return qs;
    }

    DaVinciStatus QScript::Flush()
    {
#define OUTPUT_CONFIG_ENTRY(config) config << "=" << GetConfiguration(config)
        ofstream ofs(qsFileName);
        ofs.precision(4);
        ofs << "[Configuration]" << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigCameraPresetting) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigPackageName) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigActivityName) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigIconName) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigApkName) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigPushData) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigStatusBarHeight) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigNavigationBarHeight) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigStartOrientation) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigCameraType) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigVideoRecording) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigMultiLayerMode) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigRecordedVideoType) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigReplayVideoType) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigOSversion) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigDPI) << endl;
        ofs << OUTPUT_CONFIG_ENTRY(ConfigResolution) << endl;
        ofs << "[Events and Actions]" << endl;

        for (auto qsEvent : eventAndActions)
        {
            string opcodeName = opcodeNameTable[qsEvent->Opcode()];
            boost::shared_ptr<QSEventAndAction::Spec> spec = qscriptSpec[opcodeName];
            // For long time host recording (unit: hour), the width between label and timestamp should be not smaller than 12 (e.g.,3600000.0000)
            ofs << qsEvent->LineNumber() << fixed << setfill(' ') << setw(20) << qsEvent->TimeStamp() << ": " << opcodeName;
            unsigned int intOprIndex = 0, strOprIndex = 0;
            for (unsigned int i = 0; i < spec->NumOperands(); i++)
            {
                switch ((*spec)[i])
                {
                case QSEventAndAction::Spec::OperandType::All:
                    ofs << " " << qsEvent->StrOperand(strOprIndex);
                    break;
                case QSEventAndAction::Spec::OperandType::Integer:
                    ofs << " " << qsEvent->IntOperand(intOprIndex++);
                    break;
                case QSEventAndAction::Spec::OperandType::Orientation:
                    ofs << " " << (int)qsEvent->GetOrientation();
                    break;
                case QSEventAndAction::Spec::OperandType::String:
                    ofs << " " << qsEvent->StrOperand(strOprIndex++);
                    break;
                default:
                    DAVINCI_LOG_ERROR << "Unexpected operand type " << static_cast<int>((*spec)[i]) << " in opcode " << opcodeName;
                    assert(false);
                    return errc::argument_out_of_domain;
                }
                if ((*spec)[i] == QSEventAndAction::Spec::OperandType::All)
                {
                    break;
                }
            }
            ofs << endl;
        }
        return DaVinciStatusSuccess;
    }

    string QScript::GetResourceFullPath(const string &resPath)
    {
        path scriptPath(qsFileName);
        path resourcePath(resPath);
        if (resourcePath.is_complete())
        {
            return resPath;
        }
        else
        {
            path resourceFullPath = scriptPath.parent_path();
            resourceFullPath /= resPath;
            return resourceFullPath.string();
        }
    }

    string QScript::GetQsFileName()
    {
        return qsFileName;
    }

    string QScript::GetQtsFileName()
    {
        return qtsFileName;
    }

    string QScript::GetVideoFileName()
    {
        std::string recordedVideoType = GetConfiguration(ConfigRecordedVideoType);
        boost::filesystem::path recordedVideoFilePath(videoFileName);
        if (recordedVideoType.length() > 0)
        {
            boost::to_lower(recordedVideoType);
            recordedVideoFilePath.replace_extension(recordedVideoType);
        }
        return recordedVideoFilePath.string();
    }

    string QScript::GetAudioFileName()
    {
        return audioFileName;
    }

    string QScript::GetSensorCheckFileName()
    {
        return sensorCheckFileName;
    }

    string QScript::GetScriptName()
    {
        return scriptName;
    }   

    string QScript::GetConfiguration(const string &key)
    {
        return configurationEntries[key];
    }

    DaVinciStatus QScript::SetConfiguration(const string &key, const string &value)
    {
        configurationEntries[key] = value;
        return DaVinciStatusSuccess;
    }

    unsigned int QScript::NumEventAndActions()
    {
        return (unsigned int)(eventAndActions.size());
    }

    boost::shared_ptr<QSEventAndAction> QScript::EventAndAction(int ip)
    {
        if (ip <= 0 || ip > static_cast<int>(eventAndActions.size()))
        {
            return nullptr;
        }
        else
        {
            return eventAndActions[ip - 1];
        }
    }

    void QScript::SetEventOrActionTimeStampReplay(int ip, double timeStamp)
    {
        boost::shared_ptr<QSEventAndAction> qevent = EventAndAction(ip);
        if (qevent != nullptr)
        {
            qevent->TimeStampReplay() = timeStamp;
            trace.push_back(TraceInfo(qevent->LineNumber(), timeStamp));
        }
    }

    DaVinciStatus QScript::AppendEventAndAction(const boost::shared_ptr<QSEventAndAction> &eventAndAction)
    {
        return InsertEventAndAction(eventAndAction, (int)(eventAndActions.size()) + 1);
    }

    bool QScript::UsedHyperSoftCam()
    {
        string cameraType = GetConfiguration(ConfigCameraType);
        if (!cameraType.empty() &&
            (boost::starts_with(to_lower_copy(cameraType), "hypersoftcam") ||
            boost::starts_with(to_lower_copy(cameraType), "cameraless")))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool QScript::IsStartHorizontal()
    {
        string startOrientationStr = GetConfiguration(ConfigStartOrientation);
        if (to_lower_copy(startOrientationStr) == "horizontal")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    string QScript::GetPackageName()
    {
        return GetConfiguration(ConfigPackageName);
    }

    string QScript::GetActivityName()
    {
        return GetConfiguration(ConfigActivityName);
    }

    string QScript::GetApkName()
    {
        return GetConfiguration(ConfigApkName);
    }

    string QScript::GetIconName()
    {
        return GetConfiguration(ConfigIconName);
    }

    string QScript::GetOSversion()
    {
        return GetConfiguration(ConfigOSversion);
    }

    string QScript::GetDPI()
    {
        return GetConfiguration(ConfigDPI);
    }

    // Suppose default recording device is N7 (1200x1920)
    string QScript::GetResolutionWidth()
    {
        string width = GetConfiguration(ConfigResolutionWidth);
        if(width == "")
            width = "1200";

        return width;
    }

    string QScript::GetResolutionHeight()
    {
        string height = GetConfiguration(ConfigResolutionHeight);
        if(height == "")
            height = "1920";

        return height;
    }

    vector<int> QScript::GetStatusBarHeight() 
    {
        string heightStr = GetConfiguration(ConfigStatusBarHeight);
        vector<string> items;
        split(items, heightStr, is_any_of("-"));

        int portraitStatusBarHeight = 0, landscapeStatusBarHeight = 0;
        if(items.size() == 2)
        {
            TryParse(items[0], portraitStatusBarHeight);
            TryParse(items[1], landscapeStatusBarHeight);
        }

        vector<int> heights;
        heights.push_back(portraitStatusBarHeight);
        heights.push_back(landscapeStatusBarHeight);

        return heights;
    }

    vector<int> QScript::GetNavigationBarHeight() 
    {
        string heightStr = GetConfiguration(ConfigNavigationBarHeight);
        vector<string> items;
        split(items, heightStr, is_any_of("-"));

        int portraitNavigationBarHeight = 0, landscapeNavigationBarHeight = 0;
        if(items.size() == 2)
        {
            TryParse(items[0], portraitNavigationBarHeight);
            TryParse(items[1], landscapeNavigationBarHeight);
        }

        vector<int> heights;
        heights.push_back(portraitNavigationBarHeight);
        heights.push_back(landscapeNavigationBarHeight);

        return heights;
    }

    bool QScript::NeedVideoRecording()
    {
        string needVideoRecording = GetConfiguration(ConfigVideoRecording);
        if (boost::to_lower_copy(needVideoRecording) == "false")
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool QScript::IsMultiLayerMode()
    {
        string multiLayerMode = GetConfiguration(ConfigMultiLayerMode);
        if (boost::to_lower_copy(multiLayerMode) == "true")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool QScript::IsOpcodeInTouch(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_TOUCHMOVE ||
            opcode == QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL ||
            opcode == QSEventAndAction::OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY ||
            opcode == QSEventAndAction::OPCODE_MULTI_TOUCHMOVE ||
            opcode == QSEventAndAction::OPCODE_MULTI_TOUCHMOVE_HORIZONTAL ||
            opcode == QSEventAndAction::OPCODE_TOUCHUP ||
            opcode == QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL ||
            opcode == QSEventAndAction::OPCODE_TOUCHUP_MATCHED_IMAGE_XY ||
            opcode == QSEventAndAction::OPCODE_MULTI_TOUCHUP ||
            opcode == QSEventAndAction::OPCODE_MULTI_TOUCHUP_HORIZONTAL;
    }

    bool QScript::IsOpcodeVirtualNavigation(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_HOME ||
            opcode == QSEventAndAction::OPCODE_BACK ||
            opcode == QSEventAndAction::OPCODE_MENU;
    }

    bool QScript::IsOpcodeCrash(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_CRASH_ON || opcode == QSEventAndAction::OPCODE_CRASH_OFF;
    }

    bool QScript::IsOpcodeTouch(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_TOUCHDOWN ||
            opcode == QSEventAndAction::OPCODE_TOUCHUP ||
            opcode == QSEventAndAction::OPCODE_TOUCHDOWN_HORIZONTAL ||
            opcode == QSEventAndAction::OPCODE_TOUCHUP_HORIZONTAL ||
            opcode == QSEventAndAction::OPCODE_TOUCHMOVE ||
            opcode == QSEventAndAction::OPCODE_TOUCHMOVE_HORIZONTAL;
    }

    bool QScript::IsOpcodeForTilt(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_TILTANTICLOCKWISE ||
            opcode == QSEventAndAction::OPCODE_TILTCLOCKWISE ||
            opcode == QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE||
            opcode == QSEventAndAction::OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE ||
            opcode == QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE ||
            opcode == QSEventAndAction::OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE ||
            opcode == QSEventAndAction::OPCODE_TILTDOWN ||
            opcode == QSEventAndAction::OPCODE_TILTLEFT ||
            opcode == QSEventAndAction::OPCODE_TILTRIGHT ||
            opcode == QSEventAndAction::OPCODE_TILTTO ||
            opcode == QSEventAndAction::OPCODE_TILTUP ||
            opcode == QSEventAndAction::OPCODE_TILTZAXISTO;
    }

    bool QScript::IsOpcodeNonTouch(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_BACK ||
            opcode == QSEventAndAction::OPCODE_HOME ||
            opcode == QSEventAndAction::OPCODE_MENU ||
            opcode == QSEventAndAction::OPCODE_SET_TEXT ||
            opcode == QSEventAndAction::OPCODE_VOLUME_DOWN||
            opcode == QSEventAndAction::OPCODE_VOLUME_UP ||
            opcode == QSEventAndAction::OPCODE_DEVICE_POWER ||
            opcode == QSEventAndAction::OPCODE_BRIGHTNESS ||
            opcode == QSEventAndAction::OPCODE_POWER ||
            opcode == QSEventAndAction::OPCODE_LIGHT_UP ||
            opcode == QSEventAndAction::OPCODE_IMAGE_CHECK ||
            opcode == QSEventAndAction::OPCODE_INSTALL_APP ||
            opcode == QSEventAndAction::OPCODE_PUSH_DATA ||
            opcode == QSEventAndAction::OPCODE_CLEAR_DATA ||
            opcode == QSEventAndAction::OPCODE_STOP_APP ||
            opcode == QSEventAndAction::OPCODE_UNINSTALL_APP ||
            opcode == QSEventAndAction::OPCODE_START_APP ||
            opcode == QSEventAndAction::OPCODE_MESSAGE ||
            opcode == QSEventAndAction::OPCODE_EXIT ||
            opcode == QSEventAndAction::OPCODE_FPS_MEASURE_START ||
            opcode == QSEventAndAction::OPCODE_FPS_MEASURE_STOP ||
            opcode == QSEventAndAction::OPCODE_DUMP_UI_LAYOUT ||
            opcode == QSEventAndAction::OPCODE_AI_BUILTIN ||
            opcode == QSEventAndAction::OPCODE_AUDIO_PLAY || 
            opcode == QSEventAndAction::OPCODE_CRASH_ON || 
            opcode == QSEventAndAction::OPCODE_CRASH_OFF || 
            opcode == QSEventAndAction::OPCODE_IF_MATCH_IMAGE || 
            opcode == QSEventAndAction::OPCODE_DRAG || 
            opcode == QSEventAndAction::OPCODE_BUTTON || 
            IsOpcodeForTilt(opcode);
    }

    bool QScript::IsOpcodeNonTouchShowFrame(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_BACK ||
            opcode == QSEventAndAction::OPCODE_HOME ||
            opcode == QSEventAndAction::OPCODE_MENU ||
            // opcode == QSEventAndAction::OPCODE_SET_TEXT: next action shows the right frame of OPCODE_SET_TEXT 
            opcode == QSEventAndAction::OPCODE_VOLUME_DOWN||
            opcode == QSEventAndAction::OPCODE_VOLUME_UP ||
            opcode == QSEventAndAction::OPCODE_DEVICE_POWER ||
            opcode == QSEventAndAction::OPCODE_BRIGHTNESS ||
            opcode == QSEventAndAction::OPCODE_POWER ||
            opcode == QSEventAndAction::OPCODE_LIGHT_UP ||
            opcode == QSEventAndAction::OPCODE_IMAGE_CHECK ||
            opcode == QSEventAndAction::OPCODE_PUSH_DATA ||
            opcode == QSEventAndAction::OPCODE_CLEAR_DATA ||
            opcode == QSEventAndAction::OPCODE_MESSAGE ||
            opcode == QSEventAndAction::OPCODE_FPS_MEASURE_START ||
            opcode == QSEventAndAction::OPCODE_FPS_MEASURE_STOP ||
            opcode == QSEventAndAction::OPCODE_DUMP_UI_LAYOUT ||
            opcode == QSEventAndAction::OPCODE_AI_BUILTIN ||
            opcode == QSEventAndAction::OPCODE_AUDIO_PLAY || 
            IsOpcodeForTilt(opcode);
    }

    bool QScript::IsOpcodeNonTouchBeforeAppStart(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_UNINSTALL_APP ||
            opcode == QSEventAndAction::OPCODE_INSTALL_APP ||
            opcode == QSEventAndAction::OPCODE_START_APP;
    }

    bool QScript::IsOpcodeNonTouchCheckProcess(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_SET_TEXT ||
            opcode == QSEventAndAction::OPCODE_VOLUME_DOWN||
            opcode == QSEventAndAction::OPCODE_VOLUME_UP ||
            opcode == QSEventAndAction::OPCODE_DEVICE_POWER ||
            opcode == QSEventAndAction::OPCODE_BRIGHTNESS ||
            opcode == QSEventAndAction::OPCODE_POWER ||
            opcode == QSEventAndAction::OPCODE_LIGHT_UP ||
            opcode == QSEventAndAction::OPCODE_START_APP ||
            opcode == QSEventAndAction::OPCODE_MESSAGE ||
            opcode == QSEventAndAction::OPCODE_AI_BUILTIN || 
            opcode == QSEventAndAction::OPCODE_AUDIO_PLAY || 
            IsOpcodeForTilt(opcode);
    }

    bool QScript::IsOpcodeIfCondition(int opcode)
    {
        return opcode == QSEventAndAction::OPCODE_IF_MATCH_IMAGE ||
            opcode == QSEventAndAction::OPCODE_IF_MATCH_IMAGE_WAIT ||
            opcode ==  QSEventAndAction::OPCODE_IF_SOURCE_IMAGE_MATCH || 
            opcode ==  QSEventAndAction::OPCODE_IF_MATCH_AUDIO || 
            opcode ==  QSEventAndAction::OPCODE_IF_MATCH_REGEX || 
            opcode ==  QSEventAndAction::OPCODE_IF_MATCH_TXT || 
            opcode ==  QSEventAndAction::OPCODE_IF_SOURCE_LAYOUT_MATCH;
    }

    QScript::ImageObject::ImageObject() : angle(-1), angleError(10), ratioReferenceMatch(-1.0f)
    {
    }

    int & QScript::ImageObject::Angle()
    {
        return angle;
    }

    int & QScript::ImageObject::AngleError()
    {
        return angleError;
    }

    string & QScript::ImageObject::ImageName()
    {
        return imageName;
    }

    float & QScript::ImageObject::RatioReferenceMatch()
    {
        return ratioReferenceMatch;
    }

    boost::shared_ptr<QScript::ImageObject> QScript::ImageObject::Parse(const string &operand)
    {
        boost::shared_ptr<ImageObject> theObject = boost::shared_ptr<ImageObject>(new ImageObject());
        if (operand.find("|") != string::npos)
        {
            vector<string> components;
            split(components, operand, is_any_of("|"));
            theObject->ImageName() = components[0];
            if (components.size() > 1)
            {
                vector<string> angleAndError;
                split(angleAndError, components[1], is_any_of(","));
                TryParse(angleAndError[0], theObject->Angle());
                if (angleAndError.size() > 1)
                {
                    TryParse(angleAndError[1], theObject->AngleError());
                }
            }
        }
        else
        {
            string uri = operand;
            if (uri.find("://") == string::npos)
            {  
                uri.insert(0, "file:///");
            }
            vector<string> sects;
            ParseURI(sects, uri);
            if (sects.size() == 7)
            {
                if (!sects[4].empty())
                {
                    theObject->ImageName() = sects[4]; 
                }
                if (!sects[5].empty())
                {
                    unordered_map<string,string> map;
                    ParseQuery(map, sects[5]);
                    if (map.find("ratioReferenceMatch") != map.end())
                    {
                        float ratio;
                        if (TryParse(map["ratioReferenceMatch"], ratio))
                        {
                            theObject->RatioReferenceMatch() = ratio;
                        }                            
                    }
                    if (map.find("angle") != map.end())
                    {
                        int angle;
                        if (TryParse(map["angle"], angle))
                        {
                            theObject->Angle() = angle;
                        }                            
                    }
                    if (map.find("angleError") != map.end())
                    {
                        int angleError;
                        if (TryParse(map["angleError"], angleError))
                        {
                            theObject->AngleError() = angleError;
                        }                            
                    }
                }
            }
        }
        return theObject;
    }

    bool QScript::ImageObject::AngleMatch(double angleToMatch)
    {
        return angle < 0 || fabs(angleToMatch - angle) <= static_cast<double>(angleError) ||
            fabs(fabs(angleToMatch - angle) - 360) <= static_cast<double>(angleError);
    }

    string QScript::ImageObject::ToString()
    {
        string ret = imageName;
        bool appendRatio = false;
        if (ratioReferenceMatch > .0f &&
            ratioReferenceMatch <= 1.0f)
        {
            appendRatio = true;
        }
        if (angle >= 0)
        {
            ret += string("|") + boost::lexical_cast<string>(angle);
            if (angleError != 10)
            {
                ret += string(",") + boost::lexical_cast<string>(angleError);
            }
            if (appendRatio)
            {
                ret += "&ratioReferenceMatch=" + boost::lexical_cast<string>(ratioReferenceMatch);
            }
        }
        else if (appendRatio)
        {
            ret += "?ratioReferenceMatch=" + boost::lexical_cast<string>(ratioReferenceMatch);
        }
        return ret;
    }

    QScript::TraceInfo::TraceInfo(int ln, double ts) : lineNumber(ln), timeStamp(ts)
    {
    }

    int & QScript::TraceInfo::LineNumber()
    {
        return lineNumber;
    }

    double & QScript::TraceInfo::TimeStamp()
    {
        return timeStamp;
    }

    int QScript::LineNumberToIp(int lineNumber)
    {
        try
        {
            return lineNumberToIpMap.at(lineNumber);
        }
        catch (...)
        {
            return 0;
        }
    }

    vector<double> & QScript::TimeStamp()
    {
        return timeStampList;
    }

    vector<QScript::TraceInfo> & QScript::Trace()
    {
        return trace;
    }

    int QScript::GetUnusedLineNumber()
    {
        int line = 1;
        while (line > 0)
        {
            bool used = false;
            for (auto ev : eventAndActions)
            {
                if (ev != nullptr && ev->LineNumber() == line)
                {
                    used = true;
                    break;
                }
            }
            if (used)
            {
                line++;
            }
            else
            {
                return line;
            }
        }
        return -1;
    }


    DaVinciStatus QScript::InsertEventAndAction(const boost::shared_ptr<QSEventAndAction> &eventAndAction, int ip)
    {
        if (ip <= 0 || ip > static_cast<int>(eventAndActions.size() + 1))
        {
            return errc::invalid_argument;
        }

        if(eventAndAction->LineNumber() == 0)
        {
            eventAndAction->LineNumber() = GetUnusedLineNumber();
        }
        eventAndActions.insert(eventAndActions.begin() + (ip - 1), eventAndAction);

        RefreshLineNumberToIpMap();
        return DaVinciStatusSuccess;
    }

    void QScript::RemoveEventAndAction(int lineNum)
    {
        for (auto it = eventAndActions.begin(); it != eventAndActions.end();) {
            if ((*it)->LineNumber() == lineNum) {
                it = eventAndActions.erase(it);
                break;
            } else {
                ++it;
            }
        }
    }

    void QScript::AdjustID(std::unordered_map<string, string> keyValueMap, int i)
    {
        vector<string> pairs2;
        vector<string> keys;
        string key;

        key = "layout";
        if(MapContainKey(key, keyValueMap))
        {
            pairs2.push_back(key + "=" + string(keyValueMap[key]));
            keys.push_back(key);
        }

        key = "sb";
        if(MapContainKey(key, keyValueMap))
        {
            pairs2.push_back(key + "=" + string(keyValueMap[key]));
            keys.push_back(key);
        }

        key = "nb";
        if(MapContainKey(key, keyValueMap))
        {
            pairs2.push_back(key + "=" + string(keyValueMap[key]));
            keys.push_back(key);
        }

        key = "kb";
        if(MapContainKey(key, keyValueMap))
        {
            pairs2.push_back(key + "=" + string(keyValueMap[key]));
            keys.push_back(key);
        }

        key = "fw";
        if(MapContainKey(key, keyValueMap))
        {
            pairs2.push_back(key + "=" + string(keyValueMap[key]));
            keys.push_back(key);
        }

        keys.push_back("id");
        keys.push_back("ratio");

        for(auto pair : keyValueMap)
        {
            if(!ContainVectorElement(keys, pair.first))
            {
                pairs2.push_back(pair.first + "=" + string(keyValueMap[pair.first]));
            }
        }

        string trimStr2 = boost::algorithm::join(pairs2,"&");
        eventAndActions[i]->StrOperand(0) = trimStr2;
    }

    void QScript::AdjustTimeStamp(int i)
    {
        eventAndActions[i]->TimeStamp() -= 10;
    }

    vector<boost::shared_ptr<QSEventAndAction>> QScript::GetAllQSEventsAndActions()
    {
        return eventAndActions;
    }

    void QScript::RefreshLineNumberToIpMap()
    {
        int ip = 1;
        for (auto ev : eventAndActions)
        {
            if (ev != nullptr)
            {
                lineNumberToIpMap[ev->LineNumber()] = ip;
            }
            ip++;
        }
    }
}
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

#ifndef __QSCRIPT_HPP__
#define __QSCRIPT_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include <vector>
#include <unordered_map>

#include "DaVinciStatus.hpp"
#include "DeviceControlCommon.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// Define Q script events and actions.
    /// Each event or action contains two timestamps, opcode, four integer operands, and one string operand.
    /// </summary>
    class QSEventAndAction : public boost::enable_shared_from_this<QSEventAndAction>
    {
    public:
        class Spec
        {
        public:
            enum class OperandType
            {
                Unknown = -1,
                Integer = 0,
                String,
                Orientation,
                All // use all the operands as a single string
            };

            Spec(int opc, unsigned int oprNum, unsigned int mandatoryOprNum, ...);

            int Opcode() const;
            unsigned int NumOperands() const;
            unsigned int NumMandatoryOperands() const;
            OperandType operator[](int index) const;
        private:
            int opcode;
            unsigned int numOperands;
            unsigned int numMandatoryOperands;
            vector<OperandType> operandTypes;
        };

        /// <summary>
        /// Mouse event flag used in OPCODE_MOUSE_EVENT
        /// </summary>
        enum class MouseEventFlag
        {
            /// <summary>
            /// mouse move
            /// </summary>
            Move = 0x0001,
            /// <summary>
            /// left button down
            /// </summary>
            LeftDown = 0x0002,
            /// <summary>
            /// left button up
            /// </summary>
            LeftUp = 0x0004,
            /// <summary>
            /// right button down
            /// </summary>
            RightDown = 0x0008,
            /// <summary>
            /// right button up
            /// </summary>
            RightUp = 0x0010,
            /// <summary>
            /// middle button down
            /// </summary>
            MiddleDown = 0x0020,
            /// <summary>
            /// middle button up
            /// </summary>
            MiddleUp = 0x0040,
            /// <summary>
            /// wheel down
            /// </summary>
            Wheel = 0x0800,
        };

        /// <summary>
        /// OPCODE_INVALID: invalid opcode, used for checking opcode
        /// </summary>
        static const int OPCODE_INVALID = 0;
        /// <summary>
        /// OPCODE_CLICK: click on one point (x, y)
        /// </summary>
        static const int OPCODE_CLICK = 1;
        /// <summary>
        /// OPCODE_SWIPE: swipe action with four operands - source (x1, y1) and destiny (x2, y2)
        /// </summary>
        static const int OPCODE_SWIPE = 2;
        /// <summary>
        /// OPCODE_HOME: no operand, just send an HOME action to device
        /// </summary>
        static const int OPCODE_HOME = 3;
        /// <summary>
        /// OPCODE_BACK: no operand, just send an BACK action to device
        /// </summary>
        static const int OPCODE_BACK = 4;
        /// <summary>
        /// OPCODE_MENU: no operand, just send an MENU action to device
        /// </summary>
        static const int OPCODE_MENU = 5;
        /// <summary>
        /// OPCODE_DRAG: drag action with four operands - source position (x1, y1) and destiny (x2, y2)
        /// </summary>
        static const int OPCODE_DRAG = 6;
        /// <summary>
        /// OPCODE_START_APP: start an application with activity name, a string type operand
        /// </summary>
        static const int OPCODE_START_APP = 7;
        /// <summary>
        /// OPCODE_SET_CLICKING_HORIZONTAL: set entering horizontal mode, no operand
        /// </summary>
        static const int OPCODE_SET_CLICKING_HORIZONTAL = 8;
        /// <summary>
        /// OPCODE_TOUCHDOWN: touch down action with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHDOWN = 9;
        /// <summary>
        /// OPCODE_TOUCHDOWN_HORIZONTAL: touch down action in horizontal mode with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHDOWN_HORIZONTAL = 10;
        /// <summary>
        /// OPCODE_TOUCHUP: touch up action with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHUP = 11;
        /// <summary>
        /// OPCODE_TOUCHUP_HORIZONTAL: touch up action in horizontal mode with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHUP_HORIZONTAL = 12;
        /// <summary>
        /// OPCODE_TOUCHMOVE: touch move action with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHMOVE = 13;
        /// <summary>
        /// OPCODE_TOUCHMOVE_HORIZONTAL: touch move action with two operands (x, y)
        /// </summary>
        static const int OPCODE_TOUCHMOVE_HORIZONTAL = 14;
        /// <summary>
        /// OPCODE_OCR: recognize image as text and return the text, using only operand1 as the rule below:
        ///     1 - PARAM_THICK_FONT;
        ///     2 - PARAM_SMALL_FONT;
        ///     4 - PARAM_ROI_UPPER;
        ///     8 - PARAM_ROI_MIDDLE;
        ///     12- PARAM_ROI_LOWER;
        /// </summary>
        static const int OPCODE_OCR = 15;
        /// <summary>
        /// OPCODE_TILTUP: tilt the plate upward for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTUP = 16;
        /// <summary>
        /// OPCODE_TILTDOWN: tilt the plate downward for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTDOWN = 17;
        /// <summary>
        /// OPCODE_TILTLEFT: tilt the plate to the left for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTLEFT = 18;
        /// <summary>
        /// OPCODE_TILTRIGHT: tilt the plate to the right for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTRIGHT = 19;
        /// <summary>
        /// OPCODE_TILTTO: tilt the plate to a specified arm0/arm1 degree as in operand1 and operand2, operand3 (optional) and operand4 (optional) are used to define the speed
        /// </summary>
        static const int OPCODE_TILTTO = 20;
        /// <summary>
        /// OPCODE_PREVIEW: set entering preview mode in DaVinci 
        /// </summary>
        static const int OPCODE_PREVIEW = 21;
        /// <summary>
        /// OPCODE_MATCH_ON: turn on matching in result checker, and then the image in playing video will be checked against the training video
        /// </summary>
        static const int OPCODE_MATCH_ON = 22;
        /// <summary>
        /// OPCODE_MATCH_OFF: turn f matching in result checker, and then the image will not be checked until it is turned on again
        /// </summary>
        static const int OPCODE_MATCH_OFF = 23;
        /// <summary>
        /// OPCODE_IF_MATCH_IMAGE: image_path[|angle,[angle error]]/line1/line2
        /// check if the current frame matches the image provided by image_path with an optional angle in degree.
        /// if yes, go to line1; if not or the image doesn't exist go to line2.
        /// Do not care about angle if it is not specified. If specified, angle error specifies the allowed
        /// error value on matching. It is 10 degree by default.
        /// During runtime, the param line1 and line2 are resolved to ip1 and ip2.
        /// </summary>
        static const int OPCODE_IF_MATCH_IMAGE = 24;
        /// <summary>
        /// OPCODE_MESSAGE: allow the user to output some words on the screen or message box
        /// </summary>
        static const int OPCODE_MESSAGE = 25;
        /// <summary>
        /// OPCODE_MATCH_ERROR: throw an error message and record a user-defined error by increasing ResultChecker.script_errors
        /// </summary>
        static const int OPCODE_ERROR = 26;
        /// <summary>
        /// OPCODE_CLICK_MATCHED_IMAGE: image_path/xoffset/yoffset
        /// click on the image matched in the last OPCODE_IF_MATCH_IMAGE call
        /// with the same image_path. The clicking point is the image center offset
        /// by xoffset and yoffset. The X and Y axis depend on the screen layout.
        /// </summary>
        static const int OPCODE_CLICK_MATCHED_IMAGE = 27;
        /// <summary>
        /// OPCODE_GOTO: line
        /// Unconditional jump.
        /// </summary>
        static const int OPCODE_GOTO = 28;
        /// <summary>
        /// OPCODE_EXIT: exit the script directly.
        /// </summary>
        static const int OPCODE_EXIT = 29;
        /// <summary>
        /// OPCODE_INSTALL_APP: install application
        /// </summary>
        static const int OPCODE_INSTALL_APP = 30;
        /// <summary>
        /// OPCODE_UNINSTALL_APP: uninstall application
        /// </summary>
        static const int OPCODE_UNINSTALL_APP = 31;
        /// <summary>
        /// OPCODE_PUSH_DATA: push data for application
        /// </summary>
        static const int OPCODE_PUSH_DATA = 32;
        /// <summary>
        /// OPCODE_CALL_SCRIPT: call script for handle unexpected cases
        /// </summary>
        static const int OPCODE_CALL_SCRIPT = 33;
        /// <summary>
        /// OPCODE_MULTI_TOUCHDOWN: touch down action with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHDOWN = 34;
        /// <summary>
        /// OPCODE_TOUCHDOWN_HORIZONTAL: touch down action in horizontal mode with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHDOWN_HORIZONTAL = 35;
        /// <summary>
        /// OPCODE_MULTI_TOUCHUP: touch up action with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHUP = 36;
        /// <summary>
        /// OPCODE_MULTI_TOUCHUP_HORIZONTAL: touch up action in horizontal mode with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHUP_HORIZONTAL = 37;
        /// <summary>
        /// OPCODE_MULTI_TOUCHMOVE: touch move action with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHMOVE = 38;
        /// <summary>
        /// OPCODE_MULTI_TOUCHMOVE_HORIZONTAL: touch move action with three operands (x, y, ptr)
        /// </summary>
        static const int OPCODE_MULTI_TOUCHMOVE_HORIZONTAL = 39;
        /// <summary>
        /// OPCODE_NEXT_SCRIPT: finish the current script and, call the next script
        /// </summary>
        static const int OPCODE_NEXT_SCRIPT = 40;
        /// <summary>
        /// OPCODE_CALL_EXTERNAL_SCRIPT: call external script
        /// </summary>
        static const int OPCODE_CALL_EXTERNAL_SCRIPT = 43;
        /// <summary>
        /// OPCODE_ELAPSED_TIME: print the elapsed time in milliseconds
        /// </summary>
        static const int OPCODE_ELAPSED_TIME = 44;
        /// <summary>
        /// OPCODE_FPS_MEASURE_START: [start.png]
        /// start FPS measurement if start.png is matched, or the timestamp >= start_timeStamp. 
        /// operand string: file name of image. If the image is matched, DaVinci start to measure FPS.
        /// start_timeStamp for the action/instruction is the timestamp of starting FPS measurement.
        /// </summary>
        static const int OPCODE_FPS_MEASURE_START = 45;
        /// <summary>
        /// OPCODE_FPS_MEASURE_STOP: [stop.png]
        /// stop FPS measurement if the stop.png is matched, or the timestamp >= stop_timeStamp. 
        /// operand string: file name of image. If the image is matched, DaVinci stop to measure FPS.
        /// stop_timeStamp for the action/instruction is the timestamp of stopping FPS measurement.
        /// </summary>
        static const int OPCODE_FPS_MEASURE_STOP = 46;
        /// <summary>
        /// OPCODE_CONCURRENT_IF_MATCH: image_file_name[|angle,[angle error]] instr_count [match_once]
        /// used for handling dynamic scenarios, like random Ads. It compare every
        /// frames and do specified actions if matching the image with an optional angle. The actions start 
        /// from next instruction and its block length is specified by instr_count.
        /// operand string: image file name to be compared
        /// operand 1: instruction count for handling dynamic scenarios
        /// </summary>
        static const int OPCODE_CONCURRENT_IF_MATCH = 47;
        /// <summary>
        /// OPCODE_IF_MATCH_TXT: string ocr_param line1 line2
        /// check if the current frame has the string text (matched on word boundary)
        /// if yes, go to line1; if not or the text doesn't exist go to line2.
        /// During runtime, the param line1 and line2 are resolved to ip1 and ip2.
        /// </summary>
        static const int OPCODE_IF_MATCH_TXT = 48;
        /// <summary>
        /// OPCODE_CLICK_MATCHED_TXT: string num_matched x_offset y_offset
        /// click on the "num_matched"th matched text in the last OPCODE_IF_MATCH_TXT call
        /// with the same text. "num_matched" is zero-based
        /// </summary>
        static const int OPCODE_CLICK_MATCHED_TXT = 49;
        /// <summary>
        /// OPCODE_BATCH_PROCESSING: batch process for smoke test and Q scripts
        /// </summary>
        static const int OPCODE_BATCH_PROCESSING = 50;
        /// <summary>
        /// OPCODE_IF_MATCH_REGEX: pattern ocr_param line1 line2
        /// Similar to OPCODE_IF_MATCH_TXT but matching a regular expression pattern instead of a string.
        /// </summary>
        static const int OPCODE_IF_MATCH_REGEX = 51;
        /// <summary>
        /// OPCODE_CLICK_MATCHED_REGEX: pattern num_matched x_offset y_offset
        /// Similar to OPCODE_CLICK_MATCHED_REG but click the previously matching regular expression
        /// with OPCODE_IF_MATCH_REGEX.
        /// </summary>
        static const int OPCODE_CLICK_MATCHED_REGEX = 52;
        /// <summary>
        /// OPCODE_AUDIO_RECORD_START: audio_file
        /// Start record audio into the specified audio file
        /// </summary>
        static const int OPCODE_AUDIO_RECORD_START = 53;
        /// <summary>
        /// OPCODE_AUDIO_RECORD_STOP: audio_file
        /// Stop record audio and close the specified audio file
        /// </summary>
        static const int OPCODE_AUDIO_RECORD_STOP = 54;
        /// <summary>
        /// OPCODE_IF_MATCH_AUDIO: audio_file1 audio_file2 line1 line2
        /// Check if audio_file1 matched audio_file2, jumps to line1, otherwise jumps to line2.
        /// </summary>
        static const int OPCODE_IF_MATCH_AUDIO = 55;
        /// <summary>
        /// OPCODE_AI_BUILTIN: builtin AI strategy
        /// Support AI-needed games
        /// </summary>
        static const int OPCODE_AI_BUILTIN = 56;
        /// <summary>
        /// OPCODE_IMAGE_MATCHED_TIME: image_file_name
        /// Report the elapsed time of the latest matching of the provided image file.
        /// </summary>
        static const int OPCODE_IMAGE_MATCHED_TIME = 57;
        /// <summary>
        /// OPCODE_FLICK_ON
        /// Change the flick detection state to on.
        /// </summary>
        static const int OPCODE_FLICK_ON = 58;
        /// <summary>
        /// OPCODE_FLICK_OFF: image_file_name
        /// Change the flick detection state to off.
        /// </summary>
        static const int OPCODE_FLICK_OFF = 59;
        /// <summary>
        /// OPCODE_AUDIO_PLAY: audio_file
        /// Play the specified audio file
        /// </summary>
        static const int OPCODE_AUDIO_PLAY = 60;
        /// <summary>
        /// OPCODE_IF_MATCH_IMAGE_WAIT: image_file_name[|angle,[angle error]] label1 label2 timeout
        /// When matching the specified image with an optional angle, it jumps to label1. When timeout is reached, it jumps to label2.
        /// The timeout is in milli-second.
        /// </summary>
        static const int OPCODE_IF_MATCH_IMAGE_WAIT = 61;
        /// <summary>
        /// OPCODE_CLICK_MATCHED_IMAGE_XY: image_file_name x_offset y_offset X Y
        /// If there is a matched image, then click it like OPCODE_CLICK_MATCHED_IMAGE.
        /// If there is none, then click on that coordinate (X, Y).
        /// </summary>
        static const int OPCODE_CLICK_MATCHED_IMAGE_XY = 62;
        /// <summary>
        /// OPCODE_CLEAR_DATA: clear the app data
        /// </summary>
        static const int OPCODE_CLEAR_DATA = 63;
        /// <summary>
        /// OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY: image_file_name x_offset y_offset X Y
        /// If there is a matched image, then touch down it like OPCODE_CLICK_MATCHED_IMAGE.
        /// If there is none, then click on that coordinate (X, Y).
        /// </summary>
        static const int OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY = 64;
        /// <summary>
        /// OPCODE_TOUCHUP_MATCHED_IMAGE_XY: image_file_name x_offset y_offset X Y
        /// If there is a matched image, then touch up it like OPCODE_CLICK_MATCHED_IMAGE.
        /// If there is none, then click on that coordinate (X, Y).
        /// </summary>
        static const int OPCODE_TOUCHUP_MATCHED_IMAGE_XY = 65;
        /// <summary>
        /// OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY: image_file_name x_offset y_offset X Y
        /// If there is a matched image, then touch move it like OPCODE_CLICK_MATCHED_IMAGE.
        /// If there is none, then click on that coordinate (X, Y).
        /// </summary>
        static const int OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY = 66;
        /// <summary>
        /// OPCODE_MOUSE_EVENT:event_type X Y delta
        /// <param name="mouse_evt">The mouse event</param>
        /// <param name="x">Position in x-axis</param>
        /// <param name="y">Position in y-axis</param>
        /// <param name="data">The delta value for mouse</param>
        /// </summary>
        static const int OPCODE_MOUSE_EVENT = 67;
        /// <summary>
        /// OPCODE_KEYBOARD_EVENT:event_type keycode
        /// </summary>
        static const int OPCODE_KEYBOARD_EVENT = 68;
        /// <summary>
        /// OPCODE_IF_SOURCE_IMAGE_MATCH: source_image_file target_image_file yes_ip no_ip
        /// </summary>
        static const int OPCODE_IF_SOURCE_IMAGE_MATCH = 69;
        /// <summary>
        /// OPCODE_IF_SOURCE_LAYOUT_MATCH: source_layout_file target_layout_file yes_ip no_ip
        /// </summary>
        static const int OPCODE_IF_SOURCE_LAYOUT_MATCH = 70;
        /// <summary>
        /// OPCODE_CRASH_ON: enable app crash detection via matching template crash dialog
        /// </summary>
        static const int OPCODE_CRASH_ON = 71;
        /// <summary>
        /// OPCODE_CRASH_OFF:disable app crash detection (default)
        /// </summary>
        static const int OPCODE_CRASH_OFF = 72;
        /// <summary>
        /// OPCODE_SET_VARIABLE: variable_name value
        /// This opcode is used to declare a variable and set its initial value.
        /// The "value" operand could be a constant or an expression.
        /// </summary>
        static const int OPCODE_SET_VARIABLE = 73;
        /// <summary>
        /// OPCODE_LOOP: loop_count target_label
        /// The "loop_count" is a variable defined earlier by OPCODE_SET_VARIABLE.
        /// If it is larger than zero, its value is decremented and the script jumps
        /// to the "target_label". Otherwise, it jumps to the opcode following OPCODE_LOOP.
        /// </summary>
        static const int OPCODE_LOOP = 74;
        /// <summary>
        /// OPCODE_STOP_APP
        /// Stop the application via adb command
        /// </summary>
        static const int OPCODE_STOP_APP = 75;
        /// <summary>
        /// OPCODE_SET_TEXT: text
        /// Set the text to the application field via agent
        /// </summary>
        static const int OPCODE_SET_TEXT = 76;
        /// <summary>
        /// OPCODE_EXPLORATORY_TEST: bias_file
        /// Run exploratory test within Q script
        /// </summary>
        static const int OPCODE_EXPLORATORY_TEST = 77;
        /// <summary>
        /// OPCODE_POWER
        /// Press once to power off the device into sleep mode; click twice to bring it up
        /// </summary>
        static const int OPCODE_POWER = 78;
        /// <summary>
        /// OPCODE_LIGHT_UP
        /// Light the screen up
        /// </summary>
        static const int OPCODE_LIGHT_UP = 79;
        /// <summary>
        /// OPCODE_VOLUME_UP
        /// Volume up the sound
        /// </summary>
        static const int OPCODE_VOLUME_UP = 80;
        /// <summary>
        /// OPCODE_VOLUME_DOWN
        /// Volume down the sound
        /// </summary>
        static const int OPCODE_VOLUME_DOWN = 81;
        /// <summary>
        /// Operate devices power button.
        /// Press button:   OPCODE_DEVICE_POWER 0
        /// Release button: OPCODE_DEVICE_POWER 1
        /// Short press button: OPCODE_DEVICE_POWER 2 time_duration
        /// <param name="time_duration">Indicated the time duration (milliseconds) of power pusher short press action.</param>
        /// </summary>
        static const int OPCODE_DEVICE_POWER = 82;
        /// <summary>
        /// OPCODE_AUDIO_MATCH_ON
        /// Turn on audio matching
        /// </summary>
        static const int OPCODE_AUDIO_MATCH_ON = 83;
        /// <summary>
        /// OPCODE_AUDIO_MATCH_OFF
        /// Turn off audio matching
        /// </summary>
        static const int OPCODE_AUDIO_MATCH_OFF = 84;
        /// <summary>
        /// OPCODE_HOLDER_UP: lift the holder upward for a defined distance as in operand1 (operand2 is written while not used)
        /// </summary>
        static const int OPCODE_HOLDER_UP = 85;
        /// <summary>
        /// OPCODE_HOLDER_DOWN: lift the holder downward for a defined distance as in operand1 (operand2 is written while not used)
        /// </summary>
        static const int OPCODE_HOLDER_DOWN = 86;
        /// <summary>
        /// OPCODE_BUTTON operand1: press button
        /// operand1: button name
        /// </summary>
        static const int OPCODE_BUTTON = 87;
        /// <summary>
        /// OPCODE_USB_IN: plug in the USB port defined as in operand1 (operand2 is written while not used)
        /// Plug in USB port 1:         OPCODE_USB_IN 1
        /// Plug in USB port 2:         OPCODE_USB_IN 2
        /// </summary>
        static const int OPCODE_USB_IN = 88;
        /// <summary>
        /// OPCODE_USB_OUT: plug out the USB port defined as in operand1 (operand2 is written while not used)
        /// Plug out USB port 1:        OPCODE_USB_OUT 1
        /// Plug out USB port 2:        OPCODE_USB_OUT 2
        /// </summary>
        static const int OPCODE_USB_OUT = 89;
        /// <summary>
        /// OPCODE_RANDOM_ACTION ACTION_TYPE|PROBABILITY
        /// ACTION_TYPE: CLICK, SWIPE, MOVE, HOME, BACK, MENU, POWER, LIGHT UP, and VOLUMN DOWN/UP
        /// Specify one main ACTION_TYPE with probability and other ACTION_TYPE with left probability 
        /// </summary>
        static const int OPCODE_RANDOM_ACTION = 90;
        /// <summary>
        /// Operation for brightness adjustment
        /// Brightness up:   OPCODE_BRIGHTNESS 0
        /// Brightness down: OPCODE_BRIGHTNESS 1
        /// </summary>
        static const int OPCODE_BRIGHTNESS = 91;
        /// <summary>
        /// Operation for reference image check
        /// OPCODE_IMAGE_CHECK: reference_image_path?ratioReferenceMatch=<value> <label> <timeout>
        // Check if reference image matches current frame until matched or timeout, if not matched go to label
        /// </summary>
        static const int OPCODE_IMAGE_CHECK = 92;
        /// <summary>
        /// OPCODE_TILTCLOCKWISE: tilt the plate clockwise around Z axis for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTCLOCKWISE = 93;
        /// <summary>
        /// OPCODE_TILTANTICLOCKWISE: tilt the plate anticlockwise around Z axis for a defined degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTANTICLOCKWISE = 94;
        /// <summary>
        /// OPCODE_DUMP_UI_LAYOUT: dump uiautomator data
        /// </summary>
        static const int OPCODE_DUMP_UI_LAYOUT = 95;
        /// <summary>
        /// OPCODE_EARPHONE_IN: plug in the Earphone Puller defined as in operand1 (operand2 is written while not used)
        /// Plug in Earphone Puller 1:         OPCODE_EARPHONE_IN 1 (only have one Earphone Puller for each FFRD now)
        /// </summary>
        static const int OPCODE_EARPHONE_IN = 96;
        /// <summary>
        /// OPCODE_EARPHONE_OUT: pull out the Earphone Puller defined as in operand1 (operand2 is written while not used)
        /// Pull out Earphone Puller 1:        OPCODE_EARPHONE_OUT 1 (only have one Earphone Puller for each FFRD now)
        /// </summary>
        static const int OPCODE_EARPHONE_OUT = 97;        
        /// <summary>
        /// OPCODE_RELAY_CONNECT: connect the wire connection with relay, port number is defined as in operand1 (operand2 is written while not used)
        /// Connect relay port 1:       OPCODE_RELAY_CONNECT 1  (Default settings)
        /// Connect relay port 2:       OPCODE_RELAY_CONNECT 2
        /// Connect relay port 3:       OPCODE_RELAY_CONNECT 3
        /// Connect relay port 4:       OPCODE_RELAY_CONNECT 4
        /// </summary>
        static const int OPCODE_RELAY_CONNECT = 98;
        /// <summary>
        /// OPCODE_RELAY_DISCONNECT: disconnect the wire connection with relay, port number is defined as in operand1 (operand2 is written while not used)
        /// Disconnect relay port 1:       OPCODE_RELAY_DISCONNECT 1  (Default settings)
        /// Disconnect relay port 2:       OPCODE_RELAY_DISCONNECT 2
        /// Disconnect relay port 3:       OPCODE_RELAY_DISCONNECT 3
        /// Disconnect relay port 4:       OPCODE_RELAY_DISCONNECT 4
        /// </summary>
        static const int OPCODE_RELAY_DISCONNECT = 99;
        /// <summary>
        /// OPCODE_POWER_MEASURE_START: Mark beginning of Power Test and start doing Power Test
        /// </summary>
        static const int OPCODE_POWER_MEASURE_START = 100;
        /// <summary>
        /// OPCODE_POWER_MEASURE_STOP: Mark end of Power Test and stop doing Power Test
        /// </summary>
        static const int OPCODE_POWER_MEASURE_STOP = 101;        
        /// <summary>
        /// OPCODE_TILTZAXISTO: tilt the plate to a specified Z axis degree as in operand1, operand2 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTZAXISTO = 102;
        /// <summary>
        /// OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE: countinuous-tilt the inner plate clockwise, operand1 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTCONTINUOUS_INNER_CLOCKWISE = 103;
        /// <summary>
        /// OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE: countinuous-tilt the outer plate clockwise, operand1 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTCONTINUOUS_OUTER_CLOCKWISE = 104;
        /// <summary>
        /// OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE: countinuous-tilt the inner plate anticlockwise, operand1 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTCONTINUOUS_INNER_ANTICLOCKWISE = 105;
        /// <summary>
        /// OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE: countinuous-tilt the outer plate anticlockwise, operand1 (optional) is used to define the speed
        /// </summary>
        static const int OPCODE_TILTCONTINUOUS_OUTER_ANTICLOCKWISE = 106;
        /// <summary>
        /// OPCODE_SENSOR_CHECK: check the sensor result from agent and FFRD
        /// </summary>
        static const int OPCODE_SENSOR_CHECK = 107;



        /// <summary> Constructor. </summary>
        ///
        /// <param name="ts">    The ts. </param>
        /// <param name="opc">   The opc. </param>
        /// <param name="opStr"> (Optional) the operation string. </param>
        /// <param name="o">     (Optional) the Orientation to process. </param>
        QSEventAndAction(double ts, int opc, const string &opStr = "", Orientation o = Orientation::Unknown);

        /// <summary> Constructor. </summary>
        ///
        /// <param name="ts">    The ts. </param>
        /// <param name="opc">   The opc. </param>
        /// <param name="opr1">  The first operator. </param>
        /// <param name="opr2">  The second operator. </param>
        /// <param name="opr3">  The third operator. </param>
        /// <param name="opr4">  The fourth operator. </param>
        /// <param name="opStr"> (Optional) the operation string. </param>
        /// <param name="o">     (Optional) the Orientation to process. </param>
        QSEventAndAction(double ts, int opc, int opr1, int opr2, int opr3, int opr4, const string &opStr = "", Orientation o = Orientation::Unknown);

        /// <summary> Constructor. </summary>
        ///
        /// <param name="ts">      The ts. </param>
        /// <param name="opc">     The opc. </param>
        /// <param name="intOprs"> The int oprs. </param>
        /// <param name="strOprs"> The oprs. </param>
        /// <param name="o">       (Optional) the Orientation to process. </param>
        QSEventAndAction(double ts, int opc, const vector<int> &intOprs, const vector<string> &strOprs, Orientation o = Orientation::Unknown);

        int & LineNumber();

        unsigned int NumIntOperands();

        int & IntOperand(int index);

        void SetIntOperand(int index, int val);

        unsigned int NumStrOperands();

        string & StrOperand(int index);

        double & TimeStamp();

        void SetTimeStamp(double timeStamp);

        double & TimeStampReplay();

        int & Opcode();

        Orientation & GetOrientation();

        vector<string> & GetStrOperands();

        string ToString();        

    private:
        /// <summary>
        /// Line number of this event. The line number does not need to be
        /// consecutive. It just serves as a label for jump target. It should be
        /// positive, otherwise the line number has not been assigned yet.
        /// </summary>
        int lineNumber;

        /// <summary>
        /// timestamp contains the timing of each event/action when it is being recorded.
        /// </summary>
        double timeStamp;

        /// <summary> The timestamp when the event/action is replayed </summary>
        double timeStampReplay;

        /// <summary>
        /// opcode defines what action should be done with each Q event/action.
        /// </summary>
        int opcode;

        /// <summary> The operands of integer type. </summary>
        vector<int> intOperands;

        /// <summary> The operands of string type. </summary>
        vector<string> strOperands;

        /// <summary>
        /// The device orientation corresponding to the event and action.
        /// </summary>
        Orientation orientation;

        int defaultIntOperand;
        string defaultStrOperand;

        string raw;

        void Init(double ts, int opc, const vector<int> &intOprs, const vector<string> &strOprs, Orientation o = Orientation::Unknown);
    };

    class QScript : public boost::enable_shared_from_this<QScript>
    {
        friend class QSEventAndAction;

    public:
        class ImageObject
        {
        public:
            /// <summary>
            /// The path to the image
            /// </summary>
            string & ImageName();
            /// <summary>
            /// Valid value ranges from 0 to 359. Ignored if not valid.
            /// If valid, the angle is relative to the landscape frame clockwise.
            /// </summary>
            int & Angle();
            /// <summary>
            /// If Angle is valid, it specifies the error allowed to match
            /// the angle.
            /// </summary>
            int & AngleError();
            /// <summary>
            /// Valid value ranges from 0 to 1.0. Ignored if not valid.
            /// If valid, the ratio means percentage matched feature points between reference and observation.
            /// </summary>
            float & RatioReferenceMatch();

            ImageObject();

            /// <summary>
            /// Parse the operand string provided to IF_MATCH opcode to get
            /// image object components.
            /// </summary>
            /// <param name="operand">A string separated by "|"</param>
            /// <returns></returns>
            static boost::shared_ptr<ImageObject> Parse(const string &operand);

            /// <summary>
            /// Check whether the provided angleToMatch matches this image object with angle.
            /// Always matches if the current Angle is not valid.
            /// </summary>
            /// <param name="angleToMatch"></param>
            /// <returns></returns>
            bool AngleMatch(double angleToMatch);

            /// <summary>
            /// Get the required string format in IF_MATCH opcodes
            /// </summary>
            /// <returns></returns>
            string ToString();

        private:
            string imageName;
            int angle;
            int angleError;
            float ratioReferenceMatch;
        };

        class TraceInfo
        {
        public:
            TraceInfo(int ln, double ts);
            int & LineNumber();
            double & TimeStamp();            
        private:
            int lineNumber;
            double timeStamp;            
        };

        static const string ConfigCameraPresetting;
        static const string ConfigPackageName;
        static const string ConfigActivityName;
        static const string ConfigApkName;
        static const string ConfigIconName;
        static const string ConfigPushData;
        static const string ConfigStatusBarHeight;
        static const string ConfigNavigationBarHeight;
        static const string ConfigStartOrientation;
        static const string ConfigCameraType;
        static const string ConfigVideoRecording;
        static const string ConfigRecordedVideoType;
        static const string ConfigReplayVideoType;
        static const string ConfigMultiLayerMode;
        static const string ConfigVideoTypeH264;
        static const string ConfigOSversion;
        static const string ConfigDPI;
        static const string ConfigResolution;
        static const string ConfigResolutionWidth;
        static const string ConfigResolutionHeight;

        static DaVinciStatus InitSpec();
        static boost::shared_ptr<QScript> Load(const string &filename);
        static boost::shared_ptr<QScript> New(const string &filename);

        static DaVinciStatus ParseTimeStampFile(const string &qts, vector<double> & timeStamps, vector<QScript::TraceInfo> & traces);

        virtual ~QScript();

        DaVinciStatus Flush();

        string GetQsFileName();

        string GetQtsFileName();

        string GetVideoFileName();

        string GetAudioFileName();

        string GetSensorCheckFileName();

        string GetScriptName();

        string GetResourceFullPath(const string &resPath);

        string GetConfiguration(const string &key);

        DaVinciStatus SetConfiguration(const string &key, const string &value);

        unsigned int NumEventAndActions();

        boost::shared_ptr<QSEventAndAction> EventAndAction(int ip);

        void SetEventOrActionTimeStampReplay(int ip, double timeStamp);

        DaVinciStatus AppendEventAndAction(const boost::shared_ptr<QSEventAndAction> &eventAndAction);

        /// <summary> Inserts an event at the specified IP position. </summary>
        ///
        /// <param name="eventAndAction"> The event and action. </param>
        /// <param name="ip">             The IP where the event is inserted. </param>
        ///
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus InsertEventAndAction(const boost::shared_ptr<QSEventAndAction> &eventAndAction, int ip);

        void RemoveEventAndAction(int lineNum);

        vector<boost::shared_ptr<QSEventAndAction>> GetAllQSEventsAndActions();

        bool UsedHyperSoftCam();

        bool IsStartHorizontal();

        string GetPackageName();

        string GetActivityName();

        string GetApkName();

        string GetIconName();

        string GetOSversion();

        string GetDPI();

        string GetResolutionWidth();

        string GetResolutionHeight();

        vector<int> GetStatusBarHeight();

        vector<int> GetNavigationBarHeight();

        bool NeedVideoRecording();

        bool IsMultiLayerMode();

        int LineNumberToIp(int lineNumber);

        vector<double> & TimeStamp();

        vector<TraceInfo> & Trace();

        int GetUnusedLineNumber();

        string ProcessQuoteSpaces(const string &str);

        static void SplitEventsAndActionsLine(const string &line, vector<string> &columns);

        /// <summary>
        /// Opcode to string
        /// </summary>
        /// <returns>string</returns>
        static string OpcodeToString(const int opcode);

        /// <summary>
        /// Opcode for crash
        /// </summary>
        /// <returns>true for crash</returns>
        static bool IsOpcodeCrash(int opcode);

        /// <summary>
        /// Opcode for virtual navigation
        /// </summary>
        /// <returns>true for crash</returns>
        static bool IsOpcodeVirtualNavigation(int opcode);

        /// <summary>
        /// Opcode for non touch opcodes
        /// </summary>
        /// <returns>true for non-touch</returns>
        static bool IsOpcodeNonTouch(int opcode);

        /// <summary>
        /// Opcode for non touch opcodes before app start
        /// </summary>
        /// <returns>true for non-touch opcode before app start</returns>
        static bool IsOpcodeNonTouchBeforeAppStart(int opcode);

        /// <summary>
        /// Opcode for non touch opcodes show frame in report
        /// </summary>
        /// <returns>true for non-touch</returns>
        static bool IsOpcodeNonTouchShowFrame(int opcode);

        /// <summary>
        /// Opcode for tilt opcodes
        /// </summary>
        /// <returns>true for tilt</returns>
        static bool IsOpcodeForTilt(int opcode);

        /// <summary>
        /// Opcode for non touch opcode check process
        /// </summary>
        /// <returns>true for non-touch</returns>
        static bool IsOpcodeNonTouchCheckProcess(int opcode);

        /// <summary>
        /// Opcode for condition opcodes
        /// </summary>
        /// <returns>true for condition opcde</returns>
        static bool IsOpcodeIfCondition(int opcode);

        /// <summary>
        /// Opcode for touch opcodes
        /// </summary>
        /// <returns>true for touch opcde</returns>
        static bool IsOpcodeTouch(int opcode);

        /// <summary>
        /// Opcode for in-touch opcodes
        /// </summary>
        /// <returns>true for touchmove opcde</returns>
        static bool IsOpcodeInTouch(int opcode);

        void AdjustID(std::unordered_map<string, string> keyValueMap, int i);

        void AdjustTimeStamp(int i);

        // TODO: add more operations on actions if needed
    private:
        static std::unordered_map<string, boost::shared_ptr<QSEventAndAction::Spec>> qscriptSpec;
        static std::unordered_map<int, string> opcodeNameTable;
        static DaVinciStatus ParseConfigLine(const boost::shared_ptr<QScript> &qs, const string &line, unsigned int lineNumber);
        static DaVinciStatus ParseEventsAndActionsLine(const boost::shared_ptr<QScript> &qs, const string &line, unsigned int lineNumber);

        string scriptName;
        string qsFileName;
        string qtsFileName;
        string videoFileName;
        string audioFileName;
        string sensorCheckFileName;

        std::unordered_map<string, string> configurationEntries;
        vector<boost::shared_ptr<QSEventAndAction>> eventAndActions;
        vector<double> timeStampList;
        vector<TraceInfo> trace;
        std::unordered_map<int, int> lineNumberToIpMap;

        QScript(const string &filename);

        bool IsTouchPaired();

        void RefreshLineNumberToIpMap();
    };
}

#endif
'''
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
'''

# -*- coding: utf-8 -* 
# Imports the monkeyrunner modules used by this program 
#
from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice  
from com.android.monkeyrunner.easy import EasyMonkeyDevice
from com.android.monkeyrunner.easy import By
import socket
import struct
import os
import sys
import types
import time
import thread

# Connects to the  device, returning a MonkeyDevice object
AGENT_PORT=31500
DEAMON_PORT=9999

ADB = "platform-tools\\adb "
if len(sys.argv) < 2:
        print "No device model provided"
        device_model = ""
else:
        device_model = sys.argv[1]
if len(sys.argv) < 3:
        print "No device name specified"
        device_name = "null"
        try:
                device = MonkeyRunner.waitForConnection(5.0)
        except:
                print "Agent Failed: DaVinciRunner waitForConnection error."
                sys.exit(-1)
else:
        device_name = sys.argv[2]
        print "connecting to device " + device_name
        if "." in device_name:
                os.system(ADB + " connect " + device_name)            
        try:
                device = MonkeyRunner.waitForConnection(5.0, device_name)
        except:
                print "Agent Failed: DaVinciRunner waitForConnection error."
                sys.exit(-1)
        if len(sys.argv) > 3:
                AGENT_PORT = int(sys.argv[3])

# use this to solve the problem that the first socket operation may break the socket.
try:
        print "Test DaVinciRunner Connection"
        device.wake()
except:
        print "First Error! Reconnecting..."
        if len(sys.argv) < 3:
                device = MonkeyRunner.waitForConnection(5.0)
        else:
                device = MonkeyRunner.waitForConnection(5.0, device_name)
        print "Reconnected."
        try:
                device.wake()
        except:
                print "Reconnection fails."
                sys.exit(-1)

try:
        device.wake()
except:
        print "Second Error! Reconnecting..."
        if len(sys.argv) < 3:
                device = MonkeyRunner.waitForConnection(5.0)
        else:
                device = MonkeyRunner.waitForConnection(5.0, device_name)
        print "Reconnected."
        try:
                device.wake()
        except:
                print "Reconnection fails."
                sys.exit(-1)

width = 0
height = 0

if device_model != "" and device_model != "null":
        print "Connecting to agent deamon..." 
        deamon=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        deamon.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY,1)
        try:
                deamon_connection=deamon.connect_ex(("127.0.0.1",DEAMON_PORT))
                if deamon_connection == 0:
                        print "Agent deamon connected."
                        is_deamon_connected=True
                else:
                        print "Agent deamon connection failed. Multi-touch unsupported."
                        is_deamon_connected=False
        except:
                print "Agent deamon connection failed. Multi-touch unsupported."
                is_deamon_connected=False
else:
        print "Device model not provided. Multi-touch unsupported."
        is_deamon_connected=False

agent=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
agent.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

agent.bind(("127.0.0.1", AGENT_PORT))

agent.listen(1)
print "Listen at port " + str(AGENT_PORT)

Q_ACTION_MASK = 0xf0;
Q_TOUCH_DOWN = 0x17;
Q_TOUCH_HDOWN = 0x18;
Q_TOUCH_UP = 0x19;
Q_TOUCH_HUP = 0x1a;
Q_TOUCH_MOVE = 0x1b;
Q_TOUCH_HMOVE = 0x1c;

Q_MULTI_TOUCH = 0x20;
Q_MULTI_DOWN = 0x21;
Q_MULTI_HDOWN = 0x22;
Q_MULTI_UP = 0x23;
Q_MULTI_HUP = 0x24;
Q_MULTI_MOVE = 0x25;
Q_MULTI_HMOVE = 0x26;

Q_SWIPE_START = 0x30;
Q_SWIPE_END = 0x3f;
Q_SWIPE_HORIZONTAL_START = 0x35;
Q_SWIPE_HORIZONTAL_END = 0x36;

Q_WAKE = 0x40;
Q_SHELL_INPUT_TEXT = 0x41;
Q_SHELL_INPUT_TAP = 0x42;
Q_SHELL_INPUT_HTAP = 0x43;
Q_SHELL_SWIPE_START = 0x44;
Q_SHELL_SWIPE_END = 0x45;
Q_SHELL_SWIPE_HORIZONTAL_START = 0x46;
Q_SHELL_SWIPE_HORIZONTAL_END = 0x47;

Q_PRESS_KEY = 0x50;
Q_KEYEVENT_DOWN = 0x00;
Q_KEYEVENT_UP = 0x01;
Q_KEYEVENT_DOWN_AND_UP = 0x02;
Q_KEYEVENT_NOTSUPPORT = 0x03;

Q_KEYCODE_NOTSUPPORT = 0x00;
Q_KEYCODE_HOME = 0x01;
Q_KEYCODE_BACK = 0x02;
Q_KEYCODE_MENU = 0x03;
Q_KEYCODE_SEARCH = 0x04;
Q_KEYCODE_DPAD_CENTER = 0x05;
Q_KEYCODE_DPAD_UP = 0x06;
Q_KEYCODE_DPAD_DOWN = 0x07;
Q_KEYCODE_DPAD_LEFT = 0x08;
Q_KEYCODE_DPAD_RIGHT = 0x09;
Q_KEYCODE_ENTER = 0x11;
Q_KEYCODE_DEL = 0x12; 
Q_KEYCODE_POWER = 0x13;
Q_KEYCODE_VOLUME_DOWN = 0x14;
Q_KEYCODE_VOLUME_UP = 0x15;
Q_KEYCODE_WHITESPACE = 0x16;
Q_KEYCODE_APP_SWITCH = 0x17;
Q_KEYCODE_BRIGHTNESS_DOWN = 0x18;
Q_KEYCODE_BRIGHTNESS_UP = 0x19;
Q_KEYCODE_MEDIA_PLAY_PAUSE = 0x20;

Q_KEYCODE_A = 0x21;
Q_KEYCODE_B = 0x22;
Q_KEYCODE_C = 0x23;
Q_KEYCODE_D = 0x24;
Q_KEYCODE_E = 0x25;
Q_KEYCODE_F = 0x26;
Q_KEYCODE_G = 0x27;
Q_KEYCODE_H = 0x28;
Q_KEYCODE_I = 0x29;
Q_KEYCODE_J = 0x2a;
Q_KEYCODE_K = 0x2b;
Q_KEYCODE_L = 0x2c;
Q_KEYCODE_M = 0x2d;
Q_KEYCODE_N = 0x2e;
Q_KEYCODE_O = 0x2f;
Q_KEYCODE_P = 0x30;
Q_KEYCODE_Q = 0x31;
Q_KEYCODE_R = 0x32;
Q_KEYCODE_S = 0x33;
Q_KEYCODE_T = 0x34;
Q_KEYCODE_U = 0x35;
Q_KEYCODE_V = 0x36;
Q_KEYCODE_W = 0x37;
Q_KEYCODE_X = 0x38;
Q_KEYCODE_Y = 0x39;
Q_KEYCODE_Z = 0x3a;

Q_KEYCODE_0 = 0x51;
Q_KEYCODE_1 = 0x52;
Q_KEYCODE_2 = 0x53;
Q_KEYCODE_3 = 0x54;
Q_KEYCODE_4 = 0x55;
Q_KEYCODE_5 = 0x56;
Q_KEYCODE_6 = 0x57;
Q_KEYCODE_7 = 0x58;
Q_KEYCODE_8 = 0x59;
Q_KEYCODE_9 = 0x5a;

Q_KEYCODE_SPACE = 0x62;
Q_KEYCODE_GRAVE  = 0x64;
Q_KEYCODE_AT = 0x66;
Q_KEYCODE_POUND = 0x67;
Q_KEYCODE_START = 0x72;
Q_KEYCODE_NUMPAD_LEFT_PAREN = 0x73;
Q_KEYCODE_NUMPAD_RIGHT_PAREN = 0x74;
Q_KEYCODE_NUMPAD_SUBTRACT = 0x75;
Q_KEYCODE_PLUS = 0x77;
Q_KEYCODE_EQUALS = 0x78;
Q_KEYCODE_LEFT_BRACKET = 0x81;
Q_KEYCODE_RIGHT_BRACKET = 0x82;
Q_KEYCODE_BACKSLASH = 0x84;
Q_KEYCODE_SEMICOLON = 0x86;
Q_KEYCODE_APOSTROPHE = 0x88;
Q_KEYCODE_COMMA = 0x91;
Q_KEYCODE_PERIOD = 0x92;
Q_KEYCODE_NUMPAD_DIVIDE = 0x94;
Q_KEYCODE_FORWARD_DEL = 0x95;

Q_SETRESOLUTION = 0xf1;
Q_COORDINATE_X = 4096;
Q_COORDINATE_Y = 4096;

drag_start_x=0
drag_start_y=0
drag_end_x=0
drag_end_y=0

def getKeyEvent(x):
        return {
                Q_KEYEVENT_DOWN:MonkeyDevice.DOWN,
                Q_KEYEVENT_UP: MonkeyDevice.UP,
                Q_KEYEVENT_DOWN_AND_UP: MonkeyDevice.DOWN_AND_UP
        }.get(x, Q_KEYEVENT_NOTSUPPORT) 

def getKeyCode(x):
        return {
                Q_KEYCODE_HOME:'KEYCODE_HOME',
                Q_KEYCODE_BACK:'KEYCODE_BACK',
                Q_KEYCODE_MENU:'KEYCODE_MENU',
                Q_KEYCODE_SEARCH:'KEYCODE_SEARCH',
                Q_KEYCODE_DPAD_CENTER:'KEYCODE_DPAD_CENTER',
                Q_KEYCODE_DPAD_UP:'KEYCODE_DPAD_UP',
                Q_KEYCODE_DPAD_DOWN:'KEYCODE_DPAD_DOWN',
                Q_KEYCODE_DPAD_LEFT:'KEYCODE_DPAD_LEFT',
                Q_KEYCODE_DPAD_RIGHT:'KEYCODE_DPAD_RIGHT',
                Q_KEYCODE_MEDIA_PLAY_PAUSE:'KEYCODE_MEDIA_PLAY_PAUSE',
                Q_KEYCODE_ENTER:'KEYCODE_ENTER',
                Q_KEYCODE_DEL:'KEYCODE_DEL',
                Q_KEYCODE_FORWARD_DEL:'KEYCODE_FORWARD_DEL',
                Q_KEYCODE_POWER:'KEYCODE_POWER',
                Q_KEYCODE_VOLUME_DOWN:'KEYCODE_VOLUME_DOWN',
                Q_KEYCODE_VOLUME_UP:'KEYCODE_VOLUME_UP',
                Q_KEYCODE_WHITESPACE:'KEYCODE_SPACE',
                Q_KEYCODE_APP_SWITCH:'KEYCODE_APP_SWITCH',
                Q_KEYCODE_BRIGHTNESS_DOWN:'KEYCODE_BRIGHTNESS_DOWN',
                Q_KEYCODE_BRIGHTNESS_UP:'KEYCODE_BRIGHTNESS_UP',
                Q_KEYCODE_A:'KEYCODE_A',
                Q_KEYCODE_B:'KEYCODE_B',
                Q_KEYCODE_C:'KEYCODE_C',
                Q_KEYCODE_D:'KEYCODE_D',
                Q_KEYCODE_E:'KEYCODE_E',
                Q_KEYCODE_F:'KEYCODE_F',
                Q_KEYCODE_G:'KEYCODE_G',
                Q_KEYCODE_H:'KEYCODE_H',
                Q_KEYCODE_I:'KEYCODE_I',
                Q_KEYCODE_J:'KEYCODE_J',
                Q_KEYCODE_K:'KEYCODE_K',
                Q_KEYCODE_L:'KEYCODE_L',
                Q_KEYCODE_M:'KEYCODE_M',
                Q_KEYCODE_N:'KEYCODE_N',
                Q_KEYCODE_O:'KEYCODE_O',
                Q_KEYCODE_P:'KEYCODE_P',
                Q_KEYCODE_Q:'KEYCODE_Q',
                Q_KEYCODE_R:'KEYCODE_R',
                Q_KEYCODE_S:'KEYCODE_S',
                Q_KEYCODE_T:'KEYCODE_T',
                Q_KEYCODE_U:'KEYCODE_U',
                Q_KEYCODE_V:'KEYCODE_V',
                Q_KEYCODE_W:'KEYCODE_W',
                Q_KEYCODE_X:'KEYCODE_X',
                Q_KEYCODE_Y:'KEYCODE_Y',
                Q_KEYCODE_Z:'KEYCODE_Z',
                Q_KEYCODE_0:'KEYCODE_0',
                Q_KEYCODE_1:'KEYCODE_1',
                Q_KEYCODE_2:'KEYCODE_2',
                Q_KEYCODE_3:'KEYCODE_3',
                Q_KEYCODE_4:'KEYCODE_4',
                Q_KEYCODE_5:'KEYCODE_5',
                Q_KEYCODE_6:'KEYCODE_6',
                Q_KEYCODE_7:'KEYCODE_7',
                Q_KEYCODE_8:'KEYCODE_8',
                Q_KEYCODE_9:'KEYCODE_9',
                Q_KEYCODE_SPACE:'KEYCODE_SPACE',
                Q_KEYCODE_GRAVE:'KEYCODE_GRAVE',
                Q_KEYCODE_AT:'KEYCODE_AT',
                Q_KEYCODE_POUND:'KEYCODE_POUND',
                Q_KEYCODE_START:'KEYCODE_START',
                Q_KEYCODE_NUMPAD_LEFT_PAREN:'KEYCODE_NUMPAD_LEFT_PAREN',
                Q_KEYCODE_NUMPAD_RIGHT_PAREN:'KEYCODE_NUMPAD_RIGHT_PAREN',
                Q_KEYCODE_NUMPAD_SUBTRACT:'KEYCODE_NUMPAD_SUBTRACT',
                Q_KEYCODE_PLUS:'KEYCODE_PLUS',
                Q_KEYCODE_EQUALS:'KEYCODE_EQUALS',
                Q_KEYCODE_LEFT_BRACKET:'KEYCODE_LEFT_BRACKET',
                Q_KEYCODE_RIGHT_BRACKET:'KEYCODE_RIGHT_BRACKET',
                Q_KEYCODE_BACKSLASH:'KEYCODE_BACKSLASH',
                Q_KEYCODE_SEMICOLON:'KEYCODE_SEMICOLON',
                Q_KEYCODE_APOSTROPHE:'KEYCODE_APOSTROPHE',
                Q_KEYCODE_COMMA:'KEYCODE_COMMA',
                Q_KEYCODE_PERIOD:'KEYCODE_PERIOD',
                Q_KEYCODE_NUMPAD_DIVIDE:'KEYCODE_NUMPAD_DIVIDE',
        }.get(x, 'KEYCODE_NOTSUPPORT') 
                        

agent.setblocking(0)
### Please guarantee that the agent will connect in 6s, otherwise it will quit.
trytime = 15
trySuc = False
while(trytime > 0):
        try:
                client,address=agent.accept()
                trySuc = True
                break;
        except:
                trytime = trytime - 1
                time.sleep(1)
if(not trySuc):
        print "failed to connect agent. Quit"
        sys.exit(-1)
print "Accept"
while(1):
        msg = client.recv(4)
        if msg == "exit":
                print "received exit command"
                break;
        if msg == "test":
                #print "received test command"
                try:
#                        device.shell("ls")
                        client.send("yes")
                except:
                        try:
                                if len(sys.argv) < 3:
                                        device = MonkeyRunner.waitForConnection(0.5)
                                else:
                                        device = MonkeyRunner.waitForConnection(0.5, device_name)
#                                device.shell("ls")
                                client.send("yes")
                        except:
                                client.send("no")
                continue;

        cmd = struct.unpack('<I',msg)
        #print time.strftime("%H:%M:%S:",time.localtime())
        #print "The received command is %x" %cmd

        cmd_index = cmd[0] >> 24
        cmd_arg1 = (cmd[0] & 0x00FFF000) >> 12
        cmd_arg2 = (cmd[0] & 0x00000FFF)

        if cmd_index == Q_SETRESOLUTION:
                width = cmd_arg1
                height = cmd_arg2
                print "set resolution to width*height = %d*%d" %(width,height)
        if cmd_index == Q_SHELL_INPUT_TAP:
                try:
                        device.shell(" input tap  %d %d" %(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y))
#                        print (" input tap  %d %d" %(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y))
                except:
                        print "Connection Error! Reconnecting..."
                        print sys.exc_info()
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.shell(" input tap  %d %d" %(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y))
#                        print (" input tap  %d %d" %(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y))
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_SHELL_INPUT_HTAP:
                try:
                        device.shell(" input tap  %d %d" %(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y))
#                        print (" input tap  %d %d" %(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y))
                except:
                        print "Connection Error! Reconnecting..."
                        print sys.exc_info()
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.shell(" input tap  %d %d" %(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y))
#                        print (" input tap  %d %d" %(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y))
                        except:
                                print "Reconnection fails."
                                break;

        if cmd_index == Q_TOUCH_DOWN:
                #print "press down"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.DOWN)
                except:
                        print "Connection Error! Reconnecting..."
                        print sys.exc_info()
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                #print "press down again"
                                device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.DOWN)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_TOUCH_UP:
                #print "press up"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.UP)
                        #print time.time()
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                #print "press up again"
                                device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.UP)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_TOUCH_MOVE:
                #print "press move"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.MOVE)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.touch(cmd_arg1*width/Q_COORDINATE_X, cmd_arg2*height/Q_COORDINATE_Y, MonkeyDevice.MOVE)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_TOUCH_HDOWN:
                #print "press down in horizontal mode"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.DOWN)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                #print "press down in horizontal mode again"
                                device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.DOWN)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_TOUCH_HUP:
                #print "press up in horizontal mode"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.UP)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                #print "press up in horizontal mode again"
                                device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.UP)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_TOUCH_HMOVE:
                #print "touch move in horizontal mode"
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                continue;
                        except:
                                print "multi-touch agent is broken! Use DaVinciRunner instead."
                                is_deamon_connected = False
                try:
                        device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.MOVE)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.touch(cmd_arg1*height/Q_COORDINATE_X, cmd_arg2*width/Q_COORDINATE_Y, MonkeyDevice.MOVE)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_SWIPE_START:
                drag_start_x = cmd_arg1*width/Q_COORDINATE_X
                drag_start_y = cmd_arg2*height/Q_COORDINATE_Y
        if cmd_index == Q_SWIPE_END:
                drag_end_x = cmd_arg1*width/Q_COORDINATE_X
                drag_end_y = cmd_arg2*height/Q_COORDINATE_Y
                #print drag_start_x, drag_start_y, drag_end_x, drag_end_y
                try:
                        device.drag((drag_start_x, drag_start_y),(drag_end_x, drag_end_y),0.2,10)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.drag((drag_start_x, drag_start_y),(drag_end_x, drag_end_y),0.2,10)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_SWIPE_HORIZONTAL_START:
                drag_start_x = cmd_arg1*height/Q_COORDINATE_X
                drag_start_y = cmd_arg2*width/Q_COORDINATE_Y
        if cmd_index == Q_SWIPE_HORIZONTAL_END:
                drag_end_x = cmd_arg1*height/Q_COORDINATE_X
                drag_end_y = cmd_arg2*width/Q_COORDINATE_Y
                #print drag_start_x, drag_start_y, drag_end_x, drag_end_y
                try:
                        device.drag((drag_start_x, drag_start_y),(drag_end_x, drag_end_y),0.2,10)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.drag((drag_start_x, drag_start_y),(drag_end_x, drag_end_y),0.2,10)
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_SHELL_SWIPE_START:
                drag_start_x = cmd_arg1*width/Q_COORDINATE_X
                drag_start_y = cmd_arg2*height/Q_COORDINATE_Y
        if cmd_index == Q_SHELL_SWIPE_END:
                drag_end_x = cmd_arg1*width/Q_COORDINATE_X
                drag_end_y = cmd_arg2*height/Q_COORDINATE_Y
                #print drag_start_x, drag_start_y, drag_end_x, drag_end_y
                try:
                        device.shell(" input touchscreen swipe %d %d %d %d" %(drag_start_x, drag_start_y, drag_end_x, drag_end_y ))
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.shell(" input touchscreen swipe %d %d %d %d" %(drag_start_x, drag_start_y, drag_end_x, drag_end_y ))
                        except:
                                print "Reconnection fails."
                                break;
        if cmd_index == Q_SHELL_SWIPE_HORIZONTAL_START:
                drag_start_x = cmd_arg1*height/Q_COORDINATE_X
                drag_start_y = cmd_arg2*width/Q_COORDINATE_Y
        if cmd_index == Q_SHELL_SWIPE_HORIZONTAL_END:
                drag_end_x = cmd_arg1*height/Q_COORDINATE_X
                drag_end_y = cmd_arg2*width/Q_COORDINATE_Y
                #print drag_start_x, drag_start_y, drag_end_x, drag_end_y
                try:
                        device.shell(" input touchscreen swipe %d %d %d %d" %(drag_start_x, drag_start_y, drag_end_x, drag_end_y ))
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.shell(" input touchscreen swipe %d %d %d %d" %(drag_start_x, drag_start_y, drag_end_x, drag_end_y ))
                        except:
                                print "Reconnection fails."
                                break;

        if cmd_index == Q_SHELL_INPUT_TEXT:
# ACK for Q_SHELL_INPUT_TEXT
                client.send("ok")
                type_string = client.recv(cmd_arg1)
                try:
                        device.shell(" input text " + type_string)
                except:
                        print "Connection Error! Reconnecting..."
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.type(type_string)
                        except:
                                print "Reconnection fails."
                                break;
        if (cmd_index & Q_ACTION_MASK) == Q_MULTI_TOUCH:
# ACK for Q_MULTI_TOUCH
                client.send("ok")
                added = client.recv(4)
                val = struct.unpack('<i',added)
                ptr = val[0]
                #print "pointer is "+str(ptr)
                if is_deamon_connected:
                        try:
                                deamon.send(msg)
                                deamon.send(added)
                        except:
                                print "Warning: the following multi-touch is not supported, thus ignored."
                                is_deamon_connected=False
                else:
                        print "Warning: the following multi-touch is not supported, thus ignored."
                print ptr
                if cmd_index == Q_MULTI_DOWN:
                        print "multi down" 
                if cmd_index == Q_MULTI_UP:
                        print "multi up"
                if cmd_index == Q_MULTI_MOVE:
                        print "multi move"
                if cmd_index == Q_MULTI_HDOWN:
                        print "multi down in horizontal mode"
                if cmd_index == Q_MULTI_HUP:
                        print "multi up in horizontal mode"
                if cmd_index == Q_MULTI_HMOVE:
                        print "multi move in horizontal mode"
        if cmd_index == Q_WAKE:
                #print "wake up the screen"
                try:
                        device.wake()
                except:
                        print "Connection Error!"
                        break;

        if cmd_index == Q_PRESS_KEY:
                keyevent = getKeyEvent(cmd_arg2)
                keycode = getKeyCode(cmd_arg1)

                try:
                        if ((keycode == 'KEYCODE_NOTSUPPORT') or (keyevent == Q_KEYEVENT_NOTSUPPORT)):
                                print "press keyCode:" + keycode + " event:" + keyevent
                        else:
                                device.press(keycode, keyevent)
                        #print "press keyCode:" + keycode + " event:" + keyevent
                except:
                        print "Connection Error! Reconnecting..."
                        print sys.exc_info()
                        if len(sys.argv) < 3:
                                device = MonkeyRunner.waitForConnection(5.0)
                        else:
                                device = MonkeyRunner.waitForConnection(5.0, device_name)
                        print "Reconnected."
                        try:
                                device.press(keycode, keyevent)
                                #print "press keyCode:" + keycode + " event:" + keyevent
                        except:
                                print "Reconnection fails."
                                break;
        #print "send execute result message!"
# ACK for Commands
        client.send("ok")

print "Quit Monkey Agent Connection"
if is_deamon_connected:
        deamon.close()
client.close()
agent.close()
print "Exit"
os.system("exit")
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

#include "EventDispatcher.hpp"

namespace DaVinci
{
    EventDispatcher::EventDispatcher()
    {
        launchableActivity = "";
    }

    EventDispatcher::~EventDispatcher()
    {
    }

    void EventDispatcher::SetLaunchableActivity(string activity)
    {
        launchableActivity = activity;
    }

    void EventDispatcher::SetDevice(boost::shared_ptr<TargetDevice> device)
    {
        assert(device);
        dut = device;
        androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(device);
        assert(androidDut != nullptr);
    }

    void EventDispatcher::SendEvent(EventType type)
    {
        switch(type)
        {
        case EventMenuSwitch:
            {
                dut->PressMenu();
                ThreadSleep(MenuWaitTime); 
                break;
            }
        case EventAppSwitch:
            {
                androidDut->AdbShellCommand("am start com.example.qagent/.MainActivity"); 
                ThreadSleep(RelaunchWaitTime); 
                break;
            }
        case EventRelaunch:
            {
                dut->PressHome();
                ThreadSleep(HomeWaitTime); 
                break;
            }
        default:
            {
                DAVINCI_LOG_WARNING << "Default event type - should not reach here.";
                break;
            }
        }
    }

    void EventDispatcher::RecoverEvent(EventType type)
    {
        switch(type)
        {
        case EventMenuSwitch:
            {
                dut->PressBack();
                ThreadSleep(BackWaitTime);
                break;
            }
        case EventAppSwitch:
            {
                dut->PressBack();
                ThreadSleep(BackWaitTime);
                break;
            }
        case EventRelaunch:
            {
                androidDut->AdbShellCommand("am start " + launchableActivity); 
                ThreadSleep(RelaunchWaitTime); 
                break;
            }
        default:
            {
                DAVINCI_LOG_WARNING << "Default event type - should not reach here.";
                break;
            }
        }
    }
}

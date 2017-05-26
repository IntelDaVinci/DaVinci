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

#ifndef __SCRIPT_RECORDER_HPP__
#define __SCRIPT_RECORDER_HPP__

#include "ScriptEngineBase.hpp"
#include "TestManager.hpp"
#include "ObjectUtil.hpp"
#include "ViewHierarchyParser.hpp"
#include "OnDeviceScriptRecorder.hpp"

namespace DaVinci
{
    class ScriptRecorder : public ScriptEngineBase, public TestManager::UserActionListener
    {
    public:
        ScriptRecorder(const string &script);

        virtual bool Init() override;
        virtual void Destroy() override;

        virtual void OnKeyboard(bool isKeyDown, int keyCode) override;
        virtual void OnMouse(MouseButton button, MouseActionType actionType, int x, int y, int actionX, int actionY, int ptr, Orientation o) override;
        virtual void OnTilt(TiltAction action, int degree1, int degree2, int speed1, int speed2) override;
        virtual void OnZRotate(ZAXISTiltAction zAxisAction, int degree, int speed) override;
        virtual void OnAppAction(AppLifeCycleAction action, string info1, string info2) override;
        virtual void OnSetText(const string &text) override;
        virtual void OnOcr() override;
        virtual void OnSwipe(SwipeAction action) override;
        virtual void OnButton(ButtonAction action, ButtonEventMode mode = ButtonEventMode::DownAndUp) override;
        virtual void OnMoveHolder(int distance) override;
        virtual void OnPowerButtonPusher(PowerButtonPusherAction action) override;
        virtual void OnUsbSwitch(UsbSwitchAction action) override;
        virtual void OnEarphonePuller(EarphonePullerAction action) override;
        virtual void OnRelayController(RelayControllerAction action) override;
        virtual void OnAudioRecord(bool startRecord) override;

        static void RecordAdditionalInfo(int&x, int&y, string &info, const string &qsFileName, const string&packageName, boost::shared_ptr<TargetDevice> dut, int &treeId);

    private:
        int waveFileId;
        boost::atomic<bool> inAudioRecording;
        int treeId;
        int delay;
        boost::shared_ptr<TargetDevice> dut;
        boost::shared_ptr<ViewHierarchyParser> viewParser;
        StopWatch workWatch;
        TestProjectConfigData tstProjConfigData;
        boost::filesystem::path qsFolder;
    };
}

#endif
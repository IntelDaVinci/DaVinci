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

#ifndef __FAILURETYPE__HPP__
#define __FAILURETYPE__HPP__

#include <string>

namespace DaVinci
{
    using namespace std;

    enum FailureType
    {
        ErrMsgProcessExit,
        ErrMsgCrash,
        ErrMsgANR,
        ErrMsgLaunchFail,
        ErrMsgStorage,
        ErrMsgInstallFail,
        ErrMsgUninstallFail,
        ErrMsgX86LibMissing,
        ErrMsgBangBangProtection,
        ErrMsgWangQinShield,
        ErrMsgiJiaMi,
        ErrMsg360Protection,
        ErrMsg360ProtectionV2,
        ErrMsgBangBangProtectionPaid,
        ErrMsgNaGaProtection,
        ErrMsgCitrixXenMobile,
        ErrMsgDexMaker,
        ErrMsgCMCCBillingSDK,
        ErrMsgPayegis,
        ErrMsgQQProtection,
        ErrMsgBaiduProtection,
        ErrMsgVKey,
        ErrMsgWellbia,
        ErrMsgAliProtection,
        ErrMsgHyperTechProtection,
        ErrMsgUnsupportedFeature,
        ErrMsgShowKeyboardAfterLaunch,
        ErrMsgScriptCheck,
        ErrMsgScreenFrozen,
        ErrMsgInitialization,
        WarMsgErrorDialog,
        WarMsgDowngrade,
        WarMsgWinDeath,
        WarMsgFatalException,
        WarMsgSigsegv,
        WarMsgIDebug,
        WarMsgElinker,
        WarMsgAbnormalPage,
        WarMsgActivityEmpty,
        WarMsgAlwaysLoading,
        WarMsgAlwaysHomePage,
        WarMsgNetwork,
        WarMsgDeviceIncompatible,
        WarMsgAbnormalFont,
        WarMsgVideoBlankPage,
        WarMsgVideoFlickering,
        WarMsgReference,
        WarMsgSharingIssue,
        WarMsgLocationIssue,
        WarMsgMax,
        WarMsgAbnormalAudio
    };

    static string failureTypeString[] = {
        "Process exit to home", 
        "Crash dialog",
        "Application no response",
        "Launch fail",
        "Insufficient storage",
        "Install fail",
        "Uninstall fail",
        "X86 lib missing",
        "Bangbang protection",
        "WangQin Shield",
        "i Jia Mi",
        "360 Protection",
        "360 Protection V2",
        "BangBang Protection Paid",
        "NaGa Protection",
        "Citrix XenMobile",
        "DexMaker",
        "CMCC Billing SDK",
        "Payegis",
        "QQ Protection",
        "Baidu Protection",
        "V-key",
        "Wellbia",
        "Ali Protection",
        "HyperTech Protection",
        "Unsupported feature",
        "Show keyboard after launch app",
        "Script check fail",
        "Screen frozen",
        "Initialization issue",
        "Error Dialog",
        "Version Downgrade",
        "Warning: WIN DEATH in logcat",
        "Warning: FATAL EXCEPTION in logcat",
        "Warning: SIGSEGV in logcat",
        "Warning: I/DEBUG in logcat",
        "Warning: E/linker in logcat",
        "Abnormal page",
        "Activity name is empty",
        "Warning: Application always loading",
        "Warning: Always show home page",
        "Network issue",
        "Device Incompatible",
        "Warning: Abnormal font",
        "Video blank page",
        "Video flickering",
        "Reference",
        "Sharing issue",
        "Location issue",
        "Unknown"
    };

    string FailureTypeToString(FailureType type);

    FailureType StringToFailureType(string type);

    bool IsErrorFailureTypeForFrame(FailureType type);

    bool IsWarningFailureTypeForFrame(FailureType type);

    inline string FailureTypeToString(FailureType type)
    {
        if(type >= 0 && type < WarMsgMax)
            return failureTypeString[type];
        else
            return "Unknown";
    }

    inline FailureType StringToFailureType(string type)
    {
        if(type == "AbnormalPage")
            return WarMsgAbnormalPage;
        else if(type == "NetworkIssue")
            return WarMsgNetwork;
        else if(type == "AbnormalFont")
            return WarMsgAbnormalFont;
        else if(type == "VideoBlankPage")
            return WarMsgVideoBlankPage;
        else if(type == "VideoFlickering")
            return WarMsgVideoFlickering;
        else if(type == "Reference")
            return WarMsgReference;
        else if(type == "AlwaysLoading")
            return WarMsgAlwaysLoading;
        else if(type == "AbnormalAudio")
            return WarMsgAbnormalAudio;
        else
            return WarMsgMax;
    }

    inline bool IsErrorFailureTypeForFrame(FailureType type)
    {
        if(type == ErrMsgCrash || type == ErrMsgANR || type == ErrMsgInitialization )
            return true;
        else
            return false;
    }

    inline bool IsWarningFailureTypeForFrame(FailureType type)
    {
        if(type == WarMsgAbnormalFont || type == WarMsgNetwork || type == WarMsgAbnormalPage || type == WarMsgAlwaysLoading || type == WarMsgErrorDialog)
            return true;
        else
            return false;
    }

}
#endif	//#ifndef __FAILURETYPE__HPP__

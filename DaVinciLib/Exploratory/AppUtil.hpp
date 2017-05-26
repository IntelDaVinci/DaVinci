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

#ifndef __APPUTIL__
#define __APPUTIL__

#include <vector>
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    class AppAttribute
    {
    public:
        string packageName;
        bool isInputMethodApp;
        bool isLauncherApp;
        bool hasLoadingIssue;
        bool needSkip;
        bool needClearAccountInput;
        bool needSwipeAfterLogin;
        bool showKeyboardIssue;
        bool closeWifiApp;
        bool modifyFontApp;

        AppAttribute::AppAttribute(string p, bool isInput, bool isLauncher, bool hasLoading, bool skip, bool clearInput, bool swipeAfterLogin, bool showKeyboard, bool closeWifi, bool modifyFont)
        {
            packageName = p;
            isInputMethodApp = isInput;
            isLauncherApp = isLauncher;
            hasLoadingIssue = hasLoading;
            needSkip = skip;
            needClearAccountInput = clearInput;
            needSwipeAfterLogin = swipeAfterLogin;
            showKeyboardIssue = showKeyboard;
            closeWifiApp = closeWifi;
            modifyFontApp = modifyFont;
        }
    };

    /// <summary>
    /// Class AppUtil
    /// </summary>
    class AppUtil : public SingletonBase<AppUtil>
    {
    public:
        /// <summary>
        /// Construct function
        /// </summary>
        AppUtil();

        bool IsInputMethodApp(string packageName);
        bool IsLauncherApp(string packageName);
        bool HasLoadingIssueApp(string packageName);
        bool NeedSkipApp(string packageName);
        bool NeedClearAccountInputApp(string packageName);
        bool NeedSwipeAfterLogin(string packageName);
        bool HasShowKeyboardIssueApp(string packageName);
        bool IsCloseWifiApp(string packageName);
        bool InARCModelNames(string modelName);
        bool IsModifyFontApp(string packageName);
        void InitAllApps();
        bool InSpecialInstallNames(string packageName);

    private:
        AppAttribute AppUtil::InputMethodAppAttribute(string p)
        {
            return AppAttribute(p, true, false, false, false, false, false, false, false, false);
        }

        AppAttribute AppUtil::LauncherAppAttribute(string p)
        {
            return AppAttribute(p, false, true, false, false, false, false, false, false, false);
        }

        AppAttribute AppUtil::LoadingIssueAppAttribute(string p)
        {
            return AppAttribute(p, false, false, true, false, false, false, false, false, false);
        }

        AppAttribute AppUtil::SkipAppAttribute(string p)
        {
            return AppAttribute(p, false, false, false, true, false, false, false, false, false);
        }

        AppAttribute AppUtil::NeedClearAccountAppAttribute(string p)
        {
            return AppAttribute(p, false, false, false, false, true, false, false, false, false);
        }

        AppAttribute AppUtil::NeedSwipeAfterLoginAttribute(string p)
        {
            return AppAttribute(p, false, false, false, false, false, true, false, false, false);
        }

        AppAttribute AppUtil::ShowKeyboardIssueAttribute(string p)
        {
            return AppAttribute(p, false, false, false, false, false, false, true, false, false);
        }

        AppAttribute AppUtil::CloseWifiAttribute(string p)
        {
            return AppAttribute(p, false, false, false, false, false, false, false, true, false);
        }

        AppAttribute AppUtil::ModifyFontAttribute(string p)
        {
            return AppAttribute(p, false, false, false, false, false, false, false, false, true);
        }

        vector<AppAttribute> apps;

        vector<string> ARCModels;

        std::map<std::string, std::vector<std::string>> configApplistMap;
        
        vector<string> specialInstallName;
    };
}


#endif	//#ifndef __APPUTIL__

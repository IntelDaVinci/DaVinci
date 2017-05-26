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

#include "AppUtil.hpp"
#include "TestManager.hpp"
#include "KeywordConfigReader.hpp"

using namespace cv;

namespace DaVinci
{
    const string defaultApplistConfig = "DaVinci_Applist.xml";

    AppUtil::AppUtil()
    {
        InitAllApps();
    }

    void AppUtil::InitAllApps()
    {
        std::string fullAppConfig = TestManager::Instance().GetDaVinciResourcePath(defaultApplistConfig);
        if (fullAppConfig != "" && boost::filesystem::is_regular_file(fullAppConfig))
        {
            boost::shared_ptr<KeywordConfigReader> configReader = boost::shared_ptr<KeywordConfigReader>(new KeywordConfigReader());
            configApplistMap = configReader->ReadApplistConfig(fullAppConfig);
        }

        map<string, vector<string>>::const_iterator map_it = configApplistMap.begin();

        while (map_it != configApplistMap.end())
        {
            string map_key = map_it->first;
            vector<string> map_value = map_it->second;
            ++map_it;

            if (map_key == "InputMethodApps")
            {
                for (string s : map_value)
                    apps.push_back(InputMethodAppAttribute(s));
            }
            else if (map_key == "LauncherApps")
            {
                for (string s : map_value)
                    apps.push_back(LauncherAppAttribute(s));
            }
            else if (map_key == "LoadingIssueApps")
            {
                for (string s : map_value)
                    apps.push_back(LoadingIssueAppAttribute(s));
            }
            else if (map_key == "SkipApps")
            {
                for (string s : map_value)
                    apps.push_back(SkipAppAttribute(s));
            }
            else if (map_key == "AccountClearApps")
            {
                for (string s : map_value)
                    apps.push_back(NeedClearAccountAppAttribute(s));
            }
            else if (map_key == "SwipeAfterLoginApps")
            {
                for (string s : map_value)
                    apps.push_back(NeedSwipeAfterLoginAttribute(s));
            }
            else if (map_key == "ShowKeyboardIssueApps")
            {
                for (string s : map_value)
                    apps.push_back(ShowKeyboardIssueAttribute(s));
            }
            else if (map_key == "CloseWIFIApps")
            {
                for (string s : map_value)
                    apps.push_back(CloseWifiAttribute(s));
            }
            else if (map_key == "ARCModelName")
            {
                for (string s : map_value)
                    ARCModels.push_back(s);
            }
            else if (map_key == "ModifyFontApps")
            {
                for (string s : map_value)
                    apps.push_back(ModifyFontAttribute(s));
            }
            else if (map_key == "SpecialInstallApps")
            {
                for (string s : map_value)
                {
                    if (!s.empty())
                        specialInstallName.push_back(s);
                }
            }
        }
    }

    bool AppUtil::InARCModelNames(string model)
    {
        boost::algorithm::to_lower(model);
        for (auto arcModel : ARCModels)
        {
            boost::algorithm::to_lower(arcModel);
            if (model.find(arcModel) != string::npos)
                return true;
        }

        return false;
    }

    bool AppUtil::InSpecialInstallNames(string packageName)
    {
        for (auto sName : specialInstallName)
        {
            if (sName == packageName)
                return true;
        }

        return false;
    }


    bool AppUtil::IsInputMethodApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.isInputMethodApp && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::IsLauncherApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.isLauncherApp && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::HasLoadingIssueApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.hasLoadingIssue && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::NeedSkipApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.needSkip && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::NeedClearAccountInputApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.needClearAccountInput && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::NeedSwipeAfterLogin(string packageName)
    {
        for(auto app : apps)
        {
            if(app.needSwipeAfterLogin && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::HasShowKeyboardIssueApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.showKeyboardIssue && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::IsCloseWifiApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.closeWifiApp && app.packageName == packageName)
                return true;
        }

        return false;
    }

    bool AppUtil::IsModifyFontApp(string packageName)
    {
        for(auto app : apps)
        {
            if(app.modifyFontApp && app.packageName == packageName)
                return true;
        }

        return false;
    }
}

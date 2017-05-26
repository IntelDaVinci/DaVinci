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

#include "KeywordUtil.hpp"

namespace DaVinci
{
    KeywordUtil::KeywordUtil()
    {
    }

    std::string KeywordUtil::KeywordCategoryToString(KeywordCategory keywordCategory)
    {
        switch(keywordCategory)
        {
        case KeywordCategory::COMMON:
            return "Common";
        case KeywordCategory::NETWORKISSUE:
            return "NetworkIssue";
        case KeywordCategory::ANR:
            return "ANR";
        case KeywordCategory::CRASHDIALOG:
            return "CrashDialog";
        case KeywordCategory::INITIALIZATIONISSUE:
            return "InitializationIssue";
        case KeywordCategory::ABNORMALFONT:
            return "AbnormalFont";
        case KeywordCategory::INSTALLCONFIRM:
            return "InstallConfirm";
        case KeywordCategory::SMARTLOGIN:
            return "SmartLogin";
        case KeywordCategory::DOWNLOAD:
            return "Download";
        case KeywordCategory::LICENSE:
            return "License";
        case KeywordCategory::RNR:
            return "RnR";
        case KeywordCategory::LAUNCHERAPP:
            return "LauncherApp";
        case KeywordCategory::ERRORDIALOG:
            return "ErrorDialog";
        case KeywordCategory::PROCESSEXIT:
            return "ProcessExit";
        case KeywordCategory::LOGINFAIL:
            return "LoginFail";
        case KeywordCategory::GOOGLEFRAMEWORKISSUE:
            return "GoogleFrameworkIssue";
        case KeywordCategory::SHAREISSUE:
            return "ShareIssue";
        case KeywordCategory::LOCATIONISSUE:
            return "LocationIssue";
        case KeywordCategory::COMMONIDINFO:
            return "CommonIDInfo";
        case KeywordCategory::ABNORMALPAGEKEYWORD:
            return "AbnormalPageKeyword";
        case KeywordCategory::POPUPWINDOW:
            return "PopupWindow";
        default:
            return "Unknown";
        }
    }

    std::vector<Keyword> KeywordUtil::FilterKeywords(const std::vector<Keyword> &allKeywords, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea)
    {
        std::vector<Keyword> keywords;

        for (auto keyword : allKeywords)
        {
            cv::Rect rect = keyword.location;
            double ratio = (double)rect.width/ rect.height;
            if (ratio >= lowSideRatio && ratio <= highSideRatio && rect.area() >= lowRectArea && rect.area() <= highRectArea)
            {
                keywords.push_back(keyword);
            }
        }

        return keywords;
    }
}
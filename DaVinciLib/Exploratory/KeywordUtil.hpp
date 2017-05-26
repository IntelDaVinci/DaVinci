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

#ifndef __KEYWORDUTIL__
#define __KEYWORDUTIL__

#include "opencv/cv.h"

#include "DaVinciCommon.hpp"

namespace DaVinci
{
    enum Priority
    {
        NONE,
        LOW,
        MEDIUM,
        HIGH
    };

    enum KeywordClass
    {
        NEGATIVE,
        POSITIVE
    };

    enum KeywordCategory
    {
        COMMON = 0,
        NETWORKISSUE = 1,
        ANR = 2,
        CRASHDIALOG = 3,
        INITIALIZATIONISSUE = 4,
        ABNORMALFONT = 5,
        INSTALLCONFIRM = 6,
        SMARTLOGIN = 7,
        DOWNLOAD = 8,
        LICENSE = 9,
        RNR = 10,
        LAUNCHERAPP = 11,
        ERRORDIALOG = 12,
        PROCESSEXIT = 13,
        LOGINFAIL = 14,
        GOOGLEFRAMEWORKISSUE = 15,
        SHAREISSUE = 16,
        LOCATIONISSUE = 17,
        COMMONIDINFO = 18,
        ABNORMALPAGEKEYWORD = 19,
        POPUPWINDOW = 20,
        UNKNOWNKEYWORD = 21
    };

    struct Keyword
    {
        std::wstring text;
        Priority priority;
        cv::Rect location;
        KeywordClass keywordClass;

        bool operator==(const Keyword &keyword) const
        {
            return (text == keyword.text && priority == keyword.priority
                && location == keyword.location && keywordClass == keyword.keywordClass);
        }
    };

    struct SortKeywordsByKeywordClassAsc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.keywordClass < keyword2.keywordClass);
        }
    };

    struct SortKeywordsByKeywordClassDesc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.keywordClass > keyword2.keywordClass);
        }
    };

    struct SortKeywordsByKeywordPriorityDesc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.priority > keyword2.priority);
        }
    };

    struct SortKeywordsByKeywordLocationXAsc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.location.x < keyword2.location.x);
        }
    };

    struct SortKeywordsByKeywordLocationYAsc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.location.y < keyword2.location.y);
        }
    };

    struct SortKeywordsByKeywordLocationYDesc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.location.y > keyword2.location.y);
        }
    };

    struct SortKeywordsByKeywordAreaAsc
    {
        bool operator()(const Keyword &keyword1, const Keyword &keyword2)
        {
            return (keyword1.location.area() < keyword2.location.area());
        }
    };

    class KeywordUtil : public SingletonBase<KeywordUtil>
    {
        SIGLETON_CHILD_CLASS(KeywordUtil);

    public:
        std::string KeywordCategoryToString(KeywordCategory keywordCategory);

        std::vector<Keyword> FilterKeywords(const std::vector<Keyword> &allKeywords, double lowSideRatio, double highSideRatio, int lowRectArea, int highRectArea);
    };
}

#endif
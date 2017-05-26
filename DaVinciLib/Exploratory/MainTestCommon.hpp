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

#ifndef __MAINTESTCOMMON__HPP__
#define __MAINTESTCOMMON__HPP__

#include <string>
#include <vector>
#include <unordered_map>

#include "boost/smart_ptr/shared_ptr.hpp"

#include "DaVinciCommon.hpp"
#include "FailureType.hpp"

namespace DaVinci
{
    using namespace std;

    // Schedule event match pattern
    enum ScheduleEventMatchType
    {
        MatchType_ID = 0,
        MatchType_Virtual,
        MatchType_Stretch,
        MatchType_DPI,
        MatchType_Text,
        MatchType_Shape,
        MatchType_Akaze,
        MatchType_RecordPopUp, 
        MatchType_RecordCross, 
        MatchType_KeyboardPopUp, 
        MatchType_SystemPopUp, 
        MatchType_Random, 
        MatchType_RandomPopUp, 
        MatchType_RandomCross, 
        MatchType_Unknown
    };

    // QSPriority defined in XML
    enum QSPriority
    {
        QS_HIGH = 1,
        QS_MEDIUM,
        QS_LOW,
        QS_MAX
    };

    // Device resolution
    enum QSResolution
    {
        R_4_3,
        R_16_9,
        R_16_10
    };

    // QS Popup type
    enum QSPopUpType
    {
        PopUpType_Record_Only = 0,
        PopUpType_Replay_Only = 1,
        PopUpType_Same = 2,
        PopUpType_Unknown = 3
    };

    struct FailurePair
    {
        int index;
        string qsName;
        int startIp;
        FailureType type;

        FailurePair()
        {
            index = 0;
            qsName = "";
            startIp = 0;
            type = FailureType::WarMsgMax;
        }

        FailurePair(int d, string s, int i, FailureType t)
        {
            index = d;
            qsName = s;
            startIp = i;
            type = t;
        }
    };

    // QSIpPair: light data structure with qsName, startIp, orientation
    struct QSIpPair
    {
        string qsName;
        int startIp;
        Orientation orientation;
        bool random;
        bool touch;

        QSIpPair()
        {
            qsName = "";
            startIp = 0;
            orientation = Orientation(0);
            random = false;
            touch = true;
        }

        QSIpPair(string s, int i, Orientation o, bool r = false, bool t = true)
        {
            qsName = s;
            startIp = i;
            orientation = o;
            random = r;
            touch = t;
        }

        bool IsSameQSIpPair(const QSIpPair target) const
        {
            if(this->qsName == target.qsName && 
                this->startIp == target.startIp &&
                this->orientation == target.orientation)
                return true;
            else
                return false;
        }
    };

    // Schedule event used for QS block single step execution
    struct ScheduleEvent
    {
        string qsName;
        int startIp;
        int endIp;
        QSPriority qsPriority;

        bool isTouchMove;
        Point2f objectMatchedPoint;

        bool isAdvertisement;

        Rect mainROI;
        vector<boost::shared_ptr<Shape>> shapes;
        bool isTriangle;
        Point trianglePoint1;
        Point trianglePoint2;

        ScheduleEvent()
        {
            qsName = "";
            startIp = 0;
            endIp = 0;
            qsPriority = QSPriority::QS_MAX;
            isTouchMove = false;
            isAdvertisement = false;
        }

        ScheduleEvent(string name, int i, int j)
        {
            qsName = name;
            startIp = i;
            endIp = j;
            qsPriority = QSPriority::QS_MAX;
            isTouchMove = false;
            isAdvertisement = false;
        }

        // Check same event
        bool IsSameEvent(const ScheduleEvent target) const
        {
            if(this->qsName == target.qsName && 
                this->startIp == target.startIp &&
                this->endIp == target.endIp)
                return true;
            else
                return false;
        }

        // FIXME: for single qs, need enhancing for multiple qs
        bool IsForwardEvent(const ScheduleEvent target) const
        {
            return (this->qsName == target.qsName && this->startIp > target.startIp);
        }
    };
}
#endif //#ifndef __MAINTESTCOMMON__HPP__
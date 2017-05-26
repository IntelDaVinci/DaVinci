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

#ifndef __OBJECTLABEL__HPP__
#define __OBJECTLABEL__HPP__

#include <unordered_map>
#include "opencv2/opencv.hpp"
#include "DaVinciCommon.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    enum ObjectLabel
    {
        OBJECT_ADS = 0,
        OBJECT_BACK,
        OBJECT_BUTTON,
        OBJECT_CAMERA,
        OBJECT_CART,
        OBJECT_CATEGORY,
        OBJECT_DOWNLOAD,
        OBJECT_EDIT,
        OBJECT_FACEBOOK,
        OBJECT_FAVORITE,
        OBJECT_FEELING,
        OBJECT_FORWARD,
        OBJECT_HOME,
        OBJECT_IMAGEICON,
        OBJECT_LOCATION,
        OBJECT_MAIL,
        OBJECT_MINE,
        OBJECT_MORE,
        OBJECT_PLAY,
        OBJECT_POST,
        OBJECT_QQ,
        OBJECT_REFRESH,
        OBJECT_REPLY,
        OBJECT_SCAN,
        OBJECT_SEARCH,
        OBJECT_SETTING,
        OBJECT_SHARING,
        OBJECT_SINAWEIBO,
        OBJECT_SWIPE,
        OBJECT_TECENTWEIBO,
        OBJECT_THUMBUP,
        OBJECT_TWITTER,
        OBJECT_WECHAT,
        OBJECT_WECHATCIRCLE,
        OBJECT_YES,
        OBJECT_ZOOM,
        OBJECT_UNKNOWN
    };

    struct ObjectLabelNode
    {
        ObjectLabel label;
        float confidence;
        Rect roi;
    };

    enum ObjectPriority
    {
        OBJECT_LOW = 0,
        OBJECT_MEDIUM,
        OBJECT_HIGH
    };

    static char* objectLabelString[] = {
        "ads", 
        "back",
        "button",
        "camera",
        "cart",
        "category",
        "download",
        "edit",
        "facebook",
        "favorite",
        "feeling",
        "forward",
        "home",
        "image",
        "location",
        "mail",
        "mine",
        "more",
        "play",
        "post",
        "qq",
        "refresh",
        "reply",
        "scan",
        "search",
        "setting",
        "sharing",
        "sinaweibo",
        "swipe",
        "tecentweibo",
        "thumbup",
        "twitter",
        "wechat",
        "wechatcircle",
        "yes",
        "zoom",
        "unknown"
    };

    std::unordered_map<ObjectLabel, ObjectPriority> InitializeObjectPriority();
    string ObjectLabelToString(ObjectLabel label);
    ObjectLabel StringToObjectLabel(char* label);
    vector<ObjectLabelNode> WrapObjectLabelNodes(vector<char*> labels, vector<Rect> rects, vector<float> scores);
    void SortObjectLabelsByConfidence(std::vector<ObjectLabelNode> &keywords, bool isAsc);

    inline std::unordered_map<ObjectLabel, ObjectPriority> InitializeObjectPriority()
    {
        std::unordered_map<ObjectLabel, ObjectPriority> objectPriorityMap;
        objectPriorityMap[OBJECT_ADS] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_BACK] = OBJECT_LOW;
        objectPriorityMap[OBJECT_BUTTON] = OBJECT_LOW;
        objectPriorityMap[OBJECT_CAMERA] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_CART] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_CATEGORY] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_DOWNLOAD] = OBJECT_LOW;
        objectPriorityMap[OBJECT_EDIT] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_FACEBOOK] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_FAVORITE] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_FEELING] = OBJECT_LOW;
        objectPriorityMap[OBJECT_FORWARD] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_HOME] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_IMAGEICON] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_LOCATION] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_MAIL] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_MINE] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_MORE] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_PLAY] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_POST] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_QQ] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_REFRESH] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_REPLY] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_SCAN] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_SEARCH] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_SETTING] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_SHARING] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_SINAWEIBO] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_SWIPE] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_TECENTWEIBO] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_THUMBUP] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_TWITTER] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_WECHAT] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_WECHATCIRCLE] = OBJECT_HIGH;
        objectPriorityMap[OBJECT_YES] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_ZOOM] = OBJECT_MEDIUM;
        objectPriorityMap[OBJECT_UNKNOWN] = OBJECT_LOW;

        return objectPriorityMap;
    }

    inline string ObjectLabelToString(ObjectLabel label)
    {
        if(label >= 0 && label < OBJECT_UNKNOWN)
            return string(objectLabelString[label]);
        else
            return "unknown";
    }

    inline ObjectLabel StringToObjectLabel(char* label)
    {
        for(int i = 0; i < OBJECT_UNKNOWN; i++)
        {
            if(string(objectLabelString[i]) == string(label))
                return (ObjectLabel)(i);
        }

        return OBJECT_UNKNOWN;
    }

    inline vector<ObjectLabelNode> WrapObjectLabelNodes(vector<char*> labels, vector<float> scores, vector<Rect> rects)
    {
        assert(int(rects.size()) == int(labels.size()));
        assert(int(rects.size()) == int(scores.size()));
        vector<ObjectLabelNode> nodes;
        for(int i = 0 ; i < (int)rects.size(); i++)
        {
            ObjectLabelNode node;
            node.label = StringToObjectLabel(labels[i]);
            node.confidence = scores[i];
            node.roi = rects[i];
            nodes.push_back(node);
        }

        return nodes;
    }

    struct SortObjectLabelByConfidenceAsc
    {
        bool operator()(const ObjectLabelNode &node1, const ObjectLabelNode &node2)
        {
            return FSmaller(node1.confidence, node2.confidence);
        }
    };

    struct SortObjectLabelByConfidenceDesc
    {
        bool operator()(const ObjectLabelNode &node1, const ObjectLabelNode &node2)
        {
            return FSmaller(node2.confidence, node1.confidence);
        }
    };

    inline void SortObjectLabelsByConfidence(std::vector<ObjectLabelNode> &nodes, bool isAsc)
    {
        if (isAsc)
        {
            std::sort(nodes.begin(), nodes.end(), SortObjectLabelByConfidenceAsc());
        }
        else
        {
            std::sort(nodes.begin(), nodes.end(), SortObjectLabelByConfidenceDesc());
        }
    }

    struct SortObjectLabelByPriorityAsc
    {
        bool operator()(const ObjectLabelNode &node1, const ObjectLabelNode &node2)
        {
            std::unordered_map<ObjectLabel, ObjectPriority> priority_info = InitializeObjectPriority();
            return (priority_info[node1.label] < priority_info[node2.label]);
        }
    };

    struct SortObjectLabelByPriorityDesc
    {
        bool operator()(const ObjectLabelNode &node1, const ObjectLabelNode &node2)
        {
            std::unordered_map<ObjectLabel, ObjectPriority> priority_info = InitializeObjectPriority();
            return (priority_info[node1.label] > priority_info[node2.label]);
        }
    };

    inline void SortObjectLabelsByPriority(std::vector<ObjectLabelNode> &nodes, bool isAsc)
    {
        if (isAsc)
        {
            std::sort(nodes.begin(), nodes.end(), SortObjectLabelByPriorityAsc());
        }
        else
        {
            std::sort(nodes.begin(), nodes.end(), SortObjectLabelByPriorityDesc());
        }
    }

}
#endif	//#ifndef __OBJECTLABEL__HPP__

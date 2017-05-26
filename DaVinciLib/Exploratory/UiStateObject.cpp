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

#include <vector>
#include <regex>

#include "UiStateObject.hpp"
#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "UiRectangle.hpp"


#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

using namespace std;
using namespace xercesc;

namespace DaVinci
{
    UiStateObject::UiStateObject()
    {
        isClickable = false;
        isControl = false;
    }

    UiStateObject::UiStateObject(const DOMNode &xmlNode)
    {
        isClickable = false;
        isControl = false;
        DOMNamedNodeMap* attributes = xmlNode.getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();
        std::string rectString;
        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "text"))
            {
                SetText(XMLStrToStr(attribute->getNodeValue()));
            }
            else if (boost::equals(attribute->getNodeName(), "bounds"))
            {
                rectString = XMLStrToStr(attribute->getNodeValue());
            }
        }

        vector<string> items;
        vector<string> res;
        boost::algorithm::split(items, rectString, boost::algorithm::is_any_of("]"));
        assert(items.size() >= 2);
        string left = boost::replace_all_copy(items[0], "[", "");
        string right = boost::replace_all_copy(items[1], "[", "");
        boost::algorithm::split(items, left, boost::algorithm::is_any_of(","));
        res.push_back(items[0]);
        res.push_back(items[1]);
        boost::algorithm::split(items, right, boost::algorithm::is_any_of(","));
        res.push_back(items[0]);
        res.push_back(items[1]);

        assert(res.size() >= 4);
        UiPoint leftUp = UiPoint(boost::lexical_cast<int>(res[0]), boost::lexical_cast<int>(res[1]));
        UiPoint rightBottom = UiPoint(boost::lexical_cast<int>(res[2]), boost::lexical_cast<int>(res[3]));
        UiRectangle rect(leftUp, rightBottom);
        SetRect(rect);
    }

    const std::string UiStateObject::GetText() const
    {
        return text;
    }

    void UiStateObject::SetText(const std::string &value)
    {
        text = value;
    }

    const UiRectangle UiStateObject::GetRect() const
    {
        return rect;
    }

    void UiStateObject::SetRect(const UiRectangle &value)
    {
        rect = value;
    }

    const bool UiStateObject::GetClickable() const
    {
        return isClickable;
    }

    void UiStateObject::SetClickable(const bool clickable)
    {
        isClickable = clickable;
    }

    const bool UiStateObject::GetControlState() const
    {
        return isControl;
    }

    void UiStateObject::SetControlState(const bool control)
    {
        isControl = control;
    }

    std::string UiStateObject::takeAction(const boost::shared_ptr<UiAction> &action)
    {
        return "";
    }

    const int UiStateObject::GetHashCode() const
    {
        return GetRect().GetHashCode();
    }

    const std::string UiStateObject::GetIDInfo() const
    {
        return idInfo;
    }

    void UiStateObject::SetIDInfo(const std::string &value)
    {
        idInfo = value;
    }
}

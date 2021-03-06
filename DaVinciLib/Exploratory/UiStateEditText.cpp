﻿/*
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

#include "UiStateEditText.hpp"
#include "UiRectangle.hpp"
#include "UiPoint.hpp"

namespace DaVinci
{
    UiStateEditText::UiStateEditText(const DOMNode &xmlNode)
    {
    }

    UiStateEditText::UiStateEditText(Rect r)
    {
        SetRect(UiRectangle(UiPoint(r.x, r.y), UiPoint(r.x + r.width, r.y + r.height)));
    }

    std::string UiStateEditText::takeAction(const boost::shared_ptr<UiAction> &action)
    {
        action->ActionForEditText();
        return "Set Text";
    }

    std::string UiStateEditText::ToString()
    {
        UiRectangle rect = GetRect();
        UiPoint centerPoint = rect.GetCenter();
        std::string str("TEXT_" + centerPoint.ToString() + "_\"" + GetText() + "\"");
        return str;
    }
}

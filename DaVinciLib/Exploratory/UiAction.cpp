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

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "UiAction.hpp"
#include "UiRectangle.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectUtil.hpp"
#include "ObjectCommon.hpp"

namespace DaVinci
{
    UiAction::UiAction(const std::string &inputString, const UiRectangle &clickPosition, const cv::Mat &originalFrame, const boost::shared_ptr<TargetDevice> &device) : dut(device)
    {
        SetInputString(inputString);
        SetClickPosition(clickPosition);
        Setframe(originalFrame);
    }

    std::string UiAction::ToString()
    {
        return GetStringRepr();
    }

    const std::string &UiAction::GetInputString() const
    {
        return inputString;
    }

    void UiAction::SetInputString(const std::string &value)
    {
        inputString = value;
    }

    const UiRectangle &UiAction::GetClickPosition() const
    {
        return clickPosition;
    }

    void UiAction::SetClickPosition(const UiRectangle &value)
    {
        clickPosition = value;
    }

    const std::string &UiAction::GetStringRepr() const
    {
        return stringRepr;
    }

    void UiAction::SetStringRepr(const std::string &value)
    {
        stringRepr = value;
    }

    const cv::Mat &UiAction::Getframe() const
    {
        return frame;
    }

    void UiAction::Setframe(const cv::Mat &value)
    {
        frame = value;
    }

    void UiAction::ActionForBack() const
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            androidDut->PressBack();
            DAVINCI_LOG_INFO << "Click Back";
        }
    }

    void UiAction::ActionForClickable() const
    {
        UiRectangle rect = GetClickPosition();
        UiPoint p = rect.GetCenter();
        Point point = Point(p.GetX(), p.GetY());

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            ClickOnRotatedFrame(Getframe().size(), point);
            DAVINCI_LOG_INFO << "Click uiautomator object (" <<  boost::lexical_cast<string>(point.x) << std::string(",") + boost::lexical_cast<string>(point.y) << std::string(")");
        }
    }

    void UiAction::ActionForGeneralClickable() const
    {
        UiRectangle rect = GetClickPosition();
        UiPoint p = rect.GetCenter();
        Point point = Point(p.GetX(), p.GetY());

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            ClickOnRotatedFrame(Getframe().size(), point);
            DAVINCI_LOG_INFO << std::string("Click general object (") << boost::lexical_cast<string>(point.x) << std::string(",") +  boost::lexical_cast<string>(point.y) << std::string(")");
        }
    }

    void UiAction::ActionForEditText() const
    {
        UiRectangle rect = GetClickPosition();
        UiPoint p = rect.GetCenter();
        Point point = Point(p.GetX(), p.GetY());
        ClickOnRotatedFrame(Getframe().size(), point);

        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            androidDut->TypeString(GetInputString());
            ThreadSleep(500);

            DAVINCI_LOG_INFO << std::string("Set Text: ") << GetInputString();
        }
    }

    void UiAction::ActionForHome() const
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            androidDut->PressHome();
            DAVINCI_LOG_INFO << "Click Home";
        }
    }

    void UiAction::ActionForMenu() const
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            androidDut->PressMenu();
            DAVINCI_LOG_INFO << "Click Menu";
        }
    }

    void UiAction::ActionForSwipe() const
    {
        boost::shared_ptr<AndroidTargetDevice> androidDut = boost::dynamic_pointer_cast<AndroidTargetDevice>(dut);
        if (androidDut != nullptr)
        {
            DAVINCI_LOG_INFO << "SWIPE: " << GetInputString();

            if (GetInputString() == "Left")
            {
                DAVINCI_LOG_INFO << "Swipe Left";
                androidDut->Drag(2048, 2048, 1024, 2048, androidDut->GetCurrentOrientation());
            }
            else if (GetInputString() == "Right")
            {
                DAVINCI_LOG_INFO << "Swipe Right";
                androidDut->Drag(2048, 2048, 3072, 2048, androidDut->GetCurrentOrientation());
            }
            else if (GetInputString() == "Up")
            {
                DAVINCI_LOG_INFO << "Swipe Up";
                androidDut->Drag(2048, 2048, 2048, 1024, androidDut->GetCurrentOrientation());
            }
            else
            {
                DAVINCI_LOG_INFO << "Swipe Down";
                androidDut->Drag(2048, 2048, 2048, 3072, androidDut->GetCurrentOrientation());
            }
        }
    }
}
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

#include "UiPoint.hpp"
#include "UiRectangle.hpp"

using namespace std;

namespace DaVinci
{
    UiRectangle::UiRectangle()
    {                                    
        leftUpper = UiPoint(0,0);
        rightBottom = UiPoint(0,0);
    }

    UiRectangle::UiRectangle(const UiPoint &lu, const UiPoint &rb)
    {
        leftUpper = lu;
        rightBottom = rb;
    }

    UiPoint UiRectangle::GetCenter()
    {
        return UiPoint((leftUpper.GetX() + rightBottom.GetX()) / 2, (leftUpper.GetY() + rightBottom.GetY()) / 2);
    }

    const int UiRectangle::GetHashCode() const
    {
        return leftUpper.GetHashCode() + rightBottom.GetHashCode();
    }

    const bool UiRectangle::Equals(const UiRectangle &obj) const
    {
        return leftUpper.Equals(obj.GetLeftUpper()) && rightBottom.Equals(obj.GetRightBottom());
    }

    const UiPoint UiRectangle::GetLeftUpper() const
    {
        return leftUpper;
    }

    void UiRectangle::SetLeftUpper(const UiPoint &value)
    {
        leftUpper = value;
    }

    const UiPoint UiRectangle::GetRightBottom() const
    {
        return rightBottom;
    }

    void UiRectangle::SetRightBottom(const UiPoint &value)
    {
        rightBottom = value;
    }
}

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

using namespace std;
using namespace cv;

namespace DaVinci
{
    UiPoint::UiPoint()
    {
        SetX(0);
        SetY(0);
    }

    UiPoint::UiPoint(int x, int y)
    {
        SetX(x);
        SetY(y);
    }

    UiPoint::UiPoint(const Rect &r)
    {
        SetX(r.x + r.width / 2);
        SetY(r.y + r.height / 2);
    }

    const int UiPoint::GetHashCode() const
    {
        return GetX() ^ GetY();
    }

    const bool UiPoint::Equals(const UiPoint &obj) const
    {
        return (GetX() == obj.GetX()) && (GetY() == obj.GetY());
    }

    std::string UiPoint::ToString()
    {
        std::string str("({%d},{%d})", GetX(), GetY());
        return str;
    }

    const int UiPoint::GetX() const
    {
        return x;
    }

    void UiPoint::SetX(const int &value)
    {
        x = value;
    }

    const int UiPoint::GetY() const
    {
        return y;
    }

    void UiPoint::SetY(const int &value)
    {
        y = value;
    }
}

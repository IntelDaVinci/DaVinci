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
#include <map>

#include "opencv2/opencv.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "UiState.hpp"
#include "UiRectangle.hpp"
#include "PriorityWords.hpp"
#include "UiStateEditText.hpp"

namespace DaVinci
{
    UiState::UiState()
    {
        SetEditTextNum(0);
        SetNoClickable(false);
        SetCurrentPackageName("");
        SetClickObjNum(0);
        uiObjects = std::unordered_map<boost::shared_ptr<UiStateObject>, bool, UiStateObjectHash>();
    }

    void UiState::AddUiStateObject(const boost::shared_ptr<UiStateObject> &uso)
    {
        if(uiObjects.find(uso) == uiObjects.end())
        {
            uiObjects.insert(make_pair(uso, false));
        }
    }

    std::vector<boost::shared_ptr<UiStateObject>> UiState::GetEditTextFields() const
    {
        std::vector<boost::shared_ptr<UiStateObject>> fields;

        for(auto pair : uiObjects)
        {
            if(pair.first->GetClickable() == false)
            {
                fields.push_back(pair.first);
            }
        }

        return fields;
    }

    std::vector<boost::shared_ptr<UiStateObject>> UiState::GetClickableObjects() const
    {
        std::vector<boost::shared_ptr<UiStateObject>> objects;
        for(auto pair : uiObjects)
        {
            if(pair.first->GetClickable() == true)
            {
                objects.push_back(pair.first);
            }
        }

        return objects;
    }


    int UiState::GetHashCode() const
    {
        int result = 19;
        for (auto pair : uiObjects)
        {
            result += 31 * pair.first->GetRect().GetHashCode();
        }
        return result;
    }

    bool UiState::Equals(const UiState &obj)
    {
        UiState o = obj;
        return (o.GetHashCode() == GetHashCode());
    }

    const int &UiState::GetEditTextNum() const
    {
        return editTextNum;
    }

    void UiState::SetEditTextNum(const int &value)
    {
        editTextNum = value;
    }

    const int &UiState::GetClickObjNum() const
    {
        return clickObjNum;
    }

    void UiState::SetClickObjNum(const int &value)
    {
        clickObjNum = value;
    }

    const bool &UiState::GetNoClickable() const
    {
        return noClickable;
    }

    void UiState::SetNoClickable(const bool &value)
    {
        noClickable = value;
    }

    const std::string &UiState::GetCurrentPackageName() const
    {
        return currentPackageName;
    }

    void UiState::SetCurrentPackageName(const std::string &value)
    {
        currentPackageName = value;
    }
}

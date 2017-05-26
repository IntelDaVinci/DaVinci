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

#ifndef __UISTATE__
#define __UISTATE__

#include <unordered_map>
#include <vector>

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/process.hpp"
#include "boost/thread/thread.hpp"
#include "boost/chrono/chrono.hpp"

#include "opencv2/opencv.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "UiStateObject.hpp"

namespace DaVinci
{
    /// <summary>
    /// Class UiState
    /// </summary>
    class UiState
    {
        /// <summary>
        /// Construct
        /// </summary>
    public:
        UiState();

        /// <summary>
        /// Add UiStateObject
        /// </summary>
        /// <param name="uso"></param>
        void AddUiStateObject(const boost::shared_ptr<UiStateObject> &uso);

        /// <summary>
        /// Get edit text fields
        /// </summary>
        /// <returns></returns>
        std::vector<boost::shared_ptr<UiStateObject>> GetEditTextFields() const;

        /// <summary>
        /// Get clickable objects
        /// </summary>
        /// <returns></returns>
        std::vector<boost::shared_ptr<UiStateObject>> GetClickableObjects() const;

        /// <summary>
        /// Get hash code
        /// </summary>
        /// <returns></returns>
        virtual int GetHashCode() const;

        /// <summary>
        /// Override Equals
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        virtual bool Equals(const UiState &obj);

        const int &GetClickObjNum() const;
        void SetClickObjNum(const int &value);
        const bool &GetNoClickable() const;
        void SetNoClickable(const bool &value);
        const int &GetEditTextNum() const;
        void SetEditTextNum(const int &value);
        const std::string &GetCurrentPackageName() const;
        void SetCurrentPackageName(const std::string &value);

        struct UiStateObjectHash 
        {
            size_t operator()(const boost::shared_ptr<UiStateObject> x) const 
            { 
                return x->GetHashCode(); 
            }
        };

        //const PriorityWords &GetKeywords();
        //void SetKeywords(const PriorityWords &value);

    private:
        int editTextNum;

        /// <summary>
        /// No Clickable
        /// </summary>
        bool noClickable;

        std::string currentPackageName;
        int clickObjNum;
        std::unordered_map<boost::shared_ptr<UiStateObject>, bool, UiStateObjectHash> uiObjects;
        //PriorityWords keywords;

    };
}


#endif	//#ifndef __UISTATE__

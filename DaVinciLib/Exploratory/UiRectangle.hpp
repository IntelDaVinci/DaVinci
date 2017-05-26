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

#ifndef __UIRECTANGLE__
#define __UIRECTANGLE__

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "UiPoint.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// Class UiRectangle
    /// </summary>
    class UiRectangle
    {

    public:
        /// <summary>
        /// Construct
        /// </summary>
        UiRectangle();

        /// <summary>
        /// Construct
        /// </summary>
        /// <param name="lu"></param>
        /// <param name="rb"></param>
        UiRectangle(const UiPoint &lu, const UiPoint &rb);

        /// <summary>
        /// Get left upper
        /// </summary>
        /// <returns></returns>
        const UiPoint GetLeftUpper() const;

        /// <summary>
        /// Set left upper
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        void SetLeftUpper(const UiPoint &value);

        /// <summary>
        /// Get right bottom
        /// </summary>
        /// <returns></returns>
        const UiPoint GetRightBottom() const;

        /// <summary>
        /// Set right bottom
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        void SetRightBottom(const UiPoint &value);

        /// <summary>
        /// Get center point
        /// </summary>
        /// <returns></returns>
        UiPoint GetCenter();

        /// <summary>
        /// Get hash code
        /// </summary>
        /// <returns></returns>
        const int GetHashCode() const;

        /// <summary>
        /// Equals
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        const bool Equals(const UiRectangle &obj) const;

    private:
        /// <summary>
        /// Left upper point
        /// </summary>
        UiPoint leftUpper;

        /// <summary>
        /// Right bottom point
        /// </summary>
        UiPoint rightBottom;
    };
}


#endif	//#ifndef __UIRECTANGLE__

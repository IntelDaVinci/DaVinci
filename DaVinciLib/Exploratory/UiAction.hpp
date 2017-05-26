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

#ifndef __UIACTION__
#define __UIACTION__

#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "opencv2/opencv.hpp"

#include "DaVinciDefs.hpp"
#include "DaVinciStatus.hpp"
#include "UiRectangle.hpp"
#include "AndroidTargetDevice.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// UiAction class
    /// </summary>
    class UiAction
    {
    public:

        /// <summary>
        /// Construct 
        /// </summary>
        /// <param name="inputString"></param>
        /// <param name="clickPosition"></param>
        /// <param name="originalFrame"></param>
        UiAction(const std::string &inputString, const UiRectangle &clickPosition, const cv::Mat &originalFrame, const boost::shared_ptr<TargetDevice> &device);


        const std::string &GetInputString() const;
        void SetInputString(const std::string &value);

        const UiRectangle &GetClickPosition() const;
        void SetClickPosition(const UiRectangle &value);

        const std::string &GetStringRepr() const;
        void SetStringRepr(const std::string &value);

        const cv::Mat &Getframe() const;
        void Setframe(const cv::Mat &value);

        // <summary>
        /// Action for pressing back
        /// </summary>
        void ActionForBack() const;

        /// <summary>
        /// Action for clicking
        /// </summary>
        void ActionForClickable() const;

        /// <summary>
        /// Action for general clicking
        /// </summary>
        void ActionForGeneralClickable() const;

        /// <summary>
        /// action for editing text
        /// </summary>
        void ActionForEditText() const;

        /// <summary>
        /// action for pressing home
        /// </summary>
        void ActionForHome() const;

        /// <summary>
        /// action for pressing menu
        /// </summary>
        void ActionForMenu() const;

        /// <summary>
        /// action for pressing swiping
        /// </summary>
        void ActionForSwipe() const;

        /// <summary>
        /// Override ToString
        /// </summary>
        /// <returns></returns>
        virtual std::string ToString();

    private:
        cv::Mat frame;
        std::string inputString;
        UiRectangle clickPosition;
        std::string stringRepr;
        boost::shared_ptr<TargetDevice> dut;
    };
}


#endif	//#ifndef __UIACTION__

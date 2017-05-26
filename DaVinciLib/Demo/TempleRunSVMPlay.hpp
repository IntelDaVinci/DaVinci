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

#ifndef __TEMPLE_RUN_SVM_PLAY_HPP__
#define __TEMPLE_RUN_SVM_PLAY_HPP__

#include "TempleRunPlayBase.hpp"

namespace DaVinci
{
    class TempleRunSVMPlay : public TempleRunPlayBase
    {
    public:
        TempleRunSVMPlay();
        ~TempleRunSVMPlay();

        explicit TempleRunSVMPlay(const string &iniFile);

        int winWidth;
        int winHeight;


        virtual bool Init() override;

        cv::Mat ProcessFrame(const cv::Mat &frame, const double timeStamp);

    private:
        enum class ACTIONS
        {
            JUMP,
            LEFT,
            RIGHT,
            STRAIGHT,
            DOWN,
            TILT_LEFT,
            TILT_RIGHT,
            ACTION_COUNT,
            RUN_AGAIN
        };

        double ROI_LEFT;
        double ROI_RIGHT;
        double ROI_TOP;
        double ROI_BOTTOM;
        int ROI_WIDTH;
        int ROI_HEIGHT;


        SVM svm;
        cv::Mat map;
        boost::shared_ptr<HOGDescriptor> hog;

        int ACTION_INTERVAL;
        int TILT_ACTION_INTERVAL;
    };
}

#endif
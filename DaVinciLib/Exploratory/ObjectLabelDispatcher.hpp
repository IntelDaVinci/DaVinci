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

#ifndef __OBJECTLABELDISPATCHER__
#define __OBJECTLABELDISPATCHER__

#include <vector>

#include "ObjectLabel.hpp"
#include "AndroidTargetDevice.hpp"

namespace DaVinci
{
    class ObjectLabelDispatcher
    {
    public:
        ObjectLabelDispatcher();
        ~ObjectLabelDispatcher();
        void SetDevice(boost::shared_ptr<TargetDevice> device);
        
        // Rotated point is on full rotated frame but recognition from focused window
        void DispatchObjectAction(ObjectLabel type, Point rotatedPoint);
        void SetRotatedFrameSize(Size size);
    private:
        boost::shared_ptr<TargetDevice> dut;
        boost::shared_ptr<AndroidTargetDevice> androidDut;
        Size rotatedFrameSize;
    };
}

#endif	//#ifndef __OBJECTLABELDISPATCHER__

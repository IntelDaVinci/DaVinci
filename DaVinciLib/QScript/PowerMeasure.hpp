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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __POWER_MEASURE_HPP__
#define __POWER_MEASURE_HPP__

#include "QScript.hpp"
#include "TargetDevice.hpp"

namespace DaVinci
{
    class PowerMeasure : public boost::enable_shared_from_this<PowerMeasure>
    {
    public:
        static boost::shared_ptr<PowerMeasure> CreateInstance(boost::shared_ptr<QScript> qs, boost::shared_ptr<TargetDevice> targetDev);

    public:
        explicit PowerMeasure(boost::shared_ptr<QScript> qs, boost::shared_ptr<TargetDevice> targetDev) {};
        virtual ~PowerMeasure() {}

        virtual DaVinciStatus Start() = 0;
        virtual DaVinciStatus Stop() = 0;
        virtual DaVinciStatus CollectResult(void *) = 0;
        virtual DaVinciStatus GenerateReport() = 0;
    };
}

#endif

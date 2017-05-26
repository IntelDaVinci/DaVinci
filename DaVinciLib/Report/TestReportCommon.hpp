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

#ifndef __TEST_REPORT_COMMON_HPP__
#define __TEST_REPORT_COMMON_HPP__

namespace DaVinci
{
    enum class DavinciLogType
    {
        /// <summary>
        /// invalid log type
        /// </summary>
        INVALID = -1,
        /// <summary>
        /// SmokeTest
        /// </summary>
        SmokeTest = 1,
        /// <summary>
        /// LaunchTime
        /// </summary>
        LaunchTime = 2,
        /// <summary>
        /// Calibration
        /// </summary>
        Calibration = 3,
        /// <summary>
        /// CalibrationComb
        /// </summary>
        CalibrationComb = 4,
        /// <summary>
        /// AI
        /// </summary>
        AI = 5,
        /// <summary>
        /// ImageBox
        /// </summary>
        ImageBox = 6,
        /// <summary>
        /// Wizard
        /// </summary>
        Wizard = 7,
        /// <summary>
        /// UnitTest
        /// </summary>
        UnitTest = 8,
        /// <summary>
        /// FPS test
        /// </summary>
        FPS = 9,
        /// <summary>
        /// FPS test
        /// </summary>
        RnR = 10,
    };
}

#endif
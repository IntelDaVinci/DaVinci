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

#ifndef __RUNNING_CHECKER_HPP__
#define __RUNNING_CHECKER_HPP__

#include "Checker.hpp"
#include "ScriptReplayer.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary> A base class for resultchecker. </summary>
    class RunningChecker : public Checker
    {
    public:
            explicit RunningChecker(const boost::weak_ptr<ScriptReplayer> obj);

            virtual ~RunningChecker();

            /// <summary>
            /// Worker thread loop
            /// </summary>
            virtual void WorkerThreadLoop() override;

            /// <summary>
            /// Check if situation need to skip check.
            /// </summary>
            /// <returns>True for skip checking. </returns>
            virtual bool SkipCheck(int opcode) override;

            /// <summary>
            /// Check if app running once
            /// </summary>
            /// <param name="packageName"> package name of running process.</param>
            /// <returns>result</returns>
            static bool IsAppRunning(const string &pkgName);

            /// <summary>
            /// print out application cpu and memory usage.
            /// </summary>
            /// <param name="packageName"> package name of running process.</param>
            static bool PrintAppTopInfo(const string &pkgName);

    private:
            boost::weak_ptr<ScriptReplayer> currentQScript;
    };
}

#endif
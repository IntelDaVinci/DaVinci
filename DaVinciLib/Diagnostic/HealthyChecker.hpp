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

#ifndef _H_HEALTHYCHECKER_
#define _H_HEALTHYCHECKER_

#include "Checker.hpp"
#include "DaVinciAPI.h"

namespace DaVinci
{
    using namespace std;

    enum class CommandType
    {
        WINCMD = 0,
        ADBCMD = 1,
    };

    enum class CommandResult
    {
        NOTRUN = 0,
        PASS = 1,
        FAIL = 2,
        TIMEOUT = 3,
    };

    struct CommandChecker
    {
        int number;
        CommandType type;
        CommandResult result;
        
        string cmdStr;
        string cmdArgs;
        string outputStr;
        string keyword;
    };

    class HealthyChecker: public Checker  
    {  
    public:  
        HealthyChecker(void);  

        ~HealthyChecker(void);  

        /// <summary>
        /// Worker thread loop
        /// </summary>
        virtual void WorkerThreadLoop() override;

        /// <summary>
        /// Load the host/device command lines from configure file
        /// </summary>
        bool LoadCheckList();

        /// <summary>
        /// Stop all running checklist
        /// </summary>
        bool StopAllTest();

        /// <summary>
        /// Start run the host checklist
        /// </summary>
        bool RunHostCheckList();

        /// <summary>
        /// Start run the host checklist
        /// </summary>
        bool RunDeviceCheckList();

    private:
        static const int WaitForRunCommand = 30000; //30S

        vector <CommandChecker> hostCheckList;
        vector <CommandChecker> deviceCheckList;

        vector <CommandChecker> currentCheckList;
        boost::mutex lockOfCheckList;

    };  
}
  
#endif  

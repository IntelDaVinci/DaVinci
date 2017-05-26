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

#ifndef __BUGTRIAGE__
#define __BUGTRIAGE__

#include <string>
#include <vector>
#include <unordered_map>
#include "boost/filesystem.hpp"
#include <boost/algorithm/string.hpp>

namespace DaVinci
{
    using namespace std;

    struct MiddlewareInfo
    {
        string middlewareName;
        std::unordered_map<string, bool> libsInfo;
    };

    class BugTriage
    {
    public:
        /// <summary>
        /// Construct function
        /// </summary>
        BugTriage();

        std::unordered_map<string, bool> TriageHoudiniBug(const std::string &apkPath, std::string &missingLibName);
        std::unordered_map<string, bool> TriageMiddlewareInfo(const std::string &apkPath);

    private:
        bool JudgeMissingX86lib(std::vector<string> x86Libs, std::vector<string> armV7aLibs, std::vector<string> armV5Libs, std::string &missingLibName);
        list<MiddlewareInfo> InitMiddlewareInfo();
    };
}

#endif    //#ifndef __BUGTRIAGE__

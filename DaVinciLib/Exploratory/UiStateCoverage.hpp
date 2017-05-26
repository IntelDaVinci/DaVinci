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

#ifndef __UISTATECOVERAGE__
#define __UISTATECOVERAGE__

#include <string>
#include <unordered_map>

namespace DaVinci
{
    /// <summary>
    /// Class UiStateCoverage to calculate of a UiState
    /// </summary>
    class UiStateCoverage
    {
    private:
        std::unordered_map<std::string, std::vector<std::string>> stateHashObjectsDict;
        std::unordered_map<std::string, int> stateHashObjNumDict;
        std::unordered_map<std::string, std::vector<std::string>> stateHashActDict;
        /// <summary>
        /// default constructor
        /// </summary>
    public:
        UiStateCoverage();

        /// <summary>
        /// constructor of UiStateCoverage with one parameter
        /// </summary>
        /// <param name="covFile"></param>
        UiStateCoverage(const std::string &covFile);
        /// <summary>
        /// parse global coverage file
        /// </summary>
        /// <param name="covFile"></param>
        void initHashDict(const std::string &covFile);

        /// <summary>
        /// calc coverage of object
        /// </summary>
        /// <returns></returns>
        double coverObjectCalc();

        /// <summary>
        /// calc the coverage of action
        /// </summary>
        /// <returns></returns>
        double coverActionCalc();
        /// <summary>
        /// show dict contents
        /// </summary>
        void showStateObjecNumDict();

        /// <summary>
        /// show...
        /// </summary>
        void showStateObjectDict();

        /// <summary>
        /// show the dictionary of action!
        /// </summary>
        void showStateActionDict();

    };
}


#endif	//#ifndef __UISTATECOVERAGE__

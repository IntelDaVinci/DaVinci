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

#ifndef __BIASFILEREADER__
#define __BIASFILEREADER__

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "BiasObject.hpp"
#include "boost/core/null_deleter.hpp"

namespace DaVinci
{
    using namespace std;

    /// <summary>
    /// Class BiasFileReader
    /// </summary>
    class BiasFileReader
    {
        /// <summary>
        /// Construct
        /// </summary>
    public:
        BiasFileReader();

        void LoadSeed(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadTimeout(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadLaunchWaitTime(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadActionWaitTime(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadRandomAfterUnmatchAction(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadMaxRandomAction(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadUnmatchRatio(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadMatchPattern(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadRegion(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadTest(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadTestBasic(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadTestLog(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild);

        void LoadTestLaunch(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild);

        void LoadTestAccount(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue);

        void LoadTestClick(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue);

        void LoadTestAudio(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue);

        void LoadTestSwipe(boost::shared_ptr<BiasInfo> biasInfo, string& testChildValue);

        void LoadTestText(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue);

        void LoadTextUrl(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble);

        void LoadTextMail(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble);

        void LoadTextPlain(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble);

        void LoadTestKeyword(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild);

        void LoadScripts(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadCheckers(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadEvents(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadVideoRecording(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadAudioSaving(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        void LoadActionPossibility(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child);

        /// <summary>
        /// Load DaVinci configure
        /// </summary>
        /// <param name="xmlPath"></param>
        /// <returns></returns>
        boost::shared_ptr<BiasObject> loadDavinciConfig(const std::string &xmlPath);
    };
}


#endif	//#ifndef __BIASFILEREADER__

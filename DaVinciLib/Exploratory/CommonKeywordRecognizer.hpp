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

#ifndef __COMMONKEYWORDRECOGNIZER__
#define __COMMONKEYWORDRECOGNIZER__

#include "KeywordRecognizer.hpp"

namespace DaVinci
{
    class CommonKeywordRecognizer
    {
    private:
        boost::shared_ptr<KeywordRecognizer> keywordRecog;

        std::vector<Keyword> enWhiteList;
        std::vector<Keyword> enNegativePriorList;
        std::vector<Keyword> enBlackList;
        std::vector<Keyword> zhWhiteList;
        std::vector<Keyword> zhNegativePriorList;
        std::vector<Keyword> zhBlackList;

        std::vector<Keyword> keywords;

    public:
        CommonKeywordRecognizer();

        std::vector<Keyword> &GetKeywords();


        void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList);

        bool RecognizeCommonKeyword(std::vector<Keyword> allKeywords, const std::string &languageMode);
    };
}

#endif
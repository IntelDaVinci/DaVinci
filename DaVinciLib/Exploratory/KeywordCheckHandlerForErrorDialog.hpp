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

#ifndef _ERRORDIALOGERECOGNIZER_
#define _ERRORDIALOGERECOGNIZER_

#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/enable_shared_from_this.hpp"
#include "KeywordCheckHandler.hpp"

namespace DaVinci
{
    class KeywordCheckHandlerForErrorDialog : public KeywordCheckHandler
    {
    public:
        KeywordCheckHandlerForErrorDialog(boost::shared_ptr<KeywordRecognizer> keywordRecog);

        virtual void InitializeKeywordList(std::map<std::string, std::vector<Keyword>> &keywordList) override;

        virtual bool HandleRequestImpl(std::string &language, std::vector<Keyword> &enTexts, std::vector<Keyword> &zhTexts) override;

        virtual FailureType GetCurrentFailureType() override;

        virtual cv::Rect GetOkRect(const cv::Mat &frame) override;

    private:
        std::vector<Keyword> enBlackList;
        std::vector<Keyword> zhBlackList;
    };
}

#endif
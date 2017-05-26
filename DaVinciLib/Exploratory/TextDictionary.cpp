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

#include "TextDictionary.hpp"

#include <regex>

namespace DaVinci
{
    TextDictionary::TextDictionary()
    {
        std::vector<std::string> values;
        // Map "se.*?ngs" to "settings"
        values.push_back("settings");
        this->textDictionary["(^se.*?ngs$)"] = values;

        // Map "sign in mm face.*?k" to "sign in with facebook"
        values.clear();
        values.push_back("sign in with facebook");
        this->textDictionary["(^sign in mm face.*?k$)"] = values;

        // Map "o(f|r)(f|r)" or "mr" to "orr"
        values.clear();
        values.push_back("orr");
        this->textDictionary["(^o(f|r)(f|r)$)|(mr)"] = values;
    }

    std::vector<std::string> TextDictionary::LookUpDictionary(const std::string &key)
    {
        std::vector<std::string> values;

        for (std::unordered_map<std::string, std::vector<std::string>>::iterator it = this->textDictionary.begin(); it != this->textDictionary.end(); it++)
        {
            std::tr1::regex regex(it->first.c_str(), std::tr1::regex_constants::icase);
            std::tr1::cmatch cmatch;

            if (std::tr1::regex_search(key.c_str(), cmatch, regex))
            {
                values = it->second;
                break;
            }
        }

        return values;
    }
}
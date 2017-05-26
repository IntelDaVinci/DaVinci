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

#include "OCRCorrector.hpp"
#include <omp.h>

namespace DaVinci
{
    using namespace std;

    void OCRCorrector::SplitString(const string& str,vector<vector<string> >& sptStr)
    {
        sptStr.resize(str.length());

        for(size_t i_size=0 ; i_size < sptStr.size(); ++i_size)
        {
            sptStr[i_size].resize(2);
        }
        for(size_t i_spl  =0; i_spl < str.length(); ++i_spl)
        {
            sptStr[i_spl][0] = str.substr(0,i_spl);
            sptStr[i_spl][1] = str.substr(i_spl);
        }

    }
    void OCRCorrector::DeleteString(const vector<vector<string>>& splitStr,vector<string>& delStr)
    {
        for(size_t i_del = 0; i_del < splitStr.size(); ++i_del)
        {			
            delStr.push_back(splitStr[i_del][0]+splitStr[i_del][1].substr(1));			
        }
    }

    void OCRCorrector::ReplaceString(const vector<vector<string>>& splitStr,vector<string>& repStr)
    {
        for(size_t i_rep = 0 ; i_rep < splitStr.size(); ++i_rep)
        {		
            for(char j='a'; j<='z'; j++)
                repStr.push_back(splitStr[i_rep][0]+j+splitStr[i_rep][1].substr(1));					
        }
    }

    void OCRCorrector::InsertString(const vector<vector<string>>& splitStr,vector<string>& insStr)
    {
        char j;
        if(splitStr.size() == 0)
            return;
        for(size_t i_ins = 0; i_ins < splitStr.size(); ++i_ins)
        {			
            for(j='a'; j<='z'; j++)
                insStr.push_back(splitStr[i_ins][0]+j+splitStr[i_ins][1]);			
        }
        for(j='a';j<='z';j++)
            insStr.push_back(splitStr[0][0]+splitStr[0][1]+j);
    }

    void OCRCorrector::TransposeString(const vector<vector<string>>& splitStr,vector<string>& tranStr)
    {
        for(size_t i_tran = 0; i_tran < splitStr.size()-1; ++i_tran)
        {			
            tranStr.push_back(splitStr[i_tran][0]+splitStr[i_tran][1][1]+splitStr[i_tran][1][0]+splitStr[i_tran][1].substr(2));			
        }
    }

    void OCRCorrector::EditTwice(vector<string>& editonce_Str, vector<string>& edit_Str)
    {
#pragma omp parallel sections
        for(size_t temp=0; temp < editonce_Str.size(); temp++)
            EditOnce(editonce_Str[temp],edit_Str );
    }

    void OCRCorrector::EditOnce(const string& str, vector<string>& edit_Str)
    {
        vector<vector<string> > sptStr;

        SplitString(str, sptStr);
#pragma omp parallel sections
        {
#pragma omp section
            TransposeString(sptStr, edit_Str);
#pragma omp section
            DeleteString(sptStr, edit_Str);
#pragma omp section
            InsertString(sptStr, edit_Str);
#pragma omp section
            ReplaceString(sptStr, edit_Str);
        }
    }
    string  OCRCorrector::OcrCorrector(string str, OCRDictionary & Dic)
    {
        string result;
        vector<string> strVec1;
        vector<string> strVec2;
        OCRCorrector ct;

        if(str.length() <= 1 || Dic.DicFind(str) == true)          
            result = str;
        else
        {
            ct.EditOnce(str, strVec1);
            ct.EditTwice(strVec1, strVec2);

            result = Dic.DicFind(strVec1, strVec2);

            strVec1.clear();
            strVec2.clear();

        }
        return result;
    }
}
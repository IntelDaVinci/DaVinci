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

#include "OCRDictionary.hpp"
#include "TestManager.hpp"

namespace DaVinci
{
	using namespace std;
	void OCRDictionary::DicCreate(const string &dir)
	{
		string filename;  
		string line;  
		string word;
		int count;

		ifstream in(dir);
		if(in) 
		{  
			while (!in.eof()) 
			{   
				in>>word>>count;
				mymap[word]= count;
			}
		}  
		else  
			DAVINCI_LOG_ERROR <<"Your Dictioary is not found!" << endl;
	}

	bool OCRDictionary::DicFind(string str)
	{
		if(mymap.find(str) != mymap.end())
			return true;
		else
			return false;
	}

	string OCRDictionary::DicFind(vector<string>& edit_Str, vector<string>& edit2_Str)
	{
		string str = "Test";
		int maxnum = 0;
		int maxnum2 = 0;


		for(size_t i=0; i<edit_Str.size(); i++)
			if(mymap.find(edit_Str[i]) != mymap.end())
				if((*(mymap.find(edit_Str[i]))).second> maxnum)
				{
					maxnum = (*(mymap.find(edit_Str[i]))).second;
					str = edit_Str[i];
				}

				if(str != "Test")
					return str;
				else
				{
					for(size_t j=0; j<edit2_Str.size(); j++)
						if(mymap.find(edit2_Str[j]) != mymap.end())
							if((*(mymap.find(edit2_Str[j]))).second > maxnum2)
							{
								maxnum2 = (*(mymap.find(edit2_Str[j]))).second;
								str = edit2_Str[j];
							}

							if(str != "Test")
								return str;
							else
								return "Not found in the Dictionary!";
				}
	}
}
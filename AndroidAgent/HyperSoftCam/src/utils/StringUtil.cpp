/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "utils/Logger.h"
#include "utils/StringUtil.h"

namespace android {

Vector<String8> StringUtil::split(const String8& str, char delimiter)
{
    Vector<String8> tokens;
    unsigned int lastTokenEnd = 0;
    for (unsigned int i = 0; i < str.length(); i++) {
        if (str[i] == delimiter) {
            if ((i - lastTokenEnd) > 0) {
                tokens.push_back(substr(str, lastTokenEnd, i - lastTokenEnd));
            }
            lastTokenEnd = i + 1; // 1 for skipping delimiter
        }
    }
    if (lastTokenEnd < str.length()) {
        tokens.push_back(substr(str, lastTokenEnd, str.length() - lastTokenEnd));
    }
    return tokens;
}

String8 StringUtil::substr(const String8& str, size_t pos, size_t n)
{
    size_t l = str.length();

    if (pos >= l) {
        String8 resultDummy;
        return resultDummy;
    }
    if ((pos + n) > l) {
        n = l - pos;
    }
    String8 result(str.string() + pos, n);
    return result;
}

int StringUtil::compare(const String8& str, const char* other)
{
    return strcmp(str.string(), other);
}

bool StringUtil::endsWith(const String8& str, const char* other)
{
    size_t l1 = str.length();
    size_t l2 = strlen(other);
    const char* data = str.string();
    if (l2 > l1) {
        return false;
    }
    size_t iStr = l1 - l2;
    size_t iOther = 0;
    for(; iStr < l1; iStr++) {
        if (data[iStr] != other[iOther]) {
            return false;
        }
        iOther++;
    }
    return true;
}

bool StringUtil::startsWith(const String8& str, const char* other)
{
    size_t l1 = str.length();
    size_t l2 = strlen(other);
    const char* data = str.string();
    if (l2 > l1) {
        return false;
    }
    size_t iStr = 0;
    size_t iOther = 0;
    for(; iStr < l2; iStr++) {
        if (data[iStr] != other[iOther]) {
            return false;
        }
        iOther++;
    }
    return true;
}

String8 StringUtil::trimLeft(const String8& str)
{
    size_t begin = 0;
	size_t l = str.length();
    while (begin < l && isspace(str[begin]))
        begin++;

    size_t size = str.length() - begin;
    String8 sub_str = substr(str, begin, size);
    return sub_str;
}

int StringUtil::strToInt(const String8& str)
{
	if(str.length() > 0)
	{
	    if(str[0] == '-')
	    {
	        String8 num_str = substr(str, 1, str.length() - 1);
	        return (0 - atoi(num_str));
	    }
	    else
	    {
    	    return atoi(str);
	    }
	}
	else
	{
		return 0;
	}
}

}

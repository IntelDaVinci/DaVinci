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

#ifndef __FILE_COMMON_HPP__
#define __FILE_COMMON_HPP__

#include "boost/filesystem.hpp"

namespace DaVinci
{
    using namespace std;

    inline void getFiles(boost::filesystem::path directory, vector<boost::filesystem::path> &paths)
    {
        boost::filesystem::directory_iterator end_iter;

        if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
        {
            for(boost::filesystem::directory_iterator dir_iter(directory); dir_iter != end_iter; ++dir_iter)
            {
                paths.push_back(*dir_iter);
            }
        }
    }

    void getAllFiles(boost::filesystem::path directory, vector<boost::filesystem::path> &paths);

    inline void getDirectory(boost::filesystem::path directory, vector<boost::filesystem::path> &paths)
    {
        boost::filesystem::directory_iterator end_iter;

        if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
        {
            for(boost::filesystem::directory_iterator dir_iter(directory); dir_iter != end_iter; ++dir_iter)
            {
                if (boost::filesystem::is_directory(dir_iter->status()))
                {
                    getAllFiles(*dir_iter, paths);
                }
            }
        }
    }

    inline void getAllFiles(boost::filesystem::path directory, vector<boost::filesystem::path> &paths)
    {
        getFiles(directory, paths);
        getDirectory(directory, paths);
    }

}

#endif
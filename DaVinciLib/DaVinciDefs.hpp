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

#ifndef __DAVINCI_DEFS_HPP__
#define __DAVINCI_DEFS_HPP__
#include "DaVinciDefs.h"

// Common type definitions

#include "boost/log/trivial.hpp"

#define DAVINCI_LOG(sev) BOOST_LOG_TRIVIAL(sev)
#define DAVINCI_LOG_TRACE DAVINCI_LOG(trace)
#define DAVINCI_LOG_DEBUG DAVINCI_LOG(debug)
#define DAVINCI_LOG_INFO DAVINCI_LOG(info)
#define DAVINCI_LOG_WARNING DAVINCI_LOG(warning)
#define DAVINCI_LOG_ERROR DAVINCI_LOG(error)
#define DAVINCI_LOG_FATAL DAVINCI_LOG(fatal)

template<class T>
class std::hash<boost::shared_ptr<T>>
{
public:
    size_t operator()(const boost::shared_ptr<T>& key) const
    {
        return (size_t)key.get();
    }
};

#endif
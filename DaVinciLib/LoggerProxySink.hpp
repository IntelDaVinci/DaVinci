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

#ifndef __LOGGER_PROXY_SINK_HPP__
#define __LOGGER_PROXY_SINK_HPP__

#include "boost/function.hpp"
#include "boost/log/sinks/basic_sink_backend.hpp"

namespace DaVinci
{
    class LoggerProxySink : public boost::log::sinks::basic_formatted_sink_backend<
        char, boost::log::sinks::synchronized_feeding>
    {
    public:
        typedef boost::function<void (const string_type &msg)> LoggerProxySinkCallback;

        LoggerProxySink();

        void SetCallback(LoggerProxySinkCallback cb);
        void consume(boost::log::record_view const& rec, string_type const& logMessage);

    private:
        LoggerProxySinkCallback callback;
    };

}

#endif
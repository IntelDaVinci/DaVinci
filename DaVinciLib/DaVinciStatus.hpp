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

#ifndef __DAVINCI_STATUS_HPP__
#define __DAVINCI_STATUS_HPP__

#include <system_error>
#include <boost/system/error_code.hpp>

namespace DaVinci
{
    using namespace std;

    const int DaVinciStatusSuccess = 0;

    /// <summary> DaVinci status code combining generic error_code and DaVinci specific code </summary>
    class DaVinciStatus
    {
    public:
        DaVinciStatus(int value = 0)
            : code(value, generic_category())
        {
        }
        virtual ~DaVinciStatus()
        {
        }
        DaVinciStatus(const error_code &anotherCode)
            : code(anotherCode.default_error_condition().value(), generic_category())
        {
        }
        DaVinciStatus(const boost::system::error_code &boostCode)
            : code(boostCode.default_error_condition().value(), generic_category())
        {
        }
        bool operator==(const DaVinciStatus &status) const
        {
            return status.code == code;
        }
        bool operator!=(const DaVinciStatus &status) const
        {
            return status.code != code;
        }
        bool operator==(int status) const
        {
            return code.value() == status;
        }
        bool operator!=(int status) const
        {
            return code.value() != status;
        }
        int value() const
        {
            return code.value();
        }
        string message() const
        {
            if (code.value() == DaVinciStatusSuccess)
            {
                // DaVinciStatusSuccess(0) will be dumped as "unknown error" by default, so we handle
                // this special case here.
                return "success";
            }
            else
            {
                return code.message();
            }
        }
    private:
        error_code code;

        friend class DaVinciError;
    };

    class DaVinciError : public system_error
    {
    public:
        DaVinciError(const DaVinciStatus &status) : system_error(status.code)
        {
        }
        DaVinciError(const DaVinciStatus &status, const char *what) : system_error(status.code, what)
        {
        }
    };

    inline bool DaVinciSuccess(const DaVinciStatus &status)
    {
        return status == 0;
    }
}

#endif
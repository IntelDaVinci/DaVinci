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

#include "CaptureDevice.hpp"

namespace DaVinci
{
    DaVinciStatus CaptureDevice::Start()
    {
        boost::lock_guard<boost::mutex> lock(startStopMutex);
        if (grabThread != nullptr)
        {
            return DaVinciStatus(errc::operation_not_permitted);
        }
        shouldGrabThreadStop = false;
        grabThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CaptureDevice::GrabThreadEntry, this)));
        SetThreadName(grabThread->get_id(), "Grab frame");

        isStarted = true;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus CaptureDevice::SetStopFlag()
    {
        if (shouldGrabThreadStop == false)
        {
            shouldGrabThreadStop = true;
            return DaVinciStatusSuccess;
        }
        return DaVinciStatus(errc::operation_canceled);
    }

    DaVinciStatus CaptureDevice::Join()
    {
        boost::lock_guard<boost::mutex> lock(startStopMutex);
        if (grabThread == nullptr)
        {
            return DaVinciStatus(errc::operation_not_permitted);
        }
        grabThread->join();
        grabThread = nullptr;
        return DaVinciStatusSuccess;
    }

    DaVinciStatus CaptureDevice::Stop()
    {
        DaVinciStatus status = SetStopFlag();
        if (status != DaVinciStatusSuccess)
            return status;
        status = Join();
        isStarted = false;
        return status;
    }

    void CaptureDevice::GrabThreadEntry(void)
    {
        while (!shouldGrabThreadStop)
        {
            if (IsFrameAvailable() && !frameHandlerCallback.empty())
            {
                frameHandlerCallback(shared_from_this());
            }
            else
            {
                //CpuPause();
            }
        }
    }
}
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

#ifndef __GLOBAL_DEVICE_STATUS_HPP__
#define __GLOBAL_DEVICE_STATUS_HPP__

#include "DaVinciCommon.hpp"
#include "boost/interprocess/sync/named_mutex.hpp"
#include "boost/interprocess/managed_shared_memory.hpp"

#define MAX_DEVICE_NUMBER       256
#define MAX_DEVICE_NAME         256

namespace DaVinci
{
    typedef enum DeviceCategory
    {
        Target,
        Capture,
        Accessory,
        Unsupported
    } DeviceCategory;

    /// <summary> A singleton Manager for devices. </summary>
    class GlobalDeviceStatus
    {
        typedef struct DeviceConnectedStatus
        {
            int id;
            DeviceCategory category;
            char name[MAX_DEVICE_NAME];
            bool connected;
            unsigned long processid;
        } DeviceConnectedStatus;

        typedef struct AllDeviceConnectedStatus
        {
            DeviceConnectedStatus device[MAX_DEVICE_NUMBER];
        } AllDeviceConnectedStatus;

    public:
        /// <summary> Default constructor. </summary>
        GlobalDeviceStatus();

        /// <summary> Default constructor. </summary>
        ~GlobalDeviceStatus();

        /// <summary>
        /// Initialize device's global resource. 
        /// </summary>
        ///
        /// <returns> True for succeed. </returns>
        bool IsInitialized(); 

        /// <summary>
        /// Get the device's connect status. 
        /// </summary>
        ///
        /// <returns> True for connect, False for idle. </returns>
        bool IsConnectedByOthers(DeviceCategory category, const string &name);

        /// <summary>
        /// If the device occupied by others, return false directly.
        /// If the device not occupied by others, update the device's status and return true. 
        /// </summary>
        ///
        /// <returns> True for succeed. </returns>
        bool CheckAndUpdateStatus(DeviceCategory category, const string &name, bool connect);

    private:
        static const string GlobalDeviceSharedMemoryName;
        static const string GlobalDeviceMutexName;

        bool Initialized;

        boost::shared_ptr<boost::interprocess::shared_memory_object> sharedMemoryObject;
        boost::shared_ptr<boost::interprocess::mapped_region> sharedMemoryRegion;

        AllDeviceConnectedStatus * allDeviceStatus;
        
        void ResetList();
        void RemoveZombie();

        bool UpdateStatus(DeviceCategory category, const string &name, bool connect);
        bool IsDeviceBusy(DeviceCategory category, const string &name);

        int GetDeviceId(DeviceCategory category, const string &name);
        int GetFreeId();

        bool IsProcessAlive(unsigned int processid);

        bool IsSharedMemoryExisting(const string &sharedMemoryName);

        void PrintList();
    };
}

#endif

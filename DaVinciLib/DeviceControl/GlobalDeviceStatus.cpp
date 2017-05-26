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

#include "GlobalDeviceStatus.hpp"

#include <windows.h> 
#include <tlhelp32.h> 

namespace DaVinci
{
    using namespace boost::interprocess;

    const string GlobalDeviceStatus::GlobalDeviceSharedMemoryName = "DaVinciGlobalDeviceSharedMemory";
    const string GlobalDeviceStatus::GlobalDeviceMutexName = "DaVinciGlobalDeviceMutex";

    GlobalDeviceStatus::GlobalDeviceStatus() : sharedMemoryObject(nullptr), 
        allDeviceStatus(nullptr), sharedMemoryRegion(nullptr), Initialized(false)
    {
        bool sharedMemoryExists = false;
        Initialized = false;

        try{
            GlobalNamedMutex global_mutex(GlobalDeviceMutexName.c_str());

            //Create a shared memory object.
            if (IsSharedMemoryExisting(GlobalDeviceSharedMemoryName))
                sharedMemoryExists = true;
            else
                sharedMemoryExists = false;

            sharedMemoryObject = boost::shared_ptr<boost::interprocess::shared_memory_object>(new shared_memory_object(open_or_create, GlobalDeviceSharedMemoryName.c_str(), read_write));
            assert(sharedMemoryObject != nullptr);

            //Set shared memory size
            sharedMemoryObject->truncate(sizeof(AllDeviceConnectedStatus));
            //Map the whole shared memory in this process
            sharedMemoryRegion =  boost::shared_ptr<boost::interprocess::mapped_region>(new mapped_region(*sharedMemoryObject, read_write));
            assert(sharedMemoryRegion != nullptr);

            //Construct the shared structure in memory
            allDeviceStatus = (AllDeviceConnectedStatus *)(sharedMemoryRegion->get_address());
            assert(allDeviceStatus != nullptr);

            // Reset all items in the device list when the first DaVinci startup.
            if (!sharedMemoryExists)
                ResetList();
            else
                RemoveZombie();

            PrintList();
            Initialized = true;
        }

        catch(...){
            DAVINCI_LOG_ERROR << "Initialize failed to get shared memory information.";
            Initialized = false;
        }
    }
   
    GlobalDeviceStatus::~GlobalDeviceStatus() 
    {
       Initialized = false;
    }

    bool GlobalDeviceStatus::IsInitialized() 
    {
       return Initialized;
    }

    /// <summary>
    /// Get the device's connect status. 
    /// </summary>
    ///
    /// <returns> True for connect, False for idle. </returns>
    bool GlobalDeviceStatus::IsConnectedByOthers(DeviceCategory category, const string &name)
    {
        bool result = false;

        if (!Initialized)
            return result;

        GlobalNamedMutex global_mutex(GlobalDeviceMutexName.c_str());

        result = IsDeviceBusy(category, name);

        return result;
    }
    
    /// <summary>
    /// If the device occupied by others, return false directly.
    /// If the device not occupied by others, update the device's status and return true. 
    /// </summary>
    ///
    /// <returns> True for succeed. </returns>
    bool GlobalDeviceStatus::CheckAndUpdateStatus(DeviceCategory category, const string &name, bool connect)
    {
        bool result = false;

        if (!Initialized)
            return result;

        GlobalNamedMutex global_mutex(GlobalDeviceMutexName.c_str());

        if (IsDeviceBusy(category, name))
            result = false;
        else
            result = UpdateStatus(category, name, connect);

        return result;
    }

    bool GlobalDeviceStatus::IsDeviceBusy(DeviceCategory category, const string &name)
    {
        bool result = false;

        int id = GetDeviceId(category, name);

        if (id < MAX_DEVICE_NUMBER)
        {
            if ((allDeviceStatus->device[id].connected) && \
                (allDeviceStatus->device[id].processid != GetCurrentProcessId()))
                result = true;
        }
        else
            DAVINCI_LOG_DEBUG << "IsDeviceBusy, the device:" << name << " not in device list.";

        return result;
    }

    bool GlobalDeviceStatus::UpdateStatus(DeviceCategory category, const string &name,  bool connect)
    {
        bool result = false;

        if ((category == DeviceCategory::Unsupported) || (name.empty()))
        {
            DAVINCI_LOG_WARNING << "UpdateStatus, the name is empty!";
            return result;
        }

        int id = GetDeviceId(category, name);

        if ((name.size() < MAX_DEVICE_NAME) && (id < MAX_DEVICE_NUMBER))
        {
            allDeviceStatus->device[id].id = id;
            allDeviceStatus->device[id].connected = connect;
            allDeviceStatus->device[id].category = category;
    
            if (connect)
                allDeviceStatus->device[id].processid = GetCurrentProcessId();
            else
                allDeviceStatus->device[id].processid = 0;
                
            memset(allDeviceStatus->device[id].name, 0, MAX_DEVICE_NAME);
            strncpy_s(allDeviceStatus->device[id].name, name.c_str(), name.size());

            result = true;
            // DAVINCI_LOG_DEBUG << "UpdateStatus, Name:" << name << ", Status:" << connect;
        }
        else
            DAVINCI_LOG_WARNING << "UpdateStatus, InValid Name:" << name;

        return result;
    }

    void GlobalDeviceStatus::ResetList()
    {
        for (int id = 0; id < MAX_DEVICE_NUMBER; id ++)
        {
            allDeviceStatus->device[id].id = id;
            allDeviceStatus->device[id].connected = false;
            allDeviceStatus->device[id].category = DeviceCategory::Unsupported;
            allDeviceStatus->device[id].processid = 0;
                
            memset(allDeviceStatus->device[id].name, 0, MAX_DEVICE_NAME);
        }
    }

    void GlobalDeviceStatus::RemoveZombie()
    {
        for (int id = 0; id < MAX_DEVICE_NUMBER; id ++)
        {
            if (allDeviceStatus->device[id].connected)
            {
                if (!IsProcessAlive(allDeviceStatus->device[id].processid))
                {
                    allDeviceStatus->device[id].connected = false;
                    allDeviceStatus->device[id].processid = 0;
                }
            }
        }
    }

    void GlobalDeviceStatus::PrintList()
    {
        DAVINCI_LOG_INFO << "------------------------------------------------------------------";
        DAVINCI_LOG_INFO << "Global Devices Status:";
        DAVINCI_LOG_INFO << "Current PID: " << GetCurrentProcessId();
            
        for (int i = 0; i < MAX_DEVICE_NUMBER; i ++)
        {
            if (allDeviceStatus->device[i].category != DeviceCategory::Unsupported)
            {
                DAVINCI_LOG_INFO << "No.       :" << allDeviceStatus->device[i].id;
                DAVINCI_LOG_INFO << "Name      :" << allDeviceStatus->device[i].name;
                DAVINCI_LOG_INFO << "Category  :" << allDeviceStatus->device[i].category;
                DAVINCI_LOG_INFO << "Connected :" << allDeviceStatus->device[i].connected;
                DAVINCI_LOG_INFO << "PID       :" << allDeviceStatus->device[i].processid <<"\n";
            }
        }
        DAVINCI_LOG_INFO << "------------------------------------------------------------------";
    }

    int GlobalDeviceStatus::GetDeviceId(DeviceCategory category, const string &name)
    {
        int i = MAX_DEVICE_NUMBER;
        
        if (name.empty())
            return i;

        for (i = 0; i < MAX_DEVICE_NUMBER; i ++)
        {
            if (allDeviceStatus->device[i].category == category)
            {
                if ((string(allDeviceStatus->device[i].name).length() > 0) &&\
                    (name == string(allDeviceStatus->device[i].name)))
                {
                    DAVINCI_LOG_DEBUG << "GetDeviceId:" << i << ", Name:" << name;
                    break;
                }
            }
        }

        if (i >= MAX_DEVICE_NUMBER)
        {
            i = GetFreeId();
            DAVINCI_LOG_DEBUG << "GetDeviceId, the device:" << name << " not in device list, need to add it.";
        }

        return i;
    }

    int GlobalDeviceStatus::GetFreeId()
    {
        int i = 0;

        for (i = 0; i < MAX_DEVICE_NUMBER; i ++)
        {
            if (allDeviceStatus->device[i].category == DeviceCategory::Unsupported)
            {
                DAVINCI_LOG_DEBUG << "GetFreeId:" << i;
                break;
            }
        }
        
        // If there is no vacant item, so overwrite an idle device item.
        if (i >= MAX_DEVICE_NUMBER)
        {
            for (i = 0; i < MAX_DEVICE_NUMBER; i ++)
            {
                if (allDeviceStatus->device[i].connected == false)
                {
                    DAVINCI_LOG_DEBUG << "GetFreeId:" << i;
                    break;
                }
            }
        }

        return i;
    }

    bool GlobalDeviceStatus::IsProcessAlive(unsigned int processid)
    {
        bool isAlive = false;
        PROCESSENTRY32 entry; 
        HANDLE handle = INVALID_HANDLE_VALUE;

        try
        {
            handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 

            entry.dwSize = sizeof(entry); 
            Process32First(handle, &entry); 

            for (unsigned int i = 0; i < entry.dwSize; i++)
            { 
                Process32Next(handle, &entry);
                boost::process::process p(entry.th32ProcessID); 

                if (p.get_id() == processid)
                {
                    isAlive = true;
                    DAVINCI_LOG_DEBUG << "IsProcessAlive, the process is alvie, PID: " << p.get_id();
                    break;
                }
            } 
        }
        catch (...)
        {
            DAVINCI_LOG_WARNING << "IsProcessAlive, Check interrupted by exception!";
        }
        CloseHandle(handle); 

        return isAlive;
    }

    bool GlobalDeviceStatus::IsSharedMemoryExisting(const string &sharedMemoryName)
    {
        bool result = false;
        try
        {
            shared_memory_object segment(open_only, sharedMemoryName.c_str(), read_write);
            result = true;
        } 
        catch (const std::exception &ex) {
            std::cout << "Shared Memory Not Exists: "  << ex.what();
            result = false;
        }
        return result;
    }
}

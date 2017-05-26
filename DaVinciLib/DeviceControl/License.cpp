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

#include "License.hpp"
#include "TestManager.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"

namespace DaVinci
{
    using namespace boost::gregorian;

    const string License::DefaultLicFile = "DaVinci.lic";
    const string License::PrivateKey = "!23$Qwsx";
    const string License::CpuAbiPropStr = "ro.product.cpu.abi";
    const string License::ManufacturerPropStr = "ro.product.manufacturer";
    const string License::ModelPropStr = "ro.product.model";
    const string License::SdkPropStr = "ro.build.version.sdk";
    
    const string License::CpuAbiHead = "CPUABI=";
    const string License::ManufacturerHead = "MANUFACTURER=";
    const string License::ModelHead = "MODEL=";
    const string License::MinSdkHead = "MINSDK=";
    const string License::ExpireHead = "EXPIRE=";
    const string License::SignHead = "SIGN=";

    License::License():Initialized(false)
    {
        string licFile = DefaultLicFile;

        if (!TestManager::Instance().GetDaVinciHome().empty())
            licFile = TestManager::Instance().GetDaVinciHome() + std::string("\\") + DefaultLicFile;

        ReadLicenseFile(licFile);
    }
   
    License::~License() 
    {
        ClearWhiteList();
    }

    void License::ClearWhiteList()
    {
        whiteList.cpuapi.clear();
        whiteList.manufacturer.clear();
        whiteList.model.clear();
        whiteList.minsdk = "";
        whiteList.expire = "";
        whiteList.sign = "";
    }

    bool License::IsValidDevice(const string &deviceName)
    {
        string sdk, cpuabi, factory, model;
        int devSdk = MinSDK, minSdk = MinSDK;

        if ((!Initialized) || (deviceName.empty()))
        {
            DAVINCI_LOG_WARNING << "Invalid DaVinci.lic or invalid devicename:" << deviceName;
            return false;
        }

        GetDeviceSysprop(deviceName, SdkPropStr, sdk);
        GetDeviceSysprop(deviceName, CpuAbiPropStr, cpuabi);
        GetDeviceSysprop(deviceName, ManufacturerPropStr, factory);
        GetDeviceSysprop(deviceName, ModelPropStr, model);

        if ((!TryParse(sdk, devSdk)) \
            || (!TryParse(whiteList.minsdk, minSdk)) \
            || (devSdk < minSdk))
        {
            DAVINCI_LOG_ERROR <<"Detected unsupported target device: "<< deviceName << ", SDK version: " << devSdk << ", less than MinSDKVerion: " << minSdk;
            return false;
        }

        // By default, support all x86 devices, no expire.
        if (cpuabi == "x86") 
            return true;

        if (IsExpired(whiteList.expire))
        {
            DAVINCI_LOG_ERROR <<"Your license has expired on: " << whiteList.expire;
            return false;
        }

        if (IsDataInVector(cpuabi, whiteList.cpuapi) \
            && IsDataInVector(factory, whiteList.manufacturer) \
            && IsDataInVector(model, whiteList.model))
            return true;

        return false;
    }

    bool License::IsDataInVector(const string &data, const vector<string> &dataVector)
    {
        bool status = false;

        if ((data.empty()) || (dataVector.size() == 0))
        {
            DAVINCI_LOG_ERROR << "Invalid device, data: " << data;
            return status;
        }

        string dataLowercase = boost::algorithm::to_lower_copy(data);

        for (unsigned int i = 0; i < dataVector.size(); i ++)
        {
            if ((dataVector[i] == "all") || (dataVector[i] == dataLowercase))
            {
                status = true;
                break;
            }
        }

        return status;
    }

    DaVinciStatus License::ReadLicenseFile(const string &fileName)
    {
        vector<string> allLines;
        DaVinciStatus status = errc::file_exists;

        ClearWhiteList();
        Initialized = false;

        if ((!DaVinciSuccess(ReadAllLines(fileName, allLines))) || (allLines.empty()))
        {
            DAVINCI_LOG_ERROR << "Read license file failed!";
            return  DaVinciStatus(errc::file_exists);
        }

        for (unsigned int i = 0; i < allLines.size(); i++)
        {
            string currentString = allLines[i];

            if (boost::algorithm::contains(currentString, CpuAbiHead))
                status = GetDataVector(currentString, whiteList.cpuapi);
            else if (boost::algorithm::contains(currentString, ManufacturerHead))
                status = GetDataVector(currentString, whiteList.manufacturer);
            else if (boost::algorithm::contains(currentString, ModelHead))
                status = GetDataVector(currentString, whiteList.model);
            else if (boost::algorithm::contains(currentString, MinSdkHead))
                status = GetDataString(currentString, whiteList.minsdk);
            else if (boost::algorithm::contains(currentString, ExpireHead))
            {
                status = GetDataString(currentString, whiteList.expire);
                if (!IsValidDate(whiteList.expire))
                    status = errc::bad_message;
            }
            else if (boost::algorithm::contains(currentString, SignHead))
                status = GetDataString(currentString, whiteList.sign);
            else
                status = DaVinciStatusSuccess;

            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Invalid license file, parse the line failure: " << currentString;
                return  DaVinciStatus(errc::bad_message);
            }
        }

        if (!IsValidSignature())
        {
            DAVINCI_LOG_ERROR << "Invalid DaVinci License File detected!";
            status = DaVinciStatus(errc::bad_message);
        }
        else
        {
            Initialized = true;
            status = DaVinciStatusSuccess;
        }

        return status;
    }

    DaVinciStatus License::WriteLicenseFile(const string &fileName)
    {
        string forWrite;
        ofstream ofile(fileName);

        if (ofile.fail())
        {
            DAVINCI_LOG_ERROR << (std::string("Failed to open file: ") + fileName);
            return errc::invalid_argument;
        }
        
        // Generate the signature before write down.
        UpdateSignature();

        forWrite = CpuAbiHead;
        for (unsigned int i = 0; i < whiteList.cpuapi.size(); i++)
        {
            forWrite += whiteList.cpuapi[i];
            if (i != (whiteList.cpuapi.size() - 1))
                forWrite += ",";
            else
                forWrite += "\n";
        }
        ofile << forWrite;

        forWrite = ManufacturerHead;
        for (unsigned int i = 0; i < whiteList.manufacturer.size(); i++)
        {
            forWrite += whiteList.manufacturer[i];
            if (i != (whiteList.manufacturer.size() - 1))
                forWrite += ",";
            else
                forWrite += "\n";
        }
        ofile << forWrite;

        forWrite = ModelHead;
        for (unsigned int i = 0; i < whiteList.model.size(); i++)
        {
            forWrite += whiteList.model[i];
            if (i != (whiteList.model.size() - 1))
                forWrite += ",";
            else
                forWrite += "\n";
        }
        ofile << forWrite;

        forWrite = MinSdkHead;
        forWrite += whiteList.minsdk;
        forWrite += "\n";
        ofile << forWrite;

        forWrite = ExpireHead;
        forWrite += whiteList.expire;
        forWrite += "\n";
        ofile << forWrite;

        forWrite = SignHead;
        forWrite += whiteList.sign;
        forWrite += "\n";
        ofile << forWrite;
        ofile.close();

        return DaVinciStatusSuccess;
    }

    DaVinciStatus License::GetDataString(const string &fullDataString, string &dataString)
    {
        vector<std::string> equationStrings; 
        DaVinciStatus status = errc::bad_message;

        boost::split(equationStrings, fullDataString, boost::is_any_of("="));

        if ((equationStrings.size() > 1) && (equationStrings[1] != ""))
        {
            dataString = equationStrings[1];
            status = DaVinciStatusSuccess;
        }
        else
        {
            dataString = "";
            status = errc::bad_message;
            DAVINCI_LOG_ERROR<< "Configure data is empty in License file.";
        }

        return status;
    }

    DaVinciStatus License::GetDataVector(const string &fullDataString, vector<string> &dataVector)
    {
        string dataString;
        vector<std::string> dataItems; 
        DaVinciStatus status = errc::bad_message;

        if (DaVinciSuccess(GetDataString(fullDataString, dataString))) 
        {
            boost::split(dataItems, dataString, boost::is_any_of(","));

            for (unsigned int i = 0; i < dataItems.size(); i++)
            {
                boost::algorithm::trim(dataItems[i]);
                boost::algorithm::to_lower(dataItems[i]);
                dataVector.push_back(dataItems[i]);
                status = DaVinciStatusSuccess;
            }
        }

        return status;
    }

    DaVinciStatus License::GetDeviceSysprop(const string &deviceName, const string &sysProp, string &dataString)
    {
        DaVinciStatus status = errc::bad_message;
        auto outputStr = boost::shared_ptr<ostringstream>(new ostringstream());

        dataString = "";
        string cmd = "-s " + deviceName + " shell getprop " + sysProp;

        if (DaVinciSuccess(AndroidTargetDevice::AdbCommandNoDevice(cmd, outputStr, 5000)))
        {
            dataString = boost::algorithm::trim_copy(outputStr->str());
            boost::algorithm::to_lower(dataString);
            status = DaVinciStatusSuccess;
        }

        return status;
    }

    bool License::IsValidSignature()
    {
        string fileSign = GetSignature();
        string calSign = GenSignature();

        if (fileSign == calSign)
            return true;
        else
            return false;
    }

    string License::GenSignature()
    {
        string sign = "";
        sha1 msha1;
        unsigned int digest[5] = {0};
        unsigned int index = 0;
        string encryptDataString;

        // encrypt string by private key
        encryptDataString = PrivateKey;
        for (unsigned int i = 0; i < whiteList.cpuapi.size(); i++)
            encryptDataString += whiteList.cpuapi[i];

        for (unsigned int i = 0; i < whiteList.manufacturer.size(); i++)
            encryptDataString += whiteList.manufacturer[i];

        for (unsigned int i = 0; i < whiteList.model.size(); i++)
            encryptDataString += whiteList.model[i];

        encryptDataString += whiteList.minsdk;
        encryptDataString += whiteList.expire;

        encryptDataString += PrivateKey;

        // generate uuid by the encrypted string.
        msha1.reset();
        msha1.process_bytes(encryptDataString.c_str(), encryptDataString.length());
        msha1.get_digest(digest);

        // random pick up part of the uuid for sign 
        index = digest[0] % 5;
        sign += boost::lexical_cast<std::string>(digest[index]);

        return sign;
    }

    bool License::IsValidDate(const string &expireDate)
    {
        if (expireDate == "permanent")
            return true;

        try {
            date expire = from_undelimited_string(expireDate);
        }
        catch(...)
        {
            DAVINCI_LOG_ERROR << "The expire date format incorrect: " << expireDate;
            return false;
        }

        return true;
    }

    bool License::IsExpired(const string &expireDate)
    {
        date expire(day_clock::local_day());
        date current(day_clock::local_day());
        date_duration daysLeft(1);

        if (!IsValidDate(expireDate))
            return true;

        if (expireDate == "permanent")
            return false;

        expire = from_undelimited_string(expireDate);
        daysLeft = expire - current;
        if (daysLeft.is_negative())
            return true;

        return false;
    }

    // For generate DaVinci.lic file.
    void License::UpdateSignature()
    {
        string sign = GenSignature();

        whiteList.sign = sign;
    }

    string License::GetSignature()
    {
        return whiteList.sign;
    }
}

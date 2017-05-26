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

/*
* Intel confidential -- do not distribute further
* This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
* Please read and accept license.txt distributed with this package before using this source code
*/

#ifndef __LICENSE_HPP__
#define __LICENSE_HPP__

#include "boost/uuid/sha1.hpp"
#include "AndroidTargetDevice.hpp"

using namespace boost::uuids::detail;

namespace DaVinci
{
    /// <summary> Manager for license. </summary>
    class License
    {
        typedef struct WhiteList
        {
            vector<string> cpuapi;       //armeabi-v7a, armeabi, x86
            vector<string> manufacturer; //lenovo, asus
            vector<string> model;        //Nexus 7, Nexus 5, Nexus 9
            string minsdk;               //18->Android OS version 4.3, 19->4.4 бн 23->6.0
            string expire;
            string sign;
        } WhiteList;

    public:
        /// <summary> Default constructor. </summary>
        License();

        /// <summary> Default destruction. </summary>
        ~License();
        
        /// <summary>
        /// Check if the device is a valid device. 
        /// </summary>
        ///
        /// <returns> True for yes, False for no. </returns>
        bool IsValidDevice(const string &deviceName);

        /// <summary>
        /// Read license file's content to whitelist.
        /// </summary>
        ///
        /// <returns> success/failure </returns>
        DaVinciStatus ReadLicenseFile(const string &fileName);

        /// <summary>
        /// Write the cached whitelist to license file.
        /// </summary>
        ///
        /// <returns> success/failure </returns>
        DaVinciStatus WriteLicenseFile(const string &fileName);

        /// <summary>
        /// Check if the data in data vector. 
        /// expose it for unit test.
        /// </summary>
        ///
        /// <returns> True for yes, False for no. </returns>
        bool IsDataInVector(const string &data, const vector<string> &dataVector);

    private:
        static const string DefaultLicFile;
        static const string PrivateKey;
        static const string SdkPropStr;
        static const string CpuAbiPropStr;
        static const string ManufacturerPropStr;
        static const string ModelPropStr;

        static const string CpuAbiHead;
        static const string ManufacturerHead;
        static const string ModelHead;
        static const string MinSdkHead;
        static const string ExpireHead;
        static const string SignHead;

        static const int MinSDK = 18; //Android 4.3

        /// <summary> The white list for valid devices. </summary>
        WhiteList whiteList;
        bool Initialized;

        void ClearWhiteList();

        DaVinciStatus GetDeviceSysprop(const string &deviceName, const string &sysProp, string &dataString);
        DaVinciStatus GetDataString(const string &fullDataString, string &dataString);
        DaVinciStatus GetDataVector(const string &dataString, vector<string> &dataVector);

        string GetSignature();
        string GenSignature();
        void UpdateSignature();
        bool IsValidSignature();

        bool IsValidDate(const string &expireDate);
        bool IsExpired(const string &expireDate);
    };
}

#endif

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

#ifndef __TEST_REPORT_SUMMARY_HPP__
#define __TEST_REPORT_SUMMARY_HPP__

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <stdexcept>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>

#include "DaVinciStatus.hpp"
#include "TestReportCommon.hpp"
#include "AndroidTargetDevice.hpp"

namespace DaVinci
{
    class TestReportSummary : public boost::enable_shared_from_this<TestReportSummary>
    {
    public:
        TestReportSummary();
        ~TestReportSummary();

        /// <summary>
        /// txtReportSummaryName
        /// </summary>
        static string txtReportSummaryName;

        /// <summary> Prepare test report summary for test cases. </summary>
        /// <param name="scriptFile"> The report summary file. </param>
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus PrepareReportSummary(string& reportSummaryFile);

        /// <summary> append one Node <subcase> under /test-report/test-suite/test-result/case </summary>
        /// <param name="resulttype"> <test-result type="RNR"> </param>
        /// <param name="typevalue"> <test-result type="RNR"> </param>
        /// <param name="casename"> <case name="com.air.cc.apk"> </param>
        /// <param name="casenamevalue"> <case name="com.air.cc.apk"> </param>
        /// <param name="totalreasultvalue"> <case totalresult="pass"> </param>
        /// <param name="totalmessagevalue"> <case message="xxx"> </param>
        /// <param name="logcatvalue"> <case logcat="xxx"> </param>
        /// <param name="subcasename"> <test-result type="Smoke Test"> </param>
        /// <param name="subcaseresult"> <test-result type="Smoke Test"> </param>
        /// <param name="starttime"></param>
        /// <param name="endtime"></param>
        /// <param name="message"></param>
        /// <param name="detailmessage"></param>
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus appendSmokeResultNode(string& xmlFileName,
            string resulttype,
            string typevalue,    
            string casename,
            string casenamevalue,            
            string totalreasultvalue,
            string totalmessagevalue,
            string logcatvalue,
            string application, //apk name
            string package,
            string activity,
            string subcasename,
            string subcaseresult,
            string starttime,
            string endtime,
            string message,
            string detailmessage);

        DaVinciStatus appendFPSResultNode(string& xmlFileName,
            string resulttype, //<test-result type="FPS">
            string typevalue, 
            string casename,
            string casenamevalue,
            string totalreasultvalue,
            string totalmessagevalue,
            string logcatvalue,
            string subcasename,
            string subcaseresult,
            string avgfpsvalue,
            string maxfpsvalue,
            string minfpsvalue,
            string starttime,
            string endtime,
            string message,
            string detailmessage);

        DaVinciStatus appendPowerMeasureResultNode(
            std::string& xmlFileName,
            std::map<std::string, std::string> caseInfo,
            std::map<std::string, std::string> batteryEnergyInfo);

        DaVinciStatus appendLaunchTimeResultNode(string& xmlFileName,
            string resulttype, //<test-result type="RNR">
            string typevalue, 
            string casename,
            string casenamevalue,
            string totalreasultvalue,
            string totalmessagevalue,
            string logcatvalue,
            string subcasename,
            string subcaseresult,
            string launchtime,
            string referenceimagename,
            string referenceimageurl,
            string targetimagename,
            string targetimageurl,
            string beforeimagename,
            string beforeimageurl,
            string starttime,
            string endtime,
            string message,
            string detailmessage);

        DaVinciStatus appendCommonNodeUnmatchimage(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string opcodename,
            string line,
            string referencename,
            string referenceurl,
            string targetname,
            string targeturl);

        DaVinciStatus appendPathImage(string& xmlFileName,
            string pathnamevalue, //qs name
            string opcodename,
            string line,
            string referencename,
            string referenceurl,
            string targetname,
            string targeturl);

		DaVinciStatus addPathCoverage(string& xmlFileName,
			string pathnamevalue, //qs name
			string coveragevalue);

        DaVinciStatus addMatchresult(string& xmlFileName,
            string line,
            string matchresult);

        DaVinciStatus appendCommonNodeUnmatchObject(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string opcodename,
            string line,
            string referencename,
            string referenceurl,
            string targetname,
            string targeturl);


        DaVinciStatus appendCommonNodeCrashdialog(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string timestamp,
            string imagename,
            string imageurl);


        DaVinciStatus appendCommonNodeFlick(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string timestamp,
            string imagename,
            string imageurl);

        DaVinciStatus appendCommonNodeCommoninfo(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string message,
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string timestamp,
            string infoname,
            string infourl);

        DaVinciStatus appendCommonNodeAudio(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string totalreasult, 
            string message,
            string itemname,
            string itemresult,
            string itemstarttime,
            string itemendtime,
            string timestamp,
            string audioname,
            string audiourl);

        DaVinciStatus appendCommonNodePass(string& xmlFileName,
            string resulttype, //<test-result type="RnR">
            string typevalue, 
            string commonnamevalue,
            string starttime,
            string endtime,
            string message);

        DaVinciStatus appendSmokeImage(string& xmlFileName, 
            string pathnamevalue,
            string loop,
            string timestamp,
            string image,
            string imageurl);

        DaVinciStatus appendTxtReportSummary(string content = "\n");

        DaVinciStatus getDeviceInfo();

    private:

        boost::shared_ptr<TargetDevice> currentDevice;

        string buildVersion; 
        string build_abi2;
        string build_board; 
        string build_brand; 
        string build_device; 
        string build_fingerprint; 
        string build_manufacturer;
        string build_type; 
        string deviceID;
        string ipaddress; 
        string resolution;
        string storage_devices;

        string currentReportSummaryName;

    };
}

#endif

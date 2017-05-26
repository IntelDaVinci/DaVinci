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

#ifndef __INDEX_REPORT__
#define __INDEX_REPORT__

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
    class IndexReport : public boost::enable_shared_from_this<IndexReport>
    {
    public:
        IndexReport(std::string qs_path);
        ~IndexReport();

        DaVinciStatus GenerateReportIndex();

    private:
        std::string mFolderPath;
        std::string mHtmlContent;

        //string smoke_result;
        std::vector <std::map<std::string, std::string>> mSmokeResult;
        std::vector <std::map<std::string, std::string>> mRnrResult;
        std::vector <std::map<std::string, std::string>> mLaunchtimeResult;
        std::vector <std::map<std::string, std::string>> mFpsResult;

        DaVinciStatus ParseXml(boost::filesystem::path xml_path);

        void getAllTSFolders(boost::filesystem::path directory, vector<boost::filesystem::path> & out);

        void ParseFolder(vector<boost::filesystem::path> & folder_path);

        string HighlightResult(const string & s);
        void UpdateRnRContent();
        void UpdateFPSContent();
        void UpdateSmokeContent();
        void UpdateLaunchTimeContent();
        //void UpdateLaunchTimeContent(string & html);

    };
}



#endif
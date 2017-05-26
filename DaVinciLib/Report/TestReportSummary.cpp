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

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include <string>
#include <sstream>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <boost/core/null_deleter.hpp>

#include "DaVinciCommon.hpp"
#include "TestReport.hpp"
#include "TestReportSummary.hpp"
#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include "LauncherAppRecognizer.hpp"
#include "BaseObjectCategory.hpp"
#include "AndroidTargetDevice.hpp"
#include "QScript.hpp"

using namespace std;
using namespace xercesc;
using namespace boost::filesystem;
using namespace boost::posix_time;

namespace DaVinci
{
    string TestReportSummary::txtReportSummaryName;

    TestReportSummary::TestReportSummary()
    {
        buildVersion = "N/A";
        build_abi2 = "N/A";
        build_board = "N/A";
        build_brand = "N/A"; 
        build_device = "N/A";
        build_fingerprint = "N/A";
        build_manufacturer = "N/A";
        build_type = "N/A";
        deviceID = "N/A";
        ipaddress = "N/A";
        resolution = "N/A";
        storage_devices = "N/A";

        currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
    }

    TestReportSummary::~TestReportSummary()
    {
    }

    DaVinciStatus TestReportSummary::getDeviceInfo()
    {
        auto currentDevice = DeviceManager::Instance().GetCurrentTargetDevice();
        if (currentDevice == nullptr)
        {
            DAVINCI_LOG_ERROR << "The current device is not Android device.";
            return DaVinciStatus(errc::executable_format_error);
        }
        else
        {
            buildVersion = currentDevice->GetSystemProp("ro.build.version.release");
            if(boost::equals(buildVersion,""))
            {
                buildVersion = "N/A";
            }

            build_abi2 = currentDevice->GetSystemProp("ro.product.cpu.abi2");
            if(boost::equals(build_abi2,""))
            {
                build_abi2 = "N/A";
            }

            build_board = currentDevice->GetSystemProp("ro.product.board");
            if(boost::equals(build_board,""))
            {
                build_board = "N/A";
            }

            build_brand = currentDevice->GetSystemProp("ro.product.brand");
            if(boost::equals(build_brand,""))
            {
                build_brand = "N/A";
            }

            build_device = currentDevice->GetSystemProp("ro.product.device");
            if(boost::equals(build_device,""))
            {
                build_device = "N/A";
            }

            build_fingerprint = currentDevice->GetSystemProp("ro.build.fingerprint");
            if(boost::equals(build_fingerprint,""))
            {
                build_fingerprint = "N/A";
            }

            build_manufacturer = currentDevice->GetSystemProp("ro.product.manufacturer");
            if(boost::equals(build_manufacturer,""))
            {
                build_manufacturer = "N/A";
            }

            build_type = currentDevice->GetSystemProp("ro.build.type");
            if(boost::equals(build_type,""))
            {
                build_type = "N/A";
            }

            deviceID = currentDevice->GetDeviceName();
            if(boost::equals(deviceID,""))
            {
                deviceID = "N/A";
            }

            ipaddress = currentDevice->GetSystemProp("dhcp.wlan0.ipaddress");
            if(boost::equals(ipaddress,""))
            {
                ipaddress = "N/A";
            }

            //resolution = currentDevice->GetSystemProp("");
            //if(boost::equals(resolution,""))
            //{
            //    resolution = "N/A";
            //}

            //storage_devices = currentDevice->GetSystemProp("");
            //if(boost::equals(storage_devices,""))
            //{
            //    storage_devices = "N/A";
            //}

            return 0;
        }
    }

    /// <summary>
    /// Append info to ReportSummary.
    /// </summary>
    /// <param name="content">content: what will be added to the file ReportSummary.</param>
    DaVinciStatus TestReportSummary::appendTxtReportSummary(string content)
    {
        txtReportSummaryName = TestManager::Instance().GetDaVinciHome() + std::string("\\Davinci_Log\\") + std::string("ReportSummary.log");

        if (!boost::filesystem::exists(txtReportSummaryName)) // if txtReportSummaryName not exist, create it
        {
            string txtReportSummaryDir = TestManager::Instance().GetDaVinciHome() + std::string("\\Davinci_Log\\");
            if (!exists(txtReportSummaryDir))
            {
                // Creat DaVinci_Logs
                if (!exists(txtReportSummaryDir))
                {
                    boost::system::error_code ec;
                    create_directory(txtReportSummaryDir, ec);
                    DaVinciStatus status(ec);
                    if (!DaVinciSuccess(status))
                    {
                        DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << txtReportSummaryDir;
                        return status;
                    }
                }
                else if (!is_directory(txtReportSummaryDir))
                {
                    DAVINCI_LOG_ERROR << txtReportSummaryDir << " is not a directory.";
                    return DaVinciStatus(errc::not_a_directory);
                }
            }
        }

        std::ofstream txtOutput(txtReportSummaryName, std::ios_base::app | std::ios_base::out);
        txtOutput << content << endl << std::flush;
        txtOutput.flush();
        txtOutput.close();

        return 0;
    }

    DaVinciStatus TestReportSummary::PrepareReportSummary(string& xmlFileName)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child1 = nullptr; //test-case
            boost::shared_ptr<DOMElement> child2 = nullptr;
            boost::shared_ptr<DOMElement> child3 = nullptr;
            boost::shared_ptr<DOMElement> child4 = nullptr;
            boost::shared_ptr<DOMElement> child5 = nullptr;

            boost::shared_ptr<DOMElement> newnode11 = nullptr;
            boost::shared_ptr<DOMElement> newnode12 = nullptr;

            boost::shared_ptr<DOMElement> newnode21 = nullptr;
            boost::shared_ptr<DOMElement> newnode22 = nullptr;
            boost::shared_ptr<DOMElement> newnode23 = nullptr;

            boost::shared_ptr<DOMElement> newnode31 = nullptr;

            boost::shared_ptr<DOMElement> newnode41 = nullptr;

            getDeviceInfo();

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, create it
            {
                doc = boost::shared_ptr<DOMDocument>(domImpl->createDocument(0, StrToXMLStr("test-report").get(), 0), &XmlDocumentPtrDeleter<DOMDocument>);
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);
                DOMProcessingInstruction* procInstruction = doc->createProcessingInstruction(StrToXMLStr("xml-stylesheet").get(), StrToXMLStr(" type='text/xsl'  href='result.xsl'").get());
                doc->insertBefore(procInstruction, doc->getFirstChild());
                //doc->appendChild(procInstruction);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement());
            }

            child = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-suite").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            child1 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-device").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            child2 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-host").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            child3 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-summary").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            child4 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-result").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            child5 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("test-path").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            //Node <test-device>
            newnode11 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("deviceinfo").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            newnode12 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("camerainfo").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            //Node <test-host>
            newnode21 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("os").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            newnode22 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("java").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);
            newnode23 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("davinci").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            //Node <test-summary>
            newnode31 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("null").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            //Node <test-result>
            newnode41 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), 
                &XmlDocumentPtrDeleter<DOMElement>);

            newnode11->setAttribute(StrToXMLStr("buildVersion").get(), StrToXMLStr(buildVersion).get());
            newnode11->setAttribute(StrToXMLStr("build_abi2").get(), StrToXMLStr(build_abi2).get());
            newnode11->setAttribute(StrToXMLStr("build_board").get(), StrToXMLStr(build_board).get());
            newnode11->setAttribute(StrToXMLStr("build_brand").get(), StrToXMLStr(build_brand).get());
            newnode11->setAttribute(StrToXMLStr("build_device").get(), StrToXMLStr(build_device).get());
            newnode11->setAttribute(StrToXMLStr("build_fingerprint").get(), StrToXMLStr(build_fingerprint).get());
            newnode11->setAttribute(StrToXMLStr("build_manufacturer").get(), StrToXMLStr(build_manufacturer).get());
            newnode11->setAttribute(StrToXMLStr("build_type").get(), StrToXMLStr(build_type).get());
            newnode11->setAttribute(StrToXMLStr("deviceID").get(), StrToXMLStr(deviceID).get());
            newnode11->setAttribute(StrToXMLStr("ipaddress").get(), StrToXMLStr(ipaddress).get());
            newnode11->setAttribute(StrToXMLStr("resolution").get(), StrToXMLStr(resolution).get());
            newnode11->setAttribute(StrToXMLStr("storage_devices").get(), StrToXMLStr(storage_devices).get());

            newnode12->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("psedu").get());
            newnode12->setAttribute(StrToXMLStr("preset").get(), StrToXMLStr("High Speed").get());
            newnode12->setAttribute(StrToXMLStr("focus").get(), StrToXMLStr("1280").get());
            newnode12->setAttribute(StrToXMLStr("parameters").get(), StrToXMLStr("redhookbay High Speed High Speed").get());
            newnode12->setAttribute(StrToXMLStr("maxfps").get(), StrToXMLStr("120").get());

            // <test-host>
            child2->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(" ").get());

            double dbOSVersion;
            string strOSName;
            GetHostInfo::GetOSInfo(dbOSVersion, strOSName);

            long dwTotal;
            long dwFree;
            GetHostInfo::GetMemInfo(dwTotal, dwFree);

            string hostname;
            GetHostInfo::GetHostName(hostname);

            newnode21->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(hostname).get());
            newnode21->setAttribute(StrToXMLStr("version").get(), StrToXMLStr(strOSName).get());
            newnode21->setAttribute(StrToXMLStr("arch").get(), StrToXMLStr(" ").get());
            newnode21->setAttribute(StrToXMLStr("ram").get(), StrToXMLStr(to_string(dwTotal)).get());

            // <test-summary>
            child3->setAttribute(StrToXMLStr("failed").get(), StrToXMLStr("7").get());
            child3->setAttribute(StrToXMLStr("notExecuted").get(), StrToXMLStr("0").get());
            child3->setAttribute(StrToXMLStr("timeout").get(), StrToXMLStr("1").get());
            child3->setAttribute(StrToXMLStr("pass").get(), StrToXMLStr("17").get());

            // <test-result>
            child4->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RNR").get());			
			string folderaddress = boost::lexical_cast<std::string>(TestReport::currentQsLogPath);
			child4->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(folderaddress).get());
            //newnode41->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("com.air.cc.apk").get());

            // <test-path>
            child5->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RNR").get());

            root->appendChild(child.get());
            child->appendChild(child1.get());
            child->appendChild(child2.get());
            child->appendChild(child3.get());
            child->appendChild(child4.get());
            child->appendChild(child5.get());

            child1->appendChild(newnode11.get());
            child1->appendChild(newnode12.get());

            child2->appendChild(newnode21.get());
            child2->appendChild(newnode22.get());
            child2->appendChild(newnode23.get());

            child3->appendChild(newnode31.get());

            //child4->appendChild(newnode41.get());

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendSmokeResultNode(string& xmlFileName,
        string resulttype, //<test-result type="RNR">
        string typevalue, 
        string casename, //AppName
        string casenamevalue,
        string totalreasultvalue,
        string totalmessagevalue,
        string logcatvalue,
        string apkname,
        string package,
        string activity,
        string subcasename,
        string subcaseresult,
        string starttime,
        string endtime,
        string message,
        string detailmessage)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("case").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("apk").get(), StrToXMLStr(apkname).get());
                newnodecase->setAttribute(StrToXMLStr("package").get(), StrToXMLStr(package).get());
                newnodecase->setAttribute(StrToXMLStr("activity").get(), StrToXMLStr(activity).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("Smoke").get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr(casename).get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), casenamevalue))
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                    newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                    newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("apk").get(), StrToXMLStr(apkname).get());
                    newnodecase->setAttribute(StrToXMLStr("package").get(), StrToXMLStr(package).get());
                    newnodecase->setAttribute(StrToXMLStr("activity").get(), StrToXMLStr(activity).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("Smoke").get());

                }
            }

            newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("subcase").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(subcasename).get());
            newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(subcaseresult).get());
            newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(starttime).get());
            newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(endtime).get());

            newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("details").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodedetails->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());
            newnodedetails->setAttribute(StrToXMLStr("detailmessage").get(), StrToXMLStr(detailmessage).get());

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                newnodecase->appendChild(newnodesubcase.get()); //<case>
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                nodecase->appendChild(newnodesubcase.get()); //<case>
            }

            newnodesubcase->appendChild(newnodedetails.get()); //<subcase>

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendFPSResultNode(string& xmlFileName,
        string resulttype, //<test-result type="RNR">
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
        string detailmessage)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            XMLCh str[100];
            XMLString::transcode("Core", str, 99);
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(XMLString::transcode("test-suite"))->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(XMLString::transcode("case"))->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("FPS").get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr(casename).get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), casenamevalue))
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                    newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                    newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("FPS").get());

                }
            }

            newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("subcase").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(subcasename).get());
            newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(subcaseresult).get());
            newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(starttime).get());
            newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(endtime).get());
            newnodesubcase->setAttribute(StrToXMLStr("avgfpsvalue").get(), StrToXMLStr(avgfpsvalue).get());
            newnodesubcase->setAttribute(StrToXMLStr("minfpsvalue").get(), StrToXMLStr(minfpsvalue).get());
            newnodesubcase->setAttribute(StrToXMLStr("maxfpsvalue").get(), StrToXMLStr(maxfpsvalue).get());

            newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("details").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodedetails->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());
            newnodedetails->setAttribute(StrToXMLStr("detailmessage").get(), StrToXMLStr(detailmessage).get());

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                newnodecase->appendChild(newnodesubcase.get()); //<case>
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                nodecase->appendChild(newnodesubcase.get()); //<case>
            }

            newnodesubcase->appendChild(newnodedetails.get()); //<subcase>

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendPowerMeasureResultNode(
        std::string& xmlFileName,
        std::map<std::string, std::string> caseInfo,
        std::map<std::string, std::string> batteryEnergyInfo)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser            = nullptr;
            boost::shared_ptr<DOMDocument>     testReportDoc     = nullptr;
            boost::shared_ptr<DOMElement>      testResultElement = nullptr; //test-result
            boost::shared_ptr<DOMElement>      testcaseElement   = nullptr; //case

            XMLCh str[100];
            XMLString::transcode("Core", str, 99);
            boost::shared_ptr<DOMImplementation> domImpl =
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);
            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }

            parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
            parser->parse(xmlFileName.c_str());
            testReportDoc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());

            std::vector<boost::shared_ptr<DOMNode>> testResultNode = GetNodesFromXPath(
                testReportDoc,
                "/test-report/test-suite/test-result",
                StrToXMLStr("type").get(),
                "RNR");
            if (testResultNode.size() == 0 || testResultNode[0] == nullptr) // if Script node not exist, exit.
            {
                return DaVinciStatus(errc::no_message);
            }
            testResultElement = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(testResultNode[0].get()), boost::null_deleter());
            testcaseElement = boost::shared_ptr<DOMElement>(
                dynamic_cast<DOMElement *>(
                    testResultElement->getElementsByTagName(XMLString::transcode("test-suite"))->item(0)), 
                    boost::null_deleter());
            if (testcaseElement == nullptr) // if test-suite node not exist, exit.
            {
                testcaseElement = boost::shared_ptr<DOMElement>(
                    testReportDoc->createElement(StrToXMLStr("case").get()),
                    &XmlDocumentPtrDeleter<DOMElement>);
                assert(testcaseElement != nullptr);
            }

            testcaseElement->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("PowerTest").get());
            auto caseIt = caseInfo.find("case");
            assert(caseIt != caseInfo.end());
            testcaseElement->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(caseIt->second).get());
            auto energyUsedIt = batteryEnergyInfo.find("Battery Energy Used");

            assert(energyUsedIt != batteryEnergyInfo.end());
            testcaseElement->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(energyUsedIt->second).get());
            
            std::string detailPowerInfo;
            for (auto it = batteryEnergyInfo.begin(); it != batteryEnergyInfo.end(); it++)
            {
                detailPowerInfo += it->first;
                detailPowerInfo += ":";
                detailPowerInfo += it->second;
                detailPowerInfo += "\n";
            }
            testcaseElement->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(detailPowerInfo).get());

            testResultElement->appendChild(testcaseElement.get());  //<test-result>

            WriteDOMDocumentToXML(domImpl, testReportDoc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }


    DaVinciStatus TestReportSummary::appendLaunchTimeResultNode(string& xmlFileName,
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
        string detailmessage)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            XMLCh str[100];
            XMLString::transcode("Core", str, 99);
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(XMLString::transcode("test-suite"))->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(XMLString::transcode("case"))->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("LaunchTime").get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr(casename).get()),  &XmlDocumentPtrDeleter<DOMNode>);
				
				if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), casenamevalue))
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("case").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("logcat").get(), StrToXMLStr(logcatvalue).get());
                    newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(totalmessagevalue).get());                
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasultvalue).get());
                    newnodecase->setAttribute(StrToXMLStr(casename).get(), StrToXMLStr(casenamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("LaunchTime").get());

                }
            }

            newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("subcase").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(subcasename).get());
            newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("Launch Time").get());
            newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(starttime).get());
            newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(endtime).get());
            newnodesubcase->setAttribute(StrToXMLStr("launchtime").get(), StrToXMLStr(launchtime).get());
            newnodesubcase->setAttribute(StrToXMLStr("referenceimagename").get(), StrToXMLStr(referenceimagename).get());
            newnodesubcase->setAttribute(StrToXMLStr("referenceimageurl").get(), StrToXMLStr(referenceimageurl).get());
            newnodesubcase->setAttribute(StrToXMLStr("targetimagename").get(), StrToXMLStr(targetimagename).get());
            newnodesubcase->setAttribute(StrToXMLStr("targetimageurl").get(), StrToXMLStr(targetimageurl).get());
            newnodesubcase->setAttribute(StrToXMLStr("beforeimagename").get(), StrToXMLStr(beforeimagename).get());
            newnodesubcase->setAttribute(StrToXMLStr("beforeimageurl").get(), StrToXMLStr(beforeimageurl).get());

            newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("details").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodedetails->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());
            newnodedetails->setAttribute(StrToXMLStr("detailmessage").get(), StrToXMLStr(detailmessage).get());

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist, exit.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                newnodecase->appendChild(newnodesubcase.get()); //<case>
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                nodecase->appendChild(newnodesubcase.get()); //<case>
            }

            newnodesubcase->appendChild(newnodedetails.get()); //<subcase>

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }



    DaVinciStatus TestReportSummary::appendCommonNodeUnmatchimage(string& xmlFileName,
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
        string targeturl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            boost::shared_ptr<DOMElement> nodeitem = nullptr; //case

            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create <common> <item> <unmatchimage>.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
            }
            else // if <common> exist.
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                    newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                    newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                    newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                    newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                    newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                }
                else // if <common> node exist, and is current case.
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "unmatchimage");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                        newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                        newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                        newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                        newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                        newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                    }
                    else
                    {
                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                        newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                        newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                        newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                        newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                        newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                //newnodecase->appendChild(newnodesubcase.get()); //<case>
                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }            

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::io_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendSmokeImage(string& xmlFileName, string pathnamevalue, string loop, string timestamp, string image, string imageurl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-path
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            boost::shared_ptr<DOMElement> nodeitem = nullptr; //case

            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-path", 
                        StrToXMLStr("type").get(), 
                        "RNR");
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("smoke-path").get())->item(0)), boost::null_deleter());

            if (nodecase == nullptr) // if <smoke-path> node not exist. create <smoke-path> <item> <smokecriticalpath>.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("smoke-path").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(pathnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("smokecriticalpath").get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("smokecriticalpath").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("loop").get(), StrToXMLStr(loop).get());
                newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                newnodedetails->setAttribute(StrToXMLStr("image").get(), StrToXMLStr(image).get());
                newnodedetails->setAttribute(StrToXMLStr("imageurl").get(), StrToXMLStr(imageurl).get());
            }
            else // if <smoke-path> exist.
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), pathnamevalue)) // if <smoke-path> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("smoke-path").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(pathnamevalue).get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("smokecriticalpath").get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("smokecriticalpath").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("loop").get(), StrToXMLStr(loop).get());
                    newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                    newnodedetails->setAttribute(StrToXMLStr("image").get(), StrToXMLStr(image).get());
                    newnodedetails->setAttribute(StrToXMLStr("imageurl").get(), StrToXMLStr(imageurl).get());
                }
                else // if <smoke-path> node exist, and is current case.
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-path/smoke-path/item", 
                        StrToXMLStr("name").get(), 
                        "smokecriticalpath");

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("smokecriticalpath").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("loop").get(), StrToXMLStr(loop).get());
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("image").get(), StrToXMLStr(image).get());
                        newnodedetails->setAttribute(StrToXMLStr("imageurl").get(), StrToXMLStr(imageurl).get());
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-path>
                //newnodecase->appendChild(newnodesubcase.get()); //<case>
                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-path>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::io_error);
        }
        return DaVinciStatusSuccess;
    }


    DaVinciStatus TestReportSummary::appendPathImage(string& xmlFileName,
        string pathnamevalue, //qs name
        string opcodename,
        string line,
        string referencename,
        string referenceurl,
        string targetname,
        string targeturl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-path
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            boost::shared_ptr<DOMElement> nodeitem = nullptr; //case

            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-path", 
                        StrToXMLStr("type").get(), 
                        "RNR");
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("path").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <path> node not exist. create <path> <item> <criticalpath>.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("path").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(pathnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());
                newnodecase->setAttribute(StrToXMLStr("coverage").get(), StrToXMLStr("0%").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("criticalpath").get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("criticalpath").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
            }
            else // if <path> exist.
            {
                itemNodes = GetNodesFromXPath(doc, 
                    "/test-report/test-suite/test-path/path/item", 
                    StrToXMLStr("name").get(), 
                    "criticalpath");

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("criticalpath").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-path>
                //newnodecase->appendChild(newnodesubcase.get()); //<case>
                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-path>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }            

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::io_error);
        }

        return DaVinciStatusSuccess;
    }


    DaVinciStatus TestReportSummary::addPathCoverage(string& xmlFileName,
        string pathnamevalue, //qs name
        string coveragevalue)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-path
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {                    
                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-path/path", 
                        StrToXMLStr("type").get(), 
                        "RnR");
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }
			child4->setAttribute(StrToXMLStr("coverage").get(), StrToXMLStr(coveragevalue).get());

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::io_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::addMatchresult(string& xmlFileName,
        string line,
        string matchresult)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-path
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {                    
                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-path/path/item/criticalpath", 
                        StrToXMLStr("line").get(), 
                        line);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }
			child4->setAttribute(StrToXMLStr("matchresult").get(), StrToXMLStr(matchresult).get());

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::io_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendCommonNodeUnmatchObject(string& xmlFileName,
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
        string targeturl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            boost::shared_ptr<DOMElement> nodeitem = nullptr; //case

            std::vector<boost::shared_ptr<DOMNode>> itemNodes;
            std::vector<boost::shared_ptr<DOMNode>> itemdetailNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create <common> <item> <unmatchimage>.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchobject").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
            }
            else // if <common> exist.
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchobject").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                    newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                    newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                    newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                    newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                    newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                }
                else // if <common> node exist, and is current case.
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "unmatchobject");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchobject").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                        newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                        newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                        newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                        newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                        newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                    }
                    else
                    {
                        itemdetailNodes = GetNodesFromXPath(doc, 
                            "/test-report/test-suite/test-result/common/item/unmatchobject", 
                            StrToXMLStr("line").get(), 
                            line);
                        if (itemdetailNodes.size() == 0 || itemdetailNodes[0] == nullptr) // if item node not exist.
                        {
                            newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("unmatchobject").get()), &XmlDocumentPtrDeleter<DOMElement>);
                            newnodedetails->setAttribute(StrToXMLStr("opcodename").get(), StrToXMLStr(opcodename).get());
                            newnodedetails->setAttribute(StrToXMLStr("line").get(), StrToXMLStr(line).get());
                            newnodedetails->setAttribute(StrToXMLStr("referencename").get(), StrToXMLStr(referencename).get());
                            newnodedetails->setAttribute(StrToXMLStr("referenceurl").get(), StrToXMLStr(referenceurl).get());
                            newnodedetails->setAttribute(StrToXMLStr("targetname").get(), StrToXMLStr(targetname).get());
                            newnodedetails->setAttribute(StrToXMLStr("targeturl").get(), StrToXMLStr(targeturl).get());
                        }
                        else
                        {
                            // TODO set Attribute while this line is exist.

                        }
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                //newnodecase->appendChild(newnodesubcase.get()); //<case>
                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }            

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }


    DaVinciStatus TestReportSummary::appendCommonNodeCrashdialog(string& xmlFileName,
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
        string imageurl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("crashdialog").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("crashdialog").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                    newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                    newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                }
                else
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "crashdialog");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("crashdialog").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                    }
                    else
                    {
                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("crashdialog").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>

                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }


    DaVinciStatus TestReportSummary::appendCommonNodeFlick(string& xmlFileName,
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
        string imageurl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("flickimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("flickimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                    newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                    newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                }
                else
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "flickimage");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("flickimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                    }
                    else
                    {
                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("flickimage").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(imagename).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(imageurl).get());
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>

                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendCommonNodeCommoninfo(string& xmlFileName,
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
        string infourl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("commoninfo").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(infoname).get());
                newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(infourl).get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("commoninfo").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                    newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(infoname).get());
                    newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(infourl).get());
                }
                else
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "commoninfo");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("commoninfo").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(infoname).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(infourl).get());
                    }
                    else
                    {
                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("commoninfo").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(infoname).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(infourl).get());
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>

                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendCommonNodeAudio(string& xmlFileName,
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
        string audiourl)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase = nullptr;
            boost::shared_ptr<DOMElement> newnodedetails = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("audio").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(audioname).get());
                newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(audiourl).get());
            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr(totalreasult).get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                    newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                    newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                    newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                    newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                    newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("audio").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                    newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(audioname).get());
                    newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(audiourl).get());
                }
                else
                {
                    itemNodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result/common/item", 
                        StrToXMLStr("name").get(), 
                        "audio");
                    if (itemNodes.size() == 0 || itemNodes[0] == nullptr) // if item node not exist.
                    {
                        newnodesubcase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodesubcase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(itemname).get());
                        newnodesubcase->setAttribute(StrToXMLStr("result").get(), StrToXMLStr(itemresult).get());
                        newnodesubcase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(itemstarttime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(itemendtime).get());
                        newnodesubcase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());

                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("audio").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(audioname).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(audiourl).get());
                    }
                    else
                    {
                        newnodedetails = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("audio").get()), &XmlDocumentPtrDeleter<DOMElement>);
                        newnodedetails->setAttribute(StrToXMLStr("timestamp").get(), StrToXMLStr(timestamp).get());
                        newnodedetails->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(audioname).get());
                        newnodedetails->setAttribute(StrToXMLStr("url").get(), StrToXMLStr(audiourl).get());
                    }
                }
            }

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>

                if (newnodesubcase == nullptr) // <item>
                {
                    newnodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    newnodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                if (newnodesubcase == nullptr) // <item>
                {
                    nodecase->appendChild(itemNodes[0].get());
                    itemNodes[0]->appendChild(newnodedetails.get()); //<subcase>
                }
                else
                {
                    nodecase->appendChild(newnodesubcase.get()); //<case>
                    newnodesubcase->appendChild(newnodedetails.get()); //<subcase>
                }
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

    DaVinciStatus TestReportSummary::appendCommonNodePass(string& xmlFileName,
        string resulttype, //<test-result type="RnR">
        string typevalue, 
        string commonnamevalue,
        string starttime,
        string endtime,
        string message)
    {
        try
        {
            boost::shared_ptr<XercesDOMParser> parser = nullptr;
            boost::shared_ptr<DOMDocument> doc = nullptr;
            boost::shared_ptr<DOMElement> root = nullptr; //test-report
            boost::shared_ptr<DOMElement> child = nullptr; //test-suite
            boost::shared_ptr<DOMElement> child4 = nullptr; //test-result
            boost::shared_ptr<DOMElement> nodecase = nullptr; //case
            std::vector<boost::shared_ptr<DOMNode>> itemNodes;

            boost::shared_ptr<DOMElement> newnodecase = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase1 = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase2 = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase3 = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase4 = nullptr;
            boost::shared_ptr<DOMElement> newnodesubcase5 = nullptr;

            boost::shared_ptr<XMLCh> str = StrToXMLStr("Core");
            boost::shared_ptr<DOMImplementation> domImpl = 
                boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(str.get()), boost::null_deleter());

            boost::filesystem::path path(xmlFileName);

            if (!boost::filesystem::exists(path)) // if xml not exist, exit.
            {
                return DaVinciStatus(errc::file_exists);
            }
            else
            {
                parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
                parser->parse(xmlFileName.c_str());
                doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
                root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

                child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("test-suite").get())->item(0)), 
                    boost::null_deleter());
                if (child == nullptr) // if test-suite node not exist, exit.
                {
                    return DaVinciStatus(errc::no_message);
                }
                else
                {

                    std::vector<boost::shared_ptr<DOMNode>> child4Nodes = GetNodesFromXPath(doc, 
                        "/test-report/test-suite/test-result", 
                        StrToXMLStr(resulttype).get(), 
                        typevalue);
                    if (child4Nodes.size() == 0 || child4Nodes[0] == nullptr) // if Script node not exist, exit.
                    {
                        return DaVinciStatus(errc::no_message);
                    }
                    else
                    {
                        child4 = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(child4Nodes[0].get()), boost::null_deleter());
                    }
                }
            }

            nodecase = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("common").get())->item(0)), 
                boost::null_deleter());

            if (nodecase == nullptr) // if <common> node not exist. create.
            {
                newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr("PASS").get());
                newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                newnodecase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(starttime).get());
                newnodecase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(endtime).get());
                newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());
                newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

            }
            else 
            {
                boost::shared_ptr<DOMNode> attributes = 
                    boost::shared_ptr<DOMNode>(nodecase->getAttributes()->getNamedItem(StrToXMLStr("name").get()),  &XmlDocumentPtrDeleter<DOMNode>);

                if(!boost::equals(XMLStrToStr(attributes->getNodeValue()), commonnamevalue)) // if <common> node exist, but not for current case, creat.
                {
                    newnodecase = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("common").get()), &XmlDocumentPtrDeleter<DOMElement>);
                    newnodecase->setAttribute(StrToXMLStr("totalreasult").get(), StrToXMLStr("PASS").get());
                    newnodecase->setAttribute(StrToXMLStr("name").get(), StrToXMLStr(commonnamevalue).get());
                    newnodecase->setAttribute(StrToXMLStr("starttime").get(), StrToXMLStr(starttime).get());
                    newnodecase->setAttribute(StrToXMLStr("endtime").get(), StrToXMLStr(endtime).get());
                    newnodecase->setAttribute(StrToXMLStr("message").get(), StrToXMLStr(message).get());
                    newnodecase->setAttribute(StrToXMLStr("type").get(), StrToXMLStr("RnR").get());

                }
            }

            newnodesubcase1 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase1->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("unmatchimage").get());
            newnodesubcase1->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("PASS").get());

            newnodesubcase2 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase2->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("crashdialog").get());
            newnodesubcase2->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("PASS").get());

            newnodesubcase3 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase3->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("flickimage").get());
            newnodesubcase3->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("PASS").get());

            newnodesubcase4 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase4->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("audio").get());
            newnodesubcase4->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("PASS").get());

            newnodesubcase5 = boost::shared_ptr<DOMElement>(doc->createElement(StrToXMLStr("item").get()), &XmlDocumentPtrDeleter<DOMElement>);
            newnodesubcase5->setAttribute(StrToXMLStr("name").get(), StrToXMLStr("running").get());
            newnodesubcase5->setAttribute(StrToXMLStr("result").get(), StrToXMLStr("PASS").get());

            root->appendChild(child.get()); //<test-report>
            child->appendChild(child4.get()); //<test-suite>

            if (nodecase == nullptr) // if test-suite node not exist.
            {
                child4->appendChild(newnodecase.get()); //<test-result>
                newnodecase->appendChild(newnodesubcase1.get());
                newnodecase->appendChild(newnodesubcase2.get());
                newnodecase->appendChild(newnodesubcase3.get());
                newnodecase->appendChild(newnodesubcase4.get());
                newnodecase->appendChild(newnodesubcase5.get());
            }
            else 
            {
                child4->appendChild(nodecase.get()); //<test-result>
                nodecase->appendChild(newnodesubcase1.get());
                nodecase->appendChild(newnodesubcase2.get());
                nodecase->appendChild(newnodesubcase3.get());
                nodecase->appendChild(newnodesubcase4.get());
                nodecase->appendChild(newnodesubcase5.get());
            }

            WriteDOMDocumentToXML(domImpl, doc, xmlFileName);
        }
        catch(xercesc::XMLException& e)
        {
            char* message = xercesc::XMLString::transcode(e.getMessage());
            DAVINCI_LOG_ERROR << "Error parsing file: " << message << flush;
            XMLString::release(&message);
            return DaVinciStatus(errc::executable_format_error);
        }

        return DaVinciStatusSuccess;
    }

}



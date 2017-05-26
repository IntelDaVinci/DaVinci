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

#include <string>
#include <vector>
#include <xercesc/dom/DOM.hpp>
#include <boost/core/null_deleter.hpp>

#include "DaVinciCommon.hpp"
#include "IndexReport.hpp"

using namespace std;
using namespace xercesc;
using namespace boost::filesystem;
using namespace boost::posix_time;

namespace DaVinci
{

    IndexReport::IndexReport(string qs_path)
    {
        mFolderPath = qs_path;

        mHtmlContent = "<!DOCTYPE html><html><head><title>Test Summary</title>\
                       <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\
                       <script src=\"indexformat\\jquery-1.11.2.min.js\"></script><script src=\"indexformat\\index.js\"></script>\
                       <link rel=\"stylesheet\" href=\"indexformat\\index.css\"></head><body><div class=\"summary\">\
                       <div><font size=\"5\" face=\"Arial\"><b><center>DaVinci Test Summary</center></b></font></div>\
                       <div class=\"filter\"><span><input type=\"checkbox\" id=\"smokecb\" onclick=\"smoke(this)\">see smoke</span>\
                       </div><div class=\"filter\"><span><input type=\"checkbox\" id=\"rnrcb\" onclick=\"rnr(this)\">see rnr</span>\
                       </div><div class=\"filter\"><span><input type=\"checkbox\" id=\"fpscb\" onclick=\"fps(this)\">see fps</span>\
                        </div><div class=\"filter\"><span><input type=\"checkbox\" id=\"launchtimecb\" onclick=\"launchtime(this)\">see launchtime</span>\
                       </div><div class=\"filter\"><span><input type=\"checkbox\" id=\"failcb\" onclick=\"hide_fail(this)\"  name=\"rb\">fail only</span>\
                       </div></div>";
    }

    IndexReport::~IndexReport()
    {

    }

    void IndexReport::getAllTSFolders(boost::filesystem::path directory, vector<boost::filesystem::path> & out)
    {
        boost::filesystem::directory_iterator end_iter;

        if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
        {
            for(boost::filesystem::directory_iterator dir_iter(directory); dir_iter != end_iter; ++dir_iter)
            {
                if (boost::filesystem::is_directory(dir_iter->status()) && (*dir_iter).path().filename().string() != "indexformat")
                {
                    out.push_back(*dir_iter);
                }
            }
        }
    }


    std::string IndexReport::HighlightResult(const string & s)
    {
        string r;
        if(boost::equals(s, "PASS"))
        {
            r = "<b><font color=\"green\">" + s + "</font></b>";
        }
        else if (boost::equals(s, "FAIL"))
        {
            r = "<b><font color=\"red\">" + s + "</font></b>";
        }
        else if (boost::equals(s, "WARNING"))
        {
            r = "<b><font color=\"orange\">" + s + "</font></b>";
        }
        else if (boost::equals(s, "SKIP"))
        {
            r = "<b><font color=\"blue\">" + s + "</font></b>";
        }
        else
        {
            r = s;
        }
        return r;
    }


    DaVinciStatus IndexReport::ParseXml(boost::filesystem::path xml_path)
    {
        boost::shared_ptr<XercesDOMParser> parser = nullptr;
        boost::shared_ptr<DOMDocument> doc = nullptr;
        boost::shared_ptr<DOMElement> root = nullptr;
        boost::shared_ptr<DOMElement> child = nullptr;
        boost::shared_ptr<DOMNodeList> caselist = nullptr;
        boost::shared_ptr<DOMElement> casechild = nullptr;

        boost::shared_ptr<DOMNodeList> commonlist = nullptr;
        boost::shared_ptr<DOMElement> commonchild = nullptr;

        map<std::string, std::string> tmpResult;

        try
        {
            parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());

            parser->parse(xml_path.string().c_str());

            doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
            root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

            child = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("deviceinfo").get())->item(0)), boost::null_deleter());
            tmpResult["time"] = to_simple_string(from_iso_string(xml_path.parent_path().filename().string()));
            tmpResult["log"] = xml_path.string();

            if (child == nullptr)
            {
                tmpResult["android_version"] = "N/A";
                tmpResult["device_id"] = "N/A";
                tmpResult["device_build"] = "N/A";
                tmpResult["device_model"] = "N/A";
            }
            else
            {
                tmpResult["android_version"] = XMLStrToStr(child->getAttribute(StrToXMLStr("buildVersion").get()));
                tmpResult["device_id"] = XMLStrToStr(child->getAttribute(StrToXMLStr("deviceID").get()));
                tmpResult["device_build"] = XMLStrToStr(child->getAttribute(StrToXMLStr("build_fingerprint").get()));
                tmpResult["device_model"] = XMLStrToStr(child->getAttribute(StrToXMLStr("build_device").get()));
            }

            caselist = boost::shared_ptr<DOMNodeList>(dynamic_cast<DOMNodeList*>(root->getElementsByTagName(StrToXMLStr("case").get())), boost::null_deleter());

            if ( caselist->getLength() > 0)
            {
                auto case_length = caselist->getLength();
                for (unsigned int i = 0; i < case_length; i++)
                {
                    map<std::string, std::string> caseresult(tmpResult);

                    casechild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(caselist->item(i)), boost::null_deleter());
                    if (casechild != nullptr)
                    {
                        caseresult["test_type"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("type").get()));
                        caseresult["result"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("totalreasult").get()));
                    }
                    else
                    {
                        return -1; //Got test-type failed. No need to parse the xml any more.
                    }

                    if (boost::equals(caseresult["test_type"], "FPS"))
                    {
                        casechild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("subcase").get())->item(0)), boost::null_deleter());
                        if (casechild != nullptr)
                        {
                            caseresult["traceavg"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("avgfpsvalue").get()));
                            if(boost::equals(caseresult["traceavg"], ""))
                            {
                                caseresult["traceavg"] = "N/A";
                            }
                            caseresult["tracemax"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("maxfpsvalue").get()));
                            if(boost::equals(caseresult["tracemax"], ""))
                            {
                                caseresult["tracemax"] = "N/A";
                            }
                            caseresult["tracemin"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("minfpsvalue").get()));
                            if(boost::equals(caseresult["tracemin"], ""))
                            {
                                caseresult["tracemin"] = "N/A";
                            }
                        }
                        else
                        {
                            caseresult["traceavg"] = "N/A";
                            caseresult["tracemax"] = "N/A";
                            caseresult["tracemin"] = "N/A";
                        }

                        casechild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("subcase").get())->item(1)), boost::null_deleter());
                        if (casechild != nullptr)
                        {
                            caseresult["cameraavg"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("avgfpsvalue").get()));
                            if(boost::equals(caseresult["cameraavg"], ""))
                            {
                                caseresult["cameraavg"] = "N/A";
                            }
                            caseresult["cameramax"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("maxfpsvalue").get()));
                            if(boost::equals(caseresult["cameramax"], ""))
                            {
                                caseresult["cameramax"] = "N/A";
                            }
                            caseresult["cameramin"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("minfpsvalue").get()));
                            if(boost::equals(caseresult["tracemin"], ""))
                            {
                                caseresult["tracemin"] = "N/A";
                            }
                        }
                        else
                        {
                            caseresult["cameraavg"] = "N/A";
                            caseresult["cameramax"] = "N/A";
                            caseresult["cameramin"] = "N/A";
                        }

                        mFpsResult.push_back(caseresult);

                    }
                    else if (boost::equals(caseresult["test_type"], "Smoke"))
                    {
                        caseresult["reason"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("message").get()));
                        caseresult["apk_name"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("apk").get()));
                        caseresult["app_name"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("name").get()));
                        caselist = boost::shared_ptr<DOMNodeList>(dynamic_cast<DOMNodeList*>(root->getElementsByTagName(StrToXMLStr("subcase").get())), boost::null_deleter());
                        string smoke_field[5] = {"Install", "Launch", "Random", "Back", "Uninstall"};
                        vector<string> smoke_vector;
                        vector<string>::iterator smoke_it;
                        for(unsigned int i = 0; i < 5; i++)
                        {
                            caseresult[smoke_field[i]] = "N/A";
                            smoke_vector.push_back(smoke_field[i]);
                        }
                        auto subcase_length = caselist->getLength();

                        for(unsigned int i = 0 ; i < subcase_length; i++)
                        {
                            casechild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(caselist->item(i)), boost::null_deleter());
                            if (casechild != nullptr)
                            {
                                smoke_it = find(smoke_vector.begin(), smoke_vector.end(), XMLStrToStr(casechild->getAttribute(StrToXMLStr("name").get())));
                                if(smoke_it != smoke_vector.end())
                                {
                                    caseresult[*smoke_it] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("result").get()));
                                    if(boost::equals(caseresult[*smoke_it], ""))
                                    {
                                        caseresult[*smoke_it] = "N/A";
                                    }
                                }
                            }
                        }

                        mSmokeResult.push_back(caseresult);

                    }
                    else if (boost::equals(caseresult["test_type"], "LaunchTime"))
                    {
                       //TODO
                        casechild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(root->getElementsByTagName(StrToXMLStr("subcase").get())->item(0)), boost::null_deleter());
                        if (casechild != nullptr)
                        {
                           caseresult["launchtime"]  = XMLStrToStr(casechild->getAttribute(StrToXMLStr("launchtime").get()));
                           caseresult["beforeimagename"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("beforeimagename").get()));
                           caseresult["beforeimageurl"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("beforeimageurl").get()));
                           caseresult["referenceimagename"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("referenceimagename").get()));
                           caseresult["referenceimageurl"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("referenceimageurl").get()));
                           caseresult["targetimagename"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("targetimagename").get()));
                           caseresult["targetimageurl"] = XMLStrToStr(casechild->getAttribute(StrToXMLStr("targetimageurl").get()));
                        }
                        mLaunchtimeResult.push_back(caseresult);
                    }
                    else
                    {
                        DAVINCI_LOG_WARNING<<"Unknown Test "<<caseresult["test_type"];
                        return -1;
                    }
                }
            }
            else
            {
                commonlist = boost::shared_ptr<DOMNodeList>(dynamic_cast<DOMNodeList*>(root->getElementsByTagName(StrToXMLStr("common").get())), boost::null_deleter());
                if (commonlist != nullptr && commonlist->getLength() == 1)
                {
                    commonchild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(commonlist->item(0)), boost::null_deleter());
                    if (commonchild != nullptr)
                    {
                        tmpResult["test_type"] = XMLStrToStr(commonchild->getAttribute(StrToXMLStr("type").get()));
                    }
                    else
                    {
                        return -1;
                    }

                    tmpResult["result"] = XMLStrToStr(commonchild->getAttribute(StrToXMLStr("totalreasult").get()));

                    string rnr_field[5] = {"unmatchimage", "crashdialog", "flickimage", "audio", "running"};
                    vector<string> rnr_vector;
                    vector<string>::iterator field_it;

                    for (unsigned int i = 0; i < 5; i++)
                    {
                        tmpResult[rnr_field[i]] = "N/A";
                        rnr_vector.push_back(rnr_field[i]);
                    }

                    commonlist = boost::shared_ptr<DOMNodeList>(dynamic_cast<DOMNodeList*>(root->getElementsByTagName(StrToXMLStr("item").get())), boost::null_deleter());

                    for (unsigned int i = 0; i < commonlist->getLength(); i++)
                    {
                        commonchild = boost::shared_ptr<DOMElement>(dynamic_cast<DOMElement *>(commonlist->item(i)), boost::null_deleter());
                        if (commonchild != nullptr)
                        {
                            field_it = find(rnr_vector.begin(), rnr_vector.end(), XMLStrToStr(commonchild->getAttribute(StrToXMLStr("name").get())));
                            if(field_it != rnr_vector.end())
                            {
                                tmpResult[*field_it] = XMLStrToStr(commonchild->getAttribute(StrToXMLStr("result").get()));
                                if(boost::equals(tmpResult[*field_it], ""))
                                {
                                    tmpResult[*field_it] = "N/A";
                                }
                            }
                        }
                    }
                    mRnrResult.push_back(tmpResult);
                }
            }

            return DaVinciStatusSuccess;

        }
        catch(...)
        {
            DAVINCI_LOG_WARNING << "parse xml failed";
            return -1;
        }
    }

    void IndexReport::ParseFolder(vector<boost::filesystem::path> & folder_path)
    {
        vector<boost::filesystem::path>::reverse_iterator  end = folder_path.rend();
        boost::filesystem::path xml_file;
        for(vector<boost::filesystem::path>::reverse_iterator i =  folder_path.rbegin(); i != end; i++)
        {
            xml_file = (*i);
            xml_file /= "ReportSummary.xml";

            if( boost::filesystem::exists(xml_file))
            {
                ParseXml(xml_file);
            }
            else
            {
                DAVINCI_LOG_WARNING<<xml_file;
            }
        }
    }


    void IndexReport::UpdateRnRContent()
    {
        mHtmlContent +=  "<div id=\"rnrdiv\"><p>RnR Pass Rate: <span id=\"rnrpr\"></span></p>";
        mHtmlContent +=  "<table class=\"two\" border=\"1\" width=\"100%\"><tr id='rnrattr'>\
                         <th width=\"3%\">No</th><th width=\"6%\">Time</th>\
                         <th width=\"8%\">APP Name</th><th width=\"8%\">APK Name</th>\
                         <th width=\"5%\">App Version</th><th width=\"7%\">Device Model</th>\
                         <th width=\"6%\">Android Version</th><th width=\"6%\">Device Build</th><th width=\"6%\">Unmatch Image</th>\
                         <th width=\"6%\">Crash Dialog</th><th width=\"6%\">Flick Image</th><th width=\"4%\">Audio</th>\
                         <th width=\"4%\">Running</th><th width=\"4%\">Result</th><th width=\"4%\">Link</th></tr>";

        int num = 1;
        for (auto i:mRnrResult)
        {
            mHtmlContent += "<tr><td>" + boost::lexical_cast <string>(num++) + "</td>";
            mHtmlContent += "<td>" + i["time"]  + "</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>" + i["device_model"] + "</td>";
            mHtmlContent += "<td>" + i["android_version"] + "</td>";
            mHtmlContent += "<td>" + i["device_build"] + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["unmatchimage"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["crashdialog"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["flickimage"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["audio"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["running"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["result"]) + "</td>";
            mHtmlContent += "<td><a href=" + i["log"] + " target=\"_blank\"" + ">log</a></td></tr>";

        }
        mHtmlContent += "</table></div>";
    }


    void IndexReport::UpdateFPSContent()
    {
        mHtmlContent +=  "<div id=\"fpsdiv\"><p>FPS Result: <span id=\"fpspr\"></span></p>";
        mHtmlContent +=  "<table class=\"two\" border=\"1\" width=\"100%\"><tr id='fpsattr'>\
                         <th width=\"3%\">No</th><th width=\"6%\">Time</th>\
                         <th width=\"8%\">APP Name</th><th width=\"8%\">APK Name</th>\
                         <th width=\"5%\">App Version</th><th width=\"7%\">Device Model</th>\
                         <th width=\"6%\">Android Version</th><th width=\"6%\">Device Build</th>\
                         <th width=\"6%\">AVG(Trace)</th><th width=\"6%\">MAX(Trace)</th>\
                         <th width=\"6%\">MIN(Trace)</th><th width=\"6%\">AVG(Camera)</th><th width=\"6%\">MAX(Camera)</th>\
                         <th width=\"6%\">MIN(Camera)</th><th width=\"4%\">Link</th></tr>";

        int num = 1;
        for (auto i:mFpsResult)
        {
            mHtmlContent += "<tr><td>" + boost::lexical_cast <string>(num++) + "</td>";
            mHtmlContent += "<td>" + i["time"]  + "</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>" + i["device_model"] + "</td>";
            mHtmlContent += "<td>" + i["android_version"] + "</td>";
            mHtmlContent += "<td>" + i["device_build"] + "</td>";
            mHtmlContent += "<td>" + i["traceavg"] + "</td>";
            mHtmlContent += "<td>" + i["tracemax"] + "</td>";
            mHtmlContent += "<td>" + i["tracemin"]  + "</td>";
            mHtmlContent += "<td>" + i["cameraavg"] + "</td>";
            mHtmlContent += "<td>" + i["cameramax"] + "</td>";
            mHtmlContent += "<td>" + i["cameramin"]  + "</td>";
            mHtmlContent += "<td><a href=" + i["log"] + " target=\"_blank\"" + ">log</a></td></tr>";

        }

        mHtmlContent += "</table></div>";
    }

    void IndexReport::UpdateSmokeContent()
    {
        mHtmlContent +=  "<div id=\"smokediv\"><p>Smoke  Pass Rate: <span id=\"smokepr\"></span></p>";
        mHtmlContent +=  "<table class=\"two\" border=\"1\" width=\"100%\"><tr id='smokeattr'>\
                         <th width=\"3%\">No</th><th width=\"6%\">Time</th>\
                         <th width=\"8%\">APP Name</th><th width=\"8%\">APK Name</th>\
                         <th width=\"5%\">App Version</th><th width=\"7%\">Device Model</th>\
                         <th width=\"6%\">Android Version</th><th width=\"6%\">Device Build</th>\
                         <th width=\"5%\">Install</th><th width=\"5%\">Launch</th>\
                         <th width=\"5%\">Random</th><th width=\"5%\">Back</th><th width=\"5%\">Uninstall</th>\
                         <th width=\"5%\">Reason</th><th width=\"5%\">Result</th><th width=\"4%\">Link</th></tr>";

        int num = 1;
        for (auto i:mSmokeResult)
        {
            mHtmlContent += "<tr><td>" + boost::lexical_cast <string>(num++) + "</td>";
            mHtmlContent += "<td>" + i["time"]  + "</td>";
            mHtmlContent += "<td>" + i["app_name"] + "</td>";
            mHtmlContent += "<td>" + i["apk_name"] + "</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>" + i["device_model"] + "</td>";
            mHtmlContent += "<td>" + i["android_version"] + "</td>";
            mHtmlContent += "<td>" + i["device_build"] + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["Install"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["Launch"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["Random"])  + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["Back"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["Uninstall"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["reason"]) + "</td>";
            mHtmlContent += "<td>" + HighlightResult(i["result"]) + "</td>";
            mHtmlContent += "<td><a href=" + i["log"] + " target=\"_blank\"" + ">log</a></td></tr>";

        }

        mHtmlContent += "</table></div>";
    }

    void IndexReport::UpdateLaunchTimeContent()
    {
        mHtmlContent +=  "<div id=\"launchtimediv\"><p>Launch Time Result: <span id=\"launchtimepr\"></span></p>";
        mHtmlContent +=  "<table class=\"two\" border=\"1\" width=\"100%\"><tr id='launchtimeattr'>\
                         <th width=\"3%\">No</th><th width=\"6%\">Time</th>\
                         <th width=\"8%\">APP Name</th><th width=\"8%\">APK Name</th>\
                         <th width=\"5%\">App Version</th><th width=\"7%\">Device Model</th>\
                         <th width=\"6%\">Android Version</th><th width=\"6%\">Device Build</th>\
                         <th width=\"6%\">Before Image</th><th width=\"6%\">Reference Image</th>\
                         <th width=\"6%\">Target Image</th><th width=\"6%\">Launch Time</th>\
                         <th width=\"5%\">Result</th><th width=\"4%\">Link</th></tr>";

        int num = 1;
        for (auto i:mLaunchtimeResult)
        {
            mHtmlContent += "<tr><td>" + boost::lexical_cast <string>(num++) + "</td>";
            mHtmlContent += "<td>" + i["time"]  + "</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>N/A</td>";
            mHtmlContent += "<td>" + i["device_model"] + "</td>";
            mHtmlContent += "<td>" + i["android_version"] + "</td>";
            mHtmlContent += "<td>" + i["device_build"] + "</td>";
            mHtmlContent += "<td><a href=" + i["log"] + "\\..\\" + i["beforeimageurl"]+ " target=\"_blank\"" + ">" + i["beforeimagename"] + "</a></td>";
            mHtmlContent += "<td><a href=" + i["log"] + "\\..\\" + i["referenceimageurl"] + " target=\"_blank\"" + ">" + i["referenceimagename"] + "</a></td>";
            mHtmlContent += "<td><a href=" + i["log"] + "\\..\\" + i["targetimageurl"]+ " target=\"_blank\"" + ">" + i["targetimagename"] + "</a></td>";
            mHtmlContent += "<td>" + i["launchtime"] + "</td>";
            mHtmlContent += "<td>N/A</td>";

            mHtmlContent += "<td><a href=" + i["log"] + " target=\"_blank\"" + ">log</a></td></tr>";

        }

        mHtmlContent += "</table></div>";
    }

    DaVinciStatus IndexReport::GenerateReportIndex()
    {
        boost::filesystem::path log_path(mFolderPath);
        log_path /= "_Logs";
        //Check Log folder existence
        if (boost::filesystem::exists(log_path))
        {
            vector<boost::filesystem::path> tsfolder;
            getAllTSFolders(log_path, tsfolder);

            if (tsfolder.size() > 0)
            {
                ParseFolder(tsfolder);
            }

            if(mRnrResult.size() == 0
                && mSmokeResult.size() == 0
                && mFpsResult.size() == 0
                && mLaunchtimeResult.size() == 0)
            {
                return DaVinciStatus(errc::message_size);
            }

            if(mRnrResult.size() > 0)
            {
                UpdateRnRContent();
            }

            if(mSmokeResult.size() > 0 )
            {
                UpdateSmokeContent();
            }

            if(mFpsResult.size() > 0)
            {
                UpdateFPSContent();
            }

            if(mLaunchtimeResult.size() > 0)
            {
                UpdateLaunchTimeContent();
            }
            mHtmlContent += "</body></html>";
            ofstream indexfile( mFolderPath + "\\_Logs\\index.html", ios::out);
            indexfile<<mHtmlContent;
            indexfile.flush();
            indexfile.close();
        }

        return DaVinciStatusSuccess;
    }
}
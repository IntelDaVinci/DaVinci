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

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "BiasFileReader.hpp"
#include "boost/algorithm/string.hpp"

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

using namespace std;
using namespace xercesc;

namespace DaVinci
{
    BiasFileReader::BiasFileReader()
    {
    }

    void BiasFileReader::LoadSeed(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->seedValue = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadTimeout(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->timeOut = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadLaunchWaitTime(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->launchWaitTime = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadActionWaitTime(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->actionWaitTime = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadRandomAfterUnmatchAction(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->randomAfterUnmatchAction = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadMaxRandomAction(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->maxRandomAction = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadUnmatchRatio(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                biasInfo->unmatchRatio = boost::lexical_cast<double>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadMatchPattern(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();
        const XMLSize_t attributeCount = attributes->getLength();

        for (XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                string value = XMLStrToStr(attribute->getNodeValue());
                boost::to_lower(value);
                biasInfo->matchPattern = value;
            }
        }
    }

    void BiasFileReader::LoadRegion(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();
        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                std::string regionStr = XMLStrToStr(attribute->getNodeValue());
                int index1, index2;
                index1 = (int)regionStr.find(')',0);
                index2 = (int)regionStr.find('(',1);
                std::string ltPoint = regionStr.substr(1,index1 - 1);
                std::string rbPoint = regionStr.substr(index2 + 1,regionStr.length() - 2 - index2);
                std::vector<std::string> ltPointPair;
                boost::algorithm::split(ltPointPair, ltPoint, boost::algorithm::is_any_of(","));
                std::vector<std::string> rbPointPair;
                boost::algorithm::split(rbPointPair, rbPoint, boost::algorithm::is_any_of(","));
                biasInfo->leftTopPoint.x  = boost::lexical_cast<int>(ltPointPair[0]);
                biasInfo->leftTopPoint.y = boost::lexical_cast<int>(ltPointPair[1]);
                biasInfo->rightBottomPoint.x = boost::lexical_cast<int>(rbPointPair[0]);
                biasInfo->rightBottomPoint.y = boost::lexical_cast<int>(rbPointPair[1]);
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadTestBasic(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            std::string xaName = XMLStrToStr(attribute->getNodeName());
            std::string xaValue = XMLStrToStr(attribute->getNodeValue());
            std::string xaValueCopy = xaValue; // Keep a copy, because app name is case sensitive
            boost::to_upper(xaValue);

            if (boost::equals(xaName, "type"))
            {
                biasInfo->testType = xaValue;
            }
            else if (boost::equals(xaName, "output"))
            {
                biasInfo->output = xaValue;
            }
            else if (boost::equals(xaName, "coverage"))
            {
                if (boost::starts_with(xaValue, "Y"))
                {
                    biasInfo->coverage = true;
                }
                else
                {
                    biasInfo->coverage = false;
                }
            }
            else if (boost::equals(xaName, "mode"))
            {
                if (boost::equals(xaValue, "OBJECT"))
                {
                    biasInfo->objectMode = true;
                }
                else
                {
                    biasInfo->objectMode = false;
                }
            }
            else if (boost::equals(xaName, "compare"))
            {
                if (boost::starts_with(xaValue, "Y"))
                {
                    biasInfo->compare = true;
                }
                else
                {
                    biasInfo->compare = false;
                }
            }
            else if (boost::equals(xaName, "silent"))
            {
                if (boost::starts_with(xaValue, "Y"))
                {
                    biasInfo->silent = true;
                }
                else
                {
                    biasInfo->silent = false;
                }
            }
            else if (boost::equals(xaName, "download"))
            {
                if (boost::starts_with(xaValue, "Y"))
                {
                    biasInfo->download = true;
                }
                else
                {
                    biasInfo->download = false;
                }
            }
            else if (boost::equals(xaName, "price"))
            {
                TryParse(xaValue, biasInfo->price);
            }
            else if (boost::equals(xaName, "app"))
            {
                biasInfo->app = xaValueCopy;
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadTestLog(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild)
    {
        DOMNodeList* subSubChildren = testChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());

                if (boost::equals(xaName,"name"))
                {
                    biasInfo->logcatPackageNames.push_back(XMLStrToStr(xa->getNodeValue()));
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void BiasFileReader::LoadTestLaunch(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild)
    {
        DOMNodeList* subSubChildren = testChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            std::string launchType = "";
            std::string launchProb = "";

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());

                if (boost::equals(xaName,"value"))
                {
                    launchProb = XMLStrToStr(xa->getNodeValue());
                }
                else if (boost::equals(xaName,"type"))
                {
                    launchType = XMLStrToStr(xa->getNodeValue());
                }
                else
                {
                    continue;
                }
            }
            if (launchType == "Command")
            {
                boost::trim_right_if(launchProb, boost::algorithm::is_any_of("%"));
                biasInfo->cmdLaunchProbability = boost::lexical_cast<double>(launchProb) / 100;
            }
            else if (launchType == "Icon")
            {
                boost::trim_right_if(launchProb, boost::algorithm::is_any_of("%"));
                biasInfo->iconLaunchProbability = boost::lexical_cast<double>(launchProb) / 100;
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadTestAccount(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue)
    {
        boost::to_upper(testChildValue);

        if (boost::starts_with(testChildValue, "Y"))
        {
            biasInfo->needAccount = true;
            int i = 0;
            DOMNodeList* subSubChildren = testChild->getChildNodes();
            const XMLSize_t subSubNodeCount = subSubChildren->getLength();

            for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
            {
                DOMNode* ch = subSubChildren->item(ix);
                if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                    continue;

                DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();
                std::vector<string> accountInfo = vector<string>();
                std::string userNameInfo = "";
                std::string pswdInfo = "";
                std::string miscInfo = "";

                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "name"))
                    {
                        userNameInfo = XMLStrToStr(xa->getNodeValue());
                    }
                    else if (boost::equals(xaName, "password"))
                    {
                        pswdInfo = XMLStrToStr(xa->getNodeValue());
                    }
                    else if (boost::equals(xaName, "misc"))
                    {
                        miscInfo = XMLStrToStr(xa->getNodeValue());
                    }
                    else
                    {
                        continue;
                    }
                    if (userNameInfo != "" && pswdInfo != "")
                    {
                        accountInfo.push_back(userNameInfo);
                        accountInfo.push_back(pswdInfo);
                        biasInfo->acountMap[i] = accountInfo;
                        ++i;
                    }
                }
            }
        }
        else
        {
            biasInfo->needAccount = false;
        }
    }

    void BiasFileReader::LoadTestClick(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue)
    {
        boost::trim_right_if(testChildValue, boost::algorithm::is_any_of("%"));
        double testChildValueDouble = 0.0;
        TryParse(testChildValue, testChildValueDouble);
        biasInfo->clickPossibility = testChildValueDouble / 100;

        DOMNodeList* subSubChildren = testChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* testSubChild = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(testSubChild->getNodeName()), "#text"))
                continue;

            std::string testSubChildType = "";
            std::string testSubChildValue = "";
            double testSubChildValueDouble = 0.0;

            DOMNamedNodeMap* subSubAttributes = testSubChild->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());
                if (boost::equals(xaName, "type"))
                {
                    testSubChildType = XMLStrToStr(xa->getNodeValue());
                }
                else if (boost::equals(xaName, "value"))
                {
                    testSubChildValue = XMLStrToStr(xa->getNodeValue());
                    boost::replace_all(testSubChildValue, "%", "");
                    TryParse(testSubChildValue, testSubChildValueDouble);
                }
                else
                {
                    continue;
                }
            }
            if (testSubChildType == "PLAIN")
            {
                biasInfo->clickButton = testSubChildValueDouble / 100.00;
            }
            else if (testSubChildType == "HOME")
            {
                biasInfo->clickHome = testSubChildValueDouble / 100.00;
            }
            else if (testSubChildType == "MENU")
            {
                biasInfo->clickMenu = testSubChildValueDouble/ 100.00;
            }
            else if (testSubChildType == "BACK")
            {
                biasInfo->clickBack = testSubChildValueDouble / 100.00;
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadTestAudio(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue)
    {
        boost::to_upper(testChildValue);

        if (boost::starts_with(testChildValue, "Y"))
        {
            biasInfo->needAudio = true;
        }
        else
        {
            biasInfo->needAudio = false;
        }
    }

    void BiasFileReader::LoadTestSwipe(boost::shared_ptr<BiasInfo> biasInfo, string& testChildValue)
    {
        boost::trim_right_if(testChildValue, boost::algorithm::is_any_of("%"));
        double testChildValueDouble = 0.0;
        TryParse(testChildValue, testChildValueDouble);
        biasInfo->swipePossibility = testChildValueDouble / 100;
    }

    void BiasFileReader::LoadTextUrl(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble)
    {
        biasInfo->urlPossibility = testSubChildValueDouble / 100.00;
        DOMNodeList* subSubChildren = testSubChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());
                if (boost::equals(xaName, "value"))
                {
                    biasInfo->urlList.push_back(XMLStrToStr(xa->getNodeValue()));
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void BiasFileReader::LoadTextMail(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble)
    {
        biasInfo->mailPossibility = testSubChildValueDouble / 100.00;
        DOMNodeList* subSubChildren = testSubChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());
                if (boost::equals(xaName, "value"))
                {
                    biasInfo->mailList.push_back(XMLStrToStr(xa->getNodeValue()));
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void BiasFileReader::LoadTextPlain(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testSubChild, double testSubChildValueDouble)
    {
        biasInfo->plainPossibility = testSubChildValueDouble / 100.00;
        DOMNodeList* subSubChildren = testSubChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());
                if (boost::equals(xaName, "value"))
                {
                    biasInfo->plainList.push_back(XMLStrToStr(xa->getNodeValue()));
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void BiasFileReader::LoadTestText(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild, string& testChildValue)
    {
        boost::trim_right_if(testChildValue, boost::algorithm::is_any_of("%"));
        double testChildValueDouble = 0.0;
        TryParse(testChildValue, testChildValueDouble);
        biasInfo->textPossibility = testChildValueDouble / 100;

        DOMNodeList* subSubChildren = testChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* testSubChild = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(testSubChild->getNodeName()), "#text"))
                continue;

            std::string testSubChildType = "";
            std::string testSubChildValue = "";
            double testSubChildValueDouble = 0.0;                             

            DOMNamedNodeMap* subSubAttributes = testSubChild->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());

                if (boost::equals(xaName, "type"))
                {
                    testSubChildType = XMLStrToStr(xa->getNodeValue());
                }
                else if (boost::equals(xaName, "value"))
                {
                    testSubChildValue = XMLStrToStr(xa->getNodeValue());
                    boost::replace_all(testSubChildValue, "%", "");
                    TryParse(testSubChildValue, testSubChildValueDouble);
                }
                else
                {
                    continue;
                }
            }
            if (testSubChildType == "URL")
            {
                LoadTextUrl(biasInfo, testSubChild, testSubChildValueDouble);
            }
            else if (testSubChildType == "MAIL")
            {
                LoadTextMail(biasInfo, testSubChild, testSubChildValueDouble);
            }
            else if (testSubChildType == "PLAIN")
            {
                LoadTextPlain(biasInfo, testSubChild, testSubChildValueDouble);
            }
        }
    }

    void BiasFileReader::LoadTestKeyword(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* testChild)
    {
        DOMNodeList* subSubChildren = testChild->getChildNodes();
        const XMLSize_t subSubNodeCount = subSubChildren->getLength();

        for(XMLSize_t ix = 0; ix < subSubNodeCount; ++ix)
        {
            DOMNode* ch = subSubChildren->item(ix);
            if(boost::equals(XMLStrToStr(ch->getNodeName()), "#text"))
                continue;

            DOMNamedNodeMap* subSubAttributes = ch->getAttributes();
            const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
            {
                DOMNode* xa = subSubAttributes->item(ix);

                std::string xaName = XMLStrToStr(xa->getNodeName());
                if (boost::equals(xaName, "value"))
                {
                    biasInfo->keywords.push_back(XMLStrToStr(xa->getNodeValue()));
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void BiasFileReader::LoadTest(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        LoadTestBasic(biasInfo, child);

        DOMNodeList* subChildren = child->getChildNodes();
        const XMLSize_t subNodeCount = subChildren->getLength();

        for(XMLSize_t ix = 0; ix < subNodeCount; ++ix)
        {
            DOMNode* testChild = subChildren->item(ix);
            if(boost::equals(XMLStrToStr(testChild->getNodeName()), "#text"))
                continue;

            std::string testChildType = "";
            std::string testChildValue = "";
            std::string testChildClear = "";

            DOMNamedNodeMap* subAttributes = testChild->getAttributes();
            const XMLSize_t subAttributeCount = subAttributes->getLength();

            for(XMLSize_t ix = 0; ix < subAttributeCount; ++ix)
            {
                DOMNode* xa = subAttributes->item(ix);
                std::string xaName = XMLStrToStr(xa->getNodeName());

                if (boost::equals(xaName, "type"))
                {
                    testChildType = XMLStrToStr(xa->getNodeValue());
                }
                else if (boost::equals(xaName, "value"))
                {
                    testChildValue = XMLStrToStr(xa->getNodeValue());
                    boost::replace_all(testChildValue, "%", "");
                }
                else if (boost::equals(xaName, "clear"))
                {
                    testChildClear = XMLStrToStr(xa->getNodeValue());
                    boost::replace_all(testChildClear, "%", "");
                }
                else
                {
                    continue;
                }
            }

            // Collect logcat based on customized package name
            if (testChildType == "Log")
            {
                LoadTestLog(biasInfo, testChild);
            }
            else if (testChildType == "Launch")
            {
                LoadTestLaunch(biasInfo, testChild);
            }
            else if (testChildType == "ACCOUNT")
            {
                LoadTestAccount(biasInfo, testChild, testChildValue);
            }
            else if (testChildType == "AUDIO")
            {
                LoadTestAudio(biasInfo, testChild, testChildValue);
            }
            else if (testChildType == "SWIPE")
            {
                LoadTestSwipe(biasInfo, testChildValue);
            }
            else if (testChildType == "CLICK")
            {
                LoadTestClick(biasInfo, testChild, testChildValue);
            }
            else if (testChildType == "TEXT")
            {
                LoadTestText(biasInfo, testChild, testChildValue);
            }
            else if (testChildType == "KEYWORD")
            {
                LoadTestKeyword(biasInfo, testChild);
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadScripts(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "staticActionNumber"))
            {
                biasInfo->staticActionNumber = boost::lexical_cast<int>(XMLStrToStr(attribute->getNodeValue()).c_str());
            }
            else
            {
                continue;
            }
        }

        DOMNodeList* children = child->getChildNodes();
        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "Script"))
            {
                DOMNamedNodeMap* subSubAttributes = child->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

                string qsName, qsPriority = "LOW";
                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "name"))
                    {
                        qsName = XMLStrToStr(xa->getNodeValue());
                    }
                    else if (boost::equals(xaName, "priority"))
                    {
                        qsPriority = XMLStrToStr(xa->getNodeValue());
                    }
                }
                biasInfo->qscripts.push_back(pair<string, string>(qsName, qsPriority));
            }
        }
    }

    void BiasFileReader::LoadActionPossibility(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNodeList* children = child->getChildNodes();
        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "CommonKeywordPossibility"))
            {
                DOMNamedNodeMap* subSubAttributes = child->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "value"))
                    {
                        string keywordPos = XMLStrToStr(xa->getNodeValue());
                        boost::trim_right_if(keywordPos, boost::algorithm::is_any_of("%"));
                        double keywordPosDouble = 0.5;      // default value is 50%
                        TryParse(keywordPos, keywordPosDouble);
                        biasInfo->dispatchCommonKeywordPossibility = keywordPosDouble / 100; 
                    }
                }
            }

            if (boost::equals(nodeName, "AdsPossibility"))
            {
                DOMNamedNodeMap* subSubAttributes = child->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "value"))
                    {
                        string adsPos = XMLStrToStr(xa->getNodeValue());
                        boost::trim_right_if(adsPos, boost::algorithm::is_any_of("%"));
                        double adsPosDouble = 0.5;      // default value is 50%
                        TryParse(adsPos, adsPosDouble);
                        biasInfo->dispatchAdsPossibility = adsPosDouble / 100; 
                    }
                }
            }
        }
    }

    void BiasFileReader::LoadCheckers(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNodeList* children = child->getChildNodes();
        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "Checker"))
            {
                DOMNamedNodeMap* subSubAttributes = child->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

                FailureType type = FailureType::WarMsgMax;
                int condition = -1;
                bool enabled = true;
                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "name"))
                    {
                        type = StringToFailureType(XMLStrToStr(xa->getNodeValue()));
                    }
                    else if (boost::equals(xaName, "enable"))
                    {
                        string enabledStr = XMLStrToStr(xa->getNodeValue());
                        boost::to_upper(enabledStr);
                        if(enabledStr == "FALSE")
                            enabled = false;
                    }
                    else if (boost::equals(xaName, "value"))
                    {
                        TryParse(XMLStrToStr(xa->getNodeValue()), condition);
                    }
                }
                if(type == FailureType::WarMsgMax)
                {
                    DAVINCI_LOG_WARNING << "Unsupported checker found.";
                }
                else
                {
                    biasInfo->checkers[type] = CheckerInfo(enabled, condition);
                }
            }
        }
    }

    void BiasFileReader::LoadEvents(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNodeList* children = child->getChildNodes();
        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "Event"))
            {
                DOMNamedNodeMap* subSubAttributes = child->getAttributes();
                const XMLSize_t subSubAttributeCount = subSubAttributes->getLength();

                EventType type = EventType::EventMax;
                int number = -1;
                bool enabled = true;
                for(XMLSize_t ix = 0; ix < subSubAttributeCount; ++ix)
                {
                    DOMNode* xa = subSubAttributes->item(ix);

                    std::string xaName = XMLStrToStr(xa->getNodeName());
                    if (boost::equals(xaName, "name"))
                    {
                        type = StringToEventType(XMLStrToStr(xa->getNodeValue()));
                    }
                    else if (boost::equals(xaName, "enable"))
                    {
                        string enabledStr = XMLStrToStr(xa->getNodeValue());
                        boost::to_upper(enabledStr);
                        if(enabledStr == "FALSE")
                            enabled = false;
                    }
                    else if (boost::equals(xaName, "value"))
                    {
                        TryParse(XMLStrToStr(xa->getNodeValue()), number);
                    }
                }
                if(type == EventType::EventMax)
                {
                    DAVINCI_LOG_WARNING << "Unsupported event found.";
                }
                else
                {
                    biasInfo->events[type] = EventInfo(enabled, number);
                }
            }
        }
    }

    void BiasFileReader::LoadVideoRecording(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                string value = XMLStrToStr(attribute->getNodeValue());
                boost::to_upper(value);
                biasInfo->videoRecording = boost::equals(value, "TRUE");
            }
            else
            {
                continue;
            }
        }
    }

    void BiasFileReader::LoadAudioSaving(boost::shared_ptr<BiasInfo> biasInfo, DOMNode* child)
    {
        DOMNamedNodeMap* attributes = child->getAttributes();

        const XMLSize_t attributeCount = attributes->getLength();

        for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
        {
            DOMNode* attribute = attributes->item(ix);
            if (boost::equals(attribute->getNodeName(), "value"))
            {
                string value = XMLStrToStr(attribute->getNodeValue());
                boost::to_upper(value);
                biasInfo->audioSaving = boost::equals(value, "TRUE");
            }
            else
            {
                continue;
            }
        }
    }

    boost::shared_ptr<BiasObject> BiasFileReader::loadDavinciConfig(const std::string &xmlPath)
    {
        boost::shared_ptr<BiasInfo> biasInfo = boost::shared_ptr<BiasInfo>(new BiasInfo());
        // Initialize boolean flags
        biasInfo->objectMode = false;
        biasInfo->needAccount = false;
        biasInfo->coverage = false;
        biasInfo->needAudio = false;
        biasInfo->compare = false;
        biasInfo->silent = false;
        biasInfo->download = false;
        biasInfo->timeOut = 600;
        biasInfo->launchWaitTime = 30;
        biasInfo->actionWaitTime = 5;
        biasInfo->randomAfterUnmatchAction = 3;
        biasInfo->maxRandomAction = 3;
        biasInfo->unmatchRatio = 0.5;
        biasInfo->staticActionNumber = 0;
        biasInfo->videoRecording = false;
        biasInfo->audioSaving = false;
        biasInfo->matchPattern = "";
        biasInfo->seedValue = -1;
        biasInfo->dispatchCommonKeywordPossibility = 0.5;
        biasInfo->dispatchAdsPossibility = 0.5;
        biasInfo->checkers[FailureType::WarMsgAbnormalPage] = CheckerInfo(true, 3);
        biasInfo->checkers[FailureType::WarMsgErrorDialog] = CheckerInfo(true, 0);
        biasInfo->checkers[FailureType::WarMsgNetwork] = CheckerInfo(true, 0);
        biasInfo->checkers[FailureType::WarMsgAbnormalFont] = CheckerInfo(true, 0);

        boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        xmlParser->parse(xmlPath.c_str());

        boost::shared_ptr<DOMDocument> xmlDoc = boost::shared_ptr<DOMDocument>(xmlParser->getDocument(), boost::null_deleter());
        if(xmlDoc == nullptr)
            return nullptr;

        boost::shared_ptr<DOMElement> elementRoot = boost::shared_ptr<DOMElement>(xmlDoc->getDocumentElement(), boost::null_deleter());
        if(elementRoot == nullptr)
            return nullptr;

        boost::shared_ptr<DOMNodeList> children = boost::shared_ptr<DOMNodeList>(elementRoot->getChildNodes(), boost::null_deleter());
        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "Seed"))
            {
                LoadSeed(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Timeout"))
            {
                LoadTimeout(biasInfo, child);
            }
            else if (boost::equals(nodeName, "LaunchWaitTime"))
            {
                LoadLaunchWaitTime(biasInfo, child);
            }
            else if (boost::equals(nodeName, "ActionWaitTime"))
            {
                LoadActionWaitTime(biasInfo, child);
            }
            else if (boost::equals(nodeName, "RandomAfterUnmatchAction"))
            {
                LoadRandomAfterUnmatchAction(biasInfo, child);
            }
            else if (boost::equals(nodeName, "MaxRandomAction"))
            {
                LoadMaxRandomAction(biasInfo, child);
            }
            else if (boost::equals(nodeName, "UnmatchRatio"))
            {
                LoadUnmatchRatio(biasInfo, child);
            }
            else if (boost::equals(nodeName, "MatchPattern"))
            {
                LoadMatchPattern(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Region"))
            {
                LoadRegion(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Test"))
            {
                LoadTest(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Scripts"))
            {
                LoadScripts(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Checkers"))
            {
                LoadCheckers(biasInfo, child);
            }
            else if (boost::equals(nodeName, "ActionPossibility"))
            {
                LoadActionPossibility(biasInfo, child);
            }
            else if (boost::equals(nodeName, "Events"))
            {
                LoadEvents(biasInfo, child);
            }
            else if (boost::equals(nodeName, "VideoRecording"))
            {
                LoadVideoRecording(biasInfo, child);
            }
            else if(boost::equals(nodeName, "AudioSaving"))
            {
                LoadAudioSaving(biasInfo, child);   
            }
            else
            {
                continue;
            }
        }

        if(biasInfo->seedValue == -1)
            return nullptr;
        else
            return (boost::shared_ptr<BiasObject>)(new BiasObject(biasInfo));
    }
}

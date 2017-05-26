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

#include "DaVinciCommon.hpp"
#include "UiStateCoverage.hpp"

#include "boost/algorithm/string.hpp"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

using namespace std;
using namespace xercesc;

namespace DaVinci
{
    UiStateCoverage::UiStateCoverage()
    {
        stateHashObjectsDict = std::unordered_map<std::string, std::vector<std::string>>();
        stateHashObjNumDict = std::unordered_map<std::string, int>();
        stateHashActDict = std::unordered_map<std::string, std::vector<std::string>>();
    }

    UiStateCoverage::UiStateCoverage(const std::string &covFile)
    {
        stateHashObjectsDict = std::unordered_map<std::string, std::vector<std::string>>();
        stateHashObjNumDict = std::unordered_map<std::string, int>();
        stateHashActDict = std::unordered_map<std::string, std::vector<std::string>>();
        initHashDict(covFile);
    }

    void UiStateCoverage::initHashDict(const std::string &covFile)
    {
        DAVINCI_LOG_WARNING << std::string(covFile);
        boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        xmlParser->parse(covFile.c_str());

        DOMDocument* globalXmlCov = xmlParser->getDocument();
        DOMElement* globalXmlRoot = globalXmlCov->getDocumentElement(); 
        DOMNodeList* children = globalXmlRoot->getChildNodes();

        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            DOMNode* child = children->item(ix);
            nodeName = XMLStrToStr(child->getNodeName());

            if (boost::equals(nodeName, "#text")) continue;

            //Get the hash code of one state
            std::string stateHashStr = "";
            std::string stateObjNum = "";
            std::vector<std::string> objList = std::vector<std::string>();
            std::vector<std::string> actList = std::vector<std::string>();
            std::string objHashStr = "";
            std::string actStr = "";

            DOMNamedNodeMap* attributes = child->getAttributes();

            const XMLSize_t attributeCount = attributes->getLength();

            for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
            {
                DOMNode* attribute = attributes->item(ix);
                if (boost::equals(attribute->getNodeName(), "Hash"))
                {
                    stateHashStr = XMLStrToStr(attribute->getNodeValue());
                }

                if (boost::equals(attribute->getNodeName(), "ObjectNum"))
                {
                    stateObjNum = XMLStrToStr(attribute->getNodeValue());
                }
            }
            stateHashObjNumDict.insert(make_pair(stateHashStr, boost::lexical_cast<int>(stateObjNum)));
            stateHashActDict.insert(make_pair(stateHashStr,actList));
            stateHashObjectsDict.insert(make_pair(stateHashStr,objList));


            DOMNodeList* subChildren = child->getChildNodes();
            const XMLSize_t subNodeCount = subChildren->getLength();

            for(XMLSize_t ix = 0; ix < subNodeCount; ++ix)
            {
                DOMNode* subChild = subChildren->item(ix);
                if(boost::equals(XMLStrToStr(subChild->getNodeName()), "#text"))
                    continue;

                DOMNamedNodeMap* subAttributes = subChild->getAttributes();

                const XMLSize_t subAttributeCount = subAttributes->getLength();

                for(XMLSize_t ix = 0; ix < subAttributeCount; ++ix)
                {
                    DOMNode* subAttribute = subAttributes->item(ix);
                    if (boost::equals(subAttribute->getNodeName(), "Hash"))
                    {
                        objHashStr = XMLStrToStr(subAttribute->getNodeValue());
                    }

                    if (boost::equals(subAttribute->getNodeName(), "Action"))
                    {
                        actStr = XMLStrToStr(subAttribute->getNodeValue());
                    }
                }
                assert(actStr != "");

                if (boost::starts_with(actStr, "C"))
                {
                    if (std::find(stateHashActDict[stateHashStr].begin(), stateHashActDict[stateHashStr].end(), "Click") == stateHashActDict[stateHashStr].end())
                    {
                        stateHashActDict[stateHashStr].push_back("Click");
                    }
                }
                else if (boost::starts_with(actStr, "Sw"))
                {
                    if (std::find(stateHashActDict[stateHashStr].begin(), stateHashActDict[stateHashStr].end(), "Swipe") == stateHashActDict[stateHashStr].end())
                    {
                        stateHashActDict[stateHashStr].push_back("Swipe");
                    }
                }
                else if (boost::starts_with(actStr, "Se"))
                {
                    if (std::find(stateHashActDict[stateHashStr].begin(), stateHashActDict[stateHashStr].end(), "Set Text") == stateHashActDict[stateHashStr].end())
                    {
                        stateHashActDict[stateHashStr].push_back("Set Text");
                    }
                }
                else
                {
                    continue;
                }

                assert (objHashStr != "" && objHashStr != "0");

                if (std::find(stateHashObjectsDict[stateHashStr].begin(), stateHashObjectsDict[stateHashStr].end(), objHashStr) == stateHashObjectsDict[stateHashStr].end())
                {
                    stateHashObjectsDict[stateHashStr].push_back(objHashStr);
                }

            }
        }
    }

    double UiStateCoverage::coverObjectCalc()
    {
        int totalObjects = 0;
        double objectsCovered = 0;
        for (auto ls : stateHashObjNumDict)
        {
            totalObjects += stateHashObjNumDict[ls.first];
            objectsCovered += stateHashObjectsDict[ls.first].size();
        }

        if (totalObjects != 0)
        {
            return objectsCovered / totalObjects;
        }
        else
        {
            DAVINCI_LOG_INFO << std::string("No objects...") << std::endl;
            return -1;
        }
    }

    double UiStateCoverage::coverActionCalc()
    {
        int totalActions = 0;
        double actionCovered = 0;
        totalActions = (int)stateHashObjNumDict.size();
        totalActions *= 3;
        if (totalActions == 0)
        {
            DAVINCI_LOG_INFO << std::string("Fatal Error:No Action!") << std::endl;
            return -1;
        }
        for (auto ls : stateHashActDict)
        {
            actionCovered += stateHashActDict[ls.first].size();
        }
        return actionCovered / totalActions;
    }

    void UiStateCoverage::showStateObjecNumDict()
    {
        for (auto ls : stateHashObjNumDict)
        {
            DAVINCI_LOG_INFO << std::string("Key: ") << ls.first << std::string(" Value: ") << stateHashObjNumDict[ls.first] << std::endl;
        }
    }

    void UiStateCoverage::showStateObjectDict()
    {
        for (auto pair : stateHashObjectsDict)
        {
            DAVINCI_LOG_INFO << std::string("Key: ") << pair.first << std::endl;
            int size = (int)pair.second.size();
            for (int i = 0; i < size; i++)
            {
                DAVINCI_LOG_INFO << std::string("     Object: ") << pair.second.at(i) << std::endl;
            }
        }
    }

    void UiStateCoverage::showStateActionDict()
    {
        for (auto pair : stateHashActDict)
        {
            DAVINCI_LOG_INFO << std::string("Key: ") << pair.first << std::endl;
            int size = (int)pair.second.size();
            for (int i = 0; i < size; i++)
            {
                DAVINCI_LOG_INFO << std::string("     Action: ") << pair.second.at(i) << std::endl;
            }
        }
    }
}

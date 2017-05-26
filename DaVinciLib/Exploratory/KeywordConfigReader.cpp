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

#include <boost/algorithm/string.hpp>
#include <boost/core/null_deleter.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>

#include "DaVinciCommon.hpp"
#include "KeywordConfigReader.hpp"

namespace DaVinci
{
    KeywordConfigReader::KeywordConfigReader()
    {
    }

    std::map<std::string, std::vector<std::string>> KeywordConfigReader::ReadApplistConfig(const std::string &xmlPath)
    {
        std::map<std::string, std::vector<std::string>> configApplistMap;

        boost::shared_ptr<XercesDOMParser> parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        parser->parse(xmlPath.c_str());
        boost::shared_ptr<DOMDocument> doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
        if (doc == nullptr)
        {
            return configApplistMap;
        }

        boost::shared_ptr<DOMElement> root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

        if (root == nullptr)
        {
            return configApplistMap;
        }

        boost::shared_ptr<DOMNodeList> categoryNodes = boost::shared_ptr<DOMNodeList>(root->getChildNodes(), boost::null_deleter());
        const XMLSize_t categoryNodeCount = categoryNodes->getLength();

        for (XMLSize_t i = 0; i < categoryNodeCount; i++)
        {
            boost::shared_ptr<DOMNode> categoryNode = boost::shared_ptr<DOMNode>(categoryNodes->item(i), &XmlDocumentPtrDeleter<DOMNode>);
            std::string categoryNodeName = XMLStrToStr(categoryNode->getNodeName());
            if (boost::equals(categoryNodeName, "Category"))
            {
                std::string key = "";
                std::vector<string> appList;

                boost::shared_ptr<DOMNamedNodeMap> categoryNodeAttributes = boost::shared_ptr<DOMNamedNodeMap>(categoryNode->getAttributes(), boost::null_deleter());
                const XMLSize_t categoryNodeAttributeCount = categoryNodeAttributes->getLength();

                for (XMLSize_t j = 0; j < categoryNodeAttributeCount; j++)
                {
                    boost::shared_ptr<DOMNode> categoryNodeAttribute = boost::shared_ptr<DOMNode>(categoryNodeAttributes->item(j), &XmlDocumentPtrDeleter<DOMNode>);
                    std::string categoryNodeAttributeName = XMLStrToStr(categoryNodeAttribute->getNodeName());

                    if (boost::equals(categoryNodeAttributeName, "type"))
                    {
                        key = XMLStrToStr(categoryNodeAttribute->getNodeValue());
                        break;
                    }
                }

                boost::shared_ptr<DOMNodeList> appListNodes = boost::shared_ptr<DOMNodeList>(categoryNode->getChildNodes(), boost::null_deleter());
                const XMLSize_t appListNodeCount = appListNodes->getLength();

                for (XMLSize_t j = 0; j < appListNodeCount; j++)
                {
                    boost::shared_ptr<DOMNode> appListNode = boost::shared_ptr<DOMNode>(appListNodes->item(j), &XmlDocumentPtrDeleter<DOMNode>);
                    std::string appListNodeName = XMLStrToStr(appListNode->getNodeName());

                    if (boost::equals(appListNodeName, "AppName"))
                    {
                        boost::shared_ptr<DOMNamedNodeMap> appNodeAttributes = boost::shared_ptr<DOMNamedNodeMap>(appListNode->getAttributes(), boost::null_deleter());
                        const XMLSize_t appNodeAttributeCount = appNodeAttributes->getLength();

                        for (XMLSize_t t = 0; t < appNodeAttributeCount; t++)
                        {
                            boost::shared_ptr<DOMNode> appNodeAttribute = boost::shared_ptr<DOMNode>(appNodeAttributes->item(t), &XmlDocumentPtrDeleter<DOMNode>);
                            std::string appNodeAttributeName = XMLStrToStr(appNodeAttribute->getNodeName());

                            if (boost::equals(appNodeAttributeName, "value"))
                            {
                                appList.push_back(XMLStrToStr(appNodeAttribute->getNodeValue()));
                            }
                        }
                    }
                }
                configApplistMap[key] = appList;
            }
        }
        return configApplistMap;
    }

    std::map<std::string, std::map<std::string, std::vector<Keyword>>> KeywordConfigReader::ReadKeywordConfig(const std::string &xmlPath)
    {
        std::map<std::string, std::map<std::string, std::vector<Keyword>>> configKeywordMap;

        boost::shared_ptr<XercesDOMParser> parser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        parser->parse(xmlPath.c_str());
        boost::shared_ptr<DOMDocument> doc = boost::shared_ptr<DOMDocument>(parser->getDocument(), boost::null_deleter());
        if (doc == nullptr)
        {
            return configKeywordMap;
        }

        boost::shared_ptr<DOMElement> root = boost::shared_ptr<DOMElement>(doc->getDocumentElement(), boost::null_deleter());

        if (root == nullptr)
        {
            return configKeywordMap;
        }

        boost::shared_ptr<DOMNodeList> categoryNodes = boost::shared_ptr<DOMNodeList>(root->getChildNodes(), boost::null_deleter());
        const XMLSize_t categoryNodeCount = categoryNodes->getLength();

        for (XMLSize_t i = 0; i < categoryNodeCount; i++)
        {
            boost::shared_ptr<DOMNode> categoryNode = boost::shared_ptr<DOMNode>(categoryNodes->item(i), &XmlDocumentPtrDeleter<DOMNode>);
            std::string categoryNodeName = XMLStrToStr(categoryNode->getNodeName());

            if (boost::equals(categoryNodeName, "Category"))
            {
                std::string key = "";
                std::map<std::string, std::vector<Keyword>> value;

                boost::shared_ptr<DOMNamedNodeMap> categoryNodeAttributes = boost::shared_ptr<DOMNamedNodeMap>(categoryNode->getAttributes(), boost::null_deleter());
                const XMLSize_t categoryNodeAttributeCount = categoryNodeAttributes->getLength();

                for (XMLSize_t j = 0; j < categoryNodeAttributeCount; j++)
                {
                    boost::shared_ptr<DOMNode> categoryNodeAttribute = boost::shared_ptr<DOMNode>(categoryNodeAttributes->item(j), &XmlDocumentPtrDeleter<DOMNode>);
                    std::string categoryNodeAttributeName = XMLStrToStr(categoryNodeAttribute->getNodeName());

                    if (boost::equals(categoryNodeAttributeName, "type"))
                    {
                        key = XMLStrToStr(categoryNodeAttribute->getNodeValue());
                        break;
                    }
                }

                boost::shared_ptr<DOMNodeList> keywordListNodes = boost::shared_ptr<DOMNodeList>(categoryNode->getChildNodes(), boost::null_deleter());
                const XMLSize_t keywordListNodeCount = keywordListNodes->getLength();

                for (XMLSize_t j = 0; j < keywordListNodeCount; j++)
                {
                    boost::shared_ptr<DOMNode> keywordListNode = boost::shared_ptr<DOMNode>(keywordListNodes->item(j), &XmlDocumentPtrDeleter<DOMNode>);
                    std::string keywordListNodeName = XMLStrToStr(keywordListNode->getNodeName());

                    if (boost::equals(keywordListNodeName, "KeywordList"))
                    {
                        std::string type;
                        std::vector<Keyword> keywordList;

                        boost::shared_ptr<DOMNodeList> keywordNodes = boost::shared_ptr<DOMNodeList>(keywordListNode->getChildNodes(), boost::null_deleter());
                        const XMLSize_t keywordNodeCount = keywordNodes->getLength();

                        for (XMLSize_t k = 0; k < keywordNodeCount; k++)
                        {
                            boost::shared_ptr<DOMNode> keywordNode = boost::shared_ptr<DOMNode>(keywordNodes->item(k), &XmlDocumentPtrDeleter<DOMNode>);
                            std::string keywordNodeName = XMLStrToStr(keywordNode->getNodeName());

                            if (boost::equals(keywordNodeName, "Keyword"))
                            {
                                Keyword keyword = {L"", Priority::NONE, EmptyRect, KeywordClass::POSITIVE}; // Default value

                                boost::shared_ptr<DOMNamedNodeMap> keywordNodeAttributes = boost::shared_ptr<DOMNamedNodeMap>(keywordNode->getAttributes(), boost::null_deleter());
                                const XMLSize_t keywordNodeAttributeCount = keywordNodeAttributes->getLength();

                                for (XMLSize_t t = 0; t < keywordNodeAttributeCount; t++)
                                {
                                    boost::shared_ptr<DOMNode> keywordNodeAttribute = boost::shared_ptr<DOMNode>(keywordNodeAttributes->item(t), &XmlDocumentPtrDeleter<DOMNode>);
                                    std::string keywordNodeAttributeName = XMLStrToStr(keywordNodeAttribute->getNodeName());

                                    if (boost::equals(keywordNodeAttributeName, "type"))
                                    {
                                        type = XMLStrToStr(keywordNodeAttribute->getNodeValue());
                                    }
                                    else if (boost::equals(keywordNodeAttributeName, "value"))
                                    {
                                        keyword.text = keywordNodeAttribute->getNodeValue();
                                    }
                                    else if (boost::equals(keywordNodeAttributeName, "priority"))
                                    {
                                        std::string priorityStr = XMLStrToStr(keywordNodeAttribute->getNodeValue());
                                        keyword.priority = this->StringToEnumPriority(priorityStr);
                                    }
                                    else if (boost::equals(keywordNodeAttributeName, "class"))
                                    {
                                        std::string classStr = XMLStrToStr(keywordNodeAttribute->getNodeValue());
                                        keyword.keywordClass = this->StringToEnumClass(classStr);
                                    }
                                }

                                keywordList.push_back(keyword);
                            }
                        }

                        value[type] = keywordList;
                    }
                }

                configKeywordMap[key] = value;
            }
        }


        return configKeywordMap;
    }

    Priority KeywordConfigReader::StringToEnumPriority(std::string str)
    {
        Priority priority;

        boost::to_upper(str);
        if (boost::equals(str, "LOW"))
        {
            priority = Priority::LOW;
        }
        else if (boost::equals(str, "MEDIUM"))
        {
            priority = Priority::MEDIUM;
        }
        else if (boost::equals(str, "HIGH"))
        {
            priority = Priority::HIGH;
        }
        else
        {
            priority = Priority::NONE;
        }

        return priority;
    }

    KeywordClass KeywordConfigReader::StringToEnumClass(std::string str)
    {
        KeywordClass keywordClass = KeywordClass::POSITIVE;

        boost::to_upper(str);
        if (boost::equals(str, "NEGATIVE"))
        {
            keywordClass = KeywordClass::NEGATIVE;
        }
        else //if (boost::equals(str, "POSITIVE"))
        {
            keywordClass = KeywordClass::POSITIVE;
        }

        return keywordClass;
    }
}
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

#include "CoverageCollector.hpp"
#include "DaVinciCommon.hpp"
#include "DeviceManager.hpp"
#include "TestManager.hpp"
#include <boost/core/null_deleter.hpp>

#include "boost/smart_ptr/shared_ptr.hpp"

namespace DaVinci
{
    CoverageCollector::CoverageCollector()
    {
        oneSeedCovFile = "";
        globalCovFile = "";

        globalCovWriter = nullptr;
        oneSeedCovWriter = nullptr;
        covDict = std::map<int, std::vector<int>>();
    }

    void CoverageCollector::SetGlobalCovFile(const std::string &file)
    {
        this->globalCovFile = file;
    }

    void CoverageCollector::SetOneSeedCovFile(const std::string &file)
    {
        this->oneSeedCovFile = file;
    }

    void CoverageCollector::UpdateDictionary(const boost::shared_ptr<UiState> &newState, const boost::shared_ptr<UiStateObject> &objectState)
    {
        if (newState == nullptr)
        {
            DAVINCI_LOG_DEBUG << "Invalid UiState Reference!" << std::endl;
            return;
        }

        int stateHashCode = newState->GetHashCode();
        std::vector<int> keys = GetMapKeys(covDict);

        if (objectState != nullptr)
        {
            int objectHashCode = objectState->GetHashCode();

            if (ContainVectorElement(keys, stateHashCode))
            {
                if (!ContainVectorElement(covDict[stateHashCode], objectHashCode))
                {
                    covDict[stateHashCode].push_back(objectHashCode);
                }
            }
            else
            {
                std::vector<int> objectHashCodeList;
                objectHashCodeList.push_back(objectHashCode);
                covDict.insert(make_pair(stateHashCode, objectHashCodeList));
            }
        }
        else
        {
            if (!ContainVectorElement(keys, stateHashCode))
            {
                covDict.insert(make_pair(stateHashCode, vector<int>()));
            }
        }
    }

    void CoverageCollector::WriteCoverageLine(bool global, const boost::shared_ptr<UiState> &uState, const boost::shared_ptr<UiStateObject> &uObj, const std::string &action, const std::string &actionParam)
    {
        boost::shared_ptr<AndroidTargetDevice> currentDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(DeviceManager::Instance().GetCurrentTargetDevice());
        std::string orientation = "";
        pair<int, int> appSize;

        if (currentDevice == nullptr)
        {
            orientation = "Unknown";
            appSize.first = 0;
            appSize.second = 0;
        }
        else
        {
            orientation = OrientationToString(currentDevice->GetCurrentOrientation());
            appSize = currentDevice->GetAppSize();
        }

        boost::shared_ptr<DOMDocument> covWriter = nullptr;
        if (global)
        {
            covWriter = globalCovWriter;
        }
        else
        {
            covWriter = oneSeedCovWriter;
        }

        if (covWriter == nullptr)
        {
            DAVINCI_LOG_ERROR << std::string("Null Refernce to the coverage file writer!") << std::endl;
            return;
        }
        else
        {
            boost::shared_ptr<DOMElement> covRoot = boost::shared_ptr<DOMElement>(covWriter->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);
            std::string xmlPath = "/GET/Layout";
            std::string hashValue = boost::lexical_cast<string>(uState->GetHashCode());

            boost::shared_ptr<DOMElement> newLayout = nullptr;
            boost::shared_ptr<DOMElement> newObjAct = boost::shared_ptr<DOMElement>(covWriter->createElement(StrToXMLStr("Object").get()),  &XmlDocumentPtrDeleter<DOMElement>);

            if (uObj != nullptr)
            {
                newObjAct->setAttribute(StrToXMLStr("Hash").get(), StrToXMLStr(boost::lexical_cast<string>(uObj->GetHashCode()).c_str()).get());
                newObjAct->setAttribute(StrToXMLStr("Text").get(), StrToXMLStr(uObj->GetText().c_str()).get());
            }
            else
            {
                newObjAct->setAttribute(StrToXMLStr("Hash").get(), StrToXMLStr("0").get());
                newObjAct->setAttribute(StrToXMLStr("Text").get(), StrToXMLStr("0").get());
            }

            newObjAct->setAttribute(StrToXMLStr("Action").get(), StrToXMLStr(action.c_str()).get());
            newObjAct->setAttribute(StrToXMLStr("Action-Param").get(), StrToXMLStr(actionParam.c_str()).get());


            // TODO: add time information
            newObjAct->setAttribute(StrToXMLStr("Time").get(), StrToXMLStr("").get());

            // Handle the ordinary layout next
            vector<boost::shared_ptr<DOMNode>> nodeList = GetNodesFromXPath(covWriter, xmlPath, StrToXMLStr("Hash").get(), hashValue);
            if (nodeList.size() == 0)
            {
                newLayout = boost::shared_ptr<DOMElement>(covWriter->createElement(StrToXMLStr("Layout").get()), &XmlDocumentPtrDeleter<DOMElement>);
                newLayout->setAttribute(StrToXMLStr("Hash").get(), StrToXMLStr(boost::lexical_cast<string>(uState->GetHashCode())).get());
                newLayout->setAttribute(StrToXMLStr("ObjectNum").get(), StrToXMLStr(boost::lexical_cast<string>(uState->GetClickableObjects().size() + uState->GetEditTextFields().size())).get());

                if (!global)
                {
                    newLayout->setAttribute(StrToXMLStr("Orientation").get(), StrToXMLStr(orientation).get());
                    newLayout->setAttribute(StrToXMLStr("Appsize").get(), StrToXMLStr(boost::lexical_cast<string>(appSize.first) + std::string("x") + boost::lexical_cast<string>(appSize.second)).get());
                }
                newLayout->appendChild(newObjAct.get());
                covRoot->appendChild(newLayout.get());
            }
            else
            {
                nodeList[0]->appendChild(newObjAct.get());
            }
        }
    }

    bool CoverageCollector::CheckStateExistInGlobalCovDict(const boost::shared_ptr<UiState> &curState, const boost::shared_ptr<UiStateObject> &curStateObj)
    {
        if (covDict.empty())
        {
            DAVINCI_LOG_DEBUG << "Empty global coverage dictionary encountered when looking up the dictionary!" << std::endl;
            return false;
        }
        else
        {
            vector<int> keys = GetMapKeys(covDict);
            if (!ContainVectorElement(keys, curState->GetHashCode()))
            {
                return false;
            }
            else
            {
                std::vector<int> tmpObjLst = covDict[curState->GetHashCode()];
                if (ContainVectorElement(tmpObjLst, curStateObj->GetHashCode()))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
    }

    void CoverageCollector::DumpGlobalCovDict(const std::string &outDir)
    {
#ifdef _DEBUG

        if (covDict.empty())
        {
            DAVINCI_LOG_WARNING << std::string("Warning: empty global coverage dictionary.") << std::endl;
            return;
        }
        else
        {
            std::ofstream output(outDir + std::string("\\globalDict"), std::ios_base::app | std::ios_base::out);

            for (auto stateKey : covDict)
            {
                output << "State Hash Key:  " + stateKey.first << std::flush << std::endl;
                for (auto objectKey : stateKey.second)
                {
                    output << "----------Object: " + objectKey << std::flush << std::endl;
                }
            }
            output.flush();
            output.close();
        }
#endif
    }

    void CoverageCollector::CreateGlobalCovFile()
    {
        if (globalCovWriter == nullptr)
        {
            boost::shared_ptr<XMLCh> coreString = StrToXMLStr(std::string("Core"));
            globalCovDomImpl = boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(coreString.get()), boost::null_deleter());

            if (boost::filesystem::is_regular_file(globalCovFile))
            {
                boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser(), boost::null_deleter());
                xmlParser->parse(globalCovFile.c_str());
                globalCovWriter = boost::shared_ptr<DOMDocument>(xmlParser->getDocument(), boost::null_deleter());
            }
            else
            {
                globalCovWriter = boost::shared_ptr<DOMDocument>(globalCovDomImpl->createDocument(0, StrToXMLStr("GET").get(), 0), &XmlDocumentPtrDeleter<DOMDocument>);

                int index0 = 0;
                int index1 = 0;

                index0 = (int)globalCovFile.find_last_of("\\");
                index0 += 1;
                index1 = (int)globalCovFile.find_last_of("_");
                std::string packageNameAndAll = globalCovFile.substr(index0, index1 - index0);

                index1 = (int)packageNameAndAll.find_last_of("_");
                std::string packageName = packageNameAndAll.substr(0, index1);

                boost::shared_ptr<DOMElement> globalCovRoot = boost::shared_ptr<DOMElement>(globalCovWriter->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);
                globalCovRoot->setAttribute(StrToXMLStr("PackageName").get(), StrToXMLStr(packageName.c_str()).get());
            }
        }
    }

    void CoverageCollector::CreateLocalCovFile()
    {
        if (oneSeedCovWriter == nullptr)
        {
            boost::shared_ptr<XMLCh> coreString = StrToXMLStr(std::string("Core"));
            oneSeedCovDomImpl = boost::shared_ptr<DOMImplementation>(DOMImplementationRegistry::getDOMImplementation(coreString.get()), boost::null_deleter());

            if (boost::filesystem::is_regular_file(oneSeedCovFile))
            {
                boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser(), boost::null_deleter());
                xmlParser->parse(oneSeedCovFile.c_str());
                oneSeedCovWriter = boost::shared_ptr<DOMDocument>(xmlParser->getDocument(), boost::null_deleter());
            }
            else
            {
                oneSeedCovWriter  = boost::shared_ptr<DOMDocument>(oneSeedCovDomImpl->createDocument(0, StrToXMLStr("GET").get(), 0), &XmlDocumentPtrDeleter<DOMDocument>);

                int index0 = 0;
                int index1 = 0;

                index0 = (int)oneSeedCovFile.find_last_of("\\");
                index0 += 1;
                index1 = (int)oneSeedCovFile.find_last_of("_");
                std::string packageNameAndSeedValue = oneSeedCovFile.substr(index0, index1 - index0);

                index1 = (int)packageNameAndSeedValue.find_last_of("_");
                std::string packageName = packageNameAndSeedValue.substr(0, index1);

                index1 += 1;
                std::string seedValue = packageNameAndSeedValue.substr(index1, packageNameAndSeedValue.length() - index1);

                boost::shared_ptr<DOMElement> covRoot = boost::shared_ptr<DOMElement>(oneSeedCovWriter->getDocumentElement(), &XmlDocumentPtrDeleter<DOMElement>);
                covRoot->setAttribute(StrToXMLStr("PackageName").get(), StrToXMLStr(packageName.c_str()).get());
                covRoot->setAttribute(StrToXMLStr("Seed").get(), StrToXMLStr(seedValue.c_str()).get());
            }
        }
    }

    void CoverageCollector::FillGlobalDict(const std::string &outDir)
    {
        if(!boost::filesystem::is_directory(outDir))
            return;

        boost::filesystem::path p(outDir);
        boost::filesystem::directory_iterator dirIter(p);
        boost::filesystem::directory_iterator end;

        for( ; dirIter != end; dirIter++)
        {
            if (is_regular_file(dirIter->path())) {
                wstring wCurrentFile = dirIter->path().wstring();
                if (boost::ends_with(wCurrentFile, "_all_cov.xml"))
                {
                    AddCovToGDict(dirIter->path().string());
                    break;
                }
            }
        }
    }

    void CoverageCollector::AddCovToGDict(const std::string &covFile)
    {
        boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser());
        xmlParser->parse(covFile.c_str());

        boost::shared_ptr<DOMDocument> xmlDoc = boost::shared_ptr<DOMDocument>(xmlParser->getDocument(), boost::null_deleter());
        boost::shared_ptr<DOMElement> covRoot = boost::shared_ptr<DOMElement>(xmlDoc->getDocumentElement(), boost::null_deleter());
        boost::shared_ptr<DOMNodeList> children = boost::shared_ptr<DOMNodeList>(covRoot->getChildNodes(), boost::null_deleter());

        const XMLSize_t nodeCount = children->getLength();
        std::string nodeName;

        for(XMLSize_t ix = 0; ix < nodeCount; ++ix)
        {
            boost::shared_ptr<DOMNode> child = boost::shared_ptr<DOMNode>(children->item(ix), &XmlDocumentPtrDeleter<DOMNode>);
            if (boost::equals(child->getNodeName(), "#text")) // handle whitespace
            {
                continue;
            }

            std::string hashKey = "";
            int hashKeyInt = 0;
            std::string objHashKey = "";
            int objHashKeyInt = 0;

            boost::shared_ptr<DOMNamedNodeMap> attributes = boost::shared_ptr<DOMNamedNodeMap>(child->getAttributes(), boost::null_deleter());

            const XMLSize_t attributeCount = attributes->getLength();

            for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
            {
                boost::shared_ptr<DOMNode> attribute = boost::shared_ptr<DOMNode>(attributes->item(ix), &XmlDocumentPtrDeleter<DOMNode>);

                if (boost::equals(attribute->getNodeName(), "Hash"))
                {
                    hashKey = XMLStrToStr(attribute->getNodeValue());
                    break;
                }
            }

            assert(hashKey != "");
            TryParse(hashKey, hashKeyInt);

            boost::shared_ptr<DOMNodeList> subChildren = boost::shared_ptr<DOMNodeList>(child->getChildNodes(), boost::null_deleter());
            const XMLSize_t subNodeCount = subChildren->getLength();

            for(XMLSize_t ix = 0; ix < subNodeCount; ++ix)
            {
                boost::shared_ptr<DOMNode> subChNode = boost::shared_ptr<DOMNode>(subChildren->item(ix), &XmlDocumentPtrDeleter<DOMNode>);
                if (boost::equals(subChNode->getNodeName(), "#text")) // handle whitespace
                {
                    continue;
                }

                boost::shared_ptr<DOMNamedNodeMap> subAttributes = boost::shared_ptr<DOMNamedNodeMap>(subChNode->getAttributes(), boost::null_deleter());
                const XMLSize_t subAttributeCount = subAttributes->getLength();

                for(XMLSize_t ix = 0; ix < subAttributeCount; ++ix)
                {
                    boost::shared_ptr<DOMNode> xa = boost::shared_ptr<DOMNode>(subAttributes->item(ix), &XmlDocumentPtrDeleter<DOMNode>);
                    if (boost::equals(xa->getNodeName(), "Hash"))
                    {
                        objHashKey = XMLStrToStr(xa->getNodeValue());
                        break;
                    }
                }

                assert(objHashKey != "");
                TryParse(objHashKey, objHashKeyInt);

                vector<int> keys = GetMapKeys(covDict);
                if (ContainVectorElement(keys, hashKeyInt))
                {
                    std::vector<int> tmpObjHashLst = covDict[hashKeyInt];
                    if (!ContainVectorElement(tmpObjHashLst, objHashKeyInt))
                    {
                        tmpObjHashLst.push_back(objHashKeyInt);
                    }
                }
                else
                {
                    std::vector<int> tmpObjHashLst = std::vector<int>();
                    tmpObjHashLst.push_back(objHashKeyInt);
                    covDict.insert(make_pair(hashKeyInt, tmpObjHashLst));
                }
            }
        }
    }

    void CoverageCollector::SaveOneSeedCoverage()
    {
        if (oneSeedCovWriter != nullptr && oneSeedCovFile != "")
        {
            WriteDOMDocumentToXML(oneSeedCovDomImpl, oneSeedCovWriter, oneSeedCovFile);
        }
    }

    void CoverageCollector::SaveGlobalCoverage()
    {
        if (globalCovWriter != nullptr && globalCovFile != "")
        {
            WriteDOMDocumentToXML(globalCovDomImpl, globalCovWriter, globalCovFile);
        }
    }

    void CoverageCollector::PrepareCoverageCollector(const std::string &packageName, int seedValue, const std::string &outDir)
    {
        SetGlobalCovFile(outDir + std::string("\\") + packageName + std::string("_all_cov.xml"));
        CreateGlobalCovFile();
        FillGlobalDict(outDir);
        SetOneSeedCovFile(outDir + std::string("\\") + packageName + std::string("_") + boost::lexical_cast<string>(seedValue) + std::string("_cov.xml"));
        CreateLocalCovFile();
    }
}

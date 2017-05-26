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

#ifndef __COVERAGECOLLECTOR__
#define __COVERAGECOLLECTOR__

#include "UiStateObject.hpp"
#include "UiState.hpp"

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

#include "boost/filesystem.hpp"

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "boost/smart_ptr/shared_ptr.hpp"

namespace DaVinci
{
    /// <summary>
    /// Class CoverageCollector
    /// </summary>
    class CoverageCollector
    {
    private:
        /// <summary>
        /// One seed coverage file
        /// </summary>
        std::string oneSeedCovFile;

        /// <summary>
        /// All seeds coverage file
        /// </summary>
        std::string globalCovFile;

        /// <summary>
        /// Xml Writer for GET coverage with all seeds
        /// </summary>
        boost::shared_ptr<DOMDocument> globalCovWriter;

        /// <summary>
        /// Global dom impl
        /// </summary>
        boost::shared_ptr<DOMImplementation> globalCovDomImpl;

        /// <summary>
        /// Xml writer for GET coverage with one seed
        /// </summary>
        boost::shared_ptr<DOMDocument> oneSeedCovWriter;

        /// <summary>
        /// One seed dom impl
        /// </summary>
        boost::shared_ptr<DOMImplementation> oneSeedCovDomImpl;

        /// <summary>
        /// Dictionary for representing all seeds coverage
        /// </summary>
        std::map<int, std::vector<int>> covDict;

        /// <summary>
        /// Construct function
        /// </summary>
    public:
        CoverageCollector();

        /// <summary>
        /// Set all seeds coverage file
        /// </summary>
        /// <param name="file"></param>
        void SetGlobalCovFile(const std::string &file);

        /// <summary>
        /// Set one seed coverage file
        /// </summary>
        /// <param name="file"></param>
        void SetOneSeedCovFile(const std::string &file);

        /// <summary>
        /// Update global dictionary as a guide for action dispatcher
        /// </summary>
        /// <param name="newState"></param>
        /// <param name="objectState"></param>
        void UpdateDictionary(const boost::shared_ptr<UiState> &newState, const boost::shared_ptr<UiStateObject> &objectState);

        /// <summary>
        /// Write coverage line
        /// </summary>
        /// <param name="global"></param>
        /// <param name="uState"></param>
        /// <param name="uObj"></param>
        /// <param name="action"></param>
        /// <param name="actionParam"></param>
        /// <param name="isPriority"></param>
        /// <param name="isTutorial"></param>
        /// <param name="isLogin"></param>
        void WriteCoverageLine(bool global, const boost::shared_ptr<UiState> &uState, const boost::shared_ptr<UiStateObject> &uObj, const std::string &action, const std::string &actionParam);

        /// <summary>
        /// Check whether an object exists in global coverage dictionary
        /// </summary>
        /// <param name="curState"></param>
        /// <param name="curStateObj"></param>
        /// <returns></returns>
        bool CheckStateExistInGlobalCovDict(const boost::shared_ptr<UiState> &curState, const boost::shared_ptr<UiStateObject> &curStateObj);

        /// <summary>
        /// Dump the global dictionary to outdir (debugging code)
        /// </summary>
        /// <param name="outDir"></param>
        void DumpGlobalCovDict(const std::string &outDir);

        /// <summary>
        /// Create gloabl coverage file for all seeds
        /// </summary>
        void CreateGlobalCovFile();

        /// <summary>
        /// Create one seed coverage file
        /// </summary>
        void CreateLocalCovFile();

        /// <summary>
        /// Fill the global coverage file to global dictionary
        /// </summary>
        /// <param name="outDir"></param>
        void FillGlobalDict(const std::string &outDir);

        /// <summary>
        /// Add coverage to global dictionary
        /// </summary>
        /// <param name="covFile"></param>
        void AddCovToGDict(const std::string &covFile);

        /// <summary>
        /// Save one seed coverage
        /// </summary>
        void SaveOneSeedCoverage();

        /// <summary>
        /// Save global coverage
        /// </summary>
        void SaveGlobalCoverage();

        /// <summary>
        /// Prepare coverage collector
        /// </summary>
        /// <param name="packageName"></param>
        /// <param name="seedValue"></param>
        /// <param name="outDir"></param>
        void PrepareCoverageCollector(const std::string &packageName, int seedValue, const std::string &outDir);
    };
}


#endif	//#ifndef __COVERAGECOLLECTOR__

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

#ifndef __TEST_REPORT_HPP__
#define __TEST_REPORT_HPP__

#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include <unordered_map>

#include "DaVinciStatus.hpp"
#include "TestReportCommon.hpp"
#include "TestReportSummary.hpp"
#include "AndroidTargetDevice.hpp"
#include "QScript.hpp"

namespace DaVinci
{
    class TestReport : public boost::enable_shared_from_this<TestReport>
    {
    public:
        /// <summary>
        /// Constructor
        /// </summary>
        TestReport()
        {
        }

        void CopyDaVinciLog();

        void GetAllMediaFiles(boost::filesystem::path directory, vector<boost::filesystem::path> & out, string videoNameKeyWord);
        //File name with full path
        void CopyDaVinciVideoAndAudio(const string filename, string videoNameStartWith);

        void SetReportType(DavinciLogType logType)
        {
            // TODO
        }

        /// <summary> Prepare test report for q script replay. </summary>
        /// <param name="scriptFile"> The script file. </param>
        /// <returns> The DaVinciStatus. </returns>
        DaVinciStatus PrepareQScriptReplay(string scriptFile);

        /// <summary> Get the dir name from the file's full path. </summary>
        /// <param name="fname"> The file full path. </param>
        /// <returns> The dir name. </returns>
        string dirNameOf(const std::string& fname);

        static string currentQsLogPath;
        static string currentQsLogName;
        static string currentReportSummaryName;
        static string currentQsDirName;
        static string currentQsPath;

        void GetLogcat(string packageName, string logcatName = "logcat.txt");

    private:
        /// <summary>
        /// 
        /// </summary>
        boost::shared_ptr<TestReportSummary> testReportSummary;

        /// <summary>
        /// 
        /// </summary>
        boost::shared_ptr<TestReportSummary> testTxtReportSummary;

        /// <summary>
        /// 
        /// </summary>
        string currentReportSummaryModelName;

        string getReportSummaryModelPath(string filename);
    };
}

#endif
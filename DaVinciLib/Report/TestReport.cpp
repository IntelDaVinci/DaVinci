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
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"

#include "DeviceManager.hpp"
#include "DaVinciCommon.hpp"
#include "TestReport.hpp"
#include "TestManager.hpp"
#include "boost/algorithm/string.hpp"

using namespace std;
using namespace xercesc;
using namespace boost::filesystem;
using namespace boost::posix_time;

namespace DaVinci
{
    string TestReport::currentQsLogPath;
    string TestReport::currentQsLogName;
    string TestReport::currentReportSummaryName;
    string TestReport::currentQsDirName;
    string TestReport::currentQsPath;

    void TestReport::CopyDaVinciLog()
    {
        DAVINCI_LOG_INFO << "----------------- Post Test Handling -----------------"; 
        string _davincilog = "DaVinci.log";
        string davincilog = TestReport::currentQsLogPath + "/DaVinci.log.txt";

        if(boost::filesystem::is_regular_file(_davincilog))
        {
            DAVINCI_LOG_INFO << "Copying DaVinci Log ...";
            CopyFile(_davincilog.c_str(), davincilog.c_str(), true);
        }
        DAVINCI_LOG_INFO << "----------------- Post Test Handling -----------------"; 
    }

    void TestReport::GetAllMediaFiles(boost::filesystem::path directory, vector<boost::filesystem::path> & out, string videoNameStartWith)
    {
        boost::filesystem::directory_iterator end_iter;

        if (boost::filesystem::exists(directory) && boost::filesystem::is_directory(directory))
        {
            for(boost::filesystem::directory_iterator dir_iter(directory); dir_iter != end_iter; ++dir_iter)
            {
                if (!boost::filesystem::is_directory(*dir_iter) && (boost::filesystem::extension(*dir_iter)==".avi") ||
                    (!boost::filesystem::is_directory(*dir_iter) && (boost::filesystem::extension(*dir_iter) == ".wav") ))
                {
                    const boost::filesystem::path this_path = dir_iter->path();
                    string mediaFileName = this_path.filename().string();
                    if(boost::algorithm::starts_with(mediaFileName, videoNameStartWith))
                    {
                        out.push_back(*dir_iter);
                    }
                }
            }
        }
    }

    //videoNameKeyWord is for filtering the avi videos, by default it is "_", for GETRNR it is packagename.
    void TestReport::CopyDaVinciVideoAndAudio(const string filename, string videoNameStartWith)
    {
        boost::filesystem::path filePath(filename);
        filePath = system_complete(filePath);
        boost::filesystem::path folder = filePath.parent_path();
        boost::filesystem::path videoPath = folder;
        string videoFileName = "";
        string videoFilePath = videoPath.string();
        string replayLogVideo = currentQsLogPath + "/" + videoFileName;

        vector<boost::filesystem::path> avifiles;
        GetAllMediaFiles(folder, avifiles, videoNameStartWith + "_replay");
        if (avifiles.empty())
        {
            DAVINCI_LOG_INFO << "No replay video found to copy.";
            return;
        }

        vector<boost::filesystem::path>::reverse_iterator end = avifiles.rend();
        boost::filesystem::path avifile;
        for(vector<boost::filesystem::path>::reverse_iterator i = avifiles.rbegin(); i != end; i++)
        {
            videoFilePath = (*i).string();

            videoFileName = (*i).filename().string();            
            replayLogVideo = currentQsLogPath + "/" + videoFileName;

            if(boost::filesystem::is_regular_file(videoFilePath))
            {
                try
                {
                    DAVINCI_LOG_INFO << "Copying replay video " << videoFilePath << " please wait ...";
                    CopyFile(videoFilePath.c_str(), replayLogVideo.c_str(), true);
                    assert(boost::filesystem::is_regular_file(replayLogVideo));
                    DAVINCI_LOG_INFO << "Coping replay video " << videoFilePath << " done";
                }
                catch(...)
                {
                    DAVINCI_LOG_ERROR << "Copying replay video " << videoFilePath << " fail ...";
                }
            }
        }

        boost::filesystem::path audioPath = folder;
        string audioFileName = filePath.stem().string() + "_replay.wav";
        audioPath /= audioFileName;
        string audioFilePath = audioPath.string();
        string replayLogAudio = currentQsLogPath + "/" + audioFileName;

        if(boost::filesystem::is_regular_file(audioFilePath))
        {
            try
            {
                DAVINCI_LOG_INFO << "Copying replay audio "<< audioFilePath <<" please wait ...";
                CopyFile(audioFilePath.c_str(), replayLogAudio.c_str(), true);
                assert(boost::filesystem::is_regular_file(replayLogAudio));
                DAVINCI_LOG_INFO << "Copying replay audio done";
            }
            catch(...)
            {
                DAVINCI_LOG_ERROR << "Copying replay audio "<< audioFilePath <<" fail ...";
            }
        }
        else
        {
            DAVINCI_LOG_WARNING << "Not found replay audio "<< audioFilePath <<" yet.";
        }
    }

    DaVinciStatus TestReport::PrepareQScriptReplay(string scriptFile)
    {
        currentQsPath = scriptFile;
        path qsPath = system_complete(path(scriptFile));
        currentQsDirName = dirNameOf(qsPath.string());
        currentQsLogName = to_iso_string(second_clock::local_time());
        string rnrLogPath = dirNameOf(qsPath.string()) + "\\_Logs";
        //currentQsLogPath = qsPath.append(rnrLogPath.begin(), rnrLogPath.end()).append(currentQsLogName.begin(), currentQsLogName.end()).string();
        currentQsLogPath = currentQsDirName + "\\_Logs\\" + currentQsLogName;
        if (!exists(currentQsLogPath))
        {
            // Creat _Logs
            if (!exists(rnrLogPath))
            {
                boost::system::error_code ec;
                create_directory(rnrLogPath, ec);
                DaVinciStatus status(ec);
                if (!DaVinciSuccess(status))
                {
                    DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << rnrLogPath;
                    return status;
                }
            }
            else if (!is_directory(rnrLogPath))
            {
                DAVINCI_LOG_ERROR << rnrLogPath << " is not a directory.";
                return DaVinciStatus(errc::not_a_directory);
            }

            // Creat sub dir named with time
            boost::system::error_code ec;
            create_directory(currentQsLogPath, ec);
            DaVinciStatus status(ec);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << currentQsLogPath;
                return status;
            }
            DAVINCI_LOG_DEBUG << "Success to create log folder: " << currentQsLogPath;

            // Prepare ReportSummary.xml
            currentReportSummaryName = currentQsLogPath + "\\ReportSummary.xml";
            if(!boost::filesystem::exists(currentReportSummaryName))
            {
                string _bannerpng = getReportSummaryModelPath("banner.png");
                string _logogif = getReportSummaryModelPath("logo.gif");    
                string _resultcss = getReportSummaryModelPath("result.css");    
                string _resultxsl = getReportSummaryModelPath("result.xsl");

                string bannerpng = currentQsLogPath + "\\banner.png";
                string logogif = currentQsLogPath + "\\logo.gif";    
                string resultcss = currentQsLogPath + "\\result.css";    
                string resultxsl = currentQsLogPath + "\\result.xsl";

                CopyFile(_bannerpng.c_str(), bannerpng.c_str(), true);
                CopyFile(_logogif.c_str(), logogif.c_str(), true);
                CopyFile(_resultcss.c_str(), resultcss.c_str(), true);
                CopyFile(_resultxsl.c_str(), resultxsl.c_str(), true);
            }

            testReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());
            testReportSummary->PrepareReportSummary(currentReportSummaryName);

            //string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";
            //
            //testReportSummary->appendCommonNodePass(currentReportSummaryName,
            //    "type",
            //    "RNR","name", "starttime","endtime","message"
            //    );
            //
            //testTxtReportSummary->appendTxtReportSummary("Test only line...");
            //return status;
        }
        else if (!is_directory(currentQsLogPath))
        {
            DAVINCI_LOG_ERROR << currentQsLogPath << " is not a directory.";
            return DaVinciStatus(errc::not_a_directory);
        }

        // Creat index format dir
        string indexformatdir = currentQsDirName + "\\_Logs\\indexformat";
        string indexjquery = indexformatdir + "\\jquery-1.11.2.min.js";
        string indexjs = indexformatdir + "\\index.js";    
        string indexcss = indexformatdir + "\\index.css";

        if (!exists(indexformatdir) ||
            !boost::filesystem::exists(indexjquery) ||
            !boost::filesystem::exists(indexjs) ||
            !boost::filesystem::exists(indexcss))
        {
            boost::system::error_code ec;
            create_directory(indexformatdir, ec);
            DaVinciStatus status(ec);
            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_ERROR << "Failed to create log folder(" << status.value() << "): " << indexformatdir;
                return status;
            }
            //DAVINCI_LOG_DEBUG << "Success to create log folder: " << indexformatdir;
            string _indexjquery = getReportSummaryModelPath("jquery-1.11.2.min.js");    
            string _indexjs = getReportSummaryModelPath("index.js");    
            string _indexcss = getReportSummaryModelPath("index.css");

            //string indexjquery = indexformatdir + "\\jquery-1.11.2.min.js";
            //string indexjs = indexformatdir + "\\index.js";    
            //string indexcss = indexformatdir + "\\index.css";

            CopyFile(_indexjquery.c_str(), indexjquery.c_str(), true);
            CopyFile(_indexjs.c_str(), indexjs.c_str(), true);
            CopyFile(_indexcss.c_str(), indexcss.c_str(), true);
        }
        else if (!is_directory(indexformatdir))
        {
            DAVINCI_LOG_ERROR << indexformatdir << " is not a directory.";
            return DaVinciStatus(errc::not_a_directory);
        }

        return DaVinciStatusSuccess;
    }

    string TestReport::dirNameOf(const std::string& fname)
    {
        size_t pos = fname.find_last_of("\\/");
        return (std::string::npos == pos) ? "" : fname.substr(0, pos);
    }

    string TestReport::getReportSummaryModelPath(string filename)
    {
        return TestManager::Instance().GetDaVinciResourcePath( "Resources/Report/" + filename);
    }

    void TestReport::GetLogcat(string packageName, string logcatName)
    {
        if (!boost::filesystem::exists(currentQsLogPath))
        {
            if(currentQsLogPath != "")
            {
                boost::filesystem::create_directory(currentQsLogPath);
            }
            else
            {
                DAVINCI_LOG_WARNING << "Current Log folder is not exist.";
                return;
            }
        }

        boost::shared_ptr<AndroidTargetDevice> targetDevice;
        string logcatname = TestReport::currentQsLogPath + "/" + logcatName;
        auto device = DeviceManager::Instance().GetCurrentTargetDevice();

        targetDevice = boost::dynamic_pointer_cast<AndroidTargetDevice>(device);
        //string packageName = qs->GetPackageName();

        if (targetDevice != nullptr)
        {
            //resverd the next two lines for the later developing: picking useful message.
            //string logcatPackageName;
            //logcatPackageName += "-e " + packageName + " ";

            auto outStr = boost::shared_ptr<ostringstream>(new ostringstream());
            // Quote wraps the logcat command
            string command = "shell \"logcat -d -v time \"";
            DaVinciStatus status = targetDevice->AdbCommand(command, outStr, 10000);

            if (!DaVinciSuccess(status))
            {
                DAVINCI_LOG_DEBUG << "Get logcat failed: " << status.value();
                std::ofstream logStream(logcatname, std::ios_base::app | std::ios_base::out);
                logStream << "Get logcat failed" << std::flush;
                return;
            }

            string log = outStr->str();
            std::ofstream logStream(logcatname, std::ios_base::app | std::ios_base::out);
            logStream << log << std::flush;
        }
        else
        {
            DAVINCI_LOG_DEBUG << "No device is found!";
            std::ofstream logStream(logcatname, std::ios_base::app | std::ios_base::out);
            logStream << "No device is found!" << std::flush;
            return;
        }
    }

}
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

#include "ResultChecker.hpp"

namespace DaVinci
{
    ResultChecker::ResultChecker(const boost::weak_ptr<ScriptReplayer> obj) : Checker()
    {
        SetCheckerName("ResultChecker");
        testTxtReportSummary = boost::shared_ptr<TestReportSummary>(new TestReportSummary());
        testReport = boost::shared_ptr<TestReport>(new TestReport());

        Running = boost::shared_ptr<RunningChecker>(new RunningChecker(obj));
        Dialog = boost::shared_ptr<DialogChecker>(new DialogChecker(obj));
        Video = boost::shared_ptr<VideoChecker>(new VideoChecker(obj));
        Audio = boost::shared_ptr<AudioChecker>(new AudioChecker(obj));
        Runtime = boost::shared_ptr<Checker>(new Checker());
        Agent = boost::shared_ptr<Checker>(new Checker());

        boost::shared_ptr<ScriptReplayer> currentScriptTmp = obj.lock();
        if ((currentScriptTmp == nullptr) || (currentScriptTmp->GetQS() == nullptr))
        {
             DAVINCI_LOG_ERROR << "Failed to Init ResultChecker()!";
             throw 1;
        }

        assert(Running != nullptr);
        assert(Agent != nullptr);
        assert(Dialog != nullptr);
        assert(Video != nullptr);
        assert(Audio != nullptr);
        assert(Runtime != nullptr);

        currentQScript = currentScriptTmp;
        Runtime->SetCheckerName("RuntimeChecker");
        Agent->SetCheckerName("AgentChecker");
    }

    ResultChecker::~ResultChecker()
    {
    }

    /// <summary>
    /// Start all online checkers' worker thread
    /// </summary>
    void ResultChecker::StartOnlineChecking()
    {
        Running->Start();
        Dialog->Start();
        SetRunningFlag(true);
    }

    /// <summary>
    /// Stop all online checkers' worker thread
    /// </summary>
    void ResultChecker::StopOnlineChecking()
    {
        Running->Stop();
        Dialog->Stop();
        SetRunningFlag(false);
    }

    /// <summary>
    /// Pause all online checkers' worker thread
    /// </summary>
    void ResultChecker::PauseOnlineChecking()
    {
        Running->Pause();
        Dialog->Pause();
        SetPauseFlag(true);
    }

    /// <summary>
    /// Resume all online checkers' worker thread
    /// </summary>
    void ResultChecker::ResumeOnlineChecking()
    {
        Running->Resume();
        Dialog->Resume();
        SetPauseFlag(false);
    }

    /// <summary>
    /// Update checkpoints by opcode, say: stop all test when EXIT.
    /// </summary>
    void ResultChecker::UpdateCheckpoints(int opcode)
    {
        switch (opcode)
        {
        case QSEventAndAction::OPCODE_HOME:
        case QSEventAndAction::OPCODE_BACK:
        case QSEventAndAction::OPCODE_MENU:
        case QSEventAndAction::OPCODE_UNINSTALL_APP:
        case QSEventAndAction::OPCODE_INSTALL_APP:
        case QSEventAndAction::OPCODE_START_APP:
        case QSEventAndAction::OPCODE_STOP_APP:
        case QSEventAndAction::OPCODE_PUSH_DATA:
        case QSEventAndAction::OPCODE_EXIT:
        case QSEventAndAction::OPCODE_DEVICE_POWER:
            Running->Pause();
            break;

        case QSEventAndAction::OPCODE_NEXT_SCRIPT:
        case QSEventAndAction::OPCODE_CALL_SCRIPT:
        case QSEventAndAction::OPCODE_BATCH_PROCESSING:
            PauseOnlineChecking();
            break;

        case QSEventAndAction::OPCODE_USB_IN:
        case QSEventAndAction::OPCODE_USB_OUT:
            break;

        default:
            ResumeOnlineChecking();
            break;
        }
    }

    /// <summary>
    /// Start all offline checkers' worker thread
    /// </summary>
    void ResultChecker::StartOfflineChecking()
    {
        Video->DoOfflineChecking();
        Audio->DoOfflineChecking();
    }
    
    string& replaceSpaces(string &str, string::size_type pos = 0)
    {
        static const string delim = " ";
        pos = str.find_first_of(delim, pos);
        if (pos == string::npos)
            return str;
        return replaceSpaces(str.replace(pos, 1, "%20"));
    }

    /// <summary>
    /// Result checking report.
    /// </summary>
    void ResultChecker::FinalReport()
    {
        string inputStr;
        string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";
        string reportShowAddress = currentReportSummaryName;
        replaceSpaces(reportShowAddress, 0);
        boost::shared_ptr<ScriptReplayer> currentScriptTmp = currentQScript.lock();
        assert(currentScriptTmp != nullptr);

        if ((Runtime->GetResult() != CheckerResult::Fail)
            && (Running->GetResult() != CheckerResult::Fail)
            && (Audio->GetResult() != CheckerResult::Fail)
            && (Video->GetResult() != CheckerResult::Fail)
            && (Dialog->GetResult() != CheckerResult::Fail)
            && (Agent->GetResult() != CheckerResult::Fail))
        {
            DAVINCI_LOG_INFO <<"     Result: !!!PASS!!!";
            DAVINCI_LOG_INFO <<"     Video Mismatches = 0";
            DAVINCI_LOG_INFO <<"     Video Flicks = 0";
            DAVINCI_LOG_INFO <<"     Preview Mismatches = 0";
//            DAVINCI_LOG_INFO <<"     Object Mismatches = 0";
            DAVINCI_LOG_INFO <<"     Runtime Errors = 0";
            DAVINCI_LOG_INFO <<"     Audio Mismatches = 0";
            DAVINCI_LOG_INFO <<"     Crashes = 0";
            DAVINCI_LOG_INFO <<"     ANR = 0";
            DAVINCI_LOG_INFO <<"     Running = true";
            DAVINCI_LOG_INFO <<"     QAgent Connection = true";
            DAVINCI_LOG_INFO <<"     Report Address: ";
            DAVINCI_LOG_INFO << "file://" + reportShowAddress;

            if (testTxtReportSummary != nullptr )
            {
                testTxtReportSummary->appendTxtReportSummary("\n========");
                testTxtReportSummary->appendTxtReportSummary("\n[QScript]:[" + boost::lexical_cast<std::string>(TestReport::currentQsLogName) + "]\n");
                testTxtReportSummary->appendTxtReportSummary("QSFileName = " + currentScriptTmp->GetQsName() + ".qs");

                testTxtReportSummary->appendTxtReportSummary("LogFileName = " + boost::lexical_cast<std::string>(TestReport::currentQsLogPath) );
                testTxtReportSummary->appendTxtReportSummary("     Result: !!!PASS!!!");
                testTxtReportSummary->appendTxtReportSummary("     Video Mismatches = 0");
                testTxtReportSummary->appendTxtReportSummary("     Video Flicks = 0");
                testTxtReportSummary->appendTxtReportSummary("     Preview Mismatches = 0");
//                testTxtReportSummary->appendTxtReportSummary("     Object Mismatches = 0");
                testTxtReportSummary->appendTxtReportSummary("     Runtime Errors = 0");
                testTxtReportSummary->appendTxtReportSummary("     Audio Mismatches = 0");
                testTxtReportSummary->appendTxtReportSummary("     Crashes = 0");
                testTxtReportSummary->appendTxtReportSummary("     ANR = 0");
                testTxtReportSummary->appendTxtReportSummary("     Running = true");
                testTxtReportSummary->appendTxtReportSummary("     QAgent Connection = true");
                string name = currentScriptTmp->GetQsName() + ".qs";
                string message = boost::lexical_cast<std::string>(TestReport::currentQsLogPath);
                testTxtReportSummary->appendCommonNodePass(currentReportSummaryName,
                                                            "type",
                                                            "RNR", 
                                                            name, 
                                                            "starttime",
                                                            "endtime", 
                                                            message
                                                            );
            }
            else
            {
                DAVINCI_LOG_ERROR << "Can not find test ReportSummary in Result checker" << endl;
            }

        }
        else
        {
            DAVINCI_LOG_INFO <<"Final Result: !!!FAIL!!!";
            DAVINCI_LOG_INFO <<"     Video Mismatches = " << Video->GetVideoMismatches();
            DAVINCI_LOG_INFO <<"     Video Flicks = " << Video->GetFlicks();
            DAVINCI_LOG_INFO <<"     Preview Mismatches = " << Video->GetPreviewMismatches();
//            DAVINCI_LOG_INFO <<"     Object Mismatches = " << Video->GetObjectMismatches();
            DAVINCI_LOG_INFO <<"     Runtime Errors = " << Runtime->GetErrorCount();
            DAVINCI_LOG_INFO <<"     Audio Mismatches = " << Audio->GetMismatches();
            DAVINCI_LOG_INFO <<"     Crashes = " << (((Dialog->GetResult() == CheckerResult::Fail) &&(Dialog->GetDialogType() == DialogCheckResult::crashDialog)) ? "1" : "0");
            DAVINCI_LOG_INFO <<"     ANR = " << (((Dialog->GetResult() == CheckerResult::Fail) &&(Dialog->GetDialogType() == DialogCheckResult::anrDialog)) ? "1" : "0");
            DAVINCI_LOG_INFO <<"     Running = " << (Running->GetResult() == CheckerResult::Fail ? "False" : "True");
            DAVINCI_LOG_INFO <<"     QAgent Connection = " << (Agent->GetResult() == CheckerResult::Fail ? "False" : "True");
            DAVINCI_LOG_INFO <<"     Report Address: ";
            DAVINCI_LOG_INFO << "file://" + reportShowAddress;

            if (testTxtReportSummary != nullptr )
            {
                testTxtReportSummary->appendTxtReportSummary("\n========");
                testTxtReportSummary->appendTxtReportSummary("\n[QScript]:[" + boost::lexical_cast<std::string>(TestReport::currentQsLogName) + "]\n");
                testTxtReportSummary->appendTxtReportSummary("QSFileName = " + currentScriptTmp->GetQsName() + ".qs");

                testTxtReportSummary->appendTxtReportSummary("LogFileName = " + boost::lexical_cast<std::string>(TestReport::currentQsLogPath) );

                testTxtReportSummary->appendTxtReportSummary("Final Result: !!!FAIL!!!");

                inputStr = "     Video Mismatches = " + boost::lexical_cast<std::string>(Video->GetVideoMismatches());
                testTxtReportSummary->appendTxtReportSummary(inputStr);

                inputStr = "     Video Flicks = " + boost::lexical_cast<std::string>(Video->GetFlicks());
                testTxtReportSummary->appendTxtReportSummary(inputStr);

                inputStr = "     Preview Mismatches = " + boost::lexical_cast<std::string>(Video->GetPreviewMismatches());
                testTxtReportSummary->appendTxtReportSummary(inputStr);

//                inputStr = "     Object Mismatches = " + boost::lexical_cast<std::string>(Video->GetObjectMismatches());
//                testTxtReportSummary->appendTxtReportSummary(inputStr);

                inputStr = "     Runtime Errors = " + boost::lexical_cast<std::string>(Runtime->GetErrorCount());
                testTxtReportSummary->appendTxtReportSummary(inputStr);

                //   testTxtReportSummary->appendTxtReportSummary("     Audio Mismatches = " + ResultChecker.getAudioMismatches());
                testTxtReportSummary->appendTxtReportSummary("     Audio Mismatches = 0");

                if ((Dialog->GetResult() == CheckerResult::Fail) &&(Dialog->GetDialogType() == DialogCheckResult::crashDialog))
                    inputStr = "     Crashes = 1";
                else
                    inputStr = "     Crashes = 0";
                testTxtReportSummary->appendTxtReportSummary(inputStr);

                if ((Dialog->GetResult() == CheckerResult::Fail) &&(Dialog->GetDialogType() == DialogCheckResult::anrDialog))
                    inputStr = "     ANR = 1";
                else
                    inputStr = "     ANR = 0";
                testTxtReportSummary->appendTxtReportSummary(inputStr);

                if (Running->GetResult() == CheckerResult::Fail )
                    inputStr = "     Running = False";
                else
                    inputStr = "     Running = True";

                testTxtReportSummary->appendTxtReportSummary(inputStr);

                if (Agent->GetResult() == CheckerResult::Fail )
                    inputStr = "     QAgent Connection = False";
                else
                    inputStr = "     QAgent Connection = True";

                testTxtReportSummary->appendTxtReportSummary(inputStr);


                string name = currentScriptTmp->GetQsName() + ".qs";
                string message = boost::lexical_cast<std::string>(TestReport::currentQsLogPath);

                vector <string> dialogImages;
                vector <string> targetImages;
                vector <string> referenceImages;

                dialogImages = Dialog->GetFailedImageList();

                targetImages = Video->GetFailedImageList();
                referenceImages = Video->GetReferenceImageList();

                //Running Check
                if(Runtime->GetResult() == CheckerResult::Fail)
                {
                    string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                    string resulttype = "type"; //<test-result type="RnR">
                    string typevalue = "RNR"; 
                    string commonnamevalue = currentScriptTmp->GetQsName() + ".qs";
                    string totalreasult = "FAIL"; 
                    string message = "There are some script runtime failures.";
                    string itemname = "RunTime";
                    string itemresult = "FAIL";
                    string itemstarttime = "starttime";
                    string itemendtime = "endtime";
                    string timestamp = "timestamp";
                    string infoname = "name";
                    string infourl = "";
                    testTxtReportSummary->appendCommonNodeCommoninfo(
                        currentReportSummaryName,
                        resulttype, 
                        typevalue, 
                        commonnamevalue,
                        totalreasult, 
                        message,
                        itemname, 
                        itemresult, 
                        itemstarttime, 
                        itemendtime, 
                        timestamp, 
                        infoname, 
                        infourl);
                }

                if(Running->GetResult() == CheckerResult::Fail)
                {
                    string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                    string resulttype = "type"; //<test-result type="RnR">
                    string typevalue = "RNR"; 
                    string commonnamevalue = currentScriptTmp->GetQsName() + ".qs";
                    string totalreasult = "FAIL"; 
                    string message = "The test application stopped running.";
                    string itemname = "Running";
                    string itemresult = "FAIL";
                    string itemstarttime = "starttime";
                    string itemendtime = "endtime";
                    string timestamp = "timestamp";
                    string infoname = "name";
                    string infourl = "";
                    testTxtReportSummary->appendCommonNodeCommoninfo(
                        currentReportSummaryName,
                        resulttype, 
                        typevalue, 
                        commonnamevalue,
                        totalreasult, 
                        message,
                        itemname, 
                        itemresult, 
                        itemstarttime, 
                        itemendtime, 
                        timestamp, 
                        infoname, 
                        infourl);
                }

                if (Agent->GetResult() == CheckerResult::Fail )
                {                  
                    string currentReportSummaryName = TestReport::currentQsLogPath + "/ReportSummary.xml";                        
                    string resulttype = "type"; //<test-result type="RnR">
                    string typevalue = "RNR"; 
                    string commonnamevalue = currentScriptTmp->GetQsName() + ".qs";
                    string totalreasult = "FAIL"; 
                    string message = "The QAgent connection lost.";
                    string itemname = "QAgent Connection";
                    string itemresult = "FAIL";
                    string itemstarttime = "starttime";
                    string itemendtime = "endtime";
                    string timestamp = "timestamp";
                    string infoname = "name";
                    string infourl = "";
                    testTxtReportSummary->appendCommonNodeCommoninfo(
                        currentReportSummaryName,
                        resulttype, 
                        typevalue, 
                        commonnamevalue,
                        totalreasult, 
                        message,
                        itemname, 
                        itemresult, 
                        itemstarttime, 
                        itemendtime, 
                        timestamp, 
                        infoname, 
                        infourl);
                }

                if(targetImages.size() != referenceImages.size())
                {
                    DAVINCI_LOG_ERROR <<"Target Images numbers not the same as reference images, can not show them in Report !" << endl;
                }
                else
                {
                    int size = (int)(targetImages.size());
                    for (int i=0; i < size; i++)
                    {
                        string temptarget = targetImages[i];
                        string tempreference = referenceImages[i];
                        //DAVINCI_LOG_INFO <<"====== Files " << temptarget;

                        string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";                        
                        string resulttype = "type"; //<test-result type="RnR">
                        string typevalue = "RNR"; 
                        string commonnamevalue = currentScriptTmp->GetQsName() + ".qs";
                        string totalreasult = "FAIL"; 
                        string itemname = "unmatchimage";
                        string itemresult = "FAIL";
                        string itemstarttime = "starttime";
                        string itemendtime = "endtime";
                        string opcodename = "OPCODE";
                        string line = "Line";
                        string referencename = referenceImages[i].substr(referenceImages[i].find_last_of('/') + 1);
                        string referenceurl = "./VideoChecker/" + referencename;
                        string targetname = targetImages[i].substr(targetImages[i].find_last_of('/') + 1);
                        string targeturl = "./VideoChecker/" + targetname;
                        testTxtReportSummary->appendCommonNodeUnmatchimage(currentReportSummaryName,
                            resulttype, typevalue, commonnamevalue,totalreasult, itemname, itemresult, 
                            itemstarttime, itemendtime, opcodename, line, referencename, referenceurl, targetname,
                            targeturl
                            );

                    }
                }

                if(0 == dialogImages.size())
                {
                    DAVINCI_LOG_INFO <<"No Crash Dialog is found !" << endl;
                }
                else
                {
                    int size = (int)(dialogImages.size());
                    for (int i=0; i < size; i++)
                    {
                        string temptarget = dialogImages[i];

                        string currentReportSummaryName = TestReport::currentQsLogPath + "\\ReportSummary.xml";                        
                        string resulttype = "type"; //<test-result type="RnR">
                        string typevalue = "RNR"; 
                        string commonnamevalue = currentScriptTmp->GetQsName() + ".qs";
                        string totalreasult = "FAIL"; 
                        string itemname = "crashdialog";
                        string itemresult = "FAIL";
                        string itemstarttime = "starttime";
                        string itemendtime = "endtime";
                        string timestamp = "timestamp";
                        string imagename = dialogImages[i].substr(dialogImages[i].find_last_of('/') + 1);
                        string imageurl = "./DialogChecker/" + imagename;

                        testTxtReportSummary->appendCommonNodeCrashdialog(currentReportSummaryName,
                            resulttype, 
                            typevalue, 
                            commonnamevalue,
                            totalreasult, 
                            itemname, 
                            itemresult, 
                            itemstarttime, 
                            itemendtime, 
                            timestamp, 
                            imagename, 
                            imageurl);
                    }
                }
            }
        }

        //generate logcat
        testReport->GetLogcat(currentScriptTmp->GetQS()->GetPackageName());

        testReport->CopyDaVinciLog();
    }
}
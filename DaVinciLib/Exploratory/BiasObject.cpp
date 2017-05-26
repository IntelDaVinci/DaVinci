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

#include "BiasObject.hpp"

namespace DaVinci
{
    const std::map<int, std::vector<string>> &BiasObject::GetAccountInfo() const
    {
        return biasInfo->acountMap;
    }

    void BiasObject::SetAccountInfo(const map<int, std::vector<string>> &value)
    {
        biasInfo->acountMap = value;
    }

    const Point &BiasObject::GetLeftTopPoint() const
    {
        return biasInfo->leftTopPoint;
    }

    void BiasObject::SetLeftTopPoint(const Point &value)
    {
        biasInfo->leftTopPoint = value;
    }

    const Point &BiasObject::GetRightBottomPoint() const
    {
        return biasInfo->rightBottomPoint;
    }

    void BiasObject::SetRightBottomPoint(const Point &value)
    {
        biasInfo->rightBottomPoint = value;
    }

    const int &BiasObject::GetSeedValue() const
    {
        return biasInfo->seedValue;
    }

    void BiasObject::SetSeedValue(const int &value)
    {
        biasInfo->seedValue = value;
    }

    const int &BiasObject::GetTimeOut() const
    {
        return biasInfo->timeOut;
    }

    void BiasObject::SetTimeOut(const int &value)
    {
        biasInfo->timeOut = value;
    }

    const int &BiasObject::GetLaunchWaitTime() const
    {
        return biasInfo->launchWaitTime;
    }

    void BiasObject::SetLaunchWaitTime(const int &value)
    {
        biasInfo->launchWaitTime = value;
    }

    const int &BiasObject::GetActionWaitTime() const
    {
        return biasInfo->actionWaitTime;
    }

    void BiasObject::SetActionWaitTime(const int &value)
    {
        biasInfo->actionWaitTime = value;
    }

    const int &BiasObject::GetRandomAfterUnmatchAction() const
    {
        return biasInfo->randomAfterUnmatchAction;
    }

    void BiasObject::SetRandomAfterUnmatchAction(const int &value)
    {
        biasInfo->randomAfterUnmatchAction = value;
    }

    const int &BiasObject::GetMaxRandomAction() const
    {
        return biasInfo->maxRandomAction;
    }

    void BiasObject::SetMaxRandomAction(const int &value)
    {
        biasInfo->maxRandomAction = value;
    }

    const double &BiasObject::GetUnmatchRatio() const
    {
        return biasInfo->unmatchRatio;
    }

    void BiasObject::SetUnmatchRatio(const double &value)
    {
        biasInfo->unmatchRatio = value;
    }

    const std::string &BiasObject::GetTestType() const
    {
        return biasInfo->testType;
    }

    void BiasObject::SetTestType(const std::string &value)
    {
        biasInfo->testType = value;
    }

    const std::string &BiasObject::GetOutput() const
    {
        return biasInfo->output;
    }

    void BiasObject::SetOutput(const std::string &value)
    {
        biasInfo->output = value;
    }

    const bool &BiasObject::GetCoverage() const
    {
        return biasInfo->coverage;
    }

    void BiasObject::SetCoverage(const bool &value)
    {
        biasInfo->coverage = value;
    }

    const bool &BiasObject::GetObjectMode() const
    {
        return biasInfo->objectMode;
    }

    void BiasObject::SetObjectMode(const bool &value)
    {
        biasInfo->objectMode = value;
    }

    void BiasObject::SetCompare(const bool &value)
    {
        biasInfo->compare = value;
    }

    const bool &BiasObject::GetSilent() const
    {
        return biasInfo->silent;
    }

    void BiasObject::SetSilent(const bool &value)
    {
        biasInfo->silent = value;
    }

    const bool &BiasObject::GetDownload() const
    {
        return biasInfo->download;
    }

    void BiasObject::SetDownload(const bool &value)
    {
        biasInfo->download = value;
    }

    const double &BiasObject::GetPrice() const
    {
        return biasInfo->price;
    }

    void BiasObject::SetPrice(const double &value)
    {
        biasInfo->price = value;
    }

    const std::string &BiasObject::GetApp() const
    {
        return biasInfo->app;
    }

    void BiasObject::SetApp(const std::string &value)
    {
        biasInfo->app = value;
    }

    const bool &BiasObject::GetNeedAccount() const
    {
        return biasInfo->needAccount;
    }

    void BiasObject::SetNeedAccount(const bool &value)
    {
        biasInfo->needAccount = value;
    }

    const bool &BiasObject::GetNeedAudio() const
    {
        return biasInfo->needAudio;
    }

    void BiasObject::SetNeedAudio(const bool &value)
    {
        biasInfo->needAudio = value;
    }

    const double &BiasObject::GetClickPossibility() const
    {
        return biasInfo->clickPossibility;
    }

    void BiasObject::SetClickPossibility(const double &value)
    {
        biasInfo->clickPossibility = value;
    }

    const double &BiasObject::GetSwipePossibility() const
    {
        return biasInfo->swipePossibility;
    }

    void BiasObject::SetSwipePossibility(const double &value)
    {
        biasInfo->swipePossibility = value;
    }

    const double &BiasObject::GetTextPossibility() const
    {
        return biasInfo->textPossibility;
    }

    void BiasObject::SetTextPossibility(const double &value)
    {
        biasInfo->textPossibility = value;
    }

    const double &BiasObject::GetClickButton() const
    {
        return biasInfo->clickButton;
    }

    void BiasObject::SetClickButton(const double &value)
    {
        biasInfo->clickButton = value;
    }

    const double &BiasObject::GetClickHome() const
    {
        return biasInfo->clickHome;
    }

    void BiasObject::SetClickHome(const double &value)
    {
        biasInfo->clickHome = value;
    }

    const double &BiasObject::GetClickMenu() const
    {
        return biasInfo->clickMenu;
    }

    void BiasObject::SetClickMenu(const double &value)
    {
        biasInfo->clickMenu = value;
    }

    const double &BiasObject::GetClickBack() const
    {
        return biasInfo->clickBack;
    }

    void BiasObject::SetClickBack(const double &value)
    {
        biasInfo->clickBack = value;
    }

    const double &BiasObject::GetUrlPossibility() const
    {
        return biasInfo->urlPossibility;
    }

    void BiasObject::SetUrlPossibility(const double &value)
    {
        biasInfo->urlPossibility = value;
    }

    const double &BiasObject::GetMailPossibility() const
    {
        return biasInfo->mailPossibility;
    }

    void BiasObject::SetMailPossibility(const double &value)
    {
        biasInfo->mailPossibility = value;
    }

    const double &BiasObject::GetPlainPossibility() const
    {
        return biasInfo->plainPossibility;
    }

    void BiasObject::SetPlainPossibility(const double &value)
    {
        biasInfo->plainPossibility = value;
    }

    const std::vector<std::string> &BiasObject::GetPlainList() const
    {
        return biasInfo->plainList;
    }

    void BiasObject::SetPlainList(const std::vector<std::string> &value)
    {
        biasInfo->plainList = value;
    }

    const std::vector<std::string> &BiasObject::GetUrlList() const
    {
        return biasInfo->urlList;
    }

    void BiasObject::SetUrlList(const std::vector<std::string> &value)
    {
        biasInfo->urlList = value;
    }

    const std::vector<std::string> &BiasObject::GetMailList() const
    {
        return biasInfo->mailList;
    }

    void BiasObject::SetMailList(const std::vector<std::string> &value)
    {
        biasInfo->mailList = value;
    }

    const std::vector<std::string> &BiasObject::GetKeywords() const
    {
        return biasInfo->keywords;
    }

    void BiasObject::SetKeywords(const std::vector<std::string> &value)
    {
        biasInfo->keywords = value;
    }

    const std::vector<pair<std::string, std::string>> &BiasObject::GetQScripts() const
    {
        return biasInfo->qscripts;
    }

    void BiasObject::SetQScripts(const std::vector<pair<std::string, std::string>> &value)
    {
        biasInfo->qscripts = value;
    }

    const unordered_map<FailureType, CheckerInfo> &BiasObject::GetCheckers() const
    {
        return biasInfo->checkers;
    }

    void BiasObject::SetCheckers(const unordered_map<FailureType, CheckerInfo> &value)
    {
        biasInfo->checkers = value;
    }

    const unordered_map<EventType, EventInfo> &BiasObject::GetEvents() const
    {
        return biasInfo->events;
    }

    void BiasObject::SetEvents(const unordered_map<EventType, EventInfo> &value)
    {
        biasInfo->events = value;
    }

    const double &BiasObject::GetCmdLaunchProbability() const
    {
        return biasInfo->cmdLaunchProbability;
    }

    void BiasObject::SetCmdLaunchProbability(const double &value)
    {
        biasInfo->cmdLaunchProbability = value;
    }

    const double &BiasObject::GetIconLaunchProbability() const
    {
        return biasInfo->iconLaunchProbability;
    }

    void BiasObject::SetIconLaunchProbability(const double &value)
    {
        biasInfo->iconLaunchProbability = value;
    }

    const std::vector<std::string> &BiasObject::GetLogcatPackageNames() const
    {
        return biasInfo->logcatPackageNames;
    }

    void BiasObject::SetLogcatPackageNames(const std::vector<std::string> &value)
    {
        biasInfo->logcatPackageNames = value;
    }

    const int &BiasObject::GetStaticActionNumber() const
    {
        return biasInfo->staticActionNumber;
    }

    void BiasObject::SetStaticActionNumber(const int &value)
    {
        biasInfo->staticActionNumber = value;
    }

    const bool &BiasObject::GetVideoRecording() const
    {
        return biasInfo->videoRecording;
    }

    void BiasObject::SetVideoRecording(const bool &value)
    {
        biasInfo->videoRecording = value;
    }

    const bool &BiasObject:: GetAudioSaving() const
    {
        return biasInfo->audioSaving;
    }


    void BiasObject::SetAudioSaving(const bool &value)
    {
        biasInfo->audioSaving = value;
    }

    const std::string &BiasObject::GetMatchPattern() const
    {
        return biasInfo->matchPattern;
    }

    void BiasObject::SetMatchPattern(const std::string &value)
    {
        biasInfo->matchPattern = value;
    }

    const double &BiasObject::GetDispatchCommonKeywordPossibility() const
    {
        return biasInfo->dispatchCommonKeywordPossibility;
    }

    void BiasObject::SetDispatchCommonKeywordPossibility(const double &value)
    {
        biasInfo->dispatchCommonKeywordPossibility = value;
    }
        
    const double &BiasObject::GetDispatchAdsPossibility() const
    {
        return biasInfo->dispatchAdsPossibility;
    }

    void BiasObject::SetDispatchAdsPossibility(const double &value)
    {
        biasInfo->dispatchAdsPossibility = value;
    }

    BiasObject::BiasObject()
    {
        biasInfo = boost::shared_ptr<BiasInfo>(new BiasInfo());
    }

    BiasObject::BiasObject(boost::shared_ptr<BiasInfo> bi)
    {
        biasInfo = boost::shared_ptr<BiasInfo>(new BiasInfo());
        SetAccountInfo(bi->acountMap);
        SetSeedValue(bi->seedValue);
        SetTimeOut(bi->timeOut);
        SetLaunchWaitTime(bi->launchWaitTime);
        SetActionWaitTime(bi->actionWaitTime);
        SetRandomAfterUnmatchAction(bi->randomAfterUnmatchAction);
        SetMaxRandomAction(bi->maxRandomAction);
        SetUnmatchRatio(bi->unmatchRatio);
        SetLeftTopPoint(bi->leftTopPoint);
        SetRightBottomPoint(bi->rightBottomPoint);
        SetTestType(bi->testType);
        SetOutput(bi->output);
        SetCoverage(bi->coverage);
        SetObjectMode(bi->objectMode);
        SetCompare(bi->compare);
        SetSilent(bi->silent);
        SetDownload(bi->download);
        SetPrice(bi->price);
        SetApp(bi->app);
        SetNeedAccount(bi->needAccount);
        SetNeedAudio(bi->needAudio);
        SetClickPossibility(bi->clickPossibility);
        SetSwipePossibility(bi->swipePossibility);
        SetTextPossibility(bi->textPossibility);
        SetClickButton(bi->clickButton);
        SetClickHome(bi->clickHome);
        SetClickMenu(bi->clickMenu);
        SetClickBack(bi->clickBack);
        SetUrlPossibility(bi->urlPossibility);
        SetMailPossibility(bi->mailPossibility);
        SetPlainPossibility(bi->plainPossibility);
        SetPlainList(bi->plainList);
        SetUrlList(bi->urlList);
        SetMailList(bi->mailList);
        SetKeywords(bi->keywords);
        SetCmdLaunchProbability(bi->cmdLaunchProbability);
        SetIconLaunchProbability(bi->iconLaunchProbability);
        SetLogcatPackageNames(bi->logcatPackageNames);
        SetQScripts(bi->qscripts);
        SetCheckers(bi->checkers);
        SetEvents(bi->events);
        SetStaticActionNumber(bi->staticActionNumber);
        SetVideoRecording(bi->videoRecording);
        SetAudioSaving(bi->audioSaving);
        SetMatchPattern(bi->matchPattern);
        SetDispatchCommonKeywordPossibility(bi->dispatchCommonKeywordPossibility);
        SetDispatchAdsPossibility(bi->dispatchAdsPossibility);
    }

    void BiasObject::debugShow()
    {
        DAVINCI_LOG_DEBUG << std::string("Account number::     ") << GetAccountInfo().size() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Seed Value:          ") << GetSeedValue() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Region:              ") << GetLeftTopPoint() << std::string(" X ") << GetRightBottomPoint() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Timeout Value:       ") << GetTimeOut() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Test Type:           ") << GetTestType() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Test Output:         ") << GetOutput() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Need Account:        ") << GetNeedAccount() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Need Audio:          ") << GetNeedAudio() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Swipe Possibility:   ") << GetSwipePossibility() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Click Possibility:   ") << GetClickPossibility() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Button:         ") << GetClickButton() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Home:           ") << GetClickHome() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Menu:           ") << GetClickMenu() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Back:           ") << GetClickBack() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Text Possibility:    ") << GetTextPossibility() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Url:            ") << GetUrlPossibility() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Mail:           ") << GetMailPossibility() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("-----Plain:          ") << GetPlainPossibility() << std::endl;
        DAVINCI_LOG_DEBUG << GetPlainList().size() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Plain List Contents:") << std::endl;
        for (auto e : GetPlainList())
        {
            DAVINCI_LOG_DEBUG << std::string("-----") << e << std::endl;
        }
        DAVINCI_LOG_DEBUG << GetUrlList().size() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Url List Contents:") << std::endl;
        for (auto e : GetUrlList())
        {
            DAVINCI_LOG_DEBUG << std::string("-----") << e << std::endl;
        }
        DAVINCI_LOG_DEBUG << GetMailList().size() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Mail List Contents:") << std::endl;
        for (auto e : GetMailList())
        {
            DAVINCI_LOG_DEBUG << std::string("-----") << e << std::endl;
        }
        DAVINCI_LOG_DEBUG << std::string("Cmd Launching Possibility:   ") << GetCmdLaunchProbability() << std::endl;
        DAVINCI_LOG_DEBUG << std::string("Icon Launching Possibility:  ") << GetIconLaunchProbability() << std::endl;
        for (auto e : GetLogcatPackageNames())
        {
            DAVINCI_LOG_DEBUG << std::string("-----") << e << std::endl;
        }
    }
}

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

#ifndef __BIASOBJECT__
#define __BIASOBJECT__

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "UiPoint.hpp"
#include "FailureType.hpp"
#include "EventType.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;

    struct CheckerInfo
    {
        bool enabled;
        int condition;

        CheckerInfo()
        {
        }

        CheckerInfo(bool e, int c)
        {
            enabled = e;
            condition = c;
        }
    };

    struct EventInfo
    {
        bool enabled;
        int number;

        EventInfo()
        {
        }

        EventInfo(bool e, int n)
        {
            enabled = e;
            number = n;
        }
    };

    struct BiasInfo
    {
        /// <summary>
        /// Left Top Point
        /// </summary>
        Point leftTopPoint;

        /// <summary>
        /// Right Bottom Point
        /// </summary>
        Point rightBottomPoint;

        /// <summary>
        /// SeedValue
        /// </summary>
        int seedValue;

        /// <summary>
        /// TimeOut
        /// </summary>
        int timeOut;

        /// <summary>
        /// Launch wait time
        /// </summary>
        int launchWaitTime;

        /// <summary>
        /// Action wait time
        /// </summary>
        int actionWaitTime;

        /// <summary>
        /// Start random action after unmatch
        /// </summary>
        int randomAfterUnmatchAction;

        /// <summary>
        /// Max random actions
        /// </summary>
        int maxRandomAction;

        /// <summary>
        /// Unmatch ratio
        /// </summary>
        double unmatchRatio;

        /// <summary>
        /// TestType
        /// </summary>
        std::string testType;

        /// <summary>
        /// Output
        /// </summary>
        std::string output;

        /// <summary>
        /// Coverage
        /// </summary>
        bool coverage;

        /// <summary>
        /// Object mode
        /// </summary>
        bool objectMode;

        /// <summary>
        /// Compare with UiAutomator
        /// </summary>
        bool compare;

        /// <summary>
        /// Slient mode
        /// </summary>
        bool silent;

        /// <summary>
        /// Download mode
        /// </summary>
        bool download;

        /// <summary>
        /// Price 
        /// </summary>
        double price;

        /// <summary>
        /// Download app name
        /// </summary>
        std::string app;

        /// <summary>
        /// NeedAccount
        /// </summary>
        bool needAccount;

        /// <summary>
        /// NeedAudio
        /// </summary>
        bool needAudio;

        /// <summary>
        /// ClickPossibility
        /// </summary>
        double clickPossibility;

        /// <summary>
        /// SwipePossibility
        /// </summary>
        double swipePossibility;

        /// <summary>
        /// TextPossibility
        /// </summary>
        double textPossibility;

        /// <summary>
        /// ClickButton
        /// </summary>
        double clickButton;

        /// <summary>
        /// ClickHome
        /// </summary>
        double clickHome;

        /// <summary>
        /// ClickMenu
        /// </summary>
        double clickMenu;

        /// <summary>
        /// ClickBack
        /// </summary>
        double clickBack;

        /// <summary>
        /// UrlPossibility
        /// </summary>
        double urlPossibility;

        /// <summary>
        /// MailPossibility
        /// </summary>
        double mailPossibility;

        /// <summary>
        /// PlainPossibility
        /// </summary>
        double plainPossibility;

        /// <summary>
        /// dispatchCommonKeywordPossibility
        /// </summary>
        double dispatchCommonKeywordPossibility;

        /// <summary>
        /// dispatchCommonKeywordPossibility
        /// </summary>
        double dispatchAdsPossibility;

        /// <summary>
        /// PlainList
        /// </summary>
        std::vector<std::string> plainList;

        /// <summary>
        /// UrlList
        /// </summary>
        std::vector<std::string> urlList;

        /// <summary>
        /// MailList
        /// </summary>
        std::vector<std::string> mailList;

        /// <summary>
        /// Keywords
        /// </summary>
        std::vector<std::string> keywords;

        /// <summary>
        /// Command Launch Probability
        /// </summary>
        double cmdLaunchProbability;

        /// <summary>
        /// Icon Launch Probability
        /// </summary>
        double iconLaunchProbability;

        /// <summary>
        /// Log Package Names
        /// </summary>
        std::vector<std::string> logcatPackageNames;

        /// <summary>
        /// Account info
        /// </summary>
        std::map<int, std::vector<string>> acountMap;

        /// <summary>
        /// QScripts
        /// </summary>
        std::vector<pair<std::string, std::string>> qscripts;

        /// <summary>
        /// Checkers
        /// </summary>
        std::unordered_map<FailureType, CheckerInfo> checkers;

        /// <summary>
        /// Events
        /// </summary>
        std::unordered_map<EventType, EventInfo> events;

        int staticActionNumber;

        bool videoRecording;

        bool audioSaving;

        std::string matchPattern;
    };

    /// <summary>
    /// Class BiasObject
    /// </summary>
    class BiasObject : public boost::enable_shared_from_this<BiasObject>
    {
    private:
        boost::shared_ptr<BiasInfo> biasInfo;
    public:
        const std::map<int, std::vector<string>> &BiasObject::GetAccountInfo() const;
        void BiasObject::SetAccountInfo(const map<int, std::vector<string>> &value);

        const Point &GetLeftTopPoint() const;
        void SetLeftTopPoint(const Point &value);

        const Point &GetRightBottomPoint() const;
        void SetRightBottomPoint(const Point &value);

        const int &GetSeedValue() const;
        void SetSeedValue(const int &value);

        const int &GetTimeOut() const;
        void SetTimeOut(const int &value);

        const int &GetLaunchWaitTime() const;
        void SetLaunchWaitTime(const int &value);

        const int &GetActionWaitTime() const;
        void SetActionWaitTime(const int &value);

        const int &GetRandomAfterUnmatchAction() const;
        void SetRandomAfterUnmatchAction(const int &value);

        const int &GetMaxRandomAction() const;
        void SetMaxRandomAction(const int &value);

        const double &GetUnmatchRatio() const;
        void SetUnmatchRatio(const double &value);

        const std::string &GetTestType() const;
        void SetTestType(const std::string &value);

        const std::string &GetOutput() const;
        void SetOutput(const std::string &value);

        const bool &GetCoverage() const;
        void SetCoverage(const bool &value);

        const bool &GetObjectMode() const;
        void SetObjectMode(const bool &value);

        void SetCompare(const bool &value);

        const bool &GetSilent() const;
        void SetSilent(const bool &value);

        const bool &GetDownload() const;
        void SetDownload(const bool &value);

        const double &GetPrice() const;
        void SetPrice(const double &value);

        const std::string &GetApp() const;
        void SetApp(const std::string &value);

        const bool &GetNeedAccount() const;
        void SetNeedAccount(const bool &value);

        const bool &GetNeedAudio() const;
        void SetNeedAudio(const bool &value);

        const double &GetClickPossibility() const;
        void SetClickPossibility(const double &value);

        const double &GetSwipePossibility() const;
        void SetSwipePossibility(const double &value);

        const double &GetTextPossibility() const;
        void SetTextPossibility(const double &value);

        const double &GetClickButton() const;
        void SetClickButton(const double &value);

        const double &GetClickHome() const;
        void SetClickHome(const double &value);

        const double &GetClickMenu() const;
        void SetClickMenu(const double &value);

        const double &GetClickBack() const;
        void SetClickBack(const double &value);

        const double &GetUrlPossibility() const;
        void SetUrlPossibility(const double &value);

        const double &GetMailPossibility() const;
        void SetMailPossibility(const double &value);

        const double &GetPlainPossibility() const;
        void SetPlainPossibility(const double &value);

        const std::vector<std::string> &GetPlainList() const;
        void SetPlainList(const std::vector<std::string> &value);

        const std::vector<std::string> &GetUrlList() const;
        void SetUrlList(const std::vector<std::string> &value);

        const std::vector<std::string> &GetMailList() const;
        void SetMailList(const std::vector<std::string> &value);

        const std::vector<std::string> &GetKeywords() const;
        void SetKeywords(const std::vector<std::string> &value);

        const std::vector<pair<std::string, std::string>> &GetQScripts() const;
        void SetQScripts(const std::vector<pair<std::string, std::string>> &value);

        const std::unordered_map<FailureType, CheckerInfo> &GetCheckers() const;
        void SetCheckers(const std::unordered_map<FailureType, CheckerInfo> &value);

        const std::unordered_map<EventType, EventInfo> &GetEvents() const;
        void SetEvents(const std::unordered_map<EventType, EventInfo> &value);

        const int &GetStaticActionNumber() const;
        void SetStaticActionNumber(const int &value);

        const std::vector<std::string> &GetLogcatPackageNames() const;
        void SetLogcatPackageNames(const std::vector<std::string> &value);

        const double &GetCmdLaunchProbability() const;
        void SetCmdLaunchProbability(const double &value);

        const double &GetIconLaunchProbability() const;
        void SetIconLaunchProbability(const double &value);

        const bool &GetVideoRecording() const;
        void SetVideoRecording(const bool &value);

        const bool &GetAudioSaving() const;
        void SetAudioSaving(const bool &value);

        const std::string &GetMatchPattern() const;
        void SetMatchPattern(const std::string &matchPattern);

        const double &GetDispatchCommonKeywordPossibility() const;
        void SetDispatchCommonKeywordPossibility(const double &value);
        
        const double &GetDispatchAdsPossibility() const;
        void SetDispatchAdsPossibility(const double &value);

        /// <summary>
        /// Construct
        /// </summary>
        BiasObject();

        BiasObject(boost::shared_ptr<BiasInfo> bi);

        /// <summary>
        /// For debug
        /// </summary>
        void debugShow();
    };
}


#endif	//#ifndef __BIASOBJECT__

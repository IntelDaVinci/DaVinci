#ifndef __CRITICALPATHCONSTRUCTOR__
#define __CRITICALPATHCONSTRUCTOR__

#include "../ScriptEngine.h"
#include "../MmTimer.h"
#include "../DaVinci.QScriptEditor.h"
#include "../TestManager.h"
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include "stringconverter.h"

/*
 * Intel confidential -- do not distribute further
 * This source code is distributed covered by "Internal License Agreement -- For Internal Open Source DaVinci Program"
 * Please read and accept license.txt distributed with this package before using this source code
 */

using namespace Emgu::CV;
using namespace Emgu::CV::Structure;

namespace DaVinci
{
    namespace Exploratory
    {
        /// <summary>
        /// ActionSet
        /// </summary>
        class ActionSet : public std::enable_shared_from_this<ActionSet>
        {
            /// <summary>
            /// Image for matching
            /// </summary>
        public:
            std::string imageName = "";

            /// <summary>
            /// Action list for image
            /// </summary>
            std::vector<QSEventAndAction> actionList = std::vector<QSEventAndAction>();
        };

        /// <summary>
        /// Class UiUtil
        /// </summary>
        class CriticalPathConstructor : public std::enable_shared_from_this<CriticalPathConstructor>
        {
        private:
            std::vector<std::string> scriptList = std::vector<std::string>();

            std::shared_ptr<Multimedia::Timer> exploratoryTimer = nullptr;

            std::shared_ptr<Stopwatch> exploratoryWatch = std::make_shared<Stopwatch>();

            std::shared_ptr<Stopwatch> exeWatch = std::make_shared<Stopwatch>();

            double elapsed = 0;

            std::shared_ptr<ActionSet> actionSet = nullptr;

            int actionIndex = 0;

            cv::Mat currentFrame = nullptr;

            Point matchedCenter = Point(0, 0);

            /// <summary>
            /// Construct
            /// </summary>
        public:
            CriticalPathConstructor();

            /// <summary>
            /// Construct
            /// </summary>
            /// <param name="l"></param>
            CriticalPathConstructor(std::vector<std::string> &l);

            //public static int[] actionOpcodeList = { };

    #if !defined(ASUS)
            /// <summary>
            /// Get action line types
            /// </summary>
            /// <param name="scriptFile"></param>
            /// <returns></returns>
            std::vector<DaVinci::QScriptEditor::actionlinetype> getActionLineTypes(const std::string &scriptFile);

            /// <summary>
            /// TODO: extract the common API for Editor and GET
            /// </summary>
            /// <param name="actionLineList"></param>
            /// <param name="timeFile"></param>
            /// <param name="videoFile"></param>
            void transformObjects(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList, const std::string &timeFile, const std::string &videoFile);

            // Get the smallest unused label
        private:
            int getUnusedLabel(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList, std::vector<TestManager::traceInfo> &qsTrace);

            // Check whether a label is already used in the current scripts.
            bool isLabelInScript(int l, std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList);

                    /// <summary>
            /// map actions from script
            /// </summary>
            /// <param name="actionLineList"></param>
            /// <returns></returns>
        public:
            std::vector<ActionSet> mapActionFromScript(std::vector<DaVinci::QScriptEditor::actionlinetype> &actionLineList);

            /// <summary>
            /// transformActionLineTypeToEventAction
            /// </summary>
            /// <param name="actionLine"></param>
            /// <returns></returns>
            static std::shared_ptr<QSEventAndAction> transformActionLineToEventAction(const std::shared_ptr<DaVinci::QScriptEditor::actionlinetype> &actionLine);

            /// <summary>
            /// 
            /// </summary>
            /// <param name="scriptFiles"></param>
            /// <returns></returns>
            std::vector<ActionSet> generateAllCriticalActions(std::vector<std::string> &scriptFiles);
    #endif

        private:
            std::string GetPictureShotFileName(const std::string &videoFile);

            bool isLabelInQTS(int l, std::vector<TestManager::traceInfo> &qsTrace);

            /// <summary>
            /// Execute a QS event during exploration
            /// </summary>
            /// <param name="qs_event">QS event</param>
            /// <param name="frame">The current image</param>
            /// <param name="matchedCenter">The matched center point</param>
        public:
            static void ExecuteQSInstructionForExploration(const std::shared_ptr<QSEventAndAction> &qs_event, const cv::Mat &frame, Point matchedCenter);

        private:
            void StartExploratoryTimer();

            void StopExploratoryTimer();

            double GetQSElapsedMillisecondsForExploratory();

            void ExploratoryHandler(const std::shared_ptr<void> &sender, const std::shared_ptr<EventArgs> &e);

            // TODO
            /// <summary>
            /// 
            /// </summary>
            /// <param name="frame"></param>
            /// <returns></returns>
        public:
            bool isCriticalPath(const cv::Mat &frame);

            /// <summary>
            /// 
            /// </summary>
            /// <param name="currentSet"></param>
            /// <param name="frame"></param>
            void executeCriticalActions(const std::shared_ptr<ActionSet> &currentSet, const cv::Mat &frame);

        };
    }
}


#endif	//#ifndef __CRITICALPATHCONSTRUCTOR__

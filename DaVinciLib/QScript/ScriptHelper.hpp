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

#ifndef __SCRIPT_HELPER_HPP__
#define __SCRIPT_HELPER_HPP__

#include "boost/chrono/chrono.hpp"
#include "boost/filesystem.hpp"
#include "opencv2/opencv.hpp"

#include <unordered_map>

#include "CrossRecognizer.hpp"
#include "QScript.hpp"
#include "Shape.hpp"
#include "ShapeExtractor.hpp"
#include "FeatureMatcher.hpp"
#include "KeyboardRecognizer.hpp"
#include "AndroidBarRecognizer.hpp"

#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include "FileCommon.hpp"

#include "ObjectUtil.hpp"
#include "TextUtil.hpp"
#include "ShapeUtil.hpp"

namespace DaVinci
{
    using namespace std;

    enum KeyboardInputType
    {
        KeyboardInput_None = 0,
        KeyboardInput_Only = 1,
        KeyboardInput_Command = 2
    };

    class ScriptHelper
    {
    public:
        // Attention: serialized struct, if member changed, should sync it in SerializationUtil.hpp
        struct QSPreprocessInfo
        {
            Mat frame;
            Rect mainROI;
            int endIp;
            vector<boost::shared_ptr<Shape>> shapes;

            vector<boost::shared_ptr<Shape>> alignShapes;
            ShapeAlignOrientation alignOrientation;
            int clickedAlignShapeIndex;

            int startLine;
            int endLine;

            Point framePoint;
            Point devicePoint;

            bool isTouchMove;
            bool isTouchAction;
            boost::shared_ptr<Shape> clickedShape;
            Rect clickedTextRect;
            string clickedZhText;
            string clickedEnText;

            KeyboardInputType isVirtualKeyboard;
            string keyboardInput;

            bool isVirtualNavigation;
            bool hasVirtualNavigation;
            Rect navigationBarRect;
            NavigationBarLocation barLocation;
            int navigationEvent;

            bool isAdvertisement;
            bool isPopUpWindow;
            string focusWindow;
            vector<string> windows;

            QSPreprocessInfo()
            {
                frame = Mat();
                alignOrientation = ShapeAlignOrientation::UNKNOWN;
                clickedAlignShapeIndex = -1;

                framePoint = EmptyPoint;
                devicePoint = EmptyPoint;

                isTouchMove = false;
                isTouchAction = false;
                clickedShape = nullptr;
                clickedZhText = "";
                clickedEnText = "";
                clickedTextRect = EmptyRect;

                isVirtualKeyboard = KeyboardInputType::KeyboardInput_None;
                keyboardInput = "";

                isVirtualNavigation = false;
                hasVirtualNavigation = false;
                navigationBarRect = EmptyRect;
                navigationEvent = 0;
                barLocation = NavigationBarLocation::BarLocationUnknown;

                isAdvertisement = false;
                isPopUpWindow = false;
                focusWindow = "";
            }
        };

        ScriptHelper();

        explicit ScriptHelper(const boost::shared_ptr<QScript> &qs);
        void Init();
        void SetCurrentQS(const boost::shared_ptr<QScript> &qs);
        map<int, ScriptHelper::QSPreprocessInfo> GenerateQSPreprocessInfo(string folder, string matchPattern = "");
        map<int, ScriptHelper::QSPreprocessInfo> OrganizeQSPreprocessInfoForKeyboard(map<int, ScriptHelper::QSPreprocessInfo> qsInfos);
        void ProcessTouchDownAction(ScriptHelper::QSPreprocessInfo& qsBlockInfo, string keyStr, int start, int end, Point where, int x, int y, Size deviceSize, Mat image, Orientation orientation, string qsFilename, 
            int navigationBarHeight, int statusBarHeight, boost::filesystem::path directory, string osVersion, string matchPattern);
        static void ProcessWindowInfo(std::unordered_map<string, string> keyValueMap, Rect& sbRect, Rect& nbRect, bool& hasKeyboard, Rect& fwRect);
        static string FindAdditionalInfo(std::unordered_map<string, string> keyValueMap, string key);
        static vector<string> GetTargetWindows(vector<string>& lines, string packageName, string& focusWindow);
        static SwipeDirection GetDragDirection(vector<Point>& points, Orientation orientation);

        bool NeedTextPreprocess(string matchPattern);
        bool NeedShapePreprocess(string matchPattern);

        bool FillZhTexts(Mat textRoiFrame, Rect textRect, ScriptHelper::QSPreprocessInfo& qsBlockInfo);
        bool FillEnTexts(Mat textRoiFrame, Rect textRect, ScriptHelper::QSPreprocessInfo& qsBlockInfo);

        Orientation GetActionOrientation(boost::shared_ptr<QScript> script, int ip);

        Rect GetBoundingRect(const Mat &image, Point where, int boundThreshold = 3);

        static const int nearKShapesToClickPoint = 5;

        static bool GenActionSequenceToRnR(string actionSequenceFile);

        static Rect StringToRect(string rect);
    private:
        Rect GetBoundingRect(const Mat &image, Point where, size_t &numRequiredKeyPoints, boost::shared_ptr<FeatureMatcher> &modelMatcher, const boost::shared_ptr<FeatureMatcher> &imageMatcher = nullptr, int keyPointsStep = 50, int boundThreshold = 3);
        bool IsRectMatch(const boost::shared_ptr<FeatureMatcher> &matcher, Rect expectedRoi, int boundThreshold = 3);

        boost::shared_ptr<QScript> qsPtr;
        boost::shared_ptr<ShapeExtractor> shapeExtractor;
        boost::shared_ptr<AndroidBarRecognizer> barRecognizer;
        boost::shared_ptr<KeyboardRecognizer> keyboardRecognizer;
        boost::shared_ptr<CrossRecognizer> crossRecognizer;
    };
}

#endif
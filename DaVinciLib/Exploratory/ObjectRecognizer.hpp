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

#ifndef __OBJECTRECOGNIZER__
#define __OBJECTRECOGNIZER__

#include "ObjectUtil.hpp"
#include "AndroidBarRecognizer.hpp"
#include "HistogramRecognize.hpp"
#include "UiState.hpp"
#include "DeviceControlCommon.hpp"
#include "DaVinciCommon.hpp"
#include "ButtonDetection.hpp"
#include "ViewHierarchyParser.hpp"
#include "boost/core/null_deleter.hpp"
#include "ObjectLabel.hpp"

namespace DaVinci
{
    using namespace std;
    using namespace cv;
    using namespace caffe;

    enum ObjectType
    {
        /// <summary>
        /// Object character
        /// </summary>
        OBJECT_CHARACTER = 0,
        /// <summary>
        /// Object image
        /// </summary>
        OBJECT_IMAGE = 1,
        /// <summary>
        /// Object id
        /// </summary>
        OBJECT_ID = 2
    };

    /// <summary>
    /// Class ObjectRecognizer
    /// </summary>
    class ObjectRecognizer
    {

    private:

        AndroidBarRecognizer barRecog;

        HistogramRecognize histoRecog;

        boost::shared_ptr<ButtonDetection> buttonDecection;
        boost::shared_ptr<FasterRcnnRecognize> objectLabelDetection;
        boost::shared_ptr<ViewHierarchyParser> viewHierarchyParser;

    public:
        /// <summary>
        /// Construct
        /// </summary>
        ObjectRecognizer();

        /// <summary>
        /// Construct
        /// </summary>
        ObjectRecognizer(int frameWidth);

        void InitializeFasterRCNNObjectRecognizer();
        /// <summary>
        /// Check targetRect contains any rect in the rectangle list and record the contained rectangle list
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="targetRect"></param>
        /// <param name="outRectList"></param>
        /// <returns></returns>
        bool isContainRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList);

        /// <summary>
        /// Check targetRect should merge any rect in the rectangle list and record the shouldMerge rectangle list
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="targetRect"></param>
        /// <param name="outRectList"></param>
        /// <returns></returns>
        bool isMergeRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList);

        /// <summary>
        /// Check targetRect is intersected by any rect in the rectangle list
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="targetRect"></param>
        /// <param name="outRectList"></param>
        /// <returns></returns>
        bool isIntersectedRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList);


        /// <summary>
        /// Optimize for intersected rectangles
        /// </summary>
        /// <param name="rectList"></param>
        /// <returns></returns>
        std::vector<Rect> optimizeForIntersectedRectangles(std::vector<Rect> &rectList);

        /// <summary>
        /// Optimize for contained rectangles
        /// </summary>
        /// <param name="rectList"></param>
        /// <param name="minArea"></param>
        /// <returns></returns>
        std::vector<Rect> optimizeForContainedRectangles(std::vector<Rect> &rectList, int minArea = 50000);

        /// <summary>
        /// Check whether two rectangles should merge or not
        /// </summary>
        /// <param name="rect1"></param>
        /// <param name="rect2"></param>
        /// <param name="distance"></param>
        /// <returns></returns>
        bool shouldMerge(Rect rect1, Rect rect2, int distance = 3);

        /// <summary>
        /// Optimize for merged rectangles
        /// </summary>
        /// <param name="rectList"></param>
        /// <returns></returns>
        std::vector<Rect> optimizeForMergedRectangles(std::vector<Rect> &rectList);

        /// <summary>
        /// Detect image objects
        /// </summary>
        /// <param name="rotatedMainFrame"></param>
        /// <param name="rotatedFrameSize"></param>
        /// <param name="minArea"></param>
        /// <param name="maxArea"></param>
        /// <returns></returns>
        boost::shared_ptr<UiState> DetectImageObjects(const cv::Mat &rotatedMainFrame, const cv::Mat &rotatedFrame, Rect rectOnRotatedFrame, int minArea, int maxArea);

        /// <summary>
        /// Detect id objects
        /// </summary>
        /// <param name="lines"></param>
        /// <param name="rotatedFrameSize"></param>
        /// <param name="deviceSize"></param>
        /// <param name="fw"></param>
        /// <param name="minArea"></param>
        /// <param name="maxArea"></param>
        /// <returns></returns>
        boost::shared_ptr<UiState> DetectIdObjects(const cv::Mat &rotatedFrame, vector<string>& lines, Size rotatedFrameSize, Size deviceSize, Rect fw, int minArea, int maxArea);

        /// <summary>
        /// Create Ui State
        /// </summary>
        /// <param name="dumpedXmlPath"></param>
        /// <returns></returns>
        boost::shared_ptr<UiState> createUiState(const std::string &dumpedXmlPath);

        /// <summary>
        /// Create General UI State
        /// </summary>
        /// <param name="rotatedFrame">the image to be processed</param>
        /// <param name="lines">the lines to be parsed</param>
        /// <param name="objecType">object type for image or character recognition</param>
        /// <param name="minArea">allowed minArea</param>
        /// <param name="maxArea">allowed minArea</param>
        /// <param name="deviceSize">the device size</param>
        /// <param name="fw">the focus window</param>
        /// <returns></returns>
        boost::shared_ptr<UiState> recognizeObjects(const cv::Mat &rotatedFrame, vector<string>& lines, ObjectType objecType = OBJECT_IMAGE, int minArea = 500, int maxArea = 1000000, Size deviceSize = Size(), Rect fw = EmptyRect, Rect fwOnFrame = EmptyRect);

        bool containBigRectangle(std::vector<Rect> &rectList, int minArea, int &count);

        bool DetectObjectLabels(const cv::Mat &rotatedFrame, vector<ObjectLabelNode>& objectNodes);

        vector<ObjectLabelNode> SelectObjectLables(vector<ObjectLabelNode> objectNodes, double minConfidence, int totalNumberThreshold);

        boost::shared_ptr<UiState> LabelObjectToUiObject(vector<ObjectLabelNode> objectNodes, const cv::Mat &rotatedFrame, Rect rectOnRotatedFrame);


    private:
        bool hasSameElement(std::vector<Rect> &l1, std::vector<Rect> &l2);

        vector<Rect> SelectValidTopRects(const vector<Rect>& shapeRects, double lowSideRatio, double highSideRatio, size_t totalNumberThreshold);
    };
}


#endif	//#ifndef __OBJECTRECOGNIZER__

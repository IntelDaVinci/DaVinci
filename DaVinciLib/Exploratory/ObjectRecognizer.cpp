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

#include "ObjectRecognizer.hpp"
#include "UiStateGeneral.hpp"
#include "UiStateEditText.hpp"
#include "UiStateClickable.hpp"
#include "DaVinciCommon.hpp"
#include "DeviceControlCommon.hpp"
#include "ObjectCommon.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "TestManager.hpp"
#include "TextUtil.hpp"

namespace DaVinci
{
    ObjectRecognizer::ObjectRecognizer()
    {
        string clickableTrainFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/appbutton_iter_4000"); 
        string clickableNetFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/appbutton.prototxt"); 
        string clickableMeanFile= TestManager::Instance().GetDaVinciResourcePath("Caffedata/mean.binaryproto"); 
        buttonDecection = boost::shared_ptr<ButtonDetection>(new ButtonDetection(clickableNetFile, clickableTrainFile, clickableMeanFile));

        InitializeFasterRCNNObjectRecognizer();
        viewHierarchyParser = boost::shared_ptr<ViewHierarchyParser>(new ViewHierarchyParser());
    }

    void ObjectRecognizer::InitializeFasterRCNNObjectRecognizer()
    {
        string rpnNetDefFile = TestManager::Instance().GetDaVinciResourcePath("ObjectData/proposal_test.prototxt");
        string rpnNetTrainedFile = TestManager::Instance().GetDaVinciResourcePath("ObjectData/rnr_faster_rcnn_iter_70000.caffemodel");
        string frcnDefFile = TestManager::Instance().GetDaVinciResourcePath("ObjectData/detection_test.prototxt");
        string frcnTrainFile = TestManager::Instance().GetDaVinciResourcePath("ObjectData/rnr_faster_rcnn_iter_70000.caffemodel");
        string lastSharedOutputBlobName = "conv5";
        FasterRcnnParams param;

        param.netName = "ZF";
        for(int objectIndex = 0; objectIndex < OBJECT_UNKNOWN; objectIndex++)
        {
            param.classes->push_back(objectLabelString[objectIndex]);
        }

        (*param.imageMeans) = Vec3f(102.9801, 115.9465, 122.7717);
        param.rpnNetDefFile = const_cast<char*>(rpnNetDefFile.c_str());
        param.rpnNetTrainedFile = const_cast<char*>(rpnNetTrainedFile.c_str());
        param.frcnDefFile = const_cast<char*>(frcnDefFile.c_str());
        param.frcnTrainFile = const_cast<char*>(frcnTrainFile.c_str());
        param.lastSharedOutputBlobName = const_cast<char*>(lastSharedOutputBlobName.c_str());

        if (DeviceManager::Instance().HasCudaSupport())
        {
            DeviceManager::Instance().EnableGpuAcceleration(true);
            param.useGpu = true;
        }
        else
        {
            param.useGpu = false;
        }

        objectLabelDetection = boost::shared_ptr<FasterRcnnRecognize>(new FasterRcnnRecognize(param));
    }

    bool ObjectRecognizer::isContainRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList)
    {
        bool isContain = false;
        outRectList = std::vector<Rect>();

        for (auto r : rectList)
        {
            if (ContainRect(targetRect, r))
            {
                outRectList.push_back(r);
                isContain = true;
            }
        }

        return isContain;
    }

    bool ObjectRecognizer::isMergeRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList)
    {
        bool sMerge = false;
        outRectList = std::vector<Rect>();

        for (auto r : rectList)
        {
            if (shouldMerge(targetRect, r))
            {
                outRectList.push_back(r);
                sMerge = true;
            }
        }

        return sMerge;
    }

    bool ObjectRecognizer::isIntersectedRectangle(std::vector<Rect> &rectList, Rect targetRect, std::vector<Rect> &outRectList)
    {
        bool isIntersect = false;
        outRectList = std::vector<Rect>();

        for (auto rect : rectList)
        {
            if (IntersectWith(rect, targetRect))
            {
                outRectList.push_back(rect);
                isIntersect = true;
            }
        }

        return isIntersect;
    }

    std::vector<Rect> ObjectRecognizer::optimizeForIntersectedRectangles(std::vector<Rect> &rectList)
    {
        std::vector<Rect> intersectList;
        std::vector<Rect> noIntersectList = std::vector<Rect>();
        std::vector<Rect> failedList = std::vector<Rect>();
        for (auto copyRect : rectList)
        {
            if (!isIntersectedRectangle(noIntersectList, copyRect, intersectList))
            {
                noIntersectList.push_back(copyRect);
            }
            else
            {
                intersectList = std::vector<Rect>(optimizeForIntersectedRectangles(intersectList));

                for (auto intersectRect : intersectList)
                {
                    if (copyRect.width * copyRect.height > intersectRect.width * intersectRect.height)
                    {
                        vector<Rect>::iterator iter = std::find(noIntersectList.begin(), noIntersectList.end(), intersectRect);
                        if (iter != noIntersectList.end())
                        {
                            noIntersectList.erase(iter);
                        }

                        if (std::find(noIntersectList.begin(), noIntersectList.end(), copyRect) == noIntersectList.end())
                        {
                            if (!HasSameElements(failedList, noIntersectList))
                            {
                                noIntersectList.push_back(copyRect);
                            }
                        }
                    }
                    else
                    {
                        vector<Rect>::iterator iter = std::find(noIntersectList.begin(), noIntersectList.end(), copyRect);
                        if (iter != noIntersectList.end())
                        {
                            noIntersectList.erase(iter);
                        }

                        if (std::find(noIntersectList.begin(), noIntersectList.end(), intersectRect) == noIntersectList.end())
                        {
                            noIntersectList.push_back(intersectRect);
                        }

                        if (std::find(failedList.begin(), failedList.end(), intersectRect) == failedList.end())
                        {
                            failedList.push_back(intersectRect);
                        }
                    }
                }

                intersectList.clear();
                failedList.clear();
            }
        }

        return noIntersectList;
    }

    bool ObjectRecognizer::containBigRectangle(std::vector<Rect> &rectList, int minArea, int &count)
    {
        bool flag = false;
        count = 0;
        for (auto rect : rectList)
        {
            if (rect.width * rect.height > minArea)
            {
                flag = true;
                count++;
            }
        }

        return flag;
    }

    std::vector<Rect> ObjectRecognizer::optimizeForContainedRectangles(std::vector<Rect> &rectList, int minArea)
    {
        std::vector<Rect> containList;
        std::vector<Rect> noContainList = std::vector<Rect>();
        for (auto copyRect : rectList)
        {
            if (!isContainRectangle(noContainList, copyRect, containList))
            {
                noContainList.push_back(copyRect);
            }
            else
            {
                int count;
                bool contain = containBigRectangle(containList, minArea, count);
                // not contain: remain all the small rectangles and do not add the big one
                // contain: the small rectangle is the only contained one
                if (!contain || (contain && count == 1 && containList.size() == 1))
                {
                    for (auto containRect : containList)
                    {
                        noContainList.erase(std::remove(noContainList.begin(), noContainList.end(), containRect), noContainList.end());
                    }
                    noContainList.push_back(copyRect);
                }

                containList.clear();
            }
        }

        return noContainList;
    }

    bool ObjectRecognizer::shouldMerge(Rect rect1, Rect rect2, int distance)
    {
        /*   _    --
        |_|  |  |
        |  |
        --
        * case 1 */
        /*   --    _
        |  |  |_|
        |  |
        --
        * case 2 */

        /*   --
        |  |
        --
        -----
        |     |
        -----
        * case 3 */
        /*  -----
        |     |
        -----
        ---
        |   |
        ---
        * case 4 */

        // case 1 and 2 are for left and right mode
        // allow max distance between left and right and min distance between two tops and two bottoms
        // case 3 and 4 are for top and down mode
        // allow max distance between top and down and min distance between two lefts and two rights

        Rect sourceRect = rect1;
        Rect targetRect = rect2;

        // exchange the rect
        if (targetRect.x < sourceRect.x)
        {
            Rect tmpRect = targetRect;
            targetRect = sourceRect;
            sourceRect = tmpRect;
        }

        if (targetRect.x >= sourceRect.x && targetRect.x >= sourceRect.x + sourceRect.width && targetRect.y < sourceRect.y)
        {
            if (targetRect.x - (sourceRect.x + sourceRect.width) <= distance && sourceRect.y >= targetRect.y + distance && sourceRect.y + sourceRect.height + distance <= targetRect.y + targetRect.height)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        if (targetRect.x >= sourceRect.x && targetRect.x >= sourceRect.x + sourceRect.width && targetRect.y > sourceRect.y)
        {
            if (targetRect.x - (sourceRect.x + sourceRect.width) <= distance && targetRect.y >= sourceRect.y + distance && targetRect.y + targetRect.height + distance <= sourceRect.y + sourceRect.height)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // exchange the rect
        if (targetRect.y < sourceRect.y)
        {
            Rect tmpRect = targetRect;
            targetRect = sourceRect;
            sourceRect = tmpRect;
        }

        if (targetRect.y >= sourceRect.y && targetRect.y >= sourceRect.y + sourceRect.height && targetRect.x < sourceRect.x)
        {
            if (targetRect.y - (sourceRect.y + sourceRect.height) <= distance && sourceRect.x >= targetRect.x + distance && sourceRect.x + sourceRect.width + distance <= targetRect.x + targetRect.width)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        if (targetRect.y >= sourceRect.y && targetRect.y >= sourceRect.y + sourceRect.height && targetRect.x > sourceRect.x)
        {
            if (targetRect.y - (sourceRect.y + sourceRect.height) <= distance && targetRect.x >= sourceRect.x + distance && targetRect.x + targetRect.width + distance <= sourceRect.x + sourceRect.width)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        return false;
    }

    std::vector<Rect> ObjectRecognizer::optimizeForMergedRectangles(std::vector<Rect> &rectList)
    {
        std::vector<Rect> mergedList = std::vector<Rect>();
        std::vector<Rect> sMergeList = std::vector<Rect>();
        for (auto copyRect : rectList)
        {
            if (!isContainRectangle(mergedList, copyRect, sMergeList))
            {
                mergedList.push_back(copyRect);
            }
            else
            {
                Rect rect = ObjectUtil::Instance().MergeRectangles(sMergeList);
                for (auto mRect : sMergeList)
                {
                    mergedList.erase(std::remove(mergedList.begin(), mergedList.end(), mRect), mergedList.end());
                }

                mergedList.push_back(rect);
            }
        }

        return mergedList;
    }

    boost::shared_ptr<UiState> ObjectRecognizer::DetectImageObjects(const cv::Mat &rotatedMainFrame, const cv::Mat &rotatedFrame, Rect rectOnRotatedFrame, int minArea, int maxArea)
    {
        boost::shared_ptr<UiState> newState = boost::shared_ptr<UiState>(new UiState());

        if (rotatedMainFrame.empty())
            return newState;

        cv::Mat imageCopy, rotatedFrameCopy;
        imageCopy = rotatedMainFrame.clone();
        rotatedFrameCopy = rotatedFrame.clone();

        bool useNormalMser = false;
#ifdef _DEBUG
        Mat debugFrame = imageCopy.clone();
#endif
#ifdef DAVINCI_LIB_STATIC //since caffe mean file's issue, will fix it later
        useNormalMser = true;
#endif
        vector<Rect> shapeRects;
        // resize image
        Mat imageResize;
        vector<Rect> outputRects;
        //resize(imageCopy, imageResize, Size(), 0.5, 0.5);
        imageResize = imageCopy;
        try
        {
            //GPU may have been occupied
            buttonDecection->Detect(imageResize, outputRects, false, -1, 10, 5, 25000, 5, 250, 12500);
        }
        catch(...)
        {
            useNormalMser = true;
        }
#ifdef _DEBUG
        if(!useNormalMser)
        {
            DAVINCI_LOG_DEBUG<<"Caffe enable"<<endl;
        }
#endif
        if(!useNormalMser)
        {
            for (auto rect : outputRects)
            {
                Rect resizeRect( rect.x,  rect.y, rect.width,  rect.height); 
                shapeRects.push_back(resizeRect);
#ifdef _DEBUG
                DrawRect(debugFrame, resizeRect);
#endif
            }
#ifdef _DEBUG
            SaveImage(debugFrame, "caffe_button.png");
#endif
        }

        vector<Rect> textRects = TextUtil::Instance().ExtractSceneTexts(imageCopy, Rect(EmptyPoint, imageCopy.size()));
        for (auto  r: textRects)
        {
            if(r.area() < minArea || r.area() > maxArea)
                continue;
            shapeRects.push_back(r);
        }

        shapeRects = SelectValidTopRects(shapeRects, 0.1, 10.0, 20);
        shapeRects = ObjectUtil::Instance().RelocateRectangleList(rotatedFrame.size(), shapeRects, rectOnRotatedFrame);

        for (auto r : shapeRects)
        {
            boost::shared_ptr<UiStateObject> object = boost::shared_ptr<UiStateObject>(new UiStateGeneral(r));
            object->SetClickable(true);
            newState->AddUiStateObject(object);
#ifdef _DEBUG
            rectangle(rotatedFrameCopy, r, Red, 2);
#endif
        }

#ifdef _DEBUG
        imwrite("final.png", rotatedFrameCopy);
#endif
        return newState;
    }

    vector<Rect> ObjectRecognizer::SelectValidTopRects(const vector<Rect>& shapeRects, double lowSideRatio, double highSideRatio, size_t totalNumberThreshold)
    {
        vector<Rect> topRects;
        vector<Rect> allValidRects = vector<Rect>();

        for (auto r : shapeRects)
        {
            double sideRatio = (double)r.width / (double)r.height;
            if (sideRatio < lowSideRatio || sideRatio > highSideRatio)
            {
                continue;
            }
            allValidRects.push_back(r);
        }

        allValidRects = ObjectUtil::Instance().RemoveSimilarRects(allValidRects, 10);

        allValidRects = ObjectUtil::Instance().RemoveContainedRects(allValidRects);

        SortRects(allValidRects, UType::UTYPE_AREA, false);

        size_t number = 0;
        for (auto r : allValidRects)
        {
            if (number > totalNumberThreshold)
                break;
            topRects.push_back(r);
            number ++;
        }
        return topRects;
    }

    boost::shared_ptr<UiState> ObjectRecognizer::createUiState(const std::string &dumpedXmlPath)
    {
        std::vector<std::string> outerLayouts = std::vector<std::string> ();
        outerLayouts.push_back("android.widget.FrameLayout");
        outerLayouts.push_back("android.widget.LinearLayout");
        outerLayouts.push_back("android.widget.RelativeLayout");
        outerLayouts.push_back("android.widget.TableLayout");
        outerLayouts.push_back("android.widget.GridLayout");

        boost::shared_ptr<UiState> newState = boost::shared_ptr<UiState>(new UiState());
        std::string package = "";

        boost::shared_ptr<XercesDOMParser> xmlParser = boost::shared_ptr<XercesDOMParser>(new XercesDOMParser(), boost::null_deleter());
        xmlParser->parse(dumpedXmlPath.c_str());
        boost::shared_ptr<DOMDocument> domDoc = boost::shared_ptr<DOMDocument>(xmlParser->getDocument(), boost::null_deleter());

        std::string xmlPath = "/hierarchy/node/node/node/node/node/node/node"; // Enhancement
        vector<boost::shared_ptr<DOMNode>> nodeList = GetNodesFromXPath(domDoc, xmlPath);

        int clickableNum = 0;
        int editableNum = 0;
        for (auto node: nodeList)
        {
            DOMNamedNodeMap* attributes = node->getAttributes();
            const XMLSize_t attributeCount = attributes->getLength();

            bool attributeFound = false;
            bool packageFound = false;
            bool editclassFound = false;
            for(XMLSize_t ix = 0; ix < attributeCount; ++ix)
            {
                DOMNode* attribute = attributes->item(ix);
                if (boost::equals(attribute->getNodeName(), "class") && FindElementIterFromVector(outerLayouts, XMLStrToStr(attribute->getNodeValue())) != outerLayouts.end())
                {
                    attributeFound = true;
                    continue;
                }
                if(attributeFound && boost::equals(attribute->getNodeName(), "package"))
                {
                    package = XMLStrToStr(attribute->getNodeValue());
                    newState->SetCurrentPackageName(package);
                    packageFound = true;
                    break;
                }
                if (boost::equals(attribute->getNodeName(), "class") && boost::equals(attribute->getNodeValue(), "android.widget.EditText"))
                {
                    editclassFound = true;
                    boost::shared_ptr<UiStateObject> state = boost::shared_ptr<UiStateObject>(new UiStateEditText(*node));
                    state->SetClickable(false);
                    newState->AddUiStateObject(state);
                    editableNum++;
                    continue;
                }
                if(!editclassFound && boost::equals(attribute->getNodeName(), "clickable") && boost::equals(attribute->getNodeValue(), "true"))
                {
                    boost::shared_ptr<UiStateObject> state = boost::shared_ptr<UiStateObject>(new UiStateClickable(*node));
                    state->SetClickable(true);
                    newState->AddUiStateObject(state);
                    clickableNum++;
                }
            }
        }

        newState->SetEditTextNum(editableNum);
        if(clickableNum == 0)
        {
            newState->SetNoClickable(true);
        }
        else
        {
            newState->SetClickObjNum(clickableNum);
        }

        return newState;
    }

    boost::shared_ptr<UiState> ObjectRecognizer::DetectIdObjects(const cv::Mat &rotatedFrame, vector<string>& lines, Size rotatedFrameSize, Size deviceSize, Rect fw, int minArea, int maxArea)
    {
        viewHierarchyParser->CleanTreeControls();
        boost::shared_ptr<ViewHierarchyTree> root = viewHierarchyParser->ParseTree(lines);
        std::unordered_map<string, vector<Rect>> idMaps = viewHierarchyParser->FindAllValidIds(fw);

        vector<Rect> rects;
        for(std::unordered_map<string, vector<Rect>>::const_iterator it = idMaps.begin(); it != idMaps.end(); ++it) 
        {
            vector<Rect> idRects = it->second;
            int rectSize = (int)(idRects.size());
            if(rectSize == 1)
            {
                rects.insert(rects.begin(), idRects[0]);
            }
            else
            {
                AddVectorToVector(rects, idRects);
            }
        }

        boost::shared_ptr<UiState> newState = boost::shared_ptr<UiState>(new UiState());
        double widthRatio = 1.0;
        double heightRatio = 1.0;

        if (rotatedFrameSize.width > rotatedFrameSize.height)
        {
            widthRatio = rotatedFrameSize.width / (double)(max(deviceSize.width, deviceSize.height));
            heightRatio = rotatedFrameSize.height / (double)(min(deviceSize.width, deviceSize.height));
        }
        else
        {
            widthRatio = rotatedFrameSize.width / (double)(min(deviceSize.width, deviceSize.height));
            heightRatio = rotatedFrameSize.height / (double)(max(deviceSize.width, deviceSize.height));
        }

#ifdef _DEBUG
        Mat debugFrame = rotatedFrame.clone();
#endif
        vector<Rect> idRects;
        std::vector<pair<Rect, string>> idInfoRects;

        for (auto pointOnDevice : rects)
        {
            if(pointOnDevice.area() < minArea || pointOnDevice.area() > maxArea)
                continue;
            Point tl = pointOnDevice.tl();
            Point br = pointOnDevice.br();
            Point tlOnRotatedFrame, brOnRotatedFrame;
            tlOnRotatedFrame.x = (int)(widthRatio * tl.x);
            tlOnRotatedFrame.y = (int)(heightRatio * tl.y);
            brOnRotatedFrame.x = (int)(widthRatio * br.x);
            brOnRotatedFrame.y = (int)(heightRatio * br.y);
            Rect rectOnRotatedFrame(tlOnRotatedFrame.x, tlOnRotatedFrame.y, brOnRotatedFrame.x - tlOnRotatedFrame.x, brOnRotatedFrame.y - tlOnRotatedFrame.y);
            idRects.push_back(rectOnRotatedFrame);

            for (auto eachIdMap : idMaps)
            {
                if (std::find(eachIdMap.second.begin(), eachIdMap.second.end(), pointOnDevice) != eachIdMap.second.end())
                {
                    idInfoRects.push_back(make_pair(rectOnRotatedFrame, eachIdMap.first));
                }
            }
        }

        idRects = SelectValidTopRects(idRects, 0.5, 4, 30);

        for (auto r : idRects)
        {
            boost::shared_ptr<UiStateObject> object = boost::shared_ptr<UiStateObject>(new UiStateGeneral(r));
            object->SetClickable(true);
            object->SetControlState(true);
            for (auto idInfoRect : idInfoRects)
            {
                if (idInfoRect.first == r)
                {
                    object->SetIDInfo(idInfoRect.second);
                }
            }

            newState->AddUiStateObject(object);
#ifdef _DEBUG
            rectangle(debugFrame, r, Red, 2);
#endif
        }

#ifdef _DEBUG
        SaveImage(debugFrame, "ID_ViewHierarchy.png");
#endif
        return newState;
    }

    boost::shared_ptr<UiState> ObjectRecognizer::recognizeObjects(const cv::Mat &rotatedFrame, vector<string>& lines, ObjectType objecType, int minArea, int maxArea, Size deviceSize, Rect fw, Rect fwOnFrame)
    {
        Size rotatedFrameSize = rotatedFrame.size();
        if(objecType == ObjectType::OBJECT_ID)
        {
            return DetectIdObjects(rotatedFrame, lines, rotatedFrameSize, deviceSize, fw, minArea, maxArea);
        }
        else
        {
            Mat rotatedMainFrame; 
            ObjectUtil::Instance().CopyROIFrame(rotatedFrame, fwOnFrame, rotatedMainFrame);
            return DetectImageObjects(rotatedMainFrame, rotatedFrame, fwOnFrame, minArea, maxArea);
        }
    }

    bool ObjectRecognizer::DetectObjectLabels(const cv::Mat &rotatedFrame, vector<ObjectLabelNode>& objectNodes)
    {
        vector<char*> labels;
        vector<float> scores;
        vector<Rect> rects;
        
        try
        {
            if (rotatedFrame.empty())
                return false;

            objectLabelDetection->Predict(rotatedFrame, rects, labels, scores);
            objectNodes = WrapObjectLabelNodes(labels, scores, rects);

            if(objectNodes.empty())
                return false;
            else
                return true;
        }
        catch(std::exception &e)
        {
            DAVINCI_LOG_ERROR << "ObjectRecognizer::DetectObjectLabels exception: " << e.what();
            return false;
        }
    }

    vector<ObjectLabelNode> ObjectRecognizer::SelectObjectLables(vector<ObjectLabelNode> objectNodes, double minConfidence, int totalNumberThreshold)
    {
        vector<ObjectLabelNode> confidenceObjects;
        vector<ObjectLabelNode> topObjectNodes;

        // Filter object by confidence
        for (ObjectLabelNode objectNode : objectNodes)
        {
            if (objectNode.confidence >= minConfidence)
                confidenceObjects.push_back(objectNode);
        }

        // Sort objects by confidence first
        SortObjectLabelsByConfidence(confidenceObjects, false);

        // Sort objects by priority
        SortObjectLabelsByPriority(confidenceObjects, false);

        // Select objects 
        size_t number = 0;
        for (ObjectLabelNode object : confidenceObjects)
        {
            if (number > totalNumberThreshold)
                break;
            topObjectNodes.push_back(object);
            number ++;
        }

        return topObjectNodes;
    }

    boost::shared_ptr<UiState> ObjectRecognizer::LabelObjectToUiObject(vector<ObjectLabelNode> objectNodes, const cv::Mat &rotatedFrame, Rect rectOnRotatedFrame)
    {
        boost::shared_ptr<UiState> newState = boost::shared_ptr<UiState>(new UiState());

        DAVINCI_LOG_INFO << "Lable object number: " << objectNodes.size();

        for (ObjectLabelNode objectNode : objectNodes)
        {
            DAVINCI_LOG_INFO << ObjectLabelToString(objectNode.label);
            DAVINCI_LOG_INFO << objectNode.confidence;

            // relocate the rectangles
            Rect rect = ObjectUtil::Instance().RelocateRectangle(rotatedFrame.size(), objectNode.roi, rectOnRotatedFrame);
            DAVINCI_LOG_INFO << rect.x << " " << rect.y << " " << rect.width << " " << rect.height;

            boost::shared_ptr<UiStateObject> object = boost::shared_ptr<UiStateObject>(new UiStateGeneral(rect));
            object->SetClickable(true);
            object->SetControlState(true);
            object->SetText(ObjectLabelToString(objectNode.label));
            newState->AddUiStateObject(object);
        }

        return newState;
    }
}

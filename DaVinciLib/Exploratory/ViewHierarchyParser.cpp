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

#include "ViewHierarchyParser.hpp"
#include "boost/algorithm/string.hpp"

using namespace std;
using namespace xercesc;

namespace DaVinci
{
    ViewHierarchyParser::ViewHierarchyParser()
    {
        rootWidth = INT_MAX;
    }

    ViewHierarchyParser::~ViewHierarchyParser()
    {
        treeControls.clear();
    }

    int ViewHierarchyParser::DetetIndentLevel(string line, string base)
    {
        string trimLeft = boost::trim_left_copy(line);
        int indentLength = (int)line.length() - (int)trimLeft.length();
        int baseIndentLength = (int)base.length();
        return (indentLength - baseIndentLength) / indentUnit;
    }

    void ViewHierarchyParser::AddTreeNode(boost::shared_ptr<ViewHierarchyTree> parent, boost::shared_ptr<ViewHierarchyTree> child)
    {
        // Only for host record and replay
        child->nodeIndex = int(parent->children.size());

        parent->children.push_back(child);
        child->parent = parent;
    }

    boost::shared_ptr<ViewHierarchyTree> ViewHierarchyParser::FindParentByChildIndent(boost::shared_ptr<ViewHierarchyTree> node, int indentLevel)
    {
        if(node == nullptr)
        {
            return nullptr;
        }
        else
        {
            if(indentLevel == 0)
            {
                return node;
            }
            else if(indentLevel == 1)
            {
                return node->parent;
            }
            else
            {
                return FindParentByChildIndent(node->parent, indentLevel - 1);
            }
        }
    }

    void ViewHierarchyParser::TraverseTree(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes)
    {
        if(root == nullptr)
            return;

        DAVINCI_LOG_DEBUG << "### Id: " << root->id << " ## Rect: (" << boost::lexical_cast<string>(root->rect.x) << ", " 
            << boost::lexical_cast<string>(root->rect.y) << ", " << boost::lexical_cast<string>(root->rect.x + root->rect.width) << ", " 
            << boost::lexical_cast<string>(root->rect.y + root->rect.height) << ") ## Children: " << boost::lexical_cast<string>(root->children.size()) << endl;

        nodes.push_back(root);
        vector<boost::shared_ptr<ViewHierarchyTree>> children = root->children;

        for(auto child: children)
        {
            TraverseTree(child, nodes);
        }
    }

    void ViewHierarchyParser::TraverseTreeNodes(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes)
    {
        TraverseTree(root, nodes);
        nodes.erase(nodes.begin());
    }

    void ViewHierarchyParser::TraverseControls()
    {
        for(map<string, vector<Rect>>::const_iterator iterator = treeControls.begin(); iterator != treeControls.end(); ++iterator) 
        {
            string id = iterator->first;
            DAVINCI_LOG_INFO << "### Id: " << id << " (" << boost::lexical_cast<string>(iterator->second.size()) << ")";

            for(auto rect : iterator->second)
            {
                DAVINCI_LOG_INFO << "## Rect: (" << boost::lexical_cast<string>(rect.x) << ", " << boost::lexical_cast<string>(rect.y) << ", "
                    << boost::lexical_cast<string>(rect.x + rect.width) << ", " << boost::lexical_cast<string>(rect.y + rect.height) << ")" << endl;
            }
        }
    }

    boost::shared_ptr<ViewHierarchyTree> ViewHierarchyParser::ParseNode(string line)
    {
        string timeLine = trim_copy(line);

        boost::shared_ptr<ViewHierarchyTree> node = boost::shared_ptr<ViewHierarchyTree>(new ViewHierarchyTree());
        regex rectPattern("(-?\\d{1,4}),(\\d{1,4})-(\\d{1,4}),(\\d{1,4})");

        smatch match;
        if(regex_search(timeLine, match, rectPattern))
        {
            assert(match.size() == 5);
            // match[0] is for the complete matched string
            TryParse(match[1], node->rect.x);
            TryParse(match[2], node->rect.y);
            TryParse(match[3], node->rect.width);
            TryParse(match[4], node->rect.height);
        }

        vector<string> items;
        boost::algorithm::split(items, timeLine, boost::algorithm::is_any_of(" "));
        if(items.size() == idItemSize)
        {
            string lastItem = items[idItemSize - 1];
            node->id = lastItem.substr(0, lastItem.length() - 1);
        }

        if(rootWidth > 0 && node->rect.x >= rootWidth) 
        { 
            node->rect.width = node->rect.width - node->rect.x;  // node->rect.width records right-down x
            node->rect.x = node->rect.x % rootWidth; 
            if(node->rect.width > rootWidth)
                node->rect.width = node->rect.width % rootWidth;
        } 

        return node;
    }

    void ViewHierarchyParser::UpdateChildRect(boost::shared_ptr<ViewHierarchyTree> parent, boost::shared_ptr<ViewHierarchyTree> child)
    {
        child->rect.x = parent->rect.x + child->rect.x;
        child->rect.y = parent->rect.y + child->rect.y;
        child->rect.width = parent->rect.x + child->rect.width - child->rect.x;
        child->rect.height = parent->rect.y + child->rect.height - child->rect.y;
    }

    void ViewHierarchyParser::UpdateControls(boost::shared_ptr<ViewHierarchyTree> node)
    {
        if(node->id == "" || node->rect.width == 0 || node->rect.height == 0 || (rootWidth > 0 && node->rect.width > rootWidth))
            return;

        if(treeControls.find(node->id) != treeControls.end())
        {
            vector<Rect> existingRects = treeControls[node->id];
            existingRects.push_back(node->rect);
            treeControls[node->id] = existingRects;
        }
        else
        {
            vector<Rect> rects;
            rects.push_back(node->rect);
            treeControls[node->id] = rects;
        }
    }

    boost::shared_ptr<ViewHierarchyTree> ViewHierarchyParser::ParseTree(vector<string> lines)
    {
        string startMatchStr = "View Hierarchy:", endMatchStr = "Looper (main";
        bool isViewHierarchyStarted = false;
        bool isRootNode = false;
        string baseIndent = "";
        boost::shared_ptr<ViewHierarchyTree> root = boost::shared_ptr<ViewHierarchyTree>(new ViewHierarchyTree());
        boost::shared_ptr<ViewHierarchyTree> previousNode = boost::shared_ptr<ViewHierarchyTree>(new ViewHierarchyTree());
        int rootIndentLevel = 0, currentIndentLevel = 0, previousIndentLevel = 0;

        for(auto line : lines)
        {
            string trimLine = boost::trim_copy(line);
            if(boost::starts_with(trimLine, startMatchStr))
            {
                isViewHierarchyStarted = true;
                int matchIndex = (int)line.length() - (int)trimLine.length();
                baseIndent = line.substr(0, matchIndex);
                continue;
            }

            if(trimLine.length() == 0 || boost::starts_with(trimLine, endMatchStr))
            {
                break;
            }

            if(isViewHierarchyStarted)
            {
                if(!isRootNode)
                {
                    isRootNode = true;
                    previousIndentLevel = DetetIndentLevel(line, baseIndent);
                    rootIndentLevel = previousIndentLevel;
                    previousNode = ParseNode(line);

                    // Only for host record and replay
                    previousNode->nodeIndex = 0;
#ifdef _DEBUG
                    previousNode->line = line;
#endif
                    rootWidth = previousNode->rect.width;
                    root->children.push_back(previousNode);
                    UpdateControls(previousNode);
                }
                else
                {
                    currentIndentLevel = DetetIndentLevel(line, baseIndent);
                    boost::shared_ptr<ViewHierarchyTree> currentNode = ParseNode(line);

                    // Only for host record and replay
#ifdef _DEBUG
                    currentNode->line = line;
#endif

                    // ViewHierarchy has different ending keyword with some indents (e.g., Looper, ActiveLoader)
                    if(rootIndentLevel >= currentIndentLevel)
                        break;

                    if (currentIndentLevel == previousIndentLevel + 1) // positive indent
                    {
                        UpdateChildRect(previousNode, currentNode);
                        UpdateControls(currentNode);
                        AddTreeNode(previousNode, currentNode);
                        previousNode = currentNode;
                        previousIndentLevel = currentIndentLevel;
                    }
                    else if (currentIndentLevel == previousIndentLevel) // same indent
                    {
                        UpdateChildRect(previousNode->parent, currentNode);
                        UpdateControls(currentNode);
                        AddTreeNode(previousNode->parent, currentNode);
                        previousNode = currentNode;
                    }
                    else // negative indent
                    {
                        int negativeIndentLevel = previousIndentLevel - currentIndentLevel;
                        boost::shared_ptr<ViewHierarchyTree> parentNode = FindParentByChildIndent(previousNode, negativeIndentLevel + 1);
                        UpdateChildRect(parentNode, currentNode);
                        UpdateControls(currentNode);
                        AddTreeNode(parentNode, currentNode);
                        previousNode = currentNode;
                        previousIndentLevel = currentIndentLevel;
                    }
                }
            }
        }

        return root;
    }

    bool ViewHierarchyParser::IsValidIDRect(Rect r)
    {
        int maxArea = (rootWidth / splitNumber) * (rootWidth / splitNumber);
        int area = r.area();
        if(r.height > minHeight 
            && r.width > minWidth 
            && area < maxArea)
            return true;

        return false;
    }

    int ViewHierarchyParser::FindCloseGroupRects(map<int, vector<Rect>> groups, int k, int interval)
    {
        for(map<int, vector<Rect>>::const_iterator it = groups.begin(); it != groups.end(); ++it) 
        {
            int key = it->first;
            if(abs(key - k) <= interval)
            {
                return key;
            }
        }

        return -1;
    }

    map<int, vector<Rect>> ViewHierarchyParser::GroupIdRects(vector<Rect> rects, int interval)
    {
        map<int, vector<Rect>> xGroups;
        for(auto rect: rects)
        {
            int closeKey = FindCloseGroupRects(xGroups, rect.x, interval);
            if(closeKey != -1)
            {
                vector<Rect> xrects = xGroups[closeKey];
                xrects.push_back(rect);
                xGroups[closeKey] = xrects;
            }
            else
            {
                vector<Rect> xrects;
                xrects.push_back(rect);
                xGroups[rect.x] = xrects;
            }
        }

        map<int, vector<Rect>> yGroups;
        for(auto rect: rects)
        {
            int closeKey = FindCloseGroupRects(yGroups, rect.y, interval);

            if(closeKey != -1)
            {
                vector<Rect> yrects = yGroups[closeKey];
                yrects.push_back(rect);
                yGroups[closeKey] = yrects;
            }
            else
            {
                vector<Rect> yrects;
                yrects.push_back(rect);
                yGroups[rect.y] = yrects;
            }
        }

        if(xGroups.size() <= yGroups.size())
            return xGroups;
        else
            return yGroups;
    }

    void ViewHierarchyParser::FindValidNodesWithTreeByPoint(boost::shared_ptr<ViewHierarchyTree> root, Point p, Rect fw, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes)
    {
        if(root == nullptr)
            return;

        Rect r = root->rect;
        if(!boost::equals(root->id, "") && r.contains(p) && ContainRect(fw, r, edgeDistance) && IsValidIDRect(r))
        {
            nodes.push_back(root);
        }

        vector<boost::shared_ptr<ViewHierarchyTree>> children = root->children;
        for(auto child: children)
        {
            FindValidNodesWithTreeByPoint(child, p, fw, nodes);
        }
    }

    void ViewHierarchyParser::TraverseTreeByNode(boost::shared_ptr<ViewHierarchyTree> root, boost::shared_ptr<ViewHierarchyTree> node, vector<int>& treeInfos)
    {
        if(root == nullptr || node == nullptr)
            return;

        if(root == node)
            return;
        else
        {
            treeInfos.insert(treeInfos.begin(), node->nodeIndex);
        }

        boost::shared_ptr<ViewHierarchyTree> parent = node->parent;
        TraverseTreeByNode(root, parent, treeInfos);
    }

    boost::shared_ptr<ViewHierarchyTree> ViewHierarchyParser::FindMatchedIdByTreeInfo(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>> nodes, Point clickPointOnDevice, vector<int>& treeInfos)
    {
        double minDeltaRatio = 1.0;
        boost::shared_ptr<ViewHierarchyTree> idNode = nullptr;
        double xRatio;
        double yRatio;
        for(auto node : nodes)
        {
            xRatio = (clickPointOnDevice.x - node->rect.x) / double(node->rect.width);
            yRatio = (clickPointOnDevice.y - node->rect.y) / double(node->rect.height);
            double deltaRatio = abs(xRatio - 0.5) + abs(yRatio - 0.5);
            if(deltaRatio < minDeltaRatio)
            {
                minDeltaRatio = deltaRatio;
                idNode = node;
            }
        }

        if(idNode != nullptr)
        {
            TraverseTreeByNode(root, idNode, treeInfos);
        }

        return idNode;
    }

    unordered_map<string, vector<Rect>> ViewHierarchyParser::FindAllValidIds(Rect fw)
    {
        unordered_map<string, vector<Rect>> matchedIdMap;

        for(map<string, vector<Rect>>::const_iterator iterator = treeControls.begin(); iterator != treeControls.end(); ++iterator) 
        {
            for(auto r : iterator->second)
            {
                if(ContainRect(fw, r, edgeDistance))
                {
                    string id = iterator->first;
                    if(matchedIdMap.find(id) == matchedIdMap.end())
                    {
                        vector<Rect> rects;
                        rects.push_back(r);
                        matchedIdMap[id] = rects;
                    }
                    else
                    {
                        vector<Rect> rects = matchedIdMap[id];
                        rects.push_back(r);
                        matchedIdMap[id] = rects;
                    }
                }
            }
        }

        return matchedIdMap;
    }

    string ViewHierarchyParser::FindMatchedIdByPoint(Point p, Rect fw, Rect& rect, int& groupIndex, int& itemIndex)
    {
        unordered_map<string, vector<Rect>> matchedIdMap;
        groupIndex = 0;
        itemIndex = 0;

        // Get all matched IDs by id string only
        for(map<string, vector<Rect>>::const_iterator iterator = treeControls.begin(); iterator != treeControls.end(); ++iterator) 
        {
            for(auto r : iterator->second)
            {
                if(r.contains(p) && ContainRect(fw, r, edgeDistance))
                {
                    string id = iterator->first;
                    if(matchedIdMap.find(id) == matchedIdMap.end())
                    {
                        vector<Rect> rects;
                        rects.push_back(r);
                        matchedIdMap[id] = rects;
                    }
                    else
                    {
                        vector<Rect> rects = matchedIdMap[id];
                        rects.push_back(r);
                        matchedIdMap[id] = rects;
                    }
                }
            }
        }

        unordered_map<string, int> idSizeMap;
        unordered_map<string, Rect> idRectMap;
        double minDeltaRatio = 1.0;
        double xRatio, yRatio, deltaRatio;

        // Get valid matched IDs and minDeltaRatio
        for(auto pair : matchedIdMap)
        {
            int idSize = 0;
            Rect idRect = EmptyRect;
            minDeltaRatio = 1.0;
            for(auto r: pair.second)
            {
                if(IsValidIDRect(r))
                {
                    xRatio = (p.x - r.x) / double(r.width);
                    yRatio = (p.y - r.y) / double(r.height);
                    deltaRatio = abs(xRatio - 0.5) + abs(yRatio - 0.5);
                    if(deltaRatio < minDeltaRatio)
                    {
                        minDeltaRatio = deltaRatio;
                        idRect = r;
                    }
                    idSize++;
                }
            }

            if(idRect != EmptyRect)
            {
                idSizeMap[pair.first] = idSize;
                idRectMap[pair.first] = idRect;
            }
        }

        minDeltaRatio = 1.0;
        vector<string> idsWithMinRect;

        // Sort matched IDs by minDeltaRatio
        for(auto pair : idRectMap)
        {
            Rect rect = pair.second;
            xRatio = (p.x - rect.x) / double(rect.width);
            yRatio = (p.y - rect.y) / double(rect.height);
            deltaRatio = abs(xRatio - 0.5) + abs(yRatio - 0.5);
            if(deltaRatio <= minDeltaRatio)
            {
                if(deltaRatio < minDeltaRatio)
                {
                    idsWithMinRect.clear();
                    idsWithMinRect.push_back(pair.first);
                }
                else
                {
                    idsWithMinRect.push_back(pair.first);
                }
                minDeltaRatio = deltaRatio;
            }
        }

        // Select ID with min group size
        int minValueSize = INT_MAX;
        string matchedId = "";
        for(auto id : idsWithMinRect)
        {
            if(idSizeMap[id] < minValueSize)
            {
                matchedId = id;
                rect = idRectMap[id];
            }
        }

        // Select ID group index and item index
        if(matchedId == "")
        {
            rect = EmptyRect;
            return matchedId;
        }
        else
        {
            vector<Rect> rects = treeControls[matchedId];
            map<int, vector<Rect>> groups = GroupIdRects(rects);

            int index = 0;
            bool foundItem = false;
            for(map<int, vector<Rect>>::const_iterator it = groups.begin(); it != groups.end(); ++it) 
            {
                vector<Rect> rects = it->second;
                for(int j = 0; j < (int)(rects.size()); j++)
                {
                    if(rects[j] == rect)
                    {
                        itemIndex = j;
                        foundItem = true;
                        break;
                    }
                }

                if(foundItem)
                {
                    groupIndex = index;
                    break;
                }
                index++;
            }

            DAVINCI_LOG_INFO << "### Matched Id: " << matchedId << " with min id ratio " + boost::lexical_cast<string>(minDeltaRatio) << endl;
            return matchedId;
        }
    }

    void ViewHierarchyParser::CleanTreeControls()
    {
        treeControls.clear();
    }

    string ViewHierarchyParser::ParseIdInfo(string idWithIndex, int& groupIndex, int& itemIndex)
    {
        groupIndex = 0, itemIndex = 0;
        string id = idWithIndex;
        if(boost::contains(idWithIndex, "("))
        {
            size_t startIndex = idWithIndex.find_last_of("(");
            size_t endIndex = idWithIndex.find_last_of(")");
            string indexStr = idWithIndex.substr(startIndex + 1, endIndex - (startIndex + 1));

            vector<string> items;
            boost::algorithm::split(items, indexStr, boost::algorithm::is_any_of("-"));
            if(items.size() == 2)
            {
                TryParse(items[0], groupIndex);
                TryParse(items[1], itemIndex);
            }
            else
            {
                TryParse(items[0], itemIndex);
            }
            id = idWithIndex.substr(0, startIndex);
        }

        return id;
    }

    boost::shared_ptr<ViewHierarchyTree> ViewHierarchyParser::FindMatchedRectByIdAndTree(vector<int> recordTreeInfos, boost::shared_ptr<ViewHierarchyTree> replayRoot)
    {
        boost::shared_ptr<ViewHierarchyTree> currentNode = replayRoot;
        int size = int(recordTreeInfos.size());

        while(size > 0)
        {
            int treeInfo = recordTreeInfos[0];
            recordTreeInfos.erase(recordTreeInfos.begin());
            if(currentNode == nullptr)
            {
                DAVINCI_LOG_WARNING << "Replay view hierarchy parse error.";
                break;
            }

            int childSize = int(currentNode->children.size());
            if(treeInfo > childSize - 1)
            {
                currentNode = nullptr;
                DAVINCI_LOG_WARNING << "Record has different view hierarchy with replay.";
                break;
            }

            currentNode = currentNode->children[treeInfo];
            size = int(recordTreeInfos.size());
        }

        return currentNode;
    }

    Rect ViewHierarchyParser::FindMatchedRectById(map<string, vector<Rect>>& controls, string idWithIndex, Rect fw)
    {
        int groupIndex, itemIndex;
        string id = ParseIdInfo(idWithIndex, groupIndex, itemIndex);

        Rect matchedRect = EmptyRect;
        for(map<string, vector<Rect>>::const_iterator iterator = controls.begin(); iterator != controls.end(); ++iterator) 
        {
            if(iterator->first == id)
            {
                map<int, vector<Rect>> groups = GroupIdRects(iterator->second);

                int index = 0;
                vector<Rect> rects;
                for(map<int, vector<Rect>>::const_iterator it = groups.begin(); it != groups.end(); ++it) 
                {
                    if(index == groupIndex)
                    {
                        rects = it->second;
                        break;
                    }
                    index++;
                }

                if(itemIndex < (int)(rects.size()))
                    matchedRect = rects[itemIndex];

                break;
            }
        }

        if(!ContainRect(fw, matchedRect, edgeDistance) || !IsValidIDRect(matchedRect))
        {
            matchedRect = EmptyRect;
        }

        if(matchedRect != EmptyRect)
            DAVINCI_LOG_INFO << "### Matched Rect for ID[" << idWithIndex << "]: (" << boost::lexical_cast<string>(matchedRect.x) << ", " << boost::lexical_cast<string>(matchedRect.y) << ", "
            << boost::lexical_cast<string>(matchedRect.x + matchedRect.width) << ", " << boost::lexical_cast<string>(matchedRect.y + matchedRect.height) << ")" << endl;

        return matchedRect;
    }

    bool ViewHierarchyParser::IsRootChanged(vector<boost::shared_ptr<ViewHierarchyTree>> nodes1,vector<boost::shared_ptr<ViewHierarchyTree>> nodes2)
    {
        bool isRootChanged = false;
        if(nodes1.size() != nodes2.size())
        {
            isRootChanged = true;
        }
        else
        {
            for(size_t i=0; i<nodes1.size(); i++)
            {
                if(!boost::equals(nodes1[i]->id, nodes2[i]->id))
                {
                    isRootChanged = true;
                    break;
                }
            }
        }
        return isRootChanged;
    }
}

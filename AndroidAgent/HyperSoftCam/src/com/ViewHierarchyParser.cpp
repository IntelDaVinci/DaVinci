#include "com/ViewHierarchyParser.h"

namespace android {

    ViewHierarchyParser::ViewHierarchyParser(): rootWidth(0)
    {
        AUTO_LOG();
    }

    ViewHierarchyParser::~ViewHierarchyParser()
    {
        AUTO_LOG();
    }

    int ViewHierarchyParser::DetectIndentLevel(String8 line, String8 base)
    {
        AUTO_LOG();
        String8 trimLeft = StringUtil::trimLeft(line);
        int indentLength = line.length() - trimLeft.length();
        int baseIndentLength = base.length();
        return (indentLength - baseIndentLength) / indentUnit;
    }

    void ViewHierarchyParser::AddTreeNode(struct ViewHierarchyTree* parent, struct ViewHierarchyTree* child)
    {
        AUTO_LOG();
        parent->children.push_back(child);
        child->parent = parent;
    }

    struct ViewHierarchyTree* ViewHierarchyParser::FindParentByChildIndent(struct ViewHierarchyTree* node, int indentLevel)
    {
        AUTO_LOG();
        if(node == NULL)
        {
            return NULL;
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

    void ViewHierarchyParser::TraverseTree(struct ViewHierarchyTree* root)
    {
        AUTO_LOG();
        if(root == NULL)
        {
            return;
        }
        DWLOGD("### Id: %s ## Rect: (%d, %d, %d, %d) ## Children: %u\n\n",
              root->id.string(),
              root->rect.x,
              root->rect.y,
              root->rect.x + root->rect.width,
              root->rect.y + root->rect.height,
              root->children.size());

        Vector<struct ViewHierarchyTree*> children = root->children;

        for(unsigned int i = 0; i < children.size(); i++)
        {
            struct ViewHierarchyTree* child = children[i];
            TraverseTree(child);
        }
    }

    void ViewHierarchyParser::TraverseControls()
    {
        AUTO_LOG();
        for(unsigned int i = 0; i < controls.size(); i++) 
        {    
            String8 id = controls.keyAt(i);
            struct ViewHierarchyVector* vector = controls.valueAt(i);
            DWLOGD("### Id: %s (%d)\n", id.string(), vector->nodes.size());

            for(unsigned int j = 0; j < vector->nodes.size(); j++)
            {
                struct ViewHierarchyTree* node = vector->nodes[j];
                DWLOGD("## Rect: (%d, %d, %d, %d)\n\n",
                      node->rect.x,
                      node->rect.y,
                      node->rect.x + node->rect.width,
                      node->rect.y + node->rect.height);
            }
        }
    }

    struct ViewHierarchyTree* ViewHierarchyParser::ParseNode(String8 line)
    {
        AUTO_LOG();

        struct ViewHierarchyTree* node = new struct ViewHierarchyTree();
        Vector<String8> items = StringUtil::split(line, ' ');
        size_t size = items.size();
        if(size == basicItemSize)
        {
            String8 rectStr = items[size-1];
            Vector<String8> rectItems = StringUtil::split(rectStr, ',');
            size_t rectItemSize = rectItems.size();
            assert(rectItemSize == 3);
            String8 xStr = rectItems[0];
            String8 middleStr = rectItems[1];
            String8 heightStr = rectItems[2];
            heightStr = StringUtil::substr(heightStr, 0, heightStr.length() - 1);
            String8 yStr, widthStr;
            Vector<String8> middleRectItems = StringUtil::split(middleStr, '-');
            size_t middleRectItemSize = middleRectItems.size();
            assert(middleRectItemSize == 2 || middleRectItemSize == 3);
            if(middleRectItemSize == 2)
            {
                yStr = middleRectItems[0];
                widthStr = middleRectItems[1];
            }
            else
            {
                yStr = middleRectItems[1];
                widthStr = middleRectItems[2];
            }
            node->rect.x = StringUtil::strToInt(xStr);
            node->rect.y = StringUtil::strToInt(yStr);
            node->rect.width = StringUtil::strToInt(widthStr);
            node->rect.height = StringUtil::strToInt(heightStr);
        }
        else
        {
            if(size == itemSizeWithId)
            {
                String8 idStr = items[size-1];
                idStr = StringUtil::substr(idStr, 0, idStr.length() - 1);
                node->id = idStr;
            }

            // null@null view_hierarchy_18.log
            if(items.size() < rectIndex + 1)
                return node;
            
            String8 rectStr = items[rectIndex];
            Vector<String8> rectItems = StringUtil::split(rectStr, ',');
            size_t rectItemSize = rectItems.size();
            assert(rectItemSize == 3);
            String8 xStr = rectItems[0];
            String8 middleStr = rectItems[1];
            String8 heightStr = rectItems[2];
            String8 yStr, widthStr;
            Vector<String8> middleRectItems = StringUtil::split(middleStr, '-');
            size_t middleRectItemSize = middleRectItems.size();
            assert(middleRectItemSize == 2 || middleRectItemSize == 3);
            if(middleRectItemSize == 2)
            {
                yStr = middleRectItems[0];
                widthStr = middleRectItems[1];
            }
            else
            {
                yStr = middleRectItems[1];
                widthStr = middleRectItems[2];
            }
            node->rect.x = StringUtil::strToInt(xStr);
            node->rect.y = StringUtil::strToInt(yStr);
            node->rect.width = StringUtil::strToInt(widthStr);
            node->rect.height = StringUtil::strToInt(heightStr);
        }

        if(rootWidth > 0 && node->rect.x >= rootWidth)
        {
            node->rect.width = node->rect.width - node->rect.x;
            node->rect.x = node->rect.x % rootWidth;
            if(node->rect.width > rootWidth)
            {
                node->rect.width = node->rect.width % rootWidth;
            }
        }

        return node;
    }

    void ViewHierarchyParser::UpdateChildRect(struct ViewHierarchyTree* parent, struct ViewHierarchyTree* child)
    {
        AUTO_LOG();

        child->rect.x = parent->rect.x + child->rect.x;
        child->rect.y = parent->rect.y + child->rect.y;
        child->rect.width = parent->rect.x + child->rect.width - child->rect.x;
        child->rect.height = parent->rect.y + child->rect.height - child->rect.y;
    }

    void ViewHierarchyParser::UpdateControls(struct ViewHierarchyTree* node)
    {
        AUTO_LOG();
        if(node->id == "" 
		|| node->rect.width == 0 
		|| node->rect.height == 0 
		|| (rootWidth > 0 && node->rect.width > rootWidth))
        {
            return;
        }

        if(controls.indexOfKey(node->id) < 0)
        {
            struct ViewHierarchyVector* vector = new struct ViewHierarchyVector();
            vector->nodes.push_back(node);
            controls.add(node->id, vector);
        }
        else
        {
            struct ViewHierarchyVector* vector = controls.valueFor(node->id);
            vector->nodes.push_back(node);
            controls.replaceValueFor(node->id, vector);
        }
    }

    KeyedVector<String8, struct ViewHierarchyVector*> ViewHierarchyParser::GetControls()
    {
        AUTO_LOG();
        return controls;
    }

    struct ViewHierarchyTree* ViewHierarchyParser::ParseTree(char* region, String8 dumpFile)
    {
        AUTO_LOG();
	int tree_fd = open(dumpFile.string(),
	               O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

        String8 startMatchStr("View Hierarchy:"), endMatchStr("Looper (main");
        bool isViewHierarchyStarted = false;
        bool isRootNode = false;
        String8 baseIndent;
        struct ViewHierarchyTree* root = new struct ViewHierarchyTree();
        struct ViewHierarchyTree* previousNode = new struct ViewHierarchyTree();
        int rootIndentLevel = 0, currentIndentLevel = 0, previousIndentLevel = 0;

        const char *d = "\n";
        char *p = strtok(region, d);
	char buffer[4096] = {0};
	int len = 0;

        while(p)
        {
            String8 line(p);
    	    len = sprintf(buffer, "%s\n", p);
            write(tree_fd, buffer, len);
            memset(buffer, 0, sizeof(buffer));

            String8 trimLine = StringUtil::trimLeft(line);
            
            if(StringUtil::startsWith(trimLine, startMatchStr.string()))
            {
                isViewHierarchyStarted = true;
                int matchIndex = line.length() - trimLine.length();
                baseIndent = StringUtil::substr(line, 0, matchIndex);
                p = strtok(NULL, d);
                continue;
            }

            if(trimLine.length() == 0 || StringUtil::startsWith(trimLine, endMatchStr.string()))
            {
                break;
            }

            if(isViewHierarchyStarted)
            {
                if(!isRootNode)
                {
                    isRootNode = true;
                    previousIndentLevel = DetectIndentLevel(line, baseIndent);
                    rootIndentLevel = previousIndentLevel;
                    previousNode = ParseNode(trimLine);
                    rootWidth = previousNode->rect.width;
                    root->children.push_back(previousNode);
                    UpdateControls(previousNode);
                }
                else
                {
                    currentIndentLevel = DetectIndentLevel(line, baseIndent);
                    struct ViewHierarchyTree* currentNode = ParseNode(trimLine);

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
                        struct ViewHierarchyTree* parentNode = FindParentByChildIndent(previousNode, negativeIndentLevel + 1);
                        UpdateChildRect(parentNode, currentNode);
                        UpdateControls(currentNode);
                        AddTreeNode(parentNode, currentNode);
                        previousNode = currentNode;
                        previousIndentLevel = currentIndentLevel;
                    }
                }
            }
            p = strtok(NULL, d);
        }

        return root;
    }

    bool ViewHierarchyParser::IsValidIDRect(UiRect r)
    {
        int maxArea = (rootWidth / splitNumber) * (rootWidth / splitNumber);
        int area = r.area();
        if(r.height > minHeight
            && r.width > minWidth
            && area < maxArea)
               return true;

        return false;           
    }

    int ViewHierarchyParser::FindCloseGroupRects(KeyedVector<int, struct ViewHierarchyVector*> groups, int k, int interval)
    {
        for(unsigned int j = 0; j < groups.size(); j++)
        {
            int key = groups.keyAt(j);
            if(abs(key - k) <= interval)
            {
                return key;
            }
        }
        return -1;
    }

    KeyedVector<int, struct ViewHierarchyVector*> ViewHierarchyParser::GroupIdRects(struct ViewHierarchyVector* vector, int interval)
    {
        KeyedVector<int, struct ViewHierarchyVector*> xGroups;
        for(unsigned int j = 0; j < vector->nodes.size(); j++)
        {
            struct ViewHierarchyTree* node = vector->nodes[j];
            int closeKey = FindCloseGroupRects(xGroups, node->rect.x, interval);
            if(closeKey != -1)
            {
                struct ViewHierarchyVector* xVector = xGroups.valueFor(closeKey);
                xVector->nodes.push_back(node);
                xGroups.replaceValueFor(closeKey, xVector);
            }
            else
            {
                struct ViewHierarchyVector* xVector = new struct ViewHierarchyVector();
                xVector->nodes.push_back(node);
                xGroups.add(node->rect.x, xVector);
            }
        }

        KeyedVector<int, struct ViewHierarchyVector*> yGroups;
        for(unsigned int j = 0; j < vector->nodes.size(); j++)
        {
            struct ViewHierarchyTree* node = vector->nodes[j];
            int closeKey = FindCloseGroupRects(yGroups, node->rect.y, interval);
            if(closeKey != -1)
            {
                struct ViewHierarchyVector* yVector = yGroups.valueFor(closeKey);
                yVector->nodes.push_back(node);
                yGroups.replaceValueFor(closeKey, yVector);
            }
            else
            {
                struct ViewHierarchyVector* yVector = new struct ViewHierarchyVector();
                yVector->nodes.push_back(node);
                yGroups.add(node->rect.y, yVector);
            }
        }

        if(xGroups.size() <= yGroups.size())
        {
            return xGroups;
        }
        else
        {
            return yGroups;
        }
    }


    String8 ViewHierarchyParser::FindMatchedIdByPoint(int x, int y, UiRect fw, UiRect& rect, int& groupIndex, int& itemIndex)
    {
        AUTO_LOG();

        KeyedVector<String8, struct ViewHierarchyVector*> matchedIdMap;
        groupIndex = 0;
        itemIndex = 0;

        // Get all matched IDs by id string only
        for(unsigned int i = 0; i < controls.size(); i++) 
        {    
            struct ViewHierarchyVector* vector = controls.valueAt(i);
            for(unsigned int j = 0; j < vector->nodes.size(); j++)
            {
                struct ViewHierarchyTree* node = vector->nodes[j];
                if(node->rect.containPoint(x, y) && fw.containRect(node->rect))
                {
                    DWLOGD("### ID: %s Rect (%d, %d, %d, %d) contains point (%d, %d)", 
			node->id.string(),
			node->rect.x, 
			node->rect.y,
			node->rect.x + node->rect.width,
			node->rect.y + node->rect.height,
			x,
			y);

                    String8 id = controls.keyAt(i);
                    if(matchedIdMap.indexOfKey(id) < 0)
                    {
                        struct ViewHierarchyVector* idMapVector = new struct ViewHierarchyVector();
                        idMapVector->nodes.push_back(node);
                        matchedIdMap.add(id, idMapVector);
                    }
                    else
                    {
                        struct ViewHierarchyVector* idMapVector = matchedIdMap.valueFor(id);
                        idMapVector->nodes.push_back(node);
                        matchedIdMap.replaceValueFor(id, idMapVector);
                    }
                }
            }
        }

        int minValueSize = 1024 * 1024;
        KeyedVector<String8, int> idSizeMap;
        KeyedVector<String8, struct ViewHierarchyTree*> idRectMap;
        double minDeltaRatio = 1.0;
        double xRatio, yRatio, deltaRatio;

        // Get valid matched IDs and minDeltaRatio
        for(unsigned int i = 0; i < matchedIdMap.size(); i++) 
        {    
            int idSize = 0;
            UiRect idRect;
            struct ViewHierarchyVector* vector = matchedIdMap.valueAt(i);
            String8 id = matchedIdMap.keyAt(i);
            for(unsigned int j = 0; j < vector->nodes.size(); j++)
            {
                struct ViewHierarchyTree* node = vector->nodes[j];
                UiRect r = node->rect;
                if(IsValidIDRect(r))
                {
                    xRatio = (x - r.x) / (double)(r.width);
                    yRatio = (y - r.y) / (double)(r.height);
                    deltaRatio = abs(xRatio - 0.5) + abs(yRatio - 0.5);
                    if(deltaRatio < minDeltaRatio)
                    {
                        minDeltaRatio = deltaRatio;
                        idRect = r;
                    }
                    idSize++;
                }
            }

            if(!idRect.equals(emptyRect))
            {
                idSizeMap.add(id, idSize);
                struct ViewHierarchyTree* idRectNode = new struct ViewHierarchyTree();
                idRectNode->rect = idRect;
                idRectMap.add(id, idRectNode);
            }
        }
       
        minDeltaRatio = 1.0;
        Vector<String8> idsWithMinRect;

        // Sort matched IDs by minDeltaRatio
        for(unsigned int i = 0; i < idRectMap.size(); i++) 
        {    
            struct ViewHierarchyTree* node = idRectMap.valueAt(i);
            UiRect rect = node->rect;
            xRatio = (x - rect.x) / double(rect.width);
            yRatio = (y - rect.y) / double(rect.height);
            deltaRatio = abs(xRatio - 0.5) + abs(yRatio = 0.5);

            if(deltaRatio <= minDeltaRatio)
            {
                if(deltaRatio < minDeltaRatio)
                {
                    idsWithMinRect.clear();
                    idsWithMinRect.add(idRectMap.keyAt(i));
                }
                else
                {
                    idsWithMinRect.add(idRectMap.keyAt(i));
                }
                minDeltaRatio = deltaRatio;
            }
        }

        minValueSize = 1024 * 1024;
        String8 matchedId;
        for(unsigned int i = 0; i < idsWithMinRect.size(); i++) 
        {
            String8 id = idsWithMinRect[i];
            if(idSizeMap.valueFor(id) < minValueSize)
            {
                matchedId = id;
                struct ViewHierarchyTree* node = idRectMap.valueFor(id);
                rect = node->rect;
            }
        }

        if(matchedId == "")
	{
	    matchedId = "";
	    rect = UiRect();
	    return matchedId;
	}
	else
	{
            struct ViewHierarchyVector* vector = controls.valueFor(matchedId);
            KeyedVector<int, struct ViewHierarchyVector*> groups = GroupIdRects(vector);

            int index = 0;
            bool foundItem = false;
            
            for(unsigned int i = 0; i < groups.size(); i++) 
            {    
                struct ViewHierarchyVector* vector = groups.valueAt(i);
                for(unsigned int j = 0; j < vector->nodes.size(); j++)
                {
                    struct ViewHierarchyTree* node = vector->nodes[j];
                    if(node->rect.equals(rect))
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
	    DWLOGD("### Matched Id: %s with min ID ratio %llu\n", matchedId.string(), minDeltaRatio);
    	    return matchedId;
	}
    }
    
    UiRect ViewHierarchyParser::FindMatchedRectById(String8 idWithIndex, UiRect fw)
    {
        AUTO_LOG();
        UiRect matchedRect;
        int groupIndex = 0, itemIndex = 0;
	String8 idStr(idWithIndex), indexStr("0");
	if(StringUtil::endsWith(idStr, ")"))
	{
	    Vector<String8> items = StringUtil::split(idWithIndex, '(');
	    assert(items.size() == 2);
	    String8 rightStr = items[1];
	    indexStr = StringUtil::substr(rightStr, 0, rightStr.length() - 1);

	    Vector<String8> groupItems = StringUtil::split(indexStr, '-');
            if(groupItems.size() == 2)
            {
                groupIndex = atoi(groupItems[0]);
                itemIndex = atoi(groupItems[1]);
            }
            else
            {
                itemIndex = atoi(groupItems[0]);
            }
	    String8 leftStr = items[0];
	    idStr = StringUtil::substr(leftStr, 0, leftStr.length());
	}

        DWLOGW("### ID:%s", idStr.string());
        for(unsigned int i = 0; i < controls.size(); i++) 
        {   
            if(controls.keyAt(i) == idStr)
            {
                struct ViewHierarchyVector* vector = controls.valueFor(idStr);
                KeyedVector<int, struct ViewHierarchyVector*> groups = GroupIdRects(vector);
                
                int index = 0;
                struct ViewHierarchyVector* groupVector = new struct ViewHierarchyVector();
            
                for(unsigned int i = 0; i < groups.size(); i++) 
                {
                    if(index == groupIndex)
                    {
                        groupVector = groups.valueAt(i);
                        break;
                    }    
                    index++;
                }

                if(itemIndex < (int)(groupVector->nodes.size()))
                {
                    matchedRect = groupVector->nodes[itemIndex]->rect;
                }
            }
        }

        if(!fw.containRect(matchedRect) || !IsValidIDRect(matchedRect))
        {
            matchedRect = emptyRect;
        }

        if(!matchedRect.equals(emptyRect))
            DWLOGD("### Matched Rect: (%d, %d, %d, %d) with id %s\n",
                  matchedRect.x,
                  matchedRect.y,
                  matchedRect.x + matchedRect.width,
                  matchedRect.y + matchedRect.height,
    	          idWithIndex.string());

        return matchedRect;
    }

    void ViewHierarchyParser::ReleaseTree(struct ViewHierarchyTree* root)
    {
        AUTO_LOG();
        if(root == NULL)
        {
            return;
        }
        Vector<struct ViewHierarchyTree*> children = root->children;
        delete(root);

        for(unsigned int i = 0; i < children.size(); i++)
        {
            struct ViewHierarchyTree* child = children[i];
            ReleaseTree(child);
        }
    }

    void ViewHierarchyParser::ReleaseControls()
    {
	controls.clear();
    }

}

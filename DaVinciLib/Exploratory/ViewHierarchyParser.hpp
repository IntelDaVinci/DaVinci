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

#ifndef __VIEWHIERARCHYPARSER__
#define __VIEWHIERARCHYPARSER__

#include "DaVinciDefs.hpp"
#include "DaVinciCommon.hpp"
#include "ObjectCommon.hpp"
#include <boost/regex.hpp>

namespace DaVinci
{
    using namespace std;
    using namespace boost;

    struct ViewHierarchyTree
    {
        Rect rect;
        std::string id;
        boost::shared_ptr<ViewHierarchyTree> parent;
        vector<boost::shared_ptr<ViewHierarchyTree>> children;

        // Only for host record and replay
        int nodeIndex;
        string line;

        ViewHierarchyTree()
        {
            rect = EmptyRect;
            id = "";
            parent = nullptr;
            children = vector<boost::shared_ptr<ViewHierarchyTree>>();

            // Only for host record and replay
            nodeIndex = 0;
            line = "";
        }
    };

    /// <summary>
    /// Class ViewHierarchyParser
    /// </summary>
    class ViewHierarchyParser
    {
    public:
        /// <summary>
        /// Construct
        /// </summary>
        ViewHierarchyParser();

        ~ViewHierarchyParser();

        /// <summary>
        /// Parse view hierarchy tree
        /// </summary>
        /// <param name="lines"></param>
        /// <returns></returns>
        boost::shared_ptr<ViewHierarchyTree> ParseTree(vector<string> lines);

        /// <summary>
        /// Find parent by child and indent
        /// </summary>
        /// <param name="node"></param>
        /// <param name="indentLevel"></param>
        /// <returns></returns>
        boost::shared_ptr<ViewHierarchyTree> FindParentByChildIndent(boost::shared_ptr<ViewHierarchyTree> node, int indentLevel);

        /// <summary>
        /// Parse tree node by one line
        /// </summary>
        /// <param name="line"></param>
        /// <returns></returns>
        boost::shared_ptr<ViewHierarchyTree> ParseNode(string line);

        /// <summary>
        /// Traverse tree
        /// </summary>
        /// <param name="root"></param>
        /// <param name="nodes"></param>
        /// <returns></returns>
        void TraverseTree(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes);

        /// <summary>
        /// Traverse tree nodes
        /// </summary>
        /// <param name="root"></param>
        /// <param name="nodes"></param>
        /// <returns></returns>
        void TraverseTreeNodes(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes);

        /// <summary>
        /// Traverse controls
        /// </summary>
        /// <returns></returns>
        void TraverseControls();

        /// <summary>
        /// Find matched control by point
        /// </summary>
        /// <param name="p"></param>
        /// <param name="fw"></param>
        /// <param name="rect"></param>
        /// <param name="groupIndex"></param>
        /// <param name="itemIndex"></param>
        /// <returns></returns>
        string FindMatchedIdByPoint(Point p, Rect fw, Rect& rect, int& groupIndex, int& itemIndex);

        /// <summary>
        /// Find all valid controls
        /// </summary>
        /// <param name="fw"></param>
        /// <returns></returns>
        std::unordered_map<string, vector<Rect>> FindAllValidIds(Rect fw);

        /// <summary>
        /// Find valid nodes by point from tree
        /// </summary>
        /// <param name="root"></param>
        /// <param name="p"></param>
        /// <param name="fw"></param>
        /// <param name="nodes"></param>
        /// <returns></returns>
        void FindValidNodesWithTreeByPoint(boost::shared_ptr<ViewHierarchyTree> root, Point p, Rect fw, vector<boost::shared_ptr<ViewHierarchyTree>>& nodes);

        /// <summary>
        /// Find matched control by point with tree
        /// </summary>
        /// <param name="root"></param>
        /// <param name="nodes"></param>
        /// <param name="treeInfo"></param>
        /// <returns></returns>
        boost::shared_ptr<ViewHierarchyTree> FindMatchedIdByTreeInfo(boost::shared_ptr<ViewHierarchyTree> root, vector<boost::shared_ptr<ViewHierarchyTree>> nodes, Point p, vector<int>& treeInfo);

        /// <summary>
        /// TraverseTreeByNode
        /// </summary>
        /// <param name="root"></param>
        /// <param name="node"></param>
        /// <param name="treeInfo"></param>
        /// <returns></returns>
        void TraverseTreeByNode(boost::shared_ptr<ViewHierarchyTree> root, boost::shared_ptr<ViewHierarchyTree> node, vector<int>& treeInfo);

        /// <summary>
        /// Find matched rect by id
        /// </summary>
        /// <param name="id"></param>
        /// <param name="fw"></param>
        /// <returns></returns>
        Rect FindMatchedRectById(map<string, vector<Rect>>& controls, string id, Rect fw);

        /// <summary>
        /// Find matched rect by point and tree
        /// </summary>
        /// <param name="recordTreeInfos"></param>
        /// <param name="replayRoot"></param>
        /// <returns></returns>
        boost::shared_ptr<ViewHierarchyTree> FindMatchedRectByIdAndTree(vector<int> recordTreeInfos, boost::shared_ptr<ViewHierarchyTree> replayRoot);

        string ParseIdInfo(string idWithIndex, int& groupIndex, int& itemIndex);

        bool IsValidIDRect(Rect r);

        int FindCloseGroupRects(map<int, vector<Rect>> groups, int k, int interval);

        map<int, vector<Rect>> GroupIdRects(vector<Rect> rects, int interval = 10);

        void CleanTreeControls();

        bool IsRootChanged(vector<boost::shared_ptr<ViewHierarchyTree>> nodes1,vector<boost::shared_ptr<ViewHierarchyTree>> nodes2);

        map<string, vector<Rect>> GetTreeControls()
        {
            return treeControls;
        }

        void SetTreeControls(map<string, vector<Rect>> controls)
        {

            treeControls = controls;
        }
    private:
        int DetetIndentLevel(string line, string base);

        void AddTreeNode(boost::shared_ptr<ViewHierarchyTree> parent, boost::shared_ptr<ViewHierarchyTree> child);

        void UpdateChildRect(boost::shared_ptr<ViewHierarchyTree> parent, boost::shared_ptr<ViewHierarchyTree> child);

        void UpdateControls(boost::shared_ptr<ViewHierarchyTree> node);
    private:
        static const int indentUnit = 2;
        static const int edgeDistance = 2;
        static const int idItemSize = 6;
        static const int splitNumber = 2;
        static const int minWidth = 10;
        static const int minHeight = 10;
        int rootWidth;
        map<string, vector<Rect>> treeControls;
    };
}


#endif	//#ifndef __VIEWHIERARCHYPARSER__

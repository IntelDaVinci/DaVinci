#include <utils/String8.h>
#include <utils/KeyedVector.h>
#include "utils/Logger.h"
#include "utils/StringUtil.h"
#include "com/UiRect.h"
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utils/RefBase.h>

namespace android {

struct ViewHierarchyTree
{
    String8 id;
    UiRect rect;
    struct ViewHierarchyTree* parent;
    Vector<struct ViewHierarchyTree*> children;

    ViewHierarchyTree()
    {
        id = "";
        rect = UiRect();
        parent = NULL;
        children = Vector<struct ViewHierarchyTree*>();
    }
};

struct ViewHierarchyVector
{
    Vector<struct ViewHierarchyTree*> nodes;

    ViewHierarchyVector()
    {
        nodes = Vector<struct ViewHierarchyTree*>();
    }
};

class ViewHierarchyParser : virtual public RefBase {
public:
    ViewHierarchyParser();
    virtual ~ViewHierarchyParser();

    struct ViewHierarchyTree* ParseTree(char* region, String8 dumpFile);

    struct ViewHierarchyTree* ParseNode(String8 line);

    void TraverseTree(struct ViewHierarchyTree* root);

    void TraverseControls();
  
    KeyedVector<String8, struct ViewHierarchyVector*> GetControls();

    String8 FindMatchedIdByPoint(int x, int y, UiRect fw, UiRect& rect, int& groupIndex, int& itemIndex);

    UiRect FindMatchedRectById(String8 id, UiRect fw);

    KeyedVector<int, struct ViewHierarchyVector*> GroupIdRects(struct ViewHierarchyVector* vector, int interval = 10);

    void ReleaseTree(struct ViewHierarchyTree* root);

    void ReleaseControls();

private:
    int DetectIndentLevel(String8 line, String8 base);

    void AddTreeNode(struct ViewHierarchyTree* parent, struct ViewHierarchyTree* child);

    struct ViewHierarchyTree* FindParentByChildIndent(struct ViewHierarchyTree* node, int indent_level);

    void UpdateChildRect(struct ViewHierarchyTree* parent, struct ViewHierarchyTree* child);

    void UpdateControls(struct ViewHierarchyTree* node);

    bool IsValidIDRect(UiRect r);

    int FindCloseGroupRects(KeyedVector<int, struct ViewHierarchyVector*> groups, int k, int interval);

private:
    static const int indentUnit = 2;
    static const int splitNumber = 2;
    static const int rectIndex = 3; // id existing
    static const unsigned int basicItemSize = 4;
    static const unsigned int itemSizeWithId = 6;
    static const unsigned int minWidth = 10;
    static const unsigned int minHeight = 10;
    int rootWidth;
    UiRect emptyRect;
    KeyedVector<String8, struct ViewHierarchyVector*> controls;
};

};


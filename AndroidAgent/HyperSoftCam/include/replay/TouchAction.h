#ifndef _TOUCH_ACTION_H_
#define _TOUCH_ACTION_H_

#include "com/DumpsysWrapper.h"
#include "com/ViewHierarchyParser.h"
#include "com/WindowBarParser.h"

#include "QAction.h"
#include "replay/Device.h"

namespace android {

typedef enum {
    TOUCH_UP = 0,
    TOUCH_DOWN,
    TOUCH_MOVE
} TouchState;

struct TouchInfo
{
	int x;
	int y;
	String8 keyValueStr;

	TouchInfo()
	{
		x = 0;
		y = 0;
		keyValueStr = "";
	}
};

class TouchAction : public QAction {
public:
    TouchAction();
    virtual ~TouchAction();

    virtual bool Execute();
    void SetTouchInfo(String8 package_name,
	               TouchState touch_state,
                       int        orientation,
	       	       TouchInfo info
		     );

    void FindPointById(String8 id, BiUnit resolution, int orientation, uint16_t x, uint16_t y, UiRect& idRect);
    void FindWindowBarInfo(UiRect& sbRect, UiRect& nbRect, int& hasKb, UiRect& fwRect, int& isPopUp);
    KeyedVector<String8, String8> GetTouchDownKeyValue(String8 key_value_string);

private:
    void MapPoint();

private:
    int     m_x;
    int     m_y;
    String8 m_key_value_string;
    int     m_orientation;
    String8 m_touch_state_string;
    int     m_x_offset;
    int     m_y_offset;
    int     m_tree_id;
    String8 m_package_name;
    sp<DumpsysWrapper>         m_dumper;
    sp<ViewHierarchyParser>    m_parser;
    sp<WindowBarParser>        m_wb_parser;
};

}

#endif

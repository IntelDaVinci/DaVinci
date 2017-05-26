#include "replay/TouchAction.h"

#include "utils/Logger.h"

namespace android {

TouchAction::TouchAction(): m_tree_id(0), m_dumper(new DumpsysWrapper()), m_parser(new ViewHierarchyParser())
{
}

TouchAction::~TouchAction()
{
}

KeyedVector<String8, String8> TouchAction::GetTouchDownKeyValue(String8 touch_down_string)
{
    KeyedVector<String8, String8> key_value_vectors;
    String8 trim_str = StringUtil::trimLeft(touch_down_string);
    if(trim_str.length() == 0)
	return key_value_vectors;

    Vector<String8> line_items;
    line_items = StringUtil::split(trim_str, '&');
    String8 item_str, trim_item_str;
    Vector<String8> key_values;
    for(size_t i = 0; i < line_items.size(); i++)
    {
        item_str = line_items[i];
	trim_item_str = StringUtil::trimLeft(item_str);
    	if(trim_item_str.length() == 0)
   	    continue;
		
        key_values = StringUtil::split(trim_item_str, '=');
	if(key_value_vectors.indexOfKey(key_values[0]) < 0)
	    key_value_vectors.add(key_values[0], key_values[1]);
    }

    return key_value_vectors;
}

void TouchAction::FindWindowBarInfo(UiRect& sbRect, UiRect& nbRect, int& hasKb, UiRect& fwRect, int& isPopUp)
{
    char     buffer[256] = {0};
    int len = sprintf(buffer, "/data/local/tmp/%s/replay_%s.qs_%d.window", 
		m_package_name.string(), 
		m_package_name.string(), 
		m_tree_id);
    assert(len > 0);

    if(!m_dumper->InitializeRegion())
        return;

    m_dumper->DumpWindowBar();
    m_wb_parser->ParseInfo(m_dumper->GetRegion(), sbRect, nbRect, hasKb, fwRect, isPopUp, String8(buffer));
    m_dumper->ReleaseRegion();
}

void TouchAction::FindPointById(String8 id, BiUnit resolution, int orientation, uint16_t x, uint16_t y, UiRect& rect)
{
    char     buffer[256] = {0};
    int len = sprintf(buffer, "/data/local/tmp/%s/replay_%s.qs_%d.tree", 
		m_package_name.string(), 
		m_package_name.string(), 
		m_tree_id);
    assert(len > 0);

    if(!m_dumper->InitializeRegion())
	return;

    m_dumper->DumpViewHierarchy();

    struct ViewHierarchyTree* root = m_parser->ParseTree(m_dumper->GetRegion(), String8(buffer));

    DWLOGW("TOUCHDOWN on device screen at %u %u with %s", x, y, id.string());
	
    UiRect sbRect, nbRect, fwRect;
    int hasKb, isPopUp;

    FindWindowBarInfo(sbRect, nbRect, hasKb, fwRect, isPopUp);

    rect = m_parser->FindMatchedRectById(id, fwRect);

    m_parser->ReleaseTree(root);
    m_parser->ReleaseControls();
    m_dumper->ReleaseRegion();
}

bool TouchAction::Execute()
{
    AUTO_LOG();

    MapPoint();

    if (m_touch_state_string.length() > 0) {
        m_command = String8::format("touch %s %d %d",
                                m_touch_state_string.string(),
                                m_x,
                                m_y
                                );
        DWLOGD("commands: %s", m_command.string());
        return QAction::Execute();
    }

    return false;
}

void TouchAction::MapPoint()
{
    BiUnit               resolution         = Device::getInstance().GetResolution();
    OrientationIndicator device_orientation = Device::getInstance().GetOrientation();

    int delta = m_orientation - device_orientation;
    delta = delta < 0 ? -delta : delta;

    // Check whether device screen is reverse or not
    if (delta != 0 && delta != 2) {
	DWLOGE("Invalid device orientation");
        return;
    }

    double w_ratio = (double)resolution.a / 4096;
    double h_ratio = (double)resolution.b / 4096;

    // Device rotated
    if (device_orientation % 2 != 0) {
        double temp_ratio = w_ratio;
        w_ratio = h_ratio;
        h_ratio = temp_ratio;
    }

    m_x = (int)(((double)m_x * w_ratio) + 0.5);
    m_y = (int)(((double)m_y * h_ratio) + 0.5);

    // Device screen is reverse
    if (delta == 2) {
        m_x = resolution.a - m_x;
        m_y = resolution.b - m_y;
    }

    if(m_touch_state_string == "down")
    {
	// m_x and m_y is Portrait/Landscape coordindate
	KeyedVector<String8, String8> key_value_vectors = GetTouchDownKeyValue(m_key_value_string);
	String8 id_value;
	if(key_value_vectors.indexOfKey(String8("id")) >= 0)
	{
   	    id_value = key_value_vectors.valueFor(String8("id"));
	}

	if(id_value == "")
	    return;

	String8 ratio_value;
	if(key_value_vectors.indexOfKey(String8("ratio")) >= 0)
	{
	    ratio_value = key_value_vectors.valueFor(String8("ratio"));
	}
		
	if(ratio_value == "")
	    return;

	UiRect idRect;
	String8 ratioStr = StringUtil::substr(ratio_value, 1, ratio_value.length() - 2);
        Vector<String8> items = StringUtil::split(ratioStr, ',');
	if(items.size() != 2)
    	    return;

	++m_tree_id;


	FindPointById(id_value, resolution, m_orientation, m_x, m_y, idRect);
	int tmp_x = idRect.x + (int)(atof(items[0]) * idRect.width);
	int tmp_y = idRect.y + (int)(atof(items[1]) * idRect.height);
	m_x_offset = tmp_x - m_x;
	m_y_offset = tmp_y - m_y;
	m_x = tmp_x;
	m_y = tmp_y;
    }
    else if(m_touch_state_string == "move")
    {
    	m_x = m_x + m_x_offset;
    	m_y = m_y + m_y_offset;
    }
    else
    {
        m_x = m_x + m_x_offset;
    	m_y = m_y + m_y_offset;
	m_x_offset = 0;
	m_y_offset = 0;
    }
}

void TouchAction::SetTouchInfo(String8 package_name,
	                        TouchState touch_state,
                                int        oritention,
				TouchInfo info)
{
    AUTO_LOG();

    DWLOGD("Touch info: (%d, %d, %d, %s)", info.x, info.y, oritention, info.keyValueStr.string());

    if (touch_state == TOUCH_UP) {
        m_touch_state_string = "up";
    } else if (touch_state == TOUCH_DOWN) {
        m_touch_state_string = "down";
    } else if (touch_state == TOUCH_MOVE) {
        m_touch_state_string = "move";
    } else {
        DWLOGE("The touch state (%d) does not support.", touch_state);
    }

    m_x                = info.x;
    m_y                = info.y;
    m_key_value_string = info.keyValueStr;
    m_orientation      = oritention;
    m_package_name     = package_name;
}

}

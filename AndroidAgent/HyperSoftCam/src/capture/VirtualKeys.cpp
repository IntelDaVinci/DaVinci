#include "capture/VirtualKeys.h"

#include <sys/types.h>
#include <dirent.h>

#include "utils/StringUtil.h"
#include "utils/Logger.h"

namespace android {

bool CVirtualKeys::LoadVirtualKeyMapFile()
{
    struct dirent *file_entry           = NULL;
    DIR           *board_properties_dir = NULL;
    const String8  str_board_properties_dir("/sys/board_properties/");

    board_properties_dir = opendir(str_board_properties_dir.string());
    if (board_properties_dir == NULL) {
        DWLOGW("Cannot find board properties.");
        return false;
    }

    while ((file_entry = readdir(board_properties_dir))) {
        String8 file_name = String8(file_entry->d_name);
        if (StringUtil::startsWith(file_name, "virtualkeys")) {
            // TODO: file_name should be full path
            if (ParseVirtualKeys(str_board_properties_dir + file_name)) {
                closedir(board_properties_dir);
                return true;
            } else {
                break;
            }
        }
    }

    closedir(board_properties_dir);
    return false;
}

VirtualKeysType CVirtualKeys::IdentifyVirtualKey(int touchPointX, int touchPointY)
{
    for (int i = 0; i < m_virtual_keys.size(); i++) {
        VirtualKeyArea virtual_key_area = m_virtual_keys.valueAt(i);
        if (touchPointX >= (virtual_key_area.centerX - virtual_key_area.width / 2) &&
            touchPointX <= (virtual_key_area.centerX + virtual_key_area.width / 2) &&
            touchPointY >= (virtual_key_area.centerY - virtual_key_area.height / 2) &&
            touchPointY <= (virtual_key_area.centerY + virtual_key_area.height / 2)) {
            return m_virtual_keys.keyAt(i);
        }
    }

    return INVALID_KEY;
}

bool CVirtualKeys::ParseVirtualKeys(String8 file_name)
{
    char  line_buffer[1024]    = {0};
    FILE *virtual_key_map_file = fopen(file_name.string(), "r");

    if (virtual_key_map_file == NULL) {
        DWLOGE("Cannot open file - %s", file_name.string());
        return false;
    }

    // Please refer to
    //     https://source.android.com/devices/input/touch-devices.html#virtual-key-map-files
    // for details
    while (fgets(line_buffer, sizeof(line_buffer), virtual_key_map_file)) {
        String8 str_line = StringUtil::trimLeft(String8(line_buffer));
        if (!StringUtil::startsWith(str_line, "#")) {
            Vector<String8> ret = StringUtil::split(str_line, ':');
            for (size_t i = 0; i < ret.size(); ) {
                if (StringUtil::compare(ret[i++], "0x01") == 0) {
                    VirtualKeysType virtual_key_type = INVALID_KEY;
                    if (StringUtil::compare(ret[i], "158") == 0) {
                        virtual_key_type = BACK_KEY;
                    } else if (StringUtil::compare(ret[i], "139") == 0) {
                        virtual_key_type = MENU_KEY;
                    } else if (StringUtil::compare(ret[i], "102") == 0) {
                        virtual_key_type = HOME_KEY;
                    }

                    if (virtual_key_type != INVALID_KEY) {
                        VirtualKeyArea virtual_key_area;
                        virtual_key_area.centerX = StringUtil::strToInt(ret[++i]);
                        virtual_key_area.centerY = StringUtil::strToInt(ret[++i]);
                        virtual_key_area.width   = StringUtil::strToInt(ret[++i]);
                        virtual_key_area.height  = StringUtil::strToInt(ret[++i]);

                        m_virtual_keys.add(virtual_key_type, virtual_key_area);
                    }
                }

                i += 1;
            }
        }

        memset(line_buffer, 0, sizeof(line_buffer));
    }

    fclose(virtual_key_map_file);
    return true;
}

}

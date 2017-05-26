#ifndef _VIRTUAL_KEYS_H_
#define _VIRTUAL_KEYS_H_

#include <utils/String8.h>
#include <utils/KeyedVector.h>

namespace android {

typedef enum _VirtualKeysType {
    BACK_KEY = 0,
    MENU_KEY,
    HOME_KEY,
    INVALID_KEY = 0XFF
} VirtualKeysType;

typedef struct _VirtualKeyArea {
    int centerX;
    int centerY;
    int width;
    int height;
} VirtualKeyArea;

class CVirtualKeys {
public:
    bool LoadVirtualKeyMapFile();
    VirtualKeysType IdentifyVirtualKey(int touchPointX, int touchPointY);

private:
    bool ParseVirtualKeys(String8 file_name);

private:
    KeyedVector<VirtualKeysType, VirtualKeyArea> m_virtual_keys;
};

}

#endif


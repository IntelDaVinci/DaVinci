#include <utils/String8.h>
#include "utils/Logger.h"
#include "utils/StringUtil.h"
#include "com/UiRect.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <utils/RefBase.h>

namespace android {

class WindowBarParser : virtual public RefBase {
public:
    WindowBarParser();
    virtual ~WindowBarParser();

    UiRect ParseBarInfo(String8 line, String8 key);
    void ParseInfo(char* region, UiRect& sb, UiRect& nb, int& kb, UiRect& fw, int& popup, String8 dumpFile);
};

};


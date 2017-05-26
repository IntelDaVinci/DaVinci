#include "replay/MonkeyService.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <utils/String8.h>

#include "replay/MonkeyBroker.h"
#include "utils/Logger.h"

namespace android {

MonkeyService::MonkeyService(int listen_port)
{
    m_monkey_stream = NULL;
    m_listen_port   = listen_port;
}

MonkeyService::~MonkeyService()
{
    if (m_monkey_stream != NULL)
        Stop();
}

bool MonkeyService::Start()
{
    AUTO_LOG();

    String8 sub_command = String8::format("/data/local/tmp/davinci-monkey --port %d",
                                          m_listen_port
                                          );
    m_monkey_stream = popen(sub_command.string(), "r");
    if (m_monkey_stream == NULL) {
        DWLOGE("Cannot open monkey process! (%s)", strerror(errno));
        return false;
    }

    // Refer to
    // android-platform_sdk/blob/master/monkeyrunner/src/com/android/monkeyrunner/adb/AdbMonkeyDevice.java
    // In the file, the time out for starting monkey is 1 second.

    int try_cnt = 0;
    while (try_cnt < 10) {
        sleep(1);
        if (MonkeyBroker::getInstance().ExecuteSyncCommand(String8("wake"))) {
            DWLOGI("Succeded staring monkey as server...");
            return true;
        }
        try_cnt++;
    }

    DWLOGE("Cannot start monkey or has been time out...");
    return false;
}

bool MonkeyService::Stop()
{
    AUTO_LOG();

    if (m_monkey_stream != NULL) {
        MonkeyBroker::getInstance().ExecuteSyncCommand(String8("quit"));
        int ret = pclose(m_monkey_stream);
        if (ret < 0) {
            DWLOGE("Some error happened when closing monkey process! (%s)",
                   strerror(errno)
                   );
        }

        m_monkey_stream = NULL;
    }

    return true;
}

}

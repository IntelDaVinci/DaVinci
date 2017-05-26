#ifndef _LOGGER_H_
#define _LOGGER_H_

#if defined(LOG_TAG)
#undef LOG_TAG
#endif

#define LOG_TAG "CameraLess"

#include <utils/Log.h>
#include <stdio.h>
#include <string.h>

namespace android {

#ifdef APP_DEBUG
    #define DWLOGE(fmt, args...) \
        ALOGE("%s:%s [%d][tid:%d] " fmt, __FILE__, __FUNCTION__, __LINE__, gettid(), ##args)

    #define DWLOGW(fmt, args...) \
        ALOGW("%s:%s [%d][tid:%d] " fmt, __FILE__, __FUNCTION__, __LINE__, gettid(), ##args)

    #define DWLOGI(fmt, args...) \
        ALOGI(fmt, ##args)

    #define DWLOGD(fmt, args...) \
        ALOGD("%s:%s [%d][tid:%d] " fmt, __FILE__, __FUNCTION__, __LINE__, gettid(), ##args)
#else
    #define DWLOGE(fmt, args...) \
        ALOGE("[tid:%d] " fmt, gettid(), ##args)

    #define DWLOGW(fmt, args...) \
        ALOGW("[tid:%d] " fmt, gettid(), ##args)

    #define DWLOGI(fmt, args...) ((void)0)
    #define DWLOGD(fmt, args...) ((void)0)
#endif

class CAutoLogger {
public:
    CAutoLogger(const char *file_name, const char *funtion_name, int log_line) {
        m_function_name[strlen(funtion_name)] = '\0';
        m_file_name[strlen(file_name)] = '\0';
        DWLOGI("%s:%s [%d][tid:%d] <<<<<<<<<<", file_name, funtion_name, log_line, gettid());
        strcpy(m_function_name, funtion_name);
        strcpy(m_file_name, file_name);
        m_log_line = log_line;
    }
    ~CAutoLogger() {
        DWLOGI("%s:%s [%d][tid:%d] >>>>>>>>>>", m_file_name, m_function_name, m_log_line, gettid());
    }
private:
    char m_function_name[128];
    char m_file_name[512];
    int m_log_line;
};

#ifdef APP_DEBUG
#define AUTO_LOG() \
    CAutoLogger _auto_logger(__FILE__, __FUNCTION__, __LINE__)
#else
#define AUTO_LOG() ((void)0)
#endif
}

#endif


#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "vm/thread_defs.h"

#include <fstream>
#include <assert.h>

#define LOG_FILE_NAME_PREFIX ("remote-screen")

class CLogger {
public:
    typedef enum {
        LOG_ERRO,
        LOG_WARN,
        LOG_INFO,
        LOG_DEBUG
    } LOG_LEVEL;

    class CAutoMutex {
    public:
        CAutoMutex(MSDKMutex *mutex) {
            assert(mutex != NULL);
            mutex->Lock();
            m_mutex = mutex;
        }
        ~CAutoMutex() {
            m_mutex->Unlock();
        }
    private:
        MSDKMutex *m_mutex;
    };

public:
    static CLogger * Instance() {
        if (0 == m_gLogger) {
            m_gMutex.Lock();
            if (0 == m_gLogger) {
                m_gLogger = new CLogger();
            }
            m_gMutex.Unlock();
        }

        return m_gLogger;
    }

    static void DestoryInstance() {
        if (0 != m_gLogger) {
            m_gMutex.Lock();
            if (0 != m_gLogger) {
                delete m_gLogger;
                m_gLogger = NULL;
            }
            m_gMutex.Unlock();
        }
    }

public:
    void Log(LOG_LEVEL level, const char *format, ...);
    void DumpHex(LOG_LEVEL level, const char *data, int data_len);

protected:
    CLogger();
    ~CLogger() {
        if (m_logFile != 0) {
            m_logFile->close();
            delete m_logFile;
        }
    }

private:
    CLogger(const CLogger& obj);
    CLogger& operator=(CLogger& obj);

private:
    static CLogger *m_gLogger;
    static MSDKMutex m_gMutex;

    MSDKMutex        m_logFileMutex;
    std::ofstream   *m_logFile;
};

class CAutoLogger {
public:
    CAutoLogger(char *funtion_name, int line)
    {
        memset(m_funcName, 0, sizeof(m_funcName));
        strncpy_s(m_funcName, funtion_name, strlen(funtion_name));
        m_line = line;
        CLogger::Instance()->Log(CLogger::LOG_INFO, "%s <<<<<<<<<<", m_funcName);
    }
    ~CAutoLogger()
    {
        CLogger::Instance()->Log(CLogger::LOG_INFO, "%s >>>>>>>>>>>", m_funcName);
    }
private:
    char  m_funcName[512];
    int   m_line;
};

#define AUTO_LOG() \
    CAutoLogger _auto_logger(__FUNCTION__, __LINE__)

#define LOGE(fmt, ...) \
    CLogger::Instance()->Log(CLogger::LOG_ERRO, fmt, __VA_ARGS__);

#define LOGW(fmt, ...) \
    CLogger::Instance()->Log(CLogger::LOG_WARN, fmt, __VA_ARGS__);

#define LOGI(fmt, ...) \
    CLogger::Instance()->Log(CLogger::LOG_INFO, fmt, __VA_ARGS__);

#define LOGD(fmt, ...) \
    CLogger::Instance()->Log(CLogger::LOG_DEBUG, fmt, __VA_ARGS__);


#endif

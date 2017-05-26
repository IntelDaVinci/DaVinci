#include "logger.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

CLogger* CLogger::m_gLogger;
MSDKMutex CLogger::m_gMutex;

const char hex_asc_upper[] = "0123456789ABCDEF";
#define hex_asc_hi(x)   hex_asc_upper[((x) & 0xf0) >> 4]
#define hex_asc_lo(x)   hex_asc_upper[((x) & 0x0f)]


extern unsigned short g_log_level;

CLogger::CLogger() {
    m_logFile = NULL;

    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    char log_file_path[512] = {0};
    sprintf_s(log_file_path,
              "%s_%d-%02d-%02d.log",
              LOG_FILE_NAME_PREFIX,
              sys_tm->tm_year+1900,
              sys_tm->tm_mon+1,
              sys_tm->tm_mday);
    m_logFile = new std::ofstream(log_file_path, std::ofstream::app);
}

void CLogger::Log(LOG_LEVEL level, const char *format, ...)
{
    if (level > g_log_level)
        return;

    CAutoMutex Locker(&m_logFileMutex);

    char buf[4096] = {0};
    char input_buf[2048] = {0};

    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[32] = {0};
    switch(level)
    {
    case LOG_INFO : strcpy_s(s, "[Info   ]"); break;
    case LOG_WARN : strcpy_s(s, "[Warning]"); break;
    case LOG_ERRO : strcpy_s(s, "[Error  ]"); break;
    case LOG_DEBUG : strcpy_s(s, "[Debug  ]"); break;
    default: strcpy_s(s, "[Info   ]"); break;
    }

    sprintf_s(buf,
              "%d-%02d-%02d %02d:%02d:%02d %s [pid:%10d] ",
              my_tm.tm_year+1900,
              my_tm.tm_mon+1,
              my_tm.tm_mday,
              my_tm.tm_hour,
              my_tm.tm_min,
              my_tm.tm_sec,
              s,
              MSDKThread::GetPID());

    va_list valst;
    va_start(valst, format);
    vsprintf_s(input_buf, format, valst);
    strncat_s(buf, input_buf, 4095);
    buf[strlen(buf)-1] = '\n';
    m_logFile->write(buf, strlen(buf));
    m_logFile->flush();
    va_end(valst);
}

void CLogger::DumpHex(LOG_LEVEL level, const char *data, int data_len)
{
    if (level > g_log_level)
        return;

    const unsigned char *ptr = (const unsigned char *)data;
    unsigned char ch;
    int lx = 0;
    const static int row_size = 16;

    char line_buffer[128] = {0};
    for (int i = 0; i < data_len; i++) {
        ch = ptr[i];
        line_buffer[lx++] = hex_asc_hi(ch);
        line_buffer[lx++] = hex_asc_lo(ch);
        line_buffer[lx++] = ' ';

        if ((i + 1) % row_size == 0 || (i + 1) == data_len) {
            int mod_val = (i + 1) % row_size;
            if (mod_val != 0) {
                for (int l = 0; l < row_size - mod_val; l++) {
                    line_buffer[lx++] = ' ';
                    line_buffer[lx++] = ' ';
                    line_buffer[lx++] = ' ';
                }
            }

            line_buffer[lx++] = ' ';
            line_buffer[lx++] = ' ';
            line_buffer[lx++] = ' ';

            for (int j = i / row_size * row_size; j <= i; j++) {
                ch = ptr[j];
                line_buffer[lx++] = (isascii(ch) && isprint(ch)) ? ch : '.';
            }

            line_buffer[lx++] = '\n';
            Log(level, "%s", line_buffer);
            memset(line_buffer, 0, sizeof(line_buffer));
            lx = 0;

            if ((i + 1) == data_len)
                break;
        }
    }
}

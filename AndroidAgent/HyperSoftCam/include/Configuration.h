#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <utils/Singleton.h>
#include <utils/String8.h>

namespace android {

typedef struct _ResConf {
    uint32_t w;
    uint32_t h;
} ResConf;

typedef struct _ReplayConf {
    String8  app_qs_name;
    bool     is;
} ReplayConf;

typedef struct _StreamConf {
    uint16_t  port;
    bool      is;
} StreamConf;

typedef struct _RecordConf {
    String8  app_name;
    String8  package_name;
    String8  activity_name;
    String8  trace_in_file;
    String8  trace_out_file;
    bool     is;
} RecordConf;

typedef struct _HSCConf {
    bool       stat_mode;
    bool       id_record;
    ResConf    res_conf;
    StreamConf stream_conf;
    ReplayConf replay_conf;
    RecordConf record_conf;
    uint32_t   fps;
    bool       record_frame_index;
} HSCConf;

class Configuration : public Singleton<Configuration> {
friend class Singleton<Configuration>;

protected:
    Configuration();
    ~Configuration();

public:
    bool ParseConsoleParameters(int argc, char *argv[]);
    HSCConf* Get();

private:
    void DisplayHelp();
    void DisplayVersion();

private:
    HSCConf m_hsc_conf;
};

}

#endif

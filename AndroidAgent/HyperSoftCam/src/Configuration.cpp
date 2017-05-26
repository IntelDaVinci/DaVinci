#include "Configuration.h"

#include <getopt.h>
#include <sys/stat.h>
#include <utils/Debug.h>
#include <utils/Vector.h>

#include "utils/ConvUtils.h"
#include "utils/StringUtil.h"
#include "utils/Logger.h"

namespace android {

#if defined(AOSP19)
    #define ANDROID_OS_NAME "Android 4.4"
#elif defined(AOSP18)
    #define ANDROID_OS_NAME "Android 4.3"
#elif defined(AOSP17)
    #define ANDROID_OS_NAME "Android 4.2"
#elif defined(AOSP21)
    #define ANDROID_OS_NAME "Android 5.0"
#elif defined(AOSP22)
    #define ANDROID_OS_NAME "Android 5.1"
#elif defined(AOSP23)
    #define ANDROID_OS_NAME "Android 6.0"
#else
    #error "Does not support current android OS !!!"
#endif

#define MAIN_VER_NUM  1
#define MINOR_VER_NUM 0

#if !defined(HSC_VERSION)
#define HSC_VERSION "Invalid"
#endif

#define TRACE_IN_FILE "/data/local/tmp/A0A2338C-7CBE-42E3-AF43-4D0BB82E9968"
#define TRACE_OUT_FILE "/data/local/tmp/FA7F8C87-995C-48E3-ADDA-9ED39D3A3E94"

ANDROID_SINGLETON_STATIC_INSTANCE(Configuration)

Configuration::Configuration()
{
    m_hsc_conf.res_conf.w           = 0;
    m_hsc_conf.res_conf.h           = 0;

    m_hsc_conf.stat_mode            = false;
    m_hsc_conf.id_record            = false;

    m_hsc_conf.stream_conf.port     = 0;

    m_hsc_conf.stream_conf.is       = false;
    m_hsc_conf.replay_conf.is       = false;
    m_hsc_conf.record_conf.is       = false;

    m_hsc_conf.fps                  = 0;
    m_hsc_conf.record_frame_index   = false;
}

Configuration::~Configuration()
{
}

bool Configuration::ParseConsoleParameters(int argc, char *argv[])
{
    AUTO_LOG();

    static const struct option options[] = {
        { "help",               no_argument,        NULL, 'h' },
        { "rnr",                required_argument,  NULL, 'r' },
        { "port",               required_argument,  NULL, 'p' },
        { "play",               required_argument,  NULL, 'l' },
        { "version",            no_argument,        NULL, 'v' },
        { "stat",               no_argument,        NULL, 's' },
        { "id",                 no_argument,        NULL, 'i' },
        { "res",                required_argument,  NULL, 'e' },
        { "fps",                required_argument,  NULL, 'f' },
        { "index",              no_argument,        NULL, 'n' },
        { "rt",                 no_argument,        NULL, 't' },
        { NULL,                 0,                  NULL, 0 }
    };

    int option_index = 0;
    int id           = -1;
    while ((id = getopt_long(argc, argv, "hr:p:vsf", options, &option_index)) != -1) {
        switch (id) {
        case 'h':
            {
                DisplayHelp();
            }
            break;
        case 'v':
            {
                DisplayVersion();
            }
            break;
        case 'p':
            {
                m_hsc_conf.stream_conf.is   = true;
                m_hsc_conf.stream_conf.port = atoi(optarg);
                DWLOGI("Running in RnR mode. Listening port is (%d).", m_hsc_conf.stream_conf.port);
            }
            break;
        case 's':
            {
                //Statistics::getInstance().Enable();
                m_hsc_conf.stat_mode = true;
                DWLOGI("Running in Statistic mode. Check ADB log for time consumption of each step.");
            }
            break;
        case 'i':
            {
                m_hsc_conf.id_record = true;
                DWLOGI("Recording in id mode.");
            }
            break;
        case 'r':
            {
                m_hsc_conf.record_conf.is = true;
                Vector<String8> rnr_parameters = StringUtil::split(String8(optarg), ':');
                if (rnr_parameters.size() > 3) {
                    fprintf(stderr, "Invalid parameters for R&R. The format shoudl be app:package:activity");
                    return false;
                }

                m_hsc_conf.record_conf.app_name      = rnr_parameters[0];
                m_hsc_conf.record_conf.package_name  = rnr_parameters[1];
                m_hsc_conf.record_conf.activity_name = rnr_parameters[2];

                DWLOGI("Running in RecordingOnDevice mode(%s:%s:%s).",
                       m_hsc_conf.record_conf.app_name.string(),
                       m_hsc_conf.record_conf.package_name.string(),
                       m_hsc_conf.record_conf.activity_name.string()
                       );
            }
            break;
        case 'l':
            {
                m_hsc_conf.replay_conf.is          = true;
                m_hsc_conf.replay_conf.app_qs_name = String8(optarg);

                DWLOGI("DaVinci will play the %s on device.", optarg);
            }
            break;
        case 'e':
            {
                Vector<String8> _res = StringUtil::split(String8(optarg), 'x');
                if (_res.size() != 2) {
                    fprintf(stderr, "Invalid parameters for HyperSoftCam. The format should be WxH");
                    return 2;
                }

                m_hsc_conf.res_conf.w = atoi(_res[0]);
                m_hsc_conf.res_conf.h = atoi(_res[1]);

                DWLOGI("HyperSoftCam resolution is (%d, %d)", m_hsc_conf.res_conf.w, m_hsc_conf.res_conf.h);

                if (m_hsc_conf.res_conf.w <= 0 || m_hsc_conf.res_conf.h <= 0) {
                    fprintf(stderr, "Invalid parameters for HyperSoftCam. The format shoudl be wxh");
                    return 2;
                }
            }
            break;
        case 'f':
            {
                m_hsc_conf.fps = atoi(optarg);
                DWLOGI("HyperSoftCam FPS is (%d).", m_hsc_conf.fps);
            }
            break;
        case 'n':
            {
                m_hsc_conf.record_frame_index = true;
                DWLOGI("Record frame index and time stamp.");
            }
            break;
        case 't':
            {
                printf("%llu\n", GetCurrentTimeStamp() / 1000);
            }
            break;
        default:
            if (id != '?') {
                fprintf(stderr, "getopt_long returned unexpected value 0x%x\n", id);
                return false;
            }
            break;
        }
    }
    struct stat st;
    if (stat(TRACE_IN_FILE, &st) == 0)
    {
        m_hsc_conf.record_conf.trace_in_file = String8(TRACE_IN_FILE);
        DWLOGI("Trace input file %s", TRACE_IN_FILE);
    }
    if (stat(TRACE_OUT_FILE, &st) == 0)
    {
        m_hsc_conf.record_conf.trace_out_file = String8(TRACE_OUT_FILE);
        DWLOGI("Trace output file %s", TRACE_OUT_FILE);
    }

    return true;
}

HSCConf* Configuration::Get()
{
    return &m_hsc_conf;
}

void Configuration::DisplayHelp()
{
    fprintf(stderr,
            "Usage: HyperSoftCam\n"
            "\n"
            "Options:\n"
            "-r, --rnr\n"
            "    Record video and QScript on device\n"
            "--play\n"
            "    Replay q-script on device\n"
            "-s, --stat\n"
            "    Performance measure, focus on latency between device and host.\n"
            "-i, --id\n"
            "    Enable id recording.\n"
            "-p, --port xxxxxx\n"
            "    Listening port and waiting for TCP connection.\n"
            "-v, --version\n"
            "    Show HyperSoftCam version.\n"
            "-h, --help\n"
            "    Show help message.\n"
            "--res 800x600\n"
            "    Set HyperSoftCam resolution.\n"
            "--fps xxx\n"
            "    Set HyperSoftCam FPS.\n"
            "--index\n"
            "    Record frame index and time stamp.\n"
            "--rt\n"
            "    Record current time stamp to /data/local/tmp/cur_time.\n"
            "\n"
            "Recording continues until Ctrl-C is hit or the time limit is reached.\n"
            "\n"
            );
}


void Configuration::DisplayVersion()
{
    fprintf(stderr,
            "Version (%s-%u.%u-%s)\n",
            ANDROID_OS_NAME,
            MAIN_VER_NUM,
            MINOR_VER_NUM,
            HSC_VERSION
            );
}

}

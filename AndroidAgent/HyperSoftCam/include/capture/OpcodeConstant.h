#ifndef _OPCODE_CONSTANT_H_
#define _OPCODE_CONSTANT_H_

namespace android {

#define OP(arr_name, op) arr_name[op] = #op
enum {
    OPCODE_INVALID                    = 0,
    OPCODE_CLICK                      = 1,
    OPCODE_SWIPE                      = 2,
    OPCODE_HOME                       = 3,
    OPCODE_BACK                       = 4,
    OPCODE_MENU                       = 5,
    OPCODE_DRAG                       = 6,
    OPCODE_START_APP                  = 7,
    OPCODE_SET_CLICKING_HORIZONTAL    = 8,
    OPCODE_TOUCHDOWN                  = 9,
    OPCODE_TOUCHDOWN_HORIZONTAL       = 10,
    OPCODE_TOUCHUP                    = 11,
    OPCODE_TOUCHUP_HORIZONTAL         = 12,
    OPCODE_TOUCHMOVE                  = 13,
    OPCODE_TOUCHMOVE_HORIZONTAL       = 14,
    OPCODE_OCR                        = 15,
    OPCODE_TILTUP                     = 16,
    OPCODE_TILTDOWN                   = 17,
    OPCODE_TILTLEFT                   = 18,
    OPCODE_TILTRIGHT                  = 19,
    OPCODE_TILTTO                     = 20,
    OPCODE_PREVIEW                    = 21,
    OPCODE_MATCH_ON                   = 22,
    OPCODE_MATCH_OFF                  = 23,
    OPCODE_IF_MATCH_IMAGE             = 24,
    OPCODE_MESSAGE                    = 25,
    OPCODE_ERROR                      = 26,
    OPCODE_CLICK_MATCHED_IMAGE        = 27,
    OPCODE_GOTO                       = 28,
    OPCODE_EXIT                       = 29,
    OPCODE_INSTALL_APP                = 30,
    OPCODE_UNINSTALL_APP              = 31,
    OPCODE_PUSH_DATA                  = 32,
    OPCODE_CALL_SCRIPT                = 33,
    OPCODE_MULTI_TOUCHDOWN            = 34,
    OPCODE_MULTI_TOUCHDOWN_HORIZONTAL = 35,
    OPCODE_MULTI_TOUCHUP              = 36,
    OPCODE_MULTI_TOUCHUP_HORIZONTAL   = 37,
    OPCODE_MULTI_TOUCHMOVE            = 38,
    OPCODE_MULTI_TOUCHMOVE_HORIZONTAL = 39,
    OPCODE_NEXT_SCRIPT                = 40,
    OPCODE_HOME_DOWN                  = 41,
    OPCODE_HOME_UP                    = 42,
    OPCODE_CALL_EXTERNAL_SCRIPT       = 43,
    OPCODE_ELAPSED_TIME               = 44,
    OPCODE_FPS_MEASURE_START          = 45,
    OPCODE_FPS_MEASURE_STOP           = 46,
    OPCODE_CONCURRENT_IF_MATCH        = 47,
    OPCODE_IF_MATCH_TXT               = 48,
    OPCODE_CLICK_MATCHED_TXT          = 49,
    OPCODE_BATCH_PROCESSING           = 50,
    OPCODE_IF_MATCH_REGEX             = 51,
    OPCODE_CLICK_MATCHED_REGEX        = 52,
    OPCODE_AUDIO_RECORD_START         = 53,
    OPCODE_AUDIO_RECORD_STOP          = 54,
    OPCODE_IF_MATCH_AUDIO             = 55,
    OPCODE_AI_BUILTIN                 = 56,
    OPCODE_IMAGE_MATCHED_TIME         = 57,
    OPCODE_FLICK_ON                   = 58,
    OPCODE_FLICK_OFF                  = 59,
    OPCODE_AUDIO_PLAY                 = 60,
    OPCODE_IF_MATCH_IMAGE_WAIT        = 61,
    OPCODE_CLICK_MATCHED_IMAGE_XY     = 62,
    OPCODE_CLEAR_DATA                 = 63,
    OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY = 64,
    OPCODE_TOUCHUP_MATCHED_IMAGE_XY   = 65,
    OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY = 66,
    OPCODE_MOUSE_EVENT                = 67,
    OPCODE_KEYBOARD_EVENT             = 68,
    OPCODE_IF_SOURCE_IMAGE_MATCH      = 69,
    OPCODE_IF_SOURCE_LAYOUT_MATCH     = 70,
    OPCODE_CRASH_ON                   = 71,
    OPCODE_CRASH_OFF                  = 72,
    OPCODE_SET_VARIABLE               = 73,
    OPCODE_LOOP                       = 74,
    OPCODE_STOP_APP                   = 75,
    OPCODE_SET_TEXT                   = 76,
    OPCODE_EXPLORATORY_TEST           = 77,
    OPCODE_POWER                      = 78,
    OPCODE_LIGHT_UP                   = 79,
    OPCODE_VOLUME_UP                  = 80,
    OPCODE_VOLUME_DOWN                = 81,
    OPCODE_DEVICE_POWER               = 82,
    OPCODE_AUDIO_MATCH_ON             = 83,
    OPCODE_AUDIO_MATCH_OFF            = 84,
    OPCODE_HOLDER_UP                  = 85,
    OPCODE_HOLDER_DOWN                = 86,
    OPCODE_BUTTON                     = 87,
    OPCODE_USB_IN                     = 88,
    OPCODE_USB_OUT                    = 89,
    OPCODE_COUNT
};

#define DELARE_OPCODE_NAMES(arr_name)            \
OP(arr_name, OPCODE_INVALID);                    \
OP(arr_name, OPCODE_CLICK);                      \
OP(arr_name, OPCODE_SWIPE);                      \
OP(arr_name, OPCODE_HOME);                       \
OP(arr_name, OPCODE_BACK);                       \
OP(arr_name, OPCODE_MENU);                       \
OP(arr_name, OPCODE_DRAG);                       \
OP(arr_name, OPCODE_START_APP);                  \
OP(arr_name, OPCODE_SET_CLICKING_HORIZONTAL);    \
OP(arr_name, OPCODE_TOUCHDOWN);                  \
OP(arr_name, OPCODE_TOUCHDOWN_HORIZONTAL);       \
OP(arr_name, OPCODE_TOUCHUP);                    \
OP(arr_name, OPCODE_TOUCHUP_HORIZONTAL);         \
OP(arr_name, OPCODE_TOUCHMOVE);                  \
OP(arr_name, OPCODE_TOUCHMOVE_HORIZONTAL);       \
OP(arr_name, OPCODE_OCR);                        \
OP(arr_name, OPCODE_TILTUP);                     \
OP(arr_name, OPCODE_TILTDOWN);                   \
OP(arr_name, OPCODE_TILTLEFT);                   \
OP(arr_name, OPCODE_TILTRIGHT);                  \
OP(arr_name, OPCODE_TILTTO);                     \
OP(arr_name, OPCODE_PREVIEW);                    \
OP(arr_name, OPCODE_MATCH_ON);                   \
OP(arr_name, OPCODE_MATCH_OFF);                  \
OP(arr_name, OPCODE_IF_MATCH_IMAGE);             \
OP(arr_name, OPCODE_MESSAGE);                    \
OP(arr_name, OPCODE_ERROR);                      \
OP(arr_name, OPCODE_CLICK_MATCHED_IMAGE);        \
OP(arr_name, OPCODE_GOTO);                       \
OP(arr_name, OPCODE_EXIT);                       \
OP(arr_name, OPCODE_INSTALL_APP);                \
OP(arr_name, OPCODE_UNINSTALL_APP);              \
OP(arr_name, OPCODE_PUSH_DATA);                  \
OP(arr_name, OPCODE_CALL_SCRIPT);                \
OP(arr_name, OPCODE_MULTI_TOUCHDOWN);            \
OP(arr_name, OPCODE_MULTI_TOUCHDOWN_HORIZONTAL); \
OP(arr_name, OPCODE_MULTI_TOUCHUP);              \
OP(arr_name, OPCODE_MULTI_TOUCHUP_HORIZONTAL);   \
OP(arr_name, OPCODE_MULTI_TOUCHMOVE);            \
OP(arr_name, OPCODE_MULTI_TOUCHMOVE_HORIZONTAL); \
OP(arr_name, OPCODE_NEXT_SCRIPT);                \
OP(arr_name, OPCODE_HOME_DOWN);                  \
OP(arr_name, OPCODE_HOME_UP);                    \
OP(arr_name, OPCODE_CALL_EXTERNAL_SCRIPT);       \
OP(arr_name, OPCODE_ELAPSED_TIME);               \
OP(arr_name, OPCODE_FPS_MEASURE_START);          \
OP(arr_name, OPCODE_FPS_MEASURE_STOP);           \
OP(arr_name, OPCODE_CONCURRENT_IF_MATCH);        \
OP(arr_name, OPCODE_IF_MATCH_TXT);               \
OP(arr_name, OPCODE_CLICK_MATCHED_TXT);          \
OP(arr_name, OPCODE_BATCH_PROCESSING);           \
OP(arr_name, OPCODE_IF_MATCH_REGEX);             \
OP(arr_name, OPCODE_CLICK_MATCHED_REGEX);        \
OP(arr_name, OPCODE_AUDIO_RECORD_START);         \
OP(arr_name, OPCODE_AUDIO_RECORD_STOP);          \
OP(arr_name, OPCODE_IF_MATCH_AUDIO);             \
OP(arr_name, OPCODE_AI_BUILTIN);                 \
OP(arr_name, OPCODE_IMAGE_MATCHED_TIME);         \
OP(arr_name, OPCODE_FLICK_ON);                   \
OP(arr_name, OPCODE_FLICK_OFF);                  \
OP(arr_name, OPCODE_AUDIO_PLAY);                 \
OP(arr_name, OPCODE_IF_MATCH_IMAGE_WAIT);        \
OP(arr_name, OPCODE_CLICK_MATCHED_IMAGE_XY);     \
OP(arr_name, OPCODE_CLEAR_DATA);                 \
OP(arr_name, OPCODE_TOUCHDOWN_MATCHED_IMAGE_XY); \
OP(arr_name, OPCODE_TOUCHUP_MATCHED_IMAGE_XY);   \
OP(arr_name, OPCODE_TOUCHMOVE_MATCHED_IMAGE_XY); \
OP(arr_name, OPCODE_MOUSE_EVENT);                \
OP(arr_name, OPCODE_KEYBOARD_EVENT);             \
OP(arr_name, OPCODE_IF_SOURCE_IMAGE_MATCH);      \
OP(arr_name, OPCODE_IF_SOURCE_LAYOUT_MATCH);     \
OP(arr_name, OPCODE_CRASH_ON);                   \
OP(arr_name, OPCODE_CRASH_OFF);                  \
OP(arr_name, OPCODE_SET_VARIABLE);               \
OP(arr_name, OPCODE_LOOP);                       \
OP(arr_name, OPCODE_STOP_APP);                   \
OP(arr_name, OPCODE_SET_TEXT);                   \
OP(arr_name, OPCODE_EXPLORATORY_TEST);           \
OP(arr_name, OPCODE_POWER);                      \
OP(arr_name, OPCODE_LIGHT_UP);                   \
OP(arr_name, OPCODE_VOLUME_UP);                  \
OP(arr_name, OPCODE_VOLUME_DOWN);                \
OP(arr_name, OPCODE_DEVICE_POWER);               \
OP(arr_name, OPCODE_AUDIO_MATCH_ON);             \
OP(arr_name, OPCODE_AUDIO_MATCH_OFF);            \
OP(arr_name, OPCODE_HOLDER_UP);                  \
OP(arr_name, OPCODE_HOLDER_DOWN);                \
OP(arr_name, OPCODE_BUTTON);                     \
OP(arr_name, OPCODE_USB_IN);                     \
OP(arr_name, OPCODE_USB_OUT)

struct Resolution {
    uint32_t width;
    uint32_t height;
    Resolution() {
        width  = 0;
        height = 0;
    }
};

struct TouchResolution {
    uint32_t x_min;
    uint32_t x_max;
    uint32_t y_min;
    uint32_t y_max;
    TouchResolution() {
        x_min = 0;
        x_max = 0;
        y_min = 0;
        y_max = 0;
    }
};

struct Archive {
    int media;
    int qs;
    int qts;
    int trace_in;
    int trace_out;
};

}
#endif

#include <linux/input.h>
#include <utils/Thread.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <utils/String8.h>
#include <utils/Mutex.h>

#include "OpcodeConstant.h"
#include "Configuration.h"
#include "VirtualKeys.h"
#include "com/DumpsysWrapper.h"
#include "com/ViewHierarchyParser.h"
#include "com/WindowBarParser.h"
#include "utils/Logger.h"

namespace android {

class EvtFrameRecorder : public Thread {
public:
    EvtFrameRecorder();
    virtual ~EvtFrameRecorder();

    bool Initialize(int qs, int qts, int trace_out, TouchResolution& touch_resolution);
    void Finalize();

    void InitQsHeader(String8 app, String8 package, String8 activity, uint32_t width, uint32_t height);
    void InitQtsHeader(uint32_t version);

    void RecordRawTrace(struct input_event event);
    bool ParseAndRecordEvt(struct input_event event, DisplayInfo& info);
    bool RecordFrameInfo(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset);

    void AddEvent(struct input_event event);
    void SetBtnEvt(bool support_btn);

private:
    virtual bool threadLoop();
    bool RecordKeyEvent();
    bool RecordVirtualKeyEvent();
    bool RecordTouchEvent(const DisplayInfo& info);

private:
    enum {
        PORTRAIT        = 0,
        LANDSCAPE       = 1,
        REVERSEPORTAIT  = 2,
        REVERSLANDSCAPE = 3,
        Unknown
    };

    struct Coordinate {
        uint16_t x;
        uint16_t y;
        Coordinate() {
            x = 0;
            y = 0;
        }
    };

    struct Touch {
        enum {
            UP,
            DOWN,
            MOVE,
        } action;
        struct Coordinate coordinate;       // convert touch point to display point
        struct Coordinate coordinate_touch; // original touch point
        Touch() {
            action = MOVE;
        }
    } m_touches[10];

    struct Record {
        uint64_t timestamp;
        int32_t  opcode;
        int32_t  operand1;
        int32_t  operand2;
        int32_t  operand3;
        String8  operand4;
        Record() {
            timestamp = 0;
            opcode    = 0;
            operand1  = 0;
            operand2  = 0;
            operand3  = 0;
            operand4  = String8("");
        }
    } m_record;

    struct History {
        uint64_t no;
        uint64_t item;
        uint64_t frame;

        History() {
            no    = 0xffffff;
            item  = 1;
            frame = 0;
        }
    } m_history;

    Vector<struct input_event> m_events;
    Mutex                      m_events_op_mutex;
    sp<IBinder>                m_display;
    int                        m_qs;
    int                        m_qts;
    int                        m_trace_out;
    uint64_t                   m_record_start_ts;
    uint8_t                    m_slot; // touch slot;
    const char*                m_operation[OPCODE_COUNT];
    uint64_t                   m_baseline;
    TouchResolution            m_tsres; // touch screen resolution;

    String8                    m_qs_package;
    HSCConf*                   m_hsc_conf;
    sp<DumpsysWrapper>         m_dumper;
    sp<ViewHierarchyParser>    m_vh_parser;
    int                        m_tree_id;
    sp<WindowBarParser>        m_wb_parser;
    bool                       m_support_btn;
    CVirtualKeys               m_virtual_keys;
private:
    void RecordEvt(struct Record& record);
    String8 FindEvtId(DisplayInfo info, UiRect fw, uint16_t& x, uint16_t& y, UiRect& idRect, int& groupIndex, int& itemIndex);
    void FindWindowBarInfo(UiRect& sbRect, UiRect& nbRect, int& hasKb, UiRect& fwRect, int& isPopUp);
};

};


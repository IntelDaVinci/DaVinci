#include "capture/EvtFrameRecorder.h"

#include <ui/DisplayInfo.h>

#include "utils/ConvUtils.h"
#include "utils/Logger.h"

namespace android {

EvtFrameRecorder::EvtFrameRecorder() : m_qs(-1),
                                       m_qts(-1),
                                       m_trace_out(-1),
                                       m_record_start_ts(0),
                                       m_slot(0),
                                       m_baseline(0),
                                       m_dumper(new DumpsysWrapper()),
                                       m_vh_parser(new ViewHierarchyParser()),
                                       m_tree_id(0),
                                       m_wb_parser(new WindowBarParser()),
                                       m_support_btn(false)
{
    AUTO_LOG();
    DELARE_OPCODE_NAMES(m_operation);

    m_virtual_keys.LoadVirtualKeyMapFile();
}

EvtFrameRecorder::~EvtFrameRecorder()
{
    AUTO_LOG();
}

bool EvtFrameRecorder::Initialize(int qs, int qts, int trace_out, TouchResolution& touch_resolution)
{
    AUTO_LOG();

    m_qs        = qs;
    m_qts       = qts;
    m_trace_out = trace_out;
    m_tsres     = touch_resolution;

    // Initialize QTS file header
    InitQtsHeader(1);

    // Get main display for get screen rotation when capturing touch event
    m_display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);

    m_hsc_conf = Configuration::getInstance().Get();

    m_record_start_ts = GetCurrentTimeStamp();

    // TODO: we need return value to check reporter status
    // Start report event thread
    status_t err  = run("ReportEvent");
    if (err != NO_ERROR) {
        DWLOGE("Cannot start display wings manager !!! err(%d)", err);
        return false;
    } else {
        DWLOGI("Succeeded in starting report event thread");
        return true;
    }
}

void EvtFrameRecorder::InitQsHeader(String8 app, String8 package, String8 activity, uint32_t width, uint32_t height)
{
    AUTO_LOG();
    m_qs_package = package;

    char buffer[1024] = {0};
    int len = sprintf(buffer,
                      "[Configuration]\n"
                      "CapturedImageHeight=%u\n"
                      "CapturedImageWidth=%u\n"
                      "CameraPresetting=HIGH_RES\n"
                      "PackageName=%s\n"
                      "ActivityName=%s\n"
                      "APKName=%s.apk\n"
                      "PushData=\n"
                      "StartOrientation=\n"
                      "CameraType=Cameraless\n"
                      "VideoRecording=True\n"
                      "RecordedVideoType=H264\n"
                      "[Events and Actions]\n",
                      height,
                      width,
                      package.string(),
                      activity.string(),
                      package.string());
    DWLOGD("Recording fro app: %s, package name: %s, activity name: %s",
           app.string(),
           package.string(),
           activity.string());
    write(m_qs, buffer, len);

    memset(buffer, 0, sizeof(buffer));
    len = sprintf(buffer, "%llu", m_history.item++);
    len += sprintf(buffer+len, "%*.0f: %s\n", 13-len, 0.0, "OPCODE_UNINSTALL_APP");
    write(m_qs, buffer, len);

    memset(buffer, 0, sizeof(buffer));
    len = sprintf(buffer, "%llu", m_history.item++);
    len += sprintf(buffer+len, "%*.0f: %s\n", 13-len, 0.0, "OPCODE_INSTALL_APP");
    write(m_qs, buffer, len);

    memset(buffer, 0, sizeof(buffer));
    len = sprintf(buffer, "%llu", m_history.item++);
    len += sprintf(buffer+len, "%*.0f: %s\n", 13-len, 0.0, "OPCODE_START_APP");
    write(m_qs, buffer, len);
}

void EvtFrameRecorder::InitQtsHeader(uint32_t version)
{
    AUTO_LOG();

    char buffer[64] = {0};
    int len = sprintf(buffer, "Version:%u\n", version);
    write(m_qts, buffer, len);
}

void EvtFrameRecorder::Finalize()
{
    AUTO_LOG();

    // Stop monitor thread
    status_t err = NO_ERROR;
    err = requestExitAndWait();
    if (err != NO_ERROR) {
        DWLOGE("Cannot stop finalize !!! err(%d)", err);
    }

    char buffer[64] = {0};
    int len = sprintf(buffer, "%llu", m_history.item++);
    len += sprintf(buffer+len, "%*llu: %s\n", 13-len, (GetCurrentTimeStamp() - m_baseline)/1000, "OPCODE_STOP_APP");
    write(m_qs, buffer, len);

    len = sprintf(buffer, "%lld", m_history.item++);
    len += sprintf(buffer+len, "%*llu: %s\n", 13-len, (GetCurrentTimeStamp() - m_baseline)/1000, "OPCODE_UNINSTALL_APP");
    write(m_qs, buffer, len);

    len = sprintf(buffer, "%lld", m_history.item++);
    len += sprintf(buffer+len, "%*llu: %s\n", 13-len, (GetCurrentTimeStamp() - m_baseline)/1000, "OPCODE_EXIT");
    write(m_qs, buffer, len);
}

void EvtFrameRecorder::AddEvent(struct input_event event)
{
    AUTO_LOG();
    m_events_op_mutex.lock();
    m_events.push(event);
    m_events_op_mutex.unlock();
}

void EvtFrameRecorder::SetBtnEvt(bool support_btn)
{
    AUTO_LOG();
    m_support_btn = support_btn;
}

bool EvtFrameRecorder::threadLoop()
{
    struct input_event first_event;

    m_events_op_mutex.lock();
    if (m_events.isEmpty()) {
        m_events_op_mutex.unlock();
        return true;
    } else {
        first_event = m_events[0];
        m_events.removeAt(0);;
    }
    m_events_op_mutex.unlock();

    DisplayInfo info;
    int res = SurfaceComposerClient::getDisplayInfo(m_display, &info);
    if (res != NO_ERROR) {
        DWLOGE("Cannot query display information !!! err(%d)", res);
        return false;
    }
    RecordRawTrace(first_event);
    ParseAndRecordEvt(first_event, info);

    return true;
}

bool EvtFrameRecorder::RecordKeyEvent()
{
    switch (m_record.opcode)
    {
    case OPCODE_POWER:
    case OPCODE_VOLUME_DOWN:
    case OPCODE_VOLUME_UP:
    case OPCODE_BACK:
    case OPCODE_HOME:
    case OPCODE_MENU:
        m_record.timestamp = GetCurrentTimeStamp();
        RecordEvt(m_record);
        return true;
    default:
        return false;
    }
}

bool EvtFrameRecorder::RecordVirtualKeyEvent()
{
    VirtualKeysType ret = m_virtual_keys.IdentifyVirtualKey(
        m_touches[m_slot].coordinate_touch.x,
        m_touches[m_slot].coordinate_touch.y
        );
    if (ret == INVALID_KEY)
        return false;

    int org_opcode = m_record.opcode;

    m_record.timestamp = GetCurrentTimeStamp();

    if (ret == BACK_KEY) {
        m_record.opcode = OPCODE_BACK;
    } else if (ret == MENU_KEY) {
        m_record.opcode = OPCODE_MENU;
    } else if (ret == HOME_KEY) {
        m_record.opcode = OPCODE_HOME;
    }

    bool recorded = true;
    if (org_opcode == OPCODE_TOUCHDOWN) {
        m_record.operand1 = 1;
        m_record.operand2 = -1;
        // Record current record
        RecordEvt(m_record);
    } else if (org_opcode == OPCODE_TOUCHMOVE) {
        // Ignore touch move event if it is virtual keys
    } else if (org_opcode == OPCODE_TOUCHUP) {
        m_record.operand1 = 0;
        m_record.operand2 = -1;
        // Record current record
        RecordEvt(m_record);
    } else {
        recorded = false;
    }

    // Assume next default action is touch move, since last event is touch down/up
    m_touches[m_slot].action = Touch::MOVE;
    m_record.opcode = OPCODE_TOUCHMOVE;

    return recorded;
}

bool EvtFrameRecorder::RecordTouchEvent(const DisplayInfo& info)
{
    // Check touch point whether is out of touch resolution. For more details,
    // Please refer to https://source.android.com/devices/input/touch-devices.html
    unsigned int x = m_touches[m_slot].coordinate.x;
    unsigned int y = m_touches[m_slot].coordinate.y;
    if ((x < m_tsres.x_min || x > m_tsres.x_max) ||
        (y < m_tsres.y_min || y > m_tsres.y_max)) {
        DWLOGE("The touch event is out of touch resolutio.");
        DWLOGE("(%d, %d) is out of [(%d, %d); (%d, %d)].",
               x, y,
               m_tsres.x_min, m_tsres.y_min,
               m_tsres.x_max, m_tsres.y_max);
        return false;
    }

    // Calculate coordinate according to display orientation
    switch (info.orientation) {
    case 0:
        m_record.operand1 = (m_touches[m_slot].coordinate.x << 12) / info.w;
        m_record.operand2 = (m_touches[m_slot].coordinate.y << 12) / info.h;
        break;
    case 1:
        m_record.operand1 = (m_touches[m_slot].coordinate.y << 12) / info.h;
        m_record.operand2 = ((info.w - m_touches[m_slot].coordinate.x) << 12) / info.w;
        break;
    case 2:
        m_record.operand1 = ((info.w - m_touches[m_slot].coordinate.x) << 12) / info.w;
        m_record.operand2 = ((info.h - m_touches[m_slot].coordinate.y) << 12) / info.h;
        break;
    case 3:
        m_record.operand1 = ((info.h - m_touches[m_slot].coordinate.y) << 12) / info.h;
        m_record.operand2 = (m_touches[m_slot].coordinate.x << 12) / info.w;
        break;
    default:
        break;
    }
    m_record.operand3 = info.orientation;
    m_record.timestamp = GetCurrentTimeStamp();

    // Get android UI item id
    if (m_hsc_conf->id_record)
    {
        if(m_record.opcode == OPCODE_TOUCHDOWN)
        {
            ++m_tree_id;
            UiRect sbRect, nbRect, fwRect;
            int hasKb, isPopUp;

            FindWindowBarInfo(sbRect, nbRect, hasKb, fwRect, isPopUp);
            UiRect idRect;
            int groupIndex, itemIndex;
            uint16_t tmp_x = m_touches[m_slot].coordinate.x;
            uint16_t tmp_y = m_touches[m_slot].coordinate.y;

            String8 id_value = FindEvtId(info,
                                         fwRect,
                                         tmp_x,
                                         tmp_y,
                                         idRect,
                                         groupIndex,
                                         itemIndex
                                         );

            char buffer[512] = {0};
            int len = 0;
            if(id_value != "")
            {
                len = sprintf(buffer,
                              "id=%s(%d-%d)&ratio=(%.4f,%.4f)&layout=record_%s.qs_%d.tree&sb=(%d,%d,%d,%d)&nb=(%d,%d,%d,%d)&kb=%d&fw=(%d,%d,%d,%d)&popup=%d",
                              id_value.string(),
                              groupIndex,
                              itemIndex,
                              double((tmp_x - idRect.x)) / idRect.width,
                              double((tmp_y - idRect.y)) / idRect.height,
                              m_qs_package.string(),
                              m_tree_id,
                              sbRect.x,
                              sbRect.y,
                              sbRect.x + sbRect.width,
                              sbRect.y + sbRect.height,
                              nbRect.x,
                              nbRect.y,
                              nbRect.x + nbRect.width,
                              nbRect.y + nbRect.height,
                              hasKb,
                              fwRect.x,
                              fwRect.y,
                              fwRect.x + fwRect.width,
                              fwRect.y + fwRect.height,
                              isPopUp
                              );
                assert(len > 0);
                m_record.operand4.append(buffer);
            }
            else
            {
                len = sprintf(buffer,
                              "layout=record_%s.qs_%d.tree&sb=(%d,%d,%d,%d)&nb=(%d,%d,%d,%d)&kb=%d&fw=(%d,%d,%d,%d)&popup=%d",
                              m_qs_package.string(),
                              m_tree_id,
                              sbRect.x,
                              sbRect.y,
                              sbRect.x + sbRect.width,
                              sbRect.y + sbRect.height,
                              nbRect.x,
                              nbRect.y,
                              nbRect.x + nbRect.width,
                              nbRect.y + nbRect.height,
                              hasKb,
                              fwRect.x,
                              fwRect.y,
                              fwRect.x + fwRect.width,
                              fwRect.y + fwRect.height,
                              isPopUp
                              );
                assert(len > 0);
                m_record.operand4.append(buffer);
            }
        }
    }

    // Record current record
    RecordEvt(m_record);

    // Assume next default action is touch move, since last event is touch down/up
    m_touches[m_slot].action = Touch::MOVE;
    m_record.opcode = OPCODE_TOUCHMOVE;
    return true;
}

void EvtFrameRecorder::RecordRawTrace(struct input_event event)
{
    if (m_trace_out != -1)
    {
        uint64_t elapsed = GetCurrentTimeStamp() - m_record_start_ts;
        event.time = TimeStampToTimeval(elapsed);
        write(m_trace_out, &event, sizeof(event));
    }
}

bool EvtFrameRecorder::ParseAndRecordEvt(struct input_event event, DisplayInfo& info)
{
    if (event.type == EV_KEY) {
        switch (event.code) {
        case KEY_VOLUMEDOWN:
            DWLOGD("Captured KEY_VOLUMEDOWN event(%d).", event.value);
            m_record.opcode   = OPCODE_VOLUME_DOWN;
            m_record.operand1 = event.value;
            break;
        case KEY_VOLUMEUP:
            DWLOGD("Captured KEY_VOLUMEUP event(%d).", event.value);
            m_record.opcode   = OPCODE_VOLUME_UP;
            m_record.operand1 = event.value;
            break;
        case KEY_BACK:
            DWLOGD("Captured KEY_BACK event(%d).", event.value);
            m_record.opcode   = OPCODE_BACK;
            m_record.operand1 = event.value;
            break;
        case KEY_MENU:
            DWLOGD("Captured KEY_MENU event(%d).", event.value);
            m_record.opcode   = OPCODE_MENU;
            m_record.operand1 = event.value;
            break;
        case KEY_HOMEPAGE:
            DWLOGD("Captured KEY_HOMEPAGE event(%d).", event.value);
            m_record.opcode   = OPCODE_HOME;
            m_record.operand1 = event.value;
            break;
        case KEY_POWER:
            DWLOGD("Captured KEY_POWER event(%d).", event.value);
            m_record.opcode   = OPCODE_POWER;
            m_record.operand1 = event.value;
            break;
        case BTN_TOUCH:
            m_support_btn = true;
            if (event.value == 0) {
                m_touches[m_slot].action = Touch::UP;
                m_record.opcode = OPCODE_TOUCHUP;
                DWLOGD("BTN_TOUCH Up : OPCODE_TOUCHUP.");
            } else {
                m_touches[m_slot].action = Touch::DOWN;
                m_record.opcode = OPCODE_TOUCHDOWN;
                DWLOGD("BTN_TOUCH Down: OPCODE_TOUCHDOWN");
            }
            break;
        default:
            DWLOGW("EV_KEY(code %u, value %d) ignored!", event.code, event.value);
            break;
        }
    } if (event.type == EV_ABS) {
        switch(event.code) {
        case ABS_MT_SLOT:
            m_slot = event.value;
            // TODO: If we need support multi touch, we just need add MT_SLOT opcode
            // TODO: And we need output record
            DWLOGE("Doesn't support multi touch now!!!");
            DWLOGD("ABS_MT_SLOT: %d", event.value);
            break;
        case ABS_MT_POSITION_X:
            m_touches[m_slot].coordinate.x = (event.value - m_tsres.x_min) * info.w / (m_tsres.x_max - m_tsres.x_min + 1);
            m_touches[m_slot].coordinate_touch.x = event.value;

            DWLOGD("TOUCH RES X_MIN: %d", m_tsres.x_min);
            DWLOGD("TOUCH RES X_MAX: %d", m_tsres.x_max);
            DWLOGD("ABS_DISPLAY_H: %d", info.h);
            DWLOGD("ABS_MT_POSITION_X TOUCH: %d", event.value);
            DWLOGD("ABS_MT_POSITION_X DISPLAY: %d", m_touches[m_slot].coordinate.x);
            DWLOGD("ABS_DISPLAY_W: %d", info.w);
            DWLOGD("ABS_DISPLAY_H: %d", info.h);
            break;
        case ABS_MT_POSITION_Y:
            m_touches[m_slot].coordinate.y = (event.value - m_tsres.y_min) * info.h / (m_tsres.y_max - m_tsres.y_min + 1);
            m_touches[m_slot].coordinate_touch.y = event.value;

            DWLOGD("TOUCH RES Y_MIN: %d", m_tsres.y_min);
            DWLOGD("TOUCH RES Y_MAX: %d", m_tsres.y_max);
            DWLOGD("ABS_MT_POSITION_Y TOUCH: %d", event.value);
            DWLOGD("ABS_MT_POSITION_Y DISPLAY: %d", m_touches[m_slot].coordinate.y);
            DWLOGD("ABS_DISPLAY_W: %d", info.w);
            DWLOGD("ABS_DISPLAY_H: %d", info.h);
            break;
        case ABS_MT_TRACKING_ID:
            if (!m_support_btn) {
                if (event.value == -1) {
                    m_touches[m_slot].action = Touch::UP;
                    m_record.opcode = OPCODE_TOUCHUP;
                    DWLOGD("ABS_MT_TRACKING_ID: OPCODE_TOUCHUP");
                } else {
                    m_touches[m_slot].action = Touch::DOWN;
                    m_record.opcode = OPCODE_TOUCHDOWN;
                    DWLOGD("ABS_MT_TRACKING_ID: OPCODE_TOUCHDOWN");
                }
            }
            break;
        default:
            DWLOGW("EV_ABS(code %u, value %d) ignored!", event.code, event.value);
            break;
        }
    } else if (event.type == EV_SYN) {
        switch(event.code) {
        case SYN_REPORT:
            {
                bool ret = RecordKeyEvent();
                if (!ret)
                    ret = RecordVirtualKeyEvent();
                if (!ret)
                    ret = RecordTouchEvent(info);
                if (!ret)
                    DWLOGE("Invalid record:(OPCODE: %d)", m_record.opcode);
            }
            break;
        default:
            DWLOGW("EV_SYN(code %u, value %d) ignored!", event.code, event.value);
            break;
        }
    } else {
        DWLOGW("(type %u, code %u, value %d) ignored!", event.type, event.code, event.value);
    }
    return true;
}

bool EvtFrameRecorder::RecordFrameInfo(uint64_t frame_num, uint64_t frame_type, uint64_t frame_offset)
{
    char     buffer[256] = {0};
    uint64_t timestamp = 0;
    if (m_baseline == 0) {
        m_baseline = GetCurrentTimeStamp();
    } else {
        timestamp = GetCurrentTimeStamp() - m_baseline;
    }
    if (m_history.no != frame_num) {
        m_history.no = frame_num;
        int len = sprintf(buffer, "%llu", timestamp/1000);
        len += sprintf(buffer+len, "%*llu%*llu%*llu\n", 16-len, m_history.frame++, 4, frame_type, 12, frame_offset);
        write(m_qts, buffer, len);
    }
    return true;
}

void EvtFrameRecorder::FindWindowBarInfo(UiRect& sbRect, UiRect& nbRect, int& hasKb, UiRect& fwRect, int& isPopUp)
{
    char     buffer[256] = {0};
    int len = sprintf(buffer, "/data/local/tmp/%s/record_%s.qs_%d.window", m_qs_package.string(), m_qs_package.string(), m_tree_id);
    assert(len > 0);
    if(!m_dumper->InitializeRegion())
        return;

    m_dumper->DumpWindowBar();
    m_wb_parser->ParseInfo(m_dumper->GetRegion(), sbRect, nbRect, hasKb, fwRect, isPopUp, String8(buffer));
    m_dumper->ReleaseRegion();
}


String8 EvtFrameRecorder::FindEvtId(DisplayInfo info, UiRect fw, uint16_t& x, uint16_t& y, UiRect& idRect, int& groupIndex, int& itemIndex)
{
    char     buffer[256] = {0};
    int len = sprintf(buffer, "/data/local/tmp/%s/record_%s.qs_%d.tree", m_qs_package.string(), m_qs_package.string(), m_tree_id);
    assert(len > 0);
    if(!m_dumper->InitializeRegion())
        return String8("");

    m_dumper->DumpViewHierarchy();

    struct ViewHierarchyTree* root = m_vh_parser->ParseTree(m_dumper->GetRegion(), String8(buffer));

    DWLOGW("TOUCHDOWN on touch screen at %u %u %d", x, y, info.orientation);
    uint16_t tmp_x = x, tmp_y = y;

    switch(info.orientation)
    {
    case 0:
        x = tmp_x;
        y = tmp_y;
        break;
    case 1:
        x = tmp_y;
        y = (uint16_t)(info.w - tmp_x);
        break;
    case 2:
        x = (uint16_t)(info.w - tmp_x);
        y = (uint16_t)(info.h - tmp_y);
        break;
    case 3:
        x = (uint16_t)(info.h - tmp_y);
        y = tmp_x;
        break;
    default:
        x = tmp_x;
        y = tmp_y;
        break;
    }

    String8 id = m_vh_parser->FindMatchedIdByPoint(x, y, fw, idRect, groupIndex, itemIndex);
    m_vh_parser->ReleaseTree(root);
    m_vh_parser->ReleaseControls();
    m_dumper->ReleaseRegion();

    DWLOGW("TOUCHDOWN on device screen at %u %u with %s", x, y, id.string());
    return id;
}

void EvtFrameRecorder::RecordEvt(Record& record)
{
    char     buffer[256]  = {0};
    uint64_t record_timestamp = record.timestamp;

    if (m_baseline == 0) {
        m_baseline = record_timestamp;
        record.timestamp = 0;
    } else {
        record.timestamp = record_timestamp - m_baseline;
    }

    if (record.timestamp <= 0) {
        DWLOGE("Invalid OPCODE timestamp.(%llu - %llu)", record_timestamp , m_baseline);
    }

    int len = sprintf(buffer, "%llu", m_history.item++);
    len += sprintf(buffer+len,
                   "%*llu: %s %u",
                   13-len,
                   record.timestamp/1000,
                   m_operation[record.opcode],
                   record.operand1);
    if (record.operand2 >= 0) {
        len += sprintf(buffer + len, " %u", record.operand2);
        if (record.operand3 >= 0) {
            len += sprintf(buffer + len, " %u", record.operand3);
        }
    }

    if(m_hsc_conf->id_record && record.operand4 != "")
    {
        len += sprintf(buffer + len, " %s", record.operand4.string());
        record.operand4 = String8("");
    }
    len += sprintf(buffer + len, "\n");
    write(m_qs, buffer, len);
    DWLOGW("%s", buffer);
}

}


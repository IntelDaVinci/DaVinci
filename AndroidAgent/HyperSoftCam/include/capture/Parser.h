#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>
#include <unistd.h>
#include <utils/RefBase.h>

#include "utils/Logger.h"

#define MSDK_ZERO_MEMORY(VAR)         \
    {                                 \
        memset(&VAR, 0, sizeof(VAR)); \
    }

namespace android {

class Observer : virtual public RefBase
{
public:
    virtual ~Observer() {}
    virtual void OnNewFrame(uint64_t frame_num,
                            uint64_t frame_type,
                            uint64_t frame_offset
                            )  = 0;
    virtual void OnFrameSize(uint32_t width,
                             uint32_t height
                             ) = 0;
};

class Parser
{
public:
    Parser(sp<Observer> observer) : m_observer(observer)
    {
        m_iValue                        = 0;
        m_nBitsInValue                  = 0;
        m_nSlicePrefix                  = 0;
        m_nLeadingZeroBits              = 0;
        m_nEmulationPreventionThreeByte = 0;
        m_nAccumLength                  = 0;
        m_nCurSliceOffset               = 0;
        state                           = SCAN_NAL_FOR_SLICE;
        m_bCountLeadingZeroBits         = true;
        MSDK_ZERO_MEMORY(h264info);
        h264info.frame_num              = 0xffffffffffffffff;
    }

    virtual ~Parser()
    {
    }

    virtual inline void Input(uint8_t* data, uint32_t length)
    {
        m_uData         = data;
        m_nDataLength   = length;

        parse();

        m_nAccumLength += length;
    }

    virtual inline int64_t value()
    {
        int64_t value                   = m_iValue;

        m_bCountLeadingZeroBits         = true;
        m_nLeadingZeroBits              = 0;
        m_nEmulationPreventionThreeByte = 0;
        m_iValue                        = 0;
        m_nBitsInValue                  = 0;
        return value;
    }

    bool u(uint8_t n);
    bool uev();
    bool sev();

public:
    void parse();

private:

    enum State
    {
        SCAN_NAL_FOR_SLICE,
        SCAN_NAL_FOR_SVC_EXTENSION_FLAG,
        SCAN_NAL_FOR_SVC_IDR_FLAG,
        SCAN_NAL_FOR_SVC_PRIORITY_ID,
        SCAN_NAL_FOR_SVC_NO_INTER_LAYER_PRED_FLAG,
        SCAN_NAL_FOR_SVC_DEPENDENCY_ID,
        SCAN_NAL_FOR_SVC_QUALITY_ID,
        SCAN_NAL_FOR_SVC_TEMPORAL_ID,
        SCAN_NAL_FOR_SVC_USE_REF_BASE_PIC_FLAG,
        SCAN_NAL_FOR_SVC_DISCARDABLE_FLAG,
        SCAN_NAL_FOR_SVC_OUTPUT_FLAG,
        SCAN_NAL_FOR_SVC_RESERVED_THREE_2BITS,
        SCAN_NAL_FOR_MVC_NON_IDR_FLAG,
        SCAN_NAL_FOR_MVC_PRIORITY_ID,
        SCAN_NAL_FOR_MVC_VIEW_ID,
        SCAN_NAL_FOR_MVC_TEMPORAL_ID,
        SCAN_NAL_FOR_MVC_ANCHOR_PIC_FLAG,
        SCAN_NAL_FOR_MVC_INTER_VIEW_FLAG,
        SCAN_NAL_FOR_MVC_RESERVED_ONE_BIT,
        SCAN_SPS_FOR_PROFILE_IDC,
        SCAN_SPS_FOR_CONSTRAINT_SET_FLAG,
        SCAN_SPS_FOR_LEVEL_IDC,
        SCAN_SPS_FOR_SEQ_PARAMETER_SET_ID,
        SCAN_SPS_FOR_CHROMA_FORMAT_IDC,
        SCAN_SPS_FOR_SEPARATE_COLOUR_PLANE_FLAG,
        SCAN_SPS_FOR_BIT_DEPTH_LUMA_MINUS8,
        SCAN_SPS_FOR_BIT_DEPTH_CHROMA_MINUS8,
        SCAN_SPS_FOR_QPPRIME_Y_ZERO_TRANSFORM_BYPASS_FLAG,
        SCAN_SPS_FOR_SEQ_SCALING_MATRIX_PRESENT_FLAG,
        SCAN_SPS_FOR_SEQ_SCALING_LIST_PRESENT_FLAG,
        SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4,
        SCAN_SPS_FOR_PIC_ORDER_CNT_TYPE,
        SCAN_SPS_FOR_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4,
        SCAN_SPS_FOR_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG,
        SCAN_SPS_FOR_OFFSET_FOR_NON_REF_PIC,
        SCAN_SPS_FOR_OFFSET_FOR_TOP_TO_BOTTOM_FIELD,
        SCAN_SPS_FOR_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_TYPE,
        SCAN_SPS_FOR_OFFSET_FOR_REF_FRAME,
        SCAN_SPS_FOR_MAX_NUM_REF_FRAMES,
        SCAN_SPS_FOR_GAPS_IN_FRAME_NUM_VALUE_ALLOWED_FLAG,
        SCAN_SPS_FOR_PIC_WIDTH_IN_MSB_MINUS1,
        SCAN_SPS_FOR_PIC_HEIGHT_IN_MAP_UNITS_MINUS1,
        SCAN_SPS_FOR_FRAME_MBS_ONLY_FLAG,
        SCAN_SPS_FOR_MB_ADAPTIVE_FRAME_FIELD_FLAG,
        SCAN_SPS_FOR_DIRECT_8X8_INFERENCE_FLAG,
        SCAN_SPS_FOR_FRAME_CROPPING_FLAG,
        SCAN_SPS_FOR_FRAME_CROP_LEFT_OFFSET,
        SCAN_SPS_FOR_FRAME_CROP_RIGHT_OFFSET,
        SCAN_SPS_FOR_FRAME_CROP_TOP_OFFSET,
        SCAN_SPS_FOR_FRAME_CROP_BOTTOM_OFFSET,
        SCAN_SPS_FOR_VUI_PARAMETERS_PRESENT_FLAG,
        SCAN_SLICE_HEADER_FOR_FIRST_MB_IN_SLICE,
        SCAN_SLICE_HEADER_FOR_SLICE_TYPE,
        SCAN_SLICE_HEADER_FOR_PIC_PARAMETER_SET_ID,
        SCAN_SLICE_HEADER_FOR_COLOUR_PLANE_ID,
        SCAN_SLICE_HEADER_FOR_FRAME_NUM,
        SCAN_NAL_ALIGN_BYTE,
    } state;

    struct
    {
        uint8_t nal_unit_type;
        bool svc_extension_flag;
        struct
        {
            bool idr_flag;
            uint8_t priority_id;
            bool no_inter_layer_pred_flag;
            uint8_t dependency_id;
            uint8_t quality_id;
            uint8_t temporal_id;
            bool use_ref_base_pic_flag;
            bool discardable_flag;
            bool output_flag;
            uint8_t reserved_three_2bits;
            bool non_idr_flag;
            uint8_t view_id;
            bool anchor_pic_flag;
            bool inter_view_flag;
            uint8_t reserved_one_bit;
        } ext;
        struct
        {
            uint8_t profile_idc;
            uint8_t constraint_set_flag;
            uint8_t level_idc;
            uint64_t seq_parameter_set_id;
            uint64_t chroma_format_idc;
            bool separate_colour_plane_flag;
            uint64_t bit_depth_luma_minu8;
            uint64_t bit_depth_chroma_minus8;
            bool qpprime_y_zero_transform_bypass_flag;
            bool seq_scaling_matrix_present_flag;
            uint32_t seq_scaling_list_present_flag;
            uint64_t log2_max_frame_num_minus4;
            uint64_t pic_order_cnt_type;
            uint64_t log2_max_pic_order_cnt_lsb_minus4;
            bool delta_pic_order_always_zero_flag;
            int64_t offset_for_non_ref_pic;
            int64_t offset_for_top_to_bottom_field;
            uint64_t num_ref_frames_in_pic_order_cnt_cycle;
            int64_t* offset_for_ref_frame;
            uint64_t max_num_ref_frames;
            bool gaps_in_frame_num_value_allowed_flag;
            uint64_t pic_width_in_mbs_minus1;
            uint64_t pic_height_in_map_units_minus1;
            uint8_t frame_mbs_only_flag;
            bool mb_adaptive_frame_field_flag;
            bool direct_8x8_inference_flag;
            bool frame_croppiing_flag;
            uint64_t frame_crop_left_offset;
            uint64_t frame_crop_right_offset;
            uint64_t frame_crop_top_offset;
            uint64_t frame_crop_bottom_offset;
            bool vui_parameters_present_flag;
        } sps;
        uint64_t first_mb_in_slice;
        uint64_t slice_type;
        uint64_t pic_parameter_set_id;
        uint32_t colour_plane_id;
        uint64_t frame_num;
        bool field_pic_flag;
        bool bottom_field_flag;
        uint64_t idr_pic_id;
        uint64_t pic_order_cnt_lsb;
        int64_t delta_pic_order_cnt_bottom;
    } h264info;

    uint8_t*                  m_uData;
    uint32_t                  m_nDataLength;
    sp<Observer>              m_observer;
    uint64_t                  m_nOffsetInStream;  // first unread bit position

    int64_t                   m_iValue;
    uint8_t                   m_nBitsInValue;     // first unwrite bit position
    // for slice prefix
    uint8_t                   m_nSlicePrefix;
    bool                      m_bCountLeadingZeroBits;
    uint8_t                   m_nLeadingZeroBits;
    uint8_t                   m_nEmulationPreventionThreeByte;
    uint64_t                  m_nArraryIndex;

    uint64_t                  m_nAccumLength;
    int64_t                   m_nCurSliceOffset;
};

}
#endif

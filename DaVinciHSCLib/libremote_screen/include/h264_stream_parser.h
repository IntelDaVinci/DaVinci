#ifndef H264_STREAM_PARSER_H
#define H264_STREAM_PARSER_H

#include <mfxcommon.h>
#include "video_stream_parser.h"
#include "logger.h"

class CH264VideoParser :
    public CVideoParser
{
public:
    CH264VideoParser(CVideoParserObserver* observer, mfxU8 phase) : CVideoParser(observer, phase) { state = SCAN_NAL_FOR_SLICE; m_nSlicePrefix = 0; m_bCountLeadingZeroBits = true; m_nLeadingZeroBits = 0; m_nEmulationPreventionThreeByte = 0; MSDK_ZERO_MEMORY(h264info); }
    virtual ~CH264VideoParser() {}
    virtual inline mfxI64 value() { m_bCountLeadingZeroBits = true; m_nLeadingZeroBits = 0; m_nEmulationPreventionThreeByte = 0; return CVideoParser::value(); }

    bool u(mfxU8 n);
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
        SCAN_SLICE_HEADER_FOR_FIELD_PIC_FLAG,
        SCAN_SLICE_HEADER_FOR_BOTTOM_FIELD_FLAG,
        SCAN_SLICE_HEADER_FOR_IDR_PIC_ID,
        SCAN_SLICE_HEADER_FOR_PIC_ORDER_CNT_LSB,
        SCAN_SLICE_HEADER_FOR_DELTA_PIC_ORDER_CNT_BOTTOM,
        SCAN_NAL_ALIGN_BYTE,
    } state;

    struct
    {
        mfxU8 nal_unit_type;
        bool svc_extension_flag;
        struct
        {
            bool idr_flag;
            mfxU8 priority_id;
            bool no_inter_layer_pred_flag;
            mfxU8 dependency_id;
            mfxU8 quality_id;
            mfxU8 temporal_id;
            bool use_ref_base_pic_flag;
            bool discardable_flag;
            bool output_flag;
            mfxU8 reserved_three_2bits;
            bool non_idr_flag;
            mfxU8 view_id;
            bool anchor_pic_flag;
            bool inter_view_flag;
            mfxU8 reserved_one_bit;
        } ext;
        struct
        {
            mfxU8 profile_idc;
            mfxU8 constraint_set_flag;
            mfxU8 level_idc;
            mfxU64 seq_parameter_set_id;
            mfxU64 chroma_format_idc;
            bool separate_colour_plane_flag;
            mfxU64 bit_depth_luma_minu8;
            mfxU64 bit_depth_chroma_minus8;
            bool qpprime_y_zero_transform_bypass_flag;
            bool seq_scaling_matrix_present_flag;
            mfxU32 seq_scaling_list_present_flag;
            mfxU64 log2_max_frame_num_minus4;
            mfxU64 pic_order_cnt_type;
            mfxU64 log2_max_pic_order_cnt_lsb_minus4;
            bool delta_pic_order_always_zero_flag;
            mfxI64 offset_for_non_ref_pic;
            mfxI64 offset_for_top_to_bottom_field;
            mfxU64 num_ref_frames_in_pic_order_cnt_cycle;
            mfxI64* offset_for_ref_frame;
            mfxU64 max_num_ref_frames;
            bool gaps_in_frame_num_value_allowed_flag;
            mfxU64 pic_width_in_mbs_minus1;
            mfxU64 pic_height_in_map_units_minus1;
            mfxU8 frame_mbs_only_flag;
            bool mb_adaptive_frame_field_flag;
            bool direct_8x8_inference_flag;
            bool frame_croppiing_flag;
            mfxU64 frame_crop_left_offset;
            mfxU64 frame_crop_right_offset;
            mfxU64 frame_crop_top_offset;
            mfxU64 frame_crop_bottom_offset;
            bool vui_parameters_present_flag;
        } sps;
        mfxU64 first_mb_in_slice;
        mfxU64 slice_type;
        mfxU64 pic_parameter_set_id;
        mfxU32 colour_plane_id;
        mfxU64 frame_num;
        bool field_pic_flag;
        bool bottom_field_flag;
        mfxU64 idr_pic_id;
        mfxU64 pic_order_cnt_lsb;
        mfxI64 delta_pic_order_cnt_bottom;
    } h264info;

    // for slice prefix
    mfxU8                   m_nSlicePrefix;
    bool                    m_bCountLeadingZeroBits;
    mfxU8                   m_nLeadingZeroBits;
    mfxU8                   m_nEmulationPreventionThreeByte;
    mfxU64                  m_nArraryIndex;
};

#endif
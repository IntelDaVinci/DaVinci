#include "capture/Parser.h"

namespace android {

bool Parser::uev()
{
    if (m_bCountLeadingZeroBits)
    {
        if (u(1))
        {
            if (m_iValue)
            {
                m_bCountLeadingZeroBits = false;
            }
            else
            {
                m_nLeadingZeroBits++;
            }
            m_iValue = 0;
            m_nBitsInValue = 0;
        }
    }
    else
    {
        if (u(m_nLeadingZeroBits))
        {
            m_iValue = (1 << m_nLeadingZeroBits) - 1 + value();
//            m_iValue = (1 << m_nLeadingZeroBits) - 1 + m_iValue;
//            m_bCountLeadingZeroBits = true; m_nLeadingZeroBits = 0; m_nEmulationPreventionThreeByte = 0;  m_nBitsInValue = 0;
            return true;
        }
    }
    return false;
}

bool Parser::sev()
{
    if (uev())
    {
        if (m_iValue & 1)
        {
            m_iValue = (m_iValue >> 1) + 1;
        }
        else
        {
            m_iValue = -(m_iValue >> 1);
        }
        return true;
    }
    return false;
}

bool Parser::u(uint8_t n)
{
    if ((n - m_nBitsInValue) <= (8 - (m_nOffsetInStream & 7)))
    {
        m_iValue = (m_iValue << (n - m_nBitsInValue)) | ((m_uData[m_nOffsetInStream >> 3] & ((1 << (8 - (m_nOffsetInStream & 7))) - 1)) >> (8 - (m_nOffsetInStream & 7) - n + m_nBitsInValue));
        m_nOffsetInStream += n - m_nBitsInValue;
        m_nBitsInValue = n;
        return true;
    }
    else
    {
        m_iValue = (m_iValue << (8 - (m_nOffsetInStream & 7))) | (m_uData[m_nOffsetInStream >> 3] & ((1 << (8 - (m_nOffsetInStream & 7))) - 1));
        m_nBitsInValue += 8 - (m_nOffsetInStream & 7);
        m_nOffsetInStream += 8 - (m_nOffsetInStream & 7);
        return false;
    }
}

void Parser::parse()
{
    m_nOffsetInStream = 0;

    while (m_nOffsetInStream < (m_nDataLength << 3))
    {
        uint64_t curByteInStream = (m_nOffsetInStream >> 3);
        if ((state != SCAN_NAL_FOR_SLICE) && ((m_nOffsetInStream & 7) == 0))
        {
            if (m_nEmulationPreventionThreeByte == 2)
            {
                m_nEmulationPreventionThreeByte = 0;
                if (m_uData[curByteInStream] == 3)
                {
                    m_nOffsetInStream += 8;
                    continue;
                }
            }
            else if (m_uData[curByteInStream] == 0)
            {
                m_nEmulationPreventionThreeByte++;
            }
            else
            {
                m_nEmulationPreventionThreeByte = 0;
            }
        }
        switch (state)
        {
        case SCAN_NAL_FOR_SLICE:
            if (m_nSlicePrefix < 3)
            {
                if (m_uData[curByteInStream] == 0)
                {
                    m_nSlicePrefix++;
                }
                else
                {
                    m_nSlicePrefix = 0;
                }
            }
            else if (m_nSlicePrefix == 3)
            {
                if (m_uData[curByteInStream] == 1)
                {
                    // found new slice of a frame
                    m_nCurSliceOffset = m_nAccumLength+curByteInStream-m_nSlicePrefix;
                    m_nSlicePrefix++;
                }
                else if (m_uData[curByteInStream] != 0)
                {
                    m_nSlicePrefix = 0;
                }
            }
            else
            {
                h264info.nal_unit_type = m_uData[curByteInStream] & 0x1f;
                switch (h264info.nal_unit_type)
                {
                case 7:
                    state = SCAN_SPS_FOR_PROFILE_IDC;
                    break;
                case 1:
                case 2:
                case 5:
                    state = SCAN_SLICE_HEADER_FOR_FIRST_MB_IN_SLICE;
                    break;
                case 14:
                case 20:
                    state = SCAN_NAL_FOR_SVC_EXTENSION_FLAG;
                    break;
                default:
                    break;
                }
                m_nSlicePrefix = 0;
            }
            m_nOffsetInStream += 8;
            break;
        case SCAN_NAL_FOR_SVC_EXTENSION_FLAG:
            if (u(1))
            {
                h264info.svc_extension_flag = value() ? true : false;
                if (h264info.svc_extension_flag)
                {
                    state = SCAN_NAL_FOR_SVC_IDR_FLAG;
                }
                else
                {
                    state = SCAN_NAL_FOR_MVC_NON_IDR_FLAG;
                }
            }
            break;
        case SCAN_NAL_FOR_SVC_IDR_FLAG:
            if (u(1))
            {
                h264info.ext.idr_flag = value() ? true : false;
                state = SCAN_NAL_FOR_SVC_PRIORITY_ID;
            }
            break;
        case SCAN_NAL_FOR_SVC_PRIORITY_ID:
            if (u(6))
            {
                h264info.ext.priority_id = value();
                state = SCAN_NAL_FOR_SVC_NO_INTER_LAYER_PRED_FLAG;
            }
            break;
        case SCAN_NAL_FOR_SVC_NO_INTER_LAYER_PRED_FLAG:
            if (u(1))
            {
                h264info.ext.no_inter_layer_pred_flag = value() ? true : false;
                state = SCAN_NAL_FOR_SVC_DEPENDENCY_ID;
            }
            break;
        case SCAN_NAL_FOR_SVC_DEPENDENCY_ID:
            if (u(3))
            {
                h264info.ext.dependency_id = value();
                state = SCAN_NAL_FOR_SVC_QUALITY_ID;
            }
            break;
        case SCAN_NAL_FOR_SVC_QUALITY_ID:
            if (u(4))
            {
                h264info.ext.quality_id = value();
                state = SCAN_NAL_FOR_SVC_TEMPORAL_ID;
            }
            break;
        case SCAN_NAL_FOR_SVC_TEMPORAL_ID:
            if (u(3))
            {
                h264info.ext.temporal_id = value();
                state = SCAN_NAL_FOR_SVC_USE_REF_BASE_PIC_FLAG;
            }
            break;
        case SCAN_NAL_FOR_SVC_USE_REF_BASE_PIC_FLAG:
            if (u(1))
            {
                h264info.ext.use_ref_base_pic_flag = value() ? true : false;
                state = SCAN_NAL_FOR_SVC_DISCARDABLE_FLAG;
            }
            break;
        case SCAN_NAL_FOR_SVC_DISCARDABLE_FLAG:
            if (u(1))
            {
                h264info.ext.discardable_flag = value() ? true : false;
                state = SCAN_NAL_FOR_SVC_OUTPUT_FLAG;
            }
            break;
        case SCAN_NAL_FOR_SVC_OUTPUT_FLAG:
            if (u(1))
            {
                h264info.ext.output_flag = value() ? true : false;
                state = SCAN_NAL_FOR_SVC_RESERVED_THREE_2BITS;
            }
            break;
        case SCAN_NAL_FOR_SVC_RESERVED_THREE_2BITS:
            if (u(2))
            {
                h264info.ext.reserved_three_2bits = value();
                switch (h264info.nal_unit_type)
                {
                case 7:
                    state = SCAN_SPS_FOR_PROFILE_IDC;
                    break;
                case 1:
                case 2:
                case 5:
                    state = SCAN_SLICE_HEADER_FOR_FIRST_MB_IN_SLICE;
                    break;
                default:
                    state = SCAN_NAL_FOR_SLICE;
                    break;
                }
            }
            break;
        case SCAN_NAL_FOR_MVC_NON_IDR_FLAG:
            if (u(1))
            {
                h264info.ext.non_idr_flag = value() ? true : false;
                state = SCAN_NAL_FOR_MVC_PRIORITY_ID;
            }
            break;
        case SCAN_NAL_FOR_MVC_PRIORITY_ID:
            if (u(6))
            {
                h264info.ext.priority_id = value();
                state = SCAN_NAL_FOR_MVC_VIEW_ID;
            }
            break;
        case SCAN_NAL_FOR_MVC_VIEW_ID:
            if (u(10))
            {
                h264info.ext.view_id = value();
                state = SCAN_NAL_FOR_MVC_TEMPORAL_ID;
            }
            break;
        case SCAN_NAL_FOR_MVC_TEMPORAL_ID:
            if (u(3))
            {
                h264info.ext.temporal_id = value();
                state = SCAN_NAL_FOR_MVC_ANCHOR_PIC_FLAG;
            }
            break;
        case SCAN_NAL_FOR_MVC_ANCHOR_PIC_FLAG:
            if (u(1))
            {
                h264info.ext.anchor_pic_flag = value() ? true : false;
                state = SCAN_NAL_FOR_MVC_INTER_VIEW_FLAG;
            }
            break;
        case SCAN_NAL_FOR_MVC_INTER_VIEW_FLAG:
            if (u(1))
            {
                h264info.ext.inter_view_flag = value() ? true : false;
                state = SCAN_NAL_FOR_MVC_RESERVED_ONE_BIT;
            }
            break;
        case SCAN_NAL_FOR_MVC_RESERVED_ONE_BIT:
            if (u(1))
            {
                h264info.ext.reserved_one_bit = value();
                switch (h264info.nal_unit_type)
                {
                case 7:
                    state = SCAN_SPS_FOR_PROFILE_IDC;
                    break;
                case 1:
                case 2:
                case 5:
                    state = SCAN_SLICE_HEADER_FOR_FIRST_MB_IN_SLICE;
                    break;
                default:
                    state = SCAN_NAL_FOR_SLICE;
                    break;
                }
            }
            break;
        case SCAN_SPS_FOR_PROFILE_IDC:
            h264info.sps.profile_idc = m_uData[curByteInStream];
            m_nOffsetInStream += 8;
            state = SCAN_SPS_FOR_CONSTRAINT_SET_FLAG;
            break;
        case SCAN_SPS_FOR_CONSTRAINT_SET_FLAG:
            h264info.sps.constraint_set_flag = m_uData[curByteInStream];
            m_nOffsetInStream += 8;
            state = SCAN_SPS_FOR_LEVEL_IDC;
            break;
        case SCAN_SPS_FOR_LEVEL_IDC:
            h264info.sps.level_idc = m_uData[curByteInStream];
            m_nOffsetInStream += 8;
            state = SCAN_SPS_FOR_SEQ_PARAMETER_SET_ID;
            break;
        case SCAN_SPS_FOR_SEQ_PARAMETER_SET_ID:
            if (uev())
            {
                h264info.sps.seq_parameter_set_id = value();
                if (h264info.sps.profile_idc == 100 ||
                    h264info.sps.profile_idc == 110 ||
                    h264info.sps.profile_idc == 122 ||
                    h264info.sps.profile_idc == 244 ||
                    h264info.sps.profile_idc == 44 ||
                    h264info.sps.profile_idc == 83 ||
                    h264info.sps.profile_idc == 86 ||
                    h264info.sps.profile_idc == 118 ||
                    h264info.sps.profile_idc == 128)
                {
                    state = SCAN_SPS_FOR_CHROMA_FORMAT_IDC;
                }
                else
                {
                    state = SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4;
                }
            }
            break;
        case SCAN_SPS_FOR_CHROMA_FORMAT_IDC:
            if (uev())
            {
                h264info.sps.chroma_format_idc = value();
                if (h264info.sps.chroma_format_idc == 3)
                {
                    state = SCAN_SPS_FOR_SEPARATE_COLOUR_PLANE_FLAG;
                }
                else
                {
                    state = SCAN_SPS_FOR_BIT_DEPTH_LUMA_MINUS8;
                }
            }
            break;
        case SCAN_SPS_FOR_SEPARATE_COLOUR_PLANE_FLAG:
            if (u(1))
            {
                h264info.sps.separate_colour_plane_flag = value() ? true : false;
                state = SCAN_SPS_FOR_BIT_DEPTH_LUMA_MINUS8;
            }
            break;
        case SCAN_SPS_FOR_BIT_DEPTH_LUMA_MINUS8:
            if (uev())
            {
                h264info.sps.bit_depth_luma_minu8 = value();
                state = SCAN_SPS_FOR_BIT_DEPTH_CHROMA_MINUS8;
            }
            break;
        case SCAN_SPS_FOR_BIT_DEPTH_CHROMA_MINUS8:
            if (uev())
            {
                h264info.sps.bit_depth_chroma_minus8 = value();
                state = SCAN_SPS_FOR_QPPRIME_Y_ZERO_TRANSFORM_BYPASS_FLAG;
            }
            break;
        case SCAN_SPS_FOR_QPPRIME_Y_ZERO_TRANSFORM_BYPASS_FLAG:
            if (u(1))
            {
                h264info.sps.qpprime_y_zero_transform_bypass_flag = value() ? true : false;
                state = SCAN_SPS_FOR_SEQ_SCALING_MATRIX_PRESENT_FLAG;
            }
            break;
        case SCAN_SPS_FOR_SEQ_SCALING_MATRIX_PRESENT_FLAG:
            if (u(1))
            {
                h264info.sps.seq_scaling_matrix_present_flag = value() ? true : false;
                if (h264info.sps.seq_scaling_matrix_present_flag)
                {
                    state = SCAN_SPS_FOR_SEQ_SCALING_LIST_PRESENT_FLAG;
                }
                else
                {
                    state = SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4;
                }
            }
            break;
        case SCAN_SPS_FOR_SEQ_SCALING_LIST_PRESENT_FLAG:
            if (h264info.sps.chroma_format_idc != 3)
            {
                if (u(8))
                {
                    h264info.sps.seq_scaling_list_present_flag = value();
                    state = SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4;
                }
            }
            else
            {
                if (u(12))
                {
                    h264info.sps.seq_scaling_list_present_flag = value();
                    state = SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4;
                }
            }
            break;
        case SCAN_SPS_FOR_LOG2_MAX_FRAME_NUM_MINUS4:
            if (uev())
            {
                h264info.sps.log2_max_frame_num_minus4 = value();
//                _tprintf(_T("**************** h264info.sps.log2_max_frame_num_minus4:%llu ******************\n"), h264info.sps.log2_max_frame_num_minus4);
//                _tprintf(_T("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
//                    m_uData[0], m_uData[1], m_uData[2], m_uData[3], m_uData[4], m_uData[5], m_uData[6], m_uData[7], m_uData[8], m_uData[9], m_uData[10], m_uData[11], m_uData[12], m_uData[13], m_uData[14], m_uData[15], m_uData[16], m_uData[17], m_uData[18], m_uData[19], m_uData[0], m_uData[1]);
                state = SCAN_SPS_FOR_PIC_ORDER_CNT_TYPE;
            }
            break;
        case SCAN_SPS_FOR_PIC_ORDER_CNT_TYPE:
            if (uev())
            {
                h264info.sps.pic_order_cnt_type = value();
                if (h264info.sps.pic_order_cnt_type == 0)
                {
                    state = SCAN_SPS_FOR_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4;
                }
                else if (h264info.sps.pic_order_cnt_type == 1)
                {
                    state = SCAN_SPS_FOR_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG;
                }
                else
                {
                    state = SCAN_SPS_FOR_MAX_NUM_REF_FRAMES;
                }
            }
            break;
        case SCAN_SPS_FOR_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4:
            if (uev())
            {
                h264info.sps.log2_max_pic_order_cnt_lsb_minus4 = value();
                state = SCAN_SPS_FOR_MAX_NUM_REF_FRAMES;
            }
            break;
        case SCAN_SPS_FOR_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG:
            if (u(1))
            {
                h264info.sps.delta_pic_order_always_zero_flag = value() ? true : false;
                state = SCAN_SPS_FOR_OFFSET_FOR_NON_REF_PIC;
            }
            break;
        case SCAN_SPS_FOR_OFFSET_FOR_NON_REF_PIC:
            if (sev())
            {
                h264info.sps.offset_for_non_ref_pic = value();
                state = SCAN_SPS_FOR_OFFSET_FOR_TOP_TO_BOTTOM_FIELD;
            }
            break;
        case SCAN_SPS_FOR_OFFSET_FOR_TOP_TO_BOTTOM_FIELD:
            if (sev())
            {
                h264info.sps.offset_for_top_to_bottom_field = value();
                state = SCAN_SPS_FOR_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_TYPE;
            }
            break;
        case SCAN_SPS_FOR_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_TYPE:
            if (uev())
            {
                h264info.sps.num_ref_frames_in_pic_order_cnt_cycle = value();
                if (h264info.sps.num_ref_frames_in_pic_order_cnt_cycle != 0)
                {
                    h264info.sps.offset_for_ref_frame = new int64_t[(uint32_t)(h264info.sps.num_ref_frames_in_pic_order_cnt_cycle)];
                    m_nArraryIndex = 0;
                    state = SCAN_SPS_FOR_OFFSET_FOR_REF_FRAME;
                }
                else
                {
                    state = SCAN_SPS_FOR_MAX_NUM_REF_FRAMES;
                }
            }
            break;
        case SCAN_SPS_FOR_OFFSET_FOR_REF_FRAME:
            if (sev())
            {
                h264info.sps.offset_for_ref_frame[m_nArraryIndex++] = value();
                if (m_nArraryIndex == h264info.sps.num_ref_frames_in_pic_order_cnt_cycle)
                {
                    delete[] h264info.sps.offset_for_ref_frame;
                    h264info.sps.offset_for_ref_frame = NULL;
                    state = SCAN_SPS_FOR_MAX_NUM_REF_FRAMES;
                }
            }
            break;
        case SCAN_SPS_FOR_MAX_NUM_REF_FRAMES:
            if (uev())
            {
                h264info.sps.max_num_ref_frames = value();
                state = SCAN_SPS_FOR_GAPS_IN_FRAME_NUM_VALUE_ALLOWED_FLAG;
            }
            break;
        case SCAN_SPS_FOR_GAPS_IN_FRAME_NUM_VALUE_ALLOWED_FLAG:
            if (u(1))
            {
                h264info.sps.gaps_in_frame_num_value_allowed_flag = value() ? true : false;
                state = SCAN_SPS_FOR_PIC_WIDTH_IN_MSB_MINUS1;
            }
            break;
        case SCAN_SPS_FOR_PIC_WIDTH_IN_MSB_MINUS1:
            if (uev())
            {
                h264info.sps.pic_width_in_mbs_minus1 = value();
                state = SCAN_SPS_FOR_PIC_HEIGHT_IN_MAP_UNITS_MINUS1;
            }
            break;
        case SCAN_SPS_FOR_PIC_HEIGHT_IN_MAP_UNITS_MINUS1:
            if (uev())
            {
                h264info.sps.pic_height_in_map_units_minus1 = value();
                m_observer->OnFrameSize((h264info.sps.pic_width_in_mbs_minus1 + 1) * 16, (h264info.sps.pic_height_in_map_units_minus1 + 1) * 16);
                state = SCAN_SPS_FOR_FRAME_MBS_ONLY_FLAG;
            }
            break;
        case SCAN_SPS_FOR_FRAME_MBS_ONLY_FLAG:
            if (u(1))
            {
                h264info.sps.frame_mbs_only_flag = value() ? true : false;
                if (!h264info.sps.frame_mbs_only_flag)
                {
                    state = SCAN_SPS_FOR_MB_ADAPTIVE_FRAME_FIELD_FLAG;
                }
                else
                {
                    state = SCAN_SPS_FOR_DIRECT_8X8_INFERENCE_FLAG;
                }
            }
            break;
        case SCAN_SPS_FOR_MB_ADAPTIVE_FRAME_FIELD_FLAG:
            if (u(1))
            {
                h264info.sps.mb_adaptive_frame_field_flag = value() ? true : false;
                state = SCAN_SPS_FOR_DIRECT_8X8_INFERENCE_FLAG;
            }
            break;
        case SCAN_SPS_FOR_DIRECT_8X8_INFERENCE_FLAG:
            if (u(1))
            {
                h264info.sps.direct_8x8_inference_flag = value() ? true : false;
                state = SCAN_SPS_FOR_FRAME_CROPPING_FLAG;
            }
            break;
        case SCAN_SPS_FOR_FRAME_CROPPING_FLAG:
            if (u(1))
            {
                h264info.sps.frame_croppiing_flag = value() ? true : false;
                if (h264info.sps.frame_croppiing_flag)
                {
                    state = SCAN_SPS_FOR_FRAME_CROP_LEFT_OFFSET;
                }
                else
                {
                    state = SCAN_SPS_FOR_VUI_PARAMETERS_PRESENT_FLAG;
                }
            }
            break;
        case SCAN_SPS_FOR_FRAME_CROP_LEFT_OFFSET:
            if (uev())
            {
                h264info.sps.frame_crop_left_offset = value();
                state = SCAN_SPS_FOR_FRAME_CROP_RIGHT_OFFSET;
            }
            break;
        case SCAN_SPS_FOR_FRAME_CROP_RIGHT_OFFSET:
            if (uev())
            {
                h264info.sps.frame_crop_right_offset = value();
                state = SCAN_SPS_FOR_FRAME_CROP_TOP_OFFSET;
            }
            break;
        case SCAN_SPS_FOR_FRAME_CROP_TOP_OFFSET:
            if (uev())
            {
                h264info.sps.frame_crop_top_offset = value();
                state = SCAN_SPS_FOR_FRAME_CROP_BOTTOM_OFFSET;
            }
            break;
        case SCAN_SPS_FOR_FRAME_CROP_BOTTOM_OFFSET:
            if (uev())
            {
                h264info.sps.frame_crop_bottom_offset = value();
                state = SCAN_SPS_FOR_VUI_PARAMETERS_PRESENT_FLAG;
            }
            break;
        case SCAN_SPS_FOR_VUI_PARAMETERS_PRESENT_FLAG:
            if (u(1))
            {
                h264info.sps.vui_parameters_present_flag = value() ? true : false;
                state = SCAN_NAL_ALIGN_BYTE;
            }
            break;
        case SCAN_SLICE_HEADER_FOR_FIRST_MB_IN_SLICE:
            if (uev())
            {
                h264info.first_mb_in_slice = value();
                state = SCAN_SLICE_HEADER_FOR_SLICE_TYPE;
            }
            break;
        case SCAN_SLICE_HEADER_FOR_SLICE_TYPE:
            if (uev())
            {
                h264info.slice_type = value();
                state = SCAN_SLICE_HEADER_FOR_PIC_PARAMETER_SET_ID;
            }
            break;
        case SCAN_SLICE_HEADER_FOR_PIC_PARAMETER_SET_ID:
            if (uev())
            {
                h264info.pic_parameter_set_id = value();
                if (h264info.sps.separate_colour_plane_flag)
                {
                    state = SCAN_SLICE_HEADER_FOR_COLOUR_PLANE_ID;
                }
                else
                {
                    state = SCAN_SLICE_HEADER_FOR_FRAME_NUM;
                }
            }
            break;
        case SCAN_SLICE_HEADER_FOR_COLOUR_PLANE_ID:
            if (u(2))
            {
                h264info.colour_plane_id = value();
                state = SCAN_SLICE_HEADER_FOR_FRAME_NUM;
            }
            break;
        case SCAN_SLICE_HEADER_FOR_FRAME_NUM:
            if (u((uint8_t)(h264info.sps.log2_max_frame_num_minus4) + 4))
            {
                uint64_t frame_num = value();
                if (frame_num != h264info.frame_num)
                {
                    h264info.frame_num = frame_num;
                    m_observer->OnNewFrame(h264info.frame_num, h264info.slice_type, m_nCurSliceOffset);
                }
                state = SCAN_NAL_ALIGN_BYTE;
            }
            break;

        case SCAN_NAL_ALIGN_BYTE:
            m_nOffsetInStream = (m_nOffsetInStream + 7) & 0xfffffffffffffff8;
            state = SCAN_NAL_FOR_SLICE;
            break;
        }
    }
}

}

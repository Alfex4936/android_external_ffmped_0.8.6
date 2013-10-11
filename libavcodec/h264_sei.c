/*
 * H.26L/H.264/AVC/JVT/14496-10/... sei decoding
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * H.264 / AVC / MPEG4 part10 sei decoding.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "internal.h"
#include "avcodec.h"
#include "h264.h"
#include "golomb.h"

//#undef NDEBUG
#include <assert.h>

#if defined(__LGE__) && defined(__LGE_S3D__)
enum {
	S3D_MODE_OFF = 0,
	S3D_MODE_ON = 1,
	S3D_MODE_ANAGLYPH = 2,
} S3D_ModeType;

enum {
	S3D_FORMAT_NONE = 0,
	S3D_FORMAT_OVERUNDER,
	S3D_FORMAT_SIDEBYSIDE,
	S3D_FORMAT_ROW_IL,
	S3D_FORMAT_COL_IL,
	S3D_FORMAT_PIX_IL,
	S3D_FORMAT_CHECKB,
	S3D_FORMAT_FRM_SEQ,
} S3D_FormatType;

enum {
	S3D_ORDER_LF = 0,
	S3D_ORDER_RF,
} S3D_OrderType;

enum {
	S3D_SS_NONE = 0,
	S3D_SS_HOR,
	S3D_SS_VERT,
} S3D_SubSamplingType;

enum {
	S3D_FRAME_ARRANGE_TYPE_CHECKERBOARD = 0,
	S3D_FRAME_ARRANGE_TYPE_COLUMN = 1,
	S3D_FRAME_ARRANGE_TYPE_ROW = 2,
	S3D_FRAME_ARRANGE_TYPE_SIDE_BY_SIDE = 3,
	S3D_FRAME_ARRANGE_TYPE_TOP_BOTTOM = 4,
	S3D_FRAME_ARRANGE_TYPE_TEMPORAL = 5,
} S3D_FrameArrangeType;

typedef struct S3DParams {
	int	active;
	int	mode;
	int	fmt;
	int	order;
	int	subsampling;
} S3DParams;

typedef struct SEIStereoVideoInfo {
	uint32_t field_views_flag;                                   //!< u(1)
	uint32_t top_field_is_left_view_flag;                        //!< u(1)
	uint32_t current_frame_is_left_view_flag;                    //!< u(1)
	uint32_t next_frame_is_second_view_flag;                     //!< u(1)
	uint32_t left_view_self_contained_flag;                      //!< u(1)
	uint32_t right_view_self_contained_flag;                     //!< u(1)
} SEIStereoVideoInfo;

typedef struct SEIFramePackingArrangement {
	uint32_t frame_packing_arrangement_id;                       //!< ue(v)
	uint32_t frame_packing_arrangement_cancel_flag;              //!< u(1)
	uint32_t frame_packing_arrangement_type;                     //!< u(7)
	uint32_t quincunx_sampling_flag;                             //!< u(1)
	uint32_t content_interpretation_type;                        //!< u(6)
	uint32_t spatial_flipping_flag;                              //!< u(1)
	uint32_t frame0_flipped_flag;                                //!< u(1)
	uint32_t field_views_flag;                                   //!< u(1)
	uint32_t current_frame_is_frame0_flag;                       //!< u(7)
	uint32_t frame0_self_contained_flag;                         //!< u(1)
	uint32_t frame1_self_contained_flag;                         //!< u(1)
	uint32_t frame0_grid_position_x;                             //!< u(4)
	uint32_t frame0_grid_position_y;                             //!< u(4)
	uint32_t frame1_grid_position_x;                             //!< u(4)
	uint32_t frame1_grid_position_y;                             //!< u(4)
	uint32_t frame_packing_arrangement_reserved_byte;            //!< u(8)
	uint32_t frame_packing_arrangement_repetition_period;        //!< ue(v)
	uint32_t frame_packing_arrangement_extension_flag;           //!< u(1)
} SEIFramePackingArrangement;
#endif

static const uint8_t sei_num_clock_ts_table[9]={
    1,  1,  1,  2,  2,  3,  3,  2,  3
};

void ff_h264_reset_sei(H264Context *h) {
    h->sei_recovery_frame_cnt       = -1;
    h->sei_dpb_output_delay         =  0;
    h->sei_cpb_removal_delay        = -1;
    h->sei_buffering_period_present =  0;
#if defined(__LGE__) && defined(__LGE_S3D__)
	h->sei_s3d                      =  0;
	//av_log(NULL, AV_LOG_ERROR, "%s --> reset sei_s3d \n", __FUNCTION__);
#endif
}

static int decode_picture_timing(H264Context *h){
    MpegEncContext * const s = &h->s;
    if(h->sps.nal_hrd_parameters_present_flag || h->sps.vcl_hrd_parameters_present_flag){
        h->sei_cpb_removal_delay = get_bits(&s->gb, h->sps.cpb_removal_delay_length);
        h->sei_dpb_output_delay = get_bits(&s->gb, h->sps.dpb_output_delay_length);
    }
    if(h->sps.pic_struct_present_flag){
        unsigned int i, num_clock_ts;
        h->sei_pic_struct = get_bits(&s->gb, 4);
        h->sei_ct_type    = 0;

        if (h->sei_pic_struct > SEI_PIC_STRUCT_FRAME_TRIPLING)
            return -1;

        num_clock_ts = sei_num_clock_ts_table[h->sei_pic_struct];

        for (i = 0 ; i < num_clock_ts ; i++){
            if(get_bits(&s->gb, 1)){                  /* clock_timestamp_flag */
                unsigned int full_timestamp_flag;
                h->sei_ct_type |= 1<<get_bits(&s->gb, 2);
                skip_bits(&s->gb, 1);                 /* nuit_field_based_flag */
                skip_bits(&s->gb, 5);                 /* counting_type */
                full_timestamp_flag = get_bits(&s->gb, 1);
                skip_bits(&s->gb, 1);                 /* discontinuity_flag */
                skip_bits(&s->gb, 1);                 /* cnt_dropped_flag */
                skip_bits(&s->gb, 8);                 /* n_frames */
                if(full_timestamp_flag){
                    skip_bits(&s->gb, 6);             /* seconds_value 0..59 */
                    skip_bits(&s->gb, 6);             /* minutes_value 0..59 */
                    skip_bits(&s->gb, 5);             /* hours_value 0..23 */
                }else{
                    if(get_bits(&s->gb, 1)){          /* seconds_flag */
                        skip_bits(&s->gb, 6);         /* seconds_value range 0..59 */
                        if(get_bits(&s->gb, 1)){      /* minutes_flag */
                            skip_bits(&s->gb, 6);     /* minutes_value 0..59 */
                            if(get_bits(&s->gb, 1))   /* hours_flag */
                                skip_bits(&s->gb, 5); /* hours_value 0..23 */
                        }
                    }
                }
                if(h->sps.time_offset_length > 0)
                    skip_bits(&s->gb, h->sps.time_offset_length); /* time_offset */
            }
        }

        if(s->avctx->debug & FF_DEBUG_PICT_INFO)
            av_log(s->avctx, AV_LOG_DEBUG, "ct_type:%X pic_struct:%d\n", h->sei_ct_type, h->sei_pic_struct);
    }
    return 0;
}

static int decode_unregistered_user_data(H264Context *h, int size){
    MpegEncContext * const s = &h->s;
    uint8_t user_data[16+256];
    int e, build, i;

    if(size<16)
        return -1;

    for(i=0; i<sizeof(user_data)-1 && i<size; i++){
        user_data[i]= get_bits(&s->gb, 8);
    }

    user_data[i]= 0;
    e= sscanf(user_data+16, "x264 - core %d"/*%s - H.264/MPEG-4 AVC codec - Copyleft 2005 - http://www.videolan.org/x264.html*/, &build);
    if(e==1 && build>0)
        h->x264_build= build;

    if(s->avctx->debug & FF_DEBUG_BUGS)
        av_log(s->avctx, AV_LOG_DEBUG, "user data:\"%s\"\n", user_data+16);

    for(; i<size; i++)
        skip_bits(&s->gb, 8);

    return 0;
}

static int decode_recovery_point(H264Context *h){
    MpegEncContext * const s = &h->s;

    h->sei_recovery_frame_cnt = get_ue_golomb(&s->gb);
    skip_bits(&s->gb, 4);       /* 1b exact_match_flag, 1b broken_link_flag, 2b changing_slice_group_idc */

    return 0;
}

static int decode_buffering_period(H264Context *h){
    MpegEncContext * const s = &h->s;
    unsigned int sps_id;
    int sched_sel_idx;
    SPS *sps;

    sps_id = get_ue_golomb_31(&s->gb);
    if(sps_id > 31 || !h->sps_buffers[sps_id]) {
        av_log(h->s.avctx, AV_LOG_ERROR, "non-existing SPS %d referenced in buffering period\n", sps_id);
        return -1;
    }
    sps = h->sps_buffers[sps_id];

    // NOTE: This is really so duplicated in the standard... See H.264, D.1.1
    if (sps->nal_hrd_parameters_present_flag) {
        for (sched_sel_idx = 0; sched_sel_idx < sps->cpb_cnt; sched_sel_idx++) {
            h->initial_cpb_removal_delay[sched_sel_idx] = get_bits(&s->gb, sps->initial_cpb_removal_delay_length);
            skip_bits(&s->gb, sps->initial_cpb_removal_delay_length); // initial_cpb_removal_delay_offset
        }
    }
    if (sps->vcl_hrd_parameters_present_flag) {
        for (sched_sel_idx = 0; sched_sel_idx < sps->cpb_cnt; sched_sel_idx++) {
            h->initial_cpb_removal_delay[sched_sel_idx] = get_bits(&s->gb, sps->initial_cpb_removal_delay_length);
            skip_bits(&s->gb, sps->initial_cpb_removal_delay_length); // initial_cpb_removal_delay_offset
        }
    }

    h->sei_buffering_period_present = 1;
    return 0;
}

#if defined(__LGE__) && defined(__LGE_S3D__)
static void set_sei_s3d(H264Context *h, S3DParams *s3d_params){
	SEI_S3dType s3d_type = SEI_S3D_NORMAL;

	if (h == NULL || s3d_params == NULL)
		return;

	if (s3d_params->active) {
		s3d_type = SEI_S3D_NORMAL;

		switch (s3d_params->fmt) {
			case S3D_FORMAT_OVERUNDER:
				s3d_type = SEI_S3D_TB;
				break;
			case S3D_FORMAT_SIDEBYSIDE:
				switch (s3d_params->order) {
					case S3D_ORDER_LF:
						s3d_type = SEI_S3D_LR;
						break;
					case S3D_ORDER_RF:
						s3d_type = SEI_S3D_RL;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	h->sei_s3d = s3d_type;

	{
		AVCodecContext *avctx = h->s.avctx;
		if (avctx != NULL)
			avctx->s3d_video_type = s3d_type;
	}
}
#endif

#if defined(__LGE__) && defined(__LGE_S3D__)
static int decode_stereo_video_info(H264Context *h){
	int16_t status = 0;
	uint32_t temp;
	SEIStereoVideoInfo stereo_video_info = {0, };
	S3DParams s3d_params = {0, };
    MpegEncContext * const s = &h->s;

	s3d_params.active = 1;
	s3d_params.mode = S3D_MODE_ON;
	s3d_params.fmt = S3D_FORMAT_OVERUNDER;
	s3d_params.subsampling = S3D_SS_NONE;
	temp = get_bits(&s->gb, 1);
	stereo_video_info.field_views_flag = temp;

	if( stereo_video_info.field_views_flag ) {
		temp = get_bits(&s->gb, 1);
		stereo_video_info.top_field_is_left_view_flag = temp;
		if(stereo_video_info.top_field_is_left_view_flag)
			s3d_params.order = S3D_ORDER_LF;
		else
			s3d_params.order = S3D_ORDER_RF;
	} else {
		temp = get_bits(&s->gb, 1);
		stereo_video_info.current_frame_is_left_view_flag = temp;
		if(stereo_video_info.current_frame_is_left_view_flag)
			s3d_params.order = S3D_ORDER_LF;
		else
			s3d_params.order = S3D_ORDER_RF;

		temp = get_bits(&s->gb, 1);
		stereo_video_info.next_frame_is_second_view_flag = temp;
	}

	temp = get_bits(&s->gb, 1);
	stereo_video_info.left_view_self_contained_flag = temp;
	temp = get_bits(&s->gb, 1);
	stereo_video_info.right_view_self_contained_flag = temp;

	set_sei_s3d(h, &s3d_params);
	return 0;
}
#endif

#if defined(__LGE__) && defined(__LGE_S3D__)
static void set_frame_packing_arrangement_type(uint32_t frame_packing_arrangement_type, 
												uint32_t *format, uint32_t *subsampling) {
	if (format == NULL || subsampling == NULL)
		return;

	switch (frame_packing_arrangement_type) {
		case S3D_FRAME_ARRANGE_TYPE_CHECKERBOARD:
			*format = S3D_FORMAT_CHECKB;
			break;
		case S3D_FRAME_ARRANGE_TYPE_COLUMN:
			*format = S3D_FORMAT_COL_IL;
			break;
		case S3D_FRAME_ARRANGE_TYPE_ROW:
			*format = S3D_FORMAT_ROW_IL;
			break;
		case S3D_FRAME_ARRANGE_TYPE_SIDE_BY_SIDE:
			*format = S3D_FORMAT_SIDEBYSIDE;
			*subsampling = S3D_SS_HOR;
			break;
		case S3D_FRAME_ARRANGE_TYPE_TOP_BOTTOM:
			*format = S3D_FORMAT_OVERUNDER;
			*subsampling = S3D_SS_VERT;
			break;
		case S3D_FRAME_ARRANGE_TYPE_TEMPORAL:
			*format = S3D_FORMAT_FRM_SEQ;
			break;
		default:
			*format = S3D_FORMAT_OVERUNDER;
			break;
	}
}
#endif

#if defined(__LGE__) && defined(__LGE_S3D__)
#define PV_CLZ(A,B) while (((B) & 0x8000) == 0) {(B) <<=1; A++;}
static void ue_v(GetBitContext *s, uint32_t *codeNum) {
	uint32_t temp;
	uint tmp_cnt;
	int32_t leading_zeros = 0;
	temp = show_bits(s, 16);
	tmp_cnt = temp | 0x1;
	PV_CLZ(leading_zeros, tmp_cnt)

	if (leading_zeros < 8) {
		*codeNum = (temp >> (15 - (leading_zeros << 1))) - 1;
		skip_bits(s, (leading_zeros << 1) + 1);
	} else {
		temp = get_bits(s, (leading_zeros << 1) + 1);
		*codeNum = temp - 1;
	}
}
#endif

#if defined(__LGE__) && defined(__LGE_S3D__)
static int decode_frame_packing_arrangement(H264Context *h){
	int status = 0;
	uint32_t temp;
	SEIFramePackingArrangement frame_packing_arrangement = {0, };
	S3DParams s3d_params = {0, };
	MpegEncContext * const s = &h->s;

	s3d_params.active = 1;
	s3d_params.mode = S3D_MODE_ON;
	s3d_params.subsampling = S3D_SS_NONE;

	ue_v(&s->gb, &(frame_packing_arrangement.frame_packing_arrangement_id));
	temp = get_bits(&s->gb, 1);
	frame_packing_arrangement.frame_packing_arrangement_cancel_flag = temp;
	if(!frame_packing_arrangement.frame_packing_arrangement_cancel_flag)
	{
		temp = get_bits(&s->gb, 7);
		frame_packing_arrangement.frame_packing_arrangement_type = temp;
		set_frame_packing_arrangement_type(frame_packing_arrangement.frame_packing_arrangement_type, 
				&(s3d_params.fmt), &(s3d_params.subsampling));

		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.quincunx_sampling_flag = temp;
		temp = get_bits(&s->gb, 6);
		frame_packing_arrangement.content_interpretation_type = temp;
		if(frame_packing_arrangement.content_interpretation_type)
			s3d_params.order = S3D_ORDER_LF;
		else
			s3d_params.order = S3D_ORDER_RF;

		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.spatial_flipping_flag = temp;
		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.frame0_flipped_flag = temp;
		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.field_views_flag = temp;
		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.current_frame_is_frame0_flag = temp;
		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.frame0_self_contained_flag = temp;
		temp = get_bits(&s->gb, 1);
		frame_packing_arrangement.frame1_self_contained_flag = temp;

		if(!frame_packing_arrangement.quincunx_sampling_flag &&
				frame_packing_arrangement.content_interpretation_type != S3D_FRAME_ARRANGE_TYPE_TEMPORAL)
		{
			temp = get_bits(&s->gb, 4);
			frame_packing_arrangement.frame0_grid_position_x = temp;
			temp = get_bits(&s->gb, 4);
			frame_packing_arrangement.frame0_grid_position_y = temp;
			temp = get_bits(&s->gb, 4);
			frame_packing_arrangement.frame1_grid_position_x = temp;
			temp = get_bits(&s->gb, 4);
			frame_packing_arrangement.frame1_grid_position_y = temp;
		}
		temp = get_bits(&s->gb, 8);
		frame_packing_arrangement.frame_packing_arrangement_reserved_byte = temp;
		ue_v(&s->gb, &(frame_packing_arrangement.frame_packing_arrangement_repetition_period));
	}

	temp = get_bits(&s->gb, 1);
	frame_packing_arrangement.frame_packing_arrangement_extension_flag = temp;

	set_sei_s3d(h, &s3d_params);
	return 0;
}
#endif

int ff_h264_decode_sei(H264Context *h){
    MpegEncContext * const s = &h->s;

    while(get_bits_count(&s->gb) + 16 < s->gb.size_in_bits){
        int size, type;

        type=0;
        do{
            type+= show_bits(&s->gb, 8);
        }while(get_bits(&s->gb, 8) == 255);

        size=0;
        do{
            size+= show_bits(&s->gb, 8);
        }while(get_bits(&s->gb, 8) == 255);

        switch(type){
        case SEI_TYPE_PIC_TIMING: // Picture timing SEI
            if(decode_picture_timing(h) < 0)
                return -1;
            break;
        case SEI_TYPE_USER_DATA_UNREGISTERED:
            if(decode_unregistered_user_data(h, size) < 0)
                return -1;
            break;
        case SEI_TYPE_RECOVERY_POINT:
            if(decode_recovery_point(h) < 0)
                return -1;
            break;
        case SEI_BUFFERING_PERIOD:
            if(decode_buffering_period(h) < 0)
                return -1;
            break;
#if defined(__LGE__) && defined(__LGE_S3D__)
        case SEI_TYPE_STEREO_VIDEO_INFO:
            if(decode_stereo_video_info(h) < 0)
                return -1;
			av_log(NULL, AV_LOG_ERROR, "%s --> STEREO_VIDEO_INFO decoded, sei_s3d:%d \n", __FUNCTION__, h->sei_s3d);
            break;
        case SEI_TYPE_FRAME_PACKING_ARRANGEMENT:
            if(decode_frame_packing_arrangement(h) < 0)
                return -1;
			av_log(NULL, AV_LOG_ERROR, "%s --> FRAME_PACKING_ARRANGEMENT decoded, sei_s3d:%d \n", __FUNCTION__, h->sei_s3d);
            break;
#endif
        default:
            skip_bits(&s->gb, 8*size);
        }

        //FIXME check bits here
        align_get_bits(&s->gb);
    }

    return 0;
}

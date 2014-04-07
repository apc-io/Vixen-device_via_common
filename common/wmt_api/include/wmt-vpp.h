/*
 * Copyright (c) 2008-2013 WonderMedia Technologies, Inc. All Rights Reserved.
 *
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any
 * third party, in whole or in part, without prior written consent of
 * WonderMedia.
 *
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED
 * AS IS, WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS
 * OR IMPLIED, AND WONDERMEDIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS
 * OR IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * QUIET ENJOYMENT OR NON-INFRINGEMENT.
 */

#ifndef API_VPP_H
#define API_VPP_H

#include "com-vpp.h"

#ifdef __cplusplus
extern "C" {
#endif

int wmt_vpp_init(void);
int wmt_vpp_exit(void);

/* ------------------------------ VPP ------------------------------ */
int wmt_vpp_get_capability(vpp_cap_t *cap);
int wmt_vpp_set_i2c_byte(unsigned int addr,unsigned int index,unsigned char value);
int wmt_vpp_get_i2c_byte(unsigned int addr,unsigned int index,unsigned char *value);
int wmt_vpp_set_fbdisp(vdo_framebuf_t *fb,vdo_view_t *view);
int wmt_vpp_wait_frame(int cnt);
int wmt_vpp_scale(vdo_framebuf_t *src_fb,vdo_framebuf_t *dst_fb,int async);
int wmt_vpp_scale_finish(void);
int wmt_vpp_set_overlap(vpp_overlap_t *parm);
int wmt_vpp_scale_overlap(vdo_framebuf_t *src_fb,vdo_framebuf_t *src2_fb,vdo_framebuf_t *dst_fb,int mode);
int wmt_vpp_set_scale_pandisplay(vdo_framebuf_t *fb,vdo_view_t *view);

/* ------------------------------ MODULE ------------------------------ */
int wmt_vpp_get_fbinfo(vpp_mod_t mod,vdo_framebuf_t *fb);
int wmt_vpp_set_fbinfo(vpp_mod_t mod,vdo_framebuf_t *fb);
int wmt_vpp_get_fbaddr(vpp_mod_t mod,unsigned int *yaddr,unsigned int *caddr);
int wmt_vpp_set_fbaddr(vpp_mod_t mod,unsigned int yaddr,unsigned int caddr);
int wmt_vpp_set_csc(vpp_mod_t mod,vpp_csc_t csc_mode);
int wmt_vpp_get_csc(vpp_mod_t mod,vpp_csc_t *csc_mode);

/* ------------------------------ VOUT ------------------------------ */
int wmt_vpp_vout_get_info(vpp_vout_info_t *parm);
int wmt_vpp_vout_enable_mode(int no,int enable);
int wmt_vpp_vout_set_option(int no,unsigned int *option);
int wmt_vpp_vout_get_option(int no,unsigned int *option);
int wmt_vpp_vout_set_blank(int no,int enable);
int wmt_vpp_vout_set_resolution(int resx,int resy,int fps);
int wmt_vpp_get_resolution(unsigned int *resx,unsigned int *resy);
int wmt_vpp_get_support_vmode(vpp_vout_t no,vpp_vmode_parm_t *vmode,int max,int index);
int wmt_vpp_get_edid(vpp_vout_t no,	char *buf,	int size, int *edid_size);
int wmt_vpp_vout_get_cp_info(vpp_vout_cp_info_t *parm);
int wmt_vpp_vout_set_cp_key(vpp_vout_cp_key_t *parm);
int wmt_vpp_vout_set_audio_passthru(int enable);
int wmt_vpp_vout_set_virtual_fbdev(int enable);
int wmt_vpp_set_multi_video_decode(int enable);

/* ------------------------------ SCL ------------------------------ */
int wmt_vpp_scl_scale(vdo_framebuf_t *src_fb,vdo_framebuf_t *dst_fb);

/* ------------------------------ CURSOR ------------------------------ */
int wmt_vpp_cursor_enable(int enable);
int wmt_vpp_cursor_set_fbinfo(vdo_framebuf_t *fb);
int wmt_vpp_cursor_get_fbinfo(vdo_framebuf_t *fb);
int wmt_vpp_cursor_set_pos(unsigned int x,unsigned int y);
int wmt_vpp_cursor_get_pos(unsigned int *x,unsigned int *y);
int wmt_vpp_cursor_set_color_key(unsigned int color_key);
int wmt_vpp_cursor_set_hotspot(unsigned int x,unsigned int y);

/* ------------------------------ STREAM ------------------------------ */
int wmt_vpp_stream_enable(int enable);
int wmt_vpp_stream_get_fb(vdo_framebuf_t *fb);
int wmt_vpp_stream_put_fb(vdo_framebuf_t *fb);

#ifdef __cplusplus
}
#endif

#endif


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
#ifndef WMT_JDEC_H
/* To assert that only one occurrence is included */
#define WMT_JDEC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

#include "wmt-api.h"
#include "wmt-log.h"
#include "com-jdec.h"


/*-------------------- EXPORTED PRIVATE VARIABLES ----------------------------*/
#ifdef JPEG_COMMON_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif

#undef EXTERN

/*--------------------- EXPORTED PRIVATE FUNCTIONS  --------------------------*/

/*  following is the C++ header */
#ifdef __cplusplus
extern "C" {
#endif

int wmt_jdec_init(vd_handle_t *handle);
int wmt_jdec_exit(vd_handle_t *handle);

int wmt_jdec_buf(
    vd_handle_t      *handle,
    unsigned char    *buf_in,
    unsigned int      bufsize,
    jdec_frameinfo_t *frameinfo);

int wmt_jdec_file(vd_handle_t *handle, char *filename, jdec_frameinfo_t *frameinfo);

int wmt_jdec_header (
    vd_handle_t     *handle,
    unsigned char   *buf_in,
    unsigned int     buf_size,
    jdec_hdr_info_t *hdr);

/*!*************************************************************************
* wmt_jdec_fb_alloc
*/
/*!
* \brief   Allocate a phyiscally continuously memory for HW decoder
*
* \parameter
*   pic_w         [IN] Decoded picture width  (unit: pixel)
*   pic_h         [IN] Decoded picture height (unit: pixel)
*   src_color     [IN] Picture color format
*   decoded_color [IN] Decoded picture color format
*   arg_in        [OUT] 6 return values are stored in this structure
* \retval  0 if success, else return a negtive value
*/
int wmt_jdec_fb_alloc (
    vd_handle_t    *handle,
    unsigned int    pic_w,
    unsigned int    pic_h,
    vdo_color_fmt   src_color,
    vdo_color_fmt   decoded_color_in,
    jdec_input_t   *arg_in);

int wmt_jdec_fb_free (vd_handle_t *handle, jdec_frameinfo_t *info);

int wmt_jdec_decode_loop(
    vd_handle_t      *handle,
    jdec_param_t     *param,
    jdec_frameinfo_t *jfb);

int wmt_jdec_decode_frame(
    vd_handle_t      *handle,
    jdec_dec_param_t *jdp);


#ifdef	__cplusplus
}
#endif

#endif /* ifndef WMT_JDEC_H */

/*=== END wmt-jdec.h ==========================================================*/

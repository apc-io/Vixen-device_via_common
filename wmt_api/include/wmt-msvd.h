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
#ifndef WMT_MSVD_H
/* To assert that only one occurrence is included */
#define WMT_MSVD_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

#include "wmt-vd.h"
#include "com-msvd.h"

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/* #define  WMT_XXXX  1    *//*Example*/

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  wmt_xxx_t;  *//*Example*/

/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMT_MSVD_C 
    #define EXTERN
#else
    #define EXTERN   extern
#endif /* ifdef WMT_MSVD_C */

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/
/* extern void  wmt_vd_xxxx(void); *//*Example*/
/*  following is the C++ header	*/
#ifdef  __cplusplus
extern "C" {
#endif

int wmt_msvd_get_property(vd_handle_t *handle, msvd_property_t *prop);
int wmt_msvd_set_attribute(vd_handle_t *handle, msvd_attribute_t *att_in);
int wmt_msvd_decode_start(vd_handle_t *handle, msvd_input_t *input);
int wmt_msvd_decode_finish(vd_handle_t *handle, msvd_frameinfo_t *frame);
int wmt_msvd_flush(vd_handle_t *handle);

int wmt_msvd_open_device(vd_handle_t *handle, const char *dev_name, int vd_id);
int wmt_msvd_close_device(vd_handle_t *handle);

#ifdef  __cplusplus
}
#endif

#endif /* ifndef WMT_MSVD_H */

/*=== END wmt-msvd.h ==========================================================*/

/*
 * Copyright (c) 2013 WonderMedia Technologies, Inc. All Rights Reserved.
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
#ifndef WMT_JENC_H
/* To assert that only one occurrence is included */
#define WMT_JENC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

#include "com-jenc.h"

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/* #define  WMT_XXXX  1    *//*Example*/

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  wmt_xxx_t;  *//*Example*/

/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMT_JENC_C 
    #define EXTERN
#else
    #define EXTERN   extern
#endif /* ifdef WMT_JENC_C */

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/
/* extern void  wmt_vd_xxxx(void); *//*Example*/
/*  following is the C++ header */
#ifdef  __cplusplus
extern "C" {
#endif

int wmt_jenc_init(ve_handle_t *handle);
int wmt_jenc_exit(ve_handle_t *handle);

int wmt_jenc_encode_loop(ve_handle_t *handle, jenc_param_t *param);

#ifdef  __cplusplus
}
#endif

#endif /* ifndef WMT_JENC_H */

/*=== END wmt-jenc.h ==========================================================*/

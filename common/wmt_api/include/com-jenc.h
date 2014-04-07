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

#ifndef COM_JENC_H
/* To assert that only one occurrence is included */
#define COM_JENC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#ifdef __KERNEL__
#include "../com-ve.h"
#else
#include "com-ve.h"
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/* #define  VIAAPI_XXXX  1    *//*Example*/

#define JENC_VERSION     100   /* version: 0.01.00 */

/*------------------------------------------------------------------------------
    Definitions of enum
------------------------------------------------------------------------------*/
typedef enum { 
    MEM_MODE_USR,
        /* memory of input buffer come from user space memory (Not physical continous).
           It usually comes fomr application allocated memory.
           It has no any alignment limitation. */

    MEM_MODE_PHY
        /* memory of input buffer come from physical memory (physical continuous).
           It usually comes fomr HW video decoded frame buffer.
           It has some alignment limitation, e.g., 16 at least. */
}jenc_mem_mode;

/*------------------------------------------------------------------------------
    Definitions of structures
------------------------------------------------------------------------------*/
typedef struct jenc_pic_s {  /* Picture attribution */
    unsigned int   src_addr;  /* Encoding buffer (source) address */
                   /* 
                      For YC color domain, src_addr points to Y start address
                        src_addr
                            +-------------------+
                            |                   |
                            |     Y - Plane     |
                            |                   |
                            +-------------------+
                            |                   |
                            |     C - Plane     |
                            |                   |
                            +-------------------+
                      And start address of C-plane should be equal to
                         c_addr = src_addr + stride * img_height
                   */
    unsigned int   src_size;  /* Encoding buffer size in bytes */
                   /*
                      For YC color domain, src_size should be equal to
                        y_size + c_size
                   */
    unsigned int   img_width;
    unsigned int   img_height;
    unsigned int   stride;    /* Line size in source buffer in bytes */
    vdo_color_fmt  col_fmt;
} jenc_pic_t;

/*------------------------------------------------------------------------------
    Following structure is used for VDIOSET_ENCODE_LOOP
------------------------------------------------------------------------------*/
typedef struct {
    VE_IOCTL_CMD_M;
    /* destination encoded picture settings */
    unsigned int   buf_addr;  /* Encoding buffer (dest) address */
    unsigned int   buf_size;  /* Encoding buffer size in bytes */
    unsigned int   enc_size;  /* Really encoded data size in bytes */
    vdo_color_fmt  enc_color;
    int            quality;   /* 0(worst) - 100(best) */
    
    jenc_mem_mode  mem_mode;
    
    /* Source picture data settings */
    union {
        jenc_pic_t     src_usr;   /* for MEM_MODE_USR mode (default) */
        vdo_framebuf_t src_phy;   /* for MEM_MODE_PHY mode. This structure is
                                    defined in com-video.h (WMT API) */
    };
    unsigned int timeout;  /* lock timeout */
} jenc_param_t;


/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef JENC_COMMON_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif 

#undef EXTERN

    
#endif /* ifndef COM_JENC_H */

/*=== END com-jenc.h ==========================================================*/

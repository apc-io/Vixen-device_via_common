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
#ifndef COM_JDEC_H
/* To assert that only one occurrence is included */
#define COM_JDEC_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#ifdef __KERNEL__
#include "../com-vd.h"
#else
#include "com-vd.h"
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

#define JDEC_VERSION     200   /* version: 0.02.00 */


#define IOCTL_JPEG_INIT(cmd, _struct_type_) \
        memset((cmd), 0, sizeof(_struct_type_));


/*------------------------------------------------------------------------------
    Definitions of enum
------------------------------------------------------------------------------*/
typedef enum {
    SCALE_ORIGINAL,       /* Original size */
    SCALE_HALF,           /* 1/2 of Original size */
    SCALE_QUARTER,        /* 1/4 of Original size */
    SCALE_EIGHTH,         /* 1/8 of Original size */
    SCALE_MAX
} vd_scale_ratio;

typedef enum {
    JT_FRAME,
    JT_FIELD_TOP,
    JT_FIELD_BOTTOM
} jpeg_type;

/*------------------------------------------------------------------------------
    Definitions of structures
------------------------------------------------------------------------------*/
/* Used for AVI1 FOURCC in APP0 only */
typedef struct {
    jpeg_type  type;
    int        field_size;  /* 0 if JT_FRAME; others: data size of a field */
} avi1_t; 

/*------------------------------------------------------------------------------
   Following macro definition is used in jdec_dec_param_t
------------------------------------------------------------------------------*/
#define DECFLAG_MJPEG_BIT      BIT(28)  /* 0: JPEG still picture
                                           1: MJPG movie */
#define FLAG_MULTI_SEGMENT_EN  BIT(29)  /* 0: Multi-segment decoding is disable
                                           1: Multi-segment decoding is enable */
#define FLAG_BUFPHYCONTI       BIT(30)  /* 0: IN buffer is not phy. continously
                                           1: IN buffer is phy. continously */
#define FLAG_NOBUFALLOCATED    BIT(31)  /* 0: Ouput buffer was allocated by user
                                           1: Ouput buffer was not allocated */

/*------------------------------------------------------------------------------
    Header Information
------------------------------------------------------------------------------*/
typedef struct {
    unsigned int    profile;       /* Profile */
    unsigned int    sof_w;         /* Picture width in SOF */
    unsigned int    sof_h;         /* Picture height in SOF */
    unsigned int    filesize;      /* File size of this JPEG */
    vdo_color_fmt   src_color;     /* Picture color format */
    avi1_t          avi1;
} jdec_hdr_info_t;

/*------------------------------------------------------------------------------
    Decoder Information
------------------------------------------------------------------------------*/
typedef struct {
    unsigned int enable;   /* Enable HW Partial decode or not */
    unsigned int x;        /* X of start decode (16xN alignment) (Unit: Pixel) */
    unsigned int y;        /* Y of start decode (16xN alignment) (Unit: Pixel) */
    unsigned int w;        /* Width of start decode (16xN alignment) (Unit: Pixel) */
    unsigned int h;        /* Height of start decode (16xN alignment) (Unit: Pixel) */
} jdec_pd; /* PD: Partial Decode */

/*------------------------------------------------------------------------------
   Following structure is used for JPEG HW decoder as input arguments 
------------------------------------------------------------------------------*/
typedef struct {
    unsigned int    src_addr;  /* input buffer addr in user space */
    unsigned int    src_size;  /* input buffer size */
    unsigned int    flags;     /* control flag for HW decoder */
    /* Output Frame buffer information */
    unsigned int    dst_y_addr;  /* output address in Y (Physical addr )*/
    unsigned int    dst_y_size;  /* output buffer size in Y */
    unsigned int    dst_c_addr;  /* output address in C (Physical addr )*/
    unsigned int    dst_c_size;  /* output buffer size in C */
    unsigned int    linesize;    /* Length of one scanline (unit: pixel) for YC and RGB planes */
    unsigned int    dst_y_addr_user;  /* Y address in user space */
    unsigned int    dst_c_addr_user;  /* C address in user space */
    unsigned int    dec_to_x;         /* Decode to x (Unit: pixel) */
    unsigned int    dec_to_y;         /* Decode to y (Unit: pixel)*/
} jdec_input_t;

/*------------------------------------------------------------------------------
    Following structure is used for all HW decoder as output arguments
------------------------------------------------------------------------------*/
typedef struct {
    VD_IOCTL_CMD_M;
    vdo_framebuf_t   fb;
    unsigned int     y_addr_user;  /* Y address in user space */
    unsigned int     c_addr_user;  /* C address in user space */
    unsigned int     scaled;       /* This value should be equal to one of below
                                        SCALE_ORIGINAL = 0
                                        SCALE_HALF     = 1
                                        SCALE_QUARTER  = 2
                                        SCALE_EIGHTH   = 3
                                   */
} jdec_frameinfo_t;

/*------------------------------------------------------------------------------
    Following structure is used for VDIOSET_DECODE_LOOP
------------------------------------------------------------------------------*/
#define VD_DECODE_LOOP_M  \
    vd_scale_ratio  scale_factor;   /* Scale ratio */\
    vdo_color_fmt   dst_color;  /* Destination decoded color space */\
    unsigned char  *buf_in;\
    unsigned int    bufsize;\
    unsigned int    timeout  /* lock timeout */

typedef struct {
    VD_DECODE_LOOP_M;
} jdec_param_t;

/* Following structure was used for WMT API and driver internally */
typedef struct {
    VD_DECODE_LOOP_M;
    jdec_hdr_info_t  hdr;
    unsigned int     mmu_mode;     /* 0: Phys mode, 1: MMU mode */
    unsigned int     y_addr_user;  /* Y address in user space */
    unsigned int     c_addr_user;  /* C address in user space */
    unsigned int     dst_y_addr;  /* output address in Y (Physical addr )*/
    unsigned int     dst_y_size;  /* output buffer size in Y */
    unsigned int     dst_c_addr;  /* output address in C (Physical addr )*/
    unsigned int     dst_c_size;  /* output buffer size in C */
    unsigned int     scanline;    /* Scanline offset of FB (Unit: pixel) */
    jdec_pd          pd;          /* Settings for HW Partial decode */
    unsigned int     flags;       /* control flag for HW decoder */
    jdec_frameinfo_t jfb;
} jdec_dec_param_t;

/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef JPEG_COMMON_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/

#endif /* ifndef COM_JDEC_H */

/*=== END com-jdec.h ==========================================================*/

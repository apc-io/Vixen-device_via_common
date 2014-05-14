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
#ifndef COM_VD_H
/* To assert that only one occurrence is included */
#define COM_VD_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#ifndef COM_VIDEO_H
#ifdef __KERNEL__
    #include <mach/com-video.h>
#else
  #include "com-video.h"
#endif
#endif /* #ifdef COM_VIDEO_H */

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

/*------------------------------------------------------------------------------
    Following macros define hardware decoder type for vd_ioctl_cmd 
------------------------------------------------------------------------------*/
#define VD_JPEG    0x0001
#define VD_MAX     0x0002

#define IOCTL_CMD_INIT(cmd, _struct_type_, vd_type, version)

#define VD_IOCTL_CMD_M \
    unsigned int    identity; /* decoder type */\
    unsigned int    size      /* size of full structure */
/* End of VD_IOCTL_CMD_M */

/*------------------------------------------------------------------------------
    Definitions of Struct
------------------------------------------------------------------------------*/
typedef struct {
    VD_IOCTL_CMD_M;
} vd_ioctl_cmd;

typedef struct {
    int   vd_fd;
    int   mb_fd;
} vd_handle_t;

/*------------------------------------------------------------------------------
   Macros below are used for driver in IOCTL
------------------------------------------------------------------------------*/
#define VD_IOC_MAGIC              'k'
#define VD_IOC_MAXNR              1

/* VDIOSET_DECODE_LOOP: application send decode data to driver through the IOCTL
                   (blocking mode in driver)
*/
#define VDIOSET_DECODE_LOOP     _IOWR(VD_IOC_MAGIC, 0, vd_ioctl_cmd)

#endif /* ifndef COM_VD_H */

/*=== END com-vd.h ==========================================================*/

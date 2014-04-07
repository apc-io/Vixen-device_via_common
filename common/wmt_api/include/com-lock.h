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
 
#ifndef COM_LOCK_H
/* To assert that only one occurrence is included */
#define COM_LOCK_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/


/*Define wmt lock types*/
typedef enum 
{
    lock_jpeg = 0,  /* Lock for JPEG decoder */
    lock_video,     /* Lock for MSVD decoders */
    lock_encoder,   /* Lock for HW encoders */
    lock_max,
}wmt_lock_type;

typedef struct {
    long lock_type;    /* TYPE_JPEG or TYPE_VIDEO */
    long arg2;         /* for IO_WMT_UNLOCK, the timeout value */
    int  is_wait;      /* is someone wait for this lock? */
} wmt_lock_ioctl_arg;

/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMT_LOCK_C
    #define EXTERN
#else
    #define EXTERN   extern
#endif

#undef EXTERN

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/

/*------------------------------------------------------------------------------
   Macros below are used for driver in IOCTL
------------------------------------------------------------------------------*/
#define LOCK_IOC_MAGIC      'L'
#define IO_WMT_LOCK         _IOWR(LOCK_IOC_MAGIC, 0, unsigned long)
#define IO_WMT_UNLOCK       _IOWR(LOCK_IOC_MAGIC, 1, unsigned long)
#define IO_WMT_CHECK_WAIT   _IOR(LOCK_IOC_MAGIC, 2, unsigned long)

  
#endif /* ifndef COM_LOCK_H */

/*=== END com-lock.h ==========================================================*/

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
#ifndef WMT_LOG_H
/* To assert that only one occurrence is included */
#define WMT_LOG_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#include <stdarg.h>

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
#define WMT_LOG_FATAL   0
#define WMT_LOG_ERROR   4
#define WMT_LOG_WARNING 8
#define WMT_LOG_INFO    12
#define WMT_LOG_VERBOSE 16
#define WMT_LOG_DEBUG   20
#define WMT_LOG_DEBUG2  24
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#define WLOGE(...) wmt_log(WMT_LOG_ERROR, __VA_ARGS__)
#define WLOGW(...) wmt_log(WMT_LOG_WARNING, __VA_ARGS__)
#define WLOGI(...) wmt_log(WMT_LOG_INFO, __VA_ARGS__)
#define WLOGD(...) wmt_log(WMT_LOG_DEBUG, __VA_ARGS__)
#define WLOGV(...) wmt_log(WMT_LOG_DEBUG2, __VA_ARGS__)

/*	following is the C++ header	*/
#ifdef	__cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    API for performance analysis
------------------------------------------------------------------------------*/
void wmt_log(int level, const char *fmt, ...);
int wmt_log_get_level(void);
void wmt_log_set_level(int);
void wmt_log_set_callback(void (*)(int, const char*, va_list));
void wmt_log_default_callback(int level, const char* fmt, va_list vl);

#ifdef	__cplusplus
}
#endif	

#endif /* WMT_LOG_H */

/*=== END wmt-log.h ==========================================================*/

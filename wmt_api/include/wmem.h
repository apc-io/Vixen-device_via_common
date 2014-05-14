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
#ifndef WMT_MEM_H
#define WMT_MEM_H

#ifdef __GNUC__
#    define GCC_VERSION_FROM(x,y) (__GNUC__ > x || __GNUC__ == x && __GNUC_MINOR__ >= y)
#else
#    define GCC_VERSION_FROM(x,y) 0
#endif

#if GCC_VERSION_FROM(3,1)
    #define wmt_malloc_attrib __attribute__((__malloc__))
#else
    #define wmt_malloc_attrib
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int wmt_mem_init(const char *func, int ln);
int wmt_mem_exit(const char *func, int ln);
void *wmt_mem_malloc(unsigned int size, const char *func, int ln) wmt_malloc_attrib;
void *wmt_mem_mallocz(unsigned int size, const char *func, int ln) wmt_malloc_attrib;
void *wmt_mem_realloc(void *ptr, unsigned int size, const char *func, int ln) wmt_malloc_attrib;
void *wmt_mem_calloc(unsigned int nmemb, unsigned int size, const char *func, int ln) wmt_malloc_attrib;
char *wmt_mem_strdup(const char *s, const char *func, int ln) wmt_malloc_attrib;
void wmt_mem_free(void *ptr, const char *func, int ln);
void wmt_mem_freep(void *ptr, const char *func, int ln);

#define wmt_malloc(s) wmt_mem_malloc(s, __FUNCTION__, __LINE__)
#define wmt_mallocz(s) wmt_mem_mallocz(s, __FUNCTION__, __LINE__)
#define wmt_realloc(p,s) wmt_mem_realloc(p, s, __FUNCTION__, __LINE__)
#define wmt_calloc(n,s) wmt_mem_calloc(n, s, __FUNCTION__, __LINE__)
#define wmt_strdup(s) wmt_mem_strdup(s, __FUNCTION__, __LINE__)
#define wmt_free(p) wmt_mem_free(p, __FUNCTION__, __LINE__)
#define wmt_freep(p) wmt_mem_freep(p, __FUNCTION__, __LINE__)
#define wmt_minit() wmt_mem_init(__FUNCTION__, __LINE__)
#define wmt_mexit() wmt_mem_exit(__FUNCTION__, __LINE__)

//#define WMT_CONFIG_MEM_GUARD

#ifdef WMT_CONFIG_UT
#ifndef WMT_CONFIG_MEM_GUARD
#define WMT_CONFIG_MEM_GUARD
#endif
int wmt_mem_ut(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* WMT_MEM_H */

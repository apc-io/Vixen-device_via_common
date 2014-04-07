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
#ifndef WMTCODEC_H
#define WMTCODEC_H

#include <errno.h>
#include <stdint.h>
#include <linklist.h>

#define WMTCODEC_TOSTRING(s) #s
#define WMTCODEC_STRINGIFY(s) WMTCODEC_TOSTRING(s)

#define WMTCODEC_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#define WMTCODEC_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define WMTCODEC_VERSION(a, b, c)     WMTCODEC_VERSION_DOT(a, b, c)

#define LIBWMTCODEC_VERSION_MAJOR 0
#define LIBWMTCODEC_VERSION_MINOR 1
#define LIBWMTCODEC_VERSION_MICRO 0

#define LIBWMTCODEC_VERSION_INT  WMTCODEC_VERSION_INT(LIBWMTCODEC_VERSION_MAJOR, \
                                                 LIBWMTCODEC_VERSION_MINOR, \
                                                 LIBWMTCODEC_VERSION_MICRO)
#define LIBWMTCODEC_VERSION      WMTCODEC_VERSION(LIBWMTCODEC_VERSION_MAJOR, \
                                             LIBWMTCODEC_VERSION_MINOR, \
                                             LIBWMTCODEC_VERSION_MICRO)
#define LIBWMTCODEC_BUILD        LIBWMTCODEC_VERSION_INT
#define LIBWMTCODEC_IDENT        "Lwmtcodec." WMTCODEC_STRINGIFY(LIBWMTCODEC_VERSION)

#if EDOM > 0
#define WMTERROR(e) (-(e))
#else
#define WMTERROR(e) (e)
#endif

#define WMT_ALIGN(x,a) (((x)+(a)-1)&~((a)-1))
#define WMT_MAX(a,b) ((a) > (b) ? (a) : (b))
#define WMT_MIN(a,b) ((a) > (b) ? (b) : (a))

#define WMT_FOURCC(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef enum  {
    WMT_COLORFMT_Uused = 0,
    WMT_COLORFMT_16bitRGB565,
    WMT_COLORFMT_16bitGBR565,
    WMT_COLORFMT_24bitRGB888,
    WMT_COLORFMT_24bitGBR888,
    WMT_COLORFMT_32bitARGB8888,
    WMT_COLORFMT_32bitGBRA8888,
    WMT_COLORFMT_YUV411Planar,
    WMT_COLORFMT_YUV411SemiPlanar,
    WMT_COLORFMT_YUV420Planar,
    WMT_COLORFMT_YUV420SemiPlanar,
    WMT_COLORFMT_YUV422Planar,
    WMT_COLORFMT_YUV422SemiPlanar,
    WMT_COLORFMT_MAX,
} WMTColorFormat; 

enum {
    WMTFRAME_I_TYPE = 1,
    WMTFRAME_P_TYPE,
    WMTFRAME_B_TYPE,
};

typedef struct WMTRegion{
    int left,right;
    int top,bottom;
} WMTRegion;

typedef struct WMTFrame {
    uint8_t *data[4];
    uint32_t base[4];
    int linesize[4];
    WMTColorFormat format;
    WMTRegion cropping;
    int mbCount;
    int pictType;
    int width;
    int height;
    int key;
    int errMBs;
    int flags;
    void *opaque;
    int64_t decOrderSeqID;
    int64_t dispOrderSeqID;
    int frameID;
    int interlace;
    int aspectRateInfo;
    int picStrPresent;
    int pictureStructure;
    int topFieldFirst;
    int repeatFirstField;
    int fieldSequence;
    int progressiveFrame;
    int progressiveSequence;
    int picTimingStruct;
} WMTFrame;

typedef struct WMTFramePool {
    int count;
    WMTFrame *fbs;
} WMTFramePool;

enum {
    WMTCODEC_UNUSED = 0,
    WMTCODEC_JPEG_DECODER,
    WMTCODEC_MPEG_DECODER,
    WMTCODEC_H263_DECODER,
    WMTCODEC_DIVX_DECODER,
    WMTCODEC_MPEG4_DECODER,
    WMTCODEC_H264_DECODER,
    WMTCODEC_RV_DECODER,
    WMTCODEC_AVS_DECODER,
    WMTCODEC_VC1_DECODER,
    WMTCODEC_VPX_DECODER,
    WMTCODEC_DECODER_MAX,
    WMTCODEC_CODEC_MAX = WMTCODEC_DECODER_MAX,
};

typedef enum  {
    WMTCODEC_STATE_NONE = 0,
    WMTCODEC_STATE_INIT,
    WMTCODEC_STATE_SEQ_INIT,
    WMTCODEC_STATE_DECODED,
    WMTCODEC_STATE_FLUSH,
} WMTCodecState; 

typedef enum WMTCodecSkip{
    WMTCODEC_SKIP_NONE = 0,
    WMTCODEC_SKIP_BIDIR,
    WMTCODEC_SKIP_NONKEY,
    WMTCODEC_SKIP_ALL,
}WMTCodecSkip;

#define WMTCODEC_DECODE_FLAGS_KEY 0x1
#define WMTCODEC_DECODE_FLAGS_INTERLACE 0x2

#define WMTCODEC_FLAGS_EXTERNAL_MB 0x1
#define WMTCODEC_FLAGS_WITHOUT_REORDER 0x2
#define WMTCODEC_FLAGS_SINGLE_FRAMEBUFFER 0x4
#define WMTCODEC_FLAGS_MEET_EXTRA_FRAMEBUFFER 0x8
#define WMTCODEC_FLAGS_LARGE_FRAMEBUFFER 0x10
#define WMTCODEC_FLAGS_FRAMEBUFFER_CACHABLE 0x20

#define WMTCODEC_CONFIG_FRAMBUFFER_MAX_WIDTH  1920
#define WMTCODEC_CONFIG_FRAMBUFFER_MAX_HEIGHT 1088


typedef struct WMTCodecParam {
    int mbHandle;
    uint32_t codecTag;
    int extraDataSize;
    uint8_t *extraData;
    int width;
    int height;
    int fbWidth;
    int fbHeight;
    WMTColorFormat format;
    int extraFBs;
    int frameRateNum;
    int frameRateDen;
    int timeBaseNum;
    int timeBaseDen;
    int streaming;
    uint32_t flags;
}WMTCodecParam;

typedef struct WMTCodecContext {
    char name[16];
    int codecID;
    int mbHandle;
    WMTCodecState state;
    WMTCodecParam param;
    int64_t *curPts;
    LinkList ptsList;
    WMTCodecSkip skip;
    int frameNumber;
    int (*init)(struct WMTCodecContext *codec);
    int (*ready)(struct WMTCodecContext *codec);
    int (*decode)(struct WMTCodecContext *codec, uint8_t *data, int size, WMTFrame *frame, int *got, int flags);
    int (*flush)(struct WMTCodecContext *codec);
    int (*exit)(struct WMTCodecContext *codec);
    int (*get_buffer)(struct WMTCodecContext *codec, WMTFrame *frame);
    int (*release_buffer)(struct WMTCodecContext *codec, WMTFrame *frame);
} WMTCodecContext;

#ifdef  __cplusplus
extern "C" {
#endif

void wmt_codec_align_resolution(WMTCodecContext *codec,  int *width, int *height);
const char *wmt_codec_fmtstr(WMTColorFormat format);
unsigned int wmt_codec_version(void);
const char *wmt_codec_version_identity(void);
int wmt_codec_init(WMTCodecContext **pCodec, int codecid, WMTCodecParam *param);
int wmt_codec_exit(WMTCodecContext *codec);
int wmt_codec_flush(WMTCodecContext *codec);
int wmt_codec_decode(
    WMTCodecContext *codec, 
    uint8_t *data, int size,
    int64_t pts, int flags,
    WMTFrame *frame, int *got);

#ifdef  __cplusplus
}
#endif  

#endif /* ifndef WMTCODEC_H */

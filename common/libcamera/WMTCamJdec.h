/*
 * Copyright (c) 2008-2012 WonderMedia Technologies, Inc. All Rights Reserved.
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

#ifndef CAM_WMT_JDEC_H
#define CAM_WMT_JDEC_H

#include "wmtlock.h"
#include "wmt-vd.h"
#include "wmt-api.h"
#include "com-jdec.h"
#include "wmt-mb.h"
#include <utils/threads.h>
#include <hardware/camera.h>


extern "C" {
#undef EXTERN
#include "wmt-jdec.h"
}

// this is used for communicate btw hwlib and vgui
typedef struct hwjpeg_info_ {
    jdec_frameinfo_t  frameinfo;

    //input, either buffer or filename needs be decoded
    const void * buffer;
    int          buf_len;

    //wmt-api's output
    int          decode_ok;    
    
    unsigned int    img_w;    /* width of valid image (unit: pixel) */
    unsigned int    img_h;    /* height of valid image (unit: line) */
    unsigned int    fb_w;     /* width of frame buffer (scanline offset) (unit: pixel)*/
    unsigned int    fb_h;     /* height of frame buffer (unit: line) */
    unsigned int    y_addr_user;  /* Y address in user space */
    unsigned int    c_addr_user;  /* C address in user space */
}hwjpeg_info;

/* JPEG marker codes */
typedef enum {
    /* start of frame */
    SOF0  = 0xc0,	/* baseline */
    SOF1  = 0xc1,	/* extended sequential, huffman */
    SOF2  = 0xc2,	/* progressive, huffman */
    SOF3  = 0xc3,	/* lossless, huffman */

    SOF5  = 0xc5,	/* differential sequential, huffman */
    SOF6  = 0xc6,	/* differential progressive, huffman */
    SOF7  = 0xc7,	/* differential lossless, huffman */
    JPG   = 0xc8,	/* reserved for JPEG extension */
    SOF9  = 0xc9,	/* extended sequential, arithmetic */
    SOF10 = 0xca,	/* progressive, arithmetic */
    SOF11 = 0xcb,	/* lossless, arithmetic */

    SOF13 = 0xcd,	/* differential sequential, arithmetic */
    SOF14 = 0xce,	/* differential progressive, arithmetic */
    SOF15 = 0xcf,	/* differential lossless, arithmetic */

    DHT   = 0xc4,	/* define huffman tables */

    DAC   = 0xcc,	/* define arithmetic-coding conditioning */

    /* restart with modulo 8 count "m" */
    RST0  = 0xd0,
    RST1  = 0xd1,
    RST2  = 0xd2,
    RST3  = 0xd3,
    RST4  = 0xd4,
    RST5  = 0xd5,
    RST6  = 0xd6,
    RST7  = 0xd7,

    SOI   = 0xd8,	/* start of image */
    EOI   = 0xd9,	/* end of image */
    SOS   = 0xda,	/* start of scan */
    DQT   = 0xdb,	/* define quantization tables */
    DNL   = 0xdc,	/* define number of lines */
    DRI   = 0xdd,	/* define restart interval */
    DHP   = 0xde,	/* define hierarchical progression */
    EXP   = 0xdf,	/* expand reference components */

    APP0  = 0xe0,
    APP1  = 0xe1,
    APP2  = 0xe2,
    APP3  = 0xe3,
    APP4  = 0xe4,
    APP5  = 0xe5,
    APP6  = 0xe6,
    APP7  = 0xe7,
    APP8  = 0xe8,
    APP9  = 0xe9,
    APP10 = 0xea,
    APP11 = 0xeb,
    APP12 = 0xec,
    APP13 = 0xed,
    APP14 = 0xee,
    APP15 = 0xef,

    JPG0  = 0xf0,
    JPG1  = 0xf1,
    JPG2  = 0xf2,
    JPG3  = 0xf3,
    JPG4  = 0xf4,
    JPG5  = 0xf5,
    JPG6  = 0xf6,
    JPG7  = 0xf7,
    JPG8  = 0xf8,
    JPG9  = 0xf9,
    JPG10 = 0xfa,
    JPG11 = 0xfb,
    JPG12 = 0xfc,
    JPG13 = 0xfd,

    COM   = 0xfe,	/* comment */
    TEM   = 0x01,	/* temporary private use for arithmetic coding */

    /* 0x02 -> 0xbf reserved */
} JPEG_MARKER;

struct jdec_sync {
	unsigned int usedbyhal;
	unsigned int releasebyencoder;
	unsigned int vyaddr;
	unsigned int vcaddr;
	unsigned int yaddr;
	unsigned int encodetag;
	unsigned int fid;
};

struct record_pointer {
	unsigned int cameramem;
	unsigned int data;
};

#define RECBUFNUMs 9

namespace android {

class WMTCamJdec {

public:
    WMTCamJdec();
    ~WMTCamJdec();
	int hw_jpeg_init() ;
	int doHWJDec(unsigned char * bsbuf,unsigned int size, int thumb);
	void SetCamRecordBuf(char *buf);
	void close(void);
	int doMirror(char *src, char * dst, int w, int h, int degress);
	int doMirrorAll(int *src, int * dst, int w, int h);
	int doMirrorHeight(int *src, int * dst, int w, int h);
	int doMirrorWidth(char *src, char *dst, int w, int h);
	void releaseRecordingFrame(char *opaque);
	void setCallbacks(camera_request_memory get_memory);
	int NeonMirror(unsigned int source_y_addr, int w, int h);
	int getthumbsize(int w, int h, int *tw, int *th);
	void SWPost(unsigned int ysrc, unsigned int csrc, unsigned int w, unsigned int h, unsigned int fw);
	void SWMirrorPost(unsigned int ysrc, unsigned int csrc, unsigned int ydest, unsigned int w, unsigned int h, unsigned int fw);
	int CheckSWPatch(int w, int mode, unsigned int *size);
	int NeonMirror64_Jd_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h);
	int NeonMirror64_Jd_H_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h);
	int NeonMirror16(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h);
	int NeonMirror4(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h);	
	int NeonMirrorPreviewCallback64(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h);
	int NeonPreviewHMirror(unsigned int source_y_addr, unsigned int source_c_addr, int w, int h);
	int NeonPreview(unsigned int source_y_addr, unsigned int source_c_addr, int w, int h);
    int NeonMirror64_Jd_Cb_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, unsigned int callback_buf, int w, int h);


	unsigned int mPreviewCallbackbuf;
	char *mJdecDest;
	bool	mDirectREC;
	int mRotation;
	int mMirrorEnable;
	unsigned int mThumbW;
	unsigned int mThumbH;
private:

	char *getfreebuf(void);
	vd_handle_t jdec_handle;
	char *mCamRecordBuf;
	unsigned int mFrameIndex;
	mutable Mutex	mBufferLock;
	mutable Mutex	mRecBufferLock;
	mutable Mutex	mParamLock;
	camera_request_memory mGetMemoryCB;
	int mScaleFactor;
	unsigned int mPicW;
	unsigned int mPicH;	
	unsigned int mSWPathch;
	unsigned int *mMBpool;
	unsigned int getfreemb(unsigned int *phy);
	char *mSWPreviewBuf;
	bool mJdecinited;
};
}; // namespace android

#endif  /* CAM_WMT_JDEC_H */

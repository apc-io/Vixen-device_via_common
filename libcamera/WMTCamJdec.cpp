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

#undef LOG_TAG
#define LOG_TAG "WMTCamJdec"

#include <errno.h>
#include "WMTCamJdec.h"
#include <utils/Log.h>

#define SUCCESS 0
#define FAIL -1

static int mb_fd = -1;

namespace android {

WMTCamJdec::WMTCamJdec ()
{
    mDirectREC = false;
    mCamRecordBuf = NULL;
    mJdecDest = NULL;
    mMirrorEnable = 0;
    mRotation = 0;
    mPreviewCallbackbuf = 0;
    mSWPreviewBuf = NULL;
    mJdecinited = false;
}

WMTCamJdec::~WMTCamJdec ()
{
	if (mSWPreviewBuf)
		free(mSWPreviewBuf);
	close();
}

int WMTCamJdec::hw_jpeg_init() 
{
    static int inited = 0;
    static int initRet = -1;

    if(!inited) {
        if ((initRet = wmt_jdec_init(&jdec_handle)) != 0) {
            ALOGE("WMT VD_JPEG Initial FAIL %d!\n", initRet);
            return initRet;
        }
        inited = 1;
        mJdecinited = true;
    }
    return initRet;
}

void WMTCamJdec::SetCamRecordBuf(char *buf)
{
	struct jdec_sync *sync;
	unsigned int i;

	if (buf) {
		mFrameIndex = 0;
		memset(buf, 0, (RECBUFNUMs*sizeof(struct record_pointer)));
		mMBpool = (unsigned int *)calloc (1, 128);
		hw_jpeg_init();
		memset(mMBpool, 0, 128);
		for (i = 0; i < RECBUFNUMs*3; i+=3) {
			mMBpool[i] = (unsigned int)wmt_mb_alloc(jdec_handle.mb_fd, 1382400);
			usleep(5000);
			mMBpool[i+1] = (unsigned int)wmt_mb_user2phys(jdec_handle.mb_fd, (void *)mMBpool[i]);
		}
	} else {
		for (i = 0; i < RECBUFNUMs*3; i+=3) {
			wmt_mb_free(jdec_handle.mb_fd,(void *)mMBpool[i]); //fan
			usleep(5000);
		}
	}
	mCamRecordBuf = buf;
}

unsigned int WMTCamJdec::getfreemb(unsigned int *phy)
{
	unsigned int i;

	mBufferLock.lock();
	for (i = 2; i < RECBUFNUMs*3; i+=3) {
		if (mMBpool[i] == 0) {
			mMBpool[i] = 1;
			*phy = mMBpool[i-1];
			mBufferLock.unlock();
			return mMBpool[i-2];
		}
	}
	mBufferLock.unlock();
	ALOGE("\n\n Can not find free mb");
	return 0;
}

char *WMTCamJdec::getfreebuf(void)
{
	struct jdec_sync *sync;
	struct record_pointer *rec;
	int i;
	camera_memory_t* mem;

	rec = (struct record_pointer *)mCamRecordBuf;

	mBufferLock.lock();
	for (i = 1; i < RECBUFNUMs; i++) {
		if (rec[i].cameramem == 0) {
			mem = mGetMemoryCB(-1, sizeof(struct jdec_sync), 1, NULL);
			if (NULL == mem || NULL == mem->data) {
				ALOGE("%s: Memory failure when alloc mem", __FUNCTION__);
			} else {
				rec[i].cameramem = (unsigned int)mem;
				rec[i].data = (unsigned int)mem->data;
				rec[0].cameramem = (unsigned int)mem;
				rec[0].data = (unsigned int)mem->data;
				mBufferLock.unlock();
				return (char *)mem->data;
			}		
		}
	}
	mBufferLock.unlock();
	//ALOGE("Can not find a free frame");
	return NULL;
}

void WMTCamJdec::releaseRecordingFrame(char *opaque)
{

	struct jdec_sync *sync;
	struct record_pointer *rec;
	unsigned int i, addr ,j;
	camera_memory_t* mem;

	if (mDirectREC == false)
		return;

	rec = (struct record_pointer *)mCamRecordBuf;	
	if (!rec) {
		ALOGW("rec is NULL");
		return;
	}

	mBufferLock.lock();
	if (opaque) {
		addr = (unsigned int)opaque;		
		for (i = 1; i < RECBUFNUMs; i++) {
			if (rec[i].data == addr) {
				if (!rec[i].cameramem) {
					ALOGE("\nreleaseRecordingFrame error");
					return;
				}
				sync = (struct jdec_sync *)opaque;
				//wmt_mb_free(jdec_handle.mb_fd,(void *)sync->vyaddr);
				for (j = 0; j < RECBUFNUMs*2; j+=2) {
					if (mMBpool[j] == sync->vyaddr) {
						mMBpool[j+2] = 0;
						break;
					}
				}
				mem = (camera_memory_t*)rec[i].cameramem;
    			mem->release(mem);
    			mem = NULL;
    			mBufferLock.unlock();
    			memset(&rec[i], 0, sizeof(struct record_pointer));
    			return;
			}
		}
		ALOGE("release a buffer , but not used !!!!!!!!");
	}
	mBufferLock.unlock();
	ALOGE("release a NULL buffer  !!!!!!!!");
}

void WMTCamJdec::close(void)
{
    if (mJdecinited) {
        wmt_jdec_exit(&jdec_handle);
        mJdecinited = false;
    }
}

void WMTCamJdec::setCallbacks(camera_request_memory get_memory)
{
	mGetMemoryCB = get_memory;
}

int WMTCamJdec::CheckSWPatch(int w, int mode, unsigned int *size)
{

	mSWPathch = 0;
	if (mode == 1) {
		if (w&0x3F)
			mSWPathch = 1;
		mScaleFactor = 1;
	} else if (mode == 2) {
		if (mPicW&0x3F)
			mSWPathch = 1;
		mScaleFactor = 1;
		*size = mPicW*mPicH;
		//ALOGW("\n\npic = %dx%d",mPicW,mPicH);
	} else if (mode == 3) {
		if (mThumbW&0x3F)
			mSWPathch = 1;
		//ALOGW("\n\nthumb = %dx%d",mThumbW,mThumbH);
		switch(mPicW) {
		case 1280:
			mScaleFactor = 4;
			break;
		case 640:
			mScaleFactor = 2;
			break;
		default:
			mScaleFactor = 1;
			break;
		}
		*size = mThumbW*mThumbH;
	} else
		ALOGE("un-know mode = %d", mode);

	//ALOGW("mode = %d , mSWPathch = %d , mScaleFactor = %d , %d",mode, mSWPathch, mScaleFactor);
	return (int)mSWPathch;
}

int WMTCamJdec::getthumbsize(int w, int h, int *tw, int *th)
{	
	if (!w || !h)
		return 1;

	mPicW = w;
	mPicH = h;
	if (w == 1280) {
		*tw = 320;
		*th = 180;
	} else if (w == 640) {
		*tw = 320;
		*th = 240;
	} else if (w == 320) {
		*tw = 320;
		*th = 240;
	} else if (w == 176) {
		*tw = 176;
		*th = 144;
	} else {
		ALOGE("un-support picture %dx%d",w,h);
	}
	mThumbW = *tw;
	mThumbH = *th;
	//ALOGW("\n\n getthumbsize : pic %dx%d , thumb = %dx%d",w,h,*tw,*th);

	return 0;
}

int WMTCamJdec::doHWJDec(unsigned char * bsbuf,unsigned int filesize, int thumb) {

    int ret;
    int retry = 0;
	hwjpeg_info hwinfo;
	unsigned int i = 0, tempCb, tempCr, total;
	int *pCbCr;
	struct jdec_sync *sync;
	int needrelease = 1;
    char *source, *ump_c;
	int  ii;
    jdec_param_t  jparm;

	if (hw_jpeg_init())
		return -1;

	memset(&hwinfo, 0, sizeof(hwinfo));

    jdec_frameinfo_t  * const jfb = &hwinfo.frameinfo;
    IOCTL_JPEG_INIT(jfb, jdec_frameinfo_t);

    vd_scale_ratio scale_factor;

	scale_factor = SCALE_ORIGINAL;
    if (thumb) {
    	if (mScaleFactor == 2)
    		scale_factor = SCALE_HALF;
    	else if(mScaleFactor == 4)
    		scale_factor = SCALE_QUARTER;
    	else
    		ALOGE("un-support scale_factor = %d",mScaleFactor);
	}

    memset(&jparm, 0, sizeof(jdec_param_t));
    jparm.buf_in    = bsbuf;
    jparm.bufsize   = filesize;
    jparm.dst_color = VDO_COL_FMT_YUV420;
    jparm.scale_factor = scale_factor;
    jparm.timeout      = 30;

    while(retry < 20) {
	    ret = wmt_jdec_decode_loop(&jdec_handle, &jparm, jfb);
	    if( ret == 0 )
	    	break;

        retry++;
        usleep(1000);
		if (retry >= 20) {
			ALOGD("H/W jpeg decoder really busy.");
			return EBUSY;
		}
	}

    hwinfo.decode_ok = 1;
    hwinfo.img_w = jfb->fb.img_w;
    hwinfo.img_h = jfb->fb.img_h;
    hwinfo.fb_w  = jfb->fb.fb_w;
    hwinfo.fb_h  = jfb->fb.fb_h;
    hwinfo.y_addr_user = jfb->y_addr_user;
    hwinfo.c_addr_user = jfb->c_addr_user;

    /*--------------------------------------------------------------------------
        Step 3: Release resource
    --------------------------------------------------------------------------*/	
	if (hwinfo.decode_ok) {
		total = hwinfo.img_w*hwinfo.img_h;		
		source = mCamRecordBuf;
		if (!source) {
			if (mSWPathch) {
				SWPost((unsigned int)jfb->y_addr_user, (unsigned int)jfb->c_addr_user, hwinfo.img_w, hwinfo.img_h, hwinfo.fb_w);
				if (mPreviewCallbackbuf)
                    memcpy((char *)mPreviewCallbackbuf, mJdecDest, (hwinfo.img_w*hwinfo.img_h*12)/8);			
				goto out;
			}
			if (mPreviewCallbackbuf && mMirrorEnable) {
				if (hwinfo.img_w % 64 == 0)					
					NeonMirrorPreviewCallback64(jfb->y_addr_user, jfb->c_addr_user, 
								mPreviewCallbackbuf, (mPreviewCallbackbuf+total), 
								hwinfo.img_w, hwinfo.img_h);
				else
					ALOGE("\n\nsupport !!!!!!!!!!!!");
				goto out;
			}

			if (!(hwinfo.img_w&0x0F)) {
				if (mMirrorEnable)
					NeonPreviewHMirror(jfb->y_addr_user, jfb->c_addr_user, hwinfo.img_w, hwinfo.img_h);
				else
					NeonPreview(jfb->y_addr_user, jfb->c_addr_user, hwinfo.img_w, hwinfo.img_h);
			} else {
               memcpy(mJdecDest, (char *)jfb->y_addr_user, total);
               memcpy((char *)(mJdecDest + total), (char *)jfb->c_addr_user, total/2);
			   pCbCr = (int *)(mJdecDest+total);		
			   total = total >> 3;		
			   for(i = 0; i < total; i++) {
			   	 tempCb = tempCr = pCbCr [i];
			     pCbCr [i] = (((tempCr << 8) & 0xFF00FF00) |  ((tempCb >> 8) & 0x00FF00FF));
			   }
			}

		} else  {
			if (mDirectREC == true) {
				sync = (struct jdec_sync *)getfreebuf();
				if (!sync) {
					ALOGE("sync is NULL !!!!!!!!!!!!!!!!!!!");
				} else {
					if (mMirrorEnable) {
						i = 0;
						sync->vyaddr = getfreemb(&i);
						sync->vcaddr = sync->vyaddr+total;
						sync->yaddr = i;
						if (!sync->vyaddr)
							goto out;
						if (mSWPathch)
							SWMirrorPost(jfb->y_addr_user, jfb->c_addr_user, sync->vyaddr, hwinfo.img_w, hwinfo.img_h, hwinfo.fb_w);
						else {
						   if (mPreviewCallbackbuf){
								NeonMirror64_Jd_Cb_mirror(jfb->y_addr_user, jfb->c_addr_user, sync->vyaddr, sync->vcaddr, mPreviewCallbackbuf, hwinfo.img_w, hwinfo.img_h);
					    	} else{
						    	// Neon solution
						    	if (hwinfo.img_w % 64 == 0)					
                                	NeonMirror64_Jd_H_mirror(jfb->y_addr_user, jfb->c_addr_user, sync->vyaddr, sync->vcaddr, hwinfo.img_w, hwinfo.img_h);
								else if (hwinfo.img_w % 16 == 0)
						        	NeonMirror16(jfb->y_addr_user, jfb->c_addr_user, sync->vyaddr, sync->vcaddr, hwinfo.img_w, hwinfo.img_h);						
					        	else
                                	NeonMirror4(jfb->y_addr_user, jfb->c_addr_user, sync->vyaddr, sync->vcaddr, hwinfo.img_w, hwinfo.img_h);
							}
					    }
					} else {
						needrelease = 0;
						sync->vyaddr = (unsigned int)jfb->y_addr_user;
						sync->vcaddr = (unsigned int)jfb->c_addr_user;
						sync->yaddr = (unsigned int)wmt_mb_user2phys(jdec_handle.mb_fd, (void *)sync->vyaddr);
					}
					sync->fid = mFrameIndex;
					mFrameIndex++;
				}
			} else {
				source = mCamRecordBuf;
				SWPost((unsigned int)jfb->y_addr_user, (unsigned int)jfb->c_addr_user, hwinfo.img_w, hwinfo.img_h, hwinfo.fb_w);
				if (mMirrorEnable) {
					doMirror(mJdecDest, source, hwinfo.img_w, hwinfo.img_h, mRotation);
					if (mPreviewCallbackbuf)
					   	memcpy((char *)mPreviewCallbackbuf, mJdecDest, total+(total>>1));
				} else
					memcpy(source, mJdecDest, total+(total>>1));
			}
		}
	} else
		ALOGW("\n decode failed !!!!!!!!!!!!!!!!!!");
out:
	if (needrelease) {
		ret = wmt_jdec_fb_free(&jdec_handle, jfb);
		if (ret)
			ALOGE("wmt_jdec_fb_free error %d",ret);
	}

    return ret;
}

int WMTCamJdec::doMirror(char *src, char * dst, int w, int h, int degress)
{

	if (!mMirrorEnable) {
		return 0;
	}

	if ((src == NULL) || (dst == NULL)) {
 		ALOGE("doMirror , src or dst is NULL");
 		return -1;
	}

	if (degress == 0) {           // FLIP_H and ROT_0
		return doMirrorWidth(src, dst, w, h);;
	} else if (degress == 180) {   // FLIP_H and ROT_90
		return doMirrorWidth(src, dst, w, h);
	} else if (degress == 90) {  // FLIP_H and ROT_180
		return doMirrorHeight((int *)src, (int *)dst, w, h);
	} else if (degress == 270) {  // FLIP_H and ROT_270
		return doMirrorHeight((int *)src, (int *)dst, w, h);
	}

	return 0;
}

int WMTCamJdec::doMirrorAll(int *src, int * dst, int w, int h)
{
	int i,j;
	int tmp1, tmp2, tmp3;
	int ofs = 0;

	//put Y
	w = w >> 2;
	for (j = 0; j < h; j ++) {
		for (i = 0; i < w; i++) {
			tmp1 = tmp2 = tmp3 = src[i+(j*w)];
			tmp3 = (((tmp1 << 8) & 0xFF00FF00) | ((tmp2 >> 8) & 0x00FF00FF));
			tmp1 = tmp2 = tmp3;
			tmp3 = ( ((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF));
			dst[((h-1-j)*w)+(w-1-i)] = tmp3;
		}
	}
	//put C
	ofs = w*h;
	h = h>>1;
	for (j = 0; j < h; j ++) {
		for (i = 0; i < w; i++) {
			tmp1 = tmp2 = tmp3 = src[i+(j*w)+ofs];
			tmp3 = ( ((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF));
			dst[((h-1-j)*w)+(w-1-i)+ofs] = tmp3;
		}
	}

	return 0;
}

int WMTCamJdec::doMirrorHeight(int *src, int * dst, int w, int h)
{
	int i,j;
	int tmp1, tmp2, tmp3;
	int ofs = 0;

	//put Y
	w = w >> 2;
	for (j = 0; j < h; j ++) {
		for (i = 0; i < w; i++) {			
			dst[((h-1-j)*w)+i] = src[i+(j*w)];
		}
	}
	//put C
	ofs = w*h;
	h = h>>1;
	for (j = 0; j < h; j ++) {
		for (i = 0; i < w; i++) {
			dst[((h-1-j)*w)+i+ofs] = src[i+(j*w)+ofs];
		}
	}

	return 0;
}

int WMTCamJdec::doMirrorWidth(char *src, char *dst, int w, int h)
{
	int i,j;
	int tmp1, tmp2, tmp3;
	int ofs = 0;
	int *p_sc, *p_sy;
	int *p_dc, *p_dy;
	int ow = 0, dw = 0;


	//put Y
	//ofs = w*h;
	ow = w;
	w = w >> 2;
	dw = w-1;
	for (j = 0; j < h; j ++) {
		ofs = j*ow;
		p_sy = (int *)(src+ofs);
		p_dy = (int *)(dst+ofs);
		for (i = 0; i < w; i++) {
			tmp3 = p_sy[i];
			tmp1 = tmp2 = tmp3;
			tmp3 = (((tmp1 << 8) & 0xFF00FF00) | ((tmp2 >> 8) & 0x00FF00FF));
			tmp1 = tmp2 = tmp3;
			tmp3 = (((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF));
			p_dy[dw-i] = tmp3;
		}
	}

	//put C	
	p_sy = (int *)(src+(h*ow));
	p_dy = (int *)(dst+(h*ow));
	h = h>>1;	
	for (j = 0; j < h; j ++) {
		ofs = j*w;
		p_sc = (int *)(p_sy+ofs);
		p_dc = (int *)(p_dy+ofs);
		for (i = 0; i < w; i++) {
			tmp3 = p_sc[i];
			tmp1 = tmp2 = tmp3;
			tmp3 = (((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF));
			p_dc[dw-i] = tmp3;
		}
	}

	return 0;
}

void WMTCamJdec::SWPost(unsigned int ysrc, unsigned int csrc, unsigned int w, unsigned int h,unsigned int fw)
{
	unsigned int total = w*h;
	unsigned int *pCbCr;
	unsigned int i, tempCb, tempCr;
	unsigned int taddr = 0;

	if (mSWPreviewBuf == NULL)
		mSWPreviewBuf = (char *)calloc (1, (w*h*12)/8);

	taddr = (unsigned int)mSWPreviewBuf;

	if (w == fw) {
		memcpy((char *)taddr, (char *)ysrc, total);
		memcpy((char *)(taddr+total), (char *)csrc, (total>>1));
	} else {
		for (i = 0; i < h; i++) {
			memcpy((char *)(taddr+(i*w)), (char *)(ysrc+(i*fw)), w);
		}
		for (i = 0; i < h/2; i++) {
			memcpy((char *)(taddr+total+(i*w)), (char *)(csrc+(i*fw)), w);
		}
	}

	pCbCr = (unsigned int *)(taddr+total);
	total = total >> 3;
	for(i = 0; i < total; i++) {
    	tempCb = tempCr = pCbCr [i];
    	pCbCr [i] = (((tempCr << 8) & 0xFF00FF00) |  ((tempCb >> 8) & 0x00FF00FF));
  	}
	doMirror((char *)taddr, (char *)mJdecDest, w, h, 0);
}


void WMTCamJdec::SWMirrorPost(unsigned int ysrc, unsigned int csrc, unsigned int ydest, unsigned int w, unsigned int h,unsigned int fw)
{
	unsigned int total = w*h;
	unsigned int *pCbCr;
	unsigned int i, tempCb, tempCr;
	unsigned int taddr = 0;

	if (mSWPreviewBuf == NULL)
		mSWPreviewBuf = (char *)calloc (1, (w*h*12)/8);

	taddr = (unsigned int)mSWPreviewBuf;

	if (w == fw) {
		memcpy((char *)taddr, (char *)ysrc, total);
		memcpy((char *)(taddr+total), (char *)csrc, (total>>1));
	} else {
		for (i = 0; i < h; i++) {
			memcpy((char *)(taddr+(i*w)), (char *)(ysrc+(i*fw)), w);
		}
		for (i = 0; i < h/2; i++) {
			memcpy((char *)(taddr+total+(i*w)), (char *)(csrc+(i*fw)), w);
		}
	}

	doMirror((char *)taddr, (char *)ydest, w, h, 0);

	pCbCr = (unsigned int *)(taddr+total);
	total = total >> 3;
	for(i = 0; i < total; i++) {
    	tempCb = tempCr = pCbCr [i];
    	pCbCr [i] = (((tempCr << 8) & 0xFF00FF00) |  ((tempCb >> 8) & 0x00FF00FF));
  }
  doMirror((char *)taddr, (char *)mJdecDest, w, h, 0);
  
}

int WMTCamJdec::NeonPreviewHMirror(unsigned int source_y_addr, unsigned int source_c_addr, int w, int h)
{
    int ii, total;
	char *source, *ump_c, *ump_dest;
	total = w*h;



	if(!(w&0x1F)) {
		 for ( ii = 0 ; ii < (int)h ; ii ++)  {					   
				 source = (char *)(source_y_addr + ii*w);			   		 
				 ump_dest = (char *)(mJdecDest + ii*w); 		   
				 asm volatile ( 		   
					 "mov			r0,   %0		  \n\t" 			   
					 "mov			r1,   %1		  \n\t" 			   			   
					 "mov			r2,   %2		  \n\t" 
					 "add			r2,   r2,   %0	  \n\t"
					 ".Lo64_11HM:					  \n\t" 							   
					 "vldmia	r1!,  {d0-d7}	  \n\t" 	
					 // H mirror
			         "vrev64.8    d15,   d0	   \n\t"			 
			         "vrev64.8    d14,   d1	   \n\t"			  
			         "vrev64.8    d13,   d2	   \n\t"			  
			         "vrev64.8    d12,   d3	   \n\t"			  
			         "vrev64.8    d11,   d4	   \n\t"			  
			         "vrev64.8    d10,   d5	   \n\t"
					 "vrev64.8   d9,	 d6 		\n\t"							   
					 "vrev64.8   d8,	 d7 		\n\t"
					 // mJdecDest copy			   
					 "sub       r2,   r2,  #64 \n\t"
					 "vstmia	r2!,  {d8-d15}	  \n\t" 
					 "sub       r2,   r2,  #64 \n\t"
					 //--------------------------------------				   
					 "subs		r0,    r0,	  #64 \n\t" 			   
					 "bne			.Lo64_11HM			  \n\t" 				
					 :				
					 : "r" (w), "r" (source), "r"(ump_dest)				
					 : "r0", "r1", "r2", "r3"			   
				   );			 
			  } 	  			 
    	for ( ii = 0 ; ii < (int)(h/2) ; ii ++)	{				 
    		source = (char *)(source_c_addr + ii*w);				 
    		ump_c = (char *)(mJdecDest + ii*w + total);				 
    		asm volatile (				 
    			"mov		r0,   %0		   \n\t"				 
    			"mov		r1,   %1		   \n\t"				
    			"mov		r2,   %2		   \n\t"
    			"add		r2,   r2,   %0	  \n\t"
    			".L1H: 				   \n\t"				 
    			"vldmia 	r1!,  {d0-d7}	  \n\t" 			 
				// H mirror
				"vrev64.16	 d15,	d0	  \n\t" 			
				"vrev64.16	 d14,	d1	  \n\t" 			 
				"vrev64.16	 d13,	d2	  \n\t" 			 
				"vrev64.16	 d12,	d3	  \n\t" 			 
				"vrev64.16	 d11,	d4	  \n\t" 			 
				"vrev64.16	 d10,	d5	  \n\t"
				"vrev64.16   d9,		d6		   \n\t"							  
				"vrev64.16   d8,		d7		   \n\t"
			    //			 
			    "vrev16.8    q4,	  q4		 \n\t"				  			  
			    "vrev16.8    q5,	  q5		 \n\t"				  			  
			    "vrev16.8    q6,	  q6		 \n\t"				  			  
			    "vrev16.8    q7,	  q7		 \n\t"				 
			    //                
			    "sub       r2,   r2,  #64 \n\t"
    			"vstmia 	r2!,  {d8-d15}	  \n\t"
    			"sub       r2,   r2,  #64 \n\t"
    			//--------------------------------------				 
    			"subs		 r0,   r0,	 #64   \n\t"				 
    			"bne		 .L1H			   \n\t"				 
    			:				 
    			: "r" (w), "r" (source), "r" (ump_c) 			 
    			: "r0", "r1", "r2"			
    		);
    	}
    }
	else if (!(w&0x0F)){		
		for ( ii = 0 ; ii < (int)h ; ii ++)  {					  
				source = (char *)(source_y_addr + ii*w);					
				ump_dest = (char *)(mJdecDest + ii*w);			  
				asm volatile (			  
					"mov		   r0,	 %0 		 \n\t"				  
					"mov		   r1,	 %1 		 \n\t"							  
					"mov		   r2,	 %2 		 \n\t" 
					"add		   r2,	 r2,   %0	 \n\t"
					".Lo64_1HM:					 \n\t"								  
					"vldmia    r1!,  {d0-d1}	 \n\t"	   
					// H mirror
					"vrev64.8	  d15,	 d0 	  \n\t" 			
					"vrev64.8	  d14,	 d1 	  \n\t" 			 			 
					// mJdecDest copy			  
					"sub	   r2,	 r2,  #16 \n\t"
					"vstmia    r2!,  {d14-d15}	 \n\t" 
					"sub	   r2,	 r2,  #16 \n\t"
					//--------------------------------------				  
					"subs	   r0,	  r0,	 #16 \n\t"				  
					"bne		   .Lo64_1HM			 \n\t"				   
					:			   
					: "r" (w), "r" (source), "r"(ump_dest)			   
					: "r0", "r1", "r2", "r3"			  
				  );			
			 }		
		for ( ii = 0 ; ii < (int)(h/2) ; ii ++)	{				 
			source = (char *)(source_c_addr + ii*w);				 
			ump_c = (char *)(mJdecDest + ii*w + total);				 
			asm volatile (				 
				"mov		r0,   %0		   \n\t"				 
				"mov		r1,   %1		   \n\t"				
				"mov		r2,   %2		   \n\t"
				"add		r2,	  r2,     r0	 \n\t"
				".L16_1HMM  :					   \n\t"				 
				"vldmia 	r1!,  {d0-d1}	  \n\t" 			 
				// mJdecDest do CbCr				 
				"vrev32.16	q4,   q0		  \n\t" 							 
				"vstmia 	r2!,  {d14-d15}   \n\t" 
			    // mCamRecordBuf mirror											  			 
			    "vrev64.8   d15,   d0	   \n\t"							  
			    "vrev64.8   d14,   d1	   \n\t"
				// mJdecDest copy			  
				"sub	   r2,	 r2,  #16 \n\t"
				"vstmia    r2!,  {d14-d15}	 \n\t" 
				"sub	   r2,	 r2,  #16 \n\t"			    
				//--------------------------------------				 
				"subs		 r0,   r0,	 #16   \n\t"				 
				"bne		 .L16_1HMM 		   \n\t"				 
				:				 
				: "r" (w), "r" (source), "r" (ump_c) 			 
				: "r0", "r1", "r2"			
			);
		 }

	}

	
	return 0;
}

int WMTCamJdec::NeonPreview(unsigned int source_y_addr, unsigned int source_c_addr, int w, int h)
{
    int ii, total;
	char *source, *ump_c;

	total = w*h;
	if(!(w&0x1F)) {
    	asm volatile (
    		"mov		  r0,	%0		   \n\t"			   
    		"mov		  r1,	%1		   \n\t"			   
    		"mov		  r2,	%2		   \n\t"			   
    		".Lo0:						   \n\t"			   
			"vldmia 	 r1!,  {d0-d3}	  \n\t" 		   
			"vstmia 	 r2!,  {d0-d3}	  \n\t" 			   
    		"subs		  r0,	r0,   #32	\n\t"			   
    		"bne		  .Lo0			   \n\t"			   
    		:			   
    		: "r" (total), "r" ((char *)source_y_addr), "r" (mJdecDest)			   
    		: "r0", "r1", "r2"		 
    	);					 
    	for ( ii = 0 ; ii < (int)(h/2) ; ii ++)	{				 
    		source = (char *)(source_c_addr + ii*w);				 
    		ump_c = (char *)(mJdecDest + ii*w + total);				 
    		asm volatile (				 
    			"mov		r0,   %0		   \n\t"				 
    			"mov		r1,   %1		   \n\t"				
    			"mov		r2,   %2		   \n\t"				 
    			".L1  : 				   \n\t"				 
    			"vldmia 	r1!,  {d0-d7}	  \n\t" 			 
    			// mJdecDest do CbCr				 
    			"vrev32.8	q4,   q0		  \n\t" 			 
    			"vrev32.8	q5,   q1		  \n\t" 			 
    			"vrev32.8	q6,   q2		  \n\t" 			 
    			"vrev32.8	q7,   q3		  \n\t" 				 
    			"vstmia 	r2!,  {d8-d15}	  \n\t" 			 
    			//--------------------------------------				 
    			"subs		 r0,   r0,	 #64   \n\t"				 
    			"bne		 .L1			   \n\t"				 
    			:				 
    			: "r" (w), "r" (source), "r" (ump_c) 			 
    			: "r0", "r1", "r2"			
    		);
    	}
    }
	else if (!(w&0x0F)){
		asm volatile (			   
			"mov		  r0,	%0		   \n\t"			   
			"mov		  r1,	%1		   \n\t"			   
			"mov		  r2,	%2		   \n\t"			   
			".Lo16_0:						   \n\t"			   
			"vldmia 	 r1!,  {d0-d1}	  \n\t" 		   
			"vstmia 	 r2!,  {d0-d1}	  \n\t" 		   
			"subs		  r0,	r0,   #16	\n\t"			   
			"bne		  .Lo16_0			   \n\t"			   
			:			   
			: "r" (total), "r" ((char *)source_y_addr), "r" (mJdecDest)			   
			: "r0", "r1", "r2"	 
		);		 
		for ( ii = 0 ; ii < (int)(h/2) ; ii ++)	{				 
			source = (char *)(source_c_addr + ii*w);				 
			ump_c = (char *)(mJdecDest + ii*w + total);				 
			asm volatile (				 
				"mov		r0,   %0		   \n\t"				 
				"mov		r1,   %1		   \n\t"				
				"mov		r2,   %2		   \n\t"				 
				".L16_1  :					   \n\t"				 
				"vldmia 	r1!,  {d0-d1}	  \n\t" 			 
				// mJdecDest do CbCr				 
				"vrev32.8	q4,   q0		  \n\t" 							 
				"vstmia 	r2!,  {d14-d15}   \n\t" 			 
				//--------------------------------------				 
				"subs		 r0,   r0,	 #16   \n\t"				 
				"bne		 .L16_1 		   \n\t"				 
				:				 
				: "r" (w), "r" (source), "r" (ump_c) 			 
				: "r0", "r1", "r2"			
			);
		 }

	}
	return 0;
}

int WMTCamJdec::NeonMirror(unsigned int source_y_addr, int w, int h)
{
    int ii;
    char *source, *mb_dest;
    unsigned int source_c_addr = source_y_addr+(w*h);
	// assembly
	for ( ii = 0 ; ii < (int)h ; ii ++)  {			
		source = (char *)(source_y_addr + ii*w);		
		mb_dest = (char *)(mJdecDest + ii*w);
		asm volatile (				
			"mov		   r0,	 %0 		 \n\t"				
			"mov		   r1,	 %1 		 \n\t"				
			"mov		   r2,	 %2 		 \n\t"				
			"add		   r2,	 r2,	 r0  \n\t"				
			".LLo01:					 \n\t"				
			"vldmia    r1!,  {d0-d7}	 \n\t"				
			// mCamRecordBuf mirror 			
			"vrev64.8	 d15,	d0		 \n\t"				
			"vrev64.8	 d14,	d1		 \n\t"				
			"vrev64.8	 d13,	d2		 \n\t"				
			"vrev64.8	 d12,	d3		 \n\t"				
			"vrev64.8	 d11,	d4		 \n\t"				
			"vrev64.8	 d10,	d5		 \n\t"				
			"vrev64.8	 d9,	  d6		 \n\t"				
			"vrev64.8	 d8,	  d7		 \n\t"				
			"sub		   r2,	  r2,	 #64 \n\t"				
			"vstmia    r2!,   {d8-d15}	 \n\t"				
			"sub		   r2,	  r2,	 #64 \n\t"				
			//--------------------------------------			
			"subs	   r0,	  r0,	 #64 \n\t"				
			"bne		   .LLo01			 \n\t"				
			:			: "r" (w), "r" (source), "r" (mb_dest)				
			: "r0", "r1", "r2"			  );				 
		}		
	for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  {			
		source = (char *)(source_c_addr + ii*w);		
		mb_dest = (char *)(mJdecDest  + ii*w + w*h); 		
		asm volatile (				
			"mov		   r0,	  %0		 \n\t"				
			"mov		   r1,	  %1		 \n\t"				
			"mov		   r2,	  %2		 \n\t"				
			"add		   r2,	  r2,	 r0    \n\t"			
			".LLo02:					 \n\t"				
			"vldmia    r1!,   {d0-d7}	 \n\t"				
			// mCamRecordBuf mirror 														
			"vrev64.16	 d15,	d0		 \n\t"				
			"vrev64.16	 d14,	d1		 \n\t"				
			"vrev64.16	 d13,	d2		 \n\t"				
			"vrev64.16	 d12,	d3		 \n\t"				
			"vrev64.16	 d11,	d4		 \n\t"				
			"vrev64.16	 d10,	d5		 \n\t"				
			"vrev64.16	 d9,	  d6		 \n\t"				
			"vrev64.16	 d8,	  d7		 \n\t"				
			"sub		   r2,	  r2,	 #64 \n\t"				
			"vstmia    r2!,   {d8-d15}	 \n\t"				
			"sub		   r2,	  r2,	 #64 \n\t"				
			//--------------------------------------			
			"subs	   r0,	  r0,	 #64  \n\t" 			
			"bne		   .LLo02			  \n\t" 			
			:							
			: "r" (w), "r" (source), "r" (mb_dest)				
			: "r0", "r1", "r2"							  
		);					
	}
	return 0;
}


int WMTCamJdec::NeonMirror4(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h)
{
    int i, j;
    int *source, *mb_dest, *ump_dest;
	int tmp1, tmp2, tmp3, ofs, total, tmp_w, tmp_h;
 
	total = w*h;
    tmp_w = w >> 2;
	tmp_h = h >> 1 ;	

	//memcpy((char *)mJdecDest, (char *)source_y_addr, total);
	//memcpy((char *)(mJdecDest + total), (char *)source_c_addr, total/2);

	// assembly
	if(mRotation == 0 || mRotation == 180) {  //mRotation = 0, 180	
	    // mb y_address mirror
	    for (j = 0; j < h; j ++) {
		    ofs = j*w;
		    source = (int *)(source_y_addr + ofs);
		    mb_dest = (int *)(y_addr + ofs);
			//ump_dest = (int *)(mJdecDest + ofs);
		    for (i = 0; i < tmp_w; i++) {
			    tmp3 = source[i];
			    tmp1 = tmp2 = tmp3;
			    tmp3 = ((tmp1 << 8) & 0xFF00FF00) | ((tmp2 >> 8) & 0x00FF00FF);
			    tmp1 = tmp2 = tmp3;
			    tmp3 = ((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF);
			    mb_dest[tmp_w-1-i] = tmp3;
				ump_dest[i] = source[i];
		    }
	    }     
		// mb c_address mirror & mJdecDest copy, CbCr
	    for (j = 0; j < tmp_h; j ++) {
		    ofs = j*w;
		    source = (int *)(source_c_addr + ofs);
		    mb_dest = (int *)(c_addr + ofs);
			ump_dest = (int *)(mJdecDest + total + ofs);
		    for (i = 0; i < tmp_w; i++) {
			    tmp3 = source[i];
			    tmp1 = tmp2 = tmp3;
			    tmp3 = ((tmp1<<16) & 0xFFFF0000) | ((tmp2>>16)&0xFFFF);
			    mb_dest[tmp_w-1-i] = tmp3;
				// CbCr
				//tmp3 = source[i];
				//tmp1 = tmp2 = tmp3;
				tmp2 = ((tmp1 << 8) & 0xFF00FF00) |  ((tmp2 >> 8) & 0x00FF00FF);
				ump_dest[i] = tmp2;
		    }
	    }
	}
    else	{	//mRotation = 90, 270
	    // mb y_address mirror 
	    ofs = w*h;
	    w = w >> 2;
		source = (int *)(source_y_addr);
		mb_dest = (int *)(y_addr);
	    ump_dest = (int *)(mJdecDest);	    
	    for (j = 0; j < h; j ++) {
		    for (i = 0; i < w; i++) 			
			     mb_dest[((h-1-j)*w)+i] = ump_dest[i+(j*w)] = source[i+(j*w)];			
	    }	
		// mb c_address mirror & mJdecDest copy, CbCr
		source = (int *)(source_c_addr);
		mb_dest = (int *)(c_addr);	
		ump_dest = (int *)(mJdecDest + ofs);
		h = h >> 1;
		for ( j = 0 ; j < h ; j ++)  {			
		    for (i = 0; i < w; i++) {		
				 tmp3 = source[i+(j*w)];
 			     tmp1 = tmp2 = tmp3;
 			     mb_dest[((h-1-j)*w)+i] = tmp3;
				 tmp3 = ((tmp1 << 8) & 0xFF00FF00) |  ((tmp2 >> 8) & 0x00FF00FF);
				 ump_dest[i+(j*w)] = tmp3;
		    }
	    }
    }
    
	return 0;
}

int WMTCamJdec::NeonMirror16(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h)
{
    int ii;    
    char *source, *mb_dest, *ump_dest;

	for ( ii = 0 ; ii < (int)h ; ii ++)  { 					
		source = (char *)(source_y_addr + ii*w); 			
		mb_dest = (char *)(y_addr + ii*w); 			  
		ump_dest = (char *)(mJdecDest + ii*w);		
		asm volatile (			
			"mov			 r0,   %0		   \n\t"				
			"mov			 r1,   %1		   \n\t"				
			"mov			 r2,   %2		   \n\t"				
			"mov			 r3,   %3		   \n\t"				
			"add			 r2,   r2,	   r0  \n\t"				
			".Lo16_1:					   \n\t"								
			"vldmia	 r1!,  {d0-d1}	   \n\t"				
			// mJdecDest copy 			
			"vstmia	 r3!,  {d0-d1}	   \n\t"				
			// mCamRecordBuf mirror									  
			"vrev64.8    d15,   d0	   \n\t"				
			"vrev64.8    d14,   d1	   \n\t"								
			"sub			 r2,	r2,    #16 \n\t"				
			"vstmia	 r2!,	{d14-d15}   \n\t"				
			"sub			 r2,	r2,    #16 \n\t"	  
			//--------------------------------------					
			"subs 	 r0,	r0,    #16 \n\t"				
			"bne			 .Lo16_1			   \n\t"				 
			: 			 
			: "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				 
			: "r0", "r1", "r2", "r3"				
		);	
	  }

	  for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  { 					
		source = (char *)(source_c_addr + ii*w); 			
		mb_dest = (char *)(c_addr  + ii*w);			  
		ump_dest = (char *)(mJdecDest + ii*w + w*h);			
		asm volatile (			
			"mov			 r0,	%0		   \n\t"				
			"mov			 r1,	%1		   \n\t"				
			"mov			 r2,	%2		   \n\t"				
			"mov			 r3,	%3		   \n\t"				
			"add			 r2,	r2,    r0	 \n\t"				
			".Lo16_2:					   \n\t"							
			"vldmia	 r1!,	{d0-d1}    \n\t"				
			// mJdecDest do CbCr					  
			"vrev32.8    q4,	  q0		 \n\t"				 
			"vstmia	   r3!,   {d14-d15}	 \n\t"				  
			// mCamRecordBuf mirror											  
			"vrev64.16   d15,   d0	   \n\t"				
			"vrev64.16   d14,   d1	   \n\t"							   
			"sub			 r2,	r2,    #16 \n\t"				
			"vstmia	 r2!,	{d14-d15}   \n\t"				
			"sub			 r2,	r2,    #16 \n\t"								  
			//--------------------------------------					
			"subs 	 r0,	r0,    #16	\n\t"				
			"bne			 .Lo16_2				\n\t"			  
			: 			 
			: "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				 
			: "r0", "r1", "r2", "r3"				
		);			  
	}

	return 0;
}

int WMTCamJdec::NeonMirrorPreviewCallback64(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h)
{
    int ii;
    char *source, *mb_dest, *ump_dest;
	  for ( ii = 0 ; ii < (int)h ; ii ++)  { 					
		  source = (char *)(source_y_addr + ii*w); 			
		  mb_dest = (char *)(y_addr + ii*w); 			  
		  ump_dest = (char *)(mJdecDest + ii*w);			
		  asm volatile (			
			  "mov			 r0,   %0		   \n\t"				
			  "mov			 r1,   %1		   \n\t"				
			  "mov			 r2,   %2		   \n\t"				
			  "mov			 r3,   %3		   \n\t"
			  "add           r3,   r3,  %0     \n\t"
			  "add			 r2,   r2,	%0  \n\t"				
			  ".Lo64_011:					   \n\t"								
			  "vldmia	 r1!,  {d0-d7}	   \n\t"								
              // H mirror
			  "vrev64.8    d15,   d0	   \n\t"			 
			  "vrev64.8    d14,   d1	   \n\t"			  
			  "vrev64.8    d13,   d2	   \n\t"			  
			  "vrev64.8    d12,   d3	   \n\t"			  
			  "vrev64.8    d11,   d4	   \n\t"			  
			  "vrev64.8    d10,   d5	   \n\t"
			  "vrev64.8   d9,	 d6 		\n\t"							   
			  "vrev64.8   d8,	 d7 		\n\t"              
		      // mJdecDest copy 			
		      "sub         r3,   r3,   #64 \n\t"
		      "vstmia	 r3!,  {d8-d15}	   \n\t"
			  "sub		   r3,	 r3,   #64 \n\t"	  
		      "sub         r2,   r2,   #64 \n\t"
		      "vstmia	 r2!,  {d8-d15}	   \n\t"
			  "sub		   r2,	 r2,   #64 \n\t"			  
			  //--------------------------------------					
			  "subs 	 r0,	r0,    #64 \n\t"				
			  "bne			 .Lo64_011			   \n\t"				 
			  : 			 
			  : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				 
			  : "r0", "r1", "r2", "r3"				
			);			  
	   }					
	  for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  { 					
		  source = (char *)(source_c_addr + ii*w); 			
		  mb_dest = (char *)(c_addr  + ii*w);			  
		  ump_dest = (char *)(mJdecDest + ii*w + w*h);			
		  asm volatile (			
			  "mov			 r0,	%0		   \n\t"				
			  "mov			 r1,	%1		   \n\t"				
			  "mov			 r2,	%2		   \n\t"				
			  "mov			 r3,	%3		   \n\t"
			  "add           r3,   r3,  %0     \n\t"
			  "add			 r2,	r2, %0	   \n\t"				
			  ".Lo64_021:					   \n\t"							
			  "vldmia	 r1!,	{d0-d7}    \n\t"				
    		  // H mirror
    		  "vrev64.16   d15,   d0	\n\t"			  
    		  "vrev64.16   d14,   d1	\n\t"			   
    		  "vrev64.16   d13,   d2	\n\t"			   
    		  "vrev64.16   d12,   d3	\n\t"			   
    		  "vrev64.16   d11,   d4	\n\t"			   
    		  "vrev64.16   d10,   d5	\n\t"
    		  "vrev64.16   d9,		  d6		 \n\t"								
    		  "vrev64.16   d8,		  d7		 \n\t"	
    		  // CbCr
			  "vrev16.8    q4,	  q4		 \n\t"				  
			  "vrev16.8    q5,	  q5		 \n\t"				  
			  "vrev16.8    q6,	  q6		 \n\t"				  
			  "vrev16.8    q7,	  q7		 \n\t"
		  
    		  "sub		 r3,   r3,	#64 \n\t"
    		  "vstmia	  r3!,	{d8-d15}	\n\t"
    		  "sub		 r3,   r3,	#64 \n\t"		
    		  "sub		 r2,   r2,	#64 \n\t"
    		  "vstmia	  r2!,	{d8-d15}	\n\t"
    		  "sub		 r2,   r2,	#64 \n\t"    		  
			  //--------------------------------------					
			  "subs 	 r0,	r0,    #64	\n\t"				
			  "bne			 .Lo64_021				\n\t"	  
			  
			  : 			 
			  : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				 
			  : "r0", "r1", "r2", "r3"				
			);			  
		  }
	return 0;
}

/*
** mJdecDest -> (y)mirror (c) mirror & cbcr
** CamRecord -> (y)mirror (c) mirror 
*/
int WMTCamJdec::NeonMirror64_Jd_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h)
{
    int ii;
    char *source, *mb_dest, *ump_dest;
	// assembly
    for (ii = 0; ii < (int)h; ii ++) {
		 source = (char *)(source_y_addr + ii*w);			   
		 mb_dest = (char *)(y_addr + (h-1-ii)*w); 			 
		 ump_dest = (char *)(mJdecDest + (h-1-ii)*w); 		   
		 asm volatile ( 		   
			 "mov			r0,   %0		  \n\t" 			   
			 "mov			r1,   %1		  \n\t" 			   
			 "mov			r2,   %2		  \n\t" 			   
			 "mov			r3,   %3		  \n\t" 						   
			 ".Lo64_11MM:					  \n\t" 							   
			 "vldmia	r1!,  {d0-d7}	  \n\t" 			   
			 // mJdecDest copy			   
			 "vstmia	r3!,  {d0-d7}	  \n\t" 			   
			 // mCamRecordBuf up_side down					   
			 "vstmia	r2!,  {d0-d7}	  \n\t" 							   
			 //--------------------------------------				   
			 "subs		r0,    r0,	  #64 \n\t" 			   
			 "bne			.Lo64_11MM			  \n\t" 				
			 :				
			 : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				
			 : "r0", "r1", "r2", "r3"			   
		   );			 
	 } 	  
	 for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  {					   
		 source = (char *)(source_c_addr + ii*w);			   
		 mb_dest = (char *)(c_addr	+ ((h/2)-1-ii)*w);			 
		 ump_dest = (char *)(mJdecDest + ((h/2)-1-ii)*w + w*h); 		   
		 asm volatile ( 		   
			 "mov			r0,    %0		  \n\t" 			   
			 "mov			r1,    %1		  \n\t" 			   
			 "mov			r2,    %2		  \n\t" 			   
			 "mov			r3,    %3		  \n\t" 					   
			 ".Lo64_12MM:					  \n\t" 						   
			 "vldmia	r1!,   {d0-d7}	  \n\t" 			   
			 // mJdecDest do CbCr					 
			 "vrev32.8	  q4,	 q0 		\n\t"				 
			 "vrev32.8	  q5,	 q1 		\n\t"				 
			 "vrev32.8	  q6,	 q2 		\n\t"				 
			 "vrev32.8	  q7,	 q3 		\n\t"				 
			 "vstmia	  r3!,	 {d8-d15}	\n\t"				 
			 // mCamRecordBuf up_side down							   
			 "vstmia	r2!,   {d0-d7}	  \n\t" 								   
			 //--------------------------------------				   
			 "subs		r0,    r0,	  #64  \n\t"			   
			 "bne		.Lo64_12MM			   \n\t"				
			 :				
			 : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)				
			 : "r0", "r1", "r2", "r3"			   
		   );			 
		}
	return 0;
}

int WMTCamJdec::NeonMirror64_Jd_H_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, int w, int h)
{
    int ii, total;
    char *source, *mb_dest, *ump_dest;
	total = w*h;
	// assembly
	 for ( ii = 0 ; ii < (int)h ; ii ++)  { 				   
			 source = (char *)(source_y_addr + ii*w);
			 mb_dest = (char *)(y_addr + ii*w); 
			 ump_dest = (char *)(mJdecDest + ii*w); 		   
			 asm volatile (
				 "mov			r0,   %0		  \n\t" 			   
				 "mov			r1,   %1		  \n\t" 						   
				 "mov			r2,   %2		  \n\t" 
				 "mov			r3,   %3		  \n\t" 
				 "add			r2,   r2,	%0	  \n\t"
				 "add			r3,   r3,	%0	  \n\t"
				 ".Lo64_JD_HM:					  \n\t" 							   
				 "vldmia	r1!,  {d0-d7}	  \n\t" 	
				 // H mirror
				 "vrev64.8    d15,   d0	   \n\t"			 
				 "vrev64.8    d14,   d1	   \n\t"			  
				 "vrev64.8    d13,   d2	   \n\t"			  
				 "vrev64.8    d12,   d3	   \n\t"			  
				 "vrev64.8    d11,   d4	   \n\t"			  
				 "vrev64.8    d10,   d5	   \n\t"
				 "vrev64.8   d9,	 d6 		\n\t"							   
				 "vrev64.8   d8,	 d7 		\n\t"
				 // mJdecDest copy			   
				 "sub		r2,   r2,  #64 \n\t"
				 "vstmia	r2!,  {d8-d15}	  \n\t" 
				 "sub		r2,   r2,  #64 \n\t"
				 // mbDest copy	 
				 "sub		r3,   r3,  #64 \n\t"
				 "vstmia	r3!,  {d8-d15}	  \n\t" 
				 "sub		r3,   r3,  #64 \n\t"			 
				 //--------------------------------------				   
				 "subs		r0,    r0,	  #64 \n\t" 			   
				 "bne			.Lo64_JD_HM			  \n\t" 				
				 :				
				 : "r" (w), "r" (source), "r"(ump_dest), "r" (mb_dest) 			
				 : "r0", "r1", "r2", "r3"			   
			   );			 
		  } 				 
	for ( ii = 0 ; ii < (int)(h/2) ; ii ++) {				 
		source = (char *)(source_c_addr + ii*w);	
		mb_dest = (char *)(c_addr + ii*w); 
		ump_dest = (char *)(mJdecDest + ii*w + total); 			 
		asm volatile (				 
			"mov		r0,   %0		   \n\t"				 
			"mov		r1,   %1		   \n\t"				
			"mov		r2,   %2		   \n\t"
			"mov		r3,   %3		   \n\t"
			"add		r2,   r2,	%0	  \n\t"
			"add		r3,   r3,	%0	  \n\t"
			".L1_JD_HM:				   \n\t"				 
			"vldmia 	r1!,  {d0-d7}	  \n\t" 			 
			// H mirror
			"vrev64.16	 d15,	d0	  \n\t" 			
			"vrev64.16	 d14,	d1	  \n\t" 			 
			"vrev64.16	 d13,	d2	  \n\t" 			 
			"vrev64.16	 d12,	d3	  \n\t" 			 
			"vrev64.16	 d11,	d4	  \n\t" 			 
			"vrev64.16	 d10,	d5	  \n\t"
			"vrev64.16	 d9,		d6		   \n\t"							  
			"vrev64.16	 d8,		d7		   \n\t"
			"sub	   r3,	 r3,  #64 \n\t"
			"vstmia 	r3!,  {d8-d15}	  \n\t"
			"sub	   r3,	 r3,  #64 \n\t"				
			//			 
			"vrev16.8	 q4,	  q4		 \n\t"							  
			"vrev16.8	 q5,	  q5		 \n\t"							  
			"vrev16.8	 q6,	  q6		 \n\t"							  
			"vrev16.8	 q7,	  q7		 \n\t"				 
			//				  
			"sub	   r2,	 r2,  #64 \n\t"
			"vstmia 	r2!,  {d8-d15}	  \n\t"
			"sub	   r2,	 r2,  #64 \n\t"			
			//--------------------------------------				 
			"subs		 r0,   r0,	 #64   \n\t"				 
			"bne		 .L1_JD_HM			   \n\t"				 
			:				 
			: "r" (w), "r" (source), "r" (ump_dest), "r" (mb_dest) 		 
			: "r0", "r1", "r2", "r3"			
		);
	}

	return 0;
}

int WMTCamJdec::NeonMirror64_Jd_Cb_mirror(unsigned int source_y_addr, unsigned int source_c_addr, unsigned int y_addr, unsigned int c_addr, unsigned int callback_buf, int w, int h)
{
    int ii, total;
    char *source, *mb_dest, *ump_dest, *tmp_call_back;
	total = w*h;
	// assembly
	if(mRotation == 0 || mRotation == 180) {  //mRotation = 0, 180
	  for ( ii = 0 ; ii < (int)h ; ii ++)  { 					
		  source = (char *)(source_y_addr + ii*w); 			
		  mb_dest = (char *)(y_addr + ii*w); 			  
		  ump_dest = (char *)(mJdecDest + (h-1-ii)*w);
		  tmp_call_back = (char *)(callback_buf + (h-1-ii)*w);
		  asm volatile (			
			  "mov			 r0,   %0		   \n\t"				
			  "mov			 r1,   %1		   \n\t"				
			  "mov			 r2,   %2		   \n\t"				
			  "mov			 r3,   %3		   \n\t"
			  "mov			 r4,   %4		   \n\t"
			  "add			 r2,   r2,	   r0  \n\t"	
			  "add			 r4,   r4,	   r0  \n\t"
			  ".Lo64_01MB:					   \n\t"								
			  "vldmia	 r1!,  {d0-d7}	   \n\t"				
			  // mJdecDest copy 			
			  "vstmia	 r3!,  {d0-d7}	   \n\t"				
			  // mCamRecordBuf mirror									  
			  "vrev64.8    d15,   d0	   \n\t"				
			  "vrev64.8    d14,   d1	   \n\t"				
			  "vrev64.8    d13,   d2	   \n\t"				
			  "vrev64.8    d12,   d3	   \n\t"				
			  "vrev64.8    d11,   d4	   \n\t"				
			  "vrev64.8    d10,   d5	   \n\t"				
			  "vrev64.8    d9,	  d6	   \n\t"				
			  "vrev64.8    d8,	  d7	   \n\t"					
			  "sub			 r2,	r2,    #64 \n\t"				
			  "vstmia	 r2!,	{d8-d15}   \n\t"				
			  "sub			 r2,	r2,    #64 \n\t"
			  "sub			 r4,	r4,    #64 \n\t"				
			  "vstmia	 r4!,	{d8-d15}   \n\t"				
			  "sub			 r4,	r4,    #64 \n\t" 
			  //--------------------------------------					
			  "subs 	 r0,	r0,    #64 \n\t"				
			  "bne			 .Lo64_01MB			   \n\t"				 
			  : 			 
			  : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest), "r"(tmp_call_back)				 
			  : "r0", "r1", "r2", "r3", "r4"				
			);			  
	   }					
	  for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  { 					
		  source = (char *)(source_c_addr + ii*w); 			
		  mb_dest = (char *)(c_addr  + ii*w);			  
		  ump_dest = (char *)(mJdecDest + ((h/2)-1-ii)*w + w*h);
		  tmp_call_back = (char *)(callback_buf + (h-1-ii)*w + total);
		  asm volatile (			
			  "mov			 r0,	%0		   \n\t"				
			  "mov			 r1,	%1		   \n\t"				
			  "mov			 r2,	%2		   \n\t"				
			  "mov			 r3,	%3		   \n\t"
			  "mov			 r4,   %4		   \n\t"
			  "add			 r2,	r2,    r0	 \n\t"	
			  "add			 r4,	r4,    r0	 \n\t"
			  ".Lo64_02MB:					   \n\t"							
			  "vldmia	 r1!,	{d0-d7}    \n\t"				
			  // mJdecDest do CbCr					  
			  "vrev32.8    q4,	  q0		 \n\t"				  
			  "vrev32.8    q5,	  q1		 \n\t"				  
			  "vrev32.8    q6,	  q2		 \n\t"				  
			  "vrev32.8    q7,	  q3		 \n\t"				  
			  "vstmia	   r3!,   {d8-d15}	 \n\t"				  
			  // mCamRecordBuf mirror											  
			  "vrev64.16   d15,   d0	   \n\t"				
			  "vrev64.16   d14,   d1	   \n\t"				
			  "vrev64.16   d13,   d2	   \n\t"				
			  "vrev64.16   d12,   d3	   \n\t"				
			  "vrev64.16   d11,   d4	   \n\t"				
			  "vrev64.16   d10,   d5	   \n\t"				
			  "vrev64.16   d9,		d6		   \n\t"				
			  "vrev64.16   d8,		d7		   \n\t"						   
			  "sub			 r2,	r2,    #64 \n\t"				
			  "vstmia	 r2!,	{d8-d15}   \n\t"				
			  "sub			 r2,	r2,    #64 \n\t"	
			  // callback buff
		      "vrev32.8    q4,	  q4		 \n\t"				  
		      "vrev32.8    q5,	  q5		 \n\t"				  
		      "vrev32.8    q6,	  q6		 \n\t"				  
		      "vrev32.8    q7,	  q7		 \n\t"	
			  "sub			 r4,	r4,    #64 \n\t"				
			  "vstmia	 r4!,	{d8-d15}   \n\t"				
			  "sub			 r4,	r4,    #64 \n\t"
	  
			  //--------------------------------------					
			  "subs 	 r0,	r0,    #64	\n\t"				
			  "bne			 .Lo64_02MB				\n\t"	  
			  
			  : 			 
			  : "r" (w), "r" (source), "r" (mb_dest), "r"(ump_dest), "r"(tmp_call_back)			 
			  : "r0", "r1", "r2", "r3", "r4"				
			);			  
		  }
	  }
	  else	{	//mRotation = 90, 270
		 for ( ii = 0 ; ii < (int)h ; ii ++)  {					   
				 source = (char *)(source_y_addr + ii*w);			   
				 mb_dest = (char *)(y_addr + (h-1-ii)*w); 			 
				 ump_dest = (char *)(mJdecDest + (h-1-ii)*w);
				 tmp_call_back = (char *)(callback_buf + (h-1-ii)*w);
				 asm volatile ( 		   
					 "mov			r0,   %0		  \n\t" 			   
					 "mov			r1,   %1		  \n\t" 			   
					 "mov			r2,   %2		  \n\t" 			   
					 "mov			r3,   %3		  \n\t" 
					 "mov			r4,   %4		  \n\t"
					 ".Lo64_11MMB:					  \n\t" 							   
					 "vldmia	r1!,  {d0-d7}	  \n\t" 			   
					 // mJdecDest copy			   
					 "vstmia	r3!,  {d0-d7}	  \n\t" 			   
					 // mCamRecordBuf up_side down					   
					 "vstmia	r2!,  {d0-d7}	  \n\t" 
					 "vstmia	r4!,  {d0-d7}	  \n\t" 
					 //--------------------------------------				   
					 "subs		r0,    r0,	  #64 \n\t" 			   
					 "bne			.Lo64_11MMB			  \n\t" 				
					 :				
					 : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)	, "r"(tmp_call_back)					
					 : "r0", "r1", "r2", "r3", "r4"			   
				   );			 
			  } 	  
			 for ( ii = 0 ; ii < (int)(h/2) ; ii ++)  {					   
				 source = (char *)(source_c_addr + ii*w);			   
				 mb_dest = (char *)(c_addr	+ ((h/2)-1-ii)*w);			 
				 ump_dest = (char *)(mJdecDest + ((h/2)-1-ii)*w + w*h);
				 tmp_call_back = (char *)(callback_buf + ((h/2)-1-ii)*w + w*h);
				 asm volatile ( 
					 "mov			r0,    %0		  \n\t" 			   
					 "mov			r1,    %1		  \n\t" 			   
					 "mov			r2,    %2		  \n\t" 			   
					 "mov			r3,    %3		  \n\t"
					 "mov			r4,    %4		  \n\t"
					 ".Lo64_12MMB:					  \n\t" 						   
					 "vldmia	r1!,   {d0-d7}	  \n\t" 			   
					 // mJdecDest do CbCr					 
					 "vrev32.8	  q4,	 q0 		\n\t"				 
					 "vrev32.8	  q5,	 q1 		\n\t"				 
					 "vrev32.8	  q6,	 q2 		\n\t"				 
					 "vrev32.8	  q7,	 q3 		\n\t"				 
					 "vstmia	  r3!,	 {d8-d15}	\n\t"
					 // callback buffer
					  "vstmia	  r4!,	 {d8-d15}	\n\t"
					 // mCamRecordBuf up_side down							   
					 "vstmia	r2!,   {d0-d7}	  \n\t" 								   
					 //--------------------------------------				   
					 "subs		r0,    r0,	  #64  \n\t"			   
					 "bne		.Lo64_12MMB			   \n\t"				
					 :				
					 : "r" (w), "r" (source), "r" (mb_dest) , "r"(ump_dest)	, "r"(tmp_call_back)					
					 : "r0", "r1", "r2", "r3", "r4"			   
				   );			 
				}
	  }
	return 0;


}


}

// vim: et ts=4 shiftwidth=4


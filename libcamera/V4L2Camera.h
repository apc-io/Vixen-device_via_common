/*
 * Copyright 2013 WonderMedia Technologies, Inc. All Rights Reserved. 
 *  
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc. 
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any third party, 
 * in whole or in part, without prior written consent of WonderMedia. 
 *  
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 * WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, 
 * AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  
 */


#ifndef WONDERMEDIA_V4L2CAMERA_H_INCLUDED
#define WONDERMEDIA_V4L2CAMERA_H_INCLUDED

#define NB_BUFFER 6

#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <linux/videodev.h>
#include <hardware/camera.h>
#include <utils/threads.h>
#include "JpegCompressor.h"
#undef EXTERN   //avoid warning: "EXTERN" redefined
#include "WMTCamJdec.h"

#include <linux/videodev.h>
#include <linux/videodev2.h>
#include "UbootParam.h"

#define YUV420 1
#define YUV422 0

struct vdIn;        // forward declaration is enough [Vic]

namespace android {

class V4L2Camera {

public:
    V4L2Camera(camera_property*  p);
    ~V4L2Camera();

    int     open();
    void    close();
    bool    isOpened() const;

    void    term(int count);

    int v4l2_ioctl_s_ctrl(uint32_t id, int value);
    int v4l2_ioctl_g_ctrl(uint32_t id, int *rvalue);
	int v4l2_ioctl_queycap(void);
	int v4l2_ioctl_s_fmt(int width, int height, unsigned int pixelformat);
    int v4l2_ioctl_enum_framesizes(int index, int *width, int *height);
    int v4l2_ioctl_queryctrl(struct v4l2_queryctrl *qc);
    int v4l2_ioctl_querymenu(struct v4l2_querymenu *qm);
	int v4l2_ioctl_reqbufs(int count);
	int v4l2_ioctl_querybuf(int count);
	int v4l2_ioctl_qbuf(int count);
    int v4l2_ioctl_dqbuf(int count);

    int v4l2_enuminput(int index);
    int v4l2_s_input(int index);

    int startStreaming();
    int stopStreaming();

    int startPreview(int width, int height, int pixelformat);
    void stopPreview();

	int startSnap(int w, int h, int delay);
    int stopSnap(void);

	void convert(unsigned char *buf, unsigned char *rgb, int width, int height);    
	void setRecordBuf(char *add);
	int getPreviewSize(void);
	int grabPreviewFrame(char *recordBuffer, gralloc_mb_block *block);
	int Preview2Picture(char *recordBuffer);
	int getPictureSize(void);
	int getPictureRaw(char *jpegbuf, char *thumbraw);
   	void swCropImagetoYUV420(char *src, char *dest, int scr_w, int src_h, int src_f, int dst_w, int dst_h);	
	void setMirror(int enable);
	int swSetFrameRate(int rate);
	void setRotation(int degress);
	void releaseRecordingFrame(char *opaque);
	void setCallbacks(camera_request_memory get_memory);

    int setExposureCompensation(int value);
    int setWhiteBalance(int value);
    int setSceneMode(int value);
    int setFlashMode(int value);
    int setFocusMode(int value);
    bool setFocusArea(int dx, int dy);
    bool autoFocus(void);
    bool cancelAutoFocus(void);
    int getFocusModeResult(void);

	WMTCamJdec mJdec;
	char *mCMOSPreviewBuffer;

private:
    camera_property*  mCamProp;
	int     mVideoFd;       //v4l2 device file 
	struct vdIn *   mVideoIn;

	int             mPictureSize;
	unsigned int buffer_length;
	int v4l2_preview_width;
	int v4l2_preview_height;

	int hal_preview_width;
	int hal_preview_height;
	camera_request_memory mGetMemoryCB;
	bool mTakingPicture;
	char *mRecordBuf;
	char *mJPEGBuffer;
	char *mThumbRaw;
	unsigned int *mMBpool;
	struct v4l2_buffer *mV4L2buf;
	bool mPictureStopPreview;
	mutable Mutex	mPreviewLock;
	mutable Mutex	mParamLock;
	mutable Mutex	mRecLock;
	mutable Condition   mPreviewCondition;
	mutable Condition   mParamCondition;
	int mMirrorEnable;
	int mRotation;
    int mPixelformat;
	void YUV422toYUV420(void * cameraIn, char *record_buf);
	void YUY2toYUV420(void * cameraIn, char *record_buf);
	void RecordYUY2toYUV420(void * cameraIn, char *record_buf);
	char *getfreebuf(void);
	int EnumerateImageFormats(void);
	int setFrameRate(int fps);

	int	mFrameRate;
    int mWhiteBalance;
    int mSceneMode;
    int mFlashMode;
    int mFocusMode;
    bool mAutoFocusRunning;

    int EnumerateFmt(void);
    int EnumerateFrameSize(int fmt);
    int EnumerateFrameIntervals(int fmt, int width, int height);
    int setFlashMode(int mode, int delay_ms);
    int mAutoFlashForce;
};

}; // namespace android

#endif  /* WONDERMEDIA_V4L2CAMERA_H_INCLUDED */

// vim: et ts=4 shiftwidth=4

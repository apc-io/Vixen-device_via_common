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

//#define LOG_NDEBUG 0
#define LOG_TAG "V4L2Camera"

#include <utils/Log.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <media/IMediaPlayerService.h>
#include <binder/IServiceManager.h>

#include "UbootParam.h"
#include "videodev2_wmt.h"

#define TRACE_FUNC()    ALOGV("Enter %s", __FUNCTION__)


extern "C" {
    #include <jpeglib.h>  
    #include "../gpio/wmt-gpio-dev.h"
}

#include "CameraUtil.h"
#include "V4L2Camera.h"

using namespace android;

static int gpioCtl(int num, int on)
{
    return wmt_gpio_set(num, GPIO_DIRECTION_OUT, on);
}

static void camera_poweron(camera_property* c, bool on)
{
   if (c->type == CAMERA_TYPE_USB && c->gpio >= 0) {
       int level = on ? c->active : !c->active;
       gpioCtl(c->gpio, level);
   }
}

static int freeVideoResource()
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);

    return 0;//service->suspendVideoInstance(0X7FFFFFFF, NULL, 0);
}

struct vdIn {
    void *mem[NB_BUFFER];
    bool isStreaming;
};

namespace android {


V4L2Camera::V4L2Camera (camera_property*  p)
	: mJdec()
{
    mCamProp = p;
    mRecordBuf = NULL;
    mTakingPicture = false;
    mVideoFd = -1;
    mVideoIn = (struct vdIn *) calloc (1, sizeof (struct vdIn));
    v4l2_preview_width = 0;
    v4l2_preview_height = 0;
    mPictureStopPreview = false;
    mPictureSize = 0;
    mThumbRaw = NULL;
    mMirrorEnable = 0;
    mCMOSPreviewBuffer = NULL;
    mMBpool = NULL;
	mV4L2buf = NULL;
    mWhiteBalance = 0;
    mSceneMode = 0;
    mFlashMode = 0;
    mFocusMode = 0;
    mAutoFocusRunning = false;
    mPixelformat = V4L2_PIX_FMT_NV21;
}

V4L2Camera::~V4L2Camera()
{
    ALOGV("Enter %s", __FUNCTION__);
    free(mVideoIn);
    if (mCMOSPreviewBuffer)
        free(mCMOSPreviewBuffer);
    if (mMBpool)
        free(mMBpool);
	if (mV4L2buf)
		free(mV4L2buf);
}

int readFromFile(const char* path, char* buf, size_t size)
{
    if (!path)
        return -1;
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        ALOGE("Could not open '%s'", path);
        return -1;
    }
    
    ssize_t count = read(fd, buf, size);
    if (count > 0) {
        while (count > 0 && buf[count-1] == '\n')
            count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    } 

    close(fd);
    return count;
}

#define SYS_CLASS_VIDEO_PATH	"/sys/class/video4linux"

int scan_video_device_node(int type)
{
    char path[PATH_MAX];
    struct dirent* entry;
    int idx = -1;

    DIR* dir = opendir(SYS_CLASS_VIDEO_PATH);
    if (dir == NULL) {
        ALOGE("Could not open %s\n", SYS_CLASS_VIDEO_PATH);
        return idx;
    }

    while ((entry = readdir(dir))) {
	    const char* name = entry->d_name;

	    // ignore "." and ".."
	    if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
		    continue;
	    }

	    char buf[20];
	    // Look for "type" file in each subdirectory
	    snprintf(path, sizeof(path), "%s/%s/name", SYS_CLASS_VIDEO_PATH, name);
	    int length = readFromFile(path, buf, sizeof(buf));
	    if (length > 0) {
		    if (buf[length - 1] == '\n')
			    buf[length - 1] = 0;

            if (type == CAMERA_TYPE_CMOS) {
                if (!strncmp(buf, "cmos_cam", 8)) {
                    sscanf(name, "video%d", &idx);
                    goto found;
                }
            } else {
                if (strncmp(buf, "cmos_cam", 8)) {
                    sscanf(name, "video%d", &idx);
                    goto found;
                }
            }
	    }
    }

found:
    closedir(dir);
    return idx;
}

int V4L2Camera::open()
{
    TRACE_FUNC();

    int ret = 0;
    int retry = 0;

    if (mVideoFd >= 0) {
        ALOGW("device is already open! %d", mVideoFd);
        return 0;
    } 

    camera_poweron(mCamProp, 1);

    while (retry++ < mCamProp->retryCount) {
        int idx = scan_video_device_node(mCamProp->type);
	    sprintf(mCamProp->nodeName, "/dev/video%d", idx);
	    ALOGV("Opening %s ...", mCamProp->nodeName);
	    mVideoFd = ::open(mCamProp->nodeName, O_RDWR);
	    if (mVideoFd >= 0)
		    break;

	    freeVideoResource();
	    ALOGW("ERROR opening V4L interface: %s, retry %d", strerror(errno), retry);
	    usleep(200000);
    }

    if (mVideoFd == -1) {
        camera_poweron(mCamProp, 0);
        ALOGE("ERROR openning %s: %s", mCamProp->nodeName, strerror(errno));
        return -1;
    }

    ret = v4l2_ioctl_queycap();
    if (ret) {
        ALOGE("v4l2_ioctl_queycap() failed");
        return -1;
    }

    if (mCamProp->type == CAMERA_TYPE_CMOS) {
	    ret = v4l2_enuminput(mCamProp->cameraId);
	    if (ret) {
		    ALOGE("v4l2_enuminput() failed");
		    return -1;
	    }
	    ret = v4l2_s_input(mCamProp->cameraId);
	    if (ret) {
		    ALOGE("v4l2_s_input() failed");
		    return -1;
	    }
	    usleep(300*1000);
    }

    EnumerateFmt();

    return ret;
}


bool V4L2Camera::isOpened() const 
{
    return mVideoFd >= 0;
}

void V4L2Camera::close()
{
   TRACE_FUNC();
   if (mVideoFd >= 0) {
           ::close(mVideoFd);
           mVideoFd = -1;
           mJdec.close();
   }
   camera_poweron(mCamProp, 0);
}

int V4L2Camera::v4l2_ioctl_g_ctrl(uint32_t id, int *rvalue)
{
	struct v4l2_control ctrl;
	int ret;

	if (!isOpened()) {
		ALOGW("V4L2 not open when call %s", __FUNCTION__);
		return -1;
	}

	ctrl.id = id;
	ctrl.value = 0;

	ret = ioctl (mVideoFd, VIDIOC_G_CTRL, &ctrl);
	if (ret < 0) {
		ALOGE("Error VIDIOC_S_CTRL: FAIL %d", ret);
		return ret;
	}

	*rvalue = ctrl.value;
	return 0;
}

int V4L2Camera::v4l2_ioctl_s_ctrl(uint32_t id, int value)
{
    struct v4l2_control ctrl;

    if (!isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return 0;
    }

    ctrl.id = id;
    ctrl.value = value;

    return ioctl (mVideoFd, VIDIOC_S_CTRL, &ctrl);
}

int V4L2Camera::v4l2_ioctl_queycap(void)
{
	struct v4l2_capability cap;
	int ret;

	memset(&cap, 0, sizeof(struct v4l2_capability));
	ret = ioctl (mVideoFd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		ALOGE("Error opening device: unable to query device cap.");
		return -1;
	}

    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        ALOGE("Error opening device: video capture not supported. 0x%X", cap.capabilities);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        ALOGE("Capture device does not support streaming i/o");
        return -1;
    }

	return 0;
}

int V4L2Camera::v4l2_enuminput(int index)
{
    int ret = -1;
    struct v4l2_input input;

	if (!isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
		return -1;
    }

    input.index = index;
    ret = ioctl(mVideoFd, VIDIOC_ENUMINPUT, &input);
    if (ret) {
        ALOGE("%s: no matching index founds", __func__);
        return -1;
    }

    ALOGI("Name of input channel[%d] is %s", input.index, input.name);

    return 0;
}

int V4L2Camera::v4l2_s_input(int index)
{
    int ret = -1;
    struct v4l2_input input;

	if (!isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
		return -1;
    }

    input.index = index;

    ret = ioctl(mVideoFd, VIDIOC_S_INPUT, &input);
    if (ret){
        ALOGE("failed to ioctl: VIDIOC_S_INPUT (%d - %s)", errno, strerror(errno));
        return ret;
    }

    return ret;
}

int V4L2Camera::v4l2_ioctl_s_fmt(int width, int height, unsigned int pixelformat)
{
	struct v4l2_format format;
	int ret;

	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.pixelformat = pixelformat;
	format.fmt.pix.field = V4L2_FIELD_ANY;

	ret = ioctl(mVideoFd, VIDIOC_S_FMT, &format);
	if (ret < 0) {
		ALOGE("Open: VIDIOC_S_FMT Failed: (%dx%d) %s", width, height, strerror(errno));
		return ret;
	}
    
    if (mCamProp->mirror & 0x1)
        v4l2_ioctl_s_ctrl(V4L2_CID_VFLIP, 1);
    else
        v4l2_ioctl_s_ctrl(V4L2_CID_VFLIP, 0);

    if (mCamProp->mirror & 0x2)
        v4l2_ioctl_s_ctrl(V4L2_CID_HFLIP, 1);
    else
        v4l2_ioctl_s_ctrl(V4L2_CID_HFLIP, 0);

	return 0;
}

int V4L2Camera::v4l2_ioctl_enum_framesizes(int index, int *width, int *height)
{
    struct v4l2_frmsizeenum frmsize;
    int ret;

    if( !isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }

    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.index = index;
    frmsize.pixel_format = V4L2_PIX_FMT_YUYV;

    ret = ioctl(mVideoFd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
    if (ret == 0) {
        switch (frmsize.type) {
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            ALOGV("device supports sizes %dx%d to %dx%d using %dx%d increments",
                  frmsize.stepwise.min_width, frmsize.stepwise.min_height,
                  frmsize.stepwise.max_width, frmsize.stepwise.max_height,
                  frmsize.stepwise.step_width, frmsize.stepwise.step_height );
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            ALOGV("device supports all sizes %dx%d to %dx%d",
                  frmsize.stepwise.min_width, frmsize.stepwise.min_height,
                  frmsize.stepwise.max_width, frmsize.stepwise.max_height );
            break;
        case V4L2_FRMSIZE_TYPE_DISCRETE:
        default:
            ALOGV("{[%d] width = %d , height = %d }\n", index,
                  frmsize.discrete.width, frmsize.discrete.height);
            *width = frmsize.discrete.width;
            *height = frmsize.discrete.height;
            break;
        }
    }
    return ret;
}

int V4L2Camera::v4l2_ioctl_queryctrl(struct v4l2_queryctrl *qc)
{
    if( !isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }
    return ioctl(mVideoFd, VIDIOC_QUERYCTRL, qc);
}

int V4L2Camera::v4l2_ioctl_querymenu(struct v4l2_querymenu *qm)
{
    if( !isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }
    return ioctl(mVideoFd, VIDIOC_QUERYMENU, &qm);
}

int V4L2Camera::v4l2_ioctl_reqbufs(int count)
{
    int ret = 0;
    int retry = 0, retryCount = 5;
    struct v4l2_requestbuffers rb;

    memset(&rb, 0, sizeof(struct v4l2_requestbuffers));
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    rb.count = count;

    while(retry < retryCount) {
       ret = ioctl(mVideoFd, VIDIOC_REQBUFS, &rb);
       if (ret < 0) {
           ALOGW("Init: VIDIOC_REQBUFS failed: %s, retry = %d, count = %d",
                 strerror(errno), retry, count);
           freeVideoResource();
           retry++;
           continue;
       }
       break;
    }

    return ret;
}

int V4L2Camera::v4l2_ioctl_querybuf(int count)
{
    int i, ret;
    struct v4l2_buffer buf;

    for (i = 0; i < count; i++)	{
        memset (&buf, 0, sizeof (struct v4l2_buffer));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mVideoFd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            ALOGE("Init: Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoIn->mem[i] = mmap (0,
                                 buf.length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 mVideoFd,
                                 buf.m.offset);
        if (mVideoIn->mem[i] == MAP_FAILED) {
            ALOGE("Init: Unable to map buffer (%s)", strerror(errno));
            return -1;
        }
        if (mMBpool)
            mMBpool[i] = buf.m.offset;
    }
    buffer_length = buf.length;

    return 0;
}

int V4L2Camera::v4l2_ioctl_qbuf(int count)
{
        int i, ret;
        struct v4l2_buffer buf;

        for (i = 0; i < count; i++) {
                memset (&buf, 0, sizeof (struct v4l2_buffer));
                buf.index = i;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                ret = ioctl(mVideoFd, VIDIOC_QBUF, &buf);
                if (ret < 0) {
                        ALOGE("Init: VIDIOC_QBUF Failed");
                        return -1;
                }
        }
        return 0;
}

int V4L2Camera::v4l2_ioctl_dqbuf(int count)
{
        int i, ret;
        struct v4l2_buffer buf;

        for (i = 0; i < count; i++) {
                memset (&buf, 0, sizeof (struct v4l2_buffer));
                buf.index = i;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                ret = ioctl(mVideoFd, VIDIOC_DQBUF, &buf);
                if (ret < 0) {
                        ALOGE("Init: VIDIOC_DQBUF Failed");
                        return -1;
                }
        }
        return 0;
}

int V4L2Camera::swSetFrameRate(int rate)
{
	mFrameRate = rate;
	return 0;
}

int V4L2Camera::startPreview(int width, int height, int pixelformat)
{
    TRACE_FUNC();

    int ret = 0;
    int retry = 0;  
    int i;
    nsecs_t timeout = 2000 * 1000000LL;

    if( !isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        if ((ret = open()) < 0) {
            ALOGW("%s: open failed, %s", __FUNCTION__, strerror(errno));
            return ret;
        }
    }

    mParamLock.lock();
    if (mTakingPicture == true) {
        //ALOGW("start mParamCondition waiting");
        if (NO_ERROR != mParamCondition.waitRelative(mParamLock, timeout)) {
            ALOGW("Timeout for wait mParamLock , but still runing");
        }
        //ALOGW("return from mParamCondition waiting");
    }
    mParamLock.unlock();

    hal_preview_width = width;
    hal_preview_height = height;
    v4l2_preview_width = width;
    v4l2_preview_height = height;

    if (mCMOSPreviewBuffer)
        free(mCMOSPreviewBuffer);
    mCMOSPreviewBuffer = (char *)calloc (1, (v4l2_preview_width*v4l2_preview_width*12)/8);
    
    if (mCamProp->directREC) {
        mJdec.mDirectREC = true;
        if (mMBpool)
            free(mMBpool);
        mMBpool = (unsigned int *)calloc (1, sizeof(unsigned int)*NB_BUFFER);
        if (mV4L2buf)
            free(mV4L2buf);
        mV4L2buf = (struct v4l2_buffer *)calloc (1, sizeof(struct v4l2_buffer)*NB_BUFFER);
    }

    if (mCamProp->type == CAMERA_TYPE_CMOS) {
        if(pixelformat != V4L2_PIX_FMT_NV12)
            pixelformat = V4L2_PIX_FMT_NV21;
        // setParameters while device not open
        setWhiteBalance(-1);
        setSceneMode(-1);
        setFlashMode(-1);
        setFocusMode(-1);
    } else
        pixelformat = V4L2_PIX_FMT_YUYV;

    if (mCamProp->type == CAMERA_TYPE_USB)
        setFrameRate(30);
    else
        setFrameRate(mFrameRate);

    ret = v4l2_ioctl_s_fmt(v4l2_preview_width, v4l2_preview_height, pixelformat);
    if (ret < 0) {
        ALOGW("try 640x480 ..");
        pixelformat = V4L2_PIX_FMT_NV21;
        ret = v4l2_ioctl_s_fmt(640, 480, pixelformat);
        if (ret < 0)
            return ret;
        v4l2_preview_width = 640;
        v4l2_preview_height = 480;
        mCamProp->directREC = false;
    }

    ret = v4l2_ioctl_reqbufs(NB_BUFFER);
    if (ret < 0)
        return ret;

    ret = v4l2_ioctl_querybuf(NB_BUFFER);
    if (ret < 0)
        return ret;

    ret = v4l2_ioctl_qbuf(NB_BUFFER);
    if (ret < 0)
        return ret;

    mPixelformat = pixelformat;

    return 0;
}

void V4L2Camera::stopPreview()
{
    TRACE_FUNC();

    setFlashMode(FLASH_MODE_OFF, 0);

    if (mVideoIn->isStreaming) {
        stopStreaming();
        term(NB_BUFFER);
        v4l2_ioctl_reqbufs(0);
    }
}

void V4L2Camera::term(int count)
{
    TRACE_FUNC();

    /* Unmap buffers */
    for (int i = 0; i < count; i++) {
        if (munmap(mVideoIn->mem[i], buffer_length) < 0) {
			ALOGE("Uninit: Unmap failed");
		}
	}
}

int V4L2Camera::startStreaming()
{
    TRACE_FUNC();

	enum v4l2_buf_type type;
    int ret;
    if (!mVideoIn->isStreaming) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl (mVideoFd, VIDIOC_STREAMON, &type);
        if (ret < 0) {
            ALOGE("StartStreaming: Unable to start capture: %s", strerror(errno));
            return ret;
        }
        mVideoIn->isStreaming = true;
    }
    return 0;
}

int V4L2Camera::stopStreaming ()
{
    TRACE_FUNC();

    enum v4l2_buf_type type;
    int ret;

    if (mVideoIn->isStreaming) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = ioctl (mVideoFd, VIDIOC_STREAMOFF, &type);
        if (ret < 0) {
            ALOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
            return ret;
        }

        mVideoIn->isStreaming = false;
    }

    return 0;
}

int V4L2Camera::getPreviewSize(void)
{
  // TODO : check pixel format
  return (hal_preview_width*hal_preview_height*12)/8;
}

int V4L2Camera::getPictureSize(void)
{
  // TODO : check pixel format
  return mPictureSize;
}

int V4L2Camera::startSnap(int w, int h, int delay)
{
    TRACE_FUNC();

    const char *device;
    int retry = 0, ret = -1;
    int pixelformat;

    if( !isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }

    mParamLock.lock();
    mTakingPicture = true;
    mParamLock.unlock();

    mPictureStopPreview = true;

    v4l2_preview_width = w;
    v4l2_preview_height = h;
    hal_preview_width = w;
    hal_preview_height = h;

    if (mCMOSPreviewBuffer)
        free(mCMOSPreviewBuffer);
    mCMOSPreviewBuffer = (char *)calloc (1, (v4l2_preview_width*v4l2_preview_width*12)/8);

    setFlashMode(mFlashMode, 500);

    if (mCamProp->type == CAMERA_TYPE_CMOS)
        pixelformat = V4L2_PIX_FMT_NV21;
    else
        pixelformat = V4L2_PIX_FMT_YUYV;

    ret = v4l2_ioctl_s_fmt(v4l2_preview_width, v4l2_preview_height, pixelformat);
    if (ret < 0) {
        ALOGW("try 640x480 ..");
        ret = v4l2_ioctl_s_fmt(640, 480, pixelformat);
        if (ret < 0)
            return ret;
        v4l2_preview_width = 640;
        v4l2_preview_height = 480;
    }

    ret = v4l2_ioctl_reqbufs(3);
    if (ret < 0)
        return ret;

    ret = v4l2_ioctl_querybuf(3);
    if (ret < 0)
        return ret;

    ret = v4l2_ioctl_qbuf(3);
    if (ret < 0)
        return ret;

    startStreaming();

    if (delay > 0) {
        ALOGW("delay before %d ms before take picture",delay);
        usleep(delay*1000);
    }
    mPixelformat = pixelformat;

    return 0;
}

int V4L2Camera::stopSnap(void)
{
    int ret;
    struct v4l2_buffer buf;

    if (mPictureStopPreview == false)
            return 0;	

    setFlashMode(FLASH_MODE_OFF, 0);

    if (mCMOSPreviewBuffer) {
            free(mCMOSPreviewBuffer);
            mCMOSPreviewBuffer = NULL;
    }

    mPictureStopPreview = false;

    stopStreaming();
    term(3);
    v4l2_ioctl_reqbufs(0);

    mParamLock.lock();
    mTakingPicture = false;
    mParamCondition.signal();
    mParamLock.unlock();

    cancelAutoFocus();
    return 0;
}

int V4L2Camera::getPictureRaw(char *jpegbuf, char *thumbraw)
{
    NV21JpegCompressor compressor;
    status_t res = -1;
    nsecs_t timeout = 900 * 1000000LL;

    if (thumbraw)
            mThumbRaw = thumbraw;
    mJPEGBuffer = jpegbuf;
    //preview not stop
    if (mPictureStopPreview == false) {
            mPreviewLock.lock();
            if (NO_ERROR != mPreviewCondition.waitRelative(mPreviewLock, timeout)) {
                    ALOGW("Timeout for wait mPreviewLock , but still runing");
            }
            mPreviewLock.unlock();
    }	

    return Preview2Picture(NULL);
}

int V4L2Camera::Preview2Picture(char *recordBuffer)
{
    int ret; 
    struct v4l2_buffer buf;
    unsigned int total = 0;
    char *dest = NULL;
    int retry = 5;

    mPreviewLock.lock();
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    do {
        ret = ioctl(mVideoFd, VIDIOC_DQBUF, &buf);
        ALOGV("%s : VIDIOC_DQBUF %d, %d", __FUNCTION__, retry, ret);
    } while (ret < 0 && retry--);

    if (ret < 0) {
        ALOGE("%s : VIDIOC_DQBUF Failed, %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    buffer_length = buf.length;

    if (mCamProp->type == CAMERA_TYPE_USB) {
//        if (mMirrorEnable)
//            dest = mCMOSPreviewBuffer;
//        else
            dest = mJPEGBuffer;
        YUY2toYUV420(mVideoIn->mem[buf.index], dest);
        mJdec.mJdecDest = mJPEGBuffer;
//        if (hal_preview_width%64 == 0)
//            mJdec.NeonMirror((unsigned int)dest, hal_preview_width, hal_preview_height);
//        else
//            mJdec.doMirror(dest, mJPEGBuffer, hal_preview_width, hal_preview_height, 0);
    } else {
        if ((hal_preview_width != v4l2_preview_width) || (hal_preview_height != v4l2_preview_height)) {
            swCropImagetoYUV420((char *)mVideoIn->mem[buf.index], mJPEGBuffer, 
                                v4l2_preview_width, v4l2_preview_height, YUV420,
                                hal_preview_width, hal_preview_height);
        } else {
            memcpy(mJPEGBuffer, (char *)mVideoIn->mem[buf.index],
                   (hal_preview_width*hal_preview_height*12)/8);
        }
    }

    ret = ioctl(mVideoFd, VIDIOC_QBUF, &buf);
    mPreviewCondition.signal();
    mPreviewLock.unlock();
    if (ret < 0) {
        ALOGE("%s : VIDIOC_QBUF Failed", __FUNCTION__);
        return -1;
    }

    return 0;
}

int V4L2Camera::grabPreviewFrame(char *recordBuffer, gralloc_mb_block *block)
{
    int ret; 
    struct v4l2_buffer buf;
    unsigned int total = 0;
    char *dest = NULL;
    struct jdec_sync *sync;
	unsigned int needtoqueue = 1;

    Mutex::Autolock lock(mPreviewLock);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    ret = ioctl(mVideoFd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        ALOGE("%s : VIDIOC_DQBUF Failed", __FUNCTION__);
        return ret;
    }

    buffer_length = buf.length;

    if (!recordBuffer)
        goto out;

    if (mCamProp->type == CAMERA_TYPE_USB) {
        if (mMirrorEnable)
            dest = mCMOSPreviewBuffer;
        else
            dest = recordBuffer;
        YUY2toYUV420(mVideoIn->mem[buf.index], dest);
        mJdec.mJdecDest = recordBuffer;
        //        if (hal_preview_width%64 == 0)
        //            mJdec.NeonMirror((unsigned int)dest, hal_preview_width, hal_preview_height);
        //        else
        mJdec.doMirror(dest, recordBuffer, hal_preview_width, hal_preview_height, 0);
    } else {
        /* CMOS do mirror via u-boot parameter */
        dest = (char *)mVideoIn->mem[buf.index];

        if(block && (mPixelformat == V4L2_PIX_FMT_NV12)) {
            block->magic = 'WMMB';
            block->fb_width  = v4l2_preview_width;
            block->fb_height = v4l2_preview_height;
            block->video_width  = hal_preview_width;
            block->video_height = hal_preview_height;
            block->phy_addr     = (long)mMBpool[buf.index];
            block->cookie       = 0;

            ALOGV("block #%d %dx%d %dx%d, phy %lX", 
                  buf.index,
                  block->fb_width, block->fb_height,
                  block->video_width,block->video_height,
                  block->phy_addr);
        }
        else {
            //NV21 to YUV420 for scale
            swCropImagetoYUV420(dest, recordBuffer, 
                                v4l2_preview_width, v4l2_preview_height, YUV420,
                                hal_preview_width, hal_preview_height);
        }
    }

    if (mRecordBuf) {
        if (mCamProp->directREC && mCamProp->type == CAMERA_TYPE_CMOS) {
				needtoqueue = 0;
				mRecLock.lock();
				sync = (struct jdec_sync *)getfreebuf();
				if (sync != NULL)
					sync->yaddr = mMBpool[buf.index];
				memcpy(&mV4L2buf[buf.index], &buf, sizeof(struct v4l2_buffer));
				sync->fid = buf.index;
                sync->encodetag = mPixelformat;
				mRecLock.unlock();
        } else {
            convertNV21toNV12((uint8_t *)recordBuffer, (uint8_t *)mRecordBuf,
                              hal_preview_width, hal_preview_height);
        }
    }

out:
	if (needtoqueue) {
		ret = ioctl(mVideoFd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			ALOGE("%s : VIDIOC_QBUF Failed", __FUNCTION__);
			return -1;
		}
	}
    mPreviewCondition.signal();
    return ret;
}

char *V4L2Camera::getfreebuf(void)
{
    struct record_pointer *rec;
    int i;
    camera_memory_t* mem;

    rec = (struct record_pointer *)mRecordBuf;

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
                return (char *)mem->data;
            }
        }
    }
    ALOGE("Can not find a free frame");
    return NULL;
}

void V4L2Camera::setCallbacks(camera_request_memory get_memory)
{
	mGetMemoryCB = get_memory;
}

void V4L2Camera::releaseRecordingFrame(char *opaque)
{
    struct record_pointer *rec;
    unsigned int i, addr ,j;
    camera_memory_t* mem;
    struct jdec_sync *sync;
    int ret = -1;

    if (mCamProp->directREC == false)
        return;

    mRecLock.lock();
    rec = (struct record_pointer *)mRecordBuf;	
    if (!rec) {
        ALOGW("rec is NULL");
        mRecLock.unlock();
        return;
    }

    if (opaque) {
        addr = (unsigned int)opaque;		
        for (i = 1; i < RECBUFNUMs; i++) {
            if (rec[i].data == addr) {
                if (!rec[i].cameramem) {
                    ALOGE("\nreleaseRecordingFrame error");
                    mRecLock.unlock();
                    return;
                }
                sync = (struct jdec_sync *)opaque;
                ret = ioctl(mVideoFd, VIDIOC_QBUF, &mV4L2buf[sync->fid]);
                if (ret < 0)
                    ALOGE("%s : VIDIOC_QBUF Failed", __FUNCTION__);
                mem = (camera_memory_t*)rec[i].cameramem;
                mem->release(mem);
                mem = NULL;
                memset(&rec[i], 0, sizeof(struct record_pointer));
                mRecLock.unlock();
                return;
            }
        }
        ALOGE("release a buffer , but not used !!!!!!!!");
    }
    ALOGE("release a NULL buffer  !!!!!!!!");
    mRecLock.unlock();
}

void V4L2Camera::setRecordBuf(char *addr)
{
    if (mRecordBuf != NULL && mCamProp->directREC == true) {
        mRecLock.lock();

        //stop record called between grabPreviewFrame and frameCallbackNotifier
        //release the dequeued buffer left in the recordBuf
        struct record_pointer *rec = (struct record_pointer *)mRecordBuf;
        for (int i = 1; i < RECBUFNUMs; i++) {
            camera_memory_t*mem = (camera_memory_t*)rec[i].cameramem;
            if(mem) {
                struct jdec_sync *sync = (struct jdec_sync *)rec[i].data;
                if (ioctl(mVideoFd, VIDIOC_QBUF, &mV4L2buf[sync->fid]) < 0) {
                    ALOGE("%s : VIDIOC_QBUF Failed", __FUNCTION__);
                    continue;
                }
                mem->release(mem);
            }
        }

        memset(mRecordBuf, 0, (RECBUFNUMs*sizeof(struct record_pointer)));        
        mRecLock.unlock();
    }
    mRecordBuf = addr;
}

void V4L2Camera::setRotation(int degress)
{
	mRotation = degress;
	mJdec.mRotation = degress;
}

void V4L2Camera::setMirror(int enable)
{
	mMirrorEnable = enable;
	mJdec.mMirrorEnable = enable;
}

int V4L2Camera::setExposureCompensation(int value)
{
	return v4l2_ioctl_s_ctrl(V4L2_CID_EXPOSURE, value);
}

int V4L2Camera::setWhiteBalance(int value)
{
    if (value >= 0) {
        mWhiteBalance = value;
    }
	return v4l2_ioctl_s_ctrl(V4L2_CID_DO_WHITE_BALANCE, mWhiteBalance);
}

int V4L2Camera::setSceneMode(int value)
{
    if (value >= 0) {
        mSceneMode = value;
    }
	return v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_SCENE_MODE, mSceneMode);
}

int V4L2Camera::setFrameRate(int fps)
{
    int ret;
    struct v4l2_streamparm parm;

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mVideoFd, VIDIOC_G_PARM, &parm);
    if (ret) {
        //ALOGE("VIDIOC_G_PARM fail, ret %d.", errno);
        return ret;
    }

    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps;
    ret = ioctl(mVideoFd, VIDIOC_S_PARM, &parm);
    if (ret) {
        //ALOGD("VIDIOC_S_PARM Fail when setfps %d, ret %d.", errno, fps);
    }

    return ret;
}


int V4L2Camera::setFlashMode(int mode, int delay_ms)
{
    int need_flash = 0;

    switch (mode) {
    case FLASH_MODE_OFF:
    case FLASH_MODE_TORCH:
        return v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FLASH_MODE, mode);
    case FLASH_MODE_AUTO:
        need_flash = mAutoFlashForce;
        break;
    case FLASH_MODE_ON:
        need_flash = 1;
        break;
    default:
        return -EINVAL;
    }

    if (need_flash) {
        v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FLASH_MODE, FLASH_MODE_ON);
        usleep(delay_ms * 1000);
    }
    return 0;
}

int V4L2Camera::setFlashMode(int value)
{
    if (value >= 0) {
        mFlashMode = value;
    }

    switch (mFlashMode) {
    case FLASH_MODE_OFF:
    case FLASH_MODE_TORCH:
        /* do immediately */
        return v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FLASH_MODE, mFlashMode);
    case FLASH_MODE_AUTO:
    case FLASH_MODE_ON:
        break;
    }
    return 0;
}

bool V4L2Camera::setFocusArea(int dx, int dy)
{
    return (!v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FOCUS_POSITION_X, dx) &&
            !v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FOCUS_POSITION_Y, dy));
}

int V4L2Camera::setFocusMode(int value)
{
    if (value >= 0)
        mFocusMode = value;

	switch (mFocusMode) {
	case FOCUS_MODE_AUTO:
        break;
	case FOCUS_MODE_CONTINUOUS:
        break;
	case FOCUS_MODE_FIXED:
        break;
	case FOCUS_MODE_INFINITY:
	case FOCUS_MODE_CONTINUOUS_VIDEO:
        return v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FOCUS_MODE, mFocusMode);
	default:
		return -EINVAL;
	}
    return 0;
}

bool V4L2Camera::autoFocus(void)
{
    if (mAutoFocusRunning == true)
        return true;

    if (mFlashMode == FLASH_MODE_AUTO) {
        if (v4l2_ioctl_g_ctrl(V4L2_CID_CAMERA_FLASH_MODE_AUTO, &mAutoFlashForce) < 0) {
            ALOGE("%s: V4L2_CID_CAMERA_FLASH_MODE_AUTO failed", __func__);
        }
    }
    setFlashMode(mFlashMode, 500);

    if (v4l2_ioctl_s_ctrl(V4L2_CID_CAMERA_FOCUS_MODE, FOCUS_MODE_AUTO) < 0) {
        ALOGE("ERR(%s): V4L2_CID_CAMERA_FOCUS_MODE() fail", __func__);
        return false;
    }

    mAutoFocusRunning = true;

    return true;
}

bool V4L2Camera::cancelAutoFocus(void)
{
    if (mAutoFocusRunning == false) {
        ALOGV("DEBUG(%s):mAutoFocusRunning == false", __func__);
        return true;
    }

    setFlashMode(FLASH_MODE_OFF, 0);
    mAutoFocusRunning = false;

    return true;
}

int V4L2Camera::getFocusModeResult(void)
{
    int ret = 0;

#define AF_WATING_TIME       (100000)  //  100msec
#define TOTAL_AF_WATING_TIME (3000000) // 3000msec

    for (unsigned int i = 0; i < TOTAL_AF_WATING_TIME; i += AF_WATING_TIME) {

        if (mAutoFocusRunning == false) {
            ret = -1;
            break;
        }

        if (v4l2_ioctl_g_ctrl(V4L2_CID_CAMERA_AUTO_FOCUS_RESULT, &ret) < 0) {
            ALOGE("ERR(%s):exynos_v4l2_g_ctrl() fail", __func__);
            ret = -1;
            break;
        }

        switch(ret) {
        case FOCUS_RESULT_FOCUSING: // AF Running
            ret = 0;
            break;
        case FOCUS_RESULT_SUCCEED: // AF succeed
            ret = 1;
            break;
        case FOCUS_RESULT_FAILED:
        default :  // AF fail
            ret = -1;
            break;
        }

        if (ret != 0)
            break;

        usleep(AF_WATING_TIME);
    }

    setFlashMode(FLASH_MODE_OFF, 0);
    return ret;
}

//FIXME: move to CameraUtil.cpp
void V4L2Camera::YUV422toYUV420(void * cameraIn, char *record_buf)
{
	int NumY, j, CbCr_Total;
	int width  = v4l2_preview_width;
	int height = v4l2_preview_height;
	unsigned int * pCbCr;
	unsigned int tempCb, tempCr;

	//if (mTakingPicture == false) {
		if ((hal_preview_width != v4l2_preview_width) || (hal_preview_height != v4l2_preview_height)) {
			swCropImagetoYUV420( (char *)cameraIn, (char *)record_buf, 
                  v4l2_preview_width, v4l2_preview_height, YUV422,
                  hal_preview_width, hal_preview_height
                 );
			return;
		}
	//}

	NumY = width*height;
	height = height >> 1;

	memcpy(record_buf, (char *)cameraIn, NumY);

	unsigned char * d = (unsigned char * )record_buf + NumY;
	unsigned char * s = (unsigned char * )cameraIn + NumY;
	for (j = 0; j < height; j++) {
		memcpy(d, s, width);
		d += width;
		s += width * 2;
	}
}

//FIXME: move to CameraUtil.cpp
void V4L2Camera::swCropImagetoYUV420(char *src, char *dest,
                                     int scr_w, int src_h, int src_f,
                                     int dst_w, int dst_h)
{
    int i, j, spos, dpos;
    int tempCb, tempCr;
    int length, height, xofs = 0, yofs = 0;
    char *s = src;
    char *d = dest;
    char *cs = (char *)(s+(scr_w*src_h));
    char *cd = (char *)(d+(dst_h*dst_w));
    int *CbCrd;

    if ((hal_preview_width == v4l2_preview_width) &&
        (hal_preview_height == v4l2_preview_height)) {
        memcpy(dest, src, (dst_w*dst_h*12)/8);
        return;
    }

    length = scr_w;
    if (dst_w > scr_w) {
        xofs = (dst_w - scr_w) / 2;
        d += xofs;
        cd += xofs;
        length = scr_w;
    } else if (dst_w < scr_w) {
        xofs = (scr_w - dst_w) / 2;
        s += xofs;
        cs += xofs;
        length = dst_w;
        xofs = 0;
    }

    height = src_h;
    if (dst_h > src_h) {
        yofs = (dst_h - src_h)/2;
        d += (yofs*dst_w);
        cd += ((yofs*dst_w) >> 1);
        height = src_h; 
    } else if (src_h > dst_h) {
        yofs = (src_h - dst_h)/2;
        s += (yofs*scr_w);
        if (src_f == YUV420)
            cs += ((yofs*scr_w) >> 1);
        else if (src_f == YUV422)
            cs += (yofs*scr_w);
        height = dst_h;
        yofs = 0;
    }

    //copy Y
    for (j = 0; j < height; j++) {
        memcpy(d, s, length);
        d += dst_w;
        s += scr_w;
    }

    if (xofs || yofs) {
        CbCrd = (int *)(dest+(dst_w*dst_h));
        j = dst_w * dst_h >> 3;
        for(i = 0; i < j; i++)
            CbCrd[i] = 0x80808080;
    }

    //copy CbCr
    height = height/2;
    for (j = 0; j < height; j++) {
        memcpy(cd, cs, length);
        cd += dst_w;
        cs += scr_w;
    }
}


//FIXME: move to CameraUtil.cpp
void V4L2Camera::YUY2toYUV420(void * cameraIn, char *record_buf)
{
    unsigned int *p_y;
    unsigned int *p_c;
    int pos, i, j, k;
    unsigned int ytmp[8];
    unsigned int ctmp[8];
    unsigned int dx, resx;
    unsigned int yuyv, yuyv1;
    unsigned int *raw = (unsigned int *)cameraIn;
    int xs, xe, ys, ye, xofs = 0, yofs = 0;

	xs = 0;
	xe = v4l2_preview_width/2;
	ys = 0;
	ye = v4l2_preview_height;
	if (mPictureStopPreview == false) {
		if (v4l2_preview_width > hal_preview_width) {
			xs = (v4l2_preview_width-hal_preview_width)/4;
			xe = xs + hal_preview_width/2;
		} else if (v4l2_preview_width < hal_preview_width) {
			xofs = (hal_preview_width-v4l2_preview_width)>>3;
		}

		if (v4l2_preview_height > hal_preview_height) {
			ys = (v4l2_preview_height-hal_preview_height)/2;
			ye = ys + hal_preview_height;
		} else if (v4l2_preview_height < hal_preview_height) {
			yofs = ((hal_preview_height-v4l2_preview_height)*hal_preview_width) >> 3;
		}
		p_y = (unsigned int *)record_buf;
    	p_c = (unsigned int *)(record_buf + (hal_preview_height*hal_preview_width));
	} else {
		p_y = (unsigned int *)record_buf;
    	p_c = (unsigned int *)(record_buf + (v4l2_preview_width*v4l2_preview_height));
	}

	if (xofs || yofs) {
		j = (hal_preview_height*hal_preview_width)>>3;
		for (i = 0; i < j; i++)
			p_c[i] = 0x80808080;
	}

    if (mCamProp->rotate == 0) {
        //width = width >>1;
        k = xofs+yofs;
        dx = xofs+yofs/2;
        resx = v4l2_preview_width>>1;
        for (j = ys; j < ye; j++) {
            if (xofs || yofs) {
            	k = xofs+yofs+(hal_preview_width>>2)*j;
           		if ((j&0x01) == 0)
            		dx = xofs+yofs/2+(hal_preview_width>>3)*j;
            }
            for (i = xs; i < xe; i+=2) {
                //if (cam_mir == 1)
                //    pos = i+((height - 1 - j)*resx);
                //else
                    pos = i+(j*resx);

                yuyv = raw[pos];
                yuyv1 = raw[pos+1];
                ytmp[0] = yuyv&0xFF;
                ytmp[1] = (yuyv&0xFF0000)>>8;
                ytmp[2] = (yuyv1&0xFF)<<16;
                ytmp[3] = (yuyv1&0xFF0000)<<8;
                ytmp[0] = (ytmp[0]|ytmp[1]|ytmp[2]|ytmp[3]);
                p_y[k] = ytmp[0];
                if ((j&0x01) == 0) {
                    /*
                    ctmp[0] = (yuyv&0xFF00)>>8;
                    ctmp[1] = (yuyv&0xFF000000)>>16;
                    ctmp[2] = (yuyv1&0xFF00)<<8;
                    ctmp[3] = (yuyv1&0xFF000000);
                    */
          			ctmp[0] = (yuyv&0xFF00);
                    ctmp[1] = (yuyv&0xFF000000)>>24;
          			ctmp[2] = (yuyv1&0xFF00)<<16;
                    ctmp[3] = (yuyv1&0xFF000000)>>8;
                    ctmp[0] = (ctmp[0]|ctmp[1]|ctmp[2]|ctmp[3]);
                    p_c[dx] = ctmp[0];
                    dx++;
                }
                k++;
            }       
        }
    }
}

int V4L2Camera::EnumerateImageFormats(void)
{
    struct v4l2_fmtdesc fmt;
    int ret = -1;

    if (!isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while ((ret = ioctl(mVideoFd, VIDIOC_ENUM_FMT, &fmt)) == 0) {
        fmt.index++;
        ALOGV("{ pixelformat = %c%c%c%c , description = %s }\n",
          fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF,
          (fmt.pixelformat >> 16) & 0xFF, (fmt.pixelformat >> 24) & 0xFF,
          fmt.description);
    }
    return 0;
}

int V4L2Camera::EnumerateFmt(void)
{
    if (!isOpened()) {
        ALOGW("V4L2 not open when call %s", __FUNCTION__);
        return -1;
    }

    int index = 0;
    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(mVideoFd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        EnumerateFrameSize(fmt.pixelformat);
        fmt.index++;
    }

    return 0;
}

int V4L2Camera::EnumerateFrameSize(int fmt)
{
    // frame size
    struct v4l2_frmsizeenum frmsize;
    memset(&frmsize, 0, sizeof(frmsize));
    frmsize.index = 0;
    frmsize.pixel_format = fmt;

    while (ioctl(mVideoFd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
        switch (frmsize.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            if (EnumerateFrameIntervals(fmt, frmsize.discrete.width, frmsize.discrete.height))
                ALOGD("%c%c%c%c, %dx%d", pixfmtstr(fmt), frmsize.discrete.width, frmsize.discrete.height);
            break;
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            ALOGE("device not supports sizes %dx%d to %dx%d using %dx%d increments",
                  frmsize.stepwise.min_width, frmsize.stepwise.min_height,
                  frmsize.stepwise.max_width, frmsize.stepwise.max_height,
                  frmsize.stepwise.step_width, frmsize.stepwise.step_height );
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            ALOGE("device not supports all sizes %dx%d to %dx%d",
                  frmsize.stepwise.min_width, frmsize.stepwise.min_height,
                  frmsize.stepwise.max_width, frmsize.stepwise.max_height );
            break;
        default:
            ALOGE("device not supports type %d", frmsize.type);
            return -EINVAL;
        }
        frmsize.index++;
    }

    return 0;
}

int V4L2Camera::EnumerateFrameIntervals(int fmt, int width, int height)
{
    int ret;
    struct v4l2_frmivalenum frmivalenum;
    memset(&frmivalenum, 0, sizeof(frmivalenum));
    frmivalenum.index = 0;
    frmivalenum.pixel_format = fmt;
    frmivalenum.width = width;
    frmivalenum.height = height;

    while ((ret = ioctl(mVideoFd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum)) == 0) {
        switch (frmivalenum.type) {
        case V4L2_FRMIVAL_TYPE_DISCRETE:    //	Discrete frame interval.
            ALOGD("%c%c%c%c, %dx%d, %dfps", pixfmtstr(fmt),
                  width, height, frmivalenum.discrete.denominator);
            break;
        case V4L2_FRMIVAL_TYPE_CONTINUOUS:  //	Continuous frame interval.
        case V4L2_FRMIVAL_TYPE_STEPWISE:    //	Step-wise defined frame interval.
        default:
            ALOGE("VIDIOC_ENUM_FRAMEINTERVALS type not supported (type %d)", frmivalenum.type);
            break;
        }
        frmivalenum.index++;
    }

    return ret;
}

}; // namespace android
        
// vim: et ts=4 shiftwidth=4

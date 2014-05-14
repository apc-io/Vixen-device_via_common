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
#ifndef WMT_CAMERA_UTIL_H_INCLUDED
#define WMT_CAMERA_UTIL_H_INCLUDED

#include <stdint.h>
#include <hardware/camera.h>
#include "videodev2_wmt.h"

enum YUVBufferType {
    YUV_PIX_FMT_YUYV,
    YUV_PIX_FMT_NV21,
    YUV_PIX_FMT_NV12,
    YUV_PIX_FMT_YUV420,
    YUV_PIX_FMT_YC422,
};

struct YUVBuffer
{
    char * addr;
    int    w;
    int    h;
    int    stride;
 
    /**
     * V4L2_PIX_FMT_NV21 from cmos 
     * V4L2_PIX_FMT_YUYV from uvc (usb camera) 
     * V4L2_PIX_FMT_YUV420  for 3rd-party program's callback( also for CTS)
     */ 
    enum YUVBufferType	format;  
};
 
#define YUV_CONVERT_ROTATE0      0x0
#define YUV_CONVERT_ROTATE90     0x1
#define YUV_CONVERT_ROTATE180    0x2        
#define YUV_CONVERT_SCALE		 0x3
#define YUV_CONVERT_MIRROR		 0x4
 
/**
 * src format can be either V4L2_PIX_FMT_NV21 or V4L2_PIX_FMT_YUYV
 * 
 * dest format can be either V4L2_PIX_FMT_NV21 or V4L2_PIX_FMT_YUV420
 * @param convertFlag 
 */
void yuvBufferConvert(const YUVBuffer * src, YUVBuffer * dest, int convertFlag);

unsigned int FRAME_SIZE(int hal_pixel_format, int width, int height);

void convertNV21toNV12(uint8_t *inputBuffer, uint8_t *outputBuffer, int width, int height);
int uvtonv21(unsigned char *src, int width, int height);
int nv21touv(void * source, int width, int height);
int nv21Scale(unsigned char *src, int src_w, int src_h, unsigned char *dst, int dst_w, int dst_h);
int nv21Crop(unsigned char *src, int src_w, int src_h, unsigned char *dest, int dst_w, int dst_h);
int nv21CropScale(unsigned char *data, int pictruew, int pictrueh, int cropw, int croph);

struct FunctionTracer {
    FunctionTracer(const char * func) {
        this->funcname = func;
        ALOGV(" -> Enter %s",  funcname);
    }

    ~FunctionTracer() {
        ALOGV(" <- Leave %s", funcname);
    }
    const char * funcname;
};



/**
 * simple helper class for camera_memory.
 */
class CameraMemory
{
public:
    explicit CameraMemory() {
        this->mem  = NULL;
    }

    bool alloc(camera_request_memory req, int size, int count = 1, void * user = NULL) {
        release();
        if( !req)
            return false;

        mem = req(-1, size, count, user);
        if( mem == NULL || mem->data == NULL) {
            ALOGE("Memory alloc failed %dx%d", size, count);
            release();
            return false;
        }
        return true;
    }

    void release() {
        if(mem)  {
            mem->release(mem);
            mem = NULL;
        }
    }

    ~CameraMemory() {
        release();
    }

    char *data() {
        return mem ? (char *)mem->data : NULL;
    }

    unsigned char * udata() {
        return mem ? (unsigned char *)mem->data : NULL;
    }

    size_t size() {
        return mem ? mem->size : 0;
    }

    void * handle() {
        return mem ? mem->handle : NULL;
    }

	void swap(CameraMemory& other)	
    {
        camera_memory* tmpmem;
		tmpmem = this->mem;
		this->mem = other.mem;
		other.mem = tmpmem;
    }
	
    camera_memory * mem;
    
private:
    CameraMemory(const CameraMemory& noimpl);
    CameraMemory& operator=(const CameraMemory& noimpl);
};

/* camera_area_t
 * rectangle with weight to store the focus and metering areas.
 * x1, y1, x2, y2: from -1000 to 1000
 * weight: 0 to 1000
 */
typedef struct {
    int x1, y1, x2, y2;
    int weight;
} camera_area_t;


#endif /* WMT_CAMERA_UTIL_H_INCLUDED */

// vim: et ts=4 shiftwidth=4

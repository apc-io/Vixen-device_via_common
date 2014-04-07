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



#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <camera/CameraParameters.h>
#include <hardware/camera.h>
#include <utils/threads.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/RefBase.h>

#include "JpegCompressor.h"
#include "V4L2Camera.h"
#include "swexif.h"
#include "CameraUtil.h"

namespace android  {

class CameraHardware : public camera_device {
public:
    CameraHardware(camera_property* p);
    ~CameraHardware();

    /****************************************************************************
     * Camera Parameter
     ***************************************************************************/
    void     initDefaultParameters();

    status_t setParameters(const char* parms);
    status_t setParameters(const CameraParameters& params);
    char*    getParameters();
    void     putParameters(char* params);

    /****************************************************************************
     * Preview
     ***************************************************************************/
    status_t setPreviewWindow(struct preview_stream_ops* window);
    status_t startPreview();
    void     stopPreview();
    void     stopPreviewInternal();
    status_t startPreviewHW();
    int      getPictureSize(const CameraParameters& params, int *width, int *height);
    void     doZoom(int izoomValue);
    bool     isPreviewEnabled();
    void     getZoomedSize(int width, int height, int *rwidth, int *rheight);
    void     setSkipPreviewFrame(int frame);
    void     setSkipPictureFrame(int frame);

    void     trySetFlash(int flashmode, int delay_ms); 

    /****************************************************************************
     * Video Recording  
     ***************************************************************************/
    bool     isVideoRecordingEnabled();
    status_t enableVideoRecording();	
    void     disableVideoRecording();    
    status_t storeMetaDataInBuffers(bool enable);
    void     releaseRecordingFrame(char *opaque);

    /****************************************************************************
     * takePicture
     ***************************************************************************/
    status_t takePicture();
    status_t cancelPicture();

    /****************************************************************************
     * Callback  
     ***************************************************************************/
    void    setCallbacks(camera_notify_callback notify_cb,
                         camera_data_callback data_cb,
                         camera_data_timestamp_callback data_cb_timestamp,
                         camera_request_memory get_memory,
                         void* user);

    void    postFrameToPreviewWindow();
    void    frameCallbackNotifier(nsecs_t timestamp);

    /****************************************************************************
     * Message Control 
     ***************************************************************************/
    void    enableMessage(uint msg_type);
    void    disableMessage(uint msg_type);
    bool    isMessageEnabled(uint msg_type);

    /****************************************************************************
     * Camera Error
     ***************************************************************************/
    void    onCameraDeviceError(int err);

    /****************************************************************************
     * autoFocus
     ***************************************************************************/
    status_t    autoFocus();
    status_t    cancelAutoFocus();

    /****************************************************************************
     * others
     ***************************************************************************/	
    void     releaseCamera(void);
    status_t sendCommand(unsigned int cmd, unsigned int arg1, unsigned int arg2);

	int scalePictiure(CameraMemory* picraw,int* srcWidth, int* srcHeight,
                                  CameraMemory* dest_mem);
private:
    class PreviewThread : public Thread {
        CameraHardware* mHardware;
    public:
        PreviewThread(CameraHardware* hw)
        : Thread(false), mHardware(hw) { }

        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->inPreviewThread();
            // loop until we need to quit
            return false;
        }
    };
    status_t inPreviewThread(); 

    sp<PreviewThread>   mPreviewThread;    
    mutable Mutex		mPreviewLock;
    mutable Condition   mPreviewCondition;
    volatile bool       mExitPreviewThread; //ask to exit preview thread now.
    volatile bool       mPreviewEnabled;    
    volatile bool       mPreviewDeferred;
    volatile bool       mPreviewStopped;   

    class PictureThread : public Thread {
        CameraHardware *mHardware;
    public:
        PictureThread(CameraHardware *hw)
        : Thread(false), mHardware(hw)
        {
        }

        virtual bool threadLoop() {
            mHardware->inPictureThread();
            return false;
        }
    };
    status_t inPictureThread(); 
    sp<PictureThread>   mPictureThread;
    bool    mTakingPicture;
    bool    mPictureHandled;

    // auto focus
    class AutoFocusThread : public Thread {
        CameraHardware *mHardware;
    public:
        AutoFocusThread(CameraHardware *hw): Thread(false), mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraAutoFocusThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->inAutoFocusThread();
            return true;
        }
    };
    status_t inAutoFocusThread();
    sp<AutoFocusThread>     mAutoFocusThread;
    mutable Mutex			mFocusLock;
    mutable Condition       mFocusCondition;
    bool                    mExitAutoFocusThread;

    /****************************************************************************
     * Data members 
     ***************************************************************************/
    V4L2Camera mCamera;
    camera_property   *  mCamProp;

    /* Locks this instance for data changes. */    
    mutable Mutex			mVideoRecLock;

    /* Locks this instance for data change. */
    Mutex 					mObjectLock;
    mutable Mutex			mParamLock;

    /* Camera parameters. */
    CameraParameters		mParameters;

    CameraMemory            mPreviewMemory;
    CameraMemory     		mRecordMemory;

    preview_stream_ops*		mPreviewWindow;

    swExif mswExif;
    int     getZoomRatios();
    int *   mZoomRatios;

    /****************************************************************************
     * Preview parameter 
     ***************************************************************************/
    int		mPreviewFormat;
    int		mPreviewFrameWidth;
    int		mPreviewFrameHeight;
    int 	mPreviewFrameSize;

    bool 	mZoomSupport;
    int 	mCurrentZoomIndex;
    int 	mTargetZoomIndex;
    int 	mPicDelay;
    int     mSkipPreviewFrame;
    int     mSkipPictureFrame;

    /****************************************************************************
     * Recording parameter 
     ***************************************************************************/	
    bool		mVideoRecEnabled;

    /****************************************************************************
     * Picture
     ***************************************************************************/
    int     mJpegQuality;
    bool    mHasthumbnail;

    bool    mLiveCamOptimize;

    /****************************************************************************
     * Callback parameter 
     ***************************************************************************/
    camera_notify_callback          mNotifyCB;
    camera_data_callback            mDataCB;
    camera_data_timestamp_callback  mDataCBTimestamp;
    camera_request_memory           mGetMemoryCB;
    void*                           mCBOpaque;
    unsigned int                    mMessageEnabler;

    void *mCurrentFrame;
};

}; // namespace android

#endif

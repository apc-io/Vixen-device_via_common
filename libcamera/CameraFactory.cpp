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
#define LOG_TAG "CameraFactory"

#include <cutils/log.h>
#include "CameraHardware.h"
#include "UbootParam.h"
#include <camera/Camera.h>
#include <stdio.h>
#include <string.h>


using namespace android;

static int camera_open(const hw_module_t* module, const char* name,
                            hw_device_t** device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

camera_property gCameraProperty[MAX_CAMERAS_SUPPORTED];

/*
 * Required HAL header.
 */
struct hw_module_methods_t CameraModuleMethods = {
    open: camera_open
};


camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag:           HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id:            CAMERA_HARDWARE_MODULE_ID,
        name:          "WM8880 Camera Module",
        author:        "WonderMedia Dev Team",
        methods:       &CameraModuleMethods,
        dso:           NULL,
        reserved:      {0},
    },
    get_number_of_cameras : camera_get_number_of_cameras,
    get_camera_info       : camera_get_camera_info,
};

static inline CameraHardware * hardware(struct camera_device * device)
{
    return (CameraHardware *)device->priv;
}

/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/
static int camera_set_preview_window(struct camera_device * device,
                                     struct preview_stream_ops *window)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return -hw->setPreviewWindow(window);
}

static void camera_set_callbacks(struct camera_device * device,
                                 camera_notify_callback notify_cb,
                                 camera_data_callback data_cb,
                                 camera_data_timestamp_callback data_cb_timestamp,
                                 camera_request_memory get_memory,
                                 void *user)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    hw->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

static void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGV("Call %s: %x", __FUNCTION__, msg_type);
    CameraHardware * hw = hardware(device);
    hw->enableMessage(msg_type);
}

static void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGV("Call %s: %x", __FUNCTION__, msg_type);
    CameraHardware * hw = hardware(device);

    hw->disableMessage(msg_type);
}

static int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    ALOGV("Call %s: %x", __FUNCTION__, msg_type);
    CameraHardware * hw = hardware(device);

    return hw->isMessageEnabled(msg_type);
}

static int camera_start_preview(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return hw->startPreview();
}

static void camera_stop_preview(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);

    hw->stopPreview();
}

static int camera_preview_enabled(struct camera_device * device)
{
    CameraHardware * hw = hardware(device);

    int ret = hw->isPreviewEnabled();
    ALOGV("Call %s ret %d", __FUNCTION__, ret);
    return ret;
}

static int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    ALOGV("Call %s: %d", __FUNCTION__, enable);
    CameraHardware * hw = hardware(device);

    return hw->storeMetaDataInBuffers(enable);
}

static int camera_start_recording(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return hw->enableVideoRecording();
}

static void camera_stop_recording(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    hw->disableVideoRecording();
}

static int camera_recording_enabled(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);

    return hw->isVideoRecordingEnabled();
}

static void camera_release_recording_frame(struct camera_device * device,
                                           const void *opaque)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    hw->releaseRecordingFrame((char *)opaque);
}

static int camera_auto_focus(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);

    return hw->autoFocus();
}

static int camera_cancel_auto_focus(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
   CameraHardware * hw = hardware(device);

    return hw->cancelAutoFocus();
}

static int camera_take_picture(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return hw->takePicture();
}

static int camera_cancel_picture(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return hw->cancelPicture();
}

static int camera_set_parameters(struct camera_device * device, const char *params)
{
    ALOGV("Call %s:\n  %s", __FUNCTION__, params);
    CameraHardware * hw = hardware(device);
    return hw->setParameters(params);
}

static char* camera_get_parameters(struct camera_device * device)
{
    CameraHardware * hw = hardware(device);

    char * ret = hw->getParameters();
    ALOGV("Call %s ret:\n%s", __FUNCTION__, ret);

    return ret;
}

static void camera_put_parameters(struct camera_device *device, char *params)
{
    ALOGV("Call %s:\n  %s", __FUNCTION__, params);
    CameraHardware * hw = hardware(device);

    hw->putParameters(params);
}

static int camera_send_command(struct camera_device * device,
                               int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGV("Call %s: %d,%d,%d", __FUNCTION__, cmd, arg1, arg2);
    CameraHardware * hw = hardware(device);
    return hw->sendCommand(cmd, arg1, arg2);
}

static void camera_release(struct camera_device * device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    hw->releaseCamera();
}

static int camera_dump(struct camera_device * device, int fd)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware(device);
    return 0;
    //return hw->dumpCamera(fd);
}

static int camera_device_close(hw_device_t* device)
{
    ALOGV("Call %s", __FUNCTION__);
    CameraHardware * hw = hardware((camera_device * )device);
    delete hw;
    return 0;
}

static android::CameraInfo sCameraInfo[] = {
    {
        CAMERA_FACING_BACK,
        0,                     /* orientation */
    },
    {
        CAMERA_FACING_FRONT,
        180,                    /* orientation */
    },
};

static int wmt_camera_init(void)
{
    char envbuf[128];
    int len = sizeof(envbuf);
    camera_property cameraenv[MAX_CAMERAS_SUPPORTED];
    int n;

    memset(envbuf, 0, len);
    if (wmt_getsyspara(ENV_NAME_CAMERA, envbuf, &len)) {
        ALOGW("Read %s env failed. No camera present!", ENV_NAME_CAMERA);
        return 0;
    }

    memset(cameraenv, 0, sizeof(cameraenv));
    camera_property* c0 = &cameraenv[0];
    camera_property* c1 = &cameraenv[1];

    n = sscanf(envbuf, "%d:%d:%d:%d:%x:%d:%d:%d:%d:%x",
               &c1->type, &c1->gpio, &c1->active, &c1->mirror, &c1->cap,
               &c0->type, &c0->gpio, &c0->active, &c0->mirror, &c0->cap);
    if (n < 5) {
        ALOGW("Invalid %s env param. No camera present!", ENV_NAME_CAMERA);
        return 0;
    }

    n = 0;
    for (int i = 0; i < MAX_CAMERAS_SUPPORTED; i++) {
        camera_property* c = &cameraenv[i];
        if (c->type) {
            c->facing = sCameraInfo[i].facing;
            c->orientation = sCameraInfo[i].orientation;
            c->cameraId = n;
            c->retryCount = (c->type == CAMERA_TYPE_USB) ? 20 : 1;
            c->directREC = false;//temp close direct record

            gCameraProperty[n] = *c;

            ALOGD("Camera Env #%d: type %d, gpio %d, pol %d, mirror %d, cap %x",
                  n, c->type, c->gpio, c->active, c->mirror, c->cap);
            n++;
        }
    }
    return n;
}

static camera_device_ops_t camera_ops = 
{
    set_preview_window :  camera_set_preview_window,
    set_callbacks      :  camera_set_callbacks,
    enable_msg_type    :  camera_enable_msg_type,
    disable_msg_type   :  camera_disable_msg_type,
    msg_type_enabled   :  camera_msg_type_enabled,
    start_preview      :  camera_start_preview,
    stop_preview       :  camera_stop_preview,
    preview_enabled    :  camera_preview_enabled,
    store_meta_data_in_buffers  :  camera_store_meta_data_in_buffers,
    start_recording    :  camera_start_recording,
    stop_recording     :  camera_stop_recording,
    recording_enabled  :  camera_recording_enabled,
    release_recording_frame  :  camera_release_recording_frame,
    auto_focus         :  camera_auto_focus,
    cancel_auto_focus  :  camera_cancel_auto_focus,
    take_picture       :  camera_take_picture,
    cancel_picture     :  camera_cancel_picture,
    set_parameters     :  camera_set_parameters,
    get_parameters     :  camera_get_parameters,
    put_parameters     :  camera_put_parameters,
    send_command       :  camera_send_command,
    release            :  camera_release,
    dump               :  camera_dump,
};


static int camera_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    ALOGV("Call %s, name=%s", __FUNCTION__, name);

    int cameraid;

    *device = NULL;

    if (name == NULL) {
        ALOGE("camera open null name");
        return EINVAL;
    }

    cameraid = atoi(name);
    if (cameraid > MAX_CAMERAS_SUPPORTED || cameraid < 0) {
        ALOGE("camera service provided cameraid out of bounds, "
              "cameraid = %d, num supported = %d",
              cameraid, MAX_CAMERAS_SUPPORTED);
        return EINVAL;
    }

    camera_property * c = &gCameraProperty[cameraid];

    if (!c->type) {
        ALOGE("camera open invalid cameraid %s", name);
        return EINVAL;
    }

    if(c->cameraId != cameraid){
        ALOGE("Bug!!! unmatched camera id %d vs %d", cameraid, c->cameraId);
        return EINVAL;
    }

    CameraHardware * hw = new CameraHardware(c);
    if (!hw) {
        ALOGE("CameraHardware allocation fail");
        return ENOMEM;
    }

    hw->common.tag     = HARDWARE_DEVICE_TAG;
    hw->common.version = 0;
    hw->common.module  = (hw_module_t *)(module);
    hw->common.close = camera_device_close;
    hw->ops = &camera_ops;
    hw->priv = hw;

    *device = &hw->common;
    return 0;
}


static int camera_get_number_of_cameras(void)
{
    int n = wmt_camera_init();
    ALOGV("Call %s ret %d", __FUNCTION__, n);
    return n;
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{    
    memset(info, 0, sizeof(*info));

    info->facing = gCameraProperty[camera_id].facing;
    info->orientation = gCameraProperty[camera_id].orientation;

    ALOGD("Call %s: id: %d, facing %d, orientation %d",
          __FUNCTION__, camera_id, info->facing, info->orientation);
    return 0;
}
 
// vim: et ts=4 shiftwidth=4

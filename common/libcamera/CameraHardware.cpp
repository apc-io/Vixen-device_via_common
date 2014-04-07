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
#define LOG_TAG "CameraHardware"

#include <utils/Log.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <utils/Timers.h>
#include <cutils/properties.h>

#include "UbootParam.h"
#include "CameraHardware.h"
#include "videodev2_wmt.h"

#define CAMHAL_GRALLOC_USAGE (GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER)

#ifdef LOG_NDEBUG
    #define TRACE_FUNC()   FunctionTracer funcTrace(__FUNCTION__)
#else
    #define TRACE_FUNC()
#endif

#define CAMERA_AUTO_FOCUS_DISTANCES_STR     "0.10,1.20,Infinity"


namespace android {

/* current time in ms */
static inline int64_t currentTime() {
    return nanoseconds_to_milliseconds(systemTime(SYSTEM_TIME_MONOTONIC));
}


#define WMT_CAMERA_SCALE    "ro.wmt.camera.scale"

int getCameraScale(char *value)
{
   return property_get(WMT_CAMERA_SCALE, value, NULL);    
}

CameraHardware::CameraHardware(camera_property* p)
    : mCamera(p)
{
    TRACE_FUNC();

    mCamProp = p;

    mVideoRecEnabled = false;
    mMessageEnabler = 0;

    mTakingPicture = false;
    mPictureHandled = false;

    mPreviewWindow = NULL;
    mExitPreviewThread = false;
    mPreviewEnabled = false;
    mPreviewDeferred = true;
    mPreviewStopped = true;

    mExitAutoFocusThread = false;

    mCurrentZoomIndex = 0;
    mTargetZoomIndex = 0;
    mPicDelay = -1;
    mHasthumbnail = true;
    mLiveCamOptimize = false;

    mZoomSupport = false;
    mSkipPreviewFrame = 0;
    mSkipPictureFrame = 0;
    mCurrentFrame = NULL;

    mZoomRatios = NULL;

    if (mCamera.open() < 0) {
        ALOGE("ERR(%s):Fail to open %d", __FUNCTION__, p->cameraId);
        return;
    }

    initDefaultParameters();

    mPreviewThread = new PreviewThread(this);
    mPictureThread = new PictureThread(this);
    mAutoFocusThread = new AutoFocusThread(this);
}

CameraHardware::~CameraHardware()
{
    TRACE_FUNC();
    mCamera.close();
}

int CameraHardware::getZoomRatios()
{
    int ratios[] = {100,110,120,130,140,150,160,170,
		180,190,200,210,220,230,240,250,260,270,280};
    mZoomRatios = (int *) calloc(1, sizeof(ratios));
    memcpy(mZoomRatios, ratios, sizeof(ratios));
    return sizeof(ratios) / sizeof(ratios[0]);
}

void CameraHardware::setSkipPreviewFrame(int frame)
{
    Mutex::Autolock locker(&mObjectLock);
    if (frame < mSkipPreviewFrame)
        return;

    mSkipPreviewFrame = frame;
}

void CameraHardware::setSkipPictureFrame(int frame)
{
    Mutex::Autolock locker(&mObjectLock);
    if (frame < mSkipPictureFrame)
        return;

    mSkipPictureFrame = frame;
}

// Parse string like "(1, 2, 3, 4, ..., N)"
// num is pointer to an allocated array of size N
static int parseNDimVector(const char *str, int *num, int N, char delim = ',')
{
    char *start, *end;
    if(num == NULL) {
        ALOGE("Invalid output array (num == NULL)");
        return -1;
    }
    //check if string starts and ends with parantheses
    if(str[0] != '(' || str[strlen(str)-1] != ')') {
        ALOGE("Invalid format of string %s, valid format is (n1, n2, n3, n4 ...)", str);
        return -1;
    }
    start = (char*) str;
    start++;
    for(int i=0; i<N; i++) {
        *(num+i) = (int) strtol(start, &end, 10);
        if(*end != delim && i < N-1) {
            ALOGE("Cannot find delimeter '%c' in string \"%s\". end = %c", delim, str, *end);
            return -1;
        }
        start = end+1;
    }
    return 0;
}

// parse string like "(1, 2, 3, 4, 5),(1, 2, 3, 4, 5),..."
static int parseCameraAreaString(const char* str, int max_num_areas,
                                 camera_area_t *pAreas, int *num_areas_found)
{
    char area_str[32];
    const char *start, *end, *p;
    start = str; end = NULL;
    int values[5], index=0;
    *num_areas_found = 0;

    while(start != NULL) {
       if(*start != '(') {
            ALOGE("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       end = strchr(start, ')');
       if(end == NULL) {
            ALOGE("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       int i;
       for (i=0,p=start; p<=end; p++, i++) {
           area_str[i] = *p;
       }
       area_str[i] = '\0';
       if(parseNDimVector(area_str, values, 5) < 0){
            ALOGE("%s: error: Failed to parse the area string: %s", __func__, area_str);
            return -1;
       }
       // no more areas than max_num_areas are accepted.
       if(index >= max_num_areas) {
            ALOGE("%s: error: too many areas specified %s", __func__, str);
            return -1;
       }
       pAreas[index].x1 = values[0];
       pAreas[index].y1 = values[1];
       pAreas[index].x2 = values[2];
       pAreas[index].y2 = values[3];
       pAreas[index].weight = values[4];

       index++;
       start = strchr(end, '('); // serach for next '('
    }
    (*num_areas_found) = index;
    return 0;
}

static bool validateCameraAreas(camera_area_t *areas, int num_areas)
{
    for(int i=0; i<num_areas; i++) {

        // handle special case (0, 0, 0, 0, 0)
        if((areas[i].x1 == 0) && (areas[i].y1 == 0)
            && (areas[i].x2 == 0) && (areas[i].y2 == 0) && (areas[i].weight == 0)) {
            continue;
        }
        if(areas[i].x1 < -1000) return false;               // left should be >= -1000
        if(areas[i].y1 < -1000) return false;               // top  should be >= -1000
        if(areas[i].x2 > 1000) return false;                // right  should be <= 1000
        if(areas[i].y2 > 1000) return false;                // bottom should be <= 1000
        if(areas[i].weight <= 0 || areas[i].weight > 1000)  // weight should be in [1, 1000]
            return false;
        if(areas[i].x1 >= areas[i].x2) {                    // left should be < right
            return false;
        }
        if(areas[i].y1 >= areas[i].y2)                      // top should be < bottom
            return false;
    }
    return true;
}

void CameraHardware::initDefaultParameters()
{
    TRACE_FUNC();

    CameraParameters p;
    String8 parameterString;
    int i = 0, max;
    char str[16];
    char strBuf[256];

    p.setPreviewSize(640, 480);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "176x144,320x240,640x480");
    p.setPreviewFormat("yuv420sp");// CameraTest.java say that default format should be yuv420sp
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, "yuv420sp,yuv420p");

    // frame-rate
    p.setPreviewFrameRate(15);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, "5,15,30");
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "5000,30000");
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(5000,30000)");

    //video
    //p.set(CameraParameters::KEY_VIDEO_SIZE, "640x480");
    //p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, "176x144,320x240,640x480");
    //p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, "640x480");
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    // jpeg
    p.setPictureFormat("jpeg");
    p.set(CameraParameters::KEY_JPEG_QUALITY, "90");
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, "jpeg");

    // picture size
    int index = 0;
    bool flagFirst = true;
    int width = 640, height = 480;
    parameterString.setTo("640x480");
    while (mCamera.v4l2_ioctl_enum_framesizes(index, &width, &height) == 0) {
        if (flagFirst == true) {
            flagFirst = false;
            parameterString.setTo("");
        } else
            parameterString.append(",");
        sprintf(strBuf, "%dx%d", width, height);
        parameterString.append(strBuf);
        ++index;
    }
    p.setPictureSize(width, height);
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, parameterString.string());

    // thumbnail
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "160");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "120");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, "160x120,0x0");

    // focus
    struct v4l2_queryctrl focus;
    focus.id = V4L2_CID_FOCUS_AUTO;
    if (!mCamera.v4l2_ioctl_queryctrl(&focus)) {
        parameterString.setTo("");
        parameterString.append(CameraParameters::FOCUS_MODE_AUTO);
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_FIXED);
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);

        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_INFINITY);
//        parameterString.append(",");
//        parameterString.append(CameraParameters::FOCUS_MODE_MACRO);
//        parameterString.append(",");
//        parameterString.append(CameraParameters::FOCUS_MODE_EDOF);
//        parameterString.append(",");
//        parameterString.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);

        p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, parameterString.string());
        p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_AUTO);
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 1);
    } else {
        p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, CameraParameters::FOCUS_MODE_FIXED);
        p.set(CameraParameters::KEY_FOCUS_MODE, CameraParameters::FOCUS_MODE_FIXED);
    }
    p.set(CameraParameters::KEY_FOCAL_LENGTH, "2.78");
    p.set(CameraParameters::KEY_FOCUS_DISTANCES, CAMERA_AUTO_FOCUS_DISTANCES_STR);

    // exposure
    struct v4l2_queryctrl exposure;
    exposure.id = V4L2_CID_EXPOSURE;
    if (!mCamera.v4l2_ioctl_queryctrl(&exposure)) {
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, exposure.default_value);
        p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, exposure.maximum);
        p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, exposure.minimum);
        sprintf(str, "%f", (float)exposure.step); 
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, str);
    } else {
        p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION,"6");
        p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION,"-6");
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP,"0.3333333333");
    }

    // zoom
    max = getZoomRatios();

    if ((max != 0)&&(mZoomRatios != NULL)) {
        p.set(CameraParameters::KEY_ZOOM, "0");
        sprintf(str, "%d", (max-1));
        p.set(CameraParameters::KEY_MAX_ZOOM, str);
        sprintf(str, "%d", mZoomRatios[0]);
        parameterString = str;
        for (i = 1; i < max; i++) {
            parameterString.append(",");
            sprintf(str, "%d", mZoomRatios[i]);
            parameterString.append(str);
        }
        p.set(CameraParameters::KEY_ZOOM_RATIOS, parameterString.string());
        p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");
        p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        mZoomSupport = true;
    } else {
        ALOGW("Zoom is not support");
        p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
    }

    // white balance
    if (mCamProp->type == CAMERA_TYPE_CMOS) {
            parameterString.setTo("");
            parameterString.append(CameraParameters::WHITE_BALANCE_AUTO);
            parameterString.append(",");
            parameterString.append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
            parameterString.append(",");
            parameterString.append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
            parameterString.append(",");
            parameterString.append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
            parameterString.append(",");
            parameterString.append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
            parameterString.append(",");
            parameterString.append(CameraParameters::WHITE_BALANCE_TWILIGHT);
            p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, parameterString.string());
            p.set(CameraParameters::KEY_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);
    } else {
            p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);
            p.set(CameraParameters::KEY_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);
    }

    // scene mode
    if (mCamProp->type == CAMERA_TYPE_CMOS) {
        parameterString.setTo("");
        parameterString.append(CameraParameters::SCENE_MODE_AUTO);
        parameterString.append(",");
        parameterString.append(CameraParameters::SCENE_MODE_NIGHT);

        // hdr
        if (mCamProp->cap & CAMERA_CAP_HDR) {
            parameterString.append(",");
            parameterString.append(CameraParameters::SCENE_MODE_HDR);
        }

        p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, parameterString.string());
        p.set(CameraParameters::KEY_SCENE_MODE, CameraParameters::SCENE_MODE_AUTO);
    } else {
            p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, CameraParameters::SCENE_MODE_AUTO);
            p.set(CameraParameters::KEY_SCENE_MODE, CameraParameters::SCENE_MODE_AUTO);
    }

    // flash
    if (mCamProp->cap & CAMERA_CAP_FLASH) {
        parameterString.setTo("");
        parameterString.append(CameraParameters::FLASH_MODE_OFF);
        parameterString.append(",");
        parameterString.append(CameraParameters::FLASH_MODE_ON);
        parameterString.append(",");
        parameterString.append(CameraParameters::FLASH_MODE_TORCH);
        if (mCamProp->cap & CAMERA_CAP_FLASH_AUTO) {
            parameterString.append(",");
            parameterString.append(CameraParameters::FLASH_MODE_AUTO);
        }

        p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, parameterString.string());
        p.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_OFF);
    }

    //others
    p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "60");
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "90");

    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, "0");
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, "0");
    p.set(CameraParameters::KEY_ROTATION, 0);

    p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");

    mParameters = p;
    if (setParameters(p) != NO_ERROR) {
        ALOGE("Failed to set default parameters?!");
    }
}

status_t CameraHardware::setParameters(const CameraParameters& params)
{
    TRACE_FUNC();

    int val[2];

    Mutex::Autolock lock(mParamLock);

    ALOGV("Camera parameters: %s", params.flatten().string());

    // preview format
    const char *strNewPreviewFormat = params.getPreviewFormat();
    if (!strcmp(strNewPreviewFormat, "yuv420sp"))
        mPreviewFormat = V4L2_PIX_FMT_NV21;
    else if (!strcmp(strNewPreviewFormat, "yuv420p"))
        mPreviewFormat = V4L2_PIX_FMT_YVU420;   /* YV12 */
    else {
        ALOGE("preview not supported %s", params.getPreviewFormat());
        return BAD_VALUE;
    }
    mParameters.setPreviewFormat("yuv420sp");

    // preview size
    int newPreviewW, newPreviewH;
    params.getPreviewSize(&newPreviewW, &newPreviewH);
    ALOGV("DEBUG(%s):newPreviewW x newPreviewH = %dx%d", __FUNCTION__, newPreviewW, newPreviewH);
    if ((newPreviewW <= 0) || (newPreviewH <= 0))
        return BAD_VALUE;
    mParameters.setPreviewSize(newPreviewW, newPreviewH);

    // picture format
    const char *newPictureFormat = params.getPictureFormat();
    if (strcmp(newPictureFormat, "jpeg") != 0) {
        ALOGE("Only jpeg still pictures are supported");
        return BAD_VALUE;
    }
    mParameters.setPictureFormat(newPictureFormat);

    int newPictureW = 0;
    int newPictureH = 0;
    params.getPictureSize(&newPictureW, &newPictureH);
    ALOGV("DEBUG(%s):newPictureW x newPictureH = %dx%d", __FUNCTION__, newPictureW, newPictureH);
    if (0 < newPictureW && 0 < newPictureH) {
        mParameters.setPictureSize(newPictureW, newPictureH);
    }

    // JPEG image quality
    int newJpegQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    ALOGV("DEBUG(%s):newJpegQuality %d", __FUNCTION__, newJpegQuality);
    // we ignore bad values
    if (newJpegQuality >=1 && newJpegQuality <= 100) {
        mParameters.set(CameraParameters::KEY_JPEG_QUALITY, newJpegQuality);
    }

    // JPEG thumbnail size
    int newJpegThumbnailW = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int newJpegThumbnailH = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if (0 <= newJpegThumbnailW && 0 <= newJpegThumbnailH) {
        mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  newJpegThumbnailW);
        mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, newJpegThumbnailH);
    }

    // JPEG thumbnail quality
    int newJpegThumbnailQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    ALOGV("DEBUG(%s):newJpegThumbnailQuality %d", __FUNCTION__, newJpegThumbnailQuality);
    // we ignore bad values
    if (newJpegThumbnailQuality >=1 && newJpegThumbnailQuality <= 100) {
        mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, newJpegThumbnailQuality);
    }

    // fps range
    int newMinFps = 0;
    int newMaxFps = 0;
    params.getPreviewFpsRange(&newMinFps, &newMaxFps);
    /* Check basic validation if scene mode is different */
    if ((newMaxFps < newMinFps) || (newMinFps < 0) || (newMaxFps < 0))
        return BAD_VALUE;

    // frame rate
    int newFrameRate = params.getPreviewFrameRate();
    ALOGV("DEBUG(%s):newFrameRate %d", __FUNCTION__, newFrameRate);
    // ignore any fps request, we're determine fps automatically based
    // on scene mode.  don't return an error because it causes CTS failure.
    if (newFrameRate != mParameters.getPreviewFrameRate()) {
        mParameters.setPreviewFrameRate(newFrameRate);
    }
    mCamera.swSetFrameRate(newFrameRate);

    // zoom
    if (mZoomSupport == true) {
        mTargetZoomIndex = params.getInt(CameraParameters::KEY_ZOOM);
        int maxZoomIndex = params.getInt(CameraParameters::KEY_MAX_ZOOM);
        ALOGV("DEBUG(%s): mTargetZoomIndex %d, maxZoomIndex %d",
              __FUNCTION__, mTargetZoomIndex, maxZoomIndex);
        if (mTargetZoomIndex > maxZoomIndex)
            return BAD_VALUE;

        mParameters.set(CameraParameters::KEY_ZOOM, mTargetZoomIndex);
    }

    // rotate
    int newRotation = params.getInt(CameraParameters::KEY_ROTATION);
    mCamera.setRotation(newRotation);
    mParameters.set(CameraParameters::KEY_ROTATION, newRotation);

    // scene mode
    const char *strNewSceneMode = params.get(CameraParameters::KEY_SCENE_MODE);
    const char *strNewFlashMode = params.get(CameraParameters::KEY_FLASH_MODE);
    const char *strNewWhitebalance = params.get(CameraParameters::KEY_WHITE_BALANCE);

    int minExposureCompensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
    int maxExposureCompensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
    int newExposureCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);

    ALOGV("DEBUG(%s):strNewSceneMode %s", __FUNCTION__, strNewSceneMode);
    if (strNewSceneMode != NULL) {
        int newSceneMode = -1;
        if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_AUTO)) {
            newSceneMode = SCENE_MODE_AUTO;
        } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT)) {
            newSceneMode = SCENE_MODE_NIGHTSHOT;
            strNewFlashMode = CameraParameters::FLASH_MODE_OFF;
            strNewWhitebalance = CameraParameters::WHITE_BALANCE_AUTO;
        } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_HDR)) {
            newSceneMode = SCENE_MODE_AUTO;
//            newExposureCompensation = 0;
            strNewFlashMode = CameraParameters::FLASH_MODE_OFF;
        } else {
            ALOGE("ERR(%s):Invalid scene mode(%s)", __FUNCTION__, strNewSceneMode);
            return UNKNOWN_ERROR;
        }
        if (newSceneMode >= 0) {
            mCamera.setSceneMode(newSceneMode);
            mParameters.set(CameraParameters::KEY_SCENE_MODE, strNewSceneMode);
        }
    }

    // exposure
    ALOGV("DEBUG(%s):newExposureCompensation %d", __FUNCTION__, newExposureCompensation);
    if ((minExposureCompensation <= newExposureCompensation) &&
        (newExposureCompensation <= maxExposureCompensation)) {
        mCamera.setExposureCompensation(newExposureCompensation);
        mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, newExposureCompensation);
    }

    // white balance
    ALOGV("DEBUG(%s):strNewWhitebalance %s", __FUNCTION__, strNewWhitebalance);
    if (strNewWhitebalance != NULL) {
        int value = -1;
        if (!strcmp(strNewWhitebalance, CameraParameters::WHITE_BALANCE_AUTO))
            value = WHITE_BALANCE_AUTO;
        else if (!strcmp(strNewWhitebalance, CameraParameters::WHITE_BALANCE_INCANDESCENT))
            value = WHITE_BALANCE_INCANDESCENCE;
        else if (!strcmp(strNewWhitebalance, CameraParameters::WHITE_BALANCE_FLUORESCENT))
            value = WHITE_BALANCE_FLUORESCENT;
        else if (!strcmp(strNewWhitebalance, CameraParameters::WHITE_BALANCE_DAYLIGHT))
            value = WHITE_BALANCE_DAYLIGHT;
        else if (!strcmp(strNewWhitebalance, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
            value = WHITE_BALANCE_CLOUDY;
        else {
            //twilight, shade, warm_flourescent
            ALOGE("ERR(%s):Invalid white balance(%s)", __FUNCTION__, strNewWhitebalance);
            return UNKNOWN_ERROR;
        }
        if (value >= 0) {
            mCamera.setWhiteBalance(value);
            mParameters.set(CameraParameters::KEY_WHITE_BALANCE, strNewWhitebalance);
        }
    }

    // flash
    ALOGV("DEBUG(%s):strNewFlashMode %s", __FUNCTION__, strNewFlashMode);
    if (strNewFlashMode != NULL) {
        int  newFlashMode = -1;
        if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_OFF)) {
            newFlashMode = FLASH_MODE_OFF;
        } else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_TORCH)) {
            newFlashMode = FLASH_MODE_TORCH;
        } else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_AUTO)) {
            newFlashMode = FLASH_MODE_AUTO;
        } else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_ON)) {
            newFlashMode = FLASH_MODE_ON;
        } else {
            ALOGE("ERR(%s):unmatched flash_mode(%s)", __FUNCTION__, strNewFlashMode);
            return UNKNOWN_ERROR;
        }
        if (newFlashMode >= 0) {
            mCamera.setFlashMode(newFlashMode);
            mParameters.set(CameraParameters::KEY_FLASH_MODE, strNewFlashMode);
        }
    }

    // focus areas
    const char *strNewFocusAreas = params.get(CameraParameters::KEY_FOCUS_AREAS);
    int max_num_af_areas = mParameters.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);
    if (strNewFocusAreas != NULL && max_num_af_areas != 0) {
        camera_area_t *areas = new camera_area_t[max_num_af_areas];
        int num_areas_found = 0;
        if (parseCameraAreaString(strNewFocusAreas, max_num_af_areas, areas, &num_areas_found) < 0) {
            ALOGE("%s: Failed to parse the string: %s", __func__, strNewFocusAreas);
            delete areas;
            return BAD_VALUE;
        }
        for(int i = 0; i < num_areas_found; i++) {
            ALOGV("FocusArea[%d] = (%d, %d, %d, %d, %d)", i, (areas[i].x1), (areas[i].y1),
                        (areas[i].x2), (areas[i].y2), (areas[i].weight));
        }
        if (validateCameraAreas(areas, num_areas_found) == false) {
            ALOGE("%s: invalid areas specified : %s", __func__, strNewFocusAreas);
            delete areas;
            return BAD_VALUE;
        }
        //for special area string (0, 0, 0, 0, 0), set the num_areas_found to 0,
        //so no action is takenby the lower layer
        if(num_areas_found == 1 && (areas[0].x1 == 0) && (areas[0].y1 == 0)
            && (areas[0].x2 == 0) && (areas[0].y2 == 0) && (areas[0].weight == 0)) {
            num_areas_found = 0;
        }
        mParameters.set(CameraParameters::KEY_FOCUS_AREAS, strNewFocusAreas);
        if (num_areas_found == 1) {
            uint16_t x1, x2, y1, y2, dx, dy;
            //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
            x1 = (uint16_t)((areas[0].x1 + 1000.0f)*(newPreviewW/2000.0f));
            y1 = (uint16_t)((areas[0].y1 + 1000.0f)*(newPreviewH/2000.0f));
            x2 = (uint16_t)((areas[0].x2 + 1000.0f)*(newPreviewW/2000.0f));
            y2 = (uint16_t)((areas[0].y2 + 1000.0f)*(newPreviewH/2000.0f));
            dx = (x2 + x1)/2;
            dy = (y2 + y1)/2;
            ALOGD("%s: %dx%d, x1 %d, y1 %d, x2 %d, y2 %d, (%dx%d)", __FUNCTION__,
                  newPreviewW, newPreviewH, x1, y1, x2, y2, dx, dy);
            mCamera.setFocusArea(dx, dy);
        }
    }

    // focus
    const char *strNewFocusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
    ALOGV("DEBUG(%s):strNewFocusMode %s", __FUNCTION__, strNewFocusMode);
    if (strNewFocusMode != NULL) {

        int  newFocusMode = -1;
        if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_AUTO)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_FIXED)) {
            newFocusMode = FOCUS_MODE_FIXED;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_VIDEO;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_INFINITY)) {
            newFocusMode = FOCUS_MODE_INFINITY;
        } else {
            ALOGE("ERR(%s):unmatched focus_mode(%s)", __FUNCTION__, strNewFocusMode);
            return UNKNOWN_ERROR;
        }

        if (newFocusMode >= 0) {
            mCamera.setFocusMode(newFocusMode);
            mParameters.set(CameraParameters::KEY_FOCUS_MODE, strNewFocusMode);
            mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES, CAMERA_AUTO_FOCUS_DISTANCES_STR);
        }
    }

    // gps altitude
    const char *strNewGpsAltitude = params.get(CameraParameters::KEY_GPS_ALTITUDE);
    if (strNewGpsAltitude)
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, strNewGpsAltitude);
    else
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);

    // gps latitude
    const char *strNewGpsLatitude = params.get(CameraParameters::KEY_GPS_LATITUDE);
    if (strNewGpsLatitude)
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, strNewGpsLatitude);
    else
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);

    // gps longitude
    const char *strNewGpsLongtitude = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    if (strNewGpsLongtitude)
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, strNewGpsLongtitude);
    else
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);

    // gps processing method
    const char *strNewGpsProcessingMethod = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (strNewGpsProcessingMethod)
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, strNewGpsProcessingMethod);
    else
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);

    // gps timestamp
    const char *strNewGpsTimestamp = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
    if (strNewGpsTimestamp)
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, strNewGpsTimestamp);
    else
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);

    const char * liveCamOpt = params.get("wmt.livecam.opt");
    mLiveCamOptimize  = (mCamProp->directREC && liveCamOpt && atoi(liveCamOpt) == 1);
    if(mLiveCamOptimize)
        mPreviewFormat = V4L2_PIX_FMT_NV12;

    // color effect
    // anti shake
    // anti banding

    return NO_ERROR;
}

status_t CameraHardware::setParameters(const char* parms)
{
    TRACE_FUNC();

    CameraParameters new_param;
    String8 str8_param(parms);
    new_param.unflatten(str8_param);

    return setParameters(new_param);
}

char* CameraHardware::getParameters()
{
    ALOGV("Enter %s", __FUNCTION__);
    //Mutex::Autolock lock(mParamLock);
    String8 params(mParameters.flatten());
    return strdup(params.string());
}

/* This method simply frees parameters allocated in getParameters(). */
void CameraHardware::putParameters(char* params)
{
    TRACE_FUNC();
    free(params);
}

status_t CameraHardware::startPreview()
{
    TRACE_FUNC();

    status_t ret;

    Mutex::Autolock lock(mPreviewLock);

    if (mPreviewEnabled) {
        //already running
        ALOGW("startPreview:  but already running");
        return INVALID_OPERATION;
    }

    mPreviewEnabled = true;
    setSkipPreviewFrame(1);

    if (mPreviewWindow == NULL) {
        ALOGV("DEBUG(%s):deferring", __FUNCTION__);
        mPreviewDeferred = true;
        return NO_ERROR;
    }

    ret = startPreviewHW();
    if (ret != NO_ERROR) {
        ALOGE("startPreviewHW failed: %d", ret);
        return ret;
    }
    mPreviewDeferred = false;

    mPreviewCondition.signal();
    return ret;
}

void CameraHardware::stopPreview()
{
    TRACE_FUNC();

    /* if picture wait inPictureThread ending... */
    if (mTakingPicture == true) {
        for (int i = 0; i < 10; i++) {
            if (mPictureHandled)
                break;
            usleep(200000);
        }
    }

    /* workaround for cts testMultiCameraRelease.
     * call mCamera.close() if Preview running. */

    if (mPreviewEnabled == true) {
        stopPreviewInternal();
        mCamera.close();
    } else {
        /* stop after takepicture */
        stopPreviewInternal();
    }
}

void CameraHardware::stopPreviewInternal()
{
    TRACE_FUNC();

    const nsecs_t timeout   = milliseconds_to_nanoseconds(100);       //100ms
    const nsecs_t totalWait = 3000;            //3000ms

    Mutex::Autolock lock(mPreviewLock);
    mPreviewEnabled = false;
    mPreviewCondition.signal();

    //wait until preview really stopped or timeout
    nsecs_t start = currentTime();
    while (!mPreviewStopped) {
        mPreviewCondition.waitRelative(mPreviewLock, timeout);
        if (currentTime() - start > totalWait) {
            ALOGW("Wait stopPreview Timeout!");
            mCamera.stopPreview();
            mPreviewStopped = true;
            break;
        }
    }
}

bool CameraHardware::isPreviewEnabled()
{
    TRACE_FUNC();
    return mPreviewEnabled;
}

status_t CameraHardware::startPreviewHW()
{
    TRACE_FUNC();

    mParameters.getPreviewSize(&mPreviewFrameWidth, &mPreviewFrameHeight);
    mPreviewFrameSize = FRAME_SIZE(mPreviewFormat, mPreviewFrameWidth, mPreviewFrameHeight);
    ALOGD("Camera mPreviewFrameSize %d(%dx%d).", mPreviewFrameSize, mPreviewFrameWidth, mPreviewFrameHeight);

    if (mCamera.startPreview(mPreviewFrameWidth, mPreviewFrameHeight, mPreviewFormat) < 0) {
        ALOGE("Camera startPreview failed with %dx%d.", mPreviewFrameWidth, mPreviewFrameHeight);
        return UNKNOWN_ERROR;
    }

    if( !mPreviewMemory.alloc(mGetMemoryCB, mLiveCamOptimize ? sizeof(jdec_sync) : mPreviewFrameSize)) {
        return UNKNOWN_ERROR;
    }

    if (mCamProp->directREC)
        mRecordMemory.alloc(mGetMemoryCB, RECBUFNUMs*sizeof(struct record_pointer));
    else
        mRecordMemory.alloc(mGetMemoryCB, mPreviewFrameSize);

    if (!mRecordMemory.mem) {
        return UNKNOWN_ERROR;
    }

    if(mCamProp->directREC)
        memset(mRecordMemory.data(), 0, RECBUFNUMs*sizeof(struct record_pointer));

    if (mCamProp->mirror) {
        ALOGD("Mirror is enable");
        mCamera.setMirror(1);
    }

    if (mCamera.startStreaming()) {
        ALOGE("startPreview failed: device StartStreaming failed");
        return UNKNOWN_ERROR;
    }

    mPreviewStopped = false;
    return NO_ERROR;
}

int CameraHardware::inPreviewThread()
{
    while (!mExitPreviewThread) {
        {
            Mutex::Autolock lock(mPreviewLock);
            //wait mPreviewEnabled 
            while (!mPreviewEnabled && !mExitPreviewThread) {
                if (!mPreviewStopped) {
                    mCamera.stopPreview();
                    mPreviewStopped = true;
                }
                //notify Binder thread we are stopped.
                mPreviewCondition.broadcast();
                //and wait another request.
                mPreviewCondition.wait(mPreviewLock);
            }
            if(mExitPreviewThread)
                goto ExitPreview;
//            mPreviewStopped = false;
        }

        {
            //Mutex::Autolock lock(mObjectLock);
            //prevent mPreviewWindow changed.
            if ((mPreviewWindow != NULL) && mPreviewDeferred == false) {
                nsecs_t timeStamp = systemTime(SYSTEM_TIME_MONOTONIC);
                postFrameToPreviewWindow();
                frameCallbackNotifier(timeStamp);
            }
        }
    }
ExitPreview:
    {
        Mutex::Autolock lock(mPreviewLock);
        mCamera.stopPreview();
        mPreviewStopped = true;
        mPreviewCondition.broadcast();
        ALOGD("PreviewThread exited");
        return NO_ERROR;
    }
}

status_t CameraHardware::setPreviewWindow(struct preview_stream_ops* window)
{
    TRACE_FUNC();
    ALOGV("%s: current: %p -> new: %p", __FUNCTION__, mPreviewWindow, window);

    mPreviewWindow = window;

    if (window == NULL) {
        return NO_ERROR;
    }

    Mutex::Autolock lock(mPreviewLock);

    status_t res = NO_ERROR;

    mParameters.getPreviewSize(&mPreviewFrameWidth, &mPreviewFrameHeight);

    res = mPreviewWindow->set_buffer_count(mPreviewWindow, NB_BUFFER);
    if (res != NO_ERROR) {
        ALOGE("%s: Error in set_buffer_count %d -> %s",
              __FUNCTION__, -res, strerror(-res));
        return INVALID_OPERATION;
    }


    int exUsage = 0;
    if (mCamProp->directREC && mLiveCamOptimize)
        exUsage = GRALLOC_USAGE_WMT_MB;

    res = mPreviewWindow->set_usage(mPreviewWindow, CAMHAL_GRALLOC_USAGE | exUsage);
    if (res != NO_ERROR) {
        window = NULL;
        res = -res; // set_usage returns a negative errno.
        ALOGE("%s: Error setting preview window usage %d -> %s",
              __FUNCTION__, res, strerror(res));
    }

    res = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                                               mPreviewFrameWidth,
                                               mPreviewFrameHeight,
                                               HAL_PIXEL_FORMAT_YCrCb_420_SP);
    if (res != NO_ERROR) {
        ALOGE("%s: Error in set_buffers_geometry %d -> %s",
              __FUNCTION__, -res, strerror(-res));
        return INVALID_OPERATION;
    }

    int tmp = 0;
    mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &tmp);
    if (tmp >= NB_BUFFER) {
        ALOGE("undequeued_buffer = %d , NB_BUFFER = %d",tmp,NB_BUFFER);
    }

    if (mPreviewEnabled == true && mPreviewDeferred == true) {
        startPreviewHW();
        mPreviewDeferred = false;
        mPreviewCondition.signal();
    }

    return res;
}

void CameraHardware::getZoomedSize(int width, int height, int *rwidth, int *rheight)
{
    int zoomValue;

    zoomValue = mZoomRatios[mCurrentZoomIndex];

    width = (int)((float)width * 100) / zoomValue;
    height = (int)((float)height * 100) / zoomValue;

    *rwidth = width & ~0x7;
    *rheight = height & ~0x7;

    ALOGV("zoomValue %d, width %d(%d), height %d(%d)",
          zoomValue, width, *rwidth, height, *rheight);
}

void CameraHardware::doZoom(int izoomValue)
{
    int zoomValue=0;
    float temp, left, top, right, bottom;

    mCurrentZoomIndex = mTargetZoomIndex;
    zoomValue = mZoomRatios[mTargetZoomIndex];
    temp=(float)zoomValue/100.0;
    left=mPreviewFrameWidth*(temp-1)/temp/2.0;
    top=mPreviewFrameHeight*(temp-1)/temp/2.0;
    right=mPreviewFrameWidth-left;
    bottom=mPreviewFrameHeight-top;

    ALOGV("zoomvalue %d, letf %f, top %f, right %f, bottom %f", zoomValue,
          left, top, right, bottom);

    mPreviewWindow->set_crop(mPreviewWindow, (int)left, (int)top, (int)right,
                             (int)bottom);
}

void CameraHardware::postFrameToPreviewWindow()
{
    //TRACE_FUNC;
    int res;

    while (mSkipPreviewFrame > 0) {
        mObjectLock.lock();
        mSkipPreviewFrame--;
        mObjectLock.unlock();
        mCamera.grabPreviewFrame(NULL, NULL);
        ALOGV("%s: skipping preview frame %d", __FUNCTION__, mSkipPreviewFrame);
    }

    /* Dequeue a window buffer for display. */
    buffer_handle_t* buffer = NULL;
    int stride = 0;
    res = mPreviewWindow->dequeue_buffer(mPreviewWindow, &buffer, &stride);
    if (res != NO_ERROR || buffer == NULL) {
        ALOGE("%s: Unable to dequeue preview window buffer: %d -> %s",
              __FUNCTION__, -res, strerror(-res));
        return;
    }

    /* Let the preview window to lock the buffer. */
    res = mPreviewWindow->lock_buffer(mPreviewWindow, buffer);
    if (res != NO_ERROR) {
        ALOGE("%s: Unable to lock preview window buffer: %d -> %s",
              __FUNCTION__, -res, strerror(-res));
        mPreviewWindow->cancel_buffer(mPreviewWindow, buffer);
        return;
    }

    /* Now let the graphics framework to lock the buffer, and provide
     * us with the framebuffer data address. */
    void* bufferPtr = NULL;
    const Rect rect(mPreviewFrameWidth, mPreviewFrameHeight);
    GraphicBufferMapper& grbuffer_mapper(GraphicBufferMapper::get());
    res = grbuffer_mapper.lock(*buffer, CAMHAL_GRALLOC_USAGE, rect, &bufferPtr);
    if (res != NO_ERROR) {
        ALOGE("%s: grbuffer_mapper.lock failure: %d -> %s",
              __FUNCTION__, res, strerror(res));
        mPreviewWindow->cancel_buffer(mPreviewWindow, buffer);
        return;
    }

    struct jdec_sync sync;
    sync.yaddr = 0;
    {
        Mutex::Autolock lock(mVideoRecLock);
        gralloc_mb_block block;
        block.magic = 0;

        mCamera.grabPreviewFrame((char *)bufferPtr, mLiveCamOptimize ? &block : NULL);

        if( block.magic == 'WMMB') {
            //mPreviewWindow->set_usage(mPreviewWindow, CAMHAL_GRALLOC_USAGE | GRALLOC_USAGE_WMT_MB);
            memcpy(bufferPtr, &block, sizeof(block));
            sync.yaddr = block.phy_addr;
            sync.encodetag = VDO_COL_FMT_NV12;
            //note: bufferPtr is only 4K
        }
        mCurrentFrame = bufferPtr;
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        if (!mPreviewStopped) {
            if(mLiveCamOptimize && sync.yaddr != 0)
                memcpy(mPreviewMemory.data(), &sync, sizeof(jdec_sync));
            else
                memcpy(mPreviewMemory.data(), (char *)bufferPtr, mPreviewFrameSize);
            mDataCB(CAMERA_MSG_PREVIEW_FRAME, mPreviewMemory.mem, 0, NULL, mCBOpaque);
        }
    }

    if ((mCurrentZoomIndex != mTargetZoomIndex) && (mZoomSupport == true)) {
        ALOGV("Zoom Current index = %d",mTargetZoomIndex);
        doZoom(0);
    }

    //post this buffer and show it.
    mPreviewWindow->enqueue_buffer(mPreviewWindow, buffer);
    grbuffer_mapper.unlock(*buffer);
}

void CameraHardware::frameCallbackNotifier(nsecs_t timestamp)
{
    camera_memory_t* mem;
    unsigned int addr;

    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled()) {
        if ((mZoomSupport == true) && (mCurrentZoomIndex > 0)) {
            int cropw, croph;
            getZoomedSize(mPreviewFrameWidth, mPreviewFrameHeight, &cropw, &croph);
            nv21CropScale(mRecordMemory.udata(), mPreviewFrameWidth, mPreviewFrameHeight, cropw, croph);
        }
        if (mCamProp->directREC) {
            addr = *(unsigned int *)mRecordMemory.data();
            if (addr) {
                mem = (camera_memory_t *)addr;
                mDataCBTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME, mem, 0, mCBOpaque);
            } else
                ALOGW("RecordMemory is null");
        } else
            mDataCBTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME, mRecordMemory.mem, 0, mCBOpaque);
    }

    if(!isVideoRecordingEnabled() && (mCamProp->directREC))
        mCamera.setRecordBuf(NULL);
}

void CameraHardware::releaseRecordingFrame(char *opaque)
{
    Mutex::Autolock lock(mVideoRecLock);
    if (mCamProp->directREC) {
        mCamera.releaseRecordingFrame(opaque);
    }
}

int CameraHardware::getPictureSize(const CameraParameters& params, int *width, int *height)
{
    int w = 0, h = 0;
    params.getPictureSize(&w, &h);

    *width = w;
    *height = h;

    return (w * h * 12) / 8;
}

status_t CameraHardware::inPictureThread()
{
    TRACE_FUNC();

    status_t res = 0;
    NV21JpegCompressor compressor;
    NV21JpegCompressor thumbcomp;
    CameraMemory jpeg_buff;
    CameraMemory picraw;
    CameraMemory thumbraw;
    CameraMemory tmpMem;

    int picsize = 0;
    char patch[2];
    int exif_len, thumbsize = 0;
    char *jpegbuffer = NULL;
    int pictruew = 0, pictrueh = 0;
    int thumbw = 0, thumbh = 0;
    int previewsize = 0 , picturesize = 0;

    CameraParameters params;

    mPictureHandled = false;

    //if (mTakingPicture) {
    /* The sequence of callbacks during picture taking is:
     *  - CAMERA_MSG_SHUTTER
     *  - CAMERA_MSG_RAW_IMAGE_NOTIFY
     *  - CAMERA_MSG_COMPRESSED_IMAGE
     */
//    if (isMessageEnabled(CAMERA_MSG_SHUTTER)) {
//        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCBOpaque);
//    }
//    if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
//        mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCBOpaque);
//    }
    if (!isMessageEnabled(CAMERA_MSG_COMPRESSED_IMAGE)) {
        mTakingPicture = false;
        return 0;
    }

    {
        Mutex::Autolock lock(mParamLock);
        params = mParameters;
    }

    if (isVideoRecordingEnabled()) {
        pictruew    = mPreviewFrameWidth;
        pictrueh    = mPreviewFrameHeight;
        picturesize = (pictruew * pictrueh * 12) / 8;
    } else {
        picturesize = getPictureSize(params, &pictruew, &pictrueh);
        if (!picturesize ) {
            ALOGE("%s : Picture = %d",__FUNCTION__, picturesize);
            res = -1;
            goto out;
        }
    }

    if (!picraw.alloc(mGetMemoryCB, picturesize)) {
        res = -1;
        goto out;
    }

    thumbw = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    thumbh = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if ((thumbw <= 0) || (thumbh <= 0)) {
        ALOGE("thumbnail size %dx%d , no thumnnail support", thumbw, thumbh);
        mHasthumbnail = false;
    } else {
        if( !thumbraw.alloc(mGetMemoryCB, (thumbw*thumbh*12)/8))
            mHasthumbnail = false;
    }

    if (isVideoRecordingEnabled()) {
        Mutex::Autolock lock(mVideoRecLock);
//        res = mCamera.grabPreviewFrame(picraw.data());
        if (mCurrentFrame) {
            memcpy(picraw.data(), mCurrentFrame, picturesize);
        }
    } else {
        mCamera.startSnap(pictruew, pictrueh, mPicDelay);
        while (mSkipPictureFrame > 0) {
            ALOGV("%s skipping Pictiure frame %d", __FUNCTION__, mSkipPictureFrame);
            mCamera.grabPreviewFrame(NULL, NULL);
            mObjectLock.lock();
            mSkipPictureFrame--;
            mObjectLock.unlock();
        }
        res = mCamera.getPictureRaw(picraw.data(), NULL);
        mCamera.stopSnap();
    }

    if (res < 0) {
        ALOGE("grab picture data failed");
        goto out;
    }

    if (0) {
        char path[] = "/cache/yuv.raw";
        int fd = open(path, O_RDWR);
        if (fd >= 0) {
            write(fd, picraw.data(), picturesize);
            close(fd);
        } else
            ALOGE("######### open %s failed: %s", path, strerror(errno));
    }

    if (isMessageEnabled(CAMERA_MSG_SHUTTER)) {
        mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCBOpaque);
    }
    if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
        mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCBOpaque);
    }

    res = scalePictiure(&picraw, &pictruew, &pictrueh, &tmpMem);
    if (res == 0) {
        picraw.swap(tmpMem);
        tmpMem.release();
    }

    // zoom
    if ((mZoomSupport == true) && (mCurrentZoomIndex > 0)) {
        int cropw, croph;
        getZoomedSize(pictruew, pictrueh, &cropw, &croph);
        ALOGV("%s ratio %d, pictruew %d, pictrueh %d, cropw %d, croph %d",
              __FUNCTION__, mCurrentZoomIndex, pictruew, pictrueh, cropw, croph);
        nv21CropScale(picraw.udata(), pictruew, pictrueh, cropw, croph);
    }

    jpegbuffer = picraw.data();
    res = compressor.compressRawImage(jpegbuffer, pictruew, pictrueh, mJpegQuality);
    picsize = compressor.getCompressedSize();

    if ((res != NO_ERROR) || (picsize == 0)) {
        ALOGE("%s: Compression failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
        goto out;
    }

    // exif
    if (mHasthumbnail == false) {
        mswExif.setThumbnail(false);
        res = NO_ERROR;
    }
    if (mHasthumbnail == true) {
        nv21Scale(picraw.udata(), pictruew, pictrueh,
                  thumbraw.udata(), thumbw, thumbh);

        res = thumbcomp.compressRawImage(thumbraw.data(), thumbw, thumbh, mJpegQuality);
        if (res == NO_ERROR) {
            thumbsize = thumbcomp.getCompressedSize();
            if (thumbsize) {
                mswExif.setThumbnailsize(thumbsize);
            } else {
                res = -1;
                ALOGE("Thumbnail failed : size = 0");
                goto out;
            }
        } else {
            ALOGE("Create Thumbnail failed");
            goto out;
        }
    }

    if (res == NO_ERROR) {
        res = -1;
        mswExif.setParameters(params);
        if (!mswExif.createswexif()) {
            if (!mswExif.getExiflength(&exif_len)) {
                //ALOGW("exif_len = 0x%x !!!!!!!!!!!!!",exif_len);
                jpeg_buff.alloc(mGetMemoryCB, (picsize+thumbsize+exif_len-2));

                if ( jpeg_buff.mem) {
                    //ALOGW("JPEG buffer = %p",jpeg_buff->data);
                    jpegbuffer = jpeg_buff.data();
                    mswExif.copyExif(jpegbuffer);
                    //ALOGW("copy thumnail start");
                    if (thumbsize != 0) {
                        thumbcomp.getCompressedImage(jpegbuffer+exif_len);
                        //ALOGW("put thumbnail  to = %p !!!!!!!!!!!!!",(char *)(jpegbuffer+exif_len));
                        exif_len += thumbsize;
                    }
                    //ALOGW("copy thumnail end");
                    res = NO_ERROR;
                } else
                    ALOGE("%s: Memory failure in CAMERA_MSG_COMPRESSED_IMAGE", __FUNCTION__);
            } else
                ALOGE("Exif Length == 0");
        } else
            ALOGE("Create Exif failed");
        if (res != NO_ERROR)
            goto out;
    }

    patch[0] = *(jpegbuffer+exif_len-1);
    patch[1] = *(jpegbuffer+exif_len-2);
    //ALOGW("copy JPEG to callback start");
    compressor.getCompressedImage(jpegbuffer+exif_len-2);

    //ALOGW("copy JPEG to callback end");
    *(jpegbuffer+exif_len-1) = patch[0];
    *(jpegbuffer+exif_len-2) = patch[1];

    ALOGV("put JPEG  to = %p !!!!!!!!!!!!!",(jpegbuffer+exif_len));
    mPictureHandled = true;
    mDataCB(CAMERA_MSG_COMPRESSED_IMAGE, jpeg_buff.mem, 0, NULL, mCBOpaque);
    res = NO_ERROR;
out:
    mTakingPicture = false;
    mPictureHandled = true;
    ALOGV("%s return %d", __FUNCTION__, res);
    return res;
}

status_t CameraHardware::cancelPicture()
{
    if (mPictureThread.get()) {
        mPictureThread->requestExitAndWait();
    }
    return NO_ERROR;
}


status_t CameraHardware::takePicture()
{
    TRACE_FUNC();

    CameraParameters params;
    {
        Mutex::Autolock lock(mParamLock);
        params = mParameters;
    }
    mTakingPicture = true;

    if (!isVideoRecordingEnabled()) {
        //stop preview and switch to high quality mode.
        stopPreviewInternal();
    }

    /* Get JPEG quality. */
    int jpeg_quality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (jpeg_quality <= 0) {
        jpeg_quality = 90;  /* Fall back to default. */
    }
    /* Deliver one frame only. */
    mJpegQuality = jpeg_quality;

    setSkipPictureFrame(1);

    mPictureThread->requestExitAndWait(); /* exit last thread */
    if (mPictureThread->run("CameraPictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
        ALOGE("Couldn't run picture thread (%s)", strerror(errno));
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t CameraHardware::enableVideoRecording()
{
    Mutex::Autolock lock(mVideoRecLock);
    mVideoRecEnabled = true;
    if (mRecordMemory.mem) {
        mCamera.setRecordBuf(mRecordMemory.data());
    }
    return NO_ERROR;
}

bool CameraHardware::isVideoRecordingEnabled() {
    return mVideoRecEnabled;
}

void CameraHardware::disableVideoRecording()
{
    Mutex::Autolock lock(mVideoRecLock);
    mVideoRecEnabled = false;

    //For directREC we should release in inPreviewThread
    if(!mCamProp->directREC)
        mCamera.setRecordBuf(NULL);
}

/****************************************************************************
 * Callback
 ***************************************************************************/

status_t CameraHardware::storeMetaDataInBuffers(bool enable)
{
    /* Return INVALID_OPERATION means HAL does not support metadata. So HAL will
     * return actual frame data with CAMERA_MSG_VIDEO_FRRAME. Return
     * INVALID_OPERATION to mean metadata is not supported. */
    return INVALID_OPERATION;
}

void CameraHardware::enableMessage(uint msg_type)
{
    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler |= msg_type;
    ALOGV("enableMessage %X, Currently %X", msg_type, mMessageEnabler);
}

bool CameraHardware::isMessageEnabled(uint msg_type) {
    return mMessageEnabler & msg_type;
}

void CameraHardware::disableMessage(uint msg_type)
{
    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler &= ~msg_type;
    ALOGV("disableMessage %X, Currently %X", msg_type, mMessageEnabler);
}

void CameraHardware::onCameraDeviceError(int err)
{
    if (isMessageEnabled(CAMERA_MSG_ERROR) && mNotifyCB != NULL) {
        mNotifyCB(CAMERA_MSG_ERROR, err, 0, mCBOpaque);
    }
}

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
                                  camera_data_callback data_cb,
                                  camera_data_timestamp_callback data_cb_timestamp,
                                  camera_request_memory get_memory,
                                  void* user)
{
    ALOGV("%s: %p, %p, %p, %p (%p)",
          __FUNCTION__, notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    Mutex::Autolock locker(&mObjectLock);
    mNotifyCB = notify_cb;
    mDataCB = data_cb;
    mDataCBTimestamp = data_cb_timestamp;
    mGetMemoryCB = get_memory;
    mCBOpaque = user;
    mCamera.mJdec.setCallbacks(get_memory);
    mCamera.setCallbacks(get_memory);
}


// ---------------------------------------------------------------------------

status_t CameraHardware::inAutoFocusThread()
{
    TRACE_FUNC();

    bool afResult = false;

    mFocusLock.lock();
    /*  check early exit request */
    if (mExitAutoFocusThread == true) {
        mFocusLock.unlock();
        ALOGV("DEBUG(%s):exiting on request0", __FUNCTION__);
        return true;
    }

    mFocusCondition.wait(mFocusLock);
    /*  check early exit request */
    if (mExitAutoFocusThread == true) {
        mFocusLock.unlock();
        ALOGV("DEBUG(%s):exiting on request1", __FUNCTION__);
        return true;
    }
    mFocusLock.unlock();

    mCamera.autoFocus();

    switch (mCamera.getFocusModeResult()) {
    case 0:
        ALOGV("DEBUG(%s):AF Cancelled !!", __FUNCTION__);
        afResult = true;
        break;
    case 1:
        ALOGV("DEBUG(%s):AF Success!!", __FUNCTION__);
        afResult = true;
        break;
    default:
        ALOGV("DEBUG(%s):AF Fail !!", __FUNCTION__);
        afResult = false;
        break;
    }

    if (isMessageEnabled(CAMERA_MSG_FOCUS)) {
        mNotifyCB(CAMERA_MSG_FOCUS, afResult, 0, mCBOpaque);
    }

    return true;
}

status_t CameraHardware::autoFocus()
{
    TRACE_FUNC();

    mFocusCondition.signal();
    return NO_ERROR;
}

status_t CameraHardware::cancelAutoFocus()
{
    TRACE_FUNC();

    mCamera.cancelAutoFocus();
    //[Vic] Nothing to do as we just create an autoFocus thread and report done.
    return NO_ERROR;
}

// ---------------------------------------------------------------------------

status_t CameraHardware::sendCommand(unsigned int cmd, unsigned int arg1, unsigned int arg2)
{
    TRACE_FUNC();
    return BAD_VALUE;
}

void CameraHardware::releaseCamera()
{
    TRACE_FUNC();

    if (mPreviewThread != NULL) {
        {
            /* this thread is normally already in it's threadLoop but blocked
             * on the condition variable or running.  signal it so it wakes
             * up and can exit.
             */
            Mutex::Autolock lock(mPreviewLock);
            mExitPreviewThread = true;
            mPreviewCondition.broadcast();
        }
        //must release mPreviewLock here.
        mPreviewThread->requestExitAndWait();
        mPreviewThread.clear();
    }

    if (mPictureThread != NULL) {
        mPictureThread->requestExitAndWait();
        mPictureThread.clear();
    }
    if (mAutoFocusThread != NULL) {
        /*  this thread is normally already in it's threadLoop but
         *  blocked on the condition variable.
         *  signal it so it wakes up and can exit.  */
        mFocusLock.lock();
        mAutoFocusThread->requestExit();
        mExitAutoFocusThread = true;
        mFocusCondition.signal();
        mFocusLock.unlock();
        mAutoFocusThread->requestExitAndWait();
        mAutoFocusThread.clear();
    }
    if(mCamProp->directREC)
        mCamera.setRecordBuf(NULL);

    mCamera.close();

    mPreviewMemory.release();
    mRecordMemory.release();

    if (mZoomRatios) {
        free(mZoomRatios);
        mZoomRatios = NULL;
    }
}




//--------------------------------------------------------------------------------
char* getPropertyStr(const char* value, int facing)
{
   char  buf[20]={0};
   char* strhw =NULL;
   char* pos;
   
   if(value==NULL || strlen(value) < 7) 
   	 return NULL;

   pos = strchr(value, ':');
   if (pos == NULL)
   	  return NULL;   
   
   if (facing ==CAMERA_FACING_FRONT)
   {
      strncpy(buf, value, (pos - value));   
      strhw = strdup(buf);	  
   }
   else if (facing ==CAMERA_FACING_BACK)
   {
      strcpy(buf, pos + 1);
	  strhw = strdup(buf);
   }
   return strhw;
}


//get string ro.wmt.camera.scale=1280x960:640x480 to w=1280 h=960
int getPropertyWH(const char* value, int* width,int* height, int facing)
{
   char strwidth[20]={0};
   char strheight[20]={0};
   int w ,h;
   char* pos;

   char* str = getPropertyStr(value, facing);
            
   if(str==NULL || strlen(str) < 3) 
   	 return -1;

   pos = strchr(str, 'x');
   if (pos == NULL)
   	  return -1;   
   
   strncpy(strwidth, str, (pos - str));   
   strcpy(strheight, pos + 1);

   w = atoi(strwidth);
   h = atoi (strheight);

   ALOGV("%s -- %s, width=%d,heiht=%d",__FUNCTION__,str, w,h);
   
   if (w <=0 || h <= 0 )
      return -1;      
   
   *width = w;
   *height = h;

   return 0;   
}

//ro.wmt.camera.scale=1280x960:640x480
int CameraHardware::scalePictiure(CameraMemory* picraw,int* srcWidth, int* srcHeight,
                                  CameraMemory* dest_mem)
{
   int ret = 0;
   char str[32]={0};  
   int width, height;     
   
   if(getCameraScale(str) <= 0 ) 
   	  return -1;
   
   ret = getPropertyWH(str, &width, &height, mCamProp->facing);		 
   if (ret !=0) 		 	
	   return -1; 
         
   int picturesize = (width * height * 12) / 8;
   if (!dest_mem->alloc(mGetMemoryCB, picturesize)) 
      return -1;
   
   nv21Scale(picraw->udata(), *srcWidth, *srcHeight,
                 dest_mem->udata(), width, height);
   *srcWidth = width;
   *srcHeight = height;
        
   return 0;
}


}; // namespace android

// vim: et ts=4 shiftwidth=4

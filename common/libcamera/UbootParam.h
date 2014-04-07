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

#ifndef CAMERA_UBOOT_PARAM_H
#define CAMERA_UBOOT_PARAM_H

extern "C" {
	int wmt_getsyspara(const char *varname, char *varval, int * varlen);
}

#define ENV_NAME_CAMERA "wmt.camera.param"

#define MAX_CAMERAS_SUPPORTED 2


#define CAMERA_TYPE_NA          0   /* not avaliable */
#define CAMERA_TYPE_CMOS        1   /* CMOS */
#define CAMERA_TYPE_USB         2   /* USB  */

#define pixfmtstr(x) (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, \
	((x) >> 24) & 0xff

// sync with wmt-cmos.c in kernel.
enum {
    CAMERA_CAP_FLASH        = 1 << 0,
    CAMERA_CAP_FLASH_AUTO   = 1 << 1,
    CAMERA_CAP_HDR          = 1 << 2,
};

/**
 * Camera property from uboot env or query the driver.
 */
struct camera_property {
    // uboot env <type>:<gpio>:<active>:<mirror>:<cap>.
    int         type;       /* CAMERA_TYPE_NA */
    int         gpio;
    int         active;
    int         mirror;
    uint32_t    cap;

    // other camera param: TODO:
    int     facing;
    int     orientation;
    int     cameraId;       /* id: start from 0 */
    char    nodeName[20];  /* node name */
    int     retryCount;

    bool    directREC;  /* pass Y/C to wmt h264 enc */
    int     rotate;
};


extern camera_property gCameraProperty[MAX_CAMERAS_SUPPORTED];

#endif /* CAMERA_UBOOT_PARAM_H */

// vim: et ts=4 shiftwidth=4

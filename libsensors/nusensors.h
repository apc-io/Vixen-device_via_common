/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_03311516_SENSORS_H
#define ANDROID_03311516_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <math.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

#include "kxte9.h"


__BEGIN_DECLS

#define SENSOR_DEBUG 0

/*****************************************************************************/

extern int init_nusensors(hw_module_t const* module, hw_device_t** device);
extern int acc_is_used(void);
extern int lgt_is_used(void);
extern int get_acc_drvid(void);
extern int get_lgt_drvid(void);
extern int get_pxm_drvid(void);
extern int get_mag_drvid(void);
extern int get_ore_drvid(void);
extern int get_gyro_drvid(void);
extern int get_sensor_num(void);

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_O  (2)
#define ID_T  (3)
#define ID_P  (4)
#define ID_L  (5)
#define ID_B  (6)
#define ID_G  (7)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* the SFH7743 is a binary proximity sensor that triggers around 6 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_CM  6.0f

/*****************************************************************************/

#define REMOTE_SENSOR_CONTROL_SOCKET   "remotesensorcontrol"
//#define AKM_DEVICE_NAME             "/dev/akm8975_aot"
#define ACCELEROMETER_DEVICE_NAME   "/dev/sensor_ctrl"
/* Input Device Name */
#define ACC_INPUT_NAME	"g-sensor"
#define GYRO_INPUT_NAME "gyroscope"
#define LIGHTING_DEVICE_NAME        "/dev/lsensor_ctrl"
#define PROXIMITY_DEVICE_NAME        "/dev/psensor_ctrl"
#define MAGNETIC_DEVICE_NAME	  "/dev/ecompass_ctrl"
#define ORIENTATION_DEVICE_NAME	  "/dev/ecompass_ctrl"
//#define BAROMETER_DEVICE_NAME       "/dev/bmp085"
//#define GYROSCOPE_DEVICE_NAME       "/dev/l3g4200d"
#define GYROSCOPE_DEVICE_NAME       "/dev/gyro_ctrl"

#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Y
#define EVENT_TYPE_ACCEL_Z          ABS_Z

#define EVENT_TYPE_YAW              REL_RX
#define EVENT_TYPE_PITCH            REL_RY
#define EVENT_TYPE_ROLL             REL_RZ
#define EVENT_TYPE_ORIENT_STATUS    REL_HWHEEL

#if 0
#define EVENT_TYPE_MAGV_X           REL_DIAL
#define EVENT_TYPE_MAGV_Y           REL_WHEEL
#define EVENT_TYPE_MAGV_Z           REL_MISC
#endif

#define EVENT_TYPE_LIGHT            ABS_MISC
#define EVENT_TYPE_PRESSURE         ABS_PRESSURE

#define EVENT_TYPE_GYRO_P           ABS_X
#define EVENT_TYPE_GYRO_R           ABS_Y
#define EVENT_TYPE_GYRO_Y           ABS_Z

#define LSG_MMA7660A                (1000.0f)
#define MAX_RANGE_MMA7660A          (GRAVITY_EARTH*1.5f)
#define LSG_D08                     (256.0f)
#define MAX_RANGE_D08               (GRAVITY_EARTH*3.0f)
#define LSG_D06                     (32.0f)
#define MAX_RANGE_D06               (GRAVITY_EARTH*2.0f)
#define LSG_MC3230                  (85.0f)
#define MAX_RANGE_MC3230            (GRAVITY_EARTH*1.5f)
#define LSG_D10                     (1024.0f)
#define MAX_RANGE_D10              (GRAVITY_EARTH*4.0f)
#define LSG_MXC622X                 (64.0f)
#define MAX_RANGE_MXC622X           (GRAVITY_EARTH*2.0f)

#define MAX_RANGE_MMA8452Q         (GRAVITY_EARTH*2.0f)
#define MAX_RANGE_KIONIX         (GRAVITY_EARTH*2.0f)
#define MAX_RANGE_STK8312          (GRAVITY_EARTH*6.0f)
// conversion of acceleration data to SI units (m/s^2)
#define CONVERT_A_MMA7660A          (GRAVITY_EARTH / LSG_MMA7660A)
#define CONVERT_A_D08        (GRAVITY_EARTH / LSG_D08)
#define CONVERT_A_D06        (GRAVITY_EARTH / LSG_D06)
#define CONVERT_A_MC3230     (GRAVITY_EARTH / LSG_MC3230)
#define CONVERT_A_D10       (GRAVITY_EARTH / LSG_D10)
#define CONVERT_A_MXC622X    (GRAVITY_EARTH / LSG_MXC622X)
#define CONVERT_A_MMA8452Q         (GRAVITY_EARTH / 16384.0)
#define CONVERT_A_KIONIX         (GRAVITY_EARTH / 16384.0)
#define CONVERT_A_STK8312         (GRAVITY_EARTH / 21.34)  
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#if 0
#define CONVERT_M                   (1.0f/16.0f)
#define CONVERT_M_X                 (CONVERT_M)
#define CONVERT_M_Y                 (CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)
#endif
// conversion of magnetic data to uT units
// 32768 = 1Guass
#if 1
#define CONVERT_M			(100.f / 32768.0f)
#define CONVERT_M_X			(CONVERT_M)
#define CONVERT_M_Y			(CONVERT_M)
#define CONVERT_M_Z			(CONVERT_M)
#endif
#if 0
#define CONVERT_M			(1.0f / 1000.0f)
#define CONVERT_M_X			(CONVERT_M)
#define CONVERT_M_Y			(CONVERT_M)
#define CONVERT_M_Z			(CONVERT_M)
#endif
#if 0
#define CONVERT_O                   (1.0f/64.0f)
#define CONVERT_O_Y                 (CONVERT_O)
#define CONVERT_O_P                 (CONVERT_O)
#define CONVERT_O_R                 (-CONVERT_O)
#endif

// 65536 = 360Degree
#define CONVERT_O			(360.0f / 65536.0f)
#define CONVERT_O_Y			(CONVERT_O)
#define CONVERT_O_P			(CONVERT_O)
#define CONVERT_O_R			(CONVERT_O)

// conversion of angular velocity(millidegrees/second) to rad/s
#define MAX_RANGE_G                 (2000.0f * ((float)(M_PI/180.0f)))
#define CONVERT_G                   ((70.0f/1000.0f) * ((float)(M_PI/180.0f)))
#define CONVERT_G_P                 (CONVERT_G)
#define CONVERT_G_R                 (CONVERT_G)
#define CONVERT_G_Y                 (CONVERT_G)

#define CONVERT_B                   (1.0f/100.0f)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_03311516_SENSORS_H

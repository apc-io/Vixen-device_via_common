/*
 * Copyright (C) 2010 Motorola, Inc.
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
#include <fcntl.h>
#include <unistd.h>
#include <hardware/sensors.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include "nusensors.h"
#include "wmtsensor.h"
#include "kxte9.h"


/*****************************************************************************/
static inline int isRemoteEnable() 
{
    char buf[PROPERTY_VALUE_MAX];
    property_get("ro.wmt.rmtctrl.enable", buf, "0");
    return buf[0] == '1' ? 1 : 0;
}


/*
 * The SENSORS Module
 */
#define REAL_SENSOR_NUM 10
 
static  int l_accdrvid = -1; // the id of accelerometer sensor driver
static  int l_lgtdrvid = -1; // the id of light sensor driver
static  int l_pxmdrvid = -1; // the id of proximity sensor driver
static  int l_magdrvid = -1; // the id of magnetic sensor driver
static  int l_oredrvid = -1; // the id of orientation sensor driver
static  int l_gyrdrvid = -1; //the id of gyroscope sensor driver

static  int l_sensornum = 0;
static  struct sensor_t accSensorList[] = {
	{ "MMA7660 Accelerometer",
		"Freescale",
	    1, SENSORS_HANDLE_BASE+ID_A,
	    SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_MMA7660A, CONVERT_A_MMA7660A, 3.0f, 20000, { } },
    { "MC3230 3-axis Accelerometer",
        "mCube",
        1, SENSORS_HANDLE_BASE+ID_A,
        SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_MC3230, CONVERT_A_MC3230, 0.23f, 20000, { } },
    { "DMARD08 Accelerometer",
	    "DMT",
        1, SENSORS_HANDLE_BASE+ID_A,
        SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_D08, CONVERT_A_D08, 0.25f, 20000, { } },
    { "DMARD06 Accelerometer",
       "DMT",
       1, SENSORS_HANDLE_BASE+ID_A,
       SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_D06, CONVERT_A_D06, 0.145f, 20000, { } },
	{ "DMARD10 Accelerometer",
       "DMT",
       1, SENSORS_HANDLE_BASE+ID_A,
       SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_D10, CONVERT_A_D10, 0.145f, 20000, { } },
	{ "Memsic MXC622x 2-axis Accelerometer",
       "Memsic",
       1, SENSORS_HANDLE_BASE+ID_A,
       SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_MXC622X, CONVERT_A_MXC622X, 0.7f, 20000, { } },
       { "MMA8452Q Accelerometer",
		"Freescale",
	    1, SENSORS_HANDLE_BASE+ID_A,
	    SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_MMA8452Q, CONVERT_A_MMA8452Q, 3.0f, 20000, { } },
	
	{ "stk8312 Accelerometer",
		"STK",
	    1, SENSORS_HANDLE_BASE+ID_A,
	    SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_STK8312, CONVERT_A_STK8312, 3.0f, 20000, { } },	 

	{ "kionix Accelerometer",
		"kionix",
	    1, SENSORS_HANDLE_BASE+ID_A,
	    SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_KIONIX, CONVERT_A_KIONIX, 3.0f, 20000, { } },
};

static  struct sensor_t remoteSensorList[] = {
		{ 	"WMT Virtual Accelerometer",
			"WMT, Inc.",
			VER_MAJOR_DEV,
			SENSORS_HANDLE_BASE+ID_A,
			SENSOR_TYPE_ACCELEROMETER,
			MAX_G_DEV * GRAVITY_EARTH * 2,
			1 / MAX_SENS_DEV * GRAVITY_EARTH,
			PWR_DEV,
			0,
			{ }
		},
};


static  struct sensor_t lgtSensorList[] = {
	{ "Ambient Light sensor",
	    "Intersil",
	    1, SENSORS_HANDLE_BASE+ID_L,
	    SENSOR_TYPE_LIGHT, 16000.0f, 1.0f, 0.003f, 0,  { } },

};


static  struct sensor_t pxmSensorList[] = {
	{ "Proximity sensor",
	    "sensortek",
	    1, SENSORS_HANDLE_BASE+ID_P,
	    SENSOR_TYPE_PROXIMITY, 16000.0f, 1.0f, 0.003f, 0,  { } },

};

static struct sensor_t gyroSensorList[] = {
	{ "L3G4200D Gyroscope sensor",
	  "ST Micro",
	  1, SENSORS_HANDLE_BASE+ID_G,
	  SENSOR_TYPE_GYROSCOPE, MAX_RANGE_G, CONVERT_G, 6.1f, 1250, { } },
};

static  struct sensor_t magSensorList[] = {
	{ 	"msesnor328x",
		"Memsic",
		1,
		SENSORS_HANDLE_BASE+ID_M,
		SENSOR_TYPE_MAGNETIC_FIELD,
		8.0,
		100.0f/512.0f,
		0.003,
		0,
		{}
	},	
	   		
};

static  struct sensor_t oreSensorList[] = {
	{	"Mag & Acc Combo Orientation Sensor",
		"Memsic",
		1,
		SENSORS_HANDLE_BASE + ID_O,
		SENSOR_TYPE_ORIENTATION,
		2.0,
		1.0,
		0.0,
		0,
		{}
	},
};


//*********************add end 2013-4-18	
static struct sensor_t sSensorList[REAL_SENSOR_NUM];// = {
	/*{ "MMA7660 Accelerometer",
		"Freescale",
	        1, SENSORS_HANDLE_BASE+ID_A,
	        SENSOR_TYPE_ACCELEROMETER, MAX_RANGE_A, CONVERT_A, 3.0f, 20000, { } },
	{ "ISL29023 Ambient Light sensor",
	    "Intersil",
	        1, SENSORS_HANDLE_BASE+ID_L,
	        SENSOR_TYPE_LIGHT, 16000.0f, 1.0f, 0.003f, 0,  { } },
	{ "AK8975 3-axis Magnetic field sensor",
                "Asahi Kasei",
                1, SENSORS_HANDLE_BASE+ID_M,
                SENSOR_TYPE_MAGNETIC_FIELD, 2000.0f, CONVERT_M, 6.8f, 30000, { } },
	{ "AK8975 Orientation sensor",
                "Asahi Kasei",
                1, SENSORS_HANDLE_BASE+ID_O,
                SENSOR_TYPE_ORIENTATION, 360.0f, CONVERT_O, 7.05f, 30000, { } },
	{ "BMP085 Pressure sensor",
                "Bosch",
                1, SENSORS_HANDLE_BASE+ID_B,
                SENSOR_TYPE_PRESSURE, 110000.0f, 1.0f, 1.0f, 30000, { } },
	{ "L3G4200D Gyroscope sensor",
                "ST Micro",
                1, SENSORS_HANDLE_BASE+ID_G,
                SENSOR_TYPE_GYROSCOPE, MAX_RANGE_G, CONVERT_G, 6.1f, 1250, { } },*/
//};

void sensor_cpy(struct sensor_t* dest, struct sensor_t* src)
{
/*
	int i = 0;
	
	dest->name = src->name;
	dest->vendor = src->vendor;
	dest->version = src->version;
	dest->handle = src->handle;
	dest->type = src->type;
	dest->maxRange = src->maxRange;
	dest->resolution = src->resolution;
	dest->power = src->power;
	dest->minDelay = src->minDelay;
	for (i = 0; i < sizeof(dest->reserved)/sizeof(dest->reserved[0]); i++)
	{
		dest->reserved[i] = src->reserved[i];
	};
*/
	
	memcpy(dest, src, sizeof(struct sensor_t));
}

unsigned int sensor_get_drvid(char* devname)
{
	int fd;
	unsigned int drvid = -1;
	// open the dev
	fd = open(devname, O_RDONLY);
	if (fd < 0)
	{
		// no device
		return -1;
	}
	if (ioctl(fd, WMT_IOCTL_SENSOR_GET_DRVID, &drvid))
	{
		close(fd);
		return -1;
	}
	close(fd);
	ALOGD_IF(SENSOR_DEBUG, "get sensor_drv_id=%d", drvid);
	return drvid;
	// get the driver id from ioctr
}

int acc_is_used(void)
{
	return l_accdrvid>=0?1:0;
}

int lgt_is_used(void)
{
	return l_lgtdrvid>=0?1:0;
}

int get_acc_drvid(void)
{
	return l_accdrvid;
}

int get_lgt_drvid(void)
{
	return l_lgtdrvid;
}

int get_pxm_drvid(void)
{
	return l_pxmdrvid;
}

int get_gyro_drvid(void)
{
	return l_gyrdrvid;
}

int get_mag_drvid(void)
{
	return l_magdrvid;
}

int get_ore_drvid(void)
{
	return l_oredrvid;
}

int get_sensor_num(void)
{
	return l_sensornum;
}

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    *list = sSensorList;
    //LOGD_IF(SENSOR_DEBUG, "[sensors__get_sensors_list] sensornum=%d", l_sensornum);
    ALOGD_IF(1, "[sensors__get_sensors_list] sensornum=%d", l_sensornum);
    return l_sensornum;
    //return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "WMT SENSORS Module",
        .author = "WMT",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};

/*****************************************************************************/

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
	l_sensornum = 0;
	// get acc sensor
	l_accdrvid = sensor_get_drvid(ACCELEROMETER_DEVICE_NAME);
	if (l_accdrvid >= 0)
	{
		sensor_cpy((sSensorList+l_sensornum), (accSensorList+l_accdrvid));
		l_sensornum++;
		ALOGD_IF(SENSOR_DEBUG, "[open_sensors](find acc) sensor_num=%d", l_sensornum);
	}
    else
    {
        if (1/*isRemoteEnable()*/)
        {
            sensor_cpy((sSensorList+l_sensornum), (remoteSensorList));
		    l_sensornum++;
		    ALOGD_IF(SENSOR_DEBUG, "[open_sensors](find remote) sensor_num=%d", l_sensornum);
        }
    }

	// get light sensor
	l_lgtdrvid = sensor_get_drvid(LIGHTING_DEVICE_NAME);
	if (l_lgtdrvid >= 0)
	{
		sensor_cpy((sSensorList+l_sensornum), (lgtSensorList+l_lgtdrvid));
		l_sensornum++;
		ALOGD_IF(SENSOR_DEBUG, "[open_sensors](find light) sensor_num=%d", l_sensornum);

	}


	// get proximity sensor
	l_pxmdrvid = sensor_get_drvid(PROXIMITY_DEVICE_NAME);
	if (l_pxmdrvid >= 0)
	{
		sensor_cpy((sSensorList+l_sensornum), (pxmSensorList+l_pxmdrvid));
		l_sensornum++;
		ALOGD_IF(SENSOR_DEBUG, "[open_sensors](find proximity) sensor_num=%d", l_sensornum);

	}

	// get gyroscope sensor
	l_gyrdrvid = sensor_get_drvid(GYROSCOPE_DEVICE_NAME);
	if(l_gyrdrvid >= 0)
	{
		sensor_cpy(sSensorList+l_sensornum, gyroSensorList+l_gyrdrvid);
		l_sensornum++;
		ALOGD_IF(SENSOR_DEBUG, "[open_sensors](find gyro) sensor_num=%d", l_sensornum);
	}

	//FIX ME add by rambo for magnetic & orientation sensor 2013-4-18
	l_magdrvid = sensor_get_drvid(MAGNETIC_DEVICE_NAME);
	//l_magdrvid = -1;
	if (l_magdrvid >= 0)
	{	
		sensor_cpy((sSensorList+l_sensornum), magSensorList);
		l_sensornum++;
		//property_set("msensor.memsicd", "1");
	}
	
	l_oredrvid =  sensor_get_drvid(ORIENTATION_DEVICE_NAME);
	if (l_gyrdrvid >= 0)
		l_oredrvid = -1;
	if (l_oredrvid >= 0)
	{	
		sensor_cpy((sSensorList+l_sensornum), oreSensorList);
		l_sensornum++;
	}
	//***********add end ****************************************************
	//LOGD_IF(SENSOR_DEBUG, "[open_sensors] sensor_num=%d", l_sensornum);
	ALOGD_IF(1, "[open_sensors] sensor_num=%d", l_sensornum);

    return init_nusensors(module, device);
}

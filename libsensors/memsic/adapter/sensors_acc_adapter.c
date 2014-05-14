/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2009
 *****************************************************************************/

/**
 * @file
 * @author  Robbie Cao<hjcao@memsic.cn>
 *
 * @brief
 * This file implement the interface of acceleration adapter.
 */

#include <sensors_acc_adapter.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#define LOG_TAG "SensorACC"
#include <cutils/log.h>
#include <sys/ioctl.h>
#include <errno.h>
#define ACCELEROMETER_DEVICE_NAME   "/dev/sensor_ctrl"
#define SENSONR_NUM	3
#define WMTGSENSOR_IOCTL_MAGIC  0x09
#define WMT_IOCTL_SENSOR_GET_DRVID	 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x04, unsigned int)

#define D10_ID	4
#define LOG_TAG	"memsicd"

#if ((defined DEVICE_ACC_MXC6202XG) ||	\
     (defined DEVICE_ACC_MXC6202XH) ||	\
     (defined DEVICE_ACC_MXC6202XM) ||	\
     (defined DEVICE_ACC_MXC6202XN))
#define DEVICE_ACC_MXC6202X
#endif

#if (defined DEVICE_ACC_MXC6202X)
#include <sensors_acc_mxc6202x.h>
#elif (defined DEVICE_ACC_MXC622X)
#include <sensors_acc_mxc622x.h>
#elif (defined DEVICE_ACC_BMA220)
#include <sensors_acc_bma220.h>
#elif (defined DEVICE_ACC_BMA222)
#include <sensors_acc_bma222.h>
#elif (defined DEVICE_ACC_BMA023)
#include <sensors_acc_bma023.h>
#elif (defined DEVICE_ACC_LIS33DE)
#include <sensors_acc_lis33de.h>
#else
#endif

//#error "Acc sensor device not specify!"
#include "sensors_acc_dmt10.h"
#include "sensors_acc_mma7660.h"
#include "sensors_acc_mc3230.h"
#include "sensors_acc_generic.h"

static struct device_acc_t dev_acc[SENSONR_NUM+1] = {
	{
	init		: acc_mma7660_init,

	open		: acc_mma7660_open,
	close		: acc_mma7660_close,

	read_data	: acc_mma7660_read_data,
	get_offset	: acc_mma7660_get_offset,
	set_new_offset	: acc_mma7660_set_new_offset,
	get_sensitivity	: acc_mma7660_get_sensitivity,
	get_install_dir	: acc_mma7660_get_install_dir},
	
	{
	init		: acc_mc3230_init,

	open		: acc_mc3230_open,
	close		: acc_mc3230_close,

	read_data	: acc_mc3230_read_data,
	get_offset	: acc_mc3230_get_offset,
	set_new_offset	: acc_mc3230_set_new_offset,
	get_sensitivity	: acc_mc3230_get_sensitivity,
	get_install_dir	: acc_mc3230_get_install_dir},
	
	{
	init		: acc_dmt10_init,

	open		: acc_dmt10_open,
	close		: acc_dmt10_close,

	read_data	: acc_dmt10_read_data,
	get_offset	: acc_dmt10_get_offset,
	set_new_offset	: acc_dmt10_set_new_offset,
	get_sensitivity	: acc_dmt10_get_sensitivity,
	get_install_dir	: acc_dmt10_get_install_dir},
	
	{
	init		: acc_generic_init,

	open		: acc_generic_open,
	close		: acc_generic_close,

	read_data	: acc_generic_read_data,
	get_offset	: acc_generic_get_offset,
	set_new_offset	: acc_generic_set_new_offset,
	get_sensitivity	: acc_generic_get_sensitivity,
	get_install_dir	: acc_generic_get_install_dir}
	
};	
	

static  int l_accdrvid;
static  int sensor_get_drvid(char* devname)
{
	int fd;
	unsigned int drvid = -1;
	
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
	//ALOGD_IF(SENSOR_DEBUG, "get sensor_drv_id=%d", drvid);
	return drvid;
	// get the driver id from ioctr
}

//#error "Acc sensor device not specify!"
struct device_acc_t *sensors_get_acc_device(void)
{
	ALOGD("%s use generic gsensor!", __FUNCTION__);
	return &dev_acc[3]; //return generic
	
	l_accdrvid = sensor_get_drvid(ACCELEROMETER_DEVICE_NAME);
	if (l_accdrvid < 0){
		ALOGE("<<<<NO fixed gsensor found!\n");
		return NULL;
	}
	ALOGD("<<<sensors_acc_adapter get gsensor id %d\n", l_accdrvid);
	if (l_accdrvid < D10_ID)
		return &dev_acc[l_accdrvid];
	else
		return &dev_acc[2];			
	
}


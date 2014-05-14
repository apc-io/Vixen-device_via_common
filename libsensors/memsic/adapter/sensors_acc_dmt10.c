#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#define LOG_TAG "SensorACC"
#include <utils/Log.h>
#include "sensors_acc_dmt10.h"

#define SENSOR_DATA_SIZE	3
#define IOCTL_MAGIC  0x09
#define SENSOR_RESET    		_IO(IOCTL_MAGIC, 0)
#define SENSOR_CALIBRATION   	_IOWR(IOCTL_MAGIC,  1, int[SENSOR_DATA_SIZE])
#define SENSOR_GET_OFFSET  		_IOR(IOCTL_MAGIC,  2, int[SENSOR_DATA_SIZE])
#define SENSOR_SET_OFFSET  		_IOWR(IOCTL_MAGIC,  3, int[SENSOR_DATA_SIZE])
#define SENSOR_READ_ACCEL_XYZ  	_IOR(IOCTL_MAGIC,  4, int[SENSOR_DATA_SIZE])
#define SENSOR_SETYPR  			_IOW(IOCTL_MAGIC,  5, int[SENSOR_DATA_SIZE])
#define SENSOR_GET_OPEN_STATUS	_IO(IOCTL_MAGIC,  6)
#define SENSOR_GET_CLOSE_STATUS	_IO(IOCTL_MAGIC,  7)
#define SENSOR_GET_DELAY		_IOR(IOCTL_MAGIC,  8, unsigned int*)

#define DMT10_OFFSET_X	0
#define DMT10_OFFSET_Y	0
#define DMT10_OFFSET_Z	0
#define DMT10_SENSITIVITY_X 1024
#define DMT10_SENSITIVITY_Y 1024
#define DMT10_SENSITIVITY_Z 1024
#define DMT10_INSTALL_DIR	0

int acc_dmt10_init(void)
{
	return 0;
}

int acc_dmt10_open(void)
{
	char path[]="/sys/class/accelemeter/dmt/enable_acc";
	char value[2]={0,0};
	value[0]= '1';
	
	int ifd, amt;

	ifd = open(path, O_WRONLY);
    	if (ifd < 0) {
        	printf("Write_attr failed to open %s (%s)",path, strerror(errno));
        	return -1;
	}

    	amt = write(ifd, value, 1);
	amt = ((amt == -1) ? -errno : 0);
	//ALOGE_IF(amt < 0, "Write_int failed to write %s (%s)",
	//	path, strerror(errno));
    	close(ifd);
	
	int fd;
	fd = open("/dev/dmt", O_RDWR);
	if (fd < 0) {
		printf("<<<fail to open /dev/dmt\n");
		return -1;
	}

	return fd;
}

int acc_dmt10_close(int fd)
{
	close(fd);

	return 0;
}

int acc_dmt10_read_data(int fd, int *data)
{
	int res = ioctl(fd, SENSOR_READ_ACCEL_XYZ, data);
	//data[2] = dmt10_OFFSET_Z;
	return res;
}

int acc_dmt10_get_offset(int fd, int *offset_xyz)
{
	offset_xyz[0] = DMT10_OFFSET_X;
	offset_xyz[1] = DMT10_OFFSET_Y;
	offset_xyz[2] = DMT10_OFFSET_Z;

	return 0;
}

int acc_dmt10_set_new_offset(int fd, int *offset_xyz)
{
	return 0;
}

int acc_dmt10_get_sensitivity(int fd, int *sensit_xyz)
{
	sensit_xyz[0] = DMT10_SENSITIVITY_X;
	sensit_xyz[1] = DMT10_SENSITIVITY_Y;
	sensit_xyz[2] = DMT10_SENSITIVITY_Z;

	return 0;
}

int acc_dmt10_get_install_dir(void)
{
	return DMT10_INSTALL_DIR;
}
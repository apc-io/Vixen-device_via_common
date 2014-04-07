#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#define LOG_TAG "SensorACC"
#include <cutils/log.h>
#include "sensors_acc_mma7660.h"

#define SENSOR_DATA_SIZE	3
#define WMTGSENSOR_IOCTL_MAGIC  0x09

#define ECS_IOCTL_APP_READ_XYZ		_IOW(WMTGSENSOR_IOCTL_MAGIC, 0x05, int[SENSOR_DATA_SIZE])

#define MMA7660_OFFSET_X	0
#define MMA7660_OFFSET_Y	0
#define MMA7660_OFFSET_Z	0
#define MMA7660_SENSITIVITY_X  1000
#define MMA7660_SENSITIVITY_Y  1000
#define MMA7660_SENSITIVITY_Z  1000
#define MMA7660_INSTALL_DIR	0

int acc_mma7660_init(void)
{
	return 0;
}

int acc_mma7660_open(void)
{
	
	int fd;
	fd = open("/dev/sensor_ctrl", O_RDWR);
	if (fd < 0) {
		ALOGE("<<<fail to open /dev/dmt\n");
		return -1;
	}

	return fd;
}

int acc_mma7660_close(int fd)
{
	close(fd);

	return 0;
}

int acc_mma7660_read_data(int fd, int *data)
{
	int res = ioctl(fd, ECS_IOCTL_APP_READ_XYZ, data);
	//data[2] = mma7660_OFFSET_Z;
	return res;
}

int acc_mma7660_get_offset(int fd, int *offset_xyz)
{
	offset_xyz[0] = MMA7660_OFFSET_X;
	offset_xyz[1] = MMA7660_OFFSET_Y;
	offset_xyz[2] = MMA7660_OFFSET_Z;

	return 0;
}

int acc_mma7660_set_new_offset(int fd, int *offset_xyz)
{
	return 0;
}

int acc_mma7660_get_sensitivity(int fd, int *sensit_xyz)
{
	sensit_xyz[0] = MMA7660_SENSITIVITY_X;
	sensit_xyz[1] = MMA7660_SENSITIVITY_Y;
	sensit_xyz[2] = MMA7660_SENSITIVITY_Z;

	return 0;
}

int acc_mma7660_get_install_dir(void)
{
	return MMA7660_INSTALL_DIR;
}
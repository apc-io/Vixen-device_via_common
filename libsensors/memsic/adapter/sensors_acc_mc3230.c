#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#define LOG_TAG "SensorACC"
#include <cutils/log.h>
#include "sensors_acc_mc3230.h"

#define SENSOR_DATA_SIZE	3
#define IOCTL_MAGIC  0x09
#define MC32X0_IOC_MAGIC   0xA1


#define MC32X0_READ_XYZ			_IOWR(MC32X0_IOC_MAGIC,15,short)

#define MC3230_OFFSET_X	0
#define MC3230_OFFSET_Y	0
#define MC3230_OFFSET_Z	0
#define MC3230_SENSITIVITY_X 85
#define MC3230_SENSITIVITY_Y 85
#define MC3230_SENSITIVITY_Z 85
#define MC3230_INSTALL_DIR	0

int acc_mc3230_init(void)
{
	return 0;
}

int acc_mc3230_open(void)
{

	
	int fd;
	fd = open("/dev/sensor_ctrl", O_RDWR);
	if (fd < 0) {
		printf("<<<fail to open /dev/dmt\n");
		return -1;
	}

	return fd;
}

int acc_mc3230_close(int fd)
{
	close(fd);

	return 0;
}

int acc_mc3230_read_data(int fd, int *data)
{
	int res = ioctl(fd, MC32X0_READ_XYZ, data);
	//data[2] = mc3230_OFFSET_Z;
	return res;
}

int acc_mc3230_get_offset(int fd, int *offset_xyz)
{
	offset_xyz[0] = MC3230_OFFSET_X;
	offset_xyz[1] = MC3230_OFFSET_Y;
	offset_xyz[2] = MC3230_OFFSET_Z;

	return 0;
}

int acc_mc3230_set_new_offset(int fd, int *offset_xyz)
{
	return 0;
}

int acc_mc3230_get_sensitivity(int fd, int *sensit_xyz)
{
	sensit_xyz[0] = MC3230_SENSITIVITY_X;
	sensit_xyz[1] = MC3230_SENSITIVITY_Y;
	sensit_xyz[2] = MC3230_SENSITIVITY_Z;

	return 0;
}

int acc_mc3230_get_install_dir(void)
{
	return MC3230_INSTALL_DIR;
}
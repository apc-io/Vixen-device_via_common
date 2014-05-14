#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#define LOG_TAG "SensorACC"
#include <cutils/log.h>
#include "sensors_acc_generic.h"

#define SENSOR_DATA_SIZE	3
#define WMTGSENSOR_IOCTL_MAGIC  0x09

#define ECS_IOCTL_APP_READ_XYZ		_IOW(WMTGSENSOR_IOCTL_MAGIC, 0x05, int[SENSOR_DATA_SIZE])

#define GENERIC_OFFSET_X	0
#define GENERIC_OFFSET_Y	0
#define GENERIC_OFFSET_Z	0
#define GENERIC_SENSITIVITY_X  98000000
#define GENERIC_SENSITIVITY_Y  98000000
#define GENERIC_SENSITIVITY_Z  98000000
#define GENERIC_INSTALL_DIR	0

#define CTL_FIFO "/data/misc/ctl_fifo"
#define DATA_FIFO "/data/misc/data_fifo"

static int data_fd = 0;
static int ctl_fd = 0;

int acc_generic_init(void)
{
	return 0;
}

int acc_generic_open(void)
{
	
	
	ctl_fd = open(CTL_FIFO, O_RDWR);
	if (ctl_fd < 0) {
		ALOGE("<<<fail to open %s:%s\n", CTL_FIFO, strerror(errno));
		return -1;
	}
	data_fd = open(DATA_FIFO, O_RDWR);
	if (data_fd < 0) {
		ALOGE("<<<fail to open %s:%s\n", DATA_FIFO, strerror(errno));
		return -1;
	}

	return data_fd;
}

int acc_generic_close(int fd)
{
	if (ctl_fd >= 0)
		close(ctl_fd);
	if (data_fd >= 0)
		close(data_fd);
	return 0;
}

int acc_generic_read_data(int fd, int *data)
{
	char *buf = "DATA";
	char ctl_buf[10] = {0};
	int ret = 0;
	//ret = read(ctl_fd, ctl_buf, sizeof(ctl_buf));
	ret = write(ctl_fd, buf, sizeof(buf));
	ret = read(data_fd, (void *)data, sizeof(int)*3);
	//ALOGD("%s x,y,z %d %d %d", __FUNCTION__, data[0], data[1], data[2]);
	return 0; //means success
}

int acc_generic_get_offset(int fd, int *offset_xyz)
{
	offset_xyz[0] = GENERIC_OFFSET_X;
	offset_xyz[1] = GENERIC_OFFSET_Y;
	offset_xyz[2] = GENERIC_OFFSET_Z;

	return 0;
}

int acc_generic_set_new_offset(int fd, int *offset_xyz)
{
	return 0;
}

int acc_generic_get_sensitivity(int fd, int *sensit_xyz)
{
	sensit_xyz[0] = GENERIC_SENSITIVITY_X;
	sensit_xyz[1] = GENERIC_SENSITIVITY_Y;
	sensit_xyz[2] = GENERIC_SENSITIVITY_Z;

	return 0;
}

int acc_generic_get_install_dir(void)
{
	return GENERIC_INSTALL_DIR;
}
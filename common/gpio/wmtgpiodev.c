/*++
Copyright (c) 2008 WonderMedia Technologies, Inc. All Rights Reserved.

This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
and may contain trade secrets and/or other confidential information of
WonderMedia Technologies, Inc. This file shall not be disclosed to any third
party, in whole or in part, without prior written consent of WonderMedia.

THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED,
AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
NON-INFRINGEMENT.
--*/

#define LOG_NDEBUG 0

#define LOG_TAG "wmtgpio"
#include <utils/Log.h>

#define	_GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>

#include <asm/types.h>
#include <sys/mman.h>
/*#include 	<sys/shm.h>*/

#include "wmt-gpio-dev.h"

int wmt_gpioconfig(struct gpio_cfg_t *gpioconf)
{
	int fd = -1;
	int err = -1;

	if (gpioconf == NULL)
		return -EINVAL;

	fd = open("/dev/wmtgpio", O_RDWR);
	if (fd < 0) {
		printf("open /dev/wmtgpio failed : %s",strerror(errno));
		return -ENODEV;
	}

	err = ioctl(fd, GPIOCFG, gpioconf);
	if (err)
		printf("io error : gpio device node");

	close(fd);
	return err;

}

int wmt_gpioset(struct wmt_reg_op_t *gpioset)
{
	int fd = -1;
	int err = -1;

	if (gpioset == NULL)
		return -EINVAL;

	fd = open("/dev/wmtgpio", O_RDWR);
	if (fd < 0) {
		printf("open /dev/wmtgpio failed : %s",strerror(errno));
		return -ENODEV;
	}

	err = ioctl(fd, GPIOWREG, gpioset);
	if (err)
		printf("io error : gpio device node");

	close(fd);
	return err;
}

int wmt_gpioget(struct wmt_reg_op_t *gpioget)
{
	int fd = -1;
	int err = -1;

	if (gpioget == NULL)
		return -EINVAL;

	fd = open("/dev/wmtgpio", O_RDWR);
	if (fd < 0) {
		printf("open /dev/wmtgpio failed : %s",strerror(errno));
		return -ENODEV;
	}

	err = ioctl(fd, GPIORREG, gpioget);
	if (err)
		printf("io error : gpio device node");

	close(fd);
	return err;
}

#define DEV_NODE "/dev/wmtgpio"

/* copy from drivers/char/wmt_gpio.c */
struct user_gpio {
    int gpio;
    int type;
    int value;
};

int wmt_gpio_set(int gpio, int direction, int value)
{
    int fd;
    struct user_gpio ug;

    fd = open(DEV_NODE, O_WRONLY);
    if (fd < 0) {
        ALOGE("open %s failed\n", DEV_NODE);
        return -1;
    }

    ug.gpio = gpio;
    ug.type = direction;
    ug.value = value;

    if (write(fd, &ug, sizeof(ug)) != sizeof(ug)) {
        ALOGE("write failed\n");
    }

    ALOGD("GPIO%d -- %d\n", ug.gpio, ug.value);

    close(fd);
    return 0;
}

//end of file

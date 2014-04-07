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


#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#define LOG_TAG "WMT_ENV"
#include <cutils/log.h>


#include "mtd/mtd-user.h"

#define GEIO_MAGIC		0x69
#define GEIOGET_CHIP_ID		_IOR(GEIO_MAGIC, 5, unsigned int)
#define GPIO_REG_BASE			0xd8110000
#define GPIO_BONDING			0x110
#define SCC_REG_BASE			0xd8120000
#define SCC_VT3357ID			0x700

int	wmt_getsyspara(const char	*varname,	char *varval,	int	*varlen);

/*
 * Description
 *    This function is used to get the WMT SoC information including
 *    chipid & bondingid.
 * Arguments
 *    chipid : [OUT] return the chipid as
 *             Bit[31:16] Project ID. For example, WM8510 will report 3426.
 *             Bit[15:8] Major Revision. These bits indicate the major revision
 *             value of the part. The initial default value of these bits is
 *             8'h01.  For each subsequent major revision, which is defined as
 *             requiring more than metal edits, this value will be incremented
 *             by 1'b1.
 *             Bit[7:0] Metal Edit Revision. These bits indicate the metal edit
 *             revision value of the part.  The initial default value of these
 *             bits is 8'h01.  For each subsequent metal edit revision, this
 *             value will be incremented by 1'b1.
 *    bondingid : [OUT] retur the bonding option value.
 */
int wmt_getsocinfo(unsigned int *chipid, unsigned int *bondingid)
{
	unsigned int addr, pa;
	unsigned int va;

	int fd;
	int ret = 0;

	*chipid = 0;
	*bondingid = 0;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
		fd = open("/dev/graphics/fb0", O_RDWR, 0);
	if (fd < 0) {
		ALOGE("wmt_getsocinfo error, can't open fb0: %d", errno);
		return -1;
	}
	
	ioctl(fd, GEIOGET_CHIP_ID, chipid);
    close(fd);

/* 
    // No /dev/mem  permission for Android Java program
	addr = GPIO_REG_BASE;
	fd = open("/dev/mem", O_RDWR);
	if (fd < 0)
		return 1;

	pa = addr & ~0xfff; // 4K alignment for mmaped address.
	va = (unsigned int)mmap(NULL, 0x1000,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa);
	*bondingid = *(unsigned int *)(va + GPIO_BONDING);
	munmap((void *)(va), 0x1000);
	close(fd);

	addr = SCC_REG_BASE;
	fd = open("/dev/mem", O_RDWR);
	if (fd < 0)
		return 1;

	pa = addr & ~0xfff; // 4K alignment for mmaped address.
	va = (unsigned int)mmap(NULL, 0x1000,
		PROT_READ|PROT_WRITE, MAP_SHARED, fd, pa);

	*chipid = *(unsigned int *)(va + SCC_VT3357ID);
	if ((*chipid & 0x33570000) != 0x3357)
		*chipid = *(unsigned int *)(va);
	munmap((void *)(va), 0x1000);
	close(fd);
*/
	return ret;
}


/*
 * Function : int wmt_setsyspara(char *varname, char *varval)
 *            the function is called by device drivers and application to
 *            set the system parameter.
 * parameter :
 * varname : [IN] The system parameter name which application program or
 *           device driver expect to set.
 * varval : [IN] Pointer to a buffer to store the system parpmeter value
 *          for setting. If the pointer is NULL and the system parameter
 *          is existed then the system parameter will be clear after this
 *          is returned success.
 * return : 0 indicate success, Nonzero indicates failure.
 */
int wmt_setsyspara(const char *varname, const char *varval)
{
    struct env_info_user env_data;
	int mtd_fd;

    /* Open the device */
    mtd_fd = open("/dev/mtd/mtd0", O_RDWR);
    if (mtd_fd == -1) {
        ALOGE("open /dev/mtd/mtd0 error: %s", strerror(errno));
        return -1;
    }

	const char rostr[] = ".ro";
	int len1 = strlen(varname);
	int len2 = strlen(rostr);	
	
	if(len1 > len2)
	{
		if(!strcmp(varname + len1 - len2,rostr))//end with ".ro"
		{
			char varvaltemp[64] = {0};
			int length= sizeof(varvaltemp);
			
			if(!wmt_getsyspara(varname,varvaltemp,&length))
			{
				ALOGI("%s is read only",varname);
				return -1;	
			}
		}
	}

    memcpy(env_data.varname, varname, 200);

    if (varval == NULL)
        env_data.varpoint = NULL;
    else {
        env_data.varpoint = (char*)varval;
        memcpy(env_data.varval, varval, 200);
    }

    if (ioctl(mtd_fd, MEMSETENV, &env_data) != 0) {
        ALOGE("MEMSETENV %s error: %s", varname, strerror(errno));
        close(mtd_fd);
        return -1;
    }
    ALOGD("set syspara %s %s", varname, varval ? varval : "");
    close(mtd_fd);
    return 0;
}
/*
 * Function : int wmt_getsyspara(char *varname, char *varval, int *varlen)
 *            the function is called by device drivers and application to
 *            get the system parameter if existed.
 * parameter :
 * varname : [IN] The system parameter name which application program or
 *           device driver expect to retrieve.
 * varval : [OUT] Pointer to a buffer to store the system parpmeter value.
 * varlen : [IN] The buffer size for the varval pointer.
 * return : 0 indicate success, Nonzero indicates failure.
 */
int	wmt_getsyspara(const char	*varname,	char *varval,	int	*varlen)
{
	struct env_info_user env_data;
	int ret, mtd_fd;
    const int max_name = sizeof(env_data.varname) - 1;

    ALOGD("get syspara %s", varname);
    

    /* Open the device */
    mtd_fd = open("/dev/mtd/mtd0", O_RDWR);
    if (mtd_fd == -1) {
        varval[0] = '\0';
        *varlen = 0;
        ALOGE("open /dev/mtd/mtd0 error: %s", strerror(errno));
        return -1;
    }

    strncpy(env_data.varname, varname, max_name);
    env_data.varname[max_name] = '\0';

    env_data.varlen = *varlen;

    ret = ioctl(mtd_fd, MEMGETENV, &env_data);
    if (ret != 0) {
        varval[0] = '\0';
        *varlen = 0;
        ALOGE("MEMGETENV get %s error: %d %s", varname, ret, strerror(errno));
        close(mtd_fd);
        return -1;
    }
    memcpy(varval, env_data.varval, *varlen);
    *varlen = env_data.varlen;

    close(mtd_fd);
    return 0;
}




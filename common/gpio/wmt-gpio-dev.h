/*
################################################################################
#                                                                              #
# Copyright c 2009-2012  WonderMedia Technologies, Inc.   All Rights Reserved. #
#                                                                              #
# This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.  #
# and may contain trade secrets and/or other confidential information of       #
# WonderMedia Technologies, Inc. This file shall not be disclosed to any third #
# party, in whole or in part, without prior written consent of WonderMedia.    #
#                                                                              #
# THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  #
# WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, #
# AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED          #
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET       #
# ENJOYMENT OR NON-INFRINGEMENT.                                               #
#                                                                              #
################################################################################
*/
#ifndef WMT_GPIO_DEV_H
/* To assert that only one occurrence is included */
#define WMT_GPIO_DEV_H

/*--- wmt-pwm.h---------------------------------------------------------------
*   Copyright (C) 2009 WonderMedia Tech. Inc.
*
* MODULE       : wmt-pwm.h --
* AUTHOR       : Sam Shen
* DATE         : 2009/8/12
* DESCRIPTION  :
*------------------------------------------------------------------------------*/

/*--- History -------------------------------------------------------------------
*Version 0.01 , Sam Shen, 2009/8/12
*	First version
*
*------------------------------------------------------------------------------*/
/*-------------------- MODULE DEPENDENCY -------------------------------------*/


/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  pwm_xxx_t;  *//*Example*/
struct wmt_reg_op_t{
	unsigned int addr;
	unsigned int bitmap;
	unsigned int regval;
};

struct gpio_operation_t {
	struct  wmt_reg_op_t ctl;
	struct  wmt_reg_op_t oc;
	struct  wmt_reg_op_t od;
	struct  wmt_reg_op_t id;
};

struct gpio_cfg_t {
	struct  wmt_reg_op_t ctl;
	struct  wmt_reg_op_t oc;
};

struct dedicated_gpio_operation_t {
	unsigned int no;
	unsigned int oc;
	unsigned int val;
};

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/
//#define GPIO_IOC_MAGIC	'g'
#define GPIO_IOC_MAGIC	'6'

#define GPIODEDICATED _IOWR(GPIO_IOC_MAGIC, 0, void *)
#define GPIOCFG _IOW(GPIO_IOC_MAGIC, 1, void *)
#define GPIOWREG _IOW(GPIO_IOC_MAGIC, 2, void *)
#define GPIORREG _IOWR(GPIO_IOC_MAGIC, 3, void *)

#define FREESYSCACHES _IO(GPIO_IOC_MAGIC, 4)

#define GPIO_IOC_MAXNR	5

// /sys/class/gpio/xxx
enum {
        GPIO_DIRECTION_OUT = 0,
        GPIO_DIRECTION_IN,
};

#ifdef __cplusplus
extern "C" {
#endif

extern int wmt_gpio_set(int gpio, int direction, int value);

#ifdef __cplusplus
}
#endif

#endif
/*=== END wmt-pwm.h ==========================================================*/

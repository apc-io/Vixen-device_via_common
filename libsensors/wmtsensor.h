#ifndef __WMTSENSOR_20120713_H__
#define __WMTSENSOR_20120713_H__
/*********************************************************************
** the interface from driver.
*/


//////////////////////// sensor driver id ///////////////////////
// acc
#define MMA7660_DRVID 0
#define MC3230_DRVID 1
#define DMARD08_DRVID 2
#define DMARD06_DRVID 3
#define DMARD10_DRVID 4
#define MXC622X_DRVID 5
#define MMA8452Q_DRVID 6
#define STK8312_DRVID 7
#define KIONIX_DRVID 8

// light
#define ISL29023_DRVID 0

//gyro
#define L3G4200D_DRVID 0

#define MMC328x_DRVID 0
///////////////////////// ioctrl cmd ////////////////////////
#define WMTGSENSOR_IOCTL_MAGIC  0x09
#define WMT_IOCTL_SENSOR_CAL_OFFSET  _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x01, int) //offset calibration
#define ECS_IOCTL_APP_SET_AFLAG		 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x02, short)
#define ECS_IOCTL_APP_SET_DELAY		 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x03, short)
#define WMT_IOCTL_SENSOR_GET_DRVID	 _IOW(WMTGSENSOR_IOCTL_MAGIC, 0x04, unsigned int)
#define WMT_IOCTL_SENOR_GET_RESOLUTION		 _IOR(WMTGSENSOR_IOCTL_MAGIC, 0x05, short)

//light sensor itctl
#define WMT_LSENSOR_IOCTL_MAGIC  0x10
//#define WMT_IOCTL_SENSOR_CAL_OFFSET  _IOW(WMT_LSENSOR_IOCTL_MAGIC, 0x01, int) //offset calibration
#define LIGHT_IOCTL_SET_ENABLE		 _IOW(WMT_LSENSOR_IOCTL_MAGIC, 0x01, short)
//#define ECS_IOCTL_APP_SET_DELAY		 _IOW(WMT_LSENSOR_IOCTL_MAGIC, 0x03, short)

#define WMT_GYRO_IOCTL_MAGIC   0x11
#define GYRO_IOCTL_SET_ENABLE  _IOW(WMT_GYRO_IOCTL_MAGIC, 0, int)
#define GYRO_IOCTL_GET_ENABLE  _IOW(WMT_GYRO_IOCTL_MAGIC, 1, int)
#define GYRO_IOCTL_SET_DELAY   _IOW(WMT_GYRO_IOCTL_MAGIC, 2, int)
#define GYRO_IOCTL_GET_DELAY   _IOW(WMT_GYRO_IOCTL_MAGIC, 3, int)

#endif

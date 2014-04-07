LOCAL_PATH := $(call my-dir)

# prebuilt algorithm libs
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := compasslib_h5_gcc_armv4t.a compasslib_h6_gcc_armv4t.a
include $(BUILD_MULTI_PREBUILT)

# MEMSIC sensors daemon
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= 
LOCAL_STATIC_LIBRARIES	:= compasslib_h5_gcc_armv4t compasslib_h6_gcc_armv4t
LOCAL_LDLIBS		+= -Idl
LOCAL_CFLAGS		+= -static
# use the following macro to specify acc/mag sensor and orientation algorithm
# Accelerometer:
# DEVICE_ACC_MXC6202XG - MEMSIC MXC6202xG 2-axis acc-sensor
# DEVICE_ACC_MXC6202XH - MEMSIC MXC6202xH 2-axis acc-sensor
# DEVICE_ACC_MXC6202XM - MEMSIC MXC6202xM 2-axis acc-sensor
# DEVICE_ACC_MXC6202XN - MEMSIC MXC6202xN 2-axis acc-sensor
# DEVICE_ACC_MXC622X   - MEMSIC MXC622x (DTOS) 2-axis acc-sensor
# DEVICE_ACC_BMA220    - BOSCH BMA220
# DEVICE_ACC_BMA023    - BOSCH BMA023
# Magnetic sensor:
# DEVICE_MAG_MMC312X   - MEMSIC MMC3120 3-axis mag-sensor
# DEVICE_MAG_MMC314X   - MEMSIC MMC3140 3-axis mag-sensor
# DEVICE_MAG_MMC328XMS   - MEMSIC MMC328XMS 3-axis mag-sensor
# DEVICE_MAG_MMC328XMA   - MEMSIC MMC328XMA 3-axis mag-sensor
# Orientation algorithm:
# COMPASS_ALGO_H5      - IDS 3+2 algorithm
# COMPASS_ALGO_H6      - IDS 3+3 algorithm
#LOCAL_CFLAGS		+= -DDEVICE_ACC_BMA222 -DDEVICE_MAG_MMC314X -DCOMPASS_ALGO_H6
LOCAL_CFLAGS		+= -DDEVICE_MAG_MMC328XMS -DCOMPASS_ALGO_H6
LOCAL_C_INCLUDES	+= $(LOCAL_PATH)/adapter		\
			   $(LOCAL_PATH)/libs/H5_V1.2		\
			   $(LOCAL_PATH)/libs/H6_V1.0

LOCAL_SRC_FILES		:= daemon/memsicd.c			\
			   adapter/sensors_coordinate.c		\
			   adapter/sensors_acc_adapter.c    	\
			   adapter/sensors_acc_dmt10.c    	\
			   adapter/sensors_acc_mma7660.c    	\
			   adapter/sensors_acc_mc3230.c    	\
			   adapter/sensors_acc_generic.c    	\
			   adapter/sensors_mag_adapter.c    	\
			   adapter/sensors_mag_mmc31xx.c    	\
			   adapter/sensors_mag_mmc328x.c    	\
			   adapter/sensors_algo_adapter.c	\
			   adapter/sensors_algo_ids_h5.c	\
			   adapter/sensors_algo_ids_h6.c	\
			   adapter/sensors_algo_ids_util.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils 
LOCAL_MODULE := memsicd
LOCAL_MODULE_TAGS := $(TARGET_BUILD_VARIANT)
#LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)




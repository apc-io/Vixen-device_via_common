# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)


# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so

LOCAL_MODULE := sensors.wmt

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := $(TARGET_BUILD_VARIANT)

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

LOCAL_SRC_FILES := AccelerationSensor7660A.cpp \
                    AccelerationSensorD10.cpp \
				   AccelerationSensorD06.cpp \
				   AccelerationSensorD08.cpp \
				   AccelerationSensorMC3230.cpp \
				   AccelerationSensorMxc622x.cpp \
				   LightSensor29023.cpp \
				   ProximitySensor.cpp \
				   GyroSensorL3g4200d.cpp \
				   sensors.c 			\
				   nusensors.cpp 			\
				   InputEventReader.cpp		\
				   SensorBase.cpp		\
				   MagneticSensor.cpp \
				   OrientationSensor.cpp \
                   AccelerationSensorGeneric.cpp 

LOCAL_LDFLAGS += -lwmtstatic -L$(LOCAL_PATH) 

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils 
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))

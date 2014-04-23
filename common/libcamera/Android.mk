# Copyright (C) 2011 The Android Open Source Project
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


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libui \
    libstagefright_omx \
    libwmtenv \
    libswexif \
    libwmtapi \
    libgui \
	libwmtgpio \
	libmedia 

# JPEG conversion libraries and includes.
LOCAL_SHARED_LIBRARIES += \
	libjpeg \
	libskia \
	libandroid_runtime \

LOCAL_STATIC_LIBRARIES += libyuv_static

LOCAL_C_INCLUDES += external/jpeg \
					external/skia/include/core/ \
					frameworks/base/core/jni/android/graphics \
					frameworks/base/include \
					frameworks/base/include/media/stagefright/openmax \
					device/wmt/common/ \
					device/wmt/common/wmt_api/include \
					frameworks/native/include \
					external/libyuv/files/include \

LOCAL_SRC_FILES := \
    CameraFactory.cpp \
    CameraHardware.cpp \
	V4L2Camera.cpp     \
	JpegCompressor.cpp \
	swexif.cpp \
	WMTCamJdec.cpp	\
	CameraUtil.cpp \

LOCAL_MODULE := camera.wmt

LOCAL_MODULE_TAGS := optional    
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS) 
LOCAL_SRC_FILES := media_profiles.xml
LOCAL_MODULE_PATH  := $(PRODUCT_OUT)/system/etc
include $(WMT_PREBUILT)

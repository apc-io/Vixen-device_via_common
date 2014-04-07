LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES := ump.ko:system/modules/ump.ko
LOCAL_SRC_FILES += mali.ko:system/modules/mali.ko

LOCAL_SRC_FILES += libMali.so:system/lib/libMali.so
LOCAL_SRC_FILES += libUMP.so:system/lib/libUMP.so

LOCAL_SRC_FILES += libMali.so:obj/lib/libMali.so
LOCAL_SRC_FILES += libUMP.so:obj/lib/libUMP.so

LOCAL_SRC_FILES += egl.cfg:system/lib/egl/egl.cfg
LOCAL_SRC_FILES += libEGL_mali.so:system/lib/egl/libEGL_mali.so
LOCAL_SRC_FILES += libGLESv1_CM_mali.so:system/lib/egl/libGLESv1_CM_mali.so
LOCAL_SRC_FILES += libGLESv2_mali.so:system/lib/egl/libGLESv2_mali.so

LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libMali_intermediates/export_includes 
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libUMP_intermediates/export_includes 

LOCAL_MODULE_TAGS := $(TARGET_BUILD_VARIANT) 

include $(WMT_PREBUILT)

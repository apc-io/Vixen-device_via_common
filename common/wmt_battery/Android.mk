LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include external/stlport/libstlport.mk
LOCAL_C_INCLUDES += \
    frameworks/base/include/ \
    frameworks/native/include/ \

LOCAL_SHARED_LIBRARIES:= \
        liblog \
        libwmtenv \
		libutils \
		libcutils \
        libandroid_runtime \
        libstlport

LOCAL_SRC_FILES:= \
		battery.cpp \
        wmtbattery.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= wmtbattery

include $(BUILD_EXECUTABLE)

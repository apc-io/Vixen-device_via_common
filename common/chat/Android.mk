ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := chat
LOCAL_CFLAGS := 
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := chat.c
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_EXECUTABLE)

endif

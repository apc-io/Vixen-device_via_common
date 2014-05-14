LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtnative.so:system/lib/libwmtnative.so
LOCAL_SRC_FILES += libwmtnative.so:obj/SHARED_LIBRARIES/libwmtnative_intermediates/LINKED/libwmtnative.so
LOCAL_SRC_FILES += libwmtnative.so:symbols/system/lib/libwmtnative.so
LOCAL_SRC_FILES += libwmtnative.so:obj/lib/libwmtnative.so
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libwmtnative_intermediates/export_includes

include $(WMT_PREBUILT)


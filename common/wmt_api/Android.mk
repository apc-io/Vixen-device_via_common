LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtapi.so:system/lib/libwmtapi.so
LOCAL_SRC_FILES += libwmtapi.so:obj/SHARED_LIBRARIES/libwmtapi_intermediates/LINKED/libwmtapi.so
LOCAL_SRC_FILES += libwmtapi.so:symbols/system/lib/libwmtapi.so
LOCAL_SRC_FILES += libwmtapi.so:obj/lib/libwmtapi.so
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libwmtapi_intermediates/export_includes

include $(WMT_PREBUILT)




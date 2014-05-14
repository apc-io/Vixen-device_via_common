LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtcodec.so:system/lib/libwmtcodec.so
LOCAL_SRC_FILES += libwmtcodec.so:symbols/system/lib/libwmtcodec.so
LOCAL_SRC_FILES += libwmtcodec.so:obj/lib/libwmtcodec.so

LOCAL_SRC_FILES += empty:obj/SHARED_LIBRARIES/libwmtcodec_intermediates/export_includes
LOCAL_SRC_FILES += empty:obj/SHARED_LIBRARIES/libwmtcodec_intermediates/import_includes

include $(WMT_PREBUILT)


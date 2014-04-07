LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtmsvd.so:system/lib/libwmtmsvd.so
LOCAL_SRC_FILES += libwmtmsvd.so:symbols/system/lib/libwmtmsvd.so
LOCAL_SRC_FILES += libwmtmsvd.so:obj/lib/libwmtmsvd.so

LOCAL_SRC_FILES += empty:obj/SHARED_LIBRARIES/libwmtmsvd_intermediates/import_includes
LOCAL_SRC_FILES += empty:obj/SHARED_LIBRARIES/libwmtmsvd_intermediates/export_includes

include $(WMT_PREBUILT)


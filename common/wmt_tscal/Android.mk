LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libpointertch.so:system/lib/libpointertch.so
LOCAL_SRC_FILES += libpointertch.so:obj/SHARED_LIBRARIES/libpointertch_intermediates/LINKED/libpointertch.so
LOCAL_SRC_FILES += libpointertch.so:symbols/system/lib/libpointertch.so
LOCAL_SRC_FILES += libpointertch.so:obj/lib/libpointertch.so
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libpointertch_intermediates/export_includes

include $(WMT_PREBUILT)


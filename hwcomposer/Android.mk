LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += hwcomposer.wmt.so:system/lib/hw/hwcomposer.wmt.so
LOCAL_SRC_FILES += hwcomposer.wmt.so:obj/SHARED_LIBRARIES/hwcomposer.wmt_intermediates/LINKED/hwcomposer.wmt.so
LOCAL_SRC_FILES += hwcomposer.wmt.so:symbols/system/lib/hw/hwcomposer.wmt.so
LOCAL_SRC_FILES += hwcomposer.wmt.so:obj/lib/hw/hwcomposer.wmt.so
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/hwcomposer.wmt_intermediates/export_includes

include $(WMT_PREBUILT)


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtegl.so:system/lib/libwmtegl.so
LOCAL_SRC_FILES += libwmtegl.so:obj/SHARED_LIBRARIES/libwmtegl_intermediates/LINKED/libwmtegl.so
LOCAL_SRC_FILES += libwmtegl.so:symbols/system/lib/libwmtegl.so
LOCAL_SRC_FILES += libwmtegl.so:obj/lib/libwmtegl.so
#LOCAL_SRC_FILES += video-tv.png:system/etc/video-tv.png
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libwmtegl_intermediates/export_includes

include $(WMT_PREBUILT)



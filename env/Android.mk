LOCAL_PATH := $(call my-dir)

NOTICE-TARGET-STATIC_LIBRARIES-libwmtenv_static: ;

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += libwmtenv.so:system/lib/libwmtenv.so
LOCAL_SRC_FILES += libwmtenv.so:obj/SHARED_LIBRARIES/libwmtenv_intermediates/LINKED/libwmtenv.so
LOCAL_SRC_FILES += libwmtenv.so:symbols/system/lib/libwmtenv.so
LOCAL_SRC_FILES += libwmtenv.so:obj/lib/libwmtenv.so
LOCAL_SRC_FILES += wmtenv:system/bin/wmtenv
LOCAL_SRC_FILES += wmtenv:obj/EXECUTABLES/wmtenv_intermediates/LINKED/wmtenv
LOCAL_SRC_FILES += wmtenv:symbols/system/bin/wmtenv
LOCAL_SRC_FILES += wmtenv:obj/EXECUTABLES/wmtenv_intermediates/wmtenv
LOCAL_SRC_FILES += savechange2env:system/bin/savechange2env
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/libwmtenv_intermediates/export_includes
LOCAL_SRC_FILES += export_includes:obj/EXECUTABLES/wmtenv_intermediates/export_includes
LOCAL_SRC_FILES += export_includes:obj/EXECUTABLES/savechange2env_intermediates/export_includes

LOCAL_SRC_FILES += libwmtenv_static.a:obj/STATIC_LIBRARIES/libwmtenv_static_intermediates/libwmtenv_static.a
LOCAL_SRC_FILES += export_includes:obj/STATIC_LIBRARIES/libwmtenv_static_intermediates/export_includes
include $(WMT_PREBUILT)


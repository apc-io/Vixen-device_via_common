LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES += audio.primary.wmt.so:system/lib/hw/audio.primary.wmt.so
LOCAL_SRC_FILES += audio.primary.wmt.so:obj/SHARED_LIBRARIES/audio.primary.wmt_intermediates/LINKED/audio.primary.wmt.so
LOCAL_SRC_FILES += audio.primary.wmt.so:symbols/system/lib/hw/audio.primary.wmt.so
LOCAL_SRC_FILES += audio.primary.wmt.so:obj/lib/audio.primary.wmt.so
LOCAL_SRC_FILES += audio.primary.wm8994.so:system/lib/hw/audio.primary.wm8994.so
LOCAL_SRC_FILES += audio.primary.wm8994.so:obj/SHARED_LIBRARIES/audio.primary.wm8994_intermediates/LINKED/audio.primary.wm8994.so
LOCAL_SRC_FILES += audio.primary.wm8994.so:symbols/system/lib/hw/audio.primary.wm8994.so
LOCAL_SRC_FILES += audio.primary.wm8994.so:obj/lib/audio.primary.wm8994.so
LOCAL_SRC_FILES += bt_downlink:system/bin/bt_downlink
LOCAL_SRC_FILES += bt_downlink:obj/EXECUTABLES/bt_downlink_intermediates/LINKED/bt_downlink
LOCAL_SRC_FILES += bt_downlink:symbols/system/bin/bt_downlink
LOCAL_SRC_FILES += bt_downlink:obj/obj/EXECUTABLES/bt_downlink_intermediates/bt_downlink
LOCAL_SRC_FILES += bt_uplink:system/bin/bt_uplink
LOCAL_SRC_FILES += bt_uplink:obj/EXECUTABLES/bt_uplink_intermediates/LINKED/bt_uplink
LOCAL_SRC_FILES += bt_uplink:symbols/system/bin/bt_uplink
LOCAL_SRC_FILES += bt_uplink:obj/EXECUTABLES/bt_uplink_intermediates/bt_uplink
LOCAL_SRC_FILES += audio_policy.conf:system/etc/audio_policy.conf
LOCAL_SRC_FILES += export_includes:obj/EXECUTABLES/bt_uplink_intermediates/export_includes
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/audio.primary.wmt_intermediates/export_includes
LOCAL_SRC_FILES += export_includes:obj/EXECUTABLES/bt_downlink_intermediates/export_includes
LOCAL_SRC_FILES += export_includes:obj/SHARED_LIBRARIES/audio.primary.wm8994_intermediates/export_includes

#LOCAL_SRC_FILES += mixer_paths_wm8994_default.xml:system/etc/mixer_paths_wm8994_default.xml

include $(WMT_PREBUILT)

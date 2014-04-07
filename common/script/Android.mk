LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 


LOCAL_SRC_FILES := runinitscript.sh:system/etc/wmt/runinitscript.sh \
    S40modules:system/etc/wmt/S40modules \
    S50misc:system/etc/wmt/S50misc \
    S66lowmemorythreshold:system/etc/wmt/S66lowmemorythreshold \
    pm.sh:system/etc/wmt/pm.sh \
    poweroff.sh:system/etc/wmt/poweroff.sh \
    usbfs.sh:system/etc/wmt/script/usbfs.sh \
    force.sh:system/etc/wmt/script/force.sh \
    init_mali.sh:system/etc/wmt/script/init_mali.sh \
    restore.sh:system/.restore/restore.sh \
    sys_partition_end.sh:system/.restore/sys_partition_end.sh \
    autodetection.sh:system/.restore/autodetection.sh \
    ota_addon.sh:system/.restore/ota_addon.sh \
    uboot_env_exclude:system/.restore/uboot_env_exclude

include $(WMT_PREBUILT)

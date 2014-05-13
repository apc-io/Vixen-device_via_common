#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.tuna), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH := $(call my-dir)

# if some modules are built directly from this directory (not subdirectories),
# their rules should be written here.

include $(CLEAR_VARS) 
#copy wmt special files
LOCAL_SRC_FILES := extra_packages/uzImage.bin:kernel \
    init.wmt.rc:root/init.wmt.rc \
    init.wmt.usb.rc:root/init.wmt.usb.rc \
    fstab.wmt:root/fstab.wmt \
    vold.fstab:system/etc/vold.fstab \
    default.prop:system/default.prop \
    boot.mov:system/wmtapp/bootanimation/boot.mov \
    default.mp4:system/wmtapp/launcher-res/wallpaper/default.mp4 \
    init.rtl8192c.rc:root/init.rtl8192c.rc \
    wifi/mtk7601/init.mtk7601.rc:root/init.mtk7601.rc \
    wifi/mtk7601/RT2870STA_7601.dat:system/etc/firmware/RT2870STA_7601.dat \
    wifi/mtk7601/RT2870AP.dat:system/etc/firmware/RT2870AP.dat \
    wifi/rtl8723a/rtk8723_bt_config:system/etc/firmware/rtk8723_bt_config\
    wifi/rtl8723a/rtk8723a:system/etc/firmware/rtk8723a\
    wifi/mt5931/WIFI_RAM_CODE_MT5931:system/etc/firmware/WIFI_RAM_CODE_MT5931 \
    wifi/mt5931/init_mt5931.rc:root/init.mt5931.rc \
    init.mtk6620.rc:root/init.mtk6620.rc \
    init.eagle.rc:root/init.eagle.rc \
    wifi/eagle/esp_init_data.bin:system/etc/firmware/esp_init_data.bin \
    wifi/eagle/init_data.conf:system/etc/firmware/init_data.conf \
    init.bcm.rc:root/init.bcm.rc \
    init.bcm6476.rc:root/init.bcm6476.rc \
    wifi/bcm/fw_bcm40181a2.bin:system/etc/firmware/fw_bcm40181a2.bin \
    wifi/bcm/fw_bcm40181a2_apsta.bin:system/etc/firmware/fw_bcm40181a2_apsta.bin \
    wifi/bcm/fw_bcm40181a2_p2p.bin:system/etc/firmware/fw_bcm40181a2_p2p.bin \
    wifi/bcm/nvram_ap6181.txt:system/etc/firmware/nvram_ap6181.txt \
    init_nmc1000.rc:root/init_nmc1000.rc \
    wifi/bcm_ap6330/fw_bcm40183b2_ag.bin:system/etc/firmware/fw_bcm40183b2.bin \
    wifi/bcm_ap6330/fw_bcm40183b2_ag_apsta.bin:system/etc/firmware/fw_bcm40183b2_apsta.bin \
    wifi/bcm_ap6330/fw_bcm40183b2_ag_p2p.bin:system/etc/firmware/fw_bcm40183b2_p2p.bin \
    wifi/bcm_ap6330/nvram_ap6330.txt:system/etc/firmware/nvram_ap6330.txt \
    init.bcm6330.rc:root/init.bcm6330.rc \
    native.bin:system/wmtapp/bootanimation/native.bin \
    useragent.properties:system/wmtapp/useragent.properties \
    pause.png:system/wmtapp/pause.png \
	isdbt/isdbt_rio.inp:/system/etc/firmware/isdbt_rio.inp \
	isdbt/isdbt_santos.inp:/system/etc/firmware/isdbt_santos.inp \
	ipv6/dhcp6c.conf:/system/etc/dhcp6c.conf \
	ipv6/libnl.so:/system/lib/libnl.so \
	ipv6/libCTCCfgSetting.so:/system/lib/libCTCCfgSetting.so \
	ipv6/dhcp6c:/system/bin/dhcp6c 


include $(WMT_PREBUILT)

$(shell mkdir -p $(TARGET_ROOT_OUT))
WMT_PACKAGES_BUSYBOX := $(shell find device/via/vixen/extra_packages -name "busybox*.tgz")
$(info $(WMT_PACKAGES_BUSYBOX) --> $(TARGET_ROOT_OUT))
$(shell tar zxf $(WMT_PACKAGES_BUSYBOX) -C $(TARGET_ROOT_OUT))


include $(call all-makefiles-under,$(LOCAL_PATH))



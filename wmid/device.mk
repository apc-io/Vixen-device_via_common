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


PRODUCT_CHARACTERISTICS := tablet

PRODUCT_PROPERTY_OVERRIDES += \
    qemu.hw.mainkeys=0

DEVICE_PACKAGE_OVERLAYS := \
    device/wmt/wmid/overlay

PRODUCT_PACKAGES := \
    audio.primary.wmt \
    audio.usb.default \
    libaudioutils \
    libtinyalsa \
    libbluetooth_mtk \
    libbt-hci-mtk6622 \
    MTK_MT6622_E2_Patch.nb0 \
    tinycap tinymix tinyplay 

PRODUCT_PACKAGES += \
    audio.r_submix.default

# Mtd utilities
PRODUCT_PACKAGES += \
    flash_erase \
    nanddump \
    nandwrite \
    flashcp

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)

PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
        frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml \
        frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
        frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml


# BPlus for brcm ap6476 bluetooth
#PRODUCT_COPY_FILES += \
#	device/wmt/wmid/wifi/bcm_ap6476/bplus.default.so:system/lib/hw/bplus.default.so \
#	device/wmt/wmid/wifi/bcm_ap6476/iop_bt.db:system/etc/bluetooth/iop_bt.db \
#	device/wmt/wmid/wifi/bcm_ap6476/bcm2076b1.hcd:system/etc/bluetooth/bcm2076b1.hcd \
#	device/wmt/wmid/wifi/bcm_ap6476/auto_pair_devlist.conf:system/etc/bluetooth/auto_pair_devlist.conf \
#	device/wmt/wmid/wifi/bcm_ap6476/bt_did.conf:system/etc/bluetooth/bt_did.conf \
#	device/wmt/wmid/wifi/bcm_ap6476/bt_stack.conf:system/etc/bluetooth/bt_stack.conf \
#	device/wmt/wmid/wifi/bcm_ap6476/bt_vendor.conf:system/etc/bluetooth/bt_vendor.conf

#brcm ap6476 GPS(Enable follow copy procedure if you need GPS feature)
#PRODUCT_COPY_FILES += \
#	device/wmt/wmid/wifi/bcm_ap6476/gps.default.so:system/lib/hw/gps.default.so \
#	device/wmt/wmid/wifi/bcm_ap6476/glgps:system/bin/glgps \
#	device/wmt/wmid/wifi/bcm_ap6476/gpsconfig.xml:system/etc/gps/gpsconfig.xml

# BPlus for brcm ap6476 wifi
PRODUCT_COPY_FILES += \
	device/wmt/wmid/wifi/bcm_ap6476/fw_bcm40181a2.bin:/system/etc/firmware/fw_bcm40181a2.bin \
	device/wmt/wmid/wifi/bcm_ap6476/fw_bcm40181a2_apsta.bin:/system/etc/firmware/fw_bcm40181a2_apsta.bin \
	device/wmt/wmid/wifi/bcm_ap6476/fw_bcm40181a2_p2p.bin:/system/etc/firmware/fw_bcm40181a2_p2p.bin \
	device/wmt/wmid/wifi/bcm_ap6476/nvram_ap6476.txt:/system/etc/firmware/nvram_ap6476.txt
	
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

PRODUCT_PROPERTY_OVERRIDES += \
    ro.opengles.version=131072
    
PRODUCT_PROPERTY_OVERRIDES += \
    hwui.render_dirty_regions=false

DEVICE_PACKAGE_OVERLAYS := \
    device/wmt/common/overlay

# Email/Mms
PRODUCT_PACKAGES += \
    Email \
    Mms \
    Utk

# Charger mode
PRODUCT_PACKAGES += \
    charger \
    charger_res_images

# USB accessory
PRODUCT_PACKAGES += \
    com.android.future.usb.accessory
PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
        frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml

# Location
PRODUCT_COPY_FILES += \
        frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml

# Camera
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml

# WiFi
PRODUCT_PACKAGES += \
    netd_mt7601 \
    netd_mtk6620
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml

# WiFi NMI NNC1000
PRODUCT_PACKAGES += \
	wifi_firmware7.2.bin \
	wifi_firmware_ap7.2.bin \
	wifi_firmware6.21.bin \
	wifi_firmware7.0.bin \
	wifi_firmware_ap7.0.bin \
	wifi_firmware7.01.bin \
	wifi_firmware7.02.bin \
	wifi_firmware7.03.bin \
	wifi_firmware7.04.bin \
	wifi_firmware_ap7.04.bin \
	wifi_firmware7.11.bin \
	wifi_firmware_ap7.11.bin \
	WifiSupport

# Sensor
PRODUCT_PACKAGES += \
    sensors.wmt
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml
    
# SpeechRecorder
PRODUCT_PACKAGES += \
        libsrec_jni
		
# default add this for easy debug.
PRODUCT_PACKAGES += \
        tcpdump
		

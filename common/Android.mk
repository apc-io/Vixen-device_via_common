LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS) 
#copy wmt special files
#for NTFS-3G storage: ntfs-3g
#for OMX Codec: media_codecs.xml
LOCAL_SRC_FILES := ntfs-3g:root/bin/ntfs-3g \
                   lrz:system/bin/lrz \
                   lsz:system/bin/lsz \
                   media_codecs.xml:system/etc/media_codecs.xml \
                   android.hardware.screen.xml:system/etc/permissions/android.hardware.screen.xml \
                   android.hardware.microphone.xml:system/etc/permissions/android.hardware.microphone.xml \
				   wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf

include $(WMT_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))

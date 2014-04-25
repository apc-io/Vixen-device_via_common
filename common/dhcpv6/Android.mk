LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false  

#       resolv_token.c

LOCAL_SRC_FILES:= client6_addr.c   client6_token.c  confdata.c   dhcp6c.c  lease.c  \
client6_parse.c  common.c         dad_parse.c  hash.c    lease_token.c  timer.c 

LOCAL_C_INCLUDES :=  $(LOCAL_PATH) device/wmt/common/libnl/include \

	
LOCAL_SHARED_LIBRARIES := \
	libnl   libc libcutils libnetutils\
							

LOCAL_CFLAGS := -DHAVE_CONFIG_H  -fPIE -D_GNU_SOURCE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= dhcp6c
    
include $(BUILD_EXECUTABLE)

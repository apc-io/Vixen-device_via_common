# Copyright (C) 2011 The Android Open Source Project
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


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_SHARED_LIBRARIES:= 

# JPEG conversion libraries and includes.
LOCAL_SHARED_LIBRARIES +=  

LOCAL_STATIC_LIBRARIES += 
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libm
LOCAL_C_INCLUDES += device/wmt/common/libnl/include  device/wmt/common/libnl/include2

LOCAL_SRC_FILES := \
addr.c        cache_mngt.c  cls_api.c  doc.c     genl_family.c  lookup.c    nexthop.c  qdisc_api.c   route.c        socket.c \
api.c         cbq.c         cls_obj.c  dsmark.c  handlers.c     mngt.c      nfnl.c     qdisc.c       route_obj.c    tbf.c \
attr.c        class_api.c   ct.c       family.c  htb.c          msg.c       nl.c       qdisc_obj.c   route_utils.c  tc.c \
blackhole.c   class.c       ct_obj.c   fifo.c    link.c         neigh.c     object.c   red.c         rtnl.c         u32.c \
cache.c       classifier.c  ctrl.c     fw.c      log.c          neightbl.c  police.c   request.c     rule.c         utils.c \
cache_mngr.c  class_obj.c   data.c     genl.c    log_obj.c      netem.c     prio.c     route_addr.c  sfq.c          vlan.c \

LOCAL_MODULE := libnl

LOCAL_MODULE_TAGS := optional    
include $(BUILD_SHARED_LIBRARY)



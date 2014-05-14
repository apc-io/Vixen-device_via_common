#!/system/bin/sh
################################################################################
#                                                                              #
# Copyright c 2009-2011  WonderMedia Technologies, Inc.   All Rights Reserved. #
#                                                                              #
# This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.  #
# and may contain trade secrets and/or other confidential information of       #
# WonderMedia Technologies, Inc. This file shall not be disclosed to any third #
# party, in whole or in part, without prior written consent of WonderMedia.    #
#                                                                              #
# THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  #
# WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, #
# AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED          #
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET       #
# ENJOYMENT OR NON-INFRINGEMENT.                                               #
#                                                                              #
################################################################################

KVER=$(/system/bin/cat /proc/sys/kernel/osrelease)
MEM_SIZE=536870912

memtotal=`wmtenv get memtotal`
memtotal=${memtotal:0:3}
if [ $memtotal -lt 512 ]; then
  MEM_SIZE=268435456
fi

/system/bin/insmod /lib/modules/$KVER/ump.ko ump_debug_level=0 ump_backend=1 ump_memory_size=$MEM_SIZE
/system/bin/chmod 0666 /dev/ump

/system/bin/insmod /lib/modules/$KVER/mali.ko mali_debug_level=0 mali_l2_enable=3 mali_shared_mem_size=$MEM_SIZE
/system/bin/chmod 0666 /dev/mali

# disable wait vsync for hwcomposer
echo 0 > /sys/module/ge/parameters/vsync

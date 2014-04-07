#!/bin/sh

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
###################################################################


######## It is just for FIRST installation or OTA installer
######## It is NOT for Android run-time
info()
{
    echo "[$TAG] $@" > /dev/console 
}

#######################################check if create swap partition########################################
# if  devie memory less than 512 M, then create a  swap partition

check_if_open_swap() {
	if [ "$enable_swap" == "1" ]; then
		info "bedin build swap partition. Please wait a few minutes"
		dd if=/dev/zero of=${DATA_MNT}/swap_file bs=1M count=128
		mkswap ${DATA_MNT}/swap_file
		info "end build swap"
	else
		info "no need  build swap for device which memory more than 512M : $memtotal"
	fi
}

TAG=`basename $0`
info "running ..."

local SYS_MNT=$1
local DATA_MNT=$2
local FROM_OTA=$3
#    /bin/chmod 0660  ${DATA_MNT}/data/com.android.providers.settings/databases/*
#    1000 is uid:guid of settings.apk's
#    /bin/chown -hR 1000:1000  ${DATA_MNT}/data/com.android.providers.settings

##chown/chmod and do something for special files...
#for ppp3G
cp ${SYS_MNT}/bin/pppd ${SYS_MNT}/bin/ppp3g

#for all
chmod 644 ${SYS_MNT}/app/*
chmod 644 ${SYS_MNT}/vendor/app/*
chmod 644 ${SYS_MNT}/framework/*
chmod -R 755 ${SYS_MNT}/bin
chmod -R 755 ${SYS_MNT}/xbin
chmod -R 755 ${SYS_MNT}/etc
chmod -R 755 ${SYS_MNT}/lib
chmod -R 755 ${SYS_MNT}/modules

chmod 755 ${DATA_MNT}/app
chmod 644 ${DATA_MNT}/app/*


#for 2G/3G sim-card
ln -s /data/ppp/peers/ ${SYS_MNT}/etc/ppp/peers

#for recovery
chmod +x ${SYS_MNT}/.restore/restore.sh

#for WiFi
chown 1010:1010 ${SYS_MNT}/etc/wifi/wpa_supplicant.conf
chmod 0660 ${SYS_MNT}/etc/wifi/wpa_supplicant.conf

#for bt mtk6622
touch ${SYS_MNT}/../data/bt_mtk6622_ready
chmod 0777 ${SYS_MNT}/../data/bt_mtk6622_ready

#for su/suw, add super mode and change owner (it will be called by SuperUser.apk)
chown 0:0  ${SYS_MNT}/xbin/su
chown 0:0  ${SYS_MNT}/xbin/suw
chmod 4755  ${SYS_MNT}/xbin/su
chmod 4755  ${SYS_MNT}/xbin/suw

check_if_open_swap

chmod +x ${SYS_MNT}/.restore/autodetection.sh
#OTA don't want to call this, because of two reason:
# 1 modify file in system may cause incremental OTA update fail
# 2 OTA packag should contain all auto detection info stored in system partition
if [ -z "${FROM_OTA}" ] ; then
   ${SYS_MNT}/.restore/autodetection.sh  ${SYS_MNT} ${DATA_MNT}
else
   info "in ota, ignore autodetection.sh"
   echo "in ota, ignore autodetection.sh"
fi
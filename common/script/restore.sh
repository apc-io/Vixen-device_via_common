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

# function: restore backup files to data partition
mount_data()
{
    local root_dev=$1
    local mnt_data=$2
    local mtd_data_blk=
    if [ "$root_dev" == "TF" ]; then
        echo "[WMT] restore android-data & cache (TF)..."
        mtd_data_blk=/dev/block/mmcblk1p7
    elif [ "$root_dev" == "UDisk" ]; then
        echo "[WMT] restore android-data & cache (UDisk)..."
        mtd_data_blk=/dev/block/sda7
    else
        echo "[WMT] restore android-data & cache (Nand)..."
        mtd_data_blk=`cat /proc/mtd | grep '\"data\"' | cut -d: -f1 | sed 's/mtd/\/dev\/block\/mtdblock/g'`
        if [ -z ${mtd_data_blk} ];then
            echo "[WMT] *E* can't find android-data partition!"
            return 1
        fi
    fi

    mkdir -p ${mnt_data}
    if [ "$root_dev" == "UDisk" ] || [ "$root_dev" == "TF" ]; then
        mount -t ext4 ${mtd_data_blk}  ${mnt_data}
    else
        mount -t yaffs2 ${mtd_data_blk} ${mnt_data}
    fi

    if [ $? -ne 0 ] ; then
        echo "*E*  Failed to mount Android-Data partition(${mtd_data_blk})!!"
        return 1
    fi

    return 0
}

is_mounted()
{
    local target_path=$1
    local mounted_path=`cat /proc/mounts  | grep "$target_path" | cut -d' ' -f2`
    if [ "$target_path" == "$mounted_path" ] ; then
         echo "$mounted_path is mounted"
         return 0
    fi
    
    echo "[WMT] *E* expected mount path is $target_path, mounted path is $mounted_path"
    
    return 1
}


boot_type=`wmtenv get wmt.boot.dev`
data_mnt=/data
mount_data ${boot_type} ${data_mnt}
is_mounted $data_mnt
if [ $? -ne 0 ] ; then
    return 1
fi

echo "[WMT] copy files to android-data partition..."
restore_dir=`dirname $0`
mkdir -p ${data_mnt}/app
if [ -f ${restore_dir}/data.tar ]; then
    tar xf ${restore_dir}/data.tar -C ${data_mnt}
    #creat Download folder or GTS will failed
    dl_path=${data_mnt}/media/0/Download
    mkdir -p ${dl_path}
    chmod 775 -R  ${dl_path}
    chown 1023:1023 -R ${dl_path}
fi
cp -aR ${restore_dir}/data_app/* ${data_mnt}/app
chmod -R 644  ${data_mnt}/app

echo "[WMT] install data partition, done"

if [ -f /system/etc/wifi/softap.conf ]; then
    mkdir -p ${data_mnt}/misc/wifi/
    cp -v /system/etc/wifi/softap.conf ${data_mnt}/misc/wifi/softap.conf
fi

mem=`wmtenv get memtotal | sed 's/M//'`
if  [ ! -z "$mem" ] && [ $mem -lt 512 ] ; then
    echo "bedin build swap partition. Please wait a few minutes"
    dd if=/dev/zero of=${data_mnt}/swap_file bs=1M count=128
    mkswap ${data_mnt}/swap_file
    echo "end build swap"
else
    echo "no need  build swap for device which memory more than 512M : $mem M "
fi

sync
umount ${data_mnt}
if [ $? -ne 0 ] ; then
    echo "*E*  Failed to umount Android-Data partition(${data_mnt})!!"
fi

#restore U-Boot parameters for TVBox/TVDongle
display_param=`wmtenv get wmt.display.param`
display_type=`echo $display_param | cut -c1-5`
echo "display_param = $display_param"
echo "display_type = $display_type"
if [ "$display_type" == "8:1:0" ] ; then
        if [ "$display_param" != "8:1:0:1920:1080:60" ] ; then
                echo "change wmt.display.param to 8:1:0:1920:1080:60"
                wmtenv set wmt.display.param 8:1:0:1920:1080:60
        fi

        display_tvformat=`wmtenv get wmt.display.tvformat`
        echo "display_tvformat = $display_tvformat"
        if [ "$display_tvformat" == "NTSC" ] || [ "$display_tvformat" == "ntsc" ] ; then
                echo "change wmt.display.tvformat to PAL"
                wmtenv set wmt.display.tvformat PAL
        fi
fi

return 0

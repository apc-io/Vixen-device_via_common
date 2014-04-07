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

BLK_NAND_LOGO="logo"

#offset in blk_Nand_Logo for Charge Animation in uboot
#UBOOT_CHARGE_LOG_OFFSET=0x800000

restore_dir=`dirname $0`


get_mtd ()
{
    desc=`echo $1 | sed 's/\ /\\\ /g'`
    cat /proc/mtd | grep "\"$desc\"" | cut -d: -f1 | sed 's/mtd/\/dev\/mtd\/mtd/g'
}

sf_inst ()
{
    local name=$1
    local file=$2
    local mtd=
    local flashcp

    mtd=`get_mtd "$name"`

    if [ -z "$mtd" ]; then
        printf "Error: No MTD partition called $name.\n"
        return 1
    fi

    if [ -e "$file" ]; then
        printf "Update %s to %s ... \n" $file $mtd
        flashcp=`which flashcp`
        if [ "$flashcp" != "" ]; then
            flash_erase $mtd 0 0
            flashcp $file $mtd
        else
           printf "flashcp not exists.\n"
        fi
        printf "done\n"
        return 0
    else
        printf "Error: not found $file, skip update $name.\n"
        return 1
    fi
}

inst_sfimg ()
{
    local name=$1
    local file=$2
    local mtd=
    local shname=`echo "$name" | sed 's/ //g'`
    local mtdsha=
    local filesha=
    
    if [ ! -e $file ] ; then
        printf "$file not exists\n"
        return 1
    fi
    sha1sum=`which sha1sum`
    if [ "$sha1sum" == "" ] ; then
        printf "sha1sum not exists\n"
        return 1
    fi
    
    mtd=`get_mtd "$name"`
    
    if [ -z "$mtd" ]; then
        printf "Error: No MTD partition called $name.\n"
        return 1
    fi    
    
    cat $mtd > /tmp/board_$shname
    mtdsha=`sha1sum /tmp/board_$shname | cut -d' ' -f1`
    filesha=`sha1sum $file | cut -d' ' -f1`
    
    if [ "$mtdsha" != "$filesha" ] ; then
       printf "Trying to install $file to $name\n" 
       sf_inst "$name" "$file" 
       if [ "$?" == "0" ] ; then
          rm -f $file
       fi
    else
       printf "$file and $name is same\n"
       rm -f $file
    fi
    
}

update_logo()
{
    echo  "Installing u-boot logo to NAND..."
    
    local name="${BLK_NAND_LOGO}"
    local mtd=
    
    mtd=`get_mtd "$name"`

    if [ -z "$mtd" ]; then
        echo "No MTD partition called $name."
        return 1
    fi

    if [ ! -e "$mtd" ]; then
        echo "No MTD partition called $mtd."
        return 2
    fi

    flash_erase $mtd 0 0
    
    if [ -f $restore_dir/logo.out ]; then
        nandwrite -p $mtd $restore_dir/logo.out
        echo "update $restore_dir/logo.out ret $?"
    else
        echo "Skip $restore_dir/u-boot-logo.bmp"
    fi

    return 0
}

mount_data()
{
    local root_dev=$1
    local mnt_data=$2
    local mtd_data_blk=
    if [ "$root_dev" == "TF" ]; then
        echo "[WMT] ota-addon android-data & cache (TF)..."
        mtd_data_blk=/dev/block/mmcblk1p7
    elif [ "$root_dev" == "UDisk" ]; then
        echo "[WMT] ota-addon android-data & cache (UDisk)..."
        mtd_data_blk=/dev/block/sda7
    else
        echo "[WMT] ota-addon android-data & cache (Nand)..."
        mtd_data_blk=`cat /proc/mtd | grep '\"data\"' | cut -d: -f1 | sed 's/mtd/\/dev\/block\/mtdblock/g'`
        if [ -z ${mtd_data_blk} ];then
            echo "[WMT] *E* ota-addon can't find android-data partition!"
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
        echo "[WMT] *E* ota-addon failed to mount Android-Data partition(${mtd_data_blk})!!"
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


######## It is just for OTA installer

printf "\nExecute $0\n\n"

#disable garbage collection of yaffs2, or it will be too slow for ESLC(only for Hynix nand)
echo 0 >  /sys/module/yaffs/parameters/yaffs_bg_enable

chmod +x $restore_dir/sys_partition_end.sh
chmod +x $restore_dir/restore.sh

$restore_dir/sys_partition_end.sh /system / OTA

boot_type=`wmtenv get wmt.boot.dev`

if [ "$boot_type" == "NAND" ] ; then
  update_logo
elif [ "$boot_type" == "TF" ] ; then
  mkdir /firmware
  mount /dev/block/mmcblk1p1 /firmware
  cp -a /system/.restore/*.bmp /firmware/
  umount /firmware
  rmdir /firmware
else
  echo "$boot_type not supported"
fi

inst_sfimg "u-boot-SF" "/system/uboot.img"
inst_sfimg "w-load-SF" "/system/wload.img"
#inst_sfimg "u-boot env. cfg. 1-SF" "/system/uboot_env1.img"
#inst_sfimg "u-boot env. cfg. 2-SF" "/system/uboot_env2.img"


env1mtd=`get_mtd "u-boot env. cfg. 1-SF"`
env2mtd=`get_mtd "u-boot env. cfg. 2-SF"`

if [ -z "$env1mtd" ]; then
    printf "Error: No MTD partition called u-boot env. cfg. 1-SF\n"
    exit 1
fi  

if [ -z "$env2mtd" ]; then
    printf "Error: No MTD partition called u-boot env. cfg. 2-SF\n"
    exit 1
fi  

cat $env1mtd > /tmp/current_ubootenv1.img
cat $env2mtd > /tmp/current_ubootenv2.img

local env1_active_bit=`hexdump  -s 4 -n 16 /system/uboot_env1.img  | head -c 12 | cut -d' ' -f2 | sed 's/^[0-9a-fA-F]\{2\}//g'`
local env2_active_bit=`hexdump  -s 4 -n 16 /system/uboot_env2.img  | head -c 12 | cut -d' ' -f2 | sed 's/^[0-9a-fA-F]\{2\}//g'`
local curr_env1_active_bit=`hexdump  -s 4 -n 16 /tmp/current_ubootenv1.img  | head -c 12 | cut -d' ' -f2 | sed 's/^[0-9a-fA-F]\{2\}//g'`
local curr_env2_active_bit=`hexdump  -s 4 -n 16 /tmp/current_ubootenv2.img  | head -c 12 | cut -d' ' -f2 | sed 's/^[0-9a-fA-F]\{2\}//g'`

local suffix=
local curr_suffix=

if [ "$env1_active_bit" == "01" ] ; then
    if [ "$curr_env1_active_bit" == "01" ] ; then
       suffix=1
       curr_suffix=1
    else
       suffix=1
       curr_suffix=2    
    fi
elif [ "$env2_active_bit" == "01" ] ; then
    if [ "$curr_env1_active_bit" == "01" ] ; then
       suffix=2
       curr_suffix=1
    else
       suffix=2
       curr_suffix=2    
    fi
fi

chmod +x /system/bin/savechange2env
printf "/system/bin/savechange2env /tmp/current_ubootenv$curr_suffix.img /system/uboot_env$suffix.img %restore_dir/uboot_env_exclude\n" 
/system/bin/savechange2env /tmp/current_ubootenv$curr_suffix.img /system/uboot_env$suffix.img $restore_dir/uboot_env_exclude

pkg_ver=`cat /system/default.prop | grep "ro.wmt.pkgver" | cut -d= -f2`
mtd_local=`get_mtd LocalDisk`
if [ ! -z "$mtd_local" ]; then
    echo "[WMT] ota-addon, upgrade from 4.0 to 4.1 by OTA!"
    #format data partition for 4.0 to 4.1 by OTA
    mtd_data=`get_mtd data`
    if [ ! -z "$mtd_data" ]; then
        flash_erase $mtd_data 0 0
    else
        echo "[WMT] ota-addon, not found data partition!"
    fi

    #erase LocalDisk partition
    flash_erase $mtd_local 0 0
else
    echo "[WMT] ota-addon, not found Local partition, old is alread 4.1!"
fi

#remove all old user data in /system/.restore/data.tar to avoid problems
#olduserdatadirs=`tar tf $restore_dir/data.tar | grep 'data' | cut -d/ -f3 | uniq`
#data_mnt=/data
#mount_data ${boot_type} ${data_mnt}
#is_mounted ${data_mnt}
#if [ $? -eq 0 ] ; then
#    for dir in ${olduserdatadirs} ; do
#        echo "[WMT] remove old user data in ${data_mnt}/data/${dir}"
#        rm -rf ${data_mnt}/data/${dir}
#    done
#    sync
#    umount ${data_mnt}
#fi

#$restore_dir/restore.sh

#do sth for WiFi after OTA
data_mnt=/data
mount_data ${boot_type} ${data_mnt}
is_mounted ${data_mnt}
if [ $? -eq 0 ] ; then
    echo "[WMT] ota-addon do sth for WiFi after 4.0 to 4.1 by OTA"
    rm -f ${data_mnt}/property/persist.sys.wifi.modules_name
    rm -f ${data_mnt}/misc/wifi/rt3070sta.ko
    rm -rf ${data_mnt}/system/wpa_supplicant
fi

#delete com.android.vending for google play
if [ -e ${data_mnt}/data/com.android.vending ] ; then
    rm -rf ${data_mnt}/user/*/com.android.vending
    rm -rf ${data_mnt}/user/*/com.android.vending.updater
fi

#for gtalk...
rm -rf ${data_mnt}/user/*/com.google.android.*

rm -rf ${data_mnt}/dalvik-cache 

#for wondertv
rm -f ${data_mnt}/app/com.wmt.wondertv*apk

#enable garbage collection of yaffs2
echo 1 >  /sys/module/yaffs/parameters/yaffs_bg_enable


debug=`wmtenv get wmt.ota.debug`
if [ "$debug" == "1" ] ; then
   sleep 86400
fi 

printf "\nExecute $0, Done!\n\n"

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


info()
{
    echo "[$TAG] $@" > /dev/console 
}

TAG=`basename $0`
info "auto detection running ..."
local SYS_MNT=$1
local DATA_MNT=$2

#To detect which g-sensor is used and match its hal so file.
MODULE_PATH=${SYS_MNT}/modules/3.4.5-default
GSENSOR_CFG_FILE="${SYS_MNT}/etc/gsensor_configed"
GSENSOR_NAME="mc3230 dmard06 dmard08 dmard10 mma7660 mxc622x mma8452q stk8312"
SYSCLS_PATH="/sys/class"
if [ ! -f ${GSENSOR_CFG_FILE} ]; then
    for sname in ${GSENSOR_NAME}
    do
        insmod ${MODULE_PATH}/s_wmt_gsensor_${sname}.ko
        if [ -d ${SYSCLS_PATH}/${sname} ]; then
            echo -n -e "s_wmt_gsensor_${sname}">"$GSENSOR_CFG_FILE"
            info "G-Sensor configed (${sname})"
            if [ ${sname} == "mc3230" ]; then
                cp -v g_sensor_calibration/mCubeAcc_mutilanguages.apk ${SYS_MNT}/vendor/app/
                cp -v g_sensor_calibration/libsensorcontrol.so ${SYS_MNT}/lib/
                cp -v g_sensor_calibration/sensorcontrol.default.so ${SYS_MNT}/lib/hw/
                chmod 644 ${SYS_MNT}/vendor/app/*
                chmod -R 755 ${SYS_MNT}/lib
            elif [ ${sname} == "dmard10" ]; then
                cp -v g_sensor_calibration/DMT_Calibration_LAUNCHER.apk  ${SYS_MNT}/vendor/app/
                chmod 644 ${SYS_MNT}/vendor/app/*
	    elif [ ${sname} == "stk8312" ]; then
                cp -v g_sensor_calibration/STKAccCali_file.apk  ${SYS_MNT}/vendor/app/
                chmod 644 ${SYS_MNT}/vendor/app/*
            fi
            break;
        fi
    done
fi

#active the ug3105 first for MP
UG31XX=${MODULE_PATH}/s_wmt_batt_ug31xx.ko
if [ -e ${UG31XX} ]; then
    insmod ${UG31XX}
fi




RIL_MODEM_ENABLE_TRUE="ril.modem.enable=1"
RIL_MODEM_ENABLE_VALUE=`grep $RIL_MODEM_ENABLE_TRUE ${SYS_MNT}/default.prop 2>/dev/null`
RIL_MODEM_LOWPULSE="ril.lowpulse=1"
RIL_MODEM_LOWPULSE_VALUE=`grep $RIL_MODEM_LOWPULSE ${SYS_MNT}/default.prop 2>/dev/null`

MSTAR="1b20:0302"
SUNPLUS="04fc:3003"
CYIT="231e:0036"
TELECOM="15eb:0001"
HUAWEI_W="12d1:1001"
HUAWEI_TD="12d1:1d50"
HAIER_EVDO="201e:1022"
if [ "$RIL_MODEM_ENABLE_VALUE" != "" ];then
    echo 151 > /sys/class/gpio/export
    echo 154 > /sys/class/gpio/export
    echo 155 > /sys/class/gpio/export
    echo out > /sys/devices/virtual/gpio/gpio151/direction
    echo out > /sys/devices/virtual/gpio/gpio154/direction
    echo out > /sys/devices/virtual/gpio/gpio155/direction
    echo 0 > /sys/devices/virtual/gpio/gpio155/value
    sleep 3
    echo 1 > /sys/devices/virtual/gpio/gpio155/value
    sleep 1
    if [ "$RIL_MODEM_LOWPULSE_VALUE" != "" ];then
        echo 1 > /sys/devices/virtual/gpio/gpio151/value
        echo 1 > /sys/devices/virtual/gpio/gpio154/value
        sleep 1
        echo 0 > /sys/devices/virtual/gpio/gpio151/value
        echo 0 > /sys/devices/virtual/gpio/gpio154/value
        sleep 1
        echo 1 > /sys/devices/virtual/gpio/gpio151/value
        echo 1 > /sys/devices/virtual/gpio/gpio154/value    
    else
        echo 0 > /sys/devices/virtual/gpio/gpio151/value
        echo 0 > /sys/devices/virtual/gpio/gpio154/value
        sleep 1
        echo 1 > /sys/devices/virtual/gpio/gpio151/value
        echo 1 > /sys/devices/virtual/gpio/gpio154/value
        sleep 1
        echo 0 > /sys/devices/virtual/gpio/gpio151/value
        echo 0 > /sys/devices/virtual/gpio/gpio154/value
    fi
    info "Power the modem"
    insmod $MODULE_PATH/cdc-acm.ko
    insmod $MODULE_PATH/usb_wwan.ko
    insmod $MODULE_PATH/option.ko
    insmod $MODULE_PATH/via_option.ko
    sleep 15

    TYPE=`lsusb | cut -d\  -f6`
    for usb_item in $TYPE
    do
        case "${usb_item}" in
            $MSTAR) echo 1 > ${SYS_MNT}/etc/ppp/ril.modem.type;;
            $SUNPLUS) echo 2 > ${SYS_MNT}/etc/ppp/ril.modem.type;;
            $CYIT) echo 4 > ${SYS_MNT}/etc/ppp/ril.modem.type;;
            $TELECOM)
                echo 5 > ${SYS_MNT}/etc/ppp/ril.modem.type
                mv $MODULE_PATH/option.ko $MODULE_PATH/option-bk.ko
                ;;
            $HUAWEI_W) echo 8 > ${SYS_MNT}/etc/ppp/ril.modem.type;;
            $HUAWEI_TD) echo 9 > ${SYS_MNT}/etc/ppp/ril.modem.type;;
            $HAIER_EVDO)
                echo 5 > ${SYS_MNT}/etc/ppp/ril.modem.type
                echo 5 > ${SYS_MNT}/etc/ppp/ril.modem.type2
                mv $MODULE_PATH/option.ko $MODULE_PATH/option-bk.ko
                ;;
        *)
            info "${usb_item} is not a modem or hasn't been supported now";;
        esac
    done

    info "Configure 2/3G modem done!"
else
    info "No RIL enabled!"
fi
###############set ethernet dongle rtl8152 led configuration#############
	dir_bak=$PWD
	insmod $MODULE_PATH/r8152_setled.ko
	sleep 1
	cd  ${SYS_MNT}/etc
	info `${SYS_MNT}/bin/pg8152-arm-cortex-a9 /r /efuse |grep "EFUSE_LED_SEL_CFG" |awk '{print $5$6}'`
	info `cat ${SYS_MNT}/etc/EF8152b.cfg |grep ^LED_SEL_CFG|awk '{print $3$4}'`
        if [ `${SYS_MNT}/bin/pg8152-arm-cortex-a9 /r /efuse |grep "EFUSE_LED_SEL_CFG" |awk '{print $5$6}'`x != `cat ${SYS_MNT}/etc/EF8152b.cfg |grep ^LED_SEL_CFG|awk '{print $3$4}'`x ];then
                info "rtl8152 unequ"
                ${SYS_MNT}/bin/pg8152-arm-cortex-a9  /efuse
        else
                info "rtl8152 equ"
        fi
	cd $dir_bak
	

#########################wifi device detect###################################
mtk6620_depend_insmod() {
    ln -s ${SYS_MNT}/etc/firmware /etc/firmware
    ln -s ${SYS_MNT}/etc/nvram /data/nvram
    insmod $MODULE_PATH/mtk_hif_sdio.ko
    insmod $MODULE_PATH/mtk_stp_wmt.ko
    insmod $MODULE_PATH/mtk_stp_uart.ko
    insmod $MODULE_PATH/mtk_wmt_wifi.ko
    insmod $MODULE_PATH/wlan_mt6620.ko
    mknod /dev/stpwmt c 190 0
    mknod /dev/stpgps c 191 0
    mknod /dev/fm c 193 0
    mknod /dev/wmtWifi c 194 0
    ${SYS_MNT}/bin/6620_launcher_static -b 921600 -p ${SYS_MNT}/etc/firmware -d /dev/ttyS1 &
    sleep 4
    echo "1">/dev/wmtWifi
    sleep 3
}

mtk6620_depend_rmmod() {
    killall 6620_launcher_static
    /system/bin/rmmod mtk_wmt_wifi
    /system/bin/rmmod mtk_stp_uart
    /system/bin/rmmod mtk_stp_wmt
    /system/bin/rmmod mtk_hif_sdio
}
mtk5931_depend_insmod() {
    ln -s ${SYS_MNT}/etc/firmware /etc/firmware
    insmod $MODULE_PATH/wlan_drv_5931_power.ko
    mknod /dev/wmtWifi c 194 0
    echo "1">/dev/wmtWifi
}

mtk5931_depend_rmmod() {
    echo "0">/dev/wmtWifi
    /system/bin/rmmod wlan_drv_5931_power
    /system/bin/rmmod wlan_mt5931
}

mkdir -p ${SYS_MNT}/etc/wifi

# Vixen only support 8188eu
#wifi_drivers="eagle 8192cu 8188eu 8723au mt7601Usta nmc1000 wlan_mt5931 bcmdhd_ap6330 bcmdhd_ap6476 bcmdhd wlan_mt6620"
wifi_drivers="8188eu"
for wifi_drv in $wifi_drivers
do
    info "detect $wifi_drv"
    if [ "$wifi_drv" = "wlan_mt6620" ]; then
            mtk6620_depend_insmod
    elif [ "$wifi_drv" = "wlan_mt5931" ]; then
            mtk5931_depend_insmod
    fi    

	if [ "$wifi_drv" = "bcmdhd_ap6330" ]; then
		insmod $MODULE_PATH/wmt_rfkill_bluetooth.ko
	fi

	if [ "$wifi_drv" = "mt7601Usta" ]; then
		insmod $MODULE_PATH/mtprealloc7601Usta.ko
	fi	
    
    insmod $MODULE_PATH/${wifi_drv}.ko
    sleep 1
    if [ "$wifi_drv" = "mt7601Usta" ]; then   ##fix the problem: rmmod mt7601Usta module lead to system hangup
        ifconfig wlan0 up
        sleep 1
        ifconfig wlan0 down
        sleep 1
    fi

    /system/bin/rmmod $wifi_drv
    wifi_name=`cat /proc/wifi_config`

    wifi_name_without_ko=`cat ${SYS_MNT}/etc/wifi_module`                                
    if [ "$wifi_name_without_ko" = "" ];then                                                
        echo "${SYS_MNT}/etc/wifi_module not found"                                                        
    else                                                                        
        #find  /system/etc/wifi_module ,skip wifi detection   
        wifi_drv="$wifi_name_without_ko"
        wifi_name="${wifi_name_without_ko}.ko"
    fi
	
    info "wifi_name:$wifi_name, wifi driver: $wifi_drv"
    local isWiFiDetected=0
    if [ "$wifi_name" = "${wifi_drv}.ko" ]; then
        info "find $wifi_drv"
        isWiFiDetected=1

        if [ "$wifi_drv" = "8192cu" -o "$wifi_drv" = "8188eu" -o "$wifi_drv" = "8723au" ]; then
            info "enter $wifi_drv branch"
            if [ "$wifi_drv" = "8192cu" ]; then
                echo "wifi.driver.name=8192cu">>${SYS_MNT}/default.prop
                echo "wifi.driver.path=/system/modules/3.4.5-default/8192cu.ko">>${SYS_MNT}/default.prop
                cp -f ${SYS_MNT}/bin/wpa_supplicant_rtl8192c ${SYS_MNT}/bin/wpa_supplicant
                cp -f ${SYS_MNT}/bin/hostapd_rtl8192c ${SYS_MNT}/bin/hostapd
            elif [ "$wifi_drv" = "8188eu" ]; then
                echo "wifi.driver.name=8188eu">>${SYS_MNT}/default.prop
                echo "wifi.driver.path=/system/modules/3.4.5-default/8188eu.ko">>${SYS_MNT}/default.prop
                cp -f ${SYS_MNT}/bin/wpa_supplicant_rtl8192e ${SYS_MNT}/bin/wpa_supplicant
                cp -f ${SYS_MNT}/bin/hostapd_rtl8192e ${SYS_MNT}/bin/hostapd
            elif [ "$wifi_drv" = "8723au" ]; then
                echo "wifi.driver.name=8723au">>${SYS_MNT}/default.prop
                echo "wifi.driver.path=/system/modules/3.4.5-default/8723au.ko">>${SYS_MNT}/default.prop
                cp -f ${SYS_MNT}/bin/wpa_supplicant_rtl8192e ${SYS_MNT}/bin/wpa_supplicant
                cp -f ${SYS_MNT}/bin/hostapd_rtl8192e ${SYS_MNT}/bin/hostapd
	        #delete 6620 bluetooth related files
	        info "delete 6620 bluetooth related files"
		rm -rf  ${SYS_MNT}/lib/libbluetooth_em.so
		rm -rf  ${SYS_MNT}/lib/libbluetooth_mtk_pure.so
		rm -rf  ${SYS_MNT}/lib/libbt-vendor_6620.so
		rm -rf  ${SYS_MNT}/lib/libbluetooth_mtk_6620.so
		rm -rf  ${SYS_MNT}/lib/hw/bluetooth.mtk6620.so
		rm -rf  ${SYS_MNT}/lib/libbluetooth_mtk.so
		#copy 8723 bluetooth related files
		info "copy 8723 bluetooth related files"
		cp -vf 8723bt/libbt-hci.so ${SYS_MNT}/lib/libbt-hci.so
		cp -vf 8723bt/libbt-utils.so ${SYS_MNT}/lib/libbt-utils.so
		cp -vf 8723bt/libbt-vendor.so ${SYS_MNT}/lib/libbt-vendor.so
		cp -vf 8723bt/audio.a2dp.default.so ${SYS_MNT}/lib/hw/
		cp -vf 8723bt/bluetooth.default.so ${SYS_MNT}/lib/hw/
		#copy bluetooth apk and config xml file
		info "copy bluetooth apk and config xml file"
		info "sys_mnt: ${SYS_MNT}"
		cp -rf bluetooth/system/* ${SYS_MNT}/
            fi
            echo "wifi.driver.arg=ifname=wlan0 if2name=p2p0">>${SYS_MNT}/default.prop
            cp -f ${SYS_MNT}/lib/libhardware_legacy_rtl.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.rtl8192c.rc
        elif [ "$wifi_drv" = "rt3070sta_rt5370" -o "$wifi_drv" = "sci9211" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_sci ${SYS_MNT}/bin/wpa_supplicant
            if [ -f ${SYS_MNT}/etc/permissions/android.hardware.wifi.direct.xml ]; then
                rm ${SYS_MNT}/etc/permissions/android.hardware.wifi.direct.xml
            fi
        elif [ "$wifi_drv" = "mt7601Usta" ]; then
            info "enter $wifi_drv branch"
			echo "wifi.driver.name=mt7601Usta">>${SYS_MNT}/default.prop
            cp -f ${SYS_MNT}/bin/wpa_supplicant_mt7601 ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/netd_mt7601 ${SYS_MNT}/bin/netd
            cp -f ${SYS_MNT}/lib/hostapd_cli_mt7601.so ${SYS_MNT}/lib/hostapd_cli.so
            cp -f ${SYS_MNT}/lib/libwpa_client_mt7601.so ${SYS_MNT}/lib/libwpa_client.so
            cp -f ${SYS_MNT}/lib/libhardware_legacy_mt7601.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.mtk7601.rc
        elif [ "$wifi_drv" = "wlan_mt6620" ]; then
            info "enter $wifi_drv branch"
            ln -s /system/modules/3.4.5-default ${SYS_MNT}/lib/modules
            cp -f ${SYS_MNT}/bin/wpa_supplicant_mtk6620 ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/netd_mtk6620 ${SYS_MNT}/bin/netd
            cp -f ${SYS_MNT}/lib/libwpa_client_mtk6620.so ${SYS_MNT}/lib/libwpa_client.so
            cp -f ${SYS_MNT}/lib/libhardware_legacy_mtk6620.so ${SYS_MNT}/lib/libhardware_legacy.so
            cp -f ${SYS_MNT}/lib/libbluetooth_mtk_6620.so ${SYS_MNT}/lib/libbluetooth_mtk.so
            cp -f ${SYS_MNT}/lib/libbt-vendor_6620.so ${SYS_MNT}/lib/libbt-vendor.so
            cp -rf /mnt/mmcblk0p1/FirmwareInstall/bluetooth/system/* ${SYS_MNT}/
            cp -f /mnt/mmcblk0p1/FirmwareInstall/optional/FMRadio.apk ${SYS_MNT}/app/
            echo "wmt.gps.so.path=/system/lib/hw/gps.mtk6620.so">>${SYS_MNT}/default.prop
            echo "wmt.bluetooth.so.path=/system/lib/hw/bluetooth.mtk6620.so">>${SYS_MNT}/default.prop
            echo "ro.wmt.ui.tether_wifi_intf=ap0">>${SYS_MNT}/default.prop
            echo "#!/bin/sh">${SYS_MNT}/etc/wmt/S99wifi.sh
            echo "echo \"1\">/sys/mmc2/intr">>${SYS_MNT}/etc/wmt/S99wifi.sh
            chmod 755 ${SYS_MNT}/etc/wmt/S99wifi.sh
            setenv wmt.init.rc init.mtk6620.rc
            setenv wmt.gpo.wifi 9c:1:0
        elif [ "$wifi_drv" = "wlan_mt5931" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_mt5931 ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/netd_mt5931 ${SYS_MNT}/bin/netd
            cp -f ${SYS_MNT}/lib/libwpa_client_mt5931.so ${SYS_MNT}/lib/libwpa_client.so
            cp -f ${SYS_MNT}/lib/libhardware_legacy_mt5931.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.mt5931.rc
        elif [ "$wifi_drv" = "nmc1000" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_nmc1000 ${SYS_MNT}/bin/wpa_supplicant
#            cp -f ${SYS_MNT}/bin/netd_mt5931 ${SYS_MNT}/bin/netd
            cp -f ${SYS_MNT}/lib/libwpa_client_nmc1000.so ${SYS_MNT}/lib/libwpa_client.so
            cp -f ${SYS_MNT}/lib/libhardware_legacy_nmc1000.so ${SYS_MNT}/lib/libhardware_legacy.so
            rm -rf ${SYS_MNT}/etc/permissions/android.hardware.wifi.direct.xml
            setenv wmt.init.rc init_nmc1000.rc
            setenv wmt.gpo.wifi 9c:1:0
        elif [ "$wifi_drv" = "eagle" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_eagle ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/netd_eagle ${SYS_MNT}/bin/netd
            cp -f ${SYS_MNT}/bin/hostapd_eagle ${SYS_MNT}/bin/hostapd
            cp -f ${SYS_MNT}/lib/libwpa_client_eagle.so ${SYS_MNT}/lib/libwpa_client.so
            cp -f ${SYS_MNT}/lib/libhardware_legacy_eagle.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.eagle.rc
            setenv wmt.gpo.wifi 9c:1:1000
        elif [ "$wifi_drv" = "bcmdhd" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_bcm ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/hostapd_bcm ${SYS_MNT}/bin/hostapd
            cp -f ${SYS_MNT}/lib/libhardware_legacy_bcm.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.bcm.rc
            setenv wmt.gpo.wifi 9c:1:0
        elif [ "$wifi_drv" = "bcmdhd_ap6476" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_bcm ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/hostapd_bcm ${SYS_MNT}/bin/hostapd
            cp -f ${SYS_MNT}/lib/libhardware_legacy_bcm_ap6476.so ${SYS_MNT}/lib/libhardware_legacy.so
#			cp -f {SYS_MNT}/etc/firmware/nvram_ap6476.txt {SYS_MNT}/etc/firmware/nvram_ap6181.txt
#			cp -f $MODULE_PATH/bcmdhd_ap6476.ko $MODULE_PATH/bcmdhd.ko
            setenv wmt.init.rc init.bcm6476.rc
#            setenv wmt.gpo.wifi 9c:1:0
        elif [ "$wifi_drv" = "bcmdhd_ap6330" ]; then
            info "enter $wifi_drv branch"
            cp -f ${SYS_MNT}/bin/wpa_supplicant_bcm ${SYS_MNT}/bin/wpa_supplicant
            cp -f ${SYS_MNT}/bin/hostapd_bcm ${SYS_MNT}/bin/hostapd
            cp -f ${SYS_MNT}/lib/libhardware_legacy_bcm_ap6330.so ${SYS_MNT}/lib/libhardware_legacy.so
            setenv wmt.init.rc init.bcm6330.rc
        fi

        chmod 755 ${SYS_MNT}/lib/libbt-vendor.so
        chmod 755 ${SYS_MNT}/lib/libbluetooth_mtk.so
        chmod 755 ${SYS_MNT}/lib/libhardware_legacy.so
        chmod 755 ${SYS_MNT}/lib/libwpa_client.so
        chmod a+x ${SYS_MNT}/bin/netd
        chmod a+x ${SYS_MNT}/bin/wpa_supplicant
        chmod a+x ${SYS_MNT}/bin/hostapd
    fi

    if [ "$wifi_drv" = "wlan_mt6620" ]; then
        mtk6620_depend_rmmod
    elif [ "$wifi_drv" = "wlan_mt5931" ]; then
        mtk5931_depend_rmmod
    elif [ "$wifi_drv" = "bcmdhd_ap6330" ]; then
        /system/bin/rmmod wmt_rfkill_bluetooth
    fi
	
    if [ "$isWiFiDetected" = "1" ]; then
        info "return because detected WiFi"
        break
    fi
    sync
done
###########

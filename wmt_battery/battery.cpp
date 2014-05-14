/*
 * Copyright 2013 WonderMedia Technologies, Inc. All Rights Reserved. 
 *  
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc. 
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any third party, 
 * in whole or in part, without prior written consent of WonderMedia. 
 *  
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 * WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, 
 * AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  
 */

#include "battery.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <utils/Timers.h>
#include <cutils/properties.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BATTERY_CONFIGURE_PATH "/proc/battery_calibration"
#define BUFFER_SIZE 128

#define ADAPTER_PLUG_OUT   		0
#define ADAPTER_PLUG_IN    		1

#define HOP_VOLTAGE_RANGE 40
#define CALIBRATE_VOLTAGE_ARRAY_SIZE 3 //calibration size. at least more than two.
#define SYSTEM_BOOT_LAST		30 * 1000 //30 seconds.
#define SUSPEND_RESUME_LAST		4 * 1000 //4 seconds.
#define ADAPTER_STATE_CHANGE_LAST	4 * 1000 //5 seconds.
#define SUSPEND_RESUME_LAST_MINUTE	8 * 60 * 1000 //10 minutes.
#define ADAPTER_CHARGING_STEP	2 //make sure not hop more than 2 percent in a minutes if is charging.
#define ADAPTER_CHARGING_FULL_STEP	3 //make sure if is charged full, quick rise to 100.
static bool bat_susp_long_flag = false; //0: suspend less than SUSPEND_RESUME_LAST_MINUTE time.
static bool bat_susp_resu_flag = false; //0: normal, 1--suspend then resume
static bool adapter_change_flag = false; //use current capacity to return insetead regard it as hop voltage
static bool manual_hop_to_full = false; //if true, next step although it is not full, show 100% instead.
static bool need_reprot_bat_state = false;

static int mWritePos, mCount;

enum {
	POWER_SUPPLY_STATUS_UNKNOWN = 0,
	POWER_SUPPLY_STATUS_CHARGING = 1,
	POWER_SUPPLY_STATUS_DISCHARGING = 2,
	POWER_SUPPLY_STATUS_NOT_CHARGING = 3,
	POWER_SUPPLY_STATUS_FULL = 4
};

enum {
	BM_VOLUME = 0,
	BM_BRIGHTNESS = 1,
	BM_WIFI = 2,
	BM_VIDEO = 3,
	BM_USBDEVICE = 4,
	BM_HDMI = 5,
	BM_COUNT
};

enum {
	STATUS = 0,
	DCIN = 1,
	VOLTAGE = 2,
	FULL = 3,
	SLEEP = 4,
	DEBUG = 5,
	VOLUME = 6,
	BRIGHTNESS = 7,
	WIFI = 8,
	VIDEO = 9,
	USB = 10,
	HDMI = 11,
	RAW_COUNT
};

static struct RawData {
	int status;
	int dcin;
	int voltage;
	int full;
	int sleep;
	int debug;
	int volume;
	int brightness;
	int wifi;
	int video;
	int usb;
	int hdmi;
	int capacity;
	int repair_vol;
} m_RawDataBuffer[BUFFER_SIZE];

struct battery_session {
	bool inited;
	int capacity;
	int timestamp;
	struct RawData data;
} bat_session = { false, 0, 0 };

typedef struct {
	int voltage_effnorm;
	int voltage_effmax;
} module_effect;

static struct battery_module_calibration {
	module_effect brightness_ef[10];
	module_effect wifi_ef[10];
	module_effect adapter_ef[10];
} module_calibration;

struct battery_config {
	int update_time;
	int wmt_operation;
	int ADC_USED;
	int ADC_TYP; // 1--read capability from vt1603 by SPI;3--Read capability from vt1603 by I2C
	int charge_max;
	int charge_min;
	int discharge_vp[11]; //battery capacity curve:v0--v10
	int lowVoltageLevel;
	int judge_full_by_time;
};

struct battery_config bat_config;
static int module_usage[BM_COUNT];

static int debug_level() {
	char value[PROPERTY_VALUE_MAX];
	int level = 0;
	if(property_get("persist.battery.debug",value,"0")>0) {
		level = atoi(value);
		ALOGD("debug level = %d.",level);
	}
	return level;
}

static int level = debug_level();
#define DEBUG(fmt, args...)  if(level > 0) ALOGD(fmt, ##args)

extern "C" {
int wmt_getsyspara(const char *varname, char *varval, int * varlen);
}

static int adc2cap(int adc);
static int rawDataBuffer_push();
static RawData* rawDataBuffer_pop();
static RawData* rawDataBuffer_read(int cursor);
static int rawDataBuffer_remove();

//if run service the first time , force update battery capacity multi time.
static long boot_complete_time = -1;

static int64_t getCurrentTime()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* current time in ms */
static inline int time_stamp() {
	static int64_t init_time = 0;
	if (init_time == 0)
		init_time = getCurrentTime();
	return getCurrentTime()- init_time;
}

static int getsyspara(const char *varname, char *varval, int * varlen) {
	*varlen = 200;
	int res = wmt_getsyspara(varname, varval, varlen);
	return res;
}

bool BatteryServer::batt_init() {
	char buf[200];
	int varlen;
	memset(buf, 0, 200);

	if (getsyspara("wmt.io.bat", buf, &varlen) == 0) {
		sscanf(buf, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
				&bat_config.ADC_TYP, &bat_config.wmt_operation,
				&bat_config.update_time, &bat_config.charge_max,
				&bat_config.charge_min, &bat_config.discharge_vp[10],
				&bat_config.discharge_vp[9], &bat_config.discharge_vp[8],
				&bat_config.discharge_vp[7], &bat_config.discharge_vp[6],
				&bat_config.discharge_vp[5], &bat_config.discharge_vp[4],
				&bat_config.discharge_vp[3], &bat_config.discharge_vp[2],
				&bat_config.discharge_vp[1], &bat_config.discharge_vp[0]);
	} else {
		ALOGD("Not found wmt.io.bat, return false!");
		return false;
	}
	memset(&module_calibration, 0, sizeof(struct battery_module_calibration));

	if (getsyspara("wmt.io.bateff.brightness", buf, &varlen) == 0) {
		sscanf(buf,
				"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
				&module_calibration.brightness_ef[9].voltage_effnorm,
				&module_calibration.brightness_ef[9].voltage_effmax,
				&module_calibration.brightness_ef[8].voltage_effnorm,
				&module_calibration.brightness_ef[8].voltage_effmax,
				&module_calibration.brightness_ef[7].voltage_effnorm,
				&module_calibration.brightness_ef[7].voltage_effmax,
				&module_calibration.brightness_ef[6].voltage_effnorm,
				&module_calibration.brightness_ef[6].voltage_effmax,
				&module_calibration.brightness_ef[5].voltage_effnorm,
				&module_calibration.brightness_ef[5].voltage_effmax,
				&module_calibration.brightness_ef[4].voltage_effnorm,
				&module_calibration.brightness_ef[4].voltage_effmax,
				&module_calibration.brightness_ef[3].voltage_effnorm,
				&module_calibration.brightness_ef[3].voltage_effmax,
				&module_calibration.brightness_ef[2].voltage_effnorm,
				&module_calibration.brightness_ef[2].voltage_effmax,
				&module_calibration.brightness_ef[1].voltage_effnorm,
				&module_calibration.brightness_ef[1].voltage_effmax,
				&module_calibration.brightness_ef[0].voltage_effnorm,
				&module_calibration.brightness_ef[0].voltage_effmax);
	} else {
		module_calibration.brightness_ef[9].voltage_effnorm = 0xeb1;
		module_calibration.brightness_ef[9].voltage_effmax = 0xef3;
		module_calibration.brightness_ef[8].voltage_effnorm = 0xe57;
		module_calibration.brightness_ef[8].voltage_effmax = 0xe98;
		module_calibration.brightness_ef[7].voltage_effnorm = 0xe13;
		module_calibration.brightness_ef[7].voltage_effmax = 0xe55;
		module_calibration.brightness_ef[6].voltage_effnorm = 0xdcc;
		module_calibration.brightness_ef[6].voltage_effmax = 0xe0f;
		module_calibration.brightness_ef[5].voltage_effnorm = 0xdaa;
		module_calibration.brightness_ef[5].voltage_effmax = 0xdee;
		module_calibration.brightness_ef[4].voltage_effnorm = 0xd83;
		module_calibration.brightness_ef[4].voltage_effmax = 0xdca;
		module_calibration.brightness_ef[3].voltage_effnorm = 0xd56;
		module_calibration.brightness_ef[3].voltage_effmax = 0xda0;
		module_calibration.brightness_ef[2].voltage_effnorm = 0xd24;
		module_calibration.brightness_ef[2].voltage_effmax = 0xd71;
		module_calibration.brightness_ef[1].voltage_effnorm = 0xce9;
		module_calibration.brightness_ef[1].voltage_effmax = 0xd43;
		module_calibration.brightness_ef[0].voltage_effnorm = 0xcbf;
		module_calibration.brightness_ef[0].voltage_effmax = 0xd18;
	}

	if (getsyspara("wmt.io.bateff.wifi", buf, &varlen) == 0) {
		sscanf(buf,
				"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
				&module_calibration.wifi_ef[9].voltage_effnorm,
				&module_calibration.wifi_ef[9].voltage_effmax,
				&module_calibration.wifi_ef[8].voltage_effnorm,
				&module_calibration.wifi_ef[8].voltage_effmax,
				&module_calibration.wifi_ef[7].voltage_effnorm,
				&module_calibration.wifi_ef[7].voltage_effmax,
				&module_calibration.wifi_ef[6].voltage_effnorm,
				&module_calibration.wifi_ef[6].voltage_effmax,
				&module_calibration.wifi_ef[5].voltage_effnorm,
				&module_calibration.wifi_ef[5].voltage_effmax,
				&module_calibration.wifi_ef[4].voltage_effnorm,
				&module_calibration.wifi_ef[4].voltage_effmax,
				&module_calibration.wifi_ef[3].voltage_effnorm,
				&module_calibration.wifi_ef[3].voltage_effmax,
				&module_calibration.wifi_ef[2].voltage_effnorm,
				&module_calibration.wifi_ef[2].voltage_effmax,
				&module_calibration.wifi_ef[1].voltage_effnorm,
				&module_calibration.wifi_ef[1].voltage_effmax,
				&module_calibration.wifi_ef[0].voltage_effnorm,
				&module_calibration.wifi_ef[0].voltage_effmax);
	} else {
		module_calibration.wifi_ef[9].voltage_effnorm = 0xea8;
		module_calibration.wifi_ef[9].voltage_effmax = 0xea0;
		module_calibration.wifi_ef[8].voltage_effnorm = 0xe57;
		module_calibration.wifi_ef[8].voltage_effmax = 0xe3d;
		module_calibration.wifi_ef[7].voltage_effnorm = 0xe12;
		module_calibration.wifi_ef[7].voltage_effmax = 0xdf6;
		module_calibration.wifi_ef[6].voltage_effnorm = 0xdcc;
		module_calibration.wifi_ef[6].voltage_effmax = 0xdaf;
		module_calibration.wifi_ef[5].voltage_effnorm = 0xda9;
		module_calibration.wifi_ef[5].voltage_effmax = 0xda5;
		module_calibration.wifi_ef[4].voltage_effnorm = 0xd82;
		module_calibration.wifi_ef[4].voltage_effmax = 0xd7e;
		module_calibration.wifi_ef[3].voltage_effnorm = 0xd55;
		module_calibration.wifi_ef[3].voltage_effmax = 0xd50;
		module_calibration.wifi_ef[2].voltage_effnorm = 0xd24;
		module_calibration.wifi_ef[2].voltage_effmax = 0xd1d;
		module_calibration.wifi_ef[1].voltage_effnorm = 0xce8;
		module_calibration.wifi_ef[1].voltage_effmax = 0xccc;
		module_calibration.wifi_ef[0].voltage_effnorm = 0xcbe;
		module_calibration.wifi_ef[0].voltage_effmax = 0xcb1;
	}

	if (getsyspara("wmt.io.bateff.adapter", buf, &varlen) == 0) {
		sscanf(buf,
				"%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x:%x-%x",
				&module_calibration.adapter_ef[9].voltage_effnorm,
				&module_calibration.adapter_ef[9].voltage_effmax,
				&module_calibration.adapter_ef[8].voltage_effnorm,
				&module_calibration.adapter_ef[8].voltage_effmax,
				&module_calibration.adapter_ef[7].voltage_effnorm,
				&module_calibration.adapter_ef[7].voltage_effmax,
				&module_calibration.adapter_ef[6].voltage_effnorm,
				&module_calibration.adapter_ef[6].voltage_effmax,
				&module_calibration.adapter_ef[5].voltage_effnorm,
				&module_calibration.adapter_ef[5].voltage_effmax,
				&module_calibration.adapter_ef[4].voltage_effnorm,
				&module_calibration.adapter_ef[4].voltage_effmax,
				&module_calibration.adapter_ef[3].voltage_effnorm,
				&module_calibration.adapter_ef[3].voltage_effmax,
				&module_calibration.adapter_ef[2].voltage_effnorm,
				&module_calibration.adapter_ef[2].voltage_effmax,
				&module_calibration.adapter_ef[1].voltage_effnorm,
				&module_calibration.adapter_ef[1].voltage_effmax,
				&module_calibration.adapter_ef[0].voltage_effnorm,
				&module_calibration.adapter_ef[0].voltage_effmax);
	} else {
		module_calibration.adapter_ef[9].voltage_effnorm = 0xe9b;
		module_calibration.adapter_ef[9].voltage_effmax = 0xf54;
		module_calibration.adapter_ef[8].voltage_effnorm = 0xe50;
		module_calibration.adapter_ef[8].voltage_effmax = 0xf02;
		module_calibration.adapter_ef[7].voltage_effnorm = 0xe0c;
		module_calibration.adapter_ef[7].voltage_effmax = 0xeb9;
		module_calibration.adapter_ef[6].voltage_effnorm = 0xde7;
		module_calibration.adapter_ef[6].voltage_effmax = 0xe97;
		module_calibration.adapter_ef[5].voltage_effnorm = 0xdc5;
		module_calibration.adapter_ef[5].voltage_effmax = 0xe76;
		module_calibration.adapter_ef[4].voltage_effnorm = 0xda4;
		module_calibration.adapter_ef[4].voltage_effmax = 0xe55;
		module_calibration.adapter_ef[3].voltage_effnorm = 0xd7c;
		module_calibration.adapter_ef[3].voltage_effmax = 0xe2d;
		module_calibration.adapter_ef[2].voltage_effnorm = 0xd4e;
		module_calibration.adapter_ef[2].voltage_effmax = 0xe06;
		module_calibration.adapter_ef[1].voltage_effnorm = 0xd1c;
		module_calibration.adapter_ef[1].voltage_effmax = 0xdd8;
		module_calibration.adapter_ef[0].voltage_effnorm = 0xce0;
		module_calibration.adapter_ef[0].voltage_effmax = 0xdaa;
	}

	boot_complete_time = time_stamp();

	return true;
}

static int get_history_vol(int vol[], int size) {
	if (mCount < size)
		size = mCount;

	int len = 0;
	for (int i = 0; i < size; i++) {
		vol[i] = rawDataBuffer_read(i + 1)->repair_vol;
		len++;
	}

	return len;
}

static void reset_voltage_array(int vol[], int size) {
	for (int i = 0; i < size; i++) {
		vol[i] = 0;
	}
}

//去除最大最小值，然后返回一个平均值
static int get_average_value(int adcVal[], int len) {
	int returnValue;
	int maxValue = adcVal[0], minValue = adcVal[0], sum = adcVal[0];
	int i = 1;
	for (; i < len; i++) {
		if (adcVal[i] > maxValue) {
			maxValue = adcVal[i];
		}
		if (adcVal[i] < minValue) {
			minValue = adcVal[i];
		}
		sum += adcVal[i];
	}

	if (len > 2) {
		sum = sum - maxValue - minValue;
		returnValue = sum / (len - 2);
	} else if (len == 2) {
		returnValue = sum / 2;
	} else {
		return sum;
	}
	return returnValue;
}

//采用三阶滤波，使波形呈现平缓/滑上升或下降趋势，去掉突变adc值的影响。
static double filter2_weight[] = { 0.3, 0.7 };
static double filter3_weight[] = { 0.2, 0.4, 0.4 };
static int get_best_value(int adcVal[], int len) {
	double* k;
	if (len == 2)
		k = filter2_weight;
	else if (len == 3)
		k = filter3_weight;
	else
		return get_average_value(adcVal, len);

	int i = 0;
	double sum = 0;
	while (i < len) {
		sum = sum + k[i] * adcVal[i];
		i++;
	}

	return sum;
}

//出现大幅度越变的转折点时，以线性减缓或递增处理，如长时间休眠，导致电压突变。
static int get_hop_value(int y0, int y1, int hop_vol[], int size) {
	int hop = size;
	int repair_vol = y0;
	long _step = (y1 - y0) * 1.000f / hop;

	int loop = 0;
	while (loop < hop) {
		repair_vol = repair_vol + _step;
		hop_vol[loop] = repair_vol;
		loop++;
	}

	return hop;
}

//当实际电量低于10%，并且是处于放电状态时， 动态改变下降步长，防止电量过低而导致强制关机！
static int get_low_capacity_step(int capacity) {
	int step;

	int distance = bat_session.capacity - capacity;
	if (distance <= 0)
		return 0;

	switch (distance / 10) {
	case 1:
	case 2:
	case 3:
	case 4:
		step = 3;
		break;
	case 5:
		step = 4;
		break;
	case 6:
		step = 5;
		break;
	case 7:
	case 8:
	case 9:
		step = 6;
		break;
	default:
		step = 1;
	}

	return step;
}

static int formatCapacity(int cap, int step) {
	if (cap < 0)
		cap = 0;
	if (cap > 0)
		cap = ((cap - 1) / step + 1) * step;
	if (cap > 100)
		cap = 100;

	return cap;
}

static inline bool is_charging(struct battery_session *sess) {
	return (sess->data.status == POWER_SUPPLY_STATUS_CHARGING
			|| sess->data.status == POWER_SUPPLY_STATUS_FULL);
}

static inline bool is_plugedin(struct battery_session *sess) {
	return sess->data.dcin == ADAPTER_PLUG_IN;
}

static inline bool is_update(struct battery_session *sess) {
	return ((bat_session.data.dcin != sess->data.dcin)
			|| (bat_session.data.status != sess->data.status)
			|| (bat_session.capacity != sess->capacity));
}

static int voltage_calibrate(int repair_adc) {
	int averageVol[CALIBRATE_VOLTAGE_ARRAY_SIZE];
	static int hop_voltage_count = 0;

	//if not in linear mode ,and hop voltage continuelly occured one times ,  discard them.
	if (abs(repair_adc - rawDataBuffer_read(2)->repair_vol) > HOP_VOLTAGE_RANGE && hop_voltage_count < 1) {
		DEBUG("hop point continuelly occured ,discard them !");
		rawDataBuffer_remove();
		hop_voltage_count++;
		return rawDataBuffer_read(1)->repair_vol;
	}else{
		hop_voltage_count = 0;
	}

	reset_voltage_array(averageVol, CALIBRATE_VOLTAGE_ARRAY_SIZE);

	if (bat_susp_long_flag) {
		//采用二阶滤波算法，获取一组平滑变化的曲线值，只在唤醒第一次起效。
		DEBUG("Using two-stage filtering algorithm !");
		bat_susp_long_flag = false;
		int read_len = get_history_vol(averageVol, CALIBRATE_VOLTAGE_ARRAY_SIZE -1);
		if (read_len >= (CALIBRATE_VOLTAGE_ARRAY_SIZE - 1)) {
			repair_adc = get_best_value(averageVol, CALIBRATE_VOLTAGE_ARRAY_SIZE -1);
		}
	} else {
		//采用三阶滤波算法，获取一组平滑变化的曲线值。
		int read_len = get_history_vol(averageVol, CALIBRATE_VOLTAGE_ARRAY_SIZE);
		if (read_len >= CALIBRATE_VOLTAGE_ARRAY_SIZE) {
			repair_adc = get_best_value(averageVol, CALIBRATE_VOLTAGE_ARRAY_SIZE);
		}
	}

	return repair_adc;
}

static int getEffect(int adc, int usage, module_effect* effects, int size) {
	int effect = 0, i;
	for (i = size - 2; i >= 0; i--) {
		if (adc < effects[i+1].voltage_effmax && adc >= effects[i].voltage_effmax) {
			effect = (effects[i].voltage_effmax - effects[i].voltage_effnorm)
					* usage / 100;
			return effect;
		}
	}

	if (adc >= effects[size - 1].voltage_effmax) {
		effect = (effects[size - 1].voltage_effmax
				- effects[size - 1].voltage_effnorm) * usage / 100;
	} else if (adc <= effects[0].voltage_effmax) {
		effect = (effects[0].voltage_effmax - effects[0].voltage_effnorm)
				* usage / 100;
	}

	return effect;
}

static int adc_repair(struct battery_session *sess) {
	int adc = sess->data.voltage;
	int adc_comp = adc;

	//if now is charging or usb adapter is plug in.
	if (is_charging(sess)) {
		adc_comp -= getEffect(adc, 100, module_calibration.adapter_ef,
				ARRAY_SIZE(module_calibration.adapter_ef));
		adc_comp += 10;
	} else if (is_plugedin(sess)) {
		if (bat_session.capacity >= 0 && bat_session.capacity <= 100) {
			int comp = (60 * (100 - bat_session.capacity)) / 100;
			adc_comp -= comp;
			DEBUG("compensation USB-PC(%d) -> adc_comp %d\n",comp, adc_comp);
		}
	}

	// brightness
	int usage = (100 - module_usage[BM_BRIGHTNESS]) / 2;
	if (usage < 0)
		usage = 0;
	if (usage > 100)
		usage = 100;
	int comp = getEffect(adc, usage, module_calibration.brightness_ef,
			ARRAY_SIZE(module_calibration.brightness_ef));
	adc_comp -= comp;

	// video
	comp = (module_usage[BM_VIDEO] * 25) / 100;
	adc_comp += comp;

	// usb/wifi
	comp = getEffect(adc, module_usage[BM_USBDEVICE],
			module_calibration.wifi_ef, ARRAY_SIZE(module_calibration.wifi_ef));
	adc_comp += comp;

	// hdmi
	if (module_usage[BM_HDMI] > 0) {
		comp = 15;
		adc_comp += comp;
	}

	return adc_comp;
}

static int adc2cap(int adc_val) {
	int capacity = 0;
	int i;

	if (adc_val >= bat_config.discharge_vp[10]) {
		capacity = 100;
	} else if (adc_val <= bat_config.discharge_vp[0]) {
		capacity = 0;
	} else {
		for (i = 10; i >= 1; i--) {
			if ((adc_val <= bat_config.discharge_vp[i])
					&& (adc_val > bat_config.discharge_vp[i - 1])) {
				capacity = (adc_val - bat_config.discharge_vp[i - 1]) * 10
						/ (bat_config.discharge_vp[i]
								- bat_config.discharge_vp[i - 1])
						+ (i - 1) * 10;
			}
		}
	}

	return capacity;
}

static int get_resume_capacity(struct battery_session *sess) {
	//for two stage filter.
	int repair_val = adc_repair(sess);
	rawDataBuffer_pop()->repair_vol = repair_val;

	int capacity = adc2cap(repair_val);
	if (capacity <= 0){
		ALOGD("Get resume capacity low than zero!!");
		capacity = 20;
	}

	rawDataBuffer_pop()->capacity = capacity;
	bat_session.capacity = capacity;
	bat_session.timestamp = sess->timestamp;
	bat_session.data = sess->data;
	need_reprot_bat_state = true;
	DEBUG("Resume after long time, return capacity instead (%d -> %d).",repair_val, capacity);
	return capacity;
}

//When adapter plug in or out, or resume from supend,
//dont call this function, because the voltage is not reliable, try to use old capacity only
static int read_capacity(struct battery_session *sess) {
	int capacity = 0;
	int repair_val;

	repair_val = adc_repair(sess);
	DEBUG("repair_val = %d",repair_val);
	// here because next step will read the pop data of repair_vol.
	rawDataBuffer_pop()->repair_vol = repair_val;

	//对电压做三阶滤波或线性平滑处理
	repair_val = voltage_calibrate(repair_val);
	rawDataBuffer_pop()->repair_vol = repair_val;

	//FIX ME:  电池滤波，避免电压抖动引起的越变？
	capacity = adc2cap(repair_val);
	DEBUG("adc_val %d , adc_repair_val %d, capacity %d\n", sess->data.voltage,
			repair_val, capacity);

	capacity = formatCapacity(capacity, 2);

	//because suspend long time, the capacity may have big hop ,so record the history capcity here.
	rawDataBuffer_pop()->capacity = capacity;
	return capacity;
}

static void get_battery_session(struct battery_session *sess) {
	RawData* data = rawDataBuffer_pop();

	sess->data = *data;
	sess->timestamp = time_stamp();
}

static void inline init_battery_session(struct battery_session *sess) {
	get_battery_session(sess);
}

static int batt_report(void) {
	static int boot_stat = 0;
	static int boot_max_cap = -1;
	static int resu_timestamp = 0;
	static int adapter_timestamp = 0;
	static int charging_full_timestamp = -1;
	static bool is_charging_full_wait = false;

	int capacity = 0;
	struct battery_session curr_sess;
	get_battery_session(&curr_sess);

	if (!bat_session.inited) {
		init_battery_session(&bat_session);
		bat_session.capacity = -1; //wait for next time to init it.
		bat_session.inited = true;
	}

	if (curr_sess.data.debug == 1) {
		DEBUG("just for battery compare tool, make sure not shut down!");
		if(bat_session.capacity != 90){
			bat_session.capacity = 90;
			bat_session.timestamp = curr_sess.timestamp;
			need_reprot_bat_state = true;
		}

		return 90;
	}

	if (curr_sess.data.full == 1) {
		curr_sess.capacity = 100;
		if (is_update(&curr_sess)) {
			DEBUG("Now is full, show capacity 100 instead!");
			bat_session.capacity = 100;
			bat_session.timestamp = curr_sess.timestamp;
			bat_session.data = curr_sess.data;

			rawDataBuffer_pop()->repair_vol = adc_repair(&curr_sess);
			rawDataBuffer_pop()->capacity = 100;
			need_reprot_bat_state = true;
		}else{
			rawDataBuffer_remove();
		}

		return 100;
	}

	if ((boot_stat < 30) && (time_stamp() - boot_complete_time) <= SYSTEM_BOOT_LAST) {
		boot_stat++;
		int cap = read_capacity(&curr_sess);
		cap = cap > 16 ? cap : 16;

		if(cap > boot_max_cap){
			//Note: here not remove history data buffer!
			DEBUG("Boot complete within little time ,refresh new capacity (%d)!",cap);
			boot_max_cap = cap;
		}else{
			//No need save buffer data!
			rawDataBuffer_remove();
		}

		if(boot_stat == 1){
			need_reprot_bat_state = true;
			return cap;
		}

		return -1;
	}else if(boot_max_cap > 0){
		//here should reset boot_max_cap to make sure that only report the boot fix capacity one time.
		rawDataBuffer_remove();

		int cap = boot_max_cap;
		need_reprot_bat_state = true;
		boot_max_cap = -1;
		DEBUG("Boot end, and report the boot fix capacity (%d)!",cap);

		return cap;
	}

	if (curr_sess.data.sleep == 1) {

		//Just waking up after SUSPEND_RESUME_LAST_MINUTE, so mark it here, next step will use.
		if ((curr_sess.timestamp - bat_session.timestamp) > SUSPEND_RESUME_LAST_MINUTE){
			bat_susp_long_flag = true;
			DEBUG("Wake and make long suspend flag is set!");
		}
		manual_hop_to_full = false;
		bat_susp_resu_flag = true;
		bat_session.data = curr_sess.data;
		resu_timestamp = curr_sess.timestamp;
	}

	if (curr_sess.data.dcin != bat_session.data.dcin) {
		// External adapter plug-out or plug-in. here should update bat_session.data member.
		manual_hop_to_full = false;
		adapter_change_flag = true;
		bat_session.data = curr_sess.data;
		adapter_timestamp = curr_sess.timestamp;

		if(!is_charging(&curr_sess)){
			DEBUG("Change to discharging ,so reset charging full wait mark!");
			is_charging_full_wait = false;
			charging_full_timestamp = -1;
		}
	}

	if (bat_susp_resu_flag
			&& (curr_sess.timestamp - resu_timestamp) < SUSPEND_RESUME_LAST) {
		rawDataBuffer_remove();
		//return the real old capacity instead!
		if(bat_susp_long_flag){
			DEBUG("Resume has called , return last real capacity instead (%d).",
					rawDataBuffer_pop()->capacity);
			need_reprot_bat_state = true;
			return rawDataBuffer_pop()->capacity>0? rawDataBuffer_pop()->capacity : 20;
		}
		DEBUG("Resume has called , return last capacity instead (%d).", curr_sess.data.voltage);
		return -1;
	} else if (bat_susp_resu_flag) {
		bat_susp_resu_flag = false;

		//waking up after SUSPEND_RESUME_LAST_MINUTE,should get newest value instead.
		if(bat_susp_long_flag)
			return get_resume_capacity(&curr_sess);
	}

	if (adapter_change_flag
			&& (curr_sess.timestamp - adapter_timestamp) < ADAPTER_STATE_CHANGE_LAST) {
		DEBUG("Adapter has changed, return last capacity instead (%d).", curr_sess.data.voltage);
		rawDataBuffer_remove();
		return -1;
	} else if (adapter_change_flag) {
		adapter_change_flag = false;
	}

	capacity = read_capacity(&curr_sess);

	if (bat_session.capacity == -1) {
		//confirm if service is start first time ,init the history capacity here.and report capacity force!
		DEBUG("Start first time ,init capacity (%d) here,and report capacity force!", capacity);
		bat_session.capacity = capacity;
		bat_session.timestamp = curr_sess.timestamp;
		need_reprot_bat_state = true;
	}

	//强制拉平电量，减小波动
	int temp_capacity = capacity;
	if (is_charging(&curr_sess)) {
		if(bat_session.capacity>=98 && !is_charging_full_wait){
			DEBUG("Now is nearly 100, wait for some time here!");
			charging_full_timestamp = curr_sess.timestamp;
			is_charging_full_wait = true;
		}

		DEBUG("full_timestamp = %d",charging_full_timestamp);
		if(charging_full_timestamp>=0 && bat_session.capacity>=98 &&
				((curr_sess.timestamp - charging_full_timestamp) < 15 * 60 * 1000)){
			capacity = 99;

		} else {
			//if in charging mode , make sure the voltage on forward trend. hop not more than 2% every minutes.
			if ((curr_sess.timestamp - bat_session.timestamp) > 10 * 60 * 1000) {
				DEBUG("because stay for a long time, just increase it.");
				capacity = bat_session.capacity + ADAPTER_CHARGING_STEP;

			} else if (capacity < bat_session.capacity) {
				capacity = bat_session.capacity;

			} else if ((capacity - bat_session.capacity)
					>= ADAPTER_CHARGING_STEP) {
				if ((curr_sess.timestamp - bat_session.timestamp) <= 30 * 1000)
					capacity = bat_session.capacity;
				else
					capacity = bat_session.capacity + ADAPTER_CHARGING_STEP;
			}
		}
	} else if (capacity > 10) {
		//if in discharging mode and capacity>10% , make sure the voltage on downward trend. hop not more than 1% every minutes
		if ((curr_sess.timestamp - bat_session.timestamp) > 10 * 60 * 1000) {
			DEBUG("because stay for a long time, just reduce it.");
			capacity = bat_session.capacity - ADAPTER_CHARGING_STEP;

		} else if (capacity > bat_session.capacity) {
			capacity = bat_session.capacity;

		} else if ((bat_session.capacity - capacity) >= ADAPTER_CHARGING_STEP) {
			if ((curr_sess.timestamp - bat_session.timestamp) <= 30 * 1000)
				capacity = bat_session.capacity;
			else
				capacity = bat_session.capacity - ADAPTER_CHARGING_STEP;
		}
	} else { //here in critical discharging mode, used another algorithm.
		int step = get_low_capacity_step(capacity);
		capacity = bat_session.capacity - step;
	}

	if (capacity < 0)
		capacity = 0;
	else if (capacity > 100)
		capacity = 100;
	DEBUG("capacity hop from (%d -> %d).", temp_capacity, capacity);

	curr_sess.capacity = capacity;

	//here majorly process reporting logic!
	if (bat_session.capacity <= 10 && !is_charging(&curr_sess)) {
		if (bat_session.capacity <= 5
				&& (curr_sess.timestamp - bat_session.timestamp) > 60 * 1000) {
			DEBUG("now low than 5 and every minutes report.");
			need_reprot_bat_state = true;
		} else if (bat_session.capacity > 5
				&& (curr_sess.timestamp - bat_session.timestamp) > 120 * 1000) {
			DEBUG("now low than 10 and every two minutes report.");
			need_reprot_bat_state = true;
		} else if (curr_sess.capacity < 2) {
			DEBUG("now low than 2 ,so immedially report.");
			need_reprot_bat_state = true;
		} else if (is_update(&curr_sess)) {
			DEBUG("now capacity low than 10 and should update,but no need report.");
			bat_session.data = curr_sess.data;
			bat_session.capacity = curr_sess.capacity;
			bat_session.timestamp = curr_sess.timestamp;
		}
	} else if (is_update(&curr_sess)) {
		//now capacity is more than 10%.
		DEBUG("now more than 10 or isCharging,so should update immedially.");
		need_reprot_bat_state = true;
	}

	if (need_reprot_bat_state) {
		bat_session.data = curr_sess.data;
		bat_session.capacity = curr_sess.capacity;
		bat_session.timestamp = curr_sess.timestamp;
		return bat_session.capacity;
	}

	return -1;
}

static int reportUevent(const char* path, int capacity) {
	if (!path)
		return -1;
	
	int fd = open(path, O_WRONLY | O_APPEND, 0);
	if (fd == -1) {
		ALOGE("Could not open '%s'", path);
		return -1;
	}

	char buf[50];
	sprintf(buf, "capacity=%d", capacity);
	int count = write(fd, buf, sizeof(buf));
	close(fd);
	return count;
}

static bool rawDataBuffer_isFull() {
	return mCount >= BUFFER_SIZE;
}

static int rawDataBuffer_push(int data[]) {
	RawData * rawData = &m_RawDataBuffer[mWritePos++];
	if (mWritePos == BUFFER_SIZE)
		mWritePos = 0;

	if (mCount < BUFFER_SIZE)
		mCount++;

	*rawData = {data[STATUS], data[DCIN], data[VOLTAGE], data[FULL], data[SLEEP],data[DEBUG],
		data[VOLUME], data[BRIGHTNESS], data[WIFI], data[VIDEO], data[USB], data[HDMI]};
	rawData->repair_vol = data[VOLTAGE];

	return mWritePos;
}

static RawData* rawDataBuffer_pop() {
	if (mCount <= 0)
		return &m_RawDataBuffer[0];

	int index = (mWritePos - 1 + BUFFER_SIZE) % BUFFER_SIZE;
	return &m_RawDataBuffer[index];
}

//read the newest cursor of raw data.
static RawData* rawDataBuffer_read(int cursor) {
	if (cursor > mCount)
		return rawDataBuffer_pop();

	int index = (mWritePos - cursor + BUFFER_SIZE) % BUFFER_SIZE;
	return &m_RawDataBuffer[index];
}

//remove the newest raw data.
static int rawDataBuffer_remove() {
	if (mCount <= 0)
		return 0;

	mCount--;
	mWritePos = (mWritePos - 1 + BUFFER_SIZE) % BUFFER_SIZE;
	m_RawDataBuffer[mWritePos] = {0,0,0,0,0,0,0,0,0,0,0,0};

	return mWritePos;
}

static int initCompensation(const char* path) {
	if (!path)
		return -1;
	FILE * fd = fopen(path, "ro");
	if (fd == NULL) {
		ALOGE("Could not open '%s'", path);
		return -1;
	}

	int res[RAW_COUNT] = { 0 };
	char buf[200];
	int row = -1, colum = 0;
	bool stop = false;
	while (fgets(buf, 200, fd) != NULL) {
		row++;
		colum = 0;
		std::stringstream ss(buf);
		std::string sub_str;
		while (getline(ss, sub_str, ':')) {
			colum++;
			if (colum == 2 && row < RAW_COUNT) {
				int val = atoi(sub_str.c_str());
				res[row] = val;
				break;
			}
		}
	}
	fclose(fd);

	rawDataBuffer_push(res);
	for (int i = 6, j = 0; i < BM_COUNT + 6; i++, j++) {
		module_usage[j] = res[i];
	}

	return 0;
}

bool BatteryServer::batt_update() {

	if (initCompensation(BATTERY_CONFIGURE_PATH) == -1) {
		ALOGD("Read battery compensation failed!");
		return false;
	}


	int result = batt_report();
	if (need_reprot_bat_state) {
		if (result >= 0) {
			reportUevent(BATTERY_CONFIGURE_PATH,  result);
		}
		need_reprot_bat_state = false;
	}

	return true;
}

void BatteryServer::batt_deinit() {
	
}

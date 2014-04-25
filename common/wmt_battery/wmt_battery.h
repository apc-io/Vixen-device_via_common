#ifndef WMT_BATTERY_H_INCLUDED
#define WMT_BATTERY_H_INCLUDED  1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum
{
    BM_VOLUME        = 0,
    BM_BRIGHTNESS    = 1,
    BM_WIFI          = 2,
    BM_VIDEO         = 3,
    BM_USBDEVICE     = 4,
    BM_HDMI          = 5,
    BM_COUNT		 = BM_HDMI
} BatteryModule;
/**
 * @param module    : Module ID
 * @param factor    :
 *                  该模块本身消耗电量的全开程度百分比，例如播放1080P电影，那么可能是100，
 *                  播放320P电影，可能是20（仅仅针对该模块）
 *
 *                  音量，如果0，那么是0，如果100，那么是100。具体引起电压下降多少，后续
 *                  有系数处理。
 *
 *                  背光：如果最低亮度，那么为0，如果最高，那么为100。其他可能要判断一下，给个
 *                  0-100之间的数值。
 *
 *                  HDMI：如果是1080P显示，那么是100，如果是720P，可能是70，如果关，应该是0.
 *
 *                  WIFI：关为0，如果开，可能要根据是否连上，收发速率给个大概值。
 */

static void inline reportModuleChangeEventForBattery(BatteryModule module, int factor)

{
	char buf[64];
    int f = open("/proc/battery_calibration",O_WRONLY);
	if(f == -1)
		return;

    sprintf(buf, "MODULE_CHANGE:%d-%d",module,factor);
    write(f, buf,sizeof(buf));
    close(f);
}
#endif //WMT_BATTERY_H_INCLUDED


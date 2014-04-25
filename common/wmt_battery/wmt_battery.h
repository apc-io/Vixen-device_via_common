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
 *                  ��ģ�鱾�����ĵ�����ȫ���̶Ȱٷֱȣ����粥��1080P��Ӱ����ô������100��
 *                  ����320P��Ӱ��������20��������Ը�ģ�飩
 *
 *                  ���������0����ô��0�����100����ô��100�����������ѹ�½����٣�����
 *                  ��ϵ������
 *
 *                  ���⣺���������ȣ���ôΪ0�������ߣ���ôΪ100����������Ҫ�ж�һ�£�����
 *                  0-100֮�����ֵ��
 *
 *                  HDMI�������1080P��ʾ����ô��100�������720P��������70������أ�Ӧ����0.
 *
 *                  WIFI����Ϊ0�������������Ҫ�����Ƿ����ϣ��շ����ʸ������ֵ��
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


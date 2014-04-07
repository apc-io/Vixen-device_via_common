#define LOG_TAG "BatteryServer"

#ifndef BATTERY_H_
#define BATTERY_H_

#include <utils/Thread.h>
#include <cutils/log.h>
#include <android_runtime/AndroidRuntime.h>
using namespace android;

class BatteryServer: public Thread {

public:

    BatteryServer() {
	  
    }

    ~BatteryServer() {
        batt_deinit();
    }

    bool threadLoop() {
	 bool init = batt_init();
	
        while (init && batt_update()) {
            usleep(m_interval * 1000);  //microseconds
        }

		ALOGD("Thread loop is false, exit!");
		return false;
	}

    void setInterval(int second) {
        m_interval = second * 1000;
    }

private:
    unsigned long m_interval; //millisecond

    bool batt_init();
    void batt_deinit();
    bool batt_update();
};

#endif /* BATTERY_H_ */

// vim: et ts=4 shiftwidth=4

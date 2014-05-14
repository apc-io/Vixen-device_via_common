/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "MagneticSensor.h"
#include "nusensors.h"
#include "wmtsensor.h"

#define LOGE_IF ALOGE_IF
#define MEMSICD_PROP "msensor.memsicd"
/*****************************************************************************/
#define INPUT_NAME_MAG		"ecompass_data"  // "DMT_accel" "ecompass_data"
#define SensorDevice		"/dev/ecompass_ctrl"

MagneticSensor::MagneticSensor()
    : SensorBase(SensorDevice, INPUT_NAME_MAG),
      mEnabled(0),
      //mOrientationEnabled(0),
      mInputReader(32)
{ 
	int flags;
	int err = 0;
   	mPendingEvent.version = sizeof(sensors_event_t);
   	mPendingEvent.sensor = ID_M;
   	mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
	mPendingEvent.magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
	memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));

	open_device();
	err = ioctl(dev_fd, ECOMPASS_IOC_GET_MFLAG, &flags);
	err = err<0 ? -errno : 0;
	LOGE_IF(err, "ECOMPASS_IOC_SET_MFLAG failed (%s)", strerror(-err));
	mEnabled = flags;	
    	
	close_device();
    	
}

MagneticSensor::~MagneticSensor() {
}

int MagneticSensor::enable(int32_t, int en)
{
   //ALOGE("<<<<magnetic sensor enable :%d\n", en);
	int flags = en ? 1 : 0;
    	int err = 0;
	char buf[PROPERTY_VALUE_MAX] = {0};
	 
    	if (flags != mEnabled) 
    	{	if (flags == 1) {
			property_get("msensor.memsicd", buf, "0");
			if (strcmp(buf, "0") == 0)
				property_set("msensor.memsicd", "1");
			
		}	
		else
			property_set("msensor.memsicd", "0");
		open_device();
        	
       		err = ioctl(dev_fd, ECOMPASS_IOC_SET_MFLAG, &flags);
        	err = err<0 ? -errno : 0;
        	LOGE_IF(err, "ECOMPASS_IOC_SET_MFLAG failed (%s)", strerror(-err));
        	mEnabled = flags;
        	
       	    close_device();
    	}
    	
	return 0;
}

int MagneticSensor::setDelay(int32_t handle, int64_t ns)
{
	if (ns < 0)
		return -EINVAL;
	if (mEnabled ) 
	{
        	int delay = ns / 1000000;
		ALOGD("msensor send ECOMPASS_IOC_SET_DELAY %d\n", delay);
        	
		open_device();
        	if (ioctl(dev_fd, ECOMPASS_IOC_SET_DELAY, &delay))
        	{
            	return -errno;
        	}
		close_device();
   	}
   	return 0;
}




int MagneticSensor::readEvents(sensors_event_t* data, int count)
{
	//ALOGE("<<<<magnetic sensor readEvents\n");
	if (count < 1)
        return -EINVAL;

    	ssize_t n = mInputReader.fill(data_fd);
    	if (n < 0)
        	return n;
    	int numEventReceived = 0;
        input_event const* event;
	
    	while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
		
        if (type == EV_ABS) {
		
		switch (event->code){
		case EVENT_TYPE_MAGV_X:
		
		mPendingEvent.magnetic.x = event->value * CONVERT_M_X;
		break;
		case EVENT_TYPE_MAGV_Y:
		
		mPendingEvent.magnetic.y = event->value * CONVERT_M_Y;
		break;
		case EVENT_TYPE_MAGV_Z:
		
		mPendingEvent.magnetic.z = event->value * CONVERT_M_Z;
		break;
		
		case EVENT_TYPE_MAGV_STATUS:
		// accuracy of the magnetic sensor
				
		mPendingEvent.magnetic.status = event->value;
		break;
		
		default:
		break;
		}
        	
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            mPendingEvent.timestamp = time;
            if (mEnabled) {
            	ALOGE_IF(0, "x=%x,y=%x,z=%x", (int)mPendingEvent.acceleration.x,
            								(int)mPendingEvent.acceleration.y,
            								(int)mPendingEvent.acceleration.z);
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        // accelerometer sends valid ABS events for
        // userspace using EVIOCGABS
        } else if (type != EV_ABS) { 
            ALOGE("MagneticSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    	}
	return numEventReceived;
}




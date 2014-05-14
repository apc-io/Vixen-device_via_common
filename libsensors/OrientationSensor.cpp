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
#include "wmtsensor.h"
#include "OrientationSensor.h"
#include "nusensors.h"

/*****************************************************************************/
#define LOGE_IF ALOGE_IF
#define INPUT_NAME_MAG		"ecompass_data"
#define SensorDevice		"/dev/ecompass_ctrl"

/* Use 'e' as magic number */
#define ECOMPASS_IOM			'e'
#define ECOMPASS_IOC_SET_OFLAG		_IOW(ECOMPASS_IOM, 0x14, short)
#define ECOMPASS_IOC_SET_DELAY		_IOW(ECOMPASS_IOM, 0x01, short)
#define EVENT_TYPE_ORIENT_YAW		ABS_RX
#define EVENT_TYPE_ORIENT_PITCH		ABS_RY
#define EVENT_TYPE_ORIENT_ROLL		ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS	ABS_RUDDER

OrientationSensor::OrientationSensor()
    : SensorBase(SensorDevice, INPUT_NAME_MAG),
      mEnabled(0),
      //mOrientationEnabled(0),
      mInputReader(32)
{
	//ALOGE("<<<oresensor init\n");
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_O;
    mPendingEvent.type = SENSOR_TYPE_ORIENTATION;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_LOW;//
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));

    open_device();

    /*int flags = 0;
    if (!ioctl(dev_fd, KXTF9_IOCTL_GET_ENABLE, &flags)) {
        if (flags)  {
            mEnabled = 1;
        }
    }
    if (!mEnabled) {
        close_device();
    }*/
    mEnabled = 1;
    mEnabled = 0; //FIXME 2013-4-18
}

OrientationSensor::~OrientationSensor() 
{
}

int OrientationSensor::enable(int32_t handle, int en)
{
	//ALOGE("<<<<oresensor sensor enable %d\n", en);
	int flags = en ? 1 : 0;
    	int err = 0;
	char buf[PROPERTY_VALUE_MAX] = {0};
    	if (flags != mEnabled) 
    	{
		if (flags == 1){
			property_get("msensor.memsicd", buf, "0");
			if (strcmp(buf, "0") == 0)
				property_set("msensor.memsicd", "1");
		}	
		else
			property_set("msensor.memsicd", "0");
		open_device();
        	
       		err = ioctl(dev_fd, ECOMPASS_IOC_SET_OFLAG, &flags);
        	err = err<0 ? -errno : 0;
        	LOGE_IF(err, "ECOMPASS_IOC_SET_MFLAG failed (%s)", strerror(-err));
        	mEnabled = flags;
       	       
    	}	
	return 0;
}

int OrientationSensor::setDelay(int32_t handle, int64_t ns)
{
	if (ns < 0)
        return -EINVAL;
	if (mEnabled ) 
	{
        	int delay = ns / 1000000;
        	ALOGD_IF(0,"send ECS_IOCTL_APP_SET_DELAY");
        	if (ioctl(dev_fd, ECOMPASS_IOC_SET_DELAY, &delay))
        	{
            	return -errno;
        	}
   	}
   	return 0;
}

int OrientationSensor::readEvents(sensors_event_t* data, int count)
{
	//ALOGE("<<<<oresensor sensor readEvents\n");
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
		case EVENT_TYPE_ORIENT_YAW:
		
		mPendingEvent.orientation.x = event->value * CONVERT_O_Y;
		//ALOGE("<<<orex %f\n", mPendingEvent.orientation.x);
		break;
		case EVENT_TYPE_ORIENT_PITCH:
		
		mPendingEvent.orientation.y = event->value * CONVERT_O_P;
		//ALOGE("<<<orey %f\n", mPendingEvent.orientation.y);
		break;
		case EVENT_TYPE_ORIENT_ROLL:
		
		mPendingEvent.orientation.z = event->value * CONVERT_O_R;
		//ALOGE("<<<orez %f\n", mPendingEvent.orientation.z);
		break;
		
		case EVENT_TYPE_ORIENT_STATUS:
		// accuracy of the magnetic sensor
				
		mPendingEvent.orientation.status = event->value;
		//ALOGE("<<<oreStatus %d\n", mPendingEvent.orientation.status);
		break;
		
		default:
		break;
		}
        	
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            mPendingEvent.timestamp = time;
            if (mEnabled) {
            	ALOGE_IF(0, "x=%x,y=%x,z=%x", (int)mPendingEvent.orientation.x,
            								(int)mPendingEvent.orientation.y,
            								(int)mPendingEvent.orientation.z);
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        // accelerometer sends valid ABS events for
        // userspace using EVIOCGABS
        } else if (type != EV_ABS) { 
            ALOGE("OrientationSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    	}
	return numEventReceived;
}
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

#include "AccelerationSensorD08.h"
#include "wmtsensor.h"
/*****************************************************************************/

AccelerationSensorD08::AccelerationSensorD08()
    : SensorBase(ACCELEROMETER_DEVICE_NAME, ACC_INPUT_NAME),
      mEnabled(0),
      //mOrientationEnabled(0),
      mInputReader(32)
{
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));
	
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

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
}

AccelerationSensorD08::~AccelerationSensorD08() {
}

int AccelerationSensorD08::enable(int32_t, int en)
{
    int flags = en ? 1 : 0;
    int err = 0;
    if (flags != mEnabled) {
        /*// don't turn the accelerometer off, if the orientation
        // sensor is enabled
        if (mOrientationEnabled && !en) {
            mEnabled = flags;
            return 0;
        }*/
        if (flags) {
            open_device();
        }
        err = ioctl(dev_fd, ECS_IOCTL_APP_SET_AFLAG, &flags);
        err = err<0 ? -errno : 0;
        ALOGE_IF(err, "ECS_IOCTL_APP_SET_AFLAG failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = flags;
        }
        if (!flags) {
            close_device();
        }
    }
    return err;
}
/*
int AccelerationSensorD08::enableOrientation(int en)
{
    int flags = en ? 1 : 0;
    int err = 0;
    if (flags != mOrientationEnabled) {
        // don't turn the accelerometer off, if the user has requested it
        if (mEnabled && !en) {
            mOrientationEnabled = flags;
            return 0;
        }

        if (flags) {
            open_device();
        }
        err = ioctl(dev_fd, KXTF9_IOCTL_SET_ENABLE, &flags);
        err = err<0 ? -errno : 0;
        LOGE_IF(err, "KXTF9_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mOrientationEnabled = flags;
        }
        if (!flags) {
            close_device();
        }
    }
    return err;
}
*/
int AccelerationSensorD08::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    if (mEnabled /*|| mOrientationEnabled*/) {
        int delay = ns / 1000000;
        if (ioctl(dev_fd, ECS_IOCTL_APP_SET_DELAY, &delay)) {
            return -errno;
        }
    }
    return 0;
}


static void msleep(int ms)
{
	for(int i=0;i<ms;ms--)
	{
		usleep(1000);
	};
}


int AccelerationSensorD08::readEvents(sensors_event_t* data, int count)
{
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
            processEvent(event->code, event->value);
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            mPendingEvent.timestamp = time;
            if (mEnabled) {
            	ALOGD_IF(0, "x=%x,y=%x,z=%x", (int)mPendingEvent.acceleration.x,
            								(int)mPendingEvent.acceleration.y,
            								(int)mPendingEvent.acceleration.z);
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        // accelerometer sends valid ABS events for
        // userspace using EVIOCGABS
        } else if (type != EV_ABS) { 
            ALOGE("AccelerationSensorD08: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}


void AccelerationSensorD08::processEvent(int code, int value)
{
	/*int x,y,z;

	// unpacket the merging value
	x = 0xFF&value;
	y = 0xFF&(value>>8);
	z=  0xff&(value>>16);
	LOGE_IF(0, "rx=%x,ry=%x,rz=%x,xyz=%x", x,y,z,value);
	if (x&0x80)
	{
		x = x - 256;
	}
	if (y&0x80)
	{
		y = y -256;
	}
	if (z&0x80)
	{
		z= z - 256;
	}
	*/
	//LOGD_IF(0, "x=%x,y=%x,z=%x", x,y,z);

	
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = value * GRAVITY_EARTH / LSG_D08;
            //mPendingEvent.acceleration.y = y * GRAVITY_EARTH / LSG_D08;
            //mPendingEvent.acceleration.z = z * GRAVITY_EARTH / LSG_D08;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = value * GRAVITY_EARTH / LSG_D08;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = value * GRAVITY_EARTH / LSG_D08;
            break;
    }
}

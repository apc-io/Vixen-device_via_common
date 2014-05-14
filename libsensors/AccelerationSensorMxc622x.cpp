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
#include "wmtsensor.h"
#include "AccelerationSensorMxc622x.h"
/*****************************************************************************/

AccelerationSensorMxc622x::AccelerationSensorMxc622x()
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

AccelerationSensorMxc622x::~AccelerationSensorMxc622x() {
}

int AccelerationSensorMxc622x::enable(int32_t, int en)
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
int AccelerationSensorMxc622x::enableOrientation(int en)
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
        ALOGE_IF(err, "KXTF9_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
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
int AccelerationSensorMxc622x::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    if (mEnabled /*|| mOrientationEnabled*/) {
        int delay = ns / 1000000;
        if (ioctl(dev_fd, ECS_IOCTL_APP_SET_DELAY, &delay)) {
            return -errno;
        }
        #if SENSOR_DEBUG
        ALOGD_IF(SENSOR_DEBUG,"AccelerationSensorMxc622x::setDelay] delay =%d\n",delay);
        #endif
    }
    
    return 0;
}

int AccelerationSensorMxc622x::readEvents(sensors_event_t* data, int count)
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
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        // accelerometer sends valid ABS events for
        // userspace using EVIOCGABS
        } else if (type != EV_ABS) { 
            ALOGE("AccelerationSensorMxc622x: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}


void AccelerationSensorMxc622x::processEvent(int code, int value)
{
	int x,y,z;

	// unpacket the merging value
	x = 0xFF&value;
	y = 0xFF&(value>>8);
	z=  0xff&(value>>16);
	#if SENSOR_DEBUG
	ALOGE_IF(SENSOR_DEBUG, "rx=%x,ry=%x,rz=%x,xyz=%x", x,y,z,value);
	#endif
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
	#if SENSOR_DEBUG
	ALOGE_IF(SENSOR_DEBUG, "x=%x,y=%x,z=%x", x,y,z);
	#endif

	
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = x * GRAVITY_EARTH / LSG_MXC622X;
            mPendingEvent.acceleration.y = y * GRAVITY_EARTH / LSG_MXC622X;
            mPendingEvent.acceleration.z = z * GRAVITY_EARTH / LSG_MXC622X;
            break;
        /*case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = value * CONVERT_A_Z;
            break;*/
    }
}

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

#include "AccelerationSensorD06.h"
#include "wmtsensor.h"
//////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/

AccelerationSensorD06::AccelerationSensorD06()
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

AccelerationSensorD06::~AccelerationSensorD06() {
}

int AccelerationSensorD06::enable(int32_t, int en)
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

int AccelerationSensorD06::setDelay(int32_t handle, int64_t ns)
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

int AccelerationSensorD06::readEvents(sensors_event_t* data, int count)
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
            ALOGE("AccelerationSensorD06: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}


void AccelerationSensorD06::processEvent(int code, int value)
{
	int x,y,z;
	int tx,ty,tz;

	tz = value & 0x0FF;
	ty = (value >> 8) & 0x0FF;
	tx = (value >> 16) & 0xFF;

	x = (tx & 0x80)!=0 ? (tx-256) : tx;
	y = (ty & 0x80)!=0 ? (ty-256) : ty;
	z = (tz & 0x80)!=0 ? (tz-256) : tz;

	ALOGD_IF(0, "x=%x,y=%x,z=%x", x,y,z);

	
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = x * GRAVITY_EARTH / LSG_D06;
            mPendingEvent.acceleration.y = y * GRAVITY_EARTH / LSG_D06;
            mPendingEvent.acceleration.z = z * GRAVITY_EARTH / LSG_D06;
            break;
        /*case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = value * CONVERT_A_Z;
            break;*/
    }
}

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
#include "GyroSensorL3g4200d.h"

#define GYRO_DEBUG 1
/*****************************************************************************/

GyroSensorL3g4200d::GyroSensorL3g4200d()
    : SensorBase(GYROSCOPE_DEVICE_NAME, GYRO_INPUT_NAME),
      mEnabled(0),
      mInputReader(32)
{
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));

    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_G;
    mPendingEvent.type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvent.gyro.status = SENSOR_STATUS_ACCURACY_HIGH;

    open_device();

    int flags = 0;
    if (!ioctl(dev_fd, GYRO_IOCTL_GET_ENABLE, &flags)) {
        if (flags)  {
            mEnabled = 1;
        }
    }

    if (!mEnabled) {
        close_device();
    }
}

GyroSensorL3g4200d::~GyroSensorL3g4200d() {
}

int GyroSensorL3g4200d::enable(int32_t, int en)
{
    int flags = en ? 1 : 0;
    int err = 0;
    if (flags != mEnabled) {
        if (flags) {
            open_device();
        }
        err = ioctl(dev_fd, GYRO_IOCTL_SET_ENABLE, &flags);
        err = err<0 ? -errno : 0;
        ALOGE_IF(err, "GYRO_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = flags;
        }
        if (!flags) {
            close_device();
        }
    }
    return err;
}

int GyroSensorL3g4200d::setDelay(int32_t handle, int64_t ns)
{
	int64_t tmp_ns = ns;

    if (ns < 0)
        return -EINVAL;

	//limit the rate to reasonable one
	if (ns < 5000000LL)
        ns = 5000000LL;

    int delay = ns / 1000;
    if (ioctl(dev_fd, GYRO_IOCTL_SET_DELAY, &delay)) {
        return -errno;
    }

#if GYRO_DEBUG
    ALOGD_IF(GYRO_DEBUG, "gyro: would setDelay = %llu ns, in fact setDelay = %d us\n", tmp_ns, delay);
#endif

    return 0;
}

int GyroSensorL3g4200d::readEvents(sensors_event_t* data, int count)
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
        } else {
            ALOGE("GyroSensorL3g4200d: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}


void GyroSensorL3g4200d::processEvent(int code, int value)
{
	#if GYRO_DEBUG
	//ALOGE_IF(GYRO_DEBUG, "gyro: code = 0x%x, value = 0x%x", code, value);
	#endif

    switch (code) {
        case EVENT_TYPE_GYRO_P:
            mPendingEvent.gyro.x = value * CONVERT_G_P;
            break;
        case EVENT_TYPE_GYRO_R:
            mPendingEvent.gyro.y = value * CONVERT_G_R;
            break;
        case EVENT_TYPE_GYRO_Y:
            mPendingEvent.gyro.z = value * CONVERT_G_Y;
            break;
    }
}

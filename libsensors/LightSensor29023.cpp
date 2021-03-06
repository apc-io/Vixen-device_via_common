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
#include "LightSensor29023.h"
#include "nusensors.h"
/*****************************************************************************/

LightSensor29023::LightSensor29023()
    : SensorBase(LIGHTING_DEVICE_NAME, "lsensor_lux"),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
}

LightSensor29023::~LightSensor29023() {
}

int LightSensor29023::enable(int32_t, int en) {
    int err;
    en = en ? 1 : 0;
    if(mEnabled != en) {
        if (en) {
            open_device();
        }
        err = ioctl(dev_fd, LIGHT_IOCTL_SET_ENABLE,&en);
        err = err<0 ? -errno : 0;
        ALOGE_IF(err, "LIGHT_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = en;
        }
        if (!en) {
            close_device();
        }
    }
    return 0;
}

bool LightSensor29023::hasPendingEvents() const {
    return mHasPendingEvent;
}

int LightSensor29023::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == EVENT_TYPE_LIGHT) {
                mPendingEvent.light = indexToValue(event->value);
                #if SENSOR_DEBUG
        		ALOGD_IF(SENSOR_DEBUG,"LightSensor29023::readEvents light =%f\n",mPendingEvent.light);
        		#endif
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {            
            ALOGE("LightSensor29023: unknown event (type=%d, code=%d)",
                        type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

float LightSensor29023::indexToValue(size_t index) const
{
    return float(index);
}

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

#include "AccelerationSensorD10.h"
#include "wmtsensor.h"
/*****************************************************************************/

#define INPUT_NAME_ACC		"DMT_accel"
#define SensorDevice		"/dev/dmt"


AccelerationSensorD10::AccelerationSensorD10()
    : SensorBase(SensorDevice, INPUT_NAME_ACC),
      mEnabled(0),
      //mOrientationEnabled(0),
      mInputReader(32)
{
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));
	
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;


    //open_device();

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

AccelerationSensorD10::~AccelerationSensorD10() {
}

static int write_sys_attribute(const char *path, const char *value, int bytes)
{
    int ifd, amt;

	ifd = open(path, O_WRONLY);
    if (ifd < 0) {
        ALOGE("Write_attr failed to open %s (%s)",path, strerror(errno));
        return -1;
	}

    amt = write(ifd, value, bytes);
	amt = ((amt == -1) ? -errno : 0);
	ALOGE_IF(amt < 0, "Write_int failed to write %s (%s)",
		path, strerror(errno));
    close(ifd);
	return amt;
}


int AccelerationSensorD10::enable(int32_t, int en)
{
   	char path[]="/sys/class/accelemeter/dmt/enable_acc";
	char value[2]={0,0};
	value[0]= en ? '1':'0';
	//ALOGD("%s:enable=%s\n",__func__,value);
	write_sys_attribute(path,value, 1);
	return 0;
}

int AccelerationSensorD10::setDelay(int32_t handle, int64_t ns)
{
	char value[8]={0};
	int bytes;
	int delay = 0;
	char path[]="/sys/class/accelemeter/dmt/delay_acc";

	delay = ns / 1000000;
	bytes=sprintf(value,"%u",delay);
	//ALOGD("value=%s ,gsensor=%u, bytes=%d\n",value,delay,bytes);

	write_sys_attribute(path,value, bytes);
	return 0;
}


static void msleep(int ms)
{
	for(int i=0;i<ms;ms--)
	{
		usleep(1000);
	};
}


int AccelerationSensorD10::readEvents(sensors_event_t* data, int count)
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
            ALOGE("AccelerationSensorD10: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}


void AccelerationSensorD10::processEvent(int code, int value)
{
	/*int x,y,z;

	// unpacket the merging value
	x = 0xFF&value;
	y = 0xFF&(value>>8);
	z=  0xff&(value>>16);
	ALOGE_IF(0, "rx=%x,ry=%x,rz=%x,xyz=%x", x,y,z,value);
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
	//ALOGE_IF(0, "x=%x,y=%x,z=%x", x,y,z);


    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingEvent.acceleration.x = value * GRAVITY_EARTH / LSG_D10;
            //mPendingEvent.acceleration.y = y * GRAVITY_EARTH / LSG_D10;
            //mPendingEvent.acceleration.z = z * GRAVITY_EARTH / LSG_D10;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingEvent.acceleration.y = value * GRAVITY_EARTH / LSG_D10;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingEvent.acceleration.z = value * GRAVITY_EARTH / LSG_D10;
            break;
    }
}

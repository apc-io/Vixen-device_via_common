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

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <cutils/atomic.h>
#include <cutils/log.h>
#include <cutils/sockets.h>
//#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <cutils/properties.h>

#include "wmtsensor.h"
#include "nusensors.h"
#include "AccelerationSensor7660A.h"
#include "AccelerationSensorMC3230.h"
#include "AccelerationSensorD08.h"
#include "AccelerationSensorD06.h"
#include "AccelerationSensorD10.h"
#include "AccelerationSensorMxc622x.h"
#include "LightSensor29023.h"
#include "ProximitySensor.h"
#include "RemoteSensor.h"
#include "MagneticSensor.h"
#include "OrientationSensor.h"
//#include "AkmSensor.h"
//#include "PressureSensor.h"
//#include "GyroSensor.h"
#include "GyroSensorL3g4200d.h"
#include "AccelerationSensorGeneric.h"

/*****************************************************************************/
static int remoteEnabled = -1;
static inline int isRemoteEnable() 
{
    if (remoteEnabled == -1)
    {
        char buf[PROPERTY_VALUE_MAX];
        property_get("ro.wmt.rmtctrl.enable", buf, "0");
        remoteEnabled = buf[0] == '1' ? 1 : 0;
    }

    return remoteEnabled;
}


struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int pollEventsIndex(int index, sensors_event_t* data, int count);
    
    int debug_poll_time;
private:
    enum {
	acceleration    = 0,
	light           = 1,
	proximity	= 2,
	//akm		= 2,
	//pressure	= 3,
	gyro		= 3,
	magnetic	= 4,
	orientation	= 5,
        numSensorDrivers,
        numFds,
    };
    //int mSensorNum = 0;

    size_t wake ;//= numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds + 2];
    int mWritePipeFd;
    int mLocalControlFd;
    SensorBase* mSensors[numSensorDrivers];
    SensorBase* mLocalSensor;
    RemoteSensor* mRemoteSensor;
    int mPollIndex[numSensorDrivers];
    pthread_t thread;	
    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
                return acceleration;
			 case ID_L:
                return light;
            case ID_P:
                return proximity;
            case ID_M:
            	return magnetic;
            case ID_O:
               return orientation;
            case ID_G:
                return gyro;
//          case ID_B:
                // return pressure;
            default:
            	break;
               
        }
        return -EINVAL;
    }
    SensorBase* getAccSensor();
    SensorBase* getLgtSensor();
    SensorBase* getPxmSensor();
    SensorBase* getMagSensor();
    SensorBase* getOreSensor();
    SensorBase* getGyroSensor();
};

/*****************************************************************************/
int sensors_poll_context_t::pollEventsIndex(int index, sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

	do {
    	if (count) {
           
            do {
                n = poll(&mPollFds[index], 1, nbEvents ? 0 : -1);
            } while (n < 0 && errno == EINTR);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
           
        }
        // see if we have some leftover from the last poll()        
        for (int i=0 ; count && i<1; i++) {
			ALOGD_IF(SENSOR_DEBUG, "poll i=%d...\n", i);
        	if (mSensors[index] != NULL)
        	{
	            SensorBase* const sensor(mSensors[index]);
	            //LOGD_IF(SENSOR_DEBUG, "pollEvents...\n");
	            if ((mPollFds[index].revents & POLLIN) || (sensor->hasPendingEvents())) 
	            {
	            	ALOGD_IF(SENSOR_DEBUG,"[pollEvents] sensor_i=%d\n", i);
	                int nb = sensor->readEvents(data, count);
	                if (nb < count) {
	                    // no more data for this sensor
	                    mPollFds[index].revents = 0;
	                }
	                count -= nb;
	                nbEvents += nb;
	                data += nb;
	            }
            }
        }
	}
	while (n && count);
        // see if we have some leftover from the control socket and listen socket
     
    
    return nbEvents;	
}
    

SensorBase* sensors_poll_context_t::getAccSensor()
{
    mLocalSensor = NULL;
    mRemoteSensor = NULL;
    if (true/*sRemoteEnable()*/)
    {
        mRemoteSensor = createRemoteSensor();
    }
    
	switch (get_acc_drvid())
	{
		case MMA7660_DRVID:
			mLocalSensor = new AccelerationSensor7660A();
			break;
		case MC3230_DRVID:
			#if SENSOR_DEBUG
			ALOGD_IF(SENSOR_DEBUG, "[getAccSensor] Create mc3230!\n");
			#endif
			mLocalSensor = new AccelerationSensorMC3230();
			break;
		case DMARD08_DRVID:
			mLocalSensor = new AccelerationSensorD08();
			break;
		case DMARD06_DRVID:
			mLocalSensor = new AccelerationSensorD06(); 
			break;
		case DMARD10_DRVID:
			mLocalSensor = new AccelerationSensorD10(); 
			break;
		case MXC622X_DRVID:
			mLocalSensor = new AccelerationSensorMxc622x(); 
			break;
		case MMA8452Q_DRVID:
		case STK8312_DRVID:
		case KIONIX_DRVID:
			mLocalSensor = new AccelerationSensorGeneric();
			break;	
        case -1:
            if (true/*isRemoteEnable()*/)
                return mRemoteSensor;
            break;
            
	};
	return mLocalSensor;
}

SensorBase* sensors_poll_context_t::getLgtSensor()
{
	switch (get_lgt_drvid())
	{
		case ISL29023_DRVID:
			return (new LightSensor29023());
			break;
	};
	return NULL;
}

SensorBase* sensors_poll_context_t::getPxmSensor()
{
	switch (get_pxm_drvid())
	{
		case ISL29023_DRVID:
			return (new ProximitySensor());
			break;
	};
	return NULL;
}

SensorBase* sensors_poll_context_t::getGyroSensor()
{
	switch (get_gyro_drvid())
	{
		case L3G4200D_DRVID:
			return (new GyroSensorL3g4200d());
	}

	return NULL;
}

#if 1
SensorBase* sensors_poll_context_t::getMagSensor()
{
	switch (get_mag_drvid())
	{
		case MMC328x_DRVID:
			return (new MagneticSensor());
		//default :
			//return (new MagneticSensor());
	}
    
	return NULL;
}
#endif 
SensorBase* sensors_poll_context_t::getOreSensor()
{
	switch (get_ore_drvid())
	{
		case MMC328x_DRVID:
			return (new OrientationSensor());
			break;
		//default :
			//return (new OrientationSensor());
	};
	return NULL;
}
//*************************add end 2013-4-18*************
void *pthreadFn(void *arg)
{
#define CTL_FIFO "/data/misc/ctl_fifo"
#define DATA_FIFO "/data/misc/data_fifo"
#define AMP 1000000
	int fd = 0;
	int ret = 0;
	int ctl_fd=0, data_fd=0;
	int grav_buf[3] = {0};
	int ctl_buf[3] = {0};
	int test_buf[3] = {0};
	
	sensors_event_t data;
	sensors_poll_context_t *dev = (sensors_poll_context_t *)arg;
	if (!dev)
	{
		ALOGE("%s input param NULL", __FUNCTION__);
		pthread_exit((void *)"bad param");
		return NULL;	
	}
	//fd = dev->getPollfd(0);
	umask(0);
	ret = mkfifo(CTL_FIFO, 0666);
	if (ret)
	{
		ALOGD("fail to mkfifo %s:%s", CTL_FIFO, strerror(errno));
	}
	ret = mkfifo(DATA_FIFO, 0666);
	if (ret)
	{
		ALOGD("fail to mkfifo %s:%s", DATA_FIFO, strerror(errno));
	}
	
	ALOGD("begin to open fd !!");
	ctl_fd = open(CTL_FIFO, O_RDWR);
	if (ctl_fd < 0)
	{
		ALOGE("open %s fail :%s", CTL_FIFO, strerror(errno));
		pthread_exit((void *)"bad param");
		return NULL;
	}
	ALOGD("open ctl fd ok!");
	data_fd = open(DATA_FIFO, O_RDWR);
	if (data_fd < 0)
	{
		ALOGE("open %s fail :%s", DATA_FIFO, strerror(errno));
		pthread_exit((void *)"bad param");
		return NULL;
	}
	ALOGD("open ctl data fd, ok!!");
	prctl(PR_SET_NAME, (unsigned long)"Gsensor handle");
	//dev->activate(ID_A, 1);
	//dev->setDelay(ID_A, 0);
	//dev->activate(ID_A, 1);
	memset(&data, 0, sizeof(data)); 
	while (1){
		//ALOGD("wait for ctl...");
		read(ctl_fd, ctl_buf, (sizeof(ctl_buf)));
		dev->activate(ID_A, 1);
		//dev->setDelay(ID_A, 0);
		//ALOGD("get ctl %s", ctl_buf);
		dev->pollEventsIndex(0, &data, 1);
		//ALOGD("in %s fd %d", __FUNCTION__, fd);
		//ALOGD("x,y,z %f, %f, %f", data.acceleration.x, data.acceleration.y, data.acceleration.z);
		grav_buf[0] = int (data.acceleration.x * AMP);
		grav_buf[1] = int (data.acceleration.y * AMP);
		grav_buf[2] = int (data.acceleration.z * AMP);
		write(data_fd, (void *)grav_buf, sizeof(grav_buf));
		//ALOGD("x,y,z int %d %d %d", grav_buf[0], grav_buf[1], grav_buf[2]);
		//read(data_fd, (void *)test_buf, sizeof(test_buf));
		//ALOGD("read from data_fd: %d %d %d", test_buf[0], test_buf[1], test_buf[2]);
		//sleep(2);
	}
	pthread_exit((void *)"exit ok");	
	return NULL;
}
sensors_poll_context_t::sensors_poll_context_t()
{
	int pollIndex = 0;

    int accelerationIndex;
    int controlIndex;
    int listenIndex;
    int ret = 0;
    mSensors[acceleration] = getAccSensor();//new AccelerationSensor();
    if (mSensors[acceleration] != NULL)
    {
        accelerationIndex = pollIndex;
	    mPollFds[pollIndex].fd = mSensors[acceleration]->getFd();
	    mPollFds[pollIndex].events = POLLIN;
	    mPollFds[pollIndex].revents = 0;
	    mPollIndex[acceleration] = pollIndex;
	    pollIndex++;
    }

    mSensors[light] = getLgtSensor(); //new LightSensor();
    if (mSensors[light] != NULL)
    {
	    mPollFds[pollIndex].fd = mSensors[light]->getFd();
	    mPollFds[pollIndex].events = POLLIN;
	    mPollFds[pollIndex].revents = 0;
	    mPollIndex[light] = pollIndex;
	    pollIndex++;
    }
    mSensors[proximity] = getPxmSensor(); //new ProximitySensor();
    if (mSensors[proximity] != NULL)
    {
	    mPollFds[pollIndex].fd = mSensors[proximity]->getFd();
	    mPollFds[pollIndex].events = POLLIN;
	    mPollFds[pollIndex].revents = 0;
	    mPollIndex[proximity] = pollIndex;
	    pollIndex++;
    }
    //add by rambo for magnetic & orientation 2013-4-18
    mSensors[magnetic] = getMagSensor(); // new MagneticSensor()
    if (mSensors[magnetic] != NULL)
    {
    	mPollFds[pollIndex].fd = mSensors[magnetic]->getFd();
	    mPollFds[pollIndex].events = POLLIN;
	    mPollFds[pollIndex].revents = 0;
	    mPollIndex[magnetic] = pollIndex;// !!!!! proximity
	    pollIndex++;
    }
    mSensors[orientation] = getOreSensor(); // new OrientationSensor()
    if (mSensors[orientation] != NULL)
    {
	mPollFds[pollIndex].fd = mSensors[orientation]->getFd();
	mPollFds[pollIndex].events = POLLIN;
	mPollFds[pollIndex].revents = 0;
	mPollIndex[orientation] = pollIndex;
	pollIndex++;
	
	//and here create a thread, communicate with memsicd to send gsensor data 2013-7-19
	//pthreadFn((void *)this);
#if 1	
	pthread_attr_t attr;
	ret = pthread_attr_init(&attr);
	
	int policy = 0;
	int scope = 0;
	ret = pthread_attr_getschedpolicy(&attr, &policy);
	ALOGD("%s schedual policy is %d", __FUNCTION__, policy); // 0
	ALOGD("sched_RR :%d fifo %d, other %d", SCHED_RR, SCHED_FIFO, SCHED_OTHER);
#if 1 //if use default schedu policy, thread priority is so low.	
	ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
	ret = pthread_attr_getschedpolicy(&attr, &policy);
	ALOGD("%s schedual policy is %d", __FUNCTION__, policy); // 0
	
	struct sched_param param;
	ret = pthread_attr_getschedparam(&attr, &param);
	ALOGD("shced param %d", param.sched_priority); // default priority 0
	param.sched_priority = 85;
	ret = pthread_attr_setschedparam(&attr, &param);
#endif	
	scope = pthread_attr_getscope(&attr);
	ALOGD("pthread scope %d, PTHREAD_SCOPE_SYSTEM %d PROCESS %d", scope, \
	PTHREAD_SCOPE_SYSTEM, PTHREAD_SCOPE_PROCESS);
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&thread, /*NULL*/&attr, pthreadFn, (void *)this);
	if (ret != 0)
	{
		ALOGE("pthread_create error:%s!, compass can't get gsensor data!", strerror(errno));
	}
	ALOGD("pid %d, tid %d", getpid(), thread);	
	//sched_RR :2 fifo 1, other 0
#endif	
	
    }
/*
    mSensors[akm] = new AkmSensor();
    mPollFds[akm].fd = mSensors[akm]->getFd();
    mPollFds[akm].events = POLLIN;
    mPollFds[akm].revents = 0;

    mSensors[pressure] = new PressureSensor();
    mPollFds[pressure].fd = mSensors[pressure]->getFd();
    mPollFds[pressure].events = POLLIN;
    mPollFds[pressure].revents = 0;
*/

    mSensors[gyro] = getGyroSensor();
    if (mSensors[gyro] != NULL)
    {
	    mPollFds[pollIndex].fd = mSensors[gyro]->getFd();
	    mPollFds[pollIndex].events = POLLIN;
	    mPollFds[pollIndex].revents = 0;
	    mPollIndex[gyro] = pollIndex;
	    pollIndex++;
    }

    int wakeFds[2];
    int result = pipe(wakeFds);
    //LOGD_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    wake = pollIndex;
    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
    pollIndex++;

    controlIndex = pollIndex;
    mLocalControlFd = socket_local_server(REMOTE_SENSOR_CONTROL_SOCKET, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    fcntl(mLocalControlFd, F_SETFD, FD_CLOEXEC);  
    //LOGD_IF(mLocalControlFd<0, "error creating local server socket (%s)", strerror(errno));
    mPollFds[controlIndex].fd = mLocalControlFd;
    mPollFds[controlIndex].events = POLLIN;
    mPollFds[controlIndex].revents = 0;
    pollIndex++;
    
    listenIndex = pollIndex;
    mPollFds[listenIndex].fd = -1;
    mPollFds[listenIndex].events = POLLIN;
    mPollFds[listenIndex].revents = 0;
    pollIndex++;
    if (true/*isRemoteEnable()*/)
    {
        mRemoteSensor->setContextData(mPollFds, accelerationIndex, controlIndex, &mSensors[accelerationIndex]);
    }
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=1 ; i<numSensorDrivers ; i++) {
    	if (mSensors[i] != NULL)
    	{
        	delete mSensors[i];
        	mSensors[i] = NULL;
        }
    }
    if (mLocalSensor != NULL)
    {
        delete mLocalSensor;
        mLocalSensor = NULL;
    }
    if (mRemoteSensor != NULL)
    {
        delete mRemoteSensor;
        mRemoteSensor = NULL;
    }
    mSensors[acceleration] = NULL;
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
    close(mLocalControlFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err = -1;

	#if SENSOR_DEBUG
    ALOGD_IF(SENSOR_DEBUG,"[activate] index=%d\n", index);
    #endif
    if (mSensors[index] != NULL)
    {
    	err = mSensors[index]->enable(handle, enabled);
    }
    /*if (!err && handle == ID_O) {
        err = static_cast<AccelerationSensor*>(
                mSensors[acceleration])->enableOrientation(enabled);
    }*/
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGD_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;

	#if SENSOR_DEBUG
    ALOGD_IF(SENSOR_DEBUG,"[setDelay] index=%d\n", index);
    #endif
    if (mSensors[index] != NULL)
    {
    	return mSensors[index]->setDelay(handle, ns);
    } 
	return -1;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    struct timeval timeb, timea;
    long tb = 0, ta = 0;
    int nbEvents = 0;
    int n = 0;

	#if SENSOR_DEBUG
    ALOGD_IF(SENSOR_DEBUG, "pollEvents enter count=%d...\n",count);
    #endif 
    do {
    	if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            /*n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            if (n<0) {
                LOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }*/
	    if (debug_poll_time) {
		gettimeofday(&timeb, NULL);
		tb = timeb.tv_sec*1000 + timeb.tv_usec/1000;
	    }
	    
            do {
                n = poll(mPollFds, get_sensor_num()+3 /*numFds+2*/, nbEvents ? 0 : -1);
            } while (n < 0 && errno == EINTR);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGD_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGD_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // see if we have some leftover from the last poll()        
        for (int i=0 ; count && i<numSensorDrivers; i++) {
			ALOGD_IF(SENSOR_DEBUG, "poll i=%d...\n", i);
        	if (mSensors[i] != NULL)
        	{
	            SensorBase* const sensor(mSensors[i]);
	            //LOGD_IF(SENSOR_DEBUG, "pollEvents...\n");
	            if ((mPollFds[mPollIndex[i]].revents & POLLIN) || (sensor->hasPendingEvents())) 
	            {
	            	ALOGD_IF(SENSOR_DEBUG,"[pollEvents] sensor_i=%d\n", i);
	                int nb = sensor->readEvents(data, count);
			if (debug_poll_time) {
				gettimeofday(&timea, NULL);
				ta = timea.tv_sec*1000 + timea.tv_usec/1000;	
				ALOGW("<<<<<read %ld ms\n", ta-tb);
			}
	                if (nb < count) {
	                    // no more data for this sensor
	                    mPollFds[mPollIndex[i]].revents = 0;
	                }
	                count -= nb;
	                nbEvents += nb;
	                data += nb;
	            }/* else {
	            	LOGD_IF(1, "Can't find pollevnets...\n");
	            }*/
            }
        }
        // see if we have some leftover from the control socket and listen socket
        if (mRemoteSensor != NULL)
            mRemoteSensor->checkEvents();
        
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
    int status = -EINVAL;
    char buf[PROPERTY_VALUE_MAX] = {0};
    sensors_poll_context_t *dev = new sensors_poll_context_t();
    memset(&dev->device, 0, sizeof(sensors_poll_device_t));
    dev->debug_poll_time = 0;
    
    property_get("wmt.sensor.debug.poll", buf, "0");
    ALOGW("wmt.sensor.debug.poll %s", buf);
    if (strncmp(buf, "1", 1) == 0){
	dev->debug_poll_time = 1;
	ALOGW("debug sensor polling time!");
    }	
    else {
	dev->debug_poll_time = 0;
	ALOGW("closed debug sensor polling time!");
    }		
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;
    return status;
}

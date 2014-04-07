/*
 * Copyright 2011 WonderMedia Technologies, Inc. All Rights Reserved. 
 *  
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc. 
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any third party, 
 * in whole or in part, without prior written consent of WonderMedia. 
 *  
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 * WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, 
 * AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  
 */


#ifndef ANDROID_REMOTE_SENSOR_H
#define ANDROID_REMOTE_SENSOR_H


#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <utils/threads.h>
#include <cutils/properties.h>

#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"


class RemoteSensor : public SensorBase {

public:
	RemoteSensor(const char* dev_name, 
					 const char* data_name): SensorBase(dev_name, data_name){};
    virtual ~RemoteSensor(){};

    virtual void setContextData(pollfd* pollfds, int accIdx, int ctrlIdx, SensorBase** sensor) = 0;
    virtual void checkEvents() = 0;
};

RemoteSensor* createRemoteSensor();

/*****************************************************************************/

#endif  // ANDROID_REMOTE_SENSOR_H


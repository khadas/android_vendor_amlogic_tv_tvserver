/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CBlobDevice"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "CBlobDeviceE2prom.h"
#include "CBlobDevice.h"
#include "CTvLog.h"

CBlobDevice::CBlobDevice()
{
    m_dev_path[0] = '\0';
}

CBlobDevice::~CBlobDevice()
{
}


#define LOG_TAG "tvserver"

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


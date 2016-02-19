#define LOG_TAG "CBlobDeviceE2prom"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "CBlobDeviceE2prom.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"
#include "CTvLog.h"

CBlobDeviceE2prom::CBlobDeviceE2prom()
{
}

CBlobDeviceE2prom::~CBlobDeviceE2prom()
{
}

int CBlobDeviceE2prom::WriteBytes(int offset __unused, int size __unused, unsigned char *buf __unused)
{
    return 0;
}

int CBlobDeviceE2prom::ReadBytes(int offset __unused, int size __unused, unsigned char *buf __unused)
{
    return 0;
}

int CBlobDeviceE2prom::EraseAllData()
{
    return 0;
}

int CBlobDeviceE2prom::InitCheck()
{
    return 0;
}

int CBlobDeviceE2prom::OpenDevice()
{
    return 0;
}

int CBlobDeviceE2prom::CloseDevice()
{
    return 0;
}


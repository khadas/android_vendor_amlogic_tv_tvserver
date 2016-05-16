#define LOG_NDEBUG 0

#define LOG_TAG "tvserver"

#include <utils/Log.h>
#include <stdlib.h>
#include <string.h>
#include <tvutils.h>

#include "CTvGpio.h"


CTvGpio::CTvGpio()
{
    mGpioPinNum = 0;
    memset(mGpioName, 0, 64);
}

CTvGpio::~CTvGpio()
{
    if (mGpioPinNum > 0)
        tvWriteSysfs(GPIO_UNEXPORT, mGpioPinNum);
}

int CTvGpio::processCommand(const char *port_name, bool is_out, int edge)
{
    ALOGV("%s, port_name=[%s], is_out=[%d], edge=[%d], gpio_pin=[%d]", __FUNCTION__, port_name, is_out, edge, mGpioPinNum);
    if (strncmp(port_name, "GPIO", 4) != 0)
        return -1;

    char pin_value[10] = {0};
    if (strcmp(mGpioName, port_name) != 0 && tvWriteSysfs(GPIO_NAME_TO_PIN, port_name)) {
        strcpy(mGpioName, port_name);
        tvReadSysfs(GPIO_NAME_TO_PIN, pin_value);
        mGpioPinNum = atoi(pin_value);
    }
    ALOGV("%s, port_name=[%s], is_out=[%d], edge=[%d], gpio_pin=[%d]", __FUNCTION__, port_name, is_out, edge, mGpioPinNum);

    int ret = -1;
    if (mGpioPinNum > 0) {
        if (is_out) {
            ret = setGpioOutEdge(edge);
        } else {
            ret = getGpioInEdge();
        }
    }

    return ret;
}

int CTvGpio::setGpioOutEdge(int edge)
{
    ALOGD("%s, gpio_pin=[%d], edge=[%d]", __FUNCTION__, mGpioPinNum, edge);

    char direction[128] = {0};
    char value[128] = {0};
    if (tvWriteSysfs(GPIO_EXPORT, mGpioPinNum)) {
        GPIO_DIRECTION(direction, mGpioPinNum);
        GPIO_VALUE(value, mGpioPinNum);
        ALOGV("dirction path:[%s], value path:[%s]", direction, value);

        tvWriteSysfs(direction, "out");
        tvWriteSysfs(value, edge);
        return 0;
    }

    ALOGE("%s, FAILED...", __FUNCTION__);
    return -1;
}

int CTvGpio::getGpioInEdge()
{
    ALOGD("%s, gpio_pin=[%d]", __FUNCTION__, mGpioPinNum);

    char direction[128] = {0};
    char value[128] = {0};
    char edge[128] = {0};
    if (tvWriteSysfs(GPIO_EXPORT, mGpioPinNum)) {
        GPIO_DIRECTION(direction, mGpioPinNum);
        GPIO_VALUE(value, mGpioPinNum);
        ALOGV("dirction path:[%s], value path:[%s]", direction, value);

        tvWriteSysfs(direction, "in");
        tvReadSysfs(value, edge);
        ALOGD("edge:[%s]", edge);

        return atoi(edge);
    }

    ALOGE("%s, FAILED...", __FUNCTION__);
    return -1;
}


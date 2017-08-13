/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef TV_SETTING_DEVICE_FACTORY
#define TV_SETTING_DEVICE_FACTORY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "CBlobDevice.h"

#include <tvconfig.h>

#include "../tvin/CTvin.h"

class CTvSettingDeviceFactory {
public:
    CTvSettingDeviceFactory();
    ~CTvSettingDeviceFactory();
    CBlobDevice *getSaveDeviceFromConfigFile();
private:
    CBlobDevice *mpCurDevice;
};
#endif

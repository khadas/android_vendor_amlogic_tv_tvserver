#ifndef TV_SETTING_DEVICE_FACTORY
#define TV_SETTING_DEVICE_FACTORY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "CBlobDevice.h"

#include <tvconfig.h>

#include "CTvSettingCfg.h"
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
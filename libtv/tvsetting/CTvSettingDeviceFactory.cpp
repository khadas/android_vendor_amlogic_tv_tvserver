#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvSettingDeviceFactory"
//#define LOG_NDEBUG 0

#include "CTvSettingDeviceFactory.h"
#include "CBlobDeviceE2prom.h"
#include "CBlobDeviceFile.h"

#include <tvconfig.h>

#include "CTvLog.h"
#include "CTvSettingCfg.h"

CTvSettingDeviceFactory::CTvSettingDeviceFactory()
{
    mpCurDevice = NULL;
}

CTvSettingDeviceFactory::~CTvSettingDeviceFactory()
{
    if (mpCurDevice != NULL) {
        delete mpCurDevice;
        mpCurDevice = NULL;
    }
}

CBlobDevice *CTvSettingDeviceFactory::getSaveDeviceFromConfigFile()
{
    const char *device_type = config_get_str(CFG_SECTION_SETTING, "store.device.type", "file");
    const char *device_path = config_get_str(CFG_SECTION_SETTING, "device_path", "/param/default_data");
    const char *device_size = config_get_str(CFG_SECTION_SETTING, "device_size", "0x1000");
    LOGD("getSaveDeviceFromConfigFile type=%s path=%s size=%s", device_type, device_path, device_size);

    if (mpCurDevice != NULL) delete mpCurDevice;

    if (strcmp(device_type, "file") == 0) {
        mpCurDevice = new CBlobDeviceFile();
    } else if (strcmp(device_type, "e2prom") == 0) {
    } else if (strcmp(device_type, "ram") == 0) {
    }

    return mpCurDevice;
}

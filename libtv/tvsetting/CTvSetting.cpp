#define LOG_TAG "TvSetting"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cutils/properties.h>

#include <netinet/ether.h>
#include <netinet/if_ether.h>

#include <netutils/ifc.h>
#include <netutils/dhcp.h>

#include "CTvSetting.h"

#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"

#include "../tv/CTvLog.h"
#define CC_DEF_CHARACTER_CHAR_VAL                   (0x8A)

pthread_mutex_t ssm_r_w_op_mutex = PTHREAD_MUTEX_INITIALIZER;

/************************ Start APIs For UI ************************/

CTvSettingDeviceFactory *mpSettingDeviceFactory = NULL;
CBlobDevice *mpCurDevice = NULL;

bool CTvSettingLoad()
{
    mpSettingDeviceFactory = new CTvSettingDeviceFactory();
    mpCurDevice = mpSettingDeviceFactory->getSaveDeviceFromConfigFile();
    if (mpCurDevice == NULL) {
        LOGD("%s, CTvSettingLoad = NULL", CFG_SECTION_TV);
        return false;
    } else {
        mpCurDevice->OpenDevice();
    }
    return true;
}

bool CTvSettingunLoad()
{
    if (mpSettingDeviceFactory != NULL) {
        delete mpSettingDeviceFactory;
        mpSettingDeviceFactory = NULL;
    }
    return true;
}

int CTvSettingdoSuspend()
{
    return mpCurDevice->CloseDevice();
}

int CTvSettingdoResume()
{
    return mpCurDevice->OpenDevice();
}
template<typename T>
static int SSMWriteNTypes(int offset, int data_len, T *data_buf)
{
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (mpCurDevice == NULL) {
        LOGE("%s, ssm_device is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (mpCurDevice->WriteBytes(offset, data_len * sizeof(T),
                                (unsigned char *) data_buf) < 0) {
        LOGE("%s, device WriteNBytes error.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}

template<typename T>
static int SSMReadNTypes(int offset, int data_len, T *data_buf)
{
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (mpCurDevice == NULL) {
        LOGE("%s, ssm_device is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (mpCurDevice->ReadBytes(offset, data_len * sizeof(T),
                               (unsigned char *) data_buf) < 0) {
        LOGE("%s, device ReadNBytes error.\n", CFG_SECTION_TV);
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}
int SSMSaveFlash_One_N310_N311(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;
    LOGD ( "~~~ SSMSaveFlash_One ~~~##offset %d##rw_val %d##" , offset, rw_val);

    return SSMWriteNTypes(offset, 1, &tmp_val);
}

int SSMReadFlash_One_N310_N311(int offset)
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(offset, 1, &tmp_val) < 0) {
        return -1;
    }
    LOGD ( "~~~ SSMReadFlash_One ~~~##offset %d##rw_val %d##" , offset, tmp_val);

    return tmp_val;
}

int SSMSaveFlash_N_N310_N311(int offset, int data_len, int *data_buf)
{
    int i = 0;
    unsigned char *ptr = NULL;

    ptr = new unsigned char[data_len];

    if (ptr != NULL) {
        for (i = 0; i < data_len; i++) {
            ptr[i] = data_buf[i];
        }
    } else {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    if (SSMWriteNTypes(offset, data_len, ptr) < 0) {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    delete ptr;
    ptr = NULL;

    return 0;
}

int SSMReadFlash_N_N310_N311(int offset, int data_len, int *data_buf)
{
    int i = 0;
    unsigned char *ptr = NULL;

    ptr = new unsigned char[data_len];

    if (ptr != NULL) {
        if (SSMReadNTypes(offset, data_len, ptr) < 0) {
            delete ptr;
            ptr = NULL;

            return -1;
        }
    } else {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    for (i = 0; i < data_len; i++) {
        data_buf[i] = ptr[i];
    }

    delete ptr;
    ptr = NULL;

    return 0;
}
int EEPWriteOneByte(int offset, unsigned char *data_buf)
{
    int fd = 0;
    const char *device_path = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.path", "/sys/devices/i2c-2/2-0050/eeprom");
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    LOGD ( "~~~EEPWriteOneByte~~~##offset %d##rw_val %s##" , offset, data_buf);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    fd = open(device_path, O_RDWR);

    if (fd < 0) {
        LOGE("%s, ####i2c test device open failed####.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    lseek(fd, offset, SEEK_SET);

    if (write(fd, data_buf, 1) < 0) {
        LOGE("%s, device WriteOneBytes error.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    close(fd);

    LOGE("%s, device WriteOneBytes OK.\n", CFG_SECTION_TV);

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}

int EEPReadOneByte(int offset , unsigned char *data_buf)
{
    int fd = 0;
    //const char* device_type = config_get_str(CFG_SECTION_SETTING, "peripheral.eeprom.device", "disable");
    const char *device_path = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.path", "/sys/devices/i2c-2/2-0050/eeprom");

    pthread_mutex_lock(&ssm_r_w_op_mutex);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    fd = open(device_path, O_RDWR);

    if (fd < 0) {
        LOGE("%s, ssm_device is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }
    lseek(fd, offset, SEEK_SET);

    if (read(fd, data_buf, 1) < 0) {
        LOGE("%s, device ReadOneBytes error.\n", CFG_SECTION_TV);
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    close(fd);

    LOGD ( "~~~EEPReadOneByte~~~##offset %d##rw_val %s##" , offset, data_buf);

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}

int EEPWriteNByte(int offset, int data_len, unsigned char *data_buf)
{
    int fd = 0;
    const char *device_path = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.path", "/sys/devices/i2c-2/2-0050/eeprom");
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    LOGD ( "~~~EEPWriteNByte~~~##offset %d##data_len %d##" , offset, data_len);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    fd = open(device_path, O_RDWR);

    if (fd < 0) {
        LOGE("%s, ####i2c test device open failed####.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    lseek(fd, offset, SEEK_SET);

    if (write(fd, data_buf, data_len) < 0) {
        LOGE("%s, device WriteNBytes error.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    close(fd);

    LOGE("%s, device WriteNBytes OK.\n", CFG_SECTION_TV);

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}
int EEPReadNByte(int offset, int data_len, unsigned char *data_buf)
{
    int fd = 0;
    const char *device_path = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.path", "/sys/devices/i2c-2/2-0050/eeprom");
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    if (data_buf == NULL) {
        LOGE("%s, data_buf is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    fd = open(device_path, O_RDWR);

    if (fd < 0) {
        LOGE("%s, ssm_device is NULL.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }
    lseek(fd, offset, SEEK_SET);

    if (read(fd, data_buf, data_len) < 0) {
        LOGE("%s, device ReadNBytes error.\n", CFG_SECTION_TV);

        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    close(fd);

    LOGD ( "~~~EEPReadNByte~~~##offset %d##data_len %d##" , offset, data_len);

    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}


int SSMSaveEEP_One_N310_N311(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;
    const char *device_config = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.device", "disable");

    if (strcmp(device_config, "enable") != 0) {
        LOGD ( "~~~ SSMSaveEEP_One ~~~##peripheral.eeprom.device error##");
        return -1;
    }
    LOGD ( "~~~SSMSaveEEP_One~~~##offset %d##rw_val %d##" , offset, rw_val);

    return EEPWriteOneByte(offset, &tmp_val);
}

int SSMReadEEP_One_N310_N311(int offset)
{
    unsigned char tmp_val = 0;
    const char *device_config = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.device", "disable");

    if (strcmp(device_config, "enable") != 0) {
        LOGD ( "~~~ SSMReadEEP_One ~~~##peripheral.eeprom.device error##");
        return -1;
    }

    if (EEPReadOneByte(offset, &tmp_val) < 0) {
        return -1;
    }
    LOGD ( "~~~SSMReadEEP_One~~~##offset %d##rw_val %d##" , offset, tmp_val);

    return tmp_val;
}

int SSMSaveEEP_N_N310_N311(int offset, int data_len, int *data_buf)
{
    int i = 0;
    unsigned char *ptr = NULL;
    const char *device_config = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.device", "disable");

    if (strcmp(device_config, "enable") != 0) {
        LOGD ( "~~~ SSMSaveEEP_N ~~~##peripheral.eeprom.device error##");
        return -1;
    }

    ptr = new unsigned char[data_len];

    if (ptr != NULL) {
        for (i = 0; i < data_len; i++) {
            ptr[i] = data_buf[i];
        }
    } else {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    if (EEPWriteNByte(offset, data_len, ptr) < 0) {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    delete ptr;
    ptr = NULL;

    return 0;
}

int SSMReadEEP_N_N310_N311(int offset, int data_len, int *data_buf)
{
    int i = 0;
    unsigned char *ptr = NULL;
    const char *device_config = config_get_str(CFG_SECTION_TV, "peripheral.eeprom.device", "disable");

    if (strcmp(device_config, "enable") != 0) {
        LOGD ( "~~~ SSMReadEEP_N ~~~##peripheral.eeprom.device error##");
        return -1;
    }
    ptr = new unsigned char[data_len];

    if (ptr != NULL) {
        if (EEPReadNByte(offset, data_len, ptr) < 0) {
            delete ptr;
            ptr = NULL;

            return -1;
        }
    } else {
        delete ptr;
        ptr = NULL;

        return -1;
    }

    for (i = 0; i < data_len; i++) {
        data_buf[i] = ptr[i];
    }

    delete ptr;
    ptr = NULL;

    return 0;
}
/************************ Start APIs For UI ************************/
int MiscSSMRestoreDefault()
{
    SSMSaveFactoryBurnMode(0);
    SSMSavePowerOnOffChannel(1);
    SSMSaveSystemLanguage(0);
    SSMSaveAgingMode(0);
    SSMSavePanelType(0);
    SSMSavePowerOnMusicSwitch(0);
    SSMSavePowerOnMusicVolume(20);
    SSMSaveSystemSleepTimer(0xFFFFFFFF);
    SSMSaveInputSourceParentalControl(0, 0);
    SSMSaveParentalControlSwitch(0);
    SSMSaveSerialCMDSwitchValue(0);
    SSMSaveBlackoutEnable(0);
    return 0;
}

int MiscSSMFacRestoreDefault()
{
    SSMSaveSystemLanguage(0);
    SSMSavePowerOnMusicSwitch(1);
    SSMSavePowerOnMusicVolume(20);
    SSMSaveSystemSleepTimer(0xFFFFFFFF);
    SSMSaveInputSourceParentalControl(0, 0);
    SSMSaveParentalControlSwitch(0);
    SSMSaveSearchNavigateFlag(1);
    SSMSaveInputNumLimit(2);
    SSMSaveLocalDimingOnOffFlg(0);

    return 0;
}

int ReservedSSMRestoreDefault()
{
    SSMSaveBurnWriteCharaterChar(CC_DEF_CHARACTER_CHAR_VAL);

    return 0;
}

int SSMSaveBurnWriteCharaterChar(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RSV_W_CHARACTER_CHAR_START, 1, &tmp_val);
}

int SSMReadBurnWriteCharaterChar()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RSV_W_CHARACTER_CHAR_START, 1, &tmp_val) < 0) {
        return -1;
    }

    return tmp_val;
}

int SSMSaveFactoryBurnMode(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_FBMF_START, 1, &tmp_val);
}

int SSMReadFactoryBurnMode()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_FBMF_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val != 0) {
        return 1;
    }

    return 0;
}

int SSMSavePowerOnOffChannel(int rw_val)
{
    unsigned char tmp_val = rw_val;
    return SSMWriteNTypes(SSM_RW_POWER_CHANNEL_START, 1, &tmp_val);
}

int SSMReadPowerOnOffChannel()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_POWER_CHANNEL_START, 1, &tmp_val) < 0) {
        return 0;
    }
    return tmp_val;
}

int SSMSaveLastSelectSourceInput(int rw_val)
{
    unsigned char tmp_val = rw_val;
    return SSMWriteNTypes(SSM_RW_LAST_SOURCE_INPUT_START, 1, &tmp_val);
}

int SSMReadLastSelectSourceInput()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_LAST_SOURCE_INPUT_START, 1, &tmp_val) < 0) {
        return 0;
    }
    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveSystemLanguage(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_SYS_LANGUAGE_START, 1, &tmp_val);
}

int SSMReadSystemLanguage()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_SYS_LANGUAGE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveAgingMode(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_AGING_MODE_START, 1, &tmp_val);
}

int SSMReadAgingMode()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_AGING_MODE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSavePanelType(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_PANEL_TYPE_START, 1, &tmp_val);
}

int SSMReadPanelType()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_PANEL_TYPE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSavePowerOnMusicSwitch(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_POWER_ON_MUSIC_SWITCH_START, 1, &tmp_val);
}

int SSMReadPowerOnMusicSwitch()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_POWER_ON_MUSIC_SWITCH_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSavePowerOnMusicVolume(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_POWER_ON_MUSIC_VOL_START, 1, &tmp_val);
}

int SSMReadPowerOnMusicVolume()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_POWER_ON_MUSIC_VOL_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveSystemSleepTimer(int rw_val)
{
    return SSMWriteNTypes(SSM_RW_SYS_SLEEP_TIMER_START, 1, &rw_val);
}

int SSMReadSystemSleepTimer()
{
    int tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_SYS_SLEEP_TIMER_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val < 0) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveInputSourceParentalControl(int source_index,
                                      unsigned char ctl_flag)
{
    int tmp_val = 0;

    if (source_index < 0 || source_index > 31) {
        return -1;
    }

    if (ctl_flag != 0 && ctl_flag != 1) {
        return -1;
    }

    if (SSMReadNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4,
                      (unsigned char *) &tmp_val) < 0) {
        return -1;
    }

    tmp_val = (tmp_val & (~(1 << source_index))) | (ctl_flag << source_index);

    return SSMWriteNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4,
                          (unsigned char *) &tmp_val);
}

int SSMReadInputSourceParentalControl(int source_index)
{
    int tmp_val = 0;

    if (SSMReadParentalControlSwitch() == 0) {
        return 0;
    }

    if (SSMReadNTypes(SSM_RW_INPUT_SRC_PARENTAL_CTL_START, 4,
                      (unsigned char *) &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val & (1 << source_index)) {
        return 1;
    }

    return 0;
}

int SSMSaveParentalControlSwitch(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_PARENTAL_CTL_SWITCH_START, 1, &tmp_val);
}

int SSMReadParentalControlSwitch()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_PARENTAL_CTL_SWITCH_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val != 0) {
        tmp_val = 1;
    }

    return tmp_val;
}

int SSMGetCustomerDataStart()
{
    return SSM_RW_CUSTOMER_DATA_START;
}

int SSMGetCustomerDataLen()
{
    return SSM_RW_CUSTOMER_DATA_LEN;
}

int SSMGetATVDataStart()
{
    return SSM_RW_ATV_START;
}

int SSMGetATVDataLen()
{
    return SSM_RW_ATV_LEN;
}

int SSMGetVPPDataStart()
{
    return SSM_RW_VPP_START;
}

int SSMGetVPPDataLen()
{
    return SSM_RW_VPP_LEN;
}

int SSMSaveSearchNavigateFlag(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_SEARCH_NAVIGATE_FLAG_START, 1, &tmp_val);
}

int SSMReadSearchNavigateFlag()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_SEARCH_NAVIGATE_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveInputNumLimit(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_INPUT_NUMBER_LIMIT_START, 1, &tmp_val);
}

int SSMReadInputNumLimit()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_INPUT_NUMBER_LIMIT_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveLocalDimingOnOffFlg(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_LOCAL_DIMING_START, 1, &tmp_val);
}

int SSMReadLocalDimingOnOffFlg()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_LOCAL_DIMING_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveVDac2DValue(unsigned short rw_val)
{
    return SSMWriteNTypes(SSM_RW_VDAC_2D_START, 1, &rw_val);
}

int SSMReadVDac2DValue()
{
    unsigned short tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_VDAC_2D_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveVDac3DValue(unsigned short rw_val)
{
    return SSMWriteNTypes(SSM_RW_VDAC_3D_START, 1, &rw_val);
}

int SSMReadVDac3DValue()
{
    unsigned short tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_VDAC_3D_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveChromaStatus(int mode)
{
    int fd = -1, ret = -1;
    char value[20] = "";

    sprintf(value, "%d", mode);

    fd = open("/sys/class/tvafe/tvafe0/cvd_reg8a", O_RDWR);

    if (fd < 0) {
        LOGE("open /sys/class/tvafe/tvafe0/cvd_reg8a ERROR(%s)!!\n",
             strerror(errno));
        return -1;
    }

    ret = write(fd, value, strlen(value));

    close(fd);

    return ret;
}

int SSMSaveNonStandardValue(unsigned short rw_val)
{
    int i = 0, tmp_ret = 0;
    unsigned char data[] = { 0, 0 };

    {
        data[0] = (unsigned char) rw_val;
        rw_val >>= 8;
        data[1] = (unsigned char) rw_val;
    }

    LOGD("%s, save NonStandard_value = %d", CFG_SECTION_TV, rw_val);

    return SSMWriteNTypes(SSM_RW_NON_STANDARD_START, 2, data);
}

int SSMReadNonStandardValue(void)
{
    int i = 0, value = 0;
    int data[] = { 0, 0 };

    if (SSMReadNTypes(SSM_RW_NON_STANDARD_START, 2, data) < 0) {
        LOGE("%s, read NonStandard_value error.", CFG_SECTION_TV);
        return 0;
    }

    {
        value += data[1];
        value <<= 8;
        value += data[0];
    }

    LOGD("%s, read NonStandard_value = %d.", CFG_SECTION_TV, value);

    return value;
}

int SSMSaveAdbSwitchValue(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_ADB_SWITCH_START, 1, &tmp_val);
}

int SSMReadAdbSwitchValue(void)
{
    unsigned char switch_val = 0;

    if (SSMReadNTypes(SSM_RW_ADB_SWITCH_START, 1, &switch_val) < 0) {
        LOGD("%s, read switch value error", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, read switch value = %d", CFG_SECTION_TV, switch_val);

    return switch_val;
}

int SSMSaveSerialCMDSwitchValue(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_SERIAL_CMD_SWITCH_START, 1, &tmp_val);
}

int SSMReadSerialCMDSwitchValue(void)
{
    unsigned char switch_val = 0;

    if (SSMReadNTypes(SSM_RW_SERIAL_CMD_SWITCH_START, 1, &switch_val) < 0) {
        LOGD("%s, read switch value error", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, read switch value = %d", CFG_SECTION_TV, switch_val);

    return switch_val;
}

int SSMSaveNoiseGateThresholdValue(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_NOISE_GATE_THRESHOLD_START, 1, &tmp_val);
}

int SSMReadNoiseGateThresholdValue(void)
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_NOISE_GATE_THRESHOLD_START, 1, &tmp_val) < 0) {
        LOGD("%s, read NoiseGateThreshold error", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, read NoiseGateThreshold = %d", CFG_SECTION_TV, tmp_val);

    return tmp_val;
}

int SSMSaveGraphyBacklight(int rw_val)
{
    unsigned char tmp_val = rw_val;

    if (rw_val < 0 || rw_val > 100) {
        return -1;
    }

    return SSMWriteNTypes(SSM_RW_UI_GRHPHY_BACKLIGHT_START, 1, &tmp_val);
}

int SSMReadGraphyBacklight(void)
{
    unsigned char value = 0;

    if (SSMReadNTypes(SSM_RW_UI_GRHPHY_BACKLIGHT_START, 1, &value) < 0) {
        LOGD("%s, read graphybacklight error.\n", CFG_SECTION_TV);
        return -1;
    }

    if (/*value < 0 || */value > 100) {
        LOGD("%s, range of graphybacklight (%d) is not between 0-100.\n",
             CFG_SECTION_TV, value);
        return -1;
    }

    return value;
}

int SSMSaveFastSuspendFlag(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_FASTSUSPEND_FLAG_START, 1, &tmp_val);
}

int SSMReadFastSuspendFlag(void)
{
    unsigned char value = 0;

    if (SSMReadNTypes(SSM_RW_FASTSUSPEND_FLAG_START, 1, &value) < 0) {
        LOGD("%s, read FastSuspendFlag error.\n", CFG_SECTION_TV);
        return -1;
    }

    return value;
}

int SSMSaveCABufferSizeValue(unsigned short rw_val)
{
    int i = 0, tmp_ret = 0;
    unsigned char data[] = { 0, 0 };

    {
        data[0] = (unsigned char) rw_val;
        rw_val >>= 8;
        data[1] = (unsigned char) rw_val;
    }

    return SSMWriteNTypes(SSM_RW_CA_BUFFER_SIZE_START, 2, data);
}

int SSMReadCABufferSizeValue(void)
{
    int i = 0, value = 0;
    unsigned char data[] = { 0, 0 };

    if (SSMReadNTypes(SSM_RW_CA_BUFFER_SIZE_START, 2, data) < 0) {
        LOGE("%s, read ca_buffer_size error", CFG_SECTION_TV);
        return 0;
    }

    {
        value += data[1];
        value <<= 8;
        value += data[0];
    }

    return value;
}

int SSMSaveStandbyMode(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_STANDBY_MODE_FLAG_START, 1, &tmp_val);
}

int SSMReadStandbyMode()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_STANDBY_MODE_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveHDMIEQMode(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_HDMIEQ_MODE_START, 1, &tmp_val);
}

int SSMReadHDMIEQMode()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_HDMIEQ_MODE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveLogoOnOffFlag(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_LOGO_ON_OFF_FLAG_START, 1, &tmp_val);
}

int SSMReadLogoOnOffFlag()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_LOGO_ON_OFF_FLAG_START, 1, &tmp_val) < 0) {
        return 0;
    }

    return tmp_val;
}

int SSMSaveHDMIInternalMode(unsigned int rw_val)
{
    int i = 0, tmp_ret = 0;
    unsigned char data[] = { 0, 0, 0, 0 };

    for (i = 3; i >= 0; i--) {
        data[i] = (unsigned char) rw_val;
        rw_val >>= 8;
    }

    return SSMWriteNTypes(SSM_RW_HDMIINTERNAL_MODE_START, 4, data);
}

int SSMReadHDMIInternalMode()
{
    int i = 0, value = 0;
    int data[] = { 0, 0, 0, 0 };

    if (SSMReadNTypes(SSM_RW_HDMIINTERNAL_MODE_START, 4, data) < 0) {
        return 0;
    }

    for (i = 0; i < 4; i++) {
        value <<= 8;
        value += data[i];
    }

    return value;
}

int SSMSaveParentalControlPassWord(unsigned char *password, int size)
{
    return SSMWriteNTypes(SSM_RW_PARENTAL_CTL_PASSWORD_START, size, password);
}

int SSMReadParentalControlPassWord(unsigned short  *password)
{
    if (SSMReadNTypes(SSM_RW_PARENTAL_CTL_PASSWORD_START,
                      SSM_RW_PARENTAL_CTL_PASSWORD_LEN, (unsigned char *) password)
            < 0) {
        return -1;
    }
    return 0;
}

int SSMSaveDisable3D(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_DISABLE_3D_START, 1, &tmp_val);
}

int SSMReadDisable3D()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_DISABLE_3D_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveGlobalOgoEnable(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_GLOBAL_OGO_ENABLE_START, 1, &tmp_val);
}

int SSMReadGlobalOgoEnable()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(SSM_RW_GLOBAL_OGO_ENABLE_START, 1, &tmp_val) < 0) {
        return 0;
    }

    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMDeviceMarkCheck()
{
    int i = 0, failed_count = 0;
    int mark_offset[3] = { 0, 0, 0 };
    unsigned char mark_values[3] = { 0, 0, 0 };
    unsigned char tmp_ch = 0;

    //read temp one byte
    SSMReadNTypes(0, 1, &tmp_ch);

    mark_offset[0] = SSM_MARK_01_START;
    mark_offset[1] = SSM_MARK_02_START;
    mark_offset[2] = SSM_MARK_03_START;

    mark_values[0] = SSM_MARK_01_VALUE;
    mark_values[1] = SSM_MARK_02_VALUE;
    mark_values[2] = SSM_MARK_03_VALUE;

    if (SSMReadBurnWriteCharaterChar() != CC_DEF_CHARACTER_CHAR_VAL) {
        SSMSaveBurnWriteCharaterChar(CC_DEF_CHARACTER_CHAR_VAL);
    }

    failed_count = 0;
    for (i = 0; i < 3; i++) {
        tmp_ch = 0;
        if (SSMReadNTypes(mark_offset[i], 1, &tmp_ch) < 0) {
            LOGE("%s, SSMDeviceMarkCheck Read Mark failed!!!\n", CFG_SECTION_TV);
            break;
        }

        if (tmp_ch != mark_values[i]) {
            failed_count += 1;
            LOGE(
                "%s, SSMDeviceMarkCheck Mark[%d]'s offset = %d, Mark[%d]'s Value = %d, read value = %d.\n",
                CFG_SECTION_TV, i, mark_offset[i], i, mark_values[i], tmp_ch);
        }
    }

    if (failed_count >= 3) {
        return -1;
    }

    return 0;
}

int SSMRestoreDeviceMarkValues()
{
    int i;
    int mark_offset[3] = {
        (int) SSM_MARK_01_START, //
        (int) SSM_MARK_02_START, //
        (int) SSM_MARK_03_START, //
    };

    unsigned char mark_values[3] = {
        //
        (unsigned char) SSM_MARK_01_VALUE, (unsigned char) SSM_MARK_02_VALUE,
        (unsigned char) SSM_MARK_03_VALUE, //
    };

    for (i = 0; i < 3; i++) {
        if (SSMWriteNTypes(mark_offset[i], 1, &(mark_values[i])) < 0) {
            LOGD("SSMRestoreDeviceMarkValues Write Mark failed.\n");
            break;
        }
    }

    if (i < 3) {
        return -1;
    }

    return 0;
}

static int SSMGetPreCopyingEnableCfg()
{
    const char *prop_value;

    prop_value = config_get_str(CFG_SECTION_TV, "ssm.precopying.en", "null");
    if (strcmp(prop_value, "null") == 0 || strcmp(prop_value, "0") == 0
            || strcmp(prop_value, "disable") == 0) {
        return 0;
    }

    return 1;
}

static int SSMGetPreCopyingDevicePathCfg(char dev_path[])
{
    const char *prop_value;

    if (dev_path == NULL) {
        return -1;
    }

    prop_value = config_get_str(CFG_SECTION_TV, "ssm.precopying.devpath", "null");
    if (strcmp(prop_value, "null") == 0) {
        return 1;
    }

    strcpy(dev_path, prop_value);

    return 0;
}

static unsigned char gTempDataBuf[4096] = { 0 };
int SSMHandlePreCopying()
{
    int device_fd = -1;
    int i = 0, tmp_size = 0;
    unsigned char tmp_ch = 0;
    char tmpPreCopyingDevicePath[256] = { '\0' };

    if (SSMGetPreCopyingEnableCfg() == 0) {
        LOGD("%s, Pre copying is disable now.\n", CFG_SECTION_TV);
        return 0;
    }

    //read temp one byte
    SSMReadNTypes(0, 1, &tmp_ch);

    SSMGetPreCopyingDevicePathCfg(tmpPreCopyingDevicePath);

    device_fd = open(tmpPreCopyingDevicePath, O_RDONLY);
    if (device_fd < 0) {
        LOGE("%s, Open device file \"%s\" error: %s.\n", CFG_SECTION_TV,
             tmpPreCopyingDevicePath, strerror(errno));
        return -1;
    }

    tmp_size = lseek(device_fd, 0, SEEK_END);
    if (tmp_size == 4096) {
        lseek(device_fd, 0, SEEK_SET);
        read(device_fd, gTempDataBuf, tmp_size);

        SSMWriteNTypes(0, tmp_size, gTempDataBuf);
    }

    close(device_fd);

    remove(tmpPreCopyingDevicePath);

    return 0;
}

int SSMSaveDTVType(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(SSM_RW_DTV_TYPE_START, 1, &tmp_val);
}

int SSMReadDTVType(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(SSM_RW_DTV_TYPE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

/************************ End APIs For UI ************************/

// other api
int GetSSMCfgBufferData(const char *key_str, int *buf_item_count, int radix,
                        unsigned char data_buf[])
{
    int cfg_item_count = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *config_value;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    config_value = config_get_str(CFG_SECTION_TV, key_str, "null");
    if (strcasecmp(config_value, "null") == 0) {
        LOGE("%s, can't get config \"%s\"!!!\n", CFG_SECTION_TV, key_str);
        return -1;
    }

    cfg_item_count = 0;

    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);

    char *pSave;
    token = strtok_r(data_str, strDelimit, &pSave);
    while (token != NULL) {
        if (cfg_item_count < *buf_item_count) {
            data_buf[cfg_item_count] = strtol(token, NULL, radix);

            token = strtok_r(NULL, strDelimit, &pSave);
            cfg_item_count += 1;
        } else {
            LOGE("%s, we get data count more than desire count (%d)!!!\n",
                 CFG_SECTION_TV, *buf_item_count);
            return -1;
        }
    }

    *buf_item_count = cfg_item_count;

    return 0;
}

int SSMSaveSourceInput(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_POS_SOURCE_INPUT_START, 1, &rw_val);
}

int SSMReadSourceInput()
{
    unsigned char tmp_val = 0;

    if (SSMReadNTypes(TVIN_DATA_POS_SOURCE_INPUT_START, 1, &tmp_val) < 0) {
        return 0;
    }
    if (tmp_val == CBlobDevice::CC_INIT_BYTE_VAL) {
        tmp_val = 0;
    }

    return tmp_val;
}

int SSMSaveCVBSStd(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_CVBS_STD_START, 1, &rw_val);
}

int SSMReadCVBSStd(unsigned char *rw_val)
{
    return SSMReadNTypes(TVIN_DATA_CVBS_STD_START, 1, rw_val);
}

int SSMSave3DMode(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_POS_3D_MODE_START, 1, &rw_val);
}

int SSMRead3DMode(unsigned char *rw_val)
{
    return SSMReadNTypes(TVIN_DATA_POS_3D_MODE_START, 1, rw_val);
}

int SSMSave3DLRSwitch(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_POS_3D_LRSWITCH_START, 1, &rw_val);
}

int SSMRead3DLRSwitch(unsigned char *rw_val)
{
    return SSMReadNTypes(TVIN_DATA_POS_3D_LRSWITCH_START, 1, rw_val);
}

int SSMSave3DDepth(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_POS_3D_DEPTH_START, 1, &rw_val);
}

int SSMRead3DDepth(unsigned char *rw_val)
{
    return SSMReadNTypes(TVIN_DATA_POS_3D_DEPTH_START, 1, rw_val);
}

int SSMSave3DTO2D(unsigned char rw_val)
{
    return SSMWriteNTypes(TVIN_DATA_POS_3D_TO2D_START, 1, &rw_val);
}

int SSMRead3DTO2D(unsigned char *rw_val)
{
    return SSMReadNTypes(TVIN_DATA_POS_3D_TO2D_START, 1, rw_val);
}

int SSMSaveBrightness(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_BRIGHTNESS_START + offset, 1, &tmp_val);
}

int SSMReadBrightness(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_BRIGHTNESS_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveContrast(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_CONTRAST_START + offset, 1, &tmp_val);
}

int SSMReadContrast(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_CONTRAST_START + offset, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveSaturation(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_SATURATION_START + offset, 1, &tmp_val);
}

int SSMReadSaturation(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_SATURATION_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveHue(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_HUE_START + offset, 1, &tmp_val);
}

int SSMReadHue(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_HUE_START + offset, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveSharpness(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_SHARPNESS_START + offset, 1, &tmp_val);
}

int SSMReadSharpness(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_SHARPNESS_START + offset, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveSceneMode(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_SCENE_MODE_START, 1, &tmp_val);
}

int SSMReadSceneMode(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_SCENE_MODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSavePictureMode(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_PICTURE_MODE_START + offset, 1, &tmp_val);
}

int SSMReadPictureMode(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_PICTURE_MODE_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveColorTemperature(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_COLOR_TEMP_START + offset, 1, &tmp_val);
}

int SSMReadColorTemperature(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_COLOR_TEMP_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveNoiseReduction(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_NOISE_REDUCTION_START + offset, 1,
                          &tmp_val);
}

int SSMReadNoiseReduction(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_NOISE_REDUCTION_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveDisplayMode(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_DISPLAY_MODE_START + offset, 1, &tmp_val);
}

int SSMReadDisplayMode(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_DISPLAY_MODE_START + offset, 1,
                            &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveBackLightVal(int offset, int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_BACKLIGHT_START + offset, 1, &tmp_val);
}

int SSMReadBackLightVal(int offset, int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_BACKLIGHT_START + offset, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN360BackLightVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_BACKLIGHT_START , 1, &tmp_val);
}

int SSMReadFBCN360BackLightVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_BACKLIGHT_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN360ColorTempVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_COLORTEMP_START , 1, &tmp_val);
}

int SSMReadFBCN360ColorTempVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_COLORTEMP_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}


int SSMSaveFBCELECmodeVal(int rw_val)
{
    unsigned char tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_FBC_ELECMODE_START , 1, &tmp_val);
}

int SSMReadFBCELECmodeVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret =  SSMReadNTypes(VPP_DATA_POS_FBC_ELECMODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}


int SSMSaveColorDemoMode(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_COLOR_DEMO_MODE_START, 1, &rw_val);
}

int SSMReadColorDemoMode(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_COLOR_DEMO_MODE_START, 1, rw_val);
}

int SSMSaveColorBaseMode(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_COLOR_BASE_MODE_START, 1, &rw_val);
}

int SSMReadColorBaseMode(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_COLOR_BASE_MODE_START, 1, rw_val);
}

int SSMSaveRGBGainRStart(int offset, unsigned int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_R_START + offset, 1, &rw_val);
}

int SSMReadRGBGainRStart(int offset, unsigned int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_R_START + offset, 1, rw_val);
}

int SSMSaveRGBGainGStart(int offset, unsigned int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_G_START + offset, 1, &rw_val);
}

int SSMReadRGBGainGStart(int offset, unsigned int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_G_START + offset, 1, rw_val);
}

int SSMSaveRGBGainBStart(int offset, unsigned int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_B_START + offset, 1, &rw_val);
}

int SSMReadRGBGainBStart(int offset, unsigned int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_B_START + offset, 1, rw_val);
}

int SSMSaveRGBPostOffsetRStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_R_START + offset, 1,
                          &rw_val);
}

int SSMReadRGBPostOffsetRStart(int offset, int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_R_START + offset, 1,
                         rw_val);
}

int SSMSaveRGBPostOffsetGStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_G_START + offset, 1,
                          &rw_val);
}

int SSMReadRGBPostOffsetGStart(int offset, int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_G_START + offset, 1,
                         rw_val);
}

int SSMSaveRGBPostOffsetBStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_B_START + offset, 1,
                          &rw_val);
}

int SSMReadRGBPostOffsetBStart(int offset, int *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_B_START + offset, 1,
                         rw_val);
}

int SSMSaveRGBValueStart(int offset, int8_t rw_val)
{
    return SSMWriteNTypes(VPP_DATA_RGB_START + offset, 1, &rw_val);
}

int SSMReadRGBValueStart(int offset, int8_t *rw_val)
{
    return SSMReadNTypes(VPP_DATA_RGB_START + offset, 1, &rw_val);
}

int SSMSaveDBCStart(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_DBC_START, 1, &rw_val);
}

int SSMReadDBCStart(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_DBC_START, 1, rw_val);
}

int SSMSaveColorSpaceStart(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_COLOR_SPACE_START, 1, &rw_val);
}

int SSMReadColorSpaceStart(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_COLOR_SPACE_START, 1, rw_val);
}

int SSMSaveDnlpStart(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_DNLP_START, 1, &rw_val);
}

int SSMReadDnlpStart(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_DNLP_START, 1, rw_val);
}

int SSMSavePanoramaStart(int offset, unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_PANORAMA_START + offset, 1, &rw_val);
}

int SSMReadPanoramaStart(int offset, unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_PANORAMA_START + offset, 1, rw_val);
}

int SSMSaveTestPattern(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, &rw_val);
}

int SSMReadTestPattern(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, rw_val);
}

int SSMSaveAPL(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_APL_START, VPP_DATA_APL_SIZE, &rw_val);
}

int SSMReadAPL(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_APL_START, VPP_DATA_APL_SIZE, rw_val);
}

int SSMSaveAPL2(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_APL2_START, VPP_DATA_APL2_SIZE, &rw_val);
}

int SSMReadAPL2(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_APL2_START, VPP_DATA_APL2_SIZE, rw_val);
}

int SSMSaveBD(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_BD_START, VPP_DATA_BD_SIZE, &rw_val);
}

int SSMReadBD(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_BD_START, VPP_DATA_BD_SIZE, rw_val);
}

int SSMSaveBP(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_BP_START, VPP_DATA_BP_SIZE, &rw_val);
}

int SSMReadBP(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_BP_START, VPP_DATA_BP_SIZE, rw_val);
}

int SSMSaveDDRSSC(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_DDR_SSC_START, 1, &rw_val);
}

int SSMReadDDRSSC(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_DDR_SSC_START, 1, rw_val);
}

int SSMSaveLVDSSSC(unsigned char *rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_LVDS_SSC_START, 2, rw_val);
}

int SSMReadLVDSSSC(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_LVDS_SSC_START, 2, rw_val);
}

int SSMSaveDreamPanel(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_DREAM_PANEL_START, 1, &rw_val);
}

int SSMReadDreamPanel(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_DREAM_PANEL_START, 1, rw_val);
}

int SSMSaveUserNatureLightSwitch(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_USER_NATURE_SWITCH_START, 1, &rw_val);
}

int SSMReadUserNatureLightSwitch(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_USER_NATURE_SWITCH_START, 1, rw_val);
}

int SSMSaveDBCBacklightEnable(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_DBC_BACKLIGHT_START, 1, &rw_val);
}

int SSMReadDBCBacklightEnable(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_DBC_BACKLIGHT_START, 1, rw_val);
}

int SSMSaveDBCBacklightStd(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_DBC_STANDARD_START, 1, &rw_val);
}

int SSMReadDBCBacklightStd(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_DBC_STANDARD_START, 1, rw_val);
}

int SSMSaveDBCEnable(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_DBC_ENABLE_START, 1, &rw_val);
}

int SSMReadDBCEnable(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_DBC_ENABLE_START, 1, rw_val);
}

int SSMSaveGammaValue(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_GAMMA_VALUE_START, 1, &tmp_val);
}

int SSMReadGammaValue(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_GAMMA_VALUE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveBackLightReverse(unsigned char rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_BACKLIGHT_REVERSE_START, 1, &rw_val);
}

int SSMReadBackLightReverse(unsigned char *rw_val)
{
    return SSMReadNTypes(VPP_DATA_POS_BACKLIGHT_REVERSE_START, 1, rw_val);
}

int SSMSaveAudioMasterVolume(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_MASTR_VOLUME_VAL, 1, &rw_val);
}

int SSMReadAudioMasterVolume(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_MASTR_VOLUME_VAL, 1, rw_val);
}

int SSMSaveAudioBalanceVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_BALANCE_VAL, 1, &rw_val);
}

int SSMReadAudioBalanceVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_BALANCE_VAL, 1, rw_val);
}

int SSMSaveAudioSupperBassVolume(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SUPPERBASS_VOLUME_VAL, 1, &rw_val);
}

int SSMReadAudioSupperBassVolume(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SUPPERBASS_VOLUME_VAL, 1, rw_val);
}

int SSMSaveAudioSupperBassSwitch(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SUPPERBASS_SWITCH, 1, &rw_val);
}

int SSMReadAudioSupperBassSwitch(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SUPPERBASS_SWITCH, 1, rw_val);
}

int SSMSaveAudioSRSSurroundSwitch(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SRS_SURROUND_SWITCH, 1, &rw_val);
}

int SSMReadAudioSRSSurroundSwitch(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SRS_SURROUND_SWITCH, 1, rw_val);
}

int SSMSaveAudioSRSDialogClaritySwitch(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SRS_DIALOG_CLARITY_SWITCH, 1, &rw_val);
}

int SSMSaveAudioDbxTvValue(int son_value, int vol_value, int sur_value)
{
    int8_t rw_val = son_value;
    SSMWriteNTypes(SSM_AUD_DBX_TV_SON, 1, &rw_val);
    rw_val = vol_value;
    SSMWriteNTypes(SSM_AUD_DBX_TV_VAL, 1, &rw_val);
    rw_val = sur_value;
    SSMWriteNTypes(SSM_AUD_DBX_TV_SUR, 1, &rw_val);
    return 0;
}

int SSMReadAudioDbxTvValue(int *son_value, int *vol_value, int *sur_value)
{
    unsigned char rw_val;
    SSMReadNTypes(SSM_AUD_DBX_TV_SON, 1, &rw_val);
    *son_value = rw_val;
    SSMReadNTypes(SSM_AUD_DBX_TV_VAL, 1, &rw_val);
    *vol_value = rw_val;
    SSMReadNTypes(SSM_AUD_DBX_TV_SUR, 1, &rw_val);
    *sur_value = rw_val;
    return 0;
}

int SSMReadAudioSRSDialogClaritySwitch(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SRS_DIALOG_CLARITY_SWITCH, 1, rw_val);
}

int SSMSaveAudioSRSTruBassSwitch(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SRS_TRUEBASS_SWITCH, 1, &rw_val);
}

int SSMReadAudioSRSTruBassSwitch(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SRS_TRUEBASS_SWITCH, 1, rw_val);
}

int SSMSaveAudioBassVolume(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_BASS_VOLUME_VAL, 1, &rw_val);
}

int SSMReadAudioBassVolume(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_BASS_VOLUME_VAL, 1, rw_val);
}

int SSMSaveAudioTrebleVolume(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_TREBLE_VOLUME_VAL, 1, &rw_val);
}

int SSMReadAudioTrebleVolume(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_TREBLE_VOLUME_VAL, 1, rw_val);
}

int SSMSaveAudioSoundModeVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SOUND_MODE_VAL, 1, &rw_val);
}

int SSMReadAudioSoundModeVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SOUND_MODE_VAL, 1, rw_val);
}

int SSMSaveAudioWallEffectSwitch(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_WALL_EFFCT_SWITCH, 1, &rw_val);
}

int SSMReadAudioWallEffectSwitch(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_WALL_EFFCT_SWITCH, 1, rw_val);
}

int SSMSaveAudioSPDIFSwitchVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SPDIF_SWITCH, 1, &rw_val);
}

int SSMReadAudioSPDIFSwitchVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SPDIF_SWITCH, 1, rw_val);
}

int SSMSaveAudioSPDIFModeVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SPDIF_MODE_VAL, 1, &rw_val);
}

int SSMReadAudioSPDIFModeVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SPDIF_MODE_VAL, 1, rw_val);
}

int SSMSaveAudioEQModeVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_EQ_MODE_VAL, 1, &rw_val);
}

int SSMReadAudioEQModeVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_EQ_MODE_VAL, 1, rw_val);
}

int SSMSaveAudioEQGain(int offset, int size, int8_t tmp_buf[])
{
    return SSMWriteNTypes(SSM_AUD_EQ_GAIN + offset, size, tmp_buf);
}

int SSMReadAudioEQGain(int offset __unused, int size, int8_t tmp_buf[])
{
    return SSMReadNTypes(SSM_AUD_EQ_GAIN, size, tmp_buf);
}

int SSMSaveAudioAVOutMuteVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_AVOUT_MUTE, 1, &rw_val);
}

int SSMReadAudioAVOutMuteVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_AVOUT_MUTE, 1, rw_val);
}

int SSMSaveAudioSPIDFMuteVal(int8_t rw_val)
{
    return SSMWriteNTypes(SSM_AUD_SPIDF_MUTE, 1, &rw_val);
}

int SSMReadAudioSPIDFMuteVal(int8_t *rw_val)
{
    return SSMReadNTypes(SSM_AUD_SPIDF_MUTE, 1, rw_val);
}
int SSMSaveBlackoutEnable(int8_t enable)
{
    return SSMWriteNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, &enable);
}

int SSMReadBlackoutEnable(int8_t *enable)
{
    return SSMReadNTypes(SSM_RW_BLACKOUT_ENABLE_START, 1, enable);
}
int SSMSaveFBCN310BackLightVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_BACKLIGHT_START , 1, &tmp_val);
}

int SSMReadFBCN310BackLightVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_BACKLIGHT_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN310ColorTempVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_COLORTEMP_START , 1, &tmp_val);
}

int SSMReadFBCN310ColorTempVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_COLORTEMP_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN310LightsensorVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START , 1, &tmp_val);
}

int SSMReadFBCN310LightsensorVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN310Dream_PanelVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_DREAMPANEL_START , 1, &tmp_val);
}

int SSMReadFBCN310Dream_PanelVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_DREAMPANEL_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN310MULT_PQVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_MULTI_PQ_START , 1, &tmp_val);
}

int SSMReadFBCN310MULT_PQVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_MULTI_PQ_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveFBCN310MEMCVal(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_FBC_N310_MEMC_START , 1, &tmp_val);
}

int SSMReadFBCN310MEMCVal(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_FBC_N310_MEMC_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}

int SSMSaveN311_VbyOne_Spread_Spectrum_Val(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START , 1, &tmp_val);
}

int SSMReadN311_VbyOne_Spread_Spectrum_Val(int *rw_val)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return tmp_ret;
}
int SSMSaveN311_Bluetooth_Vol(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return SSMWriteNTypes(VPP_DATA_POS_N311_BLUETOOTH_VAL_START , 1, &tmp_val);
}

int SSMReadN311_Bluetooth_Vol(void)
{
    int tmp_ret = 0;
    unsigned char tmp_val = 0;

    tmp_ret = SSMReadNTypes(VPP_DATA_POS_N311_BLUETOOTH_VAL_START, 1, &tmp_val);

    if (tmp_ret < 0) {
        return 0;
    }

    return tmp_val;
}
int SSMSave_DRC_ONOFF_Val(int rw_val)
{
    unsigned char tmp_val = rw_val;

    return  SSMWriteNTypes(SSM_AUD_DRC_ONOFF , 1, &tmp_val);

}
int SSMRead_DRC_ONOFF_Val(void)
{
    unsigned char tmp_val = 0;
    int tmp_ret = 0;

    tmp_ret = SSMReadNTypes(SSM_AUD_DRC_ONOFF, 1, &tmp_val);

    if (tmp_ret < 0) {
        return 0;
    }

    return tmp_val;

}

int SSMSave_PANEL_ID_Val(int rw_val)
{
    unsigned char tmp_val = rw_val;
    return  SSMWriteNTypes(SSM_RW_PANEL_ID_START , 1, &tmp_val);
}
int SSMRead_PANEL_ID_Val(void)
{
    unsigned char tmp_val = 0;
    int tmp_ret = 0;
    tmp_ret = SSMReadNTypes(SSM_RW_PANEL_ID_START, 1, &tmp_val);
    if (tmp_ret < 0) {
        return 0;
    }
    return tmp_val;
}

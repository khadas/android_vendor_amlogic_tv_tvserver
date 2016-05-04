#define LOG_TAG "tvserver"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <android/log.h>
#include <cutils/android_reboot.h>
#include "../tvconfig/tvconfig.h"
#include "../tvsetting/CTvSetting.h"
#include <cutils/properties.h>
#include <dirent.h>

#include <utils/threads.h>
#include <binder/IServiceManager.h>
#include <systemcontrol/ISystemControlService.h>

#include "tvutils.h"
#include "CTvLog.h"

using namespace android;

#define CC_HEAD_CHKSUM_LEN      (9)
#define CC_VERSION_LEN          (5)

static pthread_mutex_t file_attr_control_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t UserPet_ThreadId = 0;
static unsigned char is_turnon_user_pet_thread = false;
static unsigned char is_user_pet_thread_start = false;
static unsigned int user_counter = 0;
static unsigned int user_pet_terminal = 1;

static int gFBCPrjInfoRDPass = 0;
static char gFBCPrjInfoBuf[1024] = {0};

static Mutex amLock;
static sp<ISystemControlService> amSystemControlService;
class DeathNotifier: public IBinder::DeathRecipient {
public:
    DeathNotifier() {
    }

    void binderDied(const wp<IBinder> &who __unused) {
        ALOGW("system_control died!");

        amSystemControlService.clear();
    }
};


static sp<DeathNotifier> amDeathNotifier;
static const sp<ISystemControlService> &getSystemControlService()
{
    Mutex::Autolock _l(amLock);
    if (amSystemControlService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("system_control"));
            if (binder != 0)
                break;
            ALOGW("SystemControlService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (amDeathNotifier == NULL) {
            amDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(amDeathNotifier);
        amSystemControlService = interface_cast<ISystemControlService>(binder);
    }
    ALOGE_IF(amSystemControlService == 0, "no System Control Service!?");

    return amSystemControlService;
}

int getBootEnv(const char *key, char *value, const char *def_val)
{
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        String16 v;
        if (sws->getBootEnv(String16(key), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }

    strcpy(value, def_val);
    return -1;
}

void setBootEnv(const char *key, const char *value)
{
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        sws->setBootEnv(String16(key), String16(value));
    }
}

int writeSys(const char *path, const char *val) {
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        ALOGE("writeSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    //ALOGI("write %s, val:%s\n", path, val);

    int len = write(fd, val, strlen(val));
    close(fd);
    return len;
}

int readSys(const char *path, char *buf, int count) {
    int fd, len;

    if ( NULL == buf ) {
        ALOGE("buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        ALOGE("readSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        ALOGE("read %s error, %s\n", path, strerror(errno));
        goto exit;
    }

    //ALOGI("read %s, result length:%d, val:%s\n", path, len, buf);

exit:
    close(fd);
    return len;
}

int tvReadSysfs(const char *path, char *value) {
#ifdef USE_SYSTEM_CONTROL
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        String16 v;
        if (sws->readSysfs(String16(path), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }
    return -1;
#else
    char buf[SYS_STR_LEN+1] = {0};
    int len = readSys(path, (char*)buf, SYS_STR_LEN);
    strcpy(value, buf);
    return len;
#endif
}

int tvWriteSysfs(const char *path, const char *value) {
#ifdef USE_SYSTEM_CONTROL
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        sws->writeSysfs(String16(path), String16(value));
    }
    return 0;
#else
    return writeSys(path, value);
#endif
}

int Tv_MiscRegs(const char *cmd)
{
    FILE *fp = NULL;
    fp = fopen("/sys/class/register/reg", "w");

    if (fp != NULL && cmd != NULL) {
        fprintf(fp, "%s", cmd);
    } else {
        LOGE("Open /sys/class/register/reg ERROR(%s)!!\n", strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int TvMisc_SetLVDSSSC(int val)
{
    FILE *fp;

    fp = fopen("/sys/class/lcd/ss", "w");
    if (fp != NULL) {
        fprintf(fp, "%d", val);
        fclose(fp);
    } else {
        LOGE("open /sys/class/lcd/ss ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
    return 0;
}

int TvMisc_SetUserCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/user_pet_timeout", "w");
    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
    return 0;
}

int TvMisc_SetUserCounter(int count)
{
    FILE *fp;

    fp = fopen("/sys/module/aml_wdt/parameters/user_pet", "w");
    if (fp != NULL) {
        fprintf(fp, "%d", count);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetUserPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/module/aml_wdt/parameters/user_pet_reset_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet_reset_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/reset_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/reset_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/ping_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/ping_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/wdt_timeout", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/wdt_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

//0-turn off
//1-force non-standard
//2-force normal
int Set_Fixed_NonStandard(int value)
{
    int fd = -1, ret = -1;
    char set_vale[32] = {0};

    sprintf(set_vale, "%d", value);

    fd = open("/sys/module/tvin_afe/parameters/force_nostd", O_RDWR);
    if (fd >= 0) {
        ret = write(fd, set_vale, strlen(set_vale));
    }

    if (ret <= 0) {
        LOGE("%s -> set /sys/module/tvin_afe/parameters/force_nostd error(%s)!\n", CFG_SECTION_TV, strerror(errno));
    }

    close(fd);
    return ret;
}

static void *UserPet_TreadRun(void *data __unused)
{
    while (is_turnon_user_pet_thread == true) {
        if (is_user_pet_thread_start == true) {
            usleep(1000 * 1000);
            if (++user_counter == 0xffffffff)
                user_counter = 1;
            TvMisc_SetUserCounter(user_counter);
        } else {
            usleep(10000 * 1000);
        }
    }
    if (user_pet_terminal == 1) {
        user_counter = 0;
    } else {
        user_counter = 1;
    }
    TvMisc_SetUserCounter(user_counter);
    return ((void *) 0);
}

static int UserPet_CreateThread(void)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

    is_turnon_user_pet_thread = true;
    is_user_pet_thread_start = true;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&UserPet_ThreadId, &attr, &UserPet_TreadRun, NULL);
    pthread_attr_destroy(&attr);
    return ret;
}

static void UserPet_KillThread(void)
{
    int i = 0, dly = 600;
    is_turnon_user_pet_thread = false;
    is_user_pet_thread_start = false;
    for (i = 0; i < 2; i++) {
        usleep(dly * 1000);
    }
    pthread_join(UserPet_ThreadId, NULL);
    UserPet_ThreadId = 0;
    LOGD("%s, done.", CFG_SECTION_TV);
}

void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable,
    unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset)
{
    TvMisc_SetSystemPetCounterTimeOut(kernelpet_timeout);
    TvMisc_SetSystemPetEnable(1);
    if (kernelpet_disable) {
        TvMisc_SetSystemPetResetEnable(0);
    } else {
        TvMisc_SetSystemPetResetEnable(1);
    }
    if (userpet_enable) {
        TvMisc_SetUserCounterTimeOut(userpet_timeout);
        TvMisc_SetUserPetResetEnable(userpet_reset);
        UserPet_CreateThread();
    } else {
        TvMisc_SetUserCounter(0);
        TvMisc_SetUserPetResetEnable(0);
    }
}

void TvMisc_DisableWDT(unsigned int userpet_enable)
{
    if (userpet_enable) {
        user_pet_terminal = 0;
        UserPet_KillThread();
    }
}

/*---------------delete dir---------------*/
int TvMisc_DeleteDirFiles(const char *strPath, int flag)
{
    int status;
    char tmp[256];
    switch (flag) {
    case 0:
        sprintf(tmp, "rm -f %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    case 1:
        sprintf(tmp, "cd %s", strPath);
        LOGE("%s", tmp);
        status = system(tmp);
        if (status > 0 || status < 0)
            return -1;
        sprintf(tmp, "cd %s;rm -rf *", strPath);
        system(tmp);
        LOGE("%s", tmp);
        break;
    case 2:
        sprintf(tmp, "rm -rf %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    }
    return 0;
}

//check file exist or not
bool Tv_Utils_IsFileExist(const char *file_name)
{
    struct stat tmp_st;

    return stat(file_name, &tmp_st) == 0;
}

int reboot_sys_by_fbc_edid_info()
{
    int ret = -1;
    int fd = -1;
    int edid_info_len = 256;
    unsigned char fbc_edid_info[edid_info_len];
    int env_different_as_cur = 0;
    char outputmode_prop_value[256];
    char lcd_reverse_prop_value[256];

    LOGD("get edid info from fbc!");
    memset(outputmode_prop_value, '\0', 256);
    memset(lcd_reverse_prop_value, '\0', 256);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    getBootEnv(UBOOTENV_LCD_REVERSE, lcd_reverse_prop_value, "null" );

    fd = open("/sys/class/amhdmitx/amhdmitx0/edid_info", O_RDWR);
    if (fd < 0) {
        LOGW("open edid node error\n");
        return -1;
    }
    ret = read(fd, fbc_edid_info, edid_info_len);
    if (ret < 0) {
        LOGW("read edid node error\n");
        return -1;
    }

    if ((0xfb == fbc_edid_info[250]) && (0x0c == fbc_edid_info[251])) {
        LOGD("RX is FBC!");
        // set outputmode env
        ret = 0;//is Fbc
        switch (fbc_edid_info[252] & 0x0f) {
        case 0x0:
            if (0 != strcmp(outputmode_prop_value, "1080p") &&
                    0 != strcmp(outputmode_prop_value, "1080p50hz")
               ) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "1080p");
            }
            break;
        case 0x1:
            if (0 != strcmp(outputmode_prop_value, "4k2k60hz420") &&
                    0 != strcmp(outputmode_prop_value, "4k2k50hz420")
               ) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "4k2k60hz420");
            }
            break;
        case 0x2:
            if (0 != strcmp(outputmode_prop_value, "1366*768")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "1366*768");
            }
            break;
        default:
            break;
        }

        // set RX 3D Info
        //switch((fbc_edid_info[252]>>4)&0x0f)

        // set lcd_reverse env
        switch (fbc_edid_info[253]) {
        case 0x0:
            if (0 != strcmp(lcd_reverse_prop_value, "0")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_LCD_REVERSE, "0");
            }
            break;
        case 0x1:
            if (0 != strcmp(lcd_reverse_prop_value, "1")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_LCD_REVERSE, "1");
            }
            break;
        default:
            break;
        }
    }
    close(fd);
    fd = -1;
    //ret = -1;
    if (1 == env_different_as_cur) {
        LOGW("env change , reboot system\n");
        system("reboot");
    }
    return ret;
}

int reboot_sys_by_fbc_uart_panel_info(CFbcCommunication *fbc)
{
    int ret = -1;
    char outputmode_prop_value[256] = {0};
    char lcd_reverse_prop_value[256] = {0};
    int env_different_as_cur = 0;
    int panel_reverse = -1;
    int panel_outputmode = -1;

    char panel_model[64] = {0};

    if (fbc == NULL) {
        LOGW("there is no fbc!!!\n");
        return -1;
    }

    fbc->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);
    if (0 == panel_model[0]) {
        LOGD("device is not fbc\n");
        return -1;
    }
    LOGD("device is fbc, get panel info from fbc!\n");
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    getBootEnv(UBOOTENV_LCD_REVERSE, lcd_reverse_prop_value, "null" );

    fbc->cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_SERIAL, &panel_reverse);
    fbc->cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_SERIAL, &panel_outputmode);
    LOGD("panel_reverse = %d, panel_outputmode = %d\n", panel_reverse, panel_outputmode);
    LOGD("panel_output prop = %s, panel reverse prop = %s\n", outputmode_prop_value, lcd_reverse_prop_value);
    switch (panel_outputmode) {
    case T_1080P50HZ:
        if (0 != strcmp(outputmode_prop_value, "1080p50hz")) {
            LOGD("panel_output changed to 1080p50hz\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "1080p50hz");
        }
        break;
    case T_2160P50HZ420:
        if (0 != strcmp(outputmode_prop_value, "2160p50hz420")) {
            LOGD("panel_output changed to 2160p50hz420\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz420");
        }
        break;
    case T_1080P50HZ44410BIT:
        if (0 != strcmp(outputmode_prop_value, "1080p50hz44410bit")) {
            LOGD("panel_output changed to 1080p50hz44410bit\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "1080p50hz44410bit");
        }
        break;
    case T_2160P50HZ42010BIT:
        if (0 != strcmp(outputmode_prop_value, "2160p50hz42010bit")) {
            LOGD("panel_output changed to 2160p50hz42010bit\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz42010bit");
        }
        break;
    case T_2160P50HZ42210BIT:
        if (0 != strcmp(outputmode_prop_value, "2160p50hz42210bit")) {
            LOGD("panel_output changed to 2160p50hz42210bit\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz42210bit");
        }
        break;
    case T_2160P50HZ444:
        if (0 != strcmp(outputmode_prop_value, "2160p50hz444")) {
            LOGD("panel_output changed to 2160p50hz444\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz444");
        }
        break;
    default:
        break;
    }

    // set RX 3D Info
    //switch((fbc_edid_info[252]>>4)&0x0f)

    // set lcd_reverse env
    switch (panel_reverse) {
    case 0x0:
        if (0 != strcmp(lcd_reverse_prop_value, "0")) {
            LOGD("panel_reverse changed to 0\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_LCD_REVERSE, "0");
        }
        break;
    case 0x1:
        if (0 != strcmp(lcd_reverse_prop_value, "1")) {
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_LCD_REVERSE, "1");
        }
        break;
    default:
        break;
    }

    ret = -1;
    if (1 == env_different_as_cur) {
        LOGW("env change , reboot system\n");
        system("reboot");
    }
    return 0;
}

int GetPlatformHaveDDFlag()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "platform.havedd", "null");
    if (strcmp(config_value, "true") == 0 || strcmp(config_value, "1") == 0) {
        return 1;
    }

    return 0;
}

int GetPlatformProjectInfoSrc()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "platform.projectinfo.src", "null");
    if (strcmp(config_value, "null") == 0 || strcmp(config_value, "prop") == 0) {
        return 0;
    } else if (strcmp(config_value, "emmckey") == 0) {
        return 1;
    } else if (strcmp(config_value, "fbc_ver") == 0) {
        return 2;
    }

    return 0;
}

unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len)
{
    static const unsigned int s_crc32[16] = {
        0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
        0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8,
        0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
    unsigned int crcu32 = crc;
    //if (buf_len < 0)
    //    return 0;
    if (!ptr) return 0;
    crcu32 = ~crcu32;
    while (buf_len--) {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }
    return ~crcu32;
}

static int check_projectinfo_data_valid(char *data_str, int chksum_head_len, int ver_len)
{
    int tmp_len = 0, tmp_ver = 0;
    char *endp = NULL;
    unsigned long src_chksum = 0, cal_chksum = 0;
    char tmp_buf[129] = { 0 };

    if (data_str != NULL) {
        tmp_len = strlen(data_str);
        if (tmp_len > chksum_head_len + ver_len) {
            cal_chksum = CalCRC32(0, (unsigned char *)(data_str + chksum_head_len), tmp_len - chksum_head_len);
            memcpy(tmp_buf, data_str, chksum_head_len);
            tmp_buf[chksum_head_len] = 0;
            src_chksum = strtoul(tmp_buf, &endp, 16);
            if (cal_chksum == src_chksum) {
                memcpy(tmp_buf, data_str + chksum_head_len, ver_len);
                if ((tmp_buf[0] == 'v' || tmp_buf[0] == 'V') && isxdigit(tmp_buf[1]) && isxdigit(tmp_buf[2]) && isxdigit(tmp_buf[3])) {
                    tmp_ver = strtoul(tmp_buf + 1, &endp, 16);
                    if (tmp_ver <= 0) {
                        LOGD("%s, project_info data version error!!!\n", __FUNCTION__);
                        return -1;
                    }
                } else {
                    LOGD("%s, project_info data version error!!!\n", __FUNCTION__);
                    return -1;
                }

                return tmp_ver;
            } else {
                LOGD("%s, cal_chksum = %x\n", __FUNCTION__, (unsigned int)cal_chksum);
                LOGD("%s, src_chksum = %x\n", __FUNCTION__, (unsigned int)src_chksum);
            }
        }

        LOGD("%s, project_info data error!!!\n", __FUNCTION__);
        return -1;
    }

    LOGD("%s, project_info data is NULL!!!\n", __FUNCTION__);
    return -1;
}

static int GetProjectInfoOriData(char data_str[], CFbcCommunication *fbcIns)
{
    int src_type = GetPlatformProjectInfoSrc();

    if (src_type == 0) {
        //memset(data_str, '\0', sizeof(data_str));//sizeof pointer has issue
        getBootEnv(UBOOTENV_PROJECT_INFO, data_str, (char *)"null");
        if (strcmp(data_str, "null") == 0) {
            LOGE("%s, get project info data error!!!\n", __FUNCTION__);
            return -1;
        }

        return 0;
    } else if (src_type == 1) {
        return -1;
    } else if (src_type == 2) {
        int i = 0, tmp_len = 0, tmp_val = 0, item_cnt = 0;
        int tmp_rd_fail_flag = 0;
        unsigned int cal_chksum = 0;
        char sw_version[64];
        char build_time[64];
        char git_version[64];
        char git_branch[64];
        char build_name[64];
        char tmp_buf[512] = {0};

        if (fbcIns != NULL) {
            if (gFBCPrjInfoRDPass == 0) {
                memset((void *)gFBCPrjInfoBuf, 0, sizeof(gFBCPrjInfoBuf));
            }

            if (gFBCPrjInfoRDPass == 1) {
                strcpy(data_str, gFBCPrjInfoBuf);
                LOGD("%s, rd once just return, data_str = %s\n", __FUNCTION__, data_str);
                return 0;
            }

            if (fbcIns->cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_SERIAL, sw_version, build_time, git_version, git_branch, build_name) == 0) {
                if (sw_version[0] == '1' || sw_version[0] == '2') {
                    strcpy(build_name, "2");

                    strcpy(tmp_buf, "v001,fbc_");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");
                    strcat(tmp_buf, "HV550QU2-305");
                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s\n", __FUNCTION__, data_str);
                } else {
                    tmp_val = 0;
                    if (fbcIns->cfbc_Get_FBC_project_id(COMM_DEV_SERIAL, &tmp_val) == 0) {
                        sprintf(build_name, "fbc_%d", tmp_val);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcpy(build_name, "fbc_0");
                        LOGD("%s, get project id from fbc error!!!\n", __FUNCTION__);
                    }

                    strcpy(tmp_buf, "v001,");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");

                    memset(git_branch, 0, sizeof(git_branch));
                    if (fbcIns->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, git_branch) == 0) {
                        strcat(tmp_buf, git_branch);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcat(tmp_buf, build_name);
                        LOGD("%s, get panel info from fbc error!!!\n", __FUNCTION__);
                    }

                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s\n", __FUNCTION__, data_str);

                    if (tmp_rd_fail_flag == 0) {
                        gFBCPrjInfoRDPass = 1;
                        strcpy(gFBCPrjInfoBuf, data_str);
                    }
                }

                return 0;
            }

            return -1;
        }
    }

    return -1;
}

static int handle_prj_info_by_ver(int ver, int item_ind, char *item_str, project_info_t *proj_info_ptr)
{
    if (ver == 1) {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    } else {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }  else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    }

    return 0;
}

int GetProjectInfo(project_info_t *proj_info_ptr, CFbcCommunication *fbcIns)
{
    int i = 0, tmp_ret = 0, tmp_val = 0, tmp_len = 0;
    int item_cnt = 0, handle_prj_info_data_flag = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    char tmp_buf[1024] = { 0 };
    char data_str[1024] = { 0 };

    if (GetProjectInfoOriData(data_str, fbcIns) < 0) {
        return -1;
    }

    memset((void *)proj_info_ptr->version, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_type, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_outputmode, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_rev, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->amp_curve_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);

    //check project info data is valid
    handle_prj_info_data_flag = check_projectinfo_data_valid(data_str, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN);

    //handle project info data
    if (handle_prj_info_data_flag > 0) {
        item_cnt = 0;
        memset((void *)tmp_buf, 0, sizeof(tmp_buf));
        strncpy(tmp_buf, data_str + CC_HEAD_CHKSUM_LEN, sizeof(tmp_buf) - 1);
        token = strtok(tmp_buf, strDelimit);
        while (token != NULL) {
            handle_prj_info_by_ver(handle_prj_info_data_flag, item_cnt, token, proj_info_ptr);

            token = strtok(NULL, strDelimit);
            item_cnt += 1;
        }

        return 0;
    }

    return -1;
}

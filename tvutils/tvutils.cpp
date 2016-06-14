#define LOG_TAG "tvserver"
#define LOG_TV_TAG "tvutils"

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
#include <cutils/properties.h>
#include <dirent.h>

#include <utils/threads.h>
#include <binder/IServiceManager.h>
#include <systemcontrol/ISystemControlService.h>

#include "tvconfig.h"
#include "tvutils.h"
#include "CTvLog.h"

using namespace android;

static pthread_mutex_t file_attr_control_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t UserPet_ThreadId = 0;
static unsigned char is_turnon_user_pet_thread = false;
static unsigned char is_user_pet_thread_start = false;
static unsigned int user_counter = 0;
static unsigned int user_pet_terminal = 1;

static Mutex amLock;
static sp<ISystemControlService> amSystemControlService;
class DeathNotifier: public IBinder::DeathRecipient {
public:
    DeathNotifier() {
    }

    void binderDied(const wp<IBinder> &who __unused) {
        LOGW("system_control died!");

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
            LOGW("SystemControlService not published, waiting...");
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
        LOGE("writeSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    //LOGI("write %s, val:%s\n", path, val);

    int len = write(fd, val, strlen(val));
    close(fd);
    return len;
}

int readSys(const char *path, char *buf, int count) {
    int fd, len;

    if ( NULL == buf ) {
        LOGE("buf is NULL");
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        LOGE("readSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        LOGE("read %s error, %s\n", path, strerror(errno));
        goto exit;
    }

    int i , j;
    for (i = 0, j = 0; i <= len -1; i++) {
        //change '\0' to 0x20(spacing), otherwise the string buffer will be cut off ,if the last char is '\0' should not replace it
        if (0x0 == buf[i] && i < len - 1) {
            buf[i] = 0x20;
        }
        /* delete all the character of '\n' */
        if (0x0a != buf[i]) {
            buf[j++] = buf[i];
        }
    }
    buf[j] = 0x0;

    //LOGI("read %s, result length:%d, val:%s\n", path, len, buf);

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

int tvWriteSysfs(const char *path, int value, int base)
{
    char str_value[64] = {0};
    if (base == 16) {
        sprintf(str_value, "0x%-x", value);
    } else {
        sprintf(str_value, "%d", value);
    }
    LOGD("tvWriteSysfs, str_value = %s", str_value);
#ifdef USE_SYSTEM_CONTROL
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        sws->writeSysfs(String16(path), String16(str_value));
    }
    return 0;
#else
    return writeSys(path, str_value);
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
bool isFileExist(const char *file_name)
{
    struct stat tmp_st;

    return stat(file_name, &tmp_st) == 0;
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


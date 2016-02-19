#include "CAudioCustomerCtrl.h"
#include <dlfcn.h>
#include "CTvLog.h"
#include "tvsetting/audio_cfg.h"

CAudioCustomerCtrl::CAudioCustomerCtrl()
{
    gExternalDacLibHandler = NULL;
    mpDacMuteFun = NULL;
    mpDacMainVolFun = NULL;
    mpDacBalanceFun = NULL;
    mpSetSourceTypeFun = NULL;
}

CAudioCustomerCtrl::~CAudioCustomerCtrl()
{
    UnLoadExternalDacLib();
}

int CAudioCustomerCtrl::UnLoadExternalDacLib(void)
{
    if (gExternalDacLibHandler != NULL) {
        dlclose(gExternalDacLibHandler);
        gExternalDacLibHandler = NULL;
    }

    mpDacMuteFun = NULL;
    mpDacMainVolFun = NULL;
    mpDacBalanceFun = NULL;
    mpSetSourceTypeFun = NULL;

    return 0;
}

int CAudioCustomerCtrl::LoadExternalDacLib(void)
{
    char *error;

    LOGD("%s, entering...\n", __FUNCTION__);

    if (gExternalDacLibHandler != NULL) {
        return 0;
    }

    const char *config_value = GetAudExtDacLibPath();
    gExternalDacLibHandler = dlopen(config_value, RTLD_NOW);
    if (!gExternalDacLibHandler) {
        LOGE("%s, failed to load external dac lib (%s)\n", __FUNCTION__, config_value);
        goto Error;
    }

    mpDacMuteFun = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacMute");
    if (mpDacMuteFun == NULL) {
        LOGE("%s, fail find fun mpDacMuteFun()\n", __FUNCTION__);
        goto Error;
    }
    mpDacMainVolFun = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacMainVolume");
    if (mpDacMainVolFun == NULL) {
        LOGE("%s, fail find fun HandleDacMainVolume()\n", __FUNCTION__);
        goto Error;
    }
    mpDacBalanceFun = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleDacBalance");
    if (mpDacBalanceFun == NULL) {
        LOGE("%s, fail find fun HandleDacBalance()\n", __FUNCTION__);
        goto Error;
    }
    mpSetSourceTypeFun = (int(*)(int))dlsym(gExternalDacLibHandler, "HandleAudioSourceType");
    if (mpSetSourceTypeFun == NULL) {
        LOGE("%s, fail find fun HandleAudioSourceType()\n", __FUNCTION__);
        goto Error;
    }

    return 0;

Error: //
    mpDacMuteFun = NULL;
    mpDacMainVolFun = NULL;
    mpDacBalanceFun = NULL;
    mpSetSourceTypeFun = NULL;

    if (gExternalDacLibHandler != NULL) {
        dlclose(gExternalDacLibHandler);
        gExternalDacLibHandler = NULL;
    }
    return -1;
}

// 0  mute,  1 unmute
int CAudioCustomerCtrl::SetMute(int mute)
{
    int ret = LoadExternalDacLib();
    if (mpDacMuteFun != NULL) {
        ret = (*mpDacMuteFun)(mute);
    }
    return ret;
}

int CAudioCustomerCtrl::SetVolumeBar(int vol)
{
    int ret = LoadExternalDacLib();
    if (mpDacMainVolFun != NULL) {
        LOGD("%s, call external dac lib HandleDacMainVolume (para = %d).\n", __FUNCTION__, vol);
        ret = (*mpDacMainVolFun)(vol);
    }
    return ret;
}

int CAudioCustomerCtrl::SetBlance(int balance)
{
    int ret = LoadExternalDacLib();
    if (mpDacBalanceFun != NULL) {
        LOGD("%s, call external dac lib HandleDacBalance (para = %d).\n", __FUNCTION__, balance);
        ret = (*mpDacBalanceFun)(balance);
    }
    return ret;
}

int CAudioCustomerCtrl::SetSource(int source)
{
    int ret = LoadExternalDacLib();
    if (mpSetSourceTypeFun != NULL) {
        LOGD("%s, call external dac lib HandleAudioSourceType (para = %d).\n", __FUNCTION__, source);
        ret = (*mpSetSourceTypeFun)(source);
    }
    return ret;
}

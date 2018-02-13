/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _TV_UTILS_H_
#define _TV_UTILS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utils/Mutex.h>
#include <string>
#include <map>
#include "PQType.h"

#ifndef DVB_SUCCESS
#define DVB_SUCCESS     (0)
#endif

#ifndef DVB_FAILURE
#define DVB_FAILURE     (-1)
#endif

#define SYS_STR_LEN                         1024
#define AUDIO_OUTMODE_PATH "/sys/module/atvdemod_fe/parameters/aud_mode"
#define ATVDEMODE_DEBUG_PATH "/sys/class/amlatvdemod/atvdemod_debug"
#define AUDIO_STREAM_OUTMODE_PATH "/sys/module/atvdemod_fe/parameters/signal_audmode"

int tvReadSysfs(const char *path, char *value);
int tvWriteSysfs(const char *path, const char *value);
int tvWriteSysfs(const char *path, int value, int base=10);
int tvWriteDisplayMode(const char *mode);
//add for PQ
int tvResetLastVppSettingsSourceType(void);
int tvLoadPQSettings(source_input_param_t source_input_param);
int tvLoadCpqLdimRegs(void);
int tvSSMReadNTypes(int id, int data_len,  int *data_buf, int offset);
int tvSSMWriteNTypes(int id, int data_len, int data_buf, int offset);
int tvGetActualAddr(int id);
int tvGetActualSize(int id);

int tvSetPQMode ( vpp_picture_mode_t mode, int is_save, int is_autoswitch);
vpp_picture_mode_t tvGetPQMode ( void );
int tvSavePQMode ( vpp_picture_mode_t mode);
int tvGetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t *pq_para);
int tvGetAutoSwitchPCModeFlag(void);

int tvSetBrightness ( int brightness, int is_save );
int tvGetBrightness ( void );
int tvSaveBrightness ( int brightness);
int tvSetContrast ( int contrast, int is_save );
int tvGetContrast ( void );
int tvSaveContrast ( int contrast );
int tvSetSaturation ( int satuation, int is_save );
int tvGetSaturation ( void );
int tvSaveSaturation ( int satuation );
int tvSetHue ( int hue, int is_save );
int tvGetHue ( void );
int tvSaveHue ( int hue );
int tvSetSharpness ( int value, int en, int is_save );
int tvGetSharpness ( void );
int tvSaveSharpness ( int value );
int tvSetColorTemperature ( vpp_color_temperature_mode_t mode, int is_save );
int tvGetColorTemperature ( void );
int tvSaveColorTemperature ( vpp_color_temperature_mode_t mode );
int tvSetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
int tvGetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, pq_color_param_t id);
int tvSaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
int tvSetNoiseReductionMode ( vpp_noise_reduction_mode_t mode, int is_save );
int tvGetNoiseReductionMode ( void );
int tvSaveNoiseReductionMode ( vpp_noise_reduction_mode_t mode );
int tvSetGamma(int gamma_curve, int is_save);
int tvGetGamma(void);
int tvSetEyeProtectionMode(source_input_param_t source_input_param, int enable);
int tvGetEyeProtectionMode(void);
int tvGetDisplayMode(source_input_param_t source_input_param);
int tvFactoryResetNonlinear(void);
int tvGetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_param_t id);

int tvFactoryResetPQMode(void);
int tvFactoryResetColorTemp(void);
int tvFactorySetPQParam(source_input_param_t source_input_param, int pq_mode, vpp_pq_param_t id, int value);
int tvFactoryGetPQParam(source_input_param_t source_input_param, int pq_mode, vpp_pq_param_t id);
int tvFactorySetColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value);
int tvFactoryGetColorTemperatureParam(int colortemperature_mode, pq_color_param_t id);
int tvFactorySaveColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value);
int tvFactorySetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t);
int tvFactoryGetOverscanParam(source_input_param_t source_input_param, int id);
int tvFactorySetNolineParams(source_input_param_t source_input_param, int type, noline_params_t noline_params);
int tvFactoryGetNolineParams(source_input_param_t source_input_param, int type, int id);
int tvFactorySetGamma(int gamma_r, int gamma_g, int gamma_b);
int tvFactorySetParamsDefault(void);
int tvFactorySSMRestore(void);
int tvSSMRecovery(void);
int tvGetSSMStatus(void);
int tvSetPLLValues(source_input_param_t source_input_param);
int tvSetCVD2Values(source_input_param_t source_input_param);
int tvSetPQConfig(Set_Flag_Cmd_t id, int value);
int tvSetCurrentSourceInfo(tv_source_input_t tv_source_input, tv_source_input_type_t source_type,tvin_port_t source_port,
                                    tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);

//PQ end

extern int Tv_MiscRegs(const char *cmd);

extern int TvMisc_SetLVDSSSC(int val);
extern int TvMisc_SetUserCounterTimeOut(int timeout);
extern int TvMisc_SetUserCounter(int count);
extern int TvMisc_SetUserPetResetEnable(int enable);
extern int TvMisc_SetSystemPetResetEnable(int enable);
extern int TvMisc_SetSystemPetEnable(int enable);
extern int TvMisc_SetSystemPetCounter(int count);
extern int SetAudioOutmode(int mode);
extern int GetAudioOutmode();
extern int GetAudioStreamOutmode();

extern void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable, unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset);
extern void TvMisc_DisableWDT(unsigned int userpet_enable);
extern int GetTvDBDefineSize();

extern int Set_Fixed_NonStandard(int value);


extern int TvMisc_DeleteDirFiles(const char *strPath, int flag);

extern bool isFileExist(const char *file_name);
extern int GetPlatformHaveDDFlag();
extern int getBootEnv(const char *key, char *value, const char *def_val);
extern void setBootEnv(const char *key, const char *value);
extern int readSysfs(const char *path, char *value);
extern void writeSysfs(const char *path, const char *value);
extern int GetFileAttrIntValue(const char *fp, int flag = O_RDWR);

template<typename T1, typename T2>
int ArrayCopy(T1 dst_buf[], T2 src_buf[], int copy_size)
{
    int i = 0;

    for (i = 0; i < copy_size; i++) {
        dst_buf[i] = src_buf[i];
    }

    return 0;
};


typedef std::map<std::string, std::string> STR_MAP;

extern void jsonToMap(STR_MAP &m, const std::string &j);
extern void mapToJson(std::string &j, const STR_MAP &m);
extern void mapToJson(std::string &j, const STR_MAP &m, const std::string &k);
extern void mapToJsonAppend(std::string &j, const STR_MAP &m, const std::string &k);

class Paras {
protected:
    STR_MAP mparas;

public:
    Paras() {}
    Paras(const Paras &paras):mparas(paras.mparas) {}
    Paras(const char *paras) { jsonToMap(mparas, std::string(paras)); }
    Paras(const STR_MAP &paras):mparas(paras) {}

    void clear() { mparas.clear(); }

    int getInt (const char *key, int def) const;
    void setInt(const char *key, int v);

    const std::string toString() { std::string s; mapToJson(s, mparas); return s; }

    Paras operator + (const Paras &p);
    Paras& operator = (const Paras &p) { mparas = p.mparas; return *this; };
};

float getUptimeSeconds();

extern int jsonGetInt(const char *json, const char *obj, const char *value, int def);
extern const std::string jsonGetString(const char *json, const char *obj, const char *value, const char *def);

extern int paramGetInt(const char *param, const char *section, const char *value, int def);
extern const std::string paramGetString(const char *param, const char *section, const char *value, const char *def);

inline const char *toReadable(const char *s) { return s? s : "null"; }

extern bool propertyGetBool(const char *prop, bool def);

#endif  //__TV_MISC_H__

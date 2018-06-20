/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CVpp"

#include "CVpp.h"
#include <CTvLog.h>
#include "../tvsetting/CTvSetting.h"
#include <cutils/properties.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>
#include "CTvLog.h"
#include <tvconfig.h>
#include "CAv.h"
#include "../tvin/CHDMIRxManager.h"


#define PI 3.14159265358979

CVpp *CVpp::mInstance;
CVpp *CVpp::getInstance()
{
    if (NULL == mInstance) mInstance = new CVpp();
    return mInstance;
}

CVpp::CVpp()
{
    mHdmiOutFbc = false;
    mbVppCfg_backlight_reverse = false;
    mbVppCfg_backlight_init = false;
    mbVppCfg_pqmode_depend_bklight = false;
    fbcIns = GetSingletonFBC();
}

CVpp::~CVpp()
{

}

int CVpp::Vpp_Init(bool hdmiOutFbc)
{
    int ret = -1;
    int backlight = DEFAULT_BACKLIGHT_BRIGHTNESS;
    int offset_r = 0, offset_g = 0, offset_b = 0, gain_r = 1024, gain_g = 1024, gain_b = 1024;

    mHdmiOutFbc = hdmiOutFbc;

    Vpp_SetVppConfig();

    if (SSMReadNonStandardValue() & 1) {
        Set_Fixed_NonStandard(0); //close
    } else {
        Set_Fixed_NonStandard(2); //open
    }

    if (!LoadVppLdimRegs()) {
        LOGD ("Load local dimming regs success.\n");
    }
    else {
        LOGD ("Load local dimming regs failure!!!\n");
    }
    return ret;
}

int CVpp::Vpp_ResetLastVppSettingsSourceType(void)
{
    return tvResetLastVppSettingsSourceType();
}

int CVpp::LoadVppLdimRegs()
{
    return tvLoadCpqLdimRegs();
}

int CVpp::LoadVppSettings(tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                          is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType(tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);
    source_input_param.sig_fmt = sig_fmt;
    source_input_param.is3d = is3d;
    source_input_param.trans_fmt = trans_fmt;
    return tvLoadPQSettings(source_input_param);
}

int CVpp::Vpp_SetVppConfig(void)
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "vpp.pqmode.depend.bklight", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(PQ_DEPEND_BACKLIGHT, 1);
    } else {
        tvSetPQConfig(PQ_DEPEND_BACKLIGHT, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.backlight.reverse", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(BACKLIGHT_REVERSE, 1);
    } else {
        tvSetPQConfig(BACKLIGHT_REVERSE, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.backlight.init", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(BACKLIGHT_INIT, 1);
    } else {
        tvSetPQConfig(BACKLIGHT_INIT, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.color.temp.bysource", "enable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(COLORTEMP_BY_SOURCE, 1);
    } else {
        tvSetPQConfig(COLORTEMP_BY_SOURCE, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.pqwithout.hue", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(PQ_WITHOUT_HUE, 1);
    } else {
        tvSetPQConfig(PQ_WITHOUT_HUE, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.hue.reverse", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(HUE_REVERSE, 1);
    } else {
        tvSetPQConfig(HUE_REVERSE, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.gamma.onoff", "null");
    if (strcmp(config_value, "disable") == 0) {
        tvSetPQConfig(GAMMA_ONOFF, 1);
    } else {
        tvSetPQConfig(GAMMA_ONOFF, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.new.cm", "disable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(NEW_CM, 1);
    } else {
        tvSetPQConfig(NEW_CM, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.new.nr", "disable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(NEW_NR, 1);
    } else {
        tvSetPQConfig(NEW_NR, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.contrast.withOSD", "disable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(CONTRAST_WITHOSD, 1);
    } else {
        tvSetPQConfig(CONTRAST_WITHOSD, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.hue.withOSD", "disable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(HUE_WITHOSD, 1);
    } else {
        tvSetPQConfig(HUE_WITHOSD, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.brightness.withOSD", "disable");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(BRIGHTNESS_WITHOSD, 1);
    } else {
        tvSetPQConfig(BRIGHTNESS_WITHOSD, 0);
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.xvycc.switch_control", "null");
    if (strcmp(config_value, "enable") == 0) {
        tvSetPQConfig(XVYCC_SWITCH_CONTROL, 1);
    } else {
        tvSetPQConfig(XVYCC_SWITCH_CONTROL, 0);
    }

    return 0;
}

int CVpp::SetPQMode(vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                        tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save, int is_autoswitch)
{
    return tvSetPQMode(pq_mode, is_save, is_autoswitch);

}

vpp_picture_mode_t CVpp::GetPQMode(tv_source_input_t tv_source_input)
{
    return tvGetPQMode();
}

int CVpp::SavePQMode(vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input)
{

    return tvSavePQMode(pq_mode);
}

void CVpp::enableMonitorMode(bool enable)
{
    if (enable) {
        tvWriteSysfs(DI_NR2_ENABLE, "0");
        tvWriteSysfs(AMVECM_PC_MODE, "0");
        if (mHdmiOutFbc && fbcIns) {
            fbcIns->cfbc_EnterPCMode(0);
        }
    } else {
        tvWriteSysfs(DI_NR2_ENABLE, "1");
        tvWriteSysfs(AMVECM_PC_MODE, "1");
        if (mHdmiOutFbc && fbcIns) {
            fbcIns->cfbc_EnterPCMode(1);
        }
    }
}


 int CVpp::SetBrightness(int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                         tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
 {
     return tvSetBrightness(value, is_save);
 }

 int CVpp::GetBrightness( tv_source_input_t tv_source_input)
 {
     return tvGetBrightness();
 }

 int CVpp::SaveBrightness(int value, tv_source_input_t tv_source_input)
 {
     return tvSaveBrightness(value);
 }

 int CVpp::SetSaturation(int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                         tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
 {
     return tvSetSaturation(value, is_save);
 }

 int CVpp::GetSaturation(tv_source_input_t tv_source_input)
 {
     return tvGetSaturation();
 }

 int CVpp::SaveSaturation(int value, tv_source_input_t tv_source_input)
 {
     return tvSaveSaturation(value);
 }

 int CVpp::SetColorTemperature(vpp_color_temperature_mode_t temp_mode, tv_source_input_t tv_source_input, int is_save)
 {
     return tvSetColorTemperature(temp_mode, is_save);
 }

 vpp_color_temperature_mode_t CVpp::GetColorTemperature(tv_source_input_t tv_source_input)
 {
     return (vpp_color_temperature_mode_t)tvGetColorTemperature();
 }

 int CVpp::SaveColorTemperature(vpp_color_temperature_mode_t temp_mode, tv_source_input_t tv_source_input)
 {
     return tvSaveColorTemperature(temp_mode);
 }

 int CVpp::SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode,
                                 tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                                 tvin_trans_fmt_t trans_fmt, int is_save)
 {
     return tvSetNoiseReductionMode(nr_mode, is_save);
 }

 vpp_noise_reduction_mode_t CVpp::GetNoiseReductionMode(tv_source_input_t tv_source_input)
 {
     return (vpp_noise_reduction_mode_t)tvGetNoiseReductionMode();
 }

 int CVpp::SaveNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode, tv_source_input_t tv_source_input)
  {
      return tvSaveNoiseReductionMode(nr_mode);
  }

 int CVpp::SetSharpness(int value, tv_source_input_t tv_source_input, int is_enable,
                        is_3d_type_t is3d, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, int is_save)
 {
     return tvSetSharpness(value, is_enable, is_save);
 }

 int CVpp::GetSharpness(tv_source_input_t tv_source_input)
 {
     return tvGetSharpness();
 }

 int CVpp::SaveSharpness(int value, tv_source_input_t tv_source_input)
 {
     return tvSaveSharpness(value);
 }

 int CVpp::SetHue(int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                  tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
 {
     return tvSetHue(value, is_save);
 }

 int CVpp::GetHue(tv_source_input_t tv_source_input)
 {
     return tvGetHue();
 }

 int CVpp::SaveHue(int value, tv_source_input_t tv_source_input)
 {
     return tvSaveHue(value);
 }

 int CVpp::SetContrast(int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt,
                       tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
 {
     return tvSetContrast(value, is_save);
 }

 int CVpp::GetContrast(tv_source_input_t tv_source_input)
 {
     return tvGetContrast();
 }

 int CVpp::SaveContrast(int value, tv_source_input_t tv_source_input)
 {
     return tvSaveContrast(value);
 }

 int CVpp::SetEyeProtectionMode(tv_source_input_t tv_source_input, int enable, int is_save)
 {
     return tvSetEyeProtectionMode(tv_source_input, enable, is_save);
 }

 int CVpp::GetEyeProtectionMode(tv_source_input_t tv_source_input)
 {
     return tvGetEyeProtectionMode(tv_source_input);
 }

int CVpp::VPP_SetNonLinearFactor(int value)
{
    LOGD("VPP_SetNonLinearFactor /sys/class/video/nonlinear_factor : %d", value);
    FILE *fp = fopen("/sys/class/video/nonlinear_factor", "w");
    if (fp == NULL) {
        LOGE("Open /sys/class/video/nonlinear_factor error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save)
{
    return tvSetGamma((int)gamma_curve, is_save);
}

int CVpp::GetGammaValue()
{
    return tvGetGamma();
}

int CVpp::SetDisplayMode(vpp_display_mode_t disp_mode, tv_source_input_t tv_source_input, int is_save)
{
    return tvSetDisplayMode(disp_mode, tv_source_input, is_save);
}

vpp_display_mode_t CVpp::GetDisplayMode(tv_source_input_t tv_source_input)
{
    return (vpp_display_mode_t)tvGetDisplayMode(tv_source_input);
}

int CVpp::SaveDisplayMode(vpp_display_mode_t disp_mode, tv_source_input_t tv_source_input)
{
    return tvSaveDisplayMode(disp_mode, tv_source_input);
}

int CVpp::SetBacklight(int value, tv_source_input_t tv_source_input, int is_save)
{
    int ret = -1;
    if (mHdmiOutFbc) {
        if (fbcIns != NULL) {
            ret = fbcIns->cfbc_Set_Backlight(COMM_DEV_SERIAL, value*255/100);
        }
    } else {
        ret = tvSetBacklight(tv_source_input, value, is_save);
    }

    return ret;
}

int CVpp::GetBacklight(tv_source_input_t tv_source_input)
{
    return tvGetBacklight(tv_source_input);
}

int CVpp::SaveBacklight(int value, tv_source_input_t tv_source_input)
{
    return tvSaveBacklight(tv_source_input, value);
}

int CVpp::VPP_SetBackLight_Switch(int value)
{
    LOGD("VPP_SetBackLight_Switch VPP_PANEL_BACKLIGHT_DEV_PATH : %d ", value);
    FILE *fp = fopen(VPP_PANEL_BACKLIGHT_DEV_PATH, "w");
    if (fp == NULL) {
        LOGE("Open VPP_PANEL_BACKLIGHT_DEV_PATH error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::VPP_GetBackLight_Switch(void)
{
    int value;
    int ret;

    FILE *fp = fopen(VPP_PANEL_BACKLIGHT_DEV_PATH, "w");
    if (fp == NULL) {
        LOGE("Open VPP_PANEL_BACKLIGHT_DEV_PATH error(%s)!\n", strerror(errno));
        return -1;
    }

    ret = fscanf(fp, "%d", &value);
    if (ret == -1)
    {
        return -1;
    }
    LOGD("VPP_GetBackLight_Switch VPP_PANEL_BACKLIGHT_DEV_PATH : %d", value);
    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        return value;
    }
}

int CVpp::FactorySetPQMode_Brightness(int source_type, int pq_mode, int brightness)
{
    source_input_param_t source_input_param;
    source_input_param.source_type = (tv_source_input_type_t)source_type;

    return tvFactorySetPQParam(source_input_param, pq_mode, BRIGHTNESS, brightness);
}

int CVpp::FactoryGetPQMode_Brightness(int tv_source_input, int pq_mode)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactoryGetPQParam(source_input_param, pq_mode, BRIGHTNESS);
}

int CVpp::FactorySetPQMode_Contrast(int tv_source_input, int pq_mode, int contrast)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactorySetPQParam(source_input_param, pq_mode, CONTRAST, contrast);
}

int CVpp::FactoryGetPQMode_Contrast(int tv_source_input, int pq_mode)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactoryGetPQParam(source_input_param, pq_mode, CONTRAST);
}

int CVpp::FactorySetPQMode_Saturation(int tv_source_input, int pq_mode, int saturation)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactorySetPQParam(source_input_param, pq_mode, SATURATION, saturation);
}

int CVpp::FactoryGetPQMode_Saturation(int tv_source_input, int pq_mode)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactoryGetPQParam(source_input_param, pq_mode, SATURATION);
}

int CVpp::FactorySetPQMode_Hue(int tv_source_input, int pq_mode, int hue)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactorySetPQParam(source_input_param, pq_mode, HUE, hue);
}

int CVpp::FactoryGetPQMode_Hue(int tv_source_input, int pq_mode)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactoryGetPQParam(source_input_param, pq_mode, HUE);
}

int CVpp::FactorySetPQMode_Sharpness(int tv_source_input, int pq_mode, int sharpness)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactorySetPQParam(source_input_param, pq_mode, SHARPNESS, sharpness);
}

int CVpp::FactoryGetPQMode_Sharpness(int tv_source_input, int pq_mode)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    source_input_param.source_port = CTvin::Tvin_GetSourcePortBySourceType(source_input_param.source_type);

    return tvFactoryGetPQParam(source_input_param, pq_mode, SHARPNESS);
}

int CVpp::GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    params->en = tvGetColorTemperatureParams(Tempmode, EN);
    params->r_post_offset = tvGetColorTemperatureParams(Tempmode, POST_OFFSET_R);
    params->g_post_offset = tvGetColorTemperatureParams(Tempmode, POST_OFFSET_G);
    params->b_post_offset = tvGetColorTemperatureParams(Tempmode, POST_OFFSET_B);
    params->r_gain = tvGetColorTemperatureParams(Tempmode, GAIN_R);
    params->g_gain = tvGetColorTemperatureParams(Tempmode, GAIN_G);
    params->b_gain = tvGetColorTemperatureParams(Tempmode, GAIN_B);
    params->r_pre_offset = tvGetColorTemperatureParams(Tempmode, PRE_OFFSET_R);
    params->g_pre_offset = tvGetColorTemperatureParams(Tempmode, PRE_OFFSET_G);
    params->b_pre_offset = tvGetColorTemperatureParams(Tempmode, PRE_OFFSET_B);

    return 0;
}

int CVpp::SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    return tvSetColorTemperatureParams(Tempmode, params);
}


int CVpp::FactorySetColorTemp_Rgain(int source_type, int colortemp_mode, int rgain)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, GAIN_R, rgain);
}

int CVpp::FactorySaveColorTemp_Rgain(int source_type __unused, int colortemp_mode, int rgain)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, GAIN_R, rgain);
}

int CVpp::FactoryGetColorTemp_Rgain(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, GAIN_R);
}

int CVpp::FactorySetColorTemp_Ggain(int source_type, int colortemp_mode, int ggain)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, GAIN_G, ggain);
}

int CVpp::FactorySaveColorTemp_Ggain(int source_type __unused, int colortemp_mode, int ggain)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, GAIN_G, ggain);
}

int CVpp::FactoryGetColorTemp_Ggain(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, GAIN_G);
}

int CVpp::FactorySetColorTemp_Bgain(int source_type, int colortemp_mode, int bgain)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, GAIN_B, bgain);
}

int CVpp::FactorySaveColorTemp_Bgain(int source_type __unused, int colortemp_mode, int bgain)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, GAIN_B, bgain);
}

int CVpp::FactoryGetColorTemp_Bgain(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, GAIN_B);
}

int CVpp::FactorySetColorTemp_Roffset(int source_type, int colortemp_mode, int roffset)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_R, roffset);
}

int CVpp::FactorySaveColorTemp_Roffset(int source_type __unused, int colortemp_mode, int roffset)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_R, roffset);
}

int CVpp::FactoryGetColorTemp_Roffset(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_R);
}

int CVpp::FactorySetColorTemp_Goffset(int source_type, int colortemp_mode, int goffset)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_G, goffset);
}

int CVpp::FactorySaveColorTemp_Goffset(int source_type __unused, int colortemp_mode, int goffset)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_G, goffset);
}

int CVpp::FactoryGetColorTemp_Goffset(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_G);
}

int CVpp::FactorySetColorTemp_Boffset(int source_type, int colortemp_mode, int boffset)
{
    return tvFactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_B, boffset);
}

int CVpp::FactorySaveColorTemp_Boffset(int source_type __unused, int colortemp_mode, int boffset)
{
    return tvFactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_B, boffset);
}

int CVpp::FactoryGetColorTemp_Boffset(int source_type __unused, int colortemp_mode)
{
    return tvFactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_B);
}

int CVpp::FactoryGetTestPattern(void)
{
    unsigned char data = VPP_TEST_PATTERN_NONE;
    SSMReadTestPattern(&data);
    return data;
}

int CVpp::FactoryResetPQMode(void)
{
    return tvFactoryResetPQMode();
}

int CVpp::FactoryResetNonlinear(void)
{
    return tvFactoryResetNonlinear();
}

int CVpp::FactoryResetColorTemp(void)
{
    return tvFactoryResetColorTemp();
}

int CVpp::FactorySetParamsDefault(void)
{
    return tvFactorySetParamsDefault();
}

int CVpp::FactorySetDDRSSC(int step)
{
    if (step < 0 || step > 5) {
        return -1;
    }

    return SSMSaveDDRSSC(step);
}

int CVpp::FactoryGetDDRSSC(void)
{
    unsigned char data = 0;
    SSMReadDDRSSC(&data);
    return data;
}

int CVpp::SetLVDSSSC(int step)
{
    int ret = -1;
    if (step > 4)
        step = 4;
    ret = TvMisc_SetLVDSSSC(step);
    return ret;
}

int CVpp::FactorySetLVDSSSC(int step)
{
    int ret = -1;
    unsigned char data[2] = {0, 0};
    char cmd_tmp_1[128];
    int value = 0, panel_idx = 0, tmp = 0;
    const char *PanelIdx;
    if (step > 3)
        step = 3;

    PanelIdx = config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );
    panel_idx = strtoul(PanelIdx, NULL, 10);
    LOGD ("%s, panel_idx = %x", __FUNCTION__, panel_idx);
    SSMReadLVDSSSC(data);

    //every 2 bits represent one panel, use 2 byte store 8 panels
    value = (data[1] << 8) | data[0];
    step = step & 0x03;
    panel_idx = panel_idx * 2;
    tmp = 3 << panel_idx;
    value = (value & (~tmp)) | (step << panel_idx);
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    LOGD ("%s, tmp = %x, save value = %x", __FUNCTION__, tmp, value);

    SetLVDSSSC(step);
    return SSMSaveLVDSSSC(data);
}

int CVpp::FactoryGetLVDSSSC(void)
{
    unsigned char data[2] = {0, 0};
    int value = 0, panel_idx = 0;
    const char *PanelIdx = config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );

    panel_idx = strtoul(PanelIdx, NULL, 10);
    SSMReadLVDSSSC(data);
    value = (data[1] << 8) | data[0];
    value = (value >> (2 * panel_idx)) & 0x03;
    LOGD ("%s, panel_idx = %x, value= %d", __FUNCTION__, panel_idx, value);
    return value;
}

noline_params_t CVpp::FactoryGetNolineParams(int type, int source_type)
{
    int ret = -1;
    noline_params_t noline_params;
    source_input_param_t source_input_param;

    source_input_param.source_type = (tv_source_input_type_t)source_type;
    noline_params.osd0 = tvFactoryGetNolineParams(source_input_param, type, OSD_0);
    noline_params.osd25 = tvFactoryGetNolineParams(source_input_param, type, OSD_25);
    noline_params.osd50 = tvFactoryGetNolineParams(source_input_param, type, OSD_50);
    noline_params.osd75 = tvFactoryGetNolineParams(source_input_param, type, OSD_75);
    noline_params.osd100 = tvFactoryGetNolineParams(source_input_param, type, OSD_100);

    return noline_params;
}

int CVpp::FactorySetNolineParams(int type, int tv_source_input, noline_params_t noline_params)
{
    int ret = 0;
    source_input_param_t source_input_param;
    source_input_param.source_input = (tv_source_input_t)tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    ret |= tvFactorySetNolineParams(source_input_param, type, noline_params);
    return ret;
}

int CVpp::FactorySetOverscan(int source_type, int fmt, int trans_fmt, tvin_cutwin_t cutwin_t)
{
    int ret = 0;
    source_input_param_t source_input_param;
    source_input_param.source_type = (tv_source_input_type_t)source_type;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;

    ret |= tvFactorySetOverscanParam(source_input_param, cutwin_t);

    return ret;
}

tvin_cutwin_t CVpp::FactoryGetOverscan(int source_type, int fmt, is_3d_type_t is3d, int trans_fmt)
{
    source_input_param_t source_input_param;
    tvin_cutwin_t cutwin_t;
    source_input_param.source_type = (tv_source_input_type_t)source_type;
    source_input_param.sig_fmt = (tvin_sig_fmt_t)fmt;
    source_input_param.trans_fmt = (tvin_trans_fmt_t)trans_fmt;
    source_input_param.is3d = is3d;

    cutwin_t.he = tvFactoryGetOverscanParam(source_input_param, CUTWIN_HE);
    cutwin_t.hs = tvFactoryGetOverscanParam(source_input_param, CUTWIN_HS);
    cutwin_t.ve = tvFactoryGetOverscanParam(source_input_param, CUTWIN_VE);
    cutwin_t.vs = tvFactoryGetOverscanParam(source_input_param, CUTWIN_VS);

    return cutwin_t;
}

int CVpp::FactorySetGamma(tcon_gamma_table_t *gamma_r, tcon_gamma_table_t *gamma_g, tcon_gamma_table_t *gamma_b)
{
    int gamma_r_value = gamma_r->data[0];
    int gamma_g_value = gamma_g->data[0];
    int gamma_b_value = gamma_b->data[0];
    LOGD("%s", __FUNCTION__);
    return tvFactorySetGamma(gamma_r_value, gamma_g_value, gamma_b_value);
}

int CVpp::VPPSSMFacRestoreDefault()
{
    tvFactorySSMRestore();
    VPP_SSMRestorToDefault(0, true);

    return 0;
}

void CVpp::video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab)
{
    signed short ma = (signed short) (cos((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);
    signed short mb = (signed short) (sin((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);

    if (ma > 511) {
        ma = 511;
    }

    if (ma < -512) {
        ma = -512;
    }

    if (mb > 511) {
        mb = 511;
    }

    if (mb < -512) {
        mb = -512;
    }

    *mab = ((ma & 0x3ff) << 16) | (mb & 0x3ff);
}

void CVpp::video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab)
{
    signed long temp = *mab;
    signed int ma = (signed int) ((temp << 6) >> 22);
    signed int mb = (signed int) ((temp << 22) >> 22);
    signed int sat16 = (signed int) ((sqrt(
                                          ((float) ma * (float) ma + (float) mb * (float) mb) / 65536.0) - 1.0) * 128.0);
    signed int hue16 = (signed int) (atan((float) mb / (float) ma) * 128.0 / PI);

    if (sat16 > 127) {
        sat16 = 127;
    }

    if (sat16 < -128) {
        sat16 = -128;
    }

    if (hue16 > 127) {
        hue16 = 127;
    }

    if (hue16 < -128) {
        hue16 = -128;
    }

    *sat = (signed char) sat16;
    *hue = (signed char) hue16;
}

int CVpp::VPP_SetGrayPattern(int value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 255) {
        value = 255;
    }
    value = value << 16 | 0x8080;

    LOGD("VPP_SetGrayPattern /sys/class/video/test_screen : %x", value);
    FILE *fp = fopen("/sys/class/video/test_screen", "w");
    if (fp == NULL) {
        LOGE("Open /sys/classs/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "0x%x", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_GetGrayPattern()
{
    int value;
    int ret;

    LOGD("VPP_GetGrayPattern /sys/class/video/test_screen");
    FILE *fp = fopen("/sys/class/video/test_screen", "r+");
    if (fp == NULL) {
        LOGE("Open /sys/class/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    ret = fscanf(fp, "%d", &value);
    if (ret == -1)
    {
        return -1;
    }
    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        value = value >> 16;
        if (value > 255) {
            value = 255;
        }
        return value;
    }
}

int CVpp::VPP_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
    char set_str[32];

    LOGD("VPP_SetVideoCrop /sys/class/video/crop : %d %d %d %d##", Voffset0, Hoffset0, Voffset1, Hoffset1);
    int fd = open("/sys/class/video/crop", O_RDWR);
    if (fd < 0) {
        LOGE("Open /sys/class/video/crop error(%s)!\n", strerror(errno));
        return -1;
    }

    memset(set_str, 0, 32);
    sprintf(set_str, "%d %d %d %d", Voffset0, Hoffset0, Voffset1, Hoffset1);
    write(fd, set_str, strlen(set_str));
    close(fd);

    return 0;
}

int CVpp::VPP_SetScalerPathSel(const unsigned int value)
{
    FILE *fp = NULL;

    fp = fopen(SYS_VIDEO_SCALER_PATH_SEL, "w");
    LOGD("VPP_SetScalerPathSel %s : %d", SYS_VIDEO_SCALER_PATH_SEL, value);
    if (fp == NULL) {
        LOGE("Open %s error(%s)!\n",SYS_VIDEO_SCALER_PATH_SEL, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::VPP_FBCSetColorTemperature(vpp_color_temperature_mode_t temp_mode)
{
    int ret = -1;
    if (fbcIns != NULL) {
        ret = fbcIns->fbcSetEyeProtection(COMM_DEV_SERIAL, 1/*GetEyeProtectionMode()*/);
        ret |= fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, temp_mode);
    }
    return ret;
}

int CVpp::VPP_FBCColorTempBatchSet(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
    switch (Tempmode) {
    case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
        mode = 1;   //COLOR_TEMP_STD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_WARM:
        mode = 2;   //COLOR_TEMP_WARM
        break;
    case VPP_COLOR_TEMPERATURE_MODE_COLD:
        mode = 0;  //COLOR_TEMP_COLD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_USER:
        mode = 3;   //COLOR_TEMP_USER
        break;
    default:
        break;
    }
    r_gain = (params.r_gain * 255) / 2047; // u1.10, range 0~2047, default is 1024 (1.0x)
    g_gain = (params.g_gain * 255) / 2047;
    b_gain = (params.b_gain * 255) / 2047;
    r_offset = (params.r_post_offset + 1024) * 255 / 2047; // s11.0, range -1024~+1023, default is 0
    g_offset = (params.g_post_offset + 1024) * 255 / 2047;
    b_offset = (params.b_post_offset + 1024) * 255 / 2047;
    LOGD ( "~colorTempBatchSet##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_WB_Batch(COMM_DEV_SERIAL, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        return 0;
    }

    return -1;
}

int CVpp::VPP_FBCColorTempBatchGet(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
    switch (Tempmode) {
    case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
        mode = 1;   //COLOR_TEMP_STD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_WARM:
        mode = 2;   //COLOR_TEMP_WARM
        break;
    case VPP_COLOR_TEMPERATURE_MODE_COLD:
        mode = 0;  //COLOR_TEMP_COLD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_USER:
        mode = 3;   //COLOR_TEMP_USER
        break;
    default:
        break;
    }

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_WB_Batch(COMM_DEV_SERIAL, mode, &r_gain, &g_gain, &b_gain, &r_offset, &g_offset, &b_offset);
        LOGD ( "~colorTempBatchGet##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

        params->r_gain = (r_gain * 2047) / 255;
        params->g_gain = (g_gain * 2047) / 255;
        params->b_gain = (b_gain * 2047) / 255;
        params->r_post_offset = (r_offset * 2047) / 255 - 1024;
        params->g_post_offset = (g_offset * 2047) / 255 - 1024;
        params->b_post_offset = (b_offset * 2047) / 255 - 1024;
        return 0;
    }

    return -1;
}

int CVpp::VPP_SSMRestorToDefault(int id, bool resetAll)
{
    int i = 0, tmp_val = 0;
    int tmp_panorama_nor = 0, tmp_panorama_full = 0;

    if (resetAll || VPP_DATA_USER_NATURE_SWITCH_START == id)
        SSMSaveUserNatureLightSwitch(1);

    if (resetAll || SSM_RW_UI_GRHPHY_BACKLIGHT_START == id)
        SSMSaveGraphyBacklight(100);

    if (resetAll || VPP_DATA_POS_DBC_START == id)
        SSMSaveDBCStart(0);

    if (resetAll || VPP_DATA_POS_DNLP_START == id)
        SSMSaveDnlpStart(0); //0: ON,1: OFF,default is on

    if (resetAll || VPP_DATA_APL_START == id)
        SSMSaveAPL(30);

    if (resetAll || VPP_DATA_APL2_START == id)
        SSMSaveAPL2(30);

    if (resetAll || VPP_DATA_BD_START == id)
        SSMSaveBD(30);

    if (resetAll || VPP_DATA_BP_START == id)
        SSMSaveBP(30);

    if (resetAll || VPP_DATA_POS_FBC_ELECMODE_START == id)
        SSMSaveFBCELECmodeVal(11);

    if (resetAll || VPP_DATA_POS_FBC_BACKLIGHT_START == id)
        SSMSaveFBCN360BackLightVal(10);

    if (resetAll || VPP_DATA_POS_FBC_COLORTEMP_START == id)
        SSMSaveFBCN360ColorTempVal(1); // standard colortemp

    if (resetAll || VPP_DATA_POS_FBC_N310_COLORTEMP_START == id)
        SSMSaveFBCN310ColorTempVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START == id)
        SSMSaveFBCN310LightsensorVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_DREAMPANEL_START == id)
        SSMSaveFBCN310Dream_PanelVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MULTI_PQ_START == id)
        SSMSaveFBCN310MULT_PQVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MEMC_START == id)
        SSMSaveFBCN310MEMCVal(2);

    if (resetAll || VPP_DATA_POS_FBC_N310_BACKLIGHT_START == id)
        SSMSaveFBCN310BackLightVal(254);

    tmp_panorama_nor = VPP_PANORAMA_MODE_NORMAL;
    tmp_panorama_full = VPP_PANORAMA_MODE_FULL;
    for (i = 0; i < SOURCE_MAX; i++) {
        if (resetAll || VPP_DATA_POS_PANORAMA_START == id) {
            if (i == SOURCE_TYPE_HDMI) {
                SSMSavePanoramaStart(i, tmp_panorama_full);
            } else {
                SSMSavePanoramaStart(i, tmp_panorama_nor);
            }
        }
    }

    return 0;
}

int CVpp::TV_SSMRecovery()
{
    int ret = 0 ;
    ret = tvSSMRecovery();
    ret |= VPP_SSMRestorToDefault(0, true);

    return ret;
}

int CVpp::VPP_SetPLLValues (tv_source_input_t tv_source_input, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt)
{
    source_input_param_t source_input_param;
    source_input_param.source_input = tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType(tv_source_input);
    source_input_param.source_port = source_port;
    source_input_param.sig_fmt = sig_fmt;

    return tvSetPLLValues(source_input_param);
}

int CVpp::VPP_SetCVD2Values (tv_source_input_t tv_source_input, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt)

{
    source_input_param_t source_input_param;
    source_input_param.source_input = tv_source_input;
    source_input_param.source_type = CTvin::Tvin_SourceInputToSourceInputType(tv_source_input);
    source_input_param.source_port = source_port;
    source_input_param.sig_fmt = sig_fmt;

    return tvSetCVD2Values(source_input_param);
}

int CVpp::VPP_GetSSMStatus (void)
{
    return tvGetSSMStatus();
}


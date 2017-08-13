/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_VPP_H
#define _C_VPP_H
#include "../tvin/CTvin.h"
#include "fbcutils/CFbcCommunication.h"
#include <tvutils.h>

#define GLOBAL_OGO_FORMAT_FLAG  0x6688
#define RGB_FORMAT          0
#define YCbCr_422_FORMAT    1
#define YCbCr_444_FORMAT    2
#define PQ_USER_DATA_FROM_E2P   0
#define PQ_USER_DATA_FROM_DB    1

#define VPP_PANEL_BACKLIGHT_DEV_PATH   "/sys/class/aml_bl/power"
#define BACKLIGHT_BRIGHTNESS           "/sys/class/backlight/aml-bl/brightness"
#define DI_NR2_ENABLE                  "/sys/module/di/parameters/nr2_en"
#define AMVECM_PC_MODE                 "/sys/class/amvecm/pc_mode"

// default backlight value 10%
#define DEFAULT_BACKLIGHT_BRIGHTNESS 10

#define MODE_VPP_3D_DISABLE     0x00000000
#define MODE_VPP_3D_ENABLE      0x00000001
#define MODE_VPP_3D_AUTO        0x00000002
#define MODE_VPP_3D_LR          0x00000004
#define MODE_VPP_3D_TB          0x00000008
#define MODE_VPP_3D_LA          0x00000010
#define MODE_VPP_3D_FA         0x00000020
#define MODE_VPP_3D_LR_SWITCH   0x00000100
#define MODE_VPP_3D_TO_2D_L     0x00000200
#define MODE_VPP_3D_TO_2D_R     0x00000400

typedef enum vpp_panorama_mode_e {
    VPP_PANORAMA_MODE_FULL,
    VPP_PANORAMA_MODE_NORMAL,
    VPP_PANORAMA_MODE_MAX,
} vpp_panorama_mode_t;

typedef enum vpp_dream_panel_e {
    VPP_DREAM_PANEL_OFF,
    VPP_DREAM_PANEL_LIGHT,
    VPP_DREAM_PANEL_SCENE,
    VPP_DREAM_PANEL_FULL,
    VPP_DREAM_PANEL_DEMO,
    VPP_DREAM_PANEL_MAX,
} vpp_dream_panel_t;


class CVpp {
public:
    CVpp();
    ~CVpp();
    int Vpp_Init ( const char *pq_db_path, bool hdmiOutFbc);
    int Vpp_ResetLastVppSettingsSourceType(void);
    int LoadVppLdimRegs();
    int LoadVppSettings ( tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_GetVppConfig();
    int SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    vpp_picture_mode_t GetPQMode ( tv_source_input_t tv_source_input );
    int SavePQMode ( vpp_picture_mode_t pq_mode,  tv_source_input_t tv_source_input );
    int Vpp_GetPQModeValue ( tv_source_input_t, vpp_picture_mode_t, vpp_pq_para_t * );
    void enableMonitorMode(bool enable);
    int SetBrightness ( int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetBrightness ( tv_source_input_t tv_source_input );
    int SaveBrightness (int value, tv_source_input_t tv_source_input);
    int SetSaturation ( int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetSaturation ( tv_source_input_t tv_source_input );
    int SaveSaturation ( int value, tv_source_input_t tv_source_input );
    int SetColorTemperature ( vpp_color_temperature_mode_t temp_mode, tv_source_input_t tv_source_input, int is_save );
    vpp_color_temperature_mode_t GetColorTemperature ( tv_source_input_t tv_source_input );
    int SaveColorTemperature ( vpp_color_temperature_mode_t temp_mode, tv_source_input_t tv_source_input );

    int SaveNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_t tv_source_input );
    int SetNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt, int is_save );
    vpp_noise_reduction_mode_t GetNoiseReductionMode ( tv_source_input_t tv_source_input );
    int SetSharpness ( int value, tv_source_input_t tv_source_input, int is_enable, is_3d_type_t is3d, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, int is_save );
    int GetSharpness ( tv_source_input_t tv_source_input );
    int SaveSharpness ( int value, tv_source_input_t tv_source_input );
    int SetHue ( int value, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetHue ( tv_source_input_t tv_source_input );
    int SaveHue (int value, tv_source_input_t tv_source_input );
    int SetContrast ( int value, tv_source_input_t tv_source_input,  tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetContrast ( tv_source_input_t tv_source_input );
    int SaveContrast ( int value, tv_source_input_t tv_source_input );
    int SetEyeProtectionMode(tv_source_input_t tv_source_input, int enable);
    int GetEyeProtectionMode();
    int SetDNLP ( tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int SetBaseColorMode ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    vpp_color_basemode_t GetBaseColorMode ( void );
    int SaveBaseColorMode ( vpp_color_basemode_t basemode );
    int SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save);
    int GetGammaValue();
    vpp_display_mode_t GetDisplayMode ( tv_source_input_t tv_source_input );
    int SetBacklight ( int value, tv_source_input_t tv_source_input, int is_save );
    int GetBacklight ( tv_source_input_t tv_source_input );
    int SetBacklightWithoutSave ( int value, tv_source_input_t tv_source_input );
    int SaveBacklight ( int value, tv_source_input_t tv_source_input );
    int FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness );
    int FactoryGetPQMode_Brightness ( int tv_source_input, int pq_mode );
    int FactorySetPQMode_Contrast ( int tv_source_input, int pq_mode, int contrast );
    int FactoryGetPQMode_Contrast ( int tv_source_input, int pq_mode );
    int FactorySetPQMode_Saturation ( int tv_source_input, int pq_mode, int saturation );
    int FactoryGetPQMode_Saturation ( int source_type, int pq_mode );
    int FactorySetPQMode_Hue ( int tv_source_input, int pq_mode, int hue );
    int FactoryGetPQMode_Hue ( int tv_source_input, int pq_mode );
    int FactorySetPQMode_Sharpness ( int tv_source_input, int pq_mode, int sharpness );
    int FactoryGetPQMode_Sharpness ( int tv_source_input, int pq_mode );
    int FactorySetColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain );
    int FactorySaveColorTemp_Rgain ( int source_type, int colortemp_mode, int rgain );
    int FactoryGetColorTemp_Rgain ( int source_type, int colortemp_mode );
    int FactorySetColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain );
    int FactorySaveColorTemp_Ggain ( int source_type, int colortemp_mode, int ggain );
    int FactoryGetColorTemp_Ggain ( int source_type, int colortemp_mode );
    int FactorySetColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain );
    int FactorySaveColorTemp_Bgain ( int source_type, int colortemp_mode, int bgain );
    int FactoryGetColorTemp_Bgain ( int source_type, int colortemp_mode );
    int FactorySetColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset );
    int FactorySaveColorTemp_Roffset ( int source_type, int colortemp_mode, int roffset );
    int FactoryGetColorTemp_Roffset ( int source_type, int colortemp_mode );
    int FactorySetColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset );
    int FactorySaveColorTemp_Goffset ( int source_type, int colortemp_mode, int goffset );
    int FactoryGetColorTemp_Goffset ( int source_type, int colortemp_mode );
    int FactorySetColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset );
    int FactorySaveColorTemp_Boffset ( int source_type, int colortemp_mode, int boffset );
    int FactoryGetColorTemp_Boffset ( int source_type, int colortemp_mode );
    //int Tv_FactorySaveRGBDatatoAllSrc ( int source_type, int colortemp_mode );
    int FactoryGetTestPattern ( void );
    int FactoryResetPQMode ( void );
    int FactoryResetColorTemp ( void );
    int FactorySetParamsDefault ( void );
    int FactorySetDDRSSC ( int step );
    int FactoryGetDDRSSC();
    int FactorySetLVDSSSC ( int step );
    int FactoryGetLVDSSSC();
    int SetLVDSSSC(int step);
    int FactorySetNolineParams ( int type, int tv_source_input, noline_params_t noline_params );
    noline_params_t FactoryGetNolineParams ( int type, int source_type );
    int FactorySetOverscan ( int source_type, int fmt, int trans_fmt, tvin_cutwin_t cutwin_t );
    tvin_cutwin_t FactoryGetOverscan ( int source_type, int fmt, is_3d_type_t is3d, int trans_fmt );
    int FactorySetGamma(tcon_gamma_table_t gamma_r, tcon_gamma_table_t gamma_g, tcon_gamma_table_t gamma_b);

    int VPPSSMFacRestoreDefault();

    int GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int SetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    int SaveColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );

    int FactoryResetNonlinear();
    tvin_cutwin_t GetOverscan ( tv_source_input_t tv_source_input, tvin_sig_fmt_t fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int VPP_SetPLLValues (tv_source_input_t tv_source_input, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt);
    int VPP_SetCVD2Values (tv_source_input_t tv_source_input, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt);
    //api
    int VPP_SetVideoCrop ( int Voffset0, int Hoffset0, int Voffset1, int Hoffset1 );
    int VPP_SetNonLinearFactor ( int value );
    int VPP_SetGrayPattern(int value);
    int VPP_GetGrayPattern();
    int VPP_SetBackLight_Switch ( int value );
    int VPP_GetBackLight_Switch ( void );
    int VPP_SetScalerPathSel (const unsigned int value);
    int VPP_SSMRestorToDefault(int id, bool resetAll);
    int TV_SSMRecovery();
    int VPP_GetSSMStatus (void);
    static CVpp *getInstance();
private:
    //
    int VPP_SetCMRegisterMap ( struct cm_regmap_s *pRegMap );
    void video_set_saturation_hue ( signed char saturation, signed char hue, signed long *mab );
    void video_get_saturation_hue ( signed char *sat, signed char *hue, signed long *mab );
    int VPP_SetBackLightLevel ( int value );
    int VPP_FBCColorTempBatchGet(vpp_color_temperature_mode_t, tcon_rgb_ogo_t *);
    int VPP_FBCColorTempBatchSet(vpp_color_temperature_mode_t, tcon_rgb_ogo_t);
    int VPP_FBCSetColorTemperature(vpp_color_temperature_mode_t);

    static CVpp *mInstance;
    bool mHdmiOutFbc;
    CFbcCommunication *fbcIns;
    //cfg
    bool mbVppCfg_backlight_reverse;
    bool mbVppCfg_backlight_init;
    bool mbVppCfg_pqmode_depend_bklight;
};
#endif

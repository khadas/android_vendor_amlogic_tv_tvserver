#ifndef __TV_SETTING_CFG__H__
#define __TV_SETTING_CFG__H__
#include "../tvin/CTvin.h"

static const int SSM_MEM_START = 0;
static const int SSM_MEM_OFFSET = 10;

/** for chksum start */
static const int SSM_RES0_START = SSM_MEM_START + 0;
static const int CHKSUM_PROJECT_ID_OFFSET                   =      (SSM_RES0_START + 0);
static const int CHKSUM_MAC_ADDRESS_OFFSET                  =      (SSM_RES0_START + 2);
static const int CHKSUM_HDCP_KEY_OFFSET                     =      (SSM_RES0_START + 4);
static const int CHKSUM_BARCODE_OFFSET                      =      (SSM_RES0_START + 6);
// reserved0 section
static const int SSM_RSV_W_CHARACTER_CHAR_START = SSM_RES0_START + 0x0A;

static const int SSM_RES0_END = SSM_RSV_W_CHARACTER_CHAR_START + 1 + SSM_MEM_OFFSET;
static const int SSM_RES0_LEN = SSM_RES0_END - SSM_RES0_START;
/** for chksum end */

static const int SSM_CR_START = SSM_RES0_END;
static const int SSM_CR_LEN = 1536;
static const int SSM_CR_END = SSM_CR_START + SSM_CR_LEN;

//Read & write section
static const int SSM_MARK_01_START = SSM_CR_END + 0;
static const int SSM_MARK_01_LEN = 1;

static const int SSM_RW_ATV_START = SSM_MARK_01_START + SSM_MARK_01_LEN;
static const int SSM_RW_ATV_LEN = 0;

static const int SSM_MARK_02_START = SSM_RW_ATV_START + SSM_RW_ATV_LEN;
static const int SSM_MARK_02_LEN = 1;

static const int SSM_MARK_03_START = SSM_MARK_02_START + SSM_MARK_02_LEN;
static const int SSM_MARK_03_LEN = 1;
static const int SSM_MARK_END = SSM_MARK_03_START + 1 + SSM_MEM_OFFSET;

//Mark r/w values
static const int SSM_MARK_01_VALUE = 0xDD;
static const int SSM_MARK_02_VALUE = 0x88;
static const int SSM_MARK_03_VALUE = 0xCC;


/** Read & write misc section start */
static const int SSM_RW_MISC_START = SSM_MARK_END;
//Factory Burn Mode Flag, 1 byte
static const int SSM_RW_FBMF_START                          = (SSM_RW_MISC_START + 0);
//using default hdcp key flag, 1 byte
static const int SSM_RW_DEF_HDCP_START                      = (SSM_RW_FBMF_START + 1);
//Power on/off channel type, 1 byte
static const int SSM_RW_POWER_CHANNEL_START                 = (SSM_RW_DEF_HDCP_START + 1);
//Last tv select input source type, 1 byte
static const int SSM_RW_LAST_SOURCE_INPUT_START             = (SSM_RW_POWER_CHANNEL_START + 1);
//system language, 1 byte
static const int SSM_RW_SYS_LANGUAGE_START                  = (SSM_RW_LAST_SOURCE_INPUT_START + 1);
//aging mode, 1 byte
static const int SSM_RW_AGING_MODE_START                    = (SSM_RW_SYS_LANGUAGE_START + 1);
//panel type, 1 byte
static const int SSM_RW_PANEL_TYPE_START                    = (SSM_RW_AGING_MODE_START + 1);
//power on music switch, 1 byte
static const int SSM_RW_POWER_ON_MUSIC_SWITCH_START         = (SSM_RW_PANEL_TYPE_START + 1);
//power on music volume, 1 byte
static const int SSM_RW_POWER_ON_MUSIC_VOL_START            = (SSM_RW_POWER_ON_MUSIC_SWITCH_START + 1);
//system sleep timer, 4 bytes
static const int SSM_RW_SYS_SLEEP_TIMER_START               = (SSM_RW_POWER_ON_MUSIC_VOL_START + 1);
//tv input source parental control, 4 bytes
static const int SSM_RW_INPUT_SRC_PARENTAL_CTL_START        = (SSM_RW_SYS_SLEEP_TIMER_START + 4);
//parental control switch, 4 bytes
static const int SSM_RW_PARENTAL_CTL_SWITCH_START           = (SSM_RW_INPUT_SRC_PARENTAL_CTL_START + 4);
//parental control pass word, 16 byte
static const int SSM_RW_PARENTAL_CTL_PASSWORD_START         = (SSM_RW_PARENTAL_CTL_SWITCH_START + 1);
static const int SSM_RW_SEARCH_NAVIGATE_FLAG_START          = (SSM_RW_PARENTAL_CTL_PASSWORD_START + 16);
static const int SSM_RW_INPUT_NUMBER_LIMIT_START            = (SSM_RW_SEARCH_NAVIGATE_FLAG_START + 1);
static const int SSM_RW_SERIAL_ONOFF_FLAG_START             = (SSM_RW_INPUT_NUMBER_LIMIT_START + 1);
static const int SSM_RW_STANDBY_MODE_FLAG_START             = (SSM_RW_SERIAL_ONOFF_FLAG_START + 1);
static const int SSM_RW_HDMIEQ_MODE_START                   = (SSM_RW_STANDBY_MODE_FLAG_START + 1);
static const int SSM_RW_LOGO_ON_OFF_FLAG_START              = (SSM_RW_HDMIEQ_MODE_START + 1);
static const int SSM_RW_HDMIINTERNAL_MODE_START             = (SSM_RW_LOGO_ON_OFF_FLAG_START +  1);
static const int SSM_RW_DISABLE_3D_START                    = (SSM_RW_HDMIINTERNAL_MODE_START +  4);
static const int SSM_RW_GLOBAL_OGO_ENABLE_START             = (SSM_RW_DISABLE_3D_START +  1);
static const int SSM_RW_LOCAL_DIMING_START                  = (SSM_RW_GLOBAL_OGO_ENABLE_START +  1);
static const int SSM_RW_VDAC_2D_START                       = (SSM_RW_LOCAL_DIMING_START + 1);
static const int SSM_RW_VDAC_3D_START                       = (SSM_RW_VDAC_2D_START + 2);
static const int SSM_RW_NON_STANDARD_START                  = (SSM_RW_VDAC_3D_START + 2);
static const int SSM_RW_ADB_SWITCH_START                    = (SSM_RW_NON_STANDARD_START + 2);
static const int SSM_RW_SERIAL_CMD_SWITCH_START             = (SSM_RW_ADB_SWITCH_START + 1);
static const int SSM_RW_CA_BUFFER_SIZE_START                = (SSM_RW_SERIAL_CMD_SWITCH_START + 1);
static const int SSM_RW_NOISE_GATE_THRESHOLD_START          = (SSM_RW_CA_BUFFER_SIZE_START + 2);
static const int SSM_RW_DTV_TYPE_START                      = (SSM_RW_NOISE_GATE_THRESHOLD_START +  1);
static const int SSM_RW_UI_GRHPHY_BACKLIGHT_START           = (SSM_RW_DTV_TYPE_START + 1);
static const int SSM_RW_FASTSUSPEND_FLAG_START              = (SSM_RW_UI_GRHPHY_BACKLIGHT_START + 1);
static const int SSM_RW_BLACKOUT_ENABLE_START               = (SSM_RW_FASTSUSPEND_FLAG_START + 1);
static const int SSM_RW_PANEL_ID_START                      = (SSM_RW_BLACKOUT_ENABLE_START + 1);

static const int SSM_RW_MISC_LAST     = SSM_RW_PANEL_ID_START;
static const int SSM_RW_MISC_LAST_LEN = 1;
static const int SSM_RW_MISC_END = SSM_RW_MISC_LAST + SSM_RW_MISC_LAST_LEN + SSM_MEM_OFFSET;
static const int SSM_RW_MISC_LEN = SSM_RW_MISC_END - SSM_RW_MISC_START;
/** Read & write misc section end */


/** Audio data section start */
static const int SSM_RW_AUDIO_START = SSM_RW_MISC_END;
static const int SSM_AUD_MASTR_VOLUME_VAL                   = (SSM_RW_AUDIO_START + 0);//1 byte
static const int SSM_AUD_BALANCE_VAL                        = (SSM_AUD_MASTR_VOLUME_VAL + 1);
static const int SSM_AUD_SUPPERBASS_VOLUME_VAL              = (SSM_AUD_BALANCE_VAL + 1);
static const int SSM_AUD_SUPPERBASS_SWITCH                  = (SSM_AUD_SUPPERBASS_VOLUME_VAL + 1);
static const int SSM_AUD_SRS_SURROUND_SWITCH                = (SSM_AUD_SUPPERBASS_SWITCH + 1);
static const int SSM_AUD_SRS_DIALOG_CLARITY_SWITCH          = (SSM_AUD_SRS_SURROUND_SWITCH + 1);
static const int SSM_AUD_SRS_TRUEBASS_SWITCH                = (SSM_AUD_SRS_DIALOG_CLARITY_SWITCH + 1);
static const int SSM_AUD_BASS_VOLUME_VAL                    = (SSM_AUD_SRS_TRUEBASS_SWITCH + 1);
static const int SSM_AUD_TREBLE_VOLUME_VAL                  = (SSM_AUD_BASS_VOLUME_VAL + 1);
static const int SSM_AUD_SOUND_MODE_VAL                     = (SSM_AUD_TREBLE_VOLUME_VAL + 1);
static const int SSM_AUD_WALL_EFFCT_SWITCH                  = (SSM_AUD_SOUND_MODE_VAL + 1);
static const int SSM_AUD_SPDIF_SWITCH                       = (SSM_AUD_WALL_EFFCT_SWITCH + 1);
static const int SSM_AUD_SPDIF_MODE_VAL                     = (SSM_AUD_SPDIF_SWITCH + 1);
static const int SSM_AUD_EQ_MODE_VAL                        = (SSM_RW_AUDIO_START + 32);
static const int SSM_AUD_EQ_GAIN                            = (SSM_AUD_EQ_MODE_VAL + 1);
static const int SSM_AUD_NOLINE_POINTS                      = (SSM_AUD_EQ_GAIN + 16);
static const int SSM_AUD_DBX_TV_SON                         = (SSM_AUD_NOLINE_POINTS + 2);
static const int SSM_AUD_DBX_TV_VAL                         = (SSM_AUD_NOLINE_POINTS + 3);
static const int SSM_AUD_DBX_TV_SUR                         = (SSM_AUD_NOLINE_POINTS + 4);
static const int SSM_AUD_AVOUT_MUTE                         = (SSM_AUD_DBX_TV_SUR + 2);
static const int SSM_AUD_SPIDF_MUTE                         = (SSM_AUD_AVOUT_MUTE + 1);
static const int SSM_AUD_DRC_ONOFF                          = (SSM_AUD_SPIDF_MUTE + 1);

static const int SSM_RW_AUDIO_LAST     = SSM_AUD_DRC_ONOFF;
static const int SSM_RW_AUDIO_LAST_LEN = 1;
static const int SSM_RW_AUDIO_END = SSM_RW_AUDIO_LAST + SSM_RW_AUDIO_LAST_LEN + SSM_MEM_OFFSET;
static const int SSM_RW_AUDIO_LEN = SSM_RW_AUDIO_END - SSM_RW_AUDIO_START;
/** Audio data section end */


/** VPP Data start */
static const int SSM_RW_VPP_START = SSM_RW_AUDIO_END;
//ColorDemoMode 1byte
static const int VPP_DATA_POS_COLOR_DEMO_MODE_START         = (SSM_RW_VPP_START + 0);
//ColorBaseMode 1byte
static const int VPP_DATA_POS_COLOR_BASE_MODE_START         = (VPP_DATA_POS_COLOR_DEMO_MODE_START + 1);
//TestPattern 1byte
static const int VPP_DATA_POS_TEST_PATTERN_START            = (VPP_DATA_POS_COLOR_BASE_MODE_START + 1);
//DDR SSC 1byte
static const int VPP_DATA_POS_DDR_SSC_START                 = (VPP_DATA_POS_TEST_PATTERN_START + 1);
//LVDS SSC 1byte
static const int VPP_DATA_POS_LVDS_SSC_START                = (VPP_DATA_POS_DDR_SSC_START + 1);
//dream panel 1byte
static const int VPP_DATA_POS_DREAM_PANEL_START             = (VPP_DATA_POS_LVDS_SSC_START + 2);
//Backlight reverse 1byte
static const int VPP_DATA_POS_BACKLIGHT_REVERSE_START       = (VPP_DATA_POS_DREAM_PANEL_START + 1);
//Brightness 1*7=7byte
static const int VPP_DATA_POS_BRIGHTNESS_START              = (VPP_DATA_POS_BACKLIGHT_REVERSE_START + 1);
//Contrast 1*7=7byte
static const int VPP_DATA_POS_CONTRAST_START                = (VPP_DATA_POS_BRIGHTNESS_START + 1*(SOURCE_TYPE_MAX));
//Saturation 1*7=7byte
static const int VPP_DATA_POS_SATURATION_START              = (VPP_DATA_POS_CONTRAST_START + 1*(SOURCE_TYPE_MAX));
//Hue 1*7=7byte
static const int VPP_DATA_POS_HUE_START                     = (VPP_DATA_POS_SATURATION_START + 1*(SOURCE_TYPE_MAX));
//Sharpness 1*7=7byte
static const int VPP_DATA_POS_SHARPNESS_START               = (VPP_DATA_POS_HUE_START + 1*(SOURCE_TYPE_MAX));
//ColorTemperature 1*7=7byte
static const int VPP_DATA_POS_COLOR_TEMP_START              = (VPP_DATA_POS_SHARPNESS_START + 1*(SOURCE_TYPE_MAX));
//NoiseReduction 1*7=7byte
static const int VPP_DATA_POS_NOISE_REDUCTION_START         = (VPP_DATA_POS_COLOR_TEMP_START + 1*(SOURCE_TYPE_MAX));
//SceneMode 1byte
static const int VPP_DATA_POS_SCENE_MODE_START              = (VPP_DATA_POS_NOISE_REDUCTION_START + 1*(SOURCE_TYPE_MAX));
//PictureMode 1*7=7byte
static const int VPP_DATA_POS_PICTURE_MODE_START            = (VPP_DATA_POS_SCENE_MODE_START + 1);
//DisplayMode 1*7=7byte
static const int VPP_DATA_POS_DISPLAY_MODE_START            = (VPP_DATA_POS_PICTURE_MODE_START + 1*(SOURCE_TYPE_MAX));
//Backlight 1*7=7byte
static const int VPP_DATA_POS_BACKLIGHT_START               = (VPP_DATA_POS_DISPLAY_MODE_START + 1*(SOURCE_TYPE_MAX));
//RGB_Gain_R 4byte
static const int VPP_DATA_POS_RGB_GAIN_R_START              = (VPP_DATA_POS_BACKLIGHT_START + 1*(SOURCE_TYPE_MAX));
//RGB_Gain_G 4byte
static const int VPP_DATA_POS_RGB_GAIN_G_START              = (VPP_DATA_POS_RGB_GAIN_R_START + sizeof(int));
//RGB_Gain_B 4byte
static const int VPP_DATA_POS_RGB_GAIN_B_START              = (VPP_DATA_POS_RGB_GAIN_B_START + sizeof(int));
//RGB_Post_Offset_R 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_R_START       = (VPP_DATA_POS_RGB_POST_OFFSET_R_START + sizeof(int));
//RGB_Post_Offset_G 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_G_START       = (VPP_DATA_POS_RGB_POST_OFFSET_R_START + sizeof(int));
//RGB_Post_Offset_B 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_B_START       = (VPP_DATA_POS_RGB_POST_OFFSET_G_START + sizeof(int));
//dbc_Enable 1byte
static const int VPP_DATA_POS_DBC_START                     = (VPP_DATA_POS_RGB_POST_OFFSET_B_START  + sizeof(int));
//project id 1byte
static const int VPP_DATA_PROJECT_ID_START                  = (VPP_DATA_POS_DBC_START  + 1);
//dnlp 1byte
static const int VPP_DATA_POS_DNLP_START                    = (VPP_DATA_PROJECT_ID_START  + 1);
//panorama 1*7 = 7byte
static const int VPP_DATA_POS_PANORAMA_START                = (VPP_DATA_POS_DNLP_START  + 1);
//APL 1 byte
static const int VPP_DATA_APL_START                         = (VPP_DATA_POS_PANORAMA_START + 1*(SOURCE_TYPE_MAX));
//APL2 1 byte
static const int VPP_DATA_APL2_START                        = (VPP_DATA_APL_START + 1);
//BD 1 byte
static const int VPP_DATA_BD_START                          = (VPP_DATA_APL2_START + 1);
//BP 1 byte
static const int VPP_DATA_BP_START                          = (VPP_DATA_BD_START + 1);
//Factory RGB 3*6 = 18byte
static const int VPP_DATA_RGB_START                         = (VPP_DATA_BP_START + 1);
//COLOR_SPACE 1 byte
static const int VPP_DATA_COLOR_SPACE_START                 = (VPP_DATA_RGB_START + 18);
static const int VPP_DATA_USER_NATURE_SWITCH_START          = (VPP_DATA_COLOR_SPACE_START + 1);
//gamma value 1 byte
static const int VPP_DATA_GAMMA_VALUE_START                 = (VPP_DATA_USER_NATURE_SWITCH_START + 1);
//dbc backlight enable 1byte
static const int VPP_DATA_DBC_BACKLIGHT_START               = (VPP_DATA_GAMMA_VALUE_START + 1);
//dbc backlight standard 1byte
static const int VPP_DATA_DBC_STANDARD_START                = (VPP_DATA_DBC_BACKLIGHT_START + 1);
//dbc backlight enable 1byte
static const int VPP_DATA_DBC_ENABLE_START                  = (VPP_DATA_DBC_STANDARD_START + 1);

//fbc Backlight 1 byte
static const int VPP_DATA_POS_FBC_BACKLIGHT_START           = (VPP_DATA_DBC_ENABLE_START + 1);
//fbc Elecmode 1 byte
static const int VPP_DATA_POS_FBC_ELECMODE_START            = (VPP_DATA_POS_FBC_BACKLIGHT_START + 1);
//fbc colortemp 1 byte
static const int VPP_DATA_POS_FBC_COLORTEMP_START           = (VPP_DATA_POS_FBC_ELECMODE_START + 1);
static const int VPP_DATA_POS_FBC_N310_BACKLIGHT_START      = (VPP_DATA_POS_FBC_COLORTEMP_START + 1);
//fbc colortemp 1 byte N310
static const int VPP_DATA_POS_FBC_N310_COLORTEMP_START      = (VPP_DATA_POS_FBC_N310_BACKLIGHT_START + 1);
//fbc lightsensor 1 byte N310
static const int VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START    = (VPP_DATA_POS_FBC_N310_COLORTEMP_START + 1);
//fbc MEMC 1 byte N310
static const int VPP_DATA_POS_FBC_N310_MEMC_START           = (VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START + 1);
//fbc DREAMPANEL 1 byte N310
static const int VPP_DATA_POS_FBC_N310_DREAMPANEL_START     = (VPP_DATA_POS_FBC_N310_MEMC_START + 1);
//fbc Multi_pq 1 byte N310
static const int VPP_DATA_POS_FBC_N310_MULTI_PQ_START       = (VPP_DATA_POS_FBC_N310_DREAMPANEL_START + 1);
//fbc Multi_pq 1 byte N310
static const int VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START = (VPP_DATA_POS_FBC_N310_MULTI_PQ_START + 1);
//bluetooth volume 1 byte N311
static const int VPP_DATA_POS_N311_BLUETOOTH_VAL_START      = (VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START + 1);
//eye protection mode, 1 byte
static const int VPP_DATA_EYE_PROTECTION_MODE_START         = (VPP_DATA_POS_N311_BLUETOOTH_VAL_START + 1);

static const int SSM_RW_VPP_LAST     = VPP_DATA_EYE_PROTECTION_MODE_START;
static const int SSM_RW_VPP_LAST_LEN = 1;
static const int SSM_RW_VPP_END = SSM_RW_VPP_LAST + SSM_RW_VPP_LAST_LEN + SSM_MEM_OFFSET;
static const int SSM_RW_VPP_LEN = SSM_RW_VPP_END - SSM_RW_VPP_START;
/** VPP Data end */


/** Tvin data section start */
static const int SSM_RW_VDIN_START = SSM_RW_VPP_END;
//SourceInput 1byte
static const int TVIN_DATA_POS_SOURCE_INPUT_START           =      (SSM_RW_VDIN_START + 0);
//CVBS Std 1byte
static const int TVIN_DATA_CVBS_STD_START                   =      (TVIN_DATA_POS_SOURCE_INPUT_START + 1);
//3DMode 1byte
static const int TVIN_DATA_POS_3D_MODE_START                =      (TVIN_DATA_CVBS_STD_START + 1);
//3DLRSwitch 1byte
static const int TVIN_DATA_POS_3D_LRSWITCH_START            =      (TVIN_DATA_POS_3D_MODE_START + 1);
//3DDepth 1byte
static const int TVIN_DATA_POS_3D_DEPTH_START               =      (TVIN_DATA_POS_3D_LRSWITCH_START + 1);
//3DTo2D 1byte
static const int TVIN_DATA_POS_3D_TO2D_START                =      (TVIN_DATA_POS_3D_DEPTH_START + 1);
//3DTo2DNEW 1byte
static const int TVIN_DATA_POS_3D_TO2DNEW_START             =      (TVIN_DATA_POS_3D_TO2D_START + 1);

static const int SSM_RW_VDIN_LAST      = TVIN_DATA_POS_3D_TO2DNEW_START;
static const int SSM_RW_VDIN_LAST_LEN  = 1;
static const int SSM_RW_VDIN_END = SSM_RW_VDIN_LAST + SSM_RW_VDIN_LAST_LEN + SSM_MEM_OFFSET;
static const int SSM_RW_VDIN_LEN = SSM_RW_VDIN_END - SSM_RW_VDIN_START;
/** Tvin data section end */


/** HDMI_EDID_1.4or2.0 start */
static const int SSM_RW_CUSTOMER_DATA_START  = SSM_RW_VDIN_END;
static const int CUSTOMER_DATA_POS_HDMI1_EDID_START         =      (SSM_RW_CUSTOMER_DATA_START + 0);
static const int CUSTOMER_DATA_POS_HDMI2_EDID_START         =      (CUSTOMER_DATA_POS_HDMI1_EDID_START + 1);
static const int CUSTOMER_DATA_POS_HDMI3_EDID_START         =      (CUSTOMER_DATA_POS_HDMI2_EDID_START + 1);
//HDMI_HDCP_KEY_SWITCHER
static const int CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START =      (CUSTOMER_DATA_POS_HDMI3_EDID_START + 1);

static const int SSM_RW_CUSTOMER_DATA_LAST     = CUSTOMER_DATA_POS_HDMI_HDCP_SWITCHER_START;
static const int SSM_RW_CUSTOMER_DATA_LAST_LEN = 1;
static const int SSM_RW_CUSTOMER_DATA_END = SSM_RW_CUSTOMER_DATA_LAST + SSM_RW_CUSTOMER_DATA_LAST_LEN + SSM_MEM_OFFSET;
static const int SSM_RW_CUSTOMER_DATA_LEN = SSM_RW_CUSTOMER_DATA_END - SSM_RW_CUSTOMER_DATA_START;
/** HDMI_1_EDID_1.4or2.0 end */


#endif

#ifndef __TV_SETTING_CFG__H__
#define __TV_SETTING_CFG__H__
#include "../tvin/CTvin.h"

//Sections
static const int SSM_RES0_START = 0;
static const int SSM_RES0_LEN = 32;
static const int SSM_CR_START = SSM_RES0_START + SSM_RES0_LEN;
static const int SSM_CR_LEN = 1536;
static const int SSM_RW_START = SSM_CR_START + SSM_CR_LEN;
static const int SSM_RW_LEN = 2048;
static const int SSM_RES1_START = SSM_RW_START + SSM_RW_LEN;
static const int SSM_RES1_LEN = 64;

// reserved0 section
static const int SSM_RSV_W_CHARACTER_CHAR_START = SSM_RES0_START + 0x0A;
static const int SSM_RSV_W_CHARACTER_CHAR_LEN = 1;

//Read & write section
static const int SSM_MARK_01_START = SSM_RW_START + 0;
static const int SSM_MARK_01_LEN = 1;

static const int SSM_RW_AUDIO_START = SSM_MARK_01_START + SSM_MARK_01_LEN;
static const int SSM_RW_AUDIO_LEN = 192;

static const int SSM_RW_VPP_START = SSM_RW_AUDIO_START + SSM_RW_AUDIO_LEN;
static const int SSM_RW_VPP_LEN = 230;

static const int SSM_RW_VDIN_START = SSM_RW_VPP_START + SSM_RW_VPP_LEN;
static const int SSM_RW_VDIN_LEN = 826;

static const int SSM_RW_ATV_START = SSM_RW_VDIN_START + SSM_RW_VDIN_LEN;
static const int SSM_RW_ATV_LEN = 0;

static const int SSM_RW_MISC_START = SSM_RW_ATV_START + SSM_RW_ATV_LEN;
static const int SSM_RW_MISC_LEN = 86;

static const int SSM_RW_CUSTOMER_DATA_START = SSM_RW_MISC_START + SSM_RW_MISC_LEN;
static const int SSM_RW_CUSTOMER_DATA_LEN = 100;

static const int SSM_MARK_02_START = SSM_RW_CUSTOMER_DATA_START + SSM_RW_CUSTOMER_DATA_LEN;
static const int SSM_MARK_02_LEN = 1;

static const int SSM_RW_TEST_START = SSM_MARK_02_START + SSM_MARK_02_LEN;
static const int SSM_RW_TEST_LEN = 3;

static const int SSM_MARK_03_START = SSM_RW_TEST_START + SSM_RW_TEST_LEN;
static const int SSM_MARK_03_LEN = 1;

//Mark r/w values
static const int SSM_MARK_01_VALUE = 0xDD;
static const int SSM_MARK_02_VALUE = 0x88;
static const int SSM_MARK_03_VALUE = 0xCC;


//Read & write section(1824 Bytes)
static const int SSM_RW_AUDIO_END                           =      (SSM_RW_AUDIO_START + SSM_RW_AUDIO_LEN);
static const int SSM_RW_VPP_END                             =      (SSM_RW_VPP_START + SSM_RW_VPP_LEN);
static const int SSM_RW_VDIN_END                            =      (SSM_RW_VDIN_START + SSM_RW_VDIN_LEN);
static const int SSM_RW_ATV_END                             =      (SSM_RW_ATV_START + SSM_RW_ATV_LEN);
static const int SSM_RW_MISC_END                            =      (SSM_RW_MISC_START + SSM_RW_MISC_LEN);

static const int SSM_CR_END                                 =      (SSM_CR_START + SSM_CR_LEN);

//Read & write misc section
static const int SSM_RW_FBMF_START                          =      (SSM_RW_MISC_START); //Factory Burn Mode Flag
static const int SSM_RW_FBMF_LEN                            =      (1);
static const int SSM_RW_DEF_HDCP_START                      =      (SSM_RW_FBMF_START + SSM_RW_FBMF_LEN); //using default hdcp key flag
static const int SSM_RW_DEF_HDCP_LEN                        =      (1);
static const int SSM_RW_POWER_CHANNEL_START                 =      (SSM_RW_DEF_HDCP_START + SSM_RW_DEF_HDCP_LEN); //Power on/off channel type
static const int SSM_RW_POWER_CHANNEL_LEN                   =      (1);
static const int SSM_RW_LAST_SOURCE_INPUT_START             =      (SSM_RW_POWER_CHANNEL_START + SSM_RW_POWER_CHANNEL_LEN); //Last tv select input source type
static const int SSM_RW_LAST_SOURCE_INPUT_LEN               =      (1);
static const int SSM_RW_SYS_LANGUAGE_START                  =      (SSM_RW_LAST_SOURCE_INPUT_START + SSM_RW_LAST_SOURCE_INPUT_LEN); //system language
static const int SSM_RW_SYS_LANGUAGE_LEN                    =      (1);
static const int SSM_RW_AGING_MODE_START                    =      (SSM_RW_SYS_LANGUAGE_START + SSM_RW_SYS_LANGUAGE_LEN); //aging mode
static const int SSM_RW_AGING_MODE_LEN                      =      (1);
static const int SSM_RW_PANEL_TYPE_START                    =      (SSM_RW_AGING_MODE_START + SSM_RW_AGING_MODE_LEN); //panel type
static const int SSM_RW_PANEL_TYPE_LEN                      =      (1);
static const int SSM_RW_POWER_ON_MUSIC_SWITCH_START         =      (SSM_RW_PANEL_TYPE_START + SSM_RW_PANEL_TYPE_LEN); //power on music switch
static const int SSM_RW_POWER_ON_MUSIC_SWITCH_LEN           =      (1);
static const int SSM_RW_POWER_ON_MUSIC_VOL_START            =      (SSM_RW_POWER_ON_MUSIC_SWITCH_START + SSM_RW_POWER_ON_MUSIC_SWITCH_LEN); //power on music volume
static const int SSM_RW_POWER_ON_MUSIC_VOL_LEN              =      (1);
static const int SSM_RW_SYS_SLEEP_TIMER_START               =      (SSM_RW_POWER_ON_MUSIC_VOL_START + SSM_RW_POWER_ON_MUSIC_VOL_LEN); //system sleep timer
static const int SSM_RW_SYS_SLEEP_TIMER_LEN                 =      (4);
static const int SSM_RW_INPUT_SRC_PARENTAL_CTL_START        =      (SSM_RW_SYS_SLEEP_TIMER_START + SSM_RW_SYS_SLEEP_TIMER_LEN); //tv input source parental control
static const int SSM_RW_INPUT_SRC_PARENTAL_CTL_LEN          =      (4);
static const int SSM_RW_PARENTAL_CTL_SWITCH_START           =      (SSM_RW_INPUT_SRC_PARENTAL_CTL_START + SSM_RW_INPUT_SRC_PARENTAL_CTL_LEN); //parental control switch
static const int SSM_RW_PARENTAL_CTL_SWITCH_LEN             =      (1);
static const int SSM_RW_PARENTAL_CTL_PASSWORD_START         =      (SSM_RW_PARENTAL_CTL_SWITCH_START + SSM_RW_PARENTAL_CTL_SWITCH_LEN); //parental control pass word
static const int SSM_RW_PARENTAL_CTL_PASSWORD_LEN           =      (16);
static const int SSM_RW_SEARCH_NAVIGATE_FLAG_START          =      (SSM_RW_PARENTAL_CTL_PASSWORD_START + SSM_RW_PARENTAL_CTL_PASSWORD_LEN);
static const int SSM_RW_SEARCH_NAVIGATE_FLAG_LEN            =      (1);
static const int SSM_RW_INPUT_NUMBER_LIMIT_START            =      (SSM_RW_SEARCH_NAVIGATE_FLAG_START + SSM_RW_SEARCH_NAVIGATE_FLAG_LEN);
static const int SSM_RW_INPUT_NUMBER_LIMIT_LEN              =      (1);
static const int SSM_RW_SERIAL_ONOFF_FLAG_START             =      (SSM_RW_INPUT_NUMBER_LIMIT_START + SSM_RW_INPUT_NUMBER_LIMIT_LEN);
static const int SSM_RW_SERIAL_ONOFF_FLAG_LEN               =      (1);
static const int SSM_RW_STANDBY_MODE_FLAG_START             =      (SSM_RW_SERIAL_ONOFF_FLAG_START + SSM_RW_SERIAL_ONOFF_FLAG_LEN);
static const int SSM_RW_STANDBY_MODE_FLAG_LEN               =      (1);
static const int SSM_RW_HDMIEQ_MODE_START                   =      (SSM_RW_STANDBY_MODE_FLAG_START + SSM_RW_STANDBY_MODE_FLAG_LEN);
static const int SSM_RW_HDMIEQ_MODE_LEN                     =      (1);
static const int SSM_RW_LOGO_ON_OFF_FLAG_START              =      (SSM_RW_HDMIEQ_MODE_START + SSM_RW_HDMIEQ_MODE_LEN);
static const int SSM_RW_LOGO_ON_OFF_FLAG_LEN                =      (1);
static const int SSM_RW_HDMIINTERNAL_MODE_START             =      (SSM_RW_LOGO_ON_OFF_FLAG_START +  SSM_RW_LOGO_ON_OFF_FLAG_LEN);
static const int SSM_RW_HDMIINTERNAL_MODE_LEN               =      (4);
static const int SSM_RW_DISABLE_3D_START                    =      (SSM_RW_HDMIINTERNAL_MODE_START +  SSM_RW_HDMIINTERNAL_MODE_LEN);
static const int SSM_RW_DISABLE_3D_LEN                      =      (1);
static const int SSM_RW_GLOBAL_OGO_ENABLE_START             =      (SSM_RW_DISABLE_3D_START +  SSM_RW_DISABLE_3D_LEN);
static const int SSM_RW_GLOBAL_OGO_ENABLE_LEN               =      (1);
static const int SSM_RW_LOCAL_DIMING_START                  =      (SSM_RW_GLOBAL_OGO_ENABLE_START +  SSM_RW_GLOBAL_OGO_ENABLE_LEN);
static const int SSM_RW_LOCAL_DIMING_LEN                    =      (1);
static const int SSM_RW_VDAC_2D_START                       =      (SSM_RW_LOCAL_DIMING_START + SSM_RW_LOCAL_DIMING_LEN);
static const int SSM_RW_VDAC_2D_LEN                         =      (2);
static const int SSM_RW_VDAC_3D_START                       =      (SSM_RW_VDAC_2D_START + SSM_RW_VDAC_2D_LEN);
static const int SSM_RW_VDAC_3D_LEN                         =      (2);

static const int SSM_RW_NON_STANDARD_START                  =      (SSM_RW_VDAC_3D_START + SSM_RW_VDAC_3D_LEN);
static const int SSM_RW_NON_STANDARD_LEN                    =      (2);
static const int SSM_RW_ADB_SWITCH_START                    =      (SSM_RW_NON_STANDARD_START + SSM_RW_NON_STANDARD_LEN);
static const int SSM_RW_ADB_SWITCH_LEN                      =      (1);
static const int SSM_RW_SERIAL_CMD_SWITCH_START             =      (SSM_RW_ADB_SWITCH_START + SSM_RW_ADB_SWITCH_LEN);
static const int SSM_RW_SERIAL_CMD_SWITCH_LEN               =      (1);

static const int SSM_RW_CA_BUFFER_SIZE_START                =      (SSM_RW_SERIAL_CMD_SWITCH_START + SSM_RW_SERIAL_CMD_SWITCH_LEN);
static const int SSM_RW_CA_BUFFER_SIZE_LEN                  =      (2);

static const int SSM_RW_NOISE_GATE_THRESHOLD_START          =      (SSM_RW_CA_BUFFER_SIZE_START + SSM_RW_CA_BUFFER_SIZE_LEN);
static const int SSM_RW_NOISE_GATE_THRESHOLD_LEN            =      (1);

static const int SSM_RW_DTV_TYPE_START                      =      (SSM_RW_NOISE_GATE_THRESHOLD_START +  SSM_RW_NOISE_GATE_THRESHOLD_LEN);
static const int SSM_RW_DTV_TYPE_LEN                        =      (1);

static const int SSM_RW_UI_GRHPHY_BACKLIGHT_START           =      (SSM_RW_DTV_TYPE_START + SSM_RW_DTV_TYPE_LEN);
static const int SSM_RW_UI_GRHPHY_BACKLIGHT_LEN             =      (1);

static const int SSM_RW_FASTSUSPEND_FLAG_START              =      (SSM_RW_UI_GRHPHY_BACKLIGHT_START + SSM_RW_UI_GRHPHY_BACKLIGHT_LEN);
static const int SSM_RW_FASTSUSPEND_FLAG_LEN                =      (1);

static const int SSM_RW_BLACKOUT_ENABLE_START              =      (SSM_RW_FASTSUSPEND_FLAG_START + SSM_RW_FASTSUSPEND_FLAG_LEN);
static const int SSM_RW_BLACKOUT_ENABLE__LEN                =      (1);

static const int SSM_RW_PANEL_ID_START                      =      (SSM_RW_BLACKOUT_ENABLE_START + SSM_RW_BLACKOUT_ENABLE__LEN);
static const int SSM_RW_PANEL_ID_LEN                        =      (1);
//Audio data section
static const int SSM_AUD_MASTR_VOLUME_VAL                   =      (SSM_RW_AUDIO_START + 0);
static const int SSM_AUD_BALANCE_VAL                        =      (SSM_AUD_MASTR_VOLUME_VAL + 1);
static const int SSM_AUD_SUPPERBASS_VOLUME_VAL              =      (SSM_AUD_BALANCE_VAL + 1);
static const int SSM_AUD_SUPPERBASS_SWITCH                  =      (SSM_AUD_SUPPERBASS_VOLUME_VAL + 1);
static const int SSM_AUD_SRS_SURROUND_SWITCH                =      (SSM_AUD_SUPPERBASS_SWITCH + 1);
static const int SSM_AUD_SRS_DIALOG_CLARITY_SWITCH          =      (SSM_AUD_SRS_SURROUND_SWITCH + 1);
static const int SSM_AUD_SRS_TRUEBASS_SWITCH                =      (SSM_AUD_SRS_DIALOG_CLARITY_SWITCH + 1);
static const int SSM_AUD_BASS_VOLUME_VAL                    =      (SSM_AUD_SRS_TRUEBASS_SWITCH + 1);
static const int SSM_AUD_TREBLE_VOLUME_VAL                  =      (SSM_AUD_BASS_VOLUME_VAL + 1);
static const int SSM_AUD_SOUND_MODE_VAL                     =      (SSM_AUD_TREBLE_VOLUME_VAL + 1);
static const int SSM_AUD_WALL_EFFCT_SWITCH                  =      (SSM_AUD_SOUND_MODE_VAL + 1);
static const int SSM_AUD_SPDIF_SWITCH                       =      (SSM_AUD_WALL_EFFCT_SWITCH + 1);
static const int SSM_AUD_SPDIF_MODE_VAL                     =      (SSM_AUD_SPDIF_SWITCH + 1);
static const int SSM_AUD_EQ_MODE_VAL                        =      (SSM_RW_AUDIO_START + 32);
static const int SSM_AUD_EQ_GAIN                            =      (SSM_AUD_EQ_MODE_VAL + 1);
static const int SSM_AUD_NOLINE_POINTS                      =      (SSM_AUD_EQ_GAIN + 16);
static const int SSM_AUD_DBX_TV_SON               =      (SSM_AUD_NOLINE_POINTS + 2);
static const int SSM_AUD_DBX_TV_VAL               =      (SSM_AUD_NOLINE_POINTS + 2 + 1);
static const int SSM_AUD_DBX_TV_SUR               =      (SSM_AUD_NOLINE_POINTS + 2 + 2);
static const int SSM_AUD_DBX_TV_SIZE = 3;
static const int SSM_AUD_AVOUT_MUTE                         =      (SSM_AUD_DBX_TV_SUR + 2);
static const int SSM_AUD_SPIDF_MUTE                         =      (SSM_AUD_AVOUT_MUTE + 1);
static const int SSM_AUD_DRC_ONOFF                          =      (SSM_AUD_SPIDF_MUTE + 1);

//VPP Data (115 bytes)
//ColorDemoMode 1byte
static const int VPP_DATA_POS_COLOR_DEMO_MODE_START         =      (SSM_RW_VPP_START + 0);
static const int VPP_DATA_POS_COLOR_DEMO_MODE_SIZE          =      (1);
static const int VPP_DATA_POS_COLOR_DEMO_MODE_END           =      (VPP_DATA_POS_COLOR_DEMO_MODE_START + VPP_DATA_POS_COLOR_DEMO_MODE_SIZE);
//ColorBaseMode 1byte
static const int VPP_DATA_POS_COLOR_BASE_MODE_START         =      (VPP_DATA_POS_COLOR_DEMO_MODE_END + 0);
static const int VPP_DATA_POS_COLOR_BASE_MODE_SIZE          =      (1);
static const int VPP_DATA_POS_COLOR_BASE_MODE_END           =      (VPP_DATA_POS_COLOR_BASE_MODE_START + VPP_DATA_POS_COLOR_BASE_MODE_SIZE);
//TestPattern 1byte
static const int VPP_DATA_POS_TEST_PATTERN_START            =      (VPP_DATA_POS_COLOR_BASE_MODE_END + 0);
static const int VPP_DATA_POS_TEST_PATTERN_SIZE             =      (1);
static const int VPP_DATA_POS_TEST_PATTERN_END              =      (VPP_DATA_POS_TEST_PATTERN_START + VPP_DATA_POS_TEST_PATTERN_SIZE);
//DDR SSC 1byte
static const int VPP_DATA_POS_DDR_SSC_START                 =      (VPP_DATA_POS_TEST_PATTERN_END + 0);
static const int VPP_DATA_POS_DDR_SSC_SIZE                  =      (1);
static const int VPP_DATA_POS_DDR_SSC_END                   =      (VPP_DATA_POS_DDR_SSC_START + VPP_DATA_POS_DDR_SSC_SIZE);
//LVDS SSC 1byte
static const int VPP_DATA_POS_LVDS_SSC_START                =      (VPP_DATA_POS_DDR_SSC_END + 0);
static const int VPP_DATA_POS_LVDS_SSC_SIZE                 =      (2);
static const int VPP_DATA_POS_LVDS_SSC_END                  =      (VPP_DATA_POS_LVDS_SSC_START + VPP_DATA_POS_LVDS_SSC_SIZE);
//dream panel 1byte
static const int VPP_DATA_POS_DREAM_PANEL_START             =      (VPP_DATA_POS_LVDS_SSC_END + 0);
static const int VPP_DATA_POS_DREAM_PANEL_SIZE              =      (1);
static const int VPP_DATA_POS_DREAM_PANEL_END               =      (VPP_DATA_POS_DREAM_PANEL_START + VPP_DATA_POS_DREAM_PANEL_SIZE);
//Backlight reverse 1byte
static const int VPP_DATA_POS_BACKLIGHT_REVERSE_START       =      (VPP_DATA_POS_DREAM_PANEL_END + 0);
static const int VPP_DATA_POS_BACKLIGHT_REVERSE_SIZE        =      (1);
static const int VPP_DATA_POS_BACKLIGHT_REVERSE_END         =      (VPP_DATA_POS_BACKLIGHT_REVERSE_START + VPP_DATA_POS_BACKLIGHT_REVERSE_SIZE);
//Brightness 1*7=7byte
static const int VPP_DATA_POS_BRIGHTNESS_START              =      (VPP_DATA_POS_BACKLIGHT_REVERSE_END + 0);
static const int VPP_DATA_POS_BRIGHTNESS_SIZE               =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_BRIGHTNESS_END                =      (VPP_DATA_POS_BRIGHTNESS_START + VPP_DATA_POS_BRIGHTNESS_SIZE);
//Contrast 1*7=7byte
static const int VPP_DATA_POS_CONTRAST_START                =      (VPP_DATA_POS_BRIGHTNESS_END + 0);
static const int VPP_DATA_POS_CONTRAST_SIZE                 =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_CONTRAST_END                  =      (VPP_DATA_POS_CONTRAST_START + VPP_DATA_POS_CONTRAST_SIZE);
//Saturation 1*7=7byte
static const int VPP_DATA_POS_SATURATION_START              =      (VPP_DATA_POS_CONTRAST_END + 0);
static const int VPP_DATA_POS_SATURATION_SIZE               =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_SATURATION_END                =      (VPP_DATA_POS_SATURATION_START + VPP_DATA_POS_SATURATION_SIZE);
//Hue 1*7=7byte
static const int VPP_DATA_POS_HUE_START                     =      (VPP_DATA_POS_SATURATION_END + 0);
static const int VPP_DATA_POS_HUE_SIZE                      =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_HUE_END                       =      (VPP_DATA_POS_HUE_START + VPP_DATA_POS_HUE_SIZE);
//Sharpness 1*7=7byte
static const int VPP_DATA_POS_SHARPNESS_START               =      (VPP_DATA_POS_HUE_END + 0);
static const int VPP_DATA_POS_SHARPNESS_SIZE                =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_SHARPNESS_END                 =      (VPP_DATA_POS_SHARPNESS_START + VPP_DATA_POS_SHARPNESS_SIZE);
//ColorTemperature 1*7=7byte
static const int VPP_DATA_POS_COLOR_TEMP_START              =      (VPP_DATA_POS_SHARPNESS_END + 0);
static const int VPP_DATA_POS_COLOR_TEMP_SIZE               =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_COLOR_TEMP_END                =      (VPP_DATA_POS_COLOR_TEMP_START + VPP_DATA_POS_COLOR_TEMP_SIZE);
//NoiseReduction 1*7=7byte
static const int VPP_DATA_POS_NOISE_REDUCTION_START         =      (VPP_DATA_POS_COLOR_TEMP_END + 0);
static const int VPP_DATA_POS_NOISE_REDUCTION_SIZE          =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_NOISE_REDUCTION_END           =      (VPP_DATA_POS_NOISE_REDUCTION_START + VPP_DATA_POS_NOISE_REDUCTION_SIZE);
//SceneMode 1byte
static const int VPP_DATA_POS_SCENE_MODE_START              =      (VPP_DATA_POS_NOISE_REDUCTION_END + 0);
static const int VPP_DATA_POS_SCENE_MODE_SIZE               =      (1);
static const int VPP_DATA_POS_SCENE_MODE_END                =      (VPP_DATA_POS_SCENE_MODE_START + VPP_DATA_POS_SCENE_MODE_SIZE);
//PictureMode 1*7=7byte
static const int VPP_DATA_POS_PICTURE_MODE_START            =      (VPP_DATA_POS_SCENE_MODE_END + 0);
static const int VPP_DATA_POS_PICTURE_MODE_SIZE             =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_PICTURE_MODE_END              =      (VPP_DATA_POS_PICTURE_MODE_START + VPP_DATA_POS_PICTURE_MODE_SIZE);
//DisplayMode 1*7=7byte
static const int VPP_DATA_POS_DISPLAY_MODE_START            =      (VPP_DATA_POS_PICTURE_MODE_END + 0);
static const int VPP_DATA_POS_DISPLAY_MODE_SIZE             =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_DISPLAY_MODE_END              =      (VPP_DATA_POS_DISPLAY_MODE_START + VPP_DATA_POS_DISPLAY_MODE_SIZE);
//Backlight 1*7=7byte
static const int VPP_DATA_POS_BACKLIGHT_START               =      (VPP_DATA_POS_DISPLAY_MODE_END + 0);
static const int VPP_DATA_POS_BACKLIGHT_SIZE                =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_BACKLIGHT_END                 =      (VPP_DATA_POS_BACKLIGHT_START + VPP_DATA_POS_BACKLIGHT_SIZE);
//RGB_Gain_R 4byte
static const int VPP_DATA_POS_RGB_GAIN_R_START              =      (VPP_DATA_POS_BACKLIGHT_END + 0);
static const int VPP_DATA_POS_RGB_GAIN_R_SIZE               =      (sizeof(int));
static const int VPP_DATA_POS_RGB_GAIN_R_END                =      (VPP_DATA_POS_RGB_GAIN_R_START + VPP_DATA_POS_RGB_GAIN_R_SIZE);
//RGB_Gain_G 4byte
static const int VPP_DATA_POS_RGB_GAIN_G_START              =      (VPP_DATA_POS_RGB_GAIN_R_END + 0);
static const int VPP_DATA_POS_RGB_GAIN_G_SIZE               =      (sizeof(int));
static const int VPP_DATA_POS_RGB_GAIN_G_END                =      (VPP_DATA_POS_RGB_GAIN_G_START + VPP_DATA_POS_RGB_GAIN_G_SIZE);
//RGB_Gain_B 4byte
static const int VPP_DATA_POS_RGB_GAIN_B_START              =      (VPP_DATA_POS_RGB_GAIN_G_END + 0);
static const int VPP_DATA_POS_RGB_GAIN_B_SIZE               =      (sizeof(int));
static const int VPP_DATA_POS_RGB_GAIN_B_END                =      (VPP_DATA_POS_RGB_GAIN_B_START + VPP_DATA_POS_RGB_GAIN_B_SIZE);
//RGB_Post_Offset_R 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_R_START       =      (VPP_DATA_POS_RGB_GAIN_B_END + 0);
static const int VPP_DATA_POS_RGB_POST_OFFSET_R_SIZE        =      (sizeof(int));
static const int VPP_DATA_POS_RGB_POST_OFFSET_R_END         =      (VPP_DATA_POS_RGB_POST_OFFSET_R_START + VPP_DATA_POS_RGB_POST_OFFSET_R_SIZE);
//RGB_Post_Offset_G 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_G_START       =      (VPP_DATA_POS_RGB_POST_OFFSET_R_END + 0);
static const int VPP_DATA_POS_RGB_POST_OFFSET_G_SIZE        =      (sizeof(int));
static const int VPP_DATA_POS_RGB_POST_OFFSET_G_END         =      (VPP_DATA_POS_RGB_POST_OFFSET_G_START + VPP_DATA_POS_RGB_POST_OFFSET_G_SIZE);
//RGB_Post_Offset_B 4byte
static const int VPP_DATA_POS_RGB_POST_OFFSET_B_START       =      (VPP_DATA_POS_RGB_POST_OFFSET_G_END + 0);
static const int VPP_DATA_POS_RGB_POST_OFFSET_B_SIZE        =      (sizeof(int));
static const int VPP_DATA_POS_RGB_POST_OFFSET_B_END         =      (VPP_DATA_POS_RGB_POST_OFFSET_B_START + VPP_DATA_POS_RGB_POST_OFFSET_B_SIZE);
//dbc_Enable 1byte
static const int VPP_DATA_POS_DBC_START                     =      (VPP_DATA_POS_RGB_POST_OFFSET_B_END  + 0);
static const int VPP_DATA_POS_DBC_SIZE                      =      (1);
static const int VPP_DATA_POS_DBC_END                       =      (VPP_DATA_POS_DBC_START + VPP_DATA_POS_DBC_SIZE);
//project id 1byte
static const int VPP_DATA_PROJECT_ID_START                  =      (VPP_DATA_POS_DBC_END  + 0);
static const int VPP_DATA_PROJECT_ID_SIZE                   =      (1);
static const int VPP_DATA_PROJECT_ID_END                    =      (VPP_DATA_PROJECT_ID_START + VPP_DATA_PROJECT_ID_SIZE);
//dnlp 1byte
static const int VPP_DATA_POS_DNLP_START                    =      (VPP_DATA_PROJECT_ID_END  + 0);
static const int VPP_DATA_POS_DNLP_SIZE                     =      (1);
static const int VPP_DATA_POS_DNLP_END                      =      (VPP_DATA_POS_DNLP_START + VPP_DATA_POS_DNLP_SIZE);
//panorama 1*7 = 7byte
static const int VPP_DATA_POS_PANORAMA_START                =      (VPP_DATA_POS_DNLP_END  + 0);
static const int VPP_DATA_POS_PANORAMA_SIZE                 =      (1 * (SOURCE_TYPE_MAX));
static const int VPP_DATA_POS_PANORAMA_END                  =      (VPP_DATA_POS_PANORAMA_START + VPP_DATA_POS_PANORAMA_SIZE);
//APL 1 byte
static const int VPP_DATA_APL_START                         =      (VPP_DATA_POS_PANORAMA_END + 0);
static const int VPP_DATA_APL_SIZE                          =      (1);
static const int VPP_DATA_APL_END                           =      (VPP_DATA_APL_START + VPP_DATA_APL_SIZE);
//APL2 1 byte
static const int VPP_DATA_APL2_START                        =      (VPP_DATA_APL_END + 0);
static const int VPP_DATA_APL2_SIZE                         =      (1);
static const int VPP_DATA_APL2_END                          =      (VPP_DATA_APL2_START + VPP_DATA_APL2_SIZE);
//BD 1 byte
static const int VPP_DATA_BD_START                          =      (VPP_DATA_APL2_END + 0);
static const int VPP_DATA_BD_SIZE                           =      (1);
static const int VPP_DATA_BD_END                            =      (VPP_DATA_BD_START + VPP_DATA_BD_SIZE);
//BP 1 byte
static const int VPP_DATA_BP_START                          =      (VPP_DATA_BD_END + 0);
static const int VPP_DATA_BP_SIZE                           =      (1);
static const int VPP_DATA_BP_END                            =      (VPP_DATA_BP_START + VPP_DATA_BP_SIZE);

//Factory RGB 3*6 = 18byte
static const int VPP_DATA_RGB_START                         =      (VPP_DATA_BP_END + 0);
static const int VPP_DATA_RGB_SIZE                          =      (18);
static const int VPP_DATA_RGB_END                           =      (VPP_DATA_RGB_START + VPP_DATA_RGB_SIZE);

//COLOR_SPACE 1 byte
static const int VPP_DATA_COLOR_SPACE_START                 =      (VPP_DATA_RGB_END + 0);
static const int VPP_DATA_COLOR_SPACE_SIZE                  =      (1);
static const int VPP_DATA_COLOR_SPACE_END                   =      (VPP_DATA_COLOR_SPACE_START + VPP_DATA_COLOR_SPACE_SIZE);

static const int VPP_DATA_USER_NATURE_SWITCH_START          =      (VPP_DATA_COLOR_SPACE_END + 0);
static const int VPP_DATA_USER_NATURE_SWITCH_SIZE           =      (1);
static const int VPP_DATA_USER_NATURE_SWITCH_END            =      (VPP_DATA_USER_NATURE_SWITCH_START + VPP_DATA_USER_NATURE_SWITCH_SIZE);

//gamma value 1 byte
static const int VPP_DATA_GAMMA_VALUE_START                 =      (VPP_DATA_USER_NATURE_SWITCH_END + 0);
static const int VPP_DATA_GAMMA_VALUE_SIZE                  =      (1);
static const int VPP_DATA_GAMMA_VALUE_END                   =      (VPP_DATA_GAMMA_VALUE_START + VPP_DATA_GAMMA_VALUE_SIZE);

//dbc backlight enable 1byte
static const int VPP_DATA_DBC_BACKLIGHT_START               =      (VPP_DATA_GAMMA_VALUE_END + 0);
static const int VPP_DATA_DBC_BACKLIGHT_SIZE                =      (1);
static const int VPP_DATA_DBC_BACKLIGHT_END                 =      (VPP_DATA_DBC_BACKLIGHT_START + VPP_DATA_DBC_BACKLIGHT_SIZE);
//dbc backlight standard 1byte
static const int VPP_DATA_DBC_STANDARD_START                =      (VPP_DATA_DBC_BACKLIGHT_END + 0);
static const int VPP_DATA_DBC_STANDARD_SIZE                 =      (1);
static const int VPP_DATA_DBC_STANDARD_END                  =      (VPP_DATA_DBC_STANDARD_START + VPP_DATA_DBC_STANDARD_SIZE);
//dbc backlight enable 1byte
static const int VPP_DATA_DBC_ENABLE_START                  =      (VPP_DATA_DBC_STANDARD_END + 0);
static const int VPP_DATA_DBC_ENABLE_SIZE                   =      (1);
static const int VPP_DATA_DBC_ENABLE_END                    =      (VPP_DATA_DBC_ENABLE_START + VPP_DATA_DBC_ENABLE_SIZE);

//fbc Backlight 1 byte
static const int VPP_DATA_POS_FBC_BACKLIGHT_START               =      (VPP_DATA_DBC_ENABLE_END + 0);
static const int VPP_DATA_POS_FBC_BACKLIGHT_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_BACKLIGHT_END                 =      (VPP_DATA_POS_FBC_BACKLIGHT_START + VPP_DATA_POS_FBC_BACKLIGHT_SIZE);
//fbc Elecmode 1 byte
static const int VPP_DATA_POS_FBC_ELECMODE_START               =      (VPP_DATA_POS_FBC_BACKLIGHT_END + 0);
static const int VPP_DATA_POS_FBC_ELECMODE_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_ELECMODE_END                 =      (VPP_DATA_POS_FBC_ELECMODE_START + VPP_DATA_POS_FBC_ELECMODE_SIZE);
//fbc colortemp 1 byte
static const int VPP_DATA_POS_FBC_COLORTEMP_START               =      (VPP_DATA_POS_FBC_ELECMODE_END + 0);
static const int VPP_DATA_POS_FBC_COLORTEMP_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_COLORTEMP_END                 =      (VPP_DATA_POS_FBC_COLORTEMP_START + VPP_DATA_POS_FBC_COLORTEMP_SIZE);

static const int VPP_DATA_POS_FBC_N310_BACKLIGHT_START               =      (VPP_DATA_POS_FBC_COLORTEMP_END + 0);
static const int VPP_DATA_POS_FBC_N310_BACKLIGHT_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_BACKLIGHT_END                 =      (VPP_DATA_POS_FBC_N310_BACKLIGHT_START + VPP_DATA_POS_FBC_N310_BACKLIGHT_SIZE);
//fbc colortemp 1 byte N310
static const int VPP_DATA_POS_FBC_N310_COLORTEMP_START               =      (VPP_DATA_POS_FBC_N310_BACKLIGHT_END + 0);
static const int VPP_DATA_POS_FBC_N310_COLORTEMP_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_COLORTEMP_END                 =      (VPP_DATA_POS_FBC_N310_COLORTEMP_START + VPP_DATA_POS_FBC_N310_COLORTEMP_SIZE);
//fbc lightsensor 1 byte N310
static const int VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START               =      (VPP_DATA_POS_FBC_N310_COLORTEMP_END + 0);
static const int VPP_DATA_POS_FBC_N310_LIGHTSENSOR_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_LIGHTSENSOR_END                 =      (VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START + VPP_DATA_POS_FBC_N310_LIGHTSENSOR_SIZE);
//fbc MEMC 1 byte N310
static const int VPP_DATA_POS_FBC_N310_MEMC_START               =      (VPP_DATA_POS_FBC_N310_LIGHTSENSOR_END + 0);
static const int VPP_DATA_POS_FBC_N310_MEMC_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_MEMC_END                 =      (VPP_DATA_POS_FBC_N310_MEMC_START + VPP_DATA_POS_FBC_N310_MEMC_SIZE);
//fbc DREAMPANEL 1 byte N310
static const int VPP_DATA_POS_FBC_N310_DREAMPANEL_START               =      (VPP_DATA_POS_FBC_N310_MEMC_END + 0);
static const int VPP_DATA_POS_FBC_N310_DREAMPANEL_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_DREAMPANEL_END                 =      (VPP_DATA_POS_FBC_N310_DREAMPANEL_START + VPP_DATA_POS_FBC_N310_DREAMPANEL_SIZE);
//fbc Multi_pq 1 byte N310
static const int VPP_DATA_POS_FBC_N310_MULTI_PQ_START               =      (VPP_DATA_POS_FBC_N310_DREAMPANEL_END + 0);
static const int VPP_DATA_POS_FBC_N310_MULTI_PQ_SIZE                =      (1);
static const int VPP_DATA_POS_FBC_N310_MULTI_PQ_END                 =      (VPP_DATA_POS_FBC_N310_MULTI_PQ_START + VPP_DATA_POS_FBC_N310_MULTI_PQ_SIZE);
//fbc Multi_pq 1 byte N310
static const int VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START               =      (VPP_DATA_POS_FBC_N310_MULTI_PQ_END + 0);
static const int VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_SIZE                =      (1);
static const int VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_END                 =      (VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_START + VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_SIZE);
//bluetooth volume 1 byte N311
static const int VPP_DATA_POS_N311_BLUETOOTH_VAL_START               =      (VPP_DATA_POS_N311_VBYONE_SPREAD_SPECTRUM_END + 0);
static const int VPP_DATA_POS_N311_BLUETOOTH_VAL_SIZE                =      (1);
static const int VPP_DATA_POS_N311_BLUETOOTH_VAL_END                 =      (VPP_DATA_POS_N311_BLUETOOTH_VAL_START + VPP_DATA_POS_N311_BLUETOOTH_VAL_SIZE);

//Tvin data section (581 bytes)
//SourceInput 1byte
static const int TVIN_DATA_POS_SOURCE_INPUT_START           =      (SSM_RW_VDIN_START + 0);
static const int TVIN_DATA_POS_SOURCE_INPUT_SIZE            =      (1);
static const int TVIN_DATA_POS_SOURCE_INPUT_END             =      (TVIN_DATA_POS_SOURCE_INPUT_START + TVIN_DATA_POS_SOURCE_INPUT_SIZE);
//CVBS Std 1byte
static const int TVIN_DATA_CVBS_STD_START                   =      (TVIN_DATA_POS_SOURCE_INPUT_END + 0);
static const int TVIN_DATA_CVBS_STD_SIZE                    =      (1);
static const int TVIN_DATA_CVBS_STD_END                     =      (TVIN_DATA_CVBS_STD_START + TVIN_DATA_CVBS_STD_SIZE);
//3DMode 1byte
static const int TVIN_DATA_POS_3D_MODE_START                =      (TVIN_DATA_CVBS_STD_END + 0);
static const int TVIN_DATA_POS_3D_MODE_SIZE                 =      (1);
static const int TVIN_DATA_POS_3D_MODE_END                  =      (TVIN_DATA_POS_3D_MODE_START + TVIN_DATA_POS_3D_MODE_SIZE);
//3DLRSwitch 1byte
static const int TVIN_DATA_POS_3D_LRSWITCH_START            =      (TVIN_DATA_POS_3D_MODE_END + 0);
static const int TVIN_DATA_POS_3D_LRSWITCH_SIZE             =      (1);
static const int TVIN_DATA_POS_3D_LRSWITCH_END              =      (TVIN_DATA_POS_3D_LRSWITCH_START + TVIN_DATA_POS_3D_LRSWITCH_SIZE);
//3DDepth 1byte
static const int TVIN_DATA_POS_3D_DEPTH_START               =      (TVIN_DATA_POS_3D_LRSWITCH_END + 0);
static const int TVIN_DATA_POS_3D_DEPTH_SIZE                =      (1);
static const int TVIN_DATA_POS_3D_DEPTH_END                 =      (TVIN_DATA_POS_3D_DEPTH_START + TVIN_DATA_POS_3D_DEPTH_SIZE);
//3DTo2D 1byte
static const int TVIN_DATA_POS_3D_TO2D_START                =      (TVIN_DATA_POS_3D_DEPTH_END + 0);
static const int TVIN_DATA_POS_3D_TO2D_SIZE                 =      (1);
static const int TVIN_DATA_POS_3D_TO2D_END                  =      (TVIN_DATA_POS_3D_TO2D_START + TVIN_DATA_POS_3D_TO2D_SIZE);
//3DTo2DNEW 1byte
static const int TVIN_DATA_POS_3D_TO2DNEW_START             =      (TVIN_DATA_POS_3D_TO2D_END + 0);
static const int TVIN_DATA_POS_3D_TO2DNEW_SIZE              =      (1);
static const int TVIN_DATA_POS_3D_TO2DNEW_END               =      (TVIN_DATA_POS_3D_TO2DNEW_START + TVIN_DATA_POS_3D_TO2DNEW_SIZE);

static const int CHKSUM_PROJECT_ID_OFFSET                   =      (SSM_RES0_START + 0);
static const int CHKSUM_MAC_ADDRESS_OFFSET                  =      (SSM_RES0_START + 2);
static const int CHKSUM_HDCP_KEY_OFFSET                     =      (SSM_RES0_START + 4);
static const int CHKSUM_BARCODE_OFFSET                      =      (SSM_RES0_START + 6);

#endif

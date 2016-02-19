#include "CTvSettingDeviceFactory.h"
#include "CBlobDeviceE2prom.h"
#include "CBlobDeviceFile.h"

#include "../tvconfig/tvconfig.h"

#include "CTvLog.h"
#include "CTvSettingCfg.h"

#define CC_PRINT_DATA_INFO    (0)


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

/*int CTvSettingDeviceFactory::SSMPrintAllDataInfo() {
#if CC_PRINT_DATA_INFO == 1
    static int gPrintInfoFlag = 0;

    if (gPrintInfoFlag == 1) {
        return 0;
    }

    gPrintInfoFlag = 1;

    SSMPrintDataInfo("Section reserved0", SSM_RESERVED0_START, SSM_RESERVED0_LEN);
    SSMPrintDataInfo("Section HDCP key", SSM_RO_HDCP_KEY_START, SSM_RO_HDCP_KEY_LEN);
    SSMPrintDataInfo("Section MAC address", SSM_RO_MAC_START, SSM_RO_MAC_LEN);
    SSMPrintDataInfo("Section VGA EDID", SSM_RO_VGA_EDID_START, SSM_RO_VGA_EDID_LEN);
    SSMPrintDataInfo("Section HDMI EDID", SSM_RO_HDMI_EDID_START, SSM_RO_HDMI_EDID_LEN);

    SSMPrintDataInfo("Section Mark 01", SSM_MARK_01_START, SSM_MARK_01_LEN);

    SSMPrintDataInfo("Section Audio", SSM_RW_AUDIO_START, SSM_RW_AUDIO_LEN);
    SSMPrintDataInfo("Section Audio Master Volume", SSM_AUD_MASTR_VOLUME_VAL, 1);
    SSMPrintDataInfo("Section Audio Balance Value", SSM_AUD_BALANCE_VAL, 1);
    SSMPrintDataInfo("Section Audio SupperBass Volume", SSM_AUD_SUPPERBASS_VOLUME_VAL, 1);
    SSMPrintDataInfo("Section Audio SupperBass Switch", SSM_AUD_SUPPERBASS_SWITCH, 1);
    SSMPrintDataInfo("Section Audio SRS Surround Switch", SSM_AUD_SRS_SURROUND_SWITCH, 1);
    SSMPrintDataInfo("Section Audio SRS Dialog Clarity Switch", SSM_AUD_SRS_DIALOG_CLARITY_SWITCH, 1);
    SSMPrintDataInfo("Section Audio SRS Trubass switch", SSM_AUD_SRS_TRUEBASS_SWITCH, 1);
    SSMPrintDataInfo("Section Audio Bass Volume", SSM_AUD_BASS_VOLUME_VAL, 1);
    SSMPrintDataInfo("Section Audio Treble Volume", SSM_AUD_TREBLE_VOLUME_VAL, 1);
    SSMPrintDataInfo("Section Audio Sound Mode Value", SSM_AUD_SOUND_MODE_VAL, 1);
    SSMPrintDataInfo("Section Audio Wall effect switch", SSM_AUD_WALL_EFFCT_SWITCH, 1);
    SSMPrintDataInfo("Section Audio spdif switch", SSM_AUD_SPDIF_SWITCH, 1);
    SSMPrintDataInfo("Section Audio spdif mode", SSM_AUD_SPDIF_MODE_VAL, 1);
    SSMPrintDataInfo("Section Audio Tv Master Volume", SSM_AUD_TV_MASTR_VOLUME_VAL, 1);
    SSMPrintDataInfo("Section Audio Sound enhance", SSM_AUD_SOUND_ENHANCE_SWITCH, 1);
    SSMPrintDataInfo("Section Audio EQ mode", SSM_AUD_EQ_MODE_VAL, 32);
    SSMPrintDataInfo("Section Audio EQ gain", SSM_AUD_EQ_GAIN, 1);
    SSMPrintDataInfo("Section Audio No Line points", SSM_AUD_NOLINE_POINTS, 16);

    SSMPrintDataInfo("Section VPP", SSM_RW_VPP_START, SSM_RW_VPP_LEN);
    SSMPrintDataInfo("Section VPP color Demo mode", VPP_DATA_POS_COLOR_DEMO_MODE_START, VPP_DATA_POS_COLOR_DEMO_MODE_SIZE);
    SSMPrintDataInfo("Section VPP color Base mode", VPP_DATA_POS_COLOR_BASE_MODE_START, VPP_DATA_POS_COLOR_BASE_MODE_SIZE);
    SSMPrintDataInfo("Section VPP test pattern", VPP_DATA_POS_TEST_PATTERN_START, VPP_DATA_POS_TEST_PATTERN_SIZE);
    SSMPrintDataInfo("Section VPP DDR SSC", VPP_DATA_POS_DDR_SSC_START, VPP_DATA_POS_DDR_SSC_SIZE);
    SSMPrintDataInfo("Section VPP LVDS SSC", VPP_DATA_POS_LVDS_SSC_START, VPP_DATA_POS_LVDS_SSC_SIZE);
    SSMPrintDataInfo("Section VPP Dream panel", VPP_DATA_POS_DREAM_PANEL_START, VPP_DATA_POS_DREAM_PANEL_SIZE);
    SSMPrintDataInfo("Section VPP BackLight Reverse", VPP_DATA_POS_BACKLIGHT_REVERSE_START, VPP_DATA_POS_BACKLIGHT_REVERSE_SIZE);
    SSMPrintDataInfo("Section VPP Brightness", VPP_DATA_POS_BRIGHTNESS_START, VPP_DATA_POS_BRIGHTNESS_SIZE);
    SSMPrintDataInfo("Section VPP Contrast", VPP_DATA_POS_CONTRAST_START, VPP_DATA_POS_CONTRAST_SIZE);
    SSMPrintDataInfo("Section VPP Saturation", VPP_DATA_POS_SATURATION_START, VPP_DATA_POS_SATURATION_SIZE);
    SSMPrintDataInfo("Section VPP Hue", VPP_DATA_POS_HUE_START, VPP_DATA_POS_HUE_SIZE);
    SSMPrintDataInfo("Section VPP Sharpness", VPP_DATA_POS_SHARPNESS_START, VPP_DATA_POS_SHARPNESS_SIZE);
    SSMPrintDataInfo("Section VPP color temp", VPP_DATA_POS_COLOR_TEMP_START, VPP_DATA_POS_COLOR_TEMP_SIZE);
    SSMPrintDataInfo("Section VPP Noise Reduction", VPP_DATA_POS_NOISE_REDUCTION_START, VPP_DATA_POS_NOISE_REDUCTION_SIZE);
    SSMPrintDataInfo("Section VPP Picture Mode", VPP_DATA_POS_PICTURE_MODE_START, VPP_DATA_POS_PICTURE_MODE_SIZE);
    SSMPrintDataInfo("Section VPP Display Mode", VPP_DATA_POS_DISPLAY_MODE_START, VPP_DATA_POS_DISPLAY_MODE_SIZE);
    SSMPrintDataInfo("Section VPP BackLight", VPP_DATA_POS_BACKLIGHT_START, VPP_DATA_POS_BACKLIGHT_SIZE);
    SSMPrintDataInfo("Section VPP RGB R Gain", VPP_DATA_POS_RGB_GAIN_R_START, VPP_DATA_POS_RGB_GAIN_R_SIZE);
    SSMPrintDataInfo("Section VPP RGB G Gain", VPP_DATA_POS_RGB_GAIN_G_START, VPP_DATA_POS_RGB_GAIN_G_SIZE);
    SSMPrintDataInfo("Section VPP RGB B Gain", VPP_DATA_POS_RGB_GAIN_B_START, VPP_DATA_POS_RGB_GAIN_B_SIZE);
    SSMPrintDataInfo("Section VPP RGB R Offset", VPP_DATA_POS_RGB_POST_OFFSET_R_START, VPP_DATA_POS_RGB_POST_OFFSET_R_SIZE);
    SSMPrintDataInfo("Section VPP RGB G Offset", VPP_DATA_POS_RGB_POST_OFFSET_G_START, VPP_DATA_POS_RGB_POST_OFFSET_G_SIZE);
    SSMPrintDataInfo("Section VPP RGB B Offset", VPP_DATA_POS_RGB_POST_OFFSET_B_START, VPP_DATA_POS_RGB_POST_OFFSET_B_SIZE);
    SSMPrintDataInfo("Section VPP DBC", VPP_DATA_POS_DBC_START, VPP_DATA_POS_DBC_SIZE);
    SSMPrintDataInfo("Section VPP Project ID", VPP_DATA_PROJECT_ID_START, VPP_DATA_PROJECT_ID_SIZE);
    SSMPrintDataInfo("Section VPP DNLP", VPP_DATA_POS_DNLP_START, VPP_DATA_POS_DNLP_SIZE);
    SSMPrintDataInfo("Section VPP Panorama", VPP_DATA_POS_PANORAMA_START, VPP_DATA_POS_PANORAMA_SIZE);
    SSMPrintDataInfo("Section VPP APL", VPP_DATA_APL_START, VPP_DATA_APL_SIZE);
    SSMPrintDataInfo("Section VPP APL2", VPP_DATA_APL2_START, VPP_DATA_APL2_SIZE);
    SSMPrintDataInfo("Section VPP BD", VPP_DATA_BD_START, VPP_DATA_BD_SIZE);
    SSMPrintDataInfo("Section VPP BP", VPP_DATA_BP_START, VPP_DATA_BP_SIZE);
    SSMPrintDataInfo("Section VPP RGB", VPP_DATA_RGB_START, VPP_DATA_RGB_SIZE);
    SSMPrintDataInfo("Section VPP color Space", VPP_DATA_COLOR_SPACE_START, VPP_DATA_COLOR_SPACE_SIZE);
    SSMPrintDataInfo("Section VPP User Nature Switch", VPP_DATA_USER_NATURE_SWITCH_START, VPP_DATA_USER_NATURE_SWITCH_SIZE);
    SSMPrintDataInfo("Section VPP Gamma value", VPP_DATA_GAMMA_VALUE_START, VPP_DATA_GAMMA_VALUE_SIZE);

    SSMPrintDataInfo("Section VDIN", SSM_RW_VDIN_START, SSM_RW_VDIN_LEN);
    SSMPrintDataInfo("Section VDIN source input", TVIN_DATA_POS_SOURCE_INPUT_START, TVIN_DATA_POS_SOURCE_INPUT_SIZE);
    SSMPrintDataInfo("Section VDIN CVBS Std", TVIN_DATA_CVBS_STD_START, TVIN_DATA_CVBS_STD_SIZE);
    SSMPrintDataInfo("Section VDIN 3D mode", TVIN_DATA_POS_3D_MODE_START, TVIN_DATA_POS_3D_MODE_SIZE);
    SSMPrintDataInfo("Section VDIN 3D LR Switch", TVIN_DATA_POS_3D_LRSWITCH_START, TVIN_DATA_POS_3D_LRSWITCH_SIZE);
    SSMPrintDataInfo("Section VDIN 3D Depth", TVIN_DATA_POS_3D_DEPTH_START, TVIN_DATA_POS_3D_DEPTH_SIZE);
    SSMPrintDataInfo("Section VDIN 3D To 2D", TVIN_DATA_POS_3D_TO2D_START, TVIN_DATA_POS_3D_TO2D_SIZE);
    SSMPrintDataInfo("Section VDIN 3D To 2D New", TVIN_DATA_POS_3D_TO2DNEW_START, TVIN_DATA_POS_3D_TO2DNEW_SIZE);
    SSMPrintDataInfo("Section VDIN VGA Adjust", TVIN_DATA_POS_VGA_ADJ_VALUE_START, TVIN_DATA_POS_VGA_ADJ_VALUE_SIZE);
    SSMPrintDataInfo("Section VDIN ADC Calibration flag", TVIN_DATA_POS_ADC_CAL_FLAG_VALUE_START, TVIN_DATA_POS_ADC_CAL_FLAG_VALUE_SIZE);
    SSMPrintDataInfo("Section VDIN ADC Calibration data", TVIN_DATA_POS_ADC_CAL_VALUE_START, TVIN_DATA_POS_ADC_CAL_VALUE_SIZE);
    SSMPrintDataInfo("Section VDIN YPbPr ADC Calibration data", TVIN_DATA_YPbPr_ADC_CAL_VALUE_START, TVIN_DATA_YPbPr_ADC_CAL_VALUE_SIZE);

    SSMPrintDataInfo("Section ATV", SSM_RW_ATV_START, SSM_RW_ATV_LEN);
    SSMPrintDataInfo("Section ATV Channel Info Data", CC_ATV_SSM_CHAN_INFO_START, CC_ATV_SSM_CHAN_INFO_LEN);
    SSMPrintDataInfo("Section ATV Current Channel", CC_ATV_SSM_CURRENT_CHAN_START, CC_ATV_SSM_CURRENT_CHAN_LEN);
    SSMPrintDataInfo("Section ATV Channel Total", CC_ATV_SSM_CHAN_TOTAL_START, CC_ATV_SSM_CHAN_TOTAL_LEN);
    SSMPrintDataInfo("Section ATV Desc En", CC_ATV_SSM_DESC_EN_START, CC_ATV_SSM_DESC_EN_LEN);

    SSMPrintDataInfo("Section MISC", SSM_RW_MISC_START, SSM_RW_MISC_LEN);
    SSMPrintDataInfo("Section MISC FBMF", SSM_RW_FBMF_START, SSM_RW_FBMF_LEN);
    SSMPrintDataInfo("Section MISC Def HDCP", SSM_RW_DEF_HDCP_START, SSM_RW_DEF_HDCP_LEN);
    SSMPrintDataInfo("Section MISC power channel", SSM_RW_POWER_CHANNEL_START, SSM_RW_POWER_CHANNEL_LEN);
    SSMPrintDataInfo("Section MISC last select input src", SSM_RW_LAST_SOURCE_INPUT_START, SSM_RW_LAST_SOURCE_INPUT_LEN);
    SSMPrintDataInfo("Section MISC sys language", SSM_RW_SYS_LANGUAGE_START, SSM_RW_SYS_LANGUAGE_LEN);
    SSMPrintDataInfo("Section MISC aging mode", SSM_RW_AGING_MODE_START, SSM_RW_AGING_MODE_LEN);
    SSMPrintDataInfo("Section MISC panel type", SSM_RW_PANEL_TYPE_START, SSM_RW_PANEL_TYPE_LEN);
    SSMPrintDataInfo("Section MISC power on music switch", SSM_RW_POWER_ON_MUSIC_SWITCH_START, SSM_RW_POWER_ON_MUSIC_SWITCH_LEN);
    SSMPrintDataInfo("Section MISC power on music vol", SSM_RW_POWER_ON_MUSIC_VOL_START, SSM_RW_POWER_ON_MUSIC_VOL_LEN);
    SSMPrintDataInfo("Section MISC system sleep timer", SSM_RW_SYS_SLEEP_TIMER_START, SSM_RW_SYS_SLEEP_TIMER_LEN);
    SSMPrintDataInfo("Section MISC input src parental", SSM_RW_INPUT_SRC_PARENTAL_CTL_START, SSM_RW_INPUT_SRC_PARENTAL_CTL_LEN);
    SSMPrintDataInfo("Section MISC parental contrl switch", SSM_RW_PARENTAL_CTL_SWITCH_START, SSM_RW_PARENTAL_CTL_SWITCH_LEN);
    SSMPrintDataInfo("Section MISC parental contrl passwd", SSM_RW_PARENTAL_CTL_PASSWORD_START, SSM_RW_PARENTAL_CTL_PASSWORD_LEN);
    SSMPrintDataInfo("Section MISC serach navigate flag", SSM_RW_SEARCH_NAVIGATE_FLAG_START, SSM_RW_SEARCH_NAVIGATE_FLAG_LEN);
    SSMPrintDataInfo("Section MISC input number limit", SSM_RW_INPUT_NUMBER_LIMIT_START, SSM_RW_INPUT_NUMBER_LIMIT_LEN);
    SSMPrintDataInfo("Section MISC serial onoff flag", SSM_RW_SERIAL_ONOFF_FLAG_START, SSM_RW_SERIAL_ONOFF_FLAG_LEN);
    SSMPrintDataInfo("Section MISC standby mode flag", SSM_RW_STANDBY_MODE_FLAG_START, SSM_RW_STANDBY_MODE_FLAG_LEN);
    SSMPrintDataInfo("Section MISC HDMIEQ mode", SSM_RW_HDMIEQ_MODE_START, SSM_RW_HDMIEQ_MODE_LEN);
    SSMPrintDataInfo("Section MISC LOGO on/off", SSM_RW_LOGO_ON_OFF_FLAG_START, SSM_RW_LOGO_ON_OFF_FLAG_LEN);
    SSMPrintDataInfo("Section MISC Disable 3D", SSM_RW_DISABLE_3D_START, SSM_RW_DISABLE_3D_LEN);
    SSMPrintDataInfo("Section MISC Global OGO enable", SSM_RW_GLOBAL_OGO_ENABLE_START, SSM_RW_GLOBAL_OGO_ENABLE_LEN);
    SSMPrintDataInfo("Section MISC Local Diming", SSM_RW_LOCAL_DIMING_START, SSM_RW_LOCAL_DIMING_LEN);
    SSMPrintDataInfo("Section MISC VDAC 2D", SSM_RW_VDAC_2D_START, SSM_RW_VDAC_2D_LEN);
    SSMPrintDataInfo("Section MISC VDAC 3D", SSM_RW_VDAC_3D_START, SSM_RW_VDAC_3D_LEN);
    SSMPrintDataInfo("Section MISC Non Standard", SSM_RW_NON_STANDARD_START, SSM_RW_NON_STANDARD_LEN);
    SSMPrintDataInfo("Section MISC adb switch", SSM_RW_ADB_SWITCH_START, SSM_RW_ADB_SWITCH_LEN);
    SSMPrintDataInfo("Section MISC serial cmd switch", SSM_RW_SERIAL_CMD_SWITCH_START, SSM_RW_SERIAL_CMD_SWITCH_LEN);
    SSMPrintDataInfo("Section MISC ca buffer size", SSM_RW_CA_BUFFER_SIZE_START, SSM_RW_CA_BUFFER_SIZE_LEN);
    SSMPrintDataInfo("Section MISC noise gate threshold", SSM_RW_NOISE_GATE_THRESHOLD_START, SSM_RW_NOISE_GATE_THRESHOLD_LEN);
    SSMPrintDataInfo("Section MISC DTV Type", SSM_RW_DTV_TYPE_START, SSM_RW_DTV_TYPE_LEN);
    SSMPrintDataInfo("Section MISC UI graphy backlight", SSM_RW_UI_GRHPHY_BACKLIGHT_START, SSM_RW_UI_GRHPHY_BACKLIGHT_LEN);
    SSMPrintDataInfo("Section MISC fast suspend flag", SSM_RW_FASTSUSPEND_FLAG_START, SSM_RW_FASTSUSPEND_FLAG_LEN);

    SSMPrintDataInfo("Section Customer", SSM_RW_CUSTOMER_DATA_START, SSM_RW_CUSTOMER_DATA_LEN);

    SSMPrintDataInfo("Section Mark 02", SSM_MARK_02_START, SSM_MARK_02_LEN);

    SSMPrintDataInfo("Section RW Test", SSM_RW_TEST_START, SSM_RW_TEST_LEN);

    SSMPrintDataInfo("Section Mark 03", SSM_MARK_03_START, SSM_MARK_03_LEN);

    SSMPrintDataInfo("Section Barcode(device id)", SSM_RSV_BAR_CODE_START, SSM_RSV_BAR_CODE_LEN);
#endif

    return 0;
}*/

CBlobDevice *CTvSettingDeviceFactory::getSaveDeviceFromConfigFile()
{
    const char *device_type = config_get_str(CFG_SECTION_SETTING, "store.device.type", "file");
    const char *device_path = config_get_str(CFG_SECTION_SETTING, "device_path", "/param/default_data");
    const char *device_size = config_get_str(CFG_SECTION_SETTING, "device_size", "0x1000");
    LOGD("getSaveDeviceFromConfigFile type=%s path=%s size=%s", device_type, device_path, device_size);

    //
    if (mpCurDevice != NULL) delete mpCurDevice;

    if (strcmp(device_type, "file") == 0) {
        mpCurDevice = new CBlobDeviceFile();
    } else if (strcmp(device_type, "e2prom") == 0) {
    } else if (strcmp(device_type, "ram") == 0) {
    }

    return mpCurDevice;
}

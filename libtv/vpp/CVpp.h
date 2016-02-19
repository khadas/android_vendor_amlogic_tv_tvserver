#ifndef _C_VPP_H
#define _C_VPP_H
#include "amstream.h"
#include "cm.h"
#include "amvecm.h"
#include "pqdata.h"
#include "../tvin/CTvin.h"
#define GLOBAL_OGO_FORMAT_FLAG  0x6688
#define DEFAULT_RANGE   0
#define LIMIT_RANGE     1
#define FULL_RANGE      2
#define RGB_FORMAT          0
#define YCbCr_422_FORMAT    1
#define YCbCr_444_FORMAT    2
#define PQ_USER_DATA_FROM_E2P   0
#define PQ_USER_DATA_FROM_DB    1

#define VPP_DEV_PATH    "/dev/amvecm"
#define VPP_3D_DEV_PATH    "/dev/amvideo"

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

typedef union tag_suc {
    short s;
    unsigned char c[2];
} SUC;

typedef union tag_usuc {
    unsigned short s;
    unsigned char c[2];
} USUC;

typedef enum is_3d_type_e {
    INDEX_3D_INVALID = -1,
    INDEX_2D = 0,
    INDEX_3D = 1,
} is_3d_type_t;

typedef enum vpp_deblock_mode_e {
    VPP_DEBLOCK_MODE_OFF,
    VPP_DEBLOCK_MODE_LOW,
    VPP_DEBLOCK_MODE_MIDDLE,
    VPP_DEBLOCK_MODE_HIGH,
    VPP_DEBLOCK_MODE_AUTO,
} vpp_deblock_mode_t;

typedef enum vpp_panorama_mode_e {
    VPP_PANORAMA_MODE_FULL,
    VPP_PANORAMA_MODE_NORMAL,
    VPP_PANORAMA_MODE_MAX,
} vpp_panorama_mode_t;

typedef enum vpp_color_space_type_e {
    VPP_COLOR_SPACE_AUTO,
    VPP_COLOR_SPACE_YUV,
    VPP_COLOR_SPACE_RGB,
} vpp_color_space_type_t;

typedef enum vpp_display_mode_e {
    VPP_DISPLAY_MODE_169,
    VPP_DISPLAY_MODE_PERSON,
    VPP_DISPLAY_MODE_MOVIE,
    VPP_DISPLAY_MODE_CAPTION,
    VPP_DISPLAY_MODE_MODE43,
    VPP_DISPLAY_MODE_FULL,
    VPP_DISPLAY_MODE_NORMAL,
    VPP_DISPLAY_MODE_NOSCALEUP,
    VPP_DISPLAY_MODE_CROP_FULL,
    VPP_DISPLAY_MODE_CROP,
    VPP_DISPLAY_MODE_ZOOM,
    VPP_DISPLAY_MODE_FULL_REAL,//add for N360 by haifeng.liu
    VPP_DISPLAY_MODE_MAX,
} vpp_display_mode_t;

typedef enum vpp_color_demomode_e {
    VPP_COLOR_DEMO_MODE_ALLON,
    VPP_COLOR_DEMO_MODE_YOFF,
    VPP_COLOR_DEMO_MODE_COFF,
    VPP_COLOR_DEMO_MODE_GOFF,
    VPP_COLOR_DEMO_MODE_MOFF,
    VPP_COLOR_DEMO_MODE_ROFF,
    VPP_COLOR_DEMO_MODE_BOFF,
    VPP_COLOR_DEMO_MODE_RGBOFF,
    VPP_COLOR_DEMO_MODE_YMCOFF,
    VPP_COLOR_DEMO_MODE_ALLOFF,
    VPP_COLOR_DEMO_MODE_MAX,
} vpp_color_demomode_t;

typedef enum vpp_color_basemode_e {
    VPP_COLOR_BASE_MODE_OFF,
    VPP_COLOR_BASE_MODE_OPTIMIZE,
    VPP_COLOR_BASE_MODE_ENHANCE,
    VPP_COLOR_BASE_MODE_DEMO,
    VPP_COLOR_BASE_MODE_MAX,
} vpp_color_basemode_t;

typedef enum vpp_color_management2_e {
    VPP_COLOR_MANAGEMENT2_MODE_OFF,
    VPP_COLOR_MANAGEMENT2_MODE_OPTIMIZE,
    VPP_COLOR_MANAGEMENT2_MODE_ENHANCE,
    VPP_COLOR_MANAGEMENT2_MODE_DEMO,
    VPP_COLOR_MANAGEMENT2_MODE_MAX,
} vpp_color_management2_t;

typedef enum vpp_noise_reduction2_mode_e {
    VPP_NOISE_REDUCTION2_MODE_OFF,
    VPP_NOISE_REDUCTION2_MODE_LOW,
    VPP_NOISE_REDUCTION2_MODE_MID,
    VPP_NOISE_REDUCTION2_MODE_HIGH,
    VPP_NOISE_REDUCTION2_MODE_AUTO,
    VPP_NOISE_REDUCTION2_MODE_MAX,
} vpp_noise_reduction2_mode_t;
typedef enum vpp_xvycc_mode_e {
    VPP_XVYCC_MODE_OFF,
    VPP_XVYCC_MODE_STANDARD,
    VPP_XVYCC_MODE_ENHANCE,
    VPP_XVYCC_MODE_MAX,
} vpp_xvycc_mode_t;

typedef enum vpp_mcdi_mode_e {
    VPP_MCDI_MODE_OFF,
    VPP_MCDI_MODE_STANDARD,
    VPP_MCDI_MODE_ENHANCE,
    VPP_MCDI_MODE_MAX,
} vpp_mcdi_mode_t;

typedef enum vpp_picture_mode_e {
    VPP_PICTURE_MODE_STANDARD,
    VPP_PICTURE_MODE_BRIGHT,
    VPP_PICTURE_MODE_SOFT,
    VPP_PICTURE_MODE_USER,
    VPP_PICTURE_MODE_MOVIE,
    VPP_PICTURE_MODE_COLORFUL,
    VPP_PICTURE_MODE_MAX,
} vpp_picture_mode_t;

typedef enum vpp_color_temperature_mode_e {
    VPP_COLOR_TEMPERATURE_MODE_STANDARD,
    VPP_COLOR_TEMPERATURE_MODE_WARM,
    VPP_COLOR_TEMPERATURE_MODE_COLD,
    VPP_COLOR_TEMPERATURE_MODE_USER,
    VPP_COLOR_TEMPERATURE_MODE_MAX,
} vpp_color_temperature_mode_t;

typedef enum vpp_noise_reduction_mode_e {
    VPP_NOISE_REDUCTION_MODE_OFF,
    VPP_NOISE_REDUCTION_MODE_LOW,
    VPP_NOISE_REDUCTION_MODE_MID,
    VPP_NOISE_REDUCTION_MODE_HIGH,
    VPP_NOISE_REDUCTION_MODE_AUTO,
    VPP_NOISE_REDUCTION_MODE_MAX,
} vpp_noise_reduction_mode_t;

typedef enum vpp_test_pattern_e {
    VPP_TEST_PATTERN_NONE,
    VPP_TEST_PATTERN_RED,
    VPP_TEST_PATTERN_GREEN,
    VPP_TEST_PATTERN_BLUE,
    VPP_TEST_PATTERN_WHITE,
    VPP_TEST_PATTERN_BLACK,
    VPP_TEST_PATTERN_MAX,
} vpp_test_pattern_e;

typedef struct vpp_pq_para_s {
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int sharpness;
    int backlight;
    int nr;
} vpp_pq_para_t;

typedef enum noline_params_type_e {
    NOLINE_PARAMS_TYPE_BRIGHTNESS,
    NOLINE_PARAMS_TYPE_CONTRAST,
    NOLINE_PARAMS_TYPE_SATURATION,
    NOLINE_PARAMS_TYPE_HUE,
    NOLINE_PARAMS_TYPE_SHARPNESS,
    NOLINE_PARAMS_TYPE_VOLUME,
    NOLINE_PARAMS_TYPE_BACKLIGHT,
    NOLINE_PARAMS_TYPE_MAX,
} noline_params_type_t;

typedef struct noline_params_s {
    int osd0;
    int osd25;
    int osd50;
    int osd75;
    int osd100;
} noline_params_t;

typedef enum vpp_dream_panel_e {
    VPP_DREAM_PANEL_OFF,
    VPP_DREAM_PANEL_LIGHT,
    VPP_DREAM_PANEL_SCENE,
    VPP_DREAM_PANEL_FULL,
    VPP_DREAM_PANEL_DEMO,
    VPP_DREAM_PANEL_MAX,
} vpp_dream_panel_t;

class CPqData;

class CVpp {
public:
    CVpp();
    ~CVpp();
    int doSuspend();
    int doResume();
    int Vpp_Init ( const char *pq_db_path );
    int Vpp_Uninit ( void );
    CPqData *getPqData();
    int Vpp_ResetLastVppSettingsSourceType ( void );
    int Vpp_SetColorDemoMode ( vpp_color_demomode_t demomode );
    int Vpp_SetBaseColorMode ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetColorTemperatureUser ( vpp_color_temperature_mode_t temp_mode, tv_source_input_type_t source_type );
    int Vpp_SetColorTemperature ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetBrightness ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetContrast ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetSaturation ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetHue ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_port_t source_port, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt  );
    int Vpp_SetSharpness ( int value, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode,  tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetXVYCCMode ( vpp_xvycc_mode_t xvycc_mode, tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int Vpp_SetDeblockMode(vpp_deblock_mode_t mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetMCDIMode ( vpp_mcdi_mode_t mcdi_mode, tv_source_input_type_t source_type , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_SetZoom ( int value );
    int Vpp_LoadDI(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int Vpp_LoadBasicRegs ( tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int VppRegisterInterface();
    int RGBGainValueSSMToRisterMapping ( int gainValue );
    int RGBOffsetValueSSMToRisterMapping ( int gainValue );
    int SetRGBValue ( vpp_color_temperature_mode_t temp_mode,  unsigned char data_buf[] );
    int GetRGBValue ( vpp_color_temperature_mode_t temp_mode,  tcon_rgb_ogo_t *p_rgbogo );
    int SetColorSpaceMode ( vpp_color_space_type_t colorSpace );
    //int Tv_SetGammaValue(int gammaValue);
    int GetGammaValue();
    //int Tv_SetColorDemoMode ( vpp_color_demomode_t demomode );
    vpp_color_demomode_t GetColorDemoMode ( void );

    int SetBaseColorMode ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    vpp_color_basemode_t GetBaseColorMode ( void );
    int SetBaseColorModeWithoutSave ( vpp_color_basemode_t basemode , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);
    int SaveBaseColorMode ( vpp_color_basemode_t basemode );
    int SetColorTemperature ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type, int is_save );
    vpp_color_temperature_mode_t GetColorTemperature ( tv_source_input_type_t source_type );
    int SetColorTempWithoutSave ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type );
    int SaveColorTemp ( vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type );
    int SetBrightness ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetBrightness ( tv_source_input_type_t source_type );
    int SetContrast ( int value, tv_source_input_type_t source_type,  tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetContrast ( tv_source_input_type_t source_type );
    int SetSaturation ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetSaturation ( tv_source_input_type_t source_type );
    int SetHue ( int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    int GetHue ( tv_source_input_type_t source_type );
    int SetSharpness ( int value, tv_source_input_type_t source_type, int is_enable, is_3d_type_t is3d, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, int is_save );
    int GetSharpness ( tv_source_input_type_t source_type );
    int SetBacklight ( int value, tv_source_input_type_t source_type, int is_save );
    int GetBacklight ( tv_source_input_type_t source_type );
    int SetBacklightWithoutSave ( int value, tv_source_input_type_t source_type );
    int SaveBacklight ( int value, tv_source_input_type_t source_type );
    int SetPQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save );
    vpp_picture_mode_t GetPQMode ( tv_source_input_type_t source_type );
    int SavePQMode ( vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type );
    int SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, int is_save );
    vpp_display_mode_t GetDisplayMode ( tv_source_input_type_t source_type );
    int SaveNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_type_t source_type );
    int SetNoiseReductionMode ( vpp_noise_reduction_mode_t nr_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt, int is_save );
    vpp_noise_reduction_mode_t GetNoiseReductionMode ( tv_source_input_type_t source_type );
    int SetDNLP ( tv_source_input_type_t source_type, tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    int LoadVppSettings ( tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    //int Tv_SetVppParamDefault(void);


    int FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness );
    int FactoryGetPQMode_Brightness ( int source_type, int pq_mode );
    int FactorySetPQMode_Contrast ( int source_type, int pq_mode, int contrast );
    int FactoryGetPQMode_Contrast ( int source_type, int pq_mode );
    int FactorySetPQMode_Saturation ( int source_type, int pq_mode, int saturation );
    int FactoryGetPQMode_Saturation ( int source_type, int pq_mode );
    int FactorySetPQMode_Hue ( int source_type, int pq_mode, int hue );
    int FactoryGetPQMode_Hue ( int source_type, int pq_mode );
    int FactorySetPQMode_Sharpness ( int source_type, int pq_mode, int sharpness );
    int FactoryGetPQMode_Sharpness ( int source_type, int pq_mode );
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
    int FactorySetNolineParams ( int type, int source_type, noline_params_t noline_params );
    noline_params_t FactoryGetNolineParams ( int type, int source_type );
    int FactorySetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt, tvin_cutwin_t cutwin_t );
    tvin_cutwin_t FactoryGetOverscan ( int source_type, int fmt, is_3d_type_t is3d, int trans_fmt );
    int FactorySetBacklightPWM_Frequency(int freq);
    int FactoryGetBacklightPWM_Frequency ( void );
    int FactorySetBacklight_Switch_status ( int status );
    int FactoryGetBacklight_Switch_status ( void );
    int FactorySetBacklightPWM_Duty ( int duty );
    int FactoryGetBacklightPWM_Duty ( void );
    int FactorySetLVDS_ColorDepth ( int depth );
    int FactoryGetLVDS_ColorDepth ( void );
    int FactorySetLVDS_ColorDither_status ( int status );
    int FactoryGetLVDS_ColorDither_status ( void );
    int FactorySetLVDS_Mapping_status ( int status );
    int FactoryGetLVDS_Mapping_status ( void );
    int FactorySetLVDS_PortSwap_status ( int status );
    int FactoryGetLVDS_PortSwap_status ( void );

    int VPPSSMRestoreDefault();
    int VPPSSMFacRestoreDefault();


    int GetHistogram_AVE();
    int SetDnlp_OFF();
    int SetDnlp_ON ( void );
    int GetDnlp_Status();

    int GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int ReadColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int SetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    int SaveColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    unsigned short CalColorTemperatureParamsChecksum ( void );
    int SetColorTempParamsChecksum ( void );
    unsigned short GetColorTempParamsChecksum ( void );
    int CheckTempDataLable ( void );
    int SetTempDataLable ( void );
    int CheckColorTemperatureParams ( void );
    int RestoeColorTemperatureParamsFromDB ( tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt );
    int CheckColorTemperatureParamAlldata ( tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt );
    void  Tvin_SetVideoScreenColorType ( int type );
    int Vpp_GetVppConfig();
    int Vpp_GetPQModeValue ( tv_source_input_type_t, vpp_picture_mode_t, vpp_pq_para_t * );
    int Vpp_SetPQParams ( tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode, vpp_pq_para_t pq_para , tvin_port_t source_port , tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt);

    int Vpp_LoadGammaDefault ( tv_source_input_type_t, tvin_sig_fmt_t );
    int Vpp_LoadGammaSpecial ( int );
    int Vpp_LoadGamma ( tv_source_input_type_t, tvin_sig_fmt_t );
    int FactoryResetNonlinear();
    tvin_cutwin_t GetOverscan ( tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    static is_3d_type_t Check2Dor3D ( is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt );
    //api
    int VPP_SetVEBlackExtension ( const struct ve_bext_s *pBExt );
    int VPP_SetVideoCrop ( int Voffset0, int Hoffset0, int Voffset1, int Hoffset1 );
    int VPP_SetNonLinearFactor ( int value );
    int VPP_SetGrayPattern(int value);
    int VPP_GetGrayPattern();
    int VPP_SplitScreenEffect(int width, int v_register);
    int VPP_SetBackLight_Switch ( int value );
    int VPP_GetBackLight_Switch ( void );
    int VPP_SetScalerPathSel (const unsigned int value);

    static CVpp *getInstance();
private:
    //
    int VPP_OpenModule ( void );
    int VPP_CloseModule ( void );
    int VPP_SetVideoBrightness ( int value );
    int VPP_SetVideoContrast ( int value );
    int VPP_SetVideoSaturationHue ( int satVal, int hueVal );
    int VPP_SetCMRegisterMap ( struct cm_regmap_s *pRegMap );
    int Vpp_LoadRegs ( am_regs_t regs );
    int VPP_SetRGBOGO ( const struct tcon_rgb_ogo_s *rgbogo );
    int VPP_GetRGBOGO ( const struct tcon_rgb_ogo_s *rgbogo );
    int VPP_SetGammaOnOff ( unsigned char onoff );
    int VPP_SetGammaTbl_R ( unsigned short red[256] );
    int VPP_SetGammaTbl_G ( unsigned short green[256] );
    int VPP_SetGammaTbl_B ( unsigned short blue[256] );
    void video_set_saturation_hue ( signed char saturation, signed char hue, signed long *mab );
    void video_get_saturation_hue ( signed char *sat, signed char *hue, signed long *mab );
    int VPP_SetBackLightLevel ( int value );
    int VPP_SetVEDNLP ( const struct ve_dnlp_s *pDNLP );
    int VPP_SetVENewDNLP ( const ve_dnlp_table_t *pDNLP );
    int Vpp_GetAVGHistogram ( struct ve_hist_s *hist );
    int Vpp_SetDnlpOff ( void );
    int Vpp_SetDnlpOn ( void );
    int VPP_SetVESharpness ( const ve_hsvs_s * );
    int VPP_SetVEChromaCoring ( const ve_ccor_s * );
    int VPP_SetVEBlueEnh ( const ve_benh_s * );
    int VPP_SetVEDemo ( const ve_demo_s * );
    int VPP_SetVERegisterMap ( const ve_regmap_s * );
    int VPP_SetVEDebug ( const long long unsigned int * );
    int VPP_SetCMRegion ( const cm_region_s * );
    int VPP_SetCMTopLayer ( const cm_top_s * );
    int VPP_SetCMDemo ( const cm_demo_s * );

    int VPP_DeviceIOCtl ( int request, ... );
    int VPP3D_DeviceIOCtl ( int, ... );
    int VPP_SetCMDebug ( const long long unsigned int * );
    int VPP_SetAVSyncEnable ( unsigned int );
    int VPP_SetVideoNoiseReduction ( int );
    int VPP_SetVideoSaturation ( int );
    int VPP_SetVideoHue ( int );
    int VPP_SetDeinterlaceMode ( int );

    int isPreviewWindow();

    static CVpp *mInstance;
    tv_source_input_type_t vpp_setting_last_source_type;
    tvin_sig_fmt_t vpp_setting_last_sig_fmt;
    tvin_trans_fmt_t vpp_setting_last_trans_fmt;
    //cfg
    bool mbVppCfg_backlight_reverse;
    bool mbVppCfg_backlight_init;
    bool mbVppCfg_pqmode_without_hue;
    bool mbVppCfg_hue_reverse;
    bool mbVppCfg_gamma_onoff;
    bool mbVppCfg_whitebalance_sameparam;
    bool mbVppCfg_new_cm;
    bool mbVppCfg_new_nr;
    bool mbVppCfg_panorama_switch;
    bool mbVppCfg_pqmode_depend_bklight;
    bool mbVppCfg_colortemp_by_source;

    CPqData *mpPqData;

    int vpp_amvideo_fd;
    int vpp_amvideo_3d_fd;
};
#endif

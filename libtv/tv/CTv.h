
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CDtv.h
//  @ Date : 2013-11
//  @ Author :

#if !defined(_CDTV_H)
#define _CDTV_H
#include <stdint.h>
#include <sys/time.h>
#include <am_epg.h>
#include <am_mem.h>
#include <utils/threads.h>
#include "CTvProgram.h"
#include "CTvEpg.h"
#include "CTvScanner.h"
#include "CTvLog.h"
#include "CTvTime.h"
#include "CTvEvent.h"
#include "CTvEv.h"
#include "CTvBooking.h"
#include "CFrontEnd.h"
#include "../vpp/CVpp.h"
#include "../vpp/CPQdb.h"
#include "../tvin/CTvin.h"
#include "../tvin/CHDMIRxCEC.h"
#include "../tvutils/CMsgQueue.h"
#include "../tvutils/CSerialCommunication.h"
#include "../tvutils/serial_operate.h"
#include "CTvRecord.h"
#include "CTvSubtitle.h"
#include "CUpgradeFBC.h"
#include "CAv.h"
#include "CTvDmx.h"
#include "../audio/CTvAudio.h"
#include "AutoBackLight.h"
#include "CAutoPQparam.h"
#include "tvin/CSourceConnectDetect.h"
using namespace android;

static const char *TV_CONFIG_FILE_PATH = "/param/tvconfig.conf";
static const char *TV_DB_PATH = "/param/dtv.db";
static const char *TV_CONFIG_FILE_SYSTEM_PATH = "/system/etc/tvconfig.conf";
static const char *TV_CONFIG_FILE_PARAM_PATH = "/param/tvconfig.conf";
static const char *TV_CHANNEL_LIST_SYSTEM_PATH = "/system/etc/tv_default.xml";
static const char *TV_CHANNEL_LIST_PARAM_PATH = "/param/tv_default.xml";
static const char *TV_SSM_DATA_SYSTEM_PATH = "/system/etc/ssm_data";
static const char *TV_SSM_DATA_PARAM_PATH = "/param/ssm_data";

#define DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN "/sys/class/tsync/av_threshold_min"
#define AV_THRESHOLD_MIN_MS "540000" //6S = 6*90000




typedef enum tv_fmt_ratio_e {
    RATIO_43,
    RATIO_169,
    RATIO_MAX,
} tv_fmt_ratio_t;

typedef enum tv_color_fmt_e {
    COLOR_RGB444,
    COLOR_YUV422,
    COLOR_YUV444,
    COLOR_MAX,
} tv_color_fmt_t;



typedef enum tv_window_mode_e {
    NORMAL_WONDOW,
    PREVIEW_WONDOW,
} tv_window_mode_t;

typedef enum tv_dtv_scan_running_status_e {
    DTV_SCAN_RUNNING_NORMAL,
    DTV_SCAN_RUNNING_ANALYZE_CHANNEL,
} tv_dtv_scan_running_status_t;

typedef struct tv_config_s {
    bool kernelpet_disable;
    unsigned int kernelpet_timeout;
    bool userpet;
    unsigned int userpet_timeout;
    unsigned int userpet_reset;
    bool memory512m;
} tv_config_t;

typedef enum TvRunStatus_s {
    TV_INIT_ED = -1,
    TV_OPEN_ED = 0,
    TV_START_ED ,
    TV_RESUME_ED,
    TV_PAUSE_ED,
    TV_STOP_ED,
    TV_CLOSE_ED,
} TvRunStatus_t;

class CTv : public CTvin::CTvinSigDetect::ISigDetectObserver, public CSourceConnectDetect::ISourceConnectObserver, public CHDMIRxCEC::IHDMIRxCECObserver, public CUpgradeFBC::IUpgradeFBCObserver, public CSerialCommunication::ISerialCommunicationObserver, public CTvSubtitle::IObserver, public  CTv2d4GHeadSetDetect::IHeadSetObserver {
public:
    static const int TV_ACTION_NULL = 0x0000;
    static const int TV_ACTION_STARTING = 0x0001;
    static const int TV_ACTION_STOPING = 0x0002;
    static const int TV_ACTION_SCANNING = 0x0004;
    static const int TV_ACTION_PLAYING = 0x0008;
    static const int TV_ACTION_RECORDING = 0x0010;
    static const int TV_ACTION_SOURCE_SWITCHING = 0x0020;
public:
    class TvIObserver {
    public:
        TvIObserver() {};
        virtual ~TvIObserver() {};
        virtual void onTvEvent ( const CTvEv &ev ) = 0;
    };
    //main
    CTv();
    virtual ~CTv();
    virtual int OpenTv ( void );
    virtual int CloseTv ( void );
    virtual int StartTvLock ();
    virtual int StopTvLock ( void );
    virtual int DoSuspend(int type);
    virtual int DoResume(int type);
    virtual int DoInstabootSuspend();
    virtual int DoInstabootResume();
    virtual TvRunStatus_t GetTvStatus();
    virtual int ClearAnalogFrontEnd();
    virtual tv_source_input_t GetLastSourceInput (void);
    virtual int SetSourceSwitchInput (tv_source_input_t source_input );
    virtual tv_source_input_t GetCurrentSourceInputLock ( void );
    virtual tvin_info_t GetCurrentSignalInfo ( void );
    virtual int SetPreviewWindow ( tvin_window_pos_t pos );
    virtual int dtvAutoScan();
    virtual int dtvManualScan (int beginFreq, int endFreq, int modulation = -1);
    virtual int atvAutoScan(int videoStd, int audioStd, int searchType);
    virtual int clearAllProgram(int arg0);
    virtual int clearDbAllProgramInfoTable();
    virtual void setSourceSwitchAndPlay();
    virtual int GetDisplayResolutionConfig();
    virtual int GetDisplayResolutionInfo();
    virtual int atvMunualScan ( int startFreq, int endFreq, int videoStd, int audioStd, int store_Type = 0, int channel_num = 0 );
    virtual int stopScanLock();
    virtual int dtvAutoScanAtscLock(int attenna, int videoStd, int audioStd);
    virtual void SetRecordFileName ( char *name );
    virtual  void StartToRecord();
    virtual void StopRecording();
    virtual void SetRecCurTsOrCurProgram ( int sel );
    virtual int startCC(int country, int src, int channel, int service);
    virtual int stopCC();
    virtual void printDebugInfo();
    virtual int playProgramLock ( int progId );
    virtual int playDvbcProgram ( int progId );
    virtual int playDtmbProgram ( int progId );
    virtual int playAtvProgram ( int, int, int, int, int);
    virtual int playDtvProgram ( int, int, int, int, int, int, int, int, int, int);
    virtual int stopPlayingLock();
    virtual int resetFrontEndPara ( frontend_para_set_t feParms );
    virtual int SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt );
    virtual void startAutoBackLight();
    virtual void stopAutoBackLight();
    virtual const char *getPqDbPath();
    virtual void onHdmiSrChanged(int  sr, bool bInit);
    virtual void onHMDIAudioStatusChanged(int status);
    virtual int GetATVAFCType();
    virtual int GetATVSourceTimerSwitch();
    int SetCurProgramAudioVolumeCompensationVal ( int tmpVal );
    int GetAudioVolumeCompensationVal(int progDbId);
    //dtv audio track info
    int getAudioTrackNum ( int progId );
    int getAudioInfoByIndex ( int progId, int idx, int *pAFmt, String8 &lang );
    int switchAudioTrack ( int progId, int idx );
    int switchAudioTrack ( int aPid, int aFmt, int aParam );
    int getVideoFormatInfo ( int *pWidth, int *pHeight, int *pFPS, int *pInterlace );
    int ResetAudioDecoderForPCMOutput();
    int  setAudioChannel ( int channelIdx );
    int getAudioChannel();
    int setTvObserver (TvIObserver *ob);
    int getAtscAttenna();
    long getTvTime()
    {
        return mTvTime.getTime();
    };
    void getFrontendStatus();
    int getFrontendSignalStrength();
    int getFrontendSNR();
    int getFrontendBER();
    int getChannelInfoBydbID ( int dbID, channel_info_t &chan_info );
    int setBlackoutEnable(int enable);
    int getSaveBlackoutEnable();
    int getAutoBackLight_on_off();
    int saveATVProgramID ( int dbID );
    int getATVProgramID ( void );
    int saveDTVProgramID ( int dbID );
    int getDTVProgramID ( void );
    int getCurPlayProgId();
    int getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq );
    int getAverageLuma();
    int setAutobacklightData(const char *value);
    int getAutoBacklightData(int *data);

    virtual int Tv_SSMRestoreDefaultSetting();

    //
    int GetSourceConnectStatus(tv_source_input_t source_input);
    int IsDVISignal();
    int isVgaFmtInHdmi();
    int isSDFmtInHdmi ( void );
    //
    int getHDMIFrameRate ( void );
    void RefreshAudioMasterVolume ( tv_source_input_t source_input );

    int Tv_Set3DMode ( VIDEO_3D_MODE_T mode );
    VIDEO_3D_MODE_T Tv_Get3DMode ( void );
    int Tv_Set3DLRSwith ( int on_off);
    int Tv_Get3DLRSwith ( void );
    int Tv_Set3DTo2DMode ( int mode);
    int Tv_Get3DTo2DMode ( void );
    int Tv_Set3DDepth ( int value );
    int GetSave3DDepth ( void );
    is_3d_type_t Check2Dor3D ( VIDEO_3D_MODE_T mode3d, tvin_trans_fmt_t trans_fmt );
    int Tvin_SetPLLValues ();
    int SetCVD2Values ();
    unsigned int Vpp_GetDisplayResolutionInfo(tvin_window_pos_t *win_pos);
    int get_hdmi_sampling_rate();
    //SSM
    virtual int Tv_SSMFacRestoreDefaultSetting();
    int Tv_GetHistgram(int *histgram_buf);
    int StartHeadSetDetect();
    virtual void onHeadSetDetect(int state, int para);

    CTvin::CTvinSigDetect mSigDetectThread;
    CSourceConnectDetect mSourceConnectDetectThread;
    CHDMIRxCEC mHDMIRxCEC;
    CUpgradeFBC *mpUpgradeFBC;
    CSerialCommunication mSerialA;
    CSerialCommunication mSerialB;
    CSerialCommunication mSerialC;
    CTvSubtitle mSubtitle;
    CTv2d4GHeadSetDetect mHeadSet;

    int SendHDMIRxCECCustomMessage(unsigned char data_buf[]);
    int SendHDMIRxCECCustomMessageAndWaitReply(unsigned char data_buf[], unsigned char reply_buf[], int WaitCmd, int timeout);
    int SendHDMIRxCECBoradcastStandbyMessage();
    int SendHDMIRxCECGiveCECVersionMessage(tv_source_input_t source_input, unsigned char data_buf[]);
    int SendHDMIRxCECGiveDeviceVendorIDMessage(tv_source_input_t source_input, unsigned char data_buf[]);
    int SendHDMIRxCECGiveOSDNameMessage(tv_source_input_t source_input, unsigned char data_buf[]);

    int GetHdmiHdcpKeyKsvInfo(int data_buf[]);
    virtual bool hdmiOutWithFbc();
    int StartUpgradeFBC(char *file_name, int mode, int upgrade_blk_size);
    int SetSerialSwitch(int dev_id, int switch_val);
    int SendSerialData(int dev_id, int data_len, unsigned char data_buf[]);
    int SetDebugSerialOnOff(int on_off);
    int GetDebugSerialOnOff();
    int ChannelExport(const char *destPath);
    int ChannelImport(const char *srcPath);
    int Tv_GetProjectInfo(project_info_t *ptrInfo);
    int Tv_GetPlatformType();
    int Tv_HandeHDMIEDIDFilePathConfig();
    int Tv_SetDDDRCMode(tv_source_input_t source_input);
    int Tv_SetAudioSourceType (tv_source_input_t source_input);

    //PQ
    virtual int Tv_SetBrightness ( int brightness, tv_source_input_type_t source_type, int is_save );
    virtual int Tv_GetBrightness ( tv_source_input_type_t source_type );
    virtual int Tv_SaveBrightness ( int brightness, tv_source_input_type_t source_type );
    virtual int Tv_SetContrast ( int contrast, tv_source_input_type_t source_type,  int is_save );
    virtual int Tv_GetContrast ( tv_source_input_type_t source_type );
    virtual int Tv_SaveContrast ( int contrast, tv_source_input_type_t source_type );
    virtual int Tv_SetSaturation ( int satuation, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, int is_save );
    virtual int Tv_GetSaturation ( tv_source_input_type_t source_type );
    virtual int Tv_SaveSaturation ( int satuation, tv_source_input_type_t source_type );
    virtual int Tv_SetHue ( int hue, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, int is_save );
    virtual int Tv_GetHue ( tv_source_input_type_t source_type );
    virtual int Tv_SaveHue ( int hue, tv_source_input_type_t source_type );
    virtual int Tv_SetPQMode ( vpp_picture_mode_t mode, tv_source_input_type_t source_type, int is_save );
    virtual vpp_picture_mode_t Tv_GetPQMode ( tv_source_input_type_t source_type );
    virtual int Tv_SavePQMode ( vpp_picture_mode_t mode, tv_source_input_type_t source_type );
    virtual int Tv_SetSharpness ( int value, tv_source_input_type_t source_type, int en, int is_save );
    virtual int Tv_GetSharpness ( tv_source_input_type_t source_type );
    virtual int Tv_SaveSharpness ( int value, tv_source_input_type_t source_type );
    virtual int Tv_SetBacklight ( int value, tv_source_input_type_t source_type, int is_save );
    virtual int Tv_GetBacklight ( tv_source_input_type_t source_type );
    virtual int Tv_SaveBacklight ( int value, tv_source_input_type_t source_type );
    int Tv_SetBacklight_Switch ( int value );
    int Tv_GetBacklight_Switch ( void );
    int Tv_SetColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type, int is_save );
    vpp_color_temperature_mode_t Tv_GetColorTemperature ( tv_source_input_type_t source_type );
    virtual int Tv_SetDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save );
    virtual int Tv_SaveDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type );
    virtual int Tv_SaveColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type );
    virtual vpp_display_mode_t Tv_GetDisplayMode ( tv_source_input_type_t source_type );
    virtual int Tv_SetNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type, int is_save );
    virtual vpp_noise_reduction_mode_t Tv_GetNoiseReductionMode ( tv_source_input_type_t source_type );
    virtual int Tv_SaveNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type );
    int Tv_SetSplitScreenDemoStatus(tv_source_input_type_t source_type, int onoff_status);
    int Tv_GetSplitScreenDemoStatus(tv_source_input_type_t source_type);
    int Tv_SplitScreenEffect(int mode, int width, int reverse = 0);
    //
    int Tv_FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness );
    int Tv_FactoryGetPQMode_Brightness ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Contrast ( int source_type, int pq_mode, int contrast );
    int Tv_FactoryGetPQMode_Contrast ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Saturation ( int source_type, int pq_mode, int saturation );
    int Tv_FactoryGetPQMode_Saturation ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Hue ( int source_type, int pq_mode, int hue );
    int Tv_FactoryGetPQMode_Hue ( int source_type, int pq_mode );
    int Tv_FactorySetPQMode_Sharpness ( int source_type, int pq_mode, int sharpness );
    int Tv_FactoryGetPQMode_Sharpness ( int source_type, int pq_mode );

    int GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int Tv_FactorySetTestPattern ( int pattern );
    int Tv_FactoryGetTestPattern ( void );
    int Tv_FactorySetScreenColor ( int vdin_blending_mask, int y, int u, int v );
    int Tv_FactoryResetPQMode ( void );
    int Tv_FactoryResetColorTemp ( void );
    int Tv_FactorySetParamsDefault ( void );
    int Tv_FactorySetDDRSSC ( int step );
    int Tv_FactoryGetDDRSSC ( void );
    int Tv_FactorySetLVDSSSC ( int step );
    int Tv_FactoryGetLVDSSSC ( void );
    void Tv_Spread_Spectrum(void);
    int Tv_FactorySetNolineParams ( int noline_params_type, int source_type, noline_params_t noline_params );
    noline_params_t Tv_FactoryGetNolineParams ( int noline_params_type, int source_type );
    int Tv_FactorySetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt, tvin_cutwin_t cutwin_t );
    tvin_cutwin_t Tv_FactoryGetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt );
    int Tv_ReplacePqDb(const char *newFilePath = NULL);
    //end PQ

    //TV TO FBC
    int Tv_FactorySet_FBC_Brightness ( int value );
    int Tv_FactoryGet_FBC_Brightness ( void );
    int Tv_FactorySet_FBC_Contrast( int value );
    int Tv_FactoryGet_FBC_Contrast ( void );
    int Tv_FactorySet_FBC_Saturation( int value );
    int Tv_FactoryGet_FBC_Saturation ( void );
    int Tv_FactorySet_FBC_HueColorTint( int value );
    int Tv_FactoryGet_FBC_HueColorTint ( void );
    virtual int Tv_FactorySet_FBC_Backlight ( int value );
    virtual int Tv_FactoryGet_FBC_Backlight ( void );
    int Tv_FactorySet_FBC_Backlight_N360 ( int value );
    int Tv_FactoryGet_FBC_Backlight_N360 ( void );
    int Tv_FactorySet_FBC_ELEC_MODE( int value );
    int Tv_FactoryGet_FBC_ELEC_MODE( void );
    int Tv_FactorySet_FBC_BACKLIGHT_N360( int value );
    int Tv_FactoryGet_FBC_BACKLIGHT_N360( void );
    int Tv_FactorySet_FBC_Picture_Mode ( int mode );
    int Tv_FactoryGet_FBC_Picture_Mode ( void );
    int Tv_FactorySet_FBC_Set_Test_Pattern ( int mode );
    int Tv_FactoryGet_FBC_Get_Test_Pattern ( void );
    int Tv_FactorySet_FBC_Gain_Red( int value );
    int Tv_FactoryGet_FBC_Gain_Red ( void );
    int Tv_FactorySet_FBC_Gain_Green( int value );
    int Tv_FactoryGet_FBC_Gain_Green( void );
    int Tv_FactorySet_FBC_Gain_Blue( int value );
    int Tv_FactoryGet_FBC_Gain_Blue ( void );
    int Tv_FactorySet_FBC_Offset_Red( int value );
    int Tv_FactoryGet_FBC_Offset_Red ( void );
    int Tv_FactorySet_FBC_Offset_Green( int value );
    int Tv_FactoryGet_FBC_Offset_Green( void );
    int Tv_FactorySet_FBC_Offset_Blue( int value );
    int Tv_FactoryGet_FBC_Offset_Blue ( void );
    int Tv_FactoryGetWhiteBalanceRedGain(int source_type, int colortemp_mode);
    int Tv_FactoryGetWhiteBalanceGreenGain(int source_type, int colortemp_mode);
    int Tv_FactoryGetWhiteBalanceBlueGain(int source_type, int colortemp_mode);
    int Tv_FactoryGetWhiteBalanceRedOffset(int source_type, int colortemp_mode);
    int Tv_FactoryGetWhiteBalanceGreenOffset(int source_type, int colortemp_mode);
    int Tv_FactoryGetWhiteBalanceBlueOffset(int source_type, int colortemp_mode);
    int Tv_FactorySetWhiteBalanceRedGain(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceGreenGain(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceBlueGain(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceRedOffset(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceGreenOffset(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceBlueOffset(int source_type, int colortemp_mode, int value);
    int Tv_FactorySetWhiteBalanceColorTempMode(int source_type, int colortemp_mode, int is_save);
    int Tv_FactoryGetWhiteBalanceColorTempMode(int source_type );
    int Tv_FactoryWhiteBalanceFormatInputFbcGainParams(int value);
    int Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(int value);
    int Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(int value);
    int Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(int value);
    int Tv_FactorySaveWhiteBalancePramas(int source_type, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset);
    int Tv_FactoryCloseWhiteBalanceGrayPattern();
    int Tv_FactoryOpenWhiteBalanceGrayPattern();
    int Tv_FactorySetWhiteBalanceGrayPattern(int value);
    int  Tv_FactoryGetWhiteBalanceGrayPattern();
    int Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(int Tempmode);
    int Tv_FactoryWhiteBalanceColorTempMappingFbc2G9(int Tempmode);
    int Tv_FactorySet_FBC_GrayPattern(int value);
    int Tv_FactoryOpen_FBC_GrayPattern();
    int Tv_FactoryClose_FBC_GrayPattern();
    int Tv_FactorySet_FBC_ColorTemp_Mode( int mode );
    int Tv_FactoryGet_FBC_ColorTemp_Mode ( void );
    int Tv_FactorySet_FBC_ColorTemp_Mode_N360( int mode );
    int Tv_FactoryGet_FBC_ColorTemp_Mode_N360 ( void );
    int Tv_FactorySet_FBC_LockN_state(int value);
    int Tv_FactorySet_FBC_WB_Initial( int status );
    int Tv_FactoryGet_FBC_WB_Initial ( void );
    virtual int Tv_FactorySet_FBC_ColorTemp_Batch(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
    virtual int Tv_FactoryGet_FBC_ColorTemp_Batch ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int Tv_FactorySet_WB_G9_To_FBC( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params );
    int Tv_FactoryGet_WB_G9_To_FBC ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int Tv_FactoryGetItemFromBatch(vpp_color_temperature_mode_t colortemp_mode, int item);
    int Tv_FactorySet_FBC_CM_OnOff( unsigned char status );
    int Tv_FactoryGet_FBC_CM_OnOff (void);
    int Tv_FactorySet_FBC_DNLP_OnOff( unsigned char status );
    int Tv_FactoryGet_FBC_DNLP_OnOff (void);
    int Tv_FactorySet_FBC_Gamma_OnOff( unsigned char status );
    int Tv_FactoryGet_FBC_Gamma_OnOff (void);
    int Tv_FactorySet_FBC_WhiteBalance_OnOff( unsigned char status );
    int Tv_FactoryGet_FBC_WhiteBalance_OnOff (void);
    int Tv_FactorySet_FBC_Thermal_State( int value );
    int Tv_FactorySet_FBC_backlight_onoff(int value);
    int Tv_FactoryGet_FBC_backlight_onoff ( void );
    int Tv_FactorySet_FBC_Auto_Backlight_OnOff(unsigned char status);
    int Tv_FactoryGet_FBC_Auto_Backlight_OnOff ( void );
    int Tv_FactoryGet_FBC_VIDEO_MUTE ( void );
    int Tv_FactorySet_FBC_LVDS_SSG_Set( int value );
    int Tv_FactorySet_FBC_LightSensor_Status_N310 ( int value );
    int Tv_FactoryGet_FBC_LightSensor_Status_N310 ( void );
    int Tv_FactorySet_FBC_Dream_Panel_Status_N310 ( int value );
    int Tv_FactoryGet_FBC_Dream_Panel_Status_N310 ( void );
    int Tv_FactorySet_FBC_MULT_PQ_Status_N310 ( int value );
    int Tv_FactoryGet_FBC_MULT_PQ_Status_N310 ( void );
    int Tv_FactorySet_FBC_MEMC_Status_N310 ( int value );
    int Tv_FactoryGet_FBC_MEMC_Status_N310 ( void );
    int Tv_FactorySet_FBC_ColorTemp_Mode_N310( int mode );
    int Tv_FactoryGet_FBC_ColorTemp_Mode_N310 ( void );
    virtual int Tv_FactorySet_FBC_Backlight_N310 ( int value );
    virtual int Tv_FactoryGet_FBC_Backlight_N310 ( void );
    int Tv_FactorySet_FBC_Bluetooth_IIS_N310 ( int value );
    int Tv_FactoryGet_FBC_Bluetooth_IIS_N310 ( void );
    int Tv_FactorySet_FBC_Led_N310 ( int val_1, int val_2, int val_3 );
    int Tv_FactorySet_VbyOne_Spread_Spectrum_N311 ( int value );
    int Tv_FactoryGet_VbyOne_Spread_Spectrum_N311 ( void );
    int Tv_FactorySet_FBC_AP_STANDBY_N310 ( int value );
    int Tv_FactoryGet_FBC_AP_STANDBY_N310( void );
    int Tv_FactorySet_Uboot_Stage(int value);
    //end TV TO FBC

    int Tv_SetTestPattern(int value);
    int Tv_Set2k4k_ScalerUp_Mode ( int value );
    int Tv_Get2k4k_ScalerUp_Mode ( void );
    //audio
    virtual int Tv_SetDRC_OnOff(int on_off);
    virtual int Tv_GetDRC_OnOff(void);
    virtual void updateSubtitle(int, int);
    int setSubtitleBuffer(char *);
    int initSubtitle(int, int) ;
    int lockSubtitle();
    int unlockSubtitle();
    int getSubSwitchStatus();
    int startSubtitle(int dmx_id, int pid, int page_id, int anc_page_id) ;
    int stopSubtitle() ;

    //audio
    virtual void TvAudioOpen();
    virtual void AudioCtlUninit();
    virtual int SetAudioMuteForSystem(int muteOrUnmute);
    virtual int GetAudioMuteForSystem();
    virtual int SetAudioMuteForTv(int muteOrUnmute);
    virtual int GetAudioMuteForTv();
    virtual char *GetMainVolLutTableExtraName();
    virtual int SetDbxTvMode(int mode, int son_value, int vol_value, int sur_value);
    virtual int GetDbxTvMode(int *mode, int *son_value, int *vol_value, int *sur_value);
    int AudioHandleHeadsetPlugIn();
    int AudioHandleHeadsetPullOut();
    int SetAudioAVOutMute(int muteStatus);
    int GetAudioAVOutMute();
    int SetAudioSPDIFMute(int muteStatus);
    int GetAudioSPDIFMute();
    int SetDacMute(int muteStatus, int mute_type);
    int SetAudioI2sMute(int);
    int SetAudioMasterVolume(int tmp_vol);
    int GetAudioMasterVolume();
    int SaveCurAudioMasterVolume(int tmp_vol);
    int GetCurAudioMasterVolume();
    int SetAudioBalance(int tmp_val);
    int GetAudioBalance();
    int SaveCurAudioBalance(int tmp_val);
    int GetCurAudioBalance();
    int SetAudioSupperBassVolume(int tmp_vol);
    int GetAudioSupperBassVolume();
    int SaveCurAudioSupperBassVolume(int tmp_vol);
    int GetCurAudioSupperBassVolume();
    int SetAudioSupperBassSwitch(int tmp_val);
    int GetAudioSupperBassSwitch();
    int SaveCurAudioSupperBassSwitch(int tmp_val);
    int GetCurAudioSupperBassSwitch();
    int SetAudioSRSSurround(int tmp_val);
    int GetAudioSRSSurround();
    int SaveCurAudioSrsSurround(int tmp_val);
    int GetCurAudioSRSSurround();
    int SetAudioSrsDialogClarity(int tmp_val);
    int GetAudioSrsDialogClarity();
    int SaveCurAudioSrsDialogClarity(int tmp_val);
    int GetCurAudioSrsDialogClarity();
    int SetAudioSrsTruBass(int tmp_val);
    int GetAudioSrsTruBass();
    int SaveCurAudioSrsTruBass(int tmp_val);
    int GetCurAudioSrsTruBass();
    int SetAudioSPDIFSwitch(int tmp_val);
    int GetCurAudioSPDIFSwitch();
    int SaveCurAudioSPDIFSwitch(int tmp_val);

    //Audio SPDIF Mode
    int SetAudioSPDIFMode(int tmp_val);
    int GetCurAudioSPDIFMode();
    int SaveCurAudioSPDIFMode(int tmp_val);
    int SetAudioBassVolume(int tmp_vol);
    int GetAudioBassVolume();
    int SaveCurAudioBassVolume(int tmp_vol);
    int GetCurAudioBassVolume();
    int SetAudioTrebleVolume(int tmp_vol);
    int GetAudioTrebleVolume();
    int SaveCurAudioTrebleVolume(int tmp_vol);
    int GetCurAudioTrebleVolume();
    int SetAudioSoundMode(int tmp_val);
    int GetAudioSoundMode();
    int SaveCurAudioSoundMode(int tmp_val);
    int GetCurAudioSoundMode();
    int SetAudioWallEffect(int tmp_val);
    int GetAudioWallEffect();
    int SaveCurAudioWallEffect(int tmp_val);
    int GetCurAudioWallEffect();
    int SetAudioEQMode(int tmp_val);
    int GetAudioEQMode();
    int SaveCurAudioEQMode(int tmp_val);
    int GetCurAudioEQMode();
    int GetAudioEQRange(int range_buf[]);
    int GetAudioEQBandCount();
    int SetAudioEQGain(int gain_buf[]);
    int GetAudioEQGain(int gain_buf[]);
    int GetCurAudioEQGain(int gain_buf[]);
    int SaveCurAudioEQGain(int gain_buf[]);
    int SetAudioEQSwitch(int switch_val);
    int OpenAmAudio(unsigned int sr, int input_device, int output_device);
    int CloseAmAudio(void);
    int SetAmAudioInputSr(unsigned int sr, int output_device);
    int SetAmAudioOutputMode(int mode);
    int SetAmAudioMusicGain(int gain);
    int SetAmAudioLeftGain(int gain);
    int SetAmAudioRightGain(int gain);
    int SetAudioVolumeCompensationVal(int tmp_vol_comp_val);
    int AudioLineInSelectChannel(int audio_channel);
    int AudioSetLineInCaptureVolume(int l_vol, int r_vol);
    int SetKalaokIO(int level);
    int setAudioPcmPlaybackVolume(int val);

    int openTvAudio();
    int closeTvAudio();

    int InitTvAudio(int sr, int input_device);
    int UnInitTvAudio();
    int AudioChangeSampleRate(int sr);
    int AudioSetAudioInSource(int audio_src_in_type);
    int AudioSetAudioSourceType(int source_type);
    int AudioSSMRestoreDefaultSetting();
    int SetAudioDumpDataFlag(int tmp_flag);
    int GetAudioDumpDataFlag();
    int SetAudioLeftRightMode(unsigned int mode);
    unsigned int GetAudioLeftRightMode();
    int SwitchAVOutBypass (int);
    int SetAudioSwitchIO(int value);
    int SetOutput_Swap(int value);
    int HanldeAudioInputSr(unsigned int);
    void AudioSetVolumeDigitLUTBuf(int lut_sel_flag, int *MainVolLutBuf);
    int SetADC_Digital_Capture_Volume(int value);
    int SetPGA_IN_Value(int value);
    int SetDAC_Digital_PlayBack_Volume(int value);
    int InitSetTvAudioCard();
    int UnInitSetTvAudioCard();
    void RefreshSrsEffectAndDacGain();
    int SetCustomEQGain();
    int SetAtvInGain(int gain_val);
    int GetHdmiAvHotplugDetectOnoff();
private:
    int SendCmdToOffBoardFBCExternalDac(int, int);
    int LoadCurAudioSPDIFMode();
    int LoadCurAudioMasterVolume();
    int LoadCurAudioBalance();
    int LoadCurAudioSupperBassVolume();
    int LoadCurAudioSupperBassSwitch();
    int LoadCurAudioSrsSurround();
    int LoadCurAudioSrsDialogClarity();
    int LoadCurAudioSPDIFSwitch();
    void SetSupperBassSRSSpeakerSize();
    int LoadCurAudioSoundMode();
    int LoadCurAudioEQMode();
    int LoadCurAudioSrsTruBass();
    int RealSaveCurAudioBassVolume(int, int);
    int LoadCurAudioBassVolume();
    int RealSaveCurAudioTrebleVolume(int, int);
    int LoadCurAudioTrebleVolume();
    int HandleTrebleBassVolume();
    int LoadCurAudioWallEffect();
    int RealReadCurAudioEQGain(int *);
    int RealSaveCurAudioEQGain(int *, int);
    int LoadCurAudioEQGain();
    int MappingEQGain(int *, int *, int);
    int RestoreToAudioDefEQGain(int *);
    int GetCustomEQGain(int *);
    int AudioSetEQGain(int *);
    int handleEQGainBeforeSet(int *, int *);
    int RealSetEQGain(int *);
    int SetSpecialModeEQGain(int);
    int SetSpecialIndexEQGain(int, int);
    int SaveSpecialIndexEQGain(int, int);
    void LoadAudioCtl();
    void InitSetAudioCtl();
    int GetBassUIMinGainVal();
    int GetBassUIMaxGainVal();
    int GetTrebleUIMinGainVal();
    int GetTrebleUIMaxGainVal();
    int MappingLine(int, int, int, int, int);
    int MappingTrebleBassAndEqualizer(int, int, int, int);
    int SetSPDIFMode(int mode_val);

    CAudioAlsa mAudioAlsa;
    CAudioEffect mAudioEffect;

    CAudioCustomerCtrl mCustomerCtrl;
    int mCurAudioMasterVolume;
    int mCurAudioBalance;
    int mCurAudioSupperBassVolume;
    int mCurAudioSupperBassSwitch;
    int mCurAudioSRSSurround;
    int mCurAudioSrsDialogClarity;
    int mCurAudioSrsTruBass;
    int mCurAudioSPDIFSwitch;
    int mCurAudioSPDIFMode;
    int mCurAudioBassVolume;
    int mCurAudioTrebleVolume;
    int mCurAudioSoundMode;
    int mCurAudioWallEffect;
    int mCurAudioEQMode;
    int mCustomAudioMasterVolume;
    int mCustomAudioBalance;
    int mCustomAudioSupperBassVolume;
    int mCustomAudioSupperBassSwitch;
    int mCustomAudioSRSSurround;
    int mCustomAudioSrsDialogClarity;
    int mCustomAudioSrsTruBass;
    int mCustomAudioBassVolume;
    int mCustomAudioTrebleVolume;
    int mCustomAudioSoundMode;
    int mCustomAudioWallEffect;
    int mCustomAudioEQMode;
    int mCustomAudioSoundEnhancementSwitch;
    int mCustomEQGainBuf[CC_BAND_ITEM_CNT];
    int mCurEQGainBuf[CC_BAND_ITEM_CNT] ;
    int8_t mCurEQGainChBuf[CC_BAND_ITEM_CNT];
    int mWallEffectValueBuf[CC_BAND_ITEM_CNT];
    int mVolumeCompensationVal;
    int mMainVolumeBalanceVal;
    //end audio

protected:
    class CTvMsgQueue: public CMsgQueueThread, public CAv::IObserver, public CTvScanner::IObserver , public CTvEpg::IObserver, public CFrontEnd::IObserver {
    public:
        static const int TV_MSG_COMMON = 0;
        static const int TV_MSG_STOP_ANALYZE_TS = 1;
        static const int TV_MSG_START_ANALYZE_TS = 2;
        static const int TV_MSG_CHECK_FE_DELAY = 3;
        static const int TV_MSG_AV_EVENT = 4;
        static const int TV_MSG_FE_EVENT = 5;
        static const int TV_MSG_SCAN_EVENT = 6;
        static const int TV_MSG_EPG_EVENT = 7;
        static const int TV_MSG_HDMI_SR_CHANGED = 8;
        static const int TV_MSG_ENABLE_VIDEO_LATER = 9;
        CTvMsgQueue(CTv *tv);
        ~CTvMsgQueue();
        //scan observer
        void onEvent ( const CTvScanner::ScannerEvent &ev );
        //epg observer
        void onEvent ( const CTvEpg::EpgEvent &ev );
        //FE observer
        void onEvent ( const CFrontEnd::FEEvent &ev );
        //AV
        void onEvent(const CAv::AVEvent &ev);
    private:
        virtual void handleMessage ( CMessage &msg );
        CTv *mpTv;
    };

    class CTvDetectObserverForScanner: public CTvin::CTvinSigDetect::ISigDetectObserver {
    public:
        CTvDetectObserverForScanner(CTv *);
        ~CTvDetectObserverForScanner() {};
        virtual void onSigToStable();
        virtual void onSigStableToUnstable();
        virtual void onSigStableToNoSig();
        virtual void onSigUnStableToNoSig();
        virtual void onSigStillStable();
    private:
        CTv *mpTv;
        int m_sig_stable_nums;
    };
    void onEnableVideoLater(int framecount);
    int resetDmxAndAvSource();
    int stopScan();
    int stopPlaying();
    void sendTvEvent ( const CTvEv &ev );
    int startPlayTv ( int source, int vid, int aid, int vfat, int afat );
    //scan observer
    void onEvent ( const CTvScanner::ScannerEvent &ev );
    //epg observer
    void onEvent ( const CTvEpg::EpgEvent &ev );
    //FE observer
    void onEvent ( const CFrontEnd::FEEvent &ev );
    //AV
    void onEvent(const CAv::AVEvent &ev);
    bool Tv_Start_Analyze_Ts ( int channelID );
    bool Tv_Stop_Analyze_Ts();
    int Tvin_Stop ( void );
    int Tvin_GetTvinConfig();
    int Tv_init_audio();
    int Tv_MiscSetBySource ( tv_source_input_t );
    void print_version_info ( void );
    int dtvCleanProgramByFreq ( int freq );
    /*********************** Audio start **********************/
    int SetAudioVolDigitLUTTable ( tv_source_input_t source_input );
    virtual int Tv_SetAudioInSource (tv_source_input_t source_input);
    void Tv_SetAudioOutputSwap_Type (tv_source_input_t source_input);
    void Tv_ADCDigitalCapture_Volume (void);
    void Tv_SetPGAIn_Gain (void);
    void Tv_SetDACDigitalPlayBack_Volume (int audio_src_in_type);
    void Tv_SetAVOutPut_Input_gain(tv_source_input_t source_input);
    /*********************** Audio end **********************/

    //
    virtual void onSigToStable();
    virtual void onSigStableToUnstable();
    virtual void onSigStableToUnSupport();
    virtual void onSigStableToNoSig();
    virtual void onSigUnStableToUnSupport();
    virtual void onSigUnStableToNoSig();
    virtual void onSigNullToNoSig();
    virtual void onSigNoSigToUnstable();
    virtual void onSigStillStable();
    virtual void onSigStillUnstable();
    virtual void onSigStillNosig();
    virtual void onSigStillNoSupport();
    virtual void onSigStillNull();
    virtual void onStableSigFmtChange();
    virtual void onStableTransFmtChange();
    virtual void onSigDetectEnter();
    virtual void onSigDetectLoop();

    virtual void onSourceConnect(int source_type, int connect_status);
    virtual void onHDMIRxCECMessage(int msg_len, unsigned char msg_buf[]);
    virtual void onUpgradeStatus(int status, int progress);
    virtual void onSerialCommunication(int dev_id, int rd_len, unsigned char data_buf[]);
    virtual void onThermalDetect(int state);
    virtual void onVframeSizeChange();

    CTvEpg mTvEpg;
    CTvScanner mTvScanner;
    mutable Mutex mLock;
    CTvTime mTvTime;
    CTvRecord mTvRec;
    CFrontEnd mFrontDev;
    CTvDimension mTvVchip;
    CTvSubtitle mTvSub;
    CAv  mAv;
    CTvDmx mTvDmx;
    CTvMsgQueue mTvMsgQueue;
    AutoBackLight mAutoBackLight;
    CAutoPQparam mAutoPQparam;
    CTvDetectObserverForScanner mTvScannerDetectObserver;
    //
    volatile int mTvAction;
    volatile TvRunStatus_t mTvStatus;
    volatile tv_source_input_t m_source_input;
    volatile tv_source_input_t m_last_source_input;
    /* for tvin window mode and pos*/
    tvin_window_pos_t m_win_pos;
    tv_window_mode_t m_win_mode;
    int m_blackout_enable;//1 enable 0 disable
    int m_cur_playing_prog_id;
    bool mHdmiOutFbc;
    CFbcCommunication *fbcIns;


    bool mlastlockstatus;
    String8 mlastdm;
    String8 mlastabbrev;
    String8 mlastabbtext;
    pthread_t vchipthread;
    int mvchip_running;
    friend class CTvMsgQueue;
    int mCurAnalyzeTsChannelID;
    TvIObserver *mpObserver;
    tv_dtv_scan_running_status_t mDtvScanRunningStatus;
    volatile  tv_config_t gTvinConfig;
    int dtv_auto_3d_flag;
    bool mAutoSetDisplayFreq;
    int m_sig_stable_nums;
    bool mSetHdmiEdid;
    /** for L/R or B/T 3d mode overscan **/
    /** for 3D **/
    VIDEO_3D_MODE_T m_mode_3d;
    /** for HDMI-in sampling detection. **/
    int  m_hdmi_sampling_rate;
    int	m_hdmi_audio_data;
    /** for display mode to bottom **/
    int screen_mode;

    //audio mute
    int mAudioMuteStatusForTv;
    int mAudioMuteStatusForSystem;
    char mMainVolLutTableExtraName[16];
};

#endif  //_CDTV_H


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
#include "../tvin/CHDMIRxManager.h"
#include <CMsgQueue.h>
#include <serial_operate.h>
#include "CTvRecord.h"
#include "CTvSubtitle.h"
#include "CAv.h"
#include "CTvDmx.h"
#include "../audio/CTvAudio.h"
#include "AutoBackLight.h"
#include "CAutoPQparam.h"
#include "tvin/CSourceConnectDetect.h"
#include "fbcutils/fbcutils.h"

#include <CTvFactory.h>

using namespace android;

static const char *TV_CONFIG_FILE_PATH = "/param/tvconfig.conf";
static const char *TV_DB_PATH = "/param/dtv.db";
static const char *TV_CONFIG_FILE_SYSTEM_PATH = "/system/etc/tvconfig.conf";
static const char *TV_CONFIG_FILE_PARAM_PATH = "/param/tvconfig.conf";
static const char *TV_CHANNEL_LIST_SYSTEM_PATH = "/system/etc/tv_default.xml";
static const char *TV_CHANNEL_LIST_PARAM_PATH = "/param/tv_default.xml";
static const char *TV_SSM_DATA_SYSTEM_PATH = "/system/etc/ssm_data";
static const char *TV_SSM_DATA_PARAM_PATH = "/param/ssm_data";

#define BL_LOCAL_DIMING_FUNC_ENABLE "/sys/class/aml_ldim/func_en"
#define DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN "/sys/class/tsync/av_threshold_min"
#define AV_THRESHOLD_MIN_MS "540000" //6S = 6*90000

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

class CTv : public CTvin::CTvinSigDetect::ISigDetectObserver, public CSourceConnectDetect::ISourceConnectObserver, public IUpgradeFBCObserver, public CTvSubtitle::IObserver, public  CTv2d4GHeadSetDetect::IHeadSetObserver {
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
    virtual int atvAutoScan(int videoStd, int audioStd, int searchType, int procMode);
    virtual int pauseScan();
    virtual int resumeScan();
    virtual void setDvbTextCoding(char *coding);

    virtual int clearAllProgram(int arg0);
    virtual int clearDbAllProgramInfoTable();
    virtual void setSourceSwitchAndPlay();
    virtual int atvMunualScan ( int startFreq, int endFreq, int videoStd, int audioStd, int store_Type = 0, int channel_num = 0 );
    virtual int stopScanLock();
    virtual void SetRecordFileName ( char *name );
    virtual  void StartToRecord();
    virtual void StopRecording();
    virtual int playDvbcProgram ( int progId );
    virtual int playDtmbProgram ( int progId );
    virtual int playAtvProgram ( int, int, int, int, int);
    virtual int playDtvProgram ( int, int, int, int, int, int, int, int, int, int);
    virtual int stopPlayingLock();
    virtual int resetFrontEndPara ( frontend_para_set_t feParms );
    virtual int SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt );
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
    int getFrontendSignalStrength();
    int getFrontendSNR();
    int getFrontendBER();
    int getChannelInfoBydbID ( int dbID, channel_info_t &chan_info );
    int setBlackoutEnable(int enable);
    int getSaveBlackoutEnable();
    int saveATVProgramID ( int dbID );
    int getATVProgramID ( void );
    int saveDTVProgramID ( int dbID );
    int getDTVProgramID ( void );
    int getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq );
    int getAverageLuma();
    int setAutobacklightData(const char *value);
    int getAutoBacklightData(int *data);
    int getAutoBackLightStatus();
    int setAutoBackLightStatus(int status);

    virtual int Tv_SSMRestoreDefaultSetting();


    int GetSourceConnectStatus(tv_source_input_t source_input);
    int IsDVISignal();
    int isVgaFmtInHdmi();

    int getHDMIFrameRate ( void );
    void RefreshAudioMasterVolume ( tv_source_input_t source_input );

    int Tvin_SetPLLValues ();
    int SetCVD2Values ();
    unsigned int Vpp_GetDisplayResolutionInfo(tvin_window_pos_t *win_pos);
    //SSM
    virtual int Tv_SSMFacRestoreDefaultSetting();
    int StartHeadSetDetect();
    virtual void onHeadSetDetect(int state, int para);

    CTvFactory mFactoryMode;
    CTvin::CTvinSigDetect mSigDetectThread;
    CSourceConnectDetect mSourceConnectDetectThread;
    CHDMIRxManager mHDMIRxManager;

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

    int Tv_GetProjectInfo(project_info_t *ptrInfo);
    int Tv_GetPlatformType();
    int Tv_HDMIEDIDFileSelect(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version);
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
    int setEyeProtectionMode(int enable);
    int getEyeProtectionMode();
    int SetHdmiColorRangeMode(tv_hdmi_color_range_t range_mode);
    tv_hdmi_color_range_t GetHdmiColorRangeMode();
    int setGamma(vpp_gamma_curve_t gamma_curve, int is_save);

    //audio
    virtual void updateSubtitle(int, int);

    //audio
    virtual void TvAudioOpen();
    virtual void AudioCtlUninit();
    virtual int SetAudioMuteForSystem(int muteOrUnmute);
    virtual int GetAudioMuteForSystem();
    virtual int SetAudioMuteForTv(int muteOrUnmute);
    virtual char *GetMainVolLutTableExtraName();
    virtual int SetDbxTvMode(int mode, int son_value, int vol_value, int sur_value);
    virtual int GetDbxTvMode(int *mode, int *son_value, int *vol_value, int *sur_value);
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
    int setAmAudioPreGain(float pre_gain);
    float getAmAudioPreGain();
    int setAmAudioPreMute(int mute);
    int getAmAudioPreMute();

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
    int SetHdmiEdidVersion(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version);
    int SetHdmiHDCPSwitcher(tv_hdmi_hdcpkey_enable_t enable);
    int SetVideoAxis(int x, int y, int width, int heigth);

    void dump(String8 &result);
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
    int setAudioPreGain(tv_source_input_t source_input);
    float getAudioPreGain(tv_source_input_t source_input);

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
        static const int TV_MSG_SCANNING_FRAME_STABLE = 10;
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
    int startPlayTv ( int source, int vid, int aid, int pcrid, int vfat, int afat );
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
    void Tv_SetDACDigitalPlayBack_Volume (int audio_src_in_type);
    void Tv_SetAVOutPut_Input_gain(tv_source_input_t source_input);
    /*********************** Audio end **********************/

    void Tv_Spread_Spectrum();
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
    virtual void onUpgradeStatus(int status, int progress);
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
    CAv mAv;
    CTvDmx mTvDmx;
    CTvMsgQueue mTvMsgQueue;
    AutoBackLight mAutoBackLight;
    CTvDetectObserverForScanner mTvScannerDetectObserver;
    //
    volatile int mTvAction;
    volatile TvRunStatus_t mTvStatus;
    volatile tv_source_input_t m_source_input;
    volatile tv_source_input_t m_last_source_input;
    /* for tvin window mode and pos*/
    tvin_window_pos_t m_win_pos;
    tv_window_mode_t m_win_mode;
    bool mBlackoutEnable;// true: enable false: disable
    int m_cur_playing_prog_id;
    bool mHdmiOutFbc;
    CFbcCommunication *fbcIns;

    //friend class CTvMsgQueue;
    int mCurAnalyzeTsChannelID;
    TvIObserver *mpObserver;
    tv_dtv_scan_running_status_t mDtvScanRunningStatus;
    volatile tv_config_t gTvinConfig;
    bool mAutoSetDisplayFreq;
    int m_sig_stable_nums;
    bool mSetHdmiEdid;
    /** for HDMI-in sampling detection. **/
    int  m_hdmi_sampling_rate;
    int	m_hdmi_audio_data;
    /** for display mode to bottom **/

    //audio mute
    int mAudioMuteStatusForTv;
    int mAudioMuteStatusForSystem;
    char mMainVolLutTableExtraName[CC_PROJECT_INFO_ITEM_MAX_LEN];

    CTvin *mpTvin;
};

#endif  //_CDTV_H

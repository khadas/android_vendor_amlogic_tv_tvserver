#ifndef __TV_AUDIO_CFG_H__
#define __TV_AUDIO_CFG_H__

#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"

#include "../tvin/CTvin.h"

#define CC_GET_LUT_TV                           (0)
#define CC_GET_LUT_AV                           (1)
#define CC_GET_LUT_COMP                         (2)
#define CC_GET_LUT_HDMI                         (3)
#define CC_GET_LUT_VGA                          (4)
#define CC_GET_LUT_MPEG                         (5)
#define CC_GET_LUT_HDMI_4K2K                    (6)
#define CC_GET_LUT_USB_4K2K                     (7)
#define CC_GET_TYPE_CNT                         (8)

#define CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD               (1)
#define CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC          (2)
#define CC_DAC_G9TV_EXTERNAL_DAC_CUSTOMER_LIB           (3)
#define CC_DAC_G9TV_EXTERNAL_DAC_DIGITAL                (4)
#define CC_DAC_G9TV_INTERNAL_DAC                            (5)

#define CC_AUD_RESAMPLE_TYPE_HW                                         (1)
#define CC_AUD_RESAMPLE_TYPE_SW                                         (2)

int GetAudioAmpMasterNolinePointData(int get_type, const char *value_buf, int data_buf[]);
int GetAudioEQPresetBufferPtr(int * );
int GetAudioAmpMainvolBuf(const char *TableKeyName, int data_buf[]);
int GetAudioAmpSupbassvolBuf(int get_type, int data_buf[]);
int GetAudioAmplifierMasterNoLineSwitchFlag();
int GetAudioAmplifierMasterNoLinePointsCount();
int GetAudioSupperBassSwitchDisableCFG();
int GetAudioAmplifierSupperBassNoLineSwitchFlag();
int GetAudioAmplifierSupperBassNoLinePointsCount();
int GetAudioAmplifierBalanceExchangeCFG();
int GetAudioMainVolLutBufNameModifyUbootenvCFG();
int GetAudioMainVolLutBufNameModifyEnableCFG();
int GetAudioSupperBassVolLutBufNameModifyEnableCFG();

int GetAudioDVISupportEnable();
int GetTvAudioCardName(char tv_card_name_buf[]);
int GetTvAudioCardNeedSet();
int GetAudioDumpDataEnableFlagCfg();
int GetAudioWallEffectTypeCfg();

//srs user setting
int GetAudioSRSSourroundEnableCFG();
int GetAudioSRSGainCfg(const char *cfg_name, int def_gain_val);
int GetAudioEffectAmplifierGainCfg(const char *cfg_name, int def_gain_val, int max_gain_val);
int GetAudioSRSSupperBassTrubassSpeakerSizeCfg();
//tinymix control
int Get2d4gHeadsetEnable() ;
int GetAudioOutputSwapStatus(tv_source_input_t source_input);
//hardware design
int GetAudioArchitectureTypeCFG();
const char *GetAudExtDacLibPath();
int GetAudioResampleTypeCFG();
int GetKaraokAvEnable();
//audio path gain setting by user
int GetADCDigitalCaptureVol_Cfg(void);
int GetAudioInternalDacPGAInGain_Cfg(void);
int GetAudioInternalDACDigitalPlayBackVolume_Cfg(int audio_src_in_type);
const char *GetAudioAmpMainvolTableBaseName(int get_type);
int GetDefault_EQGain_Table(int *EqTable);
int GetDefaultAvOutGainBuf(int *nAvoutTable);
int GetAvOutGainBuf_Cfg(int *nAvoutTable);

#endif  //__TV_AUDIO_CFG_H__

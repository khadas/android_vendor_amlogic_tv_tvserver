#ifndef __TV_SETTING_H__
#define __TV_SETTING_H__

#include <pthread.h>
#include <stdint.h>

#include "CTvSettingCfg.h"
#include "CTvSettingDeviceFactory.h"
#include "TvKeyData.h"
#include "../tv/CTvLog.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

bool CTvSettingLoad();
bool CTvSettingunLoad();

int CTvSettingdoSuspend();
int CTvSettingdoResume();
int MiscSSMRestoreDefault();
int MiscSSMFacRestoreDefault();
int ReservedSSMRestoreDefault();

int SSMSaveEEP_One_N310_N311(int offset, int rw_val) ;
int SSMReadEEP_One_N310_N311(int offset);
int SSMSaveEEP_N_N310_N311(int offset, int data_len, int *data_buf);
int SSMReadEEP_N_N310_N311(int offset, int data_len, int *data_buf);
//int EEPWriteNByte(int offset, int data_len, unsigned char * data_buf);
//int EEPReadNByte(int offset, int data_len, unsigned char * data_buf);
int SSMSaveFlash_One_N310_N311(int offset, int rw_val);
int SSMReadFlash_One_N310_N311(int offset);
int SSMSaveFlash_N_N310_N311(int offset, int data_len, int *data_buf);
int SSMReadFlash_N_N310_N311(int offset, int data_len, int *data_buf);

int SSMSaveBurnWriteCharaterChar(int rw_val);
int SSMReadBurnWriteCharaterChar();
int SSMSaveFactoryBurnMode(int rw_val);
int SSMReadFactoryBurnMode();
int SSMSavePowerOnOffChannel(int rw_val);
int SSMReadPowerOnOffChannel();
int SSMSaveLastSelectSourceInput(int rw_val);
int SSMReadLastSelectSourceInput();
int SSMSaveSystemLanguage(int rw_val);
int SSMReadSystemLanguage();
int SSMSaveAgingMode(int rw_val);
int SSMReadAgingMode();
int SSMSavePanelType(int rw_val);
int SSMReadPanelType();
int SSMSavePowerOnMusicSwitch(int rw_val);
int SSMReadPowerOnMusicSwitch();
int SSMSavePowerOnMusicVolume(int rw_val);
int SSMReadPowerOnMusicVolume();
int SSMSaveSystemSleepTimer(int rw_val);
int SSMReadSystemSleepTimer();
int SSMSaveInputSourceParentalControl(int source_index, unsigned char ctl_flag);
int SSMReadInputSourceParentalControl(int source_index);
int SSMSaveParentalControlSwitch(int rw_val);
int SSMReadParentalControlSwitch();
int SSMGetCustomerDataStart();
int SSMGetCustomerDataLen();
int SSMGetATVDataStart();
int SSMGetATVDataLen();
int SSMGetVPPDataStart();
int SSMGetVPPDataLen();
int SSMSaveSearchNavigateFlag(int rw_val);
int SSMReadSearchNavigateFlag();
int SSMSaveInputNumLimit(int rw_val);
int SSMReadInputNumLimit();
int SSMSaveLocalDimingOnOffFlg(int rw_val);
int SSMReadLocalDimingOnOffFlg();
int SSMSaveVDac2DValue(unsigned short rw_val);
int SSMReadVDac2DValue();
int SSMSaveVDac3DValue(unsigned short rw_val);
int SSMReadVDac3DValue();
int SSMSaveChromaStatus(int mode);
int SSMSaveNonStandardValue(unsigned short rw_val);
int SSMReadNonStandardValue(void);
int SSMSaveAdbSwitchValue(int rw_val);
int SSMReadAdbSwitchValue(void);
int SSMSaveSerialCMDSwitchValue(int rw_val);
int SSMReadSerialCMDSwitchValue(void);
int SSMSaveNoiseGateThresholdValue(int rw_val);
int SSMReadNoiseGateThresholdValue(void);
int SSMSaveGraphyBacklight(int rw_val);
int SSMReadGraphyBacklight(void);
int SSMSaveFastSuspendFlag(int rw_val);
int SSMReadFastSuspendFlag(void);
int SSMSaveCABufferSizeValue(unsigned short rw_val);
int SSMReadCABufferSizeValue(void);
int SSMSaveStandbyMode(int rw_val);
int SSMReadStandbyMode();
int SSMSaveHDMIEQMode(int rw_val);
int SSMReadHDMIEQMode();
int SSMSaveLogoOnOffFlag(int rw_val);
int SSMReadLogoOnOffFlag();
int SSMSaveHDMIInternalMode(unsigned int rw_val);
int SSMReadHDMIInternalMode();
int SSMSaveParentalControlPassWord(unsigned char *password, int size);
int SSMReadParentalControlPassWord(unsigned short  *password);
int SSMSaveDisable3D(int rw_val);
int SSMReadDisable3D();
int SSMSaveGlobalOgoEnable(int rw_val);
int SSMReadGlobalOgoEnable();
int SSMDeviceMarkCheck();
int SSMRestoreDeviceMarkValues();
int SSMHandlePreCopying();
int SSMSaveDTVType(int rw_val);
int SSMReadDTVType(int *rw_val);

int GetSSMCfgBufferData(const char *key_str, int *buf_item_count, int radix, unsigned char data_buf[]);

int SSMSaveSourceInput(unsigned char rw_val);
int SSMReadSourceInput();
int SSMSaveCVBSStd(unsigned char rw_val);
int SSMReadCVBSStd(unsigned char *rw_val);
int SSMSave3DMode(unsigned char rw_val);
int SSMRead3DMode(unsigned char *rw_val);
int SSMSave3DLRSwitch(unsigned char rw_val);
int SSMRead3DLRSwitch(unsigned char *rw_val);
int SSMSave3DDepth(unsigned char rw_val);
int SSMRead3DDepth(unsigned char *rw_val);
int SSMSave3DTO2D(unsigned char rw_val);
int SSMRead3DTO2D(unsigned char *rw_val);
int SSMSaveBrightness(int offset, int rw_val);
int SSMReadBrightness(int offset, int *rw_val);
int SSMSaveContrast(int offset, int rw_val);
int SSMReadContrast(int offset, int *rw_val);
int SSMSaveSaturation(int offset, int rw_val);
int SSMReadSaturation(int offset, int *rw_val);
int SSMSaveHue(int offset, int rw_val);
int SSMReadHue(int offset, int *rw_val);
int SSMSaveSharpness(int offset, int rw_val);
int SSMReadSharpness(int offset, int *rw_val);
int SSMSaveSceneMode(int rw_val);
int SSMReadSceneMode(int *rw_val);
int SSMSavePictureMode(int offset, int rw_val);
int SSMReadPictureMode(int offset, int *rw_val);
int SSMSaveColorTemperature(int offset, int rw_val);
int SSMReadColorTemperature(int offset, int *rw_val);
int SSMSaveNoiseReduction(int offset, int rw_val);
int SSMReadNoiseReduction(int offset, int *rw_val);
int SSMSaveDisplayMode(int offset, int rw_val);
int SSMReadDisplayMode(int offset, int *rw_val);
int SSMSaveBackLightVal(int offset, int rw_val);
int SSMReadBackLightVal(int offset, int *rw_val);

int SSMReadFBCN360BackLightVal(int *rw_val);
int SSMSaveFBCN360BackLightVal(int rw_val);
int SSMSaveFBCELECmodeVal(int rw_val);
int SSMReadFBCELECmodeVal(int *rw_val);
int SSMSaveFBCN360ColorTempVal(int rw_val);
int SSMReadFBCN360ColorTempVal(int *rw_val);



int SSMSaveColorDemoMode(unsigned char rw_val);
int SSMReadColorDemoMode(unsigned char *rw_val);
int SSMSaveColorBaseMode(unsigned char rw_val);
int SSMReadColorBaseMode(unsigned char *rw_val);
int SSMSaveRGBGainRStart(int offset, unsigned int rw_val);
int SSMReadRGBGainRStart(int offset, unsigned int *rw_val);
int SSMSaveRGBGainGStart(int offset, unsigned int rw_val);
int SSMReadRGBGainGStart(int offset, unsigned int *rw_val);
int SSMSaveRGBGainBStart(int offset, unsigned int rw_val);
int SSMReadRGBGainBStart(int offset, unsigned int *rw_val);
int SSMSaveRGBPostOffsetRStart(int offset, int rw_val);
int SSMReadRGBPostOffsetRStart(int offset, int *rw_val);
int SSMSaveRGBPostOffsetGStart(int offset, int rw_val);
int SSMReadRGBPostOffsetGStart(int offset, int *rw_val);
int SSMSaveRGBPostOffsetBStart(int offset, int rw_val);
int SSMReadRGBPostOffsetBStart(int offset, int *rw_val);
int SSMSaveRGBValueStart(int offset, int8_t rw_val);
int SSMReadRGBValueStart(int offset, int8_t *rw_val);
int SSMSaveDBCStart(unsigned char rw_val);
int SSMReadDBCStart(unsigned char *rw_val);
int SSMSaveColorSpaceStart(unsigned char rw_val);
int SSMReadColorSpaceStart(unsigned char *rw_val);
int SSMSaveDnlpStart(unsigned char rw_val);
int SSMReadDnlpStart(unsigned char *rw_val);
int SSMSavePanoramaStart(int offset, unsigned char rw_val);
int SSMReadPanoramaStart(int offset, unsigned char *rw_val);
int SSMSaveTestPattern(unsigned char rw_val);
int SSMReadTestPattern(unsigned char *rw_val);
int SSMSaveAPL(unsigned char rw_val);
int SSMReadAPL(unsigned char *rw_val);
int SSMSaveAPL2(unsigned char rw_val);
int SSMReadAPL2(unsigned char *rw_val);
int SSMSaveBD(unsigned char rw_val);
int SSMReadBD(unsigned char *rw_val);
int SSMSaveBP(unsigned char rw_val);
int SSMReadBP(unsigned char *rw_val);
int SSMSaveDDRSSC(unsigned char rw_val);
int SSMReadDDRSSC(unsigned char *rw_val);
int SSMSaveLVDSSSC(unsigned char *rw_val);
int SSMReadLVDSSSC(unsigned char *rw_val);
int SSMSaveDreamPanel(unsigned char rw_val);
int SSMReadDreamPanel(unsigned char *rw_val);
int SSMSaveUserNatureLightSwitch(unsigned char rw_val);
int SSMReadUserNatureLightSwitch(unsigned char *rw_val);
int SSMSaveDBCBacklightEnable(unsigned char rw_val);
int SSMReadDBCBacklightEnable(unsigned char *rw_val);
int SSMSaveDBCBacklightStd(unsigned char rw_val);
int SSMReadDBCBacklightStd(unsigned char *rw_val);
int SSMSaveDBCEnable(unsigned char rw_val);
int SSMReadDBCEnable(unsigned char *rw_val);
int SSMSaveGammaValue(int rw_val);
int SSMReadGammaValue(int *rw_val);
int SSMSaveBackLightReverse(unsigned char rw_val);
int SSMReadBackLightReverse(unsigned char *rw_val);

int SSMSaveAudioMasterVolume(int8_t rw_val);
int SSMReadAudioMasterVolume(int8_t *rw_val);
int SSMSaveAudioBalanceVal(int8_t rw_val);
int SSMReadAudioBalanceVal(int8_t *rw_val);
int SSMSaveAudioSupperBassVolume(int8_t rw_val);
int SSMReadAudioSupperBassVolume(int8_t *rw_val);
int SSMSaveAudioSupperBassSwitch(int8_t rw_val);
int SSMReadAudioSupperBassSwitch(int8_t *rw_val);
int SSMSaveAudioSRSSurroundSwitch(int8_t rw_val);
int SSMReadAudioSRSSurroundSwitch(int8_t *rw_val);
int SSMSaveAudioSRSDialogClaritySwitch(int8_t rw_val);
int SSMReadAudioSRSDialogClaritySwitch(int8_t *rw_val);
int SSMSaveAudioSRSTruBassSwitch(int8_t rw_val);
int SSMReadAudioSRSTruBassSwitch(int8_t *rw_val);
int SSMSaveAudioBassVolume(int8_t rw_val);
int SSMReadAudioBassVolume(int8_t *rw_val);
int SSMSaveAudioTrebleVolume(int8_t rw_val);
int SSMReadAudioTrebleVolume(int8_t *rw_val);
int SSMSaveAudioSoundModeVal(int8_t rw_val);
int SSMReadAudioSoundModeVal(int8_t *rw_val);
int SSMSaveAudioWallEffectSwitch(int8_t rw_val);
int SSMReadAudioWallEffectSwitch(int8_t *rw_val);
int SSMSaveAudioSPDIFSwitchVal(int8_t rw_val);
int SSMReadAudioSPDIFSwitchVal(int8_t *rw_val);
int SSMSaveAudioSPDIFModeVal(int8_t rw_val);
int SSMReadAudioSPDIFModeVal(int8_t *rw_val);
int SSMSaveAudioEQModeVal(int8_t rw_val);
int SSMReadAudioEQModeVal(int8_t *rw_val);
int SSMSaveAudioEQGain(int offset, int size, int8_t tmp_buf[]);
int SSMReadAudioEQGain(int offset, int size, int8_t tmp_buf[]);
int SSMSaveAudioDbxTvValue(int son_value, int vol_value, int sur_value);
int SSMReadAudioDbxTvValue(int *son_value, int *vol_value, int *sur_value);
int SSMSaveAudioAVOutMuteVal(int8_t rw_val);
int SSMReadAudioAVOutMuteVal(int8_t *rw_val);
int SSMSaveAudioSPIDFMuteVal(int8_t rw_val);
int SSMReadAudioSPIDFMuteVal(int8_t *rw_val);

int SSMSaveBlackoutEnable(int8_t enable);
int SSMReadBlackoutEnable(int8_t *enable);
int SSMSaveFBCN310BackLightVal(int rw_val);
int SSMReadFBCN310BackLightVal(int *rw_val);
int SSMSaveFBCN310ColorTempVal(int rw_val);
int SSMReadFBCN310ColorTempVal(int *rw_val);
int SSMSaveFBCN310LightsensorVal(int rw_val);
int SSMReadFBCN310LightsensorVal(int *rw_val);
int SSMSaveFBCN310Dream_PanelVal(int rw_val);
int SSMReadFBCN310Dream_PanelVal(int *rw_val);
int SSMSaveFBCN310MULT_PQVal(int rw_val);
int SSMReadFBCN310MULT_PQVal(int *rw_val);
int SSMSaveFBCN310MEMCVal(int rw_val);
int SSMReadFBCN310MEMCVal(int *rw_val) ;
int SSMSaveN311_VbyOne_Spread_Spectrum_Val(int rw_val);
int SSMReadN311_VbyOne_Spread_Spectrum_Val(int *rw_val);
int SSMSaveN311_Bluetooth_Vol(int rw_val);
int SSMReadN311_Bluetooth_Vol(void) ;
int SSMSave_DRC_ONOFF_Val(int rw_val);
int SSMRead_DRC_ONOFF_Val(void);
int SSMSave_PANEL_ID_Val(int rw_val);
int SSMRead_PANEL_ID_Val(void);
#ifdef __cplusplus
}
#endif

#endif  //__TV_SSM_API_H__

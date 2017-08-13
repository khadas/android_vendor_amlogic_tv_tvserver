/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _CTV_FACTORY_H_
#define _CTV_FACTORY_H_

#include <utils/String8.h>
#include "CAv.h"
#include "../vpp/CVpp.h"
#include "../tvin/CTvin.h"
#include "fbcutils/CFbcCommunication.h"

using namespace android;

class CTvFactory {
public:
    CTvFactory();
    virtual ~CTvFactory() {};

    void init();
    int setPQModeBrightness (int sourceType, int pqMode, int brightness);
    int getPQModeBrightness (int tv_source_input, int pqMode);
    int setPQModeContrast (int sourceType, int pqMode, int contrast);
    int getPQModeContrast (int tv_source_input, int pqMode);
    int setPQModeSaturation ( int tv_source_input, int pqMode, int saturation );
    int getPQModeSaturation ( int tv_source_input, int pqMode );
    int setPQModeHue ( int tv_source_input, int pqMode, int hue );
    int getPQModeHue ( int tv_source_input, int pqMode );
    int setPQModeSharpness ( int tv_source_input, int pqMode, int sharpness );
    int getPQModeSharpness ( int tv_source_input, int pqMode );

    int getColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int setTestPattern ( int pattern );
    int getTestPattern ();
    int setScreenColor ( int vdinBlendingMask, int y, int u, int v );
    int resetPQMode ();
    int resetColorTemp ();
    int setParamsDefault ();
    int setDDRSSC ( int step );
    int getDDRSSC ();
    int setLVDSSSC ( int step );
    int getLVDSSSC ();
    int setNolineParams ( int nolineParamstype, int tv_source_input, noline_params_t nolineParams );
    noline_params_t getNolineParams ( int nolineParamstype, int tv_source_input );
    int setOverscan ( int tv_source_input, int fmt, int transFmt, tvin_cutwin_t cutwin );
    tvin_cutwin_t getOverscan ( int tv_source_input, int fmt, int transFmt );
    int replacePQDb(const char *newFilePath = NULL);
    int setGamma(tcon_gamma_table_t gamma_r, tcon_gamma_table_t gamma_g, tcon_gamma_table_t gamma_b);
    int setRGBPattern(int r, int g, int b);
    int getRGBPattern();

    //end PQ

    //TV TO FBC
    int fbcSetBrightness ( int value );
    int fbcGetBrightness ();
    int fbcSetContrast( int value );
    int fbcGetContrast ();
    int fbcSetSaturation( int value );
    int fbcGetSaturation ();
    int fbcSetHueColorTint( int value );
    int fbcGetHueColorTint ();
    virtual int fbcSetBacklight ( int value );
    virtual int fbcGetBacklight ();
    int Tv_FactorySet_FBC_Backlight_N360 ( int value );
    int Tv_FactoryGet_FBC_Backlight_N360 ();
    int fbcSetElecMode( int value );
    int fbcGetElecMode( void );
    int fbcSetBacklightN360( int value );
    int fbcGetBacklightN360( void );
    int fbcSetPictureMode ( int mode );
    int fbcGetPictureMode ();
    int fbcSetTestPattern ( int mode );
    int fbcGetTestPattern ();
    int fbcSelectTestPattern(int value);
    int fbcSetGainRed( int value );
    int fbcGetGainRed ();
    int fbcSetGainGreen( int value );
    int fbcGetGainGreen( void );
    int fbcSetGainBlue( int value );
    int fbcGetGainBlue ();
    int fbcSetOffsetRed( int value );
    int fbcGetOffsetRed ();
    int fbcSetOffsetGreen( int value );
    int fbcGetOffsetGreen( void );
    int fbcSetOffsetBlue( int value );
    int fbcGetOffsetBlue ();
    int fbcSetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save);
    int whiteBalanceGainRedGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceGainGreenGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceGainBlueGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceOffsetRedGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceOffsetGreenGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceOffsetBlueGet(int tv_source_input, int colortemp_mode);
    int whiteBalanceGainRedSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceGainGreenSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceGainBlueSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceOffsetRedSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceOffsetGreenSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceOffsetBlueSet(int tv_source_input, int colortemp_mode, int value);
    int whiteBalanceColorTempModeSet(int tv_source_input, int colortemp_mode, int is_save);
    int whiteBalanceColorTempModeGet(int sourceType );

    int whiteBalancePramSave(int tv_source_input, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset);
    int whiteBalanceGrayPatternClose();
    int whiteBalanceGrayPatternOpen();
    int whiteBalanceGrayPatternSet(int value);
    int whiteBalanceGrayPatternGet();

    int fbcColorTempModeSet( int mode );
    int fbcColorTempModeGet ();
    int fbcColorTempModeN360Set( int mode );
    int fbcColorTempModeN360Get ();
    int fbcWBInitialSet( int status );
    int fbcWBInitialGet ();
    int fbcSetColorTemperature(vpp_color_temperature_mode_t temp_mode);

    int fbcSetThermalState( int value );
    int fbcBacklightOnOffSet(int value);
    int fbcBacklightOnOffGet ();
    int fbcSetAutoBacklightOnOff(unsigned char status);
    int fbcGetAutoBacklightOnOff ();
    int fbcGetVideoMute ();
    int fbcLvdsSsgSet( int value );
    int fbcLightSensorStatusN310Set ( int value );
    int fbcLightSensorStatusN310Get ();
    int fbcDreamPanelStatusN310Set ( int value );
    int fbcDreamPanelStatusN310Get ();
    int fbcMultPQStatusN310Set ( int value );
    int fbcMultPQStatusN310Get ();
    int fbcMemcStatusN310Set ( int value );
    int fbcMemcStatusN310Get ();
    //end TV TO FBC
private:
    int getItemFromBatch(vpp_color_temperature_mode_t colortemp_mode, int item);
    int formatInputGainParams(int value);
    int formatInputOffsetParams(int value);
    int formatOutputOffsetParams(int value);
    int formatOutputGainParams(int value);

    int colorTempBatchGet ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params );
    int colorTempBatchSet(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);

    int fbcGrayPatternSet(int value);
    int fbcGrayPatternOpen();
    int fbcGrayPatternClose();

    bool mHdmiOutFbc;
    CFbcCommunication *mFbcObj;
    CAv mAv;
};

#endif/*_CTV_FACTORY_H_*/


/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2016/02/25
 *  @par function description:
 *  - 1 tv factory test function
 */

#ifndef _CTV_FACTORY_H_
#define _CTV_FACTORY_H_

#include <utils/String8.h>
#include "CAv.h"
#include "../vpp/CVpp.h"
#include "../vpp/CPQdb.h"
#include "../tvin/CTvin.h"
#include "fbcutils/CFbcCommunication.h"

using namespace android;

class CTvFactory {
public:
    CTvFactory();
    virtual ~CTvFactory() {};

    void init();
    int setPQModeBrightness (int sourceType, int pqMode, int brightness);
    int getPQModeBrightness (int sourceType, int pqMode);
    int setPQModeContrast (int sourceType, int pqMode, int contrast);
    int getPQModeContrast (int sourceType, int pqMode);
    int setPQModeSaturation ( int sourceType, int pqMode, int saturation );
    int getPQModeSaturation ( int sourceType, int pqMode );
    int setPQModeHue ( int sourceType, int pqMode, int hue );
    int getPQModeHue ( int sourceType, int pqMode );
    int setPQModeSharpness ( int sourceType, int pqMode, int sharpness );
    int getPQModeSharpness ( int sourceType, int pqMode );

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
    int setNolineParams ( int nolineParamstype, int sourceType, noline_params_t nolineParams );
    noline_params_t getNolineParams ( int nolineParamstype, int sourceType );
    int setOverscan ( int sourceType, int fmt, int transFmt, tvin_cutwin_t cutwin );
    tvin_cutwin_t getOverscan ( int sourceType, int fmt, int transFmt );
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
    int whiteBalanceGainRedGet(int sourceType, int colortemp_mode);
    int whiteBalanceGainGreenGet(int sourceType, int colortemp_mode);
    int whiteBalanceGainBlueGet(int sourceType, int colortemp_mode);
    int whiteBalanceOffsetRedGet(int sourceType, int colortemp_mode);
    int whiteBalanceOffsetGreenGet(int sourceType, int colortemp_mode);
    int whiteBalanceOffsetBlueGet(int sourceType, int colortemp_mode);
    int whiteBalanceGainRedSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceGainGreenSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceGainBlueSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceOffsetRedSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceOffsetGreenSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceOffsetBlueSet(int sourceType, int colortemp_mode, int value);
    int whiteBalanceColorTempModeSet(int sourceType, int colortemp_mode, int is_save);
    int whiteBalanceColorTempModeGet(int sourceType );

    int whiteBalancePramSave(int sourceType, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset);
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


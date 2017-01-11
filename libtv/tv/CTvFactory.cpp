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
#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvFactory"

#include <CTvFactory.h>
#include <tvconfig.h>
#include <tvutils.h>
#include "../tvsetting/CTvSetting.h"

CTvFactory::CTvFactory()
    :mHdmiOutFbc(false),
    mFbcObj(NULL)
{
}

void CTvFactory::init()
{
    const char *value = config_get_str(CFG_SECTION_TV, CFG_FBC_USED, "true");
    if (strcmp(value, "true") == 0) {
        mHdmiOutFbc = true;
        mFbcObj = GetSingletonFBC();
    }
}

int CTvFactory::setPQModeBrightness (int sourceType __unused, int pqMode, int brightness)
{
    return CVpp::getInstance()->FactorySetPQMode_Brightness(pqMode, pqMode, brightness);
}

int CTvFactory::getPQModeBrightness (int tv_source_input, int pqMode)
{
    return CVpp::getInstance()->FactoryGetPQMode_Brightness(tv_source_input, pqMode);
}

int CTvFactory::setPQModeContrast (int tv_source_input, int pqMode, int contrast)
{
    return CVpp::getInstance()->FactorySetPQMode_Contrast(tv_source_input, pqMode, contrast);
}

int CTvFactory::getPQModeContrast (int tv_source_input, int pqMode)
{
    return CVpp::getInstance()->FactoryGetPQMode_Contrast(tv_source_input, pqMode);
}

int CTvFactory::setPQModeSaturation ( int tv_source_input, int pqMode, int saturation )
{
    return CVpp::getInstance()->FactorySetPQMode_Saturation(tv_source_input, pqMode, saturation);
}

int CTvFactory::getPQModeSaturation ( int tv_source_input, int pqMode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Saturation(tv_source_input, pqMode);
}

int CTvFactory::setPQModeHue ( int tv_source_input, int pqMode, int hue )
{
    return CVpp::getInstance()->FactorySetPQMode_Hue(tv_source_input, pqMode, hue);
}

int CTvFactory::getPQModeHue ( int tv_source_input, int pqMode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Hue(tv_source_input, pqMode);
}

int CTvFactory::setPQModeSharpness ( int tv_source_input, int pqMode, int sharpness )
{
    return CVpp::getInstance()->FactorySetPQMode_Sharpness(tv_source_input, pqMode, sharpness);
}

int CTvFactory::getPQModeSharpness ( int tv_source_input, int pqMode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Sharpness(tv_source_input, pqMode);
}

int CTvFactory::resetPQMode ()
{
    return CVpp::getInstance()->FactoryResetPQMode();
}

int CTvFactory::replacePQDb(const char *newFilePath)
{
    return CVpp::getInstance()->getPqData()->replacePQDb(newFilePath);
}

int CTvFactory::setGamma(tcon_gamma_table_t gamma_r, tcon_gamma_table_t gamma_g, tcon_gamma_table_t gamma_b)
{
    if (mHdmiOutFbc) {
        return mFbcObj->fbcSetGammaPattern(COMM_DEV_SERIAL, gamma_r.data[0], gamma_g.data[0], gamma_b.data[0]);
    } else {
        return CVpp::getInstance()->FactorySetGamma(gamma_r, gamma_g, gamma_b);
    }
}

int CTvFactory::getColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
    if (mHdmiOutFbc) {
        int ret = colorTempBatchGet((vpp_color_temperature_mode_t)Tempmode, params);
        params->r_gain = formatOutputGainParams(params->r_gain);
        params->g_gain = formatOutputGainParams(params->g_gain);
        params->b_gain = formatOutputGainParams(params->b_gain);
        params->r_post_offset = formatOutputOffsetParams(params->r_post_offset);
        params->g_post_offset = formatOutputOffsetParams(params->g_post_offset);
        params->b_post_offset = formatOutputOffsetParams(params->b_post_offset);
        return ret;
    } else {
        return CVpp::getInstance()->GetColorTemperatureParams(Tempmode, params);
    }
}

int CTvFactory::setTestPattern ( int pattern )
{
    switch ( pattern ) {
    case VPP_TEST_PATTERN_NONE:
        mAv.SetVideoScreenColor ( 3, 16, 128, 128 );
        break;

    case VPP_TEST_PATTERN_RED:
        mAv.SetVideoScreenColor ( 0, 81, 90, 240 );
        break;

    case VPP_TEST_PATTERN_GREEN:
        mAv.SetVideoScreenColor ( 0, 145, 54, 34 );
        break;

    case VPP_TEST_PATTERN_BLUE:
        mAv.SetVideoScreenColor ( 0, 41, 240, 110 );
        break;

    case VPP_TEST_PATTERN_WHITE:
        mAv.SetVideoScreenColor ( 0, 235, 128, 128 );
        break;

    case VPP_TEST_PATTERN_BLACK:
        mAv.SetVideoScreenColor ( 0, 16, 128, 128 );
        break;

    default:
        return -1;
    }
    return SSMSaveTestPattern ( pattern );
}

int CTvFactory::getTestPattern ()
{
    return CVpp::getInstance()->FactoryGetTestPattern();
}

int CTvFactory::setRGBPattern (int r, int g, int b)
{
    return mAv.setRGBScreen(r, g, b);
}

int CTvFactory::getRGBPattern()
{
    return mAv.getRGBScreen();
}

int CTvFactory::setScreenColor ( int vdinBlendingMask, int y, int u, int v )
{
    return mAv.SetVideoScreenColor ( vdinBlendingMask, y, u, v );
}

int CTvFactory::resetColorTemp ()
{
    return CVpp::getInstance()->FactoryResetColorTemp();
}

int CTvFactory::setParamsDefault ()
{
    return CVpp::getInstance()->FactorySetParamsDefault();
}

int CTvFactory::setDDRSSC ( int step )
{
    return CVpp::getInstance()->FactorySetDDRSSC(step);
}

int CTvFactory::getDDRSSC ()
{
    return CVpp::getInstance()->FactoryGetDDRSSC();
}

int CTvFactory::setLVDSSSC ( int step )
{
    return CVpp::getInstance()->FactorySetLVDSSSC(step);
}

int CTvFactory::getLVDSSSC ()
{
    return CVpp::getInstance()->FactoryGetLVDSSSC();
}

int CTvFactory::setNolineParams ( int nolineParamstype, int tv_source_input, noline_params_t nolineParams )
{
    return CVpp::getInstance()->FactorySetNolineParams(nolineParamstype, tv_source_input, nolineParams);
}

noline_params_t CTvFactory::getNolineParams ( int nolineParamstype, int tv_source_input )
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    return CVpp::getInstance()->FactoryGetNolineParams(nolineParamstype, sourceType);
}

int CTvFactory::setOverscan ( int tv_source_input, int fmt, int transFmt, tvin_cutwin_t cutwin )
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    CVpp::getInstance()->FactorySetOverscan(sourceType, fmt, transFmt, cutwin);
#if 0
    char val_buf[256];
    memset(val_buf, '\0', 256);
    LOGD("%s,%d: %d,%d,%d,%d", __FUNCTION__, __LINE__, ( int ) cutwin.vs, ( int ) cutwin.hs, ( int ) cutwin.ve, ( int ) cutwin.he);
    sprintf(val_buf, "%d,%d,%d,%d", ( int ) cutwin.vs, ( int ) cutwin.hs, ( int ) cutwin.ve, ( int ) cutwin.he );
    config_set_str(CFG_SECTION_TV, "vpp.overscan.dtv", val_buf);
    //}
#endif
    return CVpp::getInstance()->VPP_SetVideoCrop ( ( int ) cutwin.vs, ( int ) cutwin.hs, ( int ) cutwin.ve, ( int ) cutwin.he );
}

tvin_cutwin_t CTvFactory::getOverscan ( int tv_source_input, int fmt, int transFmt )
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    return CVpp::getInstance()->FactoryGetOverscan(sourceType, fmt, INDEX_2D, transFmt);
}

int CTvFactory::fbcSetBrightness ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Brightness(COMM_DEV_SERIAL, value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetBrightness ()
{
    int value = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Brightness(COMM_DEV_SERIAL, &value);
        return value;
    }
    return 0;
}

int CTvFactory::fbcSetContrast ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Contrast(COMM_DEV_SERIAL, value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetContrast ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Contrast(COMM_DEV_SERIAL, &data);
        return data;
    }
    return 0;
}

int CTvFactory::fbcSetSaturation ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Saturation(COMM_DEV_SERIAL, value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetSaturation ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Saturation(COMM_DEV_SERIAL, &data);
        return data;
    }
    return 0;
}

int CTvFactory::fbcSetHueColorTint ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_HueColorTint(COMM_DEV_SERIAL, value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetHueColorTint ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_HueColorTint(COMM_DEV_SERIAL, &data);
        return data;
    }
    return 0;
}

int CTvFactory::fbcSetBacklight ( int value )
{
    int temp_value = value;
    if (mFbcObj != NULL) {
        temp_value = temp_value * 255 / 100;
        mFbcObj->cfbc_Set_Backlight(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetBacklight ()
{
    int temp_value = 0;
    int data = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Backlight(COMM_DEV_SERIAL, &temp_value);
        if (temp_value * 100 % 255 == 0)
            temp_value = temp_value * 100 / 255;
        else
            temp_value = temp_value * 100 / 255 + 1;
        data = temp_value;
        return data;
    }

    return 0;
}

int CTvFactory::fbcSetAutoBacklightOnOff( unsigned char status)
{
    if (mFbcObj != NULL) {
        return mFbcObj->cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_SERIAL, status);
    }

    return -1;
}

int CTvFactory::fbcGetAutoBacklightOnOff( void )
{
    int temp_status = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }
    return 0;
}

int CTvFactory::fbcSetElecMode( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_SERIAL, value);
        SSMSaveFBCELECmodeVal(value);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcGetElecMode( void )
{
    int val = 0;
    SSMReadFBCELECmodeVal(&val);
    return val;
}

int CTvFactory::fbcSetBacklightN360( int value )
{
    SSMSaveFBCELECmodeVal(value);
    return -1;
}

int CTvFactory::fbcGetBacklightN360( void )
{
    int val = 0;
    SSMReadFBCELECmodeVal(&val);
    return val;
}

int CTvFactory::fbcSetThermalState( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Thermal_state(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcSetPictureMode ( int mode )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Picture_Mode(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetPictureMode ()
{
    int mode = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Picture_Mode(COMM_DEV_SERIAL, &mode);
        return mode;
    }
    return 0;
}

int CTvFactory::fbcSetTestPattern ( int mode )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Test_Pattern(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetTestPattern ()
{
    int mode = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Test_Pattern(COMM_DEV_SERIAL, &mode);
        return mode;
    }

    return 0;
}

int CTvFactory::fbcSelectTestPattern(int value)
{
    int ret = -1;
    if (mFbcObj != NULL) {
        LOGD("%s, value is %d\n", __FUNCTION__, value);
        ret = mFbcObj->cfbc_TestPattern_Select(COMM_DEV_SERIAL, value);
    }

    return ret;
}

int CTvFactory::fbcSetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save)
{
    if (mFbcObj != NULL) {
        mFbcObj->fbcSetGammaValue(COMM_DEV_SERIAL, (int)gamma_curve, is_save);
        return 0;
    }
    return -1;
}

int CTvFactory::fbcSetGainRed( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Gain_Red(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetGainRed ()
{
    int value = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Gain_Red(COMM_DEV_SERIAL, &value);
        return value;
    }

    return 0;
}

int CTvFactory::fbcSetGainGreen( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Gain_Green(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetGainGreen ()
{
    int value = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Gain_Green(COMM_DEV_SERIAL, &value);
        //value 0 ~ 2047
        return value;
    }

    return 0;
}

int CTvFactory::fbcGetVideoMute ()
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_VMute(COMM_DEV_SERIAL, 1);
    }

    return 0;
}

int CTvFactory::fbcSetGainBlue( int value )
{
    //value 0 ~ 2047
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Gain_Blue(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetGainBlue ()
{
    int value = 0;
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Gain_Blue(COMM_DEV_SERIAL, &value);
        //value 0 ~ 2047
        return value;
    }

    return 0;
}

int CTvFactory::fbcSetOffsetRed( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Offset_Red(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetOffsetRed ()
{
    int temp_value = 0, value = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Offset_Red(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTvFactory::fbcSetOffsetGreen( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Offset_Green(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetOffsetGreen ()
{
    int temp_value = 0, value = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Offset_Green(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTvFactory::fbcSetOffsetBlue( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Offset_Blue(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcGetOffsetBlue ()
{
    int temp_value = 0, value = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_Offset_Blue(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTvFactory::whiteBalanceGainRedGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        LOGD("--------- call none fbc method ---------");
        ret = CVpp::getInstance()->FactoryGetColorTemp_Rgain(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        LOGD("--------- call fbc method ---------");
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 0);
    }
    return ret;
}

int CTvFactory::whiteBalanceGainRedSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Rgain(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the red gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Rgain(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputGainParams(value);
        ret = fbcSetGainRed(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceGainGreenGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Ggain(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 1);
    }
    return ret;
}

int CTvFactory::whiteBalanceGainGreenSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Ggain(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the green gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Ggain(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputGainParams(value);
        ret = fbcSetGainGreen(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceGainBlueGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Bgain(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 2);
    }
    return ret;
}

int CTvFactory::whiteBalanceGainBlueSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Bgain(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the blue gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Bgain(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputGainParams(value);
        ret = fbcSetGainBlue(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetRedGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Roffset(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 3);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetRedSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Roffset(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the red offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Roffset(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputOffsetParams(value);
        ret = fbcSetOffsetRed(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetGreenGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Goffset(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 4);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetGreenSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Goffset(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the green offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Goffset(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputOffsetParams(value);
        ret = fbcSetOffsetGreen(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetBlueGet(int tv_source_input, int colortemp_mode)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Boffset(sourceType, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = getItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 5);
    }
    return ret;
}

int CTvFactory::whiteBalanceOffsetBlueSet(int tv_source_input, int colortemp_mode, int value)
{
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Boffset(sourceType, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the blue offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Boffset(sourceType, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = formatInputOffsetParams(value);
        ret = fbcSetOffsetBlue(value);
    }
    return ret;
}

int CTvFactory::whiteBalanceColorTempModeSet(int tv_source_input, int colortemp_mode, int is_save)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->SetColorTemperature((vpp_color_temperature_mode_t)colortemp_mode, (tv_source_input_t)tv_source_input, is_save);
    } else { //use fbc store the white balance params
        ret = fbcColorTempModeSet(colortemp_mode);
    }
    return ret;
}

int CTvFactory::whiteBalanceColorTempModeGet(int tv_source_input )
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->GetColorTemperature((tv_source_input_t)tv_source_input);
    } else { //use fbc store the white balance params
        ret = fbcColorTempModeGet();
    }
    return ret;
}

int CTvFactory::whiteBalancePramSave(int tv_source_input, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset)
{
    int ret = 0;
    tv_source_input_type_t sourceType;
    sourceType = CTvin::Tvin_SourceInputToSourceInputType((tv_source_input_t)tv_source_input);

    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        CVpp::getInstance()->SaveColorTemp((vpp_color_temperature_mode_t) tempmode, (tv_source_input_t) tv_source_input);
        CVpp::getInstance()->FactorySaveColorTemp_Rgain(sourceType, tempmode, r_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Ggain(sourceType, tempmode, g_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Bgain(sourceType, tempmode, b_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Roffset(sourceType, tempmode, r_offset);
        CVpp::getInstance()->FactorySaveColorTemp_Goffset(sourceType, tempmode, g_offset);
        CVpp::getInstance()->FactorySaveColorTemp_Boffset(sourceType, tempmode, b_offset);
    } else { //use fbc store the white balance params
        tcon_rgb_ogo_t params;

        params.r_gain = formatInputGainParams(r_gain);
        params.g_gain = formatInputGainParams(g_gain);
        params.b_gain = formatInputGainParams(b_gain);
        params.r_post_offset = formatInputOffsetParams(r_offset);
        params.g_post_offset = formatInputOffsetParams(g_offset);
        params.b_post_offset = formatInputOffsetParams(b_offset);
        ret = colorTempBatchSet((vpp_color_temperature_mode_t)tempmode, params);
    }
    return ret;
}

int CTvFactory::whiteBalanceGrayPatternClose()
{
    int useFbc = 0;
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->VPP_SetGrayPattern(0);
    } else { //use fbc store the white balance params
        ret = fbcGrayPatternClose();
    }
    return ret;
}

int CTvFactory::whiteBalanceGrayPatternOpen()
{
    int ret = 0;
    if (mHdmiOutFbc) { //use fbc store the white balance params
        ret = fbcGrayPatternOpen();
    }
    return ret;
}

int CTvFactory::whiteBalanceGrayPatternSet(int value)
{
    int ret = -1;
    if (!mHdmiOutFbc) {
        ret = CVpp::getInstance()->VPP_SetGrayPattern(value);
    } else {
        ret = fbcGrayPatternSet(value);
    }
    return ret;
}

int CTvFactory:: whiteBalanceGrayPatternGet()
{
    int ret = -1;
    if (!mHdmiOutFbc) {
        ret = CVpp::getInstance()->VPP_GetGrayPattern();
    }
    return ret;
}

int CTvFactory::fbcGrayPatternSet(int value)
{
    int ret = -1;
    unsigned char grayValue = 0;
    if (value > 255) {
        grayValue = 255;
    } else if (value < 0) {
        grayValue = 0;
    } else {
        grayValue = (unsigned char)(0xFF & value);
    }
    if (mFbcObj != NULL) {
        ret = mFbcObj->cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_SERIAL, grayValue);
    }
    return ret;
}

int CTvFactory::fbcGrayPatternOpen()
{
    int ret = -1;
    if (mFbcObj != NULL) {
        ret = mFbcObj->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 0);
    }
    return ret;
}

int CTvFactory::fbcGrayPatternClose()
{
    int ret = -1;
    if (mFbcObj != NULL) {
        ret = mFbcObj->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 1);
    }
    return ret;
}

int CTvFactory::fbcColorTempModeSet( int mode )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcColorTempModeGet ()
{
    int temp_mode = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_ColorTemp_Mode(COMM_DEV_SERIAL, &temp_mode);
        return temp_mode;
    }

    return -1;
}

int CTvFactory::fbcColorTempModeN360Set( int mode )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
        SSMSaveFBCN360ColorTempVal(mode);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcColorTempModeN360Get ()
{
    int temp_mode = 0;
    SSMReadFBCN360ColorTempVal(&temp_mode);
    return temp_mode;
}

int CTvFactory::fbcWBInitialSet( int status )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_WB_Initial(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcWBInitialGet ()
{
    int temp_status = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_WB_Initial(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTvFactory::fbcBacklightOnOffSet(int value)
{
    if (mFbcObj != NULL) {
        value = value ? 0 : 1;
        mFbcObj->cfbc_Set_backlight_onoff(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcBacklightOnOffGet()
{
    int temp_value = 0;

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_backlight_onoff(COMM_DEV_SERIAL, &temp_value);
        temp_value = temp_value ? 0 : 1;
        return temp_value;
    }

    return 0;
}

int CTvFactory::fbcLvdsSsgSet( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_LVDS_SSG_Set(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcLightSensorStatusN310Set ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_LightSensor_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310LightsensorVal(value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcLightSensorStatusN310Get ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        SSMReadFBCN310LightsensorVal(&data);
        return data;
    }

    return 0;
}

int CTvFactory::fbcDreamPanelStatusN310Set ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_Dream_Panel_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310Dream_PanelVal(value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcDreamPanelStatusN310Get ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        SSMReadFBCN310Dream_PanelVal(&data);
        return data;
    }

    return 0;
}

int CTvFactory::fbcMultPQStatusN310Set ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_MULT_PQ_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310MULT_PQVal(value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcMultPQStatusN310Get ()
{
    int data = 0;

    if (mFbcObj != NULL) {
        SSMReadFBCN310MULT_PQVal(&data);
        return data;
    }

    return 0;
}

int CTvFactory::fbcMemcStatusN310Set ( int value )
{
    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_MEMC_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310MEMCVal(value);
        return 0;
    }

    return -1;
}

int CTvFactory::fbcMemcStatusN310Get ()
{
    int data = 0;
    if (mFbcObj != NULL) {
        SSMReadFBCN310MEMCVal(&data);
        return data;
    }

    return -1;
}

int CTvFactory::getItemFromBatch(vpp_color_temperature_mode_t colortemp_mode, int item)
{
    tcon_rgb_ogo_t params;
    int ret = 0;

    colorTempBatchGet((vpp_color_temperature_mode_t)colortemp_mode, &params);
    switch (item) {
    case 0:
        ret = formatOutputGainParams(params.r_gain);
        break;
    case 1:
        ret = formatOutputGainParams(params.g_gain);
        break;
    case 2:
        ret = formatOutputGainParams(params.b_gain);
        break;
    case 3:
        ret = formatOutputOffsetParams(params.r_post_offset);
        break;
    case 4:
        ret = formatOutputOffsetParams(params.g_post_offset);
        break;
    case 5:
        ret = formatOutputOffsetParams(params.b_post_offset);
        break;
    default:
        ret = 0;
    }
    return ret;
}

int CTvFactory::formatInputGainParams(int value)
{
    int ret = 1024;
    if (value < 0) {
        ret = 0;
    } else if (value > 2047) {
        ret = 2047;
    } else {
        ret = value;
    }
    ret = ret >> 3;
    return ret;
}

int CTvFactory::formatInputOffsetParams(int value)
{
    int ret = 0;
    if (value < -1024) {
        ret = -1024;
    } else if (value > 1023) {
        ret = 1023;
    } else {
        ret = value;
    }
    ret += 1024;
    ret = ret >> 3;
    return ret;
}

int CTvFactory::formatOutputOffsetParams(int value)
{
    if (value == 255) {
        value = 1023;
    } else {
        value = value << 3;
        value -= 1024;
    }
    return value;
}

int CTvFactory::formatOutputGainParams(int value)
{
    value = value << 3;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    return value;
}

int CTvFactory::fbcSetColorTemperature(vpp_color_temperature_mode_t temp_mode)
{
    int ret = -1;
    if (CVpp::getInstance()->GetEyeProtectionMode()) {
        tcon_rgb_ogo_t rgb_ogo;
        colorTempBatchGet(temp_mode, &rgb_ogo);
        rgb_ogo.b_gain /= 2;
        ret = colorTempBatchSet(temp_mode, rgb_ogo);
    } else {
        ret = fbcColorTempModeSet(temp_mode);
    }
    return ret;
}

int CTvFactory::colorTempBatchSet(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
    switch (Tempmode) {
    case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
        mode = 1;   //COLOR_TEMP_STD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_WARM:
        mode = 2;   //COLOR_TEMP_WARM
        break;
    case VPP_COLOR_TEMPERATURE_MODE_COLD:
        mode = 0;  //COLOR_TEMP_COLD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_USER:
        mode = 3;   //COLOR_TEMP_USER
        break;
    default:
        break;
    }
    r_gain = (params.r_gain * 255) / 2047; // u1.10, range 0~2047, default is 1024 (1.0x)
    g_gain = (params.g_gain * 255) / 2047;
    b_gain = (params.b_gain * 255) / 2047;
    r_offset = (params.r_post_offset + 1024) * 255 / 2047; // s11.0, range -1024~+1023, default is 0
    g_offset = (params.g_post_offset + 1024) * 255 / 2047;
    b_offset = (params.b_post_offset + 1024) * 255 / 2047;
    LOGD ( "~colorTempBatchSet##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Set_WB_Batch(COMM_DEV_SERIAL, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        return 0;
    }

    return -1;
}

int CTvFactory::colorTempBatchGet ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
    unsigned char mode = 0, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset;
    switch (Tempmode) {
    case VPP_COLOR_TEMPERATURE_MODE_STANDARD:
        mode = 1;   //COLOR_TEMP_STD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_WARM:
        mode = 2;   //COLOR_TEMP_WARM
        break;
    case VPP_COLOR_TEMPERATURE_MODE_COLD:
        mode = 0;  //COLOR_TEMP_COLD
        break;
    case VPP_COLOR_TEMPERATURE_MODE_USER:
        mode = 3;   //COLOR_TEMP_USER
        break;
    default:
        break;
    }

    if (mFbcObj != NULL) {
        mFbcObj->cfbc_Get_WB_Batch(COMM_DEV_SERIAL, mode, &r_gain, &g_gain, &b_gain, &r_offset, &g_offset, &b_offset);
        LOGD ( "~colorTempBatchGet##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

        params->r_gain = (r_gain * 2047) / 255;
        params->g_gain = (g_gain * 2047) / 255;
        params->b_gain = (b_gain * 2047) / 255;
        params->r_post_offset = (r_offset * 2047) / 255 - 1024;
        params->g_post_offset = (g_offset * 2047) / 255 - 1024;
        params->b_post_offset = (b_offset * 2047) / 255 - 1024;
        return 0;
    }

    return -1;
}

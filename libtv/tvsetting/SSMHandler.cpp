
#define LOG_TAG "tvserver"
#define LOG_TV_TAG "SSMHandler"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <tvconfig.h>
#include <vector>
#include "CBlobDeviceFile.h"
#include "CTvLog.h"

#include "SSMHeader.h"
#include "SSMHandler.h"
#include "CTvSettingCfg.h"
#include "CTvSetting.h"
#include "../tvin/CTvin.h"
#include "../vpp/CVpp.h"
#include "../audio/CTvAudio.h"

android::Mutex SSMHandler::sLock;
SSMHandler* SSMHandler::mSSMHandler = NULL;
CBlobDevice* SSMHandler::mSSMHeaderFile = NULL;

SSMHandler* SSMHandler::GetSingletonInstance()
{

    Mutex::Autolock _l(sLock);

    if (!mSSMHandler) {
        if (mSSMHeaderFile)
            mSSMHandler = new SSMHandler();

        if (mSSMHandler && !mSSMHandler->Construct()) {
            delete mSSMHandler;
            mSSMHandler = NULL;
        }
    }

    return mSSMHandler;
}

SSMHandler::~SSMHandler()
{
    if (mFd > 0)
        close(mFd);

    mSSMHandler = NULL;
}

bool SSMHandler::Construct()
{
    bool ret = true;

    const char *device_path = config_get_str(CFG_SECTION_SETTING, mCfgPath.c_str(),
                                             mDefCfgPath.c_str());

    mFd = open(device_path, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (-1 == mFd) {
        ret = false;
        LOGD ("%s, Open %s failure\n", __func__, device_path);
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection1Verify()
{
    SSM_status_t ret = SSM_HEADER_VALID;

    lseek (mFd, 0, SEEK_SET);
    ssize_t ssize = read(mFd, &mSSMHeader_section1, sizeof (SSMHeader_section1_t));

    if (ssize != sizeof (SSMHeader_section1_t) ||
        mSSMHeader_section1.magic != gSSMHeader_section1.magic ||
        mSSMHeader_section1.count != gSSMHeader_section1.count) {
        ret = SSM_HEADER_INVALID;
    }

    if (ret != SSM_HEADER_INVALID &&
        mSSMHeader_section1.version != gSSMHeader_section1.version) {
        ret = SSM_HEADER_STRUCT_CHANGE;
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection2Verify(SSM_status_t SSM_status)
{
    return SSM_status;
}

bool SSMHandler::SSMRecreateHeader()
{
    bool ret = true;

    LOGD ("%s ---   ", __func__);

    ftruncate(mFd, 0);
    lseek (mFd, 0, SEEK_SET);

    //cal Addr and write
    write (mFd, &gSSMHeader_section1, sizeof (SSMHeader_section1_t));
    write (mFd, gSSMHeader_section2, gSSMHeader_section1.count * sizeof (SSMHeader_section2_t));

    return ret;
}

bool SSMHandler::SSMResetX(unsigned int id)
{
    bool ret = true;

    ret = id > 0 && id < gSSMHeader_section1.count;

    VPPSSMRestoreDefault(id, false);

    return ret;
}

#include <memory>
bool SSMHandler::SSMRecovery()
{
    bool ret = true;
    std::vector<SSMHeader_section2_t> vsection2;
    unsigned int size = 0;

    LOGD ("%s --- line:%d\n", __func__, __LINE__);

    lseek (mFd, sizeof (SSMHeader_section1_t), SEEK_SET);

    for (unsigned int i = 0; i < gSSMHeader_section1.count; i++) {
        SSMHeader_section2_t temp;

        read (mFd, &temp, sizeof (SSMHeader_section2_t));
        vsection2.push_back(temp);
        size += temp.size;
    }

    //std::unique_ptr<unsigned char[]> SSMBuff(new unsigned char(size));
    unsigned char* SSMBuff = (unsigned char*) malloc(size);

    if (!SSMBuff)
        ret = false;

    if (ret) {
        mSSMHeaderFile->ReadBytes(0, size, SSMBuff);
        mSSMHeaderFile->EraseAllData();

        for (unsigned int i = 0; i < gSSMHeader_section1.count; i++) {
            if (vsection2[i].size == gSSMHeader_section2[i].size) {
                mSSMHeaderFile->WriteBytes(gSSMHeader_section2[i].addr,
                                           gSSMHeader_section2[i].size, SSMBuff + vsection2[i].addr);
            }
            else {
                SSMResetX(i);
                LOGD ("id : %d size from %d -> %d", i, vsection2[i].size, gSSMHeader_section2[i].size);
            }
        }

        if (ret)
            free (SSMBuff);

        ret = SSMRecreateHeader();
    }

    return ret;
}

unsigned int SSMHandler::SSMGetActualAddr(int id)
{
    return gSSMHeader_section2[id].addr;
}

unsigned int SSMHandler::SSMGetActualSize(int id)
{
    return gSSMHeader_section2[id].size;
}

const std::string& SSMHandler::getCfgPath(bool isDefault) const
{
    return isDefault ? mDefCfgPath : mCfgPath;
}

SSM_status_t SSMHandler::SSMVerify()
{
    return  SSMSection2Verify(SSMSection1Verify());
}

SSMHandler::SSMHandler()
{
    unsigned int sum = 0;

    memset (&mSSMHeader_section1, 0, sizeof (SSMHeader_section1_t));

    for (unsigned int i = 1; i < gSSMHeader_section1.count; i++) {
        sum += gSSMHeader_section2[i-1].size;

        gSSMHeader_section2[i].addr = sum;
    }
    /*
        for (unsigned int i = 0; i < gSSMHeader_section1.count; i++) {
                LOGD ("%d:%d\n", i, gSSMHeader_section2[i].addr);
        }
*/
}

SSMHandler::SSMHandler(const SSMHandler&)
{

}

SSMHandler::SSMHandler(SSMHandler&)
{

}

SSMHandler& SSMHandler::operator = (const SSMHandler& obj)
{
    return const_cast<SSMHandler&>(obj);
}

int SSMHandler::VPPSSMRestoreDefault(int id, bool resetAll)
{
    int i = 0, tmp_val = 0;
    int tmp_panorama_nor = 0, tmp_panorama_full = 0;
    int offset_r = 0, offset_g = 0, offset_b = 0, gain_r = 1024, gain_g = 1024, gain_b = 1024;
    int8_t std_buf[6] = { 0, 0, 0, 0, 0, 0 };
    int8_t warm_buf[6] = { 0, 0, -8, 0, 0, 0 };
    int8_t cold_buf[6] = { -8, 0, 0, 0, 0, 0 };
    unsigned char tmp[2] = {0, 0};

    if (resetAll || VPP_DATA_POS_COLOR_DEMO_MODE_START == id)
        SSMSaveColorDemoMode ( VPP_COLOR_DEMO_MODE_ALLON);

    if (resetAll || VPP_DATA_POS_COLOR_BASE_MODE_START == id)
        SSMSaveColorBaseMode ( VPP_COLOR_BASE_MODE_OPTIMIZE);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_R_START == id)
        SSMSaveRGBGainRStart(0, gain_r);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_G_START == id)
        SSMSaveRGBGainGStart(0, gain_g);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_B_START == id)
        SSMSaveRGBGainBStart(0, gain_b);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_R_START == id)
        SSMSaveRGBPostOffsetRStart(0, offset_r);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_G_START== id)
        SSMSaveRGBPostOffsetGStart(0, offset_g);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_B_START == id)
        SSMSaveRGBPostOffsetBStart(0, offset_b);

    if (resetAll || VPP_DATA_USER_NATURE_SWITCH_START == id)
        SSMSaveUserNatureLightSwitch(1);

    if (resetAll || VPP_DATA_GAMMA_VALUE_START == id)
        SSMSaveGammaValue(0);

    if (resetAll || SSM_RW_UI_GRHPHY_BACKLIGHT_START == id)
        SSMSaveGraphyBacklight(100);

    if (resetAll || VPP_DATA_POS_DBC_START == id)
        SSMSaveDBCStart(0);

    if (resetAll || VPP_DATA_POS_DNLP_START == id)
        SSMSaveDnlpStart(0); //0: ON,1: OFF,default is on

    if (resetAll || VPP_DATA_APL_START == id)
        SSMSaveAPL(30);

    if (resetAll || VPP_DATA_APL2_START == id)
        SSMSaveAPL2(30);

    if (resetAll || VPP_DATA_BD_START == id)
        SSMSaveBD(30);

    if (resetAll || VPP_DATA_BP_START == id)
        SSMSaveBP(30);

    if (resetAll || VPP_DATA_POS_FBC_ELECMODE_START == id)
        SSMSaveFBCELECmodeVal(11);

    if (resetAll || VPP_DATA_POS_FBC_BACKLIGHT_START == id)
        SSMSaveFBCN360BackLightVal(10);

    if (resetAll || VPP_DATA_POS_FBC_COLORTEMP_START == id)
        SSMSaveFBCN360ColorTempVal(1); // standard colortemp

    if (resetAll || VPP_DATA_POS_FBC_N310_COLORTEMP_START == id)
        SSMSaveFBCN310ColorTempVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_LIGHTSENSOR_START == id)
        SSMSaveFBCN310LightsensorVal(0);

    if (resetAll || VPP_DATA_POS_FBC_N310_DREAMPANEL_START == id)
        SSMSaveFBCN310Dream_PanelVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MULTI_PQ_START == id)
        SSMSaveFBCN310MULT_PQVal(1);

    if (resetAll || VPP_DATA_POS_FBC_N310_MEMC_START == id)
        SSMSaveFBCN310MEMCVal(2);

    if (resetAll || VPP_DATA_POS_FBC_N310_BACKLIGHT_START == id)
        SSMSaveFBCN310BackLightVal(254);

    if (resetAll || VPP_DATA_RGB_START == id) {
        for (i = 0; i < 6; i++) {
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_STANDARD * 6, std_buf[i]); //0~5
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_WARM * 6, warm_buf[i]); //6~11
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_COLD * 6, cold_buf[i]); //12~17
        }
    }

    for (i = 0; i < SOURCE_TYPE_MAX; i++) {
        if (resetAll || VPP_DATA_COLOR_SPACE_START == id) {
            if (i == SOURCE_TYPE_HDMI) {
                SSMSaveColorSpaceStart ( VPP_COLOR_SPACE_AUTO);
            }
        }

        tmp_val = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
        tmp_panorama_nor = VPP_PANORAMA_MODE_NORMAL;
        tmp_panorama_full = VPP_PANORAMA_MODE_FULL;

        if (resetAll || VPP_DATA_POS_PANORAMA_START == id) {
            if (i == SOURCE_TYPE_HDMI) {
                SSMSavePanoramaStart(i, tmp_panorama_full);
            } else {
                SSMSavePanoramaStart(i, tmp_panorama_nor);
            }
        }

        if (resetAll || VPP_DATA_POS_COLOR_TEMP_START == id)
            SSMSaveColorTemperature(i, tmp_val);

        tmp_val = 50;
        if (resetAll || VPP_DATA_POS_BRIGHTNESS_START == id)
            SSMSaveBrightness(i, tmp_val);

        if (resetAll || VPP_DATA_POS_CONTRAST_START == id)
            SSMSaveContrast(i, tmp_val);

        if (resetAll || VPP_DATA_POS_SATURATION_START == id)
            SSMSaveSaturation(i, tmp_val);

        if (resetAll || VPP_DATA_POS_HUE_START == id)
            SSMSaveHue(i, tmp_val);

        if (resetAll || VPP_DATA_POS_SHARPNESS_START == id)
            SSMSaveSharpness(i, tmp_val);

        tmp_val = VPP_PICTURE_MODE_STANDARD;
        if (resetAll || VPP_DATA_POS_PICTURE_MODE_START == id)
            SSMSavePictureMode(i, tmp_val);

        tmp_val = VPP_DISPLAY_MODE_169;
        if (resetAll || VPP_DATA_POS_DISPLAY_MODE_START == id)
            SSMSaveDisplayMode(i, tmp_val);

        tmp_val = VPP_NOISE_REDUCTION_MODE_AUTO;
        if (resetAll || VPP_DATA_POS_NOISE_REDUCTION_START == id)
            SSMSaveNoiseReduction(i, tmp_val);

        tmp_val = DEFAULT_BACKLIGHT_BRIGHTNESS;
        if (resetAll || VPP_DATA_POS_BACKLIGHT_START == id)
            SSMSaveBackLightVal(i, tmp_val);
    }

    if (resetAll || VPP_DATA_POS_DDR_SSC_START == id)
        SSMSaveDDRSSC(0);

    if (resetAll || VPP_DATA_POS_LVDS_SSC_START == id)
        SSMSaveLVDSSSC(tmp);

    return 0;
}



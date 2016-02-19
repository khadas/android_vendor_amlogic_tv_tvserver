#define LOG_TAG "CVpp"

#include "CVpp.h"
#include <CTvLog.h>
#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"
#include <cutils/properties.h>
#include "CPQdb.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>
#include "CTvLog.h"
#include "../tvconfig/tvconfig.h"
#include "CAv.h"

CVpp *CVpp::mInstance;
CVpp *CVpp::getInstance()
{
    if (NULL == mInstance) mInstance = new CVpp();
    return mInstance;
}

CVpp::CVpp()
{
    vpp_amvideo_fd = -1;
    vpp_amvideo_3d_fd = -1;
    mpPqData = new CPqData();
}

CVpp::~CVpp()
{
    if (mpPqData != NULL) {
        delete mpPqData;
        mpPqData = NULL;
    }
}

int CVpp::Vpp_Init(const char *pq_db_path)
{
    LOGD("Vpp_Init pq_db_path = %s", pq_db_path);
    if (mpPqData->openPqDB(pq_db_path)) {
        LOGW("%s, open pq failed!", __FUNCTION__);
    } else {
        LOGD("%s, open pq success!", __FUNCTION__);
    }

    int ret = -1;
    int backlight = 100;
    int offset_r = 0, offset_g = 0, offset_b = 0, gain_r = 1024, gain_g = 1024, gain_b = 1024;

    Vpp_GetVppConfig();

    ret = VPP_OpenModule();
    backlight = GetBacklight(SOURCE_TYPE_TV);

    if (mbVppCfg_backlight_init) {
        backlight = (backlight + 100) * 255 / 200;

        if (backlight < 127 || backlight > 255) {
            backlight = 255;
        }
    }

    SetBacklight(backlight, SOURCE_TYPE_TV, 0);

    if (SSMReadNonStandardValue() & 1) {
        Set_Fixed_NonStandard(0); //close
    } else {
        Set_Fixed_NonStandard(2); //open
    }

    LoadVppSettings(SOURCE_TYPE_MPEG, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D);

    return ret;
}

int CVpp::Vpp_Uninit(void)
{
    Vpp_ResetLastVppSettingsSourceType();
    VPP_CloseModule();
    mpPqData->closeDb();
    return 0;
}
CPqData *CVpp::getPqData()
{
    return mpPqData;
}

int CVpp::doSuspend()
{
    return mpPqData->closeDb();
}

int CVpp::doResume()
{
    return mpPqData->reopenDB();
}
int CVpp::VPP_OpenModule(void)
{

    if (vpp_amvideo_fd < 0) {
        vpp_amvideo_fd = open(VPP_DEV_PATH, O_RDWR);

        LOGD("~~~open~~~##VPP_OpenModule##VPP_DEV_PATH : %s##", VPP_DEV_PATH);

        if (vpp_amvideo_fd < 0) {
            LOGE("Open vpp module, error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    if (vpp_amvideo_3d_fd < 0) {
        vpp_amvideo_3d_fd = open(VPP_3D_DEV_PATH, O_RDWR);
        LOGD("~~~open~~~##VPP_OpenModule##VPP_3D_DEV_PATH : %s##", VPP_3D_DEV_PATH);

        if (vpp_amvideo_3d_fd < 0) {
            LOGE("Open vpp 3d module, error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    return vpp_amvideo_fd;
}

int CVpp::VPP_CloseModule(void)
{
    if (vpp_amvideo_fd >= 0) {
        close ( vpp_amvideo_fd);
        vpp_amvideo_fd = -1;
    }

    if (vpp_amvideo_3d_fd >= 0) {
        close ( vpp_amvideo_3d_fd);
        vpp_amvideo_3d_fd = -1;
    }

    return 0;
}

int CVpp::VPP_DeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    tmp_ret = ioctl(vpp_amvideo_fd, request, arg);
    return tmp_ret;
}

int CVpp::Vpp_LoadRegs(am_regs_t regs)
{
    LOGD("~~~VPP_DeviceIOCtl~~~##Vpp_LoadRegs##AMVECM_IOC_LOAD_REG##");

    int count_retry = 20;
    int rt = 0;
    while (count_retry) {
        rt = VPP_DeviceIOCtl(AMVECM_IOC_LOAD_REG, &regs);
        if (rt < 0) {
            LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
            break;
        }
        break;
    }

    return rt;
}

int CVpp::isPreviewWindow()
{
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    property_get("tv.is.preview.window", prop_value, "false");
    LOGD("%s, prop tv.is.preview.window is \"%s\".\n", __FUNCTION__, prop_value);
    return (strcmp(prop_value, "true") == 0) ? 1 : 0;
}

int CVpp::LoadVppSettings(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                          is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int val = 0, ret = -1;
    vpp_color_temperature_mode_t temp_mode = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    vpp_picture_mode_t pqmode = VPP_PICTURE_MODE_STANDARD;
    vpp_display_mode_t dispmode = VPP_DISPLAY_MODE_169;
    vpp_noise_reduction_mode_t nr_mode = VPP_NOISE_REDUCTION_MODE_MID;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if ((vpp_setting_last_source_type == source_type) && (vpp_setting_last_sig_fmt == sig_fmt)
            /*&& ( vpp_setting_last_3d_status == status showbo mark)*/
            && (vpp_setting_last_trans_fmt == trans_fmt)) {
        return -1;
    }

    nr_mode = GetNoiseReductionMode(source_type);
    ret |= Vpp_SetNoiseReductionMode(nr_mode, source_type, source_port, sig_fmt, is3d, trans_fmt);
    ret |= Vpp_SetXVYCCMode(VPP_XVYCC_MODE_STANDARD, source_type, source_port, sig_fmt, is3d,
                            trans_fmt);
    ret |= Vpp_SetMCDIMode(VPP_MCDI_MODE_STANDARD, source_type, source_port, sig_fmt, is3d,
                           trans_fmt);
    ret |= Vpp_SetDeblockMode(VPP_DEBLOCK_MODE_MIDDLE, source_port, sig_fmt, is3d, trans_fmt);

    Vpp_LoadDI(source_type, sig_fmt, is3d, trans_fmt);
    Vpp_LoadBasicRegs(source_type, sig_fmt, is3d, trans_fmt);
    Vpp_LoadGamma(source_type, sig_fmt);

    ret |= Vpp_SetBaseColorMode(GetBaseColorMode(), source_port, sig_fmt, is3d, trans_fmt);

    if (isPreviewWindow()) {
        temp_mode = GetColorTemperature(SOURCE_TYPE_MPEG);
        if (temp_mode == VPP_COLOR_TEMPERATURE_MODE_USER) {
            ret |= Vpp_SetColorTemperatureUser(temp_mode, SOURCE_TYPE_MPEG);
        } else {
            CheckColorTemperatureParamAlldata(TVIN_PORT_HDMI0, sig_fmt, trans_fmt); // check colortmp backup data
            ret |= Vpp_SetColorTemperature(temp_mode, SOURCE_TYPE_MPEG, source_port, sig_fmt, trans_fmt);
        }
    } else {
        temp_mode = GetColorTemperature(source_type);
        if (temp_mode == VPP_COLOR_TEMPERATURE_MODE_USER) {
            ret |= Vpp_SetColorTemperatureUser(temp_mode, source_type);
        } else {
            CheckColorTemperatureParamAlldata(TVIN_PORT_HDMI0, sig_fmt, trans_fmt); // check colortmp backup data
            ret |= Vpp_SetColorTemperature(temp_mode, source_type, source_port, sig_fmt, trans_fmt);
        }
    }

    pqmode = GetPQMode(source_type);
    ret |= Vpp_SetPQMode(pqmode, source_type, source_port, sig_fmt, is3d, trans_fmt);

    ret |= SetDNLP(source_type, source_port, sig_fmt, is3d, trans_fmt);

    vpp_setting_last_source_type = source_type;
    vpp_setting_last_sig_fmt = sig_fmt;
    //showbo mark vpp_setting_last_3d_status = status;
    vpp_setting_last_trans_fmt = trans_fmt;

    return 0;
}

int CVpp::Vpp_GetVppConfig(void)
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "vpp.pqmode.depend.bklight", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_pqmode_depend_bklight = true;
    } else {
        mbVppCfg_pqmode_depend_bklight = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.color.temp.bysource", "enable");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_colortemp_by_source = true;
    } else {
        mbVppCfg_colortemp_by_source = true;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.panoroma.switch", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_panorama_switch = true;
    } else {
        mbVppCfg_panorama_switch = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.backlight.reverse", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_backlight_reverse = true;
    } else {
        mbVppCfg_backlight_reverse = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.backlight.init", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_backlight_init = true;
    } else {
        mbVppCfg_backlight_init = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.pqwithout.hue", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_pqmode_without_hue = true;
    } else {
        mbVppCfg_pqmode_without_hue = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.hue.reverse", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_hue_reverse = true;
    } else {
        mbVppCfg_hue_reverse = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.gamma.onoff", "null");
    if (strcmp(config_value, "disable") == 0) {
        mbVppCfg_gamma_onoff = true;
    } else {
        mbVppCfg_gamma_onoff = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.whitebalance.same_param", "null");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_whitebalance_sameparam = true;
    } else {
        mbVppCfg_whitebalance_sameparam = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.new.cm", "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_new_cm = true;
    } else {
        mbVppCfg_new_cm = false;
    }

    config_value = config_get_str(CFG_SECTION_TV, "vpp.new.nr", "disable");
    if (strcmp(config_value, "enable") == 0) {
        mbVppCfg_new_nr = true;
    } else {
        mbVppCfg_new_nr = false;
    }

    return 0;
}

int CVpp::Vpp_LoadDI(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                     is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    am_regs_t regs;
    int ret = -1;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);
    if (mpPqData->getRegValues("GeneralDITable", source_port, sig_fmt, is3d, trans_fmt, &regs) > 0) {
        if (Vpp_LoadRegs(regs) < 0) {
            LOGE("%s, Vpp_LoadRegs failed!\n", __FUNCTION__);
        } else {
            ret = 0;
        }
    } else {
        LOGE("%s getRegValues failed!\n", "Vpp_LoadDI");
    }
    return ret;
}

int CVpp::Vpp_LoadBasicRegs(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                            is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    am_regs_t regs;
    int ret = -1, enableFlag = -1;

    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (mpPqData->getRegValues("GeneralCommonTable", source_port, sig_fmt, is3d, trans_fmt, &regs) > 0) {
        if (Vpp_LoadRegs(regs) < 0) {
            LOGE("%s, Vpp_LoadRegs failed!\n", __FUNCTION__);
        } else {
            ret = 0;
        }
    } else {
        LOGE("%s getRegValues failed!\n", "Vpp_LoadBasicRegs");
    }

    if (mpPqData->LoadAllPQData(source_port, sig_fmt, is3d, trans_fmt, enableFlag) == 0) {
        ret = 0;
    } else {
        LOGE("%s, getPQData failed!\n", "Vpp_LoadBasicRegs");
        ret = -1;
    }

    return ret;
}

int CVpp::Vpp_ResetLastVppSettingsSourceType(void)
{
    vpp_setting_last_source_type = SOURCE_TYPE_MAX;
    vpp_setting_last_sig_fmt = TVIN_SIG_FMT_MAX;
    //showbo mark vpp_setting_last_3d_status = STATUS3D_MAX;
    vpp_setting_last_trans_fmt = TVIN_TFMT_3D_MAX;
    return 0;
}

int CVpp::VPP3D_DeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;

    if (vpp_amvideo_3d_fd >= 0) {
        va_start(ap, request);
        arg = va_arg ( ap, void * );
        va_end(ap);

        tmp_ret = ioctl(vpp_amvideo_3d_fd, request, arg);
        return tmp_ret;
    }

    return -1;
}

int CVpp::Vpp_GetPQModeValue(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode,
                             vpp_pq_para_t *pq_para)
{
    vpp_pq_para_t parms;
    vpp_picture_mode_t real_pq_mode;

    if (pq_para == NULL) {
        return -1;
    }

    if (pq_mode == VPP_PICTURE_MODE_MOVIE) {
        real_pq_mode = VPP_PICTURE_MODE_SOFT;
    } else if (pq_mode == VPP_PICTURE_MODE_COLORFUL) {
        real_pq_mode = VPP_PICTURE_MODE_BRIGHT;
    } else {
        real_pq_mode = pq_mode;
    }
    if (mpPqData->PQ_GetPQModeParams(source_type, real_pq_mode, pq_para) == 0) {
        if (mbVppCfg_pqmode_without_hue) {
            SSMReadHue(source_type, &(pq_para->hue));
        }
        if (pq_mode == VPP_PICTURE_MODE_MOVIE) {
            pq_para->brightness -= 10;
            pq_para->contrast -= 10;
            pq_para->saturation -= 10;
            pq_para->sharpness -= 10;
        } else if (pq_mode == VPP_PICTURE_MODE_COLORFUL) {
            pq_para->brightness += 10;
            pq_para->contrast += 10;
            pq_para->saturation += 10;
            pq_para->sharpness += 10;
        } else {
        }

    } else {
        LOGE("%s, PQ_GetPQModeParams failed!\n", "Vpp_GetPQModeValue");
        return -1;
    }

    return 0;
}

int CVpp::Vpp_SetPQParams(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode,
                          vpp_pq_para_t pq_para, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                          tvin_trans_fmt_t trans_fmt)
{
    int ret = 0, brightness = 50, contrast = 50, saturation = 50, hue = 50, sharnpess = 50;
    am_regs_t regs, regs_l;
    int level;

    if (pq_para.brightness >= 0 && pq_para.brightness <= 100) {
        if (mpPqData->PQ_GetBrightnessParams(source_port, sig_fmt, is3d, trans_fmt,
                                             pq_para.brightness, &brightness) == 0) {
        } else {
            LOGE("%s, PQ_GetBrightnessParams error!\n", "Vpp_SetPQParams");
        }

        ret |= VPP_SetVideoBrightness(brightness);
    }

    if (pq_para.contrast >= 0 && pq_para.contrast <= 100) {
        if (mpPqData->PQ_GetContrastParams(source_port, sig_fmt, is3d, trans_fmt, pq_para.contrast,
                                           &contrast) == 0) {
        } else {
            LOGE("%s, PQ_GetBrightnessParams error!\n", "Vpp_SetPQParams");
        }

        ret |= VPP_SetVideoContrast(contrast);
    }

    if (pq_para.saturation >= 0 && pq_para.saturation <= 100) {
        if (mpPqData->PQ_GetSaturationParams(source_port, sig_fmt, is3d, trans_fmt,
                                             pq_para.saturation, &saturation) == 0) {

            if (mbVppCfg_hue_reverse) {
                pq_para.hue = 100 - pq_para.hue;
            } else {
                pq_para.hue = pq_para.hue;
            }

            if (mpPqData->PQ_GetHueParams(source_port, sig_fmt, is3d, trans_fmt, pq_para.hue, &hue)
                    == 0) {
                if ((source_type == SOURCE_TYPE_TV || source_type == SOURCE_TYPE_AV) && (sig_fmt
                        == TVIN_SIG_FMT_CVBS_NTSC_M || sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_443)) {
                } else {
                    hue = 0;
                }
            } else {
                LOGE("%s, PQ_GetHueParams error!\n", "Vpp_SetPQParams");
            }
        } else {
            LOGE("%s, PQ_GetSaturationParams error!\n", "Vpp_SetPQParams");
        }

        ret |= VPP_SetVideoSaturationHue(saturation, hue);
    }

    if (pq_para.sharpness >= 0 && pq_para.sharpness <= 100) {
        level = pq_para.sharpness;

        if (mpPqData->PQ_GetSharpnessParams(source_port, sig_fmt, is3d, trans_fmt, level, &regs,
                                            &regs_l) == 0) {
            if (Vpp_LoadRegs(regs) < 0) {
                LOGE("%s, load reg for sharpness0 failed!\n", __FUNCTION__);
            }
            if (mpPqData->getSharpnessFlag() == 6 && Vpp_LoadRegs(regs_l) < 0) {
                LOGE("%s, load reg for sharpness1 failed!\n", __FUNCTION__);
            }
        } else {
            LOGE("%s, PQ_GetSharpnessParams failed!\n", __FUNCTION__);
        }
    }

    if (pq_mode == VPP_PICTURE_MODE_MOVIE) {
        ret |= SetColorTemperature(VPP_COLOR_TEMPERATURE_MODE_WARM, source_type, 1);
    } else if (pq_mode == VPP_PICTURE_MODE_COLORFUL) {
        ret |= SetColorTemperature(VPP_COLOR_TEMPERATURE_MODE_COLD, source_type, 1);
    }

    return ret;
}

int CVpp::Vpp_SetPQMode(vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type,
                        tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                        tvin_trans_fmt_t trans_fmt)
{
    vpp_pq_para_t pq_para;
    int ret = -1;

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        ret = SSMReadBrightness(source_type, &pq_para.brightness);
        ret = SSMReadContrast(source_type, &pq_para.contrast);
        ret = SSMReadSaturation(source_type, &pq_para.saturation);
        ret = SSMReadHue(source_type, &pq_para.hue);
        ret = SSMReadSharpness(source_type, &pq_para.sharpness);
    } /*else if (pq_mode == VPP_PICTURE_MODE_MOVIE) {
     ret = Vpp_GetPQModeValue ( source_type, VPP_PICTURE_MODE_SOFT, &pq_para );
     pq_para.brightness -=10;
     pq_para.contrast -=10;
     pq_para.saturation -=10;
     pq_para.sharpness -=10;
     } else if (pq_mode == VPP_PICTURE_MODE_COLORFUL) {
     ret = Vpp_GetPQModeValue ( source_type, VPP_PICTURE_MODE_BRIGHT, &pq_para );
     pq_para.brightness +=10;
     pq_para.contrast +=10;
     pq_para.saturation +=10;
     pq_para.sharpness +=10;
     }*/else {
        ret = Vpp_GetPQModeValue(source_type, pq_mode, &pq_para);
    }

    ret |= Vpp_SetPQParams(source_type, pq_mode, pq_para, source_port, sig_fmt, is3d, trans_fmt);

    return ret;
}

int CVpp::SavePQMode(vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type)
{
    vpp_pq_para_t pq_para;
    int ret = -1;
    int tmp_pic_mode = 0;

    tmp_pic_mode = (int) pq_mode;
    ret = SSMSavePictureMode(source_type, tmp_pic_mode);
    return ret;
}

int CVpp::SetPQMode(vpp_picture_mode_t pq_mode, tv_source_input_type_t source_type,
                    tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (0 == Vpp_SetPQMode(pq_mode, source_type, source_port, sig_fmt, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SavePQMode(pq_mode, source_type);
        } else {
            return 0;
        }
    }

    LOGE("%s, failed!", "SetPQMode");
    return -1;
}

vpp_picture_mode_t CVpp::GetPQMode(tv_source_input_type_t source_type)
{
    vpp_picture_mode_t data = VPP_PICTURE_MODE_STANDARD;
    int tmp_pic_mode = 0;

    SSMReadPictureMode(source_type, &tmp_pic_mode);
    data = (vpp_picture_mode_t) tmp_pic_mode;

    if (data < VPP_PICTURE_MODE_STANDARD || data >= VPP_PICTURE_MODE_MAX) {
        data = VPP_PICTURE_MODE_STANDARD;
    }

    return data;
}

int CVpp::Vpp_SetColorDemoMode(vpp_color_demomode_t demomode)
{
    cm_regmap_t regmap;
    unsigned long *temp_regmap;
    int i = 0;
    int tmp_demo_mode = 0;
    vpp_display_mode_t displaymode = VPP_DISPLAY_MODE_MODE43;

    switch (demomode) {
    case VPP_COLOR_DEMO_MODE_YOFF:
        temp_regmap = DemoColorYOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_COFF:
        temp_regmap = DemoColorCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_GOFF:
        temp_regmap = DemoColorGOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_MOFF:
        temp_regmap = DemoColorMOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ROFF:
        temp_regmap = DemoColorROffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_BOFF:
        temp_regmap = DemoColorBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_RGBOFF:
        temp_regmap = DemoColorRGBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_YMCOFF:
        temp_regmap = DemoColorYMCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLOFF:
        temp_regmap = DemoColorALLOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLON:
    default:
        if (displaymode == VPP_DISPLAY_MODE_MODE43) {
            temp_regmap = DemoColorSplit4_3RegMap;
        } else {
            temp_regmap = DemoColorSplitRegMap;
        }

        break;
    }

    for (i = 0; i < CM_REG_NUM; i++) {
        regmap.reg[i] = temp_regmap[i];
    }

    if (VPP_SetCMRegisterMap(&regmap) == 0) {
        tmp_demo_mode = demomode;
        LOGD("%s, demomode[%d] success.", "Vpp_SetColorDemoMode", demomode);
        return 0;
    }

    LOGE("%s, demomode[%d] failed.", "Vpp_SetColorDemoMode", demomode);
    return -1;
}

int CVpp::Vpp_SetBaseColorMode(vpp_color_basemode_t basemode, tvin_port_t source_port,
                               tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs;
    LOGD("%s.\n", "Vpp_SetBaseColorMode");

    if (mbVppCfg_new_cm) {
        if (mpPqData->PQ_GetCM2Params((vpp_color_management2_t) basemode, source_port, sig_fmt,
                                      is3d, trans_fmt, &regs) == 0) {
            ret = Vpp_LoadRegs(regs);
        } else {
            LOGE("PQ_GetCM2Params failed!\n");
        }
    }

    return ret;
}

int CVpp::Vpp_SetColorTemperatureUser(vpp_color_temperature_mode_t temp_mode __unused,
                                      tv_source_input_type_t source_type)
{
    tcon_rgb_ogo_t rgbogo;
    unsigned int gain_r, gain_g, gain_b;

    if (SSMReadRGBGainRStart(0, &gain_r) != 0) {
        return -1;
    }

    rgbogo.r_gain = gain_r;

    if (SSMReadRGBGainGStart(0, &gain_g) != 0) {
        return -1;
    }

    rgbogo.g_gain = gain_g;

    if (SSMReadRGBGainBStart(0, &gain_b) != 0) {
        return -1;
    }

    rgbogo.b_gain = gain_b;
    rgbogo.r_post_offset = 0;
    rgbogo.r_pre_offset = 0;
    rgbogo.g_post_offset = 0;
    rgbogo.g_pre_offset = 0;
    rgbogo.b_post_offset = 0;
    rgbogo.b_pre_offset = 0;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    LOGE("%s, source_type_user[%d] failed.", "Vpp_SetColorTemperatureUser", source_type);
    return -1;
}

int CVpp::Vpp_SetColorTemperature(vpp_color_temperature_mode_t Tempmode,
                                  tv_source_input_type_t source_type,
                                  tvin_port_t source_port __unused,
                                  tvin_sig_fmt_t sig_fmt __unused,
                                  tvin_trans_fmt_t trans_fmt __unused)
{
    tcon_rgb_ogo_t rgbogo, rgbPreOffset;
    int ret = -1;

    if (mbVppCfg_gamma_onoff) {
        VPP_SetGammaOnOff(0);
    } else {
        VPP_SetGammaOnOff(1);
    }

    GetColorTemperatureParams(Tempmode, &rgbogo);

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    LOGE("%s, source_type[%d] failed.", "Vpp_SetColorTemperature", source_type);
    return -1;
}

int CVpp::Vpp_SetBrightness(int value, tv_source_input_type_t source_type __unused, tvin_port_t source_port,
                            tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    int params;
    int level;

    if (value >= 0 && value <= 100) {
        level = value;
        LOGD("%s.\n", "Vpp_SetBrightness");

        if (mpPqData->PQ_GetBrightnessParams(source_port, sig_fmt, is3d, trans_fmt, level, &params)
                == 0) {
            if (VPP_SetVideoBrightness(params) == 0) {
                return 0;
            }
        } else {
            LOGE("%s, PQ_GetBrightnessParams failed!\n", "Vpp_SetBrightness");
        }
    }

    return ret;
}

int CVpp::VPP_SetVideoBrightness(int value)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/amvecm/brightness", "w");

    LOGD("~~~fopen~~~##VPP_SetVideoBrightness##%s : %d ##", "/sys/class/amvecm/brightness", value);

    if (fp == NULL) {
        LOGE("Open /sys/class/amvecm/brightness error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::SetBrightness(int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                        tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (0 == Vpp_SetBrightness(value, source_type, source_port, sig_fmt, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SSMSaveBrightness(source_type, value);
        } else {
            return 0;
        }
    } else {
        LOGE("%s, failed!", "SetBrightness");
        return -1;
    }
    return 0;
}

int CVpp::GetBrightness(tv_source_input_type_t source_type)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    vpp_picture_mode_t pq_mode = GetPQMode(source_type);

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        SSMReadBrightness(source_type, &data);
    } else {
        if (Vpp_GetPQModeValue(source_type, pq_mode, &pq_para) == 0) {
            data = pq_para.brightness;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    return data;
}

int CVpp::Vpp_SetContrast(int value, tv_source_input_type_t source_type __unused, tvin_port_t source_port,
                          tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    int params;
    int level;

    if (value >= 0 && value <= 100) {
        level = value;
        LOGD("%s.\n", "Vpp_SetContrast");

        if (mpPqData->PQ_GetContrastParams(source_port, sig_fmt, is3d, trans_fmt, level, &params)
                == 0) {
            if (VPP_SetVideoContrast(params) == 0) {
                return 0;
            }
        } else {
            LOGE("%s, PQ_GetContrastParams failed!\n", "Vpp_SetContrast");
        }
    }

    return ret;
}

int CVpp::VPP_SetVideoContrast(int value)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/amvecm/contrast", "w");
    LOGD("~~~fopen~~~##VPP_SetVideoContrast##%s : %d ##", "/sys/class/amvecm/contrast", value);

    if (fp == NULL) {
        LOGE("Open /sys/class/amvecm/contrast error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::SetContrast(int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                      tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (0 == Vpp_SetContrast(value, source_type, source_port, sig_fmt, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SSMSaveContrast(source_type, value);
        } else {
            return 0;
        }
    } else {
        LOGE("%s, failed!", "SetContrast");
        return -1;
    }
}

int CVpp::GetContrast(tv_source_input_type_t source_type)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    vpp_picture_mode_t pq_mode = GetPQMode(source_type);

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        SSMReadContrast(source_type, &data);
    } else {
        if (Vpp_GetPQModeValue(source_type, pq_mode, &pq_para) == 0) {
            data = pq_para.contrast;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    return data;
}

int CVpp::Vpp_SetSaturation(int value, tv_source_input_type_t source_type __unused, tvin_port_t source_port,
                            tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    int params;
    int level;
    int hue = 0;

    if (value >= 0 && value <= 100) {
        level = value;

        if (mpPqData->PQ_GetSaturationParams(source_port, sig_fmt, is3d, trans_fmt, level, &params)
                == 0) {
            if (VPP_SetVideoSaturationHue(params, hue) == 0) {
                return 0;
            }
        }
    }

    return ret;
}

int CVpp::SetSaturation(int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                        tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (0 == Vpp_SetSaturation(value, source_type, source_port, sig_fmt, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SSMSaveSaturation(source_type, value);
        } else {
            return 0;
        }
    } else {
        LOGE("%s, failed!", "SetSaturation");
        return -1;
    }
}

int CVpp::GetSaturation(tv_source_input_type_t source_type)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    vpp_picture_mode_t pq_mode = GetPQMode(source_type);

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        SSMReadSaturation(source_type, &data);
    } else {
        if (Vpp_GetPQModeValue(source_type, pq_mode, &pq_para) == 0) {
            data = pq_para.saturation;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    return data;
}

int CVpp::Vpp_SetHue(int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                     tvin_port_t source_port, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    int params, saturation_params;
    int level, saturation_level;

    if (value >= 0 && value <= 100) {
        if (mbVppCfg_hue_reverse) {
            level = 100 - value;
        } else {
            level = value;
        }

        if (mpPqData->PQ_GetHueParams(source_port, sig_fmt, is3d, trans_fmt, level, &params) == 0) {
            saturation_level = GetSaturation(source_type);

            if (mpPqData->PQ_GetSaturationParams(source_port, sig_fmt, is3d, trans_fmt,
                                                 saturation_level, &saturation_params) == 0) {
            } else {
                saturation_params = -20;
            }

            if (VPP_SetVideoSaturationHue(saturation_params, params) == 0) {
                return 0;
            }
        } else {
            LOGE("PQ_GetHueParams failed!\n");
        }
    }

    return ret;
}

int CVpp::SetHue(int value, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt,
                 tvin_trans_fmt_t trans_fmt, is_3d_type_t is3d, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (0 == Vpp_SetHue(value, source_type, sig_fmt, source_port, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SSMSaveHue(source_type, value);
        } else {
            return 0;
        }
    } else {
        LOGE("%s, failed!", "SetHue");
        return -1;
    }

    return 0;
}

int CVpp::GetHue(tv_source_input_type_t source_type)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    vpp_picture_mode_t pq_mode = GetPQMode(source_type);

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        SSMReadHue(source_type, &data);
    } else {
        if (Vpp_GetPQModeValue(source_type, pq_mode, &pq_para) == 0) {
            data = pq_para.hue;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    return data;
}

int CVpp::Vpp_SetSharpness(int value, tv_source_input_type_t source_type __unused, tvin_port_t source_port,
                           tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs, regs_l;
    int level;

    if (value >= 0 && value <= 100) {
        level = value;

        if (mpPqData->PQ_GetSharpnessParams(source_port, sig_fmt, is3d, trans_fmt, level, &regs, &regs_l) == 0) {
            LOGD("%s, sharpness flag:%d\n", __FUNCTION__, mpPqData->getSharpnessFlag());
            if (mpPqData->getSharpnessFlag() == 6) {
                if (Vpp_LoadRegs(regs) >= 0 && Vpp_LoadRegs(regs_l) >= 0)
                    ret = 0;
            } else {
                if (Vpp_LoadRegs(regs) >= 0)
                    ret = 0;
            }
        } else {
        }
    }

    return ret;
}

int CVpp::SetSharpness(int value, tv_source_input_type_t source_type, int is_enable,
                       is_3d_type_t is3d, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    if (Vpp_SetSharpness(value, source_type, source_port, sig_fmt, is3d, trans_fmt) < 0) {
        LOGE("%s, failed!", "SetSharpness");
        return -1;
    }

    if (is_save == 1) {
        if (is_enable) {
            return SSMSaveSharpness(source_type, value);

        }
    }

    return 0;
}

int CVpp::GetSharpness(tv_source_input_type_t source_type)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    vpp_picture_mode_t pq_mode = GetPQMode(source_type);

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        SSMReadSharpness(source_type, &data);
    } else {
        if (Vpp_GetPQModeValue(source_type, pq_mode, &pq_para) == 0) {
            data = pq_para.sharpness;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    LOGD("%s, data[%d].", "GetSharpness", data);
    return data;
}

int CVpp::SetColorSpaceMode(vpp_color_space_type_t colorSpace)
{
    int ret = -1, fileRet = -1;
    SSMSaveColorSpaceStart(colorSpace);

    switch (colorSpace) {
    case VPP_COLOR_SPACE_RGB:
        //marked by haifeng
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1da1 0xc" );
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1d70 0x208" );
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1d71 0x74" );
        break;

    case VPP_COLOR_SPACE_YUV:
        //marked by haifeng
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1da1 0xe" );
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1d70 0x208" );
        //        SetFileAttrValue ( "/sys/class/register/reg", "wc 0x1d71 0x76" );
        break;

    default:
        break;
    }

    return 0;
}

int CVpp::Vpp_SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode,
                                    tv_source_input_type_t source_type __unused,
                                    tvin_port_t source_port, tvin_sig_fmt_t sig_fmt,
                                    is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs;

    if (mbVppCfg_new_nr) {
        if (mpPqData->PQ_GetNR2Params((vpp_noise_reduction2_mode_t) nr_mode, source_port, sig_fmt,
                                      is3d, trans_fmt, &regs) == 0) {
            ret = Vpp_LoadRegs(regs);
        } else {
            LOGE("PQ_GetNR2Params failed!\n");
        }
    }

    return ret;
}

int CVpp::SaveNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode,
                                 tv_source_input_type_t source_type)
{
    int tmp_save_noisereduction_mode = (int) nr_mode;
    return SSMSaveNoiseReduction(source_type, tmp_save_noisereduction_mode);
}

int CVpp::SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode,
                                tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                                tvin_trans_fmt_t trans_fmt, int is_save)
{
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);
    if (0 == Vpp_SetNoiseReductionMode(nr_mode, source_type, source_port, sig_fmt, is3d, trans_fmt)) {
        if (is_save == 1) {
            return SaveNoiseReductionMode(nr_mode, source_type);
        } else {
            return 0;
        }
    }

    LOGE("%s, failed!", __FUNCTION__);
    return -1;
}

vpp_noise_reduction_mode_t CVpp::GetNoiseReductionMode(tv_source_input_type_t source_type)
{
    vpp_noise_reduction_mode_t data = VPP_NOISE_REDUCTION_MODE_MID;
    int tmp_nr_mode = 0;

    SSMReadNoiseReduction(source_type, &tmp_nr_mode);
    data = (vpp_noise_reduction_mode_t) tmp_nr_mode;

    if (data < VPP_NOISE_REDUCTION_MODE_OFF || data > VPP_NOISE_REDUCTION_MODE_AUTO) {
        data = VPP_NOISE_REDUCTION_MODE_MID;
    }

    return data;
}

int CVpp::Vpp_SetXVYCCMode(vpp_xvycc_mode_t xvycc_mode, tv_source_input_type_t source_type __unused,
                           tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                           tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs, regs_1;
    char prop_value[PROPERTY_VALUE_MAX];

    memset(prop_value, '\0', 16);
    const char *config_value;
    config_value = config_get_str(CFG_SECTION_TV, "vpp.xvycc.switch_control", "null");

    if (strcmp(config_value, "enable") == 0) {
        if (mpPqData->PQ_GetXVYCCParams((vpp_xvycc_mode_t) xvycc_mode, source_port, sig_fmt, is3d,
                                        trans_fmt, &regs, &regs_1) == 0) {
            ret = Vpp_LoadRegs(regs);
            ret |= Vpp_LoadRegs(regs_1);
        } else {
            LOGE("PQ_GetXVYCCParams failed!\n");
        }
    } else {
        LOGE("disable xvycc!\n");
    }
    return ret;
}

int CVpp::Vpp_SetMCDIMode(vpp_mcdi_mode_t mcdi_mode, tv_source_input_type_t source_type __unused,
                          tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d,
                          tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs;

    if (mpPqData->PQ_GetMCDIParams((vpp_mcdi_mode_t) mcdi_mode, source_port, sig_fmt, is3d,
                                   trans_fmt, &regs) == 0) {
        ret = Vpp_LoadRegs(regs);
    } else {
        LOGE("%s, PQ_GetMCDIParams failed!\n", __FUNCTION__);
    }
    return ret;
}

int CVpp::Vpp_SetDeblockMode(vpp_deblock_mode_t mode, tvin_port_t source_port,
                             tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs;

    if (mpPqData->PQ_GetDeblockParams(mode, source_port, sig_fmt, is3d, trans_fmt, &regs) == 0) {
        ret = Vpp_LoadRegs(regs);
    } else {
        LOGE("%s PQ_GetDeblockParams failed!\n", __FUNCTION__);
    }
    return ret;
}

int CVpp::Vpp_LoadGammaDefault(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt)
{
    int ret = -1;
    int panel_id = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType(source_type);

    LOGD("Enter %s.\n", __FUNCTION__);
    ret = mpPqData->PQ_GetGammaTableR(panel_id, source_port, sig_fmt, &gamma_r);
    ret |= mpPqData->PQ_GetGammaTableG(panel_id, source_port, sig_fmt, &gamma_g);
    ret |= mpPqData->PQ_GetGammaTableB(panel_id, source_port, sig_fmt, &gamma_b);

    if (ret == 0) {
        VPP_SetGammaTbl_R((unsigned short *) gamma_r.data);
        VPP_SetGammaTbl_G((unsigned short *) gamma_g.data);
        VPP_SetGammaTbl_B((unsigned short *) gamma_b.data);
    } else {
        LOGE("%s, PQ_GetGammaTable failed!", __FUNCTION__);
    }

    return ret;

}

int CVpp::Vpp_LoadGammaSpecial(int gammaValue)
{
    int ret = -1;
    int panel_id = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    LOGD("Enter %s.\n", __FUNCTION__);
    ret = mpPqData->PQ_GetGammaSpecialTable(gammaValue, "Red", &gamma_r);
    ret |= mpPqData->PQ_GetGammaSpecialTable(gammaValue, "Green", &gamma_g);
    ret |= mpPqData->PQ_GetGammaSpecialTable(gammaValue, "Blue", &gamma_b);

    if (ret == 0) {
        VPP_SetGammaTbl_R((unsigned short *) gamma_r.data);
        VPP_SetGammaTbl_G((unsigned short *) gamma_g.data);
        VPP_SetGammaTbl_B((unsigned short *) gamma_b.data);
    } else {
        LOGE("%s, PQ_GetGammaSpecialTable failed!", __FUNCTION__);
    }

    return ret;

}

int CVpp::Vpp_LoadGamma(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt)
{
    int gammaValue = 0, ret = -1;

    if (SSMReadGammaValue(&gammaValue) < 0) {
        LOGE("%s, SSMReadGammaValue ERROR, So Load Default GAMMA!\n", __FUNCTION__);
        ret = Vpp_LoadGammaDefault(source_type, sig_fmt);
        return -1;
    }

    if (gammaValue < -4 || gammaValue > 4) {
        LOGE("%s, Gamma Value beyond the UI's range of -4 to 4 .\n", __FUNCTION__);
        gammaValue = 0;
        SSMSaveGammaValue(gammaValue);
    }

    switch (gammaValue) {
    case 0:
        ret = Vpp_LoadGammaDefault(source_type, sig_fmt);
        break;

    default:
        ret = Vpp_LoadGammaSpecial(gammaValue);
        break;
    }

    return ret;
}

/*int CVpp::SetGammaValue(int gammaValue)
 {
     int ret = -1;
     tvin_sig_fmt_t sig_fmt = TVIN_SIG_FMT_NULL;
     tv_source_input_type_t source_type = SOURCE_TYPE_TV;
     sig_fmt = Tvin_GetSigFormat();
     source_type = Tvin_GetSrcInputType();
     LOGD("%s, source_type = %d, sig_fmt = %d, gammaValue = %d\n", __FUNCTION__, (int)source_type,
            (int)sig_fmt, gammaValue);
     if (gammaValue >= -4 || gammaValue <= 4) {
         switch (gammaValue) {
         case 0:
             ret = Vpp_LoadGammaDefault(source_type, sig_fmt);
             break;
         default:
             ret = Vpp_LoadGammaSpecial(gammaValue);
             break;
        }
        if (0 == ret)
            ret = SSMSaveGammaValue(gammaValue);
     }
     return ret;
 }*/

int CVpp::GetGammaValue()
{
    int gammaValue = 0;

    if (SSMReadGammaValue(&gammaValue) < 0) {
        LOGE("%s, SSMReadGammaValue ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    return gammaValue;
}

/*int CVpp::SetColorDemoMode ( vpp_color_demomode_t demomode )
 {
 tv_source_input_type_t source_type = Tvin_GetSrcInputType();
 tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceType ( source_type );
 tvin_sig_fmt_t sig_fmt = Tvin_GetSigFormat();
 is_3d_type_t is3d = Tvin_Get3DStatus();
 tvin_trans_fmt_t trans_fmt = Tvin_GetSigTransFormat();

 return SetBaseColorMode ( VPP_COLOR_BASE_MODE_DEMO ,source_port,sig_fmt,status,trans_fmt);
 }*/

vpp_color_demomode_t CVpp::GetColorDemoMode(void)
{
    vpp_color_demomode_t data = VPP_COLOR_DEMO_MODE_ALLON;
    unsigned char tmp_demo_mode = 0;
    SSMReadColorDemoMode(&tmp_demo_mode);
    data = (vpp_color_demomode_t) tmp_demo_mode;

    if (data < VPP_COLOR_DEMO_MODE_ALLON || data > VPP_COLOR_DEMO_MODE_ALLOFF) {
        data = VPP_COLOR_DEMO_MODE_ALLON;
    }

    return data;
}

int CVpp::SetBaseColorModeWithoutSave(vpp_color_basemode_t basemode, tvin_port_t source_port,
                                      tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    am_regs_t regs;

    if (mbVppCfg_new_cm) {
        if (mpPqData->PQ_GetCM2Params((vpp_color_management2_t) basemode, source_port, sig_fmt,
                                      is3d, trans_fmt, &regs) == 0) {
            ret = Vpp_LoadRegs(regs);
        } else {
            LOGE("PQ_GetCM2Params failed!\n");
        }
    }

    return ret;
}

int CVpp::SaveBaseColorMode(vpp_color_basemode_t basemode)
{
    int ret = -1;

    if (basemode == VPP_COLOR_BASE_MODE_DEMO) {
        ret = 0;
    } else {
        ret |= SSMSaveColorBaseMode(basemode);
    }

    return ret;
}

int CVpp::SetBaseColorMode(vpp_color_basemode_t basemode, tvin_port_t source_port,
                           tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    if (0 == SetBaseColorModeWithoutSave(basemode, source_port, sig_fmt, is3d, trans_fmt)) {
        return SaveBaseColorMode(basemode);
    } else {
        LOGE("SetBaseColorMode() Failed!!!");
        return -1;
    }
    return 0;
}

vpp_color_basemode_t CVpp::GetBaseColorMode(void)
{
    vpp_color_basemode_t data = VPP_COLOR_BASE_MODE_OFF;
    unsigned char tmp_base_mode = 0;
    SSMReadColorBaseMode(&tmp_base_mode);
    data = (vpp_color_basemode_t) tmp_base_mode;

    if (data < VPP_COLOR_BASE_MODE_OFF || data >= VPP_COLOR_BASE_MODE_MAX) {
        data = VPP_COLOR_BASE_MODE_OPTIMIZE;
    }

    return data;
}

int CVpp::SetColorTempWithoutSave(vpp_color_temperature_mode_t Tempmode,
                                  tv_source_input_type_t source_type __unused)
{
    tcon_rgb_ogo_t rgbogo;

    if (mbVppCfg_gamma_onoff) {
        VPP_SetGammaOnOff(0);
    } else {
        VPP_SetGammaOnOff(1);
    }

    GetColorTemperatureParams(Tempmode, &rgbogo);

    return VPP_SetRGBOGO(&rgbogo);

}

int CVpp::SaveColorTemp(vpp_color_temperature_mode_t Tempmode, tv_source_input_type_t source_type)
{
    int ret = -1;
    int tmp_temp_mode = 0;
    tcon_rgb_ogo_t rgbogo;

    if (mbVppCfg_whitebalance_sameparam) {
        source_type = SOURCE_TYPE_TV; //set all source share to use one group
    }

    GetColorTemperatureParams(Tempmode, &rgbogo);

    if (Tempmode < VPP_COLOR_TEMPERATURE_MODE_USER) {
        ret = SSMSaveRGBGainRStart(0, rgbogo.r_gain);
        ret |= SSMSaveRGBGainGStart(0, rgbogo.g_gain);
        ret |= SSMSaveRGBGainBStart(0, rgbogo.b_gain);
    }

    tmp_temp_mode = (int) Tempmode;

    if (mbVppCfg_colortemp_by_source) {
        ret |= SSMSaveColorTemperature((int) source_type, tmp_temp_mode);
    } else {
        ret |= SSMSaveColorTemperature(0, tmp_temp_mode);
    }

    return ret;
}

int CVpp::SetColorTemperature(vpp_color_temperature_mode_t Tempmode,
                              tv_source_input_type_t source_type, int is_save)
{
    if (SetColorTempWithoutSave(Tempmode, source_type) < 0) {
        LOGE("%s, failed!", __FUNCTION__);
        return -1;
    } else {
        if (is_save == 1) {
            return SaveColorTemp(Tempmode, source_type);
        } else {
            return 0;
        }
    }
}

vpp_color_temperature_mode_t CVpp::GetColorTemperature(tv_source_input_type_t source_type)
{
    vpp_color_temperature_mode_t data = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    int tmp_temp_mode = 0;

    if (mbVppCfg_colortemp_by_source) {
        SSMReadColorTemperature((int) source_type, &tmp_temp_mode);
    } else {
        SSMReadColorTemperature(0, &tmp_temp_mode);
    }

    data = (vpp_color_temperature_mode_t) tmp_temp_mode;

    if (data < VPP_COLOR_TEMPERATURE_MODE_STANDARD || data > VPP_COLOR_TEMPERATURE_MODE_USER) {
        data = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    }

    return data;
}

int CVpp::VPP_SetNonLinearFactor(int value)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/video/nonlinear_factor", "w");
    LOGD("~~~fopen~~~##VPP_SetNonLinearFactor##%s : %d ##", "/sys/class/video/nonlinear_factor",
         value);

    if (fp == NULL) {
        LOGE("Open /sys/class/video/nonlinear_factor error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);

    fclose(fp);
    fp = NULL;

    return 0;
}

vpp_display_mode_t CVpp::GetDisplayMode(tv_source_input_type_t source_type)
{
    vpp_display_mode_t data = VPP_DISPLAY_MODE_169;
    int tmp_dis_mode = 0;

    SSMReadDisplayMode(source_type, &tmp_dis_mode);
    data = (vpp_display_mode_t) tmp_dis_mode;

    return data;
}

int CVpp::SetBacklightWithoutSave(int value, tv_source_input_type_t source_type __unused)
{
    int backlight_value;

    if (value < 0 || value > 100) {
        value = 100;
    }

    if (mbVppCfg_backlight_reverse) {
        backlight_value = (100 - value) * 255 / 100;
    } else {
        backlight_value = value * 255 / 100;
    }

    return VPP_SetBackLightLevel(backlight_value);
}

int CVpp::VPP_SetBackLightLevel(int value)
{
    FILE *fp = NULL;
    fp = fopen("/sys/class/backlight/aml-bl/brightness", "w");
    LOGD("~~~fopen~~~##VPP_SetBackLightLevel##%s : %d ##",
         "/sys/class/backlight/aml-bl/brightness", value);

    if (fp == NULL) {
        LOGE("Open /sys/class/backlight/aml-bl/brightness error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);

    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::SetBacklight(int value, tv_source_input_type_t source_type, int is_save)
{
    static const int MIN_BACKLIGHT_VALUE = 1;
    if (value >= MIN_BACKLIGHT_VALUE) {
        if (SetBacklightWithoutSave(value, source_type) < 0) {
            LOGE("%s, failed !", __FUNCTION__);
            return -1;
        }

    } else {
        if (SetBacklightWithoutSave(MIN_BACKLIGHT_VALUE, source_type) < 0) {
            LOGE("%s, failed !", __FUNCTION__);
            return -1;
        }
    }
    if (is_save == 1) {
        return SaveBacklight(value, source_type);
    } else {
        return 0;
    }
}

int CVpp::GetBacklight(tv_source_input_type_t source_type)
{
    int data = 0;
    vpp_pq_para_t pq_para;

    if (mbVppCfg_pqmode_depend_bklight) {
        vpp_picture_mode_t pq_mode = GetPQMode(source_type);

        if (pq_mode == VPP_PICTURE_MODE_USER) {
            SSMReadBackLightVal(source_type, &data);
        } else {
            Vpp_GetPQModeValue(source_type, pq_mode, &pq_para);
            data = pq_para.backlight;
        }
    } else {
        source_type = SOURCE_TYPE_TV;
        SSMReadBackLightVal(source_type, &data);
    }

    if (data < 0 || data > 100) {
        data = 100;
    }

    return data;
}

int CVpp::SaveBacklight(int value, tv_source_input_type_t source_type)
{
    int backlight_value, backlight_reverse = 0;
    int ret = -1;
    int tmp_pic_mode = 0;

    if (!mbVppCfg_pqmode_depend_bklight) {
        source_type = SOURCE_TYPE_TV;
    }

    if (value < 0 || value > 100) {
        value = 100;
    }

    ret = SSMSaveBackLightVal(source_type, value);

    return ret;
}

int CVpp::VPP_SetBackLight_Switch(int value)
{
    FILE *fp = NULL;
    fp = fopen("/sys/class/backlight/aml-bl/bl_power", "w");
    LOGD("~~~fopen~~~##VPP_SetBackLight_Switch##%s : %d ##",
         "/sys/class/backlight/aml-bl/bl_power", value);

    if (fp == NULL) {
        LOGE("Open /sys/class/backlight/aml-bl/bl_power error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);

    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_GetBackLight_Switch(void)
{
    FILE *fp = NULL;
    int value;

    fp = fopen("/sys/class/backlight/aml-bl/bl_power", "w");

    if (fp == NULL) {
        LOGE("Open /sys/class/backlight/aml-bl/bl_power error(%s)!\n", strerror(errno));
        return -1;
    }

    fscanf(fp, "%d", &value);
    LOGD("~~~fopen~~~##VPP_GetBackLight_Switch##%s : %d ##",
         "/sys/class/backlight/aml-bl/bl_power", value);

    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        return value;
    }
}

int CVpp::SetDNLP(tv_source_input_type_t source_type, tvin_port_t source_port,
                  tvin_sig_fmt_t sig_fmt, is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    unsigned int dnlp_switch = 0;

    int ret = -1;
    int dnlpFlag = -1;
    ve_dnlp_t dnlp;
    ve_dnlp_table_t newdnlp;

    dnlp_switch = 1;

    if (mpPqData->PQ_GetDNLPParams(source_port, sig_fmt, is3d, trans_fmt, &dnlp, &newdnlp,
                                   &dnlpFlag) == 0) {
        newdnlp.en = dnlp_switch;
        LOGE("PQ_GetDNLPParams ok!\n");
        LOGE(
            "newdnlp.en:%d,newdnlp.method:%d,newdnlp.cliprate:%d,newdnlp.lowrange:%d,newdnlp.hghrange:%d,newdnlp.lowalpha:%d,newdnlp.midalpha:%d,newdnlp.hghalpha:%d\n",
            newdnlp.en, newdnlp.method, newdnlp.cliprate, newdnlp.lowrange, newdnlp.hghrange,
            newdnlp.lowalpha, newdnlp.midalpha, newdnlp.hghalpha);
        if (source_type == SOURCE_TYPE_DTV) {
            SetFileAttrValue("/sys/module/am_vecm/parameters/dnlp_en", "0");
        } else {
            VPP_SetVENewDNLP(&newdnlp);
            SetFileAttrValue("/sys/module/am_vecm/parameters/dnlp_en", "1");
        }
        ret = 1;
    } else {
        LOGE("mpPqData->PQ_GetDNLPParams failed!\n");
    }

    return ret;
}

int CVpp::VPP_SetVEDNLP(const struct ve_dnlp_s *pDNLP)
{
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_VE_DNLP, pDNLP);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEDNLP##AMVECM_IOC_VE_DNLP##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEDNLP, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVENewDNLP(const ve_dnlp_table_t *pDNLP)
{
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_VE_NEW_DNLP, pDNLP);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVENewDNLP##AMVECM_IOC_VE_NEW_DNLP##");

    if (rt < 0) {
        LOGE("VPP_SetVENewDNLP, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::SetDnlp_OFF(void)
{
    if (Vpp_SetDnlpOff() < 0) {
        LOGE("%s failed.\n", __FUNCTION__);
        return -1;
    } else {
        LOGE("%s success.\n", __FUNCTION__);
        SSMSaveDnlpStart(1); //save dnlp status to e2rom
        return 0;
    }
}
int CVpp::SetDnlp_ON(void)
{
    if (Vpp_SetDnlpOn() < 0) {
        LOGE("%s failed.\n", __FUNCTION__);
        return -1;
    } else {
        LOGE("%s success.\n", __FUNCTION__);
        SSMSaveDnlpStart(0); //save dnlp status to e2rom
        return 0;
    }
}

int CVpp::Vpp_SetDnlpOff(void)
{
    //According linux driver to modify the AMSTREAM_IOC_VE_DNLP_DIS  to the AMVECM_IOC_VE_DNLP_DIS.
    //int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_DNLP_DIS);
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_VE_DNLP_DIS);
    LOGD("~~~VPP_DeviceIOCtl~~~##Vpp_SetDnlpOff##AMVECM_IOC_VE_DNLP_DIS##");

    if (rt < 0) {
        LOGE("Vpp_SetDnlpOff, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::Vpp_SetDnlpOn(void)
{
    //According linux driver to modify the AMSTREAM_IOC_VE_DNLP_DIS  to the AMVECM_IOC_VE_DNLP_DIS.
    //int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_DNLP_EN);
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_VE_DNLP_EN);
    LOGD("~~~VPP_DeviceIOCtl~~~##Vpp_SetDnlpOn##AMVECM_IOC_VE_DNLP_EN##");

    if (rt < 0) {
        LOGE("Vpp_SetDnlpOn, error(%s)!\n", strerror(errno));
    }

    return rt;
}
int CVpp::GetDnlp_Status()
{
    unsigned char status = 0;
    SSMReadDnlpStart(&status);
    LOGD("%s, %d.", __FUNCTION__, status);
    return status;
}

int CVpp::VPP_SetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_S_RGB_OGO, rgbogo);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetRGBOGO##AMVECM_IOC_S_RGB_OGO##");
    usleep(50 * 1000);

    if (rt < 0) {
        LOGE("Vpp_api_SetRGBOGO, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_GetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_G_RGB_OGO, rgbogo);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_GetRGBOGO##AMVECM_IOC_G_RGB_OGO##");

    if (rt < 0) {
        LOGE("Vpp_api_GetRGBOGO, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::RGBGainValueSSMToRisterMapping(int gainValue) //0~255
{
    int mapValue = 0;

    if (gainValue < 0 || gainValue > 255) {
        mapValue = 1024;
    } else {
        if (gainValue == 255) {
            mapValue = 1536;
        } else {
            mapValue = 512 + gainValue * (1536 - 512) / 256;
        }
    }

    return mapValue;//512~1536
}

//RGB OFFSET:-128~127 <-----> -512~512
int CVpp::RGBOffsetValueSSMToRisterMapping(int offsetValue) //-128~127
{
    int mapValue = 0;

    if (offsetValue < -128 || offsetValue > 127) {
        offsetValue = 0;
    }

    if (offsetValue == 127) {
        mapValue = 512;
    } else {
        mapValue = 1024 * offsetValue / 256;
    }

    return mapValue;//-512~512
}

int CVpp::FactorySetPQMode_Brightness(int source_type, int pq_mode, int brightness)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        pq_para.brightness = brightness;

        if (mpPqData->PQ_SetPQModeParams((tv_source_input_type_t) source_type,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CVpp::FactoryGetPQMode_Brightness(int source_type, int pq_mode)
{
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        return -1;
    }

    return pq_para.brightness;
}

int CVpp::FactorySetPQMode_Contrast(int source_type, int pq_mode, int contrast)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        pq_para.contrast = contrast;

        if (mpPqData->PQ_SetPQModeParams((tv_source_input_type_t) source_type,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CVpp::FactoryGetPQMode_Contrast(int source_type, int pq_mode)
{
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        return -1;
    }

    return pq_para.contrast;
}

int CVpp::FactorySetPQMode_Saturation(int source_type, int pq_mode, int saturation)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        pq_para.saturation = saturation;

        if (mpPqData->PQ_SetPQModeParams((tv_source_input_type_t) source_type,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CVpp::FactoryGetPQMode_Saturation(int source_type, int pq_mode)
{
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        return -1;
    }

    return pq_para.saturation;
}

int CVpp::FactorySetPQMode_Hue(int source_type, int pq_mode, int hue)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        pq_para.hue = hue;

        if (mpPqData->PQ_SetPQModeParams((tv_source_input_type_t) source_type,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CVpp::FactoryGetPQMode_Hue(int source_type, int pq_mode)
{
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        return -1;
    }

    return pq_para.hue;
}

int CVpp::FactorySetPQMode_Sharpness(int source_type, int pq_mode, int sharpness)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        pq_para.sharpness = sharpness;

        if (mpPqData->PQ_SetPQModeParams((tv_source_input_type_t) source_type,
                                         (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CVpp::FactoryGetPQMode_Sharpness(int source_type, int pq_mode)
{
    vpp_pq_para_t pq_para;

    if (mpPqData->PQ_GetPQModeParams((tv_source_input_type_t) source_type,
                                     (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        return -1;
    }

    return pq_para.sharpness;
}

unsigned short CVpp::CalColorTemperatureParamsChecksum(void)
{
    unsigned char data_buf[SSM_CR_RGBOGO_LEN];
    unsigned short sum = 0;
    int cnt;
    USUC usuc;

    SSMReadRGBOGOValue(0, SSM_CR_RGBOGO_LEN, data_buf);

    for (cnt = 0; cnt < SSM_CR_RGBOGO_LEN; cnt++) {
        sum += data_buf[cnt];
    }

    //checksum = 0xff - sum % 0xff;

    LOGD("%s, sum = 0x%X.\n", __FUNCTION__, sum);

    return sum;
}

int CVpp::SetColorTempParamsChecksum(void)
{
    int ret = 0;
    USUC usuc;

    usuc.s = CalColorTemperatureParamsChecksum();

    LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    ret |= SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    return ret;
}
unsigned short CVpp::GetColorTempParamsChecksum(void)
{
    USUC usuc;

    SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    return usuc.s;

}

int CVpp::CheckTempDataLable(void)
{
    USUC usuc;
    USUC ret;

    SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, ret.c);

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    if ((usuc.c[0] == ret.c[0]) && (usuc.c[1] == ret.c[1])) {
        LOGD("%s, lable ok.\n", __FUNCTION__);
        return 1;
    } else {
        LOGD("%s, lable error.\n", CFG_SECTION_TV);
        return 0;
    }
}

int CVpp::SetTempDataLable(void)
{
    USUC usuc;
    int ret = 0;

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    ret = SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, usuc.c);

    return ret;
}

int CVpp::GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    //    CheckColorTemperatureParamAlldata(source_port,sig_fmt,trans_fmt);

    return ReadColorTemperatureParams(Tempmode, params);
}

int CVpp::ReadColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;

    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        ret |= SSMReadRGBOGOValue(0, 2, usuc.c);
        params->en = usuc.s;

        ret |= SSMReadRGBOGOValue(2, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(4, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(6, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(8, 2, usuc.c);
        params->r_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(10, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(12, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(14, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(16, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(18, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        ret |= SSMReadRGBOGOValue(20, 2, usuc.c);
        params->en = usuc.s;

        ret |= SSMReadRGBOGOValue(22, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(24, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(26, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(28, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= SSMReadRGBOGOValue(30, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(32, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(34, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(36, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(38, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        ret |= SSMReadRGBOGOValue(40, 2, usuc.c);
        params->en = usuc.s;

        ret |= SSMReadRGBOGOValue(42, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(44, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(46, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= SSMReadRGBOGOValue(48, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= SSMReadRGBOGOValue(50, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= SSMReadRGBOGOValue(52, 2, usuc.c);
        params->b_gain = usuc.s;
        ret |= SSMReadRGBOGOValue(54, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(56, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= SSMReadRGBOGOValue(58, 2, suc.c);
        params->b_post_offset = suc.s;
    }

    LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
         params->r_gain, params->g_gain, params->b_gain, params->r_post_offset,
         params->g_post_offset, params->b_post_offset);

    return ret;
}

int CVpp::SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    //    CheckColorTemperatureParamAlldata(source_port,sig_fmt,trans_fmt);

    SaveColorTemperatureParams(Tempmode, params);
    SetColorTempParamsChecksum();

    return 0;
}

int CVpp::SaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;

    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        usuc.s = params.en;
        ret |= SSMSaveRGBOGOValue(0, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= SSMSaveRGBOGOValue(2, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= SSMSaveRGBOGOValue(4, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= SSMSaveRGBOGOValue(6, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= SSMSaveRGBOGOValue(8, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= SSMSaveRGBOGOValue(10, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= SSMSaveRGBOGOValue(12, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= SSMSaveRGBOGOValue(14, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= SSMSaveRGBOGOValue(16, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= SSMSaveRGBOGOValue(18, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        usuc.s = params.en;
        ret |= SSMSaveRGBOGOValue(20, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= SSMSaveRGBOGOValue(22, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= SSMSaveRGBOGOValue(24, 2, suc.c);
        suc.s = params.b_pre_offset;
        ret |= SSMSaveRGBOGOValue(26, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= SSMSaveRGBOGOValue(28, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= SSMSaveRGBOGOValue(30, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= SSMSaveRGBOGOValue(32, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= SSMSaveRGBOGOValue(34, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= SSMSaveRGBOGOValue(36, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= SSMSaveRGBOGOValue(38, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        usuc.s = params.en;
        ret |= SSMSaveRGBOGOValue(40, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= SSMSaveRGBOGOValue(42, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= SSMSaveRGBOGOValue(44, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= SSMSaveRGBOGOValue(46, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= SSMSaveRGBOGOValue(48, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= SSMSaveRGBOGOValue(50, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= SSMSaveRGBOGOValue(52, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= SSMSaveRGBOGOValue(54, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= SSMSaveRGBOGOValue(56, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= SSMSaveRGBOGOValue(58, 2, suc.c);
    }

    LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
         params.r_gain, params.g_gain, params.b_gain, params.r_post_offset,
         params.g_post_offset, params.b_post_offset);
    return ret;
}

int CVpp::CheckColorTemperatureParams(void)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    for (i = 0; i < 3; i++) {
        ReadColorTemperatureParams((vpp_color_temperature_mode_t) i, &rgbogo);

        if (rgbogo.r_gain > 2047 || rgbogo.b_gain > 2047 || rgbogo.g_gain > 2047
            /*|| rgbogo.r_gain < 0 || rgbogo.b_gain < 0 || rgbogo.g_gain < 0*/) {
            if (rgbogo.r_post_offset > 1023 || rgbogo.g_post_offset > 1023 || rgbogo.b_post_offset > 1023 ||
                rgbogo.r_post_offset < -1024 || rgbogo.g_post_offset < -1024 || rgbogo.b_post_offset < -1024) {
                return 0;
            }
        }
    }

    return 1;
}

int CVpp::RestoeColorTemperatureParamsFromDB(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt,
        tvin_trans_fmt_t trans_fmt)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    LOGD("%s, restore color temperature params from DB.\n", __FUNCTION__);

    for (i = 0; i < 3; i++) {
        mpPqData->PQ_GetColorTemperatureParams((vpp_color_temperature_mode_t) i, source_port,
                                               sig_fmt, trans_fmt, &rgbogo);
        SaveColorTemperatureParams((vpp_color_temperature_mode_t) i, rgbogo);
    }

    SetColorTempParamsChecksum();

    return 0;
}

int CVpp::CheckColorTemperatureParamAlldata(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt,
        tvin_trans_fmt_t trans_fmt)
{
    if (CheckTempDataLable() && (CalColorTemperatureParamsChecksum()
                                 == GetColorTempParamsChecksum())) {
        LOGD("%s, color temperature param lable & checksum ok.\n", __FUNCTION__);

        if (CheckColorTemperatureParams() == 0) {
            LOGD("%s, color temperature params check failed.\n", __FUNCTION__);
            RestoeColorTemperatureParamsFromDB(source_port, sig_fmt, trans_fmt);
        }
    } else {
        LOGD("%s, color temperature param data error.\n", __FUNCTION__);

        SetTempDataLable();
        RestoeColorTemperatureParamsFromDB(source_port, sig_fmt, trans_fmt);
    }

    return 0;
}

int CVpp::FactorySetColorTemp_Rgain(int source_type, int colortemp_mode, int rgain)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.r_gain = rgain;
    LOGD("%s, source_type[%d], colortemp_mode[%d], rgain[%d].", __FUNCTION__, source_type,
         colortemp_mode, rgain);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Rgain(int source_type __unused, int colortemp_mode, int rgain)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.r_gain = rgain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Rgain error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Rgain(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.r_gain;
    }

    LOGE("FactoryGetColorTemp_Rgain error!\n");
    return -1;
}

int CVpp::FactorySetColorTemp_Ggain(int source_type, int colortemp_mode, int ggain)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.g_gain = ggain;
    LOGD("%s, source_type[%d], colortemp_mode[%d], ggain[%d].", __FUNCTION__, source_type,
         colortemp_mode, ggain);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Ggain(int source_type __unused, int colortemp_mode, int ggain)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.g_gain = ggain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Ggain error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Ggain(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.g_gain;
    }

    LOGE("FactoryGetColorTemp_Ggain error!\n");
    return -1;
}

int CVpp::FactorySetColorTemp_Bgain(int source_type, int colortemp_mode, int bgain)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.b_gain = bgain;
    LOGD("%s, source_type[%d], colortemp_mode[%d], bgain[%d].", __FUNCTION__, source_type,
         colortemp_mode, bgain);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Bgain(int source_type __unused, int colortemp_mode, int bgain)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.b_gain = bgain;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Bgain error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Bgain(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.b_gain;
    }

    LOGE("FactoryGetColorTemp_Bgain error!\n");
    return -1;
}

int CVpp::FactorySetColorTemp_Roffset(int source_type, int colortemp_mode, int roffset)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.r_post_offset = roffset;
    LOGD("%s, source_type[%d], colortemp_mode[%d], r_post_offset[%d].", __FUNCTION__, source_type,
         colortemp_mode, roffset);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Roffset(int source_type __unused, int colortemp_mode, int roffset)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.r_post_offset = roffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Roffset error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Roffset(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.r_post_offset;
    }

    LOGE("FactoryGetColorTemp_Roffset error!\n");
    return -1;
}

int CVpp::FactorySetColorTemp_Goffset(int source_type, int colortemp_mode, int goffset)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.g_post_offset = goffset;
    LOGD("%s, source_type[%d], colortemp_mode[%d], g_post_offset[%d].", __FUNCTION__, source_type,
         colortemp_mode, goffset);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Goffset(int source_type __unused, int colortemp_mode, int goffset)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.g_post_offset = goffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Goffset error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Goffset(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.g_post_offset;
    }

    LOGE("FactoryGetColorTemp_Goffset error!\n");
    return -1;
}

int CVpp::FactorySetColorTemp_Boffset(int source_type, int colortemp_mode, int boffset)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo);
    rgbogo.b_post_offset = boffset;
    LOGD("%s, source_type[%d], colortemp_mode[%d], b_post_offset[%d].", __FUNCTION__, source_type,
         colortemp_mode, boffset);
    rgbogo.en = 1;

    if (VPP_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    return -1;
}

int CVpp::FactorySaveColorTemp_Boffset(int source_type __unused, int colortemp_mode, int boffset)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        rgbogo.b_post_offset = boffset;
        return SetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, rgbogo);
    }

    LOGE("FactorySaveColorTemp_Boffset error!\n");
    return -1;
}

int CVpp::FactoryGetColorTemp_Boffset(int source_type __unused, int colortemp_mode)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemp_mode, &rgbogo)) {
        return rgbogo.b_post_offset;
    }

    LOGE("FactoryGetColorTemp_Boffset error!\n");
    return -1;
}

int CVpp::FactoryGetTestPattern(void)
{
    unsigned char data = VPP_TEST_PATTERN_NONE;
    SSMReadTestPattern(&data);
    return data;
}

int CVpp::FactoryResetPQMode(void)
{
    mpPqData->PQ_ResetAllPQModeParams();
    return 0;
}

int CVpp::FactoryResetNonlinear(void)
{
    mpPqData->PQ_ResetAllNoLineParams();
    return 0;
}

int CVpp::FactoryResetColorTemp(void)
{
    mpPqData->PQ_ResetAllColorTemperatureParams();
    return 0;
}

int CVpp::FactorySetParamsDefault(void)
{
    FactoryResetPQMode();
    FactoryResetNonlinear();
    FactoryResetColorTemp();
    mpPqData->PQ_ResetAllOverscanParams();
    return 0;
}

int CVpp::FactorySetDDRSSC(int step)
{
    int ret = -1;

    switch (step) {
    case 1:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000ac86" );
        break;

    case 2:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000ac85" );
        break;

    case 3:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000ac75" );
        break;

    case 4:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000ac65" );
        break;

    case 5:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000acb3" );
        break;

    case 0:
    default:
        //        ret = Tv_MiscRegs ( "wc 0x14e6 0x0000ac24" );
        break;
    }

    if (ret < 0) {
        return -1;
    }

    return SSMSaveDDRSSC(step);
}

int CVpp::FactoryGetDDRSSC(void)
{
    unsigned char data = 0;
    SSMReadDDRSSC(&data);
    return data;
}

int CVpp::SetLVDSSSC(int step)
{
    int ret = -1;
    if (step > 4)
        step = 4;
    ret = TvMisc_SetLVDSSSC(step);
    return ret;
}
int CVpp::FactorySetLVDSSSC(int step)
{
    int ret = -1;
    unsigned char data[2] = {0, 0};
    char cmd_tmp_1[128];
    int value = 0, panel_idx = 0, tmp = 0;
    const char *PanelIdx;
    if (step > 3)
        step = 3;

    PanelIdx = config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );
    panel_idx = strtoul(PanelIdx, NULL, 10);
    LOGD ("%s, panel_idx = %x", __FUNCTION__, panel_idx);
    SSMReadLVDSSSC(data);

    //every 2 bits represent one panel, use 2 byte store 8 panels
    value = (data[1] << 8) | data[0];
    step = step & 0x03;
    panel_idx = panel_idx * 2;
    tmp = 3 << panel_idx;
    value = (value & (~tmp)) | (step << panel_idx);
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    LOGD ("%s, tmp = %x, save value = %x", __FUNCTION__, tmp, value);

    SetLVDSSSC(step);
    return SSMSaveLVDSSSC(data);
}

int CVpp::FactoryGetLVDSSSC(void)
{
    unsigned char data[2] = {0, 0};
    int value = 0, panel_idx = 0;
    const char *PanelIdx = config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );

    panel_idx = strtoul(PanelIdx, NULL, 10);
    SSMReadLVDSSSC(data);
    value = (data[1] << 8) | data[0];
    value = (value >> (2 * panel_idx)) & 0x03;
    LOGD ("%s, panel_idx = %x, value= %d", __FUNCTION__, panel_idx, value);
    return value;
}

noline_params_t CVpp::FactoryGetNolineParams(int type, int source_type)
{
    int ret = -1;
    noline_params_t noline_params;

    memset(&noline_params, 0, sizeof(noline_params_t));

    switch (type) {
    case NOLINE_PARAMS_TYPE_BRIGHTNESS:
        ret = mpPqData->PQ_GetNoLineAllBrightnessParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_CONTRAST:
        ret = mpPqData->PQ_GetNoLineAllContrastParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_SATURATION:
        ret = mpPqData->PQ_GetNoLineAllSaturationParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_HUE:
        ret = mpPqData->PQ_GetNoLineAllHueParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_SHARPNESS:
        ret = mpPqData->PQ_GetNoLineAllSharpnessParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_VOLUME:
        ret = mpPqData->PQ_GetNoLineAllVolumeParams((tv_source_input_type_t) source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    default:
        break;
    }

    return noline_params;
}

int CVpp::FactorySetNolineParams(int type, int source_type, noline_params_t noline_params)
{
    int ret = -1;

    switch (type) {
    case NOLINE_PARAMS_TYPE_BRIGHTNESS:
        ret = mpPqData->PQ_SetNoLineAllBrightnessParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_CONTRAST:
        ret = mpPqData->PQ_SetNoLineAllContrastParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);
        break;

    case NOLINE_PARAMS_TYPE_SATURATION:
        ret = mpPqData->PQ_SetNoLineAllSaturationParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);

    case NOLINE_PARAMS_TYPE_HUE:
        ret = mpPqData->PQ_SetNoLineAllHueParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);

    case NOLINE_PARAMS_TYPE_SHARPNESS:
        ret = mpPqData->PQ_SetNoLineAllSharpnessParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);

    case NOLINE_PARAMS_TYPE_VOLUME:
        ret = mpPqData->PQ_SetNoLineAllVolumeParams((tv_source_input_type_t) source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);

    default:
        break;
    }

    return ret;
}

int CVpp::FactorySetOverscan(int source_type, int fmt, int status_3d __unused, int trans_fmt,
                             tvin_cutwin_t cutwin_t)
{
    int ret = -1;
    ret = mpPqData->PQ_SetOverscanParams((tv_source_input_type_t) source_type,
                                         (tvin_sig_fmt_t) fmt, INDEX_2D, (tvin_trans_fmt_t) trans_fmt, cutwin_t);

    if (ret == 0) {
    } else {
        LOGE("%s, PQ_SetOverscanParams fail.\n", __FUNCTION__);
    }
    return ret;
}

tvin_cutwin_t CVpp::FactoryGetOverscan(int source_type, int fmt, is_3d_type_t is3d, int trans_fmt)
{
    int ret = -1;
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    if (trans_fmt < TVIN_TFMT_2D || trans_fmt > TVIN_TFMT_3D_LDGD) {
        return cutwin_t;
    }

    ret = mpPqData->PQ_GetOverscanParams((tv_source_input_type_t) source_type,
                                         (tvin_sig_fmt_t) fmt, is3d, (tvin_trans_fmt_t) trans_fmt, VPP_DISPLAY_MODE_169,
                                         &cutwin_t);

    if (ret == 0) {
    } else {
        LOGE("%s, PQ_GetOverscanParams faild.\n", __FUNCTION__);
    }

    return cutwin_t;
}

int CVpp::FactorySetBacklightPWM_Frequency(int freq)
{
    LOGD("%s,FactorySetBacklightPWM_Frequency set freq %d .\n", __FUNCTION__, freq);
    return 1;
}

int CVpp::FactoryGetBacklightPWM_Frequency(void)
{
    int freq = 50;
    LOGD("%s,FactoryGetBacklightPWM_Frequency set freq %d .\n", CFG_SECTION_TV, freq);
    return freq;
}

int CVpp::FactorySetBacklight_Switch_status(int status)
{
    LOGD("%s,FactorySetBacklight_Switch_status set status %d .\n", __FUNCTION__, status);
    return 1;
}

int CVpp::FactoryGetBacklight_Switch_status(void)
{
    int status = 1;
    LOGD("%s,FactoryGetBacklight_Switch_status get status %d .\n", __FUNCTION__, status);
    return status;
}

int CVpp::FactorySetBacklightPWM_Duty(int duty)
{
    LOGD("%s,FactorySetBacklight_Switch_status set duty %d .\n", __FUNCTION__, duty);
    return 1;
}

int CVpp::FactoryGetBacklightPWM_Duty(void)
{
    int duty = 1;
    LOGD("%s,FactoryGetBacklight_Switch_status get duty %d .\n", __FUNCTION__, duty);
    return duty;
}

int CVpp::FactorySetLVDS_ColorDepth(int depth)
{
    LOGD("%s,FactorySetLVDS_ColorDepth set depth %d .\n", CFG_SECTION_TV, depth);
    return 1;
}

int CVpp::FactoryGetLVDS_ColorDepth(void)
{
    int depth = 1;
    LOGD("%s,FactorySetLVDS_ColorDepth get freq %d .\n", __FUNCTION__, depth);
    return depth;
}

int CVpp::FactorySetLVDS_ColorDither_status(int status)
{
    LOGD("%s,FactorySetLVDS_ColorDither_status set status %d .\n", __FUNCTION__, status);
    return 1;
}

int CVpp::FactoryGetLVDS_ColorDither_status(void)
{
    int status = 1;
    LOGD("%s,FactoryGetLVDS_ColorDither_status get status %d .\n", __FUNCTION__, status);
    return status;
}

int CVpp::FactorySetLVDS_Mapping_status(int status)
{
    LOGD("%s,FactorySetLVDS_Mapping_status set status %d .\n", __FUNCTION__, status);
    return 1;
}

int CVpp::FactoryGetLVDS_Mapping_status(void)
{
    int status = 1;
    LOGD("%s,FactoryGetLVDS_Mapping_status get status %d .\n", __FUNCTION__, status);
    return status;
}

int CVpp::FactorySetLVDS_PortSwap_status(int status)
{
    LOGD("%s,FactorySetLVDS_PortSwap_status set status %d .\n", __FUNCTION__, status);
    return 1;
}

int CVpp::FactoryGetLVDS_PortSwap_status(void)
{
    int status = 1;
    LOGD("%s,FactoryGetLVDS_PortSwap_status get status %d .\n", __FUNCTION__, status);
    return status;
}

int CVpp::VPPSSMRestoreDefault()
{
    int i = 0, tmp_val = 0;
    int tmp_panorama_nor = 0, tmp_panorama_full = 0;
    int offset_r = 0, offset_g = 0, offset_b = 0, gain_r = 1024, gain_g = 1024, gain_b = 1024;
    int8_t std_buf[6] = { 0, 0, 0, 0, 0, 0 };
    int8_t warm_buf[6] = { 0, 0, -8, 0, 0, 0 };
    int8_t cold_buf[6] = { -8, 0, 0, 0, 0, 0 };
    unsigned char tmp[2] = {0, 0};

    SSMSaveColorDemoMode ( VPP_COLOR_DEMO_MODE_ALLON);
    SSMSaveColorBaseMode ( VPP_COLOR_BASE_MODE_OPTIMIZE);
    SSMSaveRGBGainRStart(0, gain_r);
    SSMSaveRGBGainGStart(0, gain_g);
    SSMSaveRGBGainBStart(0, gain_b);
    SSMSaveRGBPostOffsetRStart(0, offset_r);
    SSMSaveRGBPostOffsetGStart(0, offset_g);
    SSMSaveRGBPostOffsetBStart(0, offset_b);
    SSMSaveUserNatureLightSwitch(1);
    SSMSaveGammaValue(0);
    SSMSaveGraphyBacklight(100);
    SSMSaveDBCStart(0);
    SSMSaveDnlpStart(0); //0: ON,1: OFF,default is on
    SSMSaveAPL(30);
    SSMSaveAPL2(30);
    SSMSaveBD(30);
    SSMSaveBP(30);

    SSMSaveFBCELECmodeVal(11);
    SSMSaveFBCN360BackLightVal(10);
    SSMSaveFBCN360ColorTempVal(1); // standard colortemp

    SSMSaveFBCN310ColorTempVal(0);
    SSMSaveFBCN310LightsensorVal(0);
    SSMSaveFBCN310Dream_PanelVal(1);
    SSMSaveFBCN310MULT_PQVal(1);
    SSMSaveFBCN310MEMCVal(2);
    SSMSaveFBCN310BackLightVal(254);
    for (i = 0; i < 6; i++) {
        SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_STANDARD * 6, std_buf[i]); //0~5
        SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_WARM * 6, warm_buf[i]); //6~11
        SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_COLD * 6, cold_buf[i]); //12~17
    }

    for (i = 0; i < SOURCE_TYPE_MAX; i++) {
        if (i == SOURCE_TYPE_HDMI) {
            SSMSaveColorSpaceStart ( VPP_COLOR_SPACE_AUTO);
        }

        tmp_val = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
        tmp_panorama_nor = VPP_PANORAMA_MODE_NORMAL;
        tmp_panorama_full = VPP_PANORAMA_MODE_FULL;

        if (i == SOURCE_TYPE_HDMI) {
            SSMSavePanoramaStart(i, tmp_panorama_full);
        } else {
            SSMSavePanoramaStart(i, tmp_panorama_nor);
        }

        SSMSaveColorTemperature(i, tmp_val);
        tmp_val = 50;
        SSMSaveBrightness(i, tmp_val);
        SSMSaveContrast(i, tmp_val);
        SSMSaveSaturation(i, tmp_val);
        SSMSaveHue(i, tmp_val);
        SSMSaveSharpness(i, tmp_val);
        tmp_val = VPP_PICTURE_MODE_STANDARD;
        SSMSavePictureMode(i, tmp_val);
        tmp_val = VPP_DISPLAY_MODE_169;
        SSMSaveDisplayMode(i, tmp_val);
        tmp_val = VPP_NOISE_REDUCTION_MODE_AUTO;
        SSMSaveNoiseReduction(i, tmp_val);
        tmp_val = 100;
        SSMSaveBackLightVal(i, tmp_val);
    }

    SSMSaveDDRSSC(0);
    SSMSaveLVDSSSC(tmp);
    return 0;
}

int CVpp::VPPSSMFacRestoreDefault()
{
    return VPPSSMRestoreDefault();
}

int CVpp::SetRGBValue(vpp_color_temperature_mode_t temp_mode, unsigned char data_buf[])
{
    int8_t r_gain = 0, b_gain = 0, g_gain = 0, r_offset = 0, g_offset = 0, b_offset = 0;
    int ret = -1;
    tcon_rgb_ogo_t rgbogo;
    rgbogo.en = 1;
    rgbogo.r_pre_offset = 0;
    rgbogo.g_pre_offset = 0;
    rgbogo.b_pre_offset = 0;
    r_gain = data_buf[0];
    g_gain = data_buf[1];
    b_gain = data_buf[2];
    r_offset = data_buf[3];
    g_offset = data_buf[4];
    b_offset = data_buf[5];
    int mode = (int) temp_mode;

    switch (mode) {
    case 1:
        ret = SSMSaveRGBValueStart(0 + mode * 6, r_gain);
        ret |= SSMSaveRGBValueStart(1 + mode * 6, g_gain);
        ret |= SSMSaveRGBValueStart(2 + mode * 6, b_gain);
        ret |= SSMSaveRGBValueStart(3 + mode * 6, r_offset);
        ret |= SSMSaveRGBValueStart(4 + mode * 6, g_offset);
        ret |= SSMSaveRGBValueStart(5 + mode * 6, b_offset);
        break;

    case 2:
        ret = SSMSaveRGBValueStart(0 + mode * 6, r_gain);
        ret |= SSMSaveRGBValueStart(1 + mode * 6, g_gain);
        ret |= SSMSaveRGBValueStart(2 + mode * 6, b_gain);
        ret |= SSMSaveRGBValueStart(3 + mode * 6, r_offset);
        ret |= SSMSaveRGBValueStart(4 + mode * 6, g_offset);
        ret |= SSMSaveRGBValueStart(5 + mode * 6, b_offset);
        break;

    case 0:
    default:
        ret = SSMSaveRGBValueStart(0, r_gain);
        ret |= SSMSaveRGBValueStart(1, g_gain);
        ret |= SSMSaveRGBValueStart(2, b_gain);
        ret |= SSMSaveRGBValueStart(3, r_offset);
        ret |= SSMSaveRGBValueStart(4, g_offset);
        ret |= SSMSaveRGBValueStart(5, b_offset);
        break;
    }

    if (ret == 0) {
        rgbogo.r_gain = RGBGainValueSSMToRisterMapping(r_gain + 128);
        rgbogo.g_gain = RGBGainValueSSMToRisterMapping(g_gain + 128);
        rgbogo.b_gain = RGBGainValueSSMToRisterMapping(b_gain + 128);
        rgbogo.r_post_offset = RGBOffsetValueSSMToRisterMapping(r_offset);
        rgbogo.g_post_offset = RGBOffsetValueSSMToRisterMapping(g_offset);
        rgbogo.b_post_offset = RGBOffsetValueSSMToRisterMapping(b_offset);
        ret |= VPP_SetRGBOGO(&rgbogo);
    }

    return ret;
}

int CVpp::GetRGBValue(vpp_color_temperature_mode_t temp_mode, tcon_rgb_ogo_t *p_rgbogo)
{
    int8_t r_gain = 0, b_gain = 0, g_gain = 0, r_offset = 0, g_offset = 0, b_offset = 0;
    int ret = -1;

    p_rgbogo->en = 1;
    p_rgbogo->r_pre_offset = 0;
    p_rgbogo->g_pre_offset = 0;
    p_rgbogo->b_pre_offset = 0;
    p_rgbogo->r_gain = 0;
    p_rgbogo->g_gain = 0;
    p_rgbogo->b_gain = 0;
    p_rgbogo->r_post_offset = 0;
    p_rgbogo->g_post_offset = 0;
    p_rgbogo->b_post_offset = 0;
    int mode = (int) temp_mode;

    switch (mode) {
    case 1:
        ret = SSMReadRGBValueStart(0 + mode * 6, &r_gain);
        ret |= SSMReadRGBValueStart(1 + mode * 6, &g_gain);
        ret |= SSMReadRGBValueStart(2 + mode * 6, &b_gain);
        ret |= SSMReadRGBValueStart(3 + mode * 6, &r_offset);
        ret |= SSMReadRGBValueStart(4 + mode * 6, &g_offset);
        ret |= SSMReadRGBValueStart(5 + mode * 6, &b_offset);
        break;

    case 2:
        ret = SSMReadRGBValueStart(0 + mode * 6, &r_gain);
        ret |= SSMReadRGBValueStart(1 + mode * 6, &g_gain);
        ret |= SSMReadRGBValueStart(2 + mode * 6, &b_gain);
        ret |= SSMReadRGBValueStart(3 + mode * 6, &r_offset);
        ret |= SSMReadRGBValueStart(4 + mode * 6, &g_offset);
        ret |= SSMReadRGBValueStart(5 + mode * 6, &b_offset);
        break;

    case 0:
    default:
        ret = SSMReadRGBValueStart(0, &r_gain);
        ret |= SSMReadRGBValueStart(1, &g_gain);
        ret |= SSMReadRGBValueStart(2, &b_gain);
        ret |= SSMReadRGBValueStart(3, &r_offset);
        ret |= SSMReadRGBValueStart(4, &g_offset);
        ret |= SSMReadRGBValueStart(5, &b_offset);
        break;
    }

    p_rgbogo->r_gain = r_gain + 128; //r_gain:-128~127
    p_rgbogo->g_gain = g_gain + 128;
    p_rgbogo->b_gain = b_gain + 128;
    p_rgbogo->r_post_offset = r_offset;
    p_rgbogo->g_post_offset = g_offset;
    p_rgbogo->b_post_offset = b_offset;

    return ret;
}

#define PI 3.14159265358979
void CVpp::video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab)
{
    signed short ma = (signed short) (cos((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);
    signed short mb = (signed short) (sin((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);

    if (ma > 511) {
        ma = 511;
    }

    if (ma < -512) {
        ma = -512;
    }

    if (mb > 511) {
        mb = 511;
    }

    if (mb < -512) {
        mb = -512;
    }

    *mab = ((ma & 0x3ff) << 16) | (mb & 0x3ff);
}

void CVpp::video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab)
{
    signed long temp = *mab;
    signed int ma = (signed int) ((temp << 6) >> 22);
    signed int mb = (signed int) ((temp << 22) >> 22);
    signed int sat16 = (signed int) ((sqrt(
                                          ((float) ma * (float) ma + (float) mb * (float) mb) / 65536.0) - 1.0) * 128.0);
    signed int hue16 = (signed int) (atan((float) mb / (float) ma) * 128.0 / PI);

    if (sat16 > 127) {
        sat16 = 127;
    }

    if (sat16 < -128) {
        sat16 = -128;
    }

    if (hue16 > 127) {
        hue16 = 127;
    }

    if (hue16 < -128) {
        hue16 = -128;
    }

    *sat = (signed char) sat16;
    *hue = (signed char) hue16;
}

int CVpp::VPP_SetVideoSaturationHue(int satVal, int hueVal)
{
    FILE *fp = NULL;
    signed long temp;

    fp = fopen("/sys/class/amvecm/saturation_hue", "w");
    LOGD("~~~fopen~~~##VPP_SetVideoSaturationHue##%s : %d %d##",
         "/sys/class/amvecm/saturation_hue", satVal, hueVal);

    if (fp == NULL) {
        LOGE("Open /sys/class/amvecm/saturation_hue error(%s)!\n", strerror(errno));
        return -1;
    }

    video_set_saturation_hue(satVal, hueVal, &temp);
    fprintf(fp, "0x%lx", temp);

    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_SetVideoSaturation(int saturation)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/amvecm/saturation_hue", "w");
    LOGD("~~~fopen~~~##VPP_SetVideoSaturation##%s : %d ##", "/sys/class/amvecm/saturation_hue",
         saturation);

    if (fp == NULL) {
        LOGE("Open /sys/class/amvecm/saturation_hue error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "0x%x", saturation);

    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_SetVideoHue(int hue)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/amvecm/saturation_hue", "w");
    LOGD("~~~fopen~~~##VPP_SetVideoHue##%s : %d ##", "/sys/class/amvecm/saturation_hue", hue);

    if (fp == NULL) {
        LOGE("Open /sys/class/amvecm/saturation_hue error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "0x%x", hue);

    fclose(fp);
    fp = NULL;
    return 0;
}

int CVpp::VPP_SetGammaTbl_R(unsigned short red[256])
{
    struct tcon_gamma_table_s Redtbl;
    int rt = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Redtbl.data[i] = red[i];
    }

    rt = VPP_DeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_R, &Redtbl);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetGammaTbl_R##AMVECM_IOC_GAMMA_TABLE_R##");

    if (rt < 0) {
        LOGE("Vpp_api_SetGammaTbl_R, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetGammaTbl_G(unsigned short green[256])
{
    struct tcon_gamma_table_s Greentbl;
    int rt = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Greentbl.data[i] = green[i];
    }

    rt = VPP_DeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_G, &Greentbl);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetGammaTbl_G##AMVECM_IOC_GAMMA_TABLE_G##");

    if (rt < 0) {
        LOGE("Vpp_api_SetGammaTbl_R, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetGammaTbl_B(unsigned short blue[256])
{
    struct tcon_gamma_table_s Bluetbl;
    int rt = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Bluetbl.data[i] = blue[i];
    }

    rt = VPP_DeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_B, &Bluetbl);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetGammaTbl_B##AMVECM_IOC_GAMMA_TABLE_B##");

    if (rt < 0) {
        LOGE("Vpp_api_SetGammaTbl_R, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetGammaOnOff(unsigned char onoff)
{
    int rt = -1;

    if (onoff == 1) {
        rt = VPP_DeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_EN);
        LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetGammaOnOff##AMVECM_IOC_GAMMA_TABLE_EN##");
    }

    if (onoff == 0) {
        rt = VPP_DeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_DIS);
        LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetGammaOnOff##AMVECM_IOC_GAMMA_TABLE_DIS##");
    }

    if (rt < 0) {
        LOGE("Vpp_api_SetGammaOnOff, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetGrayPattern(int value)
{

    FILE *fp = NULL;
    if (value < 0) {
        value = 0;
    } else if (value > 255) {
        value = 255;
    }
    value = value << 16 | 0x8080;

    fp = fopen("/sys/class/video/test_screen", "w");
    LOGD("~~~fopen~~~##VPP_SetGrayPattern##%s : %x ##", "/sys/class/video/test_screen", value);

    if (fp == NULL) {
        LOGE("Open /sys/classs/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "0x%x", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_GetGrayPattern()
{
    FILE *fp = NULL;
    int value;
    fp = fopen("/sys/class/video/test_screen", "r+");

    LOGD("~~~fopen~~~##VPP_GetGrayPattern##%s  ##", "/sys/class/video/test_screen");

    if (fp == NULL) {
        LOGE("Open /sys/class/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    fscanf(fp, "%d", &value);
    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        value = value >> 16;
        if (value > 255) {
            value = 255;
        }
        return value;
    }

}

int CVpp::VPP_SplitScreenEffect(int width, int v_register)
{
    FILE *fp = fopen(SYS_DROILOGIC_DEBUG, "w");

    if (fp == NULL) {
        LOGE("Open %s ERROR(%s)!!\n",SYS_DROILOGIC_DEBUG, strerror(errno));
        return -1;
    }
    LOGD("width = %x----v_register = %x", width, v_register);
    fprintf(fp, "w %x v %x", width, v_register);
    fclose(fp);
    fp = NULL;

    return 0;
}
int CVpp::VPP_SetVideoNoiseReduction(int value)
{
    FILE *fp = NULL;

    fp = fopen("/sys/class/deinterlace/di0/parameters", "w");
    LOGD("~~~fopen~~~##VPP_SetVideoNoiseReduction##%s : %d ##",
         "/sys/class/deinterlace/di0/parameters", value);

    if (fp == NULL) {
        LOGE("Open /sys/class/deinterlace/di0/parameters ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "noise_reduction_level=%x", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::VPP_SetDeinterlaceMode(int value)
{
    FILE *fp = NULL;

    fp = fopen("/sys/module/deinterlace/parameters/deinterlace_mode", "w");
    LOGD("~~~fopen~~~##VPP_SetDeinterlaceMode##%s : %d ##",
         "/sys/module/deinterlace/parameters/deinterlace_mode", value);

    if (fp == NULL) {
        LOGE("Open /sys/module/deinterlace/parameters/deinterlace_mode error(%s)!\n",
             strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);

    fclose(fp);
    fp = NULL;

    return 0;
}

int CVpp::GetHistogram_AVE(void)
{
    ve_hist_t hist;
    hist.sum = 0;
    hist.height = 0;
    hist.width = 0;
    hist.ave = 0;

    if (Vpp_GetAVGHistogram(&hist) == 0) {
        LOGD("%s: %d.\n", __FUNCTION__, hist.ave);
    } else {
        LOGE("%s failed.\n", __FUNCTION__);
    }

    return hist.ave;
}

int CVpp::Vpp_GetAVGHistogram(struct ve_hist_s *hist)
{
    //int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_G_HIST_AVG, hist);
    int rt = VPP_DeviceIOCtl(AMVECM_IOC_G_HIST_AVG, hist);
    LOGD("~~~VPP_DeviceIOCtl~~~##Vpp_GetAVGHistogram##AMVECM_IOC_G_HIST_AVG##");

    if (rt < 0) {
        LOGE("Vpp_GetAVGHistogram, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVEBlackExtension(const struct ve_bext_s *pBExt)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_BEXT, pBExt);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEBlackExtension##AMSTREAM_IOC_VE_BEXT##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEBlackExtension, error(%s)!\n", strerror(errno));
    }

    return rt;
}

tvin_cutwin_t CVpp::GetOverscan(tv_source_input_type_t source_type, tvin_sig_fmt_t fmt,
                                is_3d_type_t is3d, tvin_trans_fmt_t trans_fmt)
{
    int ret = -1;
    char tmp_buf[16];
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    if (trans_fmt < TVIN_TFMT_2D || trans_fmt > TVIN_TFMT_3D_LDGD) {
        return cutwin_t;
    }

    vpp_display_mode_t scrmode = GetDisplayMode(source_type);
    ret = mpPqData->PQ_GetOverscanParams(source_type, fmt, is3d, trans_fmt, scrmode, &cutwin_t);

    return cutwin_t;
}

int CVpp::VPP_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
    int fd = -1;
    char set_str[32];

    fd = open("/sys/class/video/crop", O_RDWR);

    LOGD("~~~open~~~##VPP_SetVideoCrop##%s : %d %d %d %d##", "/sys/class/video/crop", Voffset0,
         Hoffset0, Voffset1, Hoffset1);

    if (fd < 0) {
        LOGE("Open /sys/class/video/crop error(%s)!\n", strerror(errno));
        return -1;
    }

    memset(set_str, 0, 32);
    sprintf(set_str, "%d %d %d %d", Voffset0, Hoffset0, Voffset1, Hoffset1);
    write(fd, set_str, strlen(set_str));
    close(fd);

    return 0;
}

int CVpp::VPP_SetVESharpness(const struct ve_hsvs_s *pHSVS)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_HSVS, pHSVS);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVESharpness##AMSTREAM_IOC_VE_HSVS##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVESharpness, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVEChromaCoring(const struct ve_ccor_s *pCCor)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_CCOR, pCCor);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEChromaCoring##AMSTREAM_IOC_VE_CCOR##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEChromaCoring, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVEBlueEnh(const struct ve_benh_s *pBEnh)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_BENH, pBEnh);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEBlueEnh##AMSTREAM_IOC_VE_BENH##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEBlueEnh, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVEDemo(const struct ve_demo_s *pDemo)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_DEMO, pDemo);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEDemo##AMSTREAM_IOC_VE_DEMO##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEDemo, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVERegisterMap(const struct ve_regmap_s *pRegMap)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_REGMAP, pRegMap);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVERegisterMap##AMSTREAM_IOC_VE_REGMAP##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVERegisterMap, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetVEDebug(const unsigned long long *pLData)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_VE_DEBUG, pLData);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetVEDebug##AMSTREAM_IOC_VE_DEBUG##");

    if (rt < 0) {
        LOGE("Vpp_api_SetVEDebug, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetCMRegion(const struct cm_region_s *pRegion)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_CM_REGION, pRegion);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetCMRegion##AMSTREAM_IOC_CM_REGION##");

    if (rt < 0) {
        LOGE("Vpp_api_SetCMRegion, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetCMTopLayer(const struct cm_top_s *pTop)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_CM_TOP, pTop);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetCMTopLayer##AMSTREAM_IOC_CM_TOP##");

    if (rt < 0) {
        LOGE("Vpp_api_SetCMTopLayer, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetCMDemo(const struct cm_demo_s *pDemo)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_CM_DEMO, pDemo);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetCMDemo##AMSTREAM_IOC_CM_DEMO##");

    if (rt < 0) {
        LOGE("Vpp_api_SetCMDemo, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetCMRegisterMap(struct cm_regmap_s *pRegMap)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_CM_REGMAP, pRegMap);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetCMRegisterMap##AMSTREAM_IOC_CM_REGMAP##");

    if (rt < 0) {
        LOGE("Vpp_api_SetCMRegisterMap, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetCMDebug(const unsigned long long *pLData)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_CM_DEBUG, pLData);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetCMDebug##AMSTREAM_IOC_CM_DEBUG##");

    if (rt < 0) {
        LOGE("=VPP CPP=> set cm debug, error (%s)", strerror(errno));
    }

    return rt;
}

int CVpp::VPP_SetAVSyncEnable(const unsigned int enable)
{
    int rt = VPP_DeviceIOCtl(AMSTREAM_IOC_SYNCENABLE, enable);
    LOGD("~~~VPP_DeviceIOCtl~~~##VPP_SetAVSyncEnable##AMSTREAM_IOC_SYNCENABLE##");

    if (rt < 0) {
        LOGE("Vpp_api_SetAVSyncEnable, error(%s)!\n", strerror(errno));
    }

    return rt;
}
int CVpp::VPP_SetScalerPathSel(const unsigned int value)
{
    FILE *fp = NULL;

    fp = fopen(SYS_VIDEO_SCALER_PATH_SEL, "w");
    LOGD("VPP_SetScalerPathSel %s : %d", SYS_VIDEO_SCALER_PATH_SEL, value);
    if (fp == NULL) {
        LOGE("Open %s error(%s)!\n",SYS_VIDEO_SCALER_PATH_SEL, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

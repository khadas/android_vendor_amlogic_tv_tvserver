#define LOG_TAG "CFrontEnd"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <time.h>
#include "am_misc.h"
#include "am_debug.h"
#include "CFrontEnd.h"
#include "util.h"
#include "tvin/CTvin.h"
extern "C" {
#include "am_av.h"
#include "am_dmx.h"
#include "linux/videodev2.h"
#include "am_fend_ctrl.h"
}

CFrontEnd::CFrontEnd()
{
    mFrontDevID = FE_DEV_ID;
    mpObserver = NULL;
    mCurFineFreq = 0;
    mCurMode =  FE_ANALOG;
    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
    mbFEOpened = false;
}

CFrontEnd::~CFrontEnd()
{
    AM_EVT_Unsubscribe(mFrontDevID, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, NULL);
    if (mFrontDevID == FE_DEV_ID)
        AM_FEND_Close(mFrontDevID);
    mFrontDevID = -1;
}

int CFrontEnd::Open(int mode)
{
    AM_FEND_OpenPara_t para;
    int rc = 0;

    if (mbFEOpened) {
        LOGD("FrontEnd have opened, return");
        return 0;
    }
    mbFEOpened = true;
    memset(&para, 0, sizeof(AM_FEND_OpenPara_t));
    para.mode = mode;
    rc = AM_FEND_Open(mFrontDevID, &para);
    if ((rc == AM_FEND_ERR_BUSY) || (rc == 0)) {
        AM_EVT_Subscribe(mFrontDevID, AM_FEND_EVT_STATUS_CHANGED, dmd_fend_callback, this);
        LOGD("%s,frontend dev open success!\n", __FUNCTION__);
        return 0;
    } else {
        LOGD("%s,frontend dev open failed! dvb error id is %d\n", __FUNCTION__, rc);
        return -1;
    }
    mCurMode =  mode;
    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
}

int CFrontEnd::Close()
{
    int rc = 0;
    if (!mbFEOpened) {
        LOGD("FrontEnd have close, but not return");
    }
    rc = AM_FEND_Close(mFrontDevID);
    mbFEOpened = false;
    mCurMode =  -1;
    mCurFreq = -1;
    mCurPara1 = -1;
    mCurPara2 = -1;
    if (rc != 0) {
        LOGD("%s,frontend_close fail! dvb error id is %d\n", __FUNCTION__, rc);
        return -1;
    } else {
        LOGD("%s,close frontend is ok\n", __FUNCTION__);
    }
    return 0;
}

int CFrontEnd::setMode(int mode)
{
    int rc = 0;
    if (mCurMode == mode) return 0;
    rc = AM_FEND_SetMode(mFrontDevID, mode);

    if (rc != 0) {
        LOGD("%s,front dev set mode failed! dvb error id is %d\n", __FUNCTION__, rc);
        return -1;
    }
    return 0;
}

int CFrontEnd::setPara(int mode, int freq, int para1, int para2)
{
    int ret = 0;
    int buff_size = 32;
    char VideoStdBuff[buff_size];
    char audioStdBuff[buff_size];
    if (mCurMode == mode && mCurFreq == freq && mCurPara1 == para1 && mCurPara2 == para2) {
        LOGD("fe setpara  is same return");
        return 0;
    }
    mCurMode = mode;
    mCurFreq = freq;
    mCurPara1 = para1;
    mCurPara2 = para2;

    AM_FENDCTRL_DVBFrontendParameters_t dvbfepara;
    memset(&dvbfepara, 0, sizeof(AM_FENDCTRL_DVBFrontendParameters_t));
    LOGD("%s,set fe para mode = %d freq=%d p1=%d p2=%d", __FUNCTION__, mode, freq, para1, para2);
    dvbfepara.m_type = mode;
    switch (mode) {
    case FE_OFDM:
        dvbfepara.terrestrial.para.frequency = freq;
        dvbfepara.terrestrial.para.u.ofdm.bandwidth = (fe_bandwidth_t)para1;
        break;
    case FE_DTMB:
        LOGD("%s,FE_DTMB is support\n", __FUNCTION__);
        LOGD("%s,freq = %d, bandwidth = %d\n", __FUNCTION__, freq, para1);
        dvbfepara.dtmb.para.frequency = freq;
        dvbfepara.dtmb.para.u.ofdm.bandwidth = (fe_bandwidth_t)para1;
        break;
    case FE_ATSC:
        dvbfepara.atsc.para.frequency = freq;
        dvbfepara.atsc.para.u.vsb.modulation = (fe_modulation_t)para1;
        break;
    case FE_QAM:
        dvbfepara.cable.para.frequency = freq;
        dvbfepara.cable.para.u.qam.symbol_rate = para1;
        dvbfepara.cable.para.u.qam.modulation  = (fe_modulation_t)para2;
        break;
    case FE_ANALOG:
        LOGD("%s,FE_ANALOG is support\n", __FUNCTION__);
        /*para2 is finetune data */
        dvbfepara.analog.para.frequency = freq;
        dvbfepara.analog.para.u.analog.std   =   para1;
        dvbfepara.analog.para.u.analog.afc_range = AFC_RANGE;
        if (para2 == 0) {
            dvbfepara.analog.para.u.analog.flag |= ANALOG_FLAG_ENABLE_AFC;
        } else {
            dvbfepara.analog.para.u.analog.flag &= ~ANALOG_FLAG_ENABLE_AFC;
            dvbfepara.analog.para.u.analog.afc_range = 0;
        }

        printAudioStdStr(para1, audioStdBuff, buff_size);
        printVideoStdStr(para1, VideoStdBuff, buff_size);
        LOGD("%s,freq = %dHz, video_std = %s, audio_std = %s, fineFreqOffset = %dHz\n", __FUNCTION__,
             freq, VideoStdBuff, audioStdBuff, para2);

        break;
    }

    ret = AM_FENDCTRL_SetPara(mFrontDevID, &dvbfepara);

    if (ret != 0) {
        LOGD("%s,fend set para failed! dvb error id is %d\n", __FUNCTION__, ret);
        return -1;
    }
    return 0;
}

int CFrontEnd::fineTune(int fineFreq)
{
    int ret = 0;
    if (mCurFineFreq == fineFreq) return -1;

    mCurFineFreq = fineFreq;
    ret = AM_FEND_FineTune(FE_DEV_ID, fineFreq);

    if (ret != 0) {
        LOGD("%s, fail! dvb error id is %d", __FUNCTION__, ret);
        return -1;
    }
    return 0;
}

int CFrontEnd::GetTSSource(AM_DMX_Source_t *src)
{
    AM_FEND_GetTSSource(mFrontDevID, src);
    return 0;
}

void CFrontEnd::dmd_fend_callback(long dev_no __unused, int event_type __unused, void *param, void *user_data)
{
    CFrontEnd *pFront = (CFrontEnd *)user_data;
    if (NULL == pFront) {
        LOGD("%s,warnning : dmd_fend_callback NULL == pFront\n", __FUNCTION__);
        return ;
    }
    if (pFront->mpObserver == NULL) {
        LOGD("%s,warnning : mpObserver NULL == mpObserver\n", __FUNCTION__);
        return;
    }
    struct dvb_frontend_event *evt = (struct dvb_frontend_event *) param;
    if (!evt)
        return;

    if (evt->status &  FE_HAS_LOCK) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_HAS_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    } else if (evt->status & FE_TIMEDOUT) {
        pFront->mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_NO_SIG;
        pFront->mCurSigEv.mCurFreq = evt->parameters.frequency;
        pFront->mpObserver->onEvent(pFront->mCurSigEv);
    }
}

int CFrontEnd::stdAndColorToAudioEnum(int data)
{
    atv_audio_std_t std = CC_ATV_AUDIO_STD_DK;
    if (((data & V4L2_STD_PAL_DK) == V4L2_STD_PAL_DK) ||
            ((data & V4L2_STD_SECAM_DK) == V4L2_STD_SECAM_DK)) {
        std =  CC_ATV_AUDIO_STD_DK;
    } else if ((data & V4L2_STD_PAL_I) == V4L2_STD_PAL_I) {
        std =  CC_ATV_AUDIO_STD_I;
    } else if (((data & V4L2_STD_PAL_BG) == V4L2_STD_PAL_BG) ||
               ((data & V4L2_STD_SECAM_B) == V4L2_STD_SECAM_B) ||
               ((data & V4L2_STD_SECAM_G) == V4L2_STD_SECAM_G )) {
        std = CC_ATV_AUDIO_STD_BG;
    } else if (((data & V4L2_STD_PAL_M) == V4L2_STD_PAL_M) ||
               ((data & V4L2_STD_NTSC_M) == V4L2_STD_NTSC_M)) {
        std = CC_ATV_AUDIO_STD_M;
    } else if ((data & V4L2_STD_SECAM_L) == V4L2_STD_SECAM_L) {
        std = CC_ATV_AUDIO_STD_L;
    }
    return  std ;
}

int CFrontEnd::stdAndColorToVideoEnum(int std)
{
    atv_video_std_t video_standard = CC_ATV_VIDEO_STD_PAL;
    if ((std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL) {
        video_standard = CC_ATV_VIDEO_STD_PAL;
    } else if ((std & V4L2_COLOR_STD_NTSC) == V4L2_COLOR_STD_NTSC) {
        video_standard = CC_ATV_VIDEO_STD_NTSC;
    } else if ((std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM) {
        video_standard = CC_ATV_VIDEO_STD_SECAM;
    }
    return video_standard;
}

bool CFrontEnd::stdIsColorAuto(int std)
{
    return  ((std & V4L2_COLOR_STD_AUTO) == V4L2_COLOR_STD_AUTO) ? true : false;
}

int CFrontEnd::addColorAutoFlag(int std)
{
    return std | V4L2_COLOR_STD_AUTO;
}
v4l2_std_id CFrontEnd::enumToStdAndColor(int videoStd, int audioStd)
{
    v4l2_std_id tmpTunerStd = 0;
    if (videoStd == CC_ATV_VIDEO_STD_PAL) {
        tmpTunerStd |= V4L2_COLOR_STD_PAL;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_PAL_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_NTSC || videoStd == CC_ATV_VIDEO_STD_AUTO) {
        tmpTunerStd |= V4L2_COLOR_STD_NTSC;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_SECAM) {
        tmpTunerStd |= V4L2_COLOR_STD_SECAM;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_SECAM_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= (V4L2_STD_SECAM_B | V4L2_STD_SECAM_G);
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        } else if (audioStd == CC_ATV_AUDIO_STD_L) {
            tmpTunerStd |= V4L2_STD_SECAM_L;
        }

    }
    return tmpTunerStd;
}

int CFrontEnd::getPara(int *mode, int *freq, int *para1, int *para2)
{

    struct dvb_frontend_parameters para;
    memset(&para, 0, sizeof(struct dvb_frontend_parameters));
    /*if((ret1=AM_FEND_GetPara(mFrontDevID, &para))<0) {
        LOGD("%s,getPara  faiture\n", __FUNCTION__ );
        return -1;

        //fpara->mode = fend_mode ;

    }*/
    *mode = mCurMode;
    *freq = mCurFreq;
    *para1 = mCurPara1;
    *para2 = mCurPara2;
    return 0;
}

int CFrontEnd::getSNR()
{
    int snr = 0;
    AM_FEND_GetSNR(mFrontDevID, &snr);
    return snr;
}

int CFrontEnd::getBER()
{
    int ber = 0;
    AM_FEND_GetBER(mFrontDevID, &ber);
    return ber;
}

int CFrontEnd::getStrength()
{
    int Strength = 0;
    AM_FEND_GetStrength(mFrontDevID, &Strength);
    return Strength;
}

int CFrontEnd::formatATVFreq(int tmp_freq)
{
    const int ATV_1MHZ = 1000000;
    const int ATV_50KHZ = 50 * 1000;
    const int ATV_25KHZ = 25 * 1000;
    int mastar = (tmp_freq / ATV_50KHZ) * ATV_50KHZ;
    int follow = tmp_freq % ATV_50KHZ;
    if (follow >= ATV_25KHZ) {
        follow = ATV_50KHZ;
    } else {
        follow = 0;
    }
    return mastar + follow;
}

int CFrontEnd::printVideoStdStr(int compStd, char strBuffer[], int buff_size)
{
    memset(strBuffer, 0, buff_size);
    int videoStd = stdAndColorToVideoEnum(compStd);
    switch (videoStd) {
    case CC_ATV_VIDEO_STD_AUTO:
        strcpy(strBuffer, "AUTO");
        break;
    case CC_ATV_VIDEO_STD_PAL:
        strcpy(strBuffer, "PAL");
        break;
    case CC_ATV_VIDEO_STD_NTSC:
        strcpy(strBuffer, "NTSC");
        break;
    case CC_ATV_VIDEO_STD_SECAM:
        strcpy(strBuffer, "SECAM");
        break;
    default:
        strcpy(strBuffer, "UnkownVideo");
        break;
    }

    return 0;
}

int CFrontEnd::printAudioStdStr(int compStd, char strBuffer[], int buff_size)
{
    memset(strBuffer, 0, buff_size);
    int audioStd = stdAndColorToAudioEnum(compStd);
    switch (audioStd) {
    case CC_ATV_AUDIO_STD_DK:
        strcpy(strBuffer, "DK");
        break;
    case CC_ATV_AUDIO_STD_I:
        strcpy(strBuffer, "I");
        break;
    case CC_ATV_AUDIO_STD_BG:
        strcpy(strBuffer, "BG");
        break;
    case CC_ATV_AUDIO_STD_M:
        strcpy(strBuffer, "M");
        break;
    case CC_ATV_AUDIO_STD_L:
        strcpy(strBuffer, "L");
        break;
    case CC_ATV_AUDIO_STD_AUTO:
        strcpy(strBuffer, "AUTO");
        break;
    default:
        strcpy(strBuffer, "UnkownAudio");
        break;
    }

    return 0;
}

int CFrontEnd::autoLoadFE()
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/class/amlfe/setting", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/amlfe/setting error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = fprintf ( fp, "%s", "autoload" );

    if ( ret < 0 ) {
        LOGW ( "autoload FE error(%s)!\n", strerror ( errno ) );
    }
    LOGD("autoLoadFE");
    fclose ( fp );
    fp = NULL;

    return ret;
}

int CFrontEnd::setCvbsAmpOut(int tmp)
{
    int rc = AM_SUCCESS;
    rc = AM_FEND_SetCvbsAmpOut(mFrontDevID, tmp);
    if (rc == AM_SUCCESS) {
        LOGD("%s, sucessful!", __FUNCTION__);
        rc =  0;
    } else {
        LOGD("%s, fail!", __FUNCTION__);
        rc =  -1;
    }
    return rc;
}

int CFrontEnd::setThreadDelay(int delay)
{
    int rc = AM_SUCCESS;

    rc = AM_FEND_SetThreadDelay(mFrontDevID, delay);
    if (rc == AM_SUCCESS) {
        LOGD("frontend_setThreadDelay sucessful!\n");
        return 0;
    } else {
        LOGD("frontend_setThreadDelay fail! %d\n\n", rc);
        return -1;
    }
}

int CFrontEnd::lock(int frequency, int symbol_rate, int modulation, int bandwidth __unused)
{
    int rt = -1;
    struct dvb_frontend_parameters fparam;
    struct dvb_frontend_info finfo;

    memset(&fparam, 0, sizeof(struct dvb_frontend_parameters));
    memset(&finfo, 0, sizeof(struct dvb_frontend_info));

    AM_FEND_GetInfo(mFrontDevID, &finfo);

    if (frequency == 0)
        return -1;
    fparam.frequency = frequency;

    switch (finfo.type) {

    case FE_QAM:
    default:

        if (symbol_rate == 0)
            return -1;
        fparam.u.qam.symbol_rate = symbol_rate;


        if (modulation == 0)
            return -1;
        fparam.u.qam.modulation = (fe_modulation_t)modulation;

        LOGD("mFrontDevID = %d;fre=%d;sym=%d;qam=%d ",
             mFrontDevID, fparam.frequency, fparam.u.qam.symbol_rate, fparam.u.qam.modulation);
        break;
    case FE_OFDM:
    case FE_ATSC:
        LOGD("mFrontDevID = %d;fre=%d;bw=%d\n ",
             mFrontDevID, fparam.frequency, fparam.u.ofdm.bandwidth);
        break;
    case FE_QPSK:
        LOGD("QPSK are not supported.\n ");
        break;
    case FE_ANALOG:
        LOGD("ANALOG is supported.\n ");
        fparam.u.analog.std =   V4L2_STD_PAL_I;
        break;
    }

    fe_status_t status;

    rt =  AM_FEND_Lock(FE_DEV_ID, &fparam, &status);

    if ((!rt) && (status & FE_HAS_LOCK)) {
        LOGD("frontend_lock sucessful!\n");
        return true;
    } else {
        LOGD("frontend_lock fail %d!\n", rt);
        return false;
    }
}

int CFrontEnd::getInfo()
{
    struct dvb_frontend_info finfo;

    AM_FEND_GetInfo(mFrontDevID, &finfo);

    //return fend_info;  noitfy FrontEnd message
    return 0;
}

int CFrontEnd::setTunerAfc(int afc)
{
    AM_FEND_SetAfc(FE_DEV_ID, afc);
    return 0;
}

int CFrontEnd::getStatus()
{
    fe_status_t status;
    AM_FEND_GetStatus(mFrontDevID, &status);

    return status;
}
int CFrontEnd::checkStatusOnce()
{
    fe_status_t status;
    AM_FEND_GetStatus(mFrontDevID, &status);
    LOGD("%s,get status = %x", __FUNCTION__, status);
    if (status &  FE_HAS_LOCK) {
        mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_HAS_SIG;
        mCurSigEv.mCurFreq = 0;
        mpObserver->onEvent(mCurSigEv);
    } else if (status & FE_TIMEDOUT) {
        mCurSigEv.mCurSigStaus = FEEvent::EVENT_FE_NO_SIG;
        mCurSigEv.mCurFreq = 0;
        mpObserver->onEvent(mCurSigEv);
    }
    return 0;
}

int CFrontEnd::stdEnumToCvbsFmt (int videoStd, int audioStd)
{
    tvin_sig_fmt_e cvbs_fmt = TVIN_SIG_FMT_NULL;

    if ( videoStd == CC_ATV_VIDEO_STD_PAL ) {
        cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
        if ( audioStd == CC_ATV_AUDIO_STD_DK ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
        } else if ( audioStd == CC_ATV_AUDIO_STD_I ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
        } else if ( audioStd == CC_ATV_AUDIO_STD_BG ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
        } else if ( audioStd == CC_ATV_AUDIO_STD_M ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_M;
        }
    } else if ( videoStd == CC_ATV_VIDEO_STD_NTSC ) {
        cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
        if ( audioStd == CC_ATV_AUDIO_STD_DK ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
        } else if ( audioStd == CC_ATV_AUDIO_STD_I ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
        } else if ( audioStd == CC_ATV_AUDIO_STD_BG ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
        } else if ( audioStd == CC_ATV_AUDIO_STD_M ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
        }
    } else if ( videoStd == CC_ATV_VIDEO_STD_SECAM ) {
        if ( audioStd == CC_ATV_AUDIO_STD_DK ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
        } else if ( audioStd == CC_ATV_AUDIO_STD_I ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
        } else if ( audioStd == CC_ATV_AUDIO_STD_BG ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
        } else if ( audioStd == CC_ATV_AUDIO_STD_M ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
        } else if ( audioStd == CC_ATV_AUDIO_STD_L ) {
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
        }
    }
    return cvbs_fmt;
}

int CFrontEnd::ClearAnalogFrontEnd()
{
    return this->setPara ( FE_ANALOG, 44250000, V4L2_COLOR_STD_PAL | V4L2_STD_PAL_DK, 0 );
}

/*to control afc function*/
int CFrontEnd::SetAnalogFrontEndTimerSwitch(int onOff)
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/module/aml_fe/parameters/aml_timer_en", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/aml_fe/parameters/aml_timer_en error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    if (onOff)
        ret = fprintf(fp, "%s", "1");
    else
        ret = fprintf(fp, "%s", "0");
    if ( ret < 0 ) {
        LOGW ( "set aml_timer_en error(%s)!\n", strerror ( errno ) );
    }
    LOGD("SetAnalogFrontEndTimerSwitch %d", onOff);
    fclose ( fp );
    fp = NULL;

    return ret;
}

int CFrontEnd::SetAnalogFrontEndSearhSlowMode(int onOff)
{
    FILE *fp = NULL;
    int ret = -1;

    fp = fopen ( "/sys/module/aml_fe/parameters/slow_mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/aml_fe/parameters/slow_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    if (onOff)
        ret = fprintf(fp, "%s", "1");
    else
        ret = fprintf(fp, "%s", "0");
    if ( ret < 0 ) {
        LOGW ( "set aml_timer_en error(%s)!\n", strerror ( errno ) );
    }
    LOGD("SetAnalogFrontEndSearhSlowMode %d", onOff);
    fclose ( fp );
    fp = NULL;

    return ret;
}

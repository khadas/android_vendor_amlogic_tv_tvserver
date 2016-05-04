#define LOG_TAG "tvserver"

#include "CAv.h"
#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <amstream.h>

CAv::CAv()
{
    mpObserver = NULL;
    mTvPlayDevId = 0;
    mVideoLayerState = VIDEO_LAYER_NONE;
    mFdAmVideo = -1;
}

CAv::~CAv()
{
}

int CAv::SetVideoWindow(int x, int y, int w, int h)
{
    return AM_AV_SetVideoWindow (mTvPlayDevId, x, y, w, h );
}

int CAv::Open()
{
    AM_AV_OpenPara_t para_av;
    memset ( &para_av, 0, sizeof ( AM_AV_OpenPara_t ) );
    int rt = AM_AV_Open ( mTvPlayDevId, &para_av );
    if ( rt != AM_SUCCESS ) {
        LOGD ( "%s, dvbplayer_open fail %d %d\n!" , __FUNCTION__,  mTvPlayDevId, rt );
        return -1;
    }

    //open audio channle output
    AM_AOUT_OpenPara_t aout_para;
    memset ( &aout_para, 0, sizeof ( AM_AOUT_OpenPara_t ) );
    rt = AM_AOUT_Open ( mTvPlayDevId, &aout_para );
    if ( AM_SUCCESS != rt ) {
        LOGD ( "%s,  BUG: CANN'T OPEN AOUT\n", __FUNCTION__);
    }

    mFdAmVideo = open ( PATH_VIDEO_AMVIDEO, O_RDWR );
    if ( mFdAmVideo < 0 ) {
        LOGE ( "mFdAmVideo < 0, error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    /*Register events*/
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_NO_DATA, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AV_DATA_RESUME, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_AUDIO_SCAMBLED, av_evt_callback, this );
    AM_EVT_Subscribe ( mTvPlayDevId, AM_AV_EVT_VIDEO_NOT_SUPPORT, av_evt_callback, this );

    return rt;
}

int CAv::Close()
{
    int iRet;
    iRet = AM_AV_Close ( mTvPlayDevId );
    iRet = AM_AOUT_Close ( mTvPlayDevId );
    if (mFdAmVideo > 0) {
        close(mFdAmVideo);
        mFdAmVideo = -1;
    }
    return iRet;
}

int CAv::GetVideoStatus(AM_AV_VideoStatus_t *status)
{
    return AM_AV_GetVideoStatus(mTvPlayDevId, status);
}

int CAv::SwitchTSAudio(int apid, AM_AV_AFormat_t afmt)
{
    return AM_AV_SwitchTSAudio (mTvPlayDevId, ( uint16_t ) apid, ( AM_AV_AFormat_t ) afmt );
}

int CAv::ResetAudioDecoder()
{
    return AM_AV_ResetAudioDecoder ( mTvPlayDevId );
}

int CAv::SetTSSource(AM_AV_TSSource_t ts_source)
{
    return AM_AV_SetTSSource ( mTvPlayDevId, ts_source );
}

int CAv::StartTS(uint16_t vpid, uint16_t apid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt)
{
    return AM_AV_StartTS ( mTvPlayDevId, vpid, apid, ( AM_AV_VFormat_t ) vfmt, ( AM_AV_AFormat_t ) afmt );
}

int CAv::StopTS()
{
    return AM_AV_StopTS (mTvPlayDevId);
}

int CAv::AudioGetOutputMode(AM_AOUT_OutputMode_t *mode)
{
    return AM_AOUT_GetOutputMode ( mTvPlayDevId, mode );
}

int CAv::AudioSetOutputMode(AM_AOUT_OutputMode_t mode)
{
    return AM_AOUT_SetOutputMode ( mTvPlayDevId, mode );
}

int CAv::EnableVideoBlackout()
{
    return AM_AV_EnableVideoBlackout(mTvPlayDevId);
}

int CAv::DisableVideoBlackout()
{
    return AM_AV_DisableVideoBlackout(mTvPlayDevId);
}

int CAv::DisableVideoWithBlueColor()
{
    LOGD("DisableVideoWithBlueColor");
    if (mVideoLayerState == VIDEO_LAYER_DISABLE_BLUE) {
        LOGD("video is disable with blue, return");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_DISABLE_BLUE;
    SetVideoScreenColor ( 0, 41, 240, 110 ); // Show blue with vdin0, postblending disabled
    return AM_AV_DisableVideo(mTvPlayDevId);
}

int CAv::DisableVideoWithBlackColor()
{
    LOGD("DisableVideoWithBlackColor");
    if (mVideoLayerState == VIDEO_LAYER_DISABLE_BLACK) {
        LOGD("video is disable with black, return");
        return 0;
    }

    mVideoLayerState = VIDEO_LAYER_DISABLE_BLACK;
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show black with vdin0, postblending disabled
    return AM_AV_DisableVideo(mTvPlayDevId);
}

//auto enable,
int CAv::EnableVideoAuto()
{
    LOGD("EnableVideoAuto");
    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enable");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show black with vdin0, postblending disabled
    ClearVideoBuffer();//disable video 2
    return 0;
}

//just enable video
int CAv::EnableVideoNow()
{
    LOGD("EnableVideoNow");

    if (mVideoLayerState == VIDEO_LAYER_ENABLE) {
        LOGW("video is enabled");
        return 0;
    }
    mVideoLayerState = VIDEO_LAYER_ENABLE;
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if (strcmp ( value, "black" )) { //don't black
        SetVideoScreenColor ( 0, 16, 128, 128 ); // Show blue with vdin0, postblending disabled
    }
    return AM_AV_EnableVideo(mTvPlayDevId);
}

int CAv::WaittingVideoPlaying(int minFrameCount , int waitTime )
{
    static const int COUNT_FOR_TIME = 20;
    int times = waitTime / COUNT_FOR_TIME;

    for (int i = 0; i < times; i++) {
        if (videoIsPlaying(minFrameCount)) {
            return 0;
        }
    }

    LOGW("EnableVideoWhenVideoPlaying time out");
    return -1;
}

int CAv::EnableVideoWhenVideoPlaying(int minFrameCount, int waitTime)
{
    int ret = WaittingVideoPlaying(minFrameCount, waitTime);
    if (ret == 0) { //ok to playing
        EnableVideoNow();
    }
    return ret;
}

bool CAv::videoIsPlaying(int minFrameCount)
{
    int value[2] = {0};
    value[0] = getVideoFrameCount();
    usleep(20 * 1000);
    value[1] = getVideoFrameCount();
    LOGD("videoIsPlaying framecount =%d = %d", value[0], value[1]);

    if (value[1] >= minFrameCount && (value[1] > value[0]))
        return true;

    return false;
}

int CAv::getVideoFrameCount()
{
    char buf[32] = {0};

    tvReadSysfs(PATH_FRAME_COUNT, buf);
    return atoi(buf);
}

tvin_sig_fmt_t CAv::getVideoResolutionToFmt()
{
    char buf[32] = {0};
    tvin_sig_fmt_e sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;

    tvReadSysfs(SYS_VIDEO_FRAME_HEIGHT, buf);
    int height = atoi(buf);
    LOGD("getVideoResolutionToFmt height = %d", height);
    if (height <= 576) {
        sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
    } else if (height > 576 && height <= 1088) {
        sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    } else {
        sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
    }
    return sig_fmt;
}

//call disable video 2
int CAv::ClearVideoBuffer()
{
    LOGD("ClearVideoBuffer");
    return AM_AV_ClearVideoBuffer(mTvPlayDevId);
}

int CAv::SetVideoScreenColor ( int vdin_blending_mask, int y, int u, int v )
{
    unsigned long value = vdin_blending_mask << 24;
    value |= ( unsigned int ) ( y << 16 ) | ( unsigned int ) ( u << 8 ) | ( unsigned int ) ( v );

    LOGD("%s, vdin_blending_mask:%d,y:%d,u:%d,v:%d", __FUNCTION__, vdin_blending_mask, y, u, v);

    char val[64] = {0};
    sprintf(val, "0x%lx", ( unsigned long ) value);
    tvWriteSysfs(VIDEO_TEST_SCREEN, val);
    return 0;
}

int CAv::SetVideoLayerDisable ( int value )
{
    LOGD("%s, value = %d" , __FUNCTION__, value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_DISABLE_VIDEO, val);
    return 0;
}

int CAv::setVideoScreenMode ( int value )
{
    LOGD("setVideoScreenMode, value = %d" , value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    tvWriteSysfs(VIDEO_SCREEN_MODE, val);
    return 0;
}

int CAv::setVideoAxis ( int h, int v, int width, int height )
{
    LOGD("%s, %d %d %d %d", __FUNCTION__, h, v, width, height);

    char value[64] = {0};
    sprintf(value, "%d %d %d %d", h, v, width, height);
    tvWriteSysfs(VIDEO_AXIS, value);
    return 0;
}

video_display_resolution_t CAv::getVideoDisplayResolution()
{
    video_display_resolution_t  resolution;
    char attrV[SYS_STR_LEN] = {0};

    tvReadSysfs(VIDEO_DEVICE_RESOLUTION, attrV);

    if (strncasecmp(attrV, "1366x768", strlen ("1366x768")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1366X768;
    } else if (strncasecmp(attrV, "3840x2160", strlen ("3840x2160")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_3840X2160;
    } else if (strncasecmp(attrV, "1920x1080", strlen ("1920x1080")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    } else {
        LOGW("video display resolution is = (%s) not define , default it", attrV);
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    }

    return resolution;
}

void CAv::av_evt_callback ( long dev_no, int event_type, void *param, void *user_data )
{
    CAv *pAv = ( CAv * ) user_data;
    if (NULL == pAv ) {
        LOGD ( "%s, ERROR : av_evt_callback NULL == pTv\n", __FUNCTION__ );
        return ;
    }
    if ( pAv->mpObserver == NULL ) {
        LOGD ( "%s, ERROR : mpObserver NULL == mpObserver\n", __FUNCTION__ );
        return;
    }
    switch ( event_type ) {
    case AM_AV_EVT_AV_NO_DATA:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_STOP;
        pAv->mCurAvEvent.param = ( int )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_AV_DATA_RESUME:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_RESUEM;
        pAv->mCurAvEvent.param = ( int )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_SCAMBLED:
    case AM_AV_EVT_AUDIO_SCAMBLED:
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_SCAMBLED;
        pAv->mCurAvEvent.param = ( int )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    case AM_AV_EVT_VIDEO_NOT_SUPPORT: {
        pAv->mCurAvEvent.type = AVEvent::EVENT_AV_UNSUPPORT;
        pAv->mCurAvEvent.param = ( int )param;
        pAv->mpObserver->onEvent(pAv->mCurAvEvent);
        break;
    }
    default:
        break;
    }
    LOGD ( "%s, av_evt_callback : dev_no %ld type %d param = %d\n",
        __FUNCTION__, dev_no, pAv->mCurAvEvent.type , (int)param);
}

int CAv::setLookupPtsForDtmb(int enable)
{
    LOGD ( "setLookupPtsForDtmb %d" , enable);

    char value[64] = {0};
    sprintf(value, "%d", enable);
    tvWriteSysfs(PATH_MEPG_DTMB_LOOKUP_PTS_FLAG, value);
    return 0;
}


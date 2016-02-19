#define LOG_TAG "CAv"

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
    mCurVideoLayerMuteState = -1;
    mCurDisableVideoColor = -1;
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
    rt = AM_AOUT_Open ( 0, &aout_para );

    if ( AM_SUCCESS != rt ) {
        LOGD ( "%s,  BUG: CANN'T OPEN AOUT\n", __FUNCTION__);
    }
    //
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
    return  AM_AV_SetTSSource ( mTvPlayDevId, ts_source );
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
    return AM_AOUT_GetOutputMode ( 0, mode );
}

int CAv::AudioSetOutputMode(AM_AOUT_OutputMode_t mode)
{
    return AM_AOUT_SetOutputMode ( 0, mode );
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
    if (mCurVideoLayerMuteState == 1 && mCurDisableVideoColor == DISABLE_VIDEO_COLOR_BLUE) {
        LOGD("video is disable with blue, return");
        return 0;
    }
    mCurVideoLayerMuteState = 1;
    mCurDisableVideoColor = DISABLE_VIDEO_COLOR_BLUE;
    SetVideoScreenColor ( 0, 41, 240, 110 ); // Show blue with vdin0, postblending disabled
    return AM_AV_DisableVideo(mTvPlayDevId);
}

int CAv::DisableVideoWithBlackColor()
{
    LOGD("DisableVideoWithBlackColor");
    if (mCurVideoLayerMuteState == 1 && mCurDisableVideoColor == DISABLE_VIDEO_COLOR_BLACK) {
        LOGD("video is disable with black, return");
        return 0;
    }
    mCurDisableVideoColor = DISABLE_VIDEO_COLOR_BLACK;
    mCurVideoLayerMuteState = 1;
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show blue with vdin0, postblending disabled
    return AM_AV_DisableVideo(mTvPlayDevId);
}
//auto enable,
int CAv::EnableVideoAuto()
{
    LOGD("EnableVideo");
    if (mCurVideoLayerMuteState == 0) {
        LOGD("video is enable, return");
        return 0;
    }
    mCurVideoLayerMuteState = 0;
    SetVideoScreenColor ( 0, 16, 128, 128 ); // Show black with vdin0, postblending disabled
    ClearVideoBuffer();//disable video 2
    return 0;
}

//just enable video
int CAv::EnableVideoNow()
{
    LOGD("EnableVideoNow");
    const char *config_value = NULL;
    if (mCurVideoLayerMuteState == 0) {
        LOGD("video is enable, return");
        return 0;
    }
    mCurVideoLayerMuteState = 0;
    config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( config_value, "black" ) == 0 ) {
    } else {
        SetVideoScreenColor ( 0, 16, 128, 128 ); // Show blue with vdin0, postblending disabled
    }
    return AM_AV_EnableVideo(mTvPlayDevId);
}

int CAv::WaittingVideoPlaying(int minFrameCount , int waitTime )
{
    //EnableVideoNow();
    static const int COUNT_FOR_TIME = 20;
    int times = waitTime / COUNT_FOR_TIME;
    int ret = -1;
    int i = 0;
    for (i = 0; i < times; i++) {
        if (videoIsPlaying(minFrameCount)) {
            ret = 0;
            break;
        }
    }
    if (i == times) {
        LOGD("EnableVideoWhenVideoPlaying time out!!!!!!!!!!!!!");
        ret =  -2;
    }
    return ret;
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
    int value[3] = {0};
    value[0] = getVideoFrameCount();
    usleep(20 * 1000);
    value[1] = getVideoFrameCount();
    //usleep(20*1000);
    //value[2] = getVideoFrameCount();
    LOGD("---videoIsPlaying framecount =%d = %d = %d", value[0], value[1], value[2]);
    if (value[1] >=  minFrameCount && (value[1] > value[0])) return true;
    else return false;
}

int CAv::getVideoFrameCount()
{
    char buf[32];
    int fd = -1;
    fd = open(PATH_FRAME_COUNT, O_RDWR);
    if (fd < 0) {
        LOGW("Open %s error(%s)!\n", PATH_FRAME_COUNT, strerror(errno));
        return -1;
    }
    read(fd, buf, sizeof(buf));
    int value = 0;
    sscanf ( buf, "%d", &value );
    close ( fd );
    return value;
}

tvin_sig_fmt_t CAv::getVideoResolutionToFmt()
{
    tvin_sig_fmt_e sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    int height = CFile::getFileAttrValue(SYS_VIDEO_FRAME_HEIGHT);
    LOGD("---------getVideoResolutionToFmt -------- height = %d", height);
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
    FILE *fp = NULL;
    unsigned long value = 0;

    value = vdin_blending_mask << 24;

    value |= ( unsigned int ) ( y << 16 ) | ( unsigned int ) ( u << 8 ) | ( unsigned int ) ( v );

    fp = fopen ( "/sys/class/video/test_screen", "w" );
    LOGD ( "~~~fopen~~~##VPP_SetVideoScreenColor##%s : vdin_blending_mask:%d,y:%d,u:%d,v:%d ##" , "/sys/class/video/test_screen", vdin_blending_mask, y, u, v);


    if ( fp == NULL ) {
        LOGE ( "Open /sys/class/video/test_screen error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "0x%lx", ( unsigned long ) value );

    fclose ( fp );
    fp = NULL;

    return 0;
}

int CAv::getVideoDisableValue()
{
    LOGD("this fun is empty!!!!!!!!");
    return 0;
}

int CAv::SetVideoLayerDisable ( int value )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/class/video/disable_video", "w" );
    LOGD ( "~~~fopen~~~##VPP_SetVideoLayerDisable##%s : %d ##" , "/sys/class/video/disable_video", value);

    if ( fp == NULL ) {
        LOGE ( "Open /sys/class/video/disable_video error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", value );

    fclose ( fp );
    fp = NULL;

    return 0;
}


int CAv::setVideoScreenMode ( int value )
{
    FILE *fp = fopen ( "/sys/class/video/screen_mode", "w" );
    LOGD ( "setVideoScreenMode %d ##" , value);
    if ( fp == NULL ) {
        LOGE ( "Open /sys/class/video/screen_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d", value );
    fclose ( fp );
    return 0;
}

int CAv::setVideoAxis ( int h, int v, int width, int height )
{
    FILE *fp = NULL;
    fp = fopen ( "/sys/class/video/axis", "w" );
    LOGD ( "setVideoAxis##%s : %d %d %d %d ##" , "/sys/class/video/axis", h, v, width, height);

    if ( fp == NULL ) {
        LOGE ( "Open /sys/class/video/axis ERROR(%s)!!\n", strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d %d %d %d", h, v, width, height );
    fclose ( fp );
    return 0;
}

int CAv::getVideoScreenMode()
{
    int value;
    FILE *fp = fopen ( "/sys/class/video/screen_mode", "r+" );

    if ( fp == NULL ) {
        LOGE ( "Open /sys/class/video/screen_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    fscanf ( fp, "%d", &value );
    fclose ( fp );
    return value;
}

video_display_resolution_t CAv::getVideoDisplayResolution()
{
    char attrV[64];
    memset (attrV, 0x0, 64);
    Tv_Utils_GetFileAttrStr ( "/sys/class/video/device_resolution", 64, attrV );
    video_display_resolution_t  resolution;
    if (strncasecmp(attrV, "1366x768", strlen ("1366x768")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1366X768;
    } else if (strncasecmp(attrV, "3840x2160", strlen ("3840x2160")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_3840X2160;
    } else if (strncasecmp(attrV, "1920x1080", strlen ("1920x1080")) == 0) {
        resolution = VPP_DISPLAY_RESOLUTION_1920X1080;
    } else {
        LOGW("video display resolution  is = (%s) not define , default it", attrV);
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

int CAv::set3DMode(VIDEO_3D_MODE_T mode, int LR_switch, int mode_3D_TO_2D)
{
    unsigned int cmd = MODE_3D_DISABLE;
    switch (mode) {
    case VIDEO_3D_MODE_DISABLE:
        cmd = MODE_3D_DISABLE;
        break;
    case VIDEO_3D_MODE_AUTO:
        cmd = MODE_3D_ENABLE | MODE_3D_AUTO;
        break;
    case VIDEO_3D_MODE_LR:
        cmd = MODE_3D_ENABLE | MODE_3D_LR;
        break;
    case VIDEO_3D_MODE_TB:
        cmd = MODE_3D_ENABLE | MODE_3D_TB;
        break;
    case VIDEO_3D_MODE_LA:
        cmd = MODE_3D_ENABLE | MODE_3D_LA;
        break;
    case VIDEO_3D_MODE_FA:
        cmd = MODE_3D_ENABLE | MODE_3D_FA;
        break;
    default:
        cmd = MODE_3D_DISABLE;
        break;
    }

    if (LR_switch == 1) {
        cmd = cmd | MODE_3D_LR_SWITCH;
    }

    if (mode_3D_TO_2D == 1) {
        cmd = cmd | MODE_3D_TO_2D_L;
    }

    if (mode_3D_TO_2D == 2) {
        cmd = cmd | MODE_3D_TO_2D_R;
    }
    LOGD("set 3d mode fd = %d cmd = 0x%x", mFdAmVideo, cmd);
    int ret = ioctl(mFdAmVideo, AMSTREAM_IOC_SET_3D_TYPE , cmd);
    if (ret < 0) {
        LOGE("set3DMode error ( %s )", strerror ( errno ));
    }
    return 0;
}

int CAv::setLookupPtsForDtmb(int enable)
{
    FILE *fp = fopen ( PATH_MEPG_DTMB_LOOKUP_PTS_FLAG, "w" );
    LOGD ( "setLookupPtsForDtmb %d ##" , enable);
    if ( fp == NULL ) {
        LOGE ( "Open %s error(%s)!\n", PATH_MEPG_DTMB_LOOKUP_PTS_FLAG, strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d", enable );
    fclose ( fp );
    return 0;
}

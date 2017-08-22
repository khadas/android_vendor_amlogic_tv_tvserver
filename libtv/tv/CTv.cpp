/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTv"
#define UNUSED(x) (void)x

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <am_epg.h>
#include <am_mem.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include <utils/threads.h>
#include <time.h>
#include <sys/prctl.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef ANDROID
#include <termios.h>
#endif
#include <string.h>
#include <signal.h>
#include <hardware_legacy/power.h>
#include <media/AudioSystem.h>
#include <sys/wait.h>


#include <tvutils.h>
#include <tvconfig.h>
#include <CFile.h>
#include <serial_operate.h>
#include <CFbcHelper.h>

#include "CTvDatabase.h"
#include "../version/version.h"
#include "../tvsetting/CTvSetting.h"

#include <hardware_legacy/power.h>

extern "C" {
#include "am_ver.h"
#include "am_misc.h"
#include "am_debug.h"
#include "am_fend.h"
}

#include <math.h>
#include <sys/ioctl.h>

#include "CTv.h"

using namespace android;

static const int WALL_EFFECT_VALUE[CC_BAND_ITEM_CNT] = { 0, 0, 1, 2, 2, 0 };


// Called each time a message is logged.
static void sqliteLogCallback(void *data, int iErrCode, const char *zMsg)
{
	//TODO
    UNUSED(data);
    LOGD( "showbo sqlite (%d) %s\n", iErrCode, zMsg);
}

bool CTv::insertedFbcDevice()
{
    bool ret = false;

    if (hdmiOutWithFbc()) {
        CFbcCommunication *fbc = GetSingletonFBC();
        char panel_model[64] = {0};

        if (fbc == NULL) {
            LOGD ("%s, there is no fbc!!!", __func__);
        }
        else {
            fbc->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);

            if (0 == panel_model[0]) {
                LOGD ("%s, device is not fbc", __func__);
            }
            else {
                LOGD ("%s, get panel info from fbc is %s", __func__, panel_model);
                ret = true;
            }
        }
    }

    return ret;
}

CTv::CTv():mTvMsgQueue(this), mTvScannerDetectObserver(this)
{
    mAudioMuteStatusForTv = CC_AUDIO_UNMUTE;
    mAudioMuteStatusForSystem = CC_AUDIO_UNMUTE;

    //copy file to param
    char buf[PROPERTY_VALUE_MAX] = {0};
    int len = property_get("tv.tvconfig.force_copy", buf, "true");

    if (isFileExist(TV_CONFIG_FILE_SYSTEM_PATH)) {
        if (!isFileExist(TV_CONFIG_FILE_PARAM_PATH) || !strcmp(buf, "true")) {
            CFile file ( TV_CONFIG_FILE_SYSTEM_PATH );

            if ( file.copyTo ( TV_CONFIG_FILE_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CONFIG_FILE_PARAM_PATH );
            }
            else {
                LOGD ("%s copy from %s to %s success.", __func__, TV_CONFIG_FILE_SYSTEM_PATH, TV_CONFIG_FILE_PARAM_PATH);
            }
        }
    }

    if (isFileExist(TV_CHANNEL_LIST_SYSTEM_PATH)) {
        if (!isFileExist(TV_CHANNEL_LIST_PARAM_PATH)) {
            CFile file ( TV_CHANNEL_LIST_SYSTEM_PATH );

            if ( file.copyTo ( TV_CHANNEL_LIST_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CHANNEL_LIST_PARAM_PATH );
            }
        }
    }

    AM_EVT_Init();

    mpObserver = NULL;
    fbcIns = NULL;
    mAutoSetDisplayFreq = false;
    mPreviewEnabled = false;

    mFrontDev = CFrontEnd::getInstance();
    mpTvin = CTvin::getInstance();
    mTvScanner = CTvScanner::getInstance();
    tv_config_load (TV_CONFIG_FILE_PATH);
    sqlite3_config (SQLITE_CONFIG_SERIALIZED);
    //sqlite3_config(SQLITE_CONFIG_LOG, &sqliteLogCallback, (void*)1);
    sqlite3_soft_heap_limit(8 * 1024 * 1024);
    // Initialize SQLite.
    sqlite3_initialize();
    CTvDatabase::GetTvDb()->InitTvDb ( TV_DB_PATH );

    if ( CTvDimension::isDimensionTblExist() == false ) {
        CTvDimension::builtinAtscDimensions();
    }

    CTvSettingLoad();

    mFactoryMode.init();
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;

    mHdmiOutFbc = insertedFbcDevice();

    m_sig_stable_nums = 0;
    mSetHdmiEdid = false;

    mBootvideoStatusDetectThread = new CBootvideoStatusDetect();
    mBootvideoStatusDetectThread->setObserver(this);

    m_hdmi_audio_data = 0;
    mSigDetectThread.setObserver ( this );
    mSourceConnectDetectThread.setObserver ( this );
    mFrontDev->setObserver ( &mTvMsgQueue );
    if (mHdmiOutFbc) {
        fbcIns = GetSingletonFBC();
        setUpgradeFbcObserver(this);
        fbcIns->cfbc_Set_DNLP(COMM_DEV_SERIAL, 0);//disable dnlp for fbc project.
    }
    mSubtitle.setObserver(this);
    mHeadSet.setObserver(this);

    mAv.setObserver(&mTvMsgQueue);
    mMainVolLutTableExtraName[0] = '\0';
    //-----------------------------------------------------------
    mCurAudioMasterVolume = CC_DEF_SOUND_VOL;
    mCurAudioBalance = CC_DEF_SOUND_BALANCE_VAL;
    mCurAudioSupperBassVolume = CC_DEF_SUPPERBASS_VOL;
    mCurAudioSupperBassSwitch = CC_SWITCH_OFF;
    mCurAudioSRSSurround = CC_SWITCH_OFF;
    mCurAudioSrsDialogClarity = CC_SWITCH_OFF;
    mCurAudioSrsTruBass = CC_SWITCH_OFF;
    mCurAudioSPDIFSwitch = CC_SWITCH_ON;
    mCurAudioSPDIFMode = CC_SPDIF_MODE_PCM;
    mCurAudioBassVolume = CC_DEF_BASS_TREBLE_VOL;
    mCurAudioTrebleVolume = CC_DEF_BASS_TREBLE_VOL;
    mCurAudioSoundMode = CC_SOUND_MODE_END;
    mCurAudioWallEffect = CC_SWITCH_OFF;
    mCurAudioEQMode = CC_EQ_MODE_START;
    mCustomAudioMasterVolume = CC_DEF_SOUND_VOL;
    mCustomAudioBalance = CC_DEF_SOUND_BALANCE_VAL;
    mCustomAudioSupperBassVolume = CC_DEF_SUPPERBASS_VOL;
    mCustomAudioSupperBassSwitch = CC_SWITCH_OFF;
    mCustomAudioSRSSurround = CC_SWITCH_OFF;
    mCustomAudioSrsDialogClarity = CC_SWITCH_OFF;
    mCustomAudioSrsTruBass = CC_SWITCH_OFF;
    mCustomAudioBassVolume = CC_DEF_BASS_TREBLE_VOL;
    mCustomAudioTrebleVolume = CC_DEF_BASS_TREBLE_VOL;
    mCustomAudioSoundMode = CC_SOUND_MODE_END;
    mCustomAudioWallEffect = CC_SWITCH_OFF;
    mCustomAudioEQMode = CC_EQ_MODE_START;
    mCustomAudioSoundEnhancementSwitch = CC_SWITCH_OFF;
    mVolumeCompensationVal = 0;

    mMainVolumeBalanceVal = 0;
    for (int i = 0; i < CC_BAND_ITEM_CNT; i++) {
        mCustomEQGainBuf[i] = 0;
        mCurEQGainBuf[i] = 0;
        mCurEQGainChBuf[i] = 0;
    }

    mTvAction &= TV_ACTION_NULL;
    mTvStatus = TV_INIT_ED;
    pGpio = new CTvGpio();
    print_version_info();
}

CTv::~CTv()
{
    mpObserver = NULL;
    CTvSettingunLoad();
    CTvDatabase::deleteTvDb();
    tv_config_unload();
    mAv.Close();
    mTvStatus = TV_INIT_ED;
    mFrontDev->Close();

    if (fbcIns != NULL) {
        fbcIns->fbcRelease();
        delete fbcIns;
        fbcIns = NULL;
    }

    if (pGpio != NULL) {
        delete pGpio;
        pGpio = NULL;
    }
}

void CTv::onEvent ( const CTvScanner::ScannerEvent &ev )
{
    LOGD ( "%s, CTv::onEvent lockStatus = %d type = %d\n", __FUNCTION__,  ev.mLockedStatus, ev.mType );

    if ( mDtvScanRunningStatus == DTV_SCAN_RUNNING_ANALYZE_CHANNEL ) {
        if ( ev.mType == CTvScanner::ScannerEvent::EVENT_SCAN_END ) {
            CMessage msg;
            msg.mType = CTvMsgQueue::TV_MSG_STOP_ANALYZE_TS;
            msg.mpData = this;
            mTvMsgQueue.sendMsg ( msg );
        } else if ( ev.mType == CTvScanner::ScannerEvent::EVENT_STORE_END ) {
            CTvEpg::EpgEvent epgev;
            epgev.type = CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE_END;
            mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
            sendTvEvent ( epgev );
        }
    } else {
        sendTvEvent ( ev );
    }
}

void CTv::onEvent ( const CFrontEnd::FEEvent &ev )
{
    const char *config_value = NULL;
    LOGD ( "[source_switch_time]: %fs, %s, FE event type = %d tvaction=%x", getUptimeSeconds(), __FUNCTION__, ev.mCurSigStaus, mTvAction);
    if (mTvAction & TV_ACTION_SCANNING) return;

    //前端事件响应处理
    if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_HAS_SIG ) { //作为信号稳定
        LOGD("[source_switch_time]: %fs, fe lock", getUptimeSeconds());
        if (/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
            TvEvent::SignalInfoEvent ev;
            ev.mStatus = TVIN_SIG_STATUS_STABLE;
            ev.mTrans_fmt = TVIN_TFMT_2D;
            ev.mFmt = TVIN_SIG_FMT_NULL;
            ev.mReserved = 0;
            sendTvEvent ( ev );
        }
    } else if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_NO_SIG ) { //作为信号消失
        LOGD("[source_switch_time]: %fs, fe unlock", getUptimeSeconds());
        if ((!(mTvAction & TV_ACTION_STOPING)) && m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //just playing
            if ( iSBlackPattern ) {
                mAv.DisableVideoWithBlackColor();
            } else {
                mAv.DisableVideoWithBlueColor();
            }
            SetAudioMuteForTv ( CC_AUDIO_MUTE );

            TvEvent::SignalInfoEvent ev;
            ev.mStatus = TVIN_SIG_STATUS_NOSIG;
            ev.mTrans_fmt = TVIN_TFMT_2D;
            ev.mFmt = TVIN_SIG_FMT_NULL;
            ev.mReserved = 0;
            sendTvEvent ( ev );
        }
    }
}

void CTv::onEvent ( const CTvEpg::EpgEvent &ev )
{
    switch ( ev.type ) {
    case CTvEpg::EpgEvent::EVENT_TDT_END:
        LOGD ( "%s, CTv::onEvent epg time = %ld", __FUNCTION__, ev.time );
        mTvTime.setTime ( ev.time );
        break;

    case CTvEpg::EpgEvent::EVENT_CHANNEL_UPDATE: {
        LOGD ( "%s, CTv:: onEvent channel update", __FUNCTION__ );
        CMessage msg;
        msg.mType = CTvMsgQueue::TV_MSG_START_ANALYZE_TS;
        msg.mpData = this;
        mCurAnalyzeTsChannelID = ev.channelID;
        mTvMsgQueue.sendMsg ( msg );
        break;
    }

    default:
        break;
    }

    sendTvEvent ( ev );
}

void CTv::onEvent(const CAv::AVEvent &ev)
{
    LOGD ( "[source_switch_time]: %fs, AVEvent = %d", getUptimeSeconds(), ev.type);
    switch ( ev.type ) {
    case CAv::AVEvent::EVENT_AV_STOP: {
        if ( mTvAction & TV_ACTION_STOPING ) {
            LOGD("tv stopping, no need CAv::AVEvent::EVENT_AV_STOP");
            break;
        }
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        SetAudioMuteForTv ( CC_AUDIO_MUTE );

        TvEvent::SignalInfoEvent ev;
        ev.mStatus = TVIN_SIG_STATUS_NOSIG;
        ev.mTrans_fmt = TVIN_TFMT_2D;
        ev.mFmt = TVIN_SIG_FMT_NULL;
        ev.mReserved = 0;
        sendTvEvent ( ev );
        break;
    }

    case CAv::AVEvent::EVENT_AV_RESUEM: {
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_PLAYBACK_RESUME;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }

    case CAv::AVEvent::EVENT_AV_SCAMBLED: {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_SCAMBLED;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }
    case CAv::AVEvent::EVENT_AV_VIDEO_AVAILABLE: {
        LOGD("[source_switch_time]: %fs, EVENT_AV_VIDEO_AVAILABLE, video available", getUptimeSeconds());
        if (m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
            if ( m_win_mode == PREVIEW_WONDOW ) {
                mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
            }
            if (mAutoSetDisplayFreq && !mPreviewEnabled) {
                mpTvin->VDIN_SetDisplayVFreq(50, mHdmiOutFbc);
            }
            SetDisplayMode(CVpp::getInstance()->GetDisplayMode(m_source_input),
                m_source_input,
                mAv.getVideoResolutionToFmt());
            usleep(50 * 1000);
            mAv.EnableVideoNow(true);
            LOGD("[source_switch_time]: %fs, EVENT_AV_VIDEO_AVAILABLE, video available ok", getUptimeSeconds());
            SetAudioMuteForTv(CC_AUDIO_UNMUTE);
            Tv_SetAudioInSource(SOURCE_DTV);
        }
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_VIDEO_AVAILABLE;
        AvPlayBackEvt.mProgramId = (int)ev.param;
        sendTvEvent(AvPlayBackEvt);
        break;
    }
    case CAv::AVEvent::EVENT_AV_UNSUPPORT: {
        LOGD("To AVS or AVS+ format");//just avs format,  not unsupport, and avs avs+
        break;
    }

    default:
        break;
    }
}

CTv::CTvMsgQueue::CTvMsgQueue(CTv *tv)
{
    mpTv = tv;
}

CTv::CTvMsgQueue::~CTvMsgQueue()
{
}

void CTv::CTvMsgQueue::handleMessage ( CMessage &msg )
{
    LOGD ("%s, CTv::CTvMsgQueue::handleMessage type = %d", __FUNCTION__,  msg.mType);

    switch ( msg.mType ) {
    case TV_MSG_COMMON:
        break;

    case TV_MSG_STOP_ANALYZE_TS:
        //mpTv->Tv_Stop_Analyze_Ts();
        break;

    case TV_MSG_START_ANALYZE_TS:
        //mpTv->Tv_Start_Analyze_Ts ( pTv->mCurAnalyzeTsChannelID );
        break;

    case TV_MSG_CHECK_FE_DELAY: {
        mpTv->mFrontDev->checkStatusOnce();
        break;
    }

    case TV_MSG_AV_EVENT: {
        mpTv->onEvent(*((CAv::AVEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_FE_EVENT: {
        mpTv->onEvent(*((CFrontEnd::FEEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_SCAN_EVENT: {
        mpTv->onEvent(*((CTvScanner::ScannerEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_EPG_EVENT: {
        mpTv->onEvent(*((CTvEpg::EpgEvent *)(msg.mpPara)));
        break;
    }

    case TV_MSG_HDMI_SR_CHANGED: {
        int sr = ((int *)(msg.mpPara))[0];
        mpTv->onHdmiSrChanged(sr, (((int *)(msg.mpPara))[1] == 0) ? true : false);
        mpTv->setAudioPreGain(mpTv->m_source_input);
        break;
    }

    case TV_MSG_ENABLE_VIDEO_LATER: {
        int fc = msg.mpPara[0];
        mpTv->onEnableVideoLater(fc);
        break;
    }

    case TV_MSG_VIDEO_AVAILABLE_LATER: {
        int fc = msg.mpPara[0];
        mpTv->onVideoAvailableLater(fc);
        break;
    }

    default:
        break;
    }
}

void CTv::CTvMsgQueue::onEvent ( const CTvScanner::ScannerEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_SCAN_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CFrontEnd::FEEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_FE_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvEpg::EpgEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_EPG_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent(const CAv::AVEvent &ev)
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_AV_EVENT;
    memcpy(msg.mpPara, (void*)&ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::onHdmiSrChanged(int sr, bool bInit)
{
    if (bInit) {
        LOGD ( "%s, Init HDMI audio, sampling rate:%d", __FUNCTION__,  sr );
        sr = HanldeAudioInputSr(sr);
        InitTvAudio (sr, CC_IN_USE_SPDIF_DEVICE);
    } else {
        LOGD ( "%s, Reset HDMI sampling rate to %d", __FUNCTION__,  sr );
        AudioChangeSampleRate ( sr );
    }
}

void CTv::onHMDIAudioStatusChanged(int status)
{
    if (status == 0) {//change to no audio data
        SetAudioMuteForTv ( CC_AUDIO_MUTE );
    } else if (status == 1) {//change to audio data come
        Tv_SetAudioInSource(m_source_input);
        usleep(200 * 1000);
        SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
    }
}

int CTv::setTvObserver ( TvIObserver *ob )
{
    mpObserver = ob;
    return 0;
}

void CTv::sendTvEvent ( const CTvEv &ev )
{
    /* send sigstate to AutoBackLight */
    if (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT) {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
        if (pEv->mStatus == TVIN_SIG_STATUS_STABLE) {
            mAutoBackLight.updateSigState(mAutoBackLight.SIG_STATE_STABLE);
        } else {
            mAutoBackLight.updateSigState(mAutoBackLight.SIG_STATE_NOSIG);
        }
    }

    LOGD ( "%s, tvaction=%x", __FUNCTION__, mTvAction);
    if ((mTvAction & TV_ACTION_SCANNING) && (ev.getEvType() == CTvEv::TV_EVENT_SIGLE_DETECT)) {
        LOGD("%s, when scanning, not send sig detect event", __FUNCTION__);
        return;
    }
    if ( mpObserver != NULL ) {
        mpObserver->onTvEvent ( ev );
    }
}

int CTv::ClearAnalogFrontEnd()
{
    return mFrontDev->ClearAnalogFrontEnd ();
}

int CTv::clearFrontEnd(int para)
{
    return mFrontDev->setPara(para, 470000000, 0, 0);
}

int CTv::Scan(const char *feparas, const char *scanparas) {
    AutoMutex _l(mLock);
    m_source_input = SOURCE_INVALID;
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    mTvAction = mTvAction | TV_ACTION_SCANNING;
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
    LOGD("fe[%s], scan[%s] %s", feparas, scanparas, __FUNCTION__);

    mSigDetectThread.requestAndWaitPauseDetect();
    mAv.StopTS();
    mpTvin->Tvin_StopDecoder();
    if ( iSBlackPattern ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }

    CTvScanner::ScanParas sp(scanparas);
    CFrontEnd::FEParas fp(feparas);

    if (sp.getAtvMode() & AM_SCAN_ATVMODE_AUTO)
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    if (sp.getDtvMode() & AM_SCAN_DTVMODE_MANUAL) {
        CTvChannel::DeleteBetweenFreq(sp.getDtvFrequency1(), sp.getDtvFrequency2());
    } else {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
        CTvEvent::CleanAllEvent();
    }

    mTvScanner->setObserver(&mTvMsgQueue);
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;

    mFrontDev->Open(FE_AUTO);

    return mTvScanner->Scan(fp, sp);
}

int CTv::dtvScan(int mode, int scan_mode, int beginFreq, int endFreq, int para1, int para2) {
    AutoMutex _l(mLock);
    mTvAction = mTvAction | TV_ACTION_SCANNING;
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
    LOGD("mode[%#x], scan_mode[%#x], freq[%d-%d], para[%d,%d] %s",
        mode, scan_mode, beginFreq, endFreq, para1, para2, __FUNCTION__);
    //mTvEpg.leaveChannel();
    mAv.StopTS();

    if ( iSBlackPattern ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }

    if (scan_mode == AM_SCAN_DTVMODE_MANUAL) {
        CTvChannel::DeleteBetweenFreq(beginFreq, endFreq);
    } else {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
        CTvEvent::CleanAllEvent();
    }
    mTvScanner->setObserver(&mTvMsgQueue);
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;

    return mTvScanner->dtvScan(mode, scan_mode, beginFreq, endFreq, para1, para2);
}

int CTv::dtvMode(const char *mode)
{
    if ( strcmp ( mode, DTV_DTMB_MODE ) == 0 )
        return (FE_DTMB);
    else if ( strcmp ( mode, DTV_DVBC_MODE ) == 0 )
        return (FE_QAM);
    else if ( strcmp ( mode, DTV_DVBS_MODE ) == 0 )
        return (FE_QPSK);
    else if ( strcmp ( mode, DTV_ATSC_MODE ) == 0 )
        return (FE_ATSC);

    return (FE_DTMB);
}

int CTv::dtvAutoScan()
{
    const char *dtv_mode = config_get_str ( CFG_SECTION_TV, CFG_DTV_MODE, DTV_DTMB_MODE);
    return dtvScan(dtvMode(dtv_mode), AM_SCAN_DTVMODE_ALLBAND, 0, 0, -1, -1);
}

int CTv::dtvCleanProgramByFreq ( int freq )
{
    int iOutRet = 0;
    CTvChannel channel;
    iOutRet = CTvChannel::SelectByFreq ( freq, channel );
    if ( -1 == iOutRet ) {
        LOGD("%s, Warnning: Current Freq not have program info in the ts_table\n", __FUNCTION__);
    }

    iOutRet == CTvProgram::deleteChannelsProgram ( channel );
    return iOutRet;
}

int CTv::dtvManualScan (int beginFreq, int endFreq, int modulation)
{
    const char *dtv_mode = config_get_str ( CFG_SECTION_TV, CFG_DTV_MODE, "dtmb");
    return dtvScan(dtvMode(dtv_mode), AM_SCAN_DTVMODE_MANUAL, beginFreq, endFreq, modulation, -1);

}

//searchType 0:not 256 1:is 256 Program
int CTv::atvAutoScan(int videoStd __unused, int audioStd __unused, int searchType, int procMode)
{
    int minScanFreq, maxScanFreq, vStd, aStd;
    AutoMutex _l( mLock );
    if ( !mATVDisplaySnow )
        mAv.DisableVideoWithBlueColor();
    mTvAction |= TV_ACTION_SCANNING;
    stopPlaying(false);
    mTvScanner->setObserver ( &mTvMsgQueue );
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    getATVMinMaxFreq (&minScanFreq, &maxScanFreq );
    if ( minScanFreq == 0 || maxScanFreq == 0 || minScanFreq > maxScanFreq ) {
        LOGE ( "%s, auto scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
        return -1;
    }
    mSigDetectThread.setVdinNoSigCheckKeepTimes(1000, false);
    mSigDetectThread.requestAndWaitPauseDetect();
    mSigDetectThread.setObserver(&mTvScannerDetectObserver);
    if ( !mATVDisplaySnow ) {
        mpTvin->Tvin_StopDecoder();
    } else {
        mpTvin->SwitchSnow( true );
    }

    vStd = CC_ATV_VIDEO_STD_PAL;
    aStd = CC_ATV_AUDIO_STD_DK;
    tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    mpTvin->VDIN_OpenPort ( source_port );
    v4l2_std_id stdAndColor = mFrontDev->enumToStdAndColor(vStd, aStd);

    int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );

    if (searchType == 0) {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    } else if (searchType == 1) { //type for skyworth, and insert 256 prog, and just update ts table
    }
    minScanFreq = mFrontDev->formatATVFreq ( minScanFreq );
    maxScanFreq = mFrontDev->formatATVFreq ( maxScanFreq );
    LOGD("%s, atv auto scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);

    mSigDetectThread.initSigState();
    mSigDetectThread.resumeDetect();
    mTvScanner->autoAtvScan ( minScanFreq, maxScanFreq, stdAndColor, searchType, procMode);
    return 0;
}

int CTv::atvAutoScan(int videoStd __unused, int audioStd __unused, int searchType)
{
    return atvAutoScan(videoStd, audioStd, searchType, 0);
}

int CTv::clearAllProgram(int arg0 __unused)
{
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_TV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO);
    return 0;
}

int CTv::atvMunualScan ( int startFreq, int endFreq, int videoStd, int audioStd,
                         int store_Type , int channel_num )
{
    int minScanFreq, maxScanFreq, vStd, aStd;

    minScanFreq = mFrontDev->formatATVFreq ( startFreq );
    maxScanFreq = mFrontDev->formatATVFreq ( endFreq );
    if ( minScanFreq == 0 || maxScanFreq == 0 ) {
        LOGE ( "%s, munual scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
        return -1;
    }

    //if  set std null AUTO, use default PAL/DK
    //if(videoStd == CC_ATV_VIDEO_STD_AUTO) {
    //    vStd = CC_ATV_VIDEO_STD_PAL;
    //    aStd = CC_ATV_AUDIO_STD_DK;
    // } else {
    vStd = videoStd;
    aStd = audioStd;
    // }

    AutoMutex _l( mLock );
    if ( !mATVDisplaySnow )
        mAv.DisableVideoWithBlueColor();
    mTvAction |= TV_ACTION_SCANNING;
    mTvScanner->setObserver ( &mTvMsgQueue );
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    v4l2_std_id stdAndColor = mFrontDev->enumToStdAndColor(vStd, aStd);

    tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    mpTvin->VDIN_OpenPort ( source_port );

    int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );
    LOGD("%s, atv manual scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);

    return mTvScanner->ATVManualScan ( minScanFreq, maxScanFreq, stdAndColor, store_Type, channel_num);
}

int CTv::getVideoFormatInfo ( int *pWidth, int *pHeight, int *pFPS, int *pInterlace )
{
    int  iOutRet = -1;
    AM_AV_VideoStatus_t video_status;

    do {
        if ( NULL == pWidth || NULL == pHeight || NULL == pFPS || NULL == pInterlace ) {
            break;
        }

        iOutRet = mAv.GetVideoStatus (&video_status );

        if ( AM_SUCCESS != iOutRet ) {
            LOGD ( "%s, ERROR: Cann't Get video format info\n", __FUNCTION__);
            break;
        }

        *pWidth = video_status.src_w;
        *pHeight = video_status.src_h;
        *pFPS = video_status.fps;
        *pInterlace = video_status.interlaced;
        //LOGD("%s, w : %d h : %d fps : %d  interlaced: %d\n", __FUNCTION__, video_status.src_w,video_status.src_h,video_status.fps,video_status.interlaced);
    } while ( false );

    return iOutRet;
}

int CTv::getAudioFormatInfo ( int fmt[2], int sample_rate[2], int resolution[2], int channels[2],
    int lfepresent[2], int *frames, int *ab_size, int *ab_data, int *ab_free)
{
    int  iOutRet = -1;
    AM_AV_AudioStatus_t audio_status;

    do {

        iOutRet = mAv.GetAudioStatus (&audio_status );

        if ( AM_SUCCESS != iOutRet ) {
            LOGD ( "%s, ERROR: Cann't Get audio format info\n", __FUNCTION__);
            break;
        }

        if (fmt) {
            fmt[0] = audio_status.aud_fmt;
            fmt[1] = audio_status.aud_fmt_orig;
        }
        if (sample_rate) {
            sample_rate[0] = audio_status.sample_rate;
            sample_rate[1] = audio_status.sample_rate_orig;
        }
        if (resolution) {
            resolution[0] = audio_status.resolution;
            resolution[1] = audio_status.resolution_orig;
        }
        if (channels) {
            channels[0] = audio_status.channels;
            channels[1] = audio_status.channels_orig;
        }
        if (lfepresent) {
            lfepresent[0] = audio_status.lfepresent;
            lfepresent[1] = audio_status.lfepresent_orig;
        }
        if (frames)
            *frames = audio_status.frames;
        if (ab_size)
            *ab_size = audio_status.ab_size;
        if (ab_data)
            *ab_data = audio_status.ab_data;
        if (ab_free)
            *ab_free = audio_status.ab_free;

    } while ( false );

    return iOutRet;
}

int CTv::stopScanLock()
{
    AutoMutex _l( mLock );
    return stopScan();
}

int CTv::stopScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }

    LOGD("%s, tv scanning , stop it\n", __FUNCTION__);
    if ( ( SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->Tvin_StopDecoder();
        mpTvin->SwitchSnow( false );
        mAv.DisableVideoWithBlackColor();
    } else {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
    }
    mSigDetectThread.requestAndWaitPauseDetect();
    mSigDetectThread.setObserver(this);
    //mTvEpg.leaveChannel();
    mTvScanner->stopScan();
    mFrontDev->Close();
    mTvAction &= ~TV_ACTION_SCANNING;
    return 0;
}

int CTv::pauseScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    return mTvScanner->pauseScan();
}

int CTv::resumeScan()
{
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    return mTvScanner->resumeScan();
}

int CTv::getScanStatus()
{
    int status;
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }
    mTvScanner->getScanStatus(&status);
    return status;
}
void CTv::operateDeviceForScan(int type)
{
    LOGD("%s : type:%d\n", __FUNCTION__, type);
    if (type & OPEN_DEV_FOR_SCAN_ATV) {
            mFrontDev->Open(FE_ANALOG);
            mFrontDev->SetAnalogFrontEndTimerSwitch(1);
     }else if  (type & OPEN_DEV_FOR_SCAN_DTV) {
            mFrontDev->Open(FE_AUTO);
            mFrontDev->SetAnalogFrontEndTimerSwitch(0);
     }else if  (type & CLOSE_DEV_FOR_SCAN) {
            mFrontDev->SetAnalogFrontEndTimerSwitch(0);
     }
}
void CTv::setDvbTextCoding(char *coding)
{
    if (!strcmp(coding, "standard")) {
        AM_SI_SetDefaultDVBTextCoding("");
    } else {
        AM_SI_SetDefaultDVBTextCoding(coding);
    }
}

int CTv::playDvbcProgram ( int progId )
{
    LOGD("%s, progId = %d\n", __FUNCTION__,  progId );

    CTvProgram prog;
    int ret = CTvProgram::selectByID ( progId, prog );
    if ( ret != 0 ) {
        return -1;
    }

    LOGD("%s, prog name = %s\n", __FUNCTION__,  prog.getName().string() );
    CTvChannel channel;
    prog.getChannel ( channel );

    mFrontDev->setPara ( channel.getMode(), channel.getFrequency(), channel.getSymbolRate(), channel.getModulation() );

    int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;

    CTvProgram::Video *pV = prog.getVideo();
    if ( pV != NULL ) {
        vpid = pV->getPID();
        vfmt = pV->getFormat();
    }

    int aindex = prog.getCurrentAudio ( String8 ( "eng" ) );
    if ( aindex >= 0 ) {
        CTvProgram::Audio *pA = prog.getAudio ( aindex );
        if ( pA != NULL ) {
            apid = pA->getPID();
            afmt = pA->getFormat();
        }
    }
    mTvEpg.leaveProgram();
    mTvEpg.leaveChannel();
    startPlayTv ( SOURCE_DTV, vpid, apid, prog.getPcrId(), vfmt, afmt );

    mTvEpg.enterChannel ( channel.getID() );
    mTvEpg.enterProgram ( prog.getID() );
    return 0;
}

int CTv::getAudioTrackNum ( int progId )
{
    int iRet, iOutCnt = 0;
    CTvProgram prog;

    do {
        if ( 0 > progId ) {
            break;
        }

        iRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iRet ) {
            break;
        }

        iOutCnt = prog.getAudioTrackSize();
    } while ( false );

    return iOutCnt;
}

int CTv::getAudioInfoByIndex ( int progId, int idx, int *pAFmt, String8 &lang )
{
    int iRet, iOutRet = -1;
    CTvProgram prog;
    CTvProgram::Audio *pA = NULL;

    do {
        if ( NULL == pAFmt || idx < 0 ) {
            break;
        }

        iRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iRet ) {
            break;
        }

        if ( idx >= prog.getAudioTrackSize() ) {
            break;
        }

        pA = prog.getAudio ( idx );
        if ( NULL == pA ) {
            break;
        }

        *pAFmt = pA->getFormat();
        lang = pA->getLang();
    } while ( false );

    return iOutRet;
}

int CTv::switchAudioTrack (int aPid, int aFmt, int aParam __unused)
{
    int iOutRet = 0;
    do {
        if ( aPid < 0 || aFmt < 0 ) {
            break;
        }

        iOutRet = mAv.SwitchTSAudio (( uint16_t ) aPid, ( AM_AV_AFormat_t ) aFmt );
        LOGD ("%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
    } while ( false );

    return iOutRet;
}

int CTv::switchAudioTrack ( int progId, int idx )
{
    int iOutRet = 0;
    CTvProgram prog;
    CTvProgram::Audio *pAudio = NULL;
    int aPid = -1;
    int aFmt = -1;

    do {
        if ( idx < 0 ) {
            break;
        }

        iOutRet = CTvProgram::selectByID ( progId, prog );
        if ( 0 != iOutRet ) {
            break;
        }
        /*if (prog.getAudioTrackSize() <= 1) {
            LOGD("%s, just one audio track, not to switch", __FUNCTION__);
            break;
        }*/
        if ( idx >= prog.getAudioTrackSize() ) {
            break;
        }

        pAudio = prog.getAudio ( idx );
        if ( NULL == pAudio ) {
            break;
        }

        aPid = pAudio->getPID();
        aFmt = pAudio->getFormat();
        if ( aPid < 0 || aFmt < 0 ) {
            break;
        }

        iOutRet = mAv.SwitchTSAudio (( uint16_t ) aPid, ( AM_AV_AFormat_t ) aFmt );
        LOGD ( "%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
    } while ( false );

    return iOutRet;
}

int CTv::ResetAudioDecoderForPCMOutput()
{
    int iOutRet = mAv.ResetAudioDecoder ();
    LOGD ( "%s, iOutRet = %d AM_AV_ResetAudioDecoder\n", __FUNCTION__,  iOutRet );
    return iOutRet;
}

int CTv::setAudioAD (int enable, int aPid, int aFmt)
{
    int iOutRet = 0;
    if ( aPid < 0 || aFmt < 0 ) {
        return iOutRet;
    }

    iOutRet = mAv.SetADAudio (enable, ( uint16_t ) aPid, ( AM_AV_AFormat_t ) aFmt );
    LOGD ("%s, iOutRet = %d (%d,%d,%d)\n", __FUNCTION__,  iOutRet, enable, aPid, aFmt );

    return iOutRet;
}

int CTv::playDtvProgram (const char *feparas, int mode, int freq, int para1, int para2,
                          int vpid, int vfmt, int apid, int afmt, int pcr, int audioCompetation)
{
    AutoMutex _l( mLock );

    SetSourceSwitchInputLocked(m_source_input_virtual, SOURCE_DTV);

    if (mBlackoutEnable)
        mAv.EnableVideoBlackout();
    else
        mAv.DisableVideoBlackout();
    //mAv.ClearVideoBuffer();

    mFrontDev->Open(FE_AUTO);
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        if (!feparas) {
            if ( SOURCE_ADTV == m_source_input_virtual ) {
                mFrontDev->setPara (FE_ATSC, freq, para1, para2);
            } else {
                mFrontDev->setPara (mode, freq, para1, para2);
            }
        } else {
            mFrontDev->setPara(feparas);
        }
    }
    mTvAction |= TV_ACTION_PLAYING;
    startPlayTv ( SOURCE_DTV, vpid, apid, pcr, vfmt, afmt );

    CMessage msg;
    msg.mDelayMs = 2000;
    msg.mType = CTvMsgQueue::TV_MSG_CHECK_FE_DELAY;
    mTvMsgQueue.sendMsg ( msg );
    SetCurProgramAudioVolumeCompensationVal ( audioCompetation );
    return 0;
}

int CTv::playDtvProgram(int mode, int freq, int para1, int para2, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgram(NULL, mode, freq, para1, para2, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}

int CTv::playDtvProgram(const char* feparas, int vpid, int vfmt, int apid,
        int afmt, int pcr, int audioCompetation) {
    return playDtvProgram(feparas, 0, 0, 0, 0, vpid, vfmt, apid, afmt, pcr, audioCompetation);
}

int CTv::playDtmbProgram ( int progId )
{
    CTvProgram prog;
    CTvChannel channel;
    int vpid = 0x1fff, apid = 0x1fff, vfmt = -1, afmt = -1;
    int aindex;
    CTvProgram::Audio *pA;
    CTvProgram::Video *pV;
    int ret = CTvProgram::selectByID ( progId, prog );
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    saveDTVProgramID ( progId );
    prog.getChannel ( channel );
    //音量补偿
    int chanVolCompValue = 0;
    chanVolCompValue = GetAudioVolumeCompensationVal(progId);
    SetCurProgramAudioVolumeCompensationVal ( chanVolCompValue );

    mFrontDev->setPara ( channel.getMode(), channel.getFrequency(), channel.getBandwidth(), 0 );

    pV = prog.getVideo();
    if ( pV != NULL ) {
        vpid = pV->getPID();
        vfmt = pV->getFormat();
    }

    aindex = prog.getCurrAudioTrackIndex();
    if ( -1 == aindex ) { //db is default
        aindex = prog.getCurrentAudio ( String8 ( "eng" ) );
        if ( aindex >= 0 ) {
            prog.setCurrAudioTrackIndex ( progId, aindex );
        }
    }

    if ( aindex >= 0 ) {
        pA = prog.getAudio ( aindex );
        if ( pA != NULL ) {
            apid = pA->getPID();
            afmt = pA->getFormat();
        }
    }

    mTvEpg.leaveProgram();
    mTvEpg.leaveChannel();
    startPlayTv ( SOURCE_DTV, vpid, apid, prog.getPcrId(), vfmt, afmt );
    mTvEpg.enterChannel ( channel.getID() );
    mTvEpg.enterProgram ( prog.getID() );
    return 0;
}

int CTv::playAtvProgram (int  freq, int videoStd, int audioStd, int fineTune __unused, int audioCompetation)
{
    SetSourceSwitchInputLocked(m_source_input_virtual, SOURCE_TV);

    mTvAction |= TV_ACTION_PLAYING;
    if ( mBlackoutEnable ) {
        mAv.EnableVideoBlackout();
    } else {
        mAv.DisableVideoBlackout();
    }
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    //image selecting channel
    mSigDetectThread.requestAndWaitPauseDetect();
    mpTvin->Tvin_StopDecoder();
    mFrontDev->Open(FE_ANALOG);
    //set CVBS
    int fmt = CFrontEnd::stdEnumToCvbsFmt (videoStd, audioStd);
    mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

    v4l2_std_id stdAndColor = mFrontDev->enumToStdAndColor (videoStd, audioStd);
    //set TUNER
    mFrontDev->setPara (FE_ANALOG, freq, stdAndColor, 1);

    mSigDetectThread.setObserver ( this );
    mSigDetectThread.initSigState();
    mSigDetectThread.resumeDetect(1000);

    SetCurProgramAudioVolumeCompensationVal ( audioCompetation );
    return 0;
}

int CTv::resetFrontEndPara ( frontend_para_set_t feParms )
{
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
    if (mTvAction & TV_ACTION_SCANNING) {
        return -1;
    }

    if ( feParms.mode == FE_ANALOG ) {
        int progID = -1;
        int tmpFreq = feParms.freq;
        int tmpfineFreq = feParms.para2;
        int mode = feParms.mode;

        //get tunerStd from videoStd and audioStd
        v4l2_std_id stdAndColor = mFrontDev->enumToStdAndColor (feParms.videoStd, feParms.audioStd);

        LOGD("%s, resetFrontEndPara- vstd=%d astd=%d stdandcolor=%lld", __FUNCTION__, feParms.videoStd, feParms.audioStd, stdAndColor);

        //set frontend parameters to tuner dev
        mSigDetectThread.requestAndWaitPauseDetect();
        mpTvin->Tvin_StopDecoder();

        //set CVBS
        int fmt = CFrontEnd::stdEnumToCvbsFmt (feParms.videoStd, feParms.audioStd );
        mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

        //set TUNER
        usleep(400 * 1000);
        mFrontDev->Open(FE_ANALOG);
        mFrontDev->setPara ( FE_ANALOG, tmpFreq, stdAndColor, 1 );
        usleep(400 * 1000);
        if ( tmpfineFreq != 0 ) {
            mFrontDev->fineTune ( tmpfineFreq / 1000 );
        }

        mSigDetectThread.initSigState();
        mSigDetectThread.resumeDetect();
    } else {
        mFrontDev->Open(feParms.mode);
        mFrontDev->setPara ( feParms.mode, feParms.freq, feParms.para1, feParms.para2 );
    }

    return 0;
}

int CTv::setFrontEnd ( const char *paras, bool force )
{
    AutoMutex _l( mLock );
    LOGD("mTvAction = %#x, %s", mTvAction, __FUNCTION__);
    if (mTvAction & TV_ACTION_SCANNING) {
        return -1;
    }

    CFrontEnd::FEParas fp(paras);

    if ( fp.getFEMode().getBase() == FE_ANALOG ) {
        if (SOURCE_DTV == m_source_input) {
            mAv.DisableVideoBlackout();
            stopPlaying(false);
        }
        int tmpFreq = fp.getFrequency();
        int tmpfineFreq = fp.getFrequency2();

        //get tunerStd from videoStd and audioStd
        v4l2_std_id stdAndColor = mFrontDev->enumToStdAndColor (fp.getVideoStd(), fp.getAudioStd());

        LOGD("%s: vstd=%d astd=%d stdandcolor=%lld", __FUNCTION__, fp.getVideoStd(), fp.getAudioStd(), stdAndColor);
        SetAudioMuteForTv ( CC_AUDIO_MUTE);
        //set frontend parameters to tuner dev
        mSigDetectThread.requestAndWaitPauseDetect();
        mpTvin->Tvin_StopDecoder();

        //set CVBS
        int fmt = CFrontEnd::stdEnumToCvbsFmt (fp.getVideoStd(), fp.getAudioStd() );
        mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

        //set TUNER
        usleep(400 * 1000);
        mFrontDev->Open(FE_AUTO);
        mFrontDev->setPara ( paras, force );
        usleep(400 * 1000);
        if ( tmpfineFreq != 0 ) {
            mFrontDev->fineTune ( tmpfineFreq / 1000, force );
        }

        mSigDetectThread.initSigState();
        mSigDetectThread.resumeDetect();
    } else {
        if (SOURCE_ADTV == m_source_input_virtual) {
            if (SOURCE_TV == m_source_input) {
                mAv.DisableVideoBlackout();
                mSigDetectThread.requestAndWaitPauseDetect();
                mpTvin->Tvin_StopDecoder();
                if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
                    mpTvin->SwitchSnow( false );
                }
            }
        }
        mFrontDev->Open(FE_AUTO);
        mFrontDev->setPara ( paras, force );
    }
    return 0;
}

int CTv::resetDmxAndAvSource()
{
    AM_DMX_Source_t curdmxSource;
    mFrontDev->GetTSSource ( &curdmxSource );
    LOGD ( "%s, AM_FEND_GetTSSource %d", __FUNCTION__, curdmxSource );
    mTvDmx.Close();
    AM_DMX_OpenPara_t para;
    memset ( &para, 0, sizeof ( para ) );
    mTvDmx.Open (para);
    mTvDmx.SetSource(curdmxSource );
    AM_AV_TSSource_t ts_source = ( AM_AV_TSSource_t ) curdmxSource;
    mAv.SetTSSource (ts_source );
    return 0;
}

int CTv::SetCurProgramAudioVolumeCompensationVal ( int tmpVal )
{
    SetAudioVolumeCompensationVal ( tmpVal );
    SetAudioMasterVolume (GetAudioMasterVolume() );

    LOGD ( "%s, VolumeCompensationVal = %d, id = -1\n", __FUNCTION__,  tmpVal );
    CTvProgram prog;
    if ( CTvProgram::selectByID ( -1, prog ) != 0 ) {
        LOGE ( "[ctv]%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
        return -1;
    }

    if (prog.updateVolComp ( -1, tmpVal ) != 0 ) {
        LOGE ( "[ctv]%s, atv progID is not matching the db's\n", __FUNCTION__);
        return -1;
    }
    return 0;
}

int CTv::GetAudioVolumeCompensationVal(int progxxId __unused)
{
    int tmpVolValue = 0;
    CTvProgram prog;
    if ( CTvProgram::selectByID ( -1, prog ) != 0 ) {
        LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
        return tmpVolValue;
    }
    tmpVolValue = prog.getChanVolume ();
    LOGD ( "%s,progid = -1 CompensationVal = %d\n", __FUNCTION__, tmpVolValue );
    if (tmpVolValue > 20 || tmpVolValue < -20) tmpVolValue = 0;
    return tmpVolValue;
}

int CTv::startPlayTv ( int source, int vid, int aid, int pcrid, int vfat, int afat )
{
    if ( source == SOURCE_DTV ) {
        AM_FileEcho ( DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN, AV_THRESHOLD_MIN_MS );
        LOGD ( "%s, startPlayTv", __FUNCTION__);
        mAv.StartTS (vid, aid, pcrid, ( AM_AV_VFormat_t ) vfat, ( AM_AV_AFormat_t ) afat );
    }
    return 0;
}

int CTv::stopPlayingLock()
{
    AutoMutex _l( mLock );
    if (mSubtitle.sub_switch_status() == 1)
        mSubtitle.sub_stop_dvb_sub();
    return stopPlaying(true);
}

int CTv::stopPlaying(bool isShowTestScreen) {
    return stopPlaying(isShowTestScreen, true);
}

int CTv::stopPlaying(bool isShowTestScreen, bool resetFE)
{
    if (!(mTvAction & TV_ACTION_PLAYING)) {
        LOGD("%s, stopplay cur action = %x not playing , return", __FUNCTION__, mTvAction);
        return 0;
    }

    if (m_source_input == SOURCE_TV) {
        //first mute
        SetAudioMuteForTv(CC_AUDIO_MUTE);
        if (resetFE)
            ClearAnalogFrontEnd();
    } else if (m_source_input ==  SOURCE_DTV) {
        mSigDetectThread.requestAndWaitPauseDetect();
        mAv.EnableVideoBlackout();
        mAv.StopTS ();
    }

    if ( SOURCE_TV != m_source_input ) {
        if ( true == isShowTestScreen ) {
            if ( iSBlackPattern ) {
                mAv.DisableVideoWithBlackColor();
            } else {
                mAv.DisableVideoWithBlueColor();
            }
        }
    }

    mTvAction &= ~TV_ACTION_PLAYING;
    return 0;
}

int CTv::getAudioChannel()
{
    int iRet = -1;
    AM_AOUT_OutputMode_t audioChanneleMod;
    do {
        iRet = mAv.AudioGetOutputMode (&audioChanneleMod );
        if ( AM_SUCCESS != iRet ) {
            LOGD ( "%s, audio GetOutputMode is FAILED %d\n", __FUNCTION__,  iRet );
            break;
        }
        LOGD ( "%s, jianfei.lan getAudioChannel iRet : %d audioChanneleMod %d\n", __FUNCTION__,  iRet, audioChanneleMod );
    } while ( 0 );
    return audioChanneleMod;
}

int CTv::setAudioChannel ( int channelIdx )
{
    int iOutRet = 0;
    AM_AOUT_OutputMode_t audioChanneleMod;
    LOGD ( "%s, channelIdx : %d\n", __FUNCTION__,  channelIdx );
    audioChanneleMod = ( AM_AOUT_OutputMode_t ) channelIdx;
    iOutRet = mAv.AudioSetOutputMode (audioChanneleMod );
    int aud_ch = 1;
    if (audioChanneleMod == AM_AOUT_OUTPUT_STEREO) {
        aud_ch = 1;
    } else if (audioChanneleMod == AM_AOUT_OUTPUT_DUAL_LEFT) {
        aud_ch = 2;
    } else if (audioChanneleMod == AM_AOUT_OUTPUT_DUAL_RIGHT) {
        aud_ch = 3;
    }
    CTvProgram::updateAudioChannel(-1, aud_ch);
    if ( AM_SUCCESS != iOutRet) {
        LOGD ( "%s, TV AM_AOUT_SetOutputMode device is FAILED %d\n", __FUNCTION__, iOutRet );
    }
    return 0;
}

int CTv::getFrontendSignalStrength()
{
    return mFrontDev->getStrength();
}

int CTv::getFrontendSNR()
{
    return mFrontDev->getSNR();
}

int CTv::getFrontendBER()
{
    return mFrontDev->getBER();
}

int CTv::getChannelInfoBydbID ( int dbID, channel_info_t &chan_info )
{
    CTvProgram prog;
    CTvChannel channel;
    Vector<sp<CTvProgram> > out;
    memset ( &chan_info, sizeof ( chan_info ), 0 );
    chan_info.freq = 44250000;
    chan_info.uInfo.atvChanInfo.videoStd = CC_ATV_VIDEO_STD_PAL;
    chan_info.uInfo.atvChanInfo.audioStd = CC_ATV_AUDIO_STD_DK;
    static const int CVBS_ID = -1;
    if (dbID == CVBS_ID) {
        unsigned char std;
        SSMReadCVBSStd(&std);
        chan_info.uInfo.atvChanInfo.videoStd = (atv_video_std_t)std;
        chan_info.uInfo.atvChanInfo.audioStd = CC_ATV_AUDIO_STD_DK;
        chan_info.uInfo.atvChanInfo.isAutoStd = (std == CC_ATV_VIDEO_STD_AUTO) ? 1 : 0;
        return 0;
    }

    int ret = CTvProgram::selectByID ( dbID, prog );
    if ( ret != 0 ) {
        LOGD ( "getChannelinfo , select dbid=%d,is not exist", dbID);
        return -1;
    }
    prog.getChannel ( channel );

    if ( CTvProgram::TYPE_ATV == prog.getProgType() ) {
        chan_info.freq = channel.getFrequency();
        chan_info.uInfo.atvChanInfo.finefreq  =  channel.getAfcData();
        LOGD("%s, get channel std = %d", __FUNCTION__, channel.getStd());
        chan_info.uInfo.atvChanInfo.videoStd =
            ( atv_video_std_t ) CFrontEnd::stdAndColorToVideoEnum ( channel.getStd() );
        chan_info.uInfo.atvChanInfo.audioStd =
            ( atv_audio_std_t ) CFrontEnd::stdAndColorToAudioEnum ( channel.getStd() );
        chan_info.uInfo.atvChanInfo.isAutoStd = ((channel.getStd() & V4L2_COLOR_STD_AUTO) ==  V4L2_COLOR_STD_AUTO) ? 1 : 0;
    } else if ( CTvProgram::TYPE_DTV == prog.getProgType()  || CTvProgram::TYPE_RADIO == prog.getProgType()) {
        chan_info.freq = channel.getFrequency();
        chan_info.uInfo.dtvChanInfo.strength = getFrontendSignalStrength();
        chan_info.uInfo.dtvChanInfo.quality = getFrontendSNR();
        chan_info.uInfo.dtvChanInfo.ber = getFrontendBER();
    }

    return 0;
}

bool CTv::Tv_Start_Analyze_Ts ( int channelID )
{
    int freq, ret;
    CTvChannel channel;

    AutoMutex _l( mLock );
    mAv.StopTS ();
    mAv.DisableVideoWithBlueColor();
    ret = CTvChannel::selectByID ( channelID, channel );
    if ( ret != 0 ) {
        LOGD ( "%s, CTv tv_updatats can not get freq by channel ID", __FUNCTION__ );
        return false;
    }

    mTvAction |= TV_ACTION_SCANNING;
    freq = channel.getFrequency();
    LOGD ( "%s, the freq = %d", __FUNCTION__,  freq );
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_ANALYZE_CHANNEL;
    mTvScanner->setObserver ( &mTvMsgQueue );
    mTvScanner->manualDtmbScan ( freq, freq ); //dtmb
    return true;
}

bool CTv::Tv_Stop_Analyze_Ts()
{
    stopScanLock();
    return true;
}

int CTv::saveATVProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "atv.get.program.id", dbID );
    return 0;
}

int CTv::getATVProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "atv.get.program.id", -1 );
}

int CTv::saveDTVProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "dtv.get.program.id", dbID );
    return 0;
}

int CTv::getDTVProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "dtv.get.program.id", -1 );
}

int CTv::saveRadioProgramID ( int dbID )
{
    config_set_int ( CFG_SECTION_TV, "radio.get.program.id", dbID );
    return 0;
}

int CTv::getRadioProgramID ( void )
{
    return config_get_int ( CFG_SECTION_TV, "radio.get.program.id", -1 );
}


int CTv::getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq )
{
    const char *strDelimit = ",";
    char *token = NULL;

    *scanMinFreq = 44250000;
    *scanMaxFreq = 868250000;

    const char *freqList = config_get_str ( CFG_SECTION_ATV, CFG_ATV_FREQ_LIST, "null" );
    if ( strcmp ( freqList, "null" ) == 0 ) {
        LOGE( "[ctv]%s, atv freq list is null \n", __FUNCTION__ );
        return -1;
    }

    char data_str[512] = {0};
    strncpy ( data_str, freqList, sizeof ( data_str ) - 1 );

    char *pSave;
    token = strtok_r ( data_str, strDelimit, &pSave);
    sscanf ( token, "%d", scanMinFreq );
    token = strtok_r ( NULL, strDelimit , &pSave);
    if ( token != NULL ) {
        sscanf ( token, "%d", scanMaxFreq );
    }
    return 0;
}

int CTv::IsDVISignal()
{
    return ( ( TVIN_SIG_FMT_NULL != mSigDetectThread.getCurSigInfo().fmt ) &&
        ( mSigDetectThread.getCurSigInfo().reserved & 0x1 ) );
}

int CTv::getHDMIFrameRate()
{
    int ConstRate[5] = {24, 25, 30, 50, 60};
    float ConstRateDiffHz[5] = {0.5, 0.5, 0.5, 2, 2};
    int fps = mSigDetectThread.getCurSigInfo().fps;
    for (int i = 0; i < 5; i++) {
        if (abs(ConstRate[i] - fps) < ConstRateDiffHz[i])
            fps = ConstRate[i];
    }
    return fps;
}

tv_source_input_t CTv::GetLastSourceInput ( void )
{
    return m_last_source_input;
}

int CTv::isVgaFmtInHdmi ( void )
{
    if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
        return -1;
    }
    return CTvin::isVgaFmtInHdmi (mSigDetectThread.getCurSigInfo().fmt);
}

void CTv::print_version_info ( void )
{
    // print tvapi version info
    LOGD ( "libtvservice git branch:%s\n", tvservice_get_git_branch_info() );
    LOGD ( "libtvservice git version:%s\n",  tvservice_get_git_version_info() );
    LOGD ( "libtvservice Last Changed:%s\n", tvservice_get_last_chaned_time_info() );
    LOGD ( "libtvservice Last Build:%s\n",  tvservice_get_build_time_info() );
    LOGD ( "libtvservice Builer Name:%s\n", tvservice_get_build_name_info() );
    LOGD ( "libtvservice board version:%s\n", tvservice_get_board_version_info() );
    LOGD ( "\n\n");
    // print dvb version info
    LOGD ( "libdvb git branch:%s\n", dvb_get_git_branch_info() );
    LOGD ( "libdvb git version:%s\n", dvb_get_git_version_info() );
    LOGD ( "libdvb Last Changed:%s\n", dvb_get_last_chaned_time_info() );
    LOGD ( "libdvb Last Build:%s\n", dvb_get_build_time_info() );
    LOGD ( "libdvb Builer Name:%s\n", dvb_get_build_name_info() );
}

int CTv::Tvin_GetTvinConfig ( void )
{
    const char *config_value;

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_KERNELPET_DISABLE, "null" );
    if ( strcmp ( config_value, "disable" ) == 0 ) {
        gTvinConfig.kernelpet_disable = true;
    } else {
        gTvinConfig.kernelpet_disable = false;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_KERNELPET_TIMEROUT, "null" );
    gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

    if ( gTvinConfig.kernelpet_timeout <= 0 || gTvinConfig.kernelpet_timeout > 40 ) {
        gTvinConfig.kernelpet_timeout = 10;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET, "null" );
    if ( strcmp ( config_value, "enable" ) == 0 ) {
        gTvinConfig.userpet = true;
    } else {
        gTvinConfig.userpet = false;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET_TIMEROUT, "null" );
    gTvinConfig.userpet_timeout = ( unsigned int ) strtol ( config_value, NULL, 10 );

    if ( gTvinConfig.userpet_timeout <= 0 || gTvinConfig.userpet_timeout > 100 ) {
        gTvinConfig.userpet_timeout = 10;
    }

    config_value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_USERPET_RESET, "null" );
    if ( strcmp ( config_value, "disable" ) == 0 ) {
        gTvinConfig.userpet_reset = 0;
    } else {
        gTvinConfig.userpet_reset = 1;
    }
    return 0;
}

TvRunStatus_t CTv::GetTvStatus()
{
    AutoMutex _l( mLock );
    return mTvStatus;
}

int CTv::OpenTv ( void )
{
    const char * value;
    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_ATV_DISPLAY_SNOW, "null" );
    if (strcmp(value, "enable") == 0 ) {
        mATVDisplaySnow = true;
    } else {
        mATVDisplaySnow = false;
    }

    value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        iSBlackPattern = true;
    } else {
        iSBlackPattern = false;
    }

    value = config_get_str ( CFG_SECTION_TV, CFG_FBC_PANEL_INFO, "null" );
    LOGD("open tv, get fbc panel info:%s\n", value);
    if (strcmp(value, "edid") == 0 ) {
        rebootSystemByEdidInfo();
    } else if (strcmp(value, "uart") == 0 ) {
        rebootSystemByUartPanelInfo(fbcIns);
    }

    mpTvin->Tvin_LoadSourceInputToPortMap();

    //tv ssm check
    SSMHandlePreCopying();

    SSM_status_t SSM_status = (SSM_status_t)(CVpp::getInstance()->VPP_GetSSMStatus());
    LOGD ("Ctv-SSM status= %d\n", SSM_status);

    if ( SSMDeviceMarkCheck() < 0 || SSM_status == SSM_HEADER_INVALID) {
        LOGD ("Restore SSMData file");
        Tv_SSMRestoreDefaultSetting();
    }else if (SSM_status == SSM_HEADER_STRUCT_CHANGE) {
        CVpp::getInstance()->TV_SSMRecovery();
    }

    mpTvin->OpenTvin();
    mpTvin->init_vdin();
    mpTvin->Tv_init_afe();

    const char *path = config_get_str(CFG_SECTION_TV, CFG_PQ_DB_PATH, DEF_DES_PQ_DB_PATH);
    CVpp::getInstance()->Vpp_Init(path, mHdmiOutFbc);
    TvAudioOpen();
    SetAudioVolDigitLUTTable(SOURCE_MPEG);

    SSMSetHDCPKey();
    system ( "/system/bin/dec" );

    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DISPLAY_FREQ_AUTO, "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        mAutoSetDisplayFreq = true;
    } else {
        mAutoSetDisplayFreq = false;
    }

    mHDMIRxManager.SetHdmiPortCecPhysicAddr();
    Tv_HandeHDMIEDIDFilePathConfig();
    SSMSetHDMIEdid(1);
    mHDMIRxManager.HdmiRxEdidUpdate();

    Tvin_GetTvinConfig();
    m_last_source_input = SOURCE_INVALID;
    m_source_input = SOURCE_INVALID;
    m_hdmi_sampling_rate = 0;

    int8_t enable;
    SSMReadBlackoutEnable(&enable);
    mBlackoutEnable = ((enable==1)?true:false);

    mFrontDev->Open(FE_AUTO);
    mFrontDev->autoLoadFE();
    mAv.Open();
    resetDmxAndAvSource();
    mSourceConnectDetectThread.startDetect();
    ClearAnalogFrontEnd();
    //mFrontDev->Close();

    mTvStatus = TV_OPEN_ED;
    return 0;
}

int CTv::CloseTv ( void )
{
    mSigDetectThread.stopDetect();
    mpTvin->Tv_uninit_afe();
    mpTvin->uninit_vdin();
    TvMisc_DisableWDT ( gTvinConfig.userpet );
    mTvStatus = TV_CLOSE_ED;
    return 0;
}

int CTv::StartTvLock ()
{
    LOGD ( "[source_switch_time]: %fs,  StartTvLock status = %d", getUptimeSeconds(), mTvStatus);
    if (mTvStatus == TV_START_ED)
        return 0;

    AutoMutex _l( mLock );
    mTvAction |= TV_ACTION_STARTING;

    //tvWriteSysfs("/sys/power/wake_lock", "tvserver.run");

    //mAv.ClearVideoBuffer();
    mAv.SetVideoLayerDisable(1);
    SwitchAVOutBypass(0);
    InitSetTvAudioCard();
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mSigDetectThread.startDetect();
    mTvMsgQueue.startMsgQueue();
    SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( m_source_input ), m_source_input, mSigDetectThread.getCurSigInfo().fmt);
    TvMisc_EnableWDT ( gTvinConfig.kernelpet_disable, gTvinConfig.userpet, gTvinConfig.kernelpet_timeout, gTvinConfig.userpet_timeout, gTvinConfig.userpet_reset );
    mpTvin->TvinApi_SetCompPhaseEnable ( 1 );
    mpTvin->VDIN_EnableRDMA ( 1 );

    mAv.SetVideoWindow (0, 0, 0, 0);

    mTvAction &= ~TV_ACTION_STARTING;
    mTvStatus = TV_START_ED;
    LOGD ( "[source_switch_time]: %fs, StartTvLock end", getUptimeSeconds());
    MnoNeedAutoSwitchToMonitorMode = false;
    return 0;
}

int CTv::DoInstabootSuspend()
{
    CTvDatabase::GetTvDb()->UnInitTvDb();
    CTvSettingdoSuspend();
    return 0;
}

int CTv::DoInstabootResume()
{
    CTvDatabase::GetTvDb()->InitTvDb(TV_DB_PATH);
    CTvSettingdoResume();
    return 0;
}

int CTv::DoSuspend(int type)
{
    if (type == 1) {
        DoInstabootSuspend();
    }
    return 0;
}

int CTv::DoResume(int type)
{
    if (type == 1) {
        DoInstabootResume();
    }
    return 0;
}

int CTv::StopTvLock ( void )
{
    LOGD("%s, call Tv_Stop status = %d \n", __FUNCTION__, mTvStatus);
    mSigDetectThread.requestAndWaitPauseDetect();
    AutoMutex _l( mLock );
    mTvAction |= TV_ACTION_STOPING;
    mAv.DisableVideoWithBlackColor();
    stopPlaying(false);
    mpTvin->Tvin_StopDecoder();
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( false );
    }
    mpTvin->VDIN_ClosePort();
    //stop scan  if scanning
    stopScan();
    mFrontDev->SetAnalogFrontEndTimerSwitch(0);
    //
    setAudioChannel(AM_AOUT_OUTPUT_STEREO);
    mpTvin->setMpeg2Vdin(0);
    mAv.setLookupPtsForDtmb(0);
    SwitchAVOutBypass(0);
    tv_audio_channel_e audio_channel = mpTvin->Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
    AudioLineInSelectChannel( audio_channel );
    AudioCtlUninit();
    SetAudioVolDigitLUTTable(SOURCE_MPEG);
    Tv_SetAudioInSource(SOURCE_MPEG);
    SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( SOURCE_MPEG ), SOURCE_MPEG, mSigDetectThread.getCurSigInfo().fmt);
    RefreshAudioMasterVolume ( SOURCE_MPEG );
    m_last_source_input = SOURCE_INVALID;
    m_source_input = SOURCE_INVALID;
    m_source_input_virtual = SOURCE_INVALID;
    mFrontDev->Close();
    mTvAction &= ~TV_ACTION_STOPING;
    mTvStatus = TV_STOP_ED;
    MnoNeedAutoSwitchToMonitorMode = false;
    if ( Get2d4gHeadsetEnable() == 1 ) {
        property_set("audio.tv_open.flg", "0");
    }

    mAv.DisableVideoWithBlackColor();
    mAv.ClearVideoBuffer();
    mAv.EnableVideoBlackout();
    SetAudioMuteForTv ( CC_AUDIO_UNMUTE );

    //tvWriteSysfs("/sys/power/wake_unlock", "tvserver.run");

    return 0;
}

int CTv::Tv_MiscSetBySource ( tv_source_input_t source_input )
{
    int ret = -1;

    switch ( source_input ) {
    case SOURCE_TV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret = tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "4" );
        break;

    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        //ret = CVpp::getInstance()->Tv_SavePanoramaMode ( VPP_PANORAMA_MODE_FULL, SOURCE_TYPE_HDMI );
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_DTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);

    case SOURCE_AV1:
    case SOURCE_AV2:
    case SOURCE_VGA:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_SVIDEO:
    case SOURCE_IPTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
    default:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        ret |= tvWriteSysfs ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;
    }
    return ret;
}

bool CTv::isVirtualSourceInput(tv_source_input_t source_input) {
    switch (source_input) {
        case SOURCE_ADTV:
            return true;
        default:
            return false;
    }
    return false;
}

int CTv::SetSourceSwitchInput(tv_source_input_t source_input)
{
   AutoMutex _l( mLock );

    LOGD ( "%s, source input = %d", __FUNCTION__, source_input );

    m_source_input_virtual = source_input;

    if (isVirtualSourceInput(source_input))
        return 0;//defer real source setting.

    return SetSourceSwitchInputLocked(m_source_input_virtual, source_input);
}

int CTv::SetSourceSwitchInput(tv_source_input_t virtual_input, tv_source_input_t source_input) {
    AutoMutex _l( mLock );
    return SetSourceSwitchInputLocked(virtual_input, source_input);
}

int CTv::SetSourceSwitchInputLocked(tv_source_input_t virtual_input, tv_source_input_t source_input)
{
    tvin_port_t cur_port;

    LOGD ( "[source_switch_time]: %fs, %s, virtual source input = %d source input = %d",
        getUptimeSeconds(), __FUNCTION__, virtual_input, source_input );

    m_source_input_virtual = virtual_input;

    Tv_SetDDDRCMode(source_input);
    if (source_input == m_source_input ) {
        LOGW ( "[ctv]%s,same source input, return", __FUNCTION__ );
        return 0;
    }
    stopPlaying(false, !(source_input == SOURCE_DTV && m_source_input == SOURCE_TV));
    KillMediaServerClient();
    //if HDMI, need to set EDID of each port
    if (mSetHdmiEdid) {
        int tmp_ret = 0;
        switch ( source_input ) {
        case SOURCE_HDMI1:
            tmp_ret = SSMSetHDMIEdid(1);
            break;
        case SOURCE_HDMI2:
            tmp_ret = SSMSetHDMIEdid(2);
            break;
        case SOURCE_HDMI3:
            tmp_ret = SSMSetHDMIEdid(3);
            break;
        case SOURCE_HDMI4:
            tmp_ret = SSMSetHDMIEdid(4);
            break;
        default:
            tmp_ret = -1;
            break;
        }
        if (tmp_ret < 0)
            LOGE ( "[ctv]%s, do not set hdmi port%d edid.ret=%d", __FUNCTION__, source_input - 4, tmp_ret );
    }
    mTvAction |= TV_ACTION_SOURCE_SWITCHING;

    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mSigDetectThread.requestAndWaitPauseDetect();
    mAv.DisableVideoWithBlackColor();
    //enable blackout, when play,disable it
    mAv.EnableVideoBlackout();
    //set front dev mode
    if ( source_input == SOURCE_TV ) {
        mFrontDev->Open(FE_ANALOG);
        mFrontDev->SetAnalogFrontEndTimerSwitch(1);
    } else if ( source_input == SOURCE_DTV ) {
        mFrontDev->Open(FE_AUTO);
        mFrontDev->SetAnalogFrontEndTimerSwitch(0);
    } else {
        mFrontDev->Close();
        mFrontDev->SetAnalogFrontEndTimerSwitch(0);
    }

    //ok
    m_last_source_input = m_source_input;
    m_source_input = source_input;
    SSMSaveSourceInput ( source_input );

    SetAudioVolumeCompensationVal ( 0 );

    if ( source_input == SOURCE_DTV ) {
        resetDmxAndAvSource();

        //we should stop audio first for audio mute.
        SwitchAVOutBypass(0);
        tv_audio_channel_e audio_channel = mpTvin->Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
        AudioLineInSelectChannel( audio_channel );
        AudioCtlUninit();
        SetAudioVolDigitLUTTable(SOURCE_MPEG);
        //
        mpTvin->Tvin_StopDecoder();
        mpTvin->VDIN_ClosePort();
        mpTvin->Tvin_WaitPathInactive ( TV_PATH_TYPE_DEFAULT );

        //double confirm we set the main volume lut buffer to mpeg
        RefreshAudioMasterVolume ( SOURCE_MPEG );
        RefreshSrsEffectAndDacGain();
        SetCustomEQGain();
        LoadAudioVirtualizer();
        mpTvin->setMpeg2Vdin(1);
        mAv.setLookupPtsForDtmb(1);
        mSigDetectThread.setDTVSigInfo(TVIN_SIG_FMT_HDMI_1920X1080P_60HZ, TVIN_TFMT_2D);
        tv_source_input_type_t source_type = mpTvin->Tvin_SourceInputToSourceInputType(m_source_input);
        tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceType(source_type);
        int ret = tvSetCurrentSourceInfo(m_source_input, source_type, source_port, mSigDetectThread.getCurSigInfo().fmt,
                                         INDEX_2D, mSigDetectThread.getCurSigInfo().trans_fmt);
        if (ret < 0) {
            LOGE("%s Set CurrentSourceInfo error!\n");
        }

        CVpp::getInstance()->LoadVppSettings(SOURCE_DTV, mSigDetectThread.getCurSigInfo().fmt, INDEX_2D,
                                             mSigDetectThread.getCurSigInfo().trans_fmt);

        Tv_SetAudioInSource ( source_input );
    } else {
        mpTvin->setMpeg2Vdin(0);
        mAv.setLookupPtsForDtmb(0);
    }

    cur_port = mpTvin->Tvin_GetSourcePortBySourceInput ( source_input );
    Tv_MiscSetBySource ( source_input );

    if (source_input != SOURCE_DTV) {
        // Uninit data
        UnInitTvAudio();
        if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3 ||
              source_input == SOURCE_HDMI4 || source_input == SOURCE_MPEG || source_input == SOURCE_DTV) {
            SwitchAVOutBypass(0);
        } else {
            SwitchAVOutBypass(1);
        }

        tv_audio_channel_e audio_channel = mpTvin->Tvin_GetInputSourceAudioChannelIndex (source_input);
        AudioLineInSelectChannel( audio_channel );

        Tv_SetAudioInSource ( source_input );
        if ( source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3
            || source_input == SOURCE_HDMI4) {
            m_hdmi_sampling_rate = 0;
            m_hdmi_audio_data = 0;
        } else if (source_input == SOURCE_SPDIF) {
            InitTvAudio(48000, CC_IN_USE_SPDIF_DEVICE);
            HanldeAudioInputSr(48000);
            //SetAudioMuteForSystem(CC_AUDIO_UNMUTE);
            //SetAudioMuteForTv(CC_AUDIO_UNMUTE);
        } else {
            InitTvAudio(48000, CC_IN_USE_I2S_DEVICE);
            HanldeAudioInputSr(-1);
        }

        if (mpTvin->SwitchPort ( cur_port ) == 0) { //ok
            if (source_input != SOURCE_SPDIF)
            {
                unsigned char std;
                SSMReadCVBSStd(&std);
                int fmt = CFrontEnd::stdEnumToCvbsFmt (std, CC_ATV_AUDIO_STD_AUTO);
                mpTvin->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

                //for HDMI source connect detect
                mpTvin->VDIN_OpenHDMIPinMuxOn(true);
                CVpp::getInstance()->Vpp_ResetLastVppSettingsSourceType();
            }
            m_sig_stable_nums = 0;
            m_sig_spdif_nums = 0;
            mSigDetectThread.setObserver ( this );
            mSigDetectThread.initSigState();
            mSigDetectThread.setVdinNoSigCheckKeepTimes(150, true);
            if (source_input != SOURCE_SPDIF)
                mSigDetectThread.setTvinSigDetectEnable(true);
            else
                mSigDetectThread.setTvinSigDetectEnable(false);
            mSigDetectThread.resumeDetect(0);
        }
    }

    Tv_SetAudioSourceType(source_input);
    RefreshAudioMasterVolume(source_input);
    Tv_SetAudioOutputSwap_Type(source_input);
    Tv_SetAVOutPut_Input_gain(source_input);

    mTvAction &= ~ TV_ACTION_SOURCE_SWITCHING;
    return 0;
}

void CTv::onSigToStable()
{
    LOGD ( "[source_switch_time]: %fs, onSigToStable start", getUptimeSeconds());

    tv_source_input_type_t source_type = mpTvin->Tvin_SourceInputToSourceInputType(m_source_input);
    tvin_port_t source_port = mpTvin->Tvin_GetSourcePortBySourceType(source_type);
    int ret = tvSetCurrentSourceInfo(m_source_input, source_type, source_port, mSigDetectThread.getCurSigInfo().fmt,
                                     INDEX_2D, mSigDetectThread.getCurSigInfo().trans_fmt);
    if (ret < 0) {
        LOGE("%s Set CurrentSourceInfo error!\n");
    }

    CVpp::getInstance()->LoadVppSettings(m_source_input, mSigDetectThread.getCurSigInfo().fmt, INDEX_2D,
                                         mSigDetectThread.getCurSigInfo().trans_fmt);


    if (mAutoSetDisplayFreq && !mPreviewEnabled) {
        int freq = 60;
        if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
            int fps = getHDMIFrameRate();
            LOGD("onSigToStable HDMI fps = %d", fps);
            if ((30 == fps) || (60 == fps)) {
                freq = 60;
            } else {
                freq = 50;
            }
            autoSwitchToMonitorMode();
        } else if ( CTvin::Tvin_is50HzFrameRateFmt ( mSigDetectThread.getCurSigInfo().fmt ) ) {
            freq = 50;
        }

        mpTvin->VDIN_SetDisplayVFreq ( freq, mHdmiOutFbc );
        LOGD ( "%s, SetDisplayVFreq %dHZ.", __FUNCTION__, freq);
    }
    //showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d

    if ( m_win_mode == PREVIEW_WONDOW ) {
        mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
        mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
    } else {
        SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( m_source_input ), m_source_input, mSigDetectThread.getCurSigInfo().fmt);
    }
    m_sig_stable_nums = 0;
    LOGD ( "[source_switch_time]: %fs, onSigToStable end", getUptimeSeconds());
}

void CTv::onSigStillStable()
{
    if ( m_sig_stable_nums == 20) {
        tvin_info_t info = mSigDetectThread.getCurSigInfo();
        TvEvent::SignalInfoEvent ev;
        ev.mTrans_fmt = info.trans_fmt;
        ev.mFmt = info.fmt;
        ev.mStatus = info.status;
        ev.mReserved = getHDMIFrameRate();
        sendTvEvent ( ev );
    }
    if (m_sig_stable_nums == 2) {
        LOGD("still stable , to start decoder");
        if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
            mpTvin->Tvin_StopDecoder();
            mpTvin->SwitchSnow( false );
        }
        int startdec_status = mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        if ( startdec_status == 0 ) { //showboz  codes from  start decode fun
            const char *value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DB_REG, "null" );
            if ( strcmp ( value, "enable" ) == 0 ) {
                usleep ( 20 * 1000 );
                Tvin_SetPLLValues ();
                usleep ( 20 * 1000 );
                SetCVD2Values ();
            }
        }
    }
    if ( m_sig_stable_nums == 3) {
        LOGD("still stable , to unmute audio/video");
        setAudioPreGain(m_source_input);
        CMessage msg;
        msg.mDelayMs = 0;
        msg.mType = CTvMsgQueue::TV_MSG_ENABLE_VIDEO_LATER;
        msg.mpPara[0] = 2;
        mTvMsgQueue.sendMsg ( msg );
        m_hdmi_audio_data = 0;
    }
    if (m_sig_stable_nums <= 20)
        m_sig_stable_nums++;
}

bool CTv::isTvViewBlocked() {
    char prop_value[PROPERTY_VALUE_MAX];
    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("persist.sys.tvview.blocked", prop_value, "false");
    LOGD("%s, persist.sys.tvview.blocked = %s", __FUNCTION__, prop_value);
    return (strcmp(prop_value, "true") == 0) ? true : false;
}

void CTv::onEnableVideoLater(int framecount)
{
    LOGD ( "[source_switch_time]: %fs, onEnableVideoLater start, wait %d video frame come out", getUptimeSeconds(), framecount);
    mAv.EnableVideoWhenVideoPlaying(framecount);
    if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
        if (isTvViewBlocked()) {
            SetAudioMuteForTv ( CC_AUDIO_MUTE );
        } else {
            SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
        }
        Tv_SetAudioInSource(m_source_input);
    }
    LOGD ( "[source_switch_time]: %fs, onEnableVideoLater end, show source on screen", getUptimeSeconds());
}

void CTv::onVideoAvailableLater(int framecount)
{
    LOGD("[ctv]onVideoAvailableLater framecount = %d", framecount);
    int ret = mAv.WaittingVideoPlaying(framecount);
    if (ret == 0) { //video available
        TvEvent::SignalInfoEvent ev;
        ev.mStatus = TVIN_SIG_STATUS_STABLE;
        ev.mTrans_fmt = TVIN_TFMT_2D;
        ev.mFmt = TVIN_SIG_FMT_NULL;
        ev.mReserved = 1;/*<< VideoAvailable flag here*/
        sendTvEvent ( ev );
    }
}

void CTv::Tv_SetAVOutPut_Input_gain(tv_source_input_t source_input)
{
    int nPgaValueIndex = 0;
    int nAdcValueIndex = 0;
    int nDdcValueIndex = 0;
    int tmpAvoutBufPtr[9];

    if (GetAvOutGainBuf_Cfg(tmpAvoutBufPtr) != 0) {
        GetDefaultAvOutGainBuf(tmpAvoutBufPtr);
    }

    switch (source_input) {
    case SOURCE_AV1:
    case SOURCE_AV2:
        nPgaValueIndex = 0;
        nAdcValueIndex = 1;
        nDdcValueIndex = 2;
        break;
    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:
    case SOURCE_DTV:
    case SOURCE_MPEG:
        nPgaValueIndex = 3;
        nAdcValueIndex = 4;
        nDdcValueIndex = 5;
        break;
    case SOURCE_TV:
        nPgaValueIndex = 6;
        nAdcValueIndex = 7;
        nDdcValueIndex = 8;
        break;
    default:
        break;
    }

    SetPGA_IN_Value(tmpAvoutBufPtr[nPgaValueIndex]);
    SetADC_Digital_Capture_Volume(tmpAvoutBufPtr[nAdcValueIndex]);
    SetDAC_Digital_PlayBack_Volume(tmpAvoutBufPtr[nDdcValueIndex]);
}

void CTv::onSigStableToUnstable()
{
    LOGD ( "%s, stable to unstable\n", __FUNCTION__);
    MnoNeedAutoSwitchToMonitorMode = false;
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        mAv.DisableVideoWithBlackColor();
        mpTvin->Tvin_StopDecoder();
    }
}

void CTv::onSigStableToUnSupport()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        mAv.DisableVideoWithBlackColor();
        mpTvin->Tvin_StopDecoder();
    }

    tvin_info_t info = mSigDetectThread.getCurSigInfo();
    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
    LOGD ( "%s, Enable blackscreen for signal change in StableToUnSupport!", __FUNCTION__ );
}

void CTv::onSigStableToNoSig()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }

    tvin_info_t info = mSigDetectThread.getCurSigInfo();
    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
    LOGD ( "%s, Enable bluescreen for signal change in StableToNoSig!", __FUNCTION__);
}

void CTv::onSigUnStableToUnSupport()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        mAv.DisableVideoWithBlackColor();
        mpTvin->Tvin_StopDecoder();
    }

    tvin_info_t info = mSigDetectThread.getCurSigInfo();

    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
    LOGD ( "%s, Enable blackscreen for signal change in UnStableToUnSupport!", __FUNCTION__);
}

void CTv::onSigUnStableToNoSig()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }

    tvin_info_t info = mSigDetectThread.getCurSigInfo();
    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
    LOGD ( "%s, Enable bluescreen for signal change in UnStableToNoSig! status = %d", __FUNCTION__, ev.mStatus );
}

void CTv::onSigNullToNoSig()
{
}

void CTv::onSigNoSigToUnstable()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }
    LOGD("Enable bluescreen for signal change in NoSigToUnstable\n");
}

void CTv::onSigStillUnstable()
{
}

void CTv::onSigStillNosig()
{
    LOGD("onSigStillNosig");
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    if ( (SOURCE_TV == m_source_input) && mATVDisplaySnow ) {
        mpTvin->SwitchSnow( true );
        mpTvin->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
        mAv.EnableVideoNow( false );
    } else {
        if ( iSBlackPattern ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        mpTvin->Tvin_StopDecoder();
    }

    tvin_info_t info = mSigDetectThread.getCurSigInfo();

    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
}

void CTv::onSigStillNoSupport()
{
    LOGD ( "%s, gxtvbb, don't send event when sitll not support", __FUNCTION__);
}

void CTv::onSigStillNull()
{
}

void CTv::onStableSigFmtChange()
{
    if ( mAutoSetDisplayFreq && !mPreviewEnabled) {
        if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
            int fps = getHDMIFrameRate();
            LOGD("onSigToStable HDMI fps get = %d", fps);
        }
        int freq = 60;
        if ( CTvin::Tvin_is50HzFrameRateFmt ( mSigDetectThread.getCurSigInfo().fmt ) ) {
            freq = 50;
        }

        mpTvin->VDIN_SetDisplayVFreq ( freq, mHdmiOutFbc );
        LOGD ( "%s, SetDisplayVFreq %dHZ.", __FUNCTION__, freq);
    }
    //showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d
    LOGD("hdmi trans_fmt = %d", mSigDetectThread.getCurSigInfo().trans_fmt);

    //load pq parameters
    CVpp::getInstance()->LoadVppSettings (m_source_input,
        mSigDetectThread.getCurSigInfo().fmt, INDEX_2D, mSigDetectThread.getCurSigInfo().trans_fmt);

    if ( m_win_mode == PREVIEW_WONDOW ) {
        mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
        mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
    } else {
        SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( m_source_input ), m_source_input, mSigDetectThread.getCurSigInfo().fmt);
    }
}

void CTv::onStableTransFmtChange()
{
    LOGD("onStableTransFmtChange trans_fmt = %d", mSigDetectThread.getCurSigInfo().trans_fmt);
}

void CTv::onSigDetectEnter()
{
}

void CTv::onSigDetectLoop()
{
    if (( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) ) {
        int sr = mpTvin->get_hdmi_sampling_rate();
        if ( ( sr > 0 ) && ( sr != m_hdmi_sampling_rate ) ) {
            LOGD("HDMI SR CHANGED");
            CMessage msg;
            msg.mDelayMs = 0;
            msg.mType = CTvMsgQueue::TV_MSG_HDMI_SR_CHANGED;
            ((int *)(msg.mpPara))[0] = sr;
            ((int *)(msg.mpPara))[1] = m_hdmi_sampling_rate;
            mTvMsgQueue.sendMsg ( msg );
            m_hdmi_sampling_rate = sr;
        }

        //m_hdmi_audio_data init is 0, not audio data , when switch to HDMI
        int hdmi_audio_data = mpTvin->TvinApi_GetHDMIAudioStatus();
        if (m_sig_stable_nums > 3 && hdmi_audio_data != m_hdmi_audio_data && sr > 0) {
            LOGD("HDMI  auds_rcv_sts CHANGED = %d", hdmi_audio_data);
            m_hdmi_audio_data = hdmi_audio_data;
            onHMDIAudioStatusChanged(hdmi_audio_data);
        }
    }else if (( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_SPDIF ) ) {
        if ( ( mAudioMuteStatusForTv == CC_AUDIO_MUTE ) && ( m_sig_spdif_nums ++ > 3 ) )
        {
            SetAudioMuteForSystem(CC_AUDIO_UNMUTE);
            SetAudioMuteForTv(CC_AUDIO_UNMUTE);
            LOGD("SPDIF UNMUTE");
        }
    }
}

CTv::CTvDetectObserverForScanner::CTvDetectObserverForScanner(CTv *pTv)
{
    mpTv = pTv;
    m_sig_stable_nums = 0;
}

void CTv::CTvDetectObserverForScanner::onSigStableToUnstable()
{
    LOGD ( "%s, stable to unstable\n", __FUNCTION__);
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);
    m_sig_stable_nums = 0;

    if ( (SOURCE_TV == mpTv->m_source_input) && mpTv->mATVDisplaySnow ) {
        mpTv->mpTvin->SwitchSnow( true );
        CTvin::getInstance()->Tvin_StartDecoder (mpTv->mSigDetectThread.getCurSigInfo() );
        mpTv->mAv.EnableVideoNow( false );
    } else {
        if ( mpTv->iSBlackPattern ) {
            mpTv->mAv.DisableVideoWithBlackColor();
        } else {
            mpTv->mAv.DisableVideoWithBlueColor();
        }
        CTvin::getInstance()->Tvin_StopDecoder();
    }
}

void CTv::CTvDetectObserverForScanner::onSigUnStableToNoSig()
{
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);

    if ( (SOURCE_TV == mpTv->m_source_input) && mpTv->mATVDisplaySnow ) {
        mpTv->mpTvin->SwitchSnow( true );
        CTvin::getInstance()->Tvin_StartDecoder (mpTv->mSigDetectThread.getCurSigInfo() );
        mpTv->mAv.EnableVideoNow( false );
    } else {
        if ( mpTv->iSBlackPattern ) {
            mpTv->mAv.DisableVideoWithBlackColor();
        } else {
            mpTv->mAv.DisableVideoWithBlueColor();
        }
        CTvin::getInstance()->Tvin_StopDecoder();
    }
}

void CTv::CTvDetectObserverForScanner::onSigStableToNoSig()
{
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);

    if ( (SOURCE_TV == mpTv->m_source_input) && mpTv->mATVDisplaySnow ) {
        mpTv->mpTvin->SwitchSnow( true );
        CTvin::getInstance()->Tvin_StartDecoder (mpTv->mSigDetectThread.getCurSigInfo() );
        mpTv->mAv.EnableVideoNow( false );
    } else {
        if ( mpTv->iSBlackPattern ) {
            mpTv->mAv.DisableVideoWithBlackColor();
        } else {
            mpTv->mAv.DisableVideoWithBlueColor();
        }
        CTvin::getInstance()->Tvin_StopDecoder();
    }
    LOGD ( "%s, Enable bluescreen for signal change in StableToNoSig!", __FUNCTION__);
}

void CTv::CTvDetectObserverForScanner::onSigToStable()
{
    mpTv->mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_16_9 );
    m_sig_stable_nums = 0;
}

void CTv::CTvDetectObserverForScanner::onSigStillStable()
{
    if (m_sig_stable_nums == 1) {
        LOGD("%s still stable , to start decoder", __FUNCTION__);
        if ( (SOURCE_TV == mpTv->m_source_input) && mpTv->mATVDisplaySnow ) {
            CTvin::getInstance()->Tvin_StopDecoder();
            CTvin::getInstance()->SwitchSnow( false );
            mpTv->mAv.EnableVideoNow( false );
        }
        int startdec_status = CTvin::getInstance()->Tvin_StartDecoder (mpTv->mSigDetectThread.getCurSigInfo() );
        if ( startdec_status == 0 ) {
            const char *value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DB_REG, "null" );
            if ( strcmp ( value, "enable" ) == 0 ) {
                usleep ( 20 * 1000 );
                mpTv->Tvin_SetPLLValues ();
                usleep ( 20 * 1000 );
                mpTv->SetCVD2Values ();
            }
        }
    }

    if (m_sig_stable_nums == 10) {
        if ( !mpTv->mATVDisplaySnow )
            mpTv->mAv.EnableVideoWhenVideoPlaying();
        TvEvent::ScanningFrameStableEvent ev;
        ev.CurScanningFreq = 0 ;
        mpTv->sendTvEvent ( ev );
    }
    m_sig_stable_nums++;
}

void CTv::onBootvideoRunning() {
    LOGD("%s,boot video is running", __FUNCTION__);
}

void CTv::onBootvideoStopped() {
    LOGD("%s,boot video has stopped", __FUNCTION__);
    SetAudioMasterVolume( GetAudioMasterVolume());
    mBootvideoStatusDetectThread->stopDetect();
    if (mpTvin->Tvin_RemovePath (TV_PATH_TYPE_TVIN) > 0) {
        mpTvin->VDIN_AddVideoPath(TV_PATH_VDIN_AMLVIDEO2_PPMGR_DEINTERLACE_AMVIDEO);
    }
}

void CTv::onSourceConnect(int source_type, int connect_status)
{
    TvEvent::SourceConnectEvent ev;
    ev.mSourceInput = source_type;
    ev.connectionState = connect_status;
    sendTvEvent(ev);
}

int CTv::GetSourceConnectStatus(tv_source_input_t source_input)
{
    return mSourceConnectDetectThread.GetSourceConnectStatus((tv_source_input_t)source_input);
}

tv_source_input_t CTv::GetCurrentSourceInputLock ( void )
{
    AutoMutex _l( mLock );
    return m_source_input;
}

tv_source_input_t CTv::GetCurrentSourceInputVirtualLock ( void )
{
    AutoMutex _l( mLock );
    return m_source_input_virtual;
}

//dtv and tvin
tvin_info_t CTv::GetCurrentSignalInfo ( void )
{
    tvin_trans_fmt det_fmt = TVIN_TFMT_2D;
    tvin_sig_status_t signalState = TVIN_SIG_STATUS_NULL;
    tvin_info_t signal_info = mSigDetectThread.getCurSigInfo();

    int feState = mFrontDev->getStatus();
    if ( (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_DTV ) ) {
        det_fmt = mpTvin->TvinApi_Get3DDectMode();
        if ((feState & FE_HAS_LOCK) == FE_HAS_LOCK) {
            signal_info.status = TVIN_SIG_STATUS_STABLE;
        } else if ((feState & FE_TIMEDOUT) == FE_TIMEDOUT) {
            signal_info.status = TVIN_SIG_STATUS_NOSIG;
        }
        if ( det_fmt != TVIN_TFMT_2D ) {
            signal_info.trans_fmt = det_fmt;
        }
        signal_info.fmt = mAv.getVideoResolutionToFmt();
    }
    return signal_info;
}

int CTv::Tvin_SetPLLValues ()
{
    tvin_sig_fmt_t sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);

    return CVpp::getInstance()->VPP_SetPLLValues(m_source_input, source_port, sig_fmt);
}

int CTv::SetCVD2Values ()
{
    tvin_sig_fmt_t sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);
    return CVpp::getInstance()->VPP_SetCVD2Values(m_source_input, source_port, sig_fmt);
}

int CTv::setPreviewWindowMode(bool mode)
{
    mPreviewEnabled = mode;
    return 0;
}

int CTv::SetPreviewWindow ( tvin_window_pos_t pos )
{
    m_win_pos.x1 = pos.x1;
    m_win_pos.y1 = pos.y1;
    m_win_pos.x2 = pos.x2;
    m_win_pos.y2 = pos.y2;
    LOGD ( "%s, SetPreviewWindow x = %d y=%d", __FUNCTION__, pos.x2, pos.y2 );

    tvin_window_pos_t def_pos;
    Vpp_GetDisplayResolutionInfo(&def_pos);

    if (pos.x1 != 0 || pos.y1 != 0 || pos.x2 != def_pos.x2 || pos.y2 != def_pos.y2) {
        m_win_mode = PREVIEW_WONDOW;
    } else {
        m_win_mode = NORMAL_WONDOW;
    }

    return mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
}

/*********************** Audio start **********************/
int CTv::SetAudioVolDigitLUTTable ( tv_source_input_t source_input )
{
    int tmpDefDigitLutBuf[CC_LUT_BUF_SIZE] = { 0 };
    int lut_table_index = 0;
    if (source_input == SOURCE_TV) {
        lut_table_index = CC_GET_LUT_TV;
    } else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
        lut_table_index = CC_GET_LUT_AV;
    } else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2) {
        lut_table_index = CC_GET_LUT_COMP;
    } else if (source_input == SOURCE_VGA) {
        lut_table_index = CC_GET_LUT_VGA;
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3
            || source_input == SOURCE_HDMI4) {
        lut_table_index = CC_GET_LUT_HDMI;
    } else if ( source_input == SOURCE_MPEG ) {
        lut_table_index = CC_GET_LUT_MPEG;
    } else if ( source_input == SOURCE_DTV ) {
        lut_table_index = CC_GET_LUT_MPEG;
    } else if ( source_input == SOURCE_MAX) {
        return 0;
    }
    char MainVolLutTableName[128];
    const char *baseName = GetAudioAmpMainvolTableBaseName(lut_table_index);
    strcpy(MainVolLutTableName, baseName);
    const char *dName = ".";
    strcat(MainVolLutTableName, dName);
    strcat(MainVolLutTableName, mMainVolLutTableExtraName);
    if ( GetAudioAmpMainvolBuf(MainVolLutTableName, tmpDefDigitLutBuf) == 0) {
        AudioSetVolumeDigitLUTBuf ( lut_table_index, tmpDefDigitLutBuf);
    }
    return 0;
}

void CTv::RefreshAudioMasterVolume ( tv_source_input_t source_input )
{
    if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3
        || source_input == SOURCE_HDMI4) {
        if ( GetAudioDVISupportEnable() == 1 ) {
            if ( IsDVISignal() ) {
                SetAudioVolDigitLUTTable ( SOURCE_MPEG );
                SetAudioMasterVolume ( GetAudioMasterVolume() );
                return;
            }
        }
    }

    SetAudioVolDigitLUTTable ( source_input );
    SetAudioMasterVolume ( GetAudioMasterVolume() );
}

int CTv::Tv_SetAudioInSource (tv_source_input_t source_input)
{
    int vol = 255;
    switch (source_input) {
    case SOURCE_TV:
        if (mpTvin->Tvin_GetAudioInSourceType(source_input) == TV_AUDIO_IN_SOURCE_TYPE_ATV) {
            AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_ATV);
            vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_ATV);
        } else {
            AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
            vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_LINEIN);
        }
        break;
    case SOURCE_SPDIF:
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_SPDIFIN);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_SPDIFIN);
        break;
    case SOURCE_AV1:
    case SOURCE_AV2:
    case SOURCE_YPBPR1:
    case SOURCE_YPBPR2:
    case SOURCE_VGA:
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_LINEIN);
        break;
    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:
    case SOURCE_MPEG:
    case SOURCE_DTV:
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_HDMI);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_HDMI);
        break;
    default:
        break;
    }
    LOGD("%s, we have read SetDAC_Digital_PlayBack_Volume = %d of source [%d].\n", __FUNCTION__, vol, source_input);
    return 0;
}

int CTv::Tv_SetAudioSourceType (tv_source_input_t source_input)
{
    int audio_source = -1;

    if (source_input == SOURCE_TV) {
        audio_source = AUDIO_ATV_SOURCE;
    } else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
        audio_source = AUDIO_AV_SOURCE;
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 ||
               source_input == SOURCE_HDMI3 || source_input == SOURCE_HDMI4) {
        audio_source = AUDIO_HDMI_SOURCE;
    } else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2 ||
               source_input == SOURCE_VGA) {
        audio_source = AUDIO_AV_SOURCE;
    } else if (source_input == SOURCE_DTV) {
        audio_source = AUDIO_MPEG_SOURCE;
    } else {
        audio_source = AUDIO_MPEG_SOURCE;
    }

    return AudioSetAudioSourceType(audio_source);
}

void CTv::Tv_SetAudioOutputSwap_Type (tv_source_input_t source_input)
{
    int sw_status = GetAudioOutputSwapStatus(source_input);
    SetOutput_Swap(sw_status);
}
/*********************** Audio end **********************/

unsigned int CTv::Vpp_GetDisplayResolutionInfo(tvin_window_pos_t *win_pos)
{
    int display_resolution = mAv.getVideoDisplayResolution();
    int width = 0, height = 0;
    switch (display_resolution) {
    case VPP_DISPLAY_RESOLUTION_1366X768:
        width = CC_RESOLUTION_1366X768_W;
        height = CC_RESOLUTION_1366X768_H;
        break;
    case VPP_DISPLAY_RESOLUTION_1920X1080:
        width = CC_RESOLUTION_1920X1080_W;
        height = CC_RESOLUTION_1920X1080_H;
        break;
    case VPP_DISPLAY_RESOLUTION_3840X2160:
        width = CC_RESOLUTION_3840X2160_W;
        height = CC_RESOLUTION_3840X2160_H;
        break;
    default:
        width = CC_RESOLUTION_3840X2160_W;
        height = CC_RESOLUTION_3840X2160_H;
        break;
    }

    if (win_pos != NULL) {
        win_pos->x1 = 0;
        win_pos->y1 = 0;
        win_pos->x2 = width - 1;
        win_pos->y2 = height - 1;
    }
    return 0;
}

int CTv::setBlackoutEnable(int enable)
{
    mBlackoutEnable = ((enable==1)?true:false);
    return SSMSaveBlackoutEnable(enable);
}

int CTv::getSaveBlackoutEnable()
{
    int8_t enable;
    SSMReadBlackoutEnable(&enable);
    return enable;
}

int CTv::setAutoBackLightStatus(int status)
{
    if (mHdmiOutFbc) {
        return mFactoryMode.fbcSetAutoBacklightOnOff(status);
    }

    return tvWriteSysfs(BL_LOCAL_DIMING_FUNC_ENABLE, status);
}

int CTv::getAutoBackLightStatus()
{
    if (mHdmiOutFbc) {
        return mFactoryMode.fbcGetAutoBacklightOnOff();
    }

    char str_value[3] = {0};
    tvReadSysfs(BL_LOCAL_DIMING_FUNC_ENABLE, str_value);
    int value = atoi(str_value);
    return (value < 0) ? -1 : value;
}

int CTv::getAverageLuma()
{
    return mpTvin->VDIN_Get_avg_luma();
}

int CTv::setAutobacklightData(const char *value)
{
    const char *KEY = "haier.autobacklight.mp.data";

    const char *keyValue = config_get_str(CFG_SECTION_TV, KEY, "null");
    if (strcmp(keyValue, value) != 0) {
        config_set_str(CFG_SECTION_TV, KEY, value);
        LOGD("%s, config string now set as: %s \n", __FUNCTION__, keyValue);
    } else {
        LOGD("%s, config string has been set as: %s \n", __FUNCTION__, keyValue);
    }
    return 0;
}

int CTv::getAutoBacklightData(int *data)
{
    char datas[512] = {0};
    char delims[] = ",";
    char *result = NULL;
    const char *KEY = "haier.autobacklight.mp.data";
    int i = 0;

    if (NULL == data) {
        LOGE("[ctv]%s, data is null", __FUNCTION__);
        return -1;
    }

    const char *keyValue = config_get_str(CFG_SECTION_TV, KEY, (char *) "null");
    if (strcmp(keyValue, "null") == 0) {
        LOGE("[ctv]%s, value is NULL\n", __FUNCTION__);
        return -1;
    }

    strncpy(datas, keyValue, sizeof(datas) - 1);
    result = strtok( datas, delims );
    while ( result != NULL ) {
        char *pd = strstr(result, ":");
        if (pd != NULL) {
            data[i] = strtol(pd + 1, NULL, 10 );
            i++;
        }
        result = strtok( NULL, delims );
    }
    return i;
}

int CTv::setLcdEnable(bool enable)
{
    int ret = -1;
    if (mHdmiOutFbc) {
        ret = fbcIns->fbcSetPanelStatus(COMM_DEV_SERIAL, enable ? 1 : 0);
    } else {
        ret = tvWriteSysfs(LCD_ENABLE, enable ? 1 : 0);
    }

    return ret;
}

/*********************** SSM  start **********************/
int CTv::Tv_SSMRestoreDefaultSetting()
{
    SSMRestoreDeviceMarkValues();
    AudioSSMRestoreDefaultSetting();
    CVpp::getInstance()->VPPSSMFacRestoreDefault();
    MiscSSMRestoreDefault();
    ReservedSSMRestoreDefault();
    SSMSaveCVBSStd ( 0 );
    SSMSaveLastSelectSourceInput ( SOURCE_TV );
    SSMSavePanelType ( 0 );
    //tvconfig default
    saveDTVProgramID ( -1 );
    saveATVProgramID ( -1 );
    SSMSaveStandbyMode( 0 );
    SSMHDMIEdidRestoreDefault();
    return 0;
}

int CTv::clearDbAllProgramInfoTable()
{
    return CTvDatabase::GetTvDb()->clearDbAllProgramInfoTable();
}

int CTv::Tv_SSMFacRestoreDefaultSetting()
{
    CVpp::getInstance()->VPPSSMFacRestoreDefault();
    AudioSSMRestoreDefaultSetting();
    MiscSSMFacRestoreDefault();
    return 0;
}
/*********************** SSM  End **********************/

//not in CTv, not use lock
void CTv::setSourceSwitchAndPlay()
{
    int progID = 0;
    CTvProgram prog;
    LOGD ( "%s\n", __FUNCTION__ );
    static const int POWERON_SOURCE_TYPE_NONE = 0;//not play source
    static const int POWERON_SOURCE_TYPE_LAST = 1;//play last save source
    static const int POWERON_SOURCE_TYPE_SETTING = 2;//play ui set source
    int to_play_source = -1;
    int powerup_type = SSMReadPowerOnOffChannel();
    LOGD("read power on source type = %d", powerup_type);
    if (powerup_type == POWERON_SOURCE_TYPE_NONE) {
        return ;
    } else if (powerup_type == POWERON_SOURCE_TYPE_LAST) {
        to_play_source = SSMReadSourceInput();
    } else if (powerup_type == POWERON_SOURCE_TYPE_SETTING) {
        to_play_source = SSMReadLastSelectSourceInput();
    }
    SetSourceSwitchInput (( tv_source_input_t ) to_play_source );
    if ( to_play_source == SOURCE_TV ) {
        progID = getATVProgramID();
    } else if ( to_play_source == SOURCE_DTV ) {
        progID = getDTVProgramID();
    }
}

//==============vchip end================================

//----------------DVR API============================
void CTv::SetRecordFileName ( char *name )
{
    mTvRec.SetRecordFileName (name);
}

void CTv::StartToRecord()
{
    int progID = getDTVProgramID();
    mTvRec.StartRecord ( progID );
}

void CTv::StopRecording()
{
    mTvRec.StopRecord();
}

int CTv::GetHdmiHdcpKeyKsvInfo(int data_buf[])
{
    int ret = -1;
    if (m_source_input != SOURCE_HDMI1 && m_source_input != SOURCE_HDMI2 && m_source_input != SOURCE_HDMI3
       &&m_source_input != SOURCE_HDMI4) {
        return -1;
    }

    struct _hdcp_ksv msg;
    ret = mHDMIRxManager.GetHdmiHdcpKeyKsvInfo(&msg);
    memset((void *)data_buf, 0, 2);
    data_buf[0] = msg.bksv0;
    data_buf[1] = msg.bksv1;
    return ret;
}

bool CTv::hdmiOutWithFbc()
{
    const char *value = config_get_str(CFG_SECTION_TV, CFG_FBC_USED, "true");
    if (strcmp(value, "true") == 0) {
        return true;
    }

    return false;
}

void CTv::onUpgradeStatus(int state, int param)
{
    TvEvent::UpgradeFBCEvent ev;
    ev.mState = state;
    ev.param = param;
    sendTvEvent(ev);
}

int CTv::StartUpgradeFBC(char *file_name, int mode, int upgrade_blk_size)
{
    return fbcIns->fbcStartUpgrade(file_name, mode, upgrade_blk_size);
}

int CTv::StartHeadSetDetect()
{
    mHeadSet.startDetect();
    return 0;
}

void CTv::onHeadSetDetect(int state, int para)
{
    TvEvent::HeadSetOf2d4GEvent ev;
    if (state == 1)
        property_set("audio.headset_plug.enable", "1");
    else
        property_set("audio.headset_plug.enable", "0");
    ev.state = state;
    ev.para = para;
    sendTvEvent(ev);
}

void CTv::onThermalDetect(int state)
{
    const char *value;
    int val = 0;
    static int preValue = -1;

    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_THRESHOLD_ENABLE, "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_THRESHOLD_VALUE, "null" );
        int threshold = atoi(value);
        LOGD ( "%s, threshold value: %d\n", __FUNCTION__, threshold);

        if (state > threshold) {
            const char *valueNormal = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_FBC_NORMAL_VALUE, "null" );
            val = atoi(valueNormal);
            if (val == 0) {
                val = 0x4210000;    //normal default
            }
            LOGD ( "%s, current temp: %d set 1\n", __FUNCTION__, state);
        } else {
            const char *valueCold = config_get_str ( CFG_SECTION_TV, CFG_TVIN_THERMAL_FBC_COLD_VALUE, "null" );
            val = atoi(valueCold);
            if (val == 0) {
                val = 0x8210000;    //cold default
            }
            LOGD ( "%s, current temp: 0x%x set 0\n", __FUNCTION__, state);
        }

        if (preValue != val) {
            LOGD ( "%s, pre value :0x%x, new value :0x%x, bypass\n", __FUNCTION__, preValue, val);
            preValue = val;
            mFactoryMode.fbcSetThermalState(val);
        }
    } else {
        LOGD ( "%s, tvin thermal threshold disable\n", __FUNCTION__);
    }
}

int CTv::Tv_GetProjectInfo(project_info_t *ptrInfo)
{
    return GetProjectInfo(ptrInfo, fbcIns);
}

int CTv::Tv_GetPlatformType()
{
    return mHdmiOutFbc ? 1 : 0;
}

int CTv::Tv_HDMIEDIDFileSelect(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version)
{
    char edid_path[100] = {0};
    char edid_path_cfg[100] = {0};

    config_set_str(CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, "customer_edid");
    if ( HDMI_EDID_VER_14== version ) {
        sprintf(edid_path, "/system/etc/port%d.bin", port);
    } else {
        sprintf(edid_path, "/system/etc/port%d_%d.bin", port,20);
    }
    sprintf(edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", port);
    if (access(edid_path, 0) < 0) {
        config_set_str(CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, "hdmi_edid");
    }
    config_set_str(CFG_SECTION_TV, edid_path_cfg, edid_path);
    return 0;
}

int CTv::Tv_HandeHDMIEDIDFilePathConfig()
{
    if (1 == GetSSMHandleHDMIEdidByCustomerEnableCFG()) {
        //set file's path for hdmi edid of each port
        for (int i = 1; i <= SSM_HDMI_PORT_MAX; i++) {
            Tv_HDMIEDIDFileSelect((tv_hdmi_port_id_t)i, SSMReadHDMIEdidMode((tv_hdmi_port_id_t)i));
        }
    }
    mSetHdmiEdid = true;
    return 0;
}

int CTv::Tv_SetDDDRCMode(tv_source_input_t source_input)
{
    if (source_input == SOURCE_DTV) {
        if (GetPlatformHaveDDFlag() == 1) {
            tvWriteSysfs(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 3");
        }
    } else {
        if (GetPlatformHaveDDFlag() == 1) {
            tvWriteSysfs(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 2");
        }
    }
    return 0;
}

//PQ
int CTv::Tv_SetBrightness ( int brightness, tv_source_input_t tv_source_input, int is_save )
{
    return CVpp::getInstance()->SetBrightness(brightness, tv_source_input,
        mSigDetectThread.getCurSigInfo().fmt,
        mSigDetectThread.getCurSigInfo().trans_fmt, INDEX_2D, is_save);
}

int CTv::Tv_GetBrightness ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetBrightness(tv_source_input);
}

int CTv::Tv_SaveBrightness ( int brightness, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveBrightness(brightness, tv_source_input);
}

int CTv::Tv_SetContrast ( int contrast, tv_source_input_t tv_source_input,  int is_save )
{
    return CVpp::getInstance()->SetContrast(contrast,  tv_source_input,
        mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, INDEX_2D, is_save);
}

int CTv::Tv_GetContrast ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetContrast(tv_source_input);
}

int CTv::Tv_SaveContrast ( int contrast, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveContrast(contrast, tv_source_input);
}

int CTv::Tv_SetSaturation ( int satuation, tv_source_input_t tv_source_input, tvin_sig_fmt_t fmt, int is_save )
{
    return CVpp::getInstance()->SetSaturation(satuation, tv_source_input,
        (tvin_sig_fmt_t)fmt, mSigDetectThread.getCurSigInfo().trans_fmt, INDEX_2D, is_save);
}

int CTv::Tv_GetSaturation ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetSaturation(tv_source_input);
}

int CTv::Tv_SaveSaturation ( int satuation, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveSaturation(satuation, tv_source_input);
}

int CTv::Tv_SetHue ( int hue, tv_source_input_t tv_source_input, tvin_sig_fmt_t fmt, int is_save )
{
    return CVpp::getInstance()->SetHue(hue, (tv_source_input_t)tv_source_input, (tvin_sig_fmt_t)fmt,
        mSigDetectThread.getCurSigInfo().trans_fmt, INDEX_2D, is_save);
}

int CTv::Tv_GetHue ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetHue(tv_source_input);
}

int CTv::Tv_SaveHue ( int hue, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveHue(hue, tv_source_input);
}

int CTv::processMonitorMode(tv_source_input_t tv_source_input, vpp_picture_mode_t mode) {
    vpp_picture_mode_t cur_mode = Tv_GetPQMode(tv_source_input);
    if (cur_mode == mode)//don't set again
        return 0;
    if ((tv_source_input < SOURCE_HDMI1) ||  (tv_source_input > SOURCE_HDMI4))//must source must be HDMI
        return -1;
    if (cur_mode == VPP_PICTURE_MODE_MONITOR) {
        SetAudioMuteForTv ( CC_AUDIO_MUTE );
        CVpp::getInstance()->enableMonitorMode(false);
        tvin_port_t cur_port = mpTvin->Tvin_GetSourcePortBySourceInput(m_source_input);
        mSigDetectThread.requestAndWaitPauseDetect();
        mpTvin->SwitchPort(cur_port);
        CVpp::getInstance()->Vpp_ResetLastVppSettingsSourceType();
        mSigDetectThread.initSigState();
        mSigDetectThread.setVdinNoSigCheckKeepTimes(150, true);
        mSigDetectThread.resumeDetect(1000);
    } else if (mode == VPP_PICTURE_MODE_MONITOR) {
        SetAudioMuteForTv ( CC_AUDIO_MUTE );
        CVpp::getInstance()->enableMonitorMode(true);
        tvin_port_t cur_port = mpTvin->Tvin_GetSourcePortBySourceInput(m_source_input);
        mSigDetectThread.requestAndWaitPauseDetect();
        mpTvin->SwitchPort(cur_port);
        mSigDetectThread.initSigState();
        mSigDetectThread.setVdinNoSigCheckKeepTimes(150, true);
        mSigDetectThread.resumeDetect(1000);
        return 0;
    }
    return -1;
}

int CTv::Tv_SetPQMode ( vpp_picture_mode_t mode, tv_source_input_t tv_source_input, int is_save )
{
    MnoNeedAutoSwitchToMonitorMode = true;
    if (processMonitorMode(tv_source_input, mode) == 0) {
        if (is_save) {
            CVpp::getInstance()->SavePQMode(mode, tv_source_input);
        }
        return 0;
    }
    return CVpp::getInstance()->SetPQMode((vpp_picture_mode_t)mode,
        (tv_source_input_t)tv_source_input, mSigDetectThread.getCurSigInfo().fmt,
        mSigDetectThread.getCurSigInfo().trans_fmt,
        INDEX_2D, is_save);
}

vpp_picture_mode_t CTv::Tv_GetPQMode ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetPQMode((tv_source_input_t)tv_source_input);
}

int CTv::Tv_SavePQMode ( vpp_picture_mode_t mode, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SavePQMode ( mode, tv_source_input );
}

int CTv::Tv_SetSharpness ( int value, tv_source_input_t tv_source_input, int en, int is_save )
{
    return CVpp::getInstance()->SetSharpness(value,
            (tv_source_input_t)tv_source_input, en,
            INDEX_2D,
            mSigDetectThread.getCurSigInfo().fmt,
            mSigDetectThread.getCurSigInfo().trans_fmt,
            is_save);
}

int CTv::Tv_GetSharpness ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetSharpness((tv_source_input_t)tv_source_input);
}

int CTv::Tv_SaveSharpness ( int value, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveSharpness(value, tv_source_input);
}

int CTv::Tv_SetBacklight ( int value, tv_source_input_t tv_source_input, int is_save )
{
    return CVpp::getInstance()->SetBacklight(value, (tv_source_input_t)tv_source_input, is_save);
}

int CTv::Tv_GetBacklight ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetBacklight((tv_source_input_t)tv_source_input);
}

int CTv::Tv_SaveBacklight ( int value, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveBacklight ( value, tv_source_input );
}

int CTv::Tv_SetBacklight_Switch ( int value )
{
    if (mHdmiOutFbc) {
        return mFactoryMode.fbcBacklightOnOffSet(value);
    } else {
        return CVpp::getInstance()->VPP_SetBackLight_Switch(value);
    }
}

int CTv::Tv_GetBacklight_Switch ( void )
{
    if (mHdmiOutFbc) {
        return mFactoryMode.fbcBacklightOnOffGet();
    } else {
        return CVpp::getInstance()->VPP_GetBackLight_Switch();
    }
}

int CTv::Tv_SetColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_t tv_source_input, int is_save )
{
    return CVpp::getInstance()->SetColorTemperature((vpp_color_temperature_mode_t)mode, (tv_source_input_t)tv_source_input, is_save);
}

vpp_color_temperature_mode_t CTv::Tv_GetColorTemperature ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetColorTemperature((tv_source_input_t)tv_source_input);
}

int CTv::Tv_SaveColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveColorTemperature ( mode, tv_source_input );
}

int CTv::Tv_SetDisplayMode ( vpp_display_mode_t mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t fmt, int is_save )
{
    int ret = SetDisplayMode((vpp_display_mode_t)mode, tv_source_input, (tvin_sig_fmt_t)fmt);
    //ret = SetDisplayMode(VPP_DISPLAY_MODE_PERSON, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt);
    if (ret == 0) {
        if (is_save == 1) {
            ret = ret | SSMSaveDisplayMode ( tv_source_input, (int)mode );
        }
    }
    return ret;
}

int CTv::SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_t tv_source_input, tvin_sig_fmt_t sig_fmt )
{
    tv_source_input_type_t source_type;
    source_type = CTvin::Tvin_SourceInputToSourceInputType(tv_source_input);
    LOGD("SetDisplayMode, display_mode = %d, source_type = %d fmt = %d  tranfmt = %d\n",  display_mode, source_type, sig_fmt, mSigDetectThread.getCurSigInfo().trans_fmt);

    tvin_cutwin_t cutwin = CVpp::getInstance()->GetOverscan ( tv_source_input, sig_fmt,
        INDEX_2D, mSigDetectThread.getCurSigInfo().trans_fmt);
    LOGD("SetDisplayMode , get crop %d %d %d %d \n", cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
    int video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;
    tvin_window_pos_t win_pos;
    int display_resolution = Vpp_GetDisplayResolutionInfo(&win_pos);

    switch ( display_mode ) {
    case VPP_DISPLAY_MODE_169:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;
        break;
    case VPP_DISPLAY_MODE_MODE43:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_4_3;
        break;
    case VPP_DISPLAY_MODE_NORMAL:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_NORMAL;
        break;
    case VPP_DISPLAY_MODE_FULL:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_NONLINEAR;
        CVpp::getInstance()->VPP_SetNonLinearFactor ( 20 );
        break;
    case VPP_DISPLAY_MODE_CROP_FULL:
        cutwin.vs = 0;
        cutwin.hs = 0;
        cutwin.ve = 0;
        cutwin.he = 0;
        break;
    case VPP_DISPLAY_MODE_NOSCALEUP:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_NORMAL_NOSCALEUP;
        break;
    case VPP_DISPLAY_MODE_FULL_REAL:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_16_9;    //added for N360 by haifeng.liu
        break;
    case VPP_DISPLAY_MODE_PERSON:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 20;
        cutwin.ve = cutwin.ve + 20;
        break;
    case VPP_DISPLAY_MODE_MOVIE:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 40;
        cutwin.ve = cutwin.ve + 40;
        break;
    case VPP_DISPLAY_MODE_CAPTION:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 55;
        cutwin.ve = cutwin.ve + 55;
        break;
    case VPP_DISPLAY_MODE_ZOOM:
        video_screen_mode = CAv::VIDEO_WIDEOPTION_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 70;
        cutwin.ve = cutwin.ve + 70;
        break;
    default:
        break;
    }
    if (source_type == SOURCE_TYPE_DTV) {
        cutwin.vs = cutwin.vs + 12;
        cutwin.ve = cutwin.ve + 12;
        cutwin.hs = cutwin.hs + 12;
        cutwin.he = cutwin.he + 12;
    }
    if (source_type == SOURCE_TYPE_HDMI) {
        if ((IsDVISignal()) || (mpTvin->GetITContent() == 1) ||
            (display_mode == VPP_DISPLAY_MODE_FULL_REAL)) {
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
        }
    }

    //mAv.setVideoAxis ( win_pos.x1, win_pos.y1, win_pos.x2, win_pos.y2 );
    mAv.setVideoScreenMode(video_screen_mode);
    CVpp::getInstance()->VPP_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
    return 0;
}

vpp_display_mode_t CTv::Tv_GetDisplayMode ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetDisplayMode((tv_source_input_t)tv_source_input);
}

int CTv::Tv_SaveDisplayMode ( vpp_display_mode_t mode, tv_source_input_t tv_source_input )
{
    return SSMSaveDisplayMode ( tv_source_input, (int)mode );
}

int CTv::setAudioPreGain(tv_source_input_t source_input)
{
    float pre_gain = getAudioPreGain(source_input);
    if (pre_gain > -100.000001 && pre_gain < -99.999999) {
        return -1;
    }

    return setAmAudioPreGain(pre_gain);
}

float CTv::getAudioPreGain(tv_source_input_t source_input)
{
    float pre_gain = -100;//default value is -100, if value of gain is -100, don't set it to AMAUDIO.
    switch (source_input) {
    case SOURCE_AV1:
    case SOURCE_AV2:
        pre_gain = config_get_float(CFG_SECTION_TV, CFG_AUDIO_PRE_GAIN_FOR_AV, -100);
        break;
    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
    case SOURCE_HDMI4:
        pre_gain = config_get_float(CFG_SECTION_TV, CFG_AUDIO_PRE_GAIN_FOR_HDMI, -100);
        break;
    case SOURCE_DTV:
        pre_gain = config_get_float(CFG_SECTION_TV, CFG_AUDIO_PRE_GAIN_FOR_DTV, -100);
        break;
    case SOURCE_MPEG:
        pre_gain = 0;
        break;
    case SOURCE_TV:
        pre_gain = config_get_float(CFG_SECTION_TV, CFG_AUDIO_PRE_GAIN_FOR_ATV, -100);
        break;
    default:
        break;
    }
    return pre_gain;
}

int CTv::setEyeProtectionMode(int enable)
{
    int ret = -1;
    if (getEyeProtectionMode() == enable)
        return ret;

    return CVpp::getInstance()->SetEyeProtectionMode(
        m_source_input, enable);
}

int CTv::getEyeProtectionMode()
{
    return CVpp::getInstance()->GetEyeProtectionMode();
}

int CTv::setGamma(vpp_gamma_curve_t gamma_curve, int is_save)
{
    if (mHdmiOutFbc) {
        return mFactoryMode.fbcSetGammaValue(gamma_curve, is_save);
    } else {
        return CVpp::getInstance()->SetGammaValue(gamma_curve, is_save);
    }
}

int CTv::Tv_SetNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_t tv_source_input, int is_save )
{
    return CVpp::getInstance()->SetNoiseReductionMode((vpp_noise_reduction_mode_t)mode,
        (tv_source_input_t)tv_source_input,
        mSigDetectThread.getCurSigInfo().fmt,
        INDEX_2D,
        mSigDetectThread.getCurSigInfo().trans_fmt, is_save);
}

int CTv::Tv_SaveNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->SaveNoiseReductionMode ( mode, tv_source_input );
}

vpp_noise_reduction_mode_t CTv::Tv_GetNoiseReductionMode ( tv_source_input_t tv_source_input )
{
    return CVpp::getInstance()->GetNoiseReductionMode((tv_source_input_t)tv_source_input);
}

//audio
void CTv::TvAudioOpen()
{
    SetAudioAVOutMute(CC_AUDIO_UNMUTE);
    SetAudioSPDIFMute(CC_AUDIO_UNMUTE);
    project_info_t tmp_info;
    if (GetProjectInfo(&tmp_info) == 0) {
        strncpy(mMainVolLutTableExtraName, tmp_info.amp_curve_name, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
    }
    openTvAudio();
}

void CTv::AudioCtlUninit()
{
    int oldMuteStatus = mAudioMuteStatusForTv;
    SetAudioMuteForTv(CC_AUDIO_MUTE);

    //AudioCtlUninit();
    AudioSetAudioInSource (CC_AUDIO_IN_SOURCE_HDMI);
    SetDAC_Digital_PlayBack_Volume(255);
    AudioSetAudioSourceType (AUDIO_MPEG_SOURCE);
    UnInitTvAudio();
    SetAudioVolumeCompensationVal(0);
    SetAudioMasterVolume(GetAudioMasterVolume());
    UnInitSetTvAudioCard();

    SetAudioMuteForTv(oldMuteStatus);
}

//audio
int CTv::SetAudioMuteForSystem(int muteOrUnmute)
{
    int ret = 0;
    LOGD("SetAudioMuteForSystem sysMuteStats=%d, tvMuteStatus=%d, toMute=%d",
        mAudioMuteStatusForSystem, mAudioMuteStatusForTv, muteOrUnmute);
    mAudioMuteStatusForSystem = muteOrUnmute;
    ret |= SetDacMute(mAudioMuteStatusForSystem, CC_DAC_MUTE_TYPE_EXTERNAL);
    ret |= SetAudioI2sMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv);
    return ret;
}

int CTv::GetAudioMuteForSystem()
{
    return mAudioMuteStatusForSystem;
}

int CTv::SetAudioMuteForTv(int muteOrUnmute)
{
    int ret = 0;
    mAudioMuteStatusForTv = muteOrUnmute;
    LOGD("SetAudioMuteForTv sysMuteStats=%d, tvMuteStatus=%d, toMute=%d",
        mAudioMuteStatusForSystem, mAudioMuteStatusForTv, muteOrUnmute);
    ret |= SetDacMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv, CC_DAC_MUTE_TYPE_EXTERNAL | CC_DAC_MUTE_TYPE_INTERNAL);
    ret |= SetAudioI2sMute(mAudioMuteStatusForTv);
    AudioSystem::setStreamMute(AUDIO_STREAM_MUSIC, mAudioMuteStatusForTv);
    return ret;
}

int CTv::GetAudioMuteForTv()
{
    return mAudioMuteStatusForTv;
}

int CTv::SetAudioSPDIFSwitch(int tmp_val)
{
    int muteStatus = CC_AUDIO_UNMUTE;

    SaveCurAudioSPDIFSwitch(tmp_val);

    if (tmp_val == CC_SWITCH_OFF /*|| mAudioMuteStatusForSystem == CC_AUDIO_MUTE || mAudioMuteStatusForTv == CC_AUDIO_MUTE*/) {
        muteStatus = CC_AUDIO_MUTE;
    } else {
        muteStatus = CC_AUDIO_UNMUTE;
    }

    SetAudioSPDIFMute(muteStatus);
    return 0;
}

void CTv::updateSubtitle(int pic_width, int pic_height)
{
    TvEvent::SubtitleEvent ev;
    ev.pic_width = pic_width;
    ev.pic_height = pic_height;
    sendTvEvent(ev);
}
//--------------------------------------------------


//Audio Mute
int CTv::SetAudioI2sMute(int muteStatus)
{
    int vol = 256;
    if (muteStatus == CC_AUDIO_MUTE) {
        vol = 0;
    } else {
        vol = 256;
    }
    CFile::setFileAttrValue(SYS_AUIDO_DIRECT_RIGHT_GAIN, vol);
    CFile::setFileAttrValue(SYS_AUIDO_DIRECT_LEFT_GAIN, vol);
    return 0;
}

int CTv::SetDacMute(int muteStatus, int mute_type)
{
    int tmp_ret = 0;
    if (mute_type & CC_DAC_MUTE_TYPE_INTERNAL) {
        tmp_ret |= mAudioAlsa.SetInternalDacMute(muteStatus);
    }

    if (mute_type & CC_DAC_MUTE_TYPE_EXTERNAL) {
        int set_val = 0;
        int aud_arch_type = GetAudioArchitectureTypeCFG();

        if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD) {
            if (muteStatus == CC_AUDIO_MUTE) {
                set_val = CC_MUTE_ON;
            } else if (muteStatus == CC_AUDIO_UNMUTE) {
                set_val = CC_MUTE_OFF;
            } else {
                return -1;
            }

            mAudioAlsa.SetExternalDacChannelSwitch(1, set_val);
            mAudioAlsa.SetExternalDacChannelSwitch(2, set_val);
            //showboz:  can disable it
            mAudioAlsa.SetExternalDacChannelSwitch(3, set_val);
        } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
            SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_MUTE, muteStatus);
        } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_CUSTOMER_LIB) {
            mCustomerCtrl.SetMute((muteStatus == CC_AUDIO_MUTE) ? CAudioCustomerCtrl::MUTE : CAudioCustomerCtrl::UNMUTE);
        } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_DIGITAL) {
            mAudioAlsa.SetDigitalMute(muteStatus);
        }
        mAudioAlsa.setAudioPcmPlaybackMute(muteStatus);
    }
    return tmp_ret;
}

int CTv::SetAudioAVOutMute(int muteStatus)
{
    SSMSaveAudioAVOutMuteVal(muteStatus);
    return mAudioAlsa.SetInternalDacMute(muteStatus);
}

int CTv::GetAudioAVOutMute()
{
    int8_t tmp_ch = 0;
    SSMReadAudioAVOutMuteVal(&tmp_ch);
    return tmp_ch;
}

int CTv::SetAudioSPDIFMute(int muteStatus)
{
    if (GetCurAudioSPDIFSwitch() == CC_SWITCH_OFF) {
        muteStatus = CC_AUDIO_MUTE;
    }

    SSMSaveAudioSPIDFMuteVal(muteStatus);
    return mAudioAlsa.SetSPDIFMute(muteStatus);
}

int CTv::GetAudioSPDIFMute()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSPIDFMuteVal(&tmp_ch);
    return tmp_ch;
}

int CTv::GetCurAudioSPDIFSwitch()
{
    return mCurAudioSPDIFSwitch;
}

int CTv::SaveCurAudioSPDIFSwitch(int tmp_val)
{
    mCurAudioSPDIFSwitch = tmp_val;
    SSMSaveAudioSPDIFSwitchVal(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSPDIFSwitch()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSPDIFSwitchVal(&tmp_ch);
    mCurAudioSPDIFSwitch = tmp_ch;
    if (mCurAudioSPDIFSwitch != CC_SWITCH_ON
            && mCurAudioSPDIFSwitch != CC_SWITCH_OFF) {
        SaveCurAudioSPDIFSwitch (CC_SWITCH_ON);
    }
    return mCurAudioSPDIFSwitch;
}

//Audio SPDIF Mode
int CTv::SetAudioSPDIFMode(int tmp_val)
{
    LOGD("%s : val = %d\n", __FUNCTION__, tmp_val);
    mCurAudioSPDIFMode = tmp_val;
    SetSPDIFMode(tmp_val);
    return 0;
}

int CTv::GetCurAudioSPDIFMode()
{
    return mCurAudioSPDIFMode;
}

int CTv::SaveCurAudioSPDIFMode(int tmp_val)
{
    mCurAudioSPDIFMode = tmp_val;
    SSMSaveAudioSPDIFModeVal(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSPDIFMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSPDIFModeVal(&tmp_ch);
    mCurAudioSPDIFMode = tmp_ch;
    if (mCurAudioSPDIFMode != CC_SPDIF_MODE_PCM
            && mCurAudioSPDIFMode != CC_SPDIF_MODE_RAW) {
        SaveCurAudioSPDIFMode (CC_SPDIF_MODE_PCM);
    }
    return mCurAudioSPDIFMode;
}

int CTv::SetAudioMasterVolume(int tmp_vol)
{
    LOGD("%s, tmp_vol = %d", __FUNCTION__, tmp_vol);
    mCustomAudioMasterVolume = tmp_vol;
    if (!isBootvideoStopped()) {
        mBootvideoStatusDetectThread->startDetect();
        return 0;
    }
    if (GetUseAndroidVolEnable()) {
        int master_vol;
        master_vol =  config_get_int(CFG_SECTION_TV, CFG_AUDIO_MASTER_VOL, 150);
        mAudioAlsa.SetExternalDacChannelVolume(0, master_vol);
        return 0;
    }

    //Volume Compensation
    tmp_vol += mVolumeCompensationVal;

    if (tmp_vol > CC_MAX_SOUND_VOL) {
        tmp_vol = CC_MAX_SOUND_VOL;
    }

    if (tmp_vol < CC_MIN_SOUND_VOL) {
        tmp_vol = CC_MIN_SOUND_VOL;
    }

    int tmp_ret = 0;
    int aud_arch_type = GetAudioArchitectureTypeCFG();

    if (aud_arch_type == CC_DAC_G9TV_INTERNAL_DAC) {
        tmp_ret = mAudioAlsa.SetInternalDacMainVolume(tmp_vol);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD) {
        int digit_vol = 0;
        int vol_gain_val = 0;
        int vol_buf[2] = {0, 0};

        //handle l&r channel volume for balance
        mAudioAlsa.CalculateBalanceVol(255, mMainVolumeBalanceVal, vol_buf);

        tmp_ret |= mAudioAlsa.SetExternalDacChannelVolume(1, vol_buf[0]);
        tmp_ret |= mAudioAlsa.SetExternalDacChannelVolume(2, vol_buf[1]);

        //handle master channel volume
        digit_vol = mAudioAlsa.TransVolumeBarVolToDigitalVol(mAudioAlsa.GetMainVolDigitLutBuf(), tmp_vol);

        vol_gain_val = mAudioAlsa.GetMainVolumeGain();
        digit_vol += vol_gain_val;
        tmp_ret |= mAudioAlsa.SetExternalDacChannelVolume(0, digit_vol);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
        tmp_ret = SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_VOLUME_BAR, tmp_vol);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_CUSTOMER_LIB) {
        tmp_ret = mCustomerCtrl.SetVolumeBar(tmp_vol);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_DIGITAL) {
        int vol_buf[2] = {0, 0};
        mAudioAlsa.CalculateBalanceVol(tmp_vol, mMainVolumeBalanceVal, vol_buf);
        tmp_ret = mAudioAlsa.SetDigitalMainVolume(vol_buf[0], vol_buf[1]);
    }
    if ( Get2d4gHeadsetEnable() == 1 ) {
        setAudioPcmPlaybackVolume(tmp_vol);
    }
    return 0;
}

int CTv::GetAudioMasterVolume()
{
    return mCustomAudioMasterVolume;
}

int CTv::GetCurAudioMasterVolume()
{
    return mCurAudioMasterVolume;
}

int CTv::SaveCurAudioMasterVolume(int tmp_vol)
{
    mCurAudioMasterVolume = tmp_vol;
    SSMSaveAudioMasterVolume(tmp_vol);
    return tmp_vol;
}

int CTv::LoadCurAudioMasterVolume()
{
    int8_t tmp_ch = 0;
    SSMReadAudioMasterVolume(&tmp_ch);
    mCurAudioMasterVolume = tmp_ch;
    if (mCurAudioMasterVolume < CC_MIN_SOUND_VOL
            || mCurAudioMasterVolume > CC_MAX_SOUND_VOL) {
        SaveCurAudioMasterVolume (CC_DEF_SOUND_VOL);
    }
    mCustomAudioMasterVolume = mCurAudioMasterVolume;

    return mCurAudioMasterVolume;
}

int CTv::SetAudioBalance(int tmp_val)
{
    mCustomAudioBalance = tmp_val;

    int aud_arch_type = GetAudioArchitectureTypeCFG();

    mMainVolumeBalanceVal = tmp_val;

    if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
        SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_BALANCE, mMainVolumeBalanceVal);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_CUSTOMER_LIB) {
        mCustomerCtrl.SetBlance(mMainVolumeBalanceVal);
    } else {
        int tmp_ret = 0;
        int vol_buf[2] = {0, 0};
        mAudioAlsa.CalculateBalanceVol(255, mMainVolumeBalanceVal, vol_buf);

        tmp_ret |= mAudioAlsa.SetExternalDacChannelVolume(1, vol_buf[0]);
        tmp_ret |= mAudioAlsa.SetExternalDacChannelVolume(2, vol_buf[1]);
    }
    return 0;
}

int CTv::GetAudioBalance()
{
    return mCustomAudioBalance;
}

int CTv::GetCurAudioBalance()
{
    return mCurAudioBalance;
}

int CTv::SaveCurAudioBalance(int tmp_val)
{
    mCurAudioBalance = tmp_val;
    SSMSaveAudioBalanceVal(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioBalance()
{
    int8_t tmp_ch = 0;
    SSMReadAudioBalanceVal(&tmp_ch);
    mCurAudioBalance = tmp_ch;
    if (mCurAudioBalance < CC_MIN_SOUND_BALANCE_VAL
            || mCurAudioBalance > CC_MAX_SOUND_BALANCE_VAL) {
        SaveCurAudioBalance (CC_DEF_SOUND_BALANCE_VAL);
    }

    mCustomAudioBalance = mCurAudioBalance;

    return mCurAudioBalance;
}

int CTv::SetAudioVolumeCompensationVal(int tmp_vol_comp_val)
{
    mVolumeCompensationVal = tmp_vol_comp_val;
    LOGD("%s, new vol comp value = %d.\n", __FUNCTION__, tmp_vol_comp_val);
    return mVolumeCompensationVal;
}

int CTv::SetAudioSupperBassVolume(int tmp_vol)
{
    mCustomAudioSupperBassVolume = tmp_vol;

    int aud_arch_type = GetAudioArchitectureTypeCFG();
    int tmp_ret = 0;

    if (aud_arch_type == CC_DAC_G9TV_INTERNAL_DAC) {
        return 0;
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD) {
        int digit_vol = 0;
        int vol_gain_val = 0;

        digit_vol = mAudioAlsa.TransVolumeBarVolToDigitalVol(mAudioAlsa.GetMainVolDigitLutBuf(), tmp_vol);

        vol_gain_val = mAudioAlsa.GetSupperBassVolumeGain();
        digit_vol += vol_gain_val;
        if (digit_vol < CC_MIN_DAC_SUB_WOOFER_VOLUME) {
            digit_vol = CC_MIN_DAC_SUB_WOOFER_VOLUME;
        } else if (digit_vol > CC_MAX_DAC_SUB_WOOFER_VOLUME) {
            digit_vol = CC_MAX_DAC_SUB_WOOFER_VOLUME;
        }

        tmp_ret = mAudioAlsa.SetExternalDacChannelVolume(3, digit_vol);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
        tmp_ret = SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_SUBCHANNEL_VOLUME, tmp_vol);
    }

    return tmp_ret;
}

int CTv::GetAudioSupperBassVolume()
{
    return mCustomAudioSupperBassVolume;
}

int CTv::GetCurAudioSupperBassVolume()
{
    return mCurAudioSupperBassVolume;
}

int CTv::SaveCurAudioSupperBassVolume(int tmp_vol)
{
    mCurAudioSupperBassVolume = tmp_vol;
    SSMSaveAudioSupperBassVolume(tmp_vol);

    return tmp_vol;
}

int CTv::LoadCurAudioSupperBassVolume()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSupperBassVolume(&tmp_ch);
    mCurAudioSupperBassVolume = tmp_ch;
    if (mCurAudioSupperBassVolume < CC_MIN_SUPPERBASS_VOL
            || mCurAudioSupperBassVolume > CC_MAX_SUPPERBASS_VOL) {
        SaveCurAudioSupperBassVolume (CC_DEF_SUPPERBASS_VOL);
    }
    mCustomAudioSupperBassVolume = mCurAudioSupperBassVolume;

    return mCurAudioSupperBassVolume;
}

int CTv::SetAudioSupperBassSwitch(int tmp_val)
{
    mCustomAudioSupperBassSwitch = tmp_val;

    if (GetAudioSupperBassSwitch() == CC_SWITCH_OFF) {
        return SetAudioSupperBassVolume(CC_MIN_SUPPERBASS_VOL);
    }

    return SetAudioSupperBassVolume(GetAudioSupperBassVolume());
}

int CTv::GetAudioSupperBassSwitch()
{
    if (GetAudioSupperBassSwitchDisableCFG() != 0) {
        return CC_SWITCH_ON;
    }

    return mCustomAudioSupperBassSwitch;
}

int CTv::GetCurAudioSupperBassSwitch()
{
    if (GetAudioSupperBassSwitchDisableCFG() != 0) {
        return CC_SWITCH_ON;
    }

    return mCurAudioSupperBassSwitch;
}

int CTv::SaveCurAudioSupperBassSwitch(int tmp_val)
{
    mCurAudioSupperBassSwitch = tmp_val;
    SSMSaveAudioSupperBassSwitch(tmp_val);
    SetSupperBassSRSSpeakerSize();
    return tmp_val;
}

int CTv::LoadCurAudioSupperBassSwitch()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSupperBassSwitch(&tmp_ch);
    mCurAudioSupperBassSwitch = tmp_ch;
    if (mCurAudioSupperBassSwitch != CC_SWITCH_ON
            && mCurAudioSupperBassSwitch != CC_SWITCH_OFF) {
        SaveCurAudioSupperBassSwitch (CC_SWITCH_OFF);
    }
    mCustomAudioSupperBassSwitch = mCurAudioSupperBassSwitch;

    return mCurAudioSupperBassSwitch;
}

void CTv::SetSupperBassSRSSpeakerSize()
{
    int tmp_speakersize = -1;

    if (GetAudioSrsTruBass() == CC_SWITCH_ON) {
        tmp_speakersize = GetAudioSRSSupperBassTrubassSpeakerSizeCfg();
        if (tmp_speakersize >= 0) {
            mAudioEffect.SetSrsTrubassSpeakerSize(tmp_speakersize);
        }
    }
}

int CTv::SetAudioSRSSurround(int tmp_val)
{
    mCustomAudioSRSSurround = tmp_val;
    RefreshSrsEffectAndDacGain();
    return 0;
}

int CTv::GetAudioSRSSurround()
{
    return mCustomAudioSRSSurround;
}

int CTv::GetCurAudioSRSSurround()
{
    return mCurAudioSRSSurround;
}

int CTv::SaveCurAudioSrsSurround(int tmp_val)
{
    mCurAudioSRSSurround = tmp_val;
    SSMSaveAudioSRSSurroundSwitch(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSrsSurround()
{
    int8_t tmp_ch = 0;

    SSMReadAudioSRSSurroundSwitch(&tmp_ch);
    mCurAudioSRSSurround = tmp_ch;
    if (mCurAudioSRSSurround != CC_SWITCH_ON
            && mCurAudioSRSSurround != CC_SWITCH_OFF) {
        SaveCurAudioSrsSurround (CC_SWITCH_OFF);
    }
    mCustomAudioSRSSurround = mCurAudioSRSSurround;

    return mCurAudioSRSSurround;
}

int CTv::SetAudioSrsDialogClarity(int tmp_val)
{
    mCustomAudioSrsDialogClarity = tmp_val;
    RefreshSrsEffectAndDacGain();
    return 0;
}

int CTv::GetAudioSrsDialogClarity()
{
    return mCustomAudioSrsDialogClarity;
}

int CTv::GetCurAudioSrsDialogClarity()
{
    return mCurAudioSrsDialogClarity;
}

int CTv::SaveCurAudioSrsDialogClarity(int tmp_val)
{
    mCurAudioSrsDialogClarity = tmp_val;
    SSMSaveAudioSRSDialogClaritySwitch(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSrsDialogClarity()
{
    int8_t tmp_ch = 0;

    SSMReadAudioSRSDialogClaritySwitch(&tmp_ch);
    mCurAudioSrsDialogClarity = tmp_ch;
    if (mCurAudioSrsDialogClarity != CC_SWITCH_ON
            && mCurAudioSrsDialogClarity != CC_SWITCH_OFF) {
        SaveCurAudioSrsDialogClarity (CC_SWITCH_OFF);
    }
    mCustomAudioSrsDialogClarity = mCurAudioSrsDialogClarity;

    return mCurAudioSrsDialogClarity;
}

int CTv::SetAudioSrsTruBass(int tmp_val)
{
    mCustomAudioSrsTruBass = tmp_val;
    RefreshSrsEffectAndDacGain();
    return 0;
}

int CTv::GetAudioSrsTruBass()
{
    return mCustomAudioSrsTruBass;
}

int CTv::GetCurAudioSrsTruBass()
{
    return mCurAudioSrsTruBass;
}

int CTv::SaveCurAudioSrsTruBass(int tmp_val)
{
    mCurAudioSrsTruBass = tmp_val;
    SSMSaveAudioSRSTruBassSwitch(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSrsTruBass()
{
    int8_t tmp_ch = 0;

    SSMReadAudioSRSTruBassSwitch(&tmp_ch);
    mCurAudioSrsTruBass = tmp_ch;
    if (mCurAudioSrsTruBass != CC_SWITCH_ON
            && mCurAudioSrsTruBass != CC_SWITCH_OFF) {
        SaveCurAudioSrsTruBass (CC_SWITCH_OFF);
    }
    mCustomAudioSrsTruBass = mCurAudioSrsTruBass;

    return mCurAudioSrsTruBass;
}

void CTv::RefreshSrsEffectAndDacGain()
{
    int tmp_gain_val = 0;
    int surround_switch = CC_SWITCH_OFF;
    int trubass_switch = CC_SWITCH_OFF;
    int dialogclarity_switch = CC_SWITCH_OFF;
    trubass_switch = GetAudioSrsTruBass();
    dialogclarity_switch = GetAudioSrsDialogClarity();
    surround_switch = GetAudioSRSSurround();

    if (GetAudioSRSSourroundEnableCFG() == 0) {
        return;
    }

    if (surround_switch == CC_SWITCH_ON) {
        mAudioEffect.SetSrsSurroundSwitch(CC_SWITCH_ON);
        tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_SOURROUND_GAIN, 50);
        mAudioEffect.SetSrsSurroundGain(tmp_gain_val);

        int input_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_INPUT_GAIN, 50);
        int out_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_OUTPUT_GAIN, 50);
        mAudioEffect.SetSrsInputOutputGain(input_gain_val, out_gain_val);

        if (trubass_switch == CC_SWITCH_ON
                && dialogclarity_switch == CC_SWITCH_OFF) {

            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_ON);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_TRUBASS_GAIN, 50);
            mAudioEffect.SetSrsTruBassGain(tmp_gain_val);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_TRUBASS_SPEAKERSIZE, 2);
            mAudioEffect.SetSrsTrubassSpeakerSize(tmp_gain_val);

            mAudioEffect.SetSrsDialogClaritySwitch (CC_SWITCH_OFF);

        } else if (trubass_switch == CC_SWITCH_OFF
                   && dialogclarity_switch == CC_SWITCH_ON) {

            mAudioEffect.SetSrsDialogClaritySwitch (CC_SWITCH_ON);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_CLARITY_GAIN, 30);
            mAudioEffect.SetSrsDialogClarityGain(tmp_gain_val);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_DEFINITION_GAIN, 20);
            mAudioEffect.SetSrsDefinitionGain(tmp_gain_val);

            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_OFF);

        } else if (trubass_switch == CC_SWITCH_ON
                   && dialogclarity_switch == CC_SWITCH_ON) {

            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_ON);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_TRUBASS_GAIN, 50);
            mAudioEffect.SetSrsTruBassGain(tmp_gain_val);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_TRUBASS_SPEAKERSIZE, 2);
            mAudioEffect.SetSrsTrubassSpeakerSize(tmp_gain_val);

            mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_ON);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_CLARITY_GAIN, 30);
            mAudioEffect.SetSrsDialogClarityGain(tmp_gain_val);
            tmp_gain_val = GetAudioSRSGainCfg(CFG_AUDIO_SRS_DEFINITION_GAIN, 20);
            mAudioEffect.SetSrsDefinitionGain(tmp_gain_val);

        } else if (trubass_switch == CC_SWITCH_OFF
                   && dialogclarity_switch == CC_SWITCH_OFF) {
            mAudioEffect.SetSrsTruBassSwitch (CC_SWITCH_OFF);
            mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_OFF);
        }
        SetSupperBassSRSSpeakerSize();
    } else {
        mAudioEffect.SetSrsSurroundSwitch (CC_SWITCH_OFF);
        mAudioEffect.SetSrsTruBassSwitch(CC_SWITCH_OFF);
        mAudioEffect.SetSrsDialogClaritySwitch(CC_SWITCH_OFF);
    }
    //Refesh DAC gain
    int main_gain_val = 0;
    if (surround_switch == CC_SWITCH_ON) {
        main_gain_val = GetAudioEffectAmplifierGainCfg(CFG_AUDIO_SRS_SOURROUND_MASTER_GAIN, 6, 24);
        if (dialogclarity_switch == CC_SWITCH_ON
                && trubass_switch == CC_SWITCH_OFF) {
            main_gain_val = GetAudioEffectAmplifierGainCfg(CFG_AUDIO_SRS_CLARITY_MASTER_GAIN, 6, 24);
        } else if (dialogclarity_switch == CC_SWITCH_OFF
                   && trubass_switch == CC_SWITCH_ON) {
            main_gain_val = GetAudioEffectAmplifierGainCfg(CFG_AUDIO_SRS_TRUBASS_MASTER_GAIN, 6, 24);
        } else if (dialogclarity_switch == CC_SWITCH_ON
                   && trubass_switch == CC_SWITCH_ON) {
            main_gain_val = GetAudioEffectAmplifierGainCfg(CFG_AUDIO_SRS_TRUBASS_CLARITY_MASTER_GAIN, 6, 24);
        }
    }
    mAudioAlsa.SetMainVolumeGain(main_gain_val);
}

int CTv::SetAudioVirtualizer(int enable, int EffectLevel)
{
    config_set_int(CFG_SECTION_TV, CFG_AUDIO_VIRTUAL_ENABLE, enable);
    config_set_int(CFG_SECTION_TV, CFG_AUDIO_VIRTUAL_LEVEL, EffectLevel);
    return mAudioEffect.SetAudioVirtualizer(enable, EffectLevel);
}

int CTv::GetAudioVirtualizerEnable()
{
    return   config_get_int(CFG_SECTION_TV, CFG_AUDIO_VIRTUAL_ENABLE, 0);
}

int CTv::GetAudioVirtualizerLevel()
{
    return    config_get_int(CFG_SECTION_TV, CFG_AUDIO_VIRTUAL_LEVEL, 100);
}

int CTv::LoadAudioVirtualizer()
{
    return SetAudioVirtualizer(GetAudioVirtualizerEnable(), GetAudioVirtualizerLevel());
}

int CTv::SetAudioBassVolume(int tmp_vol) {
    int nMinBassVol = 0, nMaxBassVol = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    if (tmp_vol < nMinBassVol || tmp_vol > nMaxBassVol) {
        tmp_vol = (nMaxBassVol + nMinBassVol) / 2;
    }

    mCustomAudioBassVolume = tmp_vol;
    tmp_vol = MappingTrebleBassAndEqualizer(GetAudioBassVolume(), 0,
                                            nMinBassVol, nMaxBassVol);
    return SetSpecialIndexEQGain(CC_EQ_BASS_IND, tmp_vol);
}

int CTv::GetAudioBassVolume()
{
    return mCustomAudioBassVolume;
}

int CTv::GetCurAudioBassVolume()
{
    return mCurAudioBassVolume;
}

int CTv::SaveCurAudioBassVolume(int tmp_vol)
{
    int nMinBassVol = 0, nMaxBassVol = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    if (tmp_vol < nMinBassVol || tmp_vol > nMaxBassVol) {
        tmp_vol = (nMaxBassVol + nMinBassVol) / 2;
    }

    RealSaveCurAudioBassVolume(tmp_vol, 1);

    tmp_vol = MappingTrebleBassAndEqualizer(GetCurAudioBassVolume(), 0,
                                            nMinBassVol, nMaxBassVol);
    return SaveSpecialIndexEQGain(CC_EQ_BASS_IND, tmp_vol);
}

int CTv::RealSaveCurAudioBassVolume(int tmp_vol, int sound_mode_judge)
{
    mCurAudioBassVolume = tmp_vol;
    SSMSaveAudioBassVolume(tmp_vol);

    if (sound_mode_judge == 1) {
        if (GetAudioSoundMode() != CC_SOUND_MODE_USER) {
            SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
            mCustomAudioSoundMode = mCurAudioSoundMode;
        }
    }
    return mCurAudioBassVolume;
}

int CTv::LoadCurAudioBassVolume()
{
    int nMinBassVol = 0, nMaxBassVol = 0;
    int8_t tmp_ch = 0;

    nMinBassVol = GetBassUIMinGainVal();
    nMaxBassVol = GetBassUIMaxGainVal();

    SSMReadAudioBassVolume(&tmp_ch);
    mCurAudioBassVolume = tmp_ch;
    if (mCurAudioBassVolume < nMinBassVol
            || mCurAudioBassVolume > nMaxBassVol) {
        RealSaveCurAudioBassVolume((nMaxBassVol + nMinBassVol) / 2, 0);
    }

    mCustomAudioBassVolume = mCurAudioBassVolume;
    return mCurAudioBassVolume;
}

int CTv::SetAudioTrebleVolume(int tmp_vol)
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    if (tmp_vol < nMinTrebleVol || tmp_vol > nMaxTrebleVol) {
        tmp_vol = (nMaxTrebleVol + nMinTrebleVol) / 2;
    }

    mCustomAudioTrebleVolume = tmp_vol;

    tmp_vol = MappingTrebleBassAndEqualizer(GetAudioTrebleVolume(), 0,
                                            nMinTrebleVol, nMaxTrebleVol);
    return SetSpecialIndexEQGain(CC_EQ_TREBLE_IND, tmp_vol);
}

int CTv::GetAudioTrebleVolume()
{
    return mCustomAudioTrebleVolume;
}

int CTv::GetCurAudioTrebleVolume()
{
    return mCurAudioTrebleVolume;
}

int CTv::SaveCurAudioTrebleVolume(int tmp_vol)
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    if (tmp_vol < nMinTrebleVol || tmp_vol > nMaxTrebleVol) {
        tmp_vol = (nMaxTrebleVol + nMinTrebleVol) / 2;
    }

    RealSaveCurAudioTrebleVolume(tmp_vol, 1);

    tmp_vol = MappingTrebleBassAndEqualizer(GetCurAudioTrebleVolume(), 0,
                                            nMinTrebleVol, nMaxTrebleVol);
    return SaveSpecialIndexEQGain(CC_EQ_TREBLE_IND, tmp_vol);
}

int CTv::RealSaveCurAudioTrebleVolume(int tmp_vol, int sound_mode_judge)
{
    mCurAudioTrebleVolume = tmp_vol;
    SSMSaveAudioTrebleVolume(tmp_vol);

    if (sound_mode_judge == 1) {
        if (GetAudioSoundMode() != CC_SOUND_MODE_USER) {
            SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
            mCustomAudioSoundMode = mCurAudioSoundMode;
        }
    }

    return mCurAudioTrebleVolume;
}

int CTv::LoadCurAudioTrebleVolume()
{
    int nMinTrebleVol = 0, nMaxTrebleVol = 0;
    int8_t tmp_ch = 0;

    nMinTrebleVol = GetTrebleUIMinGainVal();
    nMaxTrebleVol = GetTrebleUIMaxGainVal();

    SSMReadAudioTrebleVolume(&tmp_ch);
    mCurAudioTrebleVolume = tmp_ch;
    if (mCurAudioTrebleVolume < nMinTrebleVol
            || mCurAudioTrebleVolume > nMaxTrebleVol) {
        RealSaveCurAudioTrebleVolume((nMaxTrebleVol + nMinTrebleVol) / 2, 0);
    }

    mCustomAudioTrebleVolume = mCurAudioTrebleVolume;
    return mCurAudioTrebleVolume;
}

int CTv::SetAudioSoundMode(int tmp_val)
{
    mCustomAudioSoundMode = tmp_val;
    SetSpecialModeEQGain(mCustomAudioSoundMode);

    HandleTrebleBassVolume();
    return 0;
}

int CTv::GetAudioSoundMode()
{
    return mCustomAudioSoundMode;
}

int CTv::GetCurAudioSoundMode()
{
    return mCurAudioSoundMode;
}

int CTv::SaveCurAudioSoundMode(int tmp_val)
{
    mCurAudioSoundMode = tmp_val;
    SSMSaveAudioSoundModeVal(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioSoundMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioSoundModeVal(&tmp_ch);
    mCurAudioSoundMode = tmp_ch;
    if (mCurAudioSoundMode < CC_SOUND_MODE_START
            || mCurAudioSoundMode > CC_SOUND_MODE_END) {
        SaveCurAudioSoundMode (CC_SOUND_MODE_STD);
    }
    mCustomAudioSoundMode = mCurAudioSoundMode;
    return mCurAudioSoundMode;
}

int CTv::HandleTrebleBassVolume()
{
    int tmp_vol = 0;
    int tmpEQGainBuf[128] = { 0 };
    int8_t tmp_ch = 0;

    GetCustomEQGain(tmpEQGainBuf);

    tmp_vol = MappingTrebleBassAndEqualizer(tmpEQGainBuf[CC_EQ_TREBLE_IND], 1,
                                            GetTrebleUIMinGainVal(), GetTrebleUIMaxGainVal());
    mCustomAudioTrebleVolume = tmp_vol;
    mCurAudioTrebleVolume = mCustomAudioTrebleVolume;
    tmp_ch = mCustomAudioTrebleVolume;
    SSMSaveAudioTrebleVolume(tmp_ch);

    tmp_vol = MappingTrebleBassAndEqualizer(tmpEQGainBuf[CC_EQ_BASS_IND], 1,
                                            GetBassUIMinGainVal(), GetBassUIMaxGainVal());
    mCustomAudioBassVolume = tmp_vol;
    mCurAudioBassVolume = mCustomAudioBassVolume;
    tmp_ch = mCustomAudioBassVolume;
    SSMSaveAudioBassVolume(tmp_ch);
    return 0;
}

int CTv::SetAudioWallEffect(int tmp_val)
{
    int tmp_treble_val;
    int tmp_type = 0;

    mCustomAudioWallEffect = tmp_val;

    tmp_type = GetAudioWallEffectTypeCfg();
    if (tmp_type == 0) {
        SetCustomEQGain();
    } else if (tmp_type == 1) {
        int aud_arch_type = GetAudioArchitectureTypeCFG();
        if (aud_arch_type == CC_DAC_G9TV_INTERNAL_DAC) {
            return 0;
        } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD) {
            mAudioAlsa.SetExternalDacEQMode(tmp_val);
        } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
            SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_EQ_MODE, tmp_val);
        }
    }
    return 0;
}

int CTv::GetAudioWallEffect()
{
    return mCustomAudioWallEffect;
}

int CTv::GetCurAudioWallEffect()
{
    return mCurAudioWallEffect;
}

int CTv::SaveCurAudioWallEffect(int tmp_val)
{
    mCurAudioWallEffect = tmp_val;
    SSMSaveAudioWallEffectSwitch(tmp_val);
    return tmp_val;
}

int CTv::LoadCurAudioWallEffect()
{
    int8_t tmp_ch = 0;
    SSMReadAudioWallEffectSwitch(&tmp_ch);
    mCurAudioWallEffect = tmp_ch;
    if (mCurAudioWallEffect != CC_SWITCH_ON
            && mCurAudioWallEffect != CC_SWITCH_OFF) {
        SaveCurAudioWallEffect (CC_SWITCH_OFF);
    }

    mCustomAudioWallEffect = mCurAudioWallEffect;
    return mCurAudioWallEffect;
}

int CTv::SetAudioEQMode(int tmp_val)
{
    mCustomAudioEQMode = tmp_val;
    return 0;
}

int CTv::GetAudioEQMode()
{
    return mCustomAudioEQMode;
}

int CTv::GetCurAudioEQMode()
{
    return mCurAudioEQMode;
}

int CTv::SaveCurAudioEQMode(int tmp_val)
{
    mCurAudioEQMode = tmp_val;
    SSMSaveAudioEQModeVal(tmp_val);

    return tmp_val;
}

int CTv::LoadCurAudioEQMode()
{
    int8_t tmp_ch = 0;
    SSMReadAudioEQModeVal(&tmp_ch);
    mCurAudioEQMode = tmp_ch;
    if (mCurAudioEQMode < CC_EQ_MODE_START
            || mCurAudioEQMode > CC_EQ_MODE_END) {
        SaveCurAudioEQMode (CC_EQ_MODE_START);
    }
    mCustomAudioEQMode = mCurAudioEQMode;

    return mCurAudioEQMode;
}

int CTv::GetAudioEQRange(int range_buf[])
{
    range_buf[0] = CC_MIN_EQ_GAIN_VAL;
    range_buf[1] = CC_MAX_EQ_GAIN_VAL;
    return 0;
}

int CTv::GetAudioEQBandCount()
{
    return mAudioEffect.GetEQBandCount();
}

int CTv::SetAudioEQGain(int gain_buf[])
{
    return AudioSetEQGain(gain_buf);
}

int CTv::GetAudioEQGain(int gain_buf[])
{
    return GetCustomEQGain(gain_buf);
}

int CTv::GetCurAudioEQGain(int gain_buf[])
{
    RealReadCurAudioEQGain(gain_buf);
    return 0;
}

int CTv::SaveCurAudioEQGain(int gain_buf[])
{
    return RealSaveCurAudioEQGain(gain_buf, 1);
}

int CTv::RealReadCurAudioEQGain(int gain_buf[])
{
    ArrayCopy(gain_buf, mCurEQGainBuf, GetAudioEQBandCount());
    return 0;
}

int CTv::RealSaveCurAudioEQGain(int gain_buf[], int sound_mode_judge)
{
    ArrayCopy(mCurEQGainBuf, gain_buf, GetAudioEQBandCount());
    ArrayCopy(mCurEQGainChBuf, gain_buf, GetAudioEQBandCount());
    SSMSaveAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);

    if (sound_mode_judge == 1) {
        HandleTrebleBassVolume();
        SaveCurAudioSoundMode (CC_SOUND_MODE_USER);
        mCustomAudioSoundMode = mCurAudioSoundMode;
    }

    return 0;
}

int CTv::LoadCurAudioEQGain()
{
    SSMReadAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);
    ArrayCopy(mCurEQGainBuf, mCurEQGainChBuf, GetAudioEQBandCount());

    for (int i = 0; i < GetAudioEQBandCount(); i++) {
        if (mCurEQGainBuf[i] & 0x80) {
            mCurEQGainBuf[i] = mCurEQGainBuf[i] - 256;
        }
    }
    return 0;
}

int CTv::SetAudioEQSwitch(int switch_val)
{
    return mAudioEffect.SetEQSwitch(switch_val);
}

int CTv::GetBassUIMinGainVal()
{
    return 0;
}

int CTv::GetBassUIMaxGainVal()
{
    return 100;
}

int CTv::GetTrebleUIMinGainVal()
{
    return 0;
}

int CTv::GetTrebleUIMaxGainVal()
{
    return 100;
}

int CTv::MappingLine(int map_val, int src_min, int src_max, int dst_min, int dst_max)
{
    if (dst_min < 0) {
        return (map_val - (src_max + src_min) / 2) * (dst_max - dst_min)
               / (src_max - src_min);
    } else {
        return (map_val - src_min) * (dst_max - dst_min) / (src_max - src_min);
    }
}

int CTv::MappingTrebleBassAndEqualizer(int tmp_vol, int direct, int tb_min, int tb_max)
{
    int tmp_ret = 0;

    if (direct == 0) {
        tmp_ret = MappingLine(tmp_vol, tb_min, tb_max, CC_EQ_DEF_UI_MIN_GAIN, CC_EQ_DEF_UI_MAX_GAIN);
    } else {
        tmp_ret = MappingLine(tmp_vol, CC_EQ_DEF_UI_MIN_GAIN, CC_EQ_DEF_UI_MAX_GAIN, tb_min, tb_max);
    }

    LOGD("%s, tmp_vol = %d, direct = %d, tmp_ret = %d\n", __FUNCTION__, tmp_vol,
         direct, tmp_ret);

    return tmp_ret;
}

int CTv::MappingEQGain(int src_gain_buf[], int dst_gain_buf[], int direct)
{
    int i = 0;
    int nMinUIVal = 0, nMaxUIVal = 0, nMinVal = 0, nMaxVal = 0;

    nMinUIVal = CC_EQ_DEF_UI_MIN_GAIN;
    nMaxUIVal = CC_EQ_DEF_UI_MAX_GAIN;
    nMinVal = CC_MIN_EQ_GAIN_VAL;
    nMaxVal = CC_MAX_EQ_GAIN_VAL;

    if (nMinUIVal >= nMinVal && nMaxUIVal <= nMaxVal) {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_gain_buf[i] = src_gain_buf[i];
        }
    } else {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            if (direct == 0) {
                dst_gain_buf[i] = MappingLine(src_gain_buf[i], nMinUIVal,
                                              nMaxUIVal, nMinVal, nMaxVal);
            } else {
                dst_gain_buf[i] = MappingLine(src_gain_buf[i], nMinVal, nMaxVal,
                                              nMinUIVal, nMaxUIVal);
            }
        }
    }
    return 0;
}

int CTv::RestoreToAudioDefEQGain(int gain_buf[])
{
    int i = 0;

    for (i = 0; i < GetAudioEQBandCount(); i++) {
        gain_buf[i] = (CC_EQ_DEF_UI_MAX_GAIN + CC_EQ_DEF_UI_MIN_GAIN) / 2;
    }

    ArrayCopy(mCurEQGainBuf, gain_buf, GetAudioEQBandCount());
    ArrayCopy(mCurEQGainChBuf, gain_buf, GetAudioEQBandCount());
    SSMSaveAudioEQGain(0, GetAudioEQBandCount(), mCurEQGainChBuf);

    HandleTrebleBassVolume();
    SaveCurAudioSoundMode (CC_SOUND_MODE_STD);
    mCustomAudioSoundMode = mCurAudioSoundMode;
    return 0;
}

int CTv::GetCustomEQGain(int gain_buf[])
{
    ArrayCopy(gain_buf, mCustomEQGainBuf, GetAudioEQBandCount());
    return 0;
}

int CTv::SetCustomEQGain()
{
    int tmpEQGainBuf[128] = { 0 };

    if (MappingEQGain(mCustomEQGainBuf, tmpEQGainBuf, 0) < 0) {
        return -1;
    }

    return RealSetEQGain(tmpEQGainBuf);
}

int CTv::AudioSetEQGain(int gain_buf[])
{
    int tmpEQGainBuf[128] = { 0 };

    ArrayCopy(mCustomEQGainBuf, gain_buf, GetAudioEQBandCount());

    if (MappingEQGain(mCustomEQGainBuf, tmpEQGainBuf, 0) < 0) {
        return -1;
    }

    return RealSetEQGain(tmpEQGainBuf);
}

int CTv::handleEQGainBeforeSet(int src_buf[], int dst_buf[])
{
    int i = 0, nMinGain, nMaxGain;

    nMinGain = CC_MIN_EQ_GAIN_VAL;
    nMaxGain = CC_MAX_EQ_GAIN_VAL;

    if (GetAudioWallEffect() == CC_SWITCH_ON && GetAudioWallEffectTypeCfg() == 0) {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_buf[i] = WALL_EFFECT_VALUE[i] + src_buf[i];

            if (dst_buf[i] < nMinGain) {
                dst_buf[i] = nMinGain;
            }

            if (dst_buf[i] > nMaxGain) {
                dst_buf[i] = nMaxGain;
            }
        }
    } else {
        for (i = 0; i < GetAudioEQBandCount(); i++) {
            dst_buf[i] = src_buf[i];
        }
    }

    return 0;
}

int CTv::RealSetEQGain(int gain_buf[])
{
    if (GetAudioWallEffect() == CC_SWITCH_ON && GetAudioWallEffectTypeCfg() == 0) {
        for (int i = 0; i < GetAudioEQBandCount(); i++) {
            gain_buf[i] = WALL_EFFECT_VALUE[i] + gain_buf[i];

            if (gain_buf[i] < CC_MIN_EQ_GAIN_VAL) {
                gain_buf[i] = CC_MIN_EQ_GAIN_VAL;
            }

            if (gain_buf[i] > CC_MAX_EQ_GAIN_VAL) {
                gain_buf[i] = CC_MAX_EQ_GAIN_VAL;
            }
        }
    }

    mAudioEffect.SetEQValue(gain_buf);
    return 0;
}

int CTv::SetAtvInGain(int gain_val)
{
    char set_str[32] = {0};

    sprintf ( set_str, "audio_gain_set %x", gain_val );
    return tvWriteSysfs ( SYS_ATV_DEMOD_DEBUG, set_str );
}

int CTv::SetSpecialModeEQGain(int tmp_val)
{
    int tmpEQPresetBufPtr[24];
    if (GetAudioEQPresetBufferPtr(tmpEQPresetBufPtr) != 0) {
        GetDefault_EQGain_Table(tmpEQPresetBufPtr);
    }
    int tmpEQGainBuf[128] = { 0 };

    if (tmp_val == CC_SOUND_MODE_USER) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
    } else {
        ArrayCopy(tmpEQGainBuf,
                  tmpEQPresetBufPtr + tmp_val * GetAudioEQBandCount(),
                  GetAudioEQBandCount());
    }

    AudioSetEQGain(tmpEQGainBuf);
    return tmp_val;
}

int CTv::SetSpecialIndexEQGain(int buf_index, int w_val)
{
    int tmpEQGainBuf[128] = { 0 };

    if (buf_index >= 0 && buf_index < GetAudioEQBandCount()) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
        tmpEQGainBuf[buf_index] = w_val;

        return AudioSetEQGain(tmpEQGainBuf);
    }
    return -1;
}

int CTv::SaveSpecialIndexEQGain(int buf_index, int w_val)
{
    int tmpEQGainBuf[128] = { 0 };

    if (buf_index >= 0 && buf_index < GetAudioEQBandCount()) {
        RealReadCurAudioEQGain(tmpEQGainBuf);
        tmpEQGainBuf[buf_index] = w_val;

        return RealSaveCurAudioEQGain(tmpEQGainBuf, 0);
    }

    return 0;
}

// amAudio
int CTv::OpenAmAudio(unsigned int sr, int input_device, int output_device)
{
    LOGD("OpenAmAudio input_device = %d", input_device);
    return amAudioOpen(sr, input_device, output_device);
}

int CTv::CloseAmAudio(void)
{
    return amAudioClose();
}

int CTv::setAmAudioPreMute(int mute)
{
    int ret = -1;
    if (m_source_input == SOURCE_DTV) {
        ret = mAv.AudioSetPreMute(mute);
    } else {
        ret = amAudioSetPreMute(mute);
    }
    return ret;
}

int CTv::getAmAudioPreMute()
{
    uint mute = -1;
    if (m_source_input == SOURCE_DTV) {
        mAv.AudioGetPreMute(&mute);
    } else {
        amAudioGetPreMute(&mute);
    }
    return mute;
}

int CTv::setAmAudioPreGain(float pre_gain)
{
    int ret = -1;
    if (m_source_input == SOURCE_DTV) {
        ret = mAv.AudioSetPreGain(pre_gain);
    } else {
        ret = amAudioSetPreGain(pre_gain);
    }
    return ret;
}

float CTv::getAmAudioPreGain()
{
    float pre_gain = -1;
    if (m_source_input == SOURCE_DTV) {
        mAv.AudioGetPreGain(&pre_gain);
    } else {
        amAudioGetPreGain(&pre_gain);
    }
    return pre_gain;
}

int CTv::SetAmAudioInputSr(unsigned int sr, int output_device)
{
    LOGD("SetAmAudioInputSr");
    return amAudioSetInputSr(sr, CC_IN_USE_SPDIF_DEVICE, output_device);
}

int CTv::SetAmAudioOutputMode(int mode)
{
    if (mode != CC_AMAUDIO_OUT_MODE_DIRECT && mode != CC_AMAUDIO_OUT_MODE_INTER_MIX
            && mode != CC_AMAUDIO_OUT_MODE_DIRECT_MIX) {
        LOGE("[ctv]%s, mode error, it should be mix or direct!\n", __FUNCTION__);
        return -1;
    }

    return amAudioSetOutputMode(mode);
}

int CTv::SetAmAudioMusicGain(int gain)
{
    return amAudioSetMusicGain(gain);
}

int CTv::SetAmAudioLeftGain(int gain)
{
    return amAudioSetLeftGain(gain);
}

int CTv::SetAmAudioRightGain(int gain)
{
    return amAudioSetRightGain(gain);
}

int CTv::SetAudioDumpDataFlag(int tmp_flag)
{
    return amAudioSetDumpDataFlag(tmp_flag);
}

int CTv::GetAudioDumpDataFlag()
{
    return amAudioGetDumpDataFlag();
}

void CTv::AudioSetVolumeDigitLUTBuf(int lut_table_index, int *MainVolLutBuf)
{
    int tmpDefDigitLutBuf[CC_LUT_BUF_SIZE] = { 0 };
    mAudioAlsa.SetMainVolDigitLutBuf(MainVolLutBuf);

    GetAudioAmpSupbassvolBuf(lut_table_index, tmpDefDigitLutBuf);
    mAudioAlsa.SetSupperBassVolDigitLutBuf(tmpDefDigitLutBuf);
}

int CTv::InitTvAudio(int sr, int input_device)
{
    OpenAmAudio(sr, input_device, CC_OUT_USE_AMAUDIO);

    RefreshSrsEffectAndDacGain();
    SetCustomEQGain();
    LoadAudioVirtualizer();
    return 0;
}

int CTv::UnInitTvAudio()
{
    return CloseAmAudio();
}

int CTv::AudioChangeSampleRate(int sr)
{
    sr = HanldeAudioInputSr(sr);

    if (SetAmAudioInputSr(sr, CC_OUT_USE_AMAUDIO) != 0) {
        return -1;
    }

    RefreshSrsEffectAndDacGain();
    SetCustomEQGain();
    return 0;
}

int CTv::AudioSetAudioInSource(int audio_src_in_type)
{
    return mAudioAlsa.SetAudioInSource(audio_src_in_type);
}

int CTv::AudioSetAudioSourceType(int source_type)
{
    int aud_arch_type = GetAudioArchitectureTypeCFG();

    if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_ON_BOARD) {
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_OFF_BOARD_FBC) {
        SendCmdToOffBoardFBCExternalDac(AUDIO_CMD_SET_SOURCE, source_type);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_CUSTOMER_LIB) {
        mCustomerCtrl.SetSource(source_type);
    } else if (aud_arch_type == CC_DAC_G9TV_EXTERNAL_DAC_DIGITAL) {
    }
    return 0;
}

int CTv::AudioLineInSelectChannel(int audio_channel)
{
    LOGD ("%s, audio_channel = %d", __FUNCTION__, audio_channel);
    mAudioAlsa.SetInternalDacLineInSelectChannel(audio_channel);
    return 0;
}

int CTv::AudioSetLineInCaptureVolume(int l_vol, int r_vol)
{
    mAudioAlsa.SetInternalDacLineInCaptureVolume(l_vol, r_vol);
    return 0;
}

int CTv::openTvAudio()
{
    int tmp_val = 0;

    LOGD("%s, entering...\n", __FUNCTION__);
    UnInitSetTvAudioCard();

    tmp_val = GetAudioDumpDataEnableFlagCfg();
    SetAudioDumpDataFlag(tmp_val);

    tmp_val = GetAudioInternalDacPGAInGain_Cfg();
    mAudioAlsa.SetAudioInternalDacPGAInGain(tmp_val, tmp_val);

    mAudioAlsa.SetMixerBypassSwitch (CC_SWITCH_OFF);
    mAudioAlsa.SetMixerDacSwitch (CC_SWITCH_ON);

    LoadAudioCtl();

    RefreshSrsEffectAndDacGain();
    InitSetAudioCtl();
    return 0;
}

void CTv::LoadAudioCtl()
{
    // Get Current Audio Volume
    LoadCurAudioMasterVolume();

    // Get Current Audio Balance
    LoadCurAudioBalance();

    // Get Current Supper Bass Switch
    LoadCurAudioSupperBassSwitch();

    // Get Current Supper Bass Volume
    LoadCurAudioSupperBassVolume();

    // Get Current SRSSurround
    LoadCurAudioSrsSurround();

    // Get Current SRS DialogClarity
    LoadCurAudioSrsDialogClarity();

    // Get Current SRS TruBass
    LoadCurAudioSrsTruBass();

    // Get Current Audio Sound Mode
    LoadCurAudioSoundMode();

    // Get Current Audio Bass and Treble
    LoadCurAudioBassVolume();
    LoadCurAudioTrebleVolume();

    // Get Current Wall Effect
    LoadCurAudioWallEffect();

    // Get Current spdif switch
    LoadCurAudioSPDIFSwitch();

    // Get Current spdif mode
    LoadCurAudioSPDIFMode();

    // Get Current EQ mode
    LoadCurAudioEQMode();

    // Get Current EQ Gain
    LoadCurAudioEQGain();

    //Get Current Virtual Effect status
    LoadAudioVirtualizer();
}

bool CTv::isBootvideoStopped() {
    char prop_value[PROPERTY_VALUE_MAX];
    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("init.svc.bootvideo", prop_value, "null");
    LOGD("%s, init.svc.bootvideo = %s", __FUNCTION__, prop_value);
    return (strcmp(prop_value, "stopped") == 0) ? true : false;
}

void CTv::InitSetAudioCtl()
{
    // Set Current Audio balance value
    SetAudioBalance(GetAudioBalance());

    // Set Current Audio Volume
    SetAudioMasterVolume(GetAudioMasterVolume());

    // Set Current Supper Bass Volume
    SetAudioSupperBassVolume(GetAudioSupperBassVolume());

    // Set Current Audio Sound Mode
    SetAudioSoundMode(GetAudioSoundMode());

    // Set Current Audio SPDIF Switch
    SetAudioSPDIFSwitch(GetCurAudioSPDIFSwitch());

    // Set Current Audio SPDIF mode
    SetAudioSPDIFMode(GetCurAudioSPDIFMode());
}

int CTv::SetADC_Digital_Capture_Volume(int value)
{
    return mAudioAlsa.SetAudioInternalDacADCDigitalCaptureVolume( value, value);
}

int CTv::SetPGA_IN_Value(int value)
{
    return mAudioAlsa.SetAudioInternalDacPGAInGain( value, value);
}

int CTv::SetDAC_Digital_PlayBack_Volume(int value)
{
    return mAudioAlsa.SetAudioInternalDacDACDigitalPlayBackVolume( value, value);
}

int CTv::setAudioPcmPlaybackVolume(int val)
{
    int pcm_volume = 0;
    pcm_volume = val / 2;
    if (pcm_volume > 24) pcm_volume = 24;
    //return SetAudioPcmPlaybackVolume(pcm_volume);
    return 0;
}

int CTv::HanldeAudioInputSr(unsigned int sr)
{
    int tmp_cfg = 0;

    tmp_cfg = GetAudioResampleTypeCFG();
    if (tmp_cfg & CC_AUD_RESAMPLE_TYPE_HW) {
        mAudioAlsa.SetHardwareResample(sr);
    } else {
        mAudioAlsa.SetHardwareResample(-1);
    }

    if (!(tmp_cfg & CC_AUD_RESAMPLE_TYPE_SW)) {
        sr = 48000;
    }

    return sr;
}

int CTv::AudioSSMRestoreDefaultSetting()
{
    int i = 0, tmp_val = 0;
    int nMinUIVol = 0, nMaxUIVol = 0;
    int *tmp_ptr = NULL;
    int tmpEQGainBuf[128] = { 0 };
    unsigned char tmp_buf[CC_NO_LINE_POINTS_MAX_CNT] = { 0 };

    // Save Current Audio Volume
    SaveCurAudioMasterVolume (CC_DEF_SOUND_VOL);

    // Save Current Audio Balance
    SaveCurAudioBalance (CC_DEF_SOUND_BALANCE_VAL);

    // Save Current Supper Bass Switch
    SaveCurAudioSupperBassSwitch (CC_SWITCH_OFF);

    // Save Current Supper Bass Volume
    SaveCurAudioSupperBassVolume (CC_DEF_SUPPERBASS_VOL);

    // Save Current SRSSurround
    SaveCurAudioSrsSurround(CC_SWITCH_OFF);

    // Save Current SRS DialogClarity
    SaveCurAudioSrsDialogClarity(CC_SWITCH_OFF);

    // Save Current SRS TruBass
    SaveCurAudioSrsTruBass(CC_SWITCH_OFF);

    // Save Current Audio Sound Mode
    SaveCurAudioSoundMode (CC_SOUND_MODE_STD);

    // Save Current Wall Effect
    SaveCurAudioWallEffect(CC_SWITCH_OFF);

    // Save Current spdif switch
    SaveCurAudioSPDIFSwitch (CC_SWITCH_ON);

    // Save Current spdif mode
    SaveCurAudioSPDIFMode (CC_SPDIF_MODE_PCM);

    // Save Current avout and spidif mute state
    SSMSaveAudioSPIDFMuteVal(CC_AUDIO_MUTE);
    SSMSaveAudioAVOutMuteVal(CC_AUDIO_MUTE);

    // Save Current EQ mode
    SaveCurAudioEQMode (CC_EQ_MODE_NOMAL);

    // Save Current EQ Gain
    RestoreToAudioDefEQGain(tmpEQGainBuf);

    // Save Current Audio Bass and Treble
    nMinUIVol = GetBassUIMinGainVal();
    nMaxUIVol = GetBassUIMaxGainVal();
    RealSaveCurAudioBassVolume((nMinUIVol + nMaxUIVol) / 2, 0);

    nMinUIVol = GetTrebleUIMinGainVal();
    nMaxUIVol = GetTrebleUIMaxGainVal();
    RealSaveCurAudioTrebleVolume((nMinUIVol + nMaxUIVol) / 2, 0);
    return 0;
}

int CTv::InitSetTvAudioCard()
{
    int captureIdx = -1;
    char buf[32] = { 0 };
    char propValue[PROPERTY_VALUE_MAX];

#ifndef BOARD_ALSA_AUDIO_TINY
    snd_card_refresh_info();
#endif

    if (GetTvAudioCardNeedSet()) {
        int totleNum = 0;
        char cardName[64] = { 0 };

        GetTvAudioCardName(cardName);
        /*
        memset(propValue, '\0', PROPERTY_VALUE_MAX);
        property_get(PROP_AUDIO_CARD_NUM, propValue, "0");

        totleNum = strtoul(propValue, NULL, 10);
        LOGD("%s, totle number = %d\n", __FUNCTION__, totleNum);
        */
        totleNum = 8;

        for (int i = 0; i < totleNum; i++) {
            sprintf(buf, "snd.card.%d.name", i);
            memset(propValue, '\0', PROPERTY_VALUE_MAX);
            property_get(buf, propValue, "null");

            LOGD("%s, key string \"%s\", value string \"%s\".\n", __FUNCTION__, buf, propValue);
            if (strcmp(propValue, cardName) == 0) {
                captureIdx = i;
                break;
            }
        }
    }

    sprintf(buf, "%d", captureIdx);
    property_set(PROP_DEF_CAPTURE_NAME, buf);
    LOGD("%s, set \"%s\" as \"%s\".\n", __FUNCTION__, PROP_DEF_CAPTURE_NAME, buf);
    return 0;
}

int CTv::UnInitSetTvAudioCard()
{
#ifndef BOARD_ALSA_AUDIO_TINY
    snd_card_refresh_info();
#endif
    property_set(PROP_DEF_CAPTURE_NAME, "-1");
    LOGD("%s, set [%s]:[-1]\n", __FUNCTION__, PROP_DEF_CAPTURE_NAME);
    return 0;
}

int CTv::SetSPDIFMode(int mode_val)
{
    tvWriteSysfs(SYS_SPDIF_MODE_DEV_PATH, mode_val);
    return 0;
}

int CTv::SwitchAVOutBypass(int sw)
{
    if (sw == 0 ) {
        mAudioAlsa.SetMixerBypassSwitch ( CC_SWITCH_OFF );
        mAudioAlsa.SetMixerDacSwitch ( CC_SWITCH_ON );
    } else {
        mAudioAlsa.SetMixerBypassSwitch ( CC_SWITCH_ON );
        mAudioAlsa.SetMixerDacSwitch ( CC_SWITCH_OFF );
    }
    return 0;
}

int CTv::SetAudioSwitchIO(int value)
{
    return mAudioAlsa.SetAudioSwitchIO( value);
}

int CTv::SetOutput_Swap(int value)
{
    return mAudioAlsa.SetOutput_Swap( value);
}

int CTv::SendCmdToOffBoardFBCExternalDac(int cmd, int para)
{
    int set_val = 0;
    CFbcCommunication *pFBC = GetSingletonFBC();
    if (pFBC != NULL) {
        if (cmd == AUDIO_CMD_SET_MUTE) {
            if (para == CC_AUDIO_MUTE) {
                set_val = CC_MUTE_ON;
            } else if (para == CC_AUDIO_UNMUTE) {
                set_val = CC_MUTE_OFF;
            } else {
                return -1;
            }

            return pFBC->cfbc_Set_Mute(COMM_DEV_SERIAL, set_val);
        } else if (cmd == AUDIO_CMD_SET_VOLUME_BAR) {
            LOGD("%s, send AUDIO_CMD_SET_VOLUME_BAR (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_Volume_Bar(COMM_DEV_SERIAL, para);
        } else if (cmd == AUDIO_CMD_SET_BALANCE) {
            LOGD("%s, send AUDIO_CMD_SET_BALANCE (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_Balance(COMM_DEV_SERIAL, para);
        } else if (cmd == AUDIO_CMD_SET_SOURCE) {
            LOGD("%s, send AUDIO_CMD_SET_SOURCE (para = %d) to fbc.\n", __FUNCTION__, para);
            return pFBC->cfbc_Set_FBC_Audio_Source(COMM_DEV_SERIAL, para);
        }
    }
    return 0;
}

int CTv::GetHdmiAvHotplugDetectOnoff()
{
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_SSM_HDMI_AV_DETECT, "null" );
    if ( strtoul(value, NULL, 10) == 1 ) {
        return 1;
    }

    return 0;
}

int CTv::SetHdmiEdidVersion(tv_hdmi_port_id_t port, tv_hdmi_edid_version_t version)
{
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    mSigDetectThread.requestAndWaitPauseDetect();
    mAv.DisableVideoWithBlackColor();
    mpTvin->Tvin_StopDecoder();
    Tv_HDMIEDIDFileSelect(port, version);
    SSMSetHDMIEdid(port);
    mHDMIRxManager.HdmiRxEdidUpdate();
    mSigDetectThread.initSigState();
    mSigDetectThread.resumeDetect();
    return 0;
}

int CTv::SetHdmiHDCPSwitcher(tv_hdmi_hdcpkey_enable_t enable)
{
    mHDMIRxManager.HdmiRxHdcpOnOff(enable);
    return 0;
}

int CTv::SetHdmiColorRangeMode(tv_hdmi_color_range_t range_mode)
{
    return mHDMIRxManager.SetHdmiColorRangeMode(range_mode);
}

tv_hdmi_color_range_t CTv::GetHdmiColorRangeMode()
{
    return mHDMIRxManager.GetHdmiColorRangeMode();
}

int CTv::SetVideoAxis(int x, int y, int width, int heigth)
{
    return mAv.setVideoAxis(x, y, width, heigth);
}

int CTv::handleGPIO(const char *port_name, bool is_out, int edge)
{
    return pGpio->processCommand(port_name, is_out, edge);
}

int CTv::KillMediaServerClient()
{
    char buf[PROPERTY_VALUE_MAX] = { 0 };
    int len = property_get("media.player.pid", buf, "");
    if (len > 0) {
        char* end;
        int pid = strtol(buf, &end, 0);
        if (end != buf) {
            LOGD("[ctv] %s, remove video path, but video decorder has used, kill it:%d\n",
                __FUNCTION__, pid);
            kill(pid, SIGKILL);
            property_set("media.player.pid", "");
        }
    }
    return 0;
}

int CTv::autoSwitchToMonitorMode()
{
    if ( (!MnoNeedAutoSwitchToMonitorMode) && ((mSigDetectThread.getCurSigInfo().cfmt == TVIN_YUV444)
        || (mSigDetectThread.getCurSigInfo().cfmt == TVIN_RGB444)
        || (1 == IsDVISignal())//iS DVI
        || (1 == mpTvin->GetITContent())
        || (mSigDetectThread.getCurSigInfo().fmt == TVIN_SIG_FMT_HDMI_1440X240P_60HZ)
        || (mSigDetectThread.getCurSigInfo().fmt == TVIN_SIG_FMT_HDMI_2880X240P_60HZ)
        || (mSigDetectThread.getCurSigInfo().fmt == TVIN_SIG_FMT_HDMI_1440X288P_50HZ)
        || (mSigDetectThread.getCurSigInfo().fmt == TVIN_SIG_FMT_HDMI_2880X288P_50HZ)
        || ((mSigDetectThread.getCurSigInfo().fmt >= TVIN_SIG_FMT_HDMI_800X600_00HZ)
        && (mSigDetectThread.getCurSigInfo().fmt <= TVIN_SIG_FMT_HDMI_1680X1050_00HZ)))) {
        LOGD("[ctv] %s, CurSigInfo.cfmt is %d , CurSigInfo.fmt is %x \n",
                __FUNCTION__, mSigDetectThread.getCurSigInfo().cfmt, mSigDetectThread.getCurSigInfo().fmt);
        CVpp::getInstance()->enableMonitorMode(true);
        CVpp::getInstance()->SavePQMode(VPP_PICTURE_MODE_MONITOR, m_source_input);
    }
    return 0;
}

void CTv::dump(String8 &result)
{
    result.appendFormat("\naction = %x\n", mTvAction);
    result.appendFormat("status = %d\n", mTvStatus);
    result.appendFormat("current source input = %d\n", m_source_input);
    result.appendFormat("last source input = %d\n", m_last_source_input);
    result.appendFormat("hdmi out with fbc = %d\n\n", mHdmiOutFbc);

    result.appendFormat("tvserver git branch:%s\n", tvservice_get_git_branch_info());
    result.appendFormat("tvserver git version:%s\n", tvservice_get_git_version_info());
    result.appendFormat("tvserver Last Changed:%s\n", tvservice_get_last_chaned_time_info());
    result.appendFormat("tvserver Last Build:%s\n", tvservice_get_build_time_info());
    result.appendFormat("tvserver Builer Name:%s\n", tvservice_get_build_name_info());
    result.appendFormat("tvserver board version:%s\n\n", tvservice_get_board_version_info());

    result.appendFormat("libdvb git branch:%s\n", dvb_get_git_branch_info());
    result.appendFormat("libdvb git version:%s\n", dvb_get_git_version_info());
    result.appendFormat("libdvb Last Changed:%s\n", dvb_get_last_chaned_time_info());
    result.appendFormat("libdvb Last Build:%s\n", dvb_get_build_time_info());
    result.appendFormat("libdvb Builer Name:%s\n\n", dvb_get_build_name_info());
}


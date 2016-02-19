//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTv.cpp
//  @ Date : 2013-11
//  @ Author :
#define LOG_TAG "CTv"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <am_epg.h>
#include <am_mem.h>
#include "CTvDatabase.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/log.h>
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

#include "../version/version.h"
#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/CFile.h"
#include <hardware_legacy/power.h>
#include "../tvutils/serial_operate.h"

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

// Called each time a message is logged.
static void sqliteLogCallback(void *data, int iErrCode, const char *zMsg)
{
    data = data;
    LOGD( "showbo sqlite (%d) %s\n", iErrCode, zMsg);
}

CTv::CTv() :
    mTvMsgQueue(this),
    mTvScannerDetectObserver(this)
{
    mAudioMuteStatusForTv = CC_AUDIO_UNMUTE;
    mAudioMuteStatusForSystem = CC_AUDIO_UNMUTE;
    //mpClient = pClient;
    //copy file to param
    if (Tv_Utils_IsFileExist ( TV_CONFIG_FILE_SYSTEM_PATH )) {
        if (!Tv_Utils_IsFileExist ( TV_CONFIG_FILE_PARAM_PATH )) {
            CFile file ( TV_CONFIG_FILE_SYSTEM_PATH );

            if ( file.copyTo ( TV_CONFIG_FILE_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CONFIG_FILE_PARAM_PATH );
            }
        }
    }

    if (Tv_Utils_IsFileExist ( TV_CHANNEL_LIST_SYSTEM_PATH )) {
        if (!Tv_Utils_IsFileExist ( TV_CHANNEL_LIST_PARAM_PATH )) {
            CFile file ( TV_CHANNEL_LIST_SYSTEM_PATH );

            if ( file.copyTo ( TV_CHANNEL_LIST_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_CHANNEL_LIST_PARAM_PATH );
            }
        }
    }

    //ssm
    if (Tv_Utils_IsFileExist ( TV_SSM_DATA_SYSTEM_PATH )) {
        if (!Tv_Utils_IsFileExist ( TV_SSM_DATA_PARAM_PATH )) {
            CFile file ( TV_SSM_DATA_SYSTEM_PATH );

            if ( file.copyTo ( TV_SSM_DATA_PARAM_PATH ) != 0 ) {
                LOGE ( "%s, copy file = %s , error", __FUNCTION__, TV_SSM_DATA_PARAM_PATH );
            }
        }
    }

    AM_EVT_Init();
    //mTvEpg.setObserver ( &mTvMsgQueue );
    mpObserver = NULL;
    fbcIns = NULL;
    dtv_auto_3d_flag = 0;
    mAutoSetDisplayFreq = false;

    tv_config_load ( TV_CONFIG_FILE_PATH );
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

    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    if (hdmiOutWithFbc()) {
        mHdmiOutFbc = true;
    } else {
        mHdmiOutFbc = false;
    }

    m_sig_stable_nums = 0;
    mSetHdmiEdid = false;
    m_cur_playing_prog_id = -1;

    m_hdmi_audio_data = 0;
    mSigDetectThread.setObserver ( this );
    mSourceConnectDetectThread.setObserver ( this );
    mHDMIRxCEC.setObserver(this);
    mFrontDev.setObserver ( &mTvMsgQueue );
    mpUpgradeFBC = NULL;
    if (mHdmiOutFbc) {
        fbcIns = GetSingletonFBC();

        mpUpgradeFBC = new CUpgradeFBC();
        mpUpgradeFBC->setObserver(this);
    }
    mSerialA.setObserver(this);
    mSerialB.setObserver(this);
    mSerialC.setObserver(this);
    mSubtitle.setObserver(this);
    mHeadSet.setObserver(this);
    mTvScanner.setGlobalScanerObject(&mTvScanner);
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
    static const int WALL_EFFECT_VALUE[CC_BAND_ITEM_CNT] = { 0, 0, 1, 2, 2, 0 };
    for (int i = 0; i < CC_BAND_ITEM_CNT; i++) {
        mWallEffectValueBuf[i] = WALL_EFFECT_VALUE[i];
    }
    mTvAction &= TV_ACTION_NULL;
    mTvStatus = TV_INIT_ED;
    print_version_info();
};

CTv::~CTv()
{
    mpObserver = NULL;
    CTvSettingunLoad();
    CTvDatabase::deleteTvDb();
    tv_config_unload();
    mAv.Close();
    mTvStatus = TV_INIT_ED;
    mFrontDev.Close();

    if (mpUpgradeFBC != NULL) {
        delete mpUpgradeFBC;
        mpUpgradeFBC = NULL;
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
    LOGD ( "%s, FE event type = %d tvaction=%x", __FUNCTION__, ev.mCurSigStaus, mTvAction);
    if (mTvAction & TV_ACTION_SCANNING) return;

    //前端事件响应处理
    if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_HAS_SIG ) { //作为信号稳定
        if (/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //atv and other tvin source    not to use it, and if not playing, not use have sig
            if ( m_win_mode == PREVIEW_WONDOW ) {
                //mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
                mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
            }

            if ( mAutoSetDisplayFreq) {
                CTvin::getInstance()->VDIN_SetDisplayVFreq (50, mAv.getVideoDisplayResolution() , mHdmiOutFbc);
            }
            TvEvent::SignalInfoEvent ev;
            ev.mStatus = TVIN_SIG_STATUS_STABLE;
            ev.mTrans_fmt = TVIN_TFMT_2D;
            ev.mFmt = TVIN_SIG_FMT_NULL;
            ev.mReserved = 0;
            sendTvEvent ( ev );
            //if (mAv.WaittingVideoPlaying() == 0) {
            SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mAv.getVideoResolutionToFmt());
            usleep(50 * 1000);
            mAv.EnableVideoNow();
            SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
            //}
            Tv_SetAudioInSource(SOURCE_DTV);
        }
    } else if ( ev.mCurSigStaus == CFrontEnd::FEEvent::EVENT_FE_NO_SIG ) { //作为信号消失
        if (/*m_source_input == SOURCE_TV || */m_source_input == SOURCE_DTV && (mTvAction & TV_ACTION_PLAYING)) { //just playing
            config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
            if ( strcmp ( config_value, "black" ) == 0 ) {
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
    const char *config_value = NULL;
    LOGD("AVEvent = %d", ev.type);
    switch ( ev.type ) {
    case CAv::AVEvent::EVENT_AV_STOP: {
        config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
        if ( strcmp ( config_value, "black" ) == 0 ) {
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
        //if (mAv.WaittingVideoPlaying() == 0) {
        SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ),
                         CTvin::Tvin_SourceInputToSourceInputType(m_source_input),
                         mAv.getVideoResolutionToFmt());
        usleep(50 * 1000);
        mAv.EnableVideoNow();
        SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
        //}
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_PLAYBACK_RESUME;
        AvPlayBackEvt.mProgramId = ( int ) ev.param;
        sendTvEvent(AvPlayBackEvt );
        break;
    }

    case CAv::AVEvent::EVENT_AV_SCAMBLED: {
        config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
        if ( strcmp ( config_value, "black" ) == 0 ) {
            mAv.DisableVideoWithBlackColor();
        } else {
            mAv.DisableVideoWithBlueColor();
        }
        TvEvent::AVPlaybackEvent AvPlayBackEvt;
        AvPlayBackEvt.mMsgType = TvEvent::AVPlaybackEvent::EVENT_AV_SCAMBLED;
        AvPlayBackEvt.mProgramId = ( int ) ev.param;
        sendTvEvent(AvPlayBackEvt );
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
        mpTv->mFrontDev.checkStatusOnce();
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
        break;
    }

    case TV_MSG_ENABLE_VIDEO_LATER: {
        int fc = msg.mpPara[0];
        mpTv->onEnableVideoLater(fc);
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
    memcpy(msg.mpPara, &ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CFrontEnd::FEEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_FE_EVENT;
    memcpy(msg.mpPara, &ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent ( const CTvEpg::EpgEvent &ev )
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_EPG_EVENT;
    memcpy(msg.mpPara, &ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::CTvMsgQueue::onEvent(const CAv::AVEvent &ev)
{
    CMessage msg;
    msg.mDelayMs = 0;
    msg.mType = CTvMsgQueue::TV_MSG_AV_EVENT;
    memcpy(msg.mpPara, &ev, sizeof(ev));
    this->sendMsg ( msg );
}

void CTv::onHdmiSrChanged(int   sr, bool bInit)
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
    //AutoMutex lock(mLock);
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
    return mFrontDev.ClearAnalogFrontEnd ();
}

int CTv::dtvAutoScan()
{
    AutoMutex lock ( mLock );
    mTvAction |= TV_ACTION_SCANNING;
    //mTvEpg.leaveChannel();
    mAv.StopTS ();
    mAv.DisableVideoWithBlueColor();
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
    CTvEvent::CleanAllEvent();
    mTvScanner.setObserver ( &mTvMsgQueue );
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    mFrontDev.Open(FE_DTMB);
    mTvScanner.autoDtmbScan();//dtmb
    return 0;
}

int CTv::dtvCleanProgramByFreq ( int freq )
{
    int iOutRet = 0;
    CTvChannel channel;
    iOutRet = CTvChannel::SelectByFreq ( freq, channel );
    if ( -1 == iOutRet ) {
        LOGD ( "%s, Warnning: Current Freq not have program info in the ts_table\n", __FUNCTION__ );
    }

    iOutRet == CTvProgram::deleteChannelsProgram ( channel );
    return iOutRet;
}

int CTv::dtvManualScan (int beginFreq, int endFreq, int modulation)
{
    AutoMutex lock ( mLock );
    mTvAction |= TV_ACTION_SCANNING;
    //mTvEpg.leaveChannel();
    mAv.StopTS ();
    mAv.DisableVideoWithBlueColor();
    CTvChannel::DeleteBetweenFreq(beginFreq, endFreq);
    mTvScanner.setObserver ( &mTvMsgQueue );
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    mFrontDev.Open(FE_DTMB);
    int iOutRet = mTvScanner.manualDtmbScan (beginFreq, endFreq, modulation); //dtmb
    return iOutRet;
}

int CTv::dtvAutoScanAtscLock(int attenna, int videoStd, int audioStd)
{
    //for warning
    attenna = attenna;
    videoStd = videoStd;
    audioStd = audioStd;
    //showboz
    /*  int minScanFreq, maxScanFreq, vStd, aStd;
    vStd = CC_ATV_VIDEO_STD_PAL;
    aStd = CC_ATV_AUDIO_STD_DK;
    v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);

    AutoMutex lock ( mLock );
    mTvAction |= TV_ACTION_SCANNING;
    mTvEpg.leaveChannel();
    mAv.StopTS ();
    mAv.DisableVideoWithBlueColor();
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_DTV );
    CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_RADIO );
    CTvEvent::CleanAllEvent();
    mTvScanner.setObserver ( &mTvMsgQueue );
    mDtvScanRunningStatus = DTV_SCAN_RUNNING_NORMAL;
    mFrontDev.Open(FE_ATSC);
    mTvScanner.autoAtscScan(CTvScanner::AM_ATSC_ATTENNA_TYPE_AIR,  stdAndColor);//dtmb*/
    return 0;
}

//searchType 0:not 256 1:is 256 Program
int CTv::atvAutoScan(int videoStd __unused, int audioStd __unused, int searchType)
{
    int minScanFreq, maxScanFreq, vStd, aStd;
    AutoMutex lock ( mLock );
    mAv.DisableVideoWithBlueColor();
    mTvAction |= TV_ACTION_SCANNING;
    stopPlaying();
    mTvScanner.setObserver ( &mTvMsgQueue );
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    getATVMinMaxFreq (&minScanFreq, &maxScanFreq );
    if ( minScanFreq == 0 || maxScanFreq == 0 || minScanFreq > maxScanFreq ) {
        LOGE ( "%s, auto scan  freq set is error min=%d, max=%d", __FUNCTION__, minScanFreq, maxScanFreq );
        return -1;
    }
    mSigDetectThread.setVdinNoSigCheckKeepTimes(1000, false);
    mSigDetectThread.requestAndWaitPauseDetect();
    mSigDetectThread.setObserver(&mTvScannerDetectObserver);
    CTvin::getInstance()->Tvin_StopDecoder();
    //if  set std null AUTO, use default PAL/DK
    //if(videoStd == CC_ATV_VIDEO_STD_AUTO) {
    //   vStd = CC_ATV_VIDEO_STD_PAL;
    //   aStd = CC_ATV_AUDIO_STD_DK;
    //} else {
    vStd = CC_ATV_VIDEO_STD_PAL;
    aStd = CC_ATV_AUDIO_STD_DK;
    //}
    tvin_port_t source_port = CTvin::getInstance()->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    CTvin::getInstance()->VDIN_OpenPort ( source_port );
    v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);

    int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
    CTvin::getInstance()->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );

    if (searchType == 0) {
        CTvProgram::CleanAllProgramBySrvType ( CTvProgram::TYPE_ATV );
    } else if (searchType == 1) { //type for skyworth, and insert 256 prog, and just update ts table
    }
    minScanFreq = mFrontDev.formatATVFreq ( minScanFreq );
    maxScanFreq = mFrontDev.formatATVFreq ( maxScanFreq );
    LOGD("%s, atv auto scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);
    mFrontDev.Open(FE_ANALOG);
    mSigDetectThread.initSigState();
    mSigDetectThread.resumeDetect();
    mTvScanner.autoAtvScan ( minScanFreq, maxScanFreq, stdAndColor, searchType);
    return 0;
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

    minScanFreq = mFrontDev.formatATVFreq ( startFreq );
    maxScanFreq = mFrontDev.formatATVFreq ( endFreq );
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

    AutoMutex lock ( mLock );
    mAv.DisableVideoWithBlueColor();
    mTvAction |= TV_ACTION_SCANNING;
    mTvScanner.setObserver ( &mTvMsgQueue );

    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor(vStd, aStd);

    tvin_port_t source_port = CTvin::getInstance()->Tvin_GetSourcePortBySourceInput(SOURCE_TV);
    CTvin::getInstance()->VDIN_OpenPort ( source_port );

    int fmt = CFrontEnd::stdEnumToCvbsFmt (vStd, aStd);
    CTvin::getInstance()->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) TVIN_SIG_FMT_NULL );
    LOGD("%s, atv manual scan vstd=%d, astd=%d stdandcolor=%lld", __FUNCTION__, vStd, aStd, stdAndColor);
    mFrontDev.Open(FE_ANALOG);
    return mTvScanner.ATVManualScan ( minScanFreq, maxScanFreq, stdAndColor, store_Type, channel_num);
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

int CTv::stopScanLock()
{
    AutoMutex lock ( mLock );
    return stopScan();
}

int CTv::stopScan()
{
    const char *config_value = NULL;
    if (!(mTvAction & TV_ACTION_SCANNING)) {
        LOGD("%s, tv not scanning ,return\n", __FUNCTION__);
        return 0;
    }

    LOGD("%s, tv scanning , stop it\n", __FUNCTION__);
    config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( config_value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }
    mSigDetectThread.requestAndWaitPauseDetect();
    mSigDetectThread.setObserver(this);
    //mTvEpg.leaveChannel();
    mTvScanner.stopScan();
    mFrontDev.Close();
    mTvAction &= ~TV_ACTION_SCANNING;
    return 0;
}

int CTv::playProgramLock ( int progId )
{
    /*AutoMutex lock ( mLock );
    CTvProgram prog;
    int ret = CTvProgram::selectByID ( progId, prog );

    if ( ret != 0 ) {
        return -1;
    }
    int type = prog.getProgType();
    LOGD ( "%s, blackout = %dprog type = %d id = %d name = %s\n", __FUNCTION__,   m_blackout_enable, type, progId, prog.getName().string() );
    if (m_blackout_enable == 1) {
        mAv.EnableVideoBlackout();
    }else if (m_blackout_enable == 0){
        mAv.DisableVideoBlackout();
    }

    mTvAction |= TV_ACTION_PLAYING;
    m_cur_playing_prog_id = progId;
    mFrontDev.Open(FE_ANALOG);
    if ( m_source_input == SOURCE_TV && type == CTvProgram::TYPE_ATV ) {
        playAtvProgram ( progId );
    } else if ( m_source_input == SOURCE_DTV && ( type == CTvProgram::TYPE_DTV || type == CTvProgram::TYPE_RADIO ) ) {
        playDtmbProgram ( progId );
    } else {
        LOGD ( "%s, source(%d) != type(%d),not play\n", __FUNCTION__,  m_source_input, type );
    }
    //just for ui,left/right/stereo
    int audio_ch = prog.getAudioChannel();
    AM_AOUT_OutputMode_t channel = AM_AOUT_OUTPUT_STEREO;
    if (audio_ch == 1) {
        channel = AM_AOUT_OUTPUT_STEREO;
    } else if (audio_ch == 2) {
        channel = AM_AOUT_OUTPUT_DUAL_LEFT;
    } else if (audio_ch == 3) {
        channel = AM_AOUT_OUTPUT_DUAL_RIGHT;
    }
    setAudioChannel((int)channel);*/

    progId = progId;
    return 0;
}

int CTv::playDvbcProgram ( int progId )
{
    LOGD ( "%s, progId = %d\n", __FUNCTION__,  progId );

    CTvProgram prog;
    int ret = CTvProgram::selectByID ( progId, prog );
    if ( ret != 0 ) {
        return -1;
    }

    LOGD ( "%s, prog name = %s\n", __FUNCTION__,  prog.getName().string() );
    CTvChannel channel;
    prog.getChannel ( channel );

    mFrontDev.setPara ( channel.getMode(), channel.getFrequency(), channel.getSymbolRate(), channel.getModulation() );

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
    startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );

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
        LOGD ( "%s, iOutRet = %d AM_AV_SwitchTSAudio\n", __FUNCTION__,  iOutRet );
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
        if (prog.getAudioTrackSize() <= 1) {
            LOGD("%s, just one audio track, not to switch", __FUNCTION__);
            break;
        }
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
    int iOutRet = 0;

    iOutRet = mAv.ResetAudioDecoder ();
    LOGD ( "%s, iOutRet = %d AM_AV_ResetAudioDecoder\n", __FUNCTION__,  iOutRet );
    return iOutRet;
}

int CTv::playDtvProgram ( int mode, int freq, int para1, int para2,
                          int vpid, int vfmt, int apid, int afmt, int pcr __unused, int audioCompetation)
{
    AutoMutex lock ( mLock );
    mTvAction |= TV_ACTION_PLAYING;
    if ( m_blackout_enable == 1 ) {
        mAv.EnableVideoBlackout();
    } else if ( m_blackout_enable == 0 ) {
        mAv.DisableVideoBlackout();
    }
    mFrontDev.Open(FE_ANALOG);
    mFrontDev.setPara ( mode, freq, para1, para2);
    mTvAction |= TV_ACTION_PLAYING;
    startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );

    CMessage msg;
    msg.mDelayMs = 2000;
    msg.mType = CTvMsgQueue::TV_MSG_CHECK_FE_DELAY;
    mTvMsgQueue.sendMsg ( msg );
    SetCurProgramAudioVolumeCompensationVal ( audioCompetation );
    return 0;
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

    mFrontDev.setPara ( channel.getMode(), channel.getFrequency(), channel.getBandwidth(), 0 );

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
    startPlayTv ( SOURCE_DTV, vpid, apid, vfmt, afmt );
    mTvEpg.enterChannel ( channel.getID() );
    mTvEpg.enterProgram ( prog.getID() );
    return 0;
}

int CTv::playAtvProgram (int  freq, int videoStd, int audioStd, int fineTune __unused, int audioCompetation)
{
    mTvAction |= TV_ACTION_PLAYING;
    if ( m_blackout_enable == 1 ) {
        mAv.EnableVideoBlackout();
    } else if ( m_blackout_enable == 0 ) {
        mAv.DisableVideoBlackout();
    }
    SetAudioMuteForTv ( CC_AUDIO_MUTE );
    //image selecting channel
    mSigDetectThread.requestAndWaitPauseDetect();
    CTvin::getInstance()->Tvin_StopDecoder();
    mFrontDev.Open(FE_ANALOG);
    //set CVBS
    int fmt = CFrontEnd::stdEnumToCvbsFmt (videoStd, audioStd);
    CTvin::getInstance()->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

    v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor (videoStd, audioStd);
    //set TUNER
    mFrontDev.setPara (FE_ANALOG, freq, stdAndColor, 1);

    mSigDetectThread.setObserver ( this );
    mSigDetectThread.initSigState();
    mSigDetectThread.resumeDetect(1000);

    SetCurProgramAudioVolumeCompensationVal ( audioCompetation );
    return 0;
}

int CTv::resetFrontEndPara ( frontend_para_set_t feParms )
{
    if ( feParms.mode == FE_ANALOG ) {
        int progID = -1;
        int tmpFreq = feParms.freq;
        int tmpfineFreq = feParms.para2;
        int mode = feParms.mode;

        //get tunerStd from videoStd and audioStd
        v4l2_std_id stdAndColor = mFrontDev.enumToStdAndColor (feParms.videoStd, feParms.audioStd);

        LOGD("%s, resetFrontEndPara-　vstd=%d astd=%d stdandcolor=%lld", __FUNCTION__, feParms.videoStd, feParms.audioStd, stdAndColor);

        //set frontend parameters to tuner dev
        mSigDetectThread.requestAndWaitPauseDetect();
        CTvin::getInstance()->Tvin_StopDecoder();

        //set CVBS
        int fmt = CFrontEnd::stdEnumToCvbsFmt (feParms.videoStd, feParms.audioStd );
        CTvin::getInstance()->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

        //set TUNER
        usleep(400 * 1000);
        mFrontDev.setPara ( FE_ANALOG, tmpFreq, stdAndColor, 1 );
        usleep(400 * 1000);
        if ( tmpfineFreq != 0 ) {
            mFrontDev.fineTune ( tmpfineFreq / 1000 );
        }

        mSigDetectThread.initSigState();
        mSigDetectThread.resumeDetect();
    } else if ( feParms.mode == FE_DTMB ) {
        mFrontDev.setPara ( FE_DTMB, feParms.freq, feParms.para1, feParms.para2 );
    }

    return 0;
}

int CTv::resetDmxAndAvSource()
{
    AM_DMX_Source_t curdmxSource;
    mFrontDev.GetTSSource ( &curdmxSource );
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

    LOGD ( "%s, VolumeCompensationVal = %d, id = %d\n", __FUNCTION__,  tmpVal, m_cur_playing_prog_id );
    CTvProgram prog;
    if ( CTvProgram::selectByID ( m_cur_playing_prog_id, prog ) != 0 ) {
        LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
        return -1;
    }

    if (prog.updateVolComp ( m_cur_playing_prog_id, tmpVal ) != 0 ) {
        LOGE ( "%s, atv progID is not matching the db's\n", __FUNCTION__);
        return -1;
    }
    return 0;
}

int CTv::GetAudioVolumeCompensationVal(int progxxId __unused)
{
    int tmpVolValue = 0;
    CTvProgram prog;
    if ( CTvProgram::selectByID ( m_cur_playing_prog_id, prog ) != 0 ) {
        LOGE ( "%s, atv progID is not matching the db's  ret = 0\n", __FUNCTION__ );
        return tmpVolValue;
    }
    tmpVolValue = prog.getChanVolume ();
    LOGD ( "%s,progid = %d CompensationVal = %d\n", __FUNCTION__,  m_cur_playing_prog_id, tmpVolValue );
    if (tmpVolValue > 20 || tmpVolValue < -20) tmpVolValue = 0;
    return tmpVolValue;
}

int CTv::startPlayTv ( int source, int vid, int aid, int vfat, int afat )
{
    if ( source == SOURCE_DTV ) {
        AM_FileEcho ( DEVICE_CLASS_TSYNC_AV_THRESHOLD_MIN, AV_THRESHOLD_MIN_MS );
        LOGD ( "%s, startPlayTv", __FUNCTION__);
        mAv.StartTS (vid, aid, ( AM_AV_VFormat_t ) vfat, ( AM_AV_AFormat_t ) afat );
    }
    return 0;
}

int CTv::stopPlayingLock()
{
    AutoMutex lock ( mLock );
    if (getSubSwitchStatus() == 1)
        stopSubtitle();
    return stopPlaying();
}

int CTv::stopPlaying()
{
    const char *config_value = NULL;
    if (!(mTvAction & TV_ACTION_PLAYING)) {
        LOGD("%s, stopplay cur action = %x not playing , return", __FUNCTION__, mTvAction);
        return 0;
    }
    mSigDetectThread.requestAndWaitPauseDetect();
    mAv.EnableVideoBlackout();
    if (m_source_input == SOURCE_TV) {
        //first mute
        SetAudioMuteForTv(CC_AUDIO_MUTE);
        ClearAnalogFrontEnd();
    } else if (m_source_input ==  SOURCE_DTV) {
        //mFrontDev.setPara(FE_DTMB, 51000000, 0, 0);
        mAv.StopTS ();
        //mTvEpg.leaveChannel();
        //mTvEpg.leaveProgram();
    }
    config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( config_value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }
    mTvAction &= ~TV_ACTION_PLAYING;
    return 0;
}

int CTv::GetATVAFCType()
{
    return 1;
}

int CTv::GetATVSourceTimerSwitch()
{
    return 1;
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
    CTvProgram::updateAudioChannel(m_cur_playing_prog_id, aud_ch);
    if ( AM_SUCCESS != iOutRet) {
        LOGD ( "%s, jianfei.lan TV AM_AOUT_SetOutputMode device is FAILED %d\n", __FUNCTION__, iOutRet );
    }
    return 0;
}

int CTv::getFrontendSignalStrength()
{
    return mFrontDev.getStrength();
}

int CTv::getFrontendSNR()
{
    return mFrontDev.getSNR();
}

int CTv::getFrontendBER()
{
    return mFrontDev.getBER();
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

    AutoMutex lock ( mLock );
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
    mTvScanner.setObserver ( &mTvMsgQueue );
    mTvScanner.manualDtmbScan ( freq, freq ); //dtmb
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

int CTv::getATVMinMaxFreq ( int *scanMinFreq, int *scanMaxFreq )
{
    const char *strDelimit = ",";
    char *token = NULL;

    *scanMinFreq = 44250000;
    *scanMaxFreq = 868250000;

    const char *freqList = config_get_str ( CFG_SECTION_ATV, CFG_ATV_FREQ_LIST, "null" );
    if ( strcmp ( freqList, "null" ) == 0 ) {
        LOGE( "%s, atv freq list is null \n", __FUNCTION__ );
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
    return ( ( TVIN_SIG_FMT_NULL != mSigDetectThread.getCurSigInfo().fmt ) && ( mSigDetectThread.getCurSigInfo().reserved & 0x1 ) );
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

int CTv::isSDFmtInHdmi ( void )
{
    if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
        return -1;
    }
    return CTvin::isSDFmtInHdmi (mSigDetectThread.getCurSigInfo().fmt);
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
    Mutex::Autolock _l ( mLock );
    return mTvStatus;
}

int CTv::OpenTv ( void )
{
    const char *value;
    Tv_Spread_Spectrum();
    //reboot system by fbc setting.
    value = config_get_str ( CFG_SECTION_TV, CFG_FBC_PANEL_INFO, "null" );
    LOGD("open tv, get fbc panel info:%s\n", value);
    if ( strcmp ( value, "edid" ) == 0 ) {
        reboot_sys_by_fbc_edid_info();
    } else if ( strcmp ( value, "uart" ) == 0 ) {
        reboot_sys_by_fbc_uart_panel_info(fbcIns);
    }

    CTvin::getInstance()->Tvin_LoadSourceInputToPortMap();

    SSMHandlePreCopying();
    if ( SSMDeviceMarkCheck() < 0 ) {
        SSMRestoreDeviceMarkValues();
        Tv_SSMRestoreDefaultSetting();
    }
    CTvin::getInstance()->OpenTvin();
    CTvin::getInstance()->init_vdin();
    CTvin::getInstance()->Tv_init_afe();
    CVpp::getInstance()->Vpp_Init(getPqDbPath());
    TvAudioOpen();
    SetAudioVolDigitLUTTable(SOURCE_MPEG);

    SSMSetHDCPKey();
    system ( "/system/bin/dec" );

    //set color filter
    //    SetFileAttrValue ( "/sys/module/amvideo/parameters/filt_mode", "0x2100" );
    value = config_get_str ( CFG_SECTION_TV, CFG_TVIN_DISPLAY_FREQ_AUTO, "null" );
    if ( strcmp ( value, "enable" ) == 0 ) {
        mAutoSetDisplayFreq = true;
    } else {
        mAutoSetDisplayFreq = false;
    }

    Tv_HandeHDMIEDIDFilePathConfig();

    value = config_get_str ( CFG_SECTION_TV, CFG_SSM_HDMI_EDID_EN, "null" );
    if ( strtoul(value, NULL, 10) == 1 ) {
        LOGD( "%s, get config \"%s\" is \"%s\".\n", __FUNCTION__, CFG_SSM_HDMI_EDID_EN, value );
        //get hdmi edid use mode
        char prop_value[256] = {0};
        char *pEdid = NULL;
        property_get( UBOOTENV_OUTPUTMODE, prop_value, "null" );
        LOGD( "%s, get property [%s]:[%s]\n", __FUNCTION__, UBOOTENV_OUTPUTMODE, prop_value );

        if ( strcmp ( prop_value, "null" ) != 0 )
            pEdid = prop_value;
        else
            pEdid = (char *)"hdmi_edid";//set default hdmi edid

        config_set_str ( CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, pEdid );
        //set file's path for hdmi edid of each port
        for ( int i = 1; i <= SSM_HDMI_PORT_MAX; i++ ) {
            char edid_path[256] = {0};
            char edid_path_cfg[256] = {0};
            sprintf ( edid_path, "/system/etc/%s_port%d.bin", pEdid, i );
            sprintf ( edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", i );
            config_set_str ( CFG_SECTION_TV, edid_path_cfg, edid_path );
        }

        mSetHdmiEdid = true;
    }

    Tvin_GetTvinConfig();
    m_last_source_input = SOURCE_INVALID;
    m_source_input = SOURCE_INVALID;
    m_mode_3d = VIDEO_3D_MODE_DISABLE;
    m_hdmi_sampling_rate = 0;
    int8_t blackout_enable;
    SSMReadBlackoutEnable(&blackout_enable);
    m_blackout_enable = blackout_enable;

    //dtv init
    //int rt = -1;
    //opendemux();
    //rt = mFrontDev.Open(FE_ANALOG);
    //mFrontDev.autoLoadFE();
    //rt = mFrontDev.Open(FE_ANALOG);
    AM_DMX_OpenPara_t para_dmx;
    memset ( &para_dmx, 0, sizeof ( AM_DMX_OpenPara_t ) );
    mTvDmx.Open(para_dmx );
    mTvDmx.SetSource (AM_DMX_SRC_TS2);
    mAv.Open();
    //mTvEpg.Init ( 0, 0, 1, "eng zho chi", "GB2312" );
    resetDmxAndAvSource();
    mSourceConnectDetectThread.startDetect();

    if (SSMReadSerialCMDSwitchValue() == 1) {
        SetSerialSwitch(SERIAL_A, 1);
    }
    ClearAnalogFrontEnd();

    mTvStatus = TV_OPEN_ED;

    mHDMIRxCEC.start();
    mAutoPQparam.startAutoPQ(CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG));
    return 0;
}

int CTv::CloseTv ( void )
{
    LOGD ( "tv close");
    mSigDetectThread.stopDetect();
    if (mpUpgradeFBC != NULL) {
        mpUpgradeFBC->stop();
    }
    CTvin::getInstance()->Tv_uninit_afe();
    CTvin::getInstance()->uninit_vdin();
    CVpp::getInstance()->Vpp_Uninit();
    TvMisc_DisableWDT ( gTvinConfig.userpet );
    mTvStatus = TV_CLOSE_ED;
    return 0;
}

int CTv::StartTvLock ()
{
    LOGD ( "%s, tv start status = %d", __FUNCTION__, mTvStatus);
    if (mTvStatus == TV_START_ED)
        return 0;

    Mutex::Autolock _l ( mLock );
    mTvAction |= TV_ACTION_STARTING;
    mAv.ClearVideoBuffer();
    SwitchAVOutBypass(0);
    InitSetTvAudioCard();
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mFrontDev.Open(FE_ANALOG);
    mFrontDev.autoLoadFE();
    mSigDetectThread.startDetect();
    mTvMsgQueue.startMsgQueue();
    resetDmxAndAvSource();
    SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
    TvMisc_EnableWDT ( gTvinConfig.kernelpet_disable, gTvinConfig.userpet, gTvinConfig.kernelpet_timeout, gTvinConfig.userpet_timeout, gTvinConfig.userpet_reset );
    am_phase_t am_phase;
    if (CVpp::getInstance()->getPqData()->PQ_GetPhaseArray ( &am_phase ) == 0 ) {
        CTvin::getInstance()->TvinApi_SetCompPhase(am_phase);
    }
    CTvin::getInstance()->TvinApi_SetCompPhaseEnable ( 1 );
    CTvin::getInstance()->VDIN_EnableRDMA ( 1 );

    //win pos
    tvin_window_pos_t win_pos;
    Vpp_GetDisplayResolutionInfo(&win_pos);
    SetPreviewWindow ( win_pos );

    CTvin::getInstance()->Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
    if ( CTvin::getInstance()->Tvin_RemovePath ( TV_PATH_TYPE_DEFAULT ) ) {
        CTvin::getInstance()->Tvin_AddPath ( TV_PATH_DECODER_NEW3D_WITHOUTPPMGR_AMVIDEO );
    }

    mAv.SetVideoWindow (0, 0, 0, 0 );

    mTvAction &= ~TV_ACTION_STARTING;
    mTvStatus = TV_START_ED;
    return 0;
}

int CTv::DoInstabootSuspend()
{
    CTvDatabase::GetTvDb()->UnInitTvDb();
    CTvSettingdoSuspend();
    CVpp::getInstance()->doSuspend();
    LOGD("%s,  \n", __FUNCTION__);
    return 0;
}

int CTv::DoInstabootResume()
{
    CTvDatabase::GetTvDb()->InitTvDb(TV_DB_PATH);
    CTvSettingdoResume();
    CVpp::getInstance()->doResume();
    LOGD("%s, \n", __FUNCTION__);
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
    const char *value;
    Mutex::Autolock _l ( mLock );
    mAv.DisableVideoWithBlackColor();
    CVpp::getInstance()->LoadVppSettings ( SOURCE_TYPE_MPEG, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D );
    //we should stop audio first for audio mute.
    mTvAction |= TV_ACTION_STOPING;
    CTvin::getInstance()->Tvin_StopDecoder();
    CTvin::getInstance()->Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
    CTvin::getInstance()->VDIN_ClosePort();
    mTvMsgQueue.clearMsg();
    mSigDetectThread.requestAndWaitPauseDetect();
    //stop scan  if scanning
    stopScan();
    mFrontDev.SetAnalogFrontEndTimerSwitch(0);
    //first stop play(if playing)
    stopPlaying();
    //
    setAudioChannel(AM_AOUT_OUTPUT_STEREO);
    CTvin::getInstance()->setMpeg2Vdin(0);
    mAv.setLookupPtsForDtmb(0);
    SwitchAVOutBypass(0);
    tv_audio_channel_e audio_channel = CTvin::getInstance()->Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
    AudioLineInSelectChannel( audio_channel );
    AudioCtlUninit();
    SetAudioVolDigitLUTTable(SOURCE_MPEG);
    Tv_SetAudioInSource(SOURCE_MPEG);
    tvin_window_pos_t win_pos;
    Vpp_GetDisplayResolutionInfo(&win_pos);
    SetPreviewWindow (win_pos );
    //CVpp::getInstance()->LoadVppSettings ( SOURCE_TYPE_MPEG, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D );
    SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG) ), CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG), mSigDetectThread.getCurSigInfo().fmt);
    RefreshAudioMasterVolume ( SOURCE_MPEG );
    CVpp::getInstance()->VPP_SetScalerPathSel(0);
    CVpp::getInstance()->VPP_SetVideoCrop ( 0, 0, 0, 0 );
    m_last_source_input = SOURCE_INVALID;
    m_source_input = SOURCE_INVALID;
    mFrontDev.Close();
    mTvAction &= ~TV_ACTION_STOPING;
    mTvStatus = TV_STOP_ED;
    if ( Get2d4gHeadsetEnable() == 1 ) {
        property_set("audio.tv_open.flg", "0");
    }

    Tv_SetDDDRCMode(SOURCE_MPEG);
    mAv.DisableVideoWithBlackColor();
    //mAutoPQparam.startAutoPQ(CTvin::Tvin_SourceInputToSourceInputType(SOURCE_MPEG));
    mAv.ClearVideoBuffer();
    mAv.EnableVideoBlackout();
    SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
    return 0;
}

const char *CTv::getPqDbPath()
{
    return config_get_str(CFG_SECTION_TV, CFG_PQ_DB_PATH, DEF_VALUE_PQ_DB_PATH);
}

int CTv::Tv_MiscSetBySource ( tv_source_input_t source_input )
{
    int ret = -1;

    switch ( source_input ) {
    case SOURCE_TV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret = SetFileAttrValue ( SYS_VECM_DNLP_ADJ_LEVEL, "4" );
        break;

    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        //ret = CVpp::getInstance()->Tv_SavePanoramaMode ( VPP_PANORAMA_MODE_FULL, SOURCE_TYPE_HDMI );
        ret |= SetFileAttrValue ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_DTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);

    case SOURCE_AV1:
    case SOURCE_AV2:
    case SOURCE_VGA:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
        ret |= SetFileAttrValue ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;

    case SOURCE_SVIDEO:
    case SOURCE_IPTV:
        CVpp::getInstance()->VPP_SetScalerPathSel(1);
    default:
        CVpp::getInstance()->VPP_SetScalerPathSel(0);
        ret |= SetFileAttrValue ( SYS_VECM_DNLP_ADJ_LEVEL, "5" );
        break;
    }
    return ret;
}

int CTv::SetSourceSwitchInput (tv_source_input_t source_input )
{
    Mutex::Autolock _l ( mLock );
    LOGD ( "%s, source input = %d", __FUNCTION__, source_input );
    tv_source_input_t cur_source_input = m_source_input;
    tvin_port_t cur_port;

    Tv_SetDDDRCMode(source_input);
    if (source_input == cur_source_input ) {
        LOGW ( "%s,same input change display mode", __FUNCTION__ );
        return 0;
    }

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
        default:
            tmp_ret = -1;
            break;
        }
        if (tmp_ret < 0)
            LOGE ( "%s, do not set hdmi port%d edid.ret=%d", __FUNCTION__, source_input - 4, tmp_ret );
    }
    mTvAction |= TV_ACTION_SOURCE_SWITCHING;
    m_cur_playing_prog_id = -1;
    //
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mSigDetectThread.requestAndWaitPauseDetect();
    //const char *config_value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    //if ( strcmp ( config_value, "black" ) == 0 ) {
    //	mAv.DisableVideoWithBlackColor();
    //} else {
    //	mAv.DisableVideoWithBlueColor();
    //}
    mAv.DisableVideoWithBlackColor();
    //enable blackout, when play,disable it
    mAv.EnableVideoBlackout();
    //set front dev mode
    if ( source_input == SOURCE_TV ) {
        mFrontDev.Close();
        mFrontDev.Open(FE_ANALOG);
        mFrontDev.SetAnalogFrontEndTimerSwitch(GetATVSourceTimerSwitch());
        //mFrontDev.setMode ( FE_ANALOG );
    } else if ( source_input == SOURCE_DTV ) {
        mFrontDev.Close();
        mFrontDev.Open(FE_ANALOG);
        //mFrontDev.setMode (  FE_DTMB);
        mFrontDev.SetAnalogFrontEndTimerSwitch(0);
    } else {
        mFrontDev.Close();
        mFrontDev.SetAnalogFrontEndTimerSwitch(0);
    }

    //ok
    m_last_source_input = m_source_input;
    m_source_input = source_input;
    SSMSaveSourceInput ( source_input );

    tv_source_input_t pre_source_input = cur_source_input;//change
    cur_source_input = source_input;

    SetAudioVolumeCompensationVal ( 0 );

    if ( source_input == SOURCE_DTV ) {
        //we should stop audio first for audio mute.
        SwitchAVOutBypass(0);
        tv_audio_channel_e audio_channel = CTvin::getInstance()->Tvin_GetInputSourceAudioChannelIndex (SOURCE_MPEG);
        AudioLineInSelectChannel( audio_channel );
        AudioCtlUninit();
        SetAudioVolDigitLUTTable(SOURCE_MPEG);
        //
        CTvin::getInstance()->Tvin_StopDecoder();
        CTvin::getInstance()->Tvin_RemovePath ( TV_PATH_TYPE_TVIN );
        CTvin::getInstance()->VDIN_ClosePort();
        //double confirm we set the main volume lut buffer to mpeg
        RefreshAudioMasterVolume ( SOURCE_MPEG );
        RefreshSrsEffectAndDacGain();
        SetCustomEQGain();
        CTvin::getInstance()->setMpeg2Vdin(1);
        mAv.setLookupPtsForDtmb(1);
        //CVpp::getInstance()->LoadVppSettings ( SOURCE_TYPE_DTV, TVIN_SIG_FMT_NULL, INDEX_2D, TVIN_TFMT_2D );
        //mAutoPQparam.startAutoPQ(CTvin::Tvin_SourceInputToSourceInputType(SOURCE_DTV));
        Tv_SetAudioInSource ( source_input );
    } else {
        mAutoPQparam.stopAutoPQ();
        CTvin::getInstance()->setMpeg2Vdin(0);
        mAv.setLookupPtsForDtmb(0);
        CTvin::getInstance()->Tvin_AddPath ( TV_PATH_VDIN_NEW3D_WITHOUTPPMGR_AMVIDEO );
    }
    cur_port = CTvin::getInstance()->Tvin_GetSourcePortBySourceInput ( cur_source_input );
    Tv_MiscSetBySource ( source_input );

    //only DTV->TVIN or TVIN->DTV need start resource detect thread
    if ( ( pre_source_input == SOURCE_DTV || cur_source_input == SOURCE_DTV ) &&
            ( cur_source_input != pre_source_input ) ) {
        LOGD ( "%s, dtv need resource detect\n", __FUNCTION__ );
        int isNeedCheckD2D3 = ( gTvinConfig.memory512m) ? 1 : 0;
        static int sigchkcnt = 0;
        while ( true ) {
            if ( ( CTvin::getInstance()->Tvin_CheckPathActive ( TV_PATH_TYPE_DEFAULT, isNeedCheckD2D3 ) == TV_PATH_STATUS_INACTIVE )
                    && ( CTvin::getInstance()->Tvin_CheckPathActive ( TV_PATH_TYPE_TVIN, isNeedCheckD2D3 ) == TV_PATH_STATUS_INACTIVE ) ) {
                LOGD("%s, path check is ok, and break\n", __FUNCTION__);
                break;
            } else {
                if (sigchkcnt++ > 20) {
                    sigchkcnt = 0;
                    break;
                }
                LOGW ( "%s,resource is busy.", __FUNCTION__);
            }
        }
    }

    if ( source_input != SOURCE_DTV ) {
        // Uninit data
        UnInitTvAudio();
        if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3 ||
                source_input == SOURCE_MPEG || source_input == SOURCE_DTV ) {
            SwitchAVOutBypass(0);
        } else {
            SwitchAVOutBypass(1);
        }

        tv_audio_channel_e audio_channel = CTvin::getInstance()->Tvin_GetInputSourceAudioChannelIndex (source_input);
        AudioLineInSelectChannel( audio_channel );

        Tv_SetAudioInSource ( source_input );
        if ( source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3 ) {
            m_hdmi_sampling_rate = 0;
            m_hdmi_audio_data = 0;
        } else {
            InitTvAudio ( 48000, CC_IN_USE_I2S_DEVICE);
            HanldeAudioInputSr(-1);
        }
        //===========================================
        if ( CTvin::getInstance()->SwitchPort ( cur_port ) == 0 ) { //ok
            //==========================================
            unsigned char std;
            SSMReadCVBSStd(&std);
            int fmt = CFrontEnd::stdEnumToCvbsFmt (std, CC_ATV_AUDIO_STD_AUTO);
            CTvin::getInstance()->AFE_SetCVBSStd ( ( tvin_sig_fmt_t ) fmt );

            //for HDMI source connect detect
            CTvin::getInstance()->VDIN_OpenHDMIPinMuxOn(true);
            CVpp::getInstance()->Vpp_ResetLastVppSettingsSourceType();

            m_sig_stable_nums = 0;
            mSigDetectThread.setObserver ( this );
            mSigDetectThread.initSigState();
            int resumeLater = 0;
            if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
                resumeLater = 1000;
            } else {
                resumeLater = 1500;
            }
            mSigDetectThread.setVdinNoSigCheckKeepTimes(150, true);
            mSigDetectThread.resumeDetect(resumeLater);
        }
    }

    Tv_SetAudioSourceType(source_input);
    RefreshAudioMasterVolume(source_input);
    Tv_SetAudioOutputSwap_Type(source_input);
    //Tv_ADCDigitalCapture_Volume();
    Tv_SetAVOutPut_Input_gain(source_input);

    mTvAction &= ~ TV_ACTION_SOURCE_SWITCHING;
    return 0;
}

void CTv::onSigToStable()
{
    if (mAutoSetDisplayFreq) {
        int freq = 60;
        if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
            int fps = getHDMIFrameRate();
            LOGD("onSigToStable HDMI fps = %d", fps);
            if ((30 == fps) || (60 == fps)) {
                freq = 60;
            } else {
                freq = 50;
            }
        } else if ( CTvin::Tvin_is50HzFrameRateFmt ( mSigDetectThread.getCurSigInfo().fmt ) ) {
            freq = 50;
        }

        CTvin::getInstance()->VDIN_SetDisplayVFreq ( freq, mAv.getVideoDisplayResolution(), mHdmiOutFbc );
        LOGD ( "%s, SetDisplayVFreq %dHZ.", __FUNCTION__, freq);
    }
    //showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d

    CVpp::getInstance()->LoadVppSettings (CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt );

    if ( m_win_mode == PREVIEW_WONDOW ) {
        mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
        mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
    } else {
        SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
    }
    m_sig_stable_nums = 0;
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
        int startdec_status = CTvin::getInstance()->Tvin_StartDecoder ( mSigDetectThread.getCurSigInfo() );
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
        CMessage msg;
        msg.mDelayMs = 0;
        msg.mType = CTvMsgQueue::TV_MSG_ENABLE_VIDEO_LATER;
        msg.mpPara[0] = 2;
        mTvMsgQueue.sendMsg ( msg );
        m_hdmi_audio_data = 0;
    }
    m_sig_stable_nums++;
}

void CTv::onEnableVideoLater(int framecount)
{
    LOGD("onEnableVideoLater framecount = %d", framecount);
    mAv.EnableVideoWhenVideoPlaying(2);
    if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI ) {
        SetAudioMuteForTv ( CC_AUDIO_UNMUTE );
        Tv_SetAudioInSource(m_source_input);
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
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mAv.DisableVideoWithBlackColor();
    CTvin::getInstance()->Tvin_StopDecoder();
}

void CTv::onSigStableToUnSupport()
{
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    mAv.DisableVideoWithBlackColor();
    CTvin::getInstance()->Tvin_StopDecoder();

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
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }
    //SetAudioMuteForTv(CC_AUDIO_MUTE);
    CTvin::getInstance()->Tvin_StopDecoder();

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
    mAv.DisableVideoWithBlackColor();
    CTvin::getInstance()->Tvin_StopDecoder();
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
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
        //SetAudioMuteForTv(CC_AUDIO_MUTE);
    }
    CTvin::getInstance()->Tvin_StopDecoder();

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
    SetAudioMuteForTv(CC_AUDIO_MUTE);
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
        //SetAudioMuteForTv(CC_AUDIO_MUTE);
    }
    CTvin::getInstance()->Tvin_StopDecoder();

    tvin_info_t info = mSigDetectThread.getCurSigInfo();

    TvEvent::SignalInfoEvent ev;
    ev.mTrans_fmt = info.trans_fmt;
    ev.mFmt = info.fmt;
    ev.mStatus = info.status;
    ev.mReserved = info.reserved;
    sendTvEvent ( ev );
    LOGD ( "%s, Enable bluescreen for signal change in NullToNoSig!", __FUNCTION__);
}

void CTv::onSigNoSigToUnstable()
{
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mAv.DisableVideoWithBlackColor();
    } else {
        mAv.DisableVideoWithBlueColor();
    }
    LOGD("Enable bluescreen for signal change in NoSigToUnstable\n");
}

void CTv::onSigStillUnstable()
{
}

void CTv::onSigStillNosig()
{
}

void CTv::onSigStillNoSupport()
{
    LOGD ( "%s, gxtvbb, don't send event when sitll not support", __FUNCTION__);
    /*
    tvin_info_t info = mSigDetectThread.getCurSigInfo();
    TvEvent::SignalInfoEvent SigEv;
    SigEv.mTrans_fmt = info.trans_fmt;
    SigEv.mFmt = info.fmt;
    SigEv.mStatus = info.status;
    SigEv.mReserved = info.reserved;
    sendTvEvent ( SigEv );
    LOGD ( "%s, Unsupport signal", __FUNCTION__);
    */
}

void CTv::onSigStillNull()
{
}

void CTv::onStableSigFmtChange()
{
    if ( mAutoSetDisplayFreq) {
        if (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
            int fps = getHDMIFrameRate();
            LOGD("onSigToStable HDMI fps get = %d", fps);
        }
        int freq = 60;
        if ( CTvin::Tvin_is50HzFrameRateFmt ( mSigDetectThread.getCurSigInfo().fmt ) ) {
            freq = 50;
        }

        CTvin::getInstance()->VDIN_SetDisplayVFreq ( freq, mAv.getVideoDisplayResolution(), mHdmiOutFbc );
        LOGD ( "%s, SetDisplayVFreq %dHZ.", __FUNCTION__, freq);
    }
    //showbo mark  hdmi auto 3d, tran fmt  is 3d, so switch to 3d
    LOGD("hdmi trans_fmt = %d", mSigDetectThread.getCurSigInfo().trans_fmt);

    //load pq parameters
    CVpp::getInstance()->LoadVppSettings (CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt );

    if ( m_win_mode == PREVIEW_WONDOW ) {
        mAv.setVideoAxis(m_win_pos.x1, m_win_pos.y1, m_win_pos.x2, m_win_pos.y2);
        mAv.setVideoScreenMode ( CAv::VIDEO_WIDEOPTION_FULL_STRETCH );
    } else {
        SetDisplayMode ( CVpp::getInstance()->GetDisplayMode ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) ), CTvin::Tvin_SourceInputToSourceInputType(m_source_input), mSigDetectThread.getCurSigInfo().fmt);
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
        int sr = CTvin::getInstance()->get_hdmi_sampling_rate();
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
        int hdmi_audio_data = CTvin::getInstance()->TvinApi_GetHDMIAudioStatus();
        if (hdmi_audio_data != m_hdmi_audio_data) {
            LOGD("HDMI  auds_rcv_sts CHANGED = %d", hdmi_audio_data);
            m_hdmi_audio_data = hdmi_audio_data;
            onHMDIAudioStatusChanged(hdmi_audio_data);
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
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mpTv->mAv.DisableVideoWithBlackColor();
    } else {
        mpTv->mAv.DisableVideoWithBlueColor();
    }
    CTvin::getInstance()->Tvin_StopDecoder();
}

void CTv::CTvDetectObserverForScanner::onSigUnStableToNoSig()
{
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mpTv->mAv.DisableVideoWithBlackColor();
    } else {
        mpTv->mAv.DisableVideoWithBlueColor();
    }
    CTvin::getInstance()->Tvin_StopDecoder();
}

void CTv::CTvDetectObserverForScanner::onSigStableToNoSig()
{
    mpTv->SetAudioMuteForTv(CC_AUDIO_MUTE);
    const char *value = config_get_str ( CFG_SECTION_TV, CFG_BLUE_SCREEN_COLOR, "null" );
    if ( strcmp ( value, "black" ) == 0 ) {
        mpTv->mAv.DisableVideoWithBlackColor();
    } else {
        mpTv->mAv.DisableVideoWithBlueColor();
    }
    CTvin::getInstance()->Tvin_StopDecoder();
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
    if ( m_sig_stable_nums == 10 ) {
        mpTv->mAv.EnableVideoWhenVideoPlaying();
    }
    m_sig_stable_nums++;
}

void CTv::onSourceConnect(int source_type, int connect_status)
{
    TvEvent::SourceConnectEvent ev;
    ev.mSourceInput = source_type;
    ev.connectionState = connect_status;
    sendTvEvent(ev);
}

void CTv::onVframeSizeChange()
{
    if (m_source_input == SOURCE_DTV) {
        CVpp::getInstance()->LoadVppSettings ( SOURCE_TYPE_DTV, mAv.getVideoResolutionToFmt(), INDEX_2D, TVIN_TFMT_2D );
    } else if (m_source_input == SOURCE_INVALID) {
        CVpp::getInstance()->LoadVppSettings ( SOURCE_TYPE_MPEG, mAv.getVideoResolutionToFmt(), INDEX_2D, TVIN_TFMT_2D );
    }
}

int CTv::GetSourceConnectStatus(tv_source_input_t source_input)
{
    return mSourceConnectDetectThread.GetSourceConnectStatus((tv_source_input_t)source_input);
}

tv_source_input_t CTv::GetCurrentSourceInputLock ( void )
{
    Mutex::Autolock _l ( mLock );
    return m_source_input;
}

//dtv and tvin
tvin_info_t CTv::GetCurrentSignalInfo ( void )
{
    tvin_trans_fmt det_fmt = TVIN_TFMT_2D;
    tvin_sig_status_t signalState = TVIN_SIG_STATUS_NULL;
    tvin_info_t signal_info = mSigDetectThread.getCurSigInfo();

    int feState = mFrontDev.getStatus();
    if ( (CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_DTV ) ) {
        det_fmt = CTvin::getInstance()->TvinApi_Get3DDectMode();
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

int CTv::Tv_GetHistgram(int *histgram_buf)
{
    if (histgram_buf == NULL) {
        return -1;
    }
    return CTvin::getInstance()->VDIN_GetHistgram(histgram_buf);
}

int CTv::Tv_Set3DMode ( VIDEO_3D_MODE_T mode )
{
    if (mode == VIDEO_3D_MODE_AUTO) {
        CTvin::getInstance()->VDIN_SetDI3DDetc (1);
    } else {
        CTvin::getInstance()->VDIN_SetDI3DDetc (0);
    }

    mAv.set3DMode(mode, 0, 0);
    m_mode_3d = mode;
    SSMSave3DMode ( ( unsigned char ) mode );
    return 0;
}

VIDEO_3D_MODE_T CTv::Tv_Get3DMode ( void )
{
    return m_mode_3d;
}

int CTv::Tv_Set3DLRSwith ( int on_off)
{
    LOGW ( "%s,Set3D LRSwith on_off %d",  __FUNCTION__, on_off);
    mAv.set3DMode(m_mode_3d, on_off, 0);
    SSMSave3DLRSwitch(on_off);
    return 0;
}

int CTv::Tv_Get3DLRSwith ( void )
{
    unsigned char val = 0;
    SSMRead3DLRSwitch ( &val );
    return ( int ) val;
}

int CTv::Tv_Set3DTo2DMode ( int mode)
{
    LOGW ( "%s,Set3D to 2D mode %d",  __FUNCTION__  , mode);
    mAv.set3DMode(m_mode_3d, 0, mode);
    SSMSave3DTO2D ( mode );
    return 0;
}

int CTv::Tv_Get3DTo2DMode ( void )
{
    unsigned char val = 0;
    SSMRead3DTO2D ( &val );
    return val;
}

int CTv::Tv_Set3DDepth ( int value )
{
    CTvin::getInstance()->Tvin_SetDepthOf2Dto3D( value );
    SSMSave3DDepth ( value );
    return 0;
}

int CTv::GetSave3DDepth ( void )
{
    unsigned char val = 0;
    SSMRead3DDepth ( &val );
    return val;
}

is_3d_type_t CTv::Check2Dor3D ( VIDEO_3D_MODE_T mode3d, tvin_trans_fmt_t trans_fmt )
{
    if ( mode3d == VIDEO_3D_MODE_DISABLE ) {
        return INDEX_2D;
    } else if ( mode3d == VIDEO_3D_MODE_AUTO ) {
        if ( trans_fmt == TVIN_TFMT_2D ) {
            return INDEX_2D;
        } else {
            return INDEX_3D;
        }
    } else {
        return INDEX_3D;
    }
}

int CTv::Tvin_SetPLLValues ()
{
    tvin_sig_fmt_t sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);
    am_regs_t regs;

    if ( CVpp::getInstance()->getPqData()->PQ_GetPLLParams ( source_port, sig_fmt, &regs ) == 0 ) {
        LOGD ("%s,PQ_GetPLLParams(source_port[%d], sig_fmt[%d],&regs).\n", __FUNCTION__, source_port, sig_fmt );

        if ( CTvin::getInstance()->TvinApi_LoadPLLValues ( regs ) < 0 ) {
            LOGE ( "%s, TvinApi_LoadPLLValues failed!\n", __FUNCTION__ );
            return -1;
        }
    } else {
        LOGE ( "%s, PQ_GetPLLParams failed!\n", __FUNCTION__ );
        return -1;
    }

    return 0;
}

int CTv::SetCVD2Values ()
{
    tvin_sig_fmt_t sig_fmt = mSigDetectThread.getCurSigInfo().fmt;
    tvin_port_t source_port = CTvin::Tvin_GetSourcePortBySourceInput(m_source_input);
    am_regs_t regs;

    if (CVpp::getInstance()->getPqData()->PQ_GetCVD2Params ( source_port, sig_fmt, &regs ) == 0) {
        LOGD ( "%s, PQ_GetCVD2Params(source_port[%d], sig_fmt[%d],&regs).\n", __FUNCTION__,
               source_port, sig_fmt );

        if ( CTvin::getInstance()->TvinApi_LoadCVD2Values ( regs ) < 0 ) {
            LOGE ( "%s, TvinApi_LoadCVD2Values failed!\n", __FUNCTION__);
            return -1;
        }
    } else {
        LOGE ( "%s, PQ_GetCVD2Params failed!\n", __FUNCTION__);
        return -1;
    }

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
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
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
    strcat(MainVolLutTableName, GetMainVolLutTableExtraName());
    if ( GetAudioAmpMainvolBuf(MainVolLutTableName, tmpDefDigitLutBuf) == 0) {
        AudioSetVolumeDigitLUTBuf ( lut_table_index, tmpDefDigitLutBuf);
    }
    return 0;
}

void CTv::RefreshAudioMasterVolume ( tv_source_input_t source_input )
{
    if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
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
    if (source_input == SOURCE_TV) {
        if (CTvin::getInstance()->Tvin_GetAudioInSourceType(source_input) == TV_AUDIO_IN_SOURCE_TYPE_ATV) {
            AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_ATV);
            vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_ATV);
            LOGD("%s,  atv DAC_Digital_PlayBack_Volume = %d\n", __FUNCTION__, vol);
        } else {
            LOGD("%s,  dtv DAC_Digital_PlayBack_Volume = %d\n", __FUNCTION__, vol);
            AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
            vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_LINEIN);
        }
    } else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_LINEIN);
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_HDMI);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_HDMI);
    } else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2 ||
               source_input == SOURCE_VGA) {
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_LINEIN);
    } else if (source_input == SOURCE_MPEG) {
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_HDMI);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_HDMI);
    } else if (source_input == SOURCE_DTV) {
        AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_HDMI);
        vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(CC_AUDIO_IN_SOURCE_HDMI);
        LOGD("%s,  dtv DAC_Digital_PlayBack_Volume = %d\n", __FUNCTION__, vol);
    }
    LOGD("%s,  we have read SetDAC_Digital_PlayBack_Volume = %d\n", __FUNCTION__, vol);

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
               source_input == SOURCE_HDMI3) {
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

void CTv::Tv_ADCDigitalCapture_Volume (void)
{
    int capture_vol = GetADCDigitalCaptureVol_Cfg();
    LOGD("%s,  we have read ADCDigitalCaptureVol_Config = %d \n", __FUNCTION__, capture_vol);
    SetADC_Digital_Capture_Volume(capture_vol);
}

void CTv::Tv_SetPGAIn_Gain (void)
{
    int gain_val = GetAudioInternalDacPGAInGain_Cfg();
    LOGD("%s,  we have read ADCDigitalCaptureVol_Config = %d \n", __FUNCTION__, gain_val);
    SetPGA_IN_Value(gain_val);
}

void CTv::Tv_SetDACDigitalPlayBack_Volume (int audio_src_in_type)
{
    int vol = GetAudioInternalDACDigitalPlayBackVolume_Cfg(audio_src_in_type);
    LOGD("%s,  we have read DACDigitalPlayBackVolume config = %d \n", __FUNCTION__, vol);
    SetDAC_Digital_PlayBack_Volume(vol);
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
    m_blackout_enable = enable;
    return SSMSaveBlackoutEnable(enable);
}

int CTv::getSaveBlackoutEnable()
{
    int8_t enable;
    SSMReadBlackoutEnable(&enable);
    return enable;
}

void CTv::startAutoBackLight()
{
    if (mHdmiOutFbc) {
        Tv_FactorySet_FBC_Auto_Backlight_OnOff(1);
    } else {
        mAutoBackLight.startAutoBacklight(CTvin::Tvin_SourceInputToSourceInputType(m_source_input));
    }
}

void CTv::stopAutoBackLight()
{
    if (mHdmiOutFbc) {
        Tv_FactorySet_FBC_Auto_Backlight_OnOff(0);
    } else {
        mAutoBackLight.stopAutoBacklight();
    }
}

int CTv::getAutoBackLight_on_off()
{
    if (mHdmiOutFbc) {
        return Tv_FactoryGet_FBC_Auto_Backlight_OnOff();
    } else {
        return mAutoBackLight.isAutoBacklightOn() ? 1 : 0;
    }
}

int CTv::getAverageLuma()
{
    return CTvin::getInstance()->VDIN_Get_avg_luma();
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
        LOGD("%s, data is null", __FUNCTION__);
        return -1;
    }

    const char *keyValue = config_get_str(CFG_SECTION_TV, KEY, (char *) "null");
    if (strcmp(keyValue, "null") == 0) {
        LOGD("%s, value is NULL\n", __FUNCTION__);
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

/*********************** SSM  start **********************/
int CTv::Tv_SSMRestoreDefaultSetting()
{
    SSMRestoreDeviceMarkValues();
    AudioSSMRestoreDefaultSetting();
    CVpp::getInstance()->VPPSSMRestoreDefault();
    MiscSSMRestoreDefault();
    ReservedSSMRestoreDefault();
    SSMSaveCVBSStd ( 0 );
    SSMSaveLastSelectSourceInput ( SOURCE_TV );
    SSMSavePanelType ( 0 );
    //tvconfig default
    saveDTVProgramID ( -1 );
    saveATVProgramID ( -1 );
    SSMSaveStandbyMode( 0 );
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
    playProgramLock(progID);
}

int CTv::startCC(int country, int src, int channel, int service)
{
    //turn_on_cc = true;
    return mSubtitle.sub_start_atsc_cc((enum cc_param_country)country, (enum cc_param_source_type)src, channel, (enum cc_param_caption_type)service);
}

int CTv::stopCC()
{
    //because cc,vchip data both come from vbi thread , here judge cc, vchip is whether  both turn off
    /*turn_on_cc = false;
    if (config_get_int(CFG_SECTION_TV,"tv.vchip.enable", 0))
    {
        return 0;  //at ATV if vchip is on, turn off CC, just set flag not display CC, but vchip still running
    }*/
    return mSubtitle.sub_stop_atsc_cc();
}

void CTv::printDebugInfo()
{
    print_version_info();
    LOGD("%s, TvAction = %x",  __FUNCTION__, mTvAction);
    LOGD("%s, TvRunStatus = %d",  __FUNCTION__, mTvStatus);
    LOGD("%s, TvCurSourceInput = %d",  __FUNCTION__, m_source_input);
    LOGD("%s, TvLastSourceInput = %d",  __FUNCTION__, m_last_source_input);
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

void CTv::SetRecCurTsOrCurProgram ( int sel )
{
    mTvRec.SetRecCurTsOrCurProgram ( sel );
}

int CTv::GetDisplayResolutionConfig()
{
    return mAv.getVideoDisplayResolution();
}

int CTv::GetDisplayResolutionInfo()
{
    return Vpp_GetDisplayResolutionInfo(NULL);
}

void CTv::onHDMIRxCECMessage(int msg_len, unsigned char msg_buf[])
{
    TvEvent::HDMIRxCECEvent ev;

    int bufLen = sizeof(ev.mDataBuf) / sizeof(ev.mDataBuf[0]);
    if (msg_len > bufLen) {
        LOGE("%s, message len(%d) > buffer len(%d)", __FUNCTION__, msg_len, bufLen);
        return;
    }

    ev.mDataCount = msg_len;
    for (int i = 0; i < msg_len; i++) {
        ev.mDataBuf[i] = msg_buf[i];
    }
    sendTvEvent(ev);
}

int CTv::SendHDMIRxCECCustomMessage(unsigned char data_buf[])
{
    return mHDMIRxCEC.SendCustomMessage(m_source_input, data_buf);
}

int CTv::SendHDMIRxCECCustomMessageAndWaitReply(unsigned char data_buf[], unsigned char reply_buf[], int WaitCmd, int timeout)
{
    return mHDMIRxCEC.SendCustomMessageAndWaitReply(m_source_input, data_buf, reply_buf, WaitCmd, timeout);
}

int CTv::SendHDMIRxCECBoradcastStandbyMessage(void)
{
    return mHDMIRxCEC.SendBoradcastStandbyMessage(m_source_input);
}

int CTv::SendHDMIRxCECGiveCECVersionMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
    if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
        return mHDMIRxCEC.SendGiveCECVersionMessage(source_input, data_buf);
    }

    return -1;
}

int CTv::SendHDMIRxCECGiveDeviceVendorIDMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
    if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
        return mHDMIRxCEC.SendGiveDeviceVendorIDMessage(source_input, data_buf);
    }

    return -1;
}

int CTv::SendHDMIRxCECGiveOSDNameMessage(tv_source_input_t source_input, unsigned char data_buf[])
{
    if (mHDMIRxCEC.processRefreshSrcDevice(source_input) == 0) {
        return mHDMIRxCEC.SendGiveOSDNameMessage(source_input, data_buf);
    }

    return -1;
}

int CTv::GetHdmiHdcpKeyKsvInfo(int data_buf[])
{
    return CTvin::getInstance()->get_hdmi_ksv_info(m_source_input, data_buf);
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
    if (mpUpgradeFBC != NULL) {
        mpUpgradeFBC->SetUpgradeFileName(file_name);
        mpUpgradeFBC->SetUpgradeMode(mode);
        mpUpgradeFBC->SetUpgradeBlockSize(upgrade_blk_size);
        mpUpgradeFBC->start();
        return 0;
    }

    return -1;
}

void CTv::onSerialCommunication(int dev_id, int rd_len, unsigned char data_buf[])
{
    TvEvent::SerialCommunicationEvent ev;

    if (rd_len > CC_MAX_SERIAL_RD_BUF_LEN) {
        LOGE("%s, rd_len(%d) > buffer len(%d)", __FUNCTION__, rd_len, CC_MAX_SERIAL_RD_BUF_LEN);
        return;
    }

    ev.mDevId = dev_id;
    ev.mDataCount = rd_len;
    for (int i = 0; i < rd_len; i++) {
        ev.mDataBuf[i] = data_buf[i];
    }
    sendTvEvent(ev);
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
            Tv_FactorySet_FBC_Thermal_State(val);
        }
    } else {
        LOGD ( "%s, tvin thermal threshold disable\n", __FUNCTION__);
    }
}

int CTv::SetDebugSerialOnOff(int on_off)
{
    if (on_off) {
        property_set(UBOOTENV_CONSOLE, "ttyS0,115200n8");
    } else {
        property_set(UBOOTENV_CONSOLE, "off");
    }
    return 0;
}

int CTv::GetDebugSerialOnOff()
{
    char prop[256] = {0};
    property_get(UBOOTENV_CONSOLE, prop, "null" );
    if (!strcmp(prop, "ttyS0,115200n8")) {
        return 1;
    } else {
        return 0;
    }
}

int CTv::SetSerialSwitch(int dev_id, int switch_val)
{
    int tmp_ret = 0;
    if (dev_id == SERIAL_A) {
        if (switch_val == 0) {
            tmp_ret |= mSerialA.stop();
            tmp_ret |= mSerialA.CloseModule();

            tmp_ret |= system("start console");
            tmp_ret |= system("echo 7 > /proc/sys/kernel/printk");
        } else {
            tmp_ret |= system("echo 0 > /proc/sys/kernel/printk");
            tmp_ret |= system("stop console");

            mSerialA.OpenModule(dev_id);
            tmp_ret |= mSerialA.start();
        }
    } else if (dev_id == SERIAL_B) {
        if (switch_val == 0) {
            tmp_ret |= mSerialB.stop();
            tmp_ret |= mSerialB.CloseModule();
        } else {
            mSerialB.OpenModule(dev_id);
            tmp_ret = mSerialB.start();
        }
    } else if (dev_id == SERIAL_C) {
        if (switch_val == 0) {
            tmp_ret |= mSerialC.stop();
            tmp_ret |= mSerialC.CloseModule();
        } else {
            mSerialC.OpenModule(dev_id);
            tmp_ret = mSerialC.start();
        }
    }

    return tmp_ret;
}

int CTv::SendSerialData(int dev_id, int data_len, unsigned char data_buf[])
{
    int tmp_ret = 0;

    if (dev_id == SERIAL_A) {
        tmp_ret = mSerialA.sendData(data_len, data_buf);
    } else if (dev_id == SERIAL_B) {
        tmp_ret = mSerialB.sendData(data_len, data_buf);
    } else if (dev_id == SERIAL_C) {
        tmp_ret = mSerialC.sendData(data_len, data_buf);
    }

    return tmp_ret;
}

int CTv::ChannelExport(const char *destPath)
{
    char tmp[256];
    if (destPath == NULL) {
        destPath = "/storage/external_storage/sda1/";
    }

    sprintf(tmp, "cp %s %s", DEF_VALUE_CAHNNEL_DB, destPath);
    if (system(tmp) >= 0) {
        LOGD("%s, copy dtv.db from /param to udisk success !", __FUNCTION__);
        system("sync");
        FILE *fp = fopen(destPath, "r");
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
            return 0;
        }
    }

    return -1;
}

int CTv::ChannelImport(const char *srcPath)
{
    if (srcPath == NULL) {
        srcPath = "/storage/external_storage/sda1/dvb.db";
    }

    if (Tv_Utils_IsFileExist(srcPath)) {
        char tmp[256];
        const char *destPath = config_get_str(CFG_SECTION_TV, CFG_CAHNNEL_DB, (char *) DEF_VALUE_CAHNNEL_DB);

        LOGD("%s, file exist !" , srcPath);
        CTvDatabase::GetTvDb()->UnInitTvDb();
        sprintf(tmp, "rm %s", destPath);
        if (system(tmp) >= 0) {
            LOGD("%s, rm %s success !", __FUNCTION__, destPath);
            memset(tmp, 0, sizeof(tmp));
            sprintf(tmp, "cp %s %s", srcPath, destPath);
            if (system(tmp) >= 0) {
                LOGD("%s,  copy  to %s success !", srcPath, destPath);
                memset(tmp, 0, sizeof(tmp));
                sprintf(tmp, "chmod 777 %s", destPath);
                if (system(tmp) >= 0) {
                    LOGD("chmod 777 %s success !", destPath);
                    system("sync");
                    CTvDatabase::GetTvDb()->InitTvDb(TV_DB_PATH);
                    return 0;
                } else {
                    LOGD("%s, %s chmod failed !", __FUNCTION__, destPath);
                    return -1;
                }
            } else {
                LOGD("%s, copy dtv.db from udisk to %s failed !", __FUNCTION__, destPath);
                return -1;
            }
        } else {
            LOGD("%s, rm %s failed !", __FUNCTION__, destPath);
            return -2;
        }
    } else {
        LOGD("%s, dtv.db file does not exist in the udisk!" , srcPath);
        return -2;
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

int CTv::Tv_HandeHDMIEDIDFilePathConfig()
{
    char edid_path[256];
    char edid_path_cfg[256];
    bool fileExist = false;

    const char *value = config_get_str(CFG_SECTION_TV, CFG_SSM_HDMI_EDID_EN, "null");
    if (strtoul(value, NULL, 10) == 1) {
        LOGD( "%s, get config [%s]:[%s]\n", __FUNCTION__, CFG_SSM_HDMI_EDID_EN, value);
        //get hdmi edid use mode
        char propValue[256] = {0};
        property_get(UBOOTENV_OUTPUTMODE, propValue, "null");
        LOGD( "%s, get property [%s]:[%s]\n", __FUNCTION__, UBOOTENV_OUTPUTMODE, propValue);
        if (strcmp(propValue, "null") != 0) {
            fileExist = true;

            config_set_str ( CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, propValue);
            //set file's path for hdmi edid of each port
            for (int i = 1; i <= SSM_HDMI_PORT_MAX; i++) {
                memset( edid_path, '\0', 256);
                memset( edid_path_cfg, '\0', 256);
                sprintf(edid_path, "/system/etc/%s_port%d.bin", propValue, i);
                sprintf(edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", i);
                if (access(edid_path, 0) < 0) {
                    fileExist = false;
                    break;
                }
                config_set_str(CFG_SECTION_TV, edid_path_cfg, edid_path);
            }
        }

        if (!fileExist) {
            //set default hdmi edid
            config_set_str(CFG_SECTION_TV, CS_HDMI_EDID_USE_CFG, "hdmi_edid");
            //set file's path for hdmi edid of each port
            for (int i = 1; i <= SSM_HDMI_PORT_MAX; i++) {
                memset(edid_path, '\0', 256);
                memset(edid_path_cfg, '\0', 256);
                sprintf(edid_path, "/system/etc/%s_port%d.bin", "hdmi_edid", i);
                sprintf(edid_path_cfg, "ssm.handle.hdmi.port%d.edid.file.path", i);

                value = config_get_str(CFG_SECTION_TV, edid_path_cfg, "null");
                if (strcmp(value, edid_path) != 0) {
                    config_set_str(CFG_SECTION_TV, edid_path_cfg, edid_path);
                }
            }
        }

        mSetHdmiEdid = true;
    }
    return 0;
}

int CTv::Tv_SetDDDRCMode(tv_source_input_t source_input)
{
    if (source_input == SOURCE_DTV) {
        if (GetPlatformHaveDDFlag() == 1) {
            Tv_Utils_SetFileAttrStr(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 3");
        }
    } else {
        if (GetPlatformHaveDDFlag() == 1) {
            Tv_Utils_SetFileAttrStr(SYS_AUIDO_DSP_AC3_DRC, (char *)"drcmode 2");
        }
    }
    return 0;
}

//PQ
int CTv::Tv_SetBrightness ( int brightness, tv_source_input_type_t source_type, int is_save )
{
    return CVpp::getInstance()->SetBrightness(brightness, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetBrightness ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetBrightness((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveBrightness ( int brightness, tv_source_input_type_t source_type )
{
    return SSMSaveBrightness ( source_type, brightness );
}

int CTv::Tv_SetContrast ( int contrast, tv_source_input_type_t source_type,  int is_save )
{
    return CVpp::getInstance()->SetContrast(contrast, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetContrast ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetContrast((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveContrast ( int contrast, tv_source_input_type_t source_type )
{
    return SSMSaveContrast ( source_type, contrast );
}

int CTv::Tv_SetSaturation ( int satuation, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
    return CVpp::getInstance()->SetSaturation(satuation, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetSaturation ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetSaturation((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveSaturation ( int satuation, tv_source_input_type_t source_type )
{
    return SSMSaveSaturation ( source_type, satuation );
}

int CTv::Tv_SetHue ( int hue, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
    return CVpp::getInstance()->SetHue(hue, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

int CTv::Tv_GetHue ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetHue((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveHue ( int hue, tv_source_input_type_t source_type )
{
    return SSMSaveHue ( source_type, hue );
}

int CTv::Tv_SetPQMode ( vpp_picture_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
    return CVpp::getInstance()->SetPQMode((vpp_picture_mode_t)mode, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), is_save);
}

vpp_picture_mode_t CTv::Tv_GetPQMode ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetPQMode((tv_source_input_type_t)source_type);
}

int CTv::Tv_SavePQMode ( vpp_picture_mode_t mode, tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->SavePQMode ( mode, source_type );
}

int CTv::Tv_SetSharpness ( int value, tv_source_input_type_t source_type, int en, int is_save )
{
    return CVpp::getInstance()->SetSharpness(value, (tv_source_input_type_t)source_type, en, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().fmt, mSigDetectThread.getCurSigInfo().trans_fmt, is_save);
}

int CTv::Tv_GetSharpness ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetSharpness((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveSharpness ( int value, tv_source_input_type_t source_type )
{
    return SSMSaveSharpness ( source_type, value );
}

int CTv::Tv_SetBacklight ( int value, tv_source_input_type_t source_type, int is_save )
{
    if (mHdmiOutFbc) {
        return Tv_FactorySet_FBC_Backlight(value);
    } else {
        return CVpp::getInstance()->SetBacklight(value, (tv_source_input_type_t)source_type, is_save);
    }
}

int CTv::Tv_GetBacklight ( tv_source_input_type_t source_type )
{
    if (mHdmiOutFbc) {
        return Tv_FactoryGet_FBC_Backlight();
    } else {
        return CVpp::getInstance()->GetBacklight((tv_source_input_type_t)source_type);
    }
}

int CTv::Tv_SaveBacklight ( int value, tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->SaveBacklight ( value, source_type );
}

int CTv::Tv_SetBacklight_Switch ( int value )
{
    if (mHdmiOutFbc) {
        return Tv_FactorySet_FBC_backlight_onoff(value);
    } else {
        return CVpp::getInstance()->VPP_SetBackLight_Switch(value);
    }
}

int CTv::Tv_GetBacklight_Switch ( void )
{
    if (mHdmiOutFbc) {
        return Tv_FactoryGet_FBC_backlight_onoff();
    } else {
        return CVpp::getInstance()->VPP_GetBackLight_Switch();
    }
}

int CTv::Tv_SetColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
    if (mHdmiOutFbc) {
        return Tv_FactorySet_FBC_ColorTemp_Mode(mode);
    } else {
        return CVpp::getInstance()->SetColorTemperature((vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
    }
}

vpp_color_temperature_mode_t CTv::Tv_GetColorTemperature ( tv_source_input_type_t source_type )
{
    if (mHdmiOutFbc) {
        return (vpp_color_temperature_mode_t)Tv_FactoryGet_FBC_ColorTemp_Mode();
    } else {
        return CVpp::getInstance()->GetColorTemperature((tv_source_input_type_t)source_type);
    }
}

int CTv::Tv_SaveColorTemperature ( vpp_color_temperature_mode_t mode, tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->SaveColorTemp ( mode, source_type );
}

int CTv::Tv_SetDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, int is_save )
{
    int ret = 0;
    if (m_mode_3d == VIDEO_3D_MODE_DISABLE) {
        ret = SetDisplayMode((vpp_display_mode_t)mode, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt);
    } else { //3D
        ret = SetDisplayMode(VPP_DISPLAY_MODE_PERSON, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt);
    }
    if (ret == 0) {
        if (is_save == 1) {
            ret = ret | SSMSaveDisplayMode ( source_type, (int)mode );
        }
    }
    return ret;
}

int CTv::SetDisplayMode ( vpp_display_mode_t display_mode, tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt )
{
    LOGD("SetDisplayMode, display_mode = %d, source_type = %d fmt = %d  tranfmt = %d\n",  display_mode, source_type, sig_fmt, mSigDetectThread.getCurSigInfo().trans_fmt);

    tvin_cutwin_t cutwin = CVpp::getInstance()->GetOverscan ( source_type, sig_fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt);
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
        //CVpp::getInstance()->VPP_SetNonLinearFactor ( 20 );
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
    if (source_type == SOURCE_TYPE_DTV || source_type == SOURCE_TYPE_MPEG) {
        cutwin.vs = cutwin.vs + 12;
        cutwin.ve = cutwin.ve + 12;
        cutwin.hs = cutwin.hs + 12;
        cutwin.he = cutwin.he + 12;
    }
    if (source_type == SOURCE_TYPE_HDMI) {
        if ((IsDVISignal()) || (CTvin::getInstance()->GetITContent() == 1) ||
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

vpp_display_mode_t CTv::Tv_GetDisplayMode ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetDisplayMode((tv_source_input_type_t)source_type);
}

int CTv::Tv_SaveDisplayMode ( vpp_display_mode_t mode, tv_source_input_type_t source_type )
{
    return SSMSaveDisplayMode ( source_type, (int)mode );
}

int CTv::Tv_SetNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type, int is_save )
{
    return CVpp::getInstance()->SetNoiseReductionMode((vpp_noise_reduction_mode_t)mode, (tv_source_input_type_t)source_type, mSigDetectThread.getCurSigInfo().fmt, Check2Dor3D(m_mode_3d, mSigDetectThread.getCurSigInfo().trans_fmt ), mSigDetectThread.getCurSigInfo().trans_fmt, is_save);
}

int CTv::Tv_SaveNoiseReductionMode ( vpp_noise_reduction_mode_t mode, tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->SaveNoiseReductionMode ( mode, source_type );
}

int CTv::Tv_Set2k4k_ScalerUp_Mode ( int value )
{
    int ret = 0, fmt_hd_sd = 0;
    int nodeVal = 0;
    char s[8];

    LOGD("%s, value [%d]\n", __FUNCTION__ , value);

    if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI &&
            CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_AV &&
            CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_TV) {
        int fd = open(SYS_VIDEO_FRAME_HEIGHT, O_RDONLY);
        if (fd <= 0) {
            LOGE("open %s ERROR!!error = -%s- \n",SYS_VIDEO_FRAME_HEIGHT, strerror ( errno ));
            return -1;
        }
        read(fd, s, sizeof(s));
        close(fd);
        nodeVal = atoi(s);
        if (nodeVal < 900) {
            fmt_hd_sd = 0;
        } else {
            fmt_hd_sd = 1;
        }
    } else if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
        if (isSDFmtInHdmi()) {
            fmt_hd_sd = 0;
        } else {
            fmt_hd_sd = 1;
        }
    } else if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_AV ||
                CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_TV) {
        fmt_hd_sd = 0;
    }

    switch (value) {
    case 1: //  on
        if (fmt_hd_sd) {
            ret = Tv_Utils_SetFileAttrStr(SYS_VIDEO_SCALER_PATH_SEL, (char *)"0");
        } else {
            ret = Tv_Utils_SetFileAttrStr(SYS_VIDEO_SCALER_PATH_SEL, (char *)"1");
        }
        break;

    case 0: //  off
    default:
        if (fmt_hd_sd) {
            ret = Tv_Utils_SetFileAttrStr(SYS_VIDEO_SCALER_PATH_SEL, (char *)"1");
        } else {
            ret = Tv_Utils_SetFileAttrStr(SYS_VIDEO_SCALER_PATH_SEL, (char *)"0");
        }
        break;
    }
    return ret;
}

int CTv::Tv_Get2k4k_ScalerUp_Mode ( void )
{
    char attrV[64];
    int ret = 0, fmt_hd_sd = 0;
    int nodeVal = 0;
    char s[8];

    memset (attrV, '\0', 64);

    if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_HDMI &&
            CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_AV &&
            CTvin::Tvin_SourceInputToSourceInputType(m_source_input) != SOURCE_TYPE_TV) {
        int fd = open(SYS_VIDEO_FRAME_HEIGHT, O_RDONLY);
        if (fd <= 0) {
            LOGE("open %s ERROR!!error = -%s- \n",SYS_VIDEO_FRAME_HEIGHT, strerror ( errno ));
            return -1;
        }
        read(fd, s, sizeof(s));
        close(fd);
        nodeVal = atoi(s);
        if (nodeVal < 900) {
            fmt_hd_sd = 0;
        } else {
            fmt_hd_sd = 1;
        }
    } else if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_HDMI ) {
        if (isSDFmtInHdmi()) {
            fmt_hd_sd = 0;
        } else {
            fmt_hd_sd = 1;
        }
    } else if ( CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_AV ||  CTvin::Tvin_SourceInputToSourceInputType(m_source_input) == SOURCE_TYPE_TV) {
        fmt_hd_sd = 0;
    }

    Tv_Utils_GetFileAttrStr(SYS_VIDEO_SCALER_PATH_SEL, 64, attrV);
    if (strncasecmp(attrV, "1", strlen ("1")) == 0) {
        if (fmt_hd_sd) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else if (strncasecmp(attrV, "0", strlen ("0")) == 0) {
        if (fmt_hd_sd) {
            ret = 1;
        } else {
            ret = 0;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CTv::Tv_SetSplitScreenDemoStatus(tv_source_input_type_t source_type, int onoff_status)
{
    int ret = 0;
    LOGD("%s, split screen demo status source[%d],onoff_stauts[%d]\n", __FUNCTION__ , source_type, onoff_status);
    if (1 == onoff_status) {
        ret = Tv_Utils_SetFileAttrStr(SYS_DROILOGIC_DEBUG, (char *)"w 0x503c0 v 0x31d6");//sharpness screen left
        ret |= Tv_Utils_SetFileAttrStr(SYS_DROILOGIC_DEBUG, (char *)"w 0x07 v 0x174d");//nr screen left
    } else if (0 == onoff_status) {
        ret = Tv_Utils_SetFileAttrStr(SYS_DROILOGIC_DEBUG, (char *)"w 0x503c0 v 0x31d6");//sharpness screen left
        ret |= Tv_Utils_SetFileAttrStr(SYS_DROILOGIC_DEBUG, (char *)"w 0x07 v 0x174d");//nr screen left
    }

    if (fbcIns != NULL && 0 == ret) {
        ret |= fbcIns->cfbc_SET_SPLIT_SCREEN_DEMO(COMM_DEV_SERIAL, onoff_status);
    }

    return ret;
}

int CTv::Tv_GetSplitScreenDemoStatus(tv_source_input_type_t source_type)
{
    source_type = source_type;
    return 0;
}

vpp_noise_reduction_mode_t CTv::Tv_GetNoiseReductionMode ( tv_source_input_type_t source_type )
{
    return CVpp::getInstance()->GetNoiseReductionMode((tv_source_input_type_t)source_type);
}

/**
 * @parameter mode : 0:sharpness; 1:NR
 *
    1.sharpness分屏（可以控制宽度）
    分屏大小 寄存器范围是0x50000 冿x50780,左效果的范围越来越大?0x70780 冿x70000也是一枿
    左右调换控制 bit 17. 开僿控冿bit 16. 0-11bit 是控制宽度的。范围是0-0x780?920像素?
    左做效果处理  echo w 0x503c0 v 0x31d6 > /sys/class/amlogic/debug
    右做效果处理  echo w 0x703c0 v 0x31d6 > /sys/class/amlogic/debug
    echo w 0x403c0 v 0x31d6 > /sys/class/amlogic/debug

    2. NR分屏(不能控制宽度)
    左做效果处理 echo w 0x07 v 0x174d > /sys /class/amlogic/debug
    右做效果处理 echo w 0x70 v 0x174d > /sys /class/amlogic/debug
    偿                     echo w 0x77 v 0x174d > /sys/class/amlogic/debug
 */
int CTv::Tv_SplitScreenEffect(int mode, int width, int reverse)
{
    int ret = -1;

    if (mode == 0) {
        if (width == 0) {//close
            ret = CVpp::getInstance()->VPP_SplitScreenEffect(0x403c0, 0x31d6);
        } else {
            int val = 0;
            if (reverse == 0) {//left effect
                val = width + 0x50000;
            } else {//right effect
                val = 0x70780 - width;
            }
            ret = CVpp::getInstance()->VPP_SplitScreenEffect(val, 0x31d6);
        }
    } else if (mode == 1) {
        if (width == 0) {//close
            ret = CVpp::getInstance()->VPP_SplitScreenEffect(0x77, 0x174d);
        } else {
            if (reverse == 0)
                ret = CVpp::getInstance()->VPP_SplitScreenEffect(0x07, 0x174d);
            else
                ret = CVpp::getInstance()->VPP_SplitScreenEffect(0x70, 0x174d);
        }
    } else if (mode == 2) {
    }

    return ret;
}

int CTv::Tv_FactorySetPQMode_Brightness ( int source_type, int pq_mode, int brightness )
{
    return CVpp::getInstance()->FactorySetPQMode_Brightness(source_type, pq_mode, brightness);
}

int CTv::Tv_FactoryGetPQMode_Brightness ( int source_type, int pq_mode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Brightness(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Contrast ( int source_type, int pq_mode, int contrast )
{
    return CVpp::getInstance()->FactorySetPQMode_Contrast(source_type, pq_mode, contrast);
}

int CTv::Tv_FactoryGetPQMode_Contrast ( int source_type, int pq_mode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Contrast(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Saturation ( int source_type, int pq_mode, int saturation )
{
    return CVpp::getInstance()->FactorySetPQMode_Saturation(source_type, pq_mode, saturation);
}

int CTv::Tv_FactoryGetPQMode_Saturation ( int source_type, int pq_mode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Saturation(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Hue ( int source_type, int pq_mode, int hue )
{
    return CVpp::getInstance()->FactorySetPQMode_Hue(source_type, pq_mode, hue);
}

int CTv::Tv_FactoryGetPQMode_Hue ( int source_type, int pq_mode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Hue(source_type, pq_mode);
}

int CTv::Tv_FactorySetPQMode_Sharpness ( int source_type, int pq_mode, int sharpness )
{
    return CVpp::getInstance()->FactorySetPQMode_Sharpness(source_type, pq_mode, sharpness);
}

int CTv::Tv_FactoryGetPQMode_Sharpness ( int source_type, int pq_mode )
{
    return CVpp::getInstance()->FactoryGetPQMode_Sharpness(source_type, pq_mode);
}

int CTv::GetColorTemperatureParams ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
    if (mHdmiOutFbc) {
        int ret = Tv_FactoryGet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)Tempmode, params);
        params->r_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->r_gain);
        params->g_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->g_gain);
        params->b_gain = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params->b_gain);
        params->r_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->r_post_offset);
        params->g_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->g_post_offset);
        params->b_post_offset = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params->b_post_offset);
        return ret;
    } else {
        return CVpp::getInstance()->GetColorTemperatureParams(Tempmode, params);
    }
}

int CTv::Tv_FactorySetTestPattern ( int pattern )
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

int CTv::Tv_FactoryGetTestPattern ( void )
{
    return CVpp::getInstance()->FactoryGetTestPattern();
}

int CTv::Tv_FactorySetScreenColor ( int vdin_blending_mask, int y, int u, int v )
{
    return mAv.SetVideoScreenColor ( vdin_blending_mask, y, u, v );
}

int CTv::Tv_FactoryResetPQMode ( void )
{
    return CVpp::getInstance()->FactoryResetPQMode();
}

int CTv::Tv_FactoryResetColorTemp ( void )
{
    return CVpp::getInstance()->FactoryResetColorTemp();
}

int CTv::Tv_FactorySetParamsDefault ( void )
{
    return CVpp::getInstance()->FactorySetParamsDefault();
}

int CTv::Tv_FactorySetDDRSSC ( int step )
{
    return CVpp::getInstance()->FactorySetDDRSSC(step);
}

int CTv::Tv_FactoryGetDDRSSC ( void )
{
    return CVpp::getInstance()->FactoryGetDDRSSC();
}

int CTv::Tv_FactorySetLVDSSSC ( int step )
{
    return CVpp::getInstance()->FactorySetLVDSSSC(step);
}

int CTv::Tv_FactoryGetLVDSSSC ( void )
{
    return CVpp::getInstance()->FactoryGetLVDSSSC();
}

void CTv::Tv_Spread_Spectrum()
{
    int value = 0;
    value = CVpp::getInstance()->FactoryGetLVDSSSC();
    CVpp::getInstance()->FactorySetLVDSSSC(value);
}

int CTv::Tv_FactorySetNolineParams ( int noline_params_type, int source_type, noline_params_t noline_params )
{
    return CVpp::getInstance()->FactorySetNolineParams(noline_params_type, source_type, noline_params);
}

noline_params_t CTv::Tv_FactoryGetNolineParams ( int noline_params_type, int source_type )
{
    return CVpp::getInstance()->FactoryGetNolineParams(noline_params_type, source_type);
}

int CTv::Tv_FactorySetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt, tvin_cutwin_t cutwin_t )
{
    //tvin_cutwin_t cutwin_t = CVpp::getInstance()->Tv_FactoryGetOverscan(source_type, fmt, status_3d, trans_fmt);
    CVpp::getInstance()->FactorySetOverscan(source_type, fmt, status_3d, trans_fmt, cutwin_t);
    //} else {
#if 0
    char val_buf[256];
    memset(val_buf, '\0', 256);
    LOGD("%s,%d: %d,%d,%d,%d", __FUNCTION__, __LINE__, ( int ) cutwin_t.vs, ( int ) cutwin_t.hs, ( int ) cutwin_t.ve, ( int ) cutwin_t.he);
    sprintf(val_buf, "%d,%d,%d,%d", ( int ) cutwin_t.vs, ( int ) cutwin_t.hs, ( int ) cutwin_t.ve, ( int ) cutwin_t.he );
    config_set_str(CFG_SECTION_TV, "vpp.overscan.dtv", val_buf);
    //}
#endif
    return CVpp::getInstance()->VPP_SetVideoCrop ( ( int ) cutwin_t.vs, ( int ) cutwin_t.hs, ( int ) cutwin_t.ve, ( int ) cutwin_t.he );
}

tvin_cutwin_t CTv::Tv_FactoryGetOverscan ( int source_type, int fmt, int status_3d, int trans_fmt )
{
    status_3d = status_3d;
    return CVpp::getInstance()->FactoryGetOverscan(source_type, fmt, Check2Dor3D(m_mode_3d, (tvin_trans_fmt_t)trans_fmt), trans_fmt);
#if 0
    else {
        tvin_cutwin_t  cutwin;
        int ret = 0;
        char tmp_buf[16];
        tmp_buf[0] = 0;

        if ((ret |= cfg_get_one_item("vpp.overscan.dtv", ",", 0, tmp_buf)) == 0) {
            cutwin.vs = strtol(tmp_buf, NULL, 10);
        }
        if ((ret |= cfg_get_one_item("vpp.overscan.dtv", ",", 1, tmp_buf)) == 0) {
            cutwin.hs = strtol(tmp_buf, NULL, 10);
        }
        if ((ret |= cfg_get_one_item("vpp.overscan.dtv", ",", 2, tmp_buf)) == 0) {
            cutwin.ve = strtol(tmp_buf, NULL, 10);
        }
        if ((ret |= cfg_get_one_item("vpp.overscan.dtv", ",", 3, tmp_buf)) == 0) {
            cutwin.he = strtol(tmp_buf, NULL, 10);
        }
        LOGD("%s,%d: %d,%d,%d,%d", __FUNCTION__, __LINE__, ( int ) cutwin.vs, ( int ) cutwin.hs, ( int ) cutwin.ve, ( int ) cutwin.he);
        return cutwin;
    }
#endif
}

int CTv::Tv_ReplacePqDb(const char *newFilePath)
{
    return CVpp::getInstance()->getPqData()->replacePqDb(newFilePath);
}

int CTv::Tv_FactorySet_FBC_Brightness ( int value )
{
    //int temp_value = (255*value)/100;
    int temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Brightness(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_Brightness  ( void )
{
    int temp_value = 0;
    int data = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Brightness(COMM_DEV_SERIAL, &temp_value);
        //data = (temp_value*100)/255;
        data = temp_value;
        return data;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_Contrast ( int value )
{
    //int temp_value = (255*value)/100;
    int temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Contrast(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_Contrast  ( void )
{
    int temp_value = 0;
    int data = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Contrast(COMM_DEV_SERIAL, &temp_value);
        //data = (temp_value*100)/255;
        data = temp_value;
        return data;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_Saturation ( int value )
{
    //int temp_value = (255*value)/100;
    int temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Saturation(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_Saturation  ( void )
{
    int temp_value = 0;
    int data = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Saturation(COMM_DEV_SERIAL, &temp_value);
        //data = (temp_value*100)/255;
        data = temp_value;
        return data;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_HueColorTint ( int value )
{
    //int temp_value = (255*value)/100;
    int temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_HueColorTint(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_HueColorTint ( void )
{
    int temp_value = 0;
    int data = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_HueColorTint(COMM_DEV_SERIAL, &temp_value);
        //data = (temp_value*100)/255;
        data = temp_value;
        return data;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_Backlight ( int value )
{
    int temp_value = value;
    if (fbcIns != NULL) {
        temp_value = temp_value * 255 / 100;
        fbcIns->cfbc_Set_Backlight(COMM_DEV_SERIAL, temp_value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_Backlight ( void )
{
    int temp_value = 0;
    int data = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Backlight(COMM_DEV_SERIAL, &temp_value);
        if (temp_value * 100 % 255 == 0)
            temp_value = temp_value * 100 / 255;
        else
            temp_value = temp_value * 100 / 255 + 1;
        data = temp_value;
        return data;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Auto_Backlight_OnOff( unsigned char status)
{
    if (fbcIns != NULL) {
        return fbcIns->cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_SERIAL, status);
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Auto_Backlight_OnOff( void )
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_ELEC_MODE( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_SERIAL, value);
        SSMSaveFBCELECmodeVal(value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactoryGet_FBC_ELEC_MODE( void )
{
    int val = 0;
    SSMReadFBCELECmodeVal(&val);
    return val;
}

int CTv::Tv_FactorySet_FBC_BACKLIGHT_N360( int value )
{
    SSMSaveFBCELECmodeVal(value);
    return -1;
}

int CTv::Tv_FactoryGet_FBC_BACKLIGHT_N360( void )
{
    int val = 0;
    SSMReadFBCELECmodeVal(&val);
    return val;
}

int CTv::Tv_FactorySet_FBC_Thermal_State( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Thermal_state(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactorySet_FBC_Picture_Mode ( int mode )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Picture_Mode(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Picture_Mode ( void )
{
    int mode = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Picture_Mode(COMM_DEV_SERIAL, &mode);
        return mode;
    }
    return 0;
}

int CTv::Tv_FactorySet_FBC_Set_Test_Pattern ( int mode )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Test_Pattern(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Get_Test_Pattern ( void )
{
    int mode = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Test_Pattern(COMM_DEV_SERIAL, &mode);
        return mode;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Gain_Red( int value )
{
    int temp_value = 0;

    //temp_value = (value*255)/2047;
    //value 0 ~ 2047
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Gain_Red(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Red ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Gain_Red(COMM_DEV_SERIAL, &temp_value);
        //value 0 ~ 2047
        //value = (temp_value*2047)/255;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Gain_Green( int value )
{
    int temp_value = 0;

    //temp_value = (value*255)/2047;
    //value 0 ~ 2047
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Gain_Green(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Green ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Gain_Green(COMM_DEV_SERIAL, &temp_value);
        //value 0 ~ 2047
        //value = (temp_value*2047)/255;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactoryGet_FBC_VIDEO_MUTE ( void )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_VMute(COMM_DEV_SERIAL, 1);
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Gain_Blue( int value )
{
    int temp_value = 0;

    //temp_value = (value*255)/2047;
    //value 0 ~ 2047
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Gain_Blue(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Gain_Blue ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Gain_Blue(COMM_DEV_SERIAL, &temp_value);
        //value 0 ~ 2047
        //value = (temp_value*2047)/255;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Red( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Offset_Red(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Red ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Offset_Red(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Green( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Offset_Green(COMM_DEV_SERIAL, temp_value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Green ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Offset_Green(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Offset_Blue( int value )
{
    //value -1024~+1023
    int temp_value = 0;

    //temp_value = (value+1024)*255/2047;
    temp_value = value;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Offset_Blue(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Offset_Blue ( void )
{
    int temp_value = 0, value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Offset_Blue(COMM_DEV_SERIAL, &temp_value);
        //value -1024~+1023
        //value = (temp_value*2047)/255 - 1024;
        value = temp_value;

        return value;
    }

    return 0;
}

int CTv::Tv_FactoryGetWhiteBalanceRedGain(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        LOGD("--------- call none fbc method ---------");
        ret = CVpp::getInstance()->FactoryGetColorTemp_Rgain(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        LOGD("--------- call fbc method ---------");
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 0);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceGreenGain(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Ggain(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 1);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceBlueGain(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Bgain(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 2);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceRedOffset(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Roffset(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 3);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceGreenOffset(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Goffset(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 4);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceBlueOffset(int source_type, int colortemp_mode)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactoryGetColorTemp_Boffset(source_type, colortemp_mode);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGetItemFromBatch((vpp_color_temperature_mode_t)colortemp_mode, 5);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceRedGain(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Rgain(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the red gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Rgain(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
        ret = Tv_FactorySet_FBC_Gain_Red(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGreenGain(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Ggain(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the green gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Ggain(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
        ret = Tv_FactorySet_FBC_Gain_Green(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceBlueGain(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Bgain(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the blue gain to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Bgain(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(value);
        ret = Tv_FactorySet_FBC_Gain_Blue(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceRedOffset(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Roffset(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the red offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Roffset(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
        ret = Tv_FactorySet_FBC_Offset_Red(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGreenOffset(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Goffset(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the green offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Goffset(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
        ret = Tv_FactorySet_FBC_Offset_Green(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceBlueOffset(int source_type, int colortemp_mode, int value)
{
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->FactorySetColorTemp_Boffset(source_type, colortemp_mode, value);
        if (ret != -1) {
            LOGD("save the blue offset to flash");
            ret = CVpp::getInstance()->FactorySaveColorTemp_Boffset(source_type, colortemp_mode, value);
        }
    } else { //use fbc store the white balance params
        value = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(value);
        ret = Tv_FactorySet_FBC_Offset_Blue(value);
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceColorTempMode(int source_type, int colortemp_mode, int is_save)
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->SetColorTemperature((vpp_color_temperature_mode_t)colortemp_mode, (tv_source_input_type_t)source_type, is_save);
    } else { //use fbc store the white balance params
        ret = Tv_FactorySet_FBC_ColorTemp_Mode(colortemp_mode);
    }
    return ret;
}

int CTv::Tv_FactoryGetWhiteBalanceColorTempMode(int source_type )
{
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->GetColorTemperature((tv_source_input_type_t)source_type);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryGet_FBC_ColorTemp_Mode();
    }
    return ret;
}

int CTv::Tv_FactoryWhiteBalanceFormatInputFbcGainParams(int value)
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

int CTv::Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(int value)
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

int CTv::Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(int value)
{
    if (value == 255) {
        value = 1023;
    } else {
        value = value << 3;
        value -= 1024;
    }
    return value;
}

int CTv::Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(int value)
{
    value = value << 3;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    return value;
}

int CTv::Tv_FactorySaveWhiteBalancePramas(int source_type, int tempmode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset)
{
    int ret = 0;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        CVpp::getInstance()->SaveColorTemp((vpp_color_temperature_mode_t) tempmode, (tv_source_input_type_t) source_type);
        CVpp::getInstance()->FactorySaveColorTemp_Rgain(source_type, tempmode, r_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Ggain(source_type, tempmode, g_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Bgain(source_type, tempmode, b_gain);
        CVpp::getInstance()->FactorySaveColorTemp_Roffset(source_type, tempmode, r_offset);
        CVpp::getInstance()->FactorySaveColorTemp_Goffset(source_type, tempmode, g_offset);
        CVpp::getInstance()->FactorySaveColorTemp_Boffset(source_type, tempmode, b_offset);
    } else { //use fbc store the white balance params
        tcon_rgb_ogo_t params;

        params.r_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(r_gain);
        params.g_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(g_gain);
        params.b_gain = Tv_FactoryWhiteBalanceFormatInputFbcGainParams(b_gain);
        params.r_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(r_offset);
        params.g_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(g_offset);
        params.b_post_offset = Tv_FactoryWhiteBalanceFormatInputFbcOffsetParams(b_offset);
        ret = Tv_FactorySet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)tempmode, params);
    }
    return ret;
}

/**
* The color temperature enum order is diffrent bettewn G9 and Fbc, so we have to make a mapping
**/
int CTv::Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(int Tempmode)
{
    int ret = Tempmode;
    switch (Tempmode) {
    case 0:     //standard
        ret = 1;
        break;
    case 1:     //warm
        ret = 2;
        break;
    case 2:    //cold
        ret = 0;
        break;
    default:
        break;
    }
    return ret;
}

/**
* The color temperature enum order is diffrent bettewn G9 and Fbc, so we have to make a mapping
**/
int CTv::Tv_FactoryWhiteBalanceColorTempMappingFbc2G9(int Tempmode)
{
    int ret = Tempmode;
    switch (Tempmode) {
    case 0:     //cold
        ret = 2;
        break;
    case 1:     //standard
        ret = 0;
        break;
    case 2:    //warm
        ret = 1;
        break;
    default:
        break;
    }
    return ret;
}

int CTv::Tv_SetTestPattern(int value)
{
    int ret = -1;
    if (fbcIns != NULL) {
        LOGD("%s, value is %d\n", __FUNCTION__, value);
        ret = fbcIns->cfbc_TestPattern_Select(COMM_DEV_SERIAL, value);
    }

    return ret;
}

int CTv::Tv_FactoryCloseWhiteBalanceGrayPattern()
{
    int useFbc = 0;
    int ret = -1;
    if (!mHdmiOutFbc) { // not use fbc store the white balance params
        ret = CVpp::getInstance()->VPP_SetGrayPattern(0);
    } else { //use fbc store the white balance params
        ret = Tv_FactoryClose_FBC_GrayPattern();
    }
    return ret;
}

int CTv::Tv_FactoryOpenWhiteBalanceGrayPattern()
{
    int ret = 0;
    if (mHdmiOutFbc) { //use fbc store the white balance params
        ret = Tv_FactoryOpen_FBC_GrayPattern();
    }
    return ret;
}

int CTv::Tv_FactorySetWhiteBalanceGrayPattern(int value)
{
    int ret = -1;
    if (!mHdmiOutFbc) {
        ret = CVpp::getInstance()->VPP_SetGrayPattern(value);
    } else {
        ret = Tv_FactorySet_FBC_GrayPattern(value);
    }
    return ret;
}

int CTv:: Tv_FactoryGetWhiteBalanceGrayPattern()
{
    int ret = -1;
    if (!mHdmiOutFbc) {
        ret = CVpp::getInstance()->VPP_GetGrayPattern();
    }
    return ret;
}

int CTv::Tv_FactorySet_FBC_GrayPattern(int value)
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
    if (fbcIns != NULL) {
        ret = fbcIns->cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_SERIAL, grayValue);
    }
    return ret;
}

int CTv::Tv_FactoryOpen_FBC_GrayPattern()
{
    int ret = -1;
    if (fbcIns != NULL) {
        ret = fbcIns->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 0);
    }
    return ret;
}

int CTv::Tv_FactoryClose_FBC_GrayPattern()
{
    int ret = -1;
    if (fbcIns != NULL) {
        ret = fbcIns->cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_SERIAL, 1);
    }
    return ret;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Mode( int mode )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Mode ( void )
{
    int temp_mode = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_ColorTemp_Mode(COMM_DEV_SERIAL, &temp_mode);
        return temp_mode;
    }

    return -1;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Mode_N360( int mode )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
        SSMSaveFBCN360ColorTempVal(mode);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Mode_N360 ( void )
{
    int temp_mode = 0;
    SSMReadFBCN360ColorTempVal(&temp_mode);
    return temp_mode;
}

int CTv::Tv_FactorySet_FBC_LockN_state(int value)
{
    LOGE ("Tv_FactorySet_FBC_LockN_state %d!!!\n", value);
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_LockN_state(COMM_DEV_SERIAL, value);
        return 0;
    }
    return -1;
}

int CTv::Tv_FactorySet_FBC_WB_Initial( int status )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_WB_Initial(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_WB_Initial ( void )
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_WB_Initial(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Batch(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
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
    LOGD ( "~Tv_FactorySet_FBC_ColorTemp_Batch##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_WB_Batch(COMM_DEV_SERIAL, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Batch ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
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

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_WB_Batch(COMM_DEV_SERIAL, mode, &r_gain, &g_gain, &b_gain, &r_offset, &g_offset, &b_offset);
        LOGD ( "~Tv_FactoryGet_FBC_ColorTemp_Batch##%d,%d,%d,%d,%d,%d,##", r_gain, g_gain, b_gain, r_offset, g_offset, b_offset );

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

int CTv::Tv_FactorySet_WB_G9_To_FBC( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params )
{
    Tv_FactorySet_FBC_ColorTemp_Batch(Tempmode, params);
    return 0;
}

int CTv::Tv_FactoryGet_WB_G9_To_FBC ( vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params )
{
    int temp_status = 0;
    Tv_FactoryGet_FBC_ColorTemp_Batch(Tempmode, params);
    return temp_status;
}

int CTv::Tv_FactoryGetItemFromBatch(vpp_color_temperature_mode_t colortemp_mode, int item)
{
    tcon_rgb_ogo_t params;
    int ret = 0;

    Tv_FactoryGet_FBC_ColorTemp_Batch((vpp_color_temperature_mode_t)colortemp_mode, &params);
    switch (item) {
    case 0:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params.r_gain);
        break;
    case 1:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params.g_gain);
        break;
    case 2:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcGainParams(params.b_gain);
        break;
    case 3:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params.r_post_offset);
        break;
    case 4:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params.g_post_offset);
        break;
    case 5:
        ret = Tv_FactoryWhiteBalanceFormatOutputFbcOffsetParams(params.b_post_offset);
        break;
    default:
        ret = 0;
    }
    return ret;
}

int CTv::Tv_FactorySet_FBC_CM_OnOff( unsigned char status )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_CM(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_CM_OnOff (void)
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_CM(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_DNLP_OnOff( unsigned char status )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_DNLP(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_DNLP_OnOff (void)
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_DNLP(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Gamma_OnOff( unsigned char status )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Gamma(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Gamma_OnOff (void)
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Gamma(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_WhiteBalance_OnOff( unsigned char status )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_WhiteBalance_OnOff(COMM_DEV_SERIAL, status);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_WhiteBalance_OnOff (void)
{
    int temp_status = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_WhiteBalance_OnOff(COMM_DEV_SERIAL, &temp_status);
        return temp_status;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_backlight_onoff ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_backlight_onoff(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_backlight_onoff ( void )
{
    int temp_value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_backlight_onoff(COMM_DEV_SERIAL, &temp_value);
        return temp_value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_LVDS_SSG_Set( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_LVDS_SSG_Set(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactorySet_FBC_LightSensor_Status_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_LightSensor_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310LightsensorVal(value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_LightSensor_Status_N310 ( void )
{
    int data = 0;
    if (fbcIns != NULL) {
        SSMReadFBCN310LightsensorVal(&data);
        return data;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Dream_Panel_Status_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Dream_Panel_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310Dream_PanelVal(value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Dream_Panel_Status_N310 ( void )
{
    int data = 0;
    if (fbcIns != NULL) {
        SSMReadFBCN310Dream_PanelVal(&data);
        return data;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_MULT_PQ_Status_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_MULT_PQ_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310MULT_PQVal(value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_MULT_PQ_Status_N310 ( void )
{
    int data = 0;

    if (fbcIns != NULL) {
        SSMReadFBCN310MULT_PQVal(&data);
        return data;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_MEMC_Status_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_MEMC_N310(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310MEMCVal(value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_MEMC_Status_N310 ( void )
{
    int data = 0;
    if (fbcIns != NULL) {
        SSMReadFBCN310MEMCVal(&data);
        return data;
    }

    return -1;
}

int CTv::Tv_FactorySet_FBC_ColorTemp_Mode_N310( int mode )
{
    //int colorTemp = 0;
    if (fbcIns != NULL) {
        //colorTemp = Tv_FactoryWhiteBalanceColorTempMappingG92Fbc(mode);
        fbcIns->cfbc_Set_ColorTemp_Mode(COMM_DEV_SERIAL, mode);
        SSMSaveFBCN310ColorTempVal(mode);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_ColorTemp_Mode_N310 ( void )
{
    int mode = 0;
    if (fbcIns != NULL) {
        SSMReadFBCN310ColorTempVal(&mode);
        //mode = Tv_FactoryWhiteBalanceColorTempMappingFbc2G9(temp_mode);

        return mode;
    }
    return -1;
}

int CTv::Tv_FactorySet_FBC_Backlight_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Backlight(COMM_DEV_SERIAL, value);
        SSMSaveFBCN310BackLightVal(value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Backlight_N310 ( void )
{
    int val = 0;

    if (fbcIns != NULL) {
        SSMReadFBCN310BackLightVal(&val);
        return val;
    }

    return -1;
}

int CTv::Tv_FactorySet_FBC_Bluetooth_IIS_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Bluetooth_IIS_onoff(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_Bluetooth_IIS_N310 ( void )
{
    int temp_value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_Bluetooth_IIS_onoff(COMM_DEV_SERIAL, &temp_value);
        return temp_value;
    }

    return 0;
}

int CTv::Tv_FactorySet_FBC_Led_N310 ( int val_1, int val_2, int val_3 )
{
    int val = 0;

    if (fbcIns != NULL) {
        val = fbcIns->cfbc_Set_Led_onoff(COMM_DEV_SERIAL, val_1, val_2, val_3);
        return val;
    }

    return -1;
}

int CTv::Tv_FactorySet_VbyOne_Spread_Spectrum_N311 ( int value )
{
    if (fbcIns != NULL) {
        return -1;
    } else {
        LOGD("%s, Set spectrum for T868 V-by-one....%d....\n", __FUNCTION__ , value);
        SSMSaveN311_VbyOne_Spread_Spectrum_Val(value);

        switch (value) {
        case 0:
            //SetFileAttrValue ( SYS_DROILOGIC_DEBUG ,"w 0x135c5091 c 0x10ca");
            //SetFileAttrValue ( SYS_DROILOGIC_DEBUG ,"w 0x801da72c c 0x10cb");
            break;
        case 1:
            SetFileAttrValue ( SYS_DROILOGIC_DEBUG, "w 0x1ba05091 c 0x10ca");
            SetFileAttrValue ( SYS_DROILOGIC_DEBUG, "w 0x80bda72c c 0x10cb");
            break;
        default:
            LOGD("%s, Set spectrum for T868 V-by-one....%d....\n", __FUNCTION__ , value);
            break;
        }

        return 0;
    }
}

int CTv::Tv_FactoryGet_VbyOne_Spread_Spectrum_N311 ( void )
{
    int val = 0;
    if (fbcIns != NULL) {
        return -1;
    } else  {
        SSMReadN311_VbyOne_Spread_Spectrum_Val(&val);
        return val;
    }
}

int CTv::Tv_FactorySet_FBC_AP_STANDBY_N310 ( int value )
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_AP_STANDBY_N310(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
}

int CTv::Tv_FactoryGet_FBC_AP_STANDBY_N310( void )
{
    int temp_value = 0;

    if (fbcIns != NULL) {
        fbcIns->cfbc_Get_AP_STANDBY_N310(COMM_DEV_SERIAL, &temp_value);
        return temp_value;
    }

    return 0;
}

int CTv::Tv_FactorySet_Uboot_Stage(int value)
{
    if (fbcIns != NULL) {
        fbcIns->cfbc_Set_Fbc_Uboot_Stage(COMM_DEV_SERIAL, value);
        return 0;
    }

    return -1;
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
    int oldMuteStatus;

    oldMuteStatus = GetAudioMuteForTv();
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

char *CTv::GetMainVolLutTableExtraName()
{
    return mMainVolLutTableExtraName;
}

//audio
int CTv::SetAudioMuteForSystem(int muteOrUnmute)
{
    int ret = 0;
    LOGD("SetAudioMuteForSystem sysMuteStats=%d, tvMuteStatus=%d, toMute=%d", mAudioMuteStatusForSystem, mAudioMuteStatusForTv, muteOrUnmute);
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
    LOGD("SetAudioMuteForTv sysMuteStats=%d, tvMuteStatus=%d, toMute=%d", mAudioMuteStatusForSystem, mAudioMuteStatusForTv, muteOrUnmute);
    ret |= SetDacMute(mAudioMuteStatusForSystem | mAudioMuteStatusForTv, CC_DAC_MUTE_TYPE_EXTERNAL | CC_DAC_MUTE_TYPE_INTERNAL);
    ret |= SetAudioI2sMute(mAudioMuteStatusForTv);
    return ret;
}

int CTv::GetAudioMuteForTv()
{
    return mAudioMuteStatusForTv;
}

int CTv::GetDbxTvMode(int *mode, int *son_value, int *vol_value, int *sur_value)
{
    *mode = 0;
    return SSMReadAudioDbxTvValue(son_value, vol_value, sur_value);
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

int CTv::AudioHandleHeadsetPlugIn()
{
    return 0;
}

int CTv::AudioHandleHeadsetPullOut()
{
    return 0;
}

int CTv::Tv_SetDRC_OnOff(int on_off)
{
    on_off = on_off;
    return 0;
}

int CTv::Tv_GetDRC_OnOff(void)
{
    return 0;
}

void CTv::updateSubtitle(int pic_width, int pic_height)
{
    TvEvent::SubtitleEvent ev;
    ev.pic_width = pic_width;
    ev.pic_height = pic_height;
    sendTvEvent(ev);
}

int CTv::setSubtitleBuffer(char *share_mem)
{
    mSubtitle.setBuffer(share_mem);
    return 0;
}

int CTv::initSubtitle(int bitmapWidth, int bitmapHeight)
{
    return mSubtitle.sub_init(bitmapWidth, bitmapHeight);
}

int CTv::lockSubtitle()
{
    return mSubtitle.sub_lock();
}

int CTv::unlockSubtitle()
{
    return mSubtitle.sub_unlock();
}

int CTv::getSubSwitchStatus()
{
    return mSubtitle.sub_switch_status();
}

int CTv::startSubtitle(int dmx_id, int pid, int page_id, int anc_page_id)
{
    return mSubtitle.sub_start_dvb_sub(dmx_id, pid, page_id, anc_page_id);
}

int CTv::stopSubtitle()
{
    return mSubtitle.sub_stop_dvb_sub();
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
    LOGD("%s : tmp_val = %d\n", __FUNCTION__, tmp_val);
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
    mCustomAudioMasterVolume = tmp_vol;

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
        SetAudioMasterVolume(GetAudioMasterVolume());
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

int CTv::SetAudioBassVolume(int tmp_vol)
{
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

int CTv::SetDbxTvMode(int mode, int son_value, int vol_value, int sur_value)
{
    return mAudioEffect.DbxTv_SetMode(mode, son_value, vol_value, sur_value);
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
            dst_buf[i] = mWallEffectValueBuf[i] + src_buf[i];

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
            gain_buf[i] = mWallEffectValueBuf[i] + gain_buf[i];

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
    return SetFileAttrValue ( SYS_ATV_DEMOD_DEBUG, set_str );
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

int CTv::SetAmAudioInputSr(unsigned int sr, int output_device)
{
    LOGD("SetAmAudioInputSr");
    return amAudioSetInputSr(sr, CC_IN_USE_SPDIF_DEVICE, output_device);
}

int CTv::SetAmAudioOutputMode(int mode)
{
    if (mode != CC_AMAUDIO_OUT_MODE_DIRECT && mode != CC_AMAUDIO_OUT_MODE_INTER_MIX
            && mode != CC_AMAUDIO_OUT_MODE_DIRECT_MIX) {
        LOGE("%s, mode error, it should be mix or direct!\n", __FUNCTION__);
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

int CTv::SetKalaokIO(int level)
{
    LOGD ("%s, SetKalaokIO = %d", __FUNCTION__, level);
    if (GetKaraokAvEnable() == 1) {
        mAudioAlsa.SetAudioSwitchIO(level);
    }
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

int CTv::closeTvAudio()
{
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
    FILE *fp = fopen(SYS_SPDIF_MODE_DEV_PATH, "w");
    if (fp == NULL) {
        LOGE("%s, Open file %s error(%s)!\n", __FUNCTION__, SYS_SPDIF_MODE_DEV_PATH, strerror(errno));
        return -1;
    }

    if (mode_val == CC_SPDIF_MODE_PCM) {
        fprintf(fp, "%d", 0);
    } else if (mode_val == CC_SPDIF_MODE_RAW ) {
        fprintf(fp, "%d", 1);
    }

    fclose(fp);
    fp = NULL;
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


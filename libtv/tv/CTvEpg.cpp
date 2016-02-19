/*
 * amlogic 2013
 *@ Project : tv
 *@ Date : 2013-12
 *@ Author :
*/
#define LOG_TAG "CTvEpg"

#include "CTvEpg.h"
#include "CTvChannel.h"

void CTvEpg::epg_evt_callback(long dev_no, int event_type, void *param, void *user_data __unused)
{
    CTvEpg *pEpg;

    AM_EPG_GetUserData((AM_EPG_Handle_t)dev_no, (void **)&pEpg);

    if (pEpg == NULL) return;

    if (pEpg->mpObserver == NULL) {
        return;
    }
    switch (event_type) {
    case AM_EPG_EVT_NEW_TDT:
    case AM_EPG_EVT_NEW_STT: {
        int utc_time;
        AM_EPG_GetUTCTime(&utc_time);
        pEpg->mCurEpgEv.type = EpgEvent::EVENT_TDT_END;
        pEpg->mCurEpgEv.time = (long)utc_time;
        pEpg->mpObserver->onEvent(pEpg->mCurEpgEv);
    }
    break;
    case AM_EPG_EVT_UPDATE_EVENTS:
        pEpg->mCurEpgEv.type = EpgEvent::EVENT_PROGRAM_EVENTS_UPDATE;
        pEpg->mCurEpgEv.programID = (int)param;
        pEpg->mpObserver->onEvent(pEpg->mCurEpgEv);
        break;
    case AM_EPG_EVT_UPDATE_PROGRAM_AV:
        pEpg->mCurEpgEv.type = EpgEvent::EVENT_PROGRAM_AV_UPDATE;
        pEpg->mCurEpgEv.programID = (int)param;
        pEpg->mpObserver->onEvent(pEpg->mCurEpgEv);
        break;
    case AM_EPG_EVT_UPDATE_PROGRAM_NAME:
        pEpg->mCurEpgEv.type = EpgEvent::EVENT_PROGRAM_NAME_UPDATE;
        pEpg->mCurEpgEv.programID = (int)param;
        pEpg->mpObserver->onEvent(pEpg->mCurEpgEv);
        break;
    case AM_EPG_EVT_UPDATE_TS:
        pEpg->mCurEpgEv.type = EpgEvent::EVENT_CHANNEL_UPDATE;
        pEpg->mCurEpgEv.channelID = (int)param;
        pEpg->mpObserver->onEvent(pEpg->mCurEpgEv);
        break;
    default:
        break;
    }
}

void CTvEpg::Init(int fend, int dmx, int fend_mod, char *textLanguages, char *dvb_text_coding)
{
    mFend_dev_id = fend;
    mDmx_dev_id  = dmx;
    mFend_mod   = fend_mod;
    epg_create(fend, dmx, fend_mod, textLanguages);
    epg_set_dvb_text_coding(dvb_text_coding);
}

void CTvEpg::epg_create(int fend_id, int dmx_id, int src, char *textLangs)
{
    AM_EPG_CreatePara_t para;
    AM_ErrorCode_t ret;
    AM_FEND_OpenPara_t fend_para;
    AM_DMX_OpenPara_t dmx_para;

    LOGD("Opening demux%d ...", dmx_id);
    memset(&dmx_para, 0, sizeof(dmx_para));
    AM_DMX_Open(dmx_id, &dmx_para);

    para.fend_dev = fend_id;
    para.dmx_dev  = dmx_id;
    para.source   = src;
    para.hdb      = NULL;


    snprintf(para.text_langs, sizeof(para.text_langs), "%s", textLangs);


    ret = AM_EPG_Create(&para, &mEpgScanHandle);
    if (ret != AM_SUCCESS) {
        LOGD("AM_EPG_Create failed");
        return;
    }

    /*注册EIT通知事件*/
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_NEW_TDT, epg_evt_callback, NULL);
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_NEW_STT, epg_evt_callback, NULL);
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_EVENTS, epg_evt_callback, NULL);
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_PROGRAM_AV, epg_evt_callback, NULL);
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_PROGRAM_NAME, epg_evt_callback, NULL);
    AM_EVT_Subscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_TS, epg_evt_callback, NULL);
    AM_EPG_SetUserData(mEpgScanHandle, (void *)this);
}



void CTvEpg::epg_destroy()
{
    /*反注册EIT通知事件*/
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_NEW_TDT, epg_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_NEW_STT, epg_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_EVENTS, epg_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_PROGRAM_AV, epg_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_PROGRAM_NAME, epg_evt_callback, NULL);
    AM_EVT_Unsubscribe((long)mEpgScanHandle, AM_EPG_EVT_UPDATE_TS, epg_evt_callback, NULL);
    AM_EPG_Destroy(mEpgScanHandle);
    AM_DMX_Close(mDmx_dev_id);
}


void CTvEpg::epg_change_mode(int op, int mode)
{
    AM_ErrorCode_t ret;
    ret = AM_EPG_ChangeMode(mEpgScanHandle, op, mode);
    if (ret != AM_SUCCESS)
        LOGD("AM_EPG_ChangeMode failed");
}



void CTvEpg::epg_monitor_service(int srv_id)
{
    int ret = AM_EPG_MonitorService(mEpgScanHandle, srv_id);
    if (ret != AM_SUCCESS)
        LOGD("AM_EPG_MonitorService failed");
}



void CTvEpg::epg_set_dvb_text_coding(char *coding)
{
    if (!strcmp(coding, "standard")) {
        AM_SI_SetDefaultDVBTextCoding("");
    } else {
        AM_SI_SetDefaultDVBTextCoding(coding);
    }
}



/*Start scan the sections.*/
void CTvEpg::startScan(int mode)
{
    epg_change_mode(MODE_ADD, mode);
}

/*Stop scan the sections.*/
void CTvEpg::stopScan(int mode)
{
    epg_change_mode(MODE_REMOVE, mode);
}


/*Enter a channel.*/
void CTvEpg::enterChannel(int chan_id)
{

    if (chan_id == mCurScanChannelId)
        return;
    //already enter,leave it
    if (mCurScanChannelId != INVALID_ID) {
        leaveChannel();
    }

    if (mFend_mod == CTvChannel::MODE_ATSC) {
        startScan(SCAN_PSIP_ETT | SCAN_PSIP_EIT | SCAN_MGT | SCAN_VCT | SCAN_RRT | SCAN_STT);
    } else {
        startScan(SCAN_EIT_ALL | SCAN_SDT | SCAN_NIT | SCAN_TDT | SCAN_CAT);
    }

    mCurScanChannelId = chan_id;
}

/*Leave the channel.*/
void CTvEpg::leaveChannel()
{

    stopScan(SCAN_ALL);
    mCurScanChannelId = INVALID_ID;
}

/*Enter the program.*/
void CTvEpg::enterProgram(int prog_id)
{
    if (prog_id == mCurScanProgramId)
        return;

    if (mCurScanProgramId != INVALID_ID) {
        leaveProgram();
    }

    mCurScanProgramId = prog_id;
    epg_monitor_service(mCurScanProgramId);//---------db_id
    startScan(SCAN_PAT | SCAN_PMT);
}

/*Leave the program.*/
void CTvEpg::leaveProgram()
{
    if (mCurScanProgramId == INVALID_ID)
        return;

    stopScan(SCAN_PAT | SCAN_PMT);
    epg_monitor_service(-1);
    mCurScanProgramId = INVALID_ID;
}

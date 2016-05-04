#define LOG_TAG "tvserver"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <utils/Errors.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <ITvService.h>
#include <hardware/hardware.h>
#include "TvService.h"
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <tvconfig/tvconfig.h>
#include <tvsetting/CTvSetting.h>
#include <tv/CTvFactory.h>
#include <audio/CTvAudio.h>
#include <tvutils/tvutils.h>
#include <version/version.h>
#include "tvcmd.h"
#include "tv/CTvLog.h"
#include <tvdb/CTvRegion.h>
extern "C" {
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include "make_ext4fs.h"
#include "am_ver.h"
}

static int getCallingPid()
{
    return IPCThreadState::self()->getCallingPid();
}

void TvService::instantiate()
{
    android::status_t ret = defaultServiceManager()->addService(String16("tvservice"), new TvService());
    if (ret != android::OK) {
        LOGE("Couldn't register tv service!");
    }
    LOGD("instantiate add tv service result:%d", ret);
}

TvService::TvService() :BnTvService()
{
    mpScannerClient = NULL;
    mUsers = 0;
    mpTv = new CTv();
    mpTv->setTvObserver(this);
    mCapVidFrame.setObserver(this);
    mpTv->OpenTv();
}

TvService::~TvService()
{
    mpScannerClient = NULL;
    for (int i = 0; i < (int)mClients.size(); i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            LOGW("some client still connect it!");
        }
    }

    if (mpTv != NULL) {
        delete mpTv;
        mpTv = NULL;
    }
}

void TvService::onTvEvent(const CTvEv &ev)
{
    int type = ev.getEvType();
    LOGD("TvService::onTvEvent ev type = %d", type);
    switch (type) {
    case CTvEv::TV_EVENT_COMMOM:
        break;

    case CTvEv::TV_EVENT_SCANNER: {
        CTvScanner::ScannerEvent *pScannerEv = (CTvScanner::ScannerEvent *) (&ev);
        if (mpScannerClient != NULL) {
            sp<Client> ScannerClient = mpScannerClient.promote();
            if (ScannerClient != 0) {
                Parcel p;
                LOGD("scanner evt type:%d freq:%d vid:%d acnt:%d",
                     pScannerEv->mType, pScannerEv->mFrequency, pScannerEv->mVid, pScannerEv->mAcnt);
                p.writeInt32(pScannerEv->mType);
                p.writeInt32(pScannerEv->mPercent);
                p.writeInt32(pScannerEv->mTotalChannelCount);
                p.writeInt32(pScannerEv->mLockedStatus);
                p.writeInt32(pScannerEv->mChannelNumber);
                p.writeInt32(pScannerEv->mFrequency);
                p.writeString16(String16(pScannerEv->mProgramName));
                p.writeInt32(pScannerEv->mprogramType);
                p.writeString16(String16(pScannerEv->mMSG));
                p.writeInt32(pScannerEv->mStrength);
                p.writeInt32(pScannerEv->mSnr);
                //ATV
                p.writeInt32(pScannerEv->mVideoStd);
                p.writeInt32(pScannerEv->mAudioStd);
                p.writeInt32(pScannerEv->mIsAutoStd);
                //DTV
                p.writeInt32(pScannerEv->mMode);
                p.writeInt32(pScannerEv->mSymbolRate);
                p.writeInt32(pScannerEv->mModulation);
                p.writeInt32(pScannerEv->mBandwidth);
                p.writeInt32(pScannerEv->mOfdm_mode);
                p.writeInt32(pScannerEv->mTsId);
                p.writeInt32(pScannerEv->mONetId);
                p.writeInt32(pScannerEv->mServiceId);
                p.writeInt32(pScannerEv->mVid);
                p.writeInt32(pScannerEv->mVfmt);
                p.writeInt32(pScannerEv->mAcnt);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAid[i]);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAfmt[i]);
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeString16(String16(pScannerEv->mAlang[i]));
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAtype[i]);
                p.writeInt32(pScannerEv->mPcr);
                p.writeInt32(pScannerEv->mScnt);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mStype[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSstype[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid1[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeInt32(pScannerEv->mSid2[i]);
                for (int i = 0; i < pScannerEv->mScnt; i++)
                    p.writeString16(String16(pScannerEv->mSlang[i]));
                p.writeInt32(pScannerEv->mFree_ca);
                p.writeInt32(pScannerEv->mScrambled);
                p.writeInt32(pScannerEv->mScanMode);
                p.writeInt32(pScannerEv->mSdtVer);
                p.writeInt32(pScannerEv->mSortMode);

                p.writeInt32(pScannerEv->mLcnInfo.net_id);
                p.writeInt32(pScannerEv->mLcnInfo.ts_id);
                p.writeInt32(pScannerEv->mLcnInfo.service_id);
                for (int i=0; i<MAX_LCN; i++) {
                    p.writeInt32(pScannerEv->mLcnInfo.visible[i]);
                    p.writeInt32(pScannerEv->mLcnInfo.lcn[i]);
                    p.writeInt32(pScannerEv->mLcnInfo.valid[i]);
                }
                ScannerClient->notifyCallback(SCAN_EVENT_CALLBACK, p);
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_EPG: {
        CTvEpg::EpgEvent *pEpgEvent = (CTvEpg::EpgEvent *) (&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEpgEvent->type);
                    p.writeInt32(pEpgEvent->time);
                    p.writeInt32(pEpgEvent->programID);
                    p.writeInt32(pEpgEvent->channelID);
                    c->getTvClient()->notifyCallback(EPG_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_HDMI_IN_CAP: {
        CTvScreenCapture::CapEvent *pCapEvt = (CTvScreenCapture::CapEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pCapEvt->mFrameNum);
                    p.writeInt32(pCapEvt->mFrameSize);
                    p.writeInt32(pCapEvt->mFrameWide);
                    p.writeInt32(pCapEvt->mFrameHeight);
                    c->getTvClient()->notifyCallback(VFRAME_BMP_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_AV_PLAYBACK: {
        TvEvent::AVPlaybackEvent *pEv = (TvEvent::AVPlaybackEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c!= 0) {
                    Parcel p;
                    p.writeInt32(pEv->mMsgType);
                    p.writeInt32(pEv->mProgramId);
                    c->getTvClient()->notifyCallback(DTV_AV_PLAYBACK_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SIGLE_DETECT: {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mTrans_fmt);
                    p.writeInt32(pEv->mFmt);
                    p.writeInt32(pEv->mStatus);
                    p.writeInt32(pEv->mReserved);
                    c->getTvClient()->notifyCallback(SIGLE_DETECT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SUBTITLE: {
        TvEvent::SubtitleEvent *pEv = (TvEvent::SubtitleEvent *)(&ev);
        sp<Client> c = mpSubClient.promote();
        if (c != NULL) {
            Parcel p;
            p.writeInt32(pEv->pic_width);
            p.writeInt32(pEv->pic_height);
            c->notifyCallback(SUBTITLE_UPDATE_CALLBACK, p);
        }
        break;
    }

    case CTvEv::TV_EVENT_ADC_CALIBRATION: {
        TvEvent::ADCCalibrationEvent *pEv = (TvEvent::ADCCalibrationEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    c->getTvClient()->notifyCallback(ADC_CALIBRATION_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_VGA: {//VGA
        TvEvent::VGAEvent *pEv = (TvEvent::VGAEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    c->getTvClient()->notifyCallback(VGA_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SOURCE_CONNECT: {
        TvEvent::SourceConnectEvent *pEv = (TvEvent::SourceConnectEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mSourceInput);
                    p.writeInt32(pEv->connectionState);
                    c->getTvClient()->notifyCallback(SOURCE_CONNECT_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_HDMIRX_CEC: {
        TvEvent::HDMIRxCECEvent *pEv = (TvEvent::HDMIRxCECEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }
                    c->getTvClient()->notifyCallback(HDMIRX_CEC_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_UPGRADE_FBC: {
        TvEvent::UpgradeFBCEvent *pEv = (TvEvent::UpgradeFBCEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    p.writeInt32(pEv->param);
                    c->getTvClient()->notifyCallback(UPGRADE_FBC_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SERIAL_COMMUNICATION: {
        TvEvent::SerialCommunicationEvent *pEv = (TvEvent::SerialCommunicationEvent *)(&ev);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mDevId);
                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }
                    c->getTvClient()->notifyCallback(SERIAL_COMMUNICATION_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_2d4G_HEADSET: {
        TvEvent::HeadSetOf2d4GEvent *pEv = (TvEvent::HeadSetOf2d4GEvent *)(&ev);
        LOGD("SendDtvStats status: =%d para2: =%d", pEv->state, pEv->para);
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->state);
                    p.writeInt32(pEv->para);
                    c->getTvClient()->notifyCallback(HEADSET_STATUS_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_SCANNING_FRAME_STABLE: {
        TvEvent::ScanningFrameStableEvent *pEv = (TvEvent::ScanningFrameStableEvent *)(&ev);
        LOGD("Scanning Frame is stable!");
        for (int i = 0; i < (int)mClients.size(); i++) {
            wp<Client> client = mClients[i];
            if (client != 0) {
                sp<Client> c = client.promote();
                if (c != 0) {
                    Parcel p;
                    p.writeInt32(pEv->CurScanningFreq);
                    c->getTvClient()->notifyCallback(SCANNING_FRAME_STABLE_CALLBACK, p);
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

sp<ITv> TvService::connect(const sp<ITvClient> &tvClient)
{
    int callingPid = getCallingPid();
    LOGD("TvService::connect (pid %d, client %p)", callingPid, IInterface::asBinder(tvClient).get());

    Mutex::Autolock lock(mLock);

    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("TvService::connect (pid %d, same client %p) is reconnecting...", callingPid, IInterface::asBinder(tvClient).get());
                    return currentClient;
                }
            } else {
                LOGE("TvService::connect client (pid %d) not exist", callingPid);
                client.clear();
                mClients.removeAt(i);
                clientSize--;
            }
        }
    }

    sp<Client> newclient = new Client(this, tvClient, callingPid, mpTv);
    mClients.add(newclient);
    return newclient;
}

void TvService::removeClient(const sp<ITvClient> &tvClient)
{
    int callingPid = getCallingPid();

    Mutex::Autolock lock(mLock);

    int clientSize = mClients.size();
    for (int i = 0; i < clientSize; i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("find client , and remove it pid = %d, client = %p i=%d", callingPid, IInterface::asBinder(tvClient).get(), i);
                    client.clear();
                    mClients.removeAt(i);
                    break;
                }
            } else {
                LOGW("removeclient currentClient is NULL (pid %d)", callingPid);
                client.clear();
                mClients.removeAt(i);
                clientSize--;
            }
        }
    }

    LOGD("removeClient (pid %d) done", callingPid);
}

void TvService::incUsers()
{
    android_atomic_inc(&mUsers);
}

void TvService::decUsers()
{
    android_atomic_dec(&mUsers);
}

TvService::Client::Client(const sp<TvService> &tvService, const sp<ITvClient> &tvClient, pid_t clientPid, CTv *pTv)
{
    mTvService = tvService;
    mTvClient = tvClient;
    mClientPid = clientPid;
    tvService->incUsers();
    mpTv = pTv;
    mIsStartTv = false;
}

TvService::Client::~Client()
{
    if (mIsStartTv) {
        mpTv->StopTvLock();
        mIsStartTv = false;
    }

    int callingPid = getCallingPid();
    // tear down client
    LOGD("Client::~Client(pid %d, client %p)", callingPid, IInterface::asBinder(getTvClient()).get());
    // make sure we tear down the hardware
    mClientPid = callingPid;
    disconnect();
}

status_t TvService::Client::checkPid()
{
    int callingPid = getCallingPid();
    if (mClientPid == callingPid)
        return NO_ERROR;
    LOGD("Attempt to use locked tv (client %p) from different process "
         " (old pid %d, new pid %d)", IInterface::asBinder(getTvClient()).get(), mClientPid, callingPid);
    return -EBUSY;
}

status_t TvService::Client::lock()
{
    int callingPid = getCallingPid();
    LOGD("lock from pid %d (mClientPid %d)", callingPid, mClientPid);
    Mutex::Autolock _l(mLock);
    // lock tv to this client if the the tv is unlocked
    if (mClientPid == 0) {
        mClientPid = callingPid;
        return NO_ERROR;
    }
    // returns NO_ERROR if the client already owns the tv, -EBUSY otherwise
    return checkPid();
}

status_t TvService::Client::unlock()
{
    int callingPid = getCallingPid();
    LOGD("unlock from pid %d (mClientPid %d)", callingPid, mClientPid);
    Mutex::Autolock _l(mLock);
    // allow anyone to use tv
    status_t result = checkPid();
    if (result == NO_ERROR) {
        mClientPid = 0;
        // we need to remove the reference so that when app goes
        // away, the reference count goes to 0.
        mTvClient.clear();
    }
    return result;
}

status_t TvService::Client::connect(const sp<ITvClient> &client)
{
    int callingPid = getCallingPid();
    LOGD("Client::connect E (pid %d, client %p)", callingPid, IInterface::asBinder(client).get());

    {
        Mutex::Autolock _l(mLock);
        if (mClientPid != 0 && checkPid() != NO_ERROR) {
            LOGW("Tried to connect to locked tv (old pid %d, new pid %d)", mClientPid, callingPid);
            return -EBUSY;
        }

        // did the client actually change?
        if ((mTvClient != NULL) && (IInterface::asBinder(client) == IInterface::asBinder(mTvClient))) {
            LOGD("Connect to the same client");
            return NO_ERROR;
        }

        mTvClient = client;
        LOGD("Connect to the new client (pid %d, client %p)", callingPid, IInterface::asBinder(client).get());
    }

    mClientPid = callingPid;
    return NO_ERROR;
}

void TvService::Client::disconnect()
{
    int callingPid = getCallingPid();

    LOGD("Client::disconnect() E (pid %d client %p)", callingPid, IInterface::asBinder(getTvClient()).get());

    Mutex::Autolock lock(mLock);
    if (mClientPid <= 0) {
        LOGE("tv is unlocked (mClientPid = %d), don't tear down hardware", mClientPid);
        return;
    }
    if (checkPid() != NO_ERROR) {
        LOGE("Different client - don't disconnect");
        return;
    }

    mTvService->removeClient(mTvClient);
    mTvService->decUsers();

    LOGD("Client::disconnect() X (pid %d)", callingPid);
}

status_t TvService::Client::createVideoFrame(const sp<IMemory> &shareMem __unused,
    int iSourceMode __unused, int iCapVideoLayerOnly __unused)
{
#if 0
    LOGD(" mem=%d size=%d", shareMem->pointer() == NULL, shareMem->size());
    LOGD("iSourceMode :%d iCapVideoLayerOnly = %d \n", iSourceMode, iCapVideoLayerOnly);
    int Len = 0;
    Mutex::Autolock lock(mLock);
    mTvService->mCapVidFrame.InitVCap(shareMem);

    if ((1 == iSourceMode) && (1 == iCapVideoLayerOnly)) {
        mTvService->mCapVidFrame.CapMediaPlayerVideoLayerOnly(1920, 1080);
#if 0
        mTvService->mCapVidFrame.SetVideoParameter(1920, 1080, 50);
        mTvService->mCapVidFrame.VideoStart();
        mTvService->mCapVidFrame.GetVideoData(&Len);
        mTvService->mCapVidFrame.VideoStop();
#endif
    } else if (2 == iSourceMode && 0 == iCapVideoLayerOnly) {
        mTvService->mCapVidFrame.CapOsdAndVideoLayer(1920, 1080);
    } else if (2 == iSourceMode && 1 == iCapVideoLayerOnly) {
        mTvService->mCapVidFrame.CapMediaPlayerVideoLayerOnly(1920, 1080);
    } else {
        LOGD("=============== NOT SUPPORT=======================\n");
    }
    mTvService->mCapVidFrame.DeinitVideoCap();
#endif
    LOGE("do not support this function");
    return 0;
}

status_t TvService::Client::createSubtitle(const sp<IMemory> &shareMem)
{
    LOGD("createSubtitle pid = %d, mem=%d size=%d", getCallingPid(), shareMem->pointer() == NULL, shareMem->size());
    mpTv->mSubtitle.setBuffer((char *)shareMem->pointer());
    mTvService->mpSubClient = this;
    //pSub = new CTvSubtitle(share_mem, this);
    //pSub->run();
    return 0;
}

status_t TvService::Client::processCmd(const Parcel &p, Parcel *r)
{
    unsigned char dataBuf[512] = {0};
    int *ptrData = NULL;

    int cmd = p.readInt32();

    LOGD("enter client=%d cmd=%d", getCallingPid(), cmd);
    switch (cmd) {
    // Tv function
    case OPEN_TV: {
        break;
    }
    case CLOSE_TV: {
        int ret = mpTv->CloseTv();
        r->writeInt32(ret);
        break;
    }
    case START_TV: {
        int mode = p.readInt32();
        int ret = mpTv->StartTvLock();
        mIsStartTv = true;
        r->writeInt32(ret);
        break;
    }
    case STOP_TV: {
        int ret = mpTv->StopTvLock();
        r->writeInt32(ret);
        mIsStartTv = false;
        break;
    }
    case GET_TV_STATUS: {
        int ret = (int)mpTv->GetTvStatus();
        r->writeInt32(ret);
        break;
    }
    case GET_LAST_SOURCE_INPUT: {
        int ret = (int)mpTv->GetLastSourceInput();
        r->writeInt32(ret);
        break;
    }
    case GET_CURRENT_SOURCE_INPUT: {
        int ret = (int)mpTv->GetCurrentSourceInputLock();
        r->writeInt32(ret);
        break;
    }
    case GET_CURRENT_SIGNAL_INFO: {
        tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
        int frame_rate = mpTv->getHDMIFrameRate();
        r->writeInt32(siginfo.trans_fmt);
        r->writeInt32(siginfo.fmt);
        r->writeInt32(siginfo.status);
        r->writeInt32(frame_rate);
        break;
    }
    case SET_SOURCE_INPUT: {
        int sourceinput = p.readInt32();
        LOGD(" SetSourceInput sourceId= %x", sourceinput);
        int ret = mpTv->SetSourceSwitchInput((tv_source_input_t)sourceinput);
        r->writeInt32(ret);
        break;
    }
    case DO_SUSPEND: {
        int type = p.readInt32();
        int ret = mpTv->DoSuspend(type);
        r->writeInt32(ret);
        break;
    }
    case DO_RESUME: {
        int type = p.readInt32();
        int ret = mpTv->DoResume(type);
        r->writeInt32(ret);
        break;
    }
    case IS_DVI_SIGNAL: {
        int ret = mpTv->IsDVISignal();
        r->writeInt32(ret);
        break;
    }
    case IS_VGA_TIMEING_IN_HDMI: {
        int ret = mpTv->isVgaFmtInHdmi();
        r->writeInt32(ret);
        break;
    }

    case SET_PREVIEW_WINDOW: {
        tvin_window_pos_t win_pos;
        win_pos.x1 = p.readInt32();
        win_pos.y1 = p.readInt32();
        win_pos.x2 = p.readInt32();
        win_pos.y2 = p.readInt32();
        int ret = (int)mpTv->SetPreviewWindow(win_pos);
        r->writeInt32(ret);
        break;
    }

    case GET_SOURCE_CONNECT_STATUS: {
        int source_input = p.readInt32();
        int ret = mpTv->GetSourceConnectStatus((tv_source_input_t)source_input);
        r->writeInt32(ret);
        break;
    }

    case GET_SOURCE_INPUT_LIST: {
        const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
        r->writeString16(String16(value));
        break;
    }
    //Tv function END

    // HDMI
    case SET_HDMI_EDID_VER: {
        int hdmi_port_id = p.readInt32();
        int edid_ver = p.readInt32();
        int tmpRet = mpTv->SetHdmiEdidVersion((tv_hdmi_port_id_t)hdmi_port_id, (tv_hdmi_edid_version_t)edid_ver);
        r->writeInt32(tmpRet);
        break;
    }
    case SET_HDCP_KEY_ENABLE: {
        int enable = p.readInt32();
        int tmpRet = mpTv->SetHdmiHDCPSwitcher((tv_hdmi_hdcpkey_enable_t)enable);
        r->writeInt32(tmpRet);
        break;
    }
    case SET_HDMI_COLOR_RANGE_MODE: {
        int range_mode = p.readInt32();
        int tmpRet = mpTv->SetHdmiColorRangeMode((tv_hdmi_color_range_t)range_mode);
        r->writeInt32(tmpRet);
        break;
    }
    case GET_HDMI_COLOR_RANGE_MODE: {
        int tmpRet = mpTv->GetHdmiColorRangeMode();
        r->writeInt32(tmpRet);
        break;
    }
    // HDMI END

    // PQ
    case SET_BRIGHTNESS: {
        int brightness = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetBrightness(brightness, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_BRIGHTNESS: {
        int source_type = p.readInt32();
        LOGD("GET_BRIGHTNESS source type:%d", source_type);
        int ret = mpTv->Tv_GetBrightness((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_BRIGHTNESS: {
        int brightness = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveBrightness(brightness, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_CONTRAST: {
        int contrast = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetContrast(contrast, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_CONTRAST: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetContrast((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_CONTRAST: {
        int contrast = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveContrast(contrast, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_SATUATION: {
        int satuation = p.readInt32();
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetSaturation(satuation, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_SATUATION: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetSaturation((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_SATUATION: {
        int satuation = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveSaturation(satuation, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_HUE: {
        int hue = p.readInt32();
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetHue(hue, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_HUE: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetHue((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_HUE: {
        int hue = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveHue(hue, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_PQMODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetPQMode((vpp_picture_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_PQMODE: {
        int source_type = p.readInt32();
        int ret = (int)mpTv->Tv_GetPQMode((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_PQMODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SavePQMode((vpp_picture_mode_t)mode, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_SHARPNESS: {
        int value = p.readInt32();
        int source_type = p.readInt32();
        int en = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetSharpness(value, (tv_source_input_type_t)source_type, en, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_SHARPNESS: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetSharpness((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_SHARPNESS: {
        int value = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SaveSharpness(value, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_BACKLIGHT: {
        int value = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetBacklight(value, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_BACKLIGHT: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetBacklight((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_BACKLIGHT: {
        int value = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveBacklight ( value, (tv_source_input_type_t)source_type );
        r->writeInt32(ret);
        break;
    }
    case SET_BACKLIGHT_SWITCH: {
        int value = p.readInt32();
        int ret = mpTv->Tv_SetBacklight_Switch(value);
        r->writeInt32(ret);
        break;
    }
    case GET_BACKLIGHT_SWITCH: {
        int ret = mpTv->Tv_GetBacklight_Switch();
        r->writeInt32(ret);
        break;
    }
    case SET_COLOR_TEMPERATURE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetColorTemperature((vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_COLOR_TEMPERATURE: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetColorTemperature((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_COLOR_TEMPERATURE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveColorTemperature ( (vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type );
        r->writeInt32(ret);
        break;
    }
    case SET_DISPLAY_MODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetDisplayMode((vpp_display_mode_t)mode, (tv_source_input_type_t)source_type, (tvin_sig_fmt_t)fmt, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_DISPLAY_MODE: {
        int source_type = p.readInt32();
        int ret = (int)mpTv->Tv_GetDisplayMode((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_DISPLAY_MODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveDisplayMode((vpp_display_mode_t)mode, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SET_EYE_PROTETION_MODE: {
        int enable = p.readInt32();
        int ret = mpTv->setEyeProtectionMode(enable);
        r->writeInt32(ret);
        break;
    }
    case GET_EYE_PROTETION_MODE: {
        bool ret = mpTv->getEyeProtectionMode();
        r->writeInt32(ret);
        break;
    }
    case SET_GAMMA: {
        int gamma_curve = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->setGamma((vpp_gamma_curve_t)gamma_curve, is_save);
        r->writeInt32(ret);
        break;
    }
    case SET_NOISE_REDUCTION_MODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_SetNoiseReductionMode((vpp_noise_reduction_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_NOISE_REDUCTION_MODE: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_GetNoiseReductionMode((tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    case SAVE_NOISE_REDUCTION_MODE: {
        int mode = p.readInt32();
        int source_type = p.readInt32();
        int ret = mpTv->Tv_SaveNoiseReductionMode((vpp_noise_reduction_mode_t)mode, (tv_source_input_type_t)source_type);
        r->writeInt32(ret);
        break;
    }
    // PQ END

    // FACTORY
    case FACTORY_SETPQMODE_BRIGHTNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int brightness = p.readInt32();
        int ret = mpTv->mFactoryMode.setPQModeBrightness(source_type, pq_mode, brightness);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_BRIGHTNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.getPQModeBrightness(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_CONTRAST: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int contrast = p.readInt32();
        int ret = mpTv->mFactoryMode.setPQModeContrast(source_type, pq_mode, contrast);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_CONTRAST: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.getPQModeContrast(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_SATURATION: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int saturation = p.readInt32();
        int ret = mpTv->mFactoryMode.setPQModeSaturation(source_type, pq_mode, saturation);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_SATURATION: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.getPQModeSaturation(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_HUE: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int hue = p.readInt32();
        int ret = mpTv->mFactoryMode.setPQModeHue(source_type, pq_mode, hue);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_HUE: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.getPQModeHue(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_SHARPNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int sharpness = p.readInt32();
        int ret = mpTv->mFactoryMode.setPQModeSharpness(source_type, pq_mode, sharpness);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_SHARPNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.getPQModeSharpness(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETTESTPATTERN: {
        int pattern = p.readInt32();
        int ret = mpTv->mFactoryMode.setTestPattern(pattern);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETTESTPATTERN: {
        int ret = mpTv->mFactoryMode.getTestPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPATTERN_YUV: {
        int blend = p.readInt32();
        int y = p.readInt32();
        int u = p.readInt32();
        int v = p.readInt32();
        int ret = mpTv->mFactoryMode.setScreenColor(blend, y, u, v);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETPQMODE: {
        int ret = mpTv->mFactoryMode.resetPQMode();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETCOLORTEMP: {
        int ret = mpTv->mFactoryMode.resetColorTemp();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETPAMAMSDEFAULT: {
        int ret = mpTv->mFactoryMode.setParamsDefault();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETDDRSSC: {
        int setp = p.readInt32();
        int ret = mpTv->mFactoryMode.setDDRSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETDDRSSC: {
        int ret = mpTv->mFactoryMode.getDDRSSC();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDSSSC: {
        int setp = p.readInt32();
        int ret = mpTv->mFactoryMode.setLVDSSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDSSSC: {
        int ret = mpTv->mFactoryMode.getLVDSSSC();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETNOLINEPARAMS: {
        noline_params_t params;
        int noline_params_type = p.readInt32();
        int source_type = p.readInt32();
        params.osd0 = p.readInt32();
        params.osd25 = p.readInt32();
        params.osd50 = p.readInt32();
        params.osd75 = p.readInt32();
        params.osd100 = p.readInt32();
        int ret = mpTv->mFactoryMode.setNolineParams(noline_params_type, source_type, params);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETNOLINEPARAMS: {
        int noline_params_type = p.readInt32();
        int source_type = p.readInt32();
        noline_params_t params = mpTv->mFactoryMode.getNolineParams(noline_params_type, source_type);
        r->writeInt32(params.osd0);
        r->writeInt32(params.osd25);
        r->writeInt32(params.osd50);
        r->writeInt32(params.osd75);
        r->writeInt32(params.osd100);
        break;
    }
    case FACTORY_SETOVERSCAN: {
        tvin_cutwin_t cutwin_t;
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int trans_fmt = p.readInt32();
        cutwin_t.hs = p.readInt32();
        cutwin_t.he = p.readInt32();
        cutwin_t.vs = p.readInt32();
        cutwin_t.ve = p.readInt32();
        int ret = mpTv->mFactoryMode.setOverscan(source_type, fmt, trans_fmt, cutwin_t);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETOVERSCAN: {
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int trans_fmt = p.readInt32();
        tvin_cutwin_t cutwin_t = mpTv->mFactoryMode.getOverscan(source_type, fmt, trans_fmt);
        r->writeInt32(cutwin_t.hs);
        r->writeInt32(cutwin_t.he);
        r->writeInt32(cutwin_t.vs);
        r->writeInt32(cutwin_t.ve);
        break;
    }
    case FACTORY_SET_OUT_DEFAULT: {
        int ret = mpTv->Tv_SSMFacRestoreDefaultSetting();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_CLEAN_ALL_TABLE_FOR_PROGRAM: {
        int ret = mpTv->ClearAnalogFrontEnd();
        mpTv->clearDbAllProgramInfoTable();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SET_GAMMA_PATTERN: {
        tcon_gamma_table_t gamma_r, gamma_g, gamma_b;
        memset(gamma_r.data, (unsigned short)(p.readInt32()<<2), 256);
        memset(gamma_g.data, (unsigned short)(p.readInt32()<<2), 256);
        memset(gamma_b.data, (unsigned short)(p.readInt32()<<2), 256);
        int ret = mpTv->mFactoryMode.setGamma(gamma_r, gamma_g, gamma_b);
        r->writeInt32(ret);
        break;
    }
    // FACTORY END

    // AUDIO & AUDIO MUTE
    case SET_AUDIO_MUTEKEY_STATUS: {
        int status = p.readInt32();
        int ret = mpTv->SetAudioMuteForSystem(status);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_MUTEKEY_STATUS: {
        int ret = mpTv->GetAudioMuteForSystem();
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_AVOUT_MUTE_STATUS: {
        int status = p.readInt32();
        int ret = mpTv->SetAudioAVOutMute(status);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_AVOUT_MUTE_STATUS: {
        int ret = mpTv->GetAudioAVOutMute();
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_SPDIF_MUTE_STATUS: {
        int status = p.readInt32();
        int ret = mpTv->SetAudioSPDIFMute(status);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SPDIF_MUTE_STATUS: {
        int ret = mpTv->GetAudioSPDIFMute();
        r->writeInt32(ret);
        break;
    }
    // AUDIO MASTER VOLUME
    case SET_AUDIO_MASTER_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioMasterVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_MASTER_VOLUME: {
        int ret = mpTv->GetAudioMasterVolume();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_MASTER_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioMasterVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_MASTER_VOLUME: {
        int ret = mpTv->GetCurAudioMasterVolume();
        r->writeInt32(ret);
        break;
    }
    //AUDIO BALANCE
    case SET_AUDIO_BALANCE: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioBalance(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_BALANCE: {
        int ret = mpTv->GetAudioBalance();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_BALANCE: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioBalance(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_BALANCE: {
        int ret = mpTv->GetCurAudioBalance();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SUPPERBASS VOLUME
    case SET_AUDIO_SUPPER_BASS_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSupperBassVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SUPPER_BASS_VOLUME: {
        int ret = mpTv->GetAudioSupperBassVolume();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SUPPER_BASS_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSupperBassVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SUPPER_BASS_VOLUME: {
        int ret = mpTv->GetCurAudioSupperBassVolume();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SUPPERBASS SWITCH
    case SET_AUDIO_SUPPER_BASS_SWITCH: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSupperBassSwitch(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SUPPER_BASS_SWITCH: {
        int ret = mpTv->GetAudioSupperBassSwitch();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SUPPER_BASS_SWITCH: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSupperBassSwitch(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SUPPER_BASS_SWITCH: {
        int ret = mpTv->GetCurAudioSupperBassSwitch();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SRS SURROUND SWITCH
    case SET_AUDIO_SRS_SURROUND: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSRSSurround(vol);
        mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SRS_SURROUND: {
        int ret = mpTv->GetAudioSRSSurround();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SRS_SURROUND: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSrsSurround(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SRS_SURROUND: {
        int ret = mpTv->GetCurAudioSRSSurround();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SRS DIALOG CLARITY
    case SET_AUDIO_SRS_DIALOG_CLARITY: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSrsDialogClarity(vol);
        mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SRS_DIALOG_CLARITY: {
        int ret = mpTv->GetAudioSrsDialogClarity();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SRS_DIALOG_CLARITY: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSrsDialogClarity(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SRS_DIALOG_CLARITY: {
        int ret = mpTv->GetCurAudioSrsDialogClarity();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SRS TRUBASS
    case SET_AUDIO_SRS_TRU_BASS: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSrsTruBass(vol);
        mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SRS_TRU_BASS: {
        int ret = mpTv->GetAudioSrsTruBass();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SRS_TRU_BASS: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSrsTruBass(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SRS_TRU_BASS: {
        int ret = mpTv->GetCurAudioSrsTruBass();
        r->writeInt32(ret);
        break;
    }
    //AUDIO BASS
    case SET_AUDIO_BASS_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioBassVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_BASS_VOLUME: {
        int ret = mpTv->GetAudioBassVolume();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_BASS_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioBassVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_BASS_VOLUME: {
        int ret = mpTv->GetCurAudioBassVolume();
        r->writeInt32(ret);
        break;
    }
    //AUDIO TREBLE
    case SET_AUDIO_TREBLE_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioTrebleVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_TREBLE_VOLUME: {
        int ret = mpTv->GetAudioTrebleVolume();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_TREBLE_VOLUME: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioTrebleVolume(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_TREBLE_VOLUME: {
        int ret = mpTv->GetCurAudioTrebleVolume();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SOUND MODE
    case SET_AUDIO_SOUND_MODE: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioSoundMode(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_SOUND_MODE: {
        int ret = mpTv->GetAudioSoundMode();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SOUND_MODE: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSoundMode(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SOUND_MODE: {
        int ret = mpTv->GetCurAudioSoundMode();
        r->writeInt32(ret);
        break;
    }
    //AUDIO WALL EFFECT
    case SET_AUDIO_WALL_EFFECT: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioWallEffect(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_WALL_EFFECT: {
        int ret = mpTv->GetAudioWallEffect();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_WALL_EFFECT: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioWallEffect(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_WALL_EFFECT: {
        int ret = mpTv->GetCurAudioWallEffect();
        r->writeInt32(ret);
        break;
    }
    //AUDIO EQ MODE
    case SET_AUDIO_EQ_MODE: {
        int vol = p.readInt32();
        int ret = mpTv->SetAudioEQMode(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_EQ_MODE: {
        int ret = mpTv->GetAudioEQMode();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_EQ_MODE: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioEQMode(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_EQ_MODE: {
        int ret = mpTv->GetCurAudioEQMode();
        r->writeInt32(ret);
        break;
    }
    //AUDIO EQ GAIN
    case GET_AUDIO_EQ_RANGE: {
        int buf[2];
        int ret = mpTv->GetAudioEQRange(buf);
        r->writeInt32(2);
        r->writeInt32(buf[0]);
        r->writeInt32(buf[1]);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_EQ_BAND_COUNT: {
        int ret = mpTv->GetAudioEQBandCount();
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_EQ_GAIN: {
        int buf[128] = {0};
        int bufSize = p.readInt32();
        for (int i = 0; i < bufSize; i++) {
            buf[i] = p.readInt32();
        }
        int ret = mpTv->SetAudioEQGain(buf);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_EQ_GAIN: {
        int buf[128] = {0};
        int ret = mpTv->GetAudioEQGain(buf);
        int bufSize = mpTv->GetAudioEQBandCount();
        r->writeInt32(bufSize);
        for (int i = 0; i < bufSize; i++) {
            r->writeInt32(buf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_EQ_GAIN: {
        int buf[128] = {0};
        int bufSize = p.readInt32();
        for (int i = 0; i < bufSize; i++) {
            buf[i] = p.readInt32();
        }
        int ret = mpTv->SaveCurAudioEQGain(buf);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_EQ_GAIN: {
        int buf[128] = {0};
        int ret = mpTv->GetCurAudioEQGain(buf);
        int bufSize = mpTv->GetAudioEQBandCount();
        r->writeInt32(bufSize);
        for (int i = 0; i < bufSize; i++) {
            r->writeInt32(buf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_EQ_SWITCH: {
        int tmpVal = p.readInt32();
        int ret = mpTv->SetAudioEQSwitch(tmpVal);
        r->writeInt32(ret);
        break;
    }
    // AUDIO SPDIF SWITCH
    case SET_AUDIO_SPDIF_SWITCH: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SetAudioSPDIFSwitch(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SPDIF_SWITCH: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SaveCurAudioSPDIFSwitch(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SPDIF_SWITCH: {
        int ret = mpTv->GetCurAudioSPDIFSwitch();
        r->writeInt32(ret);
        break;
    }
    //AUDIO SPDIF MODE
    case SET_AUDIO_SPDIF_MODE: {
        int vol = p.readInt32();
        int progId = p.readInt32();
        int audioTrackId = p.readInt32();
        int ret = mpTv->SetAudioSPDIFMode(vol);
        mpTv->ResetAudioDecoderForPCMOutput();
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_SPDIF_MODE: {
        int vol = p.readInt32();
        int ret = mpTv->SaveCurAudioSPDIFMode(vol);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_AUDIO_SPDIF_MODE: {
        int ret = mpTv->GetCurAudioSPDIFMode();
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_OUTPUT_MODE: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SetAmAudioOutputMode(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_MUSIC_GAIN: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SetAmAudioMusicGain(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_LEFT_GAIN: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SetAmAudioLeftGain(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_RIGHT_GAIN: {
        int tmp_val = p.readInt32();
        int ret = mpTv->SetAmAudioRightGain(tmp_val);
        r->writeInt32(ret);
        break;
    }
    case SELECT_LINE_IN_CHANNEL: {
        int channel = p.readInt32();
        int ret = mpTv->AudioLineInSelectChannel(channel);
        r->writeInt32(ret);
        LOGD("SELECT_LINE_IN_CHANNEL: channel = %d; ret = %d.\n", channel, ret);
        break;
    }
    case SET_LINE_IN_CAPTURE_VOL: {
        int l_vol = p.readInt32();
        int r_vol = p.readInt32();
        int ret = mpTv->AudioSetLineInCaptureVolume(l_vol, r_vol);
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_VOL_COMP: {
        int tmpVal = p.readInt32();
        int ret = mpTv->SetCurProgramAudioVolumeCompensationVal(tmpVal);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_VOL_COMP: {
        int ret = mpTv->GetAudioVolumeCompensationVal(-1);
        r->writeInt32(ret);
        break;
    }
    // AUDIO END

    // SSM
    case SSM_INIT_DEVICE: {
        int tmpRet = 0;
        tmpRet = mpTv->Tv_SSMRestoreDefaultSetting();//mpTv->Tv_SSMInitDevice();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_POWER_ON_OFF_CHANNEL: {
        int tmpPowerChanNum = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnOffChannel(tmpPowerChanNum);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_OFF_CHANNEL: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnOffChannel();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SOURCE_INPUT: {
        int tmpSouceInput = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSourceInput(tmpSouceInput);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SOURCE_INPUT: {
        int tmpRet = 0;
        tmpRet = SSMReadSourceInput();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_LAST_SOURCE_INPUT: {
        int tmpLastSouceInput = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveLastSelectSourceInput(tmpLastSouceInput);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_LAST_SOURCE_INPUT: {
        int tmpRet = 0;
        tmpRet = SSMReadLastSelectSourceInput();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SYS_LANGUAGE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSystemLanguage(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SYS_LANGUAGE: {
        int tmpRet = 0;
        tmpRet = SSMReadSystemLanguage();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_AGING_MODE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveAgingMode(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_AGING_MODE: {
        int tmpRet = 0;
        tmpRet = SSMReadAgingMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PANEL_TYPE: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePanelType(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PANEL_TYPE: {
        int tmpRet = 0;
        tmpRet = SSMReadPanelType();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_MAC_ADDR: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = KeyData_SaveMacAddress(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_MAC_ADDR: {
        int ret = KeyData_ReadMacAddress(dataBuf);
        int size = KeyData_GetMacAddressDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_BAR_CODE: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = KeyData_SaveBarCode(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_BAR_CODE: {
        int ret = KeyData_ReadBarCode(dataBuf);
        int size = KeyData_GetBarCodeDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_HDCPKEY: {
        int size = p.readInt32();
        for (int i = 0; i < size; i++) {
            dataBuf[i] = p.readInt32();
        }
        int ret = SSMSaveHDCPKey(dataBuf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDCPKEY: {
        int ret = SSMReadHDCPKey(dataBuf);
        int size = SSMGetHDCPKeyDataLen();
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(dataBuf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_POWER_ON_MUSIC_SWITCH: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnMusicSwitch(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_MUSIC_SWITCH: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnMusicSwitch();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_POWER_ON_MUSIC_VOL: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSavePowerOnMusicVolume(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_POWER_ON_MUSIC_VOL: {
        int tmpRet = 0;
        tmpRet = SSMReadPowerOnMusicVolume();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SYS_SLEEP_TIMER: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSystemSleepTimer(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SYS_SLEEP_TIMER: {
        int tmpRet = 0;
        tmpRet = SSMReadSystemSleepTimer();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_INPUT_SRC_PARENTAL_CTL: {
        int tmpSourceIndex = p.readInt32();
        int tmpCtlFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveInputSourceParentalControl(tmpSourceIndex, tmpCtlFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_INPUT_SRC_PARENTAL_CTL: {
        int tmpSourceIndex = p.readInt32();
        int tmpRet = 0;
        tmpRet = SSMReadInputSourceParentalControl(tmpSourceIndex);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PARENTAL_CTL_SWITCH: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveParentalControlSwitch(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PARENTAL_CTL_SWITCH: {
        int tmpRet = 0;
        tmpRet = SSMReadParentalControlSwitch();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_PARENTAL_CTL_PASS_WORD: {
        String16 pass_wd_str = p.readString16();
        int tmpRet = SSMSaveParentalControlPassWord((unsigned char *)pass_wd_str.string(), pass_wd_str.size() * sizeof(unsigned short));
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_CUSTOMER_DATA_START: {
        int tmpRet = SSMGetCustomerDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_CUSTOMER_DATA_LEN: {
        int tmpRet = SSMGetCustomerDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_STANDBY_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveStandbyMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_STANDBY_MODE: {
        int tmpRet = SSMReadStandbyMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_LOGO_ON_OFF_FLAG: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet = SSMSaveLogoOnOffFlag(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_LOGO_ON_OFF_FLAG: {
        int tmpRet = SSMReadLogoOnOffFlag();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIEQ_MODE: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet = SSMSaveHDMIEQMode(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIEQ_MODE: {
        int tmpRet = SSMReadHDMIEQMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIINTERNAL_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveHDMIInternalMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIINTERNAL_MODE: {
        int tmpRet = SSMReadHDMIInternalMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_GLOBAL_OGOENABLE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveGlobalOgoEnable(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_GLOBAL_OGOENABLE: {
        int tmpRet = SSMReadGlobalOgoEnable();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NON_STANDARD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveNonStandardValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NON_STANDARD_STATUS: {
        int tmpRet = SSMReadNonStandardValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_ADB_SWITCH_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveAdbSwitchValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_ADB_SWITCH_STATUS: {
        int tmpRet = SSMReadAdbSwitchValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SERIAL_CMD_SWITCH_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveSerialCMDSwitchValue(tmp_val);
        tmpRet |= mpTv->SetSerialSwitch(SERIAL_A, tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SERIAL_CMD_SWITCH_STATUS: {
        int tmpRet = SSMReadSerialCMDSwitchValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SET_HDCP_KEY: {
        int tmpRet = SSMSetHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_REFRESH_HDCPKEY: {
        int tmpRet = SSMRefreshHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CHROMA_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveChromaStatus(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CA_BUFFER_SIZE: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveCABufferSizeValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_CA_BUFFER_SIZE: {
        int tmpRet = SSMReadCABufferSizeValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_START: {
        int tmpRet = SSMGetATVDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_LEN: {
        int tmpRet = SSMGetATVDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_START: {
        int tmpRet = SSMGetVPPDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_LEN: {
        int tmpRet = SSMGetVPPDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NOISE_GATE_THRESHOLD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet = SSMSaveNoiseGateThresholdValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NOISE_GATE_THRESHOLD_STATUS: {
        int tmpRet = SSMReadNoiseGateThresholdValue();
        r->writeInt32(tmpRet);
        break;
    }

    case SSM_SAVE_HDMI_EDID_VER: {
        int ret = -1;
        int port_id = p.readInt32();
        int ver = p.readInt32();
        ret = SSMSaveHDMIEdidMode((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDMI_EDID_VER: {
        int ret = -1;
        int port_id = p.readInt32();
        ret = SSMReadHDMIEdidMode((tv_hdmi_port_id_t)port_id);
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_HDCP_KEY_ENABLE: {
        int ret = -1;
        int enable = p.readInt32();
        ret = SSMSaveHDMIHdcpSwitcher(enable);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDCP_KEY_ENABLE: {
        int ret = -1;
        ret = SSMReadHDMIHdcpSwitcher();
        r->writeInt32(ret);
        break;
    }
    // SSM END

    //MISC
    case MISC_CFG_SET: {
        String8 key(p.readString16());
        String8 value(p.readString16());

        int tmpRet = config_set_str(CFG_SECTION_TV, key.string(), value.string());
        r->writeInt32(tmpRet);
        break;
    }
    case MISC_CFG_GET: {
        String8 key(p.readString16());
        String8 def(p.readString16());

        const char *value = config_get_str(CFG_SECTION_TV, key.string(), def.string());
        r->writeString16(String16(value));
        break;
    }
    case MISC_SET_WDT_USER_PET: {
        int counter = p.readInt32();
        int ret = TvMisc_SetUserCounter(counter);
        r->writeInt32(ret);
        break;
    }
    case MISC_SET_WDT_USER_COUNTER: {
        int counter_time_out = p.readInt32();
        int ret = TvMisc_SetUserCounterTimeOut(counter_time_out);
        r->writeInt32(ret);
        break;
    }
    case MISC_SET_WDT_USER_PET_RESET_ENABLE: {
        int enable = p.readInt32();
        int ret = TvMisc_SetUserPetResetEnable(enable);
        r->writeInt32(ret);
        break;
    }
    case MISC_GET_TV_API_VERSION: {
        // write tvapi version info
        const char *str = tvservice_get_git_branch_info();
        r->writeString16(String16(str));

        str = tvservice_get_git_version_info();
        r->writeString16(String16(str));

        str = tvservice_get_last_chaned_time_info();
        r->writeString16(String16(str));

        str = tvservice_get_build_time_info();
        r->writeString16(String16(str));

        str = tvservice_get_build_name_info();
        r->writeString16(String16(str));
        break;
    }
    case MISC_GET_DVB_API_VERSION: {
        // write dvb version info
        const char *str = dvb_get_git_branch_info();
        r->writeString16(String16(str));

        str = dvb_get_git_version_info();
        r->writeString16(String16(str));

        str = dvb_get_last_chaned_time_info();
        r->writeString16(String16(str));

        str = dvb_get_build_time_info();
        r->writeString16(String16(str));

        str = dvb_get_build_name_info();
        r->writeString16(String16(str));
        break;
    }
    case MISC_SERIAL_SWITCH: {
        int dev_id = p.readInt32();
        int switch_val = p.readInt32();
        int ret = mpTv->SetSerialSwitch(dev_id, switch_val);
        r->writeInt32(ret);
        break;
    }
    case MISC_SERIAL_SEND_DATA: {
        int devId = p.readInt32();
        int bufSize = p.readInt32();
        if (bufSize > (int)sizeof(dataBuf)) {
            bufSize = sizeof(dataBuf);
        }

        for (int i = 0; i < bufSize; i++) {
            dataBuf[i] = p.readInt32() & 0xFF;
        }

        int ret = mpTv->SendSerialData(devId, bufSize, dataBuf);
        r->writeInt32(ret);
        break;
    }
    //MISC  END

    // EXTAR
    case DTV_SUBTITLE_INIT: {
        int bitmapWidth = p.readInt32();
        int bitmapHeight = p.readInt32();
        r->writeInt32(mpTv->mSubtitle.sub_init(bitmapWidth, bitmapHeight));
        break;
    }
    case DTV_SUBTITLE_LOCK: {
        r->writeInt32(mpTv->mSubtitle.sub_lock());
        break;
    }
    case DTV_SUBTITLE_UNLOCK: {
        r->writeInt32(mpTv->mSubtitle.sub_unlock());
        break;
    }
    case DTV_GET_SUBTITLE_SWITCH: {
        r->writeInt32(mpTv->mSubtitle.sub_switch_status());
        break;
    }
    case DTV_START_SUBTITLE: {
        int dmx_id = p.readInt32();
        int pid = p.readInt32();
        int page_id = p.readInt32();
        int anc_page_id = p.readInt32();
        r->writeInt32(mpTv->mSubtitle.sub_start_dvb_sub(dmx_id, pid, page_id, anc_page_id));
        break;
    }
    case DTV_STOP_SUBTITLE: {
        r->writeInt32(mpTv->mSubtitle.sub_stop_dvb_sub());
        break;
    }
    case DTV_GET_SUBTITLE_INDEX: {
        int progId = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        r->writeInt32(prog.getSubtitleIndex(progId));
        break;
    }
    case DTV_SET_SUBTITLE_INDEX: {
        int progId = p.readInt32();
        int index = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        r->writeInt32(prog.setSubtitleIndex(progId, index));
        break;
    }
    case ATV_GET_CURRENT_PROGRAM_ID: {
        int atvLastProgramId = mpTv->getATVProgramID();
        r->writeInt32(atvLastProgramId);
        break;
    }
    case DTV_GET_CURRENT_PROGRAM_ID: {
        int dtvLastProgramId = mpTv->getDTVProgramID();
        r->writeInt32(dtvLastProgramId);
        break;
    }
    case ATV_SAVE_PROGRAM_ID: {
        int progID = p.readInt32();
        int retCnt = 0;
        mpTv->saveATVProgramID(progID);
        r->writeInt32(retCnt);
        break;
    }
    case ATV_GET_MIN_MAX_FREQ: {
        int min, max;
        int tmpRet = mpTv->getATVMinMaxFreq(&min, &max);
        r->writeInt32(min);
        r->writeInt32(max);
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_SCAN_FREQUENCY_LIST: {
        Vector<sp<CTvChannel> > out;
        int tmpRet = CTvRegion::getChannelListByName((char *)"CHINA,Default DTMB ALL", out);
        r->writeInt32(out.size());
        for (int i = 0; i < (int)out.size(); i++) {
            r->writeInt32(out[i]->getID());
            r->writeInt32(out[i]->getFrequency());
        }
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_CHANNEL_INFO: {
        int dbID = p.readInt32();
        channel_info_t chan_info;
        int ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
        r->writeInt32(chan_info.freq);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.strength);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.quality);
        r->writeInt32(chan_info.uInfo.dtvChanInfo.ber);
        r->writeInt32(ret);
        break;
    }
    case ATV_GET_CHANNEL_INFO: {
        int dbID = p.readInt32();
        channel_info_t chan_info;
        int ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
        r->writeInt32(chan_info.freq);
        r->writeInt32(chan_info.uInfo.atvChanInfo.finefreq);
        r->writeInt32(chan_info.uInfo.atvChanInfo.videoStd);
        r->writeInt32(chan_info.uInfo.atvChanInfo.audioStd);
        r->writeInt32(chan_info.uInfo.atvChanInfo.isAutoStd);
        r->writeInt32(ret);
        break;
    }
    case ATV_SCAN_MANUAL: {
        int tmpRet = 0;
        int startFreq = p.readInt32();
        int endFreq = p.readInt32();
        int videoStd = p.readInt32();
        int audioStd = p.readInt32();
        tmpRet = mpTv->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }

    case ATV_SCAN_AUTO: {
        int videoStd = p.readInt32();
        int audioStd = p.readInt32();
        int searchType = p.readInt32();
        int procMode = p.readInt32();
        int tmpRet = mpTv->atvAutoScan(videoStd, audioStd, searchType, procMode);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL: {
        int freq = p.readInt32();
        int tmpRet = mpTv->dtvManualScan(freq, freq);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL_BETWEEN_FREQ: {
        int beginFreq = p.readInt32();
        int endFreq = p.readInt32();
        int modulation = p.readInt32();
        int tmpRet = mpTv->dtvManualScan(beginFreq, endFreq, modulation);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_AUTO: {
        int tmpRet = mpTv->dtvAutoScan();
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case STOP_PROGRAM_PLAY: {
        int tmpRet = mpTv->stopPlayingLock();
        r->writeInt32(tmpRet);
        break;
    }
    case ATV_DTV_SCAN_PAUSE: {
        int tmpRet = mpTv->pauseScan();
        r->writeInt32(tmpRet);
        break;
    }
    case ATV_DTV_SCAN_RESUME: {
        int tmpRet = mpTv->resumeScan();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SET_TEXT_CODING: {
        String8 coding(p.readString16());
        mpTv->setDvbTextCoding((char *)coding.string());
        break;
    }

    case TV_CLEAR_ALL_PROGRAM: {
        int arg0 = p.readInt32();

        int tmpRet = mpTv->clearAllProgram(arg0);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_GET_KSV_INFO: {
        int data[2] = {0, 0};
        int tmpRet = mpTv->GetHdmiHdcpKeyKsvInfo(data);
        r->writeInt32(tmpRet);
        r->writeInt32(data[0]);
        r->writeInt32(data[1]);
        break;
    }
    case FACTORY_FBC_UPGRADE: {
        String8 strName(p.readString16());
        int mode = p.readInt32();
        int blkSize = p.readInt32();
        int ret = mpTv->StartUpgradeFBC((char *)strName.string(), mode, blkSize);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BRIGHTNESS: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetBrightness(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BRIGHTNESS: {
        int ret = mpTv->mFactoryMode.fbcGetBrightness();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_CONTRAST: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetContrast(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_CONTRAST: {
        int ret = mpTv->mFactoryMode.fbcGetContrast();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_SATURATION: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetSaturation(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_SATURATION: {
        int ret = mpTv->mFactoryMode.fbcGetSaturation();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_HUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetHueColorTint(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_HUE: {
        int ret = mpTv->mFactoryMode.fbcGetHueColorTint();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BACKLIGHT: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetBacklight(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT: {
        int ret = mpTv->mFactoryMode.fbcGetBacklight();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_BACKLIGHT_EN : {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcBacklightOnOffSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT_EN: {
        int ret = mpTv->mFactoryMode.fbcBacklightOnOffGet();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_LVDS_SSG: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcLvdsSsgSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_ELEC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetElecMode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_ELEC_MODE: {
        int ret = mpTv->mFactoryMode.fbcGetElecMode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_PIC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetPictureMode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_PIC_MODE: {
        int ret = mpTv->mFactoryMode.fbcGetPictureMode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_TEST_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetTestPattern(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_TEST_PATTERN: {
        int ret = mpTv->mFactoryMode.fbcGetTestPattern();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_RED: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainRed(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_RED: {
        int ret = mpTv->mFactoryMode.fbcGetGainRed();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainGreen(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_GREEN: {
        int ret = mpTv->mFactoryMode.fbcGetGainGreen();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetGainBlue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_BLUE: {
        int ret = mpTv->mFactoryMode.fbcGetGainBlue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_RED: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetRed(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_RED: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetRed();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetGreen(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_GREEN: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetGreen();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcSetOffsetBlue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_BLUE: {
        int ret = mpTv->mFactoryMode.fbcGetOffsetBlue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_COLORTEMP_MODE: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcColorTempModeSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_COLORTEMP_MODE: {
        int ret = mpTv->mFactoryMode.fbcColorTempModeGet();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_WB_INIT: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.fbcWBInitialSet(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_WB_INIT: {
        int ret = mpTv->mFactoryMode.fbcWBInitialGet();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_MAINCODE_VERSION: {
        char sw_version[64];
        char build_time[64];
        char git_version[64];
        char git_branch[64];
        char build_name[64];
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_SERIAL, sw_version, build_time, git_version, git_branch, build_name);
            r->writeString16(String16(sw_version));
            r->writeString16(String16(build_time));
            r->writeString16(String16(git_version));
            r->writeString16(String16(git_branch));
            r->writeString16(String16(build_name));
        } else {
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
            r->writeString16(String16("No FBC"));
        }
        break;
    }
    case FACTORY_SET_SN: {
        char StrFactSN[256] = {0};
        String16 strTemFactorySn = p.readString16();
        String8 strFactorySn = String8(strTemFactorySn);
        sprintf((char *)StrFactSN, "%s", strFactorySn.string());
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int iRet = fbcIns->cfbc_Set_FBC_Factory_SN(COMM_DEV_SERIAL, (const char *)StrFactSN);
            r->writeInt32(iRet);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_GET_SN: {
        char factorySerialNumber[256] = {0};
        memset((void *)factorySerialNumber, 0, 256);
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_Factory_SN(COMM_DEV_SERIAL, factorySerialNumber);
            r->writeString16(String16(factorySerialNumber));
        } else {
            r->writeString16(String16("No FBC"));
        }
        break;
    }
    case FACTORY_FBC_PANEL_GET_INFO: {
        char panel_model[64];
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            fbcIns->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);
            r->writeString16(String16(panel_model));
        } else {
            r->writeString16(String16(""));
        }
        break;
    }
    case FACTORY_FBC_PANEL_POWER_SWITCH: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_panel_power_switch(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_PANEL_SUSPEND: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_suspend(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_PANEL_USER_SETTING_DEFAULT: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_FBC_User_Setting_Default(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainRedSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainRedGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainGreenSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainGreenGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainBlueSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGainBlueGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetRedSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetRedGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetGreenSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetGreenGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetBlueSet(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceOffsetBlueGet(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_COLOR_TMP: {
        int source_type = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceColorTempModeGet(source_type);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_COLOR_TMP: {
        int source_type = p.readInt32();
        int Tempmode = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceColorTempModeSet(source_type, Tempmode, is_save);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SAVE_PRAMAS: {
        int source_type = p.readInt32();
        int mode = p.readInt32();
        int r_gain = p.readInt32();
        int g_gain = p.readInt32();
        int b_gain = p.readInt32();
        int r_offset = p.readInt32();
        int g_offset = p.readInt32();
        int b_offset = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalancePramSave(source_type, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_OPEN_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternOpen();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_CLOSE_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternClose();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GRAY_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternSet(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GRAY_PATTERN: {
        int ret = mpTv->mFactoryMode.whiteBalanceGrayPatternGet();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_ALL_PRAMAS: {
        int mode = p.readInt32();
        tcon_rgb_ogo_t params;
        int ret = mpTv->mFactoryMode.getColorTemperatureParams((vpp_color_temperature_mode_t)mode, &params);
        r->writeInt32(ret);
        r->writeInt32(params.r_gain);
        r->writeInt32(params.g_gain);
        r->writeInt32(params.b_gain);
        r->writeInt32(params.r_post_offset);
        r->writeInt32(params.g_post_offset);
        r->writeInt32(params.b_post_offset);
        break;
    }
    case STOP_SCAN: {
        mpTv->stopScanLock();
        break;
    }
    case DTV_GET_SNR: {
        int tmpRet = mpTv->getFrontendSNR();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_BER: {
        int tmpRet = mpTv->getFrontendBER();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_STRENGTH: {
        int tmpRet = mpTv->getFrontendSignalStrength();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_AUDIO_TRACK_NUM: {
        int programId = p.readInt32();
        int retCnt = mpTv->getAudioTrackNum(programId);
        r->writeInt32(retCnt);
        break;
    }
    case DTV_GET_AUDIO_TRACK_INFO: {
        int progId = p.readInt32();
        int aIdx = p.readInt32();
        int aFmt = -1;
        String8 lang;
        int iRet = mpTv->getAudioInfoByIndex(progId, aIdx, &aFmt, lang);
        r->writeInt32(aFmt);
        r->writeString16(String16(lang));
        break;
    }
    case DTV_SWITCH_AUDIO_TRACK: {
        int aPid = p.readInt32();
        int aFmt = p.readInt32();
        int aParam = p.readInt32();
        int ret = mpTv->switchAudioTrack(aPid, aFmt, aParam);
        r->writeInt32(ret);
        break;
    }
    case DTV_GET_CURR_AUDIO_TRACK_INDEX: {
        int currAduIdx = -1;
        int progId = p.readInt32();
        CTvProgram prog;
        CTvProgram::selectByID(progId, prog);
        currAduIdx = prog.getCurrAudioTrackIndex();
        r->writeInt32(currAduIdx);
        break;
    }
    case DTV_SET_AUDIO_CHANNEL_MOD: {
        int audioChannelIdx = p.readInt32();
        mpTv->setAudioChannel(audioChannelIdx);
        break;
    }
    case DTV_GET_AUDIO_CHANNEL_MOD: {
        int currChannelMod = mpTv->getAudioChannel();
        r->writeInt32(currChannelMod);
        break;
    }
    case DTV_GET_CUR_FREQ: {
        int progId = p.readInt32();
        CTvProgram prog;
        CTvChannel channel;

        int iRet = CTvProgram::selectByID(progId, prog);
        if (0 != iRet) return -1;
        prog.getChannel(channel);
        int freq = channel.getFrequency();
        r->writeInt32(freq);
        break;
    }
    case DTV_GET_EPG_UTC_TIME: {
        int utcTime = mpTv->getTvTime();
        r->writeInt32(utcTime);
        break;
    }
    case DTV_GET_EPG_INFO_POINT_IN_TIME: {
        int progid = p.readInt32();
        int utcTime = p.readInt32();
        CTvProgram prog;
        int ret = CTvProgram::selectByID(progid, prog);
        CTvEvent ev;
        ret = ev.getProgPresentEvent(prog.getSrc(), prog.getID(), utcTime, ev);
        r->writeString16(String16(ev.getName()));
        r->writeString16(String16(ev.getDescription()));
        r->writeString16(String16(ev.getExtDescription()));
        r->writeInt32(ev.getStartTime());
        r->writeInt32(ev.getEndTime());
        r->writeInt32(ev.getSubFlag());
        r->writeInt32(ev.getEventId());
        break;
    }
    case DTV_GET_EPG_INFO_DURATION: {
        Vector<sp<CTvEvent> > epgOut;
        int progid = p.readInt32();
        int iUtcStartTime = p.readInt32();
        int iDurationTime = p.readInt32();
        CTvProgram prog;
        CTvEvent ev;
        int iRet = CTvProgram::selectByID(progid, prog);
        if (0 != iRet) {
            break;
        }
        iRet = ev.getProgScheduleEvents(prog.getSrc(), prog.getID(), iUtcStartTime, iDurationTime, epgOut);
        if (0 != iRet) {
            break;
        }
        int iObOutSize = epgOut.size();
        if (0 == iObOutSize) {
            break;
        }

        r->writeInt32(iObOutSize);
        for (int i = 0; i < iObOutSize; i ++) {
            r->writeString16(String16(epgOut[i]->getName()));
            r->writeString16(String16(epgOut[i]->getDescription()));
            r->writeString16(String16(ev.getExtDescription()));
            r->writeInt32(epgOut[i]->getStartTime());
            r->writeInt32(epgOut[i]->getEndTime());
            r->writeInt32(epgOut[i]->getSubFlag());
            r->writeInt32(epgOut[i]->getEventId());
        }
        break;
    }
    case DTV_SET_PROGRAM_NAME: {
        CTvProgram prog;
        int progid = p.readInt32();
        String16 tmpName = p.readString16();
        String8 strName = String8(tmpName);
        prog.updateProgramName(progid, strName);
        break;
    }
    case DTV_SET_PROGRAM_SKIPPED: {
        CTvProgram prog;
        int progid = p.readInt32();
        bool bSkipFlag = p.readInt32();
        prog.setSkipFlag(progid, bSkipFlag);
        break;
    }
    case DTV_SET_PROGRAM_FAVORITE: {
        CTvProgram prog;
        int progid = p.readInt32();
        bool bFavorite = p.readInt32();
        prog.setFavoriteFlag(progid, bFavorite);
        break;
    }
    case DTV_DETELE_PROGRAM: {
        CTvProgram prog;
        int progid = p.readInt32();
        prog.deleteProgram(progid);
        break;
    }
    case SET_BLACKOUT_ENABLE: {
        int enable = p.readInt32();
        mpTv->setBlackoutEnable(enable);
        break;
    }
    case START_AUTO_BACKLIGHT: {
        mpTv->startAutoBackLight();
        break;
    }
    case STOP_AUTO_BACKLIGHT: {
        mpTv->stopAutoBackLight();
        break;
    }
    case IS_AUTO_BACKLIGHTING: {
        int on = mpTv->getAutoBackLight_on_off();
        r->writeInt32(on);
        break;
    }
    case GET_AVERAGE_LUMA: {
        int ret = mpTv->getAverageLuma();
        r->writeInt32(ret);
        break;
    }
    case GET_AUTO_BACKLIGHT_DATA: {
        int buf[128] = {0};
        int size = mpTv->getAutoBacklightData(buf);
        r->writeInt32(size);
        for (int i = 0; i < size; i++) {
            r->writeInt32(buf[i]);
        }
        break;
    }
    case SET_AUTO_BACKLIGHT_DATA: {
        int ret = mpTv->setAutobacklightData(String8(p.readString16()));
        r->writeInt32(ret);
        break;
    }

    case SSM_READ_BLACKOUT_ENABLE: {
        int enable = mpTv->getSaveBlackoutEnable();
        r->writeInt32(enable);
        break;
    }
    case DTV_SWAP_PROGRAM: {
        CTvProgram prog;
        int firstProgId = p.readInt32();
        int secondProgId = p.readInt32();
        CTvProgram::selectByID(firstProgId, prog);
        int firstChanOrderNum = prog.getChanOrderNum();
        CTvProgram::selectByID(secondProgId, prog);
        int secondChanOrderNum = prog.getChanOrderNum();
        prog.swapChanOrder(firstProgId, firstChanOrderNum, secondProgId, secondChanOrderNum);
        break;
    }
    case DTV_SET_PROGRAM_LOCKED: {
        CTvProgram prog;
        int progid = p.readInt32();
        bool bLocked = p.readInt32();
        prog.setLockFlag(progid, bLocked);
        break;
    }
    case DTV_GET_FREQ_BY_PROG_ID: {
        int freq = 0;
        int progid = p.readInt32();
        CTvProgram prog;
        int ret = CTvProgram::selectByID(progid, prog);
        if (ret != 0) return -1;
        CTvChannel channel;
        prog.getChannel(channel);
        freq = channel.getFrequency();
        r->writeInt32(freq);
        break;
    }
    case DTV_GET_BOOKED_EVENT: {
        CTvBooking tvBook;
        Vector<sp<CTvBooking> > vTvBookOut;
        tvBook.getBookedEventList(vTvBookOut);
        int iObOutSize = vTvBookOut.size();
        if (0 == iObOutSize) {
            break;
        }
        r->writeInt32(iObOutSize);
        for (int i = 0; i < iObOutSize; i ++) {
            r->writeString16(String16(vTvBookOut[i]->getProgName()));
            r->writeString16(String16(vTvBookOut[i]->getEvtName()));
            r->writeInt32(vTvBookOut[i]->getStartTime());
            r->writeInt32(vTvBookOut[i]->getDurationTime());
            r->writeInt32(vTvBookOut[i]->getBookId());
            r->writeInt32(vTvBookOut[i]->getProgramId());
            r->writeInt32(vTvBookOut[i]->getEventId());
        }
        break;
    }
    case SET_FRONTEND_PARA: {
        int ret = -1;
        frontend_para_set_t feParms;
        feParms.mode = (fe_type_t)p.readInt32();
        feParms.freq = p.readInt32();
        feParms.videoStd = (atv_video_std_t)p.readInt32();
        feParms.audioStd = (atv_audio_std_t)p.readInt32();
        feParms.para1 = p.readInt32();
        feParms.para2 = p.readInt32();
        mpTv->resetFrontEndPara(feParms);
        r->writeInt32(ret);
        break;
    }
    case PLAY_PROGRAM: {
        int mode = p.readInt32();
        int freq = p.readInt32();
        if (mode == FE_ANALOG) {
            int videoStd = p.readInt32();
            int audioStd = p.readInt32();
            int fineTune = p.readInt32();
            int audioCompetation = p.readInt32();
            mpTv->playAtvProgram(freq, videoStd, audioStd, fineTune, audioCompetation);
        } else {
            int para1 = p.readInt32();
            int para2 = p.readInt32();
            int vid = p.readInt32();
            int vfmt = p.readInt32();
            int aid = p.readInt32();
            int afmt = p.readInt32();
            int pcr = p.readInt32();
            int audioCompetation = p.readInt32();
            mpTv->playDtvProgram(mode, freq, para1, para2, vid, vfmt, aid, afmt, pcr, audioCompetation);
        }
        break;
    }
    case GET_PROGRAM_LIST: {
        Vector<sp<CTvProgram> > out;
        int type = p.readInt32();
        int skip = p.readInt32();
        CTvProgram::selectByType(type, skip, out);
        r->writeInt32(out.size());
        for (int i = 0; i < (int)out.size(); i++) {
            r->writeInt32(out[i]->getID());
            r->writeInt32(out[i]->getChanOrderNum());
            r->writeInt32(out[i]->getMajor());
            r->writeInt32(out[i]->getMinor());
            r->writeInt32(out[i]->getProgType());
            r->writeString16(String16(out[i]->getName()));
            r->writeInt32(out[i]->getProgSkipFlag());
            r->writeInt32(out[i]->getFavoriteFlag());
            r->writeInt32(out[i]->getVideo()->getFormat());
            CTvChannel ch;
            out[i]->getChannel(ch);
            r->writeInt32(ch.getDVBTSID());
            r->writeInt32(out[i]->getServiceId());
            r->writeInt32(out[i]->getVideo()->getPID());
            r->writeInt32(out[i]->getVideo()->getPID());

            int audioTrackSize = out[i]->getAudioTrackSize();
            r->writeInt32(audioTrackSize);
            for (int j = 0; j < audioTrackSize; j++) {
                r->writeString16(String16(out[i]->getAudio(j)->getLang()));
                r->writeInt32(out[i]->getAudio(j)->getFormat());
                r->writeInt32(out[i]->getAudio(j)->getPID());
            }
            Vector<CTvProgram::Subtitle *> mvSubtitles = out[i]->getSubtitles();
            int subTitleSize = mvSubtitles.size();
            r->writeInt32(subTitleSize);
            if (subTitleSize > 0) {
                for (int k = 0; k < subTitleSize; k++) {
                    r->writeInt32(mvSubtitles[k]->getPID());
                    r->writeString16(String16(mvSubtitles[k]->getLang()));
                    r->writeInt32(mvSubtitles[k]->getCompositionPageID());
                    r->writeInt32(mvSubtitles[k]->getAncillaryPageID());
                }
            }
            r->writeInt32(ch.getFrequency());
        }
        break;
    }
    case DTV_GET_VIDEO_FMT_INFO: {
        int srcWidth = 0;
        int srcHeight = 0;
        int srcFps = 0;
        int srcInterlace = 0;
        int iRet = -1;

        iRet == mpTv->getVideoFormatInfo(&srcWidth, &srcHeight, &srcFps, &srcInterlace);
        r->writeInt32(srcWidth);
        r->writeInt32(srcHeight);
        r->writeInt32(srcFps);
        r->writeInt32(srcInterlace);
        r->writeInt32(iRet);
    }
    break;

    case DTV_START_RECORD: {
        mpTv->SetRecordFileName((char *)String8(p.readString16()).string());
        mpTv->StartToRecord();
    }
    break;
    case DTV_STOP_RECORD:
        mpTv->StopRecording();
        break;
    case HDMIAV_HOTPLUGDETECT_ONOFF: {
        int flag = mpTv->GetHdmiAvHotplugDetectOnoff();
        r->writeInt32(flag);
        break;
    }
    // EXTAR END
    default:
        LOGD("default");
        break;
    }

    LOGD("exit client=%d cmd=%d", getCallingPid(), cmd);
    return 0;
}

int TvService::Client::notifyCallback(const int &msgtype, const Parcel &p)
{
    mTvClient->notifyCallback(msgtype, p);
    return 0;
}

status_t TvService::dump(int fd, const Vector<String16>& args)
{
    String8 result;
    if (!checkCallingPermission(String16("android.permission.DUMP"))) {
        char buffer[256];
        snprintf(buffer, 256, "Permission Denial: "
                "can't dump system_control from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        Mutex::Autolock lock(mLock);

        if (args.size() > 0) {
            for (int i = 0; i < (int)args.size(); i ++) {
                if (args[i] == String16("-s")) {
                    String8 section(args[i+1]);
                    String8 key(args[i+2]);
                    const char *value = config_get_str(section, key, "");
                    result.appendFormat("section:[%s] key:[%s] value:[%s]\n", section.string(), key.string(), value);
                }
                else if (args[i] == String16("-h")) {
                    result.append(
                        "\ntv service use to control the tv logical \n\n"
                        "usage: dumpsys tvservice [-s <SECTION> <KEY>][-h] \n"
                        "-s: get config string \n"
                        "   SECTION:[TV|ATV|SourceInputMap|SETTING|FBCUART]\n"
                        "-h: help \n\n");
                }
            }
        }
        else {
            result.appendFormat("client num = %d\n", mUsers);
            for (int i = 0; i < (int)mClients.size(); i++) {
                wp<Client> client = mClients[i];
                if (client != 0) {
                    sp<Client> c = client.promote();
                    if (c != 0) {
                        result.appendFormat("client[%d] pid = %d\n", i, c->getPid());
                    }
                }
            }

            mpTv->dump(result);
        }
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}


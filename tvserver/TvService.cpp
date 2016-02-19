#define LOG_TAG "TvService"

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

#define PICTURE_TIMEOUT seconds(5)

static int getCallingPid()
{
    return IPCThreadState::self()->getCallingPid();
}

TvService *TvService::mTvService = NULL;
CTv *TvService::mpTv = NULL;

void TvService::instantiate()
{
    defaultServiceManager()->addService(String16("aml.tvserver"), TvService::getIntance());
}

TvService::TvService() :
    BnTvService()
{
    mpStartTvClient = NULL;
    mpScannerClient = NULL;
    mUsers = 0;
    mpTv =  new CTv();
    mpTv->setTvObserver(this);
    mCapVidFrame.setObserver(this);
    mpTv->OpenTv();
}

TvService::~TvService()
{
    mpStartTvClient = NULL;
    mpScannerClient = NULL;
    int client_size = m_v_Clients.size();
    wp<Client> client;
    for (int i = 0; i < client_size; i++) {
        client = m_v_Clients[i];
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
    case CTvEv::TV_EVENT_COMMOM: {
        break;
    }
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
                ScannerClient->notifyCallback(SCAN_EVENT_CALLBACK, p);
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_EPG: {
        CTvEpg::EpgEvent *pEpgEvent = (CTvEpg::EpgEvent *) (&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEpgEvent->type);
                    p.writeInt32(pEpgEvent->time);
                    p.writeInt32(pEpgEvent->programID);
                    p.writeInt32(pEpgEvent->channelID);
                    currentClient->getTvClient()->notifyCallback(EPG_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_HDMI_IN_CAP: {
        CTvScreenCapture::CapEvent *pCapEvt  = (CTvScreenCapture::CapEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pCapEvt->mFrameNum);
                    p.writeInt32(pCapEvt->mFrameSize);
                    p.writeInt32(pCapEvt->mFrameWide);
                    p.writeInt32(pCapEvt->mFrameHeight);
                    currentClient->getTvClient()->notifyCallback(VFRAME_BMP_EVENT_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_AV_PLAYBACK: {
        TvEvent::AVPlaybackEvent *pEv = (TvEvent::AVPlaybackEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mMsgType);
                    p.writeInt32(pEv->mProgramId);
                    currentClient->getTvClient()->notifyCallback(DTV_AV_PLAYBACK_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_SIGLE_DETECT: {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);

        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mTrans_fmt);
                    p.writeInt32(pEv->mFmt);
                    p.writeInt32(pEv->mStatus);
                    p.writeInt32(pEv->mReserved);
                    currentClient->getTvClient()->notifyCallback(SIGLE_DETECT_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_SUBTITLE: {
        TvEvent::SubtitleEvent *pEv = (TvEvent::SubtitleEvent *)(&ev);
        sp<Client> pSubtitleClient = mpSubClient.promote();
        if (pSubtitleClient != NULL) {
            Parcel p;
            p.writeInt32(pEv->pic_width);
            p.writeInt32(pEv->pic_height);
            pSubtitleClient->notifyCallback(SUBTITLE_UPDATE_CALLBACK, p);
        }
        break;
    }
    case CTvEv::TV_EVENT_ADC_CALIBRATION: {
        TvEvent::ADCCalibrationEvent *pEv = (TvEvent::ADCCalibrationEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    currentClient->getTvClient()->notifyCallback(ADC_CALIBRATION_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_VGA: { //VGA
        TvEvent::VGAEvent *pEv = (TvEvent::VGAEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    currentClient->getTvClient()->notifyCallback(VGA_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_SOURCE_CONNECT: {
        TvEvent::SourceConnectEvent *pEv = (TvEvent::SourceConnectEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mSourceInput);
                    p.writeInt32(pEv->connectionState);
                    currentClient->getTvClient()->notifyCallback(SOURCE_CONNECT_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_HDMIRX_CEC: {
        TvEvent::HDMIRxCECEvent *pEv = (TvEvent::HDMIRxCECEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;

                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }

                    currentClient->getTvClient()->notifyCallback(HDMIRX_CEC_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_UPGRADE_FBC: {
        TvEvent::UpgradeFBCEvent *pEv = (TvEvent::UpgradeFBCEvent *)(&ev);

        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mState);
                    p.writeInt32(pEv->param);
                    currentClient->getTvClient()->notifyCallback(UPGRADE_FBC_CALLBACK, p);
                }
            }
        }
        break;
    }
    case CTvEv::TV_EVENT_SERIAL_COMMUNICATION: {
        TvEvent::SerialCommunicationEvent *pEv = (TvEvent::SerialCommunicationEvent *)(&ev);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->mDevId);
                    p.writeInt32(pEv->mDataCount);
                    for (int j = 0; j < pEv->mDataCount; j++) {
                        p.writeInt32(pEv->mDataBuf[j]);
                    }

                    currentClient->getTvClient()->notifyCallback(SERIAL_COMMUNICATION_CALLBACK, p);
                }
            }
        }
        break;
    }

    case CTvEv::TV_EVENT_2d4G_HEADSET: {
        TvEvent::HeadSetOf2d4GEvent *pEv = (TvEvent::HeadSetOf2d4GEvent *)(&ev);
        LOGD("SendDtvStats status: =%d para2: =%d", pEv->state, pEv->para);
        int client_size = m_v_Clients.size();
        wp<Client> client;
        for (int i = 0; i < client_size; i++) {
            client = m_v_Clients[i];
            if (client != 0) {
                sp<Client> currentClient = client.promote();
                if (currentClient != 0) {
                    Parcel p;
                    p.writeInt32(pEv->state);
                    p.writeInt32(pEv->para);
                    currentClient->getTvClient()->notifyCallback(HEADSET_STATUS_CALLBACK, p);
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
    LOGD("TvService::connect E (pid %d, client %p)", callingPid, IInterface::asBinder(tvClient).get());

    Mutex::Autolock lock(mServiceLock);

    int client_size = m_v_Clients.size();
    wp<Client> client;
    for (int i = 0; i < client_size; i++) {
        client = m_v_Clients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("TvService::connect X (pid %d, same client %p) is reconnecting...", callingPid, IInterface::asBinder(tvClient).get());
                    return currentClient;
                } else {
                    //LOGD("TvService::connect X (pid %d, new client %p) rejected. "
                    //      "(old pid %d, old client %p)", callingPid, tvClient->asBinder().get(), currentClient->mClientPid, currentTvClient->asBinder().get());
                    // if (kill(currentClient->mClientPid, 0) == -1 && errno == ESRCH) {
                    //     LOGE("The old client is dead!");
                    // }
                    //return client;
                    //return currentClient;
                    continue;
                }
            } else {
                LOGE("client (pid %d) have delete ------------not exist", callingPid);
                client.clear();
                m_v_Clients.removeAt(i);
                client_size--;
                continue;
            }
        }
    }

    if (mUsers > 0) {
        LOGE("Still have client, rejected");
        //return client;
    }
    LOGD("TvService::connect X 3");
    // client = new Client(this, tvClient, callingPid);
    //mClient = client;

    sp<Client> newclient = new Client(this, tvClient, callingPid, mpTv);
    m_v_Clients.add(newclient);

#if DEBUG_CLIENT_REFERENCES
    //client->trackMe(true, true);
#endif
    //mCurClient = client;
    LOGD("TvService::connect X 4");
    return newclient;
}

void TvService::removeClient(const sp<ITvClient> &tvClient)
{
    int callingPid = getCallingPid();

    Mutex::Autolock lock(mServiceLock);

    int client_size = m_v_Clients.size();
    wp<Client> client;
    for (int i = 0; i < client_size; i++) {
        client = m_v_Clients[i];
        if (client != 0) {
            sp<Client> currentClient = client.promote();
            if (currentClient != 0) {
                sp<ITvClient> currentTvClient(currentClient->getTvClient());
                if (IInterface::asBinder(tvClient) == IInterface::asBinder(currentTvClient)) {
                    LOGD("find client , and remove it pid = %d, client = %p i=%d", callingPid, IInterface::asBinder(tvClient).get(), i);
                    client.clear();
                    LOGD("find client , and remove it pid1 ");
                    m_v_Clients.removeAt(i);
                    LOGD("find client , and remove it pid2 ");
                    break;
                } else {
                    LOGW("removeClient (pid %d): mClient doesn't match!", callingPid);
                    continue;
                }
            } else {
                LOGW("removeclient  currentClient == 0 (pid %d)", callingPid);
                client.clear();
                m_v_Clients.removeAt(i);
                client_size--;
                continue;
            }
        } else {
            LOGW("removeclient  client == 0 (pid %d)", callingPid);
            client.clear();
            m_v_Clients.removeAt(i);
            client_size--;
            continue;
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
    int callingPid = getCallingPid();
    LOGD("Client::Client E (pid %d)", callingPid);
    mTvService = tvService;
    mTvClient = tvClient;
    mClientPid = clientPid;
    tvService->incUsers();
    mpTv = pTv;
    mIsStartTv = false;
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
        LOGD("clear mTvClient (pid %d)", callingPid);
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
        sp<ITvClient> oldClient;
        {
            Mutex::Autolock _l(mLock);
            if (mClientPid != 0 && checkPid() != NO_ERROR) {
                LOGW("Tried to connect to locked tv (old pid %d, new pid %d)", mClientPid, callingPid);
                return -EBUSY;
            }
            oldClient = mTvClient;

            // did the client actually change?
            if ((mTvClient != NULL) && (IInterface::asBinder(client) == IInterface::asBinder(mTvClient))) {
                LOGD("Connect to the same client");
                return NO_ERROR;
            }

            mTvClient = client;
            mClientPid = -1;
            LOGD("Connect to the new client (pid %d, client %p)", callingPid, IInterface::asBinder(mTvClient).get());
        }

    }
    mClientPid = callingPid;
    return NO_ERROR;
}

TvService::Client::~Client()
{
    if (mIsStartTv) mpTv->StopTvLock();

    int callingPid = getCallingPid();
    // tear down client
    LOGD("Client::~Client E (pid %d, client %p)", callingPid, IInterface::asBinder(getTvClient()).get());
    // make sure we tear down the hardware
    mClientPid = callingPid;
    disconnect();
    LOGD("Client::~Client X (pid %d)", mClientPid);
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

status_t TvService::Client::createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly)
{
    LOGD(" mem=%d size=%d", share_mem->pointer() == NULL, share_mem->size());
    LOGD("iSourceMode :%d iCapVideoLayerOnly = %d \n", iSourceMode, iCapVideoLayerOnly);
    int Len = 0;
    Mutex::Autolock lock(mLock);
    mTvService->mCapVidFrame.InitVCap(share_mem);

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

    return 0;
}

status_t TvService::Client::createSubtitle(const sp<IMemory> &share_mem)
{
    mSubBmpBuf = share_mem;
    LOGD("createSubtitle pid = %d, mem=%d size=%d", getCallingPid(), share_mem->pointer() == NULL, share_mem->size());
    mpTv->setSubtitleBuffer((char *)mSubBmpBuf->pointer());
    mTvService->mpSubClient = this;
    //pSub = new CTvSubtitle(share_mem, this);
    //pSub->run();
    return 0;
}
status_t TvService::Client::processCmd(const Parcel &p, Parcel *r)
{
    int tmp_i_buf[128] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
    unsigned char tmp_uc_buf[512] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
    unsigned char tmp_uc_buf2[512] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
    int8_t tmp_int8_buf[512] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
    static int source_input = 0;
    int *data_ptr = NULL;

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
        //mTvService->mpStartTvClient = this;
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
        int ret = 0;
        ret = (int)mpTv->GetTvStatus();
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
    case GET_CURRENT_SOURCE_INPUT_TYPE: {
        int ret = 0;//(int)mpTv->Tvin_GetSrcInputType();
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
    case IS_SOURCE_SWTICH_DONE: {
        //int ret = mpTv->Tv_IsSourceSwtichDone();
        r->writeInt32(0);
        break;
    }
    case SET_SOURCE_INPUT: {
        int sourceinput = p.readInt32();
        tvin_window_pos_t win_pos;
        LOGD(" SetSourceInput sourceId= %x", sourceinput);
        source_input = sourceinput;
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
    case RUN_ADC_AUTO_CALIBRATION: {
        //        int ret = mpTv->Tv_RunADCAutoCalibration();
        //        r->writeInt32(ret);
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
    case GET_VIDEO_PATH_STATUS: {
        int path_type = p.readInt32();
        int ret = 0;//(int)mpTv->Tvin_CheckPathActive((tv_path_type_t)path_type);
        r->writeInt32(ret);
        break;
    }
    case GET_VIDEO_STREAM_STATUS: {
        //        int ret = (int)mpTv->Tvin_CheckVideoStreamStatus();
        //        r->writeInt32(ret);
        break;
    }
    case GET_FIRST_START_SWITCH_TYPE: {
        //int tmp_val = 0;
        //int ret = (int)mpTv->Tv_GetFirstStartSwitchType(&tmp_val);
        //r->writeInt32(ret);
        r->writeInt32(1);
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

    case SET_VIDEO_DISABLE: {
        int value = p.readInt32();
        int ret = 0;//(int)mpTv->Tv_SetVideoDisable(value);
        r->writeInt32(ret);
        break;
    }

    case GET_SOURCE_CONNECT_STATUS: {
        int source_input = p.readInt32();
        int ret = mpTv->GetSourceConnectStatus((tv_source_input_t)source_input);
        r->writeInt32(ret);
        break;
    }
    //Tv function END

    //VGA
    case RUN_VGA_AUTO_ADJUST: {
        //int ret = mpTv->Tv_RunVGAAutoAdjust();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_AUTO_ADJUST_STATUS: {
        //        int fmt = p.readInt32();
        //int ret = (int) mpTv->Tv_GetVagAutoAdjustStatus();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case IS_VGA_AUTO_ADJUST_DONE: {
        int fmt = p.readInt32();
        int ret = 0;//mpTv->Tv_IsVGAAutoAdjustDone((tvin_sig_fmt_t)fmt);
        r->writeInt32(ret);
        break;
    }
    case SET_VGA_HPOS: {
        //        int value = p.readInt32();
        //        int fmt = p.readInt32();
        //int ret = mpTv->Tv_SetVGAHPos(value, (tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_HPOS: {
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_GetVGAHPos((tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }

    case SET_VGA_VPOS: {
        //        int value = p.readInt32();
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_SetVGAVPos(value, (tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_VPOS: {
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_GetVGAVPos((tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }

    case SET_VGA_CLOCK: {
        //        int value = p.readInt32();
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_SetVGAClock(value, (tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_CLOCK: {
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_GetVGAClock((tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }

    case SET_VGA_PHASE: {
        //        int value = p.readInt32();
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_SetVGAPhase(value, (tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_PHASE: {
        //        int fmt = p.readInt32();
        //        int ret = mpTv->Tv_GetVGAPhase((tvin_sig_fmt_t)fmt);
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case SET_VGA_AJUST_PARA: {
        tvafe_vga_parm_t adjparam;
        adjparam.clk_step = p.readInt32();
        adjparam.phase = p.readInt32();
        adjparam.hpos_step = p.readInt32();
        adjparam.vpos_step = p.readInt32();
        adjparam.vga_in_clean = p.readInt32();
        int fmt = p.readInt32();
        int ret = 0/*SetVGAAjustPara(adjparam, (tvin_sig_fmt_t)fmt)*/;
        r->writeInt32(ret);
        break;
    }
    case GET_VGA_AJUST_PARA: {
        tvafe_vga_parm_t adjparam;
        int fmt = p.readInt32();
        int ret = 0/*GetVGAAjustPara(&adjparam, (tvin_sig_fmt_t)fmt)*/;
        r->writeInt32(adjparam.clk_step);
        r->writeInt32(adjparam.phase);
        r->writeInt32(adjparam.hpos_step);
        r->writeInt32(adjparam.vpos_step);
        r->writeInt32(adjparam.vga_in_clean);
        r->writeInt32(ret);
        break;
    }
    case SET_VGAPARAM_DEFAULT: {
        //        int ret = (int)mpTv->TvinResetVgaAjustParam();
        //        r->writeInt32(ret);
        break;
    }
    // VGA END

    // 3D

    case SET_3D_MODE: {
        int mode = p.readInt32();
        int ret = mpTv->Tv_Set3DMode((VIDEO_3D_MODE_T)mode);
        r->writeInt32(ret);
        break;
    }
    case GET_3D_MODE: {
        int ret = (int)mpTv->Tv_Get3DMode();
        r->writeInt32(ret);
        break;
    }
    case SET_3D_LR_SWITH: {
        int on_off = p.readInt32();
        //int status = p.readInt32();
        int ret = mpTv->Tv_Set3DLRSwith(on_off);
        r->writeInt32(ret);
        break;
    }
    case GET_3D_LR_SWITH: {
        int ret = mpTv->Tv_Get3DLRSwith();
        r->writeInt32(ret);
        break;
    }
    case SET_3D_TO_2D_MODE: {
        int mode = p.readInt32();
        //int status = p.readInt32();
        int ret = mpTv->Tv_Set3DTo2DMode(mode);
        r->writeInt32(ret);
        break;
    }
    case GET_3D_TO_2D_MODE: {
        int ret = mpTv->Tv_Get3DTo2DMode();
        r->writeInt32(ret);
        break;
    }
    case SET_3D_DEPTH: {
        int value = p.readInt32();
        int ret = mpTv->Tv_Set3DDepth(value);
        r->writeInt32(ret);
        break;
    }
    case GET_3D_DEPTH: {
        int ret = mpTv->GetSave3DDepth();
        r->writeInt32(ret);
        break;
    }
    // 3D END

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
        LOGD("GET_BRIGHTNESS------------=%d", source_type);
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
    case SET_SCENEMODE: {
        int mode = p.readInt32();
        int is_save = p.readInt32();
        int ret = 0;//mpTv->Tv_SetSceneMode((vpp_scene_mode_t)mode,is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_SCENEMODE: {
        int ret = 0;//(int)mpTv->Tv_GetSceneMode();
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
        int status_3d = p.readInt32();
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
        int ret  = mpTv->Tv_SetColorTemperature((vpp_color_temperature_mode_t)mode, (tv_source_input_type_t)source_type, is_save);
        r->writeInt32(ret);
        break;
    }
    case GET_COLOR_TEMPERATURE: {
        int source_type = p.readInt32();
        int ret  = mpTv->Tv_GetColorTemperature((tv_source_input_type_t)source_type);
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
    case VPP_SPLIT_SCREEN_EFFECT: {
        int mode = p.readInt32();
        int width = p.readInt32();
        int reverse = p.readInt32();
        int ret = mpTv->Tv_SplitScreenEffect(mode, width, reverse);
        r->writeInt32(ret);
        break;
    }
    // PQ END

    // FACTORY
    case FACTORY_SETPQMODE_BRIGHTNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int brightness = p.readInt32();
        int ret = mpTv->Tv_FactorySetPQMode_Brightness(source_type, pq_mode, brightness);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_BRIGHTNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetPQMode_Brightness(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_CONTRAST: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int contrast = p.readInt32();
        int ret = mpTv->Tv_FactorySetPQMode_Contrast(source_type, pq_mode, contrast);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_CONTRAST: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetPQMode_Contrast(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_SATURATION: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int saturation = p.readInt32();
        int ret = mpTv->Tv_FactorySetPQMode_Saturation(source_type, pq_mode, saturation);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_SATURATION: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetPQMode_Saturation(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_HUE: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int hue = p.readInt32();
        int ret = mpTv->Tv_FactorySetPQMode_Hue(source_type, pq_mode, hue);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_HUE: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetPQMode_Hue(source_type, pq_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPQMODE_SHARPNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int sharpness = p.readInt32();
        int ret = mpTv->Tv_FactorySetPQMode_Sharpness(source_type, pq_mode, sharpness);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETPQMODE_SHARPNESS: {
        int source_type = p.readInt32();
        int pq_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetPQMode_Sharpness(source_type, pq_mode);

        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETTESTPATTERN: {
        int pattern = p.readInt32();
        int ret = mpTv->Tv_FactorySetTestPattern(pattern);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETTESTPATTERN: {
        int ret = mpTv->Tv_FactoryGetTestPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETPATTERN_YUV: {
        int blend = p.readInt32();
        int y = p.readInt32();
        int u = p.readInt32();
        int v = p.readInt32();
        int ret = mpTv->Tv_FactorySetScreenColor(blend, y, u, v);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETPQMODE: {
        int ret = mpTv->Tv_FactoryResetPQMode();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETCOLORTEMP: {
        int ret = mpTv->Tv_FactoryResetColorTemp();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_RESETPAMAMSDEFAULT: {
        int ret = mpTv->Tv_FactorySetParamsDefault();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETDDRSSC: {
        int setp = p.readInt32();
        int ret = mpTv->Tv_FactorySetDDRSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETDDRSSC: {
        int ret = mpTv->Tv_FactoryGetDDRSSC();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDSSSC: {
        int setp = p.readInt32();
        int ret = mpTv->Tv_FactorySetLVDSSSC(setp);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDSSSC: {
        int ret = mpTv->Tv_FactoryGetLVDSSSC();
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
        int ret = mpTv->Tv_FactorySetNolineParams(noline_params_type, source_type, params);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETNOLINEPARAMS: {
        int noline_params_type = p.readInt32();
        int source_type = p.readInt32();
        noline_params_t params = mpTv->Tv_FactoryGetNolineParams(noline_params_type, source_type);
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
        int status_3d = p.readInt32();
        int trans_fmt = p.readInt32();
        cutwin_t.hs = p.readInt32();
        cutwin_t.he = p.readInt32();
        cutwin_t.vs = p.readInt32();
        cutwin_t.ve = p.readInt32();
        int ret = mpTv->Tv_FactorySetOverscan(source_type, fmt, status_3d, trans_fmt, cutwin_t);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETOVERSCAN: {
        int source_type = p.readInt32();
        int fmt = p.readInt32();
        int status_3d = p.readInt32();
        int trans_fmt = p.readInt32();
        tvin_cutwin_t cutwin_t = mpTv->Tv_FactoryGetOverscan(source_type, fmt, status_3d, trans_fmt);
        r->writeInt32(cutwin_t.hs);
        r->writeInt32(cutwin_t.he);
        r->writeInt32(cutwin_t.vs);
        r->writeInt32(cutwin_t.ve);
        break;
    }
    case DELETE_PARAM_PQ_DB: {
        int tmpRet = mpTv->Tv_ReplacePqDb();
        r->writeInt32(tmpRet);
    }
    case REPLACE_PARAM_PQ_DB: {
        int tmpRet;
        String16 tmp_str16;
        String8 value_str;

        tmp_str16 = p.readString16();
        value_str = String8(tmp_str16);

        tmpRet = mpTv->Tv_ReplacePqDb(value_str.string());
        r->writeInt32(tmpRet);
        break;
    }
    case FACTORY_SET_OUT_DEFAULT: {
        int ret = 0;
        ret = mpTv->Tv_SSMFacRestoreDefaultSetting();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_RGAIN: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_GGAIN: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_BGAIN: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_ROFFSET: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_GOFFSET: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETGLOBALOGO_BOFFSET: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_RGAIN: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_GGAIN: {
        int p1 = p.readInt32();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_BGAIN: {
        int p1 = p.readInt32();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_ROFFSET: {
        int p1 = p.readInt32();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_GOFFSET: {
        int p1 = p.readInt32();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETGLOBALOGO_BOFFSET: {
        int p1 = p.readInt32();
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case FACTORY_CLEAN_ALL_TABLE_FOR_PROGRAM: {
        int ret = mpTv->ClearAnalogFrontEnd();
        mpTv->clearDbAllProgramInfoTable();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETBACKLIGHT_PWM_FREQUENCY: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetBacklightPWM_Frequency(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETBACKLIGHT_PWM_FREQUENCY: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetBacklightPWM_Frequency();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETBACKLIGHT_SWITCH_STATUS: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetBacklight_Switch_status(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETBACKLIGHT_SWITCH_STATUS: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetBacklight_Switch_status();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETBACKLIGHT_PWM_DUTY: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetBacklightPWM_Duty(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETBACKLIGHT_PWM_DUTY: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetBacklightPWM_Duty();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_SETLVDS_COLOR_DEPTH: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetLVDS_ColorDepth(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDS_COLOR_DEPTH: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetLVDS_ColorDepth();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDS_DITHER_STATUS: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetLVDS_ColorDither_status(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDS_DITHER_STATUS: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetLVDS_ColorDither_status();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDS_MAPPING_STATUS: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetLVDS_Mapping_status(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDS_MAPPING_STATUS: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetLVDS_Mapping_status();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_SETLVDS_PORT_SWAP_STATUS: {
        int value = p.readInt32();
        int ret = 0;//mpTv->GetVpp().Tv_FactorySetLVDS_PortSwap_status(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_GETLVDS_PORT_SWAP_STATUS: {
        int ret = 0;//mpTv->GetVpp().Tv_FactoryGetLVDS_PortSwap_status();
        r->writeInt32(ret);
        break;
    }
    // FACTORY END

    // AUDIO
    // AUDIO MUTE
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
    case SET_AUDIO_FORCE_MUTE_STATUS: {
        int status = p.readInt32();
        int ret = 0;//pTv->getTvAudio().AudioSetForceMuteStatus(status);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_FORCE_MUTE_STATUS: {
        int ret = 0;//mpTv->AudioGetForceMuteStatus();
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
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_i_buf[i] = p.readInt32();
        }
        ret = mpTv->SetAudioEQGain(tmp_i_buf);
        r->writeInt32(ret);
        break;
    }
    case GET_AUDIO_EQ_GAIN: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        ret = mpTv->GetAudioEQGain(tmp_i_buf);
        tmp_buf_size = mpTv->GetAudioEQBandCount();
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_i_buf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SAVE_CUR_AUDIO_EQ_GAIN: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_i_buf[i] = p.readInt32();
        }
        ret = mpTv->SaveCurAudioEQGain(tmp_i_buf);
        r->writeInt32(ret);
        break;
    }
    case GET_CUR_EQ_GAIN: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        ret = mpTv->GetCurAudioEQGain(tmp_i_buf);
        tmp_buf_size = mpTv->GetAudioEQBandCount();
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_i_buf[i]);
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
    case GET_AUDIO_SPDIF_SWITCH: {
        int ret = 0;//mpTv->GetAudioSPDIFSwitch();
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
    case GET_AUDIO_SPDIF_MODE: {
        int ret = 0;//mpTv->GetAudioSPDIFMode();
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
    case OPEN_AMAUDIO: {
        int sr = p.readInt32();
        int output_dev = p.readInt32();
        int ret = 0;//mpTv->OpenAmAudio(sr, output_dev);
        r->writeInt32(ret);
        break;
    }
    case CLOSE_AMAUDIO: {
        int ret = 0;//mpTv->CloseAmAudio();
        r->writeInt32(ret);
        break;
    }
    case SET_AMAUDIO_INPUT_SR: {
        int sr = p.readInt32();
        int output_dev = p.readInt32();
        int ret = 0;//mpTv->SetAmAudioInputSr(sr, output_dev);
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
    case SET_KALAOK_IO_LEVEL: {
        int level = p.readInt32();
        int ret = mpTv->SetKalaokIO(level);
        ret = mpTv->AudioSetAudioInSource(CC_AUDIO_IN_SOURCE_LINEIN);
        r->writeInt32(ret);
        LOGD("set line in source CC_AUDIO_IN_SOURCE_LINEIN\n");
        break;
    }

    case SET_LINE_IN_CAPTURE_VOL: {
        int l_vol = p.readInt32();
        int r_vol = p.readInt32();
        int ret = mpTv->AudioSetLineInCaptureVolume(l_vol, r_vol);
        r->writeInt32(ret);
        break;
    }
    case HANDLE_AUDIO_HEADSET_PLUG_IN: {
        int ret = mpTv->AudioHandleHeadsetPlugIn();
        r->writeInt32(ret);
        break;
    }
    case HANDLE_AUDIO_HEADSET_PULL_OUT: {
        int ret = mpTv->AudioHandleHeadsetPullOut();
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
    case SAVE_AUDIO_VOL_COMP: {
        int tmpVal = p.readInt32();
        int ret = -1;//mpTv->atvSaveAudioVolumeCompensationVal(tmpVal);
        LOGD("this cmd is empty!!!!!!!!!!!!!!!!!!!");
        r->writeInt32(ret);
        break;
    }
    case SET_NOISE_GATE_THRESHOLD: {
        int ret = 0;
        r->writeInt32(ret);
        break;
    }
    case SET_AUDIO_SOURCE_FOR_KARAOKE: {
        int sourceinput = p.readInt32();
        int ret = mpTv->Tv_SetAudioSourceType((tv_source_input_t)sourceinput);
        r->writeInt32(ret);
        break;
    }
    case SET_DBX_TV_MODE: {
        int mode = p.readInt32();
        int son_value = p.readInt32();
        int vol_value = p.readInt32();
        int sur_value =  p.readInt32();
        int ret = mpTv->SetDbxTvMode(mode, son_value,  vol_value,  sur_value);
        r->writeInt32(ret);
        break;
    }
    case GET_DBX_TV_MODE: {
        int mode ;
        int son_value ;
        int vol_value ;
        int sur_value ;
        int ret = mpTv->GetDbxTvMode(&mode, &son_value,  &vol_value,  &sur_value);
        r->writeInt32(mode);
        r->writeInt32(son_value);
        r->writeInt32(vol_value);
        r->writeInt32(sur_value);
        r->writeInt32(ret);
        break;
    }

    case SET_DRC_ONOFF: {
        int val = p.readInt32();
        int ret = mpTv->Tv_SetDRC_OnOff(val);
        r->writeInt32(ret);
        break;
    }
    case GET_DRC_ONOFF: {
        int ret = mpTv->Tv_GetDRC_OnOff();
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
    /*case SSM_SAVE_ONE_BYTE: {
        int tmpOffset = p.readInt32();
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMWriteOneByte(tmpOffset, tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_ONE_BYTE: {
        int tmpOffset = p.readInt32();
        int tmpRet = 0;
        SSMReadOneByte(tmpOffset, &tmpRet);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_N_BYTES: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;

        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];

            if (data_ptr != NULL) {
                for (i = 0; i < tmp_data_len; i++) {
                    data_ptr[i] = p.readInt32();
                }

                ret = SSMWriteNTypes(tmp_offset, tmp_data_len, data_ptr);

                delete data_ptr;
                data_ptr = NULL;
            }
        }

        r->writeInt32(ret);
        break;
    }
    case SSM_READ_N_BYTES: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;
        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];
            if (data_ptr != NULL) {
                ret = SSMReadNTypes(tmp_offset, tmp_data_len, data_ptr);
                if (ret < 0) {
                    tmp_data_len = 0;
                }
                r->writeInt32(tmp_data_len);
                for (i = 0; i < tmp_data_len; i++) {
                    r->writeInt32(data_ptr[i]);
                }
                delete data_ptr;
                data_ptr = NULL;
            }
        }
        r->writeInt32(ret);
        break;
    }*/
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
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_uc_buf[i] = p.readInt32();
        }
        ret = KeyData_SaveMacAddress(tmp_uc_buf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_MAC_ADDR: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        ret = KeyData_ReadMacAddress(tmp_uc_buf);
        tmp_buf_size = KeyData_GetMacAddressDataLen();
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_BAR_CODE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_uc_buf[i] = p.readInt32();
        }
        ret = KeyData_SaveBarCode(tmp_uc_buf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_BAR_CODE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        ret = KeyData_ReadBarCode(tmp_uc_buf);
        tmp_buf_size = KeyData_GetBarCodeDataLen();
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_SAVE_PROJECT_ID: {
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = KeyData_SaveProjectID(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PROJECT_ID: {
        int tmpRet = 0;
        tmpRet = KeyData_ReadProjectID();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDCPKEY: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_uc_buf[i] = p.readInt32();
        }
        ret = SSMSaveHDCPKey(tmp_uc_buf);
        r->writeInt32(ret);
        break;
    }
    case SSM_READ_HDCPKEY: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        ret = SSMReadHDCPKey(tmp_uc_buf);
        tmp_buf_size = SSMGetHDCPKeyDataLen();
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
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
    case SSM_SET_BUS_STATUS: {
        int tmpVal = p.readInt32();
        int tmpRet = 0;
        //showboz
        //tmpRet = SSMSetBusStatus(tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_BUS_STATUS: {
        int tmpRet = 0;
        //showboz
        //tmpRet = SSMGetBusStatus();
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
        int tmpRet;
        tmpRet = SSMSaveParentalControlPassWord((unsigned char *)pass_wd_str.string(), pass_wd_str.size() * sizeof(unsigned short));
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_PARENTAL_CTL_PASS_WORD: {
        int tmpChanID = p.readInt32();
        unsigned short tmp_buf [SSM_RW_PARENTAL_CTL_PASSWORD_LEN + 1];
        String16 pass_wd_str;
        SSMReadParentalControlPassWord(tmp_buf);
        tmp_buf[SSM_RW_PARENTAL_CTL_PASSWORD_LEN / sizeof(unsigned short)] = 0;
        //pass_wd_str.setTo((const unsigned short *) tmp_buf);
        //r->writeString16(pass_wd_str);
        break;
    }
    case SSM_SAVE_USING_DEF_HDCP_KEY_FLAG: {
        break;
    }
    case SSM_READ_USING_DEF_HDCP_KEY_FLAG: {
        break;
    }
    case SSM_GET_CUSTOMER_DATA_START: {
        int tmpRet = 0;
        tmpRet = SSMGetCustomerDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_CUSTOMER_DATA_LEN: {
        int tmpRet = 0;
        tmpRet = SSMGetCustomerDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_STANDBY_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveStandbyMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_STANDBY_MODE: {
        int tmpRet = 0;
        tmpRet = SSMReadStandbyMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_LOGO_ON_OFF_FLAG: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveLogoOnOffFlag(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_LOGO_ON_OFF_FLAG: {
        int tmpRet = 0;
        tmpRet = SSMReadLogoOnOffFlag();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIEQ_MODE: {
        int tmpSwitchFlag = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveHDMIEQMode(tmpSwitchFlag);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIEQ_MODE: {
        int tmpRet = 0;
        tmpRet = SSMReadHDMIEQMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_HDMIINTERNAL_MODE: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveHDMIInternalMode(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_HDMIINTERNAL_MODE: {
        int tmpRet = 0;
        tmpRet = SSMReadHDMIInternalMode();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_DISABLE_3D: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveDisable3D(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_DISABLE_3D: {
        int tmpRet = 0;
        tmpRet = SSMReadDisable3D();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_GLOBAL_OGOENABLE: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveGlobalOgoEnable(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_GLOBAL_OGOENABLE: {
        int tmpRet = 0;
        tmpRet = SSMReadGlobalOgoEnable();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NON_STANDARD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveNonStandardValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NON_STANDARD_STATUS: {
        int tmpRet = 0;
        tmpRet = SSMReadNonStandardValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_ADB_SWITCH_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveAdbSwitchValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_ADB_SWITCH_STATUS: {
        int tmpRet = 0;
        tmpRet = SSMReadAdbSwitchValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_SERIAL_CMD_SWITCH_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveSerialCMDSwitchValue(tmp_val);
        tmpRet |= mpTv->SetSerialSwitch(SERIAL_A, tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_SERIAL_CMD_SWITCH_STATUS: {
        int tmpRet = 0;
        tmpRet = SSMReadSerialCMDSwitchValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SET_HDCP_KEY: {
        int tmpRet = 0;
        tmpRet = SSMSetHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_REFRESH_HDCPKEY: {
        int tmpRet = 0;
        tmpRet = SSMRefreshHDCPKey();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CHROMA_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveChromaStatus(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_CA_BUFFER_SIZE: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveCABufferSizeValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_CA_BUFFER_SIZE: {
        int tmpRet = 0;
        tmpRet = SSMReadCABufferSizeValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_START: {
        int tmpRet = 0;
        tmpRet = SSMGetATVDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_ATV_DATA_LEN: {
        int tmpRet = 0;
        tmpRet = SSMGetATVDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_START: {
        int tmpRet = 0;
        tmpRet = SSMGetVPPDataStart();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_GET_VPP_DATA_LEN: {
        int tmpRet = 0;
        tmpRet = SSMGetVPPDataLen();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_SAVE_NOISE_GATE_THRESHOLD_STATUS: {
        int tmp_val = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveNoiseGateThresholdValue(tmp_val);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_READ_NOISE_GATE_THRESHOLD_STATUS: {
        int tmpRet = 0;
        tmpRet = SSMReadNoiseGateThresholdValue();
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_EEPROM_SAVE_ONE_BYTE_N310_N311: {
        int tmpOffset = p.readInt32();
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveEEP_One_N310_N311(tmpOffset, tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_EEPROM_READ_ONE_BYTE_N310_N311: {
        int tmpOffset = p.readInt32();
        int tmpRet = 0;
        tmpRet = SSMReadEEP_One_N310_N311(tmpOffset);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_EEPROM_SAVE_N_BYTES_N310_N311: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;

        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];

            if (data_ptr != NULL) {
                for (i = 0; i < tmp_data_len; i++) {
                    data_ptr[i] = p.readInt32();
                }

                ret = SSMSaveEEP_N_N310_N311(tmp_offset, tmp_data_len, data_ptr);

                delete data_ptr;
                data_ptr = NULL;
            }
        }

        r->writeInt32(ret);
        break;
    }
    case SSM_EEPROM_READ_N_BYTES_N310_N311: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;
        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];
            if (data_ptr != NULL) {
                ret = SSMReadEEP_N_N310_N311(tmp_offset, tmp_data_len, data_ptr);
                if (ret < 0) {
                    tmp_data_len = 0;
                }
                r->writeInt32(tmp_data_len);
                for (i = 0; i < tmp_data_len; i++) {
                    r->writeInt32(data_ptr[i]);
                }
                delete data_ptr;
                data_ptr = NULL;
            }
        }
        r->writeInt32(ret);
        break;
    }
    case SSM_FLASH_SAVE_ONE_BYTE_N310_N311: {
        int tmpOffset = p.readInt32();
        int tmpVal = p.readInt32();
        int tmpRet;
        tmpRet = SSMSaveFlash_One_N310_N311(tmpOffset, tmpVal);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_FLASH_READ_ONE_BYTE_N310_N311: {
        int tmpOffset = p.readInt32();
        int tmpRet = 0;
        tmpRet = SSMReadFlash_One_N310_N311(tmpOffset);
        r->writeInt32(tmpRet);
        break;
    }
    case SSM_FLASH_SAVE_N_BYTES_N310_N311: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;

        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];

            if (data_ptr != NULL) {
                for (i = 0; i < tmp_data_len; i++) {
                    data_ptr[i] = p.readInt32();
                }

                ret = SSMSaveFlash_N_N310_N311(tmp_offset, tmp_data_len, data_ptr);

                delete data_ptr;
                data_ptr = NULL;
            }
        }

        r->writeInt32(ret);
        break;
    }
    case SSM_FLASH_READ_N_BYTES_N310_N311: {
        int i = 0, tmp_offset = 0, tmp_data_len = 0, ret = -1;
        tmp_offset = p.readInt32();
        tmp_data_len = p.readInt32();

        if (tmp_data_len > 0) {
            data_ptr = new int[tmp_data_len];
            if (data_ptr != NULL) {
                ret = SSMReadFlash_N_N310_N311(tmp_offset, tmp_data_len, data_ptr);
                if (ret < 0) {
                    tmp_data_len = 0;
                }
                r->writeInt32(tmp_data_len);
                for (i = 0; i < tmp_data_len; i++) {
                    r->writeInt32(data_ptr[i]);
                }
                delete data_ptr;
                data_ptr = NULL;
            }
        }
        r->writeInt32(ret);
        break;
    }
    // SSM END

    //MISC
    case MISC_PROP_SET: {
        int tmpRet;
        String16 tmp_str16;
        String8 key_str, value_str;

        tmp_str16 = p.readString16();
        key_str = String8(tmp_str16);
        tmp_str16 = p.readString16();
        value_str = String8(tmp_str16);

        tmpRet = property_set(key_str.string(), value_str.string());
        r->writeInt32(tmpRet);
        break;
    }
    case MISC_PROP_GET: {
        char prop_value[PROPERTY_VALUE_MAX];
        String16 tmp_str16;
        String8 key_str, def_str;

        tmp_str16 = p.readString16();
        key_str = String8(tmp_str16);
        tmp_str16 = p.readString16();
        def_str = String8(tmp_str16);

        memset(prop_value, '\0', PROPERTY_VALUE_MAX);
        property_get(key_str.string(), prop_value, def_str.string());
        tmp_str16 = String16(prop_value);

        r->writeString16(tmp_str16);
        break;
    }
    case MISC_CFG_SET: {
        int tmpRet;
        String16 tmp_str16;
        String8 key_str, value_str;

        tmp_str16 = p.readString16();
        key_str = String8(tmp_str16);
        tmp_str16 = p.readString16();
        value_str = String8(tmp_str16);

        tmpRet = config_set_str(CFG_SECTION_TV, key_str.string(), value_str.string());
        r->writeInt32(tmpRet);
        break;
    }
    case MISC_CFG_GET: {
        const char *prop_value;
        String16 tmp_str16;
        String8 key_str, def_str;

        tmp_str16 = p.readString16();
        key_str = String8(tmp_str16);
        tmp_str16 = p.readString16();
        def_str = String8(tmp_str16);

        prop_value = config_get_str(CFG_SECTION_TV, key_str.string(), def_str.string());
        tmp_str16 = String16(prop_value);

        r->writeString16(tmp_str16);
        break;
    }
    case MISC_READ_ADC_VAL: {
        int tmpChanNum = p.readInt32();
        int tmpRet = 0;
        tmpRet = ReadADCSpecialChannelValue(tmpChanNum);
        r->writeInt32(tmpRet);
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
        const char *tmp_str8;
        String16 tmp_str16;

        // write tvapi version info
        tmp_str8 = tvservice_get_git_branch_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = tvservice_get_git_version_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = tvservice_get_last_chaned_time_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = tvservice_get_build_time_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = tvservice_get_build_name_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);
        break;
    }
    case MISC_GET_DVB_API_VERSION: {
        const char *tmp_str8;
        String16 tmp_str16;

        // write dvb version info
        tmp_str8 = dvb_get_git_branch_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = dvb_get_git_version_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = dvb_get_last_chaned_time_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = dvb_get_build_time_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

        tmp_str8 = dvb_get_build_name_info();
        tmp_str16 = String16(tmp_str8);
        r->writeString16(tmp_str16);

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
        int i = 0, bufSize = 0, ret = 0;

        int dev_id = p.readInt32();

        bufSize = p.readInt32();
        if (bufSize > (int)sizeof(tmp_uc_buf)) {
            bufSize = sizeof(tmp_uc_buf);
        }

        for (i = 0; i < bufSize; i++) {
            tmp_uc_buf[i] = p.readInt32() & 0xFF;
        }

        ret = mpTv->SendSerialData(dev_id, bufSize, tmp_uc_buf);
        r->writeInt32(ret);
        break;
    }
    case MISC_CHANNEL_EXPORT: {
        LOGD("MISC_CHANNEL_EXPORT");
        int  ret = 0;
        String16 tmp_str16;
        String8 tmp_str;

        tmp_str16 = p.readString16();
        tmp_str = String8(tmp_str16);

        ret = mpTv->ChannelExport(tmp_str.string());
        r->writeInt32(ret);
        break;
    }
    case MISC_CHANNEL_IMPORT: {
        LOGD("MISC_CHANNEL_IMPORT");
        String16 tmp_str16;
        String8 tmp_str;

        tmp_str16 = p.readString16();
        tmp_str = String8(tmp_str16);
        int  ret = 0;
        ret = mpTv->ChannelImport(tmp_str.string());
        r->writeInt32(ret);
        break;
    }
    case MISC_GET_PROJECT_INFO: {
        String16 tmp_str16;
        project_info_t tmpInfo;

        if (mpTv->Tv_GetProjectInfo(&tmpInfo) < 0) {
            strcpy(tmpInfo.version, "UNKOWN");
            strcpy(tmpInfo.panel_type, "UNKOWN");
            strcpy(tmpInfo.panel_outputmode, "UNKOWN");
            strcpy(tmpInfo.panel_rev, "UNKOWN");
            strcpy(tmpInfo.panel_name, "UNKOWN");
            strcpy(tmpInfo.amp_curve_name, "UNKOWN");
        }

        r->writeString16(String16(tmpInfo.version));
        r->writeString16(String16(tmpInfo.panel_type));
        r->writeString16(String16(tmpInfo.panel_outputmode));
        r->writeString16(String16(tmpInfo.panel_rev));
        r->writeString16(String16(tmpInfo.panel_name));
        r->writeString16(String16(tmpInfo.amp_curve_name));
        break;
    }
    case MISC_GET_PLATFORM_TYPE: {
        int ret = mpTv->Tv_GetPlatformType();
        r->writeInt32(ret);
        break;
    }
    //MISC  END

    // EXTAR
    case SET_DEBUG_SERIAL_PORT_ONOFF: {
        int on_off = p.readInt32();
        int ret = mpTv->SetDebugSerialOnOff(on_off);
        r->writeInt32(ret);
        break;
    }

    case GET_DEBUG_SERIAL_PORT_ONOFF: {
        int ret = mpTv->GetDebugSerialOnOff();
        r->writeInt32(ret);
        break;
    }
    case DELETE_DIR_FILES: {
        String16 strPath = p.readString16();
        if (strPath.size() <= 0) {
            r->writeInt32(-1);
            break;
        }
        String8 strP8 = String8(strPath);
        int tmp_flag = p.readInt32();
        int ret = TvMisc_DeleteDirFiles(strP8.string(), tmp_flag);
        r->writeInt32(ret);
        break;
    }
    case DTV_SUBTITLE_INIT: {
        int bitmapWidth = p.readInt32();
        int bitmapHeight = p.readInt32();
        r->writeInt32(mpTv->initSubtitle(bitmapWidth, bitmapHeight));
        break;
    }
    case DTV_SUBTITLE_LOCK: {
        r->writeInt32(mpTv->lockSubtitle());
        break;
    }
    case DTV_SUBTITLE_UNLOCK: {
        r->writeInt32(mpTv->unlockSubtitle());
        break;
    }
    case DTV_GET_SUBTITLE_SWITCH: {
        r->writeInt32(mpTv->getSubSwitchStatus());
        break;
    }
    case DTV_START_SUBTITLE: {
        int dmx_id = p.readInt32();
        int pid = p.readInt32();
        int page_id = p.readInt32();
        int anc_page_id = p.readInt32();
        r->writeInt32(mpTv->startSubtitle(dmx_id, pid, page_id, anc_page_id));
        break;
    }
    case DTV_STOP_SUBTITLE: {
        r->writeInt32(mpTv->stopSubtitle());
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
        int tmpRet = 0;
        LOGD("%s, atv auto scan!!!\n", "TV");
        int videoStd = p.readInt32();
        int audioStd = p.readInt32();
        int searchType = p.readInt32();
        tmpRet = mpTv->atvAutoScan(videoStd, audioStd, searchType);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL: {
        int tmpRet = 0;
        int freq = p.readInt32();
        tmpRet = mpTv->dtvManualScan(freq, freq);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_MANUAL_BETWEEN_FREQ: {
        int tmpRet = 0;
        int beginFreq = p.readInt32();
        int endFreq = p.readInt32();
        int modulation = p.readInt32();
        tmpRet = mpTv->dtvManualScan(beginFreq, endFreq, modulation);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_AUTO: {
        int tmpRet = 0;
        tmpRet = mpTv->dtvAutoScan();
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_SCAN_AUTO_ATSC: {
        int tmpRet = 0;
        int attenna = p.readInt32();
        int vstd = p.readInt32();
        int astd = p.readInt32();
        tmpRet = mpTv->dtvAutoScanAtscLock(attenna, vstd, astd);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }
    case TV_SUBTITLE_DRAW_END: {
        int tmpRet = 0;
        //if(pSub)pSub->lock.unlock();
        r->writeInt32(tmpRet);
        break;
    }
    case STOP_PROGRAM_PLAY: {
        int tmpRet = -1;
        tmpRet = mpTv->stopPlayingLock();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_TEST_1: {
        // int progid = p.readInt32();
        //CTvProgram prog;
        // int ret = CTvProgram::selectByID(progid, prog);
        //CTvEvent ev;
        //ret = ev.getProgPresentEvent(prog.getSrc(), prog.getID(), mpTv->getTvTime(), ev);
        //r->writeString16(String16(ev.getName()));
        break;
    }
    case DTV_TEST_2: {
        //int cmd = p.readInt32();
        //CTvDatabase::GetTvDb()->importXmlToDB("/etc/tv_default.xml");
        //r->writeString16(String16("xml"));
        break;
    }

    case TV_CLEAR_ALL_PROGRAM: {
        int tmpRet = 0;
        int arg0 = p.readInt32();

        tmpRet = mpTv->clearAllProgram(arg0);
        mTvService->mpScannerClient = this;
        r->writeInt32(tmpRet);
        break;
    }

    case GET_DISPLAY_RESOLUTION_CONFIG: {
        int tmpRet = 0;
        tmpRet = mpTv->GetDisplayResolutionConfig();
        r->writeInt32(tmpRet);
        break;
    }

    case GET_DISPLAY_RESOLUTION_INFO: {
        int tmpRet = 0;
        tmpRet = mpTv->GetDisplayResolutionInfo();
        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_CEC_SEND_CUSTOM_MESSAGE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_uc_buf[i] = p.readInt32();
        }

        int tmpRet = 0;
        tmpRet = mpTv->SendHDMIRxCECCustomMessage(tmp_uc_buf);
        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_CEC_SEND_CUSTOM_WAIT_REPLY_MESSAGE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        int WaitCmd = 0, timeout = 0;
        tmp_buf_size = p.readInt32();
        for (i = 0; i < tmp_buf_size; i++) {
            tmp_uc_buf[i] = p.readInt32();
        }

        WaitCmd = p.readInt32();
        timeout = p.readInt32();

        int tmpRet = 0;
        tmpRet = mpTv->SendHDMIRxCECCustomMessageAndWaitReply(tmp_uc_buf, tmp_uc_buf2, WaitCmd, timeout);

        r->writeInt32(tmpRet);
        if (tmpRet > 0) {
            for (i = 0; i < tmpRet; i++) {
                r->writeInt32(tmp_uc_buf2[i]);
            }
        }

        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_CEC_SEND_BROADCAST_STANDBY_MESSAGE: {
        int tmpRet = 0;
        tmpRet = mpTv->SendHDMIRxCECBoradcastStandbyMessage();
        r->writeInt32(tmpRet);
        break;
    }

    case HDMIRX_CEC_SEND_GIVE_CEC_VERSION_MESSAGE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        int sourceinput = p.readInt32();
        LOGD("HDMIRX_CEC_SEND_GIVE_CEC_VERSION_MESSAGE: sourceinput = %x", sourceinput);
        tmp_buf_size = mpTv->SendHDMIRxCECGiveCECVersionMessage((tv_source_input_t)sourceinput, tmp_uc_buf);
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
        }
        r->writeInt32(tmp_buf_size);
        break;
    }

    case HDMIRX_CEC_SEND_GIVE_DEV_VENDOR_ID_MESSAGE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        int sourceinput = p.readInt32();
        LOGD("HDMIRX_CEC_SEND_GIVE_DEV_VENDOR_ID_MESSAGE: sourceinput = %x", sourceinput);
        tmp_buf_size = mpTv->SendHDMIRxCECGiveDeviceVendorIDMessage((tv_source_input_t)sourceinput, tmp_uc_buf);
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
        }
        r->writeInt32(tmp_buf_size);
        break;
    }

    case HDMIRX_CEC_SEND_GIVE_OSD_NAME_MESSAGE: {
        int i = 0, tmp_buf_size = 0, ret = 0;
        int sourceinput = p.readInt32();
        LOGD("HDMIRX_CEC_SEND_GIVE_OSD_NAME_MESSAGE: sourceinput = %x", sourceinput);
        tmp_buf_size = mpTv->SendHDMIRxCECGiveOSDNameMessage((tv_source_input_t)sourceinput, tmp_uc_buf);
        r->writeInt32(tmp_buf_size);
        for (i = 0; i < tmp_buf_size; i++) {
            r->writeInt32(tmp_uc_buf[i]);
        }
        r->writeInt32(tmp_buf_size);
        break;
    }

    case GET_HDMI_KSV_INFO: {
        int tmpRet = 0;
        int ksv_data[2] = {0, 0};
        tmpRet = mpTv->GetHdmiHdcpKeyKsvInfo(ksv_data);
        r->writeInt32(tmpRet);
        r->writeInt32(ksv_data[0]);
        r->writeInt32(ksv_data[1]);
        break;
    }

    case HDMI_OUT_TOWHAT: {
        r->writeInt32(mpTv->hdmiOutWithFbc() ? 1 : 0);
        break;
    }
    case FACTORY_FBC_UPGRADE: {
        String16 tmpName = p.readString16();
        String8 strName = String8(tmpName);
        sprintf((char *)tmp_uc_buf, "%s", strName.string());
        int mode = p.readInt32();
        int upgrade_blk_size = p.readInt32();
        int ret = mpTv->StartUpgradeFBC((char *)tmp_uc_buf, mode, upgrade_blk_size);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BRIGHTNESS: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Brightness(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BRIGHTNESS: {
        int ret = mpTv->Tv_FactoryGet_FBC_Brightness();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_CONTRAST: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Contrast(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_CONTRAST: {
        int ret = mpTv->Tv_FactoryGet_FBC_Contrast();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_SATURATION: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Saturation(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_SATURATION: {
        int ret = mpTv->Tv_FactoryGet_FBC_Saturation();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_HUE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_HueColorTint(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_HUE: {
        int ret = mpTv->Tv_FactoryGet_FBC_HueColorTint();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BACKLIGHT: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Backlight(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT: {
        int ret = mpTv->Tv_FactoryGet_FBC_Backlight();
        r->writeInt32(ret);
        break;
    }
    case FACTROY_FBC_SET_LIGHT_SENSOR_STATUS_N310: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_LightSensor_Status_N310(value);
        r->writeInt32(ret);
        break;
    }

    case FACTROY_FBC_GET_LIGHT_SENSOR_STATUS_N310: {
        int ret = mpTv->Tv_FactoryGet_FBC_LightSensor_Status_N310();
        r->writeInt32(ret);
        break;
    }
    case FACTROY_FBC_SET_DREAM_PANEL_STATUS_N310: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Dream_Panel_Status_N310(value);
        r->writeInt32(ret);
        break;
    }

    case FACTROY_FBC_GET_DREAM_PANEL_STATUS_N310: {
        int ret = mpTv->Tv_FactoryGet_FBC_Dream_Panel_Status_N310();
        r->writeInt32(ret);
        break;
    }
    case FACTROY_FBC_SET_MULT_PQ_STATUS_N310: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_MULT_PQ_Status_N310(value);
        r->writeInt32(ret);
        break;
    }

    case FACTROY_FBC_GET_MULT_PQ_STATUS_N310: {
        int ret = mpTv->Tv_FactoryGet_FBC_MULT_PQ_Status_N310();
        r->writeInt32(ret);
        break;
    }
    case FACTROY_FBC_SET_MEMC_STATUS_N310: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_MEMC_Status_N310(value);
        r->writeInt32(ret);
        break;
    }

    case FACTROY_FBC_GET_MEMC_STATUS_N310: {
        int ret = mpTv->Tv_FactoryGet_FBC_MEMC_Status_N310();
        r->writeInt32(ret);
        break;
    }

    case MISC_SET_2K_TO_4K_SCALE_UP_MODE : {
        int value = p.readInt32();
        int ret = mpTv->Tv_Set2k4k_ScalerUp_Mode(value);
        r->writeInt32(ret);
        break;
    }

    case MISC_GET_2K_TO_4K_SCALE_UP_MODE: {
        int ret = mpTv->Tv_Get2k4k_ScalerUp_Mode();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_BACKLIGHT_EN : {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_backlight_onoff(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT_EN: {
        int ret = mpTv->Tv_FactoryGet_FBC_backlight_onoff();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_TEST_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->Tv_SetTestPattern(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_FBC_SET_LVDS_SSG: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_LVDS_SSG_Set(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_ELEC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_ELEC_MODE(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_ELEC_MODE: {
        int ret = mpTv->Tv_FactoryGet_FBC_ELEC_MODE();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_BACKLIGHT_N360: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_BACKLIGHT_N360(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_BACKLIGHT_N360: {
        int ret = mpTv->Tv_FactoryGet_FBC_BACKLIGHT_N360();
        r->writeInt32(ret);
        break;
    }


    case FACTORY_FBC_SET_PIC_MODE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Picture_Mode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_PIC_MODE: {
        int ret = mpTv->Tv_FactoryGet_FBC_Picture_Mode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_TEST_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Set_Test_Pattern(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_TEST_PATTERN: {
        int ret = mpTv->Tv_FactoryGet_FBC_Get_Test_Pattern();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_RED: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Gain_Red(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_RED: {
        int ret = mpTv->Tv_FactoryGet_FBC_Gain_Red();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Gain_Green(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_GREEN: {
        int ret = mpTv->Tv_FactoryGet_FBC_Gain_Green();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_GAIN_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Gain_Blue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_GAIN_BLUE: {
        int ret = mpTv->Tv_FactoryGet_FBC_Gain_Blue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_RED: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Offset_Red(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_RED: {
        int ret = mpTv->Tv_FactoryGet_FBC_Offset_Red();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_GREEN: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Offset_Green(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_GREEN: {
        int ret = mpTv->Tv_FactoryGet_FBC_Offset_Green();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_OFFSET_BLUE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_Offset_Blue(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_OFFSET_BLUE: {
        int ret = mpTv->Tv_FactoryGet_FBC_Offset_Blue();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_COLORTEMP_MODE: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_ColorTemp_Mode(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_COLORTEMP_MODE: {
        int ret = mpTv->Tv_FactoryGet_FBC_ColorTemp_Mode();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_COLORTEMP_MODE_N360: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_ColorTemp_Mode_N360(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_COLORTEMP_MODE_N360: {
        int ret = mpTv->Tv_FactoryGet_FBC_ColorTemp_Mode_N360();
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_SET_WB_INIT: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySet_FBC_WB_Initial(value);
        r->writeInt32(ret);
        break;
    }

    case FACTORY_FBC_GET_WB_INIT: {
        int ret = mpTv->Tv_FactoryGet_FBC_WB_Initial();
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
    case FACTORY_FBC_VIDEO_MUTE: {
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_Set_VMute(COMM_DEV_SERIAL, 1);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_POWER_REBOOT: {
        int value = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_SendRebootToUpgradeCmd(COMM_DEV_SERIAL, value);
            r->writeInt32(ret);
        } else {
            r->writeInt32(-1);
        }
        break;
    }
    case FACTORY_FBC_SEND_KEY_TO_FBC: {
        int keyCode = p.readInt32();
        int param = p.readInt32();
        CFbcCommunication *fbcIns = GetSingletonFBC();
        if (fbcIns != NULL) {
            int ret = fbcIns->cfbc_FBC_Send_Key_To_Fbc(COMM_DEV_SERIAL, keyCode, param);
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
        int ret = mpTv->Tv_FactorySetWhiteBalanceRedGain(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceRedGain(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceGreenGain(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceGreenGain(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GAIN_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceBlueGain(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GAIN_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceBlueGain(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceRedOffset(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_RED: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceRedOffset(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceGreenOffset(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_GREEN: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceGreenOffset(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_OFFSET_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceBlueOffset(source_type, colortemp_mode, value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_OFFSET_BLUE: {
        int source_type = p.readInt32();
        int colortemp_mode = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceBlueOffset(source_type, colortemp_mode);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_COLOR_TMP: {
        int source_type = p.readInt32();
        int ret = mpTv->Tv_FactoryGetWhiteBalanceColorTempMode(source_type);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_COLOR_TMP: {
        int source_type = p.readInt32();
        int Tempmode = p.readInt32();
        int is_save = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceColorTempMode(source_type, Tempmode, is_save);
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
        int ret = mpTv->Tv_FactorySaveWhiteBalancePramas(source_type, mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_OPEN_GRAY_PATTERN: {
        int ret = mpTv->Tv_FactoryOpenWhiteBalanceGrayPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_CLOSE_GRAY_PATTERN: {
        int ret = mpTv->Tv_FactoryCloseWhiteBalanceGrayPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_SET_GRAY_PATTERN: {
        int value = p.readInt32();
        int ret = mpTv->Tv_FactorySetWhiteBalanceGrayPattern(value);
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_GRAY_PATTERN: {
        int ret = mpTv->Tv_FactoryGetWhiteBalanceGrayPattern();
        r->writeInt32(ret);
        break;
    }
    case FACTORY_WHITE_BALANCE_GET_ALL_PRAMAS: {
        int Tempmode = p.readInt32();
        tcon_rgb_ogo_t params;
        int ret  = mpTv->GetColorTemperatureParams((vpp_color_temperature_mode_t)Tempmode, &params);
        r->writeInt32(ret);
        r->writeInt32(params.r_gain);
        r->writeInt32(params.g_gain);
        r->writeInt32(params.b_gain);
        r->writeInt32(params.r_post_offset);
        r->writeInt32(params.g_post_offset);
        r->writeInt32(params.b_post_offset);
    }
    case STOP_SCAN: {
        mpTv->stopScanLock();
        break;
    }
    case DTV_GET_SNR: {
        int tmpRet = 0;
        tmpRet = mpTv->getFrontendSNR();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_BER: {
        int tmpRet = 0;
        tmpRet = mpTv->getFrontendBER();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_STRENGTH: {
        int tmpRet = 0;
        tmpRet = mpTv->getFrontendSignalStrength();
        r->writeInt32(tmpRet);
        break;
    }
    case DTV_GET_AUDIO_TRACK_NUM: {
        int programId = p.readInt32();
        int retCnt = 0;
        retCnt = mpTv->getAudioTrackNum(programId);
        r->writeInt32(retCnt);
        break;
    }
    case DTV_GET_AUDIO_TRACK_INFO: {
        int progId = p.readInt32();
        int aIdx = p.readInt32();
        int aFmt = -1;
        int iRet = -1;
        String8 lang;
        iRet = mpTv->getAudioInfoByIndex(progId, aIdx, &aFmt, lang);
        r->writeInt32(aFmt);
        r->writeString16(String16(lang));
        break;
    }
    case DTV_SWITCH_AUDIO_TRACK: {
        int aPid = p.readInt32();
        int aFmt = p.readInt32();
        int aParam = p.readInt32();
        int ret = -1;
        ret = mpTv->switchAudioTrack(aPid, aFmt, aParam);
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
        int currChannelMod;
        currChannelMod = mpTv->getAudioChannel();
        r->writeInt32(currChannelMod);
        break;
    }
    case DTV_GET_CUR_FREQ: {
        int progId = p.readInt32();
        int freq = 0;
        int iRet = -1;
        CTvProgram prog;
        CTvChannel channel;

        iRet = CTvProgram::selectByID(progId, prog);
        if (0 != iRet) return -1;
        prog.getChannel(channel);
        freq = channel.getFrequency();
        r->writeInt32(freq);
        break;
    }
    case DTV_GET_EPG_UTC_TIME: {
        int iRet = -1;
        int utcTime = 0;

        utcTime = mpTv->getTvTime();

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
        int iRet  = -1;
        int iObOutSize = 0;
        Vector<sp<CTvEvent> > epgOut;
        int progid = p.readInt32();
        int iUtcStartTime = p.readInt32();
        int iDurationTime = p.readInt32();
        CTvProgram prog;
        CTvEvent ev;
        iRet = CTvProgram::selectByID(progid, prog);
        if (0 != iRet) {
            break;
        }
        iRet = ev.getProgScheduleEvents(prog.getSrc(), prog.getID(), iUtcStartTime, iDurationTime, epgOut);
        if (0 != iRet) {
            break;
        }
        iObOutSize = epgOut.size();
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
        int i;
        int size = mpTv->getAutoBacklightData(tmp_i_buf);
        r->writeInt32(size);
        for (i = 0; i < size; i++) {
            r->writeInt32(tmp_i_buf[i]);
        }
        break;
    }
    case SET_AUTO_BACKLIGHT_DATA: {
        String16 tmp_str16;
        String8 value_str;

        tmp_str16 = p.readString16();
        value_str = String8(tmp_str16);

        int ret = mpTv->setAutobacklightData(value_str);
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
    case DTV_SET_BOOKING_FLAG: {
        CTvEvent ev;
        int iEvtId = p.readInt32();
        bool iBookFlag = (bool)p.readInt32();
        ev.bookEvent(iEvtId, iBookFlag);
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
        char buf[256];
        String16 tmpName = p.readString16();
        String8 strName = String8(tmpName);
        sprintf(buf, "%s", strName.string());
        mpTv->SetRecordFileName(buf);
        mpTv->StartToRecord();
    }
    break;
    case DTV_STOP_RECORD:
        mpTv->StopRecording();
        break;
    case DTV_SET_RECORD_ALL_TS: {
        int sel = p.readInt32();
        mpTv->SetRecCurTsOrCurProgram(sel);
    }
    break;
    case TV_PRINT_DEBUG_INFO:
        mpTv->printDebugInfo();
        break;
    case HDMIAV_HOTPLUGDETECT_ONOFF: {
        int flag = mpTv->GetHdmiAvHotplugDetectOnoff();
        r->writeInt32(flag);
    }
    break;

    // 2.4G headset
    case START_OPEN_HEADSET_DETECT: {
        int flag = mpTv->StartHeadSetDetect();
        r->writeInt32(flag);
    }
    break;

    // EXTAR END
    default:
        LOGD("default");
        break;
    }

    LOGD("exit client=%d cmd=%d", getCallingPid(), cmd);
    return 0;
}

sp<TvService::Client> TvService::Client::getClientFromCookie(void *user)
{
    sp<Client> client = 0;
    /*
       TvService *service = static_cast<TvService*> (user);
       if (service != NULL) {
           Mutex::Autolock ourLock(service->mServiceLock);
           if (service->mClient != 0) {
               client = service->mClient.promote();
               if (client == 0) {
                   LOGE("getClientFromCookie: client appears to have died");
                   service->mClient.clear();
               }
           } else {
               LOGE("getClientFromCookie: got callback but client was NULL");
           }
       }*/

    user = user;//for warning
    return client;
}

void TvService::Client::notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2, void *user)
{
    LOGD("notifyCallback(%d)", msgType);

    sp<Client> client = getClientFromCookie(user);
    if (client == 0) {
        return;
    }

    switch (msgType) {
    //case TV_MSG_SHUTTER:
    // ext1 is the dimension of the yuv picture.
    //     client->handleShutter((image_rect_type *)ext1);
    //    break;
    default:
        sp<ITvClient> c = client->mTvClient;
        if (c != NULL) {
            //c->notifyCallback(msgType, ext1, ext2);
        }
        break;
    }

    //for warning
    ext1 = ext1;
    ext2 = ext2;

#if DEBUG_CLIENT_REFERENCES
    if (client->getStrongCount() == 1) {
        LOGE("++++++++++++++++ (NOTIFY CALLBACK) THIS WILL CAUSE A LOCKUP!");
        client->printRefs();
    }
#endif
}

int TvService::Client::notifyCallback(const int &msgtype, const Parcel &p)
{
    mTvClient->notifyCallback(msgtype, p);
    return 0;
}

status_t TvService::onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    // permission checks...
    switch (code) {
    case BnTvService::CONNECT:
        IPCThreadState *ipc = IPCThreadState::self();
        const int pid = ipc->getCallingPid();
        const int self_pid = getpid();
        if (pid != self_pid) {
            // we're called from a different process, do the real check
            /*if (!checkCallingPermission(
             String16("android.permission.TV")))
             {
             const int uid = ipc->getCallingUid();
             LOGE("Permission Denial: "
             "can't use the tv pid=%d, uid=%d", pid, uid);
             return PERMISSION_DENIED;
             }*/
        }
        break;
    }

    status_t err = BnTvService::onTransact(code, data, reply, flags);

#if DEBUG_HEAP_LEAKS
    LOGE("+++ onTransact err %d code %d", err, code);

    if (err == UNKNOWN_TRANSACTION || err == PERMISSION_DENIED) {
        LOGE("+++ onTransact code %d", code);

        CHECK_INTERFACE(ITvService, data, reply);

        switch (code) {
        case 1000: {
            if (gWeakHeap != 0) {
                sp<IMemoryHeap> h = gWeakHeap.promote();
                IMemoryHeap *p = gWeakHeap.unsafe_get();
                LOGE("CHECKING WEAK REFERENCE %p (%p)", h.get(), p);
                if (h != 0)
                    h->printRefs();
                bool attempt_to_delete = data.readInt32() == 1;
                if (attempt_to_delete) {
                    // NOT SAFE!
                    LOGE("DELETING WEAK REFERENCE %p (%p)", h.get(), p);
                    if (p) delete p;
                }
                return NO_ERROR;
            }
        }
        break;
        default:
            break;
        }
    }
#endif // DEBUG_HEAP_LEAKS
    return err;
}


/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2018/1/16
 *  @par function description:
 *  - 1 droidlogic tvservice interface
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "HIDLIntf"

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <utils/Errors.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <ITvService.h>
#include <hardware/hardware.h>
#include "DroidTvServiceIntf.h"
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <CTvLog.h>
#include <tvconfig.h>
#include <tvutils.h>
#include <tvsetting/CTvSetting.h>
#include <tv/CTvFactory.h>
#include <audio/CTvAudio.h>
#include <version/version.h>
#include "tvcmd.h"
#include <tvdb/CTvRegion.h>
#include "MemoryLeakTrackUtil.h"
extern "C" {
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#ifdef SUPPORT_ADTV
#include "am_ver.h"
#endif
}

#include "DroidTvServiceIntf.h"

using namespace android;

DroidTvServiceIntf::DroidTvServiceIntf()
{
    //mpScannerClient = NULL;
    mpTv = new CTv();
    mpTv->setTvObserver(this);
    mpTv->OpenTv();
}

DroidTvServiceIntf::~DroidTvServiceIntf()
{
    //mpScannerClient = NULL;
    /*
    for (int i = 0; i < (int)mClients.size(); i++) {
        wp<Client> client = mClients[i];
        if (client != 0) {
            LOGW("some client still connect it!");
        }
    }*/

    if (mpTv != NULL) {
        delete mpTv;
        mpTv = NULL;
    }
}

void DroidTvServiceIntf::startListener() {
    mpTv->startTvDetect();
}

void DroidTvServiceIntf::onTvEvent(const CTvEv &ev)
{
    int type = ev.getEvType();
    LOGD("DroidTvServiceIntf::onTvEvent ev type = %d", type);
    switch (type) {
    case CTvEv::TV_EVENT_SCANNER: {
        CTvScanner::ScannerEvent *pScannerEv = (CTvScanner::ScannerEvent *) (&ev);
        //if (mpScannerClient != NULL) {
            //sp<Client> ScannerClient = mpScannerClient.promote();
            //if (ScannerClient != 0) {
                Parcel p;
                LOGD("scanner evt type:%d freq:%d vid:%d acnt:%d scnt:%d",
                     pScannerEv->mType, pScannerEv->mFrequency, pScannerEv->mVid, pScannerEv->mAcnt, pScannerEv->mScnt);
                p.writeInt32(pScannerEv->mType);
                p.writeInt32(pScannerEv->mPercent);
                p.writeInt32(pScannerEv->mTotalChannelCount);
                p.writeInt32(pScannerEv->mLockedStatus);
                p.writeInt32(pScannerEv->mChannelIndex);
                p.writeInt32(pScannerEv->mFrequency);
                p.writeString16(String16(pScannerEv->mProgramName));
                p.writeInt32(pScannerEv->mprogramType);
                p.writeString16(String16(pScannerEv->mParas));
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
                p.writeInt32(pScannerEv->mReserved);
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
                for (int i = 0; i < pScannerEv->mAcnt; i++)
                    p.writeInt32(pScannerEv->mAExt[i]);
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
                p.writeInt32(pScannerEv->mMajorChannelNumber);
                p.writeInt32(pScannerEv->mMinorChannelNumber);
                p.writeInt32(pScannerEv->mSourceId);
                p.writeInt32(pScannerEv->mAccessControlled);
                p.writeInt32(pScannerEv->mHidden);
                p.writeInt32(pScannerEv->mHideGuide);
                p.writeString16(String16(pScannerEv->mVct));

                //mNotifyListener->onEvent(SCAN_EVENT_CALLBACK, p);
            //}
        //}
        break;
    }

    case CTvEv::TV_EVENT_EPG: {
        CTvEpg::EpgEvent *pEpgEvent = (CTvEpg::EpgEvent *) (&ev);
        Parcel p;
        p.writeInt32(pEpgEvent->type);
        p.writeInt32(pEpgEvent->time);
        p.writeInt32(pEpgEvent->programID);
        p.writeInt32(pEpgEvent->channelID);
        //mNotifyListener->onEvent(EPG_EVENT_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_HDMI_IN_CAP: {
        /*
        TvHidlParcel hidlParcel;

        CTvScreenCapture::CapEvent *pCapEvt = (CTvScreenCapture::CapEvent *)(&ev);

        hidlParcel.msgType = VFRAME_BMP_EVENT_CALLBACK;

        ALOGI("TV_EVENT_HDMI_IN_CAP size:%d", sizeof(CTvScreenCapture::CapEvent));

        hidlParcel.bodyInt.resize(4);
        hidlParcel.bodyInt[0] = pCapEvt->mFrameNum;
        hidlParcel.bodyInt[1] = pCapEvt->mFrameSize;
        hidlParcel.bodyInt[2] = pCapEvt->mFrameWide;
        hidlParcel.bodyInt[3] = pCapEvt->mFrameHeight;

        mNotifyListener->onEvent(hidlParcel);
        */
        break;
    }

    case CTvEv::TV_EVENT_AV_PLAYBACK: {
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = DTV_AV_PLAYBACK_CALLBACK;

        TvEvent::AVPlaybackEvent *pEv = (TvEvent::AVPlaybackEvent *)(&ev);

        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->mMsgType;
        hidlParcel.bodyInt[1] = pEv->mProgramId;

        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_SIGLE_DETECT: {
        TvEvent::SignalInfoEvent *pEv = (TvEvent::SignalInfoEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mTrans_fmt);
        p.writeInt32(pEv->mFmt);
        p.writeInt32(pEv->mStatus);
        p.writeInt32(pEv->mReserved);
        //mNotifyListener->onEvent(SIGLE_DETECT_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_SUBTITLE: {
        TvEvent::SubtitleEvent *pEv = (TvEvent::SubtitleEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->pic_width);
        p.writeInt32(pEv->pic_height);
        //mNotifyListener->onEvent(SUBTITLE_UPDATE_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_ADC_CALIBRATION: {
        TvEvent::ADCCalibrationEvent *pEv = (TvEvent::ADCCalibrationEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mState);
        //mNotifyListener->onEvent(ADC_CALIBRATION_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_VGA: {//VGA
        TvEvent::VGAEvent *pEv = (TvEvent::VGAEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mState);
        //mNotifyListener->onEvent(VGA_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_SOURCE_CONNECT: {
        TvHidlParcel hidlParcel;
        hidlParcel.msgType = SOURCE_CONNECT_CALLBACK;

        TvEvent::SourceConnectEvent *pEv = (TvEvent::SourceConnectEvent *)(&ev);

        hidlParcel.bodyInt.resize(2);
        hidlParcel.bodyInt[0] = pEv->mSourceInput;
        hidlParcel.bodyInt[1] = pEv->connectionState;

        LOGD("source connect input: =%d state: =%d", pEv->mSourceInput, pEv->connectionState);
        mNotifyListener->onEvent(hidlParcel);
        break;
    }

    case CTvEv::TV_EVENT_HDMIRX_CEC: {
        TvEvent::HDMIRxCECEvent *pEv = (TvEvent::HDMIRxCECEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mDataCount);
        for (int j = 0; j < pEv->mDataCount; j++) {
            p.writeInt32(pEv->mDataBuf[j]);
        }
        //mNotifyListener->onEvent(HDMIRX_CEC_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_UPGRADE_FBC: {
        TvEvent::UpgradeFBCEvent *pEv = (TvEvent::UpgradeFBCEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mState);
        p.writeInt32(pEv->param);
        //mNotifyListener->onEvent(UPGRADE_FBC_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_SERIAL_COMMUNICATION: {
        TvEvent::SerialCommunicationEvent *pEv = (TvEvent::SerialCommunicationEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mDevId);
        p.writeInt32(pEv->mDataCount);
        for (int j = 0; j < pEv->mDataCount; j++) {
            p.writeInt32(pEv->mDataBuf[j]);
        }
        //mNotifyListener->onEvent(SERIAL_COMMUNICATION_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_2d4G_HEADSET: {
        TvEvent::HeadSetOf2d4GEvent *pEv = (TvEvent::HeadSetOf2d4GEvent *)(&ev);
        LOGD("SendDtvStats status: =%d para2: =%d", pEv->state, pEv->para);
        Parcel p;
        p.writeInt32(pEv->state);
        p.writeInt32(pEv->para);
        //mNotifyListener->onEvent(HEADSET_STATUS_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_SCANNING_FRAME_STABLE: {
        TvEvent::ScanningFrameStableEvent *pEv = (TvEvent::ScanningFrameStableEvent *)(&ev);
        LOGD("Scanning Frame is stable!");
        Parcel p;
        p.writeInt32(pEv->CurScanningFreq);
        //mNotifyListener->onEvent(SCANNING_FRAME_STABLE_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_FRONTEND: {
        TvEvent::FrontendEvent *pEv = (TvEvent::FrontendEvent *)(&ev);
        Parcel p;
        p.writeInt32(pEv->mStatus);
        p.writeInt32(pEv->mFrequency);
        p.writeInt32(pEv->mParam1);
        p.writeInt32(pEv->mParam2);
        p.writeInt32(pEv->mParam3);
        p.writeInt32(pEv->mParam4);
        p.writeInt32(pEv->mParam5);
        p.writeInt32(pEv->mParam6);
        p.writeInt32(pEv->mParam7);
        p.writeInt32(pEv->mParam8);
        //mNotifyListener->onEvent(FRONTEND_EVENT_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_RECORDER: {
        TvEvent::RecorderEvent *pEv = (TvEvent::RecorderEvent *)(&ev);
        Parcel p;
        p.writeString16(String16(pEv->mId));
        p.writeInt32(pEv->mStatus);
        p.writeInt32(pEv->mError);
        //mNotifyListener->onEvent(RECORDER_EVENT_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_RRT: {
        CTvRrt::RrtEvent *pRrtEvent = (CTvRrt::RrtEvent *) (&ev);
        Parcel p;
        p.writeInt32(pRrtEvent->satus);
        //mNotifyListener->onEvent(RRT_EVENT_CALLBACK, p);
        break;
    }

    case CTvEv::TV_EVENT_EAS: {
        CTvEas::EasEvent *pEasEvent = (CTvEas::EasEvent *) (&ev);
        Parcel p;
        p.writeInt32(pEasEvent->eas_section_count);
        p.writeInt32(pEasEvent->table_id);
        p.writeInt32(pEasEvent->extension);
        p.writeInt32(pEasEvent->version);
        p.writeInt32(pEasEvent->current_next);
        p.writeInt32(pEasEvent->sequence_num);
        p.writeInt32(pEasEvent->protocol_version);
        p.writeInt32(pEasEvent->eas_event_id);
        p.writeInt32(pEasEvent->eas_orig_code[0]);
        p.writeInt32(pEasEvent->eas_orig_code[1]);
        p.writeInt32(pEasEvent->eas_orig_code[2]);
        p.writeInt32(pEasEvent->eas_event_code_len);
        for (int j=0;j<pEasEvent->eas_event_code_len;j++) {
            p.writeInt32(pEasEvent->eas_event_code[j]);
        }
        p.writeInt32(pEasEvent->alert_message_time_remaining);
        p.writeInt32(pEasEvent->event_start_time);
        p.writeInt32(pEasEvent->event_duration);
        p.writeInt32(pEasEvent->alert_priority);
        p.writeInt32(pEasEvent->details_OOB_source_ID);
        p.writeInt32(pEasEvent->details_major_channel_number);
        p.writeInt32(pEasEvent->details_minor_channel_number);
        p.writeInt32(pEasEvent->audio_OOB_source_ID);
        p.writeInt32(pEasEvent->location_count);
        for (int j=0;j<pEasEvent->location_count;j++) {
            p.writeInt32(pEasEvent->location[j].i_state_code);
            p.writeInt32(pEasEvent->location[j].i_country_subdiv);
            p.writeInt32(pEasEvent->location[j].i_country_code);
        }
        p.writeInt32(pEasEvent->exception_count);
        for (int j=0;j<pEasEvent->exception_count;j++) {
            p.writeInt32(pEasEvent->exception[j].i_in_band_refer);
            p.writeInt32(pEasEvent->exception[j].i_exception_major_channel_number);
            p.writeInt32(pEasEvent->exception[j].i_exception_minor_channel_number);
            p.writeInt32(pEasEvent->exception[j].exception_OOB_source_ID);
        }
        p.writeInt32(pEasEvent->multi_text_count);
        for (int j=0;j<pEasEvent->multi_text_count;j++) {
            p.writeInt32(pEasEvent->multi_text[j].lang[0]);
            p.writeInt32(pEasEvent->multi_text[j].lang[1]);
            p.writeInt32(pEasEvent->multi_text[j].lang[2]);
            p.writeInt32(pEasEvent->multi_text[j].i_type);
            p.writeInt32(pEasEvent->multi_text[j].i_compression_type);
            p.writeInt32(pEasEvent->multi_text[j].i_mode);
            p.writeInt32(pEasEvent->multi_text[j].i_number_bytes);
            for (int k=0;k<pEasEvent->multi_text[j].i_number_bytes;k++) {
                p.writeInt32(pEasEvent->multi_text[j].compressed_str[k]);
            }
        }
        p.writeInt32(pEasEvent->descriptor_text_count);
        for (int j=0;j<pEasEvent->descriptor_text_count;j++) {
            p.writeInt32(pEasEvent->descriptor[j].i_tag);
            p.writeInt32(pEasEvent->descriptor[j].i_length);
            for (int k=0;k<pEasEvent->descriptor[j].i_length;k++) {
                p.writeInt32(pEasEvent->descriptor[j].p_data[k]);
            }
        }
        //mNotifyListener->onEvent(EAS_EVENT_CALLBACK, p);
        break;
    }

    default:
        break;
    }
}

int DroidTvServiceIntf::startTv() {
    return mpTv->StartTvLock();
}

int DroidTvServiceIntf::stopTv() {
    return mpTv->StopTvLock();
}

int DroidTvServiceIntf::switchInputSrc(int32_t inputSrc) {
    LOGD("switchInputSrc sourceId= 0x%x", inputSrc);
    return mpTv->SetSourceSwitchInput((tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::getInputSrcConnectStatus(int32_t inputSrc) {
    return mpTv->GetSourceConnectStatus((tv_source_input_t)inputSrc);
}

int DroidTvServiceIntf::getCurrentInputSrc() {
    return (int)mpTv->GetCurrentSourceInputVirtualLock();
}

int DroidTvServiceIntf::getHdmiAvHotplugStatus() {
    return mpTv->GetHdmiAvHotplugDetectOnoff();
}

std::string DroidTvServiceIntf::getSupportInputDevices() {
    const char *valueStr = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
    LOGD("get support input devices = %s", valueStr);
    return std::string(valueStr);
}

int DroidTvServiceIntf::getHdmiPorts() {
    return config_get_int(CFG_SECTION_TV, CGF_DEFAULT_HDMI_PORTS, 0);
}

void DroidTvServiceIntf::getCurSignalInfo(int &fmt, int &transFmt, int &status, int &frameRate) {
    tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
    int frame_rate = mpTv->getHDMIFrameRate();

    fmt = siginfo.fmt;
    transFmt = siginfo.trans_fmt;
    status = siginfo.status;
    frameRate = frame_rate;
}

int DroidTvServiceIntf::setMiscCfg(const std::string& key, const std::string& val) {
    //LOGD("setMiscCfg key = %s, val = %s", key.c_str(), val.c_str());
    return config_set_str(CFG_SECTION_TV, key.c_str(), val.c_str());
}

std::string DroidTvServiceIntf::getMiscCfg(const std::string& key, const std::string& def) {
    const char *value = config_get_str(CFG_SECTION_TV, key.c_str(), def.c_str());
    //LOGD("getMiscCfg key = %s, def = %s, value = %s", key.c_str(), def.c_str(), value);
    return std::string(value);
}

int DroidTvServiceIntf::isDviSIgnal() {
    return mpTv->IsDVISignal();
}

int DroidTvServiceIntf::isVgaTimingInHdmi() {
    return mpTv->isVgaFmtInHdmi();
}

int DroidTvServiceIntf::processCmd(const Parcel &p) {
    unsigned char dataBuf[512] = {0};
    int ret = -1;
    int cmd = p.readInt32();

    LOGD("processCmd cmd=%d", cmd);
    switch (cmd) {
        // Tv function
        case OPEN_TV:
            break;

        case CLOSE_TV:
            ret = mpTv->CloseTv();
            break;

        case START_TV:
            //int mode = p.readInt32();
            ret = mpTv->StartTvLock();
            mIsStartTv = true;
            break;

        case STOP_TV:
            ret = mpTv->StopTvLock();
            mIsStartTv = false;
            break;
        case GET_TV_STATUS:
            ret = (int)mpTv->GetTvStatus();
            break;
        case GET_LAST_SOURCE_INPUT:
            ret = (int)mpTv->GetLastSourceInput();
            break;
        case GET_CURRENT_SOURCE_INPUT:
            ret = (int)mpTv->GetCurrentSourceInputLock();
            break;

        case GET_CURRENT_SOURCE_INPUT_VIRTUAL:
            ret = (int)mpTv->GetCurrentSourceInputVirtualLock();
            break;

        case GET_CURRENT_SIGNAL_INFO:
            /*
            tvin_info_t siginfo = mpTv->GetCurrentSignalInfo();
            int frame_rate = mpTv->getHDMIFrameRate();
            r->writeInt32(siginfo.trans_fmt);
            r->writeInt32(siginfo.fmt);
            r->writeInt32(siginfo.status);
            r->writeInt32(frame_rate);
            */
            break;

        case SET_SOURCE_INPUT: {
            int sourceinput = p.readInt32();
            LOGD(" SetSourceInput sourceId= %x", sourceinput);
            ret = mpTv->SetSourceSwitchInput((tv_source_input_t)sourceinput);
            break;
        }
        case SET_SOURCE_INPUT_EXT: {
            int sourceinput = p.readInt32();
            int vsourceinput = p.readInt32();
            LOGD(" SetSourceInputExt vsourceId= %d sourceId=%d", vsourceinput, sourceinput);
            ret = mpTv->SetSourceSwitchInput((tv_source_input_t)vsourceinput, (tv_source_input_t)sourceinput);
            break;
        }
        case DO_SUSPEND: {
            int type = p.readInt32();
            ret = mpTv->DoSuspend(type);
            break;
        }
        case DO_RESUME: {
            int type = p.readInt32();
            ret = mpTv->DoResume(type);
            break;
        }
        case IS_DVI_SIGNAL:
            ret = mpTv->IsDVISignal();
            break;

        case IS_VGA_TIMEING_IN_HDMI:
            ret = mpTv->isVgaFmtInHdmi();
            break;

        case SET_PREVIEW_WINDOW_MODE:
            ret = mpTv->setPreviewWindowMode(p.readInt32() == 1);
            break;

        case SET_PREVIEW_WINDOW: {
            tvin_window_pos_t win_pos;
            win_pos.x1 = p.readInt32();
            win_pos.y1 = p.readInt32();
            win_pos.x2 = p.readInt32();
            win_pos.y2 = p.readInt32();
            ret = (int)mpTv->SetPreviewWindow(win_pos);
            break;
        }

        case GET_SOURCE_CONNECT_STATUS: {
            int source_input = p.readInt32();
            ret = mpTv->GetSourceConnectStatus((tv_source_input_t)source_input);
            break;
        }

        case GET_SOURCE_INPUT_LIST:
            /*
            const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
            r->writeString16(String16(value));
            */
            break;

        //Tv function END

        // HDMI
        case SET_HDMI_EDID_VER: {
            int hdmi_port_id = p.readInt32();
            int edid_ver = p.readInt32();
            ret = mpTv->SetHdmiEdidVersion((tv_hdmi_port_id_t)hdmi_port_id, (tv_hdmi_edid_version_t)edid_ver);
            break;
        }
        case SET_HDCP_KEY_ENABLE: {
            int enable = p.readInt32();
            ret = mpTv->SetHdmiHDCPSwitcher((tv_hdmi_hdcpkey_enable_t)enable);
            break;
        }
        case SET_HDMI_COLOR_RANGE_MODE: {
            int range_mode = p.readInt32();
            ret = mpTv->SetHdmiColorRangeMode((tv_hdmi_color_range_t)range_mode);
            break;
        }
        case GET_HDMI_COLOR_RANGE_MODE:
            ret = mpTv->GetHdmiColorRangeMode();
            break;

        // HDMI END

        // AUDIO & AUDIO MUTE
        case SET_AUDIO_MUTE_FOR_TV: {
            int status = p.readInt32();
            if (status != mpTv->GetAudioMuteForTv()) {
                ret = mpTv->SetAudioMuteForTv(status);
            }
            break;
        }
        case SET_AUDIO_MUTEKEY_STATUS: {
            int status = p.readInt32();
            ret = mpTv->SetAudioMuteForSystem(status);
            break;
        }
        case GET_AUDIO_MUTEKEY_STATUS:
            ret = mpTv->GetAudioMuteForSystem();
            break;

        case SET_AUDIO_AVOUT_MUTE_STATUS: {
            int status = p.readInt32();
            ret = mpTv->SetAudioAVOutMute(status);
            break;
        }
        case GET_AUDIO_AVOUT_MUTE_STATUS:
            ret = mpTv->GetAudioAVOutMute();
            break;

        case SET_AUDIO_SPDIF_MUTE_STATUS: {
            int status = p.readInt32();
            ret = mpTv->SetAudioSPDIFMute(status);
            break;
        }
        case GET_AUDIO_SPDIF_MUTE_STATUS:
            ret = mpTv->GetAudioSPDIFMute();
            break;

        // AUDIO MASTER VOLUME
        case SET_AUDIO_MASTER_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioMasterVolume(vol);
            break;
        }
        case GET_AUDIO_MASTER_VOLUME:
            ret = mpTv->GetAudioMasterVolume();
            break;
        case SAVE_CUR_AUDIO_MASTER_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioMasterVolume(vol);
            break;
        }
        case GET_CUR_AUDIO_MASTER_VOLUME:
            ret = mpTv->GetCurAudioMasterVolume();
            break;
        //AUDIO BALANCE
        case SET_AUDIO_BALANCE: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioBalance(vol);
            break;
        }
        case GET_AUDIO_BALANCE:
            ret = mpTv->GetAudioBalance();
            break;

        case SAVE_CUR_AUDIO_BALANCE: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioBalance(vol);
            break;
        }
        case GET_CUR_AUDIO_BALANCE:
            ret = mpTv->GetCurAudioBalance();
            break;

        //AUDIO SUPPERBASS VOLUME
        case SET_AUDIO_SUPPER_BASS_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSupperBassVolume(vol);
            break;
        }
        case GET_AUDIO_SUPPER_BASS_VOLUME:
            ret = mpTv->GetAudioSupperBassVolume();
            break;

        case SAVE_CUR_AUDIO_SUPPER_BASS_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSupperBassVolume(vol);
            break;
        }
        case GET_CUR_AUDIO_SUPPER_BASS_VOLUME:
            ret = mpTv->GetCurAudioSupperBassVolume();
            break;

        //AUDIO SUPPERBASS SWITCH
        case SET_AUDIO_SUPPER_BASS_SWITCH: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSupperBassSwitch(vol);
            break;
        }
        case GET_AUDIO_SUPPER_BASS_SWITCH:
            ret = mpTv->GetAudioSupperBassSwitch();
            break;

        case SAVE_CUR_AUDIO_SUPPER_BASS_SWITCH: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSupperBassSwitch(vol);
            break;
        }
        case GET_CUR_AUDIO_SUPPER_BASS_SWITCH:
            ret = mpTv->GetCurAudioSupperBassSwitch();
            break;

        //AUDIO SRS SURROUND SWITCH
        case SET_AUDIO_SRS_SURROUND: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSRSSurround(vol);
            mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
            break;
        }
        case GET_AUDIO_SRS_SURROUND:
            ret = mpTv->GetAudioSRSSurround();
            break;

        case SAVE_CUR_AUDIO_SRS_SURROUND: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSrsSurround(vol);
            break;
        }
        case GET_CUR_AUDIO_SRS_SURROUND:
            ret = mpTv->GetCurAudioSRSSurround();
            break;

        //AUDIO SRS DIALOG CLARITY
        case SET_AUDIO_SRS_DIALOG_CLARITY: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSrsDialogClarity(vol);
            mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
            break;
        }
        case GET_AUDIO_SRS_DIALOG_CLARITY: {
            ret = mpTv->GetAudioSrsDialogClarity();
            break;
        }
        case SAVE_CUR_AUDIO_SRS_DIALOG_CLARITY: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSrsDialogClarity(vol);
            break;
        }
        case GET_CUR_AUDIO_SRS_DIALOG_CLARITY:
            ret = mpTv->GetCurAudioSrsDialogClarity();
            break;

        //AUDIO SRS TRUBASS
        case SET_AUDIO_SRS_TRU_BASS: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSrsTruBass(vol);
            mpTv->RefreshAudioMasterVolume(SOURCE_MAX);
            break;
        }
        case GET_AUDIO_SRS_TRU_BASS:
            ret = mpTv->GetAudioSrsTruBass();
            break;

        case SAVE_CUR_AUDIO_SRS_TRU_BASS: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSrsTruBass(vol);
            break;
        }
        case GET_CUR_AUDIO_SRS_TRU_BASS:
            ret = mpTv->GetCurAudioSrsTruBass();
            break;

        //AUDIO BASS
        case SET_AUDIO_BASS_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioBassVolume(vol);
            break;
        }
        case GET_AUDIO_BASS_VOLUME:
            ret = mpTv->GetAudioBassVolume();
            break;

        case SAVE_CUR_AUDIO_BASS_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioBassVolume(vol);
            break;
        }

        case GET_CUR_AUDIO_BASS_VOLUME:
            ret = mpTv->GetCurAudioBassVolume();
            break;

        //AUDIO TREBLE
        case SET_AUDIO_TREBLE_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioTrebleVolume(vol);
            break;
        }
        case GET_AUDIO_TREBLE_VOLUME:
            ret = mpTv->GetAudioTrebleVolume();
            break;

        case SAVE_CUR_AUDIO_TREBLE_VOLUME: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioTrebleVolume(vol);
            break;
        }
        case GET_CUR_AUDIO_TREBLE_VOLUME:
            ret = mpTv->GetCurAudioTrebleVolume();
            break;

        //AUDIO SOUND MODE
        case SET_AUDIO_SOUND_MODE: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioSoundMode(vol);
            break;
        }
        case GET_AUDIO_SOUND_MODE:
            ret = mpTv->GetAudioSoundMode();
            break;

        case SAVE_CUR_AUDIO_SOUND_MODE: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSoundMode(vol);
            break;
        }
        case GET_CUR_AUDIO_SOUND_MODE:
            ret = mpTv->GetCurAudioSoundMode();
            break;

        //AUDIO WALL EFFECT
        case SET_AUDIO_WALL_EFFECT: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioWallEffect(vol);
            break;
        }
        case GET_AUDIO_WALL_EFFECT:
            ret = mpTv->GetAudioWallEffect();
            break;

        case SAVE_CUR_AUDIO_WALL_EFFECT: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioWallEffect(vol);
            break;
        }
        case GET_CUR_AUDIO_WALL_EFFECT:
            ret = mpTv->GetCurAudioWallEffect();
            break;

        //AUDIO EQ MODE
        case SET_AUDIO_EQ_MODE: {
            int vol = p.readInt32();
            ret = mpTv->SetAudioEQMode(vol);
            break;
        }
        case GET_AUDIO_EQ_MODE:
            ret = mpTv->GetAudioEQMode();
            break;

        case SAVE_CUR_AUDIO_EQ_MODE: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioEQMode(vol);
            break;
        }
        case GET_CUR_AUDIO_EQ_MODE:
            ret = mpTv->GetCurAudioEQMode();
            break;

        //AUDIO EQ GAIN
        case GET_AUDIO_EQ_RANGE: {
            int buf[2];
            ret = mpTv->GetAudioEQRange(buf);
            //r->writeInt32(2);
            //r->writeInt32(buf[0]);
            //r->writeInt32(buf[1]);
            //r->writeInt32(ret);
            break;
        }
        case GET_AUDIO_EQ_BAND_COUNT:
            ret = mpTv->GetAudioEQBandCount();
            break;

        case SET_AUDIO_EQ_GAIN: {
            int buf[128] = {0};
            int bufSize = p.readInt32();
            for (int i = 0; i < bufSize; i++) {
                buf[i] = p.readInt32();
            }
            ret = mpTv->SetAudioEQGain(buf);
            break;
        }
        case GET_AUDIO_EQ_GAIN: {
            int buf[128] = {0};
            ret = mpTv->GetAudioEQGain(buf);
            int bufSize = mpTv->GetAudioEQBandCount();
            for (int i = 0; i < bufSize; i++) {
                //r->writeInt32(buf[i]);
            }
            //r->writeInt32(ret);
            break;
        }
        case SAVE_CUR_AUDIO_EQ_GAIN: {
            int buf[128] = {0};
            int bufSize = p.readInt32();
            for (int i = 0; i < bufSize; i++) {
                buf[i] = p.readInt32();
            }
            ret = mpTv->SaveCurAudioEQGain(buf);
            break;
        }
        case GET_CUR_EQ_GAIN: {
            int buf[128] = {0};
            ret = mpTv->GetCurAudioEQGain(buf);
            int bufSize = mpTv->GetAudioEQBandCount();
            //r->writeInt32(bufSize);
            for (int i = 0; i < bufSize; i++) {
                //r->writeInt32(buf[i]);
            }
            break;
        }
        case SET_AUDIO_EQ_SWITCH: {
            int tmpVal = p.readInt32();
            ret = mpTv->SetAudioEQSwitch(tmpVal);
            break;
        }
        // AUDIO SPDIF SWITCH
        case SET_AUDIO_SPDIF_SWITCH: {
            int tmp_val = p.readInt32();
            ret = mpTv->SetAudioSPDIFSwitch(tmp_val);
            break;
        }
        case SAVE_CUR_AUDIO_SPDIF_SWITCH: {
            int tmp_val = p.readInt32();
            ret = mpTv->SaveCurAudioSPDIFSwitch(tmp_val);
            break;
        }
        case GET_CUR_AUDIO_SPDIF_SWITCH:
            ret = mpTv->GetCurAudioSPDIFSwitch();
            break;

        //AUDIO SPDIF MODE
        case SET_AUDIO_SPDIF_MODE: {
            int vol = p.readInt32();
            int progId = p.readInt32();
            int audioTrackId = p.readInt32();
            ret = mpTv->SetAudioSPDIFMode(vol);
            mpTv->ResetAudioDecoderForPCMOutput();
            break;
        }
        case SAVE_CUR_AUDIO_SPDIF_MODE: {
            int vol = p.readInt32();
            ret = mpTv->SaveCurAudioSPDIFMode(vol);
            break;
        }
        case GET_CUR_AUDIO_SPDIF_MODE:
            ret = mpTv->GetCurAudioSPDIFMode();
            break;

        case SET_AMAUDIO_OUTPUT_MODE: {
            int tmp_val = p.readInt32();
            ret = mpTv->SetAmAudioOutputMode(tmp_val);
            break;
        }
        case SET_AMAUDIO_MUSIC_GAIN: {
            int tmp_val = p.readInt32();
            ret = mpTv->SetAmAudioMusicGain(tmp_val);
            break;
        }
        case SET_AMAUDIO_LEFT_GAIN: {
            int tmp_val = p.readInt32();
            ret = mpTv->SetAmAudioLeftGain(tmp_val);
            break;
        }
        case SET_AMAUDIO_RIGHT_GAIN: {
            int tmp_val = p.readInt32();
            ret = mpTv->SetAmAudioRightGain(tmp_val);
            break;
        }
        case SET_AMAUDIO_PRE_GAIN: {
            float tmp_val = p.readFloat();
            ret = mpTv->setAmAudioPreGain(tmp_val);
            break;
        }
        case SET_AMAUDIO_PRE_MUTE: {
            int tmp_val = p.readInt32();
            ret = mpTv->setAmAudioPreMute(tmp_val);
            break;
        }
        case GET_AMAUDIO_PRE_MUTE:
            ret = mpTv->getAmAudioPreMute();
            break;

        case SELECT_LINE_IN_CHANNEL: {
            int channel = p.readInt32();
            ret = mpTv->AudioLineInSelectChannel(channel);
            LOGD("SELECT_LINE_IN_CHANNEL: channel = %d; ret = %d.\n", channel, ret);
            break;
        }
        case SET_LINE_IN_CAPTURE_VOL: {
            int l_vol = p.readInt32();
            int r_vol = p.readInt32();
            ret = mpTv->AudioSetLineInCaptureVolume(l_vol, r_vol);
            break;
        }
        case SET_AUDIO_VOL_COMP: {
            int tmpVal = p.readInt32();
            ret = mpTv->SetCurProgramAudioVolumeCompensationVal(tmpVal);
            break;
        }
        case GET_AUDIO_VOL_COMP:
            ret = mpTv->GetAudioVolumeCompensationVal(-1);
            break;

        case SET_AUDIO_VIRTUAL: {
            int enable = p.readInt32();
            int level = p.readInt32();
            ret = mpTv->SetAudioVirtualizer(enable, level);
            break;
        }
        case GET_AUDIO_VIRTUAL_ENABLE:
            ret = mpTv->GetAudioVirtualizerEnable();
            break;

        case GET_AUDIO_VIRTUAL_LEVEL:
            ret = mpTv->GetAudioVirtualizerLevel();
            break;
        // AUDIO END

        // SSM
        case SSM_INIT_DEVICE:
            ret = mpTv->Tv_SSMRestoreDefaultSetting();//mpTv->Tv_SSMInitDevice();
            break;

        case SSM_SAVE_POWER_ON_OFF_CHANNEL: {
            int tmpPowerChanNum = p.readInt32();
            ret = SSMSavePowerOnOffChannel(tmpPowerChanNum);
            break;
        }
        case SSM_READ_POWER_ON_OFF_CHANNEL: {
            ret = SSMReadPowerOnOffChannel();
            break;
        }
        case SSM_SAVE_SOURCE_INPUT: {
            int tmpSouceInput = p.readInt32();
            ret = SSMSaveSourceInput(tmpSouceInput);
            break;
        }
        case SSM_READ_SOURCE_INPUT:
            ret = SSMReadSourceInput();
            break;

        case SSM_SAVE_LAST_SOURCE_INPUT: {
            int tmpLastSouceInput = p.readInt32();
            ret = SSMSaveLastSelectSourceInput(tmpLastSouceInput);
            break;
        }
        case SSM_READ_LAST_SOURCE_INPUT:
            ret = SSMReadLastSelectSourceInput();
            break;

        case SSM_SAVE_SYS_LANGUAGE: {
            int tmpVal = p.readInt32();
            ret = SSMSaveSystemLanguage(tmpVal);
            break;
        }
        case SSM_READ_SYS_LANGUAGE: {
            ret = SSMReadSystemLanguage();
            break;
        }
        case SSM_SAVE_AGING_MODE: {
            int tmpVal = p.readInt32();
            ret = SSMSaveAgingMode(tmpVal);
            break;
        }
        case SSM_READ_AGING_MODE:
            ret = SSMReadAgingMode();
            break;

        case SSM_SAVE_PANEL_TYPE: {
            int tmpVal = p.readInt32();
            ret = SSMSavePanelType(tmpVal);
            break;
        }
        case SSM_READ_PANEL_TYPE:
            ret = SSMReadPanelType();
            break;

        case SSM_SAVE_MAC_ADDR: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = KeyData_SaveMacAddress(dataBuf);
            break;
        }
        case SSM_READ_MAC_ADDR: {
            ret = KeyData_ReadMacAddress(dataBuf);
            int size = KeyData_GetMacAddressDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            //r->writeInt32(ret);
            break;
        }
        case SSM_SAVE_BAR_CODE: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = KeyData_SaveBarCode(dataBuf);
            break;
        }
        case SSM_READ_BAR_CODE: {
            ret = KeyData_ReadBarCode(dataBuf);
            int size = KeyData_GetBarCodeDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            break;
        }
        case SSM_SAVE_HDCPKEY: {
            int size = p.readInt32();
            for (int i = 0; i < size; i++) {
                dataBuf[i] = p.readInt32();
            }
            ret = SSMSaveHDCPKey(dataBuf);
            break;
        }
        case SSM_READ_HDCPKEY: {
            ret = SSMReadHDCPKey(dataBuf);
            int size = SSMGetHDCPKeyDataLen();
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(dataBuf[i]);
            }
            break;
        }
        case SSM_SAVE_POWER_ON_MUSIC_SWITCH: {
            int tmpVal = p.readInt32();
            ret = SSMSavePowerOnMusicSwitch(tmpVal);
            break;
        }
        case SSM_READ_POWER_ON_MUSIC_SWITCH:
            ret = SSMReadPowerOnMusicSwitch();
            break;

        case SSM_SAVE_POWER_ON_MUSIC_VOL: {
            int tmpVal = p.readInt32();
            ret = SSMSavePowerOnMusicVolume(tmpVal);
            break;
        }
        case SSM_READ_POWER_ON_MUSIC_VOL:
            ret = SSMReadPowerOnMusicVolume();
            break;
        case SSM_SAVE_SYS_SLEEP_TIMER: {
            int tmpVal = p.readInt32();
            ret = SSMSaveSystemSleepTimer(tmpVal);
            break;
        }
        case SSM_READ_SYS_SLEEP_TIMER:
            ret = SSMReadSystemSleepTimer();
            break;

        case SSM_SAVE_INPUT_SRC_PARENTAL_CTL: {
            int tmpSourceIndex = p.readInt32();
            int tmpCtlFlag = p.readInt32();
            ret = SSMSaveInputSourceParentalControl(tmpSourceIndex, tmpCtlFlag);
            break;
        }
        case SSM_READ_INPUT_SRC_PARENTAL_CTL: {
            int tmpSourceIndex = p.readInt32();
            ret = SSMReadInputSourceParentalControl(tmpSourceIndex);
            break;
        }
        case SSM_SAVE_PARENTAL_CTL_SWITCH: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveParentalControlSwitch(tmpSwitchFlag);
            break;
        }
        case SSM_READ_PARENTAL_CTL_SWITCH: {
            ret = SSMReadParentalControlSwitch();
            break;
        }
        case SSM_SAVE_PARENTAL_CTL_PASS_WORD: {
            String16 pass_wd_str = p.readString16();
            ret = SSMSaveParentalControlPassWord((unsigned char *)pass_wd_str.string(), pass_wd_str.size() * sizeof(unsigned short));
            break;
        }
        case SSM_GET_CUSTOMER_DATA_START:
            ret = SSMGetCustomerDataStart();
            break;

        case SSM_GET_CUSTOMER_DATA_LEN:
            ret = SSMGetCustomerDataLen();
            break;

        case SSM_SAVE_STANDBY_MODE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveStandbyMode(tmp_val);
            break;
        }
        case SSM_READ_STANDBY_MODE:
            ret = SSMReadStandbyMode();
            break;

        case SSM_SAVE_LOGO_ON_OFF_FLAG: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveLogoOnOffFlag(tmpSwitchFlag);
            break;
        }
        case SSM_READ_LOGO_ON_OFF_FLAG:
            ret = SSMReadLogoOnOffFlag();
            break;

        case SSM_SAVE_HDMIEQ_MODE: {
            int tmpSwitchFlag = p.readInt32();
            ret = SSMSaveHDMIEQMode(tmpSwitchFlag);
            break;
        }
        case SSM_READ_HDMIEQ_MODE:
            ret = SSMReadHDMIEQMode();
            break;

        case SSM_SAVE_HDMIINTERNAL_MODE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveHDMIInternalMode(tmp_val);
            break;
        }
        case SSM_READ_HDMIINTERNAL_MODE:
            ret = SSMReadHDMIInternalMode();
            break;

        case SSM_SAVE_GLOBAL_OGOENABLE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveGlobalOgoEnable(tmp_val);
            break;
        }
        case SSM_READ_GLOBAL_OGOENABLE:
            ret = SSMReadGlobalOgoEnable();
            break;

        case SSM_SAVE_NON_STANDARD_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveNonStandardValue(tmp_val);
            break;
        }
        case SSM_READ_NON_STANDARD_STATUS:
            ret = SSMReadNonStandardValue();
            break;

        case SSM_SAVE_ADB_SWITCH_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveAdbSwitchValue(tmp_val);
            break;
        }
        case SSM_READ_ADB_SWITCH_STATUS:
            ret = SSMReadAdbSwitchValue();
            break;

        case SSM_SET_HDCP_KEY:
            ret = SSMSetHDCPKey();
            break;

        case SSM_REFRESH_HDCPKEY:
            ret = SSMRefreshHDCPKey();
            break;

        case SSM_SAVE_CHROMA_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveChromaStatus(tmp_val);
            break;
        }
        case SSM_SAVE_CA_BUFFER_SIZE: {
            int tmp_val = p.readInt32();
            ret = SSMSaveCABufferSizeValue(tmp_val);
            break;
        }
        case SSM_READ_CA_BUFFER_SIZE:
            ret= SSMReadCABufferSizeValue();
            break;

        case SSM_GET_ATV_DATA_START:
            ret = SSMGetATVDataStart();
            break;

        case SSM_GET_ATV_DATA_LEN:
            ret = SSMGetATVDataLen();
            break;

        case SSM_GET_VPP_DATA_START:
            ret = SSMGetVPPDataStart();
            break;

        case SSM_GET_VPP_DATA_LEN:
            ret = SSMGetVPPDataLen();
            break;

        case SSM_SAVE_NOISE_GATE_THRESHOLD_STATUS: {
            int tmp_val = p.readInt32();
            ret = SSMSaveNoiseGateThresholdValue(tmp_val);
            break;
        }
        case SSM_READ_NOISE_GATE_THRESHOLD_STATUS:
            ret = SSMReadNoiseGateThresholdValue();
            break;

        case SSM_SAVE_HDMI_EDID_VER: {
            int port_id = p.readInt32();
            int ver = p.readInt32();
            ret = SSMSaveHDMIEdidMode((tv_hdmi_port_id_t)port_id, (tv_hdmi_edid_version_t)ver);
            break;
        }
        case SSM_READ_HDMI_EDID_VER: {
            int port_id = p.readInt32();
            ret = SSMReadHDMIEdidVersion((tv_hdmi_port_id_t)port_id);
            break;
        }
        case SSM_SAVE_HDCP_KEY_ENABLE: {
            int enable = p.readInt32();
            ret = SSMSaveHDMIHdcpSwitcher(enable);
            break;
        }
        case SSM_READ_HDCP_KEY_ENABLE:
            ret = SSMReadHDMIHdcpSwitcher();
            break;
        // SSM END

        //MISC
        case MISC_CFG_SET: {
            String8 key(p.readString16());
            String8 value(p.readString16());

            ret = config_set_str(CFG_SECTION_TV, key.string(), value.string());
            break;
        }
        case MISC_CFG_GET: {
            String8 key(p.readString16());
            String8 def(p.readString16());

            const char *value = config_get_str(CFG_SECTION_TV, key.string(), def.string());
            //r->writeString16(String16(value));
            break;
        }
        case MISC_SET_WDT_USER_PET: {
            int counter = p.readInt32();
            ret = TvMisc_SetUserCounter(counter);
            break;
        }
        case MISC_SET_WDT_USER_COUNTER: {
            int counter_time_out = p.readInt32();
            ret = TvMisc_SetUserCounterTimeOut(counter_time_out);
            break;
        }
        case MISC_SET_WDT_USER_PET_RESET_ENABLE: {
            int enable = p.readInt32();
            ret = TvMisc_SetUserPetResetEnable(enable);
            break;
        }
        case MISC_GET_TV_API_VERSION: {
            // write tvapi version info
            const char *str = tvservice_get_git_branch_info();
            //r->writeString16(String16(str));

            str = tvservice_get_git_version_info();
            //r->writeString16(String16(str));

            str = tvservice_get_last_chaned_time_info();
            //r->writeString16(String16(str));

            str = tvservice_get_build_time_info();
            //r->writeString16(String16(str));

            str = tvservice_get_build_name_info();
            //r->writeString16(String16(str));
            break;
        }
        case MISC_GET_DVB_API_VERSION: {
            // write dvb version info
            //const char *str = dvb_get_git_branch_info();
            //r->writeString16(String16(str));

            //str = dvb_get_git_version_info();
            //r->writeString16(String16(str));

            //str = dvb_get_last_chaned_time_info();
            //r->writeString16(String16(str));

            //str = dvb_get_build_time_info();
            //r->writeString16(String16(str));

            //str = dvb_get_build_name_info();
            //r->writeString16(String16(str));
            break;
        }
        //MISC  END

        // EXTAR
        case ATV_GET_CURRENT_PROGRAM_ID:
            ret = mpTv->getATVProgramID();
            break;

        case DTV_GET_CURRENT_PROGRAM_ID:
            ret = mpTv->getDTVProgramID();
            break;

        case ATV_SAVE_PROGRAM_ID: {
            int progID = p.readInt32();
            mpTv->saveATVProgramID(progID);
            break;
        }
        case SAVE_PROGRAM_ID: {
            int type = p.readInt32();
            int progID = p.readInt32();
            if (type == CTvProgram::TYPE_DTV)
                mpTv->saveDTVProgramID(progID);
            else if (type == CTvProgram::TYPE_RADIO)
                mpTv->saveRadioProgramID(progID);
            else
                mpTv->saveATVProgramID(progID);
            break;
        }
        case GET_PROGRAM_ID: {
            int type = p.readInt32();
            int id;
            if (type == CTvProgram::TYPE_DTV)
                id = mpTv->getDTVProgramID();
            else if (type == CTvProgram::TYPE_RADIO)
                id = mpTv->getRadioProgramID();
            else
                id = mpTv->getATVProgramID();
            ret = id;
            break;
        }

        case ATV_GET_MIN_MAX_FREQ: {
            int min, max;
            int tmpRet = mpTv->getATVMinMaxFreq(&min, &max);
            //r->writeInt32(min);
            //r->writeInt32(max);
            //r->writeInt32(tmpRet);
            break;
        }
        case DTV_GET_SCAN_FREQUENCY_LIST: {
            Vector<sp<CTvChannel> > out;
            ret = CTvRegion::getChannelListByName((char *)"CHINA,Default DTMB ALL", out);
            //r->writeInt32(out.size());
            for (int i = 0; i < (int)out.size(); i++) {
                //r->writeInt32(out[i]->getID());
                //r->writeInt32(out[i]->getFrequency());
                //r->writeInt32(out[i]->getLogicalChannelNum());
            }
            break;
        }
        case DTV_GET_SCAN_FREQUENCY_LIST_MODE: {
            int mode = p.readInt32();
            Vector<sp<CTvChannel> > out;
            ret = CTvRegion::getChannelListByName((char *)CTvScanner::getDtvScanListName(mode), out);
            //r->writeInt32(out.size());
            for (int i = 0; i < (int)out.size(); i++) {
                //r->writeInt32(out[i]->getID());
                //r->writeInt32(out[i]->getFrequency());
                //r->writeInt32(out[i]->getLogicalChannelNum());
            }
            break;
        }
        case DTV_GET_CHANNEL_INFO: {
            int dbID = p.readInt32();
            channel_info_t chan_info;
            ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
            //r->writeInt32(chan_info.freq);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.strength);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.quality);
            //r->writeInt32(chan_info.uInfo.dtvChanInfo.ber);
            break;
        }
        case ATV_GET_CHANNEL_INFO: {
            int dbID = p.readInt32();
            channel_info_t chan_info;
            ret = mpTv->getChannelInfoBydbID(dbID, chan_info);
            //r->writeInt32(chan_info.freq);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.finefreq);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.videoStd);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.audioStd);
            //r->writeInt32(chan_info.uInfo.atvChanInfo.isAutoStd);
            break;
        }
        case ATV_SCAN_MANUAL: {
            int startFreq = p.readInt32();
            int endFreq = p.readInt32();
            int videoStd = p.readInt32();
            int audioStd = p.readInt32();
            ret = mpTv->atvMunualScan(startFreq, endFreq, videoStd, audioStd);
            //mTvService->mpScannerClient = this;
            break;
        }

        case ATV_SCAN_AUTO: {
            int videoStd = p.readInt32();
            int audioStd = p.readInt32();
            int searchType = p.readInt32();
            int procMode = p.readInt32();
            ret = mpTv->atvAutoScan(videoStd, audioStd, searchType, procMode);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_MANUAL: {
            int freq = p.readInt32();
            ret = mpTv->dtvManualScan(freq, freq);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_MANUAL_BETWEEN_FREQ: {
            int beginFreq = p.readInt32();
            int endFreq = p.readInt32();
            int modulation = p.readInt32();
            ret = mpTv->dtvManualScan(beginFreq, endFreq, modulation);
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN_AUTO: {
            ret = mpTv->dtvAutoScan();
            //mTvService->mpScannerClient = this;
            break;
        }
        case DTV_SCAN: {
            int mode = p.readInt32();
            int scanmode = p.readInt32();
            int freq = p.readInt32();
            int para1 = p.readInt32();
            int para2 = p.readInt32();
            ret = mpTv->dtvScan(mode, scanmode, freq, freq, para1, para2);
            //mTvService->mpScannerClient = this;
            break;
        }
        case STOP_PROGRAM_PLAY:
            ret = mpTv->stopPlayingLock();
            break;

        case ATV_DTV_SCAN_PAUSE:
            ret = mpTv->pauseScan();
            break;

        case ATV_DTV_SCAN_RESUME:
            ret = mpTv->resumeScan();
            break;

        case ATV_DTV_GET_SCAN_STATUS:
            ret = mpTv->getScanStatus();
            break;

        case ATV_DTV_SCAN_OPERATE_DEVICE : {
            int type = p.readInt32();
            mpTv->operateDeviceForScan(type);
            break;
        }
        case DTV_SET_TEXT_CODING: {
            String8 coding(p.readString16());
            mpTv->setDvbTextCoding((char *)coding.string());
            break;
        }

        case TV_CLEAR_ALL_PROGRAM: {
            int arg0 = p.readInt32();

            ret = mpTv->clearAllProgram(arg0);
            //mTvService->mpScannerClient = this;
            break;
        }

        case HDMIRX_GET_KSV_INFO: {
            int data[2] = {0, 0};
            ret = mpTv->GetHdmiHdcpKeyKsvInfo(data);
            //r->writeInt32(data[0]);
            //r->writeInt32(data[1]);
            break;
        }

        case STOP_SCAN:
            mpTv->stopScanLock();
            break;

        case DTV_GET_SNR:
            ret = mpTv->getFrontendSNR();
            break;

        case DTV_GET_BER:
            ret = mpTv->getFrontendBER();
            break;

        case DTV_GET_STRENGTH:
            ret = mpTv->getFrontendSignalStrength();
            break;

        case DTV_GET_AUDIO_TRACK_NUM: {
            int programId = p.readInt32();
            ret = mpTv->getAudioTrackNum(programId);
            break;
        }
        case DTV_GET_AUDIO_TRACK_INFO: {
            int progId = p.readInt32();
            int aIdx = p.readInt32();
            int aFmt = -1;
            String8 lang;
            ret = mpTv->getAudioInfoByIndex(progId, aIdx, &aFmt, lang);
            //r->writeInt32(aFmt);
            //r->writeString16(String16(lang));
            break;
        }
        case DTV_SWITCH_AUDIO_TRACK: {
            int aPid = p.readInt32();
            int aFmt = p.readInt32();
            int aParam = p.readInt32();
            ret = mpTv->switchAudioTrack(aPid, aFmt, aParam);
            break;
        }
        case DTV_SET_AUDIO_AD: {
            int aEnable = p.readInt32();
            int aPid = p.readInt32();
            int aFmt = p.readInt32();
            ret = mpTv->setAudioAD(aEnable, aPid, aFmt);
            break;
        }
        case DTV_GET_CURR_AUDIO_TRACK_INDEX: {
            int currAduIdx = -1;
            int progId = p.readInt32();
            CTvProgram prog;
            CTvProgram::selectByID(progId, prog);
            currAduIdx = prog.getCurrAudioTrackIndex();
            //r->writeInt32(currAduIdx);
            break;
        }
        case DTV_SET_AUDIO_CHANNEL_MOD: {
            int audioChannelIdx = p.readInt32();
            mpTv->setAudioChannel(audioChannelIdx);
            break;
        }
        case DTV_GET_AUDIO_CHANNEL_MOD: {
            int currChannelMod = mpTv->getAudioChannel();
            //r->writeInt32(currChannelMod);
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
            //r->writeInt32(freq);
            break;
        }
        case DTV_GET_EPG_UTC_TIME: {
            int utcTime = mpTv->getTvTime();
            //r->writeInt32(utcTime);
            break;
        }
        case DTV_GET_EPG_INFO_POINT_IN_TIME: {
            int progid = p.readInt32();
            int utcTime = p.readInt32();
            CTvProgram prog;
            int ret = CTvProgram::selectByID(progid, prog);
            CTvEvent ev;
            ret = ev.getProgPresentEvent(prog.getSrc(), prog.getID(), utcTime, ev);
            //r->writeString16(String16(ev.getName()));
            //r->writeString16(String16(ev.getDescription()));
            //r->writeString16(String16(ev.getExtDescription()));
            //r->writeInt32(ev.getStartTime());
            //r->writeInt32(ev.getEndTime());
            //r->writeInt32(ev.getSubFlag());
            //r->writeInt32(ev.getEventId());
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

            //r->writeInt32(iObOutSize);
            for (int i = 0; i < iObOutSize; i ++) {
                //r->writeString16(String16(epgOut[i]->getName()));
                //r->writeString16(String16(epgOut[i]->getDescription()));
                //r->writeString16(String16(ev.getExtDescription()));
                //r->writeInt32(epgOut[i]->getStartTime());
                //r->writeInt32(epgOut[i]->getEndTime());
                //r->writeInt32(epgOut[i]->getSubFlag());
                //r->writeInt32(epgOut[i]->getEventId());
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
        case START_AUTO_BACKLIGHT:
            mpTv->setAutoBackLightStatus(1);
            break;

        case STOP_AUTO_BACKLIGHT:
            mpTv->setAutoBackLightStatus(0);
            break;

        case IS_AUTO_BACKLIGHTING: {
            int on = mpTv->getAutoBackLightStatus();
            break;
        }

        case GET_AVERAGE_LUMA:
            ret = mpTv->getAverageLuma();
            break;

        case GET_AUTO_BACKLIGHT_DATA: {
            int buf[128] = {0};
            int size = mpTv->getAutoBacklightData(buf);
            //r->writeInt32(size);
            for (int i = 0; i < size; i++) {
                //r->writeInt32(buf[i]);
            }
            break;
        }
        case SET_AUTO_BACKLIGHT_DATA:
            ret = mpTv->setAutobacklightData(String8(p.readString16()));
            break;

        case SSM_READ_BLACKOUT_ENABLE: {
            int enable = mpTv->getSaveBlackoutEnable();
            //r->writeInt32(enable);
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
            ret = CTvProgram::selectByID(progid, prog);
            if (ret != 0) return -1;
            CTvChannel channel;
            prog.getChannel(channel);
            freq = channel.getFrequency();
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
            //r->writeInt32(iObOutSize);
            for (int i = 0; i < iObOutSize; i ++) {
                //r->writeString16(String16(vTvBookOut[i]->getProgName()));
                //r->writeString16(String16(vTvBookOut[i]->getEvtName()));
                //r->writeInt32(vTvBookOut[i]->getStartTime());
                //r->writeInt32(vTvBookOut[i]->getDurationTime());
                //r->writeInt32(vTvBookOut[i]->getBookId());
                //r->writeInt32(vTvBookOut[i]->getProgramId());
                //r->writeInt32(vTvBookOut[i]->getEventId());
            }
            break;
        }
        case SET_FRONTEND_PARA: {
            frontend_para_set_t feParms;
            //feParms.mode = (fe_type_t)p.readInt32();
            feParms.freq = p.readInt32();
            feParms.videoStd = (atv_video_std_t)p.readInt32();
            feParms.audioStd = (atv_audio_std_t)p.readInt32();
            feParms.para1 = p.readInt32();
            feParms.para2 = p.readInt32();
            mpTv->resetFrontEndPara(feParms);
            break;
        }
        case PLAY_PROGRAM: {
            int mode = p.readInt32();
            int freq = p.readInt32();
            if (mode == TV_FE_ANALOG) {
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
            /*
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
            */
            break;
        }
        case DTV_GET_VIDEO_FMT_INFO: {
            int srcWidth = 0;
            int srcHeight = 0;
            int srcFps = 0;
            int srcInterlace = 0;
            int iRet = -1;

    /*
            iRet == mpTv->getVideoFormatInfo(&srcWidth, &srcHeight, &srcFps, &srcInterlace);
            r->writeInt32(srcWidth);
            r->writeInt32(srcHeight);
            r->writeInt32(srcFps);
            r->writeInt32(srcInterlace);
            r->writeInt32(iRet);
            */
        }
        break;

        case DTV_GET_AUDIO_FMT_INFO: {
            int iRet = -1;
            int fmt[2];
            int sample_rate[2];
            int resolution[2];
            int channels[2];
            int lpepresent[2];
            int frames;
            int ab_size;
            int ab_data;
            int ab_free;
            iRet == mpTv->getAudioFormatInfo(fmt, sample_rate, resolution, channels,
                lpepresent, &frames, &ab_size, &ab_data, &ab_free);
            /*
            r->writeInt32(fmt[0]);
            r->writeInt32(fmt[1]);
            r->writeInt32(sample_rate[0]);
            r->writeInt32(sample_rate[1]);
            r->writeInt32(resolution[0]);
            r->writeInt32(resolution[1]);
            r->writeInt32(channels[0]);
            r->writeInt32(channels[1]);
            r->writeInt32(lpepresent[0]);
            r->writeInt32(lpepresent[1]);
            r->writeInt32(frames);
            r->writeInt32(ab_size);
            r->writeInt32(ab_data);
            r->writeInt32(ab_free);
            r->writeInt32(iRet);
            */
        }
        break;

        case HDMIAV_HOTPLUGDETECT_ONOFF:
            ret = mpTv->GetHdmiAvHotplugDetectOnoff();
            break;

        case HANDLE_GPIO: {
            String8 strName(p.readString16());
            int is_out = p.readInt32();
            int edge = p.readInt32();
            ret = mpTv->handleGPIO((char *)strName.string(), is_out, edge);
            break;
        }
        case SET_LCD_ENABLE: {
            int enable = p.readInt32();
            ret = mpTv->setLcdEnable(enable);
            break;
        }
        case GET_ALL_TV_DEVICES:
            //const char *value = config_get_str(CFG_SECTION_TV, CGF_DEFAULT_INPUT_IDS, "null");
            //r->writeCString(value);
            break;

        case GET_HDMI_PORTS:
            ret = config_get_int(CFG_SECTION_TV, CGF_DEFAULT_HDMI_PORTS, 0);
            break;

        case TV_CLEAR_FRONTEND: {
            int para = p.readInt32();
            ret = mpTv->clearFrontEnd(para);
            break;
        }
        case TV_SET_FRONTEND: {
            int force = p.readInt32();
            String8 feparas(p.readString16());
            ret = mpTv->setFrontEnd(feparas, (force != 0));
            break;
        }
        case PLAY_PROGRAM_2: {
            String8 feparas(p.readString16());
            int vid = p.readInt32();
            int vfmt = p.readInt32();
            int aid = p.readInt32();
            int afmt = p.readInt32();
            int pcr = p.readInt32();
            int audioCompetation = p.readInt32();
            mpTv->playDtvProgram(feparas.string(), vid, vfmt, aid, afmt, pcr, audioCompetation);
            break;
        }
        case TV_SCAN_2: {
            String8 feparas(p.readString16());
            String8 scanparas(p.readString16());
            ret = mpTv->Scan(feparas.string(), scanparas.string());
            //mTvService->mpScannerClient = this;
            break;
        }
        case SET_AUDIO_OUTMODE: {
            int mode = p.readInt32();
            ret = SetAudioOutmode(mode);
            break;
        }

        case GET_AUDIO_OUTMODE:
            ret = GetAudioOutmode();
            break;

        case GET_AUDIO_STREAM_OUTMODE:
            ret = GetAudioStreamOutmode();
            break;

        case SET_AMAUDIO_VOLUME: {
            int volume = p.readInt32();
            ret = mpTv->setAmAudioVolume(float (volume));
            break;
        }
        case GET_AMAUDIO_VOLUME:
            ret = (int)mpTv->getAmAudioVolume();
            break;

        case SAVE_AMAUDIO_VOLUME: {
            int volume = p.readInt32();
            int source = p.readInt32();
            ret = mpTv->saveAmAudioVolume(volume, source);
            break;
        }
        case GET_SAVE_AMAUDIO_VOLUME: {
            int source = p.readInt32();
            ret = mpTv->getSaveAmAudioVolume(source);
            break;
        }
        case DTV_UPDATE_RRT: {
            int freq = p.readInt32();
            int modulation = p.readInt32();
            int mode = p.readInt32();
            ret = mpTv->Tv_RrtUpdate(freq, modulation, mode);
            break;
        }
        case DTV_SEARCH_RRT: {
            int rating_region_id = p.readInt32();
            int dimension_id = p.readInt32();
            int value_id = p.readInt32();
            rrt_select_info_t rrt_info;
            ret = mpTv->Tv_RrtSearch(rating_region_id, dimension_id, value_id, &rrt_info);
            //r->writeInt32(rrt_info.dimensions_name_count);
            int count = rrt_info.dimensions_name_count;
            if (count != 0) {
                //r->writeString16(String16(rrt_info.dimensions_name));
            }
            //r->writeInt32(rrt_info.rating_region_name_count);
            count = rrt_info.rating_region_name_count;
            if (count != 0) {
                //r->writeString16(String16(rrt_info.rating_region_name));
            }
            //r->writeInt32(rrt_info.rating_value_text_count);
            count = rrt_info.rating_value_text_count;
            if (count != 0) {
                //r->writeString16(String16(rrt_info.rating_value_text));
            }
            break;
        }

        case DTV_UPDATE_EAS:
            ret = mpTv->Tv_Easupdate();
            break;
        // EXTAR END

        //NEWPLAY/RECORDING
        case DTV_RECORDING_CMD:
            ret = mpTv->doRecordingCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
            break;

        case DTV_PLAY_CMD:
            ret = mpTv->doPlayCommand(p.readInt32(), String8(p.readString16()).string(), String8(p.readString16()).string());
            break;

        default:
            break;
    }

    return ret;
}

void DroidTvServiceIntf::setListener(const sp<TvServiceNotify>& listener) {
    mNotifyListener = listener;
}



status_t DroidTvServiceIntf::dump(int fd, const Vector<String16>& args)
{
#if 0
    String8 result;
    if (!checkCallingPermission(String16("android.permission.DUMP"))) {
        char buffer[256];
        snprintf(buffer, 256, "Permission Denial: "
                "can't dump system_control from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        AutoMutex _l(mServiceLock);
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
                        "usage: dumpsys tvservice [-s <SECTION> <KEY>][-m][-h] \n"
                        "-s: get config string \n"
                        "   SECTION:[TV|ATV|SourceInputMap|SETTING|FBCUART]\n"
                        "-m: track native heap memory\n"
                        "-h: help \n\n");
                }
                else if (args[i] == String16("-m")) {
                    dumpMemoryAddresses(fd);
                }
            }
        }
        else {
            /*
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
            */

            mpTv->dump(result);
        }
    }
    write(fd, result.string(), result.size());
#endif
    return NO_ERROR;
}


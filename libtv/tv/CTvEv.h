/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _CTVEV_H_
#define _CTVEV_H_

#include <utils/String8.h>
#define CC_MAX_SERIAL_RD_BUF_LEN   (1200)

using namespace android;

class CTvEv {
public:
    static const int TV_EVENT_COMMOM = 0;//通用消息
    static const int TV_EVENT_SCANNER = 1;//搜索消息
    static const int TV_EVENT_EPG = 2;//EPG
    static const int TV_EVENT_SOURCE_SWITCH = 3;//信源切换
    static const int TV_EVENT_SIGLE_DETECT = 4;
    static const int TV_EVENT_ADC_CALIBRATION = 5;//ADC校准
    static const int TV_EVENT_VGA = 6;//VGA
    static const int TV_EVENT_3D_STATE = 7;//3D
    static const int TV_EVENT_AV_PLAYBACK = 8;//PLAYBACK EVENT MSG
    static const int TV_EVENT_SERIAL_COMMUNICATION = 9;
    static const int TV_EVENT_SOURCE_CONNECT = 10;
    static const int TV_EVENT_HDMIRX_CEC = 11;
    static const int TV_EVENT_BLOCK = 12;
    static const int TV_EVENT_CC = 13; //close caption
    static const int TV_EVENT_VCHIP = 14; //VCHIP
    static const int TV_EVENT_HDMI_IN_CAP = 15;
    static const int TV_EVENT_UPGRADE_FBC = 16;
    static const int TV_EVENT_2d4G_HEADSET = 17;
    static const int TV_EVENT_AV = 18;
    static const int TV_EVENT_SUBTITLE = 19;
    static const int TV_EVENT_SCANNING_FRAME_STABLE = 20;
    static const int TV_EVENT_FRONTEND = 21;
    static const int TV_EVENT_RECORDER = 22;

    CTvEv(int type);
    virtual ~CTvEv() {};
	int getEvType() const {
        return mEvType;
    };
private:
    int mEvType;
};

namespace  TvEvent {
    //events
    class SignalInfoEvent: public CTvEv {
    public:
        SignalInfoEvent() : CTvEv ( CTvEv::TV_EVENT_SIGLE_DETECT ) {}
        ~SignalInfoEvent() {}
        int mTrans_fmt;
        int mFmt;
        int mStatus;
        int mReserved;
    };

    class VGAEvent: public CTvEv {
    public:
        VGAEvent() : CTvEv ( CTvEv::TV_EVENT_VGA ) {}
        ~VGAEvent() {}
        int mState;
    };

    class ADCCalibrationEvent: public CTvEv {
    public:
        ADCCalibrationEvent() : CTvEv ( CTvEv::TV_EVENT_ADC_CALIBRATION ) {}
        ~ADCCalibrationEvent() {}
        int mState;
    };

    class SerialCommunicationEvent: public CTvEv {
    public:
        SerialCommunicationEvent(): CTvEv(CTvEv::TV_EVENT_SERIAL_COMMUNICATION) {}
        ~SerialCommunicationEvent() {}

        int mDevId;
        int mDataCount;
        unsigned char mDataBuf[CC_MAX_SERIAL_RD_BUF_LEN];
    };

    class SourceConnectEvent: public CTvEv {
    public:
        SourceConnectEvent() : CTvEv ( CTvEv::TV_EVENT_SOURCE_CONNECT ) {}
        ~SourceConnectEvent() {}
        int mSourceInput;
        int connectionState;
    };

    class HDMIRxCECEvent: public CTvEv {
    public:
        HDMIRxCECEvent() : CTvEv ( CTvEv::TV_EVENT_HDMIRX_CEC ) {}
        ~HDMIRxCECEvent() {}
        int mDataCount;
        int mDataBuf[32];
    };

    class AVPlaybackEvent: public CTvEv {
    public:
        AVPlaybackEvent() : CTvEv ( CTvEv::TV_EVENT_AV_PLAYBACK ) {}
        ~AVPlaybackEvent() {}
        static const int EVENT_AV_PLAYBACK_NODATA       = 1;
        static const int EVENT_AV_PLAYBACK_RESUME       = 2;
        static const int EVENT_AV_SCAMBLED              = 3;
        static const int EVENT_AV_UNSUPPORT             = 4;
        static const int EVENT_AV_VIDEO_AVAILABLE       = 5;

        static const int EVENT_AV_TIMESHIFT_REC_FAIL = 6;
        static const int EVENT_AV_TIMESHIFT_PLAY_FAIL = 7;
        static const int EVENT_AV_TIMESHIFT_START_TIME_CHANGED = 8;
        static const int EVENT_AV_TIMESHIFT_CURRENT_TIME_CHANGED = 9;

        int mMsgType;
        int mProgramId;
    };

    class BlockEvent: public CTvEv {
    public:
        BlockEvent() : CTvEv ( CTvEv::TV_EVENT_BLOCK ) {}
        ~BlockEvent() {}

        bool block_status;
        int programBlockType;
        String8 vchipDimension;
        String8 vchipAbbrev;
        String8 vchipAbbtext;
    };

    class UpgradeFBCEvent: public CTvEv {
    public:
        UpgradeFBCEvent() : CTvEv ( CTvEv::TV_EVENT_UPGRADE_FBC ) {}
        ~UpgradeFBCEvent() {}
        int mState;
        int param;
    };

    class HeadSetOf2d4GEvent: public CTvEv {
    public:
        HeadSetOf2d4GEvent(): CTvEv(CTvEv::TV_EVENT_2d4G_HEADSET) {}
        ~HeadSetOf2d4GEvent() {}

        int state;
        int para;
    };

    class SubtitleEvent: public CTvEv {
    public:
        SubtitleEvent(): CTvEv(CTvEv::TV_EVENT_SUBTITLE) {}
        ~SubtitleEvent() {}
        int pic_width;
        int pic_height;
    };

    class ScanningFrameStableEvent: public CTvEv {
    public:
        ScanningFrameStableEvent(): CTvEv(CTvEv::TV_EVENT_SCANNING_FRAME_STABLE) {}
        ~ScanningFrameStableEvent() {}
        int CurScanningFreq;
    };

    class FrontendEvent: public CTvEv {
    public:
        FrontendEvent() : CTvEv ( CTvEv::TV_EVENT_FRONTEND ) {}
        ~FrontendEvent() {}
        static const int EVENT_FE_HAS_SIG = 0x01;
        static const int EVENT_FE_NO_SIG = 0x02;
        static const int EVENT_FE_INIT = 0x03;

        int mStatus;
        int mFrequency;
        int mParam1;
        int mParam2;
        int mParam3;
        int mParam4;
        int mParam5;
        int mParam6;
        int mParam7;
        int mParam8;
    };

    class RecorderEvent: public CTvEv {
    public:
        RecorderEvent() : CTvEv ( CTvEv::TV_EVENT_RECORDER) {}
        ~RecorderEvent() {}
        static const int EVENT_RECORD_START = 0x01;
        static const int EVENT_RECORD_STOP = 0x02;

        String8 mId;
        int mStatus;
        int mError;
    };

};
#endif


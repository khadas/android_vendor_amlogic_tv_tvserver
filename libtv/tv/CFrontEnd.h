#ifndef ANDROID_FRONTEND_H
#define ANDROID_FRONTEND_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "CTvLog.h"
#include "CTvEv.h"

#include "tvutils.h"

#include <map>
#include <string>

extern "C" {
#include "am_fend.h"
#include "am_vout.h"
#include "linux/dvb/frontend.h"
#include "am_fend_ctrl.h"
}
//for app
typedef enum atv_audo_std_s {
    CC_ATV_AUDIO_STD_START = 0,
    CC_ATV_AUDIO_STD_DK = 0,
    CC_ATV_AUDIO_STD_I,
    CC_ATV_AUDIO_STD_BG,
    CC_ATV_AUDIO_STD_M,
    CC_ATV_AUDIO_STD_L,
    CC_ATV_AUDIO_STD_AUTO,
    CC_ATV_AUDIO_STD_END = CC_ATV_AUDIO_STD_AUTO,
    CC_ATV_AUDIO_STD_MUTE,
} atv_audio_std_t;
//for app
typedef enum atv_video_std_s {
    CC_ATV_VIDEO_STD_START = 0,
    CC_ATV_VIDEO_STD_AUTO = 0,
    CC_ATV_VIDEO_STD_PAL,
    CC_ATV_VIDEO_STD_NTSC,
    CC_ATV_VIDEO_STD_SECAM,
    CC_ATV_VIDEO_STD_END = CC_ATV_VIDEO_STD_SECAM,
} atv_video_std_t;


//from kernel
/*COLOR MODULATION TYPE*/
static const   v4l2_std_id  V4L2_COLOR_STD_PAL  =   ((v4l2_std_id)0x04000000);
static const   v4l2_std_id  V4L2_COLOR_STD_NTSC = ((v4l2_std_id)0x08000000);
static const   v4l2_std_id  V4L2_COLOR_STD_SECAM =  ((v4l2_std_id)0x10000000);
//virtual
static const   v4l2_std_id  V4L2_COLOR_STD_AUTO  =    ((v4l2_std_id)0x02000000);

typedef struct frontend_para_set_s {
    fe_type_t mode;
    int freq;
    atv_video_std_t videoStd;
    atv_audio_std_t audioStd;
    int para1;
    int para2;
} frontend_para_set_t;

typedef struct atv_channel_info_s {
    int finefreq;
    atv_video_std_t videoStd;
    atv_audio_std_t audioStd;
    int isAutoStd;
} atv_channel_info_t;

typedef struct dtv_channel_info_s {
    int strength;
    int quality;
    int ber;
} dtv_channel_info_t;

typedef struct channel_info_s {
    int freq;
    union {
        atv_channel_info_t atvChanInfo;
        dtv_channel_info_t dtvChanInfo;
    } uInfo;
} channel_info_t;

class CFrontEnd {
public:

    static const int FE_DEV_ID = 0;
    static const int AFC_RANGE = 1000000;

    CFrontEnd();
    ~CFrontEnd();

    int Open(int mode);
    int Close();
    int setMode(int mode);
    int fineTune(int freq);
    static int formatATVFreq(int freq);
    int GetTSSource(AM_DMX_Source_t *src);
    int setPara(int mode, int freq, int para1, int para2);
    int setPara(const char *);
    int setProp(int cmd, int val);
    int ClearAnalogFrontEnd();
    int autoLoadFE();
    int SetAnalogFrontEndTimerSwitch(int onOff);
    int SetAnalogFrontEndSearhSlowMode(int onOff);

    static int stdAndColorToAudioEnum(int std);
    static int stdAndColorToVideoEnum(int std);
    static bool stdIsColorAuto(int std);
    static int addColorAutoFlag(int std);
    static int printVideoStdStr(int videoStd, char strBuffer[], int buff_size);
    static int printAudioStdStr(int audioStd, char strBuffer[], int buff_size);
    static v4l2_std_id enumToStdAndColor(int videoStd, int audioStd);
    static int stdEnumToCvbsFmt (int videoStd, int audioStd);

    static CFrontEnd *getInstance();

    class FEEvent: public CTvEv {
    public:
        //static const int EVENT_FE_HAS_SIG       = 0X01; /* found something above the noise level */
        //static const int EVENT_FE_HAS_CARRIER   = 0x02; /* found a DVB signal  */
        //static const int EVENT_FE_HAS_VITERBI   = 0X04; /* FEC is stable  */
        //static const int EVENT_FE_HAS_SYNC      = 0X08; /* found sync bytes  */
        // static const int EVENT_FE_HAS_LOCK      = 0X10; /* everything's working... */
        //static const int EVENT_FE_HAS_TIMEOUT   = 0X20; /* no lock within the last ~2 seconds */
        //static const int EVENT_FE_REINIT        = 0X40; /* frontend was reinitialized,  */
        static const int EVENT_FE_HAS_SIG = 0x01;
        static const int EVENT_FE_NO_SIG = 0x02;
        static const int EVENT_FE_INIT = 0x03;

        FEEvent(): CTvEv(CTvEv::TV_EVENT_SIGLE_DETECT)
        {
        }
        ~FEEvent()
        {
        }
        int mCurSigStaus;
        int mCurFreq;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const FEEvent &ev) = 0;
    };

    int setObserver(IObserver *ob)
    {
        mpObserver = ob;
        return 0;
    }

    int getSNR();
    int getBER();
    int getInfo();
    int getStatus();
    int checkStatusOnce();
    int getStrength();
    int setCvbsAmpOut(int amp);
    int setThreadDelay(int delay) ;
    int getPara(int *mode, int *freq, int *para1, int *para2);
    int lock(int frequency, int symbol_rate, int modulation, int bandwidth);
    int setTunerAfc(int afc);

    class FEMode {
    private:
        int mMode;

    public:
        FEMode() {}
        FEMode(int mode) { mMode = mode; }
        FEMode(const FEMode &femode):mMode(femode.mMode) {}
        /*
            fe mode:0xaabbccdd
                aa - reserved
                bb - scanlist/contury etc.
                cc - t/t2 s/s2 c/c2 identifier
                dd - femode FE_XXXX
        */
        /*mode*/
        int getMode() const { return mMode; }
        /*dd*/
        int getBase() const { return get8(0); }
        /*cc*/
        int getGen() const { return get8(1); }
        /*bb*/
        int getList() const { return get8(2); }
        /*aa*/
        int getExt() const { return get8(3); }

        void setBase(int v) { set8(0, v); }
        void setGen(int v) { set8(1, v); }
        void setList(int v) { set8(2, v); }
        void setExt(int v) { set8(3, v); }

        bool operator == (const FEMode &femode) const;

    private:
        FEMode& set8(int n, int v);
        int get8(int n) const;
    };

    class FEParas : public Paras {

    public:
        FEParas() : Paras() { }
        FEParas(const char *paras) : Paras(paras) { }

        FEMode getFEMode() const { return FEMode(getInt(FEP_MODE, -1)); }
        FEParas& setFEMode(const FEMode &fem) { setInt(FEP_MODE, fem.getMode()); return *this; }
        int getFrequency() const { return getInt(FEP_FREQ, -1); }
        FEParas& setFrequency(int f) { setInt(FEP_FREQ, f); return *this; }
        int getFrequency2() const { return getInt(FEP_FREQ2, -1); }
        FEParas& setFrequency2(int f) { setInt(FEP_FREQ2, f); return *this; }
        int getBandwidth() const { return getInt(FEP_BW, -1); }
        FEParas& setBandwidth(int b) { setInt(FEP_BW, b); return *this; }
        int getSymbolrate() const { return getInt(FEP_SR, -1); }
        FEParas& setSymbolrate(int s) { setInt(FEP_SR, s); return *this; }
        int getModulation() const { return getInt(FEP_MOD, -1); }
        FEParas& setModulation(int m) { setInt(FEP_MOD, m); return *this; }
        int getPlp() const { return getInt(FEP_PLP, -1); }
        FEParas& setPlp(int p) { setInt(FEP_PLP, p); return *this; }
        int getLayer() const { return getInt(FEP_LAYR, -1); }
        FEParas& setLayer(int l) { setInt(FEP_LAYR, l); return *this; }
        int getAudioStd() const { return getInt(FEP_ASTD, 0); }
        FEParas& setAudioStd(int s) { setInt(FEP_ASTD, s); return *this; }
        int getVideoStd() const { return getInt(FEP_VSTD, 0); }
        FEParas& setVideoStd(int s) { setInt(FEP_VSTD, s); return *this; }
        int getAfc() const { return getInt(FEP_AFC, -1); }
        FEParas& setAfc(int a) { setInt(FEP_AFC, a); return *this; }

        FEParas& fromDVBParameters(const FEMode& mode, const struct dvb_frontend_parameters *dvb);
        FEParas& fromFENDCTRLParameters(const FEMode& mode, const AM_FENDCTRL_DVBFrontendParameters_t *fendctrl);

        FEParas& operator = (const FEParas &fep) { mparas = fep.mparas; return *this; };
        bool operator == (const FEParas &fep) const;

    public:
        static const char* FEP_MODE;
        static const char* FEP_FREQ;
        static const char* FEP_FREQ2;
        static const char* FEP_BW;
        static const char* FEP_SR;
        static const char* FEP_MOD;
        static const char* FEP_PLP;
        static const char* FEP_LAYR;
        static const char* FEP_VSTD;
	static const char* FEP_ASTD;
        static const char* FEP_AFC;
    };

    /* freq: freq1==freq2 for single, else for range */
    static int convertParas(char *paras, int mode, int freq1, int freq2, int para1, int para2);

private:
    static CFrontEnd *mInstance;

    int mFrontDevID;
    int mDemuxDevID;
    int mTvPlayDevID;
    int mCurFineFreq;
    IObserver *mpObserver;
    FEEvent mCurSigEv;
    int mCurMode;
    int mCurFreq;
    int mCurPara1;
    int mCurPara2;
    bool mbFEOpened;
    FEParas mFEParas;
    static void dmd_fend_callback(long dev_no, int event_type, void *param, void *user_data);
    void saveCurrentParas(FEParas &paras);
};
#endif // ANDROID_FRONTEND_H

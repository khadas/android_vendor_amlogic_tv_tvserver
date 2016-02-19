#ifndef _C_AV_H
#define _C_AV_H
#include "am_av.h"
#include "am_aout.h"
#include "CTvEv.h"
#include "CTvLog.h"
#include "../tvin/CTvin.h"
static const char *PATH_FRAME_COUNT  = "/sys/module/di/parameters/frame_count";
//3D=====================================================================
static const char *PATH_SET_3D_TYPE = "/sys/class/video/threedim_mode";
static const char *PATH_VIDEO_AMVIDEO = "/dev/amvideo";
//3和1是做不同的3dsclaer   0是不做任何scaler
static const char *PATH_3D_SCALER = "/sys/module/amvideo/parameters/force_3d_scaler";
//0是不做3dscaler 1是做3dscaler
static const char *PATH_VIDEO_SCALER = "/sys/class/video/stereo_scaler";

static const char *PATH_MEPG_DTMB_LOOKUP_PTS_FLAG = "/sys/module/amvdec_mpeg12/parameters/dtmb_flag";
/*cmd use for 3d operation*/
#define MODE_3D_DISABLE     0x00000000
#define MODE_3D_ENABLE      0x00000001
#define MODE_3D_AUTO        0x00000002
#define MODE_3D_LR          0x00000004
#define MODE_3D_TB          0x00000008
#define MODE_3D_LA          0x00000010
#define MODE_3D_FA          0x00000020
#define MODE_3D_LR_SWITCH   0x00000100
#define MODE_3D_TO_2D_L     0x00000200
#define MODE_3D_TO_2D_R     0x00000400
#define MODE_3D_MVC     0x00000800
#define MODE_3D_OUT_TB  0x00010000
#define MODE_3D_OUT_LR  0x00020000

/*when the output mode is field alterlative*/
//LRLRLRLRL mode
#define MODE_3D_OUT_FA_L_FIRST  0x00001000
#define MODE_3D_OUT_FA_R_FIRST  0x00002000
//LBRBLBRB
#define MODE_3D_OUT_FA_LB_FIRST 0x00004000
#define MODE_3D_OUT_FA_RB_FIRST 0x00008000


typedef enum VIDEO_3D_MODE_E {
    VIDEO_3D_MODE_DISABLE,
    VIDEO_3D_MODE_AUTO,
    VIDEO_3D_MODE_LR,
    VIDEO_3D_MODE_TB,
    VIDEO_3D_MODE_LA,
    VIDEO_3D_MODE_FA
} VIDEO_3D_MODE_T;

//end 3D===================================================================
typedef enum video_display_resolution_e {
    VPP_DISPLAY_RESOLUTION_1366X768,
    VPP_DISPLAY_RESOLUTION_1920X1080,
    VPP_DISPLAY_RESOLUTION_3840X2160,
    VPP_DISPLAY_RESOLUTION_MAX,
} video_display_resolution_t;


class CAv {
public:
    CAv();
    ~CAv();
    //video screen_mode
    static const int VIDEO_WIDEOPTION_NORMAL           = 0;
    static const int VIDEO_WIDEOPTION_FULL_STRETCH     = 1;
    static const int VIDEO_WIDEOPTION_4_3              = 2;
    static const int VIDEO_WIDEOPTION_16_9             = 3;
    static const int VIDEO_WIDEOPTION_NONLINEAR        = 4;
    static const int VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5;
    static const int VIDEO_WIDEOPTION_CROP_FULL        = 6;
    static const int VIDEO_WIDEOPTION_CROP             = 7;
    //
    static const int DISABLE_VIDEO_COLOR_BLUE = 1;
    static const int DISABLE_VIDEO_COLOR_BLACK = 2;
    class AVEvent : public CTvEv {
    public:
        AVEvent(): CTvEv(CTvEv::TV_EVENT_AV)
        {

        };
        ~AVEvent()
        {};
        static const int EVENT_AV_STOP            = 1;
        static const int EVENT_AV_RESUEM           = 2;
        static const int EVENT_AV_SCAMBLED      = 3;
        static const int EVENT_AV_UNSUPPORT  = 4;
        int type;
        int param;
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const AVEvent &ev) = 0;
    };
    //1 VS n
    //int addObserver(IObserver* ob);
    //int removeObserver(IObserver* ob);

    //1 VS 1
    int setObserver(IObserver *ob)
    {
        mpObserver = ob;
        return 0;
    }

    int Open();
    int Close();
    int SetVideoWindow(int x, int y, int w, int h);
    int GetVideoStatus(AM_AV_VideoStatus_t *status);
    int SwitchTSAudio(int apid, AM_AV_AFormat_t afmt);
    int ResetAudioDecoder();
    int SetTSSource(AM_AV_TSSource_t ts_source);
    int StartTS(uint16_t vpid, uint16_t apid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt);
    int StopTS();
    int AudioGetOutputMode(AM_AOUT_OutputMode_t *mode);
    int AudioSetOutputMode(AM_AOUT_OutputMode_t mode);
    int SetVideoScreenColor (int vdin_blending_mask, int y, int u, int v);
    int DisableVideoWithBlueColor();
    int DisableVideoWithBlackColor();
    int EnableVideoAuto();
    int EnableVideoNow();
    int EnableVideoWhenVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int WaittingVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int EnableVideoBlackout();
    int DisableVideoBlackout();
    int getVideoDisableValue();
    int SetVideoLayerDisable ( int value );
    int ClearVideoBuffer();
    bool videoIsPlaying(int minFrameCount = 8);
    int setVideoScreenMode ( int value );
    int getVideoScreenMode();
    int setVideoAxis ( int h, int v, int width, int height );
    video_display_resolution_t getVideoDisplayResolution();
    //LR_switch: 1 is  enable,    3D_TO_2D:1 is L  , 2 is R
    int set3DMode(VIDEO_3D_MODE_T mode, int LR_switch, int mode_3D_TO_2D);
    //
    int setLookupPtsForDtmb(int enable);
    tvin_sig_fmt_t getVideoResolutionToFmt();
private:
    static void av_evt_callback ( long dev_no, int event_type, void *param, void *user_data );
    int getVideoFrameCount();
    int mTvPlayDevId;
    IObserver *mpObserver;
    AVEvent mCurAvEvent;
    int mCurVideoLayerMuteState;
    int mCurDisableVideoColor;

    int mFdAmVideo;
};
#endif

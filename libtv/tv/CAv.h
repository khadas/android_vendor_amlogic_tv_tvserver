#ifndef _C_AV_H
#define _C_AV_H

#include "am_av.h"
#include "am_aout.h"
#include "CTvEv.h"
#include "CTvLog.h"
#include "../tvin/CTvin.h"

static const char *PATH_FRAME_COUNT_DI  = "/sys/module/di/parameters/frame_count";
static const char *PATH_FRAME_COUNT_VIDEO = "/sys/module/amvideo/parameters/new_frame_count";
static const char *PATH_FRAME_COUNT = PATH_FRAME_COUNT_VIDEO;

static const char *PATH_VIDEO_AMVIDEO = "/dev/amvideo";
static const char *PATH_MEPG_DTMB_LOOKUP_PTS_FLAG = "/sys/module/amvdec_mpeg12/parameters/dtmb_flag";

#define VIDEO_RGB_SCREEN    "/sys/class/video/rgb_screen"
#define VIDEO_TEST_SCREEN   "/sys/class/video/test_screen"
#define VIDEO_DISABLE_VIDEO "/sys/class/video/disable_video"
#define VIDEO_SCREEN_MODE   "/sys/class/video/screen_mode"
#define VIDEO_AXIS          "/sys/class/video/axis"
#define VIDEO_DEVICE_RESOLUTION "/sys/class/video/device_resolution"
#define VIDEO_FREERUN_MODE  "/sys/class/video/freerun_mode"

enum {
    VIDEO_LAYER_NONE                    = -1,
    VIDEO_LAYER_ENABLE                  = 0,
    VIDEO_LAYER_DISABLE_BLACK           = 1,
    VIDEO_LAYER_DISABLE_BLUE            = 2
};

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
    class AVEvent : public CTvEv {
    public:
        AVEvent(): CTvEv(CTvEv::TV_EVENT_AV)
        {

        };
        ~AVEvent()
        {};
        static const int EVENT_AV_STOP             = 1;
        static const int EVENT_AV_RESUEM           = 2;
        static const int EVENT_AV_SCAMBLED         = 3;
        static const int EVENT_AV_UNSUPPORT        = 4;
        static const int EVENT_AV_VIDEO_AVAILABLE  = 5;
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
    int GetAudioStatus(AM_AV_AudioStatus_t *status);
    int SwitchTSAudio(int apid, AM_AV_AFormat_t afmt);
    int ResetAudioDecoder();
    int SetADAudio(uint enable, int apid, AM_AV_AFormat_t afmt);
    int SetTSSource(AM_AV_TSSource_t ts_source);
    int StartTS(uint16_t vpid, uint16_t apid, uint16_t pcrid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt);
    int StopTS();
    int AudioGetOutputMode(AM_AOUT_OutputMode_t *mode);
    int AudioSetOutputMode(AM_AOUT_OutputMode_t mode);
    int AudioSetPreGain(float pre_gain);
    int AudioGetPreGain(float *gain);
    int AudioSetPreMute(uint mute);
    int AudioGetPreMute(uint *mute);
    int SetVideoScreenColor (int vdin_blending_mask, int y, int u, int v);
    int DisableVideoWithBlueColor();
    int DisableVideoWithBlackColor();
    int EnableVideoAuto();
    int EnableVideoNow(bool IsShowTestScreen);
    int EnableVideoWhenVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int WaittingVideoPlaying(int minFrameCount = 8, int waitTime = 5000);
    int EnableVideoBlackout();
    int DisableVideoBlackout();
    int SetVideoLayerDisable ( int value );
    int ClearVideoBuffer();
    bool videoIsPlaying(int minFrameCount = 8);
    int setVideoScreenMode ( int value );
    int setVideoAxis ( int h, int v, int width, int height );
    video_display_resolution_t getVideoDisplayResolution();
    int setRGBScreen(int r, int g, int b);
    int getRGBScreen();

    int setLookupPtsForDtmb(int enable);
    tvin_sig_fmt_t getVideoResolutionToFmt();
private:
    static void av_evt_callback ( long dev_no, int event_type, void *param, void *user_data );
    int getVideoFrameCount();
    int mTvPlayDevId;
    IObserver *mpObserver;
    AVEvent mCurAvEvent;
    int mVideoLayerState;

    int mFdAmVideo;
};
#endif

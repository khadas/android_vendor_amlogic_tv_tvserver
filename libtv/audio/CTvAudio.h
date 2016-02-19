#ifndef __TV_AUDIO_API_H__
#define __TV_AUDIO_API_H__


#include "tvsetting/audio_cfg.h"
#include "audio_alsa.h"
#include "audio_effect.h"
#include <audio_amaudio.h>
#include "CAudioCustomerCtrl.h"

enum CC_AUDIO_SWITCH_STATUS {
    CC_SWITCH_OFF,
    CC_SWITCH_ON,
};

enum CC_AMAUDIO_OUT_MODE {
    CC_AMAUDIO_OUT_MODE_DIRECT,
    CC_AMAUDIO_OUT_MODE_INTER_MIX,
    CC_AMAUDIO_OUT_MODE_DIRECT_MIX,
};

static const  int  CC_SPDIF_MODE_PCM = 0;
static const int CC_SPDIF_MODE_RAW = 1;

static const int  CC_MIN_SOUND_VOL = 0;
static const int  CC_MAX_SOUND_VOL = 100;
static const int  CC_DEF_SOUND_VOL = 10;
static const int  CC_MIN_SUPPERBASS_VOL = 0;
static const int  CC_MAX_SUPPERBASS_VOL = 100;
static const int  CC_DEF_SUPPERBASS_VOL = 50;

static const int  CC_DEF_BASS_TREBLE_VOL            =      50;
static const int  CC_EQ_BASS_IND               =           1;
static const int  CC_EQ_TREBLE_IND           =             3;
static const int  CC_EQ_DEF_UI_MIN_GAIN     =              0;
static const int  CC_EQ_DEF_UI_MAX_GAIN      =             100;
static const int  CC_MIN_EQ_GAIN_VAL        = -10;
static const int  CC_MAX_EQ_GAIN_VAL        = 10;

static const  int CC_LUT_BUF_MASTER = 0;
static const  int CC_LUT_BUF_SUPPERBASS = 1;
static const  int CC_LUT_BUF_SIZE = 101;
static const  int CC_NO_LINE_POINTS_MAX_CNT = 101;


enum CC_AUD_SOUND_MODE {
    CC_SOUND_MODE_START = 0,
    CC_SOUND_MODE_STD = 0,
    CC_SOUND_MODE_MUSIC,
    CC_SOUND_MODE_NEWS,
    CC_SOUND_MODE_THEATER,
    CC_SOUND_MODE_USER,
    CC_SOUND_MODE_END = CC_SOUND_MODE_USER
};

enum CC_AUD_EQ_MODE {
    CC_EQ_MODE_START = 0,
    CC_EQ_MODE_NOMAL = 0,
    CC_EQ_MODE_POP,
    CC_EQ_MODE_JAZZ,
    CC_EQ_MODE_ROCK,
    CC_EQ_MODE_CLASSIC,
    CC_EQ_MODE_DANCE,
    CC_EQ_MODE_PARTY,
    CC_EQ_MODE_BASS,
    CC_EQ_MODE_TREBLE,
    CC_EQ_MODE_CUSTOM,
    CC_EQ_MODE_END = CC_EQ_MODE_CUSTOM
};

class CTvAudio {
public:
    CTvAudio();
    ~CTvAudio();
};
#endif  //__TV_AUDIO_API_H__

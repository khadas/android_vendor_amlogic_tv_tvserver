#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "audio_amaudio.h"
#include "audio_effect.h"

#define LOG_TAG "CAudioEffect"
#include "CTvLog.h"

CAudioEffect::CAudioEffect()
{
}

CAudioEffect::~CAudioEffect()
{
}

int CAudioEffect::GetEQBandCount()
{
    return CC_BAND_ITEM_CNT;
}

int CAudioEffect::SetEQSwitch(int switch_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetEQEnable(switch_val);
    return tmp_ret;
}

int CAudioEffect::GetEQSwitch()
{
    int tmp_ret = 0;
    return 0;
}

int CAudioEffect::SetEQValue(int gain_val_buf[])
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetEQGain(gain_val_buf, CC_BAND_ITEM_CNT);
    return tmp_ret;
}

int CAudioEffect::GetEQValue(int gain_val_buf[] __unused)
{
    return 0;
}

int CAudioEffect::SetSrsSurroundSwitch(int switch_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSSurroundSwitch(switch_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsSurroundGain(int gain_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSSurroundGain(gain_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsInputOutputGain(int input_gain_val, int output_gain_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSGain(input_gain_val, output_gain_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsTruBassSwitch(int switch_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSTrubassSwitch(switch_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsTruBassGain(int gain_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSTrubassGain(gain_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsDialogClaritySwitch(int switch_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSDialogClaritySwitch(switch_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsDialogClarityGain(int gain_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSDialogClarityGain(gain_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsDefinitionGain(int gain_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSDefinitionGain(gain_val);
    return tmp_ret;
}

int CAudioEffect::SetSrsTrubassSpeakerSize(int set_val)
{
    int tmp_ret = 0;
    tmp_ret |= amAudioSetSRSTrubassSpeakerSize(set_val);
    return tmp_ret;
}

int CAudioEffect::DbxTv_SetMode(int mode __unused, int son_value, int vol_value, int sur_value)
{
    son_value = son_value;
    vol_value = vol_value;
    sur_value = sur_value;

    int ret = 0;
#ifdef TV_AUDIO_USE_DBX_TV
    ret |= amAudioSetDBXTVParameter( son_value, vol_value, sur_value);
#endif
    return ret;
}

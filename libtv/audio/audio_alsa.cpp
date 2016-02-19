#define LOG_TAG "CAudioAlsa"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <tinyalsa/asoundlib.h>
#include "CFbcCommunication.h"
#include <sys/poll.h>
#include "tvsetting/audio_cfg.h"
#include "audio_alsa.h"
#include <cutils/properties.h>
#include "CTvLog.h"
#include "audio_amaudio.h"

static const int DEFAULT_MAIN_DIGIT_LUT[] = {
    //
    0, 40, 50, 60, 70, 87, 110, 120, 130, 140, // 0~9
    150, 152, 155, 158, 161, 164, 167, 170, 173, 174, // 10~19
    179, 180, 181, 182, 183, 184, 185, 186, 187, 188, // 20~29
    191, 191, 192, 193, 194, 195, 196, 196, 197, 197, // 30~39
    198, 198, 198, 199, 199, 200, 200, 201, 201, 201, // 40~49
    202, 202, 202, 203, 203, 203, 203, 204, 204, 204, // 50~59
    205, 205, 205, 205, 206, 206, 206, 206, 206, 207, // 60~69
    207, 207, 207, 207, 207, 207, 207, 208, 208, 208, // 70~79
    208, 208, 208, 208, 208, 208, 208, 208, 208, 209, // 80~89
    209, 209, 209, 209, 209, 209, 209, 209, 209, 209, // 90~99
    209, // 100
};

static const int DEFAULT_SUPPER_BASS_DIGIT_LUT_BUF[] = {
    //
    0, 122, 123, 124, 125, 126, 127, 128, 129, 130, // 0~9
    131, 132, 133, 134, 135, 136, 137, 138, 139, 140, // 10~19
    141, 142, 143, 144, 145, 147, 147, 148, 148, 149, // 20~29
    149, 150, 150, 151, 151, 152, 152, 153, 153, 154, // 30~39
    154, 155, 155, 156, 156, 157, 157, 158, 158, 159, // 40~49
    160, 160, 160, 160, 161, 161, 161, 161, 162, 162, // 50~59
    162, 163, 163, 163, 163, 164, 164, 164, 165, 165, // 60~69
    165, 165, 166, 166, 166, 167, 168, 169, 170, 171, // 70~79
    173, 174, 175, 176, 177, 179, 180, 181, 182, 183, // 80~89
    185, 186, 187, 188, 189, 191, 192, 193, 194, 195, // 90~99
    197 // 100
};

CAudioAlsa::CAudioAlsa()
{
    int card_id;
    card_id = get_aml_card();
    LOGD("[%s:%d] card_id:%d\n", __FUNCTION__, __LINE__, card_id);
    mpMixer = mixer_open(card_id);

    card_id = get_USB_Audio_card();
    LOGD("[%s:%d] card_id:%d\n", __FUNCTION__, __LINE__, card_id);
    mpUsbMixer = mixer_open(card_id);
    mMainVolumeGainVal = 0;
    mSupperBassVolumeGainVal = 0;
    ArrayCopy(mMainDigitLutBuf, DEFAULT_MAIN_DIGIT_LUT, CC_VOL_TRANS_LUT_BUF_SIZE);
    ArrayCopy(mSupperBassDigitLutBuf, DEFAULT_SUPPER_BASS_DIGIT_LUT_BUF, CC_VOL_TRANS_LUT_BUF_SIZE);
}

CAudioAlsa::~CAudioAlsa()
{
    if (NULL != mpMixer) {
        mixer_close(mpMixer);
    }

    if (NULL != mpUsbMixer) {
        mixer_close(mpUsbMixer);
    }
}

int CAudioAlsa::get_aml_card()
{
    int card = -1, err = 0;
    int fd = -1;
    char read_buf[512], *pd = NULL;
    static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    memset(read_buf, 0x0, 512);
    err = read(fd, read_buf, 512);
    if (fd < 0) {
        LOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';
    close(fd);
    return card;
}

int CAudioAlsa::AudioControlSetValue(int val_count, int data_buf[], char *match_names)
{
    struct mixer_ctl *pctl;
    unsigned int expected_val_count;
    unsigned int value_index;

    if (NULL == mpMixer) {
        LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }

    pctl = mixer_get_ctl_by_name(mpMixer, match_names);
    if (NULL == pctl) {
        LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names);
        goto err_exit;
    }
    expected_val_count = mixer_ctl_get_num_values(pctl);
    if (expected_val_count != (unsigned int)val_count) {
        LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }
    for (value_index = 0; value_index < expected_val_count; value_index++) {
        if (mixer_ctl_set_value(pctl, value_index, data_buf[value_index]) != 0) {
            LOGE("[%s:%d] Failed to set value:%d\n", __FUNCTION__, __LINE__, value_index);
            goto err_exit;
        }
    }
    return 0;
err_exit:
    return -1;
}

int CAudioAlsa::AudioControlGetValue(int val_count, int ret_buf[], char *match_names)
{
    struct mixer_ctl *pctl;
    unsigned int expected_val_count;
    unsigned int value_index;

    if (NULL == mpMixer) {
        LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }
    pctl = mixer_get_ctl_by_name(mpMixer, match_names);
    if (NULL == pctl) {
        LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names);
        goto err_exit;
    }
    expected_val_count = mixer_ctl_get_num_values(pctl);
    if (expected_val_count != (unsigned int)val_count) {
        LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }
    for (value_index = 0; value_index < expected_val_count; value_index++) {
        ret_buf[value_index] = mixer_ctl_get_value(pctl, value_index);
    }
    return 0;
err_exit:
    return -1;
}

int CAudioAlsa::get_USB_Audio_card()
{
    int card = -1, err = 0;
    int fd = -1;
    char read_buf[512], *pd = NULL;
    static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    memset(read_buf, 0x0, 512);
    err = read(fd, read_buf, 512);
    if (fd < 0) {
        LOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    pd = strstr(read_buf, "Receiver");
    if (pd == NULL) {
        LOGE("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }

    card = *(pd - 3) - '0';
    close(fd);
    return card;
}

int CAudioAlsa::HandleUSBAudioControlValue(int val_count, int data_buf[], int match_count, char **match_names)
{

    struct mixer_ctl *pctl;
    unsigned int expected_val_count;
    unsigned int match_index;
    unsigned int value_index;
    char card_id_str[9] = {0};

    if (NULL == mpUsbMixer) {
        LOGE("[%s:%d] Failed to open mixer\n", __FUNCTION__, __LINE__);
        goto err_exit;
    }

    for (match_index = 0; match_index < (unsigned int)match_count; match_index++) {
        pctl = mixer_get_ctl_by_name(mpUsbMixer, match_names[match_index]);
        if (NULL == pctl) {
            LOGE("[%s:%d] Failed to get mixer control for:%s\n", __FUNCTION__, __LINE__, match_names[match_index]);
            goto err_exit;
        }
        expected_val_count = mixer_ctl_get_num_values(pctl);
        if (expected_val_count != (unsigned int)val_count) {
            LOGE("[%s:%d] val_count != expected_val_count\n", __FUNCTION__, __LINE__);
            goto err_exit;
        }
        for (value_index = 0; value_index < expected_val_count; value_index++) {
            if (mixer_ctl_set_value(pctl, value_index, data_buf[value_index]) != 0) {
                LOGE("[%s:%d] Failed to set value:%d\n", __FUNCTION__, __LINE__, value_index);
                goto err_exit;
            }
        }
    }

    return 0;
err_exit:
    return -1;
}

int CAudioAlsa::CheckVolume(int digit_vol, int digit_min __unused, int digit_max __unused,
                            int hd_min, int hd_max)
{
    if (digit_vol < hd_min) {
        return hd_min;
    } else if (digit_vol > hd_max) {
        return hd_max;
    }

    return digit_vol;
}

int CAudioAlsa::GetTwoChannelVolume(int vol_buf[],
                                    int l_min_vol __unused, int l_max_vol __unused,
                                    int r_min_vol __unused, int r_max_vol __unused,
                                    char *match_names,
                                    int hd_min __unused, int hd_max __unused)
{
    vol_buf[0] = 0;
    vol_buf[1] = 0;
    return AudioControlGetValue(2, vol_buf, match_names);
}

int CAudioAlsa::GetLineInMaxVol()
{
    return 127;
}

int CAudioAlsa::GetLineOutMaxVol()
{
    return 255;
}

#define CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME                          (0)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_PGA_IN_GAIN                          (1)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_ADC_DIGITAL_CAPTURE_VOLUME_NAME      (2)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_DAC_DIGITAL_PLAYBACK_VOLUME_NAME     (3)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LEFT_LINEIN_SEL_NAME                 (4)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_RIGHT_LINEIN_SEL_NAME                (5)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME           (6)
#define CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME         (7)
#define CC_GET_ALSA_CTL_I2S_MUTE_NAME                                     (8)
#define CC_GET_ALSA_CTL_SPDIF_MUTE_NAME                                   (9)
#define CC_GET_ALSA_CTL_HW_RESAMPLE_NAME                                  (10)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_MASTER_VOLUME_NAME                   (11)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_VOLUME_NAME                      (12)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_VOLUME_NAME                      (13)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_VOLUME_NAME                      (14)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_SWITCH_NAME                      (15)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_SWITCH_NAME                      (16)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_SWITCH_NAME                      (17)
#define CC_GET_ALSA_CTL_EXTERNAL_DAC_EQ_MODE_NAME                         (18)
#define CC_GET_ALSA_CTL_PCM_PLAYBACK_VOLUME                               (19)
#define CC_GET_ALSA_CTL_PCM_PLAYBACK_SWITCH                               (20)
#define CC_GET_ALSA_CTL_SETOUTPUT_SWAP                                    (21)
#define CC_GET_ALSA_CTL_AUDIO_IN_SWITCH                                   (22)

static  char  gG9AlsaNames[32][48] = {
    {"Audio In Source"},//0
    {"PGA IN Gain"},//1
    {"ADC Digital Capture Volume"},//2
    {"DAC Digital Playback Volume"},//3
    {"Linein right switch"},//4
    {"Linein left switch"},
    {"ADC Digital Capture Volume"},
    {"DAC Digital Playback Volume"},
    {"Audio i2s mute"},//8
    {"Audio spdif mute"},
    {"Hardware resample enable"},//10
    {"AMP Master Volume"},
    {"AMP Ch1 Volume"},//12
    {"AMP Ch2 Volume"},
    {"AMP Ch3 Volume"},
    {"AMP Ch1 Switch"},
    {"AMP Ch2 Switch"},
    {"AMP Ch3 Switch"},//17
    {"AMP EQ Mode"},
    {"PCM Playback Volume"},
    {"PCM Playback Switch"},
    {"Output Swap"},
    {"AudioIn Switch"},//22
};


char *CAudioAlsa::GetAlsaControlName(int get_type)
{
    return gG9AlsaNames[get_type];
}


int CAudioAlsa::SetAudioInSource(int source_type)
{
    int set_val = 0;
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME);
    set_val = source_type;
    return AudioControlSetValue(1, &set_val, match_names);
}

int CAudioAlsa::GetAudioInSource(void)
{
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_AUDIO_IN_SOURCE_NAME);
    int source_type = 0;
    AudioControlGetValue(1, &source_type, match_names);
    return source_type;
}

int CAudioAlsa::SetAudioInternalDacPGAInGain(int gain_l_val, int gain_r_val)
{
    int GainBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_PGA_IN_GAIN);

    GainBuf[0] = gain_l_val;
    GainBuf[1] = gain_r_val;
    if (gain_l_val < 0 || gain_l_val > 31) {
        GainBuf[0] = 16;
    }
    if (gain_r_val < 0 || gain_r_val > 31) {
        GainBuf[1] = 16;
    }

    return AudioControlSetValue(2, GainBuf, match_names);
}

int CAudioAlsa::GetAudioInternalDacPGAInGain(int *gain_l_val, int *gain_r_val)
{
    int GainBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_PGA_IN_GAIN);

    AudioControlGetValue(2, GainBuf, match_names);

    *gain_l_val = GainBuf[0];
    *gain_r_val = GainBuf[1];

    return 0;
}

int CAudioAlsa::SetAudioInternalDacADCDigitalCaptureVolume(int vol_l_val, int vol_r_val)
{
    int VolumeBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_ADC_DIGITAL_CAPTURE_VOLUME_NAME);

    VolumeBuf[0] = vol_l_val;
    VolumeBuf[1] = vol_r_val;
    if (vol_l_val < 0 || vol_l_val > 127) {
        VolumeBuf[0] = 82;
    }
    if (vol_r_val < 0 || vol_r_val > 127) {
        VolumeBuf[1] = 82;
    }

    return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetAudioInternalDacADCDigitalCaptureVolume(int *vol_l_val, int *vol_r_val)
{
    int VolumeBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_ADC_DIGITAL_CAPTURE_VOLUME_NAME);

    AudioControlGetValue(2, VolumeBuf, match_names);

    *vol_l_val = VolumeBuf[0];
    *vol_r_val = VolumeBuf[1];

    return 0;
}

int CAudioAlsa::SetAudioInternalDacDACDigitalPlayBackVolume(int vol_l_val, int vol_r_val)
{
    int VolumeBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_DAC_DIGITAL_PLAYBACK_VOLUME_NAME);

    VolumeBuf[0] = vol_l_val;
    VolumeBuf[1] = vol_r_val;
    if (vol_l_val < 0 || vol_l_val > 255) {
        VolumeBuf[0] = 255;
    }
    if (vol_r_val < 0 || vol_r_val > 255) {
        VolumeBuf[1] = 255;
    }

    return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetAudioInternalDacDACDigitalPlayBackVolume(int *vol_l_val, int *vol_r_val)
{
    int VolumeBuf[2] = {0, 0};
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_DAC_DIGITAL_PLAYBACK_VOLUME_NAME);

    AudioControlGetValue(2, VolumeBuf, match_names);

    *vol_l_val = VolumeBuf[0];
    *vol_r_val = VolumeBuf[1];

    return 0;
}

int CAudioAlsa::SetInternalDacLineInSelectChannel(int line_in_number)
{
    int tmp_ret = 0;
    char *match_names = NULL;

    if (line_in_number < 0 || line_in_number > 7) {
        return -1;
    }

    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LEFT_LINEIN_SEL_NAME);
    tmp_ret |= AudioControlSetValue(1, &line_in_number, match_names);

    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_RIGHT_LINEIN_SEL_NAME);
    tmp_ret |= AudioControlSetValue(1, &line_in_number, match_names);

    return tmp_ret;
}

int CAudioAlsa::SetInternalDacLineInCaptureVolume(int l_vol, int r_vol)
{
    int max_val = GetLineInMaxVol();
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME);
    int VolumeBuf[2];

    VolumeBuf[0] = CheckVolume(l_vol, 0, max_val, 0, max_val);
    VolumeBuf[1] = CheckVolume(r_vol, 0, max_val, 0, max_val);

    return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetInternalDacLineInCaptureVolume(int vol_buf[])
{
    int max_val = GetLineInMaxVol();
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEIN_CAPTURE_VOLUME_NAME);

    return GetTwoChannelVolume(vol_buf, 0, max_val, 0, max_val, match_names, 0, max_val);
}

int CAudioAlsa::SetInternalDacLineOutPlayBackVolume(int l_vol, int r_vol)
{
    int max_val = GetLineOutMaxVol();
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME);
    int VolumeBuf[2];

    VolumeBuf[0] = CheckVolume(l_vol, 0, max_val, 0, max_val);
    VolumeBuf[1] = CheckVolume(r_vol, 0, max_val, 0, max_val);

    return AudioControlSetValue(2, VolumeBuf, match_names);
}

int CAudioAlsa::GetInternalDacLineOutPlayBackVolume(int vol_buf[])
{
    int max_val = GetLineOutMaxVol();
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_INTERNAL_DAC_LINEOUT_PLAYBACK_VOLUME_NAME);

    return GetTwoChannelVolume(vol_buf, 0, max_val, 0, max_val, match_names, 0, max_val);
}

int CAudioAlsa::SetAudioPcmPlaybackVolume(int vol)
{
    if (get_USB_Audio_card() == -EINVAL) return 0;
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_PCM_PLAYBACK_VOLUME);
    return HandleUSBAudioControlValue(1, &vol, 1, &match_names);
}

int CAudioAlsa::SetAudioPcmPlaybackSwitch(int vol)
{
    if (get_USB_Audio_card() == -EINVAL) return 0;
    char *match_names = GetAlsaControlName(CC_GET_ALSA_CTL_PCM_PLAYBACK_SWITCH);
    return HandleUSBAudioControlValue(1, &vol, 1, &match_names);
}

int CAudioAlsa::SetExternalDacChannelSwitch(int chan_ind, int switch_val)
{
    char *match_names = NULL;

    if (chan_ind == 1) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_SWITCH_NAME);
    } else if (chan_ind == 2) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_SWITCH_NAME);
    } else if (chan_ind == 3) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_SWITCH_NAME);
    } else {
        return -1;
    }

    return AudioControlSetValue(1, &switch_val, match_names);
}

int CAudioAlsa::SetExternalDacChannelVolume(int chan_ind, int main_vol)
{
    int tmp_ret = 0;
    char *match_names = NULL;

    if (chan_ind == 0) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_MASTER_VOLUME_NAME);
    } else if (chan_ind == 1) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH1_VOLUME_NAME);
    } else if (chan_ind == 2) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH2_VOLUME_NAME);
    } else if (chan_ind == 3) {
        match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_CH3_VOLUME_NAME);
    } else {
        return -1;
    }

    return AudioControlSetValue(1, &main_vol, match_names);
}
int CAudioAlsa::SetAudioSwitchIO(int value)
{
    char *match_names = gG9AlsaNames[CC_GET_ALSA_CTL_AUDIO_IN_SWITCH];

    return AudioControlSetValue(1, &value, match_names);
}
int CAudioAlsa::SetOutput_Swap(int value)
{
    char *match_names = gG9AlsaNames[CC_GET_ALSA_CTL_SETOUTPUT_SWAP];

    return AudioControlSetValue(1, &value, match_names);
}

int CAudioAlsa::SetExternalDacEQMode(int mode_val)
{
    char *match_names = NULL;

    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_EXTERNAL_DAC_EQ_MODE_NAME);
    return AudioControlSetValue(1, &mode_val, match_names);
}

int CAudioAlsa::SetI2SMute(int mute_status)
{
    char *match_names = NULL;

    if (mute_status != CC_I2S_MUTE_ON && mute_status != CC_I2S_MUTE_OFF) {
        LOGE("%s, I2S mute value (%d) error!\n", __FUNCTION__, mute_status);
        return -1;
    }

    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_I2S_MUTE_NAME);
    return AudioControlSetValue(1, &mute_status, match_names);
}

int CAudioAlsa::SetSPDIFMute(int mute_status)
{
    int set_val = 0;
    char *match_names = NULL;

    if (mute_status == CC_AUDIO_MUTE) {
        set_val = CC_SPDIF_MUTE_ON;
    } else if (mute_status == CC_AUDIO_UNMUTE) {
        set_val = CC_SPDIF_MUTE_OFF;
    } else {
        return -1;
    }
    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_SPDIF_MUTE_NAME);
    return AudioControlSetValue(1, &mute_status, match_names);
}

#define CC_SAMPLE_BUF_SIZE    (32)

enum CC_HW_RESAMPLE_TYPE {
    CC_HW_RESAMPLE_DISABLE,
    CC_HW_RESAMPLE_48K,
    CC_HW_RESAMPLE_44K,
    CC_HW_RESAMPLE_32K,
};

int CAudioAlsa::SetHardwareResample(int sr)
{
    int i = 0, set_val = 0, tmp_val = 0;
    int diff_val = 0x7FFFFFFF, diff_ind = -1;
    char *match_names = NULL;
    int sample_buf[CC_SAMPLE_BUF_SIZE] = {48000, 44100, 32000, -1};

    if (sr < 0) {
        set_val = 0;
    } else {
        for (i = 0; i < CC_SAMPLE_BUF_SIZE; i++) {
            if (sample_buf[i] < 0) {
                break;
            }

            if (sample_buf[i] >= sr) {
                tmp_val = sample_buf[i] - sr;
            } else {
                tmp_val = sr - sample_buf[i];
            }

            if (tmp_val <= diff_val) {
                diff_val = tmp_val;
                diff_ind = i;
            }
        }

        if (diff_ind < 0) {
            set_val = 0;
        } else {
            set_val = diff_ind + 1;
        }
    }

    LOGD("%s, set_val = %d.\n", __FUNCTION__, set_val);
    match_names = GetAlsaControlName(CC_GET_ALSA_CTL_HW_RESAMPLE_NAME);
    return AudioControlSetValue(1, &set_val, match_names);
}

int CAudioAlsa::SetMixerBypassSwitch(int switch_val)
{
    char match_names[48] = "Output Mixer Bypass Switch" ;
    int ctl_buf[2];

    if (switch_val != 0 && switch_val != 1) {
        return -1;
    }

    ctl_buf[0] = switch_val;
    ctl_buf[1] = switch_val;

    return AudioControlSetValue(2, ctl_buf, match_names);
}

int CAudioAlsa::GetMixerBypassSwitch(void)
{
    char match_names[48] = "Output Mixer Bypass Switch" ;
    int ctl_buf[2];

    AudioControlSetValue(2, ctl_buf, match_names);

    return ctl_buf[0];
}

int CAudioAlsa::SetMixerDacSwitch(int switch_val)
{
    char match_names[48] = "Output Mixer DAC Switch" ;
    int ctl_buf[2];

    if (switch_val != 0 && switch_val != 1) {
        return -1;
    }

    ctl_buf[0] = switch_val;
    ctl_buf[1] = switch_val;

    return AudioControlSetValue(2, ctl_buf, match_names);
}

int CAudioAlsa::GetMixerDacSwitch(void)
{
    int ctl_buf[2];
    AudioControlGetValue(2, ctl_buf, (char *)"Output Mixer DAC Switch");
    return ctl_buf[0];
}

int CAudioAlsa::TransVolumeBarVolToDigitalVol(int digit_lut_buf[], int digit_vol)
{
    return digit_lut_buf[digit_vol];
}

int CAudioAlsa::TransDigitalVolToVolumeBarVol(int digit_lut_buf[], int hd_vol,
        int hd_min __unused, int hd_max __unused,
        int digit_min __unused, int digit_max)
{
    int i = 0;
    for (; i < CC_VOL_TRANS_LUT_BUF_SIZE - 1; i++) {
        if (digit_lut_buf[i] >= hd_vol && digit_lut_buf[i + 1] < hd_vol)
            break;
    }

    if (i < CC_VOL_TRANS_LUT_BUF_SIZE - 1)
        return i;

    return digit_max;
}

void CAudioAlsa::SetMainVolDigitLutBuf(int digit_lut_buf[])
{
    memcpy((void *) mMainDigitLutBuf, digit_lut_buf,
           CC_VOL_TRANS_LUT_BUF_SIZE * sizeof(int));
}

int *CAudioAlsa::GetMainVolDigitLutBuf()
{
    return mMainDigitLutBuf;
}
void CAudioAlsa::SetSupperBassVolDigitLutBuf(int digit_lut_buf[])
{
    memcpy((void *) mSupperBassDigitLutBuf, digit_lut_buf,
           CC_VOL_TRANS_LUT_BUF_SIZE * sizeof(int));
}


int CAudioAlsa::SetMainVolumeGain(int gain_val)
{
    mMainVolumeGainVal = gain_val;
    return mMainVolumeGainVal;
}

int CAudioAlsa::GetMainVolumeGain()
{
    return mMainVolumeGainVal;
}

int CAudioAlsa::SetSupperBassVolumeGain(int gain_val)
{
    int tmp_val = 0;
    tmp_val = mSupperBassVolumeGainVal;
    mSupperBassVolumeGainVal = gain_val;
    return tmp_val;
}

int CAudioAlsa::GetSupperBassVolumeGain()
{
    int tmp_val = 0;
    tmp_val = mSupperBassVolumeGainVal;
    return tmp_val;
}


int CAudioAlsa::CalculateBalanceVol(int max_vol, int balance_val, int vol_buf[])
{
    int bal_mid_vol = 0, bal_cal_len = 0;
    int tmp_val = 0;

    vol_buf[0] = max_vol;
    vol_buf[1] = max_vol;

    bal_mid_vol = (CC_MIN_SOUND_BALANCE_VAL + CC_MAX_SOUND_BALANCE_VAL) / 2;
    bal_cal_len = (CC_MAX_SOUND_BALANCE_VAL - CC_MIN_SOUND_BALANCE_VAL) / 2;

    if (balance_val == bal_mid_vol) {
        LOGD(
            "%s, balance value = %d, bal_mid_vol = %d, vol_buf[0] = %d, vol_buf[1] = %d.\n",
            __FUNCTION__, balance_val, bal_mid_vol, vol_buf[0], vol_buf[1]);
        return 0;
    } else if (balance_val < bal_mid_vol) {
        vol_buf[1] = (bal_cal_len - (bal_mid_vol - balance_val)) * max_vol / bal_cal_len;
    } else if (balance_val > bal_mid_vol) {
        vol_buf[0] = (bal_cal_len - (balance_val - bal_mid_vol)) * max_vol / bal_cal_len;
    }

    if (GetAudioAmplifierBalanceExchangeCFG() != 0) {
        tmp_val = vol_buf[0];
        vol_buf[0] = vol_buf[1];
        vol_buf[1] = tmp_val;
    }

    LOGD(
        "%s, balance value = %d, bal_mid_vol = %d, vol_buf[0] = %d, vol_buf[1] = %d.\n",
        __FUNCTION__, balance_val, bal_mid_vol, vol_buf[0], vol_buf[1]);
    return 0;
}


int CAudioAlsa::SetInternalDacMute(int mute_state)
{
    int ret = 0;
    if (mute_state == CC_AUDIO_MUTE)
        ret = SetAudioInternalDacDACDigitalPlayBackVolume(0, 0);
    else
        ret = SetAudioInternalDacDACDigitalPlayBackVolume(255, 255);
    return ret;
}

int CAudioAlsa::setAudioPcmPlaybackMute(int mute_state)
{
    char prop[256];
    property_get("audio.output.double_output", prop, "null");
    if ( Get2d4gHeadsetEnable() == 1 && (strcmp(prop, "0") == 0 || strcmp(prop, "null") == 0)) {
        SetAudioPcmPlaybackSwitch(mute_state);
    }
    return 0;
}


int CAudioAlsa::SetInternalDacMainVolume(int main_vol)
{
    int tmp_ret = 0, digit_vol = 0;
    digit_vol = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, main_vol);

    return SetInternalDacLineOutPlayBackVolume(digit_vol, digit_vol);
}


static int gDigitalMuteStatus = CC_AUDIO_UNMUTE;
static int gDigitalLeftVol = 0;
static int gDigitalRightVol = 0;

int CAudioAlsa::SetDigitalVolume(int l_val, int r_val)
{
    amAudioSetLeftGain(l_val);
    amAudioSetRightGain(r_val);
    LOGD("%s, l_val = %d, r_val = %d.\n", __FUNCTION__, l_val, r_val);
    return 0;
}

int CAudioAlsa::SetDigitalMainVolume(int l_vol, int r_vol)
{
    int tmp_ret = 0, l_val = 0, r_val = 0;

    //handle l&r channel volume for balance
    l_val = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, l_vol);
    r_val = TransVolumeBarVolToDigitalVol(mMainDigitLutBuf, r_vol);

    gDigitalLeftVol = l_val;
    gDigitalRightVol = r_val;
    if (gDigitalMuteStatus == CC_AUDIO_UNMUTE) {
        SetDigitalVolume(l_val, r_val);
    }

    return tmp_ret;
}

int CAudioAlsa::SetDigitalMute(int mute_status)
{
    gDigitalMuteStatus = mute_status;
    if (mute_status == CC_AUDIO_MUTE) {
        SetDigitalVolume(0, 0);
    } else if (mute_status == CC_AUDIO_UNMUTE) {
        SetDigitalVolume(gDigitalLeftVol, gDigitalRightVol);
    }

    return 0;
}


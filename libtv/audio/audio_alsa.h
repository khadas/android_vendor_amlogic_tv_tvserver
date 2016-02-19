#ifndef __TV_AUDIO_ALSA_H__
#define __TV_AUDIO_ALSA_H__

enum CC_AUD_I2S_MUTE {
    CC_I2S_MUTE_OFF,
    CC_I2S_MUTE_ON,
};

enum CC_AUDIO_MUTE_STATUS {
    CC_MUTE_ON,
    CC_MUTE_OFF,
};
enum CC_AUD_SPDIF_MUTE {
    CC_SPDIF_MUTE_OFF,
    CC_SPDIF_MUTE_ON,
};

enum CC_AUD_IN_TYPE {
    CC_AUDIO_IN_SOURCE_LINEIN,
    CC_AUDIO_IN_SOURCE_ATV,
    CC_AUDIO_IN_SOURCE_HDMI,
};


#define CC_MIN_DAC_VOLUME                           (0)
#define CC_MAX_DAC_VOLUME                           (255)
#define CC_MIN_DAC_SUB_WOOFER_VOLUME                (0)
#define CC_MAX_DAC_SUB_WOOFER_VOLUME                (255)

#define CC_MIN_SOUND_BALANCE_VAL                    (0)
#define CC_MAX_SOUND_BALANCE_VAL                    (100)
#define CC_DEF_SOUND_BALANCE_VAL                    (50)

#define CC_DAC_MUTE_TYPE_EXTERNAL                   (1)
#define CC_DAC_MUTE_TYPE_INTERNAL                   (2)

#define CC_VOL_TRANS_LUT_BUF_SIZE                   (101)

static const int  CC_AUDIO_MUTE = 1;
static const int  CC_AUDIO_UNMUTE = 0;

enum CC_AUDIO_SOURCE_TYPE {
    AUDIO_HDMI_SOURCE,
    AUDIO_ATV_SOURCE,
    AUDIO_AV_SOURCE,
    AUDIO_MPEG_SOURCE,
    AUDIO_MAX_SOURCE
};


//use tinyalsa,is a like alsa-lib for android
class CAudioAlsa {
public:
    CAudioAlsa();
    ~CAudioAlsa();
    int SetAudioInSource(int source_type);
    int GetAudioInSource(void);
    int SetAudioInternalDacPGAInGain(int gain_l_val, int gain_r_val);
    int GetAudioInternalDacPGAInGain(int *gain_l_val, int *gain_r_val);
    int SetAudioInternalDacADCDigitalCaptureVolume(int vol_l_val, int vol_r_val);
    int GetAudioInternalDacADCDigitalCaptureVolume(int *vol_l_val, int *vol_r_val);
    int SetAudioInternalDacDACDigitalPlayBackVolume(int vol_l_val, int vol_r_val);
    int GetAudioInternalDacDACDigitalPlayBackVolume(int *vol_l_val, int *vol_r_val);
    int SetInternalDacLineInSelectChannel(int line_in_number);
    int SetInternalDacLineInCaptureVolume(int l_vol, int r_vol);
    int GetInternalDacLineInCaptureVolume(int vol_buf[]);
    int SetInternalDacLineOutPlayBackVolume(int l_vol, int r_vol);
    int GetInternalDacLineOutPlayBackVolume(int vol_buf[]);
    int SetExternalDacChannelSwitch(int chan_ind, int switch_val);
    int SetExternalDacChannelVolume(int chan_ind, int main_vol);
    int SetExternalDacEQMode(int mode_val);
    int SetI2SMute(int mute_status);
    int SetSPDIFMute(int mute_status);
    int SetHardwareResample(int sr);
    int SetAudioSwitchIO(int value);
    int SetAudioPcmPlaybackVolume(int val);
    int SetAudioPcmPlaybackSwitch(int vol);

    int SetOutput_Swap(int value);


    int SetMixerBypassSwitch(int switch_val);
    int GetMixerBypassSwitch(void);
    int SetMixerDacSwitch(int switch_val);
    int GetMixerDacSwitch(void);
    //dac
    void SetMainVolDigitLutBuf(int digit_lut_buf[]);
    int *GetMainVolDigitLutBuf();
    void SetSupperBassVolDigitLutBuf(int digit_lut_buf[]);
    int SetMainVolumeGain(int gain_val);
    int GetMainVolumeGain();
    int SetSupperBassVolumeGain(int gain_val);
    int GetSupperBassVolumeGain();
    int SetInternalDacMute(int);
    int setAudioPcmPlaybackMute(int);
    //end dac
    int TransVolumeBarVolToDigitalVol(int *, int);
    int TransDigitalVolToVolumeBarVol(int *, int, int, int, int, int);
    int CalculateBalanceVol(int, int, int *);
    int SetExternalDacMainVolume(int);
    int SetInternalDacMainVolume(int);
    int SetDigitalVolume(int, int);
    int SetDigitalMainVolume(int, int);
    int SetDigitalMute(int);
private:
    int get_aml_card();
    int AudioControlSetValue(int val_count, int data_buf[], char *match_names);
    int AudioControlGetValue(int val_count, int ret_buf[], char *match_names);
    int get_USB_Audio_card();
    int HandleUSBAudioControlValue(int val_count, int data_buf[], int match_count, char **match_names);
    int CheckVolume(int digit_vol, int digit_min, int digit_max, int hd_min, int hd_max);
    int GetTwoChannelVolume(int vol_buf[], int l_min_vol, int l_max_vol, int r_min_vol, int r_max_vol, char *match_names, int hd_min, int hd_max);
    int GetLineInMaxVol();
    int GetLineOutMaxVol();
    char *GetAlsaControlName(int get_type);
    //
    //
    //mem
    struct mixer *mpMixer;
    struct mixer *mpUsbMixer;
    int mMainVolumeBalanceVal;
    int mMainVolumeGainVal;
    int mSupperBassVolumeGainVal;
    int mMainDigitLutBuf[CC_VOL_TRANS_LUT_BUF_SIZE];
    int mSupperBassDigitLutBuf[CC_VOL_TRANS_LUT_BUF_SIZE];
};
#endif //__TV_AUDIO_ALSA_H__

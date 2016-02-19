#ifndef __TV_AUDIO_EFFECT_H__
#define __TV_AUDIO_EFFECT_H__
#define CC_BAND_ITEM_CNT                ( 6 )
class CAudioEffect {
public:
    CAudioEffect();
    ~CAudioEffect();
    int GetEQBandCount();
    int SetEQSwitch(int switch_val);
    int GetEQSwitch();
    int SetEQValue(int gain_val_buf[]);
    int GetEQValue(int gain_val_buf[]);

    int SetSrsSurroundSwitch(int switch_val);
    int SetSrsInputOutputGain(int input_gain_val, int output_gain_val);
    int SetSrsSurroundGain(int gain_val);
    int SetSrsTruBassSwitch(int switch_val);
    int SetSrsTruBassGain(int gain_val);
    int SetSrsDialogClaritySwitch(int switch_val);
    int SetSrsDialogClarityGain(int gain_val);
    int SetSrsDefinitionGain(int gain_val);
    int SetSrsTrubassSpeakerSize(int tmp_val);
    int DbxTv_SetMode(int mode, int son_value, int vol_value, int sur_value);

private:
};
#endif //__TV_AUDIO_EFFECT_H__

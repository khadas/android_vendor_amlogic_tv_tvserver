//
//
//  amlogic 2015
//
//  @ Project : tv
//  @ File Name :
//  @ Date : 2015 -7
//  @ Author : hao.fu
//
//


#if !defined(_AUTOBACKLIGHT_H)
#define _AUTOBACKLIGHT_H
#include "../tvin/CTvin.h"
#include "../tvutils/CThread.h"
#include "../vpp/CVpp.h"

class AutoBackLight: public CThread {
private:
    tv_source_input_type_t mAutoBacklightSource;
    int mCur_source_default_backlight;
    int mCur_sig_state;
    bool mAutoBacklight_OnOff_Flag;
    int mCurrent_backlight;
    int mCur_dest_backlight;

    void adjustDstBacklight();
    void adjustBacklight();
    int HistogramGet_AVE();
    bool  threadLoop();

public:
    enum SIG_STATE {
        SIG_STATE_STABLE = 1,
        SIG_STATE_NOSIG = 2,
    };

    AutoBackLight();
    ~AutoBackLight();
    void updateSigState(int state);
    void startAutoBacklight( tv_source_input_type_t source_type );
    void stopAutoBacklight();
    bool isAutoBacklightOn();
};
#endif

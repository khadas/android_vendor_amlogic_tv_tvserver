//
//
//  amlogic 2015
//
//  @ Project : tv
//  @ File Name :
//  @ Date : 2015 -9
//  @ Author : hao.fu
//
//


#if !defined(_CAUTOPQPARAM_H)
#define _CAUTOPQPARAM_H
#include "CAv.h"
#include "../tvutils/CThread.h"
#include "../vpp/CVpp.h"
#include "../tvconfig/tvconfig.h"

class CAutoPQparam: public CThread {
private:
    tv_source_input_type_t mAutoPQSource;
    bool mAutoPQ_OnOff_Flag;
    int preFmtType, curFmtType, autofreq_checkcount, autofreq_checkflag;
    int adjustPQparameters();
    bool  threadLoop();

public:

    CAutoPQparam();
    ~CAutoPQparam();
    void startAutoPQ( tv_source_input_type_t source_type );
    void stopAutoPQ();
    bool isAutoPQing();
};
#endif

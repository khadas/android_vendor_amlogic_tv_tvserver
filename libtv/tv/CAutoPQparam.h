#if !defined(_CAUTOPQPARAM_H)
#define _CAUTOPQPARAM_H
#include "CAv.h"
#include <CThread.h>
#include "../vpp/CVpp.h"
#include <tvconfig.h>

class CAutoPQparam: public CThread {
private:
    tv_source_input_t mAutoPQSource;
    bool mAutoPQ_OnOff_Flag;
    int preFmtType, curFmtType, autofreq_checkcount, autofreq_checkflag;
    int adjustPQparameters();
    bool  threadLoop();

public:

    CAutoPQparam();
    ~CAutoPQparam();
    void startAutoPQ( tv_source_input_t tv_source_input );
    void stopAutoPQ();
    bool isAutoPQing();
};
#endif
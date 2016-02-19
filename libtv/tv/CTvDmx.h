#ifndef _C_TV_DMX_H
#define _C_TV_DMX_H
#include "CTvEv.h"
#include "CTvLog.h"
#include "am_dmx.h"

class CTvDmx {
public:
    CTvDmx();
    ~CTvDmx();
    int Open(AM_DMX_OpenPara_t &para);
    int Close();
    int SetSource(AM_DMX_Source_t source);
private:
    int mDmxDevId;
};
#endif

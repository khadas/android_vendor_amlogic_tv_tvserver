/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_TV_DMX_H
#define _C_TV_DMX_H
#include "CTvEv.h"
#include "CTvLog.h"
#include "am_dmx.h"

class CTvDmx {
public:
    CTvDmx(int id);
    ~CTvDmx();
    int Open(AM_DMX_OpenPara_t &para);
    int Close();
    int SetSource(AM_DMX_Source_t source);
private:
    int mDmxDevId;
};
#endif

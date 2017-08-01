/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_FBC_LINKER_H_
#define _C_FBC_LINKER_H_

#include "CFbcHelper.h"
#include "CFbcProtocol.h"
#include "CFbcUpgrade.h"


class CFbcLinker {
public:
    CFbcLinker();
    ~CFbcLinker();

    int sendCommandToFBC(unsigned char *data, int count, int flag);
    int startFBCUpgrade(char *file_name, int mode, int blk_size);
    int setUpgradeFbcObserver(IUpgradeFBCObserver *pOb);

private:
    CFbcUpgrade *pUpgradeIns;
    CFbcProtocol *pProtocolIns;
};

extern CFbcLinker *getFbcLinkerInstance();

#endif

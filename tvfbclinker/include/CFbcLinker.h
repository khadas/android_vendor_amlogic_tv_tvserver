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

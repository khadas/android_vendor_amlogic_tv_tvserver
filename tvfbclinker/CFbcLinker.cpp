/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_NDEBUG 0

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CFbcLinker"

#include <CTvLog.h>
#include "CFbcLinker.h"


static CFbcLinker *mInstance;

CFbcLinker *getFbcLinkerInstance()
{
    if (mInstance == NULL) {
        mInstance = new CFbcLinker();
    }
    return mInstance;
}

CFbcLinker::CFbcLinker()
{
    pUpgradeIns = getFbcUpgradeInstance();
    pProtocolIns = GetFbcProtocolInstance();
}

CFbcLinker::~CFbcLinker()
{
    if (pUpgradeIns != NULL) {
        pUpgradeIns->stop();
        delete pUpgradeIns;
        pUpgradeIns = NULL;
    }
    if (pProtocolIns != NULL) {
        delete pProtocolIns;
        pProtocolIns = NULL;
    }
}

int CFbcLinker::sendCommandToFBC(unsigned char *data, int count, int flag)
{
    LOGV("%s, dev_type=%d, cmd_id=%d, flag=%d", __FUNCTION__, *data, *(data+1), flag);

    if (count < 2)
        return -1;

    int ret = -1;
    if (pProtocolIns != NULL) {
        COMM_DEV_TYPE_E toDev = (COMM_DEV_TYPE_E)*data;
        if (flag == 1) {//get something from fbc
            ret = pProtocolIns->fbcGetBatchValue(toDev, (unsigned char*)(data+1), count-1);
        } else {//set something to fbc
            ret = pProtocolIns->fbcSetBatchValue(toDev, (unsigned char*)(data+1), count-1);
        }
    }
    return ret;
}

int CFbcLinker::setUpgradeFbcObserver(IUpgradeFBCObserver *pOb)
{
    pUpgradeIns->setObserver(pOb);
    return 0;
}

int CFbcLinker::startFBCUpgrade(char *file_name, int mode, int blk_size)
{
    pUpgradeIns->SetUpgradeFileName(file_name);
    pUpgradeIns->SetUpgradeMode(mode);
    pUpgradeIns->SetUpgradeBlockSize(blk_size);
    pUpgradeIns->start();
    return 0;
}

int sendCommandToFBC(unsigned char *data, int count, int flag)
{
    CFbcLinker *pLinker = getFbcLinkerInstance();
    if (pLinker == NULL)
        return -1;

    if (CMD_FBC_RELEASE == (fbc_command_t)*(data+1) && pLinker != NULL) {
        delete pLinker;
        pLinker = NULL;
        return -1;
    }

    return pLinker->sendCommandToFBC(data, count, flag);
}

int startFBCUpgrade(char *file_name, int blk_size, int mode)
{
    CFbcLinker *pLinker = getFbcLinkerInstance();
    if (pLinker == NULL)
        return -1;
    return pLinker->startFBCUpgrade(file_name, mode, blk_size);
}

int setUpgradeFbcObserver(IUpgradeFBCObserver *pOb)
{
    CFbcLinker *pLinker = getFbcLinkerInstance();
    if (pLinker == NULL)
        return -1;
    return pLinker->setUpgradeFbcObserver(pOb);
}


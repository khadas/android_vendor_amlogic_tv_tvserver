/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CHdmiCec"

#include "CHdmiCecCmd.h"
CHdmiCec::CHdmiCec()
{
}

CHdmiCec::~CHdmiCec()
{
}

int CHdmiCec::readFile(unsigned char *pBuf __unused, unsigned int uLen __unused)
{
    int ret = 0;
    //ret = read(mFd, pBuf, uLen);
    return ret;
}


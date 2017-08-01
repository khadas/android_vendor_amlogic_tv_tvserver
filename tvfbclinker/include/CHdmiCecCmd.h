/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#include "CFile.h"

static  const char   *CEC_PATH = "/dev/aocec";
class CHdmiCec: public CFile {
public:
    CHdmiCec();
    ~CHdmiCec();
    int readFile(unsigned char *pBuf, unsigned int uLen);
};

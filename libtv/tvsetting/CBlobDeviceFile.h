/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _BLOB_FILE_H_
#define _BLOB_FILE_H_

#include "CBlobDevice.h"
class CBlobDeviceFile: public CBlobDevice {

public:
    CBlobDeviceFile();
    virtual ~CBlobDeviceFile();

    virtual int WriteBytes(int offset, int size, unsigned char *buf);
    virtual int ReadBytes(int offset, int size, unsigned char *buf);
    virtual int EraseAllData();
    virtual int InitCheck();
    virtual int OpenDevice();
    virtual int CloseDevice();

private:
    int ValidOperateCheck();
    int CreateNewFile(const char *file_name);
};

#endif // _BLOB_FILE_H_

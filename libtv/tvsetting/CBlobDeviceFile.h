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

private:

};

#endif // ANDROID_SSM_FILE_H

#ifndef BLOB_BASE_DEVICE_H
#define BLOB_BASE_DEVICE_H

class CBlobDevice {

public:
    static const int  CC_MAX_FILE_PATH = 256;
    static const int  CC_ERR_FILE_HANDLE = -1;
    static const int  CC_INIT_BYTE_VAL = 0xFF;
    static const int  CC_SSM_PROTECT_ON = 0;
    static const int  CC_SSM_PROTECT_OFF = 1;

    static const int CC_DEVICE_E2PROM    =     (0);
    static const int CC_DEVICE_SPI     =       (1);
    static const int CC_DEVICE_FILE    =     (2);
    static const int CC_DEVICE_RAM     =   (3);

    CBlobDevice();
    virtual ~CBlobDevice();

    virtual int WriteBytes(int offset, int size, unsigned char *buf) = 0;
    virtual int ReadBytes(int offset, int size, unsigned char *buf) = 0;
    virtual int EraseAllData() = 0;
    virtual int InitCheck() = 0;
    virtual int OpenDevice() = 0;
    virtual int CloseDevice() = 0;
    int WriteOneByte(int offset, unsigned char val)
    {
        return WriteBytes(offset, 1, &val);
    }
    int ReadOneByte(int offset, unsigned char *ptrVal)
    {
        return ReadBytes(offset, 1, ptrVal);
    }

    int m_dev_type; //device type
    int m_dev_total_size; //device total size(Byte)

    char m_dev_path[CC_MAX_FILE_PATH]; //device path
    int m_dev_fd;

protected:
    unsigned char *mDataBuf;

private:
    int IsFileExist(const char *file_name);

};

#endif // ANDROID_SSM_BASE_H

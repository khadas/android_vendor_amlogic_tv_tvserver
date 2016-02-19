#ifndef __TV_UPGRADE_FBC_H__
#define __TV_UPGRADE_FBC_H__

#include "CFbcCommunication.h"
#include "../tvutils/CThread.h"

#define CC_FBC_V01_00_VAL                             (0x10000000)
#define CC_FBC_V02_00_VAL                             (0x20000000)
#define CC_FBC_V02_01_VAL                             (0x20000001)
#define CC_FBC_V03_00_VAL                             (0x30000000)
#define CC_FBC_V03_01_VAL                             (0x30000001)

#define CC_FBC_V01_FILE_SIZE                          (655360)
#define CC_FBC_V02_FILE_SIZE                          (786432)
#define CC_FBC_V02_CUR_PQ_BIN_FILE_SIZE               (0xB000)
#define CC_FBC_V03_FILE_SIZE                          (2097152)
#define CC_FBC_V03_CUR_PQ_BIN_FILE_SIZE               (0xB000)

#define CC_UPGRADE_MAX_BLOCK_LEN                      (0x10000)
#define CC_UPGRADE_DATA_BUF_SIZE                      (CC_UPGRADE_MAX_BLOCK_LEN + 4)

#define CC_UPGRADE_MODE_BOOT_MAIN                     (0)
#define CC_UPGRADE_MODE_BOOT                          (1)
#define CC_UPGRADE_MODE_MAIN                          (2)
#define CC_UPGRADE_MODE_COMPACT_BOOT                  (3)
#define CC_UPGRADE_MODE_ALL                           (4)

#define CC_UPGRADE_MODE_MAIN_PQ_WB                    (5)
#define CC_UPGRADE_MODE_ALL_PQ_WB                     (6)
#define CC_UPGRADE_MODE_MAIN_WB                       (7)
#define CC_UPGRADE_MODE_ALL_WB                        (8)
#define CC_UPGRADE_MODE_MAIN_PQ                       (9)
#define CC_UPGRADE_MODE_ALL_PQ                        (10)

#define CC_UPGRADE_MODE_PQ_WB_ONLY                    (11)
#define CC_UPGRADE_MODE_WB_ONLY                       (12)
#define CC_UPGRADE_MODE_PQ_ONLY                       (13)

#define CC_UPGRADE_MODE_CUR_PQ_BIN                    (14)
#define CC_UPGRADE_MODE_ALL_PQ_BIN                    (15)

#define CC_UPGRADE_MODE_BURN                          (16)

#define CC_UPGRADE_MODE_DUMMY                         (17)

#define CC_UPGRADE_V01_BOOT_OFFSET                    (0x0)
#define CC_UPGRADE_V01_BOOT_LEN                       (0x20000)
#define CC_UPGRADE_V01_MAIN_OFFSET                    (0x20000)
#define CC_UPGRADE_V01_MAIN_LEN                       (CC_FBC_V01_FILE_SIZE - CC_UPGRADE_V01_MAIN_OFFSET)
#define CC_UPGRADE_V01_ALL_LENGTH                     (CC_FBC_V01_FILE_SIZE)

#define CC_UPGRADE_V02_COMPACT_BOOT_OFFSET            (0x0)
#define CC_UPGRADE_V02_COMPACT_BOOT_LEN               (0x10000)
#define CC_UPGRADE_V02_BOOT_OFFSET                    (0x10000)
#define CC_UPGRADE_V02_BOOT_LEN                       (0x30000)
#define CC_UPGRADE_V02_MAIN_OFFSET                    (0x40000)
#define CC_UPGRADE_V02_MAIN_LEN                       (0x80000)
#define CC_UPGRADE_V02_BOOT_BAK_OFFSET                (0xC0000)
#define CC_UPGRADE_V02_BOOT_BAK_LEN                   (0x30000)
#define CC_UPGRADE_V02_MAIN_BAK_OFFSET                (0xF0000)
#define CC_UPGRADE_V02_MAIN_BAK_LEN                   (0x80000)
#define CC_UPGRADE_V02_ALL_LENGTH                     (0x170000)

#define CC_UPGRADE_V02_CUR_PQ_OFFSET                  (0xAF000)

#define CC_UPGRADE_V03_COMPACT_BOOT_OFFSET            (CC_UPGRADE_V02_COMPACT_BOOT_OFFSET)
#define CC_UPGRADE_V03_COMPACT_BOOT_LEN               (CC_UPGRADE_V02_COMPACT_BOOT_LEN)
#define CC_UPGRADE_V03_BOOT_OFFSET                    (CC_UPGRADE_V02_BOOT_OFFSET)
#define CC_UPGRADE_V03_BOOT_LEN                       (CC_UPGRADE_V02_BOOT_LEN)
#define CC_UPGRADE_V03_MAIN_OFFSET                    (CC_UPGRADE_V02_MAIN_OFFSET)
#define CC_UPGRADE_V03_MAIN_LEN                       (CC_UPGRADE_V02_MAIN_LEN)
#define CC_UPGRADE_V03_BOOT_BAK_OFFSET                (CC_UPGRADE_V02_BOOT_BAK_OFFSET)
#define CC_UPGRADE_V03_BOOT_BAK_LEN                   (CC_UPGRADE_V02_BOOT_BAK_LEN)
#define CC_UPGRADE_V03_MAIN_BAK_OFFSET                (CC_UPGRADE_V02_MAIN_BAK_OFFSET)
#define CC_UPGRADE_V03_MAIN_BAK_LEN                   (CC_UPGRADE_V02_MAIN_BAK_LEN)
#define CC_UPGRADE_V03_PROTECT_DATA_START             (0x1FF000)
#define CC_UPGRADE_V03_PROTECT_DATA_LEN               (0x1000)
#define CC_UPGRADE_V03_ALL_LENGTH                     (CC_UPGRADE_V03_PROTECT_DATA_START)

#define CC_UPGRADE_V03_CUR_PQ_OFFSET                  (CC_UPGRADE_V02_CUR_PQ_OFFSET)


class CUpgradeFBC: public CThread {
public:
    CUpgradeFBC();
    ~CUpgradeFBC();

    int start();
    int stop();
    int GetUpgradeFBCProgress();
    int SetUpgradeFileName(char *file_name);
    int SetUpgradeFileSize(int file_size);
    int SetUpgradeBlockSize(int block_size);
    int SetUpgradeMode(int mode);

    class IUpgradeFBCObserver {
    public:
        IUpgradeFBCObserver() {};
        virtual ~IUpgradeFBCObserver() {};
        virtual void onUpgradeStatus(int state, int param) {
            state = state;
            param = param;
        };
    };
    void setObserver(IUpgradeFBCObserver *pOb)
    {
        mpObserver = pOb;
    };

private:
    bool threadLoop();

    int AddCRCToDataBuf(unsigned char data_buf[], int data_len);

    int mState;
    int mUpgradeMode;
    int mOPTotalSize;
    int mBinFileSize;
    int mUpgradeBlockSize;
    unsigned char *mBinFileBuf;
    char mFileName[256];
    unsigned char mDataBuf[CC_UPGRADE_DATA_BUF_SIZE];
    IUpgradeFBCObserver *mpObserver;
    CFbcCommunication *mCfbcIns;

    enum UpgradeState {
        STATE_STOPED = 0,
        STATE_RUNNING,
        STATE_FINISHED,
        STATE_ABORT,
    };

    enum FBCUpgradeErrorCode {
        ERR_SERIAL_CONNECT = -1,
        ERR_OPEN_BIN_FILE = -2,
        ERR_BIN_FILE_SIZE = -3,
        ERR_READ_BIN_FILE = -4,
        ERR_NOT_SUPPORT_UPGRADE_MDOE = -5,
        ERR_NOT_CORRECT_UPGRADE_BLKSIZE = -6,
        ERR_DATA_CRC_ERROR = -7,
    };
};

#endif  //__TV_UPGRADE_FBC_H__

#ifndef BLOB_E2PROM_H
#define BLOB_E2PROM_H

#include "CBlobDevice.h"

#define I2C_RETRIES             0x0701  /* number of times a device address
                                        should be polled when not
                                        acknowledging */
#define I2C_TIMEOUT             0x0702  /* set timeout in units of 10 ms */

/* NOTE: Slave address is 7 or 10 bits, but 10-bit addresses
 * are NOT supported! (due to code brokenness)
 */
#define I2C_SLAVE               0x0703  /* Use this slave address */
#define I2C_SLAVE_FORCE         0x0706  /* Use this slave address, even if it
                                    is already in use by a driver! */
#define I2C_TENBIT              0x0704  /* 0 for 7 bit addrs, != 0 for 10 bit */
#define I2C_FUNCS               0x0705  /* Get the adapter functionality mask */
#define I2C_RDWR                0x0707  /* Combined R/W transfer (one STOP only) */
#define I2C_PEC                 0x0708  /* != 0 to use PEC with SMBus */
#define I2C_SMBUS               0x0720  /* SMBus transfer */

struct i2c_msg {
    unsigned short addr; /* slave address */
    unsigned short flags;
#define I2C_M_TEN           0x0010  /* this is a ten bit chip address */
#define I2C_M_WR            0x0000  /* write data, from master to slave */
#define I2C_M_RD            0x0001  /* read data, from slave to master */
#define I2C_M_NOSTART       0x4000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR  0x2000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK    0x1000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK     0x0800  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN      0x0400  /* length will be first received byte */

    unsigned short len; /* msg length */
    unsigned char *buf; /* pointer to msg data */
};

struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    unsigned int nmsgs;
};

// 24C64 eeprom
#define E2P_MSG_BUF_SIZE        (34)

class CBlobDeviceE2prom: public CBlobDevice {

public:
    CBlobDeviceE2prom();
    virtual ~CBlobDeviceE2prom();

    virtual int WriteBytes(int offset, int size, unsigned char *buf);
    virtual int ReadBytes(int offset, int size, unsigned char *buf);
    virtual int EraseAllData();
    virtual int InitCheck();
    virtual int OpenDevice();
    virtual int CloseDevice();

private:

private:
    /*int E2P_DEV_TOTAL_SIZE;
    int E2P_DEV_RW_START_OFFSET;
    int E2P_DEV_RW_END_OFFSET;
    int E2P_DEV_W_PAGE_SIZE;
    int E2P_DEV_R_PAGE_SIZE;
    int E2P_DEV_SLAVE_ADDR;
    int E2P_DEV_RW_TEST_OFFSET;
    int device_use_buffer;
    unsigned char *device_buf;
    char gFilePathBuf[CC_MAX_FILE_PATH];*/
};


#endif

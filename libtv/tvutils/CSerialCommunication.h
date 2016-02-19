#ifndef __TV_SERIAL_COMMUNICATION_H__
#define __TV_SERIAL_COMMUNICATION_H__

#include "CThread.h"
#include "CSerialPort.h"
#include "../tv/CTvEv.h"

class CSerialCommunication: public CThread {
public:
    CSerialCommunication();
    ~CSerialCommunication();

    int start();
    int stop();
    int OpenModule(int serial_dev_id);
    int CloseModule();
    int sendData(int data_len, unsigned char data_buf[]);

    class ISerialCommunicationObserver {
    public:
        ISerialCommunicationObserver() {};
        virtual ~ISerialCommunicationObserver() {};
        virtual void onSerialCommunication(int dev_id __unused,
                                            int rd_len __unused,
                                            unsigned char data_buf[] __unused) {
        };
    };

    void setObserver(ISerialCommunicationObserver *pOb) {
        mpObserver = pOb;
    };

private:
    bool threadLoop();

    int mState;
    ISerialCommunicationObserver *mpObserver;
    CSerialPort mSerialPort;

    char mFileName[CC_MAX_FILE_PATH_LEN];
    unsigned char mDataBuf[CC_MAX_SERIAL_RD_BUF_LEN];

    enum SerialCommunicationState {
        STATE_STOPED = 0,
        STATE_RUNNING,
    };
};

#endif  //__TV_SERIAL_COMMUNICATION_H__
